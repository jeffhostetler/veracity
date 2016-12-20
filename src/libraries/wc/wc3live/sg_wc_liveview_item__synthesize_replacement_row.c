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
 * @file sg_wc_liveview_item__synthesize_replacement_row.c
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
 * Synthesize a LVI with UNCONTROLLED status as a clone/replacement
 * for the given row.
 *
 * This should model the steps in _clone_cb in sg_wc_liveview_dir.c
 *
 * You DO NOT own the returned row.
 *
 */
void sg_wc_liveview_item__synthesize_replacement_row(SG_context * pCtx,
													 sg_wc_liveview_item ** ppLVI_Replacement,
													 SG_wc_tx * pWcTx,
													 const sg_wc_liveview_item * pLVI_Input)
{
	sg_wc_prescan_row * pPrescanRow = NULL;		// we do not own this
	sg_wc_liveview_item * pLVI_Allocated = NULL;

	// ppLVI_Replacement is optional

	SG_ERR_CHECK(  sg_wc_prescan_row__synthesize_replacement_row(pCtx, &pPrescanRow, pWcTx,
																  pLVI_Input->pPrescanRow)  );

	SG_ERR_CHECK(  SG_WC_LIVEVIEW_ITEM__ALLOC__CLONE_FROM_PRESCAN(pCtx,
																  &pLVI_Allocated,
																  pWcTx,
																  pPrescanRow)  );
	// link new LVI into the parents LVD.
	// the LVD does not take ownership of the LVI.

	pLVI_Allocated->pLiveViewDir = pLVI_Input->pLiveViewDir;
	SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
												   pLVI_Allocated->pLiveViewDir->prb64LiveViewItems,
												   pLVI_Allocated->uiAliasGid,
												   pLVI_Allocated)  );

	// insert into cache and take ownership.

	SG_ERR_CHECK(  sg_wc_tx__cache__add_liveview_item(pCtx, pWcTx, pLVI_Allocated)  );

	if (ppLVI_Replacement)
		*ppLVI_Replacement = pLVI_Allocated;
	return;

fail:
	SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx, pLVI_Allocated);
}

