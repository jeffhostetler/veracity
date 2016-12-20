/*
Copyright 2011-2013 SourceGear, LLC

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

#include <sg.h>

#define sg_DRAWER_VERSION_FILE "version"

#define sg_DRAWER_VERSION__WITH_PT 1	// WDs based upon the original PendingTree are version 1.
#define sg_DRAWER_VERSION__WITH_WC 2	// WDs based upon WC re-write of PendingTree are version 2.

#define sg_DRAWER_VERSION sg_DRAWER_VERSION__WITH_WC

// With W6947 I moved the <sgtemp> directory from @/.sgtemp
// to be inside <sgdrawer> so that we only have one special
// directory cluttering up the users WD.

#define sg_SGTEMP_ENTRYNAME "t"
#define sg_SGTEMP_PATH      SG_DRAWER_DIRECTORY_NAME "/" sg_SGTEMP_ENTRYNAME

// With W6947 I don't need to bury the submodule "graveyard"
// inside <sgtemp>, I can make it a peer (without cluttering
// the user's WD).  This will make the pathnames shorter and
// might make it easier to split the purposes (transient holding
// ground vs trash can).

#define sg_GRAVEYARD_ENTRYNAME "g"
#define sg_GRAVEYARD_PATH      SG_DRAWER_DIRECTORY_NAME "/" sg_GRAVEYARD_ENTRYNAME

//////////////////////////////////////////////////////////////////

static void _write_drawer_version(SG_context* pCtx, const SG_pathname* pPathDrawer, SG_uint32 version)
{
	SG_pathname* pPathVersionFile = NULL;
	SG_file* pFile = NULL;

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathDrawer, sg_DRAWER_VERSION_FILE)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathVersionFile, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY|SG_FILE_TRUNC, 0644, &pFile)  );

	/* The version is represented by the length of the file. We write null bytes to discourage edits. */
	{
		SG_uint32 i;
		SG_byte buf[100];
		SG_uint32 lenWritten = 0;

		for (i = 0; i < version; i++)
			buf[i] = 0;
		SG_ERR_CHECK(  SG_file__write(pCtx, pFile, version, buf, &lenWritten)  );
		if (lenWritten != version)
			SG_ERR_THROW(SG_ERR_INCOMPLETEWRITE);
	}

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

static void _verify_drawer_version(SG_context* pCtx, const SG_pathname* pPathDrawer)
{
	SG_bool bDrawerDirExists = SG_FALSE;
	SG_pathname* pPathVersionFile = NULL;
	SG_fsobj_type fsobjType;
	SG_uint64 len = 0;

	// 2011/12/13 WARNING if the .sgdrawer directory does not exist, we just return
	//            WARNING rather than complaining or doing anything.  The while(true)
	//            WARNING loop in SG_workingdir__find_mapping2() uses this (poorly)
	//            WARNING to walk up the tree from the original given local path to
	//            WARNING find the WD root.
	//            WARNING
	//            WARNING Only if the directory exists, do we look at the version file.
	//            WARNING
	//            WARNING This feels wrong.

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathDrawer, &bDrawerDirExists, NULL, NULL)  );
	if (!bDrawerDirExists)
		return;

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathDrawer, sg_DRAWER_VERSION_FILE)  );

	SG_fsobj__length__pathname(pCtx, pPathVersionFile, &len, &fsobjType);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_error err = SG_ERR_UNSPECIFIED;
		SG_context__get_err(pCtx, &err);

		switch (err)
		{
		default:
			SG_ERR_RETHROW;

#if defined(MAC) || defined(LINUX)
		case SG_ERR_ERRNO(ENOENT):
#endif
#if defined(WINDOWS)
		case SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND):
		case SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND):
