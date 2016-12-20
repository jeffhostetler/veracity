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
 * @file sg_wc_prescan_dir__scan_and_match.c
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

struct _scan_data
{
	SG_wc_tx *				pWcTx;
	sg_wc_prescan_dir *		pPrescanDir;
	SG_rbtree *				prb_readdir;
};

//////////////////////////////////////////////////////////////////

/**
 * We get called once for each item under version control
 * (committed or pended) in the directory.  This includes
 * items that ARE OR WERE in the directory (both MOVE-INs
 * and MOVE-OUTs and DELETEs).
 *
 * Match them up with items observed by READDIR in the
 * directory on disk.
 *
 * NOTE: By using the entrynames in the readdir list as
 * we got them from the filesystem (and matching them with
 * the entrynames as we registered them into version control)
 * we avoid the various case-ambiguities that exists()
 * would introduce.  This is a good thing.
 *
 */
static sg_wc_pctne__foreach_cb _scan_and_match__bind_pairs__cb;

static void _scan_and_match__bind_pairs__cb(SG_context * pCtx,
											void * pVoidData,
											sg_wc_pctne__row ** ppPT)
{
	struct _scan_data * psd = (struct _scan_data *)pVoidData;
	sg_wc_pctne__row * pPT = NULL;	// we may or may not own this
	sg_wc_prescan_row * pPrescanRow_Allocated = NULL;
	const char * pszEntryname;	// we do not own this
	sg_wc_readdir__row * pRD;
	SG_uint64 uiAliasGid;
	SG_uint64 uiAliasGidParent_Current;
	SG_treenode_entry_type tneType;
	SG_bool bIsDeletePended;
	SG_bool bFoundInDir;

	pPT = *ppPT;

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_prescan_dir__scan_and_match__bind_pairs: received pctne:\n")  );
	SG_ERR_IGNORE(  sg_wc_pctne__row__debug_print(pCtx, pPT)  );
#endif

	SG_ERR_CHECK(  sg_wc_pctne__row__get_alias(pCtx, pPT, &uiAliasGid)  );
	SG_ERR_CHECK(  sg_wc_pctne__row__get_current_entryname(pCtx, pPT, &pszEntryname)  );
	SG_ERR_CHECK(  sg_wc_pctne__row__get_tne_type(pCtx, pPT, &tneType)  );
	SG_ERR_CHECK(  sg_wc_pctne__row__get_current_parent_alias(pCtx, pPT, &uiAliasGidParent_Current)  );
	SG_ERR_CHECK(  sg_wc_pctne__row__is_delete_pended(pCtx, pPT, &bIsDeletePended)  );

	SG_ASSERT(  ((tneType != SG_TREENODEENTRY_TYPE__DEVICE) && (tneType != SG_TREENODEENTRY_TYPE__INVALID))  );

	if (uiAliasGidParent_Current != psd->pPrescanDir->uiAliasGidDir)
	{
		// This item has a pended MOVE-OUT of the directory that
		// we are scanning.  *AND* the MOVE was APPLIED by a
		// previous vv command.  So the item doesn't own an entryname
		// in the current directory on disk.  So we DO NOT create
		// a SCANROW for it.
		//
		// The TNE part of the PCTNE belongs in this directory, but
		// the PC part of the PCTNE has been moved to another directory.
		//
		// Accumulate a SET of the moved-out aliases.  This SET is
		// basically constant once we have build the SCANDIR.  We'll
		// use this SET to seed the LVD (which will dynamically track
		// the moved-out-set during the TX).  The SET of these
		// moved-out "phantoms" will help when we have to do an
		// "extended" status.

		if (!psd->pPrescanDir->prb64MovedOutItems)
			SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &psd->pPrescanDir->prb64MovedOutItems)  );
		SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
													   psd->pPrescanDir->prb64MovedOutItems,
													   uiAliasGid,
													   NULL)  );

		return;
	}

	// This item belongs to this directory somehow.
	// So we DO create a SCANROW.

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPrescanRow_Allocated)  );
	pPrescanRow_Allocated->uiAliasGid = uiAliasGid;
	pPrescanRow_Allocated->pPrescanDir_Ref = psd->pPrescanDir;
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx,
										&pPrescanRow_Allocated->pStringEntryname,
										pszEntryname)  );

	// steal the caller's pPT and the data within it.
	pPrescanRow_Allocated->pTneRow    = pPT->pTneRow;   pPT->pTneRow = NULL;
	pPrescanRow_Allocated->pPcRow_Ref = pPT->pPcRow;    pPT->pPcRow  = NULL;
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
	*ppPT = NULL;

	pPrescanRow_Allocated->tneType = tneType;

	if (bIsDeletePended)
	{
		// This item currently has a pended DELETE (that has already
		// been APPLIED by a previous vv command), so it no longer owns
		// the entryname (if it is present) on the disk.
		pPrescanRow_Allocated->pRD = NULL;
		pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__CONTROLLED_INACTIVE_DELETED;
	}
	else if (pPrescanRow_Allocated->pPcRow_Ref
			 && (pPrescanRow_Allocated->pPcRow_Ref->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE))
	{
		// We marked the item sparse (not-populated) and until
		// told otherwise, we believe that.  (If they create a
		// file/directory with the same name, it isn't a match.)
		//
		// TODO 2011/10/19 I can see maybe auto-populating a directory,
		// TODO            but not a file/symlink.  For now, no.
		pPrescanRow_Allocated->pRD = NULL;
		pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE;
	}
	else
	{
		// We believe it is (or should be) present in the directory.
		//
		// See if we observed a corresponding entry on disk.
		// WE NEED TO RESPECT ***BOTH*** THE ENTRYNAME AND THE TNE-TYPE.
		// That is, if we think there should be a ("foo",file) under
		// version control and READDIR told us about ("foo",directory),
		// we don't consider it a match (because items CANNOT change
		// types) rather we need to treat it as a
		//       ("foo",file) ==> LOST
		// and later a
		//       ("foo",directory) ==> FOUND.

		if (psd->prb_readdir)
		{
			SG_ERR_CHECK(  SG_rbtree__find(pCtx, psd->prb_readdir, pszEntryname,
										   &bFoundInDir, (void **)&pRD)  );
		}
		else	// if the directory is lost/missing, mark everything within likewise.
		{
			bFoundInDir = SG_FALSE;
			pRD = NULL;
		}

		if (!bFoundInDir)
		{
			// We have a controlled item that we didn't
			// find in the directory on disk.  We call
			// this a LOST item.
			pPrescanRow_Allocated->pRD = NULL;
			pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_LOST;
		}
		else
		{
			if (pRD->tneType == tneType)
			{
				// Bind this (readdir,pctne) pair.
				// Steal the readdir item.
				SG_ERR_CHECK(  SG_rbtree__remove(pCtx, psd->prb_readdir, pszEntryname)  );
				pRD->pPrescanRow = pPrescanRow_Allocated;	// back ptr
				pPrescanRow_Allocated->pRD = pRD;
				pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED;
			}
			else
			{
				// The observed name belongs to a different
				// type of object.  Therefore it isn't a match
				// for this a match for this item.
				pPrescanRow_Allocated->pRD = NULL;
				pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_LOST;
			}
		}
	}

	// add the row to the scandir.
	// the scandir DOES NOT take ownership.
	SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
												   psd->pPrescanDir->prb64PrescanRows,
												   uiAliasGid,
												   pPrescanRow_Allocated)  );

	SG_ERR_CHECK(  sg_wc_tx__cache__add_prescan_row(pCtx, psd->pWcTx, pPrescanRow_Allocated)  );
	pPrescanRow_Allocated = NULL;

	return;

