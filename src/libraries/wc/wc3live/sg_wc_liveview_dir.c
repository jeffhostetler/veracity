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
 * @file sg_wc_liveview_dir.c
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

void sg_wc_liveview_dir__free(SG_context * pCtx,
							  sg_wc_liveview_dir * pLVD)
{
	if (!pLVD)
		return;

	// we do not own pLVD->pPrescanDir
	// we do not own the pLVI inside pLVD->prb64LiveViewItems.

	SG_RBTREE_UI64_NULLFREE(pCtx, pLVD->prb64LiveViewItems);

	SG_RBTREE_UI64_NULLFREE(pCtx, pLVD->prb64MovedOutItems);

	SG_TREENODE_NULLFREE(pCtx, pLVD->pTN_commit);

	SG_NULLFREE(pCtx, pLVD);
}

//////////////////////////////////////////////////////////////////

struct _clone_cb_data
{
	SG_wc_tx *					pWcTx;
	sg_wc_liveview_dir *		pLVD;
};

/**
 * Add a liveview_item to the liveview_dir for each
 * prescan_row that is in the prescan_dir.
 *
 */
static SG_rbtree_ui64_foreach_callback _clone_cb;

static void _clone_cb(SG_context * pCtx,
					  SG_uint64 uiAliasGid,
					  void * pVoidAssoc,
					  void * pVoidData)
{
	struct _clone_cb_data * pData = (struct _clone_cb_data *)pVoidData;
	sg_wc_prescan_row * pPrescanRow = (sg_wc_prescan_row *)pVoidAssoc;
	sg_wc_liveview_item * pLVI_Allocated = NULL;

	SG_ERR_CHECK(  SG_WC_LIVEVIEW_ITEM__ALLOC__CLONE_FROM_PRESCAN(pCtx,
																  &pLVI_Allocated,
																  pData->pWcTx,
																  pPrescanRow)  );

	// we are responsible for setting the backptr and
	// inserting the item in the LVD's rbtree.
	// the LVD DOES NOT take ownership.

	pLVI_Allocated->pLiveViewDir = pData->pLVD;
	SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
												   pData->pLVD->prb64LiveViewItems,
												   uiAliasGid,
												   pLVI_Allocated)  );

	// insert into cache and take ownership.

	SG_ERR_CHECK(  sg_wc_tx__cache__add_liveview_item(pCtx, pData->pWcTx, pLVI_Allocated)  );
	pLVI_Allocated = NULL;
	return;

fail:
	SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx, pLVI_Allocated);
}

/**
 * Create a LVD for the given alias and SCANDIR/READDIR.
 * 
 * This is the "live" (at-this-point-in-the-tx) version of this
 * directory; just as the scandir/readdir is the pre-tx version.
 *
 * BY CONSTRUCTION: Since we cannot operate on a directory in
 * the transaction *UNTIL* we have a liveview_dir on hand, there
 * CANNOT be *any* in-tx structural changes to the contents of
 * this directory.  So the liveview_dir that we need to create
 * is an EXACT CLONE of the scandir/readdir.
 *
 * This LiveViewDir (and all LiveViewItems within) are added
 * to the TX caches and are owned by the caches.  So you DO NOT
 * own the returned pointer.
 *
 */