#endif
			/* If the file is not found, pre-1.0 we "grandfather it in" and write the version 1 file.
			 * In the future, we probably want to fail with the unsupported closet version error. */
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  _write_drawer_version(pCtx, pPathDrawer, sg_DRAWER_VERSION__WITH_PT)  );
			// pretend like we observed the version 1 file
			len = sg_DRAWER_VERSION__WITH_PT;
		}
	}
	else
	{
		if (fsobjType != SG_FSOBJ_TYPE__REGULAR)
		{
			SG_ERR_THROW2(SG_ERR_UNSUPPORTED_DRAWER_VERSION, (pCtx, "the version file is invalid"));
		}
	}
	
	/* The version is represented by the length of the file so we don't have to open and parse it. */
	if (len != sg_DRAWER_VERSION)
	{
		SG_int_to_string_buffer buf;
		SG_ERR_CHECK(  SG_int64_to_sz(len, buf)  );
		SG_ERR_THROW2(SG_ERR_UNSUPPORTED_DRAWER_VERSION, (pCtx, "%s", buf));
	}

	/* Common cleanup */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);
}

//////////////////////////////////////////////////////////////////////////

void SG_workingdir__get_drawer_directory_name(
	SG_context *  pCtx,
	const char ** ppName
	)
{
	SG_NULLARGCHECK_RETURN(ppName);
	*ppName = SG_DRAWER_DIRECTORY_NAME;
}

void SG_workingdir__get_drawer_path(
	SG_context* pCtx,
	const SG_pathname* pPathWorkingDirectoryTop,
	SG_pathname** ppResult)
{
	SG_pathname* pPathDrawer = NULL;

	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDrawer, pPathWorkingDirectoryTop, SG_DRAWER_DIRECTORY_NAME)  );
	SG_ERR_CHECK(  _verify_drawer_version(pCtx, pPathDrawer)  );

	*ppResult = pPathDrawer;

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathDrawer);
}

void SG_workingdir__verify_drawer_exists(
	SG_context* pCtx,
	const SG_pathname* pPathWorkingDirectoryTop,
	SG_pathname** ppResult,
	SG_bool * pbWeCreatedDrawer)
{
	SG_pathname* pDrawerPath = NULL;
	SG_fsobj_type type = SG_FSOBJ_TYPE__UNSPECIFIED;
	SG_fsobj_perms perms;
	SG_bool bExists = SG_FALSE;

	SG_ERR_CHECK(  SG_workingdir__get_drawer_path(pCtx, pPathWorkingDirectoryTop, &pDrawerPath)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pDrawerPath, &bExists, &type, &perms)  );

	if (!bExists)
	{
		// we are creating a new drawer.  mark it as being of the version.
		SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pDrawerPath)  );
		SG_ERR_CHECK(  _write_drawer_version(pCtx, pDrawerPath, sg_DRAWER_VERSION)  );
	}
	else
	{
		if (SG_FSOBJ_TYPE__DIRECTORY != type)
		{
			SG_ERR_THROW(SG_ERR_NOT_A_DIRECTORY);
		}
	}

	if (ppResult)
		*ppResult = pDrawerPath;
	else
		SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);

	if (pbWeCreatedDrawer)
		*pbWeCreatedDrawer = !bExists;

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch the observed version of the drawer.
 * We DO NOT claim to understand what is in it.
 *
 * Note that this routine directly accesses the .sgdrawer/version
 * file without using any of the above routines.  We really should
 * refactor _verify_drawer_version() and SG_workingdir__get_drawer_path()
 * and SG_workingdir__verify_drawer_exists() so that we can call some of
 * that code here.  (We can't because we just want to get the version
 * number *without* trying to verify it nor create a new one.)
 *
 */
void SG_workingdir__get_drawer_version(SG_context * pCtx,
									   const SG_pathname * pPathWorkingDirectoryTop,
									   SG_uint32 * pVersion)
{
	SG_pathname* pPathDrawer = NULL;
	SG_pathname* pPathVersionFile = NULL;
	SG_uint64 len = 0;
	SG_fsobj_type fsobjType;
	SG_bool bDrawerDirExists;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDrawer, pPathWorkingDirectoryTop, SG_DRAWER_DIRECTORY_NAME)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathDrawer, &bDrawerDirExists, NULL, NULL)  );
	if (bDrawerDirExists)
	{
		SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathDrawer, sg_DRAWER_VERSION_FILE)  );

		SG_fsobj__length__pathname(pCtx, pPathVersionFile, &len, &fsobjType);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_error err = SG_ERR_UNSPECIFIED;
			SG_context__get_err(pCtx, &err);

			switch (err)
			{
			default:
				SG_ERR_RETHROW;

#if defined(MAC) || defined(LINUX)
			case SG_ERR_ERRNO(ENOENT):
#endif
#if defined(WINDOWS)
			case SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND):
			case SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND):
