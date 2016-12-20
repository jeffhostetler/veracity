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
 * @file sg_wc_tx__rp__commit__mark_bubble_up.c
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

static void _bubble_up_starting_with_dir(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 SG_uint64 uiAliasDir)
{
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_bool bKnown;

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx,
														 uiAliasDir,
														 &bKnown, &pLVI)  );
	SG_ASSERT( (bKnown) );

	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)
	{
		// short cut.  if this directory already has the
		// bubble-up bit set, then so will all of its parents.

		return;
	}

	pLVI->commitFlags |= SG_WC_COMMIT_FLAGS__BUBBLE_UP;

	if (pLVI->pLiveViewDir)
		SG_ERR_CHECK(  _bubble_up_starting_with_dir(pCtx, pWcTx,
													pLVI->pLiveViewDir->uiAliasGidDir)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Walk up the tree from the given starting point
 * and set the bubble-up bit on parent directories.
 *
 * We DO NOT set the bubble-up bit on the given LVI.
 *
 * For moved items, we bubble-up both parents.
 *
 */
void sg_wc_tx__rp__commit__mark_bubble_up__lvi(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   sg_wc_liveview_item * pLVI)
{
	SG_uint64 uiAlias_CurrentParent;
	SG_uint64 uiAlias_OriginalParent;

	uiAlias_CurrentParent = pLVI->pLiveViewDir->uiAliasGidDir;
	SG_ERR_CHECK(  _bubble_up_starting_with_dir(pCtx, pWcTx, uiAlias_CurrentParent)  );

	if (pLVI->statusFlags_commit == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_CHECK_RETURN(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
														   SG_FALSE, SG_FALSE,
														   &pLVI->statusFlags_commit)  );
	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__ADDED)
	{
	}
	else if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
	{
	}
	else if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
	{
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_parent_alias(pCtx, pLVI,
																	  &uiAlias_OriginalParent)  );
		if (uiAlias_OriginalParent != uiAlias_CurrentParent)
			SG_ERR_CHECK(  _bubble_up_starting_with_dir(pCtx, pWcTx, uiAlias_OriginalParent)  );
	}

fail:
	return;
}
