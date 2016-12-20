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
 * @file sg_wc_tx__fake_merge__update__build_pc.c
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
 * UPDATE is asking us to populate a "pc" table reflecting the
 * delta from "tne_L1" to the (post-merge/update) WD.
 *
 * Later, UPDATE will rebind "L0" ==> { hid_L1, tne_L1, pc_L1 }
 * in tbl_csets and DROP the tne_L0 and pc_L0 tables and DELETE
 * the "L1" row from tbl_csests.
 *
 *
 * We build the "pc_L1" table using the TX QUEUE so that they follow
 * the main-line changes made by MERGE (this may not strictly be
 * necessary), but we CANNOT use the normal QUEUE/JOURNAL routines
 * because the LVIs have already been modified by MERGE.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _insert_pc(SG_context * pCtx,
					   SG_mrg * pMrg,
					   SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
					   sg_wc_db__pc_row__flags_net flags_net,
					   const char * pszHidMerge)	// optional
{
	sg_wc_db__pc_row * pPcRow = NULL;

	SG_ERR_CHECK(  SG_WC_DB__PC_ROW__ALLOC(pCtx, &pPcRow)  );
	pPcRow->flags_net = flags_net;

	pPcRow->p_s->uiAliasGid = pMrgCSetEntry_FinalResult->uiAliasGid;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname),
							 &pPcRow->p_s->pszEntryname)  );
	if (pMrgCSetEntry_FinalResult->bufGid_Parent[0])
		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pMrg->pWcTx->pDb,
														 pMrgCSetEntry_FinalResult->bufGid_Parent,
														 &pPcRow->p_s->uiAliasGidParent)  );
	else
		pPcRow->p_s->uiAliasGidParent = SG_WC_DB__ALIAS_GID__NULL_ROOT;
	pPcRow->p_s->tneType = pMrgCSetEntry_FinalResult->tneType;

	if (pszHidMerge && *pszHidMerge)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidMerge, &pPcRow->pszHidMerge)  );

	if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE)
	{
		SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc(pCtx, &pPcRow->p_d_sparse)  );
		pPcRow->p_d_sparse->attrbits = pMrgCSetEntry_FinalResult->attrBits;
		if (pMrgCSetEntry_FinalResult->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pMrgCSetEntry_FinalResult->bufHid_Blob,
									 &pPcRow->p_d_sparse->pszHid)  );
	}

	pPcRow->ref_attrbits = pMrgCSetEntry_FinalResult->attrBits;

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_fake_pc_stmt(pCtx,
																  pMrg->pWcTx,
																  pMrg->pWcTx->pCSetRow_Other,
																  pPcRow)  );
fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
}

/**
 * We get called once for each entry in the final-result.
 * We get called in RANDOM GID order.
 *
 * Compare the item with the corresponding item in L1
 * and record any changes in "pc" table.
 *
 */
static SG_rbtree_foreach_callback _build_pc__final;
static void _build_pc__final(SG_context * pCtx,
							 const char * pszKey_GidEntry,
							 void * pVoidValue_MrgCSetEntry,
							 void * pVoidData_Mrg)
{
	SG_mrg_cset_entry * pMrgCSetEntry_FinalResult = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	SG_mrg * pMrg = (SG_mrg *)pVoidData_Mrg;
	SG_mrg_cset_entry * pMrgCSetEntry_Other = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry_Ancestor = NULL;
	SG_bool bFound_Other = SG_FALSE;
	SG_bool bFound_Ancestor = SG_FALSE;
	SG_bool bFoundLVI = SG_FALSE;
	SG_bool bIsSparse = SG_FALSE;
	sg_wc_liveview_item * pLVI = NULL;
	SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;
	sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;

	// See if the item was present but sparse in the WD before we started.
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry_FinalResult->uiAliasGid,
														 &bFoundLVI, &pLVI)  );
	bIsSparse = (pLVI && (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live)));
	if (bIsSparse)
		flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_build_pc: final: [sparse %d] [parent %65s] [gid %s] %s\n",
							   bIsSparse,
							   pMrgCSetEntry_FinalResult->bufGid_Parent,
							   pszKey_GidEntry,
							   SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname))  );
