/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// testsuite/unittests/u0013_fsobj.c
// test file io operations
//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "unittests.h"

//////////////////////////////////////////////////////////////////

void u0013_fsobj__create_pathname(SG_context * pCtx,
								  const char * szFilenameBase, const char * szFilenameSuffix,
								  SG_pathname ** ppPathname)
{
	SG_pathname * pPathnameNew = NULL;
	SG_string * pStrTemp = NULL;

	if (szFilenameBase && szFilenameSuffix)
	{
		VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx,
													&pStrTemp,
													"%s.%s",
													szFilenameBase,szFilenameSuffix)  );
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,
												  &pPathnameNew,
												  SG_string__sz(pStrTemp))  );

		SG_STRING_NULLFREE(pCtx, pStrTemp);
	}
	else
	{
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,
												  &pPathnameNew,
												  szFilenameBase)  );
	}

	*ppPathname = pPathnameNew;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pStrTemp);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameNew);
}

int u0013_fsobj__get_nonexistent_pathname(SG_context * pCtx,
										  const char * szOptionalSuffix,
										  SG_pathname ** ppPathname)
{
	SG_pathname * pPathnameNew = NULL;
	SG_bool b;
	char bufTid[SG_TID_MAX_BUFFER_LENGTH];

	while (1)
	{
		VERIFY_ERR_CHECK(  SG_tid__generate(pCtx, bufTid, sizeof(bufTid))  );

		VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__create_pathname(pCtx,bufTid,szOptionalSuffix,&pPathnameNew)  );

		VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathnameNew, &b, NULL, NULL)  );
		if (!b)
		{
			*ppPathname = pPathnameNew;
			return 1;
		}

		SG_PATHNAME_NULLFREE(pCtx, pPathnameNew);
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameNew);
	return 0;
}

//////////////////////////////////////////////////////////////////

int u0013_fsobj__test_mkdir(SG_context * pCtx)
{
	SG_pathname * pPathname = NULL;
	SG_bool bResult;
	SG_fsobj_type ft;

	VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );

	// create a new directory and verify that it is actually on disk

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__mkdir__pathname(pCtx, pPathname)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx, pPathname,&bResult,&ft,NULL)  );
	VERIFY_COND("test_mkdir:exists",(bResult));
	VERIFY_COND("test_mkdir:exists",(ft == SG_FSOBJ_TYPE__DIRECTORY));

	// try to create it again and make sure we get already-exists error

	SG_fsobj__mkdir__pathname(pCtx,pPathname);
	VERIFY_CTX_ERR_EQUALS("test_mkdir:duplicate-mkdir",pCtx,SG_ERR_DIR_ALREADY_EXISTS);
	SG_context__err_reset(pCtx);

	// now delete it and verify

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__rmdir__pathname(pCtx, pPathname)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx, pPathname,&bResult,&ft,NULL)  );
	VERIFY_COND("test_mkdir:exists",(!bResult));

	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	return 1;
}

int u0013_fsobj__test_bogus_mkdir(SG_context * pCtx)
{
	SG_pathname * pPathname = NULL;

	// create a pathname to a directory that does not exist on disk.
	// then append a bunch of sub-dirs so that we have a truly bogus
	// pathname.
	//
	// <tmp>/<gid>.u0013/a/b/c/d/

	VERIFY_ERR_CHECK(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPathname,"a")  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPathname,"b")  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPathname,"c")  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPathname,"d")  );

	// try to create the bottom directory without creating the intermediate directories first.
	// this should fail.

	SG_fsobj__mkdir__pathname(pCtx, pPathname);
	VERIFY_CTX_ERR_EQUALS("test_mkdir:bogus-mkdir",pCtx,SG_ERR_DIR_PATH_NOT_FOUND);
	SG_context__err_reset(pCtx);

	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	return 1;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	return 0;
}

int u0013_fsobj__test_getcwd(SG_context * pCtx)
{
	SG_string * pString = NULL;

	VERIFY_ERR_CHECK_RETURN(  SG_STRING__ALLOC(pCtx, &pString)  );

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__getcwd(pCtx, pString)  );
	INFOP("test_getcwd", ("cwd is [%s]",SG_string__sz(pString)));

	// TODO we can't really verify the value in the string because we
	// TODO don't know where we are -- unless we did a chdir() first....

	SG_STRING_NULLFREE(pCtx, pString);
	return 1;
}

