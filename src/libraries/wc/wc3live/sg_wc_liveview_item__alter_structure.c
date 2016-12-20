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
 * @file sg_wc_liveview_item__alter_structure.c
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
 * Deal with the bookkeeping for pLVD->prb64MovedOutItems
 * in both source and destination directories.
 *
 * [1] For the source directory (pLVI->pLiveViewDir) we DO NOT
 *     want to blindly add the item to the moved-out list.  We
 *     only want to do that IF the item was present in the dir
 *     in the baseline.  That is, if we do something like:
 *         vv move @/a/foo @/b    -- foo gets added to moved-out list of @/a
 *         vv move @/b/foo @/c
 *         vv move @/c/foo @/d
 *     we only want to remember that the item was moved-out of @/a.
 *     (Because an extended status should only report that @/a and
 *     @/d are dirty at this point.)
 *
 * [2] For the destination directory (pLVD_NewParent) we also
 *     need to see if the item is already in the moved-out list
 *     and remove it.  That is, if we do something like:
 *         vv move @/a/foo @/b    -- foo gets added to moved-out list of @/a
 *         vv move @/b/foo @/c
 *         vv move @/c/foo @/a    -- remove foo from moved-out list of @/a
 *     we need to make it look like nothing happened, because
 *     it all nets out to nothing.
 *
 */
static void _deal_with_moved_out_list(SG_context * pCtx,
									  sg_wc_liveview_item * pLVI,
									  sg_wc_liveview_dir * pLVD_NewParent)
{
	SG_bool bPresentInBaseline = SG_FALSE;
	SG_bool bSourceIsBaseline = SG_FALSE;
	SG_bool bDestinationIsBaseline = SG_FALSE;
	SG_bool bFoundInSourceSet = SG_FALSE;
	SG_bool bFoundInDestinationSet = SG_FALSE;

	// NOTE 2012/05/16 Relaxing requirement that pLVI->pPrescanRow is always
	// NOTE            set because of UPDATE can do an ADD-SPECIAL on an item
	// NOTE            that needs to be PARKED because of transient collision.
	// NOTE            See matching note in sg_wc_liveview_item__alloc__add_special().

	bPresentInBaseline = (pLVI->pPrescanRow && pLVI->pPrescanRow->pTneRow);
	if (bPresentInBaseline)
	{
		bSourceIsBaseline = (pLVI->pLiveViewDir->uiAliasGidDir
							 == pLVI->pPrescanRow->pTneRow->p_s->uiAliasGidParent);
		bDestinationIsBaseline = (pLVD_NewParent->uiAliasGidDir
								  == pLVI->pPrescanRow->pTneRow->p_s->uiAliasGidParent);
	}

	if (pLVI->pLiveViewDir->prb64MovedOutItems)
		SG_ERR_CHECK(  SG_rbtree_ui64__find(pCtx,
											pLVI->pLiveViewDir->prb64MovedOutItems,
											pLVI->uiAliasGid,
											&bFoundInSourceSet,
											NULL)  );
	if (pLVD_NewParent->prb64MovedOutItems)
		SG_ERR_CHECK(  SG_rbtree_ui64__find(pCtx,
											pLVD_NewParent->prb64MovedOutItems,
											pLVI->uiAliasGid,
											&bFoundInDestinationSet,
											NULL)  );
#if TRACE_WC_TX_MOVE_RENAME
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MoveOutList: [inBaseline %d][SourceIsBaseline %d][inSource %d][inDestination %d] %s\n",
							   bPresentInBaseline,
							   bSourceIsBaseline,
							   bFoundInSourceSet,
							   bFoundInDestinationSet,
							   SG_string__sz(pLVI->pStringEntryname))  );
