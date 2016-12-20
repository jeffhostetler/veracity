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
 * @file sg_wc_tx__commit__queue__blob__bubble_up_directory.c
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

struct _queue_dir_data
{
	sg_commit_data *		pCommitData;
	sg_wc_liveview_item * 	pLVI_dir;
	sg_wc_liveview_dir *	pLVD_dir;
	const SG_string *		pStringRepoPath_dir;
	SG_wc_port *			pPort;
};

//////////////////////////////////////////////////////////////////

/**
 * This child is participating in the commit and it is inside
 * a directory that is not part of the commit (other than to
 * have an updated content-hid).
 *
 */
static void _queue_dir3__participating_child(SG_context * pCtx,
											 struct _queue_dir_data * pQueueDirData,
											 sg_wc_liveview_item * pLVI)
{
	SG_string * pStringRepoPath = NULL;
	char * pszHidContent_owned = NULL;
	const char * pszHidContent = NULL;
	char * pszGid = NULL;
	const SG_treenode_entry * pTNE_ref;		// we do not own this
	SG_uint64 attrbits;
	SG_bool bNoTSC = SG_FALSE;		// we trust the TSC.

	SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
											SG_string__sz(pQueueDirData->pStringRepoPath_dir),
											SG_string__sz(pLVI->pStringEntryname),
											(pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
											&pStringRepoPath)  );

	SG_ASSERT(  (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__T__MASK)  );

	if (pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__U__FOUND
									|SG_WC_STATUS_FLAGS__U__IGNORED))
	{
		// We DO NOT include it in the TREENODE.
		//
		// It DOES NOT have a row in either the
		// tne_L0 or the tbl_PC tables.

		// The item will not exist in the TREENODE, so we
		// do not need to add it to the portability-collider.

		goto done;
	}

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__DELETED)
	{
		// We DO NOT include it in the TREENODE.
		// 
		// Queue a request to remove it from tne_L0 (so it
		// will not be in our view of the computed future
		// baseline).
		//
		// Queue a request to remove it from the tbl_PC table
		// (because it will not be dirty in the future WD).

		SG_ERR_CHECK(  sg_wc_tx__journal__commit__delete_tne(pCtx,
															 pQueueDirData->pCommitData->pWcTx,
															 pStringRepoPath,
															 pLVI->uiAliasGid)  );
		SG_ERR_CHECK(  sg_wc_tx__journal__commit__delete_pc(pCtx,
															pQueueDirData->pCommitData->pWcTx,
															pStringRepoPath,
															pLVI->uiAliasGid)  );

		// The item will not exist in the TREENODE, so we
		// do not need to add it to the portability-collider.

		goto done;
	}

	//////////////////////////////////////////////////////////////////

	{
		// Since __DIR_CONTENTS_PARTICIPATING was NOT set on the directory
		// only *some* of the changes within it will committed, so we need
		// to verify that this entryname will not collide with the names
		// of non-participating peers, so add it to the portability-collider.
		//
		// Note: this participating item might also be MOVED-INTO this
		// directory. [MOVED] 

		SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__check_port(pCtx,
																  pQueueDirData->pPort,
																  SG_string__sz(pLVI->pStringEntryname),
																  pLVI->tneType,
																  pStringRepoPath)  );
																  
		// We DO include it in the TREENODE (regardless
		// of whether or not it is dirty).

		SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
														 pQueueDirData->pCommitData->pWcTx->pDb,
														 pLVI->uiAliasGid,
														 &pszGid)  );

		// Item[k] in this directory is "participating".
		// That means we want the current state of the
		// item -- both __S__ and __C__ bits.

		SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits(pCtx, pLVI,
																 pQueueDirData->pCommitData->pWcTx,
																 &attrbits)  );
		if (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			// Item[k] is a sub-directory.  We DO NOT recurse here; we
			// assume that was already handled by the previous "dive" pass.
			// And during the dive, we computed candidate TNs and TN HIDs
			// for deeper sub-dirs such that we can reference them here.
			// 
			// We cannot call __get_current_content_hid() for directories
			// (because that code just wants to stat() the file system
			// (and the content-HID for a directory is a hash on the TN
			// that we construct)).

			SG_ASSERT(  (pLVI->pszHid_dir_content_commit
						 && *pLVI->pszHid_dir_content_commit)  );

			// It is IMPORTANT that we respect the value from the _dive_
			// phase and not try to replicate it here -- because of the
			// various partial commit and depth options, we may not actually
			// be committing anything from within this sub-directory and
			// the computed HID here will correctly reflect that.

			pszHidContent = pLVI->pszHid_dir_content_commit;
		}
		else
		{
			// We let the LVI layer cache the content non-dir HID for us.
			SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx, pLVI,
																		pQueueDirData->pCommitData->pWcTx,
																		bNoTSC,
																		&pszHidContent_owned, NULL)  );
			pszHidContent = pszHidContent_owned;
		}

		// Use *ALL* *CURRENT* fields that we have for item[k]
		// and create a TREENODEENTRY for it and add it to
		// the caller's directory TREENODE.

		SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__add_tne(pCtx,
															   pQueueDirData->pLVD_dir->pTN_commit,
															   pszGid,
															   SG_string__sz(pLVI->pStringEntryname),
															   pLVI->tneType,
															   pszHidContent,
															   attrbits,
															   &pTNE_ref)  );

		if (pLVI->bDirHidModified_commit
			|| (pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__S__MASK
											|SG_WC_STATUS_FLAGS__C__MASK)))
		{
			// The item is dirty and we are updating everything
			// about this item.
			//
			// Queue a request to insert/update the row in tne_L0.
			//
			// Queue a request to remove it from the tbl_PC table.
			//
			// We use pTNE_ref to do the tne_L0 insert/update because
			// we want it to match what we are putting in the TN.

			SG_ERR_CHECK(  sg_wc_tx__journal__commit__insert_tne(pCtx,
																 pQueueDirData->pCommitData->pWcTx,
																 pStringRepoPath,
																 pLVI->uiAliasGid,
																 pQueueDirData->pLVI_dir->uiAliasGid,
																 pTNE_ref)  );

			if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__A__SPARSE)
			{
				// Committing a dirty sparse item (probably a move/rename)
				// should record the dirt in the new changeset but still
				// leave it sparse in the WD.
				SG_ERR_CHECK(  sg_wc_tx__journal__commit__clean_pc_but_leave_sparse(pCtx,
																					pQueueDirData->pCommitData->pWcTx,
																					pStringRepoPath,
																					pLVI->uiAliasGid)  );
			}
			else
			{
				SG_ERR_CHECK(  sg_wc_tx__journal__commit__delete_pc(pCtx,
																	pQueueDirData->pCommitData->pWcTx,
																	pStringRepoPath,
																	pLVI->uiAliasGid)  );
			}
		}
	}

