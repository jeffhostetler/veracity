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

/**
 * This file contains code to perform a "fast-forward" merge.
 *
 * This code is intended to be called AFTER a regular
 * merge has failed with a 'cannot merge changeset
 * with ancestor' error.
 *
 * Since a "ff-merge" is basically a BRANCH MOVE-HEAD
 * and an UPDATE, we are bound by the same restrictions
 * that UPDATE has.
 *
 * WARNING: The layer-8 MERGE code calls the layer-6 "regular"
 *          merge code (in wc6merge) to try to do the normal
 *          2 parent merge.  If that fails with the ancestor
 *          error, the layer-8 MERGE code then calls us (the
 *          layer-6 ff-merge (in wc6ffmerge) code which
 *          determines if we actually qualify for a ff-merge
 *          and we call the UPDATE code to do the actual work.
 *          NOTE THAT the UPDATE code calls the wc6merge code
 *          in fake-merge update-mode, so don't be surprised
 *          if/when the wc6merge code runs twice (with different
 *          arguments).
 *
 * 
 *****************************************************************
 * In the following example, there is nothing magic
 * about the "trunk" nor "feature" branches.  They are
 * just names (aka pointers) to 2 csets.
 *****************************************************************
 * 
 * A "fast-forward merge" can be used in cases where the
 * target cset is a proper descendant of the current cset.
 * 
 *      A       # trunk branch head
 *     /
 *    B         # feature branch head
 *
 * Technically, a "fast-forward merge" is a bit of a misnomer.
 * There's nothing to do to the DAG.  CSet B already contains
 * all of the changes in A, so no merge is required.
 * 
 * The point of the "ff merge" is to move the current branch
 * head and maybe do an UPDATE on the WD to be based on B.
 *
 * This can have the net-effect of looking like a trivial
 * merge (and lets us hide the gory details from the user).
 * 
 *****************************************************************
 *
 * If we have a DAG like above and want to do a fast-forward
 * merge, there are several cases to consider:
 *
 * [1] If the WD is unattached, we THROW.
 * 
 *     The whole point is to manipulate WD's current
 *     branch head.
 *
 *     TODO 2012/09/05 Without the attachment, this is
 *     TODO            just a 'vv update -b/-r target'.
 *     TODO            Not sure I want to do this for them.
 *     TODO            If we do, does it attach too?
 *
 * [2] If the WD is attached to a branch, but the WD
 *     parent is not the (singular/unique) head of the
 *     branch, we THROW.
 *     
 *     It isn't proper to arbitrarily move the head
 *     away from its current location (whereever that
 *     is).  And we don't want to do a FF merge if
 *     the branch is in a needs-merge state, right?
 *
 * [3] If your WD CSet and the merge-target are PEERS
 *     (neither is a descendant of the other), we THROW.
 *
 *     FF-MERGE is not defined in this case.
 * 
 * [4] If your WD CSet is a DESCENDANT of the merge-target,
 *     we THROW.
 * 
 *     There isn't anything to do to the WD (it already
 *     contains all of the target's changes) *and* we
 *     *DO NOT* want to move the head of the target
 *     branch (because it is unlrelated to our
 *     current attachment and we don't do that).
 *
 *     For example, if the WD has parent B and is attached
 *     to the branch 'feature' and the user says:
 *        vv merge -b trunk
 *
 * [5] If your WD CSet is an ANCESTOR of the merge-target,
 *     we ALLOW a FF-MERGE.
 *
 *     For example, if the WD has parent A and is attached
 *     to the branch 'trunk' and the user says:
 *        vv merge -b feature
 * 
 *     we effectively need to do:
 *        vv update -r B
 *        vv branch move-head trunk -r B
 *
 *     Afterwards, the WD is still attached to
 *     trunk, has parent B, and BOTH the 'trunk'
 *     and 'feature' branches point to B:
 *
 *       A
 *      /
 *     B       # trunk & feature head
 *
 * [5a] Note that case [5] works the same as if the
 *      user says:
 *        vv merge -r B
 *
 *      That is, there may or may not be a branch head
 *      associated with the B, but we can still move
 *      the head associated with A forward.
 *
 * NOTE 2012/09/05 I'm assuming that our caller has already
 * NOTE            validated that a '-b target' evaluates
 * NOTE            to a unique target head and changeset.
 * 
 *****************************************************************
 * Also, since we are doing a 'vv branch move-head' we must
 * ensure that they have done a 'vv whoami' first.  This is
 * not required by the regular merge. (That step happens during
 * the 'vv commit'.)
 * 
 *****************************************************************
 * 
 * GIT has the concept of a no-ff merge which inserts
 * a trivial A' node and then does a regular merge.
 *
 *       A
 *      / \
 *     B   A'
 *      \ /
 *       M
 * 
 * We currently DO NOT handle this case.
 * 
 *****************************************************************     
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * See if we qualify for a FF merge.
 *
 */
