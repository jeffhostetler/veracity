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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Create a private temp-dir for the duration of pData
 * if we don't already have one.
 *
 */
void sg_vv2__diff__create_session_temp_dir(SG_context * pCtx,
										   sg_vv6diff__diff_to_stream_data * pData)
{
	char bufTidSession[SG_TID_MAX_BUFFER_LENGTH];
	SG_uint32 nrDigits = 10;

	if (pData->pPathSessionTempDir)
		return;

	// pick a space in /tmp for exporting temporary copies of the files
	// so that internal and/or external tools can compare them.

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pData->pPathSessionTempDir)  );
	SG_ERR_CHECK(  SG_tid__generate2(pCtx, bufTidSession, sizeof(bufTidSession), nrDigits)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pData->pPathSessionTempDir, bufTidSession)  );

	SG_ERR_TRY(  SG_fsobj__mkdir_recursive__pathname(pCtx, pData->pPathSessionTempDir)  );
	SG_ERR_CATCH_IGNORE(  SG_ERR_DIR_ALREADY_EXISTS  );
	SG_ERR_CATCH_END;

#if 0 && defined(DEBUG)
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "VV2_CreateSessionTempDir: %s\n",
							   SG_pathname__sz(pData->pPathSessionTempDir))  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Export a blob-of-interest from the REPO into a temporary directory
 * so that we can let the external diff tool compare it with another
 * version somewhere.
 *
 * We create a file with a GID-based name rather than re-creating
 * the working-directory hierarchy in the temp directory.  This lets
 * us flatten the export in one directory without collisions and
 * avoids move/rename and added/deleted sub-directory issues.
 *
 * pPathTempSessionDir should be something like "$TMPDIR/gid_session/".
 * This should be a unique directory name such that everything being
 * exported is isolated from other runs.  (if we are doing a
 * changeset-vs-changeset diff, we may create lots of files on each
 * side -- and our command should not interfere with other diffs
 * in progress in other processes.)
 *
 * szVersion should be a value to let us group everything from cset[0]
 * in a different directory from stuff from cset[1].  this might be
 * simply "0" and "1" or it might be the cset's HIDs.
 *
 * szGidObject is the gid of the object.
 *
 * szHidBlob is the HID of the content.  Normally this is the content
 * of the file that will be compared (corresponding to a user-file
 * under version control).  However, we may also want to use this
 * to splat an arbitrary blob to a file so that they can be compared
 * -- but this may be too weird.
 *
 * pszNameForSuffix is optional; if set we try to get a suffix
 * from it for the temp file.
 * 
 * You are responsible for freeing the returned pathname and
 * deleting the file that we create.
 */
void sg_vv2__diff__export_to_temp_file(SG_context * pCtx,
									   sg_vv6diff__diff_to_stream_data * pData,
									   const char * szVersion,	// an index (like 0 or 1, _older_ _newer_) or maybe cset HID
									   const char * szGidObject,
									   const char * szHidBlob,
									   const char * pszNameForSuffix,
									   SG_pathname ** ppPathTempFile)
{
	SG_pathname * pPathFile = NULL;
	SG_file * pFile = NULL;
	SG_bool bDirExists;
	SG_string * pStr = NULL;

	// create pathname for the temp
	// file: <tempdir>/version/gid7
	//   or: <tempdir>/version/gid7~name.suffix

	SG_ERR_CHECK(  sg_vv2__diff__create_session_temp_dir(pCtx, pData)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pData->pPathSessionTempDir,szVersion)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx,pPathFile,&bDirExists,NULL,NULL)  );
	if (!bDirExists)
		SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathFile)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStr)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStr, "%.7s~", szGidObject)  );
	if (pszNameForSuffix)
	{
		// if given a path, extract the final filename.
		char * pSlash = strrchr(pszNameForSuffix, '/');
		if (pSlash)
			pszNameForSuffix = pSlash + 1;

		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStr, pszNameForSuffix)  );
	}
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathFile, SG_string__sz(pStr))  );
	SG_STRING_NULLFREE(pCtx, pStr);

	// open the file and copy the contents of the blob into it

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx,pPathFile,SG_FILE_WRONLY|SG_FILE_CREATE_NEW,0600,&pFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx,pData->pRepo,szHidBlob,pFile,NULL)  );
	SG_ERR_CHECK(  SG_file__close(pCtx,&pFile)  );
	SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPathFile, 0400)  );

	*ppPathTempFile = pPathFile;
	return;

fail:
	if (pFile)				// only if **WE** created the file, do we try to delete it on an error.
	{
		SG_FILE_NULLCLOSE(pCtx,pFile);
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx,pPathFile)  );
	}
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_STRING_NULLFREE(pCtx, pStr);
}

//////////////////////////////////////////////////////////////////

/**
 * Create an empty temp file to provide difftools with something
 * to reference.  This has a pathname that is compatible with the
 * exported format.
 *
 */
void sg_vv2__diff__create_temp_file(SG_context * pCtx,
									sg_vv6diff__diff_to_stream_data * pData,
									const char * szVersion,	// an index (like 0 or 1, _older_ _newer_) or maybe cset HID
									const char * szGidObject,
									const char * pszNameForSuffix,
									SG_pathname ** ppPathTempFile)
{
	SG_pathname * pPathFile = NULL;
	SG_file * pFile = NULL;
	SG_bool bDirExists;
	SG_string * pStr = NULL;

	// create pathname for the temp
	// file: <tempdir>/version/gid7
	//   or: <tempdir>/version/gid7~name.suffix

	SG_ERR_CHECK(  sg_vv2__diff__create_session_temp_dir(pCtx, pData)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pData->pPathSessionTempDir,szVersion)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx,pPathFile,&bDirExists,NULL,NULL)  );
	if (!bDirExists)
		SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathFile)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStr)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStr, "%.7s~", szGidObject)  );
	if (pszNameForSuffix)
	{
		// if given a path, extract the final filename.
		char * pSlash = strrchr(pszNameForSuffix, '/');
		if (pSlash)
			pszNameForSuffix = pSlash + 1;

		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStr, pszNameForSuffix)  );
	}
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathFile, SG_string__sz(pStr))  );
	SG_STRING_NULLFREE(pCtx, pStr);

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_RDWR | SG_FILE_CREATE_NEW, 0600, &pFile) );
	SG_ERR_CHECK(  SG_file__close(pCtx,&pFile)  );
	SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPathFile, 0400)  );

	*ppPathTempFile = pPathFile;
	return;

fail:
	if (pFile)				// only if **WE** created the file, do we try to delete it on an error.
	{
		SG_FILE_NULLCLOSE(pCtx,pFile);
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx,pPathFile)  );
	}
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_STRING_NULLFREE(pCtx, pStr);
}