#endif
				// If the file is not found (meaning a pre-1.0)
				// we "grandfather it in" and assume version 1.
				SG_context__err_reset(pCtx);
				len = sg_DRAWER_VERSION__WITH_PT;
			}
		}
		else
		{
			if (fsobjType != SG_FSOBJ_TYPE__REGULAR)
			{
				SG_ERR_THROW2(SG_ERR_UNSUPPORTED_DRAWER_VERSION, (pCtx, "the version file is invalid"));
			}
		}
	}

	*pVersion = ((SG_uint32)len);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathDrawer);
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);
}

//////////////////////////////////////////////////////////////////

void SG_workingdir__set_mapping(
	SG_context* pCtx,
	const SG_pathname* pPathLocalDirectory,
	const char* pszNameRepoInstanceDescriptor, /**< The name of the repo instance descriptor */
	const char* pszidGidAnchorDirectory /**< The GID of the directory within the repo to which this is anchored.  Usually it's user root. */
	)
{
	SG_vhash* pvhNew = NULL;
	SG_vhash* pvh = NULL;
	SG_pathname* pMyPath = NULL;
	SG_pathname* pDrawerPath = NULL;
	SG_pathname* pMappingFilePath = NULL;

	SG_NULLARGCHECK_RETURN(pPathLocalDirectory);
	SG_NULLARGCHECK_RETURN(pszNameRepoInstanceDescriptor);

	/* make a copy of the path so we can modify it (adding the final slash) */
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pMyPath, pPathLocalDirectory)  );

	/* null the original parameter pointer to make sure we don't use it anymore */
	pPathLocalDirectory = NULL;

	/* make sure the path we were given is a directory that exists */
	SG_ERR_CHECK(  SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pMyPath)  );

	/* it's a directory, so it should have a final slash */
	SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pMyPath)  );

	/* make sure the name of the repo instance descriptor is valid */
	SG_ERR_CHECK(  SG_closet__descriptors__get(pCtx, pszNameRepoInstanceDescriptor, NULL, &pvh) );
	SG_VHASH_NULLFREE(pCtx, pvh);

	// TODO verify that the anchor GID is valid for that repo?

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhNew)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhNew, "descriptor", pszNameRepoInstanceDescriptor)  );
    if (pszidGidAnchorDirectory)
    {
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhNew, "anchor", pszidGidAnchorDirectory)  );
    }

	SG_ERR_CHECK(  SG_workingdir__verify_drawer_exists(pCtx, pMyPath, &pDrawerPath, NULL)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pMappingFilePath, pDrawerPath, "repo.json")  );

	SG_ERR_CHECK(  SG_vfile__update__vhash(pCtx, pMappingFilePath, "mapping", pvhNew)  );

	SG_VHASH_NULLFREE(pCtx, pvhNew);
	SG_PATHNAME_NULLFREE(pCtx, pMyPath);
	SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);
	SG_PATHNAME_NULLFREE(pCtx, pMappingFilePath);

	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);
	SG_PATHNAME_NULLFREE(pCtx, pMappingFilePath);
	SG_PATHNAME_NULLFREE(pCtx, pMyPath);
	SG_VHASH_NULLFREE(pCtx, pvhNew);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_workingdir__find_mapping(
	SG_context* pCtx,
	const SG_pathname* pPathLocalDirectory,
	SG_pathname** ppPathMappedLocalDirectory, /**< Return the actual local directory that contains the mapping */
	SG_string** ppstrNameRepoInstanceDescriptor, /**< Return the name of the repo instance descriptor */
	char** ppszidGidAnchorDirectory /**< Return the GID of the repo directory */
	)
{
	// For historical reasons, the default __find_mapping() always searches current dir and parents.

	SG_ERR_CHECK_RETURN(  SG_workingdir__find_mapping2(pCtx,
													   pPathLocalDirectory,
													   SG_TRUE,
													   ppPathMappedLocalDirectory,
													   ppstrNameRepoInstanceDescriptor,
													   ppszidGidAnchorDirectory)  );
}

