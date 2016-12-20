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

#define SG_WC_HID__UNDEFINED		"*undefined*"

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry__free(SG_context * pCtx, SG_mrg_cset_entry * pMrgCSetEntry)
{
	if (!pMrgCSetEntry)
		return;

	SG_STRING_NULLFREE(pCtx, pMrgCSetEntry->pStringEntryname);
	SG_STRING_NULLFREE(pCtx, pMrgCSetEntry->pStringPortItemLog);
	SG_VARRAY_NULLFREE(pCtx, pMrgCSetEntry->pvaPotentialPortabilityCollisions);
	SG_STRING_NULLFREE(pCtx, pMrgCSetEntry->pStringEntryname_OriginalBeforeMadeUnique);
	SG_NULLFREE(pCtx, pMrgCSetEntry);
}

/**
 * Allocate a new SG_mrg_cset_entry and add it to the CSET's entry-RBTREE.
 * We do not fill in any of the other fields.  We just allocate it and insert
 * it in the rbtree and set the back link.
 *
 * You DO NOT own the returned pointer.
 */
static void _sg_mrg_cset_entry__naked_alloc(SG_context * pCtx,
											SG_mrg * pMrg,
											SG_mrg_cset * pMrgCSet,
											const char * pszGid_Self,
											SG_mrg_cset_entry ** ppMrgCSetEntry_Allocated)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Allocated = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry;

	SG_NULLARGCHECK_RETURN(pMrgCSet);
	SG_NONEMPTYCHECK_RETURN(pszGid_Self);
	SG_NULLARGCHECK_RETURN(ppMrgCSetEntry_Allocated);

	SG_ERR_CHECK(  SG_alloc1(pCtx,pMrgCSetEntry_Allocated)  );
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx,
											  pMrgCSet->prbEntries,
											  pszGid_Self,
											  (void *)pMrgCSetEntry_Allocated)  );
	pMrgCSetEntry = pMrgCSetEntry_Allocated;		// rbtree owns it now.
	pMrgCSetEntry_Allocated = NULL;

	pMrgCSetEntry->pMrgCSet = pMrgCSet;		// link back to the cset

	// The original PendingTree version of this routine did not
	// set the bufGid_Entry field; it only allocated the entry
	// and linked it into the CSet's tables for ownership
	// purposes.
	//
	// In the new WC version, I'm going to go ahead and set the
	// bufGid_Entry field and the new parallel alias field here.
	// Since we are being driven from the CSET/TNEs directly (rather
	// than the LVIs) we don't know anything about the ALIASES yet,
	// so look it up (and if necessary, create aliases for items
	// added in other branches).

	SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
											pMrgCSetEntry->bufGid_Entry,
											SG_NrElements(pMrgCSetEntry->bufGid_Entry),
											pszGid_Self)  );
	SG_ERR_CHECK(  sg_wc_db__gid__get_or_insert_alias_from_gid(pCtx,
															   pMrg->pWcTx->pDb,
															   pszGid_Self,
															   &pMrgCSetEntry->uiAliasGid)  );

	*ppMrgCSetEntry_Allocated = pMrgCSetEntry;
	return;

fail:
	SG_MRG_CSET_ENTRY_NULLFREE(pCtx,pMrgCSetEntry_Allocated);
}

void SG_mrg_cset_entry__load__using_repo(SG_context * pCtx,
										 SG_mrg * pMrg,
										 SG_mrg_cset * pMrgCSet,
										 const char * pszGid_Parent,	// will be null for actual-root directory
										 const char * pszGid_Self,
										 const SG_treenode_entry * pTne_Self,
										 SG_mrg_cset_entry ** ppMrgCSetEntry_New)
{
	SG_mrg_cset_entry * pMrgCSetEntry;
	const char * psz_temp;

	SG_NULLARGCHECK_RETURN(pMrg);
	SG_NULLARGCHECK_RETURN(pMrgCSet);
	// pszGid_Parent can be null
	SG_NONEMPTYCHECK_RETURN(pszGid_Self);
	SG_NULLARGCHECK_RETURN(pTne_Self);
	// ppMrgCSetEntry_New is optional

	if (SG_MRG_CSET__IS_FROZEN(pMrgCSet))
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_WHILE_FROZEN);

	SG_ERR_CHECK_RETURN(  _sg_mrg_cset_entry__naked_alloc(pCtx,
														  pMrg,
														  pMrgCSet,
														  pszGid_Self,
														  &pMrgCSetEntry)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx,pTne_Self,&pMrgCSetEntry->attrBits)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx,pTne_Self,&psz_temp)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pMrgCSetEntry->pStringEntryname, psz_temp)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx,pTne_Self,&pMrgCSetEntry->tneType)  );

	if (pszGid_Parent && *pszGid_Parent)
		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSetEntry->bufGid_Parent,
												SG_NrElements(pMrgCSetEntry->bufGid_Parent),
												pszGid_Parent)  );

	// TODO do we care about the TNE version?

	// we don't set bufHid_Blob for directories.
	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx,pTne_Self,&psz_temp)  );
	if (pMrgCSetEntry->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
		SG_ERR_CHECK(  SG_mrg_cset_entry__load_subdir__using_repo(pCtx,
																  pMrg,
																  pMrgCSet,
																  pszGid_Self,
																  pMrgCSetEntry,psz_temp)  );
	else
		SG_ERR_CHECK(  SG_strcpy(pCtx,pMrgCSetEntry->bufHid_Blob,SG_NrElements(pMrgCSetEntry->bufHid_Blob),psz_temp)  );

	// this field is only defined for the branch loaded by __using_wc().
	pMrgCSetEntry->statusFlags__using_wc = SG_WC_STATUS_FLAGS__ZERO;

	// set the origin cset to our cset as a way of saying that we are the leaf/lca
	pMrgCSetEntry->pMrgCSet_CloneOrigin = pMrgCSet;

	if (ppMrgCSetEntry_New)
		*ppMrgCSetEntry_New = pMrgCSetEntry;

fail:
	return;
}

