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
 * @file sg_wc_tx__apply__store_directory.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Try to store the contents of the directory to the repo.
 *
 */ 
void sg_wc_tx__apply__store_directory(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_vhash * pvh)
{
	const char * pszRepoPath;		// we do not own this
	const char * pszHidExpected;	// we do not own this
	char * pszHidObserved = NULL;
	SG_int64 alias;
	SG_bool bKnown;

	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "src",   &pszRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "alias", &alias)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "hid",   &pszHidExpected)  );
	// We don't care about "src_sparse" field because we are not
	// going to touch the WD anyway.  We trust pLVD->pTN_commit
	// to contain what we need to store.

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__store_directory: '%s'\n"),
							   pszRepoPath)  );
#endif

	if (alias == SG_WC_DB__ALIAS_GID__NULL_ROOT)
	{
		SG_ASSERT(  (strcmp(pszHidExpected, pWcTx->pszHid_superroot_content_commit) == 0)  );
		SG_ASSERT(  (pWcTx->pTN_superroot_commit)  );

		SG_ERR_CHECK(  SG_committing__tree__add_treenode(pCtx,
														 pWcTx->pCommittingInProgress,
														 &pWcTx->pTN_superroot_commit,	// this gets stolen
														 &pszHidObserved)  );
	}
	else
	{
		sg_wc_liveview_item * pLVI;		// we do not own this
		sg_wc_liveview_dir * pLVD;		// we do not own this

		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, alias, &bKnown, &pLVI)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );

		SG_ASSERT(  (strcmp(pszHidExpected, pLVI->pszHid_dir_content_commit) == 0)  );
		SG_ASSERT(  (pLVD->pTN_commit)  );

		SG_ERR_CHECK(  SG_committing__tree__add_treenode(pCtx,
														 pWcTx->pCommittingInProgress,
														 &pLVD->pTN_commit,	// this gets stolen
														 &pszHidObserved)  );
	}

	if (strcmp(pszHidObserved, pszHidExpected) != 0)
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "The directory '%s' changed during the commit.",
						 pszRepoPath)  );

fail:
	SG_NULLFREE(pCtx, pszHidObserved);
}
