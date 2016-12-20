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
 * Queue a request to CREATE an ISSUE.
 * 
 * This is used by MERGE to report that an item has some
 * kind of conflict.
 *
 * We don't actually have to do anything to the disk, just
 * record the data in the tbl_issue table.
 *
 * The main reason for this routine is so that we can
 * maintain/sync the cached values in the LVI.
 *
 *
 * We STEAL the callers VHASHes.
 *
 *
 * TODO 2012/04/19 MERGE was originally written to only store
 * TODO            (pvhIssue, wcIssueStatus) in the DB (and
 * TODO            always write NULL for pvhSavedResolutions).
 * TODO            This means that if we only have a file edit
 * TODO            conflict and MERGE was able auto-merge it,
 * TODO            we encode that info in the first 2 fields
 * TODO            but don't write an resolution data.
 * TODO            Meaning that RESOLVE needed to recognize
 * TODO            that the stuff in pvhSavedResolutions 
 * TODO            is incomplete or needs to be seeded from
 * TODO            info in pvhIssue.
 * TODO
 * TODO            I've made the interface to this routine take
 * TODO            all 3 variables in case we want to change
 * TODO            the MERGE code to pre-seed the pvhSavedResolution
 * TODO            data when it can/could.
 *
 * TODO 2012/08/23 I'm in the middle of changing the tbl_issue table
 * TODO            to take the _x_xr_xu_ flags rather than the wcIssueStatus
 * TODO            to make it easier for RESOLVE to figure out what to do.
 * 
 */
void sg_wc_tx__queue__insert_issue(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   sg_wc_liveview_item * pLVI,
								   SG_wc_status_flags statusFlags_x_xr_xu,	// the initial __X__, __XR__, and __XU__ flags.
								   SG_vhash ** ppvhIssue,
								   SG_vhash ** ppvhSavedResolutions)
{
	SG_string * pStringIssueData = NULL;
	SG_string * pStringResolveData = NULL;
	SG_string * pStringRepoPath = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI );
	SG_NULLARGCHECK_RETURN( ppvhIssue );
	SG_NULLARGCHECK_RETURN( *ppvhIssue );
	// ppvhSavedResolutions is optional.

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pStringRepoPath)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringIssueData)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, (*ppvhIssue), pStringIssueData)  );

	if (ppvhSavedResolutions && *ppvhSavedResolutions)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringResolveData)  );
		SG_ERR_CHECK(  SG_vhash__to_json(pCtx, (*ppvhSavedResolutions), pStringResolveData)  );
	}

	SG_ERR_CHECK(  sg_wc_tx__journal__insert_issue(pCtx, pWcTx,
												   pStringRepoPath,
												   pLVI->uiAliasGid,
												   statusFlags_x_xr_xu,
												   pStringIssueData,
												   pStringResolveData)  );

	pLVI->statusFlags_x_xr_xu = statusFlags_x_xr_xu;

	SG_VHASH_NULLFREE(pCtx, pLVI->pvhIssue);
	pLVI->pvhIssue = *ppvhIssue;
	*ppvhIssue = NULL;

	SG_VHASH_NULLFREE(pCtx, pLVI->pvhSavedResolutions);
	if (ppvhSavedResolutions && *ppvhSavedResolutions)
	{
		pLVI->pvhSavedResolutions = *ppvhSavedResolutions;
		*ppvhSavedResolutions = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringIssueData);
	SG_STRING_NULLFREE(pCtx, pStringResolveData);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}
