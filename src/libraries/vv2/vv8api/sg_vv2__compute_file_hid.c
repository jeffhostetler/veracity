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

/**
 *
 * @file sg_vv2__compute_file_hid.c
 *
 * @details Compute the HID of a file on disk using the hash
 * algorithm associated with the named repo.  We DO NOT need a
 * working directory to do this since the hash algorithm is a
 * property of the repo not the WD.
 *
 * If the given repo name is null, we try to get it from the WD
 * if present.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void SG_vv2__compute_file_hid__repo(SG_context * pCtx,
									SG_repo * pRepo,
									const SG_pathname * pPathFile,
									char ** ppszHid)
{
    SG_file * pFile = NULL;
	SG_fsobj_type fsType;
	SG_bool bExists;

	SG_NULLARGCHECK_RETURN( pRepo );
	SG_NULLARGCHECK_RETURN( pPathFile );
	SG_NULLARGCHECK_RETURN( ppszHid );

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathFile, &bExists, &fsType, NULL)  );
	if (!bExists)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "%s", SG_pathname__sz(pPathFile))  );
	if (fsType != SG_FSOBJ_TYPE__REGULAR)
		SG_ERR_THROW2(  SG_ERR_NOTAFILE,
						(pCtx, "%s", SG_pathname__sz(pPathFile))  );

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile,
										   SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING,
										   SG_FSOBJ_PERMS__UNUSED,
										   &pFile)  );
    SG_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx, pRepo, pFile, ppszHid)  );

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

/**
 * Compute the HID of the given file using the hash algorithm
 * associated with the given repo.
 *
 * WE DO NOT SUPPORT ARBITRARY INPUT (pszInput) LIKE WC ROUTINES;
 * YOU HAVE TO GIVE US A REAL PATHNAME (relative or absolute) NOT
 * A REPO-PATH.
 *
 * You own the returned string.
 *
 */
void SG_vv2__compute_file_hid(SG_context * pCtx,
							  const char * pszRepoName,
							  const SG_pathname * pPathFile,
							  char ** ppszHid)
{
	SG_vhash * pvhWcInfo = NULL;
	SG_repo * pRepo = NULL;

	// pszRepoName is optional (defaults to WD if present)

	if (!pszRepoName || !*pszRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );
	}
	
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_vv2__compute_file_hid__repo(pCtx, pRepo, pPathFile, ppszHid)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