void SG_workingdir__find_mapping2(
	SG_context* pCtx,
	const SG_pathname* pPathLocalDirectory,
	SG_bool bRecursive,
	SG_pathname** ppPathMappedLocalDirectory, /**< Return the actual local directory that contains the mapping */
	SG_string** ppstrNameRepoInstanceDescriptor, /**< Return the name of the repo instance descriptor */
	char** ppszidGidAnchorDirectory /**< Return the GID of the repo directory */
	)
{
	SG_pathname* curpath = NULL;
	SG_string* result_pstrDescriptorName = NULL;
	char* result_pszidGid = NULL;
	SG_pathname* result_mappedLocalDirectory = NULL;
	SG_vhash* pvhMapping = NULL;
	SG_pathname* pDrawerPath = NULL;
	SG_pathname* pMappingFilePath = NULL;
	SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(pPathLocalDirectory);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &curpath, pPathLocalDirectory)  );

	/* it's a directory, so it should have a final slash */
	SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, curpath)  );

	while (SG_TRUE)
	{
		SG_ERR_CHECK(  SG_workingdir__get_drawer_path(pCtx, curpath, &pDrawerPath)  );

		SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pDrawerPath);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
		{
			const char* pszDescriptorName = NULL;
			const char* pszGid = NULL;

			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pMappingFilePath, pDrawerPath, "repo.json")  );
			SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);

			SG_ERR_CHECK(  SG_vfile__slurp(pCtx, pMappingFilePath, &pvh)  );

			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "mapping", &pvhMapping)  );

			SG_PATHNAME_NULLFREE(pCtx, pMappingFilePath);

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhMapping, "descriptor", &pszDescriptorName)  );
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhMapping, "anchor", &pszGid)  );

			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &result_pstrDescriptorName)  );
			SG_ERR_CHECK(  SG_string__set__sz(pCtx, result_pstrDescriptorName, pszDescriptorName)  );

			if (pszGid)
			{
				SG_ERR_CHECK(  SG_gid__alloc_clone(pCtx, pszGid, &result_pszidGid)  );
			}
			else
			{
				result_pszidGid = NULL;
			}

			SG_VHASH_NULLFREE(pCtx, pvh);

			result_mappedLocalDirectory = curpath;
			curpath = NULL;

			break;
		}
		SG_context__err_reset(pCtx);

		SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);

		if (!bRecursive)
			break;

		SG_pathname__remove_last(pCtx, curpath);

		if (SG_context__err_equals(pCtx, SG_ERR_CANNOTTRIMROOTDIRECTORY))
		{
			SG_context__err_reset(pCtx);
			break;
		}
		else
		{
			SG_ERR_CHECK_CURRENT;
		}
	}

	if (result_mappedLocalDirectory)
	{
		{
			SG_bool bExists = SG_FALSE;
			SG_ERR_CHECK(  SG_closet__descriptors__exists(pCtx, SG_string__sz(result_pstrDescriptorName), &bExists)  );
			if(!bExists)
				SG_ERR_CHECK(  SG_vv_utils__throw_NOTAREPOSITORY_for_wd_association(pCtx, SG_string__sz(result_pstrDescriptorName), result_mappedLocalDirectory)  );
		}

        if (ppPathMappedLocalDirectory)
        {
		    *ppPathMappedLocalDirectory = result_mappedLocalDirectory;
		    result_mappedLocalDirectory = NULL;
        }
        if (ppstrNameRepoInstanceDescriptor)
        {
            *ppstrNameRepoInstanceDescriptor = result_pstrDescriptorName;
            result_pstrDescriptorName = NULL;
        }
        if (ppszidGidAnchorDirectory)
        {
            *ppszidGidAnchorDirectory = result_pszidGid;
            result_pszidGid = NULL;
        }
	}
	else
	{
		SG_PATHNAME_NULLFREE(pCtx, curpath);

		if (bRecursive)
			SG_ERR_THROW2_RETURN(  SG_ERR_NOT_A_WORKING_COPY,
								   (pCtx, "The path '%s' is not inside a working copy.",
									SG_pathname__sz(pPathLocalDirectory))  );
		else
			SG_ERR_THROW2_RETURN(  SG_ERR_NOT_A_WORKING_COPY,
								   (pCtx, "The path '%s' is not the root of a working copy.",
									SG_pathname__sz(pPathLocalDirectory))  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_PATHNAME_NULLFREE(pCtx, pDrawerPath);
	SG_PATHNAME_NULLFREE(pCtx, pMappingFilePath);
	SG_PATHNAME_NULLFREE(pCtx, result_mappedLocalDirectory);
	SG_STRING_NULLFREE(pCtx, result_pstrDescriptorName);
	SG_NULLFREE(pCtx, result_pszidGid);
	SG_PATHNAME_NULLFREE(pCtx, curpath);
}

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

void SG_workingdir__get_temp_path(SG_context* pCtx,
								  const SG_pathname* pPathWorkingDirectoryTopAsGiven,
								  SG_pathname** ppResult)
{
	SG_pathname * pPathname = NULL;

	SG_NULLARGCHECK_RETURN(ppResult);

	// Create the full pathname to <sgtemp>.
	// 
	// I don't trust the given <wd-top>, get the actual one from the disk
	// using this as a starting point.

	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx,pPathWorkingDirectoryTopAsGiven,&pPathname,NULL,NULL)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPathname,sg_SGTEMP_PATH)  );

	*ppResult = pPathname;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
}

