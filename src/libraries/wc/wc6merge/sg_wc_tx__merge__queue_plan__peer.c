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
 * @file sg_wc_tx__merge__queue_plan__peer.h
 *
 * @details Handle adding items to the WD-PLAN for entries that
 * are present in both the baseline and final-result.
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _merge__queue_move_rename(SG_context * pCtx,
									  SG_mrg * pMrg,
									  SG_mrg_cset_entry * pMrgCSetEntry_FinalResult)
{
	SG_string * pString1 = NULL;
	SG_string * pString2 = NULL;

	// Create GID-domain repo-paths for the item
	// and for the destination directory.  This
	// should let us by-pass all of the transient
	// repo-path issues that plagued the PendingTree
	// version of MERGE.

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString1, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Entry)  );
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString2, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Parent)  );

	// Use layer-7 api to do the dirty work of
	// queuing up the move/rename.  This is not
	// immediate, rather it is queued/journaled.
	// Using the layer-7 TX API lets us speak
	// in extended-prefix repo-paths (using GIDs).
	// And deal with sparse handling/restrictions.

	SG_wc_tx__move_rename2(pCtx,
						   pMrg->pWcTx,
						   SG_string__sz(pString1),
						   SG_string__sz(pString2),
						   SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname),
						   SG_FALSE);
	if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
		goto done;
	
	if (SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS)
		|| SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS))
	{
		// Because the __compute_simple2() code already dealt
		// with FINAL-RESULT collisions (and unique-ified them),
		// the only collisions we should expect are TRANSIENT
		// collisions.  (The item *currently* holding the desired
		// entryname in the desired directory is going to be moved,
		// renamed, or deleted, but we haven't gotten to it yet
		// (in the iteration on the final result entries or in
		// the later delete pass.)
		//
		// Park this item and go on.

		SG_context__err_reset(pCtx);
		SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__park(pCtx, pMrg,
														 pMrgCSetEntry_FinalResult)  );
	}
	else
	{
		SG_ERR_RETHROW;
	}

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pString1);
	SG_STRING_NULLFREE(pCtx, pString2);
}

//////////////////////////////////////////////////////////////////

static void _merge__queue_set_attrbits(SG_context * pCtx,
									   SG_mrg * pMrg,
									   SG_mrg_cset_entry * pMrgCSetEntry)
{
	// set new attrbits on item without regard to anything else.

	SG_string * pString = NULL;

	// Create GID-domain repo-paths for the item.
	// This should let us by-pass all of the transient
	// repo-path issues that plagued the PendingTree
	// version of MERGE.

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString, "@%s",
											pMrgCSetEntry->bufGid_Entry)  );

	SG_ERR_CHECK(  SG_wc_tx__set_attrbits(pCtx, pMrg->pWcTx,
										  SG_string__sz(pString),
										  pMrgCSetEntry->attrBits,
										  SG_WC_STATUS_FLAGS__ZERO)  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

//////////////////////////////////////////////////////////////////

static void _merge__queue_overwrite_file_from_repo(SG_context * pCtx,
												   SG_mrg * pMrg,
												   SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
												   SG_mrg_cset_entry_neq * pNeq)
{
	SG_string * pString = NULL;
	SG_mrg_cset_entry_neq neqToClear = (SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB
										| SG_MRG_CSET_ENTRY_NEQ__ATTRBITS);
	SG_bool bBackupFile;

	switch (pMrg->eFakeMergeMode)
	{
	case SG_FAKE_MERGE_MODE__MERGE:		// item was modified in "C" and not
	case SG_FAKE_MERGE_MODE__UPDATE:	// in "B", so no backup required.
		bBackupFile = SG_FALSE;
		break;

	case SG_FAKE_MERGE_MODE__REVERT:	// dirty file in "A" and "B".
		// (Remember that in a REVERT-ALL *both* the "A" and "B" nodes were
		// set to WD.  And that here the actual virgin baseline is in "C".)
		bBackupFile = (!pMrg->bRevertNoBackups);
		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Entry)  );

#if TRACE_WC_ATTRBITS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "QueueOverwriteFileFromRepo: [neq 0x%x] [attrbits %d] %s\n",
							   *pNeq, ((SG_uint32)pMrgCSetEntry_FinalResult->attrBits),
							   SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname))  );