done:
	;
fail:
	SG_NULLFREE(pCtx, pszGid);
	SG_NULLFREE(pCtx, pszHidContent_owned);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

/**
 * This child IS NOT PARTICIPATING in the commit.  Work backwards
 * as described in [2] to get the state of the item as it was in
 * the baseline (if present).
 *
 * If this item has bubble-up set on it, substitute the freshly
 * computed content hid in place of the baseline value.
 *
 * If this item should be present in the committed TREENODE for
 * the parent directory, create a TREENODEENTRY for it and insert
 * it into the TREENODE.
 *
 * Because the contents of this TREENODE are being synthesized
 * from the details of the partial commit, we need to use the
 * portability-collider to make sure that the resulting TREENODE
 * is well-defined.
 *
 */
static void _queue_dir3__other_child(SG_context * pCtx,
									 struct _queue_dir_data * pQueueDirData,
									 sg_wc_liveview_item * pLVI)
{
	SG_string * pStringRepoPath = NULL;
	const char * pszHidContent = NULL;
	const char * pszEntryname = NULL;
	char * pszGid = NULL;
	const SG_treenode_entry * pTNE_ref;		// we do not own this
	SG_uint64 attrbits;
	SG_bool bNoTSC = SG_FALSE;		// we trust the TSC.

	// by construction
	SG_ASSERT(  ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING) == 0)  );

	SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
											SG_string__sz(pQueueDirData->pStringRepoPath_dir),
											SG_string__sz(pLVI->pStringEntryname),
											(pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
											&pStringRepoPath)  );

	if (pLVI->statusFlags_commit == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pQueueDirData->pCommitData->pWcTx, pLVI,
													SG_FALSE, SG_FALSE,
													&pLVI->statusFlags_commit)  );

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__R__RESERVED)
	{
		// We DO NOT include it in the TREENODE.
		// It should not have been possible for a
		// Reserved name to have been ADDED.
		goto done;
	}

	if (pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__U__FOUND
									|SG_WC_STATUS_FLAGS__U__IGNORED))
	{
		// We DO NOT include it in the TREENODE.
		//
		// It DOES NOT have a row in either the
		// tne_L0 or the tbl_PC tables.

		// The item will not exist in the TREENODE, so we
		// do not need to add it to the portability-collider.

		goto done;
	}

	// don't care about __U__LOST.

	if (pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__S__ADDED
									|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED))
	{
		// The item has a pending ADD -- it was not present in the baseline.

		if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)
		{
			// The item/child is a bubble-up sub-directory that
			// is not participating, but it has content that is.
			//
			// TODO 2011/12/07 I don't think this case is possible any more.
			// TODO            This case gets handled during the dive when
			// TODO            we first look at this sub-directory and its
			// TODO            contents.

			SG_ERR_THROW2(  SG_ERR_ASSERT,	// TODO 2011/12/06 make a better error code
							(pCtx, "Cannot do partial commit within an ADDED directory: '%s'",
							 SG_string__sz(pStringRepoPath))  );
		}

		// Since the item was not present in the baseline, we
		// don't want it in the TREENODE.
		//
		// Leave the item in the tne_L0 and tbl_PC table so that
		// it will still appear as ADDED after this commit.

		goto done;
	}

	// Since we don't allow partial commits after a merge.
	SG_ASSERT_RELEASE_FAIL(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MERGE_CREATED) == 0)  );

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MOVED)
	{
		// Reference Note [MOVED].
		// 
		// The item has a pending MOVE-INTO this destination directory.
		// So it was not present in this directory in the baseline.
		//
		// We also can say that neither this item nor the destination
		// directory is participating, so the destination directory
		// should not add it to the TREENODE; the source directory
		// still owns it for the duration of this commit.

		// true because destination directory not participating.
		SG_ASSERT(  ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__MOVED_INTO_MARKED_DIRECTORY) == 0)  );

		// if the source directory *is* participating (and thinks that
		// it no longer owns the item), we have a mismatch.
		if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__MOVED_OUT_OF_MARKED_DIRECTORY)
			SG_ERR_THROW2(  SG_ERR_WC_PARTIAL_COMMIT,
							(pCtx, ("The item '%s' was moved and only the source directory is being committed."
									"  Extend the selection to include this item or its parent directory."),
							 SG_string__sz(pStringRepoPath))  );
		
		// We can assume for this commit that it will still be
		// owned by the source directory (which may or may not
		// have a bubble-up bit set).
		//
		// Since the item was not present in this directory in
		// the baseline, we don't want it in this TREENODE.
		// 
		// Leave the item in the tne_L0 and tbl_PC table so that
		// it will still appear as MOVED after this commit.

		goto done;
	}

	//////////////////////////////////////////////////////////////////

	// If the item has __S__DELETED or __S__RENAMED (or is
	// structurally unchanged) we want the original entryname,
	// but we need to verify that it won't collide with other
	// things in our synthesized/hybrid TREENODE.

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntryname)  );

	SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__check_port(pCtx,
															  pQueueDirData->pPort,
															  pszEntryname,
															  pLVI->tneType,
															  pStringRepoPath)  );

	// We DO include it in the TREENODE (regardless
	// of whether or not it is dirty).

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pQueueDirData->pCommitData->pWcTx->pDb,
													 pLVI->uiAliasGid,
													 &pszGid)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI,
															  pQueueDirData->pCommitData->pWcTx,
															  &attrbits)  );
	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)
	{
		SG_ASSERT(  (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)  );
		SG_ASSERT(  (pLVI->pszHid_dir_content_commit
					 && *pLVI->pszHid_dir_content_commit)  );

		// It is IMPORTANT that we respect the value from the _dive_
		// phase and not try to replicate it here -- because of the
		// various partial commit and depth options, we may not actually
		// be committing anything from within this sub-directory and
		// the computed HID here will correctly reflect that.

		pszHidContent = pLVI->pszHid_dir_content_commit;
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI,
																	 pQueueDirData->pCommitData->pWcTx,
																	 bNoTSC,
																	 &pszHidContent)  );
	}

	SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__add_tne(pCtx,
														   pQueueDirData->pLVD_dir->pTN_commit,
														   pszGid,
														   pszEntryname,
														   pLVI->tneType,
														   pszHidContent,
														   attrbits,
														   &pTNE_ref)  );

	// since we ignore all changes except for the bubble-up content-hid,
	// we can leave the pended changes in the tbl_PC after the commit.
	// The only thing we need to do is rewrite the tne_L0 record if the
	// bubble-up content-hid changed.

	if (pLVI->bDirHidModified_commit)
	{
		SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)  );

		// Queue a request to insert/update the row in tne_L0.

		// We use pTNE_ref to do the tne_L0 insert/update because
		// we want it to match what we are putting in the TN.

		SG_ERR_CHECK(  sg_wc_tx__journal__commit__insert_tne(pCtx,
															 pQueueDirData->pCommitData->pWcTx,
															 pStringRepoPath,
															 pLVI->uiAliasGid,
															 pQueueDirData->pLVI_dir->uiAliasGid,
															 pTNE_ref)  );
	}

