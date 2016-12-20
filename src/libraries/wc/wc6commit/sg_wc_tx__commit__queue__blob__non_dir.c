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
 * @file sg_wc_tx__commit__queue__blob__non_dir.c
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
 * queue a store-blob for the contents of a file.
 *
 * we write the predicted/precomputed/cahced HID
 * to the JOURNAL.  (this is the HID that we had
 * on hand when we decided that the file was clean/dirty.)
 * the APPLY phase will verify this HID when it
 * actually creates the blob in the repo.  if APPLY
 * gets a different value, we *want* it to throw.
 *
 */
void sg_wc_tx__commit__queue__blob__file(SG_context * pCtx,
										 sg_commit_data * pCommitData,
										 const SG_string * pStringRepoPath,
										 sg_wc_liveview_item * pLVI)
{
	char * pszHidContent = NULL;
	SG_uint64 sizeContent;
	SG_bool bNoTSC = SG_FALSE;		// we trust it

	SG_ASSERT(  (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__T__MASK)  );

	// we include __S__MERGE_CREATED and __S__UPDATE_CREATED so
	// that we are sure to get the HID if they edited the file
	// after the MERGE/UPDATE (because we might not report it
	// as modified because there isn't a baseline reference).
	// See W4722.
	//
	// The (... && !deleted) is only needed for __S__MERGE_CREATED
	// and __S__UPDATE_CREATED.

	if ((pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__S__ADDED
									 |SG_WC_STATUS_FLAGS__S__MERGE_CREATED
									 |SG_WC_STATUS_FLAGS__S__UPDATE_CREATED
									 |SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED))
		&& ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__DELETED) == 0))
	{
		// We let the LVI layer cache the content hid for us.
		SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx,
																	pLVI,
																	pCommitData->pWcTx,
																	bNoTSC,
																	&pszHidContent,
																	&sizeContent)  );
		SG_ERR_CHECK(  sg_wc_tx__journal__store_blob(pCtx,
													 pCommitData->pWcTx,
													 "store_file",
													 SG_string__sz(pStringRepoPath),
													 pLVI,
													 pLVI->uiAliasGid,
													 pszHidContent,
													 sizeContent)  );
	}

fail:
	SG_NULLFREE(pCtx, pszHidContent);
}

//////////////////////////////////////////////////////////////////

/**
 * queue a store-blob for the symlink content.
 *
 */
void sg_wc_tx__commit__queue__blob__symlink(SG_context * pCtx,
											sg_commit_data * pCommitData,
											const SG_string * pStringRepoPath,
											sg_wc_liveview_item * pLVI)
{
	char * pszHidContent = NULL;
	SG_uint64 sizeContent;
	SG_bool bNoTSC = SG_FALSE;		// we trust it

	SG_ASSERT(  (pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__T__MASK)  );

	// we include __S__MERGE_CREATED and __S__UPDATE_CREATED so
	// that we are sure to get the HID if they edited the file
	// after the MERGE/UPDATE (because we might not report it
	// as modified because there isn't a baseline reference).
	// See W4722.
	//
	// The (... && !deleted) is only needed for __S__MERGE_CREATED
	// and __S__UPDATE_CREATED.

	if ((pLVI->statusFlags_commit & (SG_WC_STATUS_FLAGS__S__ADDED
									 |SG_WC_STATUS_FLAGS__S__MERGE_CREATED
									 |SG_WC_STATUS_FLAGS__S__UPDATE_CREATED
									 |SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED))
		&& ((pLVI->statusFlags_commit & SG_WC_STATUS_FLAGS__S__DELETED) == 0))
	{
		// We let the LVI layer cache the content hid for us.
		SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx,
																	pLVI,
																	pCommitData->pWcTx,
																	bNoTSC,
																	&pszHidContent,
																	&sizeContent)  );
		SG_ERR_CHECK(  sg_wc_tx__journal__store_blob(pCtx,
													 pCommitData->pWcTx,
													 "store_symlink",
													 SG_string__sz(pStringRepoPath),
													 pLVI,
													 pLVI->uiAliasGid,
													 pszHidContent,
													 sizeContent)  );
	}

fail:
	SG_NULLFREE(pCtx, pszHidContent);
}