void SG_mrg_cset_entry__load_subdir__using_repo(SG_context * pCtx,
												SG_mrg * pMrg,
												SG_mrg_cset * pMrgCSet,
												const char * pszGid_Self,
												SG_mrg_cset_entry * pMrgCSetEntry_Self,
												const char * pszHid_Blob)
{
	SG_uint32 nrEntriesInDir, k;
	SG_treenode * pTn = NULL;

	SG_NULLARGCHECK_RETURN(pMrg);
	SG_NULLARGCHECK_RETURN(pMrgCSet);
	SG_NONEMPTYCHECK_RETURN(pszGid_Self);
	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_Self);
	SG_ARGCHECK_RETURN(  (pMrgCSetEntry_Self->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY), tneType  );

	// TODO this would be a good place to hook into a treenode-cache so that
	// TODO we don't have to hit the disk each time.  granted, in any given
	// TODO CSET a treenode will only be referenced once, but we may have
	// TODO many CSETs in memory (

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx,
											   pMrg->pWcTx->pDb->pRepo,
											   pszHid_Blob,
											   &pTn)  );

	// walk contents of directory and load each entry.

	SG_ERR_CHECK(  SG_treenode__count(pCtx,pTn,&nrEntriesInDir)  );
	for (k=0; k<nrEntriesInDir; k++)
	{
		const char * pszGid_k;
		const SG_treenode_entry * pTne_k;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx,pTn,k,&pszGid_k,&pTne_k)  );
		SG_ERR_CHECK(  SG_mrg_cset_entry__load__using_repo(pCtx,
														   pMrg,
														   pMrgCSet,
														   pszGid_Self,
														   pszGid_k,
														   pTne_k,
														   NULL)  );
	}

fail:
	SG_TREENODE_NULLFREE(pCtx, pTn);
}

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry__load__using_wc(SG_context * pCtx,
									   SG_mrg * pMrg,
									   SG_mrg_cset * pMrgCSet,
									   const char * pszGid_Parent,
									   const char * pszGid_Self,
									   sg_wc_liveview_item * pLVI_Self,
									   SG_mrg_cset_entry ** ppMrgCSetEntry_New)
{
	char * pszHid_owned = NULL;
	SG_mrg_cset_entry * pMrgCSetEntry;

	SG_NULLARGCHECK_RETURN(pMrg);
	SG_NULLARGCHECK_RETURN(pMrgCSet);
	// pszGid_Parent can be null
	SG_NONEMPTYCHECK_RETURN(pszGid_Self);
	SG_NULLARGCHECK_RETURN(pLVI_Self);
	// ppMrgCSetEntry_New is optional

	if (SG_MRG_CSET__IS_FROZEN(pMrgCSet))
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_WHILE_FROZEN);

	if ((pMrg->eFakeMergeMode == SG_FAKE_MERGE_MODE__MERGE)
		|| (pMrg->eFakeMergeMode == SG_FAKE_MERGE_MODE__UPDATE))
	{
		if (pMrg->bDisallowWhenDirty)
		{
			SG_wc_status_flags sf;
			SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pMrg->pWcTx, pLVI_Self,
														SG_FALSE, SG_FALSE, &sf)  );
			if (sf & (SG_WC_STATUS_FLAGS__U__LOST
					  | SG_WC_STATUS_FLAGS__S__MASK
					  | SG_WC_STATUS_FLAGS__C__MASK))
			{
				SG_ERR_THROW2(  SG_ERR_MERGE_REQUESTED_CLEAN_WD,
								(pCtx, "The item '%s' is dirty.",
								 SG_string__sz(pLVI_Self->pStringEntryname))  );								 

			}
		}
	}

	// Silently retire any already-resolved ISSUES as we dive
	// into the WD tree.  This saves a separate pass (or 2)
	// later.  We mainly want to do this to reduce the visual
	// clutter that the user will see after the UPDATE or MERGE.

	if (pLVI_Self->pvhIssue)	// statusFlags_x_xr_xu is only defined when pvhIssue is non-null.
	{
		SG_bool bIsResolved = ((pLVI_Self->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED) != 0);
#if TRACE_WC_MERGE	
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "_retire_cb: LVI [bRes %d] [%s] %s\n",
								   bIsResolved, pszGid_Self,
								   SG_string__sz(pLVI_Self->pStringEntryname))  );