done:
	;
fail:
	SG_NULLFREE(pCtx, pszGid);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * We get called once for each item currently associated
 * with the directory.
 *
 * This does not include moved-away items.
 *
 */
static SG_rbtree_ui64_foreach_callback _queue_dir3_cb;

static void _queue_dir3_cb(SG_context * pCtx,
						   SG_uint64 uiAliasGid,
						   void * pVoid_LVI,
						   void * pVoid_Data)
{
	struct _queue_dir_data * pQueueDirData = (struct _queue_dir_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;

	SG_UNUSED( uiAliasGid );

	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
	{
		// This child is particpating.  We want the item as it
		// *currently* exists -- baseline + any/all changes.

		SG_ERR_CHECK(  _queue_dir3__participating_child(pCtx, pQueueDirData, pLVI)  );
	}
	else
	{
		// This child is not participating in any way in the
		// commit or is a bubble-up sub-directory.  Keep the
		// item as it was in the baseline.  For bubble-up
		// sub-directories, take the newly computed content HID.

		SG_ERR_CHECK(  _queue_dir3__other_child(pCtx, pQueueDirData, pLVI)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * We get called once for each item that was MOVED-OUT of
 * the directory.
 *
 * Because we are iterating on a bubble-up directory (and
 * DO NOT want the non-participating changes), we want to
 * do the backwards mapping as described in [2].  That is,
 * we want the item listed in our TREENODE in this directory
 * JUST LIKE IT WAS IN THE BASELINE (give or take the content
 * HID of a bubble-up sub-directory).  We DO NOT want the
 * destination directory to claim it *UNLESS* the moved
 * item is participating.
 *
 * See Reference Note [MOVED].
 *
 */
static SG_rbtree_ui64_foreach_callback _queue_dir3_moved_out_cb;

static void _queue_dir3_moved_out_cb(SG_context * pCtx,
									 SG_uint64 uiAliasGid,
									 void * pVoid_Assoc,
									 void * pVoid_Data)
{
	struct _queue_dir_data * pQueueDirData = (struct _queue_dir_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_string * pStringRepoPath = NULL;
	const char * pszHidContent = NULL;
	const char * pszEntryname = NULL;
	char * pszGid = NULL;
	const SG_treenode_entry * pTNE_ref;		// we do not own this
	SG_uint64 attrbits;
	SG_bool bNoTSC = SG_FALSE;		// we trust the TSC.
	SG_bool bKnown;

	SG_UNUSED( pVoid_Assoc );

	SG_ERR_CHECK_RETURN(  sg_wc_tx__liveview__fetch_random_item(pCtx,
																pQueueDirData->pCommitData->pWcTx,
																uiAliasGid,
																&bKnown, &pLVI)  );
	SG_ASSERT( (bKnown) );

	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)
	{
		// The destination directory owns it for the commit.
		SG_ASSERT(  ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__MOVED_INTO_MARKED_DIRECTORY) != 0)  );

		// So we don't want it in our TREENODE.
		// 
		// And the destination directory can take care of
		// the tne_L0 and tbl_PC tables.

		goto done;
	}

	SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
											SG_string__sz(pQueueDirData->pStringRepoPath_dir),
											SG_string__sz(pLVI->pStringEntryname),
											(pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
											&pStringRepoPath)  );

	// The source directory (us) still own it for the
	// duration of the commit.
	//
	// Add it to our TREENODE with the baseline values.

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntryname)  );
	SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__check_port(pCtx,
															  pQueueDirData->pPort,
															  pszEntryname,
															  pLVI->tneType,
															  pStringRepoPath)  );

	// We DO include it in the TREENODE (regardless
	// of whether or not it is dirty).

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pQueueDirData->pCommitData->pWcTx->pDb,
													 pLVI->uiAliasGid,
													 &pszGid)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI,
															  pQueueDirData->pCommitData->pWcTx,
															  &attrbits)  );
	if (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)
	{
		SG_ASSERT(  (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)  );
		SG_ASSERT(  (pLVI->pszHid_dir_content_commit
					 && *pLVI->pszHid_dir_content_commit)  );

		// It is IMPORTANT that we respect the value from the _dive_
		// phase and not try to replicate it here -- because of the
		// various partial commit and depth options, we may not actually
		// be committing anything from within this sub-directory and
		// the computed HID here will correctly reflect that.

		pszHidContent = pLVI->pszHid_dir_content_commit;
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI,
																	 pQueueDirData->pCommitData->pWcTx,
																	 bNoTSC,
																	 &pszHidContent)  );
	}

	SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__add_tne(pCtx,
														   pQueueDirData->pLVD_dir->pTN_commit,
														   pszGid,
														   pszEntryname,
														   pLVI->tneType,
														   pszHidContent,
														   attrbits,
														   &pTNE_ref)  );

	// since we ignore all changes except for the bubble-up content-hid,
	// we can leave the pended changes in the tbl_PC after the commit.
	// The only thing we need to do is rewrite the tne_L0 record if the
	// bubble-up content-hid changed.

	if (pLVI->bDirHidModified_commit)
	{
		SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)  );

		// Queue a request to insert/update the row in tne_L0.

		// We use pTNE_ref to do the tne_L0 insert/update because
		// we want it to match what we are putting in the TN.

		SG_ERR_CHECK(  sg_wc_tx__journal__commit__insert_tne(pCtx,
															 pQueueDirData->pCommitData->pWcTx,
															 pStringRepoPath,
															 pLVI->uiAliasGid,
															 pQueueDirData->pLVI_dir->uiAliasGid,
															 pTNE_ref)  );
	}