void sg_wc_liveview_dir__alloc__clone_from_prescan(SG_context * pCtx,
												   SG_wc_tx * pWcTx,
												   sg_wc_prescan_dir * pPrescanDir,
												   sg_wc_liveview_dir ** ppLiveViewDir)
{
	SG_uint64 uiAliasGid = pPrescanDir->uiAliasGidDir;
	sg_wc_liveview_dir * pLVD = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pLVD)  );

	pLVD->uiAliasGidDir = uiAliasGid;
	pLVD->pPrescanDir = pPrescanDir;
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pLVD->prb64LiveViewItems)  );

	{
		struct _clone_cb_data data;

		memset(&data, 0, sizeof(data));
		data.pWcTx = pWcTx;
		data.pLVD = pLVD;

		SG_ERR_CHECK(  sg_wc_prescan_dir__foreach(pCtx, pPrescanDir, _clone_cb, &data)  );
	}
	
	if (pPrescanDir->prb64MovedOutItems)
	{
		SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pLVD->prb64MovedOutItems)  );
		SG_ERR_CHECK(  SG_rbtree__add__from_other_rbtree(pCtx,
														 // SG_rbtree_ui64 is a "subclass" of SG_rbtree.
														 (SG_rbtree *)pLVD->prb64MovedOutItems,
														 (SG_rbtree *)pPrescanDir->prb64MovedOutItems)  );
	}

	// add the LiveViewDir to the cache and take ownership.

	SG_ERR_CHECK(  sg_wc_tx__cache__add_liveview_dir(pCtx, pWcTx, pLVD)  );

	*ppLiveViewDir = pLVD;
	return;

fail:
	SG_WC_LIVEVIEW_DIR__NULLFREE(pCtx, pLVD);
}

/**
 * Create a LVD for an ADDED-by-MERGE directory.
 *
 * This directory was not in the baseline and was not
 * observed by SCANDIR/READDIR; it is in the process
 * of being added by MERGE.
 *
 * The LVD for this directory gets created during the
 * QUEUE phase in the TX, but the directory on disk
 * doesn't get created until the APPLY phase.
 *
 * So we basically need to create an empty LVD for
 * it so that MERGE can also queue the special-add
 * of things in this directory.
 *
 */
void sg_wc_liveview_dir__alloc__add_special(SG_context * pCtx,
											SG_wc_tx * pWcTx,
											SG_uint64 uiAliasGid,
											sg_wc_liveview_dir ** ppLVD)
{
	sg_wc_liveview_dir * pLVD = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pLVD)  );

	pLVD->uiAliasGidDir = uiAliasGid;
	pLVD->pPrescanDir = NULL;
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pLVD->prb64LiveViewItems)  );

	// since the directory is being special-added, it
	// can't have any moved-out items.

	// add the LiveViewDir to the cache and take ownership.

	SG_ERR_CHECK(  sg_wc_tx__cache__add_liveview_dir(pCtx, pWcTx, pLVD)  );

	*ppLVD = pLVD;
	return;

fail:
	SG_WC_LIVEVIEW_DIR__NULLFREE(pCtx, pLVD);
	
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_dir__foreach(SG_context * pCtx, sg_wc_liveview_dir * pLVD,
								 SG_rbtree_ui64_foreach_callback * pfn_cb,
								 void * pVoidData)
{
	if (pLVD->prb64LiveViewItems)
		SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__foreach(pCtx,
													  pLVD->prb64LiveViewItems,
													  pfn_cb,
													  pVoidData)  );
}


//////////////////////////////////////////////////////////////////

void sg_wc_liveview_dir__foreach_moved_out(SG_context * pCtx, sg_wc_liveview_dir * pLVD,
										   SG_rbtree_ui64_foreach_callback * pfn_cb,
										   void * pVoidData)
{
	if (pLVD->prb64MovedOutItems)
		SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__foreach(pCtx,
													  pLVD->prb64MovedOutItems,
													  pfn_cb,
													  pVoidData)  );
}

//////////////////////////////////////////////////////////////////


/**
 * Find ALL LVIs in the given directory that currently
 * have the given entryname.
 *
 * Accumulate the matched items in the given vector;
 * the vector does not take ownership of the pointers
 * within.
 * 
 */
