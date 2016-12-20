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
 * Queue a stand-alone chmod to an item.
 *
 * Note that since attrbits are a dynamic
 * (rather than static) property of an item,
 * we do not update the LVI ***UNLESS THE ITEM
 * IS SPARSE*** (now that we have sparse fields
 * in tbl_PC).
 *
 * So all we need to do add an entry to the
 * journal to actually do the chmod at the
 * right time.
 *
 */
void sg_wc_tx__queue__set_attrbits(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_string * pStringRepoPath,
								   sg_wc_liveview_item * pLVI,
								   SG_int64 attrbits,
								   SG_wc_status_flags xu_mask)
{
	char * pszHidCurrent = NULL;
	SG_bool bSrcIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pStringRepoPath );

	if (bSrcIsSparse)
	{
		// There is a row in tbl_PC for this item already and 
		// with the {sparse-hid,sparse-attrbits}.  Since we were
		// only asked to change the attrbits, fetch the current
		// value of the HID.
		SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx, pLVI, pWcTx, SG_FALSE,
																	&pszHidCurrent, NULL)  );
		SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__overwrite_sparse_fields(pCtx, pLVI,
																					 pszHidCurrent,
																					 attrbits)  );
	}

	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & xu_mask))
	{
		// set the corresponding _XR_ bit for each choice set in
		// the _XU_ bits that we have on the item AND THAT are
		// included in the xu_mask; leave the other choices unresolved.
		// if we resolved the last remaining choice, set the overall
		// state.
		//
		// TODO 2012/08/27 We only set the x_xr_xu flags on the issue;
		// TODO            we DO NOT update the saved-resolution info.
		// TODO            (So the saved-resolution info could be stale
		// TODO            if they did something like accepted a rename,
		// TODO            then deleted it and restored it into the
		// TODO            baseline, and then reverted-it causing this
		// TODO            restore.  Not sure if we want to change
		// TODO            something like that into a 'accept working'
		// TODO            or 'accept baseline'.  Not sure if it matters
		// TODO            or could actually happen in practice.)

		SG_wc_status_flags xu_all = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
		SG_wc_status_flags xu_exc = (xu_all & ~xu_mask);
		SG_wc_status_flags xu_inc = (xu_all &  xu_mask);
		SG_wc_status_flags xu2xr  = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags xr     = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags sf     = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
									 | xr | xu2xr | xu_exc);
		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pWcTx, pLVI, sf)  );
	}

	SG_ERR_CHECK(  sg_wc_tx__journal__set_attrbits(pCtx, pWcTx, pLVI, pStringRepoPath, attrbits)  );

fail:
	SG_NULLFREE(pCtx, pszHidCurrent);
}
