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
 * @file sg_wc_tx__rp__commit__mark_subtree.c
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

static SG_rbtree_ui64_foreach_callback _mark_dive_cb;
static SG_rbtree_ui64_foreach_callback _moved_out_cb;

//////////////////////////////////////////////////////////////////

/**
 * The "moved-out" list is just a set[<aliases-gid>].
 * It does not have any assoc-data, so we have to
 * find the LVIs.
 *
 * We want to mark the LVI of the moved-item
 * as participating in the commit.
 *
 * **BUT** our main concern here is to get the effect
 * of the MOVE/RENAME on the item (and the bubble-up on
 * new parents)
 *
 * BUT we are not going to say anything about the
 * content of the item.  (If you edit and move a file,
 * and then only commit the original parent directory,
 * we might need to force the destination directory to
 * be committed (so that it owns the file), but we
 * don't want to force the modified content to be
 * committed.)
 *
 */
static void _moved_out_cb(SG_context * pCtx,
						  SG_uint64 uiAliasGid,
						  void * pVoid_Assoc,
						  void * pVoid_Data)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoid_Data;
	sg_wc_liveview_item * pLVI; 	// we do not own this
	SG_bool bKnown;

	SG_UNUSED( pVoid_Assoc );

	SG_ERR_CHECK_RETURN(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAliasGid,
																&bKnown, &pLVI)  );
	SG_ASSERT( (bKnown) );

	if (pLVI->statusFlags_commit == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_CHECK_RETURN(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
														   SG_FALSE, SG_FALSE,
														   &pLVI->statusFlags_commit)  );

	SG_ASSERT(  (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MOVED)  );

	// this means we visited the moved LVI from source side.

	pLVI->commitFlags |= SG_WC_COMMIT_FLAGS__MOVED_OUT_OF_MARKED_DIRECTORY;

	// set __BUBBLE_UP on the destination directory (and all of its parents).

	SG_ERR_CHECK_RETURN(  sg_wc_tx__rp__commit__mark_bubble_up__lvi(pCtx, pWcTx, pLVI)  );
}

//////////////////////////////////////////////////////////////////

struct _mark_dive_data
{
	SG_wc_tx *			pWcTx;
	SG_uint32			depth;
	SG_wc_status_flags	statusFlagsParticipatingUnion;
};

static void _mark_dive_cb(SG_context * pCtx,
						  SG_uint64 uiAliasGid,
						  void * pVoid_LVI,
						  void * pVoid_Data)
{
	struct _mark_dive_data * pDiveData = (struct _mark_dive_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;

	SG_UNUSED( uiAliasGid );

	SG_ERR_CHECK_RETURN(  sg_wc_tx__rp__commit__mark_subtree__lvi(pCtx,
																  pDiveData->pWcTx,
																  pLVI,
																  pDiveData->depth)  );

	pDiveData->statusFlagsParticipatingUnion |= pLVI->statusFlags_commit;
}

/**
 * Mark the given LVI as "participating in the commit"
 * and dive if necessary.
 *
 * Our ONLY task here is to mark the LVIs that
 * are candidates for inclusion in the changeset.
 * Because the user can give us an argv[] of
 * starting points (rather than a single "@/")
 * and because of moved items, we need to first
 * mark the whole tree *before* we try to do any
 * analysis/filtering.  After we get the complete
 * potential set of items, a later pass will
 * queue them and consider any inconsistencies.
 *
 */
void sg_wc_tx__rp__commit__mark_subtree__lvi(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 sg_wc_liveview_item * pLVI,
											 SG_uint32 depth)
{
	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		// We NEVER commit RESERVED items.
		// They should not have been able to ADD it in the first place,
		// so we may never get here.
#if TRACE_WC_COMMIT
		{
			SG_int_to_string_buffer bufui64_1;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "MarkSubtree: skip reserved [alias %s] [scan %d] '%s'\n",
									   SG_uint64_to_sz(pLVI->uiAliasGid, bufui64_1),
									   pLVI->scan_flags_Live,
									   SG_string__sz(pLVI->pStringEntryname))  );
		}
#endif
		return;
	}

	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		// This pLVI represents a FOUND/IGNORED item within
		// a participating directory.  Silently eat it.
		// That is, we don't mark FOUND/IGNORED items as
		// participating and we don't need to dive into them
		// *AND* we don't want to complain about them.
#if TRACE_WC_COMMIT
		{
			SG_int_to_string_buffer bufui64_1;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "MarkSubtree: skip uncontrolled [alias %s] [scan %d] '%s'\n",
									   SG_uint64_to_sz(pLVI->uiAliasGid, bufui64_1),
									   pLVI->scan_flags_Live,
									   SG_string__sz(pLVI->pStringEntryname))  );
		}