#endif
		if (bIsResolved)
		{
			// item is already resolved.  we can retire it for real.
			// queue a delete_issue to update the DB -- this will
			// also free the associated pLVI fields.
			SG_ERR_CHECK(  sg_wc_tx__queue__delete_issue(pCtx, pMrg->pWcTx, pLVI_Self)  );
		}
		else if (pMrg->eFakeMergeMode == SG_FAKE_MERGE_MODE__REVERT)
		{
			// since we're doing a revert-all, we can just disregard
			// any unresolved issues (without regard for whether the
			// item is dirty).
			//
			// TODO 2012/10 Prior to the changes for W4887, we'd defer
			// TODO         this until SG_mrg_cset_entry__check_for_outstanding_issue().
			// TODO         But because of the make-unique-filename stuff, we'd almost
			// TODO         always be guaranteed to have a change to whenever there was
			// TODO         an issue (part of the PendingTree legacy).
			// TODO
			// TODO         With the W4887 changes to not do the make-unique unless
			// TODO         absolutely necessary, we can get issues without changes,
			// TODO         so we force it here.  And we can probably delete some
			// TODO         code in SG_mrg_cset_entry__check_for_outstanding_issue().
			SG_ERR_CHECK(  sg_wc_tx__queue__delete_issue(pCtx, pMrg->pWcTx, pLVI_Self)  );
		}
		else
		{
			// item is currently unresolved; do nothing at this point in the TX.
			// we either want to carry-it-forward or throw if it will cause interference.
		}
	}

	// A quick pre-check of the kinds of dirt that we
	// want to allow/disallow.

	switch (pLVI_Self->scan_flags_Live)
	{
	case SG_WC_PRESCAN_FLAGS__RESERVED:
		// Since we *NEVER* let items with RESERVED names be under version
		// control, we never want to dive into reserved directories and
		// do not need to set the HID for non-directories.  And we DO NOT
		// want to add them to the merge.
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "MrgCSetEntry_LoadUsingWC: skipping reserved '%s'.\n",
								   SG_string__sz(pLVI_Self->pStringEntryname))  );
