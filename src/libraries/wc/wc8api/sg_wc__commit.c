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
 * @file sg_wc__commit.c
 *
 * @details COMMIT changes in WC creating a new changeset.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Is this a PARTIAL COMMIT ?
 *
 * We don't care about what is/isnot being committed
 * at this point; we just want a yes/no to short cut
 * some cases.
 *
 */
static void _is_partial(SG_context * pCtx,
						sg_commit_data * pCommitData)
{
	SG_uint32 nrInputs = 0;

	if (pCommitData->pCommitArgs->psaInputs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pCommitData->pCommitArgs->psaInputs, &nrInputs)  );

	if ((nrInputs == 0) && (pCommitData->pCommitArgs->depth != SG_INT32_MAX))
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Depth/Non-recursive option not allowed without one or more file-or-folder arguments.")  );

	pCommitData->bIsPartial = (nrInputs > 0);

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Build the AUDIT record using the supplied (User,When) data.
 * This allows user to override the default values and/or us
 * to automatically pick up the proper default values.  It also
 * lets us parse/validate/normalize whatever they type in.
 *
 * We construct the AUDIT record first because we need the
 * various validated fields for subsequent sanity checking
 * before we actually begin the commit.  But this has the
 * potential problem that if they pass NULL for pszWhen (meaning
 * NOW) and and then they take a week to edit the commit message,
 * then we'll have a stale timestamp when we actually do the
 * commit.  See Y8731.
 *
 * We allow for this by updating the audit record time
 * after we call the callback to their editor.
 *
 */
static void _build_audit(SG_context * pCtx,
						 sg_commit_data * pCommitData)
{
	SG_ERR_CHECK(  SG_audit__init__friendly(pCtx,
											&pCommitData->auditRecord,
											pCommitData->pWcTx->pDb->pRepo,
											pCommitData->pCommitArgs->pszWhen,
											pCommitData->pCommitArgs->pszUser)  );

#if TRACE_WC_COMMIT
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Commit: Date '%s' --> %s\n",
								   ((pCommitData->pCommitArgs->pszWhen) ? pCommitData->pCommitArgs->pszWhen : "(null)"),
								   SG_uint64_to_sz((SG_uint64)pCommitData->auditRecord.when_int64,
												   bufui64))  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Commit: User '%s' --> '%s'\n",
								   ((pCommitData->pCommitArgs->pszUser) ? pCommitData->pCommitArgs->pszUser : "(null)"),
								   pCommitData->auditRecord.who_szUserId)  );
	}
#endif

fail:
	return;
}

static void _maybe_update_time_on_audit(SG_context * pCtx,
										sg_commit_data * pCommitData)
{
	if (pCommitData->pCommitArgs->pszWhen && *pCommitData->pCommitArgs->pszWhen)
	{
		// The user specified a time on the command line
		// and does not want __WHEN__NOW treatment.
	}
	else
	{
		// The user wanted __WHEN__NOW treatment and in
		// the audit record that we constructed at the
		// start of the commit, we did that.
		//
		// But if they took a week to edit the changeset
		// comment, we want to refresh that value so that
		// it represents the actual time of the commit,
		// rather than the time they typed the command.
		//
		// See Y8731.

		SG_ERR_CHECK(  SG_audit__update_time_to_now(pCtx,
													&pCommitData->auditRecord)  );
	}

fail:
	return;
}

/**
 * We only allow commits by "active" users.
 * This requires that _build_audit() be called first.
 *
 */
static void _check_active_user(SG_context * pCtx,
							   sg_commit_data * pCommitData)
{
	SG_vhash * pvhUser = NULL;
	SG_bool bInactive;

	SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx,
											 pCommitData->pWcTx->pDb->pRepo,
											 &pCommitData->auditRecord,
											 pCommitData->auditRecord.who_szUserId,
											 &pvhUser)  );
	if (!pvhUser)
	{
		// this should not happen since _build_audit() should have failed.
		SG_ERR_THROW2(  SG_ERR_USER_NOT_FOUND,
						(pCtx, "Could not lookup user by id: '%s'.",
						 pCommitData->auditRecord.who_szUserId)  );
	}
						
	SG_ERR_CHECK(  SG_user__is_inactive(pCtx, pvhUser, &bInactive)  );
	if (bInactive)
	{
		const char * pszName = NULL;

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhUser, "name", &pszName)  );
		SG_ERR_THROW2(  SG_ERR_INACTIVE_USER,
						(pCtx, "User '%s' is inactive.", pszName)  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhUser);
}