int u0013_fsobj__test_create_perms(SG_context * pCtx)
{
	// test creating files with various mode bits (0777, ...)

	SG_fsobj_perms aPerms[] = { 0777, 0666, 0555, 0444, 0333, 0222, 0111, 0000, 0700 };
	SG_fsobj_perms permsExpected;
	SG_file* pf;
	SG_fsobj_type fsobjType;
	SG_fsobj_perms fsobjPerms;
	size_t k;
	SG_bool bExists;

#if defined(MAC) || defined(LINUX)
	mode_t my_umask = umask(0);
	umask(my_umask);
#endif

	for (k=0; k<SG_NrElements(aPerms); k++)
	{
		SG_pathname * pPathname = NULL;

#if defined(MAC) || defined(LINUX)
		permsExpected = aPerms[k] & ~my_umask;
#endif
#if defined(WINDOWS)
		// windows only has a single READONLY bit.  mode bits are a bit of a hack.
		// and it only stores user-info.  the group/other are copied from user.
		SG_fsobj_perms u = (aPerms[k] & 0200) | 0400;
		permsExpected = u | (u >> 3) | (u >> 6);
#endif

		VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );

		SG_file__open__pathname(pCtx, pPathname, SG_FILE_CREATE_NEW | SG_FILE_RDWR, aPerms[k], &pf);
		VERIFYP_CTX_IS_OK("test_create_perms", pCtx, ("__open with perms[%04o]",aPerms[k]));
		if (!SG_context__has_err(pCtx))
		{
			VERIFY_ERR_CHECK_DISCARD(  SG_file__close(pCtx,&pf)  );
			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx,pPathname,&bExists,&fsobjType,&fsobjPerms)  );
			VERIFY_COND("test_create_perms",(bExists));
			VERIFY_COND("test_create_perms",(fsobjType == SG_FSOBJ_TYPE__REGULAR));

			//INFOP("test_create_perms",("Requested perms[%04o] received perms[%04o] expected perms[%04o]",aPerms[k],fsobjPerms,permsExpected));
			VERIFY_COND("test_create_perms",(fsobjPerms == permsExpected));

			// windows cannot delete a file that has READONLY set.
			// so we need to turn it off with a chmod before trying to delete it.

			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__chmod__pathname(pCtx,pPathname,0777)  );
			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__remove__pathname(pCtx,pPathname)  );
		}

		SG_PATHNAME_NULLFREE(pCtx, pPathname);
	}

	return 1;
}

int u0013_fsobj__test_chmod(SG_context * pCtx)
{
	SG_fsobj_perms aPerms[] = { 0777, 0666, 0555, 0444, 0333, 0222, 0111, 0000, 0700 };
	SG_fsobj_perms permsExpected, permsChmodExpected;
	SG_file* pf;
	SG_fsobj_type fsobjType;
	SG_fsobj_perms fsobjPerms, fsobjPermsChmod;
	size_t k, j;
	SG_bool bExists;

#if defined(MAC) || defined(LINUX)
	mode_t my_umask = umask(0);
	umask(my_umask);
#endif
#if defined(WINDOWS)
	SG_fsobj_perms u, uChmod;
#endif

	for (k=0; k<SG_NrElements(aPerms); k++)
	{
		SG_pathname * pPathname = NULL;

#if defined(MAC) || defined(LINUX)
		permsExpected = aPerms[k] & ~my_umask;
#endif
#if defined(WINDOWS)
		// windows only has a single READONLY bit.  mode bits are a bit of a hack.
		// and it only stores user-info.  the group/other are copied from user.
		u = (aPerms[k] & 0200) | 0400;
		permsExpected = u | (u >> 3) | (u >> 6);
#endif

		// create a new file with initial permissions set to each of the values in aPerms[].

		VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );

		SG_file__open__pathname(pCtx, pPathname, SG_FILE_CREATE_NEW | SG_FILE_RDWR, aPerms[k], &pf);
		VERIFYP_CTX_IS_OK("test_chmod", pCtx, ("__open with perms[%04o]",aPerms[k]));
		if (!SG_context__has_err(pCtx))
		{
			VERIFY_ERR_CHECK_DISCARD(  SG_file__close(pCtx,&pf)  );
			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx,pPathname,&bExists,&fsobjType,&fsobjPerms)  );
			VERIFY_COND("test_chmod",(bExists));
			VERIFY_COND("test_chmod",(fsobjType == SG_FSOBJ_TYPE__REGULAR));

			// we do not test for equivalent perms here because of umask effects during creation.

			//INFOP("test_chmod",("On Create requested perms[%04o] received perms[%04o] expected perms[%04o]",aPerms[k],fsobjPerms,permsExpected));
			VERIFY_COND("test_chmod",(fsobjPerms == permsExpected));

			for (j=0; j<SG_NrElements(aPerms); j++)
			{
				// now do a chmod on the file for each of the values in aPerms[] and verify we got what we expected.

				SG_fsobj__chmod__pathname(pCtx,pPathname,aPerms[j]);
				VERIFYP_CTX_IS_OK("test_chmod", pCtx, ("__chmod with perms[%04o]",aPerms[k]));
				SG_context__err_reset(pCtx);

				VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx,pPathname,&bExists,&fsobjType,&fsobjPermsChmod)  );
				VERIFY_COND("test_chmod",(bExists));
				VERIFY_COND("test_chmod",(fsobjType == SG_FSOBJ_TYPE__REGULAR));

				// see if what we got is equivalent to what we asked for.  (we can do this
				// here because we don't have to worry about umask.)
				VERIFY_COND("test_chmod",(SG_fsobj__equivalent_perms(aPerms[j],fsobjPermsChmod)));

#if defined(MAC) || defined(LINUX)
				permsChmodExpected = aPerms[j];
#endif
#if defined(WINDOWS)
				// windows only has a single READONLY bit.  mode bits are a bit of a hack.
				// and it only stores user-info.  the group/other are copied from user.
				uChmod = (aPerms[j] & 0200) | 0400;
				permsChmodExpected = uChmod | (uChmod >> 3) | (uChmod >> 6);
#endif

				//INFOP("test_chmod",("Chmod Requested perms[%04o] received perms[%04o] expected perms[%04o]",aPerms[j],fsobjPermsChmod,permsChmodExpected));
				VERIFY_COND("test_chmod",(fsobjPermsChmod == permsChmodExpected));
			}

			// windows cannot delete a file that has READONLY set.
			// so we need to turn it off with a chmod before trying to delete it.
			// so we make sure that 0777 is the last entry in aPerms[].

			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__remove__pathname(pCtx,pPathname)  );
		}

		SG_PATHNAME_NULLFREE(pCtx, pPathname);
	}

	return 1;
}