#endif

	if (!bPresentInBaseline)
	{
		if (bFoundInSourceSet || bFoundInDestinationSet)
			SG_ERR_THROW2(  SG_ERR_ASSERT,
							(pCtx, "Item '%s' was not present in baseline; it should not appear in MOVE-OUT sets.",
							 SG_string__sz(pLVI->pStringEntryname))  );
		return;
	}

	if (bFoundInSourceSet)		// can't move it out twice without ever moving it back in first.
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "Item '%s' being moved out of a directory, but already in MOVE-OUT set.",
						 SG_string__sz(pLVI->pStringEntryname))  );

	SG_ASSERT( (bFoundInDestinationSet == bDestinationIsBaseline) );

	// [1] 

	if (bSourceIsBaseline)
	{
		if (!pLVI->pLiveViewDir->prb64MovedOutItems)
			SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pLVI->pLiveViewDir->prb64MovedOutItems)  );
		SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
													   pLVI->pLiveViewDir->prb64MovedOutItems,
													   pLVI->uiAliasGid,
													   NULL)  );
	}

	// [2] 

	if (bDestinationIsBaseline)
	{
		SG_ERR_CHECK(  SG_rbtree_ui64__remove__with_assoc(pCtx,
														  pLVD_NewParent->prb64MovedOutItems,
														  pLVI->uiAliasGid,
														  NULL)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Alter the LVI to reflect a MOVE and/or RENAME.
 * This will create and/or update the pLVI->pPcRow_PC
 * to reflect the "new" current values for the item.
 *
 * We DO NOT know about the QUEUE or JOURNAL, so we
 * do not update them.  The caller needs to do this.
 *
 */
void sg_wc_liveview_item__alter_structure__move_rename(SG_context * pCtx,
													   sg_wc_liveview_item * pLVI,
													   sg_wc_liveview_dir * pLVD_NewParent,
													   const char * pszNewEntryname)
{
	// we potentially have 4 values for the parent-directory
	// and for the entryname:
	// 
	// [1] the new values given as args to this routine.
	// 
	// [2] the LIVE IN-TX values from an earlier QUEUED
	//     operation in THIS TX.  (stored in pLVI->pPcRow_PC)
	//
	// [3] the PRE-TX value if it has a pending move/rename
	//     from an earlier TX.    (stored in pLVI->pPrescanRow->pPcRow_Ref)
	//
	// [4] the baseline value.    (stored in pLVI->pPrescanRow->pTneRow)
	//
	// Start with [2] -- falling back to [3] or [4].

	if (!pLVI->pPcRow_PC)
	{
		// If the item has not yet seen a structural change *IN THIS TX*.
		// it won't have a pPcRow_PC because the initial
		// alloc/clone does not create this field.
		//
		// The first structural change during this TX must
		// copy the PRE-TX state (either from an existing
		// pending change or from the baseline).

		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow) );	// by construction.
		if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__copy(pCtx,
														 &pLVI->pPcRow_PC,
														 pLVI->pPrescanRow->pPcRow_Ref)  );
		}
		else if (pLVI->pPrescanRow->pTneRow)
		{
			SG_bool bMarkItSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__from_tne_row(pCtx,
																 &pLVI->pPcRow_PC,
																 pLVI->pPrescanRow->pTneRow,
																 bMarkItSparse)  );
		}
		else
		{
			// We could assert pLVI->pPrescanRow->pRD
			// and/or that it is a FOUND/IGNORED item
			// and/or complain that we don't deal with
			// moves/renames of uncontrolled items.
			//
			// I don't think this code is reachable.
			SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pRD != NULL) );
			SG_ASSERT_RELEASE_FAIL( (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live)) );
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
	}

	// Update [2] with [1] as necessary.
	//
	// There are several (probably redundant) copies here to deal with:
	// 
	// [a] the __state_structural itself.

	pLVI->pPcRow_PC->p_s->uiAliasGidParent = pLVD_NewParent->uiAliasGidDir;
	if (strcmp(pszNewEntryname,
			   pLVI->pPcRow_PC->p_s->pszEntryname) != 0)
	{
		SG_NULLFREE(pCtx, pLVI->pPcRow_PC->p_s->pszEntryname);
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszNewEntryname, &pLVI->pPcRow_PC->p_s->pszEntryname)  );
	}

	// [b] the LVI convenience fields and the LVDs.

	if (strcmp(pszNewEntryname,
			   SG_string__sz(pLVI->pStringEntryname)) != 0)
	{
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pLVI->pStringEntryname, pszNewEntryname)  );
	}
	if (pLVI->pLiveViewDir != pLVD_NewParent)
	{
		SG_ERR_CHECK(  _deal_with_moved_out_list(pCtx, pLVI, pLVD_NewParent)  );

		SG_ERR_CHECK(  SG_rbtree_ui64__remove__with_assoc(pCtx, pLVI->pLiveViewDir->prb64LiveViewItems,
														  pLVI->uiAliasGid, NULL)  );
		SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx, pLVD_NewParent->prb64LiveViewItems,
													   pLVI->uiAliasGid, pLVI)  );
		pLVI->pLiveViewDir = pLVD_NewParent;
	}

	// [c] With the current [1] values in place, set the
	//     flags_net to indicate the state RELATIVE TO
	//     THE BASELINE (if present).
	//
	//     This will secretly *NOT* set a IS_MOVED/IS_RENAMED
	//     bit for something that was not present in the
	//     baseline (since we just want to report that it was
	//     ADDED with the new name/location).
	//
	// NOTE 2012/05/16 Relaxing requirement that pLVI->pPrescanRow is always
	// NOTE            set because of UPDATE can do an ADD-SPECIAL on an item
	// NOTE            that needs to be PARKED because of transient collision.
	// NOTE            See matching note in sg_wc_liveview_item__alloc__add_special().

	if (pLVI->pPrescanRow && pLVI->pPrescanRow->pTneRow)
	{
		if (strcmp(pszNewEntryname,
				   pLVI->pPrescanRow->pTneRow->p_s->pszEntryname) == 0)
			pLVI->pPcRow_PC->flags_net &= ~SG_WC_DB__PC_ROW__FLAGS_NET__RENAMED;
		else
			pLVI->pPcRow_PC->flags_net |=  SG_WC_DB__PC_ROW__FLAGS_NET__RENAMED;

		if (pLVD_NewParent->uiAliasGidDir == pLVI->pPrescanRow->pTneRow->p_s->uiAliasGidParent)
			pLVI->pPcRow_PC->flags_net &= ~SG_WC_DB__PC_ROW__FLAGS_NET__MOVED;
		else
			pLVI->pPcRow_PC->flags_net |=  SG_WC_DB__PC_ROW__FLAGS_NET__MOVED;
	}
	else
	{
		pLVI->pPcRow_PC->flags_net &= ~SG_WC_DB__PC_ROW__FLAGS_NET__RENAMED;
		pLVI->pPcRow_PC->flags_net &= ~SG_WC_DB__PC_ROW__FLAGS_NET__MOVED;
	}

	// [d] We DO NOT try to put this item in canonical form
	//     at this time.  That is, if we just cleared all of
	//     the bits in FLAGS_NET, we could in theory delete
	//     both the pPcRow_PC and the pPcRow_Ref (in the
	//     PrescanRow data) and implicitly indicate that this
	//     item hasn't changed from the baseline.
	//
	//     But this is too much at this point (and I've changed
	//     things so that the PrescanRow data is considered
	//     constant), so we leave the pPcRow_PC here with zero
	//     flags and know that it will be dealt with during the
	//     APPLY phase.

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Alter the LVI to reflect an ADD.
 * This will create the pLVI->pPcRow_PC to reflect the new values
 * (using info collected by SCANDIR/READDIR).
 *
 * Note that a normal ADD operation in the normal case converts
 * an uncontrolled item intot a controlled item.  That is we can
 * say that there is already a LVI for the FOUND/IGNORED item
 * and we are converting it.  This also means that there was
 * something on disk for SCANDIR to have observed.
 *
 * WE DO NOT USE THIS ROUTINE for items added by MERGE
 * because MERGE has special needs.
 * 
 * We DO NOT know about the QUEUE or JOURNAL, so we
 * do not update them.  The caller needs to do this.
 *
 */