fail:
	SG_WC_PRESCAN_ROW__NULLFREE(pCtx, pPrescanRow_Allocated);
}

/**
 * Add a scan_row for each item we observed in the
 * actual directory and that were not matched up
 * with a version controlled item.
 *
 */
static SG_rbtree_foreach_callback _scan_and_match__bind_leftovers__cb;

static void _scan_and_match__bind_leftovers__cb(SG_context * pCtx,
												const char * pszEntryname,
												void * pVoidRD,
												void * pVoidData)
{
	struct _scan_data * psd = (struct _scan_data *)pVoidData;
	sg_wc_readdir__row * pRD = (sg_wc_readdir__row *)pVoidRD;
	sg_wc_prescan_row * pPrescanRow_Allocated = NULL;
	SG_bool bIsReserved = SG_FALSE;

	SG_ERR_CHECK(  sg_wc_db__path__is_reserved_entryname(pCtx, psd->pWcTx->pDb, pszEntryname, &bIsReserved)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPrescanRow_Allocated)  );

	// Generate a TEMPORARY GID for it.  I hate to do
	// this for uncontrolled items since it led to a
	// few problems in the original PendingTree code,
	// but it solves more than it creates.

	SG_ERR_CHECK(  sg_wc_db__gid__insert_new(pCtx, psd->pWcTx->pDb, SG_TRUE,
											 &pPrescanRow_Allocated->uiAliasGid)  );