void SG_workingdir__generate_and_create_temp_dir_for_purpose(SG_context * pCtx,
															 const SG_pathname * pPathWorkingDirectoryTop,
															 const char * pszPurpose,
															 SG_pathname ** ppPathTempDir)
{
	SG_pathname * pPathTempRoot = NULL;
	SG_pathname * pPath = NULL;
	SG_string * pString = NULL;
	SG_int64 iTimeUTC;
	SG_time tmLocal;
	SG_uint32 kAttempt = 0;

	SG_NONEMPTYCHECK_RETURN(pszPurpose);
	SG_NULLARGCHECK_RETURN(ppPathTempDir);

	// get full path of <sgtemp>

	SG_ERR_CHECK(  SG_workingdir__get_temp_path(pCtx,pPathWorkingDirectoryTop,&pPathTempRoot)  );

	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx,&iTimeUTC)  );
	SG_ERR_CHECK(  SG_time__decode__local(pCtx,iTimeUTC,&tmLocal)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pString)  );

	while (1)
	{
		// build full path to a per-task-sub-dir <sgtemp>/<purpose>_20091201_0"
		// where <purpose> is something like "revert" or "merge".

		SG_ERR_CHECK(  SG_string__sprintf(pCtx,pString,"%s_%04d%02d%02d_%d",
										  pszPurpose,
										  tmLocal.year,tmLocal.month,tmLocal.mday,
										  kAttempt++)  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,&pPath,pPathTempRoot,SG_string__sz(pString))  );

		// try to create a NEW temp directory.  if this path already exists on disk,
		// loop and try again.  if we have a hard errors, just give up.

		SG_fsobj__mkdir_recursive__pathname(pCtx,pPath);
		if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
			goto success;

		if (SG_context__err_equals(pCtx,SG_ERR_DIR_ALREADY_EXISTS) == SG_FALSE)
			SG_ERR_RETHROW;

		SG_context__err_reset(pCtx);
		SG_PATHNAME_NULLFREE(pCtx,pPath);
	}

success:
	*ppPathTempDir = pPath;

	SG_STRING_NULLFREE(pCtx, pString);
	SG_PATHNAME_NULLFREE(pCtx, pPathTempRoot);
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_PATHNAME_NULLFREE(pCtx, pPathTempRoot);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