void _ffmerge__qualify(SG_context * pCtx,
					   const SG_pathname* pPathWc,
					   const SG_wc_merge_args * pMergeArgs,
					   char ** ppszTiedBranchName,
					   char ** ppsz_hid_wc,
					   char ** ppsz_hid_target)
{
	SG_wc_tx * pWcTx = NULL;
	SG_varray * pvaParents = NULL;
	SG_uint32 nrParents = 0;
	const char * psz_hid_wc_ref = NULL;
	char * psz_hid_wc = NULL;
	char * pszTiedBranchName = NULL;
	SG_vhash * pvh_pile = NULL;
	SG_vhash * pvh_branches = NULL;	// we do not own this
	SG_vhash * pvh_branch_info = NULL;	// we do not own this
	SG_vhash * pvh_branch_values = NULL;	// we do not own this
	SG_bool b_wc_is_head = SG_FALSE;
	SG_uint32 nrHeads = 0;
	char * psz_hid_target = NULL;
	SG_dagquery_relationship dqRel;
	SG_audit q;

	// TODO 2012/10/09 In the 2.5/WC tree, this code is only properly
	// TODO            usable by the level-8 SG_wc__merge() code.  It
	// TODO            has already done most of the pre-checks upto and
	// TODO            including getting the target-hid and verifying
	// TODO            the relationship to the baseline.
	// TODO
	// TODO            We should do one more round of refactoring and
	// TODO            pass in the target-hid and just assume the rest
	// TODO            of the setup.

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_FALSE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx, pWcTx, &pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &nrParents)  );
	if (nrParents > 1)
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,
						(pCtx, "Working copy already contains an uncommitted merge."));
	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 0, &psz_hid_wc_ref)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_wc_ref, &psz_hid_wc)  );

	SG_ERR_CHECK(  sg_wc_tx__merge__compute_preview_target(pCtx, pWcTx, pMergeArgs, &psz_hid_target)  );

	// Per K5523, make sure that the target is a proper descendant
	// of the current cset before we bother qualifying it for a ff-merge.
	// In all cases, if the target is not a descendant it doesn't matter
	// whether the WD is attached/detached, a head, or etc.
	SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pWcTx->pDb->pRepo, SG_DAGNUM__VERSION_CONTROL,
														 psz_hid_target, psz_hid_wc,
														 SG_FALSE, SG_FALSE, &dqRel)  );
	switch (dqRel)
	{
	default:
	case SG_DAGQUERY_RELATIONSHIP__UNKNOWN:
	case SG_DAGQUERY_RELATIONSHIP__SAME:			// target == wc
	case SG_DAGQUERY_RELATIONSHIP__PEER:			// [3] target is peer of wc
	case SG_DAGQUERY_RELATIONSHIP__ANCESTOR:		// [4] target is ancestor of wc
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "dqRel(%s,%s)=%d", psz_hid_target, psz_hid_wc, dqRel)  );

	case SG_DAGQUERY_RELATIONSHIP__DESCENDANT:		// [5] target is descendant of wc
		break;
	}

	// TODO 2012/10/09 Everything above this point could be omitted
	// TODO            with the refactoring described above.

	SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pWcTx, &pszTiedBranchName)  );
	if (!pszTiedBranchName || !*pszTiedBranchName)
	{
		// case [1]
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_FF_MERGE,
						(pCtx, "The working copy is not attached to a branch; just 'vv update' to the target changeset.")  );
	}

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pWcTx->pDb->pRepo, &pvh_pile)  );
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
	if (!pvh_branches)
	{
		// there are no branches?  Is this possible?
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_FF_MERGE,
						(pCtx, "The attached branch '%s' does not exist.", pszTiedBranchName)  );
	}
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branches, pszTiedBranchName, &pvh_branch_info)  );
	if (!pvh_branch_info)
	{
		// The attached branch does not exist?  Is this possible?
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_FF_MERGE,
						(pCtx, "The attached branch '%s' does not exist.", pszTiedBranchName)  );
	}
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
	if (!pvh_branch_values)
	{
		// Malformed branch sub-section?
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_FF_MERGE,
						(pCtx, "The attached branch '%s' does not exist.", pszTiedBranchName)  );
	}

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branch_values, psz_hid_wc, &b_wc_is_head)  );
	if (!b_wc_is_head)
	{
		// [2] The WC is not a/the head of this branch.
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_FF_MERGE,
						(pCtx, "The working copy is not the head of the '%s' branch.", pszTiedBranchName)  );
	}
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &nrHeads)  );
	if (nrHeads > 1)
	{
		// [2] This branch has more than one head.
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_FF_MERGE,
						(pCtx, "The branch '%s' has multiple heads.", pszTiedBranchName)  );
	}

	// We can logically do a FF-merge to the target cset
	// provides we have a whoami set.
	//
	// Create an audit structure (and let it throw if no
	// user is set) before we attempt anything.

	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pWcTx->pDb->pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

		*ppszTiedBranchName = pszTiedBranchName; pszTiedBranchName = NULL;
		*ppsz_hid_target = psz_hid_target;       psz_hid_target = NULL;
		*ppsz_hid_wc = psz_hid_wc;               psz_hid_wc = NULL;