done:
	;
fail:
	SG_NULLFREE(pCtx, pszGid);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * During a PARTIAL COMMIT we have a directory that IS NOT
 * part of the subset of the tree begin committed, but for
 * various reasons (explained below) we need to write a
 * new TREENODE for it.
 *
 * [] if the directory is a parent of something that changed
 *    and the child is being committed, then we need to update
 *    the TNE {content-hid,attrbits} of the child in
 *    this directory's TN.
 *
 * [] if the directory is a parent of something that has a
 *    structural change (add/delete/move/rename) we need
 *    to update the TNE in the TN.
 *
 * [] moved items are especially important because we need
 *    to make sure that both sides of the move get into the
 *    changeset.
 *
 * Note that we DO NOT want ALL of the changes in this
 * directory.  Only the ones that they want to be in the
 * changeset.  This means that we need to start with the
 * baseline TN and only include the requested changes.
 * 
 * *** THIS MEANS THAT THIS NEW TREENODE THAT WE STORE 
 * *** IN THE REPO IS A NEW HYBRID THAT MAY NOT HAVE 
 * *** EVER EXISTED ON DISK IN THE WD.
 *
 * *** It also means that we need to throw early and
 * *** often if we can't create this new hybrid directory.
 * *** For example, if they did a 'vv rename foo bar' in
 * *** this directory before doing a 'vv move ../xyz/foo .'
 * *** Our directory is consistent *IFF* everything is
 * *** committed, but if only 'xyz' is committed, then
 * *** we will try to create a TN with 2 foo's.
 *
 * It also means that we may only commit a subset of the
 * set of changes on a particular item.  For example, if
 * they have EDITED foo.c and MOVED it into this directory
 * (and the source directory is participating and this
 * directory is only bubble-up), then we will write a
 * TN for this (destination) directory that includes the
 * item (because the file needs to exist somewhere in the
 * tree), but we don't commit the new content (because
 * this file is not participating and/or is not in a
 * participating directory).
 *
 */