#endif

	SG_ASSERT(  (pMrgCSetEntry_FinalResult->markerValue == _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	pMrgCSetEntry_FinalResult->markerValue = _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;

	// See diagram in sg_wc_tx__fake_merge__update__load_csets().

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_Other->prbEntries, pszKey_GidEntry,
								   &bFound_Other, (void **)&pMrgCSetEntry_Other)  );
	if (bFound_Other)
	{
		// Item found in L1 -- which for UPDATE means that it will
		// have a row in tne_L0 *AFTER* the change of basis.
		pMrgCSetEntry_Other->markerValue = _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;

		SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx, pMrgCSetEntry_Other, pMrgCSetEntry_FinalResult, &neq)  );
		if (neq & SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME)
			flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__RENAMED;
		if (neq & SG_MRG_CSET_ENTRY_NEQ__GID_PARENT)
			flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__MOVED;

		// we don't care about the following (dynamic properties because
		// the PC table only reflects the structural stuff):
		// 
		// SG_MRG_CSET_ENTRY_NEQ__ATTRBITS
		// SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB
		// SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB
		// SG_MRG_CSET_ENTRY_NEQ__SUBMODULE_HID_BLOB
		//
		// and SG_MRG_CSET_ENTRY_NEQ__DELETED isn't possible here.

		if (flags_net != SG_WC_DB__PC_ROW__FLAGS_NET__ZERO)
			SG_ERR_CHECK(  _insert_pc(pCtx, pMrg, pMrgCSetEntry_FinalResult, flags_net, NULL)  );
	}
	else
	{
		// Item *NOT* found in L1 -- which for UPDATE means that
		// it WILL NOT have a row in tne_L0 *AFTER* the change of basis.
		// Hence the __ADD_SPECIAL__U bits.
		//
		// This causes problems for SPARSE items in the WD, since to
		// carry them forward as sparse after the UPDATE, they won't
		// have a corresponding tbl_TNE row for us to reference.

		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_LCA->prbEntries, pszKey_GidEntry,
									   &bFound_Ancestor, (void **)&pMrgCSetEntry_Ancestor)  );
		if (bFound_Ancestor)
		{
			// Item present in Ancestor (aka L0).  This is only possible
			// if the item was DELETED in L1.  But from the way that MERGE
			// works, we know that if the item was unchanged in WC, then
			// MERGE would have deleted it from the final-result, so we know
			// that it must be changed and we have a DELETE-VS-* CONFLICT on
			// the item and that it must remain in the final-result.
			//
			// Conceptually this is more like an ADD-SPECIAL where we want
			// to un-delete an item and preserve the original GID.
			//
			// We can set the pszHidMerge field to indicate that the content
			// is disposable if it matches this (if they later do a DELETE
			// or REVERT).
			//
			// And by setting ADD-SPECIAL, we should we see the item in parens
			// in STATUS if they later DELETE the item.
#if TRACE_WC_MERGE
			if (bIsSparse)
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
										   "_build_pc: final: sparse + add-special-u: %s\n",
										   SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname))  );