#endif
		if (ppMrgCSetEntry_New)
			*ppMrgCSetEntry_New = NULL;
		return;

	case SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED:
		// For FOUND/IGNORED items, we let it continue.
		// We basically want to allow these items to be
		// carried forward if possible.  If they cause
		// interference, we may throw when we know what
		// the problem/conflict is.
		break;

	case SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT:
		break;

	case SG_WC_PRESCAN_FLAGS__CONTROLLED_INACTIVE_DELETED:
		// A pending delete of this item.
		// Don't insert it into the pMrgCSet
		// (just like we wouldn't have for a
		// committed delete).
		//
		// The merge-proper code will just
		// have to detect the delete like it
		// does for already-committed deletes.
		{
			switch (pMrg->eFakeMergeMode)
			{
			case SG_FAKE_MERGE_MODE__MERGE:
			case SG_FAKE_MERGE_MODE__UPDATE:
				break;

			case SG_FAKE_MERGE_MODE__REVERT:
				// If we are in a REVERT-ALL we still DO NOT want to
				// insert this item into the pMrgCSet.  BUT we need
				// to add it to the kill list so that post-revert we
				// won't see ADDED-SPECIAL + REMOVED status for it.
				SG_ERR_CHECK(  sg_wc_tx__merge__add_to_kill_list(pCtx, pMrg, pLVI_Self->uiAliasGid)  );
				break;

			default:
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "Unknown eFakeMergeMode %d with deleted item %s",
								 (SG_uint32)pMrg->eFakeMergeMode,
								 SG_string__sz(pLVI_Self->pStringEntryname))  );
			}
		}
		return;

	case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED:
		// This includes both CLEAN and DIRTY items that
		// are already controlled (which includes things
		// currently marked ADDED).
		break;

	case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_LOST:
		{
			switch (pMrg->eFakeMergeMode)
			{
			case SG_FAKE_MERGE_MODE__MERGE:
				// I'm going to be harsh here and just stop the
				// merge when there are LOST items.  There are
				// several competing ideas here:
				// [] A lost item could be silently restored by
				//    us right here before we start the merge.
				//    (with the assumption that the item should
				//    be thought of as unchanged relative to the
				//    actual baseline).
				// [] A lost item means that the user did something
				//    like /bin/rm or /bin/mv without telling us.
				//    In the former case, we might infer that they
				//    meant to do a 'vv remove' and in the latter
				//    case they meant to do a 'vv rename' or 'vv move'
				//    (and there may be a corresponding FOUND item
				//    somewhere).  Either way, we should just stop
				//    before we assume something and blend that
				//    assumption with their merge results.
				// [] A lost item could be carried forward as is and
				//    just be a lost item in the result.  This could
				//    work for files/symlinks, but could get messy if
				//    the item is a directory and the other CSET has
				//    new items that belong inside the directory.
				//
				// TODO 2012/03/15 So for now, I'm going to throw.
				// TODO            Revisit this later.
				SG_ERR_THROW2(  SG_ERR_NOT_ALLOWED_WITH_LOST_ITEM,
								(pCtx, "Cannot MERGE with LOST item '%s'.",
								 SG_string__sz(pLVI_Self->pStringEntryname))  );

			case SG_FAKE_MERGE_MODE__UPDATE:
				// I'm not sure what the right answer is when we are in
				// an UPDATE.  We could "carry-forward" the LOST item so
				// that it'll still be lost in the resulting WD.
				//
				// Or we can try to restore it (assuming it exists in
				// the target CSet).
				//
				// I recently changed UPDATE to always try to restore
				// the item.  Content-wise, it means we should get the
				// content of the target (we know nothing of whether it
				// was edited before it was lost).  Structure-wise we
				// should get the normal merge behavior for move/rename.
				//
				// But this doesn't work if the item was also added in
				// the dirty WD because the target cset doesn't know
				// anything about it either.  So I'm going to let it fail.
				//
				// TODO 2012/07/26 One could debate if we should NOT throw
				// TODO            for directories and go ahead and try to
				// TODO            "restore" them.  But that might just
				// TODO            push the problem to lost stuff within
				// TODO            this directory, so it might not be much
				// TODO            of a win.

				{
					sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;
					SG_ERR_CHECK(  sg_wc_liveview_item__get_flags_net(pCtx, pLVI_Self, &flags_net)  );
					if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADDED)
					{
						SG_ERR_THROW2(  SG_ERR_NOT_ALLOWED_WITH_LOST_ITEM,
										(pCtx, "Cannot UPDATE with ADDED+LOST item '%s'.",
										 SG_string__sz(pLVI_Self->pStringEntryname))  );
					}
					else
					{
						// Let's try to restore it.
						// 
						// See sg_wc_tx__merge__queue_plan__peer()
					}
				}
				break;

			case SG_FAKE_MERGE_MODE__REVERT:
				// When in a REVERT-ALL and we see a LOST item, we let it
				// continue.  The merge engine will use this node and pair
				// it up with the peer in the "C" leaf and then use the
				// __queue__undo_lost() code to restore it.  This lets 
				// REVERT-ALL work when there are lost items.
				//
				// See sg_wc_tx__merge__queue_plan__peer()
				//
				// The only down side is that if the user did a /bin/mv
				// (rather than a /bin/rm) and thus also creating a FOUND
				// item, it will remain (since we don't know about its
				// relationship to this lost item).
				{
					sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;
					SG_ERR_CHECK(  sg_wc_liveview_item__get_flags_net(pCtx, pLVI_Self, &flags_net)  );
					if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADDED)
					{
						// For an ADDED+LOST item, there isn't anything in "B" for us to restore.
						// It should just disappear (FOUND+LOST should cancel out).
						SG_ERR_CHECK(  sg_wc_tx__merge__add_to_kill_list(pCtx, pMrg, pLVI_Self->uiAliasGid)  );
						return;
					}
					else
					{
						// Let's try to restore it.
						// 
						// See sg_wc_tx__merge__queue_plan__peer()
					}
				}
				break;

			default:
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "Unknown eFakeMergeMode %d with lost item %s",
								 (SG_uint32)pMrg->eFakeMergeMode,
								 SG_string__sz(pLVI_Self->pStringEntryname))  );
			}
		}
		break;

	case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE:
		// We treat sparse (not populated) items like __CONTROLLED_ACTIVE_MATCHED.
		// They are controlled, just not present.
		// The item may be clean or dirty (structurally).
		//
		// We will wait to complain about this item until
		// the backside of the merge when we know if we
		// will have to manipulate it and/or automerge it.
		break;

	case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN:
		// These are only possible for items that MERGE
		// added using ADD-SPECIAL or UNDO-DELETE, during
		// this (pWcTx) TX so we shouldn't expect them to
		// be present yet.
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "SG_mrg_cset_entry__load__using_wc: POSTSCAN unexpected for '%s'.",
						 SG_string__sz(pLVI_Self->pStringEntryname))  );

	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "SG_mrg_cset_entry__load__using_wc: unknown scan_flag '0x%x' for '%s'.",
						 (SG_uint32)pLVI_Self->scan_flags_Live,
						 SG_string__sz(pLVI_Self->pStringEntryname))  );
	}
	

	SG_ERR_CHECK_RETURN(  _sg_mrg_cset_entry__naked_alloc(pCtx,
														  pMrg,
														  pMrgCSet,
														  pszGid_Self,
														  &pMrgCSetEntry)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits(pCtx,
															 pLVI_Self,
															 pMrg->pWcTx,
															 (SG_uint64 *)&pMrgCSetEntry->attrBits)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
										  &pMrgCSetEntry->pStringEntryname,
										  pLVI_Self->pStringEntryname)  );

	pMrgCSetEntry->tneType = pLVI_Self->tneType;

	if (pszGid_Parent && *pszGid_Parent)
		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSetEntry->bufGid_Parent,
												SG_NrElements(pMrgCSetEntry->bufGid_Parent),
												pszGid_Parent)  );

	// TODO do we care about the TNE version?

	if (pMrgCSetEntry->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		// we don't set bufHid_Blob for directories.

		// TODO 2013/01/16 Think about maybe not diving into IGNORED directories.
		
		SG_ERR_CHECK(  SG_mrg_cset_entry__load_subdir__using_wc(pCtx,
																pMrg,
																pMrgCSet,
																pszGid_Self,
																pLVI_Self,
																pMrgCSetEntry)  );
	}
	else if (pMrgCSetEntry->tneType == SG_TREENODEENTRY_TYPE__DEVICE)
	{
		// we don't set bufHid_Blob for devices.
#if 0
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Skipping device: %s\n",
								   SG_string__sz(pLVI_Self->pStringEntryname))  );
#endif
	}
	else
	{
		if (pLVI_Self->scan_flags_Live == SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED)
		{
			// Don't compute HID hash on found/ignored items.
			// This just slows UPDATE/MERGE down when there are
			// lots of large files (like compiler trash) in the WC.
			SG_ERR_CHECK(  SG_strcpy(pCtx,
									 pMrgCSetEntry->bufHid_Blob,
									 SG_NrElements(pMrgCSetEntry->bufHid_Blob),
									 SG_WC_HID__UNDEFINED)  );
		}
		else
		{
			SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx,
																		pLVI_Self,
																		pMrg->pWcTx,
																		SG_FALSE,
																		&pszHid_owned, NULL)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx,
									 pMrgCSetEntry->bufHid_Blob,
									 SG_NrElements(pMrgCSetEntry->bufHid_Blob),
									 pszHid_owned)  );
		}
	}

	// Remember whether this item is clean/dirty so that we can report
	// it in the pvhIssue.input.* for it.  We only need to set this for
	// the __using_wc() version (since things gotten from the __using_repo()
	// version are implicitly clean).
	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pMrg->pWcTx, pLVI_Self, SG_FALSE, SG_FALSE,
												&pMrgCSetEntry->statusFlags__using_wc)  );
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MrgCSetEntry_LoadUsingWC: '%s':  ",
							   SG_string__sz(pLVI_Self->pStringEntryname))  );
	SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags_to_console(pCtx,
															   pMrgCSetEntry->statusFlags__using_wc,
															   "statusFlags__using_wc")  );