int u0013_fsobj__test_cd(SG_context * pCtx)
{
	SG_string * pString = NULL;
	SG_pathname * pPathname = NULL;
#if defined(WINDOWS)
	SG_pathname * pPath2 = NULL;
#endif
	SG_pathname * pPathnameOrig = NULL;  //The first path.


	SG_file* pf; //move this back out once jeremy figures out the Windows failure
#if defined(MAC) || defined(LINUX)
	int i = 0;
	const char* bytes = NULL;
#endif


	VERIFY_ERR_CHECK_RETURN(  SG_STRING__ALLOC(pCtx, &pString)  );

	VERIFY_ERR_CHECK_RETURN(  SG_PATHNAME__ALLOC(pCtx, &pPathnameOrig)  );

	VERIFY_ERR_CHECK_RETURN(  SG_pathname__set__from_cwd(pCtx, pPathnameOrig)   );

	VERIFY_ERR_CHECK_DISCARD(  SG_pathname__add_final_slash(pCtx, pPathnameOrig)   );

	VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );

	VERIFY_ERR_CHECK_RETURN(  SG_fsobj__mkdir__pathname(pCtx, pPathname)  );

	VERIFY_ERR_CHECK_RETURN(   SG_fsobj__cd__pathname(pCtx, pPathname)   );

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__getcwd(pCtx, pString)   );

	VERIFY_ERR_CHECK_DISCARD(  SG_pathname__add_final_slash(pCtx, pPathname)   );

#if defined(WINDOWS)
	//getcwd always returns //?/ at the front of the path.
	SG_PATHNAME__ALLOC__SZ(pCtx, &pPath2, SG_string__sz(pString));
	SG_string__set__sz(pCtx, pString, SG_pathname__sz(pPath2));
#endif
	VERIFY_COND("test_cd", (strcmp(SG_string__sz(pString),SG_pathname__sz(pPathname)) == 0));

	//Go back up to the original directory

	VERIFY_ERR_CHECK_RETURN(   SG_fsobj__cd__pathname(pCtx, pPathnameOrig)   );

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__getcwd(pCtx, pString)   );

#if defined(WINDOWS)
	//getcwd always returns //?/ at the front of the path.
	SG_pathname__set__from_sz(pCtx, pPath2, SG_string__sz(pString));
	SG_string__set__sz(pCtx, pString, SG_pathname__sz(pPath2));
	SG_PATHNAME_NULLFREE(pCtx, pPath2);
#endif

	VERIFY_COND("test_cd", (strcmp(SG_string__sz(pString),SG_pathname__sz(pPathnameOrig)) == 0));


	//Expected Failure cases:

	//Case 1.  Try to cd to a spot that isn't there
	SG_pathname__set__from_sz(pCtx, pPathname, "u0013_gobbledygook");

#if defined(MAC) || defined(LINUX)
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_fsobj__cd__pathname(pCtx, pPathname), SG_ERR_ERRNO(ENOENT)  );
#elif defined(WINDOWS)
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_fsobj__cd__pathname(pCtx, pPathname), SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND)  );
#endif
	//Case 2.  Try to cd into a file.
	//Get a new path for the file.

	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_file__open__pathname(pCtx, pPathname, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0777, &pf)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_file__close(pCtx, &pf)  );
#if defined(MAC) || defined(LINUX)
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_fsobj__cd__pathname(pCtx, pPathname), SG_ERR_ERRNO(ENOTDIR)  );
#elif defined(WINDOWS)
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_fsobj__cd__pathname(pCtx, pPathname), SG_ERR_GETLASTERROR(ERROR_DIRECTORY)  );
#endif

#if defined(MAC) || defined(LINUX)
	//Case 3.  Try to cd into a directory that you do not have execute permissions to.
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );
	VERIFY_ERR_CHECK_RETURN(  SG_fsobj__mkdir__pathname(pCtx, pPathname)  );
	VERIFY_ERR_CHECK_RETURN(  SG_fsobj__chmod__pathname(pCtx, pPathname, (SG_fsobj_perms)0666)  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_fsobj__cd__pathname(pCtx, pPathname), SG_ERR_ERRNO(EACCES)  );

	//Case 4.  A really long directory name
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_STRING_NULLFREE(pCtx, pString);
	VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathname)  );
	bytes = SG_pathname__sz(pPathname);
	SG_STRING__ALLOC__SZ(pCtx, &pString, bytes);
	for (i=0; i < 255; i++)
	{
		SG_string__append__sz(pCtx, pString, "a");
	}
	SG_pathname__set__from_sz(pCtx, pPathname, SG_string__sz(pString));

//	VERIFY_ERR_CHECK_RETURN(  SG_fsobj__mkdir__pathname(pCtx, pPathname)  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_fsobj__cd__pathname(pCtx, pPathname), SG_ERR_ERRNO(ENAMETOOLONG)  );

#endif

	SG_STRING_NULLFREE(pCtx, pString);
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameOrig);
	return 1;
}