void SG_workingdir__construct_absolute_path_from_repo_path(SG_context * pCtx,
														   const SG_pathname * pPathWorkingDirectoryTop,
														   const char * pszRepoPath,
														   SG_pathname ** ppPathAbsolute)
{
	SG_pathname * pPath = NULL;

#if defined(DEBUG)
	// TODO we should verify "<wd-top>/.sgdrawer" exists to make sure
	// TODO that we have the actual wd-root.  especially when using vv
	// TODO with relative paths while cd'd somewhere deep inside the tree.
#endif

	if (pszRepoPath[0] != '@')
	{
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALID_REPO_PATH,
							   (pCtx, "Must begin with '@/': %s", pszRepoPath)  );
	}
	else
	{
		if (pszRepoPath[1] == 0)		// quietly allow "@" as substitute for "@/"
		{
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath, pPathWorkingDirectoryTop)  );
		}
		else if (pszRepoPath[1] != '/')
		{
			SG_ERR_THROW2_RETURN(  SG_ERR_INVALID_REPO_PATH,
								   (pCtx, "Must begin with '@/': %s", pszRepoPath)  );
		}
		else
		{
			if (pszRepoPath[2])
				SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPathWorkingDirectoryTop, &pszRepoPath[2])  );
			else
				SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath, pPathWorkingDirectoryTop)  );
		}
	}

	*ppPathAbsolute = pPath;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}


void SG_workingdir__wdpath_to_repopath(
	SG_context* pCtx,
	const SG_pathname* pPathWorkingDirectoryTop,
	const SG_pathname* pPath,
	SG_bool bFinalSlash,
	SG_string** ppStrRepoPath)
{
	SG_string* pstrRoot = NULL;
	SG_string* pstrRelative = NULL;
	SG_string* pstrRepoPath = NULL;

    /* TODO fix the following line to get the real anchor path for this working
     * dir */
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrRoot, "@/")  );

	// TODO 2011/08/17 See bug S00043 __make_relative("/a/b/c/", "/a/b/c/") returns "/"
	SG_ERR_CHECK(  SG_pathname__make_relative(pCtx, pPathWorkingDirectoryTop, pPath, &pstrRelative)  );

	if (
		SG_string__sz(pstrRelative)[0] != '\0' &&
		strcmp(SG_string__sz(pstrRelative), "/") != 0
	)
	{
		//Check to make sure that the relative path does not go above the wd root.
		SG_uint32 relativeLength = SG_string__length_in_bytes(pstrRelative);
		if (relativeLength > 2)
		{
			SG_byte firstByte, secondByte;
			SG_ERR_CHECK(  SG_string__get_byte_l(pCtx, pstrRelative, 0, &firstByte)  );
			SG_ERR_CHECK(  SG_string__get_byte_l(pCtx, pstrRelative, 1, &secondByte)  );

			// TODO 2011/08/17 It would be better if we threw SG_ERR_PATH_NOT_IN_WORKING_COPY.
			if (firstByte == '.' && secondByte == '.')
				SG_ERR_THROW2(SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL, (pCtx, "Pathname '%s' is not under version control.", SG_pathname__sz(pPath))  );
		}
		SG_ERR_CHECK(  SG_repopath__combine(pCtx, pstrRoot, pstrRelative, bFinalSlash, &pstrRepoPath)  );
		SG_STRING_NULLFREE(pCtx, pstrRoot);

		*ppStrRepoPath = pstrRepoPath;
	}
	else
	{
		*ppStrRepoPath = pstrRoot;
	}
	SG_STRING_NULLFREE(pCtx, pstrRelative);

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstrRoot);
	SG_STRING_NULLFREE(pCtx, pstrRelative);
	SG_STRING_NULLFREE(pCtx, pstrRepoPath);
}

//////////////////////////////////////////////////////////////////