void sg_wc_tx__commit__queue__blob__bubble_up_directory(SG_context * pCtx,
														sg_commit_data * pCommitData,
														const SG_string * pStringRepoPath,
														sg_wc_liveview_item * pLVI)
{
	struct _queue_dir_data qd;
	const char * pszHidOriginal = NULL;		// we do not own this

	memset(&qd, 0, sizeof(qd));

	// by construction.
	SG_ASSERT(  ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__DIR_CONTENTS_PARTICIPATING) == SG_FALSE)  );
	SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP)  );
	// the directory may or may not have __PARTICIPATING

	if (pLVI->statusFlags_commit == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_CHECK_RETURN(  sg_wc__status__compute_flags(pCtx, pCommitData->pWcTx, pLVI,
														   SG_FALSE, SG_FALSE,
														   &pLVI->statusFlags_commit)  );

	if ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__ADDED)
		|| (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
		|| (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MERGE_CREATED))
	{
		if ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING) == SG_FALSE)
		{
			// We don't allow them to commit an added/created non-participating
			// bubble-up directory because that would implicitly
			// do the ADD in addition to just getting the updated
			// contents (for which there isn't a baseline reference).
			// If they want to commit something that is *within*
			// this directory, then they should include this directory
			// in the set.

			SG_ERR_THROW2(  SG_ERR_WC_PARTIAL_COMMIT,
							(pCtx, ("A committed item would be inside the added but uncommitted directory '%s'."
									"  Extend the selction to include this directory."),
							 SG_string__sz(pStringRepoPath))  );
		}
	}

	qd.pCommitData = pCommitData;
	qd.pLVI_dir = pLVI;

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pCommitData->pWcTx, pLVI, &qd.pLVD_dir)  );
	qd.pStringRepoPath_dir = pStringRepoPath;

	// Build a CANDIDATE treenode with the BASELINE contents
	// of the directory *PLUS* selected changes.
	//
	// We don't have a pre-computed set matching what we
	// want to put into the TN.
	//
	// There are 2 ways to do this:
	// 
	// [1] forward: load the baseline TN from disk and then walk
	//     the LVD and apply only the changes that we want.
	//
	// [2] backward: create an empty TN and then walk the LVD and
	//     add the changes we want and add the baseline version
	//     for changes we don't want.
	//
	// I think [2] is conceptually harder, but can be done in
	// 1 pass and with stuff we already have in rbtrees in memory.
	// Whereas, [1] requires 2 passes with an O(N^2) lookup.
	// Let's try [2].
	//
	// Also, we since we potentially only taking SOME of the
	// adds/deletes/moves/renames we need to create a
	// portability-collider to ensure that the set of
	// entrynames that we create in the TN is well-defined
	// (no collisions).

	SG_ERR_CHECK(  sg_wc_db__create_port(pCtx, pCommitData->pWcTx->pDb, &qd.pPort)  );

	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &qd.pLVD_dir->pTN_commit)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, qd.pLVD_dir->pTN_commit, SG_TN_VERSION__CURRENT)  );
	
	SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, qd.pLVD_dir, _queue_dir3_cb, &qd)  );

	if (qd.pLVD_dir->prb64MovedOutItems)
		SG_ERR_CHECK(  sg_wc_liveview_dir__foreach_moved_out(pCtx, qd.pLVD_dir, _queue_dir3_moved_out_cb, &qd)  );

	SG_ERR_CHECK(  SG_treenode__freeze(pCtx,
									   qd.pLVD_dir->pTN_commit,
									   pCommitData->pWcTx->pDb->pRepo)  );
	SG_ERR_CHECK(  SG_treenode__get_id(pCtx,
									   qd.pLVD_dir->pTN_commit,
									   &pLVI->pszHid_dir_content_commit)  );

	// Just because we set the bubble-up bit (based upon the
	// set of argv) doesn't mean that this directory actually
	// contained anything dirty.

	if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		pLVI->bDirHidModified_commit = SG_TRUE;
	}
	else if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
	{
		pLVI->bDirHidModified_commit = SG_TRUE;
	}
	else if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
	{
		pLVI->bDirHidModified_commit = SG_TRUE;
	}
	else // the baseline value is well-defined
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx,
																	 pLVI,
																	 pCommitData->pWcTx,
																	 SG_FALSE,
																	 &pszHidOriginal)  );
		pLVI->bDirHidModified_commit = (strcmp(pLVI->pszHid_dir_content_commit,pszHidOriginal) != 0);
	}

#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "CommitQueueDir3: [bStore %d] '%s' '%s'\n",
							   pLVI->bDirHidModified_commit,
							   pLVI->pszHid_dir_content_commit,
							   SG_string__sz(pStringRepoPath))  );
#endif

	if (pLVI->bDirHidModified_commit)
		SG_ERR_CHECK(  sg_wc_tx__journal__store_blob(pCtx,
													 pCommitData->pWcTx,
													 "store_directory",
													 SG_string__sz(pStringRepoPath),
													 pLVI,
													 pLVI->uiAliasGid,
													 pLVI->pszHid_dir_content_commit, 0)  );
	else
		SG_TREENODE_NULLFREE(pCtx, qd.pLVD_dir->pTN_commit);

fail:
	SG_WC_PORT_NULLFREE(pCtx, qd.pPort);
}
