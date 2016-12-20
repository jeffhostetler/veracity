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

static void _my_get_destination_lvi(SG_context * pCtx,
									SG_mrg * pMrg,
									SG_mrg_cset_entry * pMrgCSetEntry,
									sg_wc_liveview_item ** ppLVI)
{
	sg_wc_liveview_item * pLVI_Parent = NULL;	// we do not own this
	sg_wc_liveview_item * pLVI_k = NULL;		// we do not own this
	sg_wc_liveview_dir * pLVD_Parent = NULL;	// we do not own this
	SG_vector * pVecItemsFound = NULL;
	SG_uint64 uiAliasParent = 0;
	SG_uint32 nrItemsFound = 0;
	SG_bool bKnownParent = SG_FALSE;

	*ppLVI = NULL;

	SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pMrg->pWcTx->pDb,
													 pMrgCSetEntry->bufGid_Parent,
													 &uiAliasParent)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx, uiAliasParent,
														 &bKnownParent, &pLVI_Parent)  );
	if (bKnownParent && pLVI_Parent)
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pMrg->pWcTx, pLVI_Parent, &pLVD_Parent)  );
		SG_ERR_CHECK(  sg_wc_liveview_dir__find_item_by_entryname(pCtx, pLVD_Parent,
																  SG_string__sz(pMrgCSetEntry->pStringEntryname),
																  (SG_WC_LIVEVIEW_DIR__FIND_FLAGS__UNCONTROLLED
																   //|SG_WC_LIVEVIEW_DIR__FIND_FLAGS__RESERVED
																   |SG_WC_LIVEVIEW_DIR__FIND_FLAGS__MATCHED),
																  &pVecItemsFound)  );
		SG_ERR_CHECK(  SG_vector__length(pCtx, pVecItemsFound, &nrItemsFound)  );
		if (nrItemsFound > 0)
		{
			SG_ERR_CHECK(  SG_vector__get(pCtx, pVecItemsFound, 0, (void **)&pLVI_k)  );
			*ppLVI = pLVI_k;
		}
	}

fail:
	SG_VECTOR_NULLFREE(pCtx, pVecItemsFound);
}

/**
 * Try to unpark this item by moving/renaming it from the temporary
 * location to the final destination.  This happens very late in the
 * REVERT-ALL procedure.
 *
 * The call to RENAME can fail because the destination already exists.
 * The code in sg_wc_tx__rp__move_rename.c can throw this for any number
 * of reasons, in theory, but the MERGE code has already pre-screened
 * most of them.  So (I think) the only cases we'll ACTUALLY see are for:
 * 
 * [1] FOUND/IGNORED items at the start of the REVERT-ALL;
 * [2] ADDED items (at the start of the REVERT-ALL) that have been
 *     un-added/kept by this REVERT-ALL and are now just FOUND (like [1]);
 *
 *
 * Use the (parent_directory, item_entryname) to try to identify
 * the thing that is in the way.  (And verify the above assumptions.)
 * Use this info to build a polite error message or offer various FORCE
 * options.
 *
 */
static void _my_move_rename__from_revert(SG_context * pCtx,
										 SG_mrg * pMrg,
										 SG_mrg_cset_entry * pMrgCSetEntry,
										 const SG_string * pString1,
										 const SG_string * pString2)
{
	sg_wc_liveview_item * pLVI = NULL;		// we do not own this
	SG_string * pStringRepoPath = NULL;
	SG_wc_status_flags flags = SG_WC_STATUS_FLAGS__ZERO;

	SG_wc_tx__move_rename2(pCtx, pMrg->pWcTx,
						   SG_string__sz(pString1),
						   SG_string__sz(pString2),
						   SG_string__sz(pMrgCSetEntry->pStringEntryname),
						   SG_FALSE);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
		return;
	if (!SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS))
		SG_ERR_RETHROW;

	// Lookup the LVI of the thing that is in the way.
	// We push an error-level so that if it fails, we
	// can just return the original error message.
	SG_ERR_IGNORE(  _my_get_destination_lvi(pCtx, pMrg, pMrgCSetEntry, &pLVI)  );
	if (!pLVI)
		SG_ERR_RETHROW;

	SG_context__err_reset(pCtx);
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pMrg->pWcTx, pLVI, &pStringRepoPath)  );
	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pMrg->pWcTx, pLVI, SG_FALSE, SG_FALSE, &flags)  );

#if 0 && defined(DEBUG)
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "RevertAll:Unpark: target is 0x%s %s\n",
								   SG_uint64_to_sz__hex(flags, bufui64),
								   SG_string__sz(pStringRepoPath))  );
	}