#if TRACE_WC_TX_SCAN
	{
		SG_int_to_string_buffer bufui64;

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "ScanBindLeftovers: %s/%s item '%s' assigned tmp GID '%s'.\n",
								   ((bIsReserved) ? "Reserved" : "Uncontrolled"),
								   ((pRD->tneType == SG_TREENODEENTRY_TYPE__DEVICE) ? "Device" : "Normal"),
								   pszEntryname,
								   SG_uint64_to_sz(pPrescanRow_Allocated->uiAliasGid, bufui64))  );
	}
#endif

	pPrescanRow_Allocated->pPrescanDir_Ref = psd->pPrescanDir;
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx,
										&pPrescanRow_Allocated->pStringEntryname,
										pszEntryname)  );

	// we have no PCTNE data for this item because it is a FOUND item.
	pPrescanRow_Allocated->pTneRow = NULL;
	pPrescanRow_Allocated->pPcRow_Ref = NULL;

	pPrescanRow_Allocated->tneType = pRD->tneType;

	// "Steal" the readdir row data for this item.
	//
	// WARNING we cannot acutally remove the item from the readdir
	// list NOR CAN WE UPDATE THE ASSOCIATED DATA VALUE because we are
	// inside a foreach on it.  So leave the item in the list knowning
	// that our caller is about to throw the list away anyway.
	//
	// SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, psd->prb_readdir,
	//												pszEntryname, NULL, NULL)  );
	//
	// [*RD*] By setting pRD->pPrescanRow we are declaring that this pRD
	// is owned by the pPrescanRow and NOT the psd->prb_readdir rbtree.

	pRD->pPrescanRow = pPrescanRow_Allocated;	// back ptr
	pPrescanRow_Allocated->pRD = pRD;

	if (bIsReserved)
	{
		// Mark it as RESERVED.  This is a special kind of uncontrolled,
		// but different from FOUND/IGNORED because it CANNOT EVER become
		// controlled.  (You *really* don't want to be able to place .sgdrawer
		// under version control, for example.)
		pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__RESERVED;
	}
	else
	{
		// mark it as generic uncontrolled at this level.  we let STATUS
		// qualify it as FOUND or IGNORED dynamically and upon demand.
		//
		// Items of type DEVICE (FIFO, BlkDev, etc) will get this too.
		pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED;
	}

	// scandir borrows pointer to scanrow.
	SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
												   psd->pPrescanDir->prb64PrescanRows,
												   pPrescanRow_Allocated->uiAliasGid,
												   pPrescanRow_Allocated)  );

	// row-cache takes ownership of scanrow.
	SG_ERR_CHECK(  sg_wc_tx__cache__add_prescan_row(pCtx, psd->pWcTx, pPrescanRow_Allocated)  );

	pPrescanRow_Allocated = NULL;
	return;

fail:
	SG_WC_PRESCAN_ROW__NULLFREE(pCtx, pPrescanRow_Allocated);
}

//////////////////////////////////////////////////////////////////

static void _free_readdir_row_if_not_stolen(SG_context * pCtx, sg_wc_readdir__row * pRow)
{
	// [*RD*] If pRD->pPrescanRow is set we are declaring that this pRD
	// is owned by the pPrescanRow and NOT the psd->prb_readdir rbtree.
	//
	// This SG_free_callback is only used in the rbtree_free_with_assoc
	// call at the bottom.

	if (!pRow)
		return;

	if (pRow->pPrescanRow)
		return;

	SG_WC_READDIR__ROW__NULLFREE(pCtx, pRow);
}

//////////////////////////////////////////////////////////////////

