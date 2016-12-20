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
 * @file sg_wc__initialize.c
 *
 * @details Deal with creating and initializing a WD for new repo.
 * We either create a new directory or use an existing (possibly
 * non-empty) directory.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Our caller is trying to create new repo and create a WD
 * mapped to it.  The destination directory may or may not
 * have already existed on disk before we started.  If we are
 * building upon an existing directory, verify that it doesn't
 * contain any submodules because we don't yet support them.
 *
 */
static void _check_for_nested_drawer(SG_context * pCtx,
									 SG_wc_tx * pWcTx)
{
	SG_varray * pvaStatus = NULL;
	SG_string * pString_MyDrawerRepoPath = NULL;
	SG_string * pString_MatchedRepoPath = NULL;
	const char * psz_MyDrawerName = NULL;
	const char * psz_MyDrawerRepoPath = NULL;
	SG_uint32 k, nrItems;

	if (pWcTx->bWeCreated_WD || pWcTx->bWeCreated_WD_Contents)
		return;

	SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pWcTx, NULL, SG_UINT32_MAX,
									SG_FALSE, // bListUnchanged
									SG_TRUE,  // bNoIgnores
									SG_TRUE,  // bNoTSC,
									SG_FALSE, // bListSparse
									SG_TRUE,  // bListReserved
									SG_TRUE,  // bNoSort,
									&pvaStatus,
									NULL)  );
	if (!pvaStatus)
		return;

	// TODO 2012/11/13 For now I'm just going to see if there is a
	// TODO            .sgdrawer somewhere within the directory tree.
	// TODO            In theory, we could have ADD/ADDREMOVE just
	// TODO            look for them and refuse to add its parent
	// TODO            directory, but I don't to even support that
	// TODO            until we've properly dealt with submodules.
	// TODO
	// TODO            So for now, if there is a WD deeply nested within
	// TODO            this directory, we just complain.  This is mainly
	// TODO            to prevent accidents.  (Because they can still 
	// TODO            manually move a sub-WD somehere deep into this
	// TODO            directory at some point in the future.)

	SG_ERR_CHECK(  SG_workingdir__get_drawer_directory_name(pCtx, &psz_MyDrawerName)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_MyDrawerRepoPath)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString_MyDrawerRepoPath, "@/%s", psz_MyDrawerName)  );
	SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pString_MyDrawerRepoPath)  );
	psz_MyDrawerRepoPath = SG_string__sz(pString_MyDrawerRepoPath);

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus, &nrItems)  );
	for (k=0; k<nrItems; k++)
	{
		SG_vhash * pvhItem;
		SG_vhash * pvhItemStatus;
		SG_bool bIsReserved;
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaStatus, k, &pvhItem)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItemStatus, "isReserved", &bIsReserved)  );
		if (bIsReserved)
		{
			// Don't freak out over the .sgdrawer that we just created in the root.
			const char * pszRepoPath;
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszRepoPath)  );
			if (strcmp(pszRepoPath, psz_MyDrawerRepoPath) != 0)
			{
				SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pString_MatchedRepoPath, pszRepoPath)  );
				SG_ERR_CHECK(  SG_repopath__remove_last(pCtx, pString_MatchedRepoPath)  );

				SG_ERR_THROW2(  SG_ERR_ENTRY_ALREADY_UNDER_VERSION_CONTROL,
								(pCtx, "The directory '%s' contains a working copy and submodules are not yet supported.",
								 SG_string__sz(pString_MatchedRepoPath))  );
			}
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, pString_MatchedRepoPath);
	SG_STRING_NULLFREE(pCtx, pString_MyDrawerRepoPath);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

//////////////////////////////////////////////////////////////////

/**
 * Initialize a WD (create .sgdrawer and friends) and set up shop.
 * The given directory pathname can either be to a new (to be created)
 * directory -or- an existing (possibly non-empty) directory.
 * 
 * WARNING: This routine deviates from the model of the other SG_wc__
 * WARNING: routines because the WD does not yet exist (and we haven't
 * WARNING: yet created .sgdrawer).  The caller may have already created
 * WARNING: an empty directory for us, but that is it.
 *
 * WARNING: This routine also deviates in that we *ONLY* provide a wc8api
 * WARNING: version and *NOT* a wc7txapi version.  Likewise we only provide
 * WARNING: a sg.wc.initialize() version and not a sg_wc_tx.initialize() version.
 * WARNING: This is because I want it to be a complete self-contained
 * WARNING: operation (and I want to hide the number of SQL TXs that are
 * WARNING: required to get everything set up).
 *
 * Also, the given path (to the new WD root) can be an absolute or
 * relative path or NULL (we'll substitute the CWD).  Unlike the other
 * API routines, it cannot be a repo-path.
 *
 * This should only be used by 'vv init' or sg.vv2.init_new_repo()
 * (after the repo has been created).
 * 
 */
void SG_wc__initialize(SG_context * pCtx,
					   const char * pszRepoName,
					   const char * pszPath,
					   const char * pszHidCSet,
					   const char * pszTargetBranchName)
{
	SG_wc_tx * pWcTx = NULL;

	SG_NONEMPTYCHECK_RETURN( pszRepoName );		// repo-name is required
	// pszPath is optional; default to CWD.
	SG_NONEMPTYCHECK_RETURN( pszHidCSet );
	// pszTargetBranchName is optional; default to unattached.

	// Create the WD root, the drawer, and the SQL DB.
	// Load the given CSET into the tne_L0 table.
	// 
	// We DO NOT create a timestamp cache at this time
	// because of clock blurr problem.  (We can't trust
	// mtime until about 2 seconds after we create the
	// files.)
	
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN__CREATE(pCtx, &pWcTx, pszRepoName, pszPath,
												  SG_FALSE, pszHidCSet)  );
	if (pszTargetBranchName)
	{
		SG_ERR_CHECK(  sg_wc_db__branch__attach(pCtx, pWcTx->pDb, pszTargetBranchName,
												SG_VC_BRANCHES__CHECK_ATTACH_NAME__DONT_CARE, SG_TRUE)  );
	}

#if defined(DEBUG)
#define DEBUG_INIT_FAILURE__E2 "gb54c5ca2cfe349eaa3ce599100d1c4d737052960d1e411e1a51e002500da2b78.test"
	{
		// Force a failure while we are initializing a new repo using the current directory in
		// order to test the cleanup/recovery code.
		// See st_wc_checkout_W7885.js
		SG_bool bExists = SG_FALSE;
		SG_pathname * pPathTest = NULL;
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTest, DEBUG_INIT_FAILURE__E2)  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathTest, &bExists, NULL, NULL)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathTest);
		if (bExists)
			SG_ERR_THROW2(  SG_ERR_DEBUG_1, (pCtx, "Init: %s", DEBUG_INIT_FAILURE__E2)  );
	}
#endif

	SG_ERR_CHECK(  _check_for_nested_drawer(pCtx, pWcTx)  );

	SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
	SG_ERR_CHECK(  SG_wc_tx__free(pCtx, pWcTx)  );
	return;

fail:
	SG_ERR_IGNORE(  SG_wc_tx__abort_create_and_free(pCtx, &pWcTx)  );
}