void sg_wc_liveview_item__alter_structure__add(SG_context * pCtx,
											   sg_wc_liveview_item * pLVI)
{
	// pPcRow_PC is created by the first structural change.
	// 
	// ADD should be the first structural change that causes a PC ROW
	// to be created.  That is, it shouldn't be possible to have had
	// a previous MOVE, RENAME, or DELETE.

	if (pLVI->pPcRow_PC)
	{
		// Screw Case: When you the following in the same TX:
		//     var tx = new sg_wc_tx();
		//     tx.add(    { "src" : "foo" } );
		//     tx.remove( { "src" : "foo" } );
		//     tx.add(    { "src" : "foo" } );
		// 
		// the REMOVE will get converted to an UNDO-ADD (and the
		// LVI will be converted to a "special" FOUND item and the
		// pPcRow_PC will remain (with __FLAGS_NET__INVALID)).
		// [see sg_wc_liveview_item__alter_structure__undo_add()]
		// 
		// then second ADD will see the pPcRow_PC present.
		//
		// so we change it BACK into an ADD.  This will REUSE
		// the AliasGid (and the associated GID).  Not sure if
		// I like that or not.

		pLVI->scan_flags_Live = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED;
		pLVI->pPcRow_PC->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ADDED;
	}
	else
	{
		SG_ASSERT_RELEASE_FAIL( (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live)) );
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow) );                      // by construction.
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pRD != NULL) );         // by construction.
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pTneRow == NULL) );     // by construction.
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pPcRow_Ref == NULL) );  // by construction.

		SG_ERR_CHECK(  SG_WC_DB__PC_ROW__ALLOC__FROM_READDIR(pCtx,
															 &pLVI->pPcRow_PC,
															 pLVI->pPrescanRow->pRD,
															 pLVI->uiAliasGid,
															 ((pLVI->pLiveViewDir)
															  ? pLVI->pLiveViewDir->uiAliasGidDir
															  : SG_WC_DB__ALIAS_GID__NULL_ROOT))  );

		pLVI->scan_flags_Live = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED;
		pLVI->pPcRow_PC->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ADDED;
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Alter the LVI to reflect an DELETE.
 * This will create the pLVI->pPcRow_PC to reflect the new values
 * (using info collected by SCANDIR/READDIR).
 *
 * We DO NOT know about the QUEUE or JOURNAL, so we
 * do not update them.  The caller needs to do this.
 *
 */