int u0013_fsobj__test_utf8paths(SG_context * pCtx)
{
	// create a file with funky unicode chars in filename.

	SG_file* pf;
	SG_bool bExists;
	SG_fsobj_type fsobjType;
	SG_fsobj_perms fsobjPerms;
	SG_pathname * pPathname = NULL;

	unsigned char bufUtf8Suffix[] = { 'u', '0', '0', '1', '3', '_',
									  0x41,
									  0xc3, 0x80,
									  0x41,
									  0xc3, 0x86,
									  0x41,
									  0xc4, 0x80,
									  0x41,
									  0xc7, 0x9c,
									  0x41,
									  0xce, 0x94,
									  0x41,
									  0xd0, 0x90,
									  0x41,
									  0xe2, 0x88, 0xab,
									  0x41,
									  0x00
	};

	VERIFY_ERR_CHECK_RETURN(  u0013_fsobj__get_nonexistent_pathname(pCtx, (const char *)bufUtf8Suffix, &pPathname)  );

	// create should succeed because the file does not exist. (unless there are problems with funky utf8 chars.)
	VERIFY_ERR_CHECK_DISCARD(  SG_file__open__pathname(pCtx, pPathname, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0777, &pf)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_file__close(pCtx, &pf)  );

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx, pPathname,&bExists,&fsobjType,&fsobjPerms)  );
	VERIFY_COND("test_utf8paths",(bExists));
	VERIFY_COND("test_utf8paths",(fsobjType == SG_FSOBJ_TYPE__REGULAR));

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__remove__pathname(pCtx, pPathname)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathname);

	return 1;
}

int u0013_fsobj__test_mkdir_recursive(SG_context * pCtx, const char * szTemp, int kLimit)
{
	SG_pathname * pPathname = NULL;
	SG_bool bResult;
	SG_fsobj_type ft;
	int k;

	// get n ID's and create "<tmp>/<id1>/<id2>/<id3>/..."
	// as a absolute pathname.

	VERIFY_ERR_CHECK_RETURN(  SG_PATHNAME__ALLOC(pCtx, &pPathname)  );

	VERIFY_ERR_CHECK(  SG_pathname__set__from_sz(pCtx,pPathname,szTemp)  );

	// warning: this will create a very long pathname that may exceed MAX_PATH.

	for (k=0; k<kLimit; k++)
	{
		char bufTid[SG_TID_MAX_BUFFER_LENGTH];

		VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTid, sizeof(bufTid), 32)  );

		SG_pathname__append__from_sz(pCtx,pPathname,bufTid);
		VERIFYP_CTX_IS_OK("mkdir_r", pCtx, ("append buf[%s] onto[%s]",bufTid,SG_pathname__sz(pPathname)));
	}

	// create a directory where intermediate directories don't exist and
	// verify that it was created.

	INFOP("mkdir_r",("Calling mkdir_r with [%s] len[%d]",SG_pathname__sz(pPathname),SG_STRLEN(SG_pathname__sz(pPathname))));

	SG_fsobj__mkdir_recursive__pathname(pCtx,pPathname);
	VERIFYP_CTX_IS_OK("mkdir_r", pCtx, ("mkdir_r[%s]",SG_pathname__sz(pPathname)));

	if (!SG_context__has_err(pCtx))	// if we failed to create complete path, don't bother calling exist() on it.
	{
		VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx,pPathname,&bResult,&ft,NULL)  );
		VERIFY_COND("mkdir_r:exists",(bResult));
		VERIFY_COND("mkdir_r:exists",(ft == SG_FSOBJ_TYPE__DIRECTORY));
	}
	else
	{
		SG_context__err_reset(pCtx);
	}

	// try creating it again and verify that we get the new already-exists error.

	SG_fsobj__mkdir_recursive__pathname(pCtx,pPathname);
	VERIFY_CTX_ERR_EQUALS("mkdir_r:duplicate-mkdir",pCtx,SG_ERR_DIR_ALREADY_EXISTS);
	SG_context__err_reset(pCtx);

	// now cleanup, delete the sub-dirs in reverse order <idN> ... <id1>
	// we don't use the 'rmdir -r' version (because we don't have it yet).

	for (k=kLimit; k>0; k--)
	{
		// since recursive mkdir could have created some of the subdirs but
		// not the entire sequence, see if partial <id1>...<idK> exists before
		// we try to delete it.
		SG_fsobj__exists__pathname(pCtx,pPathname,&bResult,&ft,NULL);
		if (!SG_context__has_err(pCtx) && bResult)
		{
			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__rmdir__pathname(pCtx, pPathname)  );		// rmdir <idK>
			VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx,pPathname,&bResult,&ft,NULL)  );		// verify it is gone
			VERIFY_COND("mkdir_r:exists",(!bResult));
		}

		VERIFY_ERR_CHECK_DISCARD(  SG_pathname__remove_last(pCtx,pPathname)  );		// pop "/<idK>" off end of pathname
	}

	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	return 1;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	return 0;
}