#endif

	SG_ERR_CHECK(  SG_wc_tx__overwrite_file_from_repo(pCtx, pMrg->pWcTx,
													  SG_string__sz(pString),
													  pMrgCSetEntry_FinalResult->bufHid_Blob,
													  pMrgCSetEntry_FinalResult->attrBits,
													  bBackupFile,
													  SG_WC_STATUS_FLAGS__ZERO)  );

	*pNeq &= ~neqToClear;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

static void _merge__queue_overwrite_file_from_temp(SG_context * pCtx,
												   SG_mrg * pMrg,
												   SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
												   SG_mrg_cset_entry_neq * pNeq)
{
	SG_string * pString = NULL;
	SG_mrg_cset_entry_neq neqToClear = (SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB
										| SG_MRG_CSET_ENTRY_NEQ__ATTRBITS);

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Entry)  );

	// Overwrite the file content *and* always set the attrbits.
	SG_ERR_CHECK(  SG_wc_tx__overwrite_file_from_file(pCtx, pMrg->pWcTx,
													  SG_string__sz(pString),
													  pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->pPathTempFile_Result,
													  pMrgCSetEntry_FinalResult->attrBits,
													  SG_WC_STATUS_FLAGS__ZERO)  );

	*pNeq &= ~neqToClear;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

static void _merge__queue_alter_file(SG_context * pCtx,
									 SG_mrg * pMrg,
									 SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
									 sg_wc_liveview_item * pLVI,
									 SG_mrg_cset_entry_neq * pNeq)
{
	SG_bool bCreatedResultFile;
	SG_bool bIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
	SG_string * pStringRepoPath = NULL;

	SG_ASSERT(  ((*pNeq) & SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB)  );

	if (pMrgCSetEntry_FinalResult->bufHid_Blob[0] != 0)
	{
		// The HID of the final-result is ONLY set when
		// we inherit the value from one of the leaves.
		// (As opposed to any auto-merge case.)
		//
		// From 'neq' we also know that the HID doesn't
		// match the baseline, so it was modified in the
		// other branch.
		//
		// Overwrite the content by fetching it from the
		// repo.  Also deal with attrbits.
		//
		// See S6195.

		// Also, since we are fetching the content from the repo
		// we don't care about the sparseness of the file.

		SG_ERR_CHECK(  _merge__queue_overwrite_file_from_repo(pCtx, pMrg,
															  pMrgCSetEntry_FinalResult,
															  pNeq)  );
		return;
	}

	// We have 2 different versions of the file content
	// that must be merged somehow.

	if (bIsSparse)
	{
		// TODO 2012/12/05 A sparse file needs to be auto- or manually-merged.
		// TODO            In theory, we could backfill the file first and then
		// TODO            treat it like normal (so in effect the file would
		// TODO            become "un-sparse" first and then remain "un-sparse"
		// TODO            after this operation).  This side-effect make --test
		// TODO            mode tricky.
		// TODO
		// TODO            *OR* since we know the sparse file is currently
		// TODO            un-edited (it *is* sparse afterall), we could just
		// TODO            do the auto-merge stuff in TEMP like we do now and
		// TODO            populate the result (so that it would become "un-sparse"
		// TODO            as a result of this operation).  This would keep
		// TODO            --test happy.
		// TODO
		// TODO            But either way making something "un-sparse" needs
		// TODO            to inspect the parent directory (in case it too is
		// TODO            sparse).
		// TODO
		// TODO            So for now, I'm just going to let this throw.
		// TODO            In 2.5 we're only officially supporting sparse to
		// TODO            allow symlinks to sparsely-exist on Windows.
		// TODO            We'll revisit this when we allow full sparseness
		// TODO            in 3.0.
		// TODO
		// TODO 2012/12/11 I'm not sure we'll actually get here.  We need
		// TODO            to throw in _try_automerge_files() because the WC
		// TODO            version isn't populated and cannot be copied to
		// TODO            the temp dir.
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pMrg->pWcTx, pLVI,
																  &pStringRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_WC_IS_SPARSE,
						(pCtx, "The file '%s' needs to be merged, but is sparse.",
						 SG_string__sz(pStringRepoPath))  );
	}

	if (pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->flags
		& SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK)
	{
		// Auto-merge was successful, so we silently supply
		// the version of the file that auto-merge created.
		// And deal with attrbits.

		SG_ERR_CHECK(  _merge__queue_overwrite_file_from_temp(pCtx, pMrg,
															  pMrgCSetEntry_FinalResult,
															  pNeq)  );
		return;
	}

	// a file-content merge was required, but something went wrong.

	SG_ASSERT(  (pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->flags
				 & (SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT
					|SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_ERROR
					|SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__NO_RULE
					|SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD))  );

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx,
											  pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->pPathTempFile_Result,
											  &bCreatedResultFile, NULL, NULL)  );
	if (bCreatedResultFile)
	{
		// We ran the tool and it managed to spit out a conflict file.
		// Install it as the merge-result.  (This will probably have the
		// "<<<<<<<< |||||||| >>>>>>>" markers, for example.)
		//
		// This will also deal with attrbits.

		SG_ERR_CHECK(  _merge__queue_overwrite_file_from_temp(pCtx, pMrg,
															  pMrgCSetEntry_FinalResult,
															  pNeq)  );
		return;
	}
	
	// We either didn't run a tool or it had a problem and didn't
	// write out a conflict file.  So we don't have a merge-result
	// to install.
	//
	// In R00031 we decided to leave the baseline version of the
	// file in place.  This might be a little confusing since a
	// plain STATUS might not say that anything is wrong (rather
	// the detail might only show up on the conflict-portion of
	// STATUS and/or RESOVLE).
	//
	// Since we're not changing the file content, we don't need to
	// set the disposable-hid.
	//
	// Since we're not changing the file content, we'll let our
	// caller deal with attrbit changes.

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