/**
 * Verify that we have a branch or that we are (and are
 * supposed to be) detached.
 * 
 * Complain if inconsistent args given.
 *
 * Remember the computed branch name.
 *
 */
static void _check_branch(SG_context * pCtx,
						  sg_commit_data * pCommitData)
{
	SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pCommitData->pWcTx,
										&pCommitData->pszTiedBranchName)  );

	if (pCommitData->pCommitArgs->bDetached && pCommitData->pszTiedBranchName)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx,
						 "The 'detached' option is not allowed when the working copy is attached to a branch.")  );

	if (!pCommitData->pCommitArgs->bDetached && !pCommitData->pszTiedBranchName)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx,
						 "The working copy is not attached to a branch; use the 'detached' option to commit anyway.")  );

#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Commit: ok check_branch: [bDetached %d] '%s'\n",
							   pCommitData->pCommitArgs->bDetached,
							   ((pCommitData->pszTiedBranchName) ? pCommitData->pszTiedBranchName : "(null)"))  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * If they gave us any WIT associations, look them up
 * and validate.
 *
 */
static void _check_assoc(SG_context * pCtx,
						 sg_commit_data * pCommitData)
{
	const char * const * paszAssocs;	// we do not own this
	SG_uint32 nrAssoc;

	if (!pCommitData->pCommitArgs->psaAssocs)
		return;

	SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx,
													  pCommitData->pCommitArgs->psaAssocs,
													  &paszAssocs,
													  &nrAssoc)  );
	if (nrAssoc == 0)
		return;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pCommitData->pvaDescs)  );
	SG_ERR_CHECK(  SG_vc_hooks__ASK__WIT__VALIDATE_ASSOCIATIONS(pCtx,
																pCommitData->pWcTx->pDb->pRepo,
																paszAssocs,
																nrAssoc,
																pCommitData->pvaDescs)  );
#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx,
																	   pCommitData->pvaDescs,
																	   "Commit: Associations:")  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Disallow partial commits when we have a pending merge.
 * 
 * I'm going to disallow PARTIAL-COMMITS after a MERGE
 * because I'm not sure how it should work.  That is,
 * if we just commit the things indicated and say that
 * the new CSET has 2 parents:
 * [] what do we do with the remaining dirty items?
 *    going forward, does the dirt just look like normal
 *    dirt against the new baseline?
 * [] if the user then commits the remaining dirty items
 *    does that new CSET have just the partial commit as
 *    its (only) parent?
 *
 * For now, just say no.
 *
 * (Requires that we have already computed _is_partial().)
 *
 */
