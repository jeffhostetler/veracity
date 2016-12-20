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
 * @file sg_wc_tx__merge__queue_plan__delete.c
 *
 * @details We are used after the MERGE result has been
 * computed.  We are one of the last steps in the WD-PLAN
 * construction.  We are responsible for making the plan
 * steps for things that were deleted between the baseline
 * and the final-result.  Stuff that is in the WD on disk
 * (because it was in the baseline) and that needs to be
 * deleted (because it is not in the final-result).
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

struct _sg_mrg__plan__delete__data
{
	SG_mrg *		pMrg;

	// map[live-repo-path ==> pMrgCSetEntry_Baseline]
	//                        a reverse-sorted (deepest-first) list
	//                        of directories that need to be deleted.
	//                        we do not own the assoc data.

	SG_rbtree *		prbDirectoriesToDelete;
};

typedef struct _sg_mrg__plan__delete__data _sg_mrg__plan__delete__data;

//////////////////////////////////////////////////////////////////

static void _my_remove__in_revert(SG_context * pCtx,
								  SG_mrg * pMrg,
								  const char * pszRepoPath,
								  SG_mrg_cset_entry * pMrgCSetEntry_Baseline)
{
	// TODO 2012/08/07 I just added a non-recursive arg to SG_wc_tx__remove().
	// TODO            In theory, we should not need to request a recursive
	// TODO            of this item, since MERGE should have already taken
	// TODO            care of all of the details.  But I don't have time to
	// TODO            test this right now.  So I'm setting the flag to match
	// TODO            the behavior of the version of the routine prior to the
	// TODO            new arg.
	SG_bool bNonRecursive = SG_FALSE;

	// If the merge engine computes a "delete" is needed in a REVERT-ALL,
	// it means that the item was ADDED in the WD and not present in the
	// actual baseline cset.  (Remember that in a REVERT-ALL *both* the "A"
	// and "B" nodes were set to WD.  And that here the actual virgin
	// baseline is in "C".)
	//
	// For historical reasons, the variable pMrgCSetEntry_Baseline refers
	// to the "B" node.  I need to rename them.
	//
	// If the item was added by the user, then we should keep the file and
	// undo the add so that it will appear as FOUND.  The SG_wc_tx__remove()
	// code automatically handles undo-add.
	//
	// If it was added-by-update or added-by-merge (and for files hasn't
	// been edited) then we can actually remove it (both from SQL and the WD).
	// If the user has edited it, we can remove it from SQL and move it to
	// a backup.

	if (pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		// So we want to keep the item and just make it uncontrolled.  It should
		// appear as FOUND after REVERT is finished.
		//
		// It also means that we don't need to bother requesting a backup.
		SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pMrg->pWcTx, pszRepoPath,
										bNonRecursive,
										SG_FALSE,		// force
										pMrg->bRevertNoBackups,
										SG_TRUE)  );	// keep
		pMrg->nrDeleted++;
	}
	else if (pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
	{
		// We have an ADDED-BY-MERGE (aka SPECIAL-ALL) item.  This was
		// created during the previous MERGE because the item was present
		// in the other/non-baseline leaf and not present in the baseline.
		//
		// We should delete the item (unless it has been modified by the
		// user since the merge).

		SG_bool bBackup = ((pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__T__FILE)
						   && (pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
						   && (!pMrg->bRevertNoBackups));
		// cancel the predicted kill-pc-row in the post-process and let
		// __remove2() do it now or do whatever it needs to do for sparse items.
		SG_ERR_CHECK(  sg_wc_tx__merge__remove_from_kill_list(pCtx, pMrg, pMrgCSetEntry_Baseline->uiAliasGid)  );
		SG_ERR_CHECK(  SG_wc_tx__remove2(pCtx, pMrg->pWcTx, pszRepoPath,
										 bNonRecursive,
										 SG_TRUE,       // force
										 (!bBackup),    // no-backups
										 SG_FALSE,      // keep
										 SG_TRUE)  );   // bFlatten
		pMrg->nrDeleted++;
	}
	else if (pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
	{
		// We have an ADDED-BY-UPDATE (aka SPECIAL-ADD) item.  This was
		// added during a previous UPDATE because the item was dirty at
		// the time of the UPDATE and the item was deleted-from/not-present-in
		// the target CSet.  So UPDATE kept the item in the WD (and because of
		// the required change-of-basis) marked it as ADDED-BY-UPDATE.
		//
		// The current REVERT-ALL command wants to make things look like they
		// did in the *CURRENT* baseline (whether it is the previous update
		// target CSet or that of the last commit).  The point is that it is
		// ***NOT*** the pre-update baseline that caused all of this.
		//
		// So we really want to REMOVE it and have it be gone; we DO NOT want
		// the undo-add stuff to convert it to a FOUND item.  We will offer to
		// back it up if they have edited the file (since the original pre-update
		// cset), but we do not care about moves/renames/etc.
		//
		// Normally, these calls to SG_wc_tx__remove() would cause it to be marked
		// __S__DELETED + __S__UPDATE_CREATED (since we still have LVI and SQL
		// bookkeeping to deal with in this TX).  We set bFlatten to remove
		// the row for this item from the pc_L0 table so that subsequent STATUS
		// commands won't see it.

		SG_bool bBackup = ((pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__T__FILE)
						   && (pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
						   && (!pMrg->bRevertNoBackups));
		SG_ERR_CHECK(  sg_wc_tx__merge__remove_from_kill_list(pCtx, pMrg, pMrgCSetEntry_Baseline->uiAliasGid)  );
		SG_ERR_CHECK(  SG_wc_tx__remove2(pCtx, pMrg->pWcTx, pszRepoPath,
										 bNonRecursive,
										 SG_TRUE,       // force
										 (!bBackup),    // no-backups
										 SG_FALSE,      // keep
										 SG_TRUE)  );   // bFlatten
		pMrg->nrDeleted++;
	}
	else if (pMrgCSetEntry_Baseline->statusFlags__using_wc & (SG_WC_STATUS_FLAGS__U__FOUND
															  |SG_WC_STATUS_FLAGS__U__IGNORED))
	{
		// We don't own it, so don't delete it.
		// We DO NOT increment pMrg->nrDeleted.

#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Revert: skipping remove of found/ignored item '%s'.\n",
								   pszRepoPath)  );
#endif

		// TODO 2012/07/26 Should we have --force or --clean option or something
		// TODO            to request that we get rid of this stuff ?
	}
	else if (pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__R__RESERVED)
	{
		// We never remove reserved items because it should not
		// be possible to have added them.
		// We DO NOT increment pMrg->nrDeleted.

#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Revert: skipping remove of reserved item '%s'.\n",
								   pszRepoPath)  );
#endif
	}
	else
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "my_remove__in_revert: 0x%s %s",
						 SG_uint64_to_sz__hex(pMrgCSetEntry_Baseline->statusFlags__using_wc, bufui64),
						 pszRepoPath)  );
	}