void SG_workingdir__move_submodule_to_graveyard(SG_context * pCtx,
												const SG_pathname * pPathWorkingDirectoryTopAsGiven,
												const SG_pathname * pPathSubmodule,
												const char * pszGid,
												SG_pathname ** ppPathInGraveyard)
{
	SG_pathname * pPath_graveyard = NULL;
	SG_pathname * pPath = NULL;
	SG_pathname * pPathInfo = NULL;
	SG_string * pString = NULL;
	SG_vhash * pvhInfo = NULL;
	SG_int64 iTimeUTC;
	SG_time tmLocal;
	SG_uint32 kAttempt;
	SG_bool bExists;

	// move the submodule directory out of the WD and
	// into the "graveyard".  this is an area where we
	// can leave the entire submodule directory.
	//
	// someone did a "vv remove <sub>" either explicitly
	// or we need to delete it in the course of a merge
	// or update or something.
	// 
	// sg_ptnode__remove_safely() would like to delete
	// if from the WD and from the pendingtree, but we
	// are not sure if we should really delete it from
	// the disk.  Yes, the current WD no longer references
	// the submodule (with this GID at least) and so it
	// should not appear in the WD, ***BUT*** that does
	// not mean we should actually delete.  For example,
	// if it is a GIT or HG (or eventually our own in-place
	// style repo/wd), then the submodule also contains
	// a complete copy of the history -- this might be
	// their only copy.
	//
	// also, by moving it to the graveyard, we allow
	// "vv revert" recover it rather than having to
	// completely rebuild it 
	//
	// it is tempting to give this a unique path with
	// either the GID or the HID of the submodule, but
	// both schemes have their problems -- and will create
	// a very long pathname.  since each submodule directory
	// contains identifying information within (and we can
	// just sniff it), i'm just going to generate a
	// sequence number for the directory name.
	//
	// i'm also tempted to put the contents of the pTSM
	// in a vfile next to it, but i'm not sure if that'll
	// be of any value yet and it'll be potentially stale
	// data (as they could go into the directory while it
	// is in the graveyard and modify things).
	//
	// so for now, i'm only going to put the GID in a
	// peer file so that we have an idea of what it was.

	// Create the full pathname to <graveyard>.
	//
	// I don't trust the given <wd-top>, get the actual one from the disk
	// using this as a starting point.

	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx,pPathWorkingDirectoryTopAsGiven,&pPath_graveyard,NULL,NULL)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPath_graveyard,sg_GRAVEYARD_PATH)  );
	SG_fsobj__mkdir_recursive__pathname(pCtx, pPath_graveyard);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (!SG_context__err_equals(pCtx, SG_ERR_DIR_ALREADY_EXISTS))
			SG_ERR_RETHROW;
		SG_context__err_reset(pCtx);
	}

	// We want to put the submodule sub-directory and the
	// corresponding meta-data in something of the form:
	//     <graveyard>/<date>_<#>
	//     <graveyard>/<date>_<#>.json
	//
	// So we have to search for the next available spot.

	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx,&iTimeUTC)  );
	SG_ERR_CHECK(  SG_time__decode__local(pCtx,iTimeUTC,&tmLocal)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );

	kAttempt = 0;
	while (1)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx,
										  pString,
										  "%04d%02d%02d_%03d",
										  tmLocal.year,tmLocal.month,tmLocal.mday,
										  kAttempt++)  );

		SG_PATHNAME_NULLFREE(pCtx, pPath);
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,
													   &pPath,
													   pPath_graveyard,
													   SG_string__sz(pString))  );

		// yeah i know there's a race condition here...
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
		if (!bExists)
			break;
	}

	// move the submodule to the graveyard.

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "move_submodule_to_graveyard: %s %s\n",
							   SG_pathname__sz(pPathSubmodule),
							   SG_pathname__sz(pPath))  );
	SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPathSubmodule, pPath)  );

	// write meta-data about this submodule.

	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pString, ".json")  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,
												   &pPathInfo,
												   pPath_graveyard,
												   SG_string__sz(pString))  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhInfo)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo, "gid", pszGid)  );
	SG_ERR_CHECK(  SG_vfile__overwrite(pCtx, pPathInfo, pvhInfo)  );

	if (ppPathInGraveyard)
	{
		*ppPathInGraveyard = pPath;
		pPath = NULL;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_graveyard);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathInfo);
	SG_STRING_NULLFREE(pCtx, pString);
	SG_VHASH_NULLFREE(pCtx, pvhInfo);
}

//////////////////////////////////////////////////////////////////

struct _my_find_submodule_in_graveyard_data
{
	const char * pszGid_goal;
	SG_pathname * pPath_graveyard;
	SG_rbtree * prb_graveyard;
};

static SG_dir_foreach_callback _my_find_submodule_in_graveyard;

