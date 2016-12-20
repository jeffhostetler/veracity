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
 * @file sg_wc_tx__merge__determine_target_cset.c
 *
 * @details Determine the target cset of the MERGE using the
 * optional rev-spec arg and/or the state of the current WD.
 *
 * This replaces SG_vv_utils__rev_spec_to_csid_for_merge().
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _merge__compute_target_hid(SG_context * pCtx,
									   SG_mrg * pMrg)
{
	const SG_rev_spec * pRevSpec = ((pMrg->pMergeArgs) ? pMrg->pMergeArgs->pRevSpec : NULL);
	SG_stringarray * psaHids = NULL;
	SG_stringarray * psaMissingHids = NULL;
	SG_rev_spec * pRevSpec_Allocated = NULL;
	SG_bool bRequestedAttachedBranch = SG_FALSE;
	SG_stringarray * psaBranchesRequested = NULL;
	const char * pszBranchNameRequested = NULL;
	SG_uint32 nrMatched = 0;
	SG_uint32 nrMatchedExcludingParent = 0;

	if (pRevSpec)
	{
		SG_uint32 uTotal = 0u;
		SG_uint32 uBranches = 0u;

		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &uTotal)  );
		SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, pRevSpec, &uBranches)  );
		if (uTotal == 0u)
		{
			// if the rev spec is empty, just pretend it doesn't exist
			pRevSpec = NULL;
		}
		else if (uTotal > 1u)
		{
			// we can only handle a single specification
			SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Merge can accept at most one revision/tag/branch specifier."));
		}
		else if (uTotal == 1u && uBranches == 1u)
		{
			SG_ERR_CHECK(  SG_rev_spec__branches(pCtx, (/*const*/ SG_rev_spec *)pRevSpec, &psaBranchesRequested)  );
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaBranchesRequested, 0, &pszBranchNameRequested)  );

			if (pMrg->pszBranchName_Starting)
				bRequestedAttachedBranch = (strcmp(pszBranchNameRequested, pMrg->pszBranchName_Starting) == 0);
		}
	}

	if (!pRevSpec)
	{
        if (!pMrg->pszBranchName_Starting)
            SG_ERR_THROW(  SG_ERR_NOT_TIED  );

		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_Allocated)  );
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec_Allocated, pMrg->pszBranchName_Starting)  );
		pRevSpec = pRevSpec_Allocated;
		pszBranchNameRequested = pMrg->pszBranchName_Starting;
		bRequestedAttachedBranch = SG_TRUE;
	}

	// Lookup the given (or synthesized) --rev/--tag/--branch
	// and see how many csets it refers to.  Disregard/filter-out
	// any that are not present in the local repo.

	SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pMrg->pWcTx->pDb->pRepo, pRevSpec, SG_TRUE,
											  &psaHids, &psaMissingHids)  );
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHids, &nrMatched)  );
	if (nrMatched == 0)
	{
		SG_uint32 nrMissing = 0;
		SG_ASSERT_RELEASE_FAIL(  (psaMissingHids != NULL)  );
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaMissingHids, &nrMissing)  );
		if (nrMissing == 1)
		{
			const char * psz_0;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaMissingHids, 0, &psz_0)  );
			SG_ERR_THROW2(  SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT,
							(pCtx, "Branch '%s' refers to changeset '%s'. Consider pulling.",
							 pszBranchNameRequested, psz_0)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT,
							(pCtx, "Branch '%s' refers to %d changesets that are not present. Consider pulling.",
							 pszBranchNameRequested, nrMissing)  );
		}
	}
	else if (nrMatched == 1)
	{
		// We found a single unique match for our request.
		// We ***DO NOT*** disqualify the current baseline
		// in this case.  We let routines like do_cmd_merge_preview()
		// report that.

		const char * psz_0;
		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHids, 0, &psz_0)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_0, &pMrg->pszHidTarget)  );
	}
	else
	{
		// We can only get here if pRevSpec contained a "--branch ..."
		// reference (because the "--rev" lookup throws when given a
		// non-unique prefix and "--tag" can only be bound to a single
		// cset).
		//
		// If they referenced the attached branch (and the baseline is
		// pointing at a head), we'll get our baseline in the result set,
		// so get rid of it.
		SG_ERR_CHECK(  SG_stringarray__remove_all(pCtx, psaHids, pMrg->pszHid_StartingBaseline, NULL)  );
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHids, &nrMatchedExcludingParent)  );

		if (nrMatchedExcludingParent == 1)
		{
			// parent may or may not be a head of this branch, but
			// we found a single head or single other head.
			const char * psz_0;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHids, 0, &psz_0)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_0, &pMrg->pszHidTarget)  );
		}
		else if (nrMatchedExcludingParent < nrMatched)
		{
			// There were at least 3 heads of this branch and the baseline
			// is one of them.  Throwing a generic 'needs merge' message is
			// not helpful.
			SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
							(pCtx, "Branch '%s' has %d heads (excluding the baseline). Consider merging one of the other heads using --rev/--tag.",
							 pszBranchNameRequested, nrMatchedExcludingParent)  );
		}
		else //if (nrMatchedExcludingParent == nrMatched)
		{
			// The requested branch has multiple heads and the current
			// baseline is NOT one of them.  The current baseline MAY OR MAY NOT
			// be in that branch.  (And independently, we may or may not be
			// attached to that branch.)
			//
			// See how the heads are related to the current baseline.
			const char * pszDescendant0 = NULL;
			const char * pszAncestor0 = NULL;
			SG_uint32 nrDescendants = 0;
			SG_uint32 nrAncestors = 0;
			SG_uint32 k;
			for (k=0; k<nrMatched; k++)
			{
				const char * psz_k;
				SG_dagquery_relationship dqRel;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHids, k, &psz_k)  );
				SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pMrg->pWcTx->pDb->pRepo,
																	 SG_DAGNUM__VERSION_CONTROL,
																	 psz_k, pMrg->pszHid_StartingBaseline,
																	 SG_FALSE, SG_FALSE, &dqRel)  );
				if (dqRel == SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
				{
					pszDescendant0 = psz_k;
					nrDescendants++; // target[k] is descendant of baseline
				}
				else if (dqRel == SG_DAGQUERY_RELATIONSHIP__ANCESTOR)
				{
					pszAncestor0 = psz_k;
					nrAncestors++;	// target[k] is ancestor of baseline
				}
			}
			SG_ASSERT(  ((nrDescendants == 0) || (nrAncestors == 0))  );
			if (nrDescendants == 1)
			{
				if (bRequestedAttachedBranch)			// The current baseline is attached to the same branch, just not a head.
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. Only changeset '%s' is a descendant of the current baseline. Consider updating to it and then merging the branch.",
									 pszBranchNameRequested, nrMatched, pszDescendant0)  );
				else if (pMrg->pszBranchName_Starting)	// currently attached to a different branch
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. Only changeset '%s' is a descendant of the current baseline. Consider updating to it. You are attached to branch '%s'.",
									 pszBranchNameRequested, nrMatched, pszDescendant0, pMrg->pszBranchName_Starting)  );
				else									// currently detached
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. Only changeset '%s' is a descendant of the current baseline. Consider updating to it. You are not attached to a branch.",
									 pszBranchNameRequested, nrMatched, pszDescendant0)  );
			}
			else if (nrDescendants > 1)					// nrDescendants may or may not be equal to nrMatched since there may be peers too.
			{
				if (bRequestedAttachedBranch)			// The current baseline is attached to the same branch, just not a head.
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. %d are descendants of the current baseline. Consider updating to one of them and then merging the branch.",
									 pszBranchNameRequested, nrMatched, nrDescendants)  );
				else if (pMrg->pszBranchName_Starting)	// currently attached to a different branch
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. %d are descendants of the current baseline. Consider updating to one of them. You are attached to branch '%s'.",
									 pszBranchNameRequested, nrMatched, nrDescendants, pMrg->pszBranchName_Starting)  );
				else									// currently detached
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. %d are descendants of the current baseline. Consider updating to one of them. You are not attached to a branch.",
									 pszBranchNameRequested, nrMatched, nrDescendants)  );
			}
			else if (nrAncestors == 1)
			{
				if (bRequestedAttachedBranch)			// The current baseline is attached to the same branch, but the head pointer is not pointing at us.
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. Changeset '%s' is an ancestor of the current baseline. Consider moving that head forward and then merging the branch.",
									 pszBranchNameRequested, nrMatched, pszAncestor0)  );
				else if (pMrg->pszBranchName_Starting)	// currently attached to a different branch
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. Changeset '%s' is an ancestor of the current baseline. Consider moving that head forward. You are attached to branch '%s'.",
									 pszBranchNameRequested, nrMatched, pszAncestor0, pMrg->pszBranchName_Starting)  );
				else									// currently detached
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. Changeset '%s' is an ancestor of the current baseline. Consider moving that head forward. You are not attached to a branch.",
									 pszBranchNameRequested, nrMatched, pszAncestor0)  );
			}
			else if (nrAncestors > 1)					// nrAncestors may or may not be equal to nrMatched since there may be peers too.
			{
				SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
								(pCtx, "Branch '%s' has %d heads. All of them are ancestors of the current baseline. Consider moving one of the heads forward and removing the others.",
								 pszBranchNameRequested, nrMatched)  );
			}
			else										// All of the heads are peers of the current baseline.
			{
				if (bRequestedAttachedBranch)			// The current baseline is attached to the same branch, but the head pointer is not pointing at us.
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. All are peers of the current baseline. Consider merging one of the other heads using --rev/--tag.",
									 pszBranchNameRequested, nrMatched)  );
				else if (pMrg->pszBranchName_Starting)	// currently attached to a different branch
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. All are peers of the current baseline. Consider merging one of the other heads using --rev/--tag. You are attached to branch '%s'.",
									 pszBranchNameRequested, nrMatched, pMrg->pszBranchName_Starting)  );
				else									// currently detached
					SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE,
									(pCtx, "Branch '%s' has %d heads. All are peers of the current baseline. Consider merging one of the other heads using --rev/--tag. You are not attached to a branch.",
									 pszBranchNameRequested, nrMatched)  );
			}
		}
	}

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaBranchesRequested);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHids);
	SG_STRINGARRAY_NULLFREE(pCtx, psaMissingHids);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_Allocated);
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__determine_target_cset(SG_context * pCtx,
											SG_mrg * pMrg)
{
	SG_ERR_CHECK(  _merge__compute_target_hid(pCtx, pMrg)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Merge: target   (%s).\n",
							   pMrg->pszHidTarget)  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * This routine is used by UPDATE to fake a MERGE; that is, to do
 * a one-legged merge.  This routine must do set the same fields
 * in pMrg that the above routine does.
 *
 * I've isolated the effects here because UPDATE has already done
 * most of the homework/validation and we don't need to step thru
 * the stuff again here.
 *
 */
void sg_wc_tx__fake_merge__update__determine_target_cset(SG_context * pCtx,
														 SG_mrg * pMrg,
														 const char * pszHidTarget)
{
	SG_ERR_CHECK(  SG_strdup(pCtx, pszHidTarget, &pMrg->pszHidTarget)  );

	// Don't worry about pMrg->pvhPile; it is just a cache and loaded as needed.

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "FakeMergeUpdate: target   (%s).\n",
							   pMrg->pszHidTarget)  );
#endif

fail:
	return;
}

void sg_wc_tx__fake_merge__revert_all__determine_target_cset(SG_context * pCtx,
															 SG_mrg * pMrg)
{
	SG_ERR_CHECK(  SG_strdup(pCtx, pMrg->pszHid_StartingBaseline, &pMrg->pszHidTarget)  );

	// Don't worry about pMrg->pvhPile; it is just a cache and loaded as needed.

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "FakeMergeRevertAll: target   (%s).\n",
							   pMrg->pszHidTarget)  );
#endif

fail:
	return;
}
