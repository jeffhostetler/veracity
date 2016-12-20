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
 * @file sg_wc_tx__fake_merge__update__status.c
 *
 * @details UPDATE tricks MERGE into doing its homework
 * using the __fake_merge__update__ setup routines to do a one-legged
 * merge.
 *
 * In the wc.db, we have:
 * [] a row with "L0" ==> { hid_L0, tne_L0, pc_L0 } in tbl_csets;
 * [] table for the L0-associated "tne";
 * [] table for the L0-associated "pc" reflecting the delta from
 *    the "tne_L0" to the (post-merge/update) WD;
 * [] a row with "L1" ==> { hid_L1, tne_L1 } in tbl_csets;
 * [] table for the L1-associated "tne".
 *
 * UPDATE is asking us to populate a FUTURE-STATUS -- a what *would*
 * STATUS report if we actully did everything that is QUEUED.  This
 * is only needed because UPDATE will be doing a "change of basis"
 * after it finishes and so a status after update finishes will be
 * relative to the new/target changeset and we haven't done that
 * yet.  We have all the info on hand, we just need to pull it
 * together.  This needs to be a
 *      status(pMrgCSet_Other, pMrgCSet_FinalResult)
 * aka  status(L1, L1+dirt)
 *
 * Currently, the pLVI's have info for L0 in the _original_
 * fields and info for the (queued) post-update/post-merge
 * in the _current_ fields.  I'm not sure I want to muck with
 * the pLVI->scan_flags_Live, pLVI->pPrescanRow, and etc (so
 * that the _original_ routines......
 * for this.
.....
 *
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

struct _cb_data
{
	SG_mrg * pMrg;
	SG_varray * pvaStatus;
};

/**
 * We get called once for each entry in the final-result.
 * We get called in RANDOM GID order.
 *
 * Compare the item with the corresponding item in L1.
 *
 * This should be considered a specialization of sg_wc__status__compute_flags()
 * because we already know some of the answers and it uses pLVI->scan_flags_Live
 * and pLVI->flags_net which still are L0-relative.
 *
 * So we can compute the statusFlags here and then use the rest of status.
 * 
 *
 */
static SG_rbtree_foreach_callback _fm_status__final;
static void _fm_status__final(SG_context * pCtx,
							  const char * pszKey_GidEntry,
							  void * pVoidValue_MrgCSetEntry,
							  void * pVoidData)
{
	SG_mrg_cset_entry * pMrgCSetEntry_FinalResult = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	struct _cb_data * pData = (struct _cb_data *)pVoidData;
	SG_mrg * pMrg = pData->pMrg;
	SG_mrg_cset_entry * pMrgCSetEntry_Other;
	SG_mrg_cset_entry * pMrgCSetEntry_Ancestor;
	SG_string * pStringLiveRepoPath = NULL;
	char * pszHid_CurrentContent = NULL;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_bool bKnown_lvi;
	SG_bool bFound_Other;
	SG_bool bFound_Ancestor;
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	SG_uint32 nrChangeBits__Found    = 0;
	SG_uint32 nrChangeBits__Ignored  = 0;
	SG_uint32 nrChangeBits__Lost     = 0;
	SG_uint32 nrChangeBits__Added    = 0;
	SG_uint32 nrChangeBits__Deleted  = 0;
	SG_uint32 nrChangeBits__Renamed  = 0;
	SG_uint32 nrChangeBits__Moved    = 0;
	SG_uint32 nrChangeBits__Attrbits = 0;
	SG_uint32 nrChangeBits__Content  = 0;
	SG_uint32 nrChangeBits__Conflict = 0;
	const char * pszWasLabel_l = "Baseline (B)";
	const char * pszWasLabel_r = "Working";

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_fm_status: final: %s\n", pszKey_GidEntry)  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry_FinalResult->uiAliasGid,
														 &bKnown_lvi, &pLVI)  );
	SG_ASSERT( (bKnown_lvi) );

	SG_ASSERT(  (pMrgCSetEntry_FinalResult->markerValue == _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	pMrgCSetEntry_FinalResult->markerValue = _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;

	// seed statusFlags with the tne_type.
	SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx, pLVI->tneType, &statusFlags)  );

	// See if this item is present in L1.
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_Other->prbEntries, pszKey_GidEntry,
								   &bFound_Other, (void **)&pMrgCSetEntry_Other)  );
	if (bFound_Other)
	{
		SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;

		pMrgCSetEntry_Other->markerValue = _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;

		SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx, pMrgCSetEntry_Other, pMrgCSetEntry_FinalResult, &neq)  );
		if (neq == SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL)
		{
			// no status to report for this item.
			goto done;
		}
		else
		{
			if (neq & SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__RENAMED;
				nrChangeBits__Renamed = 1;
			}

			if (neq & SG_MRG_CSET_ENTRY_NEQ__GID_PARENT)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__MOVED;
				nrChangeBits__Moved = 1;
			}

			if (neq & SG_MRG_CSET_ENTRY_NEQ__ATTRBITS)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;
				nrChangeBits__Attrbits = 1;
			}

			if (neq & (SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB
					   |SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB
					   |SG_MRG_CSET_ENTRY_NEQ__SUBMODULE_HID_BLOB))
			{
				statusFlags |= SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED;
				nrChangeBits__Content = 1;
			}
			
			// note: SG_MRG_CSET_ENTRY_NEQ__DELETED isn't possible here.
		}
	}
	else
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_LCA->prbEntries, pszKey_GidEntry,
									   &bFound_Ancestor, (void **)&pMrgCSetEntry_Ancestor)  );
		if (bFound_Ancestor)
		{
			// Item present in Ancestor (aka L0).  This is only possible
			// if the item was DELETED in L1.  But from the way that MERGE
			// works, we know that if the item was unchanged in WC, then
			// MERGE would have deleted it from the final-result, so we know
			// that it must be changed and we have a DELETE-VS-* CONFLICT on
			// the item and that it must remain in the final-result.  (And we
			// won't need to set any __M__AUTO_MERGE bits.)
			//
			// Conceptually this is more like an ADD-SPECIAL where we want
			// to un-delete an item and preserve the original GID.

			statusFlags |= SG_WC_STATUS_FLAGS__S__UPDATE_CREATED;
			nrChangeBits__Added = 1;
		}
		else
		{
			// Item represents dirt present in the WC before the merge.
			// This could be ADDED or FOUND/IGNORED.

			if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
			{
				SG_bool bIgnorable;

				SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pMrg->pWcTx, pLVI, &pStringLiveRepoPath)  );
				SG_ERR_CHECK(  sg_wc_db__ignores__matches_ignorable_pattern(pCtx, pMrg->pWcTx->pDb, pStringLiveRepoPath, &bIgnorable)  );
				if (bIgnorable)
				{
					statusFlags |= SG_WC_STATUS_FLAGS__U__IGNORED;
					nrChangeBits__Ignored = 1;
				}
				else
				{
					statusFlags |= SG_WC_STATUS_FLAGS__U__FOUND;
					nrChangeBits__Found = 1;
				}
			}
			else
			{
				// We have dirt that was ADDED in WC before the UPDATE that is
				// being carried forward in the UPDATE.

				if ((pLVI->pPcRow_PC && (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U))
					|| (pLVI->pPrescanRow && pLVI->pPrescanRow->pPcRow_Ref && (pLVI->pPrescanRow->pPcRow_Ref->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U)))
				{
					// We know something about the added dirt -- it was
					// carried forward from a chain of one or more updates.

					statusFlags |= SG_WC_STATUS_FLAGS__S__UPDATE_CREATED;
					nrChangeBits__Added = 1;
				}
				else
				{
					// Conceptually this is a
					// normal ADD where we just want the item present after UPDATE
					// is finished, but it shares properties with a ADD-SPECIAL
					// because we want to preserve the item's GID that the original
					// ADD gave it.
					//
					// We want it to be have like a normal ADD so that if they later
					// delete it, we treat it like an UN-ADD.

					statusFlags |= SG_WC_STATUS_FLAGS__S__ADDED;
					nrChangeBits__Added = 1;
				}
			}
		}
	}

	// TODO 2012/06/15 I don't think we need to worry about __A__SPARSE since
	// TODO            they must be clean.
	// TODO 2012/11/26 That is, we don't have to worry about the CONTENT of a
	// TODO            sparse file/symlink because they can't be modified by
	// TODO            the user -- but sparse items can still be moved/renamed/removed.
	// 
	//
	// TODO 2012/06/15 I don't think we need to worry about __S__MERGE_CREATED
	// TODO            since it can't happen in a fake_merge.
	// 
	// TODO 2012/06/15 What about __U__LOST ?

	if (pLVI->pvhIssue)
	{
		statusFlags |= pLVI->statusFlags_x_xr_xu;

		// Per W5770 we DO NOT count the __X__ bits when
		// setting __A__MULTIPLE_CHANGE.
		// nrChangeBits__Conflict = 1;

		// See if we can tell them something about an auto-merge
		// result, if present.
		//
		// We DO NOT count any __M__AUTO_MERGE* bits when
		// setting __A__MULTIPLE_CHANGES.

		if (pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		{
			const char * pszHid_Generated = NULL;
				
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pLVI->pvhIssue, "automerge_generated_hid", &pszHid_Generated)  );
			if (pszHid_Generated)
			{
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid( pCtx, pLVI, pMrg->pWcTx, SG_FALSE,
																			 &pszHid_CurrentContent, NULL)  );

				if (strcmp(pszHid_CurrentContent, pszHid_Generated) == 0)
				{
					// WC still contains the auto-merge result that we put there.
					statusFlags |= SG_WC_STATUS_FLAGS__M__AUTO_MERGED;
				}
				else
				{
					// WC has been edited since we did the auto-merge.
					statusFlags |= SG_WC_STATUS_FLAGS__M__AUTO_MERGED_EDITED;
				}
			}
		}
	}
	
	if ((nrChangeBits__Found
		 + nrChangeBits__Ignored
		 + nrChangeBits__Lost
		 + nrChangeBits__Added
		 + nrChangeBits__Deleted
		 + nrChangeBits__Renamed
		 + nrChangeBits__Moved
		 + nrChangeBits__Attrbits
		 + nrChangeBits__Content
		 + nrChangeBits__Conflict) > 1)
	{
		statusFlags |= SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE;
	}

