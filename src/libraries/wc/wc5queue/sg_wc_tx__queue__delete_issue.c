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

//////////////////////////////////////////////////////////////////

/**
 * Queue a request to delete an issue from the tbl_issue table
 * and if necessary delete the associated temp dir from disk.
 *
 * This should only be used by COMMIT and maybe REVERT.
 * It SHOULD NOT be used by RESOLVE.
 *
 */
void sg_wc_tx__queue__delete_issue(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   sg_wc_liveview_item * pLVI)
{
	SG_ERR_CHECK(  sg_wc_tx__queue__delete_issue2(pCtx, pWcTx, pLVI->uiAliasGid, pLVI->pvhIssue)  );

	// I think it is best to go ahead and clear these now
	// under the assumption that the pLVI is modified to
	// reflect current reality throughout the queueing process.

	pLVI->statusFlags_x_xr_xu = SG_WC_STATUS_FLAGS__ZERO;
	SG_VHASH_NULLFREE(pCtx, pLVI->pvhIssue);
	SG_VHASH_NULLFREE(pCtx, pLVI->pvhSavedResolutions);

fail:
	return;
}

void sg_wc_tx__queue__delete_issue2(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									SG_uint64 uiAliasGid,
									const SG_vhash * pvhIssue)
{
	const char * pszGid = NULL;
	const char * pszRepoPathTempDir = NULL;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhIssue, "gid", &pszGid)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhIssue, "repopath_tempdir", &pszRepoPathTempDir)  );
		
	SG_ERR_CHECK(  sg_wc_tx__journal__delete_issue(pCtx, pWcTx, uiAliasGid, pszGid, pszRepoPathTempDir)  );

fail:
	return;
}