int u0013_fsobj__test_mkdir_rmdir_recursive(SG_context * pCtx, const char * szTemp, int kLimit)
{
	SG_pathname * pPathnameMyTop = NULL;
	SG_pathname * pPathnameWork = NULL;
	SG_pathname * pPathnameFile = NULL;
	SG_bool bResult;
	SG_fsobj_type ft;
	SG_file * pFile = NULL;
	int k;
	char bufTid_1[SG_TID_MAX_BUFFER_LENGTH];

	// we don't own <tmp> -- it's probably something like "/tmp".
	// create a top-level directory in <tmp> for our testing.
	// this will be "<tmp>/<id1>"

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathnameMyTop,szTemp)  );
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTid_1, sizeof(bufTid_1), 32)  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathnameMyTop, bufTid_1)  );

	// get n-1 ID's and create "<tmp>/<id1>/<id2>/<id3>/..."
	// as a absolute pathname.
	//
	// warning: this will create a very long pathname that may exceed MAX_PATH.

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPathnameWork,pPathnameMyTop)  );

	for (k=1; k<kLimit; k++)
	{
		char bufTid_k[SG_TID_MAX_BUFFER_LENGTH];

		VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTid_k, sizeof(bufTid_k), 32)  );

		VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathnameWork, bufTid_k)  );
	}

	// create a directory where intermediate directories don't exist and
	// verify that it was created.

	INFOP("mkdir_rmdir_r",("Calling mkdir_r with [%s] len[%d]",
						   SG_pathname__sz(pPathnameWork),SG_STRLEN(SG_pathname__sz(pPathnameWork))));

	SG_fsobj__mkdir_recursive__pathname(pCtx, pPathnameWork);
	VERIFYP_CTX_IS_OK("mkdir_rmdir_r", pCtx, ("mkdir_r[%s]",SG_pathname__sz(pPathnameWork)));

	if (!SG_context__has_err(pCtx))	// if we failed to create complete path, don't bother calling exist() on it.
	{
		VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx,pPathnameWork,&bResult,&ft,NULL)  );
		VERIFY_COND("mkdir_r:exists",(bResult));
		VERIFY_COND("mkdir_r:exists",(ft == SG_FSOBJ_TYPE__DIRECTORY));
	}
	else
	{
		SG_context__err_reset(pCtx);
	}

	// create a plain file in the deepest directory

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathnameFile,pPathnameWork,"foo.c")  );
	INFOP("mkdir_rmdir_r",("Creating file [%s]",SG_pathname__sz(pPathnameFile)));
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx,pPathnameFile,SG_FILE_CREATE_NEW|SG_FILE_RDWR,0777,&pFile)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameFile);

	// now use the new "rmdir -r" to delete everything that we created.

	VERIFY_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathnameMyTop)  );

	// now verify that it and everything under it is gone.

	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__exists__pathname(pCtx, pPathnameMyTop,&bResult,&ft,NULL)  );		// verify it is gone
	VERIFY_COND("rmdir_r:exists",(!bResult));

	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameMyTop);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameWork);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameFile);
	return 1;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameMyTop);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameWork);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameFile);
	return 0;
}

void u0013_fsobj__test_rename_file_within_a_directory(SG_context * pCtx)
{
	SG_pathname * pPathnameTop = NULL;
	SG_pathname * pPathname_dir_1 = NULL;
	SG_pathname * pPathname_file_1 = NULL;
	SG_pathname * pPathname_file_2 = NULL;
	SG_file * pf = NULL;
	SG_bool bResult;

	// <tmp>/<gid>.u0013/
	// <tmp>/<gid>.u0013/dir_1/
	// <tmp>/<gid>.u0013/dir_1/file_1
	// <tmp>/<gid>.u0013/dir_1/file_2

	VERIFY_ERR_CHECK(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathnameTop)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_dir_1,pPathnameTop,"dir_1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_file_1,pPathname_dir_1,"file_1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_file_2,pPathname_dir_1,"file_2")  );

	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathnameTop)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathname_dir_1)  );

	// create
	// <tmp>/<gid>.u0013/dir_1/file_1

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname_file_1, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0777, &pf)  );
	SG_FILE_NULLCLOSE(pCtx, pf);
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname_file_1,&bResult,NULL,NULL)  );
	VERIFY_COND("rename_file_within_a_directory:file_1",(bResult));

	INFOP("test_rename_file_within_a_directory",("file_1: %s",SG_pathname__sz(pPathname_file_1)));
	INFOP("test_rename_file_within_a_directory",("file_2: %s",SG_pathname__sz(pPathname_file_2)));

	// move it to
	// <tmp>/<gid>.u0013/dir_1/file_2

	VERIFY_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx,pPathname_file_1,pPathname_file_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname_file_1,&bResult,NULL,NULL)  );
	VERIFY_COND("rename_within_a_directory:file_1 should not exist",!(bResult));
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname_file_2,&bResult,NULL,NULL)  );
	VERIFY_COND("rename_within_a_directory:file_2",(bResult));

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameTop);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_dir_1);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_file_1);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_file_2);
	SG_FILE_NULLCLOSE(pCtx, pf);
}

void u0013_fsobj__test_move_file_between_directories(SG_context * pCtx)
{
	SG_pathname * pPathnameTop = NULL;
	SG_pathname * pPathname_dir_1 = NULL;
	SG_pathname * pPathname_dir_2 = NULL;
	SG_pathname * pPathname_file_1 = NULL;
	SG_pathname * pPathname_file_2 = NULL;
	SG_file * pf = NULL;
	SG_bool bResult;

	// <tmp>/<gid>.u0013/
	// <tmp>/<gid>.u0013/dir_1/
	// <tmp>/<gid>.u0013/dir_1/file_1
	// <tmp>/<gid>.u0013/dir_2/
	// <tmp>/<gid>.u0013/dir_2/file_2

	VERIFY_ERR_CHECK(  u0013_fsobj__get_nonexistent_pathname(pCtx, "u0013", &pPathnameTop)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_dir_1,pPathnameTop,"dir_1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_file_1,pPathname_dir_1,"file_1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_dir_2,pPathnameTop,"dir_2")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPathname_file_2,pPathname_dir_2,"file_2")  );

	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathnameTop)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathname_dir_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathname_dir_2)  );

	// create
	// <tmp>/<gid>.u0013/dir_1/file_1

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname_file_1, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0777, &pf)  );
	SG_FILE_NULLCLOSE(pCtx, pf);
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname_file_1,&bResult,NULL,NULL)  );
	VERIFY_COND("rename_file_within_a_directory:file_1",(bResult));

	INFOP("test_move_file_between_directories",("file_1: %s",SG_pathname__sz(pPathname_file_1)));
	INFOP("test_move_file_between_directories",("file_2: %s",SG_pathname__sz(pPathname_file_2)));

	// move it to
	// <tmp>/<gid>.u0013/dir_2/file_2

	VERIFY_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx,pPathname_file_1,pPathname_file_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname_file_1,&bResult,NULL,NULL)  );
	VERIFY_COND("test_move_file_between_directories:file_1 should not exist",!(bResult));
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname_file_2,&bResult,NULL,NULL)  );
	VERIFY_COND("test_move_file_between_directories:file_2",(bResult));

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameTop);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_dir_1);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_file_1);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_dir_2);
	SG_PATHNAME_NULLFREE(pCtx, pPathname_file_2);
	SG_FILE_NULLCLOSE(pCtx, pf);
}