#if TRACE_WC_TX
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "_fm_status: ComputeStatusFlags: 0x%s %s\n",
								   SG_uint64_to_sz__hex(statusFlags,bufui64),
								   SG_string__sz(pLVI->pStringEntryname))  );
	}
#endif

	// We know that there are changes to report.
	// Install alternate (L1) baseline into LVI so that STATUS
	// will use the L1 values in the vhash it generates (for the
	// 'B' section).

	SG_ERR_CHECK(  sg_wc_liveview_item__set_alternate_baseline_during_update(pCtx, pMrg->pWcTx, pLVI)  );
	SG_ERR_CHECK(  sg_wc__status__append(pCtx, pMrg->pWcTx, pLVI, statusFlags,
										 SG_FALSE, SG_FALSE, SG_FALSE,
										 pszWasLabel_l, pszWasLabel_r,
										 pData->pvaStatus)  );

done:
	;
fail:
	SG_NULLFREE(pCtx, pszHid_CurrentContent);
	SG_STRING_NULLFREE(pCtx, pStringLiveRepoPath);
}

/**
 * We get called once for each entry in the L1/Other CSET.
 * We get called in RANDOM GID order.
 *
 * If we didn't visit this item during _fm_status__final()
 * then this item was deleted in the final-result.
 * Record that info in the "pc" table.
 *
 */