/**
 * Build a combined status for the given directory
 * using both the actual directory on disk and the
 * info we have in the PC and tne_L0 tables.
 * This is only interested in the structural state
 * (ADDS, DELETES, ....).
 *
 * This is the state of the directory PRIOR TO THE
 * START OF THE TRANSACTION.  (Because during the QUEUE
 * phase (and before the APPLY phave) of a transaction,
 * we don't modify the working director or the PC table.
 *
 * We add the allocated scandir (and all of their scanrows)
 * to the caches in the TX, so you DO NOT own the returned
 * pointer.
 *
 * The passed in 'scan_flags' represents the observed
 * flags *on* the directory that were seen when the
 * parent directory was scanned/read.  This is primarily
 * needed so that we can avoid trying to call readdir
 * on a directory that we already know is LOST/not present
 * (and without having to do a stat() on it first).
 * 
 */
void sg_wc_prescan_dir__alloc__scan_and_match(SG_context * pCtx,
											  SG_wc_tx * pWcTx,
											  SG_uint64 uiAliasGidDir,
											  const SG_string * pStringRefRepoPath,
											  sg_wc_prescan_flags scan_flags,
											  sg_wc_prescan_dir ** ppPrescanDir)
{
	struct _scan_data sd;
	SG_pathname * pPathDirRef = NULL;
	SG_bool bPresent = SG_FALSE;

	memset(&sd,0,sizeof(sd));
	sd.pWcTx = pWcTx;

	SG_ERR_CHECK(  SG_WC_PRESCAN_DIR__ALLOC(pCtx, &sd.pPrescanDir, uiAliasGidDir, pStringRefRepoPath)  );

	SG_ERR_CHECK(  sg_wc_db__path__repopath_to_absolute(pCtx, pWcTx->pDb, pStringRefRepoPath,
														&pPathDirRef)  );

	// When the parent directory was scanned/read, we computed
	// a set of scan_flags for it -- so we already know if it
	// is lost and/or not present on disk -- so we don't need
	// to stat() it before trying to call readdir.
	bPresent = (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(scan_flags)
				|| SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_ROOT(scan_flags)
				|| SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_MATCHED(scan_flags)  );
	if (bPresent)
	{
		// Get a complete list of everything in the directory
		// as it currently is on disk.  This list may have
		// both controlled and uncontrolled items.
		//
		// With W1046 ("Have Reserved Status"), we DO NOT use __SKIP_SG.

		SG_ERR_CHECK(  sg_wc_readdir__alloc__from_scan(pCtx, pPathDirRef,
													   (SG_DIR__FOREACH__STAT
														| SG_DIR__FOREACH__SKIP_OS
														   ),
													   &sd.prb_readdir)  );
	}
	else
	{
		sd.prb_readdir = NULL;
	}

	// Iterate over everything under version control (both
	// committed and pended) in this directory and match
	// them up with the observed entrynames.

	SG_ERR_CHECK(  sg_wc_pctne__foreach_in_dir_by_parent_alias(pCtx,
															   pWcTx->pDb, pWcTx->pCSetRow_Baseline,
															   uiAliasGidDir,
															   _scan_and_match__bind_pairs__cb, &sd)  );

	if (bPresent)
	{
		// Anything not stolen from the readdir list is a FOUND/IGNORED item.
		// Add them to the scan result as an uncontrolled item.

		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, sd.prb_readdir,
										  _scan_and_match__bind_leftovers__cb, &sd)  );
	
		// We've trashed the readdir list.  Discard it before someone
		// tries to use it for something else.
		// 
		// [*RD*] During the above foreach, we may have "stolen" the pRD associated
		// data pointers from the items in the rbtree (but we did not remove/alter
		// the list because of the iteration).  Use this special version of __row__free()
		// that knows about this ownership custody battle.

		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, sd.prb_readdir, (SG_free_callback *)_free_readdir_row_if_not_stolen);
	}

	SG_PATHNAME_NULLFREE(pCtx, pPathDirRef);

	// add the new scandir to the cache and let it take ownership of it.

	SG_ERR_CHECK(  sg_wc_tx__cache__add_prescan_dir(pCtx, pWcTx, sd.pPrescanDir)  );

#if TRACE_WC_TX_SCAN
	SG_ERR_IGNORE(  sg_wc_prescan_dir_debug__dump_to_console(pCtx, sd.pPrescanDir)  );
#endif

	*ppPrescanDir = sd.pPrescanDir;
	return;

fail:
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, sd.prb_readdir, (SG_free_callback *)sg_wc_readdir__row__free);
	SG_WC_PRESCAN_DIR__NULLFREE(pCtx, sd.pPrescanDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathDirRef);
}

//////////////////////////////////////////////////////////////////
