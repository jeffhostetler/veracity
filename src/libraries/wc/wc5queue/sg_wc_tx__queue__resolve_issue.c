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
 * Queue a request to resolve (or partially resolve) an issue.
 * We assume that the caller has already
 * queued any necessary WD operations
 * (such as moves/renamed) and now just
 * needs to mark the item resolved (or
 * partially resolved).
 *
 * Since an item may have multiple conflict
 * bits set, you can use this to mark the
 * specific choices made in the pvhSavedResolutions
 * and only mark it fully resolved when
 * everything has been chosen.
 *
 * We steal the optional/given pvhSavedResolutions.
 *
 */
void sg_wc_tx__queue__resolve_issue__sr(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI,
										SG_wc_status_flags statusFlags_x_xr_xu,
										SG_vhash ** ppvhSavedResolutions)
{
	SG_string * pStringResolveData = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI );
	// ppvhSavedResolutions is optional.

	if (ppvhSavedResolutions && *ppvhSavedResolutions)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringResolveData)  );
		SG_ERR_CHECK(  SG_vhash__to_json(pCtx, (*ppvhSavedResolutions), pStringResolveData)  );
	}

	SG_ERR_CHECK(  sg_wc_tx__journal__resolve_issue__sr(pCtx, pWcTx,
														pLVI->uiAliasGid,
														statusFlags_x_xr_xu,
														pStringResolveData)  );

	pLVI->statusFlags_x_xr_xu = statusFlags_x_xr_xu;

	SG_VHASH_NULLFREE(pCtx, pLVI->pvhSavedResolutions);
	if (ppvhSavedResolutions && *ppvhSavedResolutions)
	{
		pLVI->pvhSavedResolutions = *ppvhSavedResolutions;
		*ppvhSavedResolutions = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringResolveData);
}

/**
 * Queue a request to resolve an issue, but only set the overall status.
 * Do not alter the current (or queued) resolve-info.
 *
 * THIS IS DIFFERENT FROM CALLING THE ABOVE __queue__resolve_issue__sr(...,NULL)
 *
 */
void sg_wc_tx__queue__resolve_issue__s(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   sg_wc_liveview_item * pLVI,
									   SG_wc_status_flags statusFlags_x_xr_xu)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI );

	SG_ERR_CHECK(  sg_wc_tx__journal__resolve_issue__s(pCtx, pWcTx,
													   pLVI->uiAliasGid,
													   statusFlags_x_xr_xu)  );

	pLVI->statusFlags_x_xr_xu = statusFlags_x_xr_xu;

fail:
	return;
}