#endif
		return;
	}

	if (pLVI->statusFlags_commit == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_CHECK_RETURN(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
														   SG_FALSE, SG_FALSE,
														   &pLVI->statusFlags_commit)  );

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MOVED)
	{
		// the LVI has been moved INTO this directory.

		pLVI->commitFlags |= SG_WC_COMMIT_FLAGS__MOVED_INTO_MARKED_DIRECTORY;

		// do a bubble-up on the source directory.

		SG_ERR_CHECK_RETURN(  sg_wc_tx__rp__commit__mark_bubble_up__lvi(pCtx, pWcTx, pLVI)  );
	}

	// We visited this LVI during a top-down tree walk
	// (either starting from "@/" or from one of the
	// <files_or_folders> given in argv[].  We are not
	// saying that this LVI is dirty, just that we visited
	// it during the normal walk.

#if TRACE_WC_COMMIT
	{
		SG_int_to_string_buffer bufui64_1;
		SG_int_to_string_buffer bufui64_2;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "MarkSubtree: marking 'participating' on [alias %s] [scan %d] [flags %s] '%s'\n",
								   SG_uint64_to_sz(pLVI->uiAliasGid, bufui64_1),
								   pLVI->scan_flags_Live,
								   SG_uint64_to_sz__hex(pLVI->statusFlags_commit, bufui64_2),
								   SG_string__sz(pLVI->pStringEntryname))  );
	}
#endif
	pLVI->commitFlags |= SG_WC_COMMIT_FLAGS__PARTICIPATING;

	if (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		if (depth > 0)
		{
			sg_wc_liveview_dir * pLVD;		// we do not own this
			SG_uint32 nrMoveOuts = 0;
			struct _mark_dive_data mark_dive_data;

			// Note that we DO NOT record the actual depth in
			// the LVI/LVD because this directory could be visited
			// multiple times (if more than one argv[] item caused
			// us to tree walk into it and at different depths).
			// The only thing that matters is whether we considered
			// visiting the children (which we do when depth > 0).

			pLVI->commitFlags |= SG_WC_COMMIT_FLAGS__DIR_CONTENTS_PARTICIPATING;

			mark_dive_data.pWcTx = pWcTx;
			mark_dive_data.depth = depth - 1;
			mark_dive_data.statusFlagsParticipatingUnion = SG_WC_STATUS_FLAGS__ZERO;

			SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );

			if (pLVD->prb64LiveViewItems)
			{
				// mark the immediate/current children of this
				// directory as participating too.

				SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx,
														   pLVD,
														   _mark_dive_cb,
														   &mark_dive_data)  );

				// Remember the union of all of the statusFlags of
				// the immediate children of this directory.  By
				// definition they are participating too.  This
				// serves 2 purposes (for the QUEUE phase):
				// [] It gives us an idea if the content of the
				//    directory is actually dirty (when combined
				//    with the nrMoveOuts) below.
				// [] Because this is only set/computed when (depth>0)
				//    it will let the QUEUE code know to not bother
				//    looking at the contents within the directory.
				//
				//    Note that if this directory has (depth==0),
				//    it may still have a moved-out/in item and
				//    need to be treated like a plain bubble-up
				//    directory.
				//
				// Note that this union CANNOT tell us if any
				// sub-directories within this directory have
				// dirty stuff within them.

				pLVD->statusFlagsParticipatingUnion_commit = mark_dive_data.statusFlagsParticipatingUnion;
			}
		
			if (pLVD->prb64MovedOutItems)
			{
				// to make a partial commit of a directory not be a pain for
				// the user to request, also/implicitly mark any MOVED-OUT
				// items (and bubble-up their new parents).  This DOES NOT
				// set the __PARTICIPATING bit on the moved items; we only
				// want to set enough on them to get their TNEs moved out
				// of this directory and attached to the TNs in their
				// respective destination directories.
				//
				// Another way to look at this is that the destination
				// directory is responsible for setting the child's
				// participation bit (as part of a tree-walk).
				//
				// All we are saying here is that the source directory is
				// participating and has been visited.

				SG_ERR_CHECK(  SG_rbtree__count(pCtx,
												// SG_rbtree_ui64 is "subclass" of SG_rbtree
												(SG_rbtree *)pLVD->prb64MovedOutItems,
												&nrMoveOuts)  );
				if (nrMoveOuts > 0)
					SG_ERR_CHECK(  sg_wc_liveview_dir__foreach_moved_out(pCtx,
																		 pLVD,
																		 _moved_out_cb,
																		 pWcTx)  );
			}
		}
	}

	// We DO NOT need to bubble up on the parents of this item
	// since it will have the __PARTICIPATING bit set.

fail:
	return;
}
