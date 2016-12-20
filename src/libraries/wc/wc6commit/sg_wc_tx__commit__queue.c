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
 * @file sg_wc_tx__commit__queue.c
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

static SG_rbtree_ui64_foreach_callback _commit_queue__dive_cb;

static void _commit_queue__dive_cb(SG_context * pCtx,
								   SG_uint64 uiAliasGid,
								   void * pVoid_LVI,
								   void * pVoid_Data)
{
	sg_commit_data * pCommitData = (sg_commit_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;

	SG_UNUSED( uiAliasGid );

	SG_ERR_CHECK_RETURN(  sg_wc_tx__commit__queue__lvi(pCtx, pCommitData, pLVI)  );
}

static void _commit_queue__dive(SG_context * pCtx,
								sg_commit_data * pCommitData,
								sg_wc_liveview_item * pLVI)
{
	sg_wc_liveview_dir * pLVD;		// we don't own this

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pCommitData->pWcTx, pLVI, &pLVD)  );
	if (pLVD->prb64LiveViewItems)
		SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _commit_queue__dive_cb, pCommitData)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * When we queue an item (LVI) we have to think about this in parts:
 * 
 * [a] the HID of the CONTENT    -- a blob
 * [b] the item's structural changes (moves/renames/etc)
 *
 * We can handle [a] directly when visiting the LVI.
 * But stuff for [b] are kept inside the parent directory,
 * so these are deferred until the parent is processed.
 *
 */
void sg_wc_tx__commit__queue__lvi(SG_context * pCtx,
								  sg_commit_data * pCommitData,
								  sg_wc_liveview_item * pLVI)
{
	SG_string * pStringRepoPath = NULL;
//	SG_bool bMovedIn, bMovedOut;

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx,
															  pCommitData->pWcTx,
															  pLVI,
															  &pStringRepoPath)  );

	// For moved items, we should have visited both sides
	// of the move.  We don't necessarily care if the item
	// is participating in the commit, but if we touched
	// one side, we need to have touched the other side.
