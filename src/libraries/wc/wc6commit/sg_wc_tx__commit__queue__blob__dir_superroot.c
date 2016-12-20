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
 * @file sg_wc_tx__commit__queue__blob__dir_superroot.c
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
 * I'm using the _cb notation to parallel the general form
 * in __queue__blob__dir_contents_participating(), but since
 * the superroot only has 1 entry, we don't really need to
 * actually do a foreach (or have an rbtree on which to do it).
 *
 */
static void _queue_dir2_cb(SG_context * pCtx,
						   sg_commit_data * pCommitData)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI = pCommitData->pWcTx->pLiveViewItem_Root;
	const SG_treenode_entry * pTNE_ref;		// we do not own this
	char * pszGid = NULL;
	SG_uint64 attrbits;

	SG_ASSERT(  (pLVI->uiAliasGid == pCommitData->pWcTx->uiAliasGid_Root)  );
	SG_ASSERT(  (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)  );

	// a little sanity check that _dive_ touched the root directory
	SG_ASSERT(  (pLVI->commitFlags & (SG_WC_COMMIT_FLAGS__PARTICIPATING
									  |SG_WC_COMMIT_FLAGS__BUBBLE_UP))  );

	SG_ASSERT(  (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__T__MASK)  );

	// trivial asserts to parallel _dir1 since the root directory is fixed.
	SG_ASSERT(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__DELETED) == 0)  );
	SG_ASSERT(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__U__FOUND  ) == 0)  );
	SG_ASSERT(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__U__IGNORED) == 0)  );
	SG_ASSERT(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__R__RESERVED) == 0)  );
	SG_ASSERT(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__A__SPARSE) == 0)  );

	// We DO include it in the TREENODE.

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringRepoPath, "@/")  );

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pCommitData->pWcTx->pDb,
													 pLVI->uiAliasGid,
													 &pszGid)  );

	// __get_current_attrbits() may lie
	// for the root directory.  (since they are technically
	// outside of the root directory.)  for our purposes
	// here, we don't know/care.
	SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits(pCtx,
															 pLVI,
															 pCommitData->pWcTx,
															 &attrbits)  );

	// a little sanity check that one of the __queue__blob__dir__
	// methods was able to figure out what to do with the directory.
	SG_ASSERT(  (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)  );
	SG_ASSERT(  (pLVI->pszHid_dir_content_commit
				 && *pLVI->pszHid_dir_content_commit)  );

	// We DO NOT have a set of CommitFlags on the super-root.
	// It doesn't have an LVI.  The _dir1 vs _dir3 differences
	// in how we process the contents of a directory do not
	// matter here since the super-root only has one child and
	// there are many status-flag bits that it ("@/") cannot
	// have.  So even in a partial commit, we can always treat
	// "@/" the same way.

	SG_ERR_CHECK(  sg_wc_tx__commit__queue__utils__add_tne(pCtx,
														   pCommitData->pWcTx->pTN_superroot_commit,
														   pszGid,
														   "@",
														   SG_TREENODEENTRY_TYPE_DIRECTORY,
														   pLVI->pszHid_dir_content_commit,
														   attrbits,
														   &pTNE_ref)  );

	if (pLVI->bDirHidModified_commit
		|| (pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__S__MASK
										|SG_WC_STATUS_FLAGS__C__MASK)))
	{
		// See note in _dir1.

		SG_ERR_CHECK(  sg_wc_tx__journal__commit__insert_tne(pCtx,
															 pCommitData->pWcTx,
															 pStringRepoPath,
															 pLVI->uiAliasGid,
															 SG_WC_DB__ALIAS_GID__NULL_ROOT,
															 pTNE_ref)  );
		SG_ERR_CHECK(  sg_wc_tx__journal__commit__delete_pc(pCtx,
															pCommitData->pWcTx,
															pStringRepoPath,
															pLVI->uiAliasGid)  );
	}

fail:
	SG_NULLFREE(pCtx, pszGid);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}
						   
/**
 * Build a CANDIDATE SUPER-ROOT TREENODE to contain the
 * current ROOT ("@/") directory.  Compute the new TN's
 * HID and if necessary queue a store-blob for this TN.
 *
 */
void sg_wc_tx__commit__queue__blob__dir_superroot(SG_context * pCtx,
												  sg_commit_data * pCommitData)
{
	SG_ERR_CHECK(  SG_treenode__alloc(pCtx,
									  &pCommitData->pWcTx->pTN_superroot_commit)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx,
											pCommitData->pWcTx->pTN_superroot_commit,
											SG_TN_VERSION__CURRENT)  );

	// insert a TNE for "@/" into the TN.
	SG_ERR_CHECK(  _queue_dir2_cb(pCtx, pCommitData)  );
	
	SG_ERR_CHECK(  SG_treenode__freeze(pCtx,
									   pCommitData->pWcTx->pTN_superroot_commit,
									   pCommitData->pWcTx->pDb->pRepo)  );
	SG_ERR_CHECK(  SG_treenode__get_id(pCtx,
									   pCommitData->pWcTx->pTN_superroot_commit,
									   &pCommitData->pWcTx->pszHid_superroot_content_commit)  );

#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "CommitQueueDir2: super-root hid '%s' ==> '%s'\n",
							   pCommitData->pWcTx->pCSetRow_Baseline->psz_hid_super_root,
							   pCommitData->pWcTx->pszHid_superroot_content_commit)  );
#endif

	if (strcmp(pCommitData->pWcTx->pCSetRow_Baseline->psz_hid_super_root,
			   pCommitData->pWcTx->pszHid_superroot_content_commit) != 0)
	{
		// we write a blob for the super-root if it is changing.
		// but we DO NOT put the super-root in the tne_L0 table.
		// (the tbl_csets now has a column for the super-root hid.)

		SG_ERR_CHECK(  sg_wc_tx__journal__store_blob(pCtx,
													 pCommitData->pWcTx,
													 "store_directory",
													 "*null*",	// a bogus pathname
													 NULL,
													 SG_WC_DB__ALIAS_GID__NULL_ROOT,
													 pCommitData->pWcTx->pszHid_superroot_content_commit,
													 0)  );
	}

fail:
	return;
}
