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
 * @file sg_wc_tx__commit__queue__blob__dir_contents_participating.c
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
};

static SG_rbtree_ui64_foreach_callback _queue_dir1_cb;

static void _queue_dir1_cb(SG_context * pCtx,
						   SG_uint64 uiAliasGid,
						   void * pVoid_LVI,
						   void * pVoid_Data)
{
	SG_string * pStringRepoPath = NULL;
	struct _queue_dir_data * pQueueDirData = (struct _queue_dir_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;
	char * pszHidContent_owned = NULL;
	const char * pszHidContent = NULL;
	char * pszGid = NULL;
	const SG_treenode_entry * pTNE_ref;		// we do not own this
	SG_uint64 attrbits;
	SG_bool bNoTSC = SG_FALSE;		// we trust the TSC.

	// Note that we only iterate over the items
	// still associated with the parent directory.
	// This includes present and deleted items.
	// It DOES NOT MOVED-AWAY include items.

	SG_UNUSED( uiAliasGid );

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
#if TRACE_WC_COMMIT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "_queue_dir_1: skipping reserved '%s'\n",
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif
		goto done;
	}

	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		// We DO NOT include it in the TREENODE.
		//
		// It DOES NOT have a row in either the
		// tne_L0 or the tbl_PC tables.
#if TRACE_WC_COMMIT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "_queue_dir_1: skipping uncontrolled '%s'\n",
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif
		goto done;
	}

	// __DIR_CONTENTS_PARTICIPATING on the parent directory implies
	// __PARTICIPATING on every item within.
	SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)  );

	SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
											SG_string__sz(pQueueDirData->pStringRepoPath_dir),
											SG_string__sz(pLVI->pStringEntryname),
											(pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
											&pStringRepoPath)  );

	SG_ASSERT(  (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__T__MASK)  );


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

		goto done;
	}

	//////////////////////////////////////////////////////////////////

	{
		// We DO include it in the TREENODE.
		
		SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
														 pQueueDirData->pCommitData->pWcTx->pDb,
														 pLVI->uiAliasGid,
														 &pszGid)  );

		// Item[k] in this directory is "participating".
		// That means we want the current state of the
		// item -- both __S__ and __C__ bits.
		//
		// __DIR_CONTENTS_PARTICIPATING also means that we don't
		// need to verify the validity of each entryname[k]
		// because the LVD started with the valid baseline and
		// we validated each add/delete/move/rename as it
		// occurred during one or more TXs, and since we are
		// using the entire contents of the LVD, we can *ASSUME*
		// a well-defined result.

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

		// Use *ALL* current fields that we have for item[k]
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
			// Here in _dir1_cb it doesn't matter so much because we are
			// using all of the fields, but later when we get to doing
			// partial commits (_dir3_cb,...), the TNE we construct may
			// diverge from the pLVI (for example, if we only take the
			// move/rename but not the content).

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
 * if necessary, queue a store-blob for the contents
 * of a directory that has __DIR_CONTENTS_PARTICIPATING
 * (which implies the dir has __PARTICIPATING and we
 * had (depth > 0) when we visited it).
 *
 * Note: __PARTICIPATING and __DIR_CONTENTS_PARTICIPATING
 * do not mean that the directory is dirty; rather it means that we
 * want to accept the pended changes (if any).  we still have to do
 * our homework.
 *
 * Build a CANDIDATE TREENODE with the CURRENT contents of
 * the directory and see if the resulting HID is different
 * from the baseline.  We *ASSUME* that the _dive_ pass has
 * already run on this directory so we don't need to intertwine
 * that with this.
 *
 */
void sg_wc_tx__commit__queue__blob__dir_contents_participating(SG_context * pCtx,
															   sg_commit_data * pCommitData,
															   const SG_string * pStringRepoPath,
															   sg_wc_liveview_item * pLVI)
{
	struct _queue_dir_data qd;
	const char * pszHidOriginal = NULL;		// we do not own this

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
#if TRACE_WC_COMMIT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "QueueBlob_DirContentsParticipating: skipping reserved '%s'\n",
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif
		return;
	}

	memset(&qd, 0, sizeof(qd));

	// by construction.
	SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__DIR_CONTENTS_PARTICIPATING)  );
	SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)  );
	// the directory may or may not have __BUBBLE_UP but that is
	// overshadowed by __DIR_CONTENTS_PARTICIPATING.
	
	qd.pCommitData = pCommitData;
	qd.pLVI_dir = pLVI;

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pCommitData->pWcTx, pLVI, &qd.pLVD_dir)  );
	qd.pStringRepoPath_dir = pStringRepoPath;

	// Build a CANDIDATE treenode with the CURRENT contents
	// of the directory.  Since we have __DIR_CONTENTS_PARTICIPATING,
	// this means that we get:
	// [] all of the structural changes to items within this directory.
	// [] attrbits changes on items within this directory.
	// [] for non-directories, all content changes.
	// [] for directories, we may or may not get content changes.

	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &qd.pLVD_dir->pTN_commit)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, qd.pLVD_dir->pTN_commit, SG_TN_VERSION__CURRENT)  );
	
	SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, qd.pLVD_dir, _queue_dir1_cb, &qd)  );

	SG_ERR_CHECK(  SG_treenode__freeze(pCtx,
									   qd.pLVD_dir->pTN_commit,
									   pCommitData->pWcTx->pDb->pRepo)  );
	SG_ERR_CHECK(  SG_treenode__get_id(pCtx,
									   qd.pLVD_dir->pTN_commit,
									   &pLVI->pszHid_dir_content_commit)  );

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
	else if (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__U__FOUND)
	{
		SG_ASSERT_RELEASE_FAIL( (0) );
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
							   "CommitQueueDir1: [bStore %d] '%s' '%s'\n",
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
	return;
}