#endif

			flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U;
			SG_ERR_CHECK(  _insert_pc(pCtx, pMrg, pMrgCSetEntry_FinalResult, flags_net,
									  ((pMrgCSetEntry_Ancestor->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
									   ? pMrgCSetEntry_Ancestor->bufHid_Blob : NULL))  );
		}
		else
		{
			// Item represents dirt present in the WC before the merge.
			// This could be ADDED or FOUND/IGNORED.

			if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
			{
				// Do NOT write FOUND/IGNORED items to the "pc" table.

				SG_ASSERT( (!bIsSparse) );	// by definition -- found dirt cannot be sparse.
			}
			else
			{
				// We have dirt that was ADDED in WC before the UPDATE that is
				// being carried forward in the UPDATE.

				SG_ASSERT( (!bIsSparse) );	// by definition -- added dirt cannot be sparse.

				if ((pLVI->pPcRow_PC && (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U))
					|| (pLVI->pPrescanRow && pLVI->pPrescanRow->pPcRow_Ref && (pLVI->pPrescanRow->pPcRow_Ref->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U)))
				{
					// We know something about the added dirt -- it was
					// carried forward from a chain of one or more updates.

					flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U;
					SG_ERR_CHECK(  _insert_pc(pCtx, pMrg, pMrgCSetEntry_FinalResult, flags_net,
											  ((pMrgCSetEntry_FinalResult->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
											   ? pMrgCSetEntry_FinalResult->bufHid_Blob : NULL))  );
				}
				else
				{
					// ADD this item to the "pc" table.  Conceptually this is a
					// normal ADD where we just want the item present after UPDATE
					// is finished, but it shares properties with a ADD-SPECIAL
					// because we want to preserve the item's GID that the original
					// ADD gave it.
					//
					// We DO NOT set the pszHidMerge because it is a fresh ADD and
					// isn't disposable.
					//
					// We want it to be have like a normal ADD so that if they later
					// delete it, we treat it like an UN-ADD.

					flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__ADDED;
					SG_ERR_CHECK(  _insert_pc(pCtx, pMrg, pMrgCSetEntry_FinalResult, flags_net,
											  NULL)  );
				}
			}
		}
	}

fail:
	return;
}

/**
 * We get called once for each entry in the L1/Other CSET.
 * We get called in RANDOM GID order.
 *
 * If we didn't visit this item during _build_pc__final()
 * then this item was deleted in the final-result.
 * Record that info in the "pc" table.
 *
 */
static SG_rbtree_foreach_callback _build_pc__other;
static void _build_pc__other(SG_context * pCtx,
							 const char * pszKey_GidEntry,
							 void * pVoidValue_MrgCSetEntry,
							 void * pVoidData_Mrg)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Other = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	SG_mrg * pMrg = (SG_mrg *)pVoidData_Mrg;

	// See if this item was visited in the first loop.
	if (pMrgCSetEntry_Other->markerValue != _SG_MRG__PLAN__MARKER__FLAGS__ZERO)
		return;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_build_pc: other: %s\n", pszKey_GidEntry)  );
#else
	SG_UNUSED( pszKey_GidEntry );
#endif

	// We know that the item was present in Other (aka L1) and
	// is NOT present in the final-result, so it has been deleted.

	SG_ERR_CHECK(  _insert_pc(pCtx, pMrg, pMrgCSetEntry_Other,
							  SG_WC_DB__PC_ROW__FLAGS_NET__DELETED, NULL)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

void sg_wc_tx__fake_merge__update__build_pc(SG_context * pCtx,
											SG_mrg * pMrg)
{
	// UPDATE wants to *SET* the new baseline to the UPDATE-TARGET CSET.
	// In MERGE-speak, this is the OTHER CSET (aka "L1").  If we started
	// with a CLEAN WD (and there were no conflicts), we should expect a
	// clean result.  If we started the UPDATE with dirt (or if there
	// were conflicts), we'll have some post-update dirt.  We identify
	// the post-merge dirt by comparing the "L1" with the "FinalResult"
	// that we still have in pMrg.  We write this dirt to a new "pc" table.

	// The following finds the delta in the spirit of sg_wc_tx__merge__queue_plan()
	// only we don't have to worry about doing a recursive crawlout inside
	// these loops to deal with tree-order issues.

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_build_pc: begin\n")  );
#endif
	SG_ERR_CHECK(  SG_mrg_cset__set_all_markers(pCtx,pMrg->pMrgCSet_Other,       _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	SG_ERR_CHECK(  SG_mrg_cset__set_all_markers(pCtx,pMrg->pMrgCSet_FinalResult, _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	// walk final-result and report on added/changed items.
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pMrg->pMrgCSet_FinalResult->prbEntries, _build_pc__final, pMrg)  );
	// walk other/L1 and report on any deleted items (not visited in first walk).
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pMrg->pMrgCSet_Other->prbEntries,       _build_pc__other, pMrg)  );
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_build_pc: end\n")  );
#endif

fail:
	return;
}