static void _merge__queue_alter_symlink(SG_context * pCtx,
										SG_mrg * pMrg,
										SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
										SG_mrg_cset_entry_neq * pNeq)
{
	SG_string * pString = NULL;
	SG_mrg_cset_entry_neq neqToClear = (SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB
										| SG_MRG_CSET_ENTRY_NEQ__ATTRBITS);

	// change the content of the symlink (what it points to)
	// to the value that it has in the other changeset.
	// 
	// since changing the target of a symlink (usually) requires
	// a delete/create operation in the filesystem, go ahead and
	// set the correct attrbits at the same time.

	SG_ASSERT(  (pMrgCSetEntry_FinalResult->bufHid_Blob[0] != 0)  );
	
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Entry)  );

	SG_ERR_CHECK(  SG_wc_tx__overwrite_symlink_from_repo(pCtx, pMrg->pWcTx,
														 SG_string__sz(pString),
														 pMrgCSetEntry_FinalResult->bufHid_Blob,
														 pMrgCSetEntry_FinalResult->attrBits,
														 SG_WC_STATUS_FLAGS__ZERO)  );

	*pNeq &= ~neqToClear;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__queue_plan__peer(SG_context * pCtx, SG_mrg * pMrg,
									   const char * pszKey_GidEntry,
									   SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
									   const SG_mrg_cset_entry * pMrgCSetEntry_Baseline)
{
	SG_mrg_cset_entry_neq neq;
	sg_wc_liveview_item * pLVI;
	SG_bool bKnown;
	SG_bool bLostInB;

	bLostInB = ((pMrgCSetEntry_Baseline->statusFlags__using_wc & SG_WC_STATUS_FLAGS__U__LOST) != 0);
	if (bLostInB)
	{
		switch (pMrg->eFakeMergeMode)
		{
		case SG_FAKE_MERGE_MODE__MERGE:
			// SG_mrg_cset_entry__load__using_wc() does not currently allow regular MERGEs
			// to proceed with LOST items in the WD.
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

		case SG_FAKE_MERGE_MODE__UPDATE:
			SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__undo_lost(pCtx, pMrg, pszKey_GidEntry, pMrgCSetEntry_FinalResult)  );
			return;

		case SG_FAKE_MERGE_MODE__REVERT:
			SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__undo_lost(pCtx, pMrg, pszKey_GidEntry, pMrgCSetEntry_FinalResult)  );
			return;

		default:
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
	}
	
	// compare the baseline version and the final result version.  this version of 'neq'
	// reflects only the differences in those 2 versions (and none of the history).

	SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx,
											pMrgCSetEntry_FinalResult,
											pMrgCSetEntry_Baseline,
											&neq)  );
	if (neq == SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL)
	{
#if 0 && TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("QueuePlanPeer: Baseline version unchanged in final-result: [gid %s] %s\n"),
								   pszKey_GidEntry,
								   SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname))  );