static SG_rbtree_foreach_callback _fm_status__other;
static void _fm_status__other(SG_context * pCtx,
							  const char * pszKey_GidEntry,
							  void * pVoidValue_MrgCSetEntry,
							  void * pVoidData)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Other = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	struct _cb_data * pData = (struct _cb_data *)pVoidData;
	SG_mrg * pMrg = pData->pMrg;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_bool bKnown_lvi = SG_FALSE;
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	const char * pszWasLabel_l = "Baseline (B)";
	const char * pszWasLabel_r = "Working";

	// See if this item was visited in the first loop.
	if (pMrgCSetEntry_Other->markerValue != _SG_MRG__PLAN__MARKER__FLAGS__ZERO)
		return;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_fm_status: other: %s\n", pszKey_GidEntry)  );
#else
	SG_UNUSED( pszKey_GidEntry );
#endif

	// TODO 2012/06/15 not sure if i want to assert this...........
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry_Other->uiAliasGid,
														 &bKnown_lvi, &pLVI)  );
	SG_ASSERT( (bKnown_lvi) );

	// We know that the item was present in Other (aka L1) and
	// is NOT present in the final-result, so it has been deleted.

	// seed statusFlags with the tne_type.
	SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx, pLVI->tneType, &statusFlags)  );

	statusFlags |= SG_WC_STATUS_FLAGS__S__DELETED;

	if (pLVI->pvhIssue)
		statusFlags |= pLVI->statusFlags_x_xr_xu;

	// Install alternate (L1) baseline into LVI so that STATUS
	// will use the L1 values in the vhash it generates (for the
	// 'B' section).

	SG_ERR_CHECK(  sg_wc_liveview_item__set_alternate_baseline_during_update(pCtx, pMrg->pWcTx, pLVI)  );
	SG_ERR_CHECK(  sg_wc__status__append(pCtx, pMrg->pWcTx, pLVI, statusFlags,
										 SG_FALSE, SG_FALSE, SG_FALSE,
										 pszWasLabel_l, pszWasLabel_r,
										 pData->pvaStatus)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__fake_merge__update__status(SG_context * pCtx,
										  SG_mrg * pMrg,
										  SG_varray ** ppvaStatus)
{
	struct _cb_data d;

	d.pMrg = pMrg;
	d.pvaStatus = NULL;

	SG_NULLARGCHECK_RETURN( pMrg );
	SG_NULLARGCHECK_RETURN( ppvaStatus );

	// UPDATE want to return the FUTURE-STATUS which we *would* have
	// IFF the stuff QUEUED were actually APPLIED.

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &d.pvaStatus)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_fm_status: begin\n")  );
#endif
	SG_ERR_CHECK(  SG_mrg_cset__set_all_markers(pCtx,pMrg->pMrgCSet_Other,       _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	SG_ERR_CHECK(  SG_mrg_cset__set_all_markers(pCtx,pMrg->pMrgCSet_FinalResult, _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	// walk final-result and report on added/changed items.
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pMrg->pMrgCSet_FinalResult->prbEntries, _fm_status__final, &d)  );
	// walk other/L1 and report on any deleted items (not visited in first walk).
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pMrg->pMrgCSet_Other->prbEntries,       _fm_status__other, &d)  );
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_fm_status: end\n")  );
#endif

	*ppvaStatus = d.pvaStatus;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, d.pvaStatus);
}