static void _find_all_items_with_entryname__with_flag(SG_context * pCtx,
													  const sg_wc_liveview_dir * pLVD,
													  const char * pszEntryname,
													  sg_wc_liveview_dir__find_flags findFlags,
													  SG_vector * pVecItemsFound)
{
	SG_rbtree_ui64_iterator * pIter = NULL;
	SG_uint64 uiAliasGid_k;
	sg_wc_liveview_item * pLVI_k;		// we do not own this
	SG_bool bOK;

	SG_ERR_CHECK(  SG_rbtree_ui64__iterator__first(pCtx, &pIter,
												   pLVD->prb64LiveViewItems,
												   &bOK, &uiAliasGid_k, (void **)&pLVI_k)  );
	while (bOK)
	{
		if (strcmp(pszEntryname,
				   SG_string__sz(pLVI_k->pStringEntryname)) == 0)
		{
			switch (pLVI_k->scan_flags_Live)
			{
			case SG_WC_PRESCAN_FLAGS__RESERVED:
				if (findFlags & SG_WC_LIVEVIEW_DIR__FIND_FLAGS__RESERVED)
					SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pVecItemsFound, pLVI_k, NULL)  );
				break;

			case SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED:
				if (findFlags & SG_WC_LIVEVIEW_DIR__FIND_FLAGS__UNCONTROLLED)
					SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pVecItemsFound, pLVI_k, NULL)  );
				break;

			case SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT:
			case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED:
			case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN:
				if (findFlags & SG_WC_LIVEVIEW_DIR__FIND_FLAGS__MATCHED)
					SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pVecItemsFound, pLVI_k, NULL)  );
				break;

			case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE:
				if (findFlags & SG_WC_LIVEVIEW_DIR__FIND_FLAGS__SPARSE)
					SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pVecItemsFound, pLVI_k, NULL)  );
				break;

			case SG_WC_PRESCAN_FLAGS__CONTROLLED_INACTIVE_DELETED:
				if (findFlags & SG_WC_LIVEVIEW_DIR__FIND_FLAGS__DELETED)
					SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pVecItemsFound, pLVI_k, NULL)  );
				break;

			case SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_LOST:
				if (findFlags & SG_WC_LIVEVIEW_DIR__FIND_FLAGS__LOST)
					SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pVecItemsFound, pLVI_k, NULL)  );
				break;

			default:
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "find_all_items_with_entryname: unknown scan_flag '0x%x' for '%s'.",
								 (SG_uint32)pLVI_k->scan_flags_Live, pszEntryname)  );
			}
		}

		SG_ERR_CHECK(  SG_rbtree_ui64__iterator__next(pCtx, pIter,
													  &bOK, &uiAliasGid_k, (void **)&pLVI_k)  );
	}

fail:
	SG_RBTREE_UI64_ITERATOR_NULLFREE(pCtx, pIter);
}

/**
 * Find ***ALL*** of the LVIs in the given directory
 * that currently have the given entryname.
 *
 * Because we now allow moves/renames + deletes,
 * (and sparse + found) an
 * individual entryname is not necessarily unique.
 * 
 * This routine finds the *all* of the items that match
 * the entryname and with the find-flag restrictions.
 *
 * You own the returned vector, but the vector DOES NOT
 * own the LVI pointers within.
 * 
 */
void sg_wc_liveview_dir__find_item_by_entryname(SG_context * pCtx,
												const sg_wc_liveview_dir * pLVD,
												const char * pszEntryname,
												sg_wc_liveview_dir__find_flags findFlags,
												SG_vector ** ppVecItemsFound)
{
	SG_vector * pVecItemsFound = NULL;

	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVecItemsFound, 10)  );
	
	SG_ERR_CHECK(  _find_all_items_with_entryname__with_flag(pCtx, pLVD, pszEntryname, findFlags,
															 pVecItemsFound)  );
	*ppVecItemsFound = pVecItemsFound;
	return;
	
fail:
	SG_VECTOR_NULLFREE(pCtx, pVecItemsFound);
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_dir__find_item_by_alias(SG_context * pCtx,
											const sg_wc_liveview_dir * pLVD,
											SG_uint64 uiAliasGid,
											SG_bool * pbFound,
											sg_wc_liveview_item ** ppLVI)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__find(pCtx, pLVD->prb64LiveViewItems,
											   uiAliasGid,
											   pbFound, (void **)ppLVI)  );
}

