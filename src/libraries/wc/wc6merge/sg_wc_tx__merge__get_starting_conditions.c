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
 * @file sg_wc_tx__merge__get_starting_conditions.c
 *
 * @details Basic argument validation and sniffing of the WD.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__get_starting_conditions(SG_context * pCtx,
											  SG_mrg * pMrg)
{
	SG_varray * pvaParents = NULL;
	SG_rbtree * prbAllLeaves = NULL;
	const char * pszBaseline;
	SG_uint32 nrParents;
	SG_bool bBaselineIsLeaf;

	//////////////////////////////////////////////////////////////////
	// Get the HID of the current baseline.
	// Disallow MERGE if we have a pending MERGE.

	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx,
													pMrg->pWcTx,
													&pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &nrParents)  );
	if (nrParents > 1)
		SG_ERR_THROW2(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,
						(pCtx, "Cannot MERGE with uncommitted MERGE.")  );

	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 0, &pszBaseline)  );
	SG_ERR_CHECK(  SG_strdup(pCtx, pszBaseline, &pMrg->pszHid_StartingBaseline)  );

	//////////////////////////////////////////////////////////////////
	// We now allow each REPO to specify the Hash Algorithm when it is created.
	// This means that the length of a HID is not a compile-time constant anymore;
	// However, all of the HIDs in a REPO will have the same length.  (And we only
	// need to know this length for a "%*s" format.)

	pMrg->nrHexDigitsInHIDs = SG_STRLEN(pMrg->pszHid_StartingBaseline);

	//////////////////////////////////////////////////////////////////
	// If the caller requested that the baseline be a LEAF,
	// verify that it is.  Note that we DO NOT pre-filter
	// the set of leaves using named branches -- we want the
	// complete set of leaves.

	if (pMrg->pMergeArgs && pMrg->pMergeArgs->bComplainIfBaselineNotLeaf)
	{
		SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx,
												 pMrg->pWcTx->pDb->pRepo,
												 SG_DAGNUM__VERSION_CONTROL,
												 &prbAllLeaves)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx,
									   prbAllLeaves,
									   pMrg->pszHid_StartingBaseline,
									   &bBaselineIsLeaf, NULL)  );
		if (!bBaselineIsLeaf)
			SG_ERR_THROW(  SG_ERR_BASELINE_NOT_HEAD  );
	}

	//////////////////////////////////////////////////////////////////
	// See if the WD is currently attached/tied to a branch.
	// 
	// We DO NOT care one way or another at this point.
	// 
	// Later, as we are parsing the args we can supply
	// the current branch as the default if necessary.

	SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pMrg->pWcTx, &pMrg->pszBranchName_Starting)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Merge: baseline (%s, %s).\n",
							   pMrg->pszHid_StartingBaseline,
							   ((pMrg->pszBranchName_Starting) ? pMrg->pszBranchName_Starting : ""))  );
#endif

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
	SG_RBTREE_NULLFREE(pCtx, prbAllLeaves);
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
void sg_wc_tx__fake_merge__update__get_starting_conditions(SG_context * pCtx,
														   SG_mrg * pMrg,
														   const char * pszHid_StartingBaseline,
														   const char * pszBranchName_Starting)
{
	SG_ERR_CHECK(  SG_strdup(pCtx, pszHid_StartingBaseline, &pMrg->pszHid_StartingBaseline)  );
	pMrg->nrHexDigitsInHIDs = SG_STRLEN(pMrg->pszHid_StartingBaseline);

	if (pszBranchName_Starting)
		SG_ERR_CHECK(  SG_strdup(pCtx, pszBranchName_Starting, &pMrg->pszBranchName_Starting)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "FakeMergeUpdate: baseline (%s, %s).\n",
							   pMrg->pszHid_StartingBaseline,
							   ((pMrg->pszBranchName_Starting) ? pMrg->pszBranchName_Starting : ""))  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * In a REVERT-ALL, there is only one CSet -- the current baseline.
 * It is both the source and target, so unlike UPDATE, we don't need
 * to do any gymnastics to get it.
 *
 * Likewise, REVERT-ALL does not need to deal with branches.
 *
 * However, if there is a pending merge, we need to delete or move
 * the {tne_L1,pc_L1} tables out of the way.
 *
 */ 
void sg_wc_tx__fake_merge__revert_all__get_starting_conditions(SG_context * pCtx,
															   SG_mrg * pMrg)
{
	// pMrg->pWcTx->pCSet_Baseline and (if we currently in a merge) pMrg->pWcTx->pCSet_Other
	// have already been set by SG_wc_tx__alloc__begin().

	SG_ASSERT(  (pMrg)  );
	SG_ASSERT(  (pMrg->pWcTx)  );
	SG_ASSERT(  (pMrg->pWcTx->pCSetRow_Baseline)  );
	SG_ASSERT(  (pMrg->pWcTx->pCSetRow_Baseline->psz_hid_cset)  );

	SG_ERR_CHECK(  SG_strdup(pCtx,
							 pMrg->pWcTx->pCSetRow_Baseline->psz_hid_cset,
							 &pMrg->pszHid_StartingBaseline)  );

	pMrg->nrHexDigitsInHIDs = SG_STRLEN(pMrg->pszHid_StartingBaseline);

	if (pMrg->pWcTx->pCSetRow_Other)
	{
		// We are currently in a merge.  Unset this so that we don't
		// get confused when we start tricking the merge-engine to
		// do a revert.  The merge-engine will build {A,B,C,M} in
		// pMrgCSet-space; it shouldn't have to reference the existing
		// L1.
		//
		// We can wait until the bottom of the revert to actually drop
		// the L1 tables from the DB.  (See __postmerge__cleanup).

		SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pMrg->pWcTx->pCSetRow_Other);
	}

fail:
	return;
}