/**
 * Information about a single test file system object.
 */
typedef struct _test_fsobj
{
	SG_fsobj_type eType;     //< The type of the object.
	const char*   szPath;    //< Path to the object.
	const char*   szContent; //< Content string to write to the object, or NULL for none.
}
_test_fsobj;

/**
 * A directory containing no objects.
 */
static const _test_fsobj gaEmptyDir[] = 
{
	{ SG_FSOBJ_TYPE__UNSPECIFIED, NULL, NULL },
};

/**
 * A simple directory with one object of each type.
 */
static const _test_fsobj gaSimpleDir[] = 
{
	{ SG_FSOBJ_TYPE__REGULAR,   "file",   NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder", NULL },
	{ SG_FSOBJ_TYPE__SYMLINK,   "link",   "file" },
};

/**
 * A directory with a bunch of nested subdirectories.
 */
static const _test_fsobj gaNestedDir[] = 
{
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderA", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderB", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderC", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderA", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderB", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderC", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderA", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderB", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderC", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderA/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderA/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderA/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderB/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderB/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderB/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderC/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderC/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderC/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderA/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderA/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderA/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderB/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderB/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderB/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderC/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderC/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderC/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderA/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderA/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderA/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderB/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderB/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderB/folderZ", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderC/folderX", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderC/folderY", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderC/folderZ", NULL },
};

/**
 * A directory with a number of nested files.
 */
static const _test_fsobj gaNestedFilesDir[] = 
{
	{ SG_FSOBJ_TYPE__REGULAR,   "file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "file3", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/file3", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderA", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderB", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder1/folderC", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderA", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderB", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder2/folderC", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderA", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderB", NULL },
	{ SG_FSOBJ_TYPE__DIRECTORY, "folder3/folderC", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderA/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderA/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderA/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderB/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderB/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderB/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderC/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderC/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder1/folderC/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderA/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderA/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderA/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderB/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderB/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderB/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderC/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderC/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder2/folderC/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderA/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderA/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderA/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderB/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderB/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderB/file3", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderC/file1", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderC/file2", NULL },
	{ SG_FSOBJ_TYPE__REGULAR,   "folder3/folderC/file3", NULL },
};

/**
 * A test directory with a bunch of objects in it.
 */
typedef struct _test_dir
{
	const char*        szName;
	const _test_fsobj* pObjects;
	SG_uint32          uCount;
}
_test_dir;

/**
 * Helper to statically declare a _test_dir instance that's
 * based on an array of _test_fsobj instances.
 */
#define TEST_DIR(ObjArray) { #ObjArray, ObjArray, SG_NrElements(ObjArray) },

/**
 * The set of defined test directories.
 */
static const _test_dir gaTestDirs[] = 
{
	TEST_DIR(gaEmptyDir)
	TEST_DIR(gaSimpleDir)
	TEST_DIR(gaNestedDir)
	TEST_DIR(gaNestedFilesDir)
};

#undef TEST_DIR

/**
 * Generates a temporary test path.
 */
static void u0013_fsobj__get_test_path(
	SG_context*   pCtx,  //< [in] [out] Error and context info.
	SG_pathname** ppPath //< [out] The generated temporary test path.
	)
{
	SG_pathname* pPath = NULL;
	char*        szTid = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPath)  );
	VERIFY_ERR_CHECK(  SG_tid__alloc(pCtx, &szTid)  );
	VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, szTid)  );

	*ppPath = pPath;
	pPath = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, szTid);
}

/**
 * Creates a test directory on the file system.
 */
static void u0013_fsobj__test_dir__create(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	const SG_pathname* pRoot, //< [in] Pathname of a folder to create the test directory in.
	                          //<      Will be created if it doesn't exist.
	const _test_dir*   pDir   //< [in] The specification of the directory to create within pRoot.
	)
{
	SG_uint32    uIndex   = 0u;
	SG_pathname* pPath    = NULL;
	SG_file*     pFile    = NULL;
	SG_pathname* pTarget  = NULL;
	SG_string*   sContent = NULL;

	// make sure the root folder exists
	SG_ERR_TRY(  SG_fsobj__mkdir_recursive__pathname(pCtx, pRoot)  );
	SG_ERR_CATCH_IGNORE(SG_ERR_DIR_ALREADY_EXISTS);
	SG_ERR_CATCH_END;

	// run through each object in the directory and create it
	for (uIndex = 0u; uIndex < pDir->uCount; ++uIndex)
	{
		const _test_fsobj* pObject = pDir->pObjects + uIndex;

		if (pObject->szPath == NULL)
		{
			continue;
		}

		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pRoot, pObject->szPath)  );

		switch (pObject->eType)
		{
		case SG_FSOBJ_TYPE__REGULAR:
			VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE | SG_FILE_WRONLY | SG_FILE_TRUNC, 0666, &pFile)  );
			if (pObject->szContent != NULL)
			{
				VERIFY_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, pObject->szContent)  );
			}
			SG_FILE_NULLCLOSE(pCtx, pFile);
			break;
		case SG_FSOBJ_TYPE__DIRECTORY:
			VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPath)  );
			break;