//
//	bMovedIn  = ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__MOVED_INTO_MARKED_DIRECTORY) != 0);
//	bMovedOut = ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__MOVED_OUT_OF_MARKED_DIRECTORY) != 0);
//	if (bMovedIn != bMovedOut)
//	{
//		SG_ERR_THROW2(  SG_ERR_ASSERT,
//						(pCtx, "Partial commit of MOVED item '%s' flags '0x%x'.",
//						 SG_string__sz(pStringRepoPath),
//						 pLVI->commitFlags)  );
//	}

	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
	{
		// In a partial commit: the item was explicitly named
		// or is a descendant of an explicitly named item.
		//
		// In a full commit: everything participates.
		//
		// This bit DOES NOT necessarily mean that this item
		// is dirty.
		//
		// If this item is a directory, it may ALSO have
		// __BUBBLE_UP set (we don't care).
	}
	else if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)
	{
		// In a partial commit: this item was NOT visited 
		// during the regular tree-walk of the items to be
		// committed.  This item is a:
		// [] parent directory of one of the explicitly named
		//    files/folders args.
		// [] parent directory of either half of a moved item.
		// [] parent directory of another __BUBBLE_UP item.

		SG_ASSERT( (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY) );
	}
	else
	{
		// A partial commit does not want this item
		// and we didn't need bubble-up to force it.

		goto done;
	}

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__R__RESERVED)
	{
		// We cannot commit a RESERVED item.  It shouldn't have
		// been marked participating nor should we have needed
		// to bubble up to it (since nothing within a reserved
		// directory should have been visited in the first place).

		goto done;
	}

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__U__LOST)
	{
		// They are trying to commit a LOST item.
		// Normally, we complain about this unless
		// they also said --allow-lost.

		if (!pCommitData->pCommitArgs->bAllowLost)
			SG_ERR_THROW2(  SG_ERR_CANNOT_COMMIT_LOST_ENTRY,
							(pCtx, "'%s' is LOST",
							 SG_string__sz(pStringRepoPath))  );

		// But even with --allow-lost, there are a
		// few cases that we need to reject (because
		// we don't have an HID on record for this
		// item).

		if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__ADDED)
			SG_ERR_THROW2(  SG_ERR_CANNOT_COMMIT_LOST_ENTRY,
							(pCtx, "'%s' is LOST+ADDED",
							 SG_string__sz(pStringRepoPath))  );

		// Everything is OK with allowing this LOST item
		// to be committed.  Since we don't have access
		// to the actual content of the item, we will
		// just assume that the (non-dir-content,
		// attrbits) are all clean.
		
		pLVI->statusFlags_commit &= ~SG_WC_STATUS_FLAGS__C__MASK;
	}

	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
	{
		if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__X__UNRESOLVED)
		{
			// Disallow the whole COMMIT if they try to commit an item
			// with an UNRESOLVED CONFLICT.  For a MERGE (where we only
			// handle complete commits so everything will be marked as
			// participating), this will stop us if anything needs
			// attention.  For an UPDATE (where we DO allow partial
			// commits), this is a little kinder and only stops us if
			// there is an unresolved item within the subset being
			// committed.

			SG_ERR_THROW2(  SG_ERR_CANNOT_COMMIT_WITH_UNRESOLVED_ISSUES,
							(pCtx, "'%s' is unresolved",
							 SG_string__sz(pStringRepoPath))  );
		}
	}

	// QUEUE (and JOURNAL) the actual update of the TNE
	// for this item.

	switch (pLVI->tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
		{
			if ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
				&& (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER))
				SG_ERR_THROW2(  SG_ERR_VC_LOCK_VIOLATION,
								(pCtx, "%s", SG_string__sz(pStringRepoPath))  );

			SG_ERR_CHECK(  sg_wc_tx__commit__queue__blob__file(pCtx, pCommitData, pStringRepoPath, pLVI)  );
		}
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
			SG_ERR_CHECK(  sg_wc_tx__commit__queue__blob__symlink(pCtx, pCommitData, pStringRepoPath, pLVI)  );
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		SG_ERR_CHECK(  _commit_queue__dive(pCtx, pCommitData, pLVI)  );

		if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__DIR_CONTENTS_PARTICIPATING)
			SG_ERR_CHECK(  sg_wc_tx__commit__queue__blob__dir_contents_participating(pCtx, pCommitData, pStringRepoPath, pLVI)  );
		else if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)
			SG_ERR_CHECK(  sg_wc_tx__commit__queue__blob__bubble_up_directory(pCtx, pCommitData, pStringRepoPath, pLVI)  );
		else if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
			SG_ERR_CHECK(  sg_wc_tx__commit__queue__blob__dir_non_recursive(pCtx, pCommitData, pStringRepoPath, pLVI)  );
		break;

	default:
		// subrepo
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Unknown tneType for item '%s'",
						 SG_string__sz(pStringRepoPath))  );
	}

	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
	{
		if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__X__RESOLVED)
		{
			// When committing something that was resolved, we want to
			// delete the pvhIssue data associated with it.  (Since it
			// will no longer apply after the commit.)  Historically,
			// we could just drop the tbl_issue table at the end of commit
			// (and that was fine for regular merges which are always
			// complete). But now that UPDATE can generate issue data
			// and since we allow partial-commits, we need to be a bit
			// more selective.  So we now queue the delete of the row
			// from tbl_issue (and if necessary the delete of the
			// associated temp dir) on an item-by-item basis.

			SG_ERR_CHECK(  sg_wc_tx__queue__delete_issue(pCtx, pCommitData->pWcTx, pLVI)  );
			SG_ASSERT(  (pLVI->pvhIssue == NULL)  );
			SG_ASSERT(  (pLVI->statusFlags_x_xr_xu == SG_WC_STATUS_FLAGS__ZERO)  );
			SG_ASSERT(  (pLVI->pvhSavedResolutions == NULL)  );
		}
	}

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * Do a DEPTH-FIRST walk of the tree (controlled by
 * the commitFlags on each LVI) and QUEUE operations
 * to the JOURNAL.
 *
 */
void sg_wc_tx__commit__queue(SG_context * pCtx,
							 sg_commit_data * pCommitData)
{
	SG_ERR_CHECK(  sg_wc_tx__commit__queue__lvi(pCtx,
												pCommitData,
												pCommitData->pWcTx->pLiveViewItem_Root)  );
	SG_ERR_CHECK(  sg_wc_tx__commit__queue__blob__dir_superroot(pCtx,
																pCommitData)  );

fail:
	return;
}