#endif

	// set the origin cset to our cset as a way of saying that we are the leaf/lca
	pMrgCSetEntry->pMrgCSet_CloneOrigin = pMrgCSet;

	if (ppMrgCSetEntry_New)
		*ppMrgCSetEntry_New = pMrgCSetEntry;

fail:
	SG_NULLFREE(pCtx, pszHid_owned);
}

struct _ls_ub_data
{
	SG_mrg * pMrg;
	SG_mrg_cset * pMrgCSet;
	const char * pszGid_Parent;
};

static SG_rbtree_ui64_foreach_callback _ls_ub_cb;
static void _ls_ub_cb(SG_context * pCtx,
					  SG_uint64 uiAliasGid,
					  void * pVoidAssoc,
					  void * pVoidData)
{
	struct _ls_ub_data * pData = (struct _ls_ub_data *)pVoidData;
	sg_wc_liveview_item * pLVI_k = (sg_wc_liveview_item *)pVoidAssoc;
	char * pszGid_k = NULL;

	SG_UNUSED( uiAliasGid );

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pData->pMrg->pWcTx->pDb,
													 pLVI_k->uiAliasGid,
													 &pszGid_k)  );
	SG_ERR_CHECK(  SG_mrg_cset_entry__load__using_wc(pCtx,
													 pData->pMrg,
													 pData->pMrgCSet,
													 pData->pszGid_Parent,
													 pszGid_k,
													 pLVI_k,
													 NULL)  );

fail:
	SG_NULLFREE(pCtx, pszGid_k);
}

void SG_mrg_cset_entry__load_subdir__using_wc(SG_context * pCtx,
											  SG_mrg * pMrg,
											  SG_mrg_cset * pMrgCSet,
											  const char * pszGid_Self,
											  sg_wc_liveview_item * pLVI_Self,
											  SG_mrg_cset_entry * pMrgCSetEntry_Self)
{
	sg_wc_liveview_dir * pLVD_Self = NULL;	// we do not own this
	struct _ls_ub_data data;
	
	SG_NULLARGCHECK_RETURN(pMrg);
	SG_NULLARGCHECK_RETURN(pMrgCSet);
	SG_NONEMPTYCHECK_RETURN(pszGid_Self);
	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_Self);
	SG_ARGCHECK_RETURN(  (pMrgCSetEntry_Self->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY), tneType  );

	if (pLVI_Self->scan_flags_Live == SG_WC_PRESCAN_FLAGS__RESERVED)
	{
		// Since we *NEVER* let items with RESERVED names be under version
		// control, we never want to dive into reserved directories and
		// do not need to set the HID for non-directories.
	}
	else
	{
		// the LVD describes the content *within* the directory
		// *AS IT CURRENTLY EXISTS* in the TX.

		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pMrg->pWcTx, pLVI_Self, &pLVD_Self)  );

		memset(&data, 0, sizeof(data));
		data.pMrg = pMrg;
		data.pMrgCSet = pMrgCSet;
		data.pszGid_Parent = pszGid_Self;
		SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD_Self, _ls_ub_cb, &data)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry__equal(SG_context * pCtx,
							  const SG_mrg_cset_entry * pMrgCSetEntry_1,
							  const SG_mrg_cset_entry * pMrgCSetEntry_2,
							  SG_mrg_cset_entry_neq * pNeq)
{
	SG_mrg_cset_entry_neq neq = SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL;

	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_1);
	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_2);
	SG_NULLARGCHECK_RETURN(pNeq);

#if defined(DEBUG)
	// they must call us with the same entry/object from both versions of the tree.

	if (strcmp(pMrgCSetEntry_1->bufGid_Entry,pMrgCSetEntry_2->bufGid_Entry) != 0)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "SG_mrg_cset_entry__equal: Entries refer to different objects. [%s][%s]",
								pMrgCSetEntry_1->bufGid_Entry,pMrgCSetEntry_2->bufGid_Entry)  );

	if (pMrgCSetEntry_1->uiAliasGid != pMrgCSetEntry_2->uiAliasGid)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "SG_mrg_cset_entry__equal: Entries refer to different aliases. [%s][%s]",
								pMrgCSetEntry_1->bufGid_Entry,pMrgCSetEntry_2->bufGid_Entry)  );

	// an entry/object cannot change type between 2 different versions of the tree.
	if (pMrgCSetEntry_1->tneType != pMrgCSetEntry_2->tneType)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "SG_mrg_cset_entry__equal: Entries refer to different types. [%s][%s]",
								pMrgCSetEntry_1->bufGid_Entry,pMrgCSetEntry_2->bufGid_Entry)  );