fail:
	return;
}

static void _my_remove(SG_context * pCtx,
					   SG_mrg * pMrg,
					   const char * pszRepoPath,
					   SG_mrg_cset_entry * pMrgCSetEntry_Baseline)
{
	// TODO 2012/08/07 I just added a non-recursive arg to SG_wc_tx__remove().
	// TODO            In theory, we should not need to request a recursive
	// TODO            of this item, since MERGE should have already taken
	// TODO            care of all of the details.  But I don't have time to
	// TODO            test this right now.  So I'm setting the flag to match
	// TODO            the behavior of the version of the routine prior to the
	// TODO            new arg.
	SG_bool bNonRecursive = SG_FALSE;

	switch (pMrg->eFakeMergeMode)
	{
	case SG_FAKE_MERGE_MODE__MERGE:
	case SG_FAKE_MERGE_MODE__UPDATE:
		SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pMrg->pWcTx, pszRepoPath,
										bNonRecursive,
										SG_TRUE,		// force
										SG_TRUE,		// no-backups
										SG_FALSE)  );	// keep
		pMrg->nrDeleted++;
		break;

	case SG_FAKE_MERGE_MODE__REVERT:
		SG_ERR_CHECK(  _my_remove__in_revert(pCtx, pMrg, pszRepoPath, pMrgCSetEntry_Baseline)  );
		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _sg_mrg__plan__deleted__cb;