fail:
	if (pWcTx)
	{
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
		SG_WC_TX__NULLFREE(pCtx, pWcTx);
	}
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
    SG_NULLFREE(pCtx, pszTiedBranchName);
	SG_NULLFREE(pCtx, psz_hid_target);
	SG_NULLFREE(pCtx, psz_hid_wc);
}

static void _ffmerge__move_head(SG_context * pCtx,
								const SG_pathname* pPathWc,
								char * pszTiedBranchName,
								char * psz_hid_wc,
								char * psz_hid_target)
{
	SG_wc_tx * pWcTx = NULL;
	SG_audit q;

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_FALSE)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pWcTx->pDb->pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_vc_branches__move_head(pCtx, pWcTx->pDb->pRepo,
											 pszTiedBranchName,
											 psz_hid_wc,
											 psz_hid_target,
											 &q)  );

fail:
	if (pWcTx)
	{
		// move-head does not need the TX (we only opened
		// it to get the pRepo within) so we can always
		// cancel the TX.
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
		SG_WC_TX__NULLFREE(pCtx, pWcTx);
	}
}

/**
 * Try to do a ff-merge.
 *
 */				   
void sg_wc_tx__ffmerge__main(SG_context * pCtx,
							 const SG_pathname* pPathWc,
							 const SG_wc_merge_args * pMergeArgs,
							 SG_bool bTest,
							 SG_varray ** ppvaJournal,
							 SG_vhash ** ppvhStats,
							 char ** ppszHidTarget,
							 SG_varray ** ppvaStatus)
{
	// ppvaJournal is optional
	// ppvhStats is optional
	// ppszHidTarget is optional
	// ppvaStatus is optional

	SG_rev_spec * pRevSpec = NULL;
	char * pszTiedBranchName = NULL;
	char * psz_hid_wc = NULL;
	char * psz_hid_target = NULL;
	SG_varray * pvaJournalAllocated = NULL;
	SG_vhash * pvhStatsAllocated = NULL;
	SG_varray * pvaStatusAllocated = NULL;
	SG_wc_update_args u;

	SG_ERR_CHECK(  _ffmerge__qualify(pCtx, pPathWc, pMergeArgs,
									 &pszTiedBranchName, &psz_hid_wc, &psz_hid_target)  );

	// Update with -r rev using the target we've
	// already computed.  We don't need to set an
	// attach/detach value since __qualify() will
	// verify that we are attached to a head.

	memset(&u, 0, sizeof(u));
	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_hid_target)  );
	u.pRevSpec = pRevSpec;
	u.bDisallowWhenDirty = pMergeArgs->bDisallowWhenDirty;

	SG_ERR_CHECK(  SG_wc__update(pCtx, pPathWc, &u, bTest,
								 &pvaJournalAllocated,
								 &pvhStatsAllocated,
								 &pvaStatusAllocated,
								 NULL)  );
	if (!bTest)
		SG_ERR_CHECK(  _ffmerge__move_head(pCtx, pPathWc, pszTiedBranchName,
										   psz_hid_wc, psz_hid_target)  );

	// Augment the returned the STATS with a flag to indicate
	// that we did ff-merge.
	// 
	// TODO 2012/09/11 should we append/prepend something to the 'synopsys' field too?

	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhStatsAllocated, "isFastForwardMerge", SG_TRUE)  );
	
	if (ppvaJournal)
	{
		*ppvaJournal = pvaJournalAllocated;
		pvaJournalAllocated = NULL;
	}

	if (ppvhStats)
	{
		*ppvhStats = pvhStatsAllocated;
		pvhStatsAllocated = NULL;
	}

	if (ppszHidTarget)
	{
		*ppszHidTarget = psz_hid_target;
		psz_hid_target = NULL;
	}

	if (ppvaStatus)
	{
		*ppvaStatus = pvaStatusAllocated;
		pvaStatusAllocated = NULL;
	}

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    SG_NULLFREE(pCtx, pszTiedBranchName);
	SG_NULLFREE(pCtx, psz_hid_target);
	SG_NULLFREE(pCtx, psz_hid_wc);
	SG_VARRAY_NULLFREE(pCtx, pvaJournalAllocated);
	SG_VARRAY_NULLFREE(pCtx, pvaStatusAllocated);
	SG_VHASH_NULLFREE(pCtx, pvhStatsAllocated);
}