#endif

	if (pMrgCSetEntry_1->attrBits != pMrgCSetEntry_2->attrBits)
		neq |= SG_MRG_CSET_ENTRY_NEQ__ATTRBITS;

	if (strcmp(SG_string__sz(pMrgCSetEntry_1->pStringEntryname),SG_string__sz(pMrgCSetEntry_2->pStringEntryname)) != 0)
		neq |= SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME;

	if (strcmp(pMrgCSetEntry_1->bufGid_Parent,pMrgCSetEntry_2->bufGid_Parent) != 0)
		neq |= SG_MRG_CSET_ENTRY_NEQ__GID_PARENT;

	switch (pMrgCSetEntry_1->tneType)
	{
	default:
		SG_ASSERT( (0) );	// quiets compiler

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		// since we don't set bufHid_Blob for directories, we don't include it in the
		// equality check when we are looking at a directory.
		break;

	case SG_TREENODEENTRY_TYPE__DEVICE:
		// likewise we don't set bufHid_Blob for devices.
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		// we DO NOT attempt any form of auto-merge on the pathname fragments in the symlinks.
		if (strcmp(pMrgCSetEntry_1->bufHid_Blob,pMrgCSetEntry_2->bufHid_Blob) != 0)
			neq |= SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB;
		break;

	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		// we only know the blob-hid for versions of the file that we loaded from
		// the repo.  for partial results of sub-merges, we don't yet know the HID
		// because we haven't done the auto-merge yet -or- we may have a file type
		// cannot be auto-merged.  so we treat them as non-equal.

		if (pMrgCSetEntry_1->bufHid_Blob[0] == 0)
		{
			SG_ASSERT(  (pMrgCSetEntry_1->pMrgCSetEntryConflict)  );
			SG_ASSERT(  (pMrgCSetEntry_1->pMrgCSetEntryConflict->pPathTempFile_Result)  );
			neq |= SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB;
		}
		else if (pMrgCSetEntry_2->bufHid_Blob[0] == 0)
		{
			SG_ASSERT(  (pMrgCSetEntry_2->pMrgCSetEntryConflict)  );
			SG_ASSERT(  (pMrgCSetEntry_2->pMrgCSetEntryConflict->pPathTempFile_Result)  );
			neq |= SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB;
		}
		else if (strcmp(pMrgCSetEntry_1->bufHid_Blob,pMrgCSetEntry_2->bufHid_Blob) != 0)
		{
			neq |= SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB;
		}
		break;

	case SG_TREENODEENTRY_TYPE_SUBMODULE:
		// we DO NOT attempt any form of auto-merge on the contents of the submodule.
		// if both branches refer to the same cset (and thus have the same blob HID in
		// the TNE), we're fine.  if anything changed (and we don't pretend to know
		// anything about the contents of the submodule), we just say NEQ and let the
		// caller deal with it.
		if (strcmp(pMrgCSetEntry_1->bufHid_Blob,pMrgCSetEntry_2->bufHid_Blob) != 0)
			neq |= SG_MRG_CSET_ENTRY_NEQ__SUBMODULE_HID_BLOB;
		break;
	}

	*pNeq = neq;
}

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry__clone_and_insert_in_cset(SG_context * pCtx,
												 SG_mrg * pMrg,
												 const SG_mrg_cset_entry * pMrgCSetEntry_Src,
												 SG_mrg_cset * pMrgCSet_Result,
												 SG_mrg_cset_entry ** ppMrgCSetEntry_Result)
{
	SG_mrg_cset_entry * pMrgCSetEntry;

	SG_NULLARGCHECK_RETURN(pMrgCSetEntry_Src);
	SG_NULLARGCHECK_RETURN(pMrgCSet_Result);
	// ppMrgCSetEntry_Result is optional.  (we add it to the RBTREE.)

	if (SG_MRG_CSET__IS_FROZEN(pMrgCSet_Result))
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_WHILE_FROZEN);

	// we cannot insert new entries into a CSET that we loaded from disk.
	SG_ARGCHECK_RETURN( (pMrgCSet_Result->origin == SG_MRG_CSET_ORIGIN__VIRTUAL), pMrgCSet_Result->origin );

	SG_ERR_CHECK_RETURN(  _sg_mrg_cset_entry__naked_alloc(pCtx,
														  pMrg,
														  pMrgCSet_Result,
														  pMrgCSetEntry_Src->bufGid_Entry,
														  &pMrgCSetEntry)  );

	pMrgCSetEntry->markerValue = pMrgCSetEntry_Src->markerValue;
	pMrgCSetEntry->attrBits    = pMrgCSetEntry_Src->attrBits;
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
										  &pMrgCSetEntry->pStringEntryname,
										  pMrgCSetEntry_Src->pStringEntryname)  );
	pMrgCSetEntry->tneType     = pMrgCSetEntry_Src->tneType;

	// bufHid_Blob is not set for directories.
	if (pMrgCSetEntry_Src->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_ERR_CHECK(  SG_strcpy(pCtx,
								 pMrgCSetEntry->bufHid_Blob, SG_NrElements(pMrgCSetEntry->bufHid_Blob),
								 pMrgCSetEntry_Src->bufHid_Blob)  );

		// when cloning a file (and inheriting the hid-blob), remember the CSET
		// that we inherited it from.  this lets _preview properly credit the original
		// source of the version when drawing the "graph".

		if (pMrgCSetEntry_Src->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		{
			if (pMrgCSetEntry_Src->pMrgCSet_FileHidBlobInheritedFrom)
				pMrgCSetEntry->pMrgCSet_FileHidBlobInheritedFrom = pMrgCSetEntry_Src->pMrgCSet_FileHidBlobInheritedFrom;
			else
				pMrgCSetEntry->pMrgCSet_FileHidBlobInheritedFrom = pMrgCSetEntry_Src->pMrgCSet;
		}
	}

	if (pMrgCSetEntry_Src->bufGid_Parent[0])
	{
		// this entry has a parent
		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSetEntry->bufGid_Parent, SG_NrElements(pMrgCSetEntry->bufGid_Parent),
												pMrgCSetEntry_Src->bufGid_Parent)  );
	}
	else
	{
		// this entry does not have a parent.  it must be the root.
		// stash a copy of this entry's GID in the result-cset.
		// FWIW, we could probably have assumed this GID when we allocated
		// the result-cset (since the root entry cannot be moved away) but
		// we wouldn't have allocated the new pMrgCSetEntry yet.
		SG_ASSERT(  (pMrgCSet_Result->bufGid_Root[0] == 0)  );
		SG_ASSERT(  (pMrgCSet_Result->pMrgCSetEntry_Root == NULL)  );

		SG_ERR_CHECK(  SG_gid__copy_into_buffer(pCtx,
												pMrgCSet_Result->bufGid_Root, SG_NrElements(pMrgCSet_Result->bufGid_Root),
												pMrgCSetEntry_Src->bufGid_Entry)  );
		pMrgCSet_Result->pMrgCSetEntry_Root = pMrgCSetEntry;
	}

	// remember the origin cset from which we cloned this entry.  this
	// should transend clones-of-clones (sub-merges) so that this field
	// always points back to a Leaf or LCA.  we use this to help label
	// things when we have collisions.

	pMrgCSetEntry->pMrgCSet_CloneOrigin = pMrgCSetEntry_Src->pMrgCSet_CloneOrigin;

	if (ppMrgCSetEntry_Result)
		*ppMrgCSetEntry_Result = pMrgCSetEntry;