static void _sg_mrg__plan__deleted__cb(SG_context * pCtx,
									   const char * pszKey_GidEntry,
									   void * pVoidValue_MrgCSetEntry,
									   void * pVoidData)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Baseline = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	_sg_mrg__plan__delete__data * pData = (_sg_mrg__plan__delete__data *)pVoidData;
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bFound;

	SG_UNUSED( pszKey_GidEntry );

	if (pMrgCSetEntry_Baseline->markerValue & _SG_MRG__PLAN__MARKER__FLAGS__APPLIED)
	{
		// this entry in the baseline was already processed when its peer
		// in the final-result was processed.
		return;
	}

	// if the entry did not exist in the final result, it
	// could not have been parked.
	SG_ASSERT(  ((pMrgCSetEntry_Baseline->markerValue & _SG_MRG__PLAN__MARKER__FLAGS__PARKED) == 0)  );

	// Otherwise, this entry was present in BASELINE but should
	// not be in the Final-Result.
	//
	// Use the GID and map it into the LVI code (which still
	// knows where the item is on disk and in the TX);  we need
	// to do it this way because we are talking about a baseline
	// entry that has not yet been deleted and that doesn't have
	// corresponding entries in the FinalResult.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx,
														 pData->pMrg->pWcTx,
														 pMrgCSetEntry_Baseline->uiAliasGid,
														 &bFound,
														 &pLVI)  );
	if (pLVI->pvhIssue)
		SG_ERR_CHECK(  SG_mrg_cset_entry__check_for_outstanding_issue(pCtx, pData->pMrg, pLVI,
																	  pMrgCSetEntry_Baseline)  );

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx,
															  pData->pMrg->pWcTx,
															  pLVI,
															  &pStringRepoPath)  );

	if (pMrgCSetEntry_Baseline->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		// defer directory deletes until the next pass (after we
		// (have taken care of everything within the directory).
		//
		// build reverse-sorted queue based upon the live repo-path.
		// this will let us rmdir children before parents (and
		// hopefully not require a -rf).

		if (!pData->prbDirectoriesToDelete)
			SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS2(pCtx, &pData->prbDirectoriesToDelete,
													 128, NULL,
													 SG_rbtree__compare_function__reverse_strcmp)  );

		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->prbDirectoriesToDelete,
												  SG_string__sz(pStringRepoPath),
												  pMrgCSetEntry_Baseline)  );
	}
	else
	{
		SG_ERR_CHECK(  _my_remove(pCtx, pData->pMrg, SG_string__sz(pStringRepoPath), pMrgCSetEntry_Baseline)  );

		pMrgCSetEntry_Baseline->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _sg_mrg__plan__deleted_dirs__cb;

static void _sg_mrg__plan__deleted_dirs__cb(SG_context * pCtx,
											const char * pszKey_RepoPath,
											void * pVoidValue_MrgCSetEntry,
											void * pVoidData)
{
	SG_mrg_cset_entry * pMrgCSetEntry_Baseline = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	_sg_mrg__plan__delete__data * pData = (_sg_mrg__plan__delete__data *)pVoidData;

	SG_ERR_CHECK(  _my_remove(pCtx, pData->pMrg, pszKey_RepoPath, pMrgCSetEntry_Baseline)  );

	pMrgCSetEntry_Baseline->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__queue_plan__delete(SG_context * pCtx,
										 SG_mrg * pMrg)
{
	_sg_mrg__plan__delete__data data;

	memset(&data,0,sizeof(data));
	data.pMrg = pMrg;
	// defer allocation of prbDirectoriesToDelete until we have one

	// inspect each entry in the baseline and look for ones that
	// haven't been applied yet.  since we've already handled all
	// of the peer entries, these must not be present in the final-result
	// and need to be removed from the WD.  add these deletes to the plan.

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pMrg->pMrgCSet_Baseline->prbEntries, _sg_mrg__plan__deleted__cb, &data)  );
	if (data.prbDirectoriesToDelete)
		SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, data.prbDirectoriesToDelete, _sg_mrg__plan__deleted_dirs__cb, &data)  );

fail:
	SG_RBTREE_NULLFREE(pCtx, data.prbDirectoriesToDelete);
}