#if defined(LINUX) || defined(MAC)
		case SG_FSOBJ_TYPE__SYMLINK:
			VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pTarget, pRoot, pObject->szContent)  );
			VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sContent, SG_pathname__sz(pTarget))  );
			VERIFY_ERR_CHECK(  SG_fsobj__symlink(pCtx, sContent, pPath)  );
			SG_STRING_NULLFREE(pCtx, sContent);
			break;
#endif
		default:
			break;
		}

		SG_PATHNAME_NULLFREE(pCtx, pPath);
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pTarget);
	SG_STRING_NULLFREE(pCtx, sContent);
	return;
}

/**
 * Verifies that a test directory contains the correct contents in the file system.
 */
static void u0013_fsobj__test_dir__verify(
	SG_context*        pCtx,        //< [in] [out] Error and context info.
	const SG_pathname* pSourceRoot, //< [in] Pathname of the source folder that was copied to pTargetRoot.
	const SG_pathname* pTargetRoot, //< [in] Pathname of the folder that contains the test directory.
	const _test_dir*   pDir,        //< [in] The test directory to verify the file system against.
	const char*        szTestName   //< [in] Name of the test to include in messages.
	)
{
	SG_uint32    uIndex           = 0u;
	SG_pathname* pPath            = NULL;
	SG_string*   sExpectedContent = NULL;
	SG_string*   sActualContent   = NULL;

	// run through each object in the directory and check it
	for (uIndex = 0u; uIndex < pDir->uCount; ++uIndex)
	{
		const _test_fsobj* pObject           = pDir->pObjects + uIndex;
		SG_bool            bExists           = SG_FALSE;
		SG_fsobj_type      eType             = SG_FSOBJ_TYPE__UNSPECIFIED;
		const char*        szExpectedContent = NULL;
		const char*        szActualContent   = NULL;

		if (pObject->szPath == NULL)
		{
			continue;
		}

#if defined(WINDOWS)
		SG_UNUSED(pSourceRoot);
		if (pObject->eType == SG_FSOBJ_TYPE__SYMLINK)
		{
			continue;
		}
#endif

		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pTargetRoot, pObject->szPath)  );
		if (pObject->szContent != NULL)
		{
			VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sExpectedContent, pObject->szContent)  );
		}

		VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, &eType, NULL)  );
		VERIFYP_COND("Expected object doesn't exist.", bExists == SG_TRUE, ("Test(%s) DirName(%s) Path(%s)", szTestName, pDir->szName, pObject->szPath));
		VERIFYP_COND("Object type doesn't match expectation.", eType == pObject->eType, ("Test(%s) DirName(%s) Path(%s) ExpectedType(%d) ActualType(%d)", szTestName, pDir->szName, pObject->szPath, pObject->eType, eType));

		if (bExists == SG_TRUE && eType == pObject->eType)
		{
			switch (pObject->eType)
			{
			case SG_FSOBJ_TYPE__REGULAR:
				SG_ERR_TRY(  SG_file__read_into_string(pCtx, pPath, &sActualContent)  );
				SG_ERR_CATCH_IGNORE(SG_ERR_EOF);
				SG_ERR_CATCH_END;
				break;
#if defined(LINUX) || defined(MAC)
			case SG_FSOBJ_TYPE__SYMLINK:
				VERIFY_ERR_CHECK(  SG_fsobj__readlink(pCtx, pPath, &sActualContent)  );
				if (sExpectedContent != NULL)
				{
					VERIFY_ERR_CHECK(  SG_string__insert__sz(pCtx, sExpectedContent, 0u, "/")  );
					VERIFY_ERR_CHECK(  SG_string__insert__sz(pCtx, sExpectedContent, 0u, SG_pathname__sz(pSourceRoot))  );
				}
				break;
#endif
			default:
				break;
			}
		}

		if (sExpectedContent != NULL)
		{
			szExpectedContent = SG_string__sz(sExpectedContent);
		}
		if (sActualContent != NULL)
		{
			szActualContent = SG_string__sz(sActualContent);
		}
		VERIFYP_COND("Object contents don't match expectation.", SG_strcmp__null(szActualContent, szExpectedContent) == 0, ("TestName(%s) DirName(%s) Path(%s) ExpectedContents(%s) ActualContents(%s)", szTestName, pDir->szName, pObject->szPath, szExpectedContent, szActualContent));

		SG_PATHNAME_NULLFREE(pCtx, pPath);
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, sExpectedContent);
	SG_STRING_NULLFREE(pCtx, sActualContent);
	return;
}

/**
 * Runs a single directory copy test.
 */
