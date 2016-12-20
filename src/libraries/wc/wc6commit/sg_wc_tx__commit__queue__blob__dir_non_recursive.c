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
 * @file sg_wc_tx__commit__queue__blob__dir_non_recursive.c
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
 * During a PARTIAL COMMIT we have a directory where we want to
 * do a non-recursive commit on the directory.  That is, we want
 * the changes to/on the directory, but NOT the contents *of*
 * the directory.
 *
 * For example, if they:
 *     vv add dir
 *     vv add dir/file
 *     vv commit -N dir
 * we should commit the ADD of the "dir" directory, but leave the
 * ADD of the file "file" still pending.  (Note that the actual ADD
 * of the item happens later in the parent directory "@/" as it is
 * building its TREENODE, but we have to build an EMPTY TREENODE
 * here for the contents of the "dir" directory.)
 *
 * There are other cases (when "dir" is changed but not ADDED),
 * where we want to write the existing/baseline content HID for
 * this directory.  Since this directory has __PARTICIPATING and
 * NOT __DIR_CONTENTS_PARTICIPATING and NOT __BUBBLE_UP, we assume
 * that the dive/mark phase has already gone thru the contents of
 * the directory and determined that nothing within matters (if it
 * did, a bubble-up would have been set).
 *
 * 
 * Construct a new empty TREENODE or copy the existing/baseline
 * TREENODE for this changeset.
 * 
 */
void sg_wc_tx__commit__queue__blob__dir_non_recursive(SG_context * pCtx,
													  sg_commit_data * pCommitData,
													  const SG_string * pStringRepoPath,
													  sg_wc_liveview_item * pLVI)
{
	sg_wc_liveview_dir * pLVD_dir;

	// by construction.
	SG_ASSERT(  ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__DIR_CONTENTS_PARTICIPATING) == SG_FALSE)  );
	SG_ASSERT(  ((pLVI->commitFlags & SG_WC_COMMIT_FLAGS__BUBBLE_UP) == SG_FALSE)  );
	SG_ASSERT(  (pLVI->commitFlags & SG_WC_COMMIT_FLAGS__PARTICIPATING)  );

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
		goto done;

	if (pLVI->statusFlags_commit == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_CHECK_RETURN(  sg_wc__status__compute_flags(pCtx, pCommitData->pWcTx, pLVI,
														   SG_FALSE, SG_FALSE,
														   &pLVI->statusFlags_commit)  );

	// no partial commits when in a merge
	SG_ASSERT_RELEASE_FAIL(  ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__MERGE_CREATED) == 0)  );
	
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pCommitData->pWcTx, pLVI, &pLVD_dir)  );

	if (pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__S__ADDED
									|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED))
	{
		// if the directory is new, then we have a trivial/empty TREENODE
		// because nothing within it is being considered.

		SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pLVD_dir->pTN_commit)  );
		SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pLVD_dir->pTN_commit, SG_TN_VERSION__CURRENT)  );
	
		SG_ERR_CHECK(  SG_treenode__freeze(pCtx,
										   pLVD_dir->pTN_commit,
										   pCommitData->pWcTx->pDb->pRepo)  );
		SG_ERR_CHECK(  SG_treenode__get_id(pCtx,
										   pLVD_dir->pTN_commit,
										   &pLVI->pszHid_dir_content_commit)  );

		pLVI->bDirHidModified_commit = SG_TRUE;
	}
	else
	{
		const char * pszHid;

		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx,
																	 pLVI,
																	 pCommitData->pWcTx,
																	 SG_FALSE,
																	 &pszHid)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHid, &pLVI->pszHid_dir_content_commit)  );

		pLVI->bDirHidModified_commit = SG_FALSE;
	}

#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "CommitQueueDir4: [bStore %d] '%s' '%s'\n",
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
		SG_TREENODE_NULLFREE(pCtx, pLVD_dir->pTN_commit);

done:
	;
fail:
	return;
}