#else
		SG_UNUSED( pszKey_GidEntry );
#endif
		pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;
		pMrg->nrUnchanged++;

		// NOTE 2012/10/17 With the changes for W4887 (where we don't force a make-unique-filename
		// NOTE            on conflicts) it is possible for the item in the final result to be
		// NOTE            EQUAL to the baseline version and us need to generate an ISSUE for it.
		// NOTE            If there is an outstanding issue (from a previous MERGE/UPDATE), the
		// NOTE            new issue will clash and we should shut it down.  We do that test in
		// NOTE            __prepare_issues rather than here so that we don't break the carry-forward
		// NOTE            stuff.
		return;
	}

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry_FinalResult->uiAliasGid,
														 &bKnown, &pLVI)  );
	SG_ASSERT( (bKnown) );
	if (pLVI->pvhIssue)
		SG_ERR_CHECK(  SG_mrg_cset_entry__check_for_outstanding_issue(pCtx, pMrg, pLVI,
																	  pMrgCSetEntry_FinalResult)  );

	// If there are changes, we handle them in upto 3 steps:
	// [1] update the content of the item;
	// [2] update the attrbits of the item;
	// [3] move/rename the item.
	//
	// Since [1] often involves creating/overwriting the
	// item on disk, we let it take care of setting the
	// proper attrbits during the create.  So we
	// pass the 'neq' by address so that dealt-with bits
	// can be removed from it.
	//
	// Note that we don't have a _NEQ__HID_BLOB bit for
	// directories because we don't handle the contents
	// of a directory like that.

	if (neq & SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB)
		SG_ERR_CHECK(  _merge__queue_alter_file(pCtx, pMrg,
												pMrgCSetEntry_FinalResult,
												pLVI,
												&neq)  );
	else if (neq & SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB)
		SG_ERR_CHECK(  _merge__queue_alter_symlink(pCtx, pMrg,
												   pMrgCSetEntry_FinalResult,
												   &neq)  );
	else
	{
		// TODO 2012/01/30 Deal with subrepo hid...
	}

	// if there were changes to the attrbits
	// (that have not already been addressed), do them now.

	if (neq & SG_MRG_CSET_ENTRY_NEQ__ATTRBITS)
		SG_ERR_CHECK(  _merge__queue_set_attrbits(pCtx, pMrg,
												  pMrgCSetEntry_FinalResult)  );

	// if there were any move/rename changes, handle
	// them together.  if we run into a transient
	// collision, park the otherwise fully updated
	// item and defer the move/rename.

	if (neq & (SG_MRG_CSET_ENTRY_NEQ__GID_PARENT
			   |SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME))		// a MOVE and/or RENAME
		SG_ERR_CHECK(  _merge__queue_move_rename(pCtx, pMrg,
												 pMrgCSetEntry_FinalResult)  );

	// Indicate that everything has been applied
	// for this item.  That is a bit of a lie if
	// the __MARKER__FLAGS__PARKED bit is also set.

	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;
	pMrg->nrChanged++;

	if (pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict)
		if (pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->flags
			& SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK)
			pMrg->nrAutoMerged++;

fail:
	return;
}