void sg_wc_liveview_item__alter_structure__remove(SG_context * pCtx,
												  sg_wc_liveview_item * pLVI,
												  SG_bool bFlattenAddSpecialPlusRemove)
{
	// pPcRow_PC is created by the first structural change
	// during **this** TX.  it may or may not already be
	// present.  (for example, there will be if there was
	// a previous MOVE/RENAME during this TX.)

	if (!pLVI->pPcRow_PC)
	{
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow) );	// by construction.
		if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__copy(pCtx,
														 &pLVI->pPcRow_PC,
														 pLVI->pPrescanRow->pPcRow_Ref)  );
		}
		else if (pLVI->pPrescanRow->pTneRow)
		{
			SG_bool bMarkItSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__from_tne_row(pCtx,
																 &pLVI->pPcRow_PC,
																 pLVI->pPrescanRow->pTneRow,
																 bMarkItSparse)  );
		}
		else if (pLVI->pPrescanRow->pRD)
		{
			SG_ASSERT_RELEASE_FAIL( (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live)) );

			// We don't really want to deal with deleting FOUND and/or IGNORED
			// items (because they aren't under version control and we historically
			// haven't done that), but it would be convenient to let the user
			// do a 'vv remove --force' on a directory and we go ahead and delete
			// it and any trash that we find.

			SG_ERR_CHECK(  SG_WC_DB__PC_ROW__ALLOC__FROM_READDIR(pCtx,
																 &pLVI->pPcRow_PC,
																 pLVI->pPrescanRow->pRD,
																 pLVI->uiAliasGid,
																 ((pLVI->pLiveViewDir)
																  ? pLVI->pLiveViewDir->uiAliasGidDir
																  : SG_WC_DB__ALIAS_GID__NULL_ROOT))  );
		}
		else
		{
			// not possible.
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
	}

	if (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__INVALID)
	{
		// an earlier REMOVE operation *IN THIS TX* on this item
		// has already happened and the item was changed to a
		// FOUND item via an UNDO-ADD.
		//
		// something caused us to revisit this item, such as:
		//     vv add foo
		//     vv remove foo foo
		// and we are here because of argv[2].
		// don't re-alter the fields.
		SG_ASSERT_RELEASE_FAIL( (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live)) );
	}
	else
	{
		// we mark it deleted in the LVI so that it has proper state (in
		// case it gets referenced by a later operation during this TX).

#if 0
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "TEST: __alter_structure__remove: [prior flags_net %x] [bFlatten %d] %s\n",
								   pLVI->pPcRow_PC->flags_net,
								   bFlattenAddSpecialPlusRemove,
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif
		pLVI->scan_flags_Live       =  SG_WC_PRESCAN_FLAGS__CONTROLLED_INACTIVE_DELETED;
		pLVI->pPcRow_PC->flags_net |=  SG_WC_DB__PC_ROW__FLAGS_NET__DELETED;
		if (bFlattenAddSpecialPlusRemove
			&& ((pLVI->pPcRow_PC->flags_net & (SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_M
											   |SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U)) != 0))
			pLVI->pPcRow_PC->flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__FLATTEN;

		// NOTE 2012/12/06 I'm going to say that REMOVE *does not* turn off SPARSE.
		// NOTE            This allows a REVERT-ALL of a DELETED SPARSE item to 
		// NOTE            restore it in a SPARSE state.

		// TODO 2012/03/08 Should this strip __MOVED/__RENAMED and reset the
		// TODO            corresponding _s_ fields ?  W0065
		// TODO
		// TODO 2012/05/24 I'm going to say no. I think it is nice to have
		// TODO            the extra info (in most cases).  If we do want to
		// TODO            clear these, we need to think about the moved-out
		// TODO            stuff and conceptually put it back where it was
		// TODO            and delete it (to be consistent) and I'm not sure
		// TODO            that is worth the bother.
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Alter the LVI to reflect an UNDO_ADD.  This is like a SPECIAL CASE
 * of REMOVE where they try to REMOVE an ADDED item.  We want to
 * convert this item in-place to a FOUND item and we (probably) DO NOT
 * want to delete the item from the disk.
 *
 * We can't flat-out delete this LVI and cause a new one to get created
 * for the (newly) FOUND item because of iterators on the parent LVD and
 * we don't want to invalidate any of the TX lvi/lvd caches.
 *
 * We want the item to have the same Alias-GID (at least until the
 * end of the TX).  And eventually we will want to mark the actual
 * GID as a TEMP so that it can be reclaimed.
 *
 */
void sg_wc_liveview_item__alter_structure__undo_add(SG_context * pCtx,
													sg_wc_liveview_item * pLVI)
{
	SG_ASSERT( (pLVI->pPrescanRow) );                      // by construction.
	SG_ASSERT( (pLVI->pPrescanRow->pTneRow == NULL) );     // by construction.
	// pLVI->pPrescanRow->pPcRow_Ref may or may not be set

	// Since it was added at some point, it must have a PC row.
	// But it depends on when it was ADDED (earlier in this TX
	// vs in a previous TX) and/or whether it has been modified
	// during this TX as to whether we have a
	//          pLVI->pPrescanRow->pPcRow_Ref
	// and/or   pLVI->pPcRow_PC

	SG_ASSERT(  ((pLVI->pPrescanRow->pPcRow_Ref) || (pLVI->pPcRow_PC))  );

	if (!pLVI->pPcRow_PC)
	{
		if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__copy(pCtx,
														 &pLVI->pPcRow_PC,
														 pLVI->pPrescanRow->pPcRow_Ref)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "UNDO_ADD but not PC nor Ref_PC ? '%s'",
							 SG_string__sz(pLVI->pStringEntryname))  );
		}
	}

	SG_ASSERT(  ((SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live)) == 0)  );
	SG_ASSERT(  ((SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live)) == 0)  );
	SG_ASSERT(  ((SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_ROOT(pLVI->scan_flags_Live)) == 0)  );
	SG_ASSERT(  ((SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI->scan_flags_Live)) == 0)  );
	SG_ASSERT(  ((SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live)) == 0)  );
	
	SG_ASSERT( ((SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_MATCHED(pLVI->scan_flags_Live))
				|| (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI->scan_flags_Live))) );

	// we do not alter anything in pLVI->pPrescanRow because it
	// represents the state at the BEGINNING of the TX.

	pLVI->scan_flags_Live = SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED;

	// I debated deleting the pLVI->pPcRow_PC now
	// but the pLVI->pPcRow_PC knows about any pending
	// moves/renames and etc.  And we need it to build
	// the SQL to delete the row from the tbl_PC table
	// (if it was added by a previous TX).  So we keep
	// it for now.
	//
	// In a subsequent TX, this item will appear as a
	// "normal" FOUND item (without a pPcRow_PC).  Until
	// then we'll just have to have a somewhat "special"
	// FOUND item.
	//
	// The row in the tbl_PC table will be deleted during
	// the APPLY phase.  Put a bogus status in the
	// pPcRow_PC flags until then.

	pLVI->pPcRow_PC->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__INVALID;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__alter_structure__undo_delete(SG_context * pCtx,
													   sg_wc_liveview_item * pLVI,
													   sg_wc_liveview_dir * pLVD_NewParent,
													   const char * pszNewEntryname)
{
	SG_ASSERT_RELEASE_FAIL(  (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI->scan_flags_Live))  );

	// pPcRow_PC is created by the first structural change
	// during **this** TX.  it may or may not already be
	// present.  (for example, there will be if there was
	// a previous MOVE/RENAME during this TX.)
	
	if (!pLVI->pPcRow_PC)
	{
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow) );	// by construction.
		if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__copy(pCtx,
														 &pLVI->pPcRow_PC,
														 pLVI->pPrescanRow->pPcRow_Ref)  );
		}
		else if (pLVI->pPrescanRow->pTneRow)
		{
			SG_bool bMarkItSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__from_tne_row(pCtx,
																 &pLVI->pPcRow_PC,
																 pLVI->pPrescanRow->pTneRow,
																 bMarkItSparse)  );
		}
		else if (pLVI->pPrescanRow->pRD)
		{
			// not possible.
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
		else
		{
			// not possible.
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
	}

	SG_ASSERT_RELEASE_FAIL(  (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__DELETED)  );
	
	// we clear the various __DELETED bits in the LVI so that
	// it has proper state (in case it gets referenced by a
	// later operation during this TX).
	//
	// WE DO NOT ACTUALLY POPULATE IT (restore the file).
	// That is the job of the caller (during the APPLY phase).
	//
	// We also DO NOT know whether this item's entryname can
	// be safely inserted into the directory.  That too is a
	// job of the caller.
	//
	// We mark the item as normal.

	pLVI->pPcRow_PC->flags_net &= ~SG_WC_DB__PC_ROW__FLAGS_NET__DELETED;
	pLVI->scan_flags_Live       =  SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN;

	if (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE)
		pLVI->scan_flags_Live   =  SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE;	// not "|="
	
	// Go ahead and force any move/rename now as a step in
	// the undelete-proper.  This will force the caller to
	// pre-compute everything (including __can_add_new_entry())
	// and avoid the situation where we do the undelete (into
	// the original (parent,entryname) in one step and then
	// require them to call move/rename in a second step and
	// have a transient collision with the original name/parent.
	//
	// That is, all we want is to force pLVI to get updated.

	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__move_rename(pCtx, pLVI,
																	 pLVD_NewParent,
																	 pszNewEntryname)  );