//////////////////////////////////////////////////////////////////

/**
 * SPECIAL-ADD an MERGE-CREATED or UPDATE-CREATED item to this directory.
 *
 * MERGE uses this to populate the Live state with
 * a new-ish item.  There are 2 ways to get here:
 * [a] This item did not appear in the baseline, so it
 *     doesn't have an LVI.  It was not observed by
 *     SCANDIR, so it doesn't have any READDIR/SCAN info.
 *     It does have an existing Alias/GID that we need
 *     to associate with it.
 * [b] There was a dirty delete in the WC before the
 *     MERGE started.  So it DOES have an LVI, it's
 *     just that MERGE didn't get to see it.
 *
 * We need to:
 * [] Create a LVI for the item.
 * [] Hook the LVI into the appropriate LVD.
 *
 * We DO NOT USE THIS ROUTINE for a plain ADD.
 *
 * We DO NOT know about the QUEUE or JOURNAL, so we
 * do not update them.  The caller needs to do this.
 *
 * Nor do we actually create the item on disk.
 * We only update the LVI/LVD data structures at
 * this level.
 *
 * For non-directory items, pszHidMerge should be set
 * to the content HID of the item.  This will let us
 * tell whether the file is dirty w/r/t the merge and/or
 * can be considered disposable.
 * 
 */
void sg_wc_liveview_dir__add_special(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 sg_wc_liveview_dir * pLVD_Parent,
									 const char * pszEntryname,
									 SG_uint64 uiAliasGid,
									 SG_treenode_entry_type tneType,
									 const char * pszHidMerge,
									 SG_int64 attrbits,
									 SG_wc_status_flags statusFlagsAddSpecialReason,
									 sg_wc_liveview_item ** ppLVI_New)
{
	sg_wc_liveview_item * pLVI_Allocated = NULL;
	sg_wc_liveview_item * pLVI;

	// Use the LVD for the parent directory and see if we
	// can add this entryname without potential/real collisions.
	// When MERGE calls us, it should have pre-approved the
	// entryname WRT *FINAL* collisions, but it still allows
	// *TRANSIENT* collisions to happen.
	//
	// If we do detect a (transient) collision, we throw.
	// (If this special-add is important to the caller they
	// can deal with it -- using the parking lot for example.)

	SG_ERR_CHECK(  sg_wc_liveview_dir__can_add_new_entryname(pCtx,
															 pWcTx->pDb,
															 pLVD_Parent,
															 NULL,
															 NULL,
															 pszEntryname,
															 tneType,
															 SG_FALSE)  );

	SG_ERR_CHECK(  SG_WC_LIVEVIEW_ITEM__ALLOC__ADD_SPECIAL(pCtx,
														   &pLVI_Allocated,
														   pWcTx,
														   uiAliasGid,
														   pLVD_Parent->uiAliasGidDir,
														   pszEntryname,
														   tneType,
														   pszHidMerge,
														   attrbits,
														   statusFlagsAddSpecialReason)  );

	// we are responsible for setting the backptr.
	pLVI_Allocated->pLiveViewDir = pLVD_Parent;

	// insert the item in the LVD's rbtree.
	// **BUT** the LVD DOES NOT take ownership.
	SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
												   pLVD_Parent->prb64LiveViewItems,
												   uiAliasGid,
												   pLVI_Allocated)  );

	// insert into cache and let it take ownership.
	SG_ERR_CHECK(  sg_wc_tx__cache__add_liveview_item(pCtx, pWcTx, pLVI_Allocated)  );
	pLVI = pLVI_Allocated;
	pLVI_Allocated = NULL;

	if (ppLVI_New)
		*ppLVI_New = pLVI;

fail:
	SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx, pLVI_Allocated);
}