static void _check_parents(SG_context * pCtx,
						   sg_commit_data * pCommitData)
{
	// always compute the set of parents, regardless.

	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx,
													pCommitData->pWcTx,
													&pCommitData->pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pCommitData->pvaParents, &pCommitData->nrParents)  );

	if ((pCommitData->nrParents > 1) && (pCommitData->bIsPartial))
		SG_ERR_THROW(  SG_ERR_CANNOT_PARTIAL_COMMIT_AFTER_MERGE  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * See if there is anything actually being committed.
 * We don't want to create a trivial CSET that is identical
 * to the current baseline (with the exception of the
 * primary parent).
 *
 * We do allow trivial checkins when we have a merge
 * since they could have chosen the baseline version
 * for each change/conflict.
 *
 */
static void _check_trivial(SG_context * pCtx,
						   sg_commit_data * pCommitData)
{
	if (pCommitData->nrParents > 1)
		return;

	if (pCommitData->pWcTx->pvaJournal)
	{
		SG_uint32 count_journal = 0;

		SG_ERR_CHECK(  SG_varray__count(pCtx, pCommitData->pWcTx->pvaJournal, &count_journal)  );
		if (count_journal > 0)
			return;
	}

	SG_ERR_THROW(  SG_ERR_NOTHING_TO_COMMIT  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Compute partial/complete status for displaying info
 * in their editor to get the checkin message (when -m
 * omitted).
 *
 */
static void _compute_status_for_prompt(SG_context * pCtx,
									   sg_commit_data * pCommitData)
{
	SG_ERR_CHECK(  SG_wc_tx__status__stringarray(pCtx, pCommitData->pWcTx,
												 pCommitData->pCommitArgs->psaInputs,
												 pCommitData->pCommitArgs->depth,
												 SG_FALSE,	// bListUnchanged
												 SG_FALSE,	// bNoIgnores (don't qualify FOUND vs IGNORED)
												 SG_FALSE,	// bNoTSC (we trust it)
												 SG_FALSE,	// bListSparse
												 SG_FALSE,	// bListReserved (.sgdrawer)
												 SG_FALSE,	// bNoSort
												 &pCommitData->pvaStatusForPrompt,
												 NULL)  );

#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx,
																	   pCommitData->pvaStatusForPrompt,
																	   "Commit: StatusForPrompt:")  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Check for a commit message.  If omitted and they provided
 * a prompt callback, invoke it and let them enter the text
 * using their favorite editor (or whatever).
 *
 * THIS SHOULD BE THE LAST _CHECK_ STEP THAT WE PERFORM.
 * 
 * (Because we want all of the automatic error checking/
 * validation to happen *BEFORE* we (possibly) go
 * interactive and *ASK* them to type in a commit message.)
 *
 */
static void _check_message(SG_context * pCtx,
						   sg_commit_data * pCommitData)
{
	SG_vhash * pvhUser = NULL;
	char * pszCandidate_Allocated = NULL;
	const char * pszCandidate = NULL;
	SG_bool bAllIsWhitespace = SG_FALSE;
	SG_uint32 lenLimit = 0;
	SG_uint32 lenComment = 0;

	if (pCommitData->pCommitArgs->pszMessage)		// user gave --message option
	{
		pszCandidate = pCommitData->pCommitArgs->pszMessage;
	}
	else if (pCommitData->pCommitArgs->pfnPrompt)		// no --message, go interactive
	{
		const char * pszNormalizedUserName = NULL;

		// Show a "normalized" username in their editor.

		SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx,
												 pCommitData->pWcTx->pDb->pRepo,
												 &pCommitData->auditRecord,
												 pCommitData->auditRecord.who_szUserId,
												 &pvhUser)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhUser, "name", &pszNormalizedUserName)  );

		SG_ERR_CHECK(  _compute_status_for_prompt(pCtx, pCommitData)  );
		SG_ERR_CHECK(  (*pCommitData->pCommitArgs->pfnPrompt)(pCtx,
												 SG_TRUE,	// bForCommit
												 pszNormalizedUserName,
												 pCommitData->pszTiedBranchName,
												 pCommitData->pvaStatusForPrompt,
												 pCommitData->pvaDescs,
												 &pszCandidate_Allocated)  );
		pszCandidate = pszCandidate_Allocated;

		SG_ERR_CHECK(  _maybe_update_time_on_audit(pCtx, pCommitData)  );
	}
	
	if (!pszCandidate || !*pszCandidate)
		SG_ERR_THROW(  SG_ERR_EMPTYCOMMITMESSAGE  );

	SG_ERR_CHECK(  SG_sz__is_all_whitespace(pCtx,
											pszCandidate,
											&bAllIsWhitespace)  );
	if (bAllIsWhitespace)
		SG_ERR_THROW(  SG_ERR_EMPTYCOMMITMESSAGE  );

	SG_ERR_CHECK(  SG_sz__trim(pCtx,
							   pszCandidate,
							   NULL,
							   &pCommitData->pszTrimmedMessage)  );

	SG_ERR_CHECK(  SG_vc_comments__get_length_limit(pCtx, pCommitData->pWcTx->pDb->pRepo, &lenLimit)  );
	lenComment = SG_STRLEN(pCommitData->pszTrimmedMessage);
	if (lenComment > lenLimit)
		SG_ERR_THROW2(  SG_ERR_LIMIT_EXCEEDED,
						(pCtx, "The commit comment exceeds the comment length limit by %d bytes.", lenComment - lenLimit));

fail:
	SG_NULLFREE(pCtx, pszCandidate_Allocated);
	SG_VHASH_NULLFREE(pCtx, pvhUser);
}

//////////////////////////////////////////////////////////////////

static void _post__add_stamps(SG_context * pCtx,
							  sg_commit_data * pCommitData)
{
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_stringarray__count(pCtx,
										 pCommitData->pCommitArgs->psaStamps,
										 &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszStamp_k;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx,
											   pCommitData->pCommitArgs->psaStamps,
											   k,
											   &pszStamp_k)  );
		SG_ERR_CHECK(  SG_vc_stamps__add(pCtx,
										 pCommitData->pWcTx->pDb->pRepo,
										 pCommitData->pszHidChangeset_VC,
										 pszStamp_k,
										 &pCommitData->auditRecord,
										 NULL)  );
	}

fail:
	return;
}