fail:
	return;
}

void sg_wc_liveview_item__alter_structure__undo_lost(SG_context * pCtx,
													 sg_wc_liveview_item * pLVI,
													 sg_wc_liveview_dir * pLVD_NewParent,
													 const char * pszNewEntryname)
{
	SG_ASSERT_RELEASE_FAIL(  (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI->scan_flags_Live))  );

	pLVI->scan_flags_Live       =  SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN;

	// Go ahead and force any move/rename now as a step in
	// the undelete-proper.  This will force the caller to
	// pre-compute everything (including __can_add_new_entry())
	// and avoid the situation where we restore the item (into
	// the original (parent,entryname) in one step and then
	// require them to call move/rename in a second step and
	// have a transient collision with the original name/parent.
	//
	// That is, all we want is to force pLVI to get updated.

	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__move_rename(pCtx, pLVI,
																	 pLVD_NewParent,
																	 pszNewEntryname)  );

fail:
	return;
}

/**
 * Normally when MERGE needs to set the {content-hid, attrbits} fields,
 * we can just do it in the WD and not mess with the tbl_PC row.  But
 * if the item is SPARSE (before the merge), then we have new sparse-
 * fields in the table that need to be updated so that they reflect
 * the post-merge values.
 *
 */
void sg_wc_liveview_item__alter_structure__overwrite_sparse_fields(SG_context * pCtx,
																   sg_wc_liveview_item * pLVI,
																   const char * pszHid,
																   SG_int64 attrbits)
{
	SG_bool bMarkItSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
	SG_ASSERT_RELEASE_RETURN( (bMarkItSparse) );

	if (!pLVI->pPcRow_PC)		// We have not yet modified this item in this TX
	{
		// We can assert these since a sparse item should always have a
		// pPcRow_Ref with the existing {sparse-hid,sparse-attrbits} fields.
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow) );
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pPcRow_Ref) );
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pPcRow_Ref->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE) );
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse) );

		SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__copy(pCtx,
													 &pLVI->pPcRow_PC,
													 pLVI->pPrescanRow->pPcRow_Ref)  );
	}