#endif

	if (flags & (SG_WC_STATUS_FLAGS__U__FOUND|SG_WC_STATUS_FLAGS__U__IGNORED))
	{
		// TODO 2012/07/31 support varios --force options to get rid/move the
		// TODO            uncontrolled item and try again.
		// TODO            See W6339 and W9411.

		SG_ERR_THROW2(  SG_ERR_WC_ITEM_BLOCKS_REVERT,
						(pCtx, "%s", SG_string__sz(pStringRepoPath))  );
	}
	else
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "RevertALL:Unpark target has unexpected status 0x%s %s",
						 SG_uint64_to_sz__hex(flags, bufui64),
						 SG_string__sz(pStringRepoPath))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

static void _my_move_rename(SG_context * pCtx,
							SG_mrg * pMrg,
							SG_mrg_cset_entry * pMrgCSetEntry,
							const SG_string * pString1,
							const SG_string * pString2)
{
	switch (pMrg->eFakeMergeMode)
	{
	case SG_FAKE_MERGE_MODE__MERGE:
	case SG_FAKE_MERGE_MODE__UPDATE:
		// TODO 2012/07/31 Do UPDATE and MERGE need a fall-back/force mode ?
		SG_ERR_CHECK_RETURN(  SG_wc_tx__move_rename2(pCtx,
													 pMrg->pWcTx,
													 SG_string__sz(pString1),
													 SG_string__sz(pString2),
													 SG_string__sz(pMrgCSetEntry->pStringEntryname),
													 SG_FALSE)  );
		return;

	case SG_FAKE_MERGE_MODE__REVERT:
		SG_ERR_CHECK_RETURN(  _my_move_rename__from_revert(pCtx, pMrg, pMrgCSetEntry, pString1, pString2)  );
		return;

	default:
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
	}
}

//////////////////////////////////////////////////////////////////

/**
 * We get called for each entry in the final-result.
 * We get called in RANDOM GID order.
 *
 * It the item was parked, unpark it.
 *
 */
static SG_rbtree_foreach_callback _sg_mrg__plan__unpark__cb;

static void _sg_mrg__plan__unpark__cb(SG_context * pCtx,
									  const char * pszKey_GidEntry,
									  void * pVoidValue_MrgCSetEntry,
									  void * pVoidData_Mrg)
{
	SG_mrg_cset_entry * pMrgCSetEntry = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	SG_mrg * pMrg = (SG_mrg *)pVoidData_Mrg;
	SG_string * pString1 = NULL;
	SG_string * pString2 = NULL;

	SG_UNUSED( pszKey_GidEntry );

	SG_ASSERT(  (pMrgCSetEntry->markerValue &
				 _SG_MRG__PLAN__MARKER__FLAGS__APPLIED)  );

	if ((pMrgCSetEntry->markerValue & _SG_MRG__PLAN__MARKER__FLAGS__PARKED) == 0)
		return;

	// Use a gid-domain extended repo-paths to name the item
	// and the destination directory.
	// This lets us not care where it currently is in the tree
	// or what funky temp name we gave it.

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString1, "@%s",
											pMrgCSetEntry->bufGid_Entry)  );
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString2, "@%s",
											pMrgCSetEntry->bufGid_Parent)  );
	
	// Use layer-7 api to do the dirty work of
	// queuing up the move/rename.  This is not
	// immediate, rather it is queued/journaled.
	// Using the layer-7 TX API lets us speak
	// in extended-prefix repo-paths (using GIDs).

	SG_ERR_CHECK(  _my_move_rename(pCtx, pMrg, pMrgCSetEntry, pString1, pString2)  );

	pMrgCSetEntry->markerValue &= ~_SG_MRG__PLAN__MARKER__FLAGS__PARKED;

fail:
	SG_STRING_NULLFREE(pCtx, pString1);
	SG_STRING_NULLFREE(pCtx, pString2);
}

//////////////////////////////////////////////////////////////////

/**
 * This is the last step in "queue plan".  After we have
 * dealt with all items that need to be changed, we need
 * to "un-park" any items that were parked (because of a
 * *transient* collision on the entryname.
 *
 *
 */
void sg_wc_tx__merge__queue_plan__unpark(SG_context * pCtx,
										 SG_mrg * pMrg)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__foreach(pCtx,
											 pMrg->pMrgCSet_FinalResult->prbEntries,
											 _sg_mrg__plan__unpark__cb,
											 pMrg)  );
}
