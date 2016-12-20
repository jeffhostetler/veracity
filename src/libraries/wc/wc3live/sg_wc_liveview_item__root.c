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
 * @file sg_wc_liveview_item__root.c
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
 * Alloc a special LiveViewItem for the root directory; the one
 * corresponding to "@/".  This is an *item* representing the
 * directory; it is NOT concerned with the content *within* the
 * directory.
 *
 * The LVI is added to the TX cache; so you DO NOT own the
 * returned pointer.
 *
 */
void sg_wc_liveview_item__alloc__root_item(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   sg_wc_liveview_item ** ppLVI)
{
	sg_wc_prescan_row * pPrescanRow_Root;		// we do not own this
	sg_wc_liveview_item * pLVI_Allocated = NULL;

	// we assume that __ALIAS_GID__{UNDEFINED,NULL_ROOT} are the
	// first aliases created and therefore the alias for "@/" must
	// be larger than that.
	SG_ASSERT_RELEASE_RETURN(  (pWcTx->uiAliasGid_Root > SG_WC_DB__ALIAS_GID__NULL_ROOT)   );

	if (pWcTx->pLiveViewItem_Root)
	{
		*ppLVI = pWcTx->pLiveViewItem_Root;
		return;
	}

	SG_ERR_CHECK(  sg_wc_tx__prescan__fetch_row(pCtx, pWcTx, pWcTx->uiAliasGid_Root,
												&pPrescanRow_Root)  );
	SG_ERR_CHECK(  SG_WC_LIVEVIEW_ITEM__ALLOC__CLONE_FROM_PRESCAN(pCtx, &pLVI_Allocated, pWcTx, pPrescanRow_Root)  );

	// the root directory doesn't have a parent directory.
	pLVI_Allocated->pLiveViewDir = NULL;

	// insert into cache and take ownership.
	SG_ERR_CHECK(  sg_wc_tx__cache__add_liveview_item(pCtx, pWcTx, pLVI_Allocated)  );
	*ppLVI = pLVI_Allocated;
	return;

fail:
	SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx, pLVI_Allocated);
}
