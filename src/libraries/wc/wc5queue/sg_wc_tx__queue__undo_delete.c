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

//////////////////////////////////////////////////////////////////

/**
 * QUEUE an UN-DELETE for the item into (parent,entryname).
 * 
 * TODO 2012/03/12 This routine only really needs the 2 LVI/LVD and can
 * TODO            dynamically compute the repo-paths from them.  I'm
 * TODO            passing them in here because the first caller just
 * TODO            happened to have them on hand.
 *
 * bMakeSparse should only be set when the destination directory
 * is sparse and we should implicitly make the restored item sparse
 * too.
 *
 */
void sg_wc_tx__queue__undo_delete(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_string * pStringLiveRepoPath_Dest,	// where we will restore it to
								  sg_wc_liveview_item * pLVI,
								  sg_wc_liveview_item * pLVI_DestDir,
								  const char * pszNewEntryname,
								  const SG_wc_undo_delete_args * pArgs,
								  SG_wc_status_flags xu_mask)
{
	SG_int64 attrbits;
	const char * pszHidBlob = NULL;
	const SG_string * pStringRepoPathTempFile = NULL;
	sg_wc_liveview_dir * pLVD_DestDir = NULL;

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI_DestDir->scan_flags_Live))
	{
		// The destination PARENT DIRECTORY is sparse.  Therefore
		// we cannot actually restore the item into the WD (without
		// implicitly populating the parent).
		// 
		// We can do the bookkeeping to cancel the delete,
		// but we need to implicitly mark it sparse too.

		if (pArgs && pArgs->pStringRepoPathTempFile)
		{
			// However if they give us undo-delete-args (meaning
			// that they want to override the default non-structural
			// baseline values), then we can't make it sparse if they
			// want to point it at a temp file or something.
			SG_ERR_THROW2(  SG_ERR_WC_IS_SPARSE,
							(pCtx, "Cannot undelete modified file '%s' within a sparse portion of the tree.",
							 SG_string__sz(pStringLiveRepoPath_Dest))  );
		}
	}

	// Deal with updating the LVI and the various LVDs.
	// This DOES NOT update the database, just our in-memory
	// view of it.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_DestDir, &pLVD_DestDir)  );
	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__undo_delete(pCtx,
																	 pLVI,
																	 pLVD_DestDir,
																	 pszNewEntryname)  );

	if (pArgs)
	{
		attrbits = pArgs->attrbits;
		pszHidBlob = pArgs->pszHidBlob;
		pStringRepoPathTempFile = pArgs->pStringRepoPathTempFile;
	}
	else if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live))
	{
		attrbits = pLVI->pPcRow_PC->p_d_sparse->attrbits;
		pszHidBlob = pLVI->pPcRow_PC->p_d_sparse->pszHid;
#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "UndoDelete of formerly sparse item -- assuming p_d_sparse values: %s\n",
								   pszNewEntryname)  );
#endif
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pWcTx, (SG_uint64 *)&attrbits)  );
		if (pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx, SG_FALSE, &pszHidBlob)  );
#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "UndoDelete of non-sparse item -- assuming original values: %s\n",
								   pszNewEntryname)  );
#endif

	}

	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & xu_mask))
	{
		// set the corresponding _XR_ bit for each choice set in
		// the _XU_ bits that we have on the item AND THAT are
		// included in the xu_mask; leave the other choices unresolved.
		// if we resolved the last remaining choice, set the overall
		// state.
		//
		// TODO 2012/08/27 We only set the x_xr_xu flags on the issue;
		// TODO            we DO NOT update the saved-resolution info.
		// TODO            (So the saved-resolution info could be stale
		// TODO            if they did something like accepted a rename,
		// TODO            then deleted it and restored it into the
		// TODO            baseline, and then reverted-it causing this
		// TODO            restore.  Not sure if we want to change
		// TODO            something like that into a 'accept working'
		// TODO            or 'accept baseline'.  Not sure if it matters
		// TODO            or could actually happen in practice.)

		SG_wc_status_flags xu_all = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
		SG_wc_status_flags xu_exc = (xu_all & ~xu_mask);
		SG_wc_status_flags xu_inc = (xu_all &  xu_mask);
		SG_wc_status_flags xu2xr  = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags xr     = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags sf     = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
									 | xr | xu2xr | xu_exc);
		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pWcTx, pLVI, sf)  );
	}

	// Append the step(s) required to our journal.

	SG_ERR_CHECK(  sg_wc_tx__journal__undo_delete(pCtx, pWcTx,
												  pLVI,
												  pStringLiveRepoPath_Dest,
												  pszHidBlob,
												  attrbits,
												  pStringRepoPathTempFile)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * QUEUE an restore for a lost item.
 * 
 * TODO 2012/03/12 This routine only really needs the 2 LVI/LVD and can
 * TODO            dynamically compute the repo-paths from them.  I'm
 * TODO            passing them in here because the first caller just
 * TODO            happened to have them on hand.
 *
 */
void sg_wc_tx__queue__undo_lost(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const SG_string * pStringLiveRepoPath_Dest,	// where we will restore it to
								sg_wc_liveview_item * pLVI,
								sg_wc_liveview_dir * pLVD_NewParent,
								const char * pszNewEntryname,
								const SG_wc_undo_delete_args * pArgs,
								SG_wc_status_flags xu_mask)
{
	SG_int64 attrbits;
	const char * pszHidBlob = NULL;
	const SG_string * pStringRepoPathTempFile = NULL;

	// Deal with updating the LVI and the various LVDs.
	// This DOES NOT update the database, just our in-memory
	// view of it.

	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__undo_lost(pCtx,
																   pLVI,
																   pLVD_NewParent,
																   pszNewEntryname)  );

	if (pArgs)
	{
		attrbits = pArgs->attrbits;
		pszHidBlob = pArgs->pszHidBlob;
		pStringRepoPathTempFile = pArgs->pStringRepoPathTempFile;
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pWcTx, (SG_uint64 *)&attrbits)  );
		if (pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx, SG_FALSE, &pszHidBlob)  );
	}


	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & xu_mask))
	{
		// set the corresponding _XR_ bit for each choice set in
		// the _XU_ bits that we have on the item AND THAT are
		// included in the xu_mask; leave the other choices unresolved.
		// if we resolved the last remaining choice, set the overall
		// state.
		//
		// TODO 2012/08/27 We only set the x_xr_xu flags on the issue;
		// TODO            we DO NOT update the saved-resolution info.
		// TODO            (So the saved-resolution info could be stale
		// TODO            if they did something like accepted a rename,
		// TODO            then deleted it and restored it into the
		// TODO            baseline, and then reverted-it causing this
		// TODO            restore.  Not sure if we want to change
		// TODO            something like that into a 'accept working'
		// TODO            or 'accept baseline'.  Not sure if it matters
		// TODO            or could actually happen in practice.)

		SG_wc_status_flags xu_all = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
		SG_wc_status_flags xu_exc = (xu_all & ~xu_mask);
		SG_wc_status_flags xu_inc = (xu_all &  xu_mask);
		SG_wc_status_flags xu2xr  = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags xr     = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags sf     = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
									 | xr | xu2xr | xu_exc);
		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pWcTx, pLVI, sf)  );
	}

	// Append the step(s) required to our journal.

	SG_ERR_CHECK(  sg_wc_tx__journal__undo_lost(pCtx, pWcTx,
												pLVI,
												pStringLiveRepoPath_Dest,
												pszHidBlob,
												attrbits,
												pStringRepoPathTempFile)  );

fail:
	return;
}