#if TRACE_WC_MERGE
	{
		SG_int_to_string_buffer bufui64_a;
		SG_int_to_string_buffer bufui64_b;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "AlterStructreOverwriteSparseFields: [%s ==> %s] ['%s' ==> '%s'] %s\n",
								   pLVI->pPcRow_PC->p_d_sparse->pszHid, pszHid,
								   SG_uint64_to_sz__hex(pLVI->pPcRow_PC->p_d_sparse->attrbits, bufui64_a),
								   SG_uint64_to_sz__hex(attrbits, bufui64_b),
								   SG_string__sz(pLVI->pStringEntryname))  );
	}
#endif

	pLVI->pPcRow_PC->p_d_sparse->attrbits = attrbits;
	SG_NULLFREE(pCtx, pLVI->pPcRow_PC->p_d_sparse->pszHid);
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHid, &pLVI->pPcRow_PC->p_d_sparse->pszHid)  );

fail:
	return;
}


/**
 *
 *
 */
void sg_wc_liveview_item__alter_structure__overwrite_ref_attrbits(SG_context * pCtx,
																  sg_wc_liveview_item * pLVI,
																  SG_int64 attrbits)
{
	SG_bool bMarkItSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
	SG_ASSERT_RELEASE_RETURN( (!bMarkItSparse) );

	if (!pLVI->pPcRow_PC)		// We have not yet modified this item in this TX
	{
		SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow) );	// by construction.
		if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__copy(pCtx,
														 &pLVI->pPcRow_PC,
														 pLVI->pPrescanRow->pPcRow_Ref)  );
		}
		else if (pLVI->pPrescanRow->pTneRow)
		{
			SG_bool bMarkItSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
			SG_ERR_CHECK(  sg_wc_db__pc_row__alloc__from_tne_row(pCtx,
																 &pLVI->pPcRow_PC,
																 pLVI->pPrescanRow->pTneRow,
																 bMarkItSparse)  );
		}
		else
		{
			SG_ASSERT_RELEASE_FAIL( (pLVI->pPrescanRow->pRD != NULL) );
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
	}

#if TRACE_WC_ATTRBITS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "AlterStructreOverwriteRefAttrbits: [ref-attr %d --> %d] %s\n",
							   ((SG_uint32)pLVI->pPcRow_PC->ref_attrbits),
							   ((SG_uint32)attrbits),
							   SG_string__sz(pLVI->pStringEntryname))  );
#endif

	pLVI->pPcRow_PC->ref_attrbits = attrbits;

fail:
	return;
}
