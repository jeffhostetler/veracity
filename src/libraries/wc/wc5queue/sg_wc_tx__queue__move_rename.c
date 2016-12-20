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
 * @file sg_wc_tx__queue__move_rename.c
 *
 * @details Manipulate the LVI/LVD caches to effect a MOVE and/or RENAME.
 * This is part of the QUEUE phase of the TX where we accumulate the
 * changes and look for problems.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * QUEUE a MOVE and/or RENAME operation for an AFTER-THE-FACT event.
 * (The user has already manipulated the working directory and is
 * just now telling us about it.)
 *
 * TODO 2011/09/24 This routine only really needs the 2 LVIs and can
 * TODO            dynamically compute the repo-paths from them.  I'm
 * TODO            passing them in here because the first caller just
 * TODO            happened to have them on hand.
 *
 */
void sg_wc_tx__queue__move_rename__after_the_fact(SG_context * pCtx,
												  SG_wc_tx * pWcTx,
												  const SG_string * pStringLiveRepoPath_Src,
												  const SG_string * pStringLiveRepoPath_Dest,
												  sg_wc_liveview_item * pLVI_Src,
												  sg_wc_liveview_item * pLVI_Dest,
												  SG_wc_status_flags xu_mask)
{
	sg_wc_liveview_dir * pLVD_DestDir;		// we do not own this
	SG_bool bAfterTheFact = SG_TRUE;
	SG_bool bCollidesWithSelf = SG_FALSE;

	// Since the pLVI_Dest representing the FOUND item is a placeholder,
	// replace it with the pLVI_Src in the destination directory
	// (because the pLVI_Src has the *OFFICIAL* alias-gid and
	// reference to the baseline's TNE).

	pLVD_DestDir = pLVI_Dest->pLiveViewDir;
	SG_ERR_CHECK(  SG_rbtree_ui64__remove__with_assoc(pCtx, pLVD_DestDir->prb64LiveViewItems,
													  pLVI_Dest->uiAliasGid, NULL)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__move_rename(pCtx,
																	 pLVI_Src,
																	 pLVD_DestDir,
																	 SG_string__sz(pLVI_Dest->pStringEntryname))  );

	if (pLVI_Src->pvhIssue && (pLVI_Src->statusFlags_x_xr_xu & xu_mask))
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

		SG_wc_status_flags xu_all = (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
		SG_wc_status_flags xu_exc = (xu_all & ~xu_mask);
		SG_wc_status_flags xu_inc = (xu_all &  xu_mask);
		SG_wc_status_flags xu2xr  = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags xr     = (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags sf     = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
									 | xr | xu2xr | xu_exc);
		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pWcTx, pLVI_Src, sf)  );
	}

	// Append the step(s) required to our journal.

	SG_ERR_CHECK(  sg_wc_tx__journal__move_rename(pCtx, pWcTx, pLVI_Src,
												  pStringLiveRepoPath_Src,
												  pStringLiveRepoPath_Dest,
												  bAfterTheFact,
												  bCollidesWithSelf)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Return flag if the old and new entrynames are synonyms
 * (for example "foo" and "FOO" on Windows/Mac).  This is
 * only appropriate for a simple rename (where both names
 * are defined in the same directory).
 *
 */
static void _check_old_and_new_entrynames_for_synonyms(SG_context * pCtx,
													   SG_wc_tx * pWcTx,
													   const char * pszOldEntryname,
													   const char * pszNewEntryname,
													   SG_bool * pbCollidesWithSelf)
{
	SG_wc_port * pPort = NULL;
	SG_wc_port_flags portFlags_destination;	
	const SG_string * pStringItemLog = NULL;		// we don't own this
	SG_bool bIsDuplicate = SG_FALSE;

	SG_ERR_CHECK(  sg_wc_db__create_port(pCtx, pWcTx->pDb, &pPort)  );

	SG_ERR_CHECK(  SG_wc_port__add_item(pCtx, pPort, NULL, pszNewEntryname, SG_TREENODEENTRY_TYPE__INVALID, &bIsDuplicate)  );
	SG_ERR_CHECK(  SG_wc_port__add_item(pCtx, pPort, NULL, pszOldEntryname, SG_TREENODEENTRY_TYPE__INVALID, &bIsDuplicate)  );
	SG_ASSERT_RELEASE_FAIL(  (bIsDuplicate == SG_FALSE)  );		// we already checked for this

	SG_ERR_CHECK(  SG_wc_port__get_item_result_flags(pCtx, pPort, pszNewEntryname,
													 &portFlags_destination, &pStringItemLog)  );
	*pbCollidesWithSelf = ((portFlags_destination & SG_WC_PORT_FLAGS__MASK_COLLISION) != 0);

fail:
	SG_WC_PORT_NULLFREE(pCtx, pPort);
}

//////////////////////////////////////////////////////////////////

/**
 * QUEUE a MOVE and/or RENAME in the normal (before-the-fact) case.
 * 
 * TODO 2011/09/24 This routine only really needs the 2 LVI/LVD and can
 * TODO            dynamically compute the repo-paths from them.  I'm
 * TODO            passing them in here because the first caller just
 * TODO            happened to have them on hand.
 *
 */
void sg_wc_tx__queue__move_rename__normal(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  const SG_string * pStringLiveRepoPath_Src,
										  const SG_string * pStringLiveRepoPath_Dest,
										  sg_wc_liveview_item * pLVI,
										  sg_wc_liveview_dir * pLVD_NewParent,
										  const char * pszNewEntryname,
										  SG_wc_status_flags xu_mask)
{
	SG_bool bAfterTheFact = SG_FALSE;
	SG_bool bCollidesWithSelf = SG_FALSE;

	// We DO NOT check pWcTx->bReadOnly during the QUEUE phase.
	// We only check that during the APPLY phase.  This could
	// be used to do a 'vv rename --test ...' if we wanted to.

	// If we have a RENAME-only (no MOVE), see if the current
	// and new entrynames are synonyms (e.g. 'vv rename foo FOO'
	// on Windows).  If they are we need to schedule a 2-step
	// rename with a temp file.
	//
	// We do this check with the CURRENT and FUTURE values, not
	// the BASELINE value.

	if (pLVI->pLiveViewDir == pLVD_NewParent)
		SG_ERR_CHECK(  _check_old_and_new_entrynames_for_synonyms(pCtx, pWcTx,
																  SG_string__sz(pLVI->pStringEntryname),
																  pszNewEntryname,
																  &bCollidesWithSelf)  );

	// Deal with updating the LVI and the various LVDs.
	// This DOES NOT update the database, just our in-memory
	// view of it.

	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__move_rename(pCtx,
																	 pLVI,
																	 pLVD_NewParent,
																	 pszNewEntryname)  );

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

	SG_ERR_CHECK(  sg_wc_tx__journal__move_rename(pCtx, pWcTx, pLVI,
												  pStringLiveRepoPath_Src,
												  pStringLiveRepoPath_Dest,
												  bAfterTheFact,
												  bCollidesWithSelf)  );

fail:
	return;
}