static void _my_find_submodule_in_graveyard(SG_context *      pCtx,
											const SG_string * pStringEntryName,
											SG_fsobj_stat *   pfsStat,
											void *            pVoidData)
{
	struct _my_find_submodule_in_graveyard_data * pData = (struct _my_find_submodule_in_graveyard_data *)pVoidData;
	SG_vhash * pvh = NULL;
	char * psz = NULL;
	const char * pszGid;
	SG_pathname * pPath = NULL;
	SG_pathname * pPath_json = NULL;
	const char * psz_json;
	const char * psz_suffix;
	SG_uint32 len;

	// we expect to be given the entryname of a file in the form "<date_k>_<j>.json"
	// or the entryname of a directory in the form "<date_k>_<j>".

	if (pfsStat->type != SG_FSOBJ_TYPE__REGULAR)
		return;
	len = SG_string__length_in_bytes(pStringEntryName);
	if (len < 5)
		return;
	psz_json = SG_string__sz(pStringEntryName);
	psz_suffix = &psz_json[len-5];
	if (strcmp(psz_suffix, ".json") != 0)
		return;

	// make a pathname of the form "<graveyard>/<date_k>_<j>.json".

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,
												   &pPath_json,
												   pData->pPath_graveyard,
												   SG_string__sz(pStringEntryName))  );

	// read the contained vhash and see if the gid field matches what we
	// are looking for.

	SG_ERR_CHECK(  SG_vfile__slurp(pCtx, pPath_json, &pvh)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "gid", &pszGid)  );
	if (strcmp(pszGid, pData->pszGid_goal) == 0)
	{
		// make pathname to the corresponding submodule directory.
		// this is of the form "<graveyard>/<date_k>_<j>".
		// 
		// if the directory exists, add the pathname to the rbtree.
		//
		// TODO 2011/03/30 Consider doing a submodule-sniff on the
		// TODO            directory to confirm that it is what we
		// TODO            think it is.

		SG_bool bExists = SG_FALSE;

		SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_json, &psz)  );
		psz[len-5] = 0;
		
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx,
													   &pPath,
													   pData->pPath_graveyard,
													   psz)  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
		if (bExists)
			SG_ERR_CHECK(  SG_rbtree__add(pCtx, pData->prb_graveyard, SG_pathname__sz(pPath))  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, psz);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath_json);
}

void SG_workingdir__find_submodule_in_graveyard(SG_context * pCtx,
												const SG_pathname * pPathWorkingDirectoryTop,
												const char * pszGid,
												SG_bool * pbFound,
												SG_pathname ** ppPathInGraveyard)
{
	struct _my_find_submodule_in_graveyard_data data;
	SG_bool bExists = SG_FALSE;
	SG_bool bFound = SG_FALSE;
	const char * pszPath;

	*pbFound = SG_FALSE;
	*ppPathInGraveyard = NULL;

	// we may have one or more zombie submodules in the graveyard.
	// these are of the form
	//     "<graveyard>/{<date_k>_<j>,<date_k>_<j>.json}"
	// 
	// search the graveyard for the most recent instance of this
	// submodule and return its pathname.

	memset(&data, 0, sizeof(data));
	data.pszGid_goal = pszGid;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS2(pCtx,
											 &data.prb_graveyard,
											 0, NULL,
											 SG_rbtree__compare_function__reverse_strcmp)  );

	// is there a graveyard

	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx,pPathWorkingDirectoryTop,&data.pPath_graveyard,NULL,NULL)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,data.pPath_graveyard,sg_GRAVEYARD_PATH)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, data.pPath_graveyard, &bExists, NULL, NULL)  );
	if (!bExists)
		goto fail;

	SG_ERR_CHECK(  SG_dir__foreach(pCtx, data.pPath_graveyard,
								   SG_DIR__FOREACH__STAT | SG_DIR__FOREACH__SKIP_OS,
								   _my_find_submodule_in_graveyard,
								   &data)  );

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, NULL, data.prb_graveyard,
											  &bFound,
											  &pszPath, NULL)  );
	if (!bFound)
		goto fail;
	
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,
										  ppPathInGraveyard,
										  pszPath)  );
	*pbFound = SG_TRUE;

fail:
	SG_PATHNAME_NULLFREE(pCtx, data.pPath_graveyard);
	SG_RBTREE_NULLFREE(pCtx, data.prb_graveyard);
}
