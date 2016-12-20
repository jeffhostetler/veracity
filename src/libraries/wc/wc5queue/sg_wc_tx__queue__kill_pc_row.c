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
 * I originally wanted to call this __queue__undo_add_special()
 * but that's misleading.  We are called after the REVERT portion
 * of MERGE has removed an added-by-update item.  Our goal is to
 * do post-processing on the item so that subsequent STATUS calls
 * will not report it.
 *
 * The idea is that if you do a dirty UPDATE that causes an ADDED-BY-UPDATE
 * item to be re-created and you manually 'vv remove' it, the item should
 * be gone and STATUS should still report "ADDED-by-UPDATE + REMOVED".
 * If 'vv revert --all' does the remove (or if you do a 'vv revert --all'
 * after manually removing it), STATUS should no longer report it.
 *
 * Since the actual REMOVE has already happened, all we need to do is delete
 * the row from the pc_L0 table (for subsequent STATUS commands).
 *
 */
void sg_wc_tx__queue__kill_pc_row(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  SG_uint64 uiAliasGid)
{
	sg_wc_liveview_item * pLVI = NULL;		// we do not own this.
	SG_bool bFound = SG_FALSE;

	// schedule the row to be deleted from the pc_L0 table.
	SG_ERR_CHECK(  sg_wc_tx__journal__kill_pc_row(pCtx, pWcTx, uiAliasGid)  );

	//////////////////////////////////////////////////////////////////
	// The following is only necessary to allow operations after the
	// REVERT-ALL while inside the same TX.
	//
	// Remove the cached LVI for this item from the cached LVD of the
	// parent directory.  Since a subsequent STATUS in the same TX
	// will always use the cached version of the directory (rather
	// than starting a new scan), we need to get it out of the LVD
	// so that the directory iterator won't see it.
	//
	// Note that we DO NOT need to actually delete the LVI from the
	// LVI cache (we probably could), but it can just be there unreferenced.
	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  sg_wc_tx__cache__find_liveview_item(pCtx, pWcTx, uiAliasGid, &bFound, &pLVI)  );
	SG_ASSERT(  (bFound)  );
	SG_ASSERT(  (pLVI)  );

	// We could not have gotten the root directory into this state.
	SG_ASSERT(  (pLVI != pWcTx->pLiveViewItem_Root)  );
	// So we always know that we have a parent directory.
	SG_ASSERT(  (pLVI->pLiveViewDir)  );
	SG_ASSERT(  (pLVI->pLiveViewDir->prb64LiveViewItems)  );

	SG_ERR_TRY(  SG_rbtree_ui64__remove__with_assoc(pCtx, pLVI->pLiveViewDir->prb64LiveViewItems, uiAliasGid, NULL)  );
	SG_ERR_CATCH_IGNORE( SG_ERR_NOT_FOUND );
	SG_ERR_CATCH_END;

fail:
	return;
}