static void u0013_fsobj__cpdir_recursive__pathname_pathname__test(
	SG_context*      pCtx,
	const _test_dir* pSource,
	const _test_dir* pTarget
	)
{
	SG_string*   sTestName   = NULL;
	SG_pathname* pSourceRoot = NULL;
	SG_pathname* pTargetRoot = NULL;

	// generate a name for the test
	VERIFY_ERR_CHECK(  SG_string__alloc__format(pCtx, &sTestName, "%s -> %s", pSource->szName, pTarget == NULL ? "new" : pTarget->szName)  );

	// generate a path for each directory
	VERIFY_ERR_CHECK(  u0013_fsobj__get_test_path(pCtx, &pSourceRoot)  );
	VERIFY_ERR_CHECK(  u0013_fsobj__get_test_path(pCtx, &pTargetRoot)  );

	// generate the contents of each directory
	VERIFY_ERR_CHECK(  u0013_fsobj__test_dir__create(pCtx, pSourceRoot, pSource)  );
	if (pTarget != NULL)
	{
		VERIFY_ERR_CHECK(  u0013_fsobj__test_dir__create(pCtx, pTargetRoot, pTarget)  );
	}

	// copy the source directory to the target
	VERIFY_ERR_CHECK(  SG_fsobj__cpdir_recursive__pathname_pathname(pCtx, pSourceRoot, pTargetRoot)  );

	// verify that the contents of the source directory now appear in the target
	VERIFY_ERR_CHECK(  u0013_fsobj__test_dir__verify(pCtx, pSourceRoot, pTargetRoot, pSource, SG_string__sz(sTestName))  );

	// clean up the test directories
	VERIFY_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pSourceRoot)  );
	VERIFY_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pTargetRoot)  );

fail:
	SG_STRING_NULLFREE(pCtx, sTestName);
	SG_PATHNAME_NULLFREE(pCtx, pSourceRoot);
	SG_PATHNAME_NULLFREE(pCtx, pTargetRoot);
	return;
}

/**
 * Tests SG_fsobj__cpdir_recursive__pathname_pathname.
 */
static void u0013_fsobj__cpdir_recursive__pathname_pathname(
	SG_context* pCtx
	)
{
	SG_uint32    uSourceIndex = 0u;
	SG_uint32    uTargetIndex = 0u;

	// run through each test directory and copy it to somewhere
	for (uSourceIndex = 0u; uSourceIndex < SG_NrElements(gaTestDirs); ++uSourceIndex)
	{
		// test copying the directory to a non-existant target
		VERIFY_ERR_CHECK(  u0013_fsobj__cpdir_recursive__pathname_pathname__test(pCtx, gaTestDirs + uSourceIndex, NULL)  );

		// run through each test directory and copy the current source into it
		for (uTargetIndex = 0u; uTargetIndex < SG_NrElements(gaTestDirs); ++uTargetIndex)
		{
			// test copying the source directory to the target directory
			VERIFY_ERR_CHECK(  u0013_fsobj__cpdir_recursive__pathname_pathname__test(pCtx, gaTestDirs + uSourceIndex, gaTestDirs + uTargetIndex)  );
		}
	}

fail:
	return;
}


TEST_MAIN(u0013_fsobj)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0013_fsobj__test_mkdir(pCtx)  );
	BEGIN_TEST(  u0013_fsobj__test_bogus_mkdir(pCtx)  );
	BEGIN_TEST(  u0013_fsobj__test_getcwd(pCtx)  );
	BEGIN_TEST(  u0013_fsobj__test_create_perms(pCtx)  );
	BEGIN_TEST(  u0013_fsobj__test_chmod(pCtx)  );
	BEGIN_TEST(  u0013_fsobj__test_cd(pCtx)  ); // The error conditions for cd depend on being able to correctly chmod a directory.
	BEGIN_TEST(  u0013_fsobj__test_utf8paths(pCtx)  );

	BEGIN_TEST(  u0013_fsobj__test_rename_file_within_a_directory(pCtx)  );
	BEGIN_TEST(  u0013_fsobj__test_move_file_between_directories(pCtx)  );

	// TODO replace "/tmp" and "c:/temp" with SG_pathname__alloc__user_temp().

#if defined(MAC) || defined(LINUX)
	BEGIN_TEST(  u0013_fsobj__test_mkdir_recursive(pCtx, "/tmp",4)  );
	BEGIN_TEST(  u0013_fsobj__test_mkdir_rmdir_recursive(pCtx, "/tmp",4)  );
//	BEGIN_TEST(  u0013_fsobj__test_mkdir_recursive(pCtx, "/tmp",6)  );
//	BEGIN_TEST(  u0013_fsobj__test_mkdir_recursive(pCtx, "/tmp",32)  ); // this generates a 2084 character pathname
#endif
#if defined(WINDOWS)
	BEGIN_TEST(  u0013_fsobj__test_mkdir_recursive(pCtx, "c:/temp",3)  );
	BEGIN_TEST(  u0013_fsobj__test_mkdir_rmdir_recursive(pCtx, "c:/temp",3)  );
	BEGIN_TEST(  u0013_fsobj__test_mkdir_recursive(pCtx, "c:/temp",5)  );	// this generates 332 character pathname; this should fail without UNC stuff.
	BEGIN_TEST(  u0013_fsobj__test_mkdir_recursive(pCtx, "c:/temp",32)  );	// this generates 2087 character pathname; this should fail without UNC stuff.
#endif

	BEGIN_TEST(  u0013_fsobj__cpdir_recursive__pathname_pathname(pCtx)  );

	TEMPLATE_MAIN_END;
}