static void _post__hook__add_assoc(SG_context * pCtx,
								   sg_commit_data * pCommitData)
{
	const char * const * paszAssocs;	// we do not own this
	SG_uint32 nrAssoc;

	if (!pCommitData->pCommitArgs->psaAssocs)
		return;

	SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx,
													  pCommitData->pCommitArgs->psaAssocs,
													  &paszAssocs,
													  &nrAssoc)  );
	if (nrAssoc == 0)
		return;

	SG_ERR_CHECK(  SG_vc_hooks__ASK__WIT__ADD_ASSOCIATIONS(pCtx,
														   pCommitData->pWcTx->pDb->pRepo,
														   pCommitData->pChangeset_VC,
														   pCommitData->pszTiedBranchName,
														   &pCommitData->auditRecord,
														   pCommitData->pszTrimmedMessage,
														   paszAssocs, nrAssoc,
														   pCommitData->pCommitArgs->psaStamps)  );

fail:
	return;
}

static void _post__hook__broadcast(SG_context * pCtx,
								   sg_commit_data * pCommitData)
{
	const char * const * paszAssocs = NULL;	// we do not own this
	SG_uint32 nrAssoc = 0;

	if (pCommitData->pCommitArgs->psaAssocs)
		SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx,
														  pCommitData->pCommitArgs->psaAssocs,
														  &paszAssocs,
														  &nrAssoc)  );

    SG_ERR_CHECK(  SG_vc_hooks__BROADCAST__AFTER_COMMIT(pCtx,
														pCommitData->pWcTx->pDb->pRepo,
														pCommitData->pChangeset_VC,
														pCommitData->pszTiedBranchName,
														&pCommitData->auditRecord,
														pCommitData->pszTrimmedMessage,
														paszAssocs, nrAssoc,
														pCommitData->pCommitArgs->psaStamps)  );

fail:
	return;
}

/**
 * A series of things to do *AFTER* the sql-commit of the TX
 * and wc.db has been saved (and the baseline has been updated
 * to the newly created changeset).
 *
 * TODO 2011/11/29 if any of these items fail, do we want to
 * TODO            try to keep going or throw ?
 *
 */
static void _post__deal_with_post_tx_details(SG_context * pCtx,
											 sg_commit_data * pCommitData)
{
	if (pCommitData->pszTiedBranchName && *pCommitData->pszTiedBranchName)
	{
		SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx,
												pCommitData->pWcTx->pDb->pRepo,
												pCommitData->pszTiedBranchName,
												pCommitData->pszHidChangeset_VC,
												pCommitData->pvaParents,
												SG_FALSE,
												&pCommitData->auditRecord)  );
		SG_ERR_CHECK(  SG_vc_locks__post_commit(pCtx,
												pCommitData->pWcTx->pDb->pRepo,
												&pCommitData->auditRecord,
												pCommitData->pszTiedBranchName,
												pCommitData->pChangeset_VC)  );
	}

	if (pCommitData->pszTrimmedMessage && *pCommitData->pszTrimmedMessage)
		SG_ERR_CHECK(  SG_vc_comments__add(pCtx,
										   pCommitData->pWcTx->pDb->pRepo,
										   pCommitData->pszHidChangeset_VC,
										   pCommitData->pszTrimmedMessage,
										   &pCommitData->auditRecord)  );

	if (pCommitData->pCommitArgs->psaStamps)
		SG_ERR_CHECK(  _post__add_stamps(pCtx, pCommitData)  );
	
	if (pCommitData->pCommitArgs->psaAssocs)
		SG_ERR_CHECK(  _post__hook__add_assoc(pCtx, pCommitData)  );

	SG_ERR_CHECK(  _post__hook__broadcast(pCtx, pCommitData)  );

	// if (pCommitData->nrParents > 1)
	// {
	//     Note: we no longer need to deal with the private
	//     merge temp directory here.  It is handled in the
	//     sg_wc_tx__postmerge__commit_cleanup() code along
	//     with the other housekeeping.
	// }

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Do a COMMIT -- creating a new CHANGESET using the
 * dirt in the WD.
 *
 * WARNING: This routine deviates from the model of most
 * WARNING: of the SG_wc__ routines because we cannot "pend"
 * WARNING: a COMMIT like we do a RENAME.  We actually have
 * WARNING: to create the CSET in the REPO and that is 
 * WARNING: outside the scope of the TX.
 *
 * We optionally return the JOURNAL operations (--test/--verbose).
 *
 * We optionally return the HID of the new cset (when not --test).
 *
 */