fail:
	return;
}

/**
 * Forget that we inherited the hid-blob from an entry in another CSET.
 */
void SG_mrg_cset_entry__forget_inherited_hid_blob(SG_context * pCtx,
												  SG_mrg_cset_entry * pMrgCSetEntry)
{
	SG_NULLARGCHECK_RETURN(pMrgCSetEntry);

	pMrgCSetEntry->bufHid_Blob[0] = 0;
	pMrgCSetEntry->pMrgCSet_FileHidBlobInheritedFrom = NULL;
}

//////////////////////////////////////////////////////////////////

SG_rbtree_foreach_callback SG_mrg_cset_entry__make_unique_entrynames;

void SG_mrg_cset_entry__make_unique_entrynames(SG_context * pCtx,
											   SG_UNUSED_PARAM(const char * pszKey_Gid_Entry),
											   void * pVoidAssocData_MrgCSetEntry,
											   SG_UNUSED_PARAM(void * pVoidCallerData))
{
	SG_mrg_cset_entry * pMrgCSetEntry = (SG_mrg_cset_entry *)pVoidAssocData_MrgCSetEntry;
	SG_bool bHadCollision, bHadEntrynameConflict, bHadPortabilityIssue;
	SG_bool bDeleteConflict;
	SG_bool bDivergentMove;
	SG_bool bMovesCausedPathCycle;

	SG_UNUSED(pVoidCallerData);
	SG_UNUSED(pszKey_Gid_Entry);

	if (pMrgCSetEntry->pStringEntryname_OriginalBeforeMadeUnique)
		return;

	bHadCollision         = (pMrgCSetEntry->pMrgCSetEntryCollision != NULL);

	// TODO are there other conflict flag bits we want this for?  such as delete-vs-rename?
	bHadEntrynameConflict = ((pMrgCSetEntry->pMrgCSetEntryConflict)
							 && ((pMrgCSetEntry->pMrgCSetEntryConflict->flags & (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME)) != 0));

	// TODO i'm going to assume here that if portability warnings are turned off, the flags
	// TODO here won't ever be set.
	bHadPortabilityIssue  = (pMrgCSetEntry->portFlagsObserved != 0);

	// TODO 2012/06/04 The following HACK was written for PendingTree.
	// TODO            With the changes to WC, we don't need to force
	// TODO            a rename just to make the thing look dirty because
	// TODO            WC don't do the filtering-on-save stuff like
	// TODO            PendingTree.
	// TODO
	// TODO            See W4887.
	// TODO
	// 
	// This is a bit of a hack, but I'm going to do it for now.
	// 
	// If the entry was DELETED on one side and ALTERED in any
	// way on the other side, we have a DELETE-vs-* conflict.
	// We preserve/restore the altered entry (temporarily
	// cancelling the delete if you will) and register a conflict
	// issue.
	//
	// All of this gets properly recorded in the conflict issue list.
	// *BUT* when the pendingtree gets written out, we have a slight
	// problem.
	// 
	// If the delete was in the baseline (and the non-delete change
	// is being folded in), then we restore the entry.  The
	// pendingtree will see this as an ADD (with the original GID).
	// That is fine.
	//
	// But when the non-delete change was in the baseline (and the
	// delete is being folded in), we simply preserve the entry in
	// WD and record the conflict in the conflict issue list.  The
	// problem is that simply preserving the entry means that it
	// isn't changed between the baseline and the final result, so
	// the code in pendingtree to filter out unchanged entries kicks
	// in and prunes it.
	//
	// So I'm going to give is a unique name (at least for now) to
	// both draw attention to it and to ensure that the ptnode is
	// dirty.
	bDeleteConflict = ((pMrgCSetEntry->pMrgCSetEntryConflict)
					   && ((pMrgCSetEntry->pMrgCSetEntryConflict->flags & (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__UNDELETE__MASK)) != 0));

	// We have a similar problem with divergent moves, we have to
	// put/leave the entry somewhere, so we leave it in the directory
	// that it was in in the baseline.  So without any other markings,
	// the "preserved" version will be filtered out by the pendingtree.
	bDivergentMove = ((pMrgCSetEntry->pMrgCSetEntryConflict)
					  && ((pMrgCSetEntry->pMrgCSetEntryConflict->flags & (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE)) != 0));

	// same thing for these, we had to leave them somewhere because
	// we couldn't move them properly.
	//
	// TODO I'm not sure I like to do this for divergent-moves and
	// TODO moves-caused-path-cycle, but I don't have time to debate
	// TODO that now.  It might be better to have the pendingtree
	// TODO filtering code be aware of the ISSUES and/or to actually
	// TODO put the issue in the ptnode so that the dirty check is
	// TODO all right there.
	bMovesCausedPathCycle = ((pMrgCSetEntry->pMrgCSetEntryConflict)
							 && ((pMrgCSetEntry->pMrgCSetEntryConflict->flags & (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE)) != 0));


	{
		SG_bool b1 = (bHadCollision | bHadEntrynameConflict | bHadPortabilityIssue);
		SG_bool b2 = (bDeleteConflict | bDivergentMove | bMovesCausedPathCycle);
	
		if (b2 & !b1)
		{
#if 0
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "W4887: avoiding make-unique for '%s'\n",
									   SG_string__sz(pMrgCSetEntry->pStringEntryname))  );
#endif
			return;
		}
	}

	if (!bHadCollision && !bHadEntrynameConflict && !bHadPortabilityIssue && !bDeleteConflict && !bDivergentMove && !bMovesCausedPathCycle)
		return;

	// make unique entryname for this item.  See W5683.
	// we want something like "<entryname>~<gid7>".

	SG_ERR_CHECK_RETURN(  SG_STRING__ALLOC__COPY(pCtx,
												 &pMrgCSetEntry->pStringEntryname_OriginalBeforeMadeUnique,
												 pMrgCSetEntry->pStringEntryname)  );
	SG_ERR_CHECK_RETURN(  SG_string__append__sz(pCtx, pMrgCSetEntry->pStringEntryname, "~")  );
	SG_ERR_CHECK_RETURN(  SG_string__append__buf_len(pCtx, pMrgCSetEntry->pStringEntryname,
													 (const SG_byte *)pMrgCSetEntry->bufGid_Entry, 7)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("_make_unique_entrynames: [gid %s][collision %d][conflict %d][portability %d] %s\n"),
							   pMrgCSetEntry->bufGid_Entry,
							   bHadCollision,
							   bHadEntrynameConflict,
							   bHadPortabilityIssue,
							   SG_string__sz(pMrgCSetEntry->pStringEntryname))  );
