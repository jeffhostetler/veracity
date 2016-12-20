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

// u0015_dir.c
// test routines to read a directory.
//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "unittests.h"

//////////////////////////////////////////////////////////////////

void _u0015_dir__write_file(SG_context *pCtx, const char * sz, SG_pathname * pDirectoryPath, const char * sz_filename)
{
	SG_file * pFile = NULL;
	SG_pathname * pFilePath = NULL;

	SG_ERR_CHECK( SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pFilePath, pDirectoryPath, sz_filename)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pFilePath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY, 0666, &pFile)  );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, sz)  );
	SG_ERR_CHECK(  SG_file__truncate(pCtx, pFile)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFilePath);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}


void u0015_dir__print_file_info(const SG_string * pStringFileName, SG_fsobj_stat * pFsStat)
{
	char buf_i64[40];

	switch (pFsStat->type)
	{
	case SG_FSOBJ_TYPE__REGULAR:
		INFOP("u0015_dir:file:",("name[%s] perms[%04o] type[%d] size[%s]",
								 SG_string__sz(pStringFileName),
								 pFsStat->perms, pFsStat->type,
								 SG_uint64_to_sz(pFsStat->size, buf_i64)));
		return;

	case SG_FSOBJ_TYPE__SYMLINK:
		INFOP("u0015_dir:symlink:",("name[%s] perms[%04o] type[%d] size[%s]",
									SG_string__sz(pStringFileName),
									pFsStat->perms, pFsStat->type,
									SG_uint64_to_sz(pFsStat->size, buf_i64)));
		return;

	case SG_FSOBJ_TYPE__DIRECTORY:
		INFOP("u0015_dir:directory:",("name[%s] perms[%04o] type[%d]",
									  SG_string__sz(pStringFileName),
									  pFsStat->perms, pFsStat->type));
		return;

	default:
		INFOP("u0015_dir:???:",("name[%s] perms[%04o] type[%d]",
								SG_string__sz(pStringFileName),
								pFsStat->perms, pFsStat->type));
		return;
	}
}

//////////////////////////////////////////////////////////////////

void u0015_dir__readdir(SG_context * pCtx, SG_pathname * pPathnameDirectory)
{
	SG_string * pStringFileName = NULL;
	SG_fsobj_stat fsStat;
	SG_dir * pDir = NULL;
	SG_error errReadStat;
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringFileName)  );
	
	//Now deposit a bunch of files in the temp directory.
	VERIFY_ERR_CHECK(  _u0015_dir__write_file(pCtx, "file contents", pPathnameDirectory, "readdir_1.txt")  );
	VERIFY_ERR_CHECK(  _u0015_dir__write_file(pCtx, "file contents", pPathnameDirectory, "readdir_2.txt")  );
	VERIFY_ERR_CHECK(  _u0015_dir__write_file(pCtx, "file contents", pPathnameDirectory, "readdir_3.txt")  );

	INFOP("dir__opendir",("SG_dir__open[%s]:",SG_pathname__sz(pPathnameDirectory)));

	VERIFY_ERR_CHECK_DISCARD(  SG_dir__open(pCtx,
											pPathnameDirectory,
											&pDir,
											&errReadStat,
											pStringFileName,
											&fsStat)  );
	VERIFYP_COND("dir__readdir",(SG_IS_OK(errReadStat)),("SG_dir__read[%s]",SG_pathname__sz(pPathnameDirectory)));

	do
	{
		u0015_dir__print_file_info(pStringFileName,&fsStat);

		SG_dir__read(pCtx, pDir,pStringFileName,&fsStat);
		SG_context__get_err(pCtx,&errReadStat);

	} while (SG_IS_OK(errReadStat));

	VERIFY_CTX_ERR_EQUALS("dir_readdir",pCtx,SG_ERR_NOMOREFILES);
	SG_context__err_reset(pCtx);

fail:
	SG_DIR_NULLCLOSE(pCtx, pDir);
	SG_STRING_NULLFREE(pCtx, pStringFileName);
}

//////////////////////////////////////////////////////////////////

static SG_dir_foreach_callback u0015_dir__foreach_cb;

static void u0015_dir__foreach_cb(SG_UNUSED_PARAM( SG_context * pCtx ),
								  const SG_string * pStringEntryName,
								  SG_fsobj_stat * pfsStat,
								  SG_UNUSED_PARAM( void * pVoidData ))
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pVoidData);

	u0015_dir__print_file_info(pStringEntryName,pfsStat);
}

void u0015_dir__readdir_using_foreach(SG_context * pCtx, SG_pathname * pPathnameDirectory)
{
	//Now deposit a bunch of files in the temp directory.
	VERIFY_ERR_CHECK(  _u0015_dir__write_file(pCtx, "file contents", pPathnameDirectory, "foreach_1.txt")  );
	VERIFY_ERR_CHECK(  _u0015_dir__write_file(pCtx, "file contents", pPathnameDirectory, "foreach_2.txt")  );
	VERIFY_ERR_CHECK(  _u0015_dir__write_file(pCtx, "file contents", pPathnameDirectory, "foreach_3.txt")  );
	INFOP("readdir_using_foreach",("SG_dir__foreach[%s]:",SG_pathname__sz(pPathnameDirectory)));

	VERIFY_ERR_CHECK(  SG_dir__foreach(pCtx,pPathnameDirectory,SG_DIR__FOREACH__STAT,u0015_dir__foreach_cb,NULL)  );
fail:
	;
}

//////////////////////////////////////////////////////////////////

TEST_MAIN(u0015_dir)
{
	SG_string * pstr_TempDir = NULL;
	SG_pathname * pPathnameDirectory = NULL;
	TEMPLATE_MAIN_START;

	//Create the tempdir
	VERIFY_ERR_CHECK(  SG_environment__get__str(pCtx, "TEMPDIR", &pstr_TempDir, NULL)  );
	VERIFY_COND_FAIL(  "TEMPDIR environment variable should be set", pstr_TempDir != NULL && SG_string__sz(pstr_TempDir) != NULL);
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathnameDirectory, SG_string__sz(pstr_TempDir))  );
	SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathnameDirectory)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPathnameDirectory)  );

	BEGIN_TEST(  u0015_dir__readdir(pCtx, pPathnameDirectory)  );
	BEGIN_TEST(  u0015_dir__readdir_using_foreach(pCtx, pPathnameDirectory)  );
fail:
	SG_STRING_NULLFREE(pCtx, pstr_TempDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameDirectory);
	TEMPLATE_MAIN_END;
}