void SG_wc__commit(SG_context * pCtx,
				   const SG_pathname* pPathWc,
				   const SG_wc_commit_args * pCommitArgs,
				   SG_bool bTest,			// --test is a layer-8-only flag so don't put it in SG_wc_commit_args.
				   SG_varray ** ppvaJournal,
				   char ** ppszHidNewCSet)
{
	sg_commit_data commitData;
	SG_varray * pvaJournal = NULL;
    SG_bool b_op = SG_FALSE;

	memset(&commitData, 0, sizeof(commitData));

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Committing", SG_LOG__FLAG__NONE)  );
    b_op = SG_TRUE;

	commitData.pCommitArgs = pCommitArgs;

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &commitData.pWcTx, pPathWc, SG_FALSE)  );

	// arg-check and gather data.

	SG_ERR_CHECK(  _is_partial(           pCtx, &commitData)  );

	SG_ERR_CHECK(  _build_audit(          pCtx, &commitData)  );
	SG_ERR_CHECK(  _check_active_user(    pCtx, &commitData)  );
	SG_ERR_CHECK(  _check_branch(         pCtx, &commitData)  );
	SG_ERR_CHECK(  _check_assoc(          pCtx, &commitData)  );
	SG_ERR_CHECK(  _check_parents(        pCtx, &commitData)  );

	// qualify the parts of the tree to be committed.

	SG_ERR_CHECK(  sg_wc_tx__commit__mark_subtree(pCtx, commitData.pWcTx, pCommitArgs)  );
	SG_ERR_CHECK(  sg_wc_tx__commit__mark_bubble_up(pCtx, commitData.pWcTx, pCommitArgs)  );
#if defined(DEBUG) && TRACE_WC_COMMIT
	SG_ERR_IGNORE( sg_wc_tx__commit__dump_marks(pCtx, commitData.pWcTx, "Commit")  );
#endif

	// queue everything that needs to be done to the journal
	
	SG_ERR_CHECK(  sg_wc_tx__commit__queue(pCtx, &commitData)  );
	SG_ERR_CHECK(  _check_trivial(pCtx, &commitData)  );

	if (ppvaJournal)
		SG_ERR_CHECK(  SG_WC_TX__JOURNAL__ALLOC_COPY(pCtx,
													 commitData.pWcTx,
													 &pvaJournal)  );
	if (bTest)
	{
		SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, commitData.pWcTx)  );
		SG_WC_TX__NULLFREE(pCtx, commitData.pWcTx);
	}
	else
	{
		// We can't just call SG_wc_tx__apply() because it both
		// *runs* the plan/journal *and* does a SQL-commit. we
		// need to slip in some things in some things between
		// those 2 steps.

		SG_ERR_CHECK(  _check_message(pCtx, &commitData)  );
		SG_ERR_CHECK(  sg_wc_tx__commit__apply(pCtx, &commitData)  );

		// Commit our TX (in the sql sense) so that the WD
		// officially references the new baseline.

		SG_ERR_CHECK(  sg_wc_db__tx__commit(pCtx, commitData.pWcTx->pDb)  );

		//////////////////////////////////////////////////////////////////
		// Our TX is closed and the baseline has been changed to the NEW changeset.
		// Anything that fails *after* this point cannot change that.
		//////////////////////////////////////////////////////////////////

		SG_ERR_CHECK(  _post__deal_with_post_tx_details(pCtx, &commitData)  );
	}

	if (ppvaJournal)
	{
		*ppvaJournal = pvaJournal;
		pvaJournal = NULL;
	}

	if (ppszHidNewCSet)
	{
		*ppszHidNewCSet = commitData.pszHidChangeset_VC;
		commitData.pszHidChangeset_VC = NULL;
	}
	
fail:
    if (b_op)
		SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
 
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, commitData.pWcTx)  );
	SG_WC_TX__NULLFREE(   pCtx, commitData.pWcTx);
	SG_NULLFREE(          pCtx, commitData.pszTiedBranchName);
	SG_VARRAY_NULLFREE(   pCtx, commitData.pvaParents);
	SG_VARRAY_NULLFREE(   pCtx, commitData.pvaStatusForPrompt);
	SG_NULLFREE(          pCtx, commitData.pszTrimmedMessage);
	SG_VARRAY_NULLFREE(   pCtx, commitData.pvaDescs);
	SG_CHANGESET_NULLFREE(pCtx, commitData.pChangeset_VC);
	SG_DAGNODE_NULLFREE(  pCtx, commitData.pDagnode_VC);
	SG_NULLFREE(          pCtx, commitData.pszHidChangeset_VC);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
}