#endif
}

//////////////////////////////////////////////////////////////////

/**
 * We need to make a change to an ITEM.  See if it has an outstanding ISSUE
 * from a previous UPDATE and deal with it.
 * 
 */
void SG_mrg_cset_entry__check_for_outstanding_issue(SG_context * pCtx,
													SG_mrg * pMrg,
													sg_wc_liveview_item * pLVI,
													SG_mrg_cset_entry * pMrgCSetEntry)
{
	SG_string * pStringRepoPath = NULL;

	SG_UNUSED( pMrgCSetEntry );

	if (!pLVI->pvhIssue)
		return;
	
	// The ITEM already has an ISSUE (we are in a DIRTY MERGE or DIRTY UPDATE or
	// a REVERT-ALL).
	// 
	// The usage model for UPDATE is different
	// from MERGE (even though UPDATE uses MERGE to do the dirty work).  After a
	// MERGE, you can pretty much either COMMIT or REVERT.  But after an UPDATE
	// you are not limited, so we need to think about a chain of UPDATES with
	// dirt being carried forward.
	//
	// Whereas for a REVERT-ALL, we just need to clear the issues as we undo
	// everything.

	if (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)
	{
		// TODO 2012/06/06 With the changes to silently retire already-resolved issues
		// TODO            during the __using_wc() load, we shouldn't get here anymore.
		// TODO            I'm going to leave it in for now.
		// 
		// Because the 'change of basis' to the new baseline needs to be
		// somewhat cleansing, we can get rid of ISSUES that have already
		// been dealt with (and probably have completely stale data).
		//
		// need to queue a delete of the issue and delete temp-dir if present.
		SG_ERR_CHECK(  sg_wc_tx__queue__delete_issue(pCtx, pMrg->pWcTx, pLVI)  );
		return;
	}

	switch (pMrg->eFakeMergeMode)
	{
	case SG_FAKE_MERGE_MODE__MERGE:
	case SG_FAKE_MERGE_MODE__UPDATE:
		// Just give up when we have an unresolved issue in a MERGE or UPDATE.
		// We do not want to deal with a co-mingled/combined issue.
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pMrg->pWcTx, pLVI, &pStringRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_UNRESOLVED_ITEM_WOULD_BE_AFFECTED,
						(pCtx, "'%s' has outstanding unresolved issues.", SG_string__sz(pStringRepoPath))  );

	case SG_FAKE_MERGE_MODE__REVERT:
		SG_ERR_CHECK(  sg_wc_tx__queue__delete_issue(pCtx, pMrg->pWcTx, pLVI)  );
		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

