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
 * Given pMrg->pszHid_StartingBaseline
 * and   pMrg->pszHidTarget
 * compute the LCA on the DAG.
 *
 * Given (possibley dirty) Baseline cset (B+WC)
 * and the Other cset (C), compute LCA cset (A).
 * And any interior SPCAs csets (S[k]) that we
 * happen to stumble upon.
 *
 *       A
 *      / \.
 *  B+WC   C
 *
 *
 * We now throw (in addition to hard errors):
 *     SG_ERR_MERGE_TARGET_EQUALS_BASELINE
 *     SG_ERR_MERGE_TARGET_IS_ANCESTOR_OF_BASELINE
 *     SG_ERR_MERGE_TARGET_IS_DESCENDANT_OF_BASELINE
 * when we can't compute an LCA.
 * 
 */
void sg_wc_tx__merge__compute_lca(SG_context * pCtx,
								  SG_mrg * pMrg)
{
	SG_rbtree * prb_temp = NULL;

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Merge: Computing LCA(%s, %s)....\n",
							   pMrg->pszHid_StartingBaseline,
							   pMrg->pszHidTarget)  );
#endif

	if (strcmp(pMrg->pszHid_StartingBaseline, pMrg->pszHidTarget) == 0)
		SG_ERR_THROW(  SG_ERR_MERGE_TARGET_EQUALS_BASELINE  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_temp)  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb_temp, pMrg->pszHidTarget)  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb_temp, pMrg->pszHid_StartingBaseline)  );

	SG_repo__get_dag_lca(pCtx,
						 pMrg->pWcTx->pDb->pRepo,
						 SG_DAGNUM__VERSION_CONTROL,
						 prb_temp,
						 &pMrg->pDagLca);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_DAGLCA_LEAF_IS_ANCESTOR))
		{
			SG_dagquery_relationship dqRel;

			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pMrg->pWcTx->pDb->pRepo,
																 SG_DAGNUM__VERSION_CONTROL,
																 pMrg->pszHidTarget,
																 pMrg->pszHid_StartingBaseline,
																 SG_FALSE, SG_FALSE, &dqRel)  );
			if (dqRel == SG_DAGQUERY_RELATIONSHIP__ANCESTOR)
				SG_ERR_THROW(  SG_ERR_MERGE_TARGET_IS_ANCESTOR_OF_BASELINE  );
			else
				SG_ERR_THROW(  SG_ERR_MERGE_TARGET_IS_DESCENDANT_OF_BASELINE  );
		}
		else
		{
			SG_ERR_RETHROW;
		}
	}
	else
	{
#if defined(DEBUG)
		SG_uint32 nrLCA, nrSPCA, nrLeaves;
		SG_ERR_CHECK(  SG_daglca__get_stats(pCtx,pMrg->pDagLca,&nrLCA,&nrSPCA,&nrLeaves)  );
		SG_ASSERT(  nrLCA == 1  );		// for a simple cset-vs-baseline, we must have 1 LCA and 2 Leaves.
		SG_ASSERT(  nrLeaves == 2  );	// the number of SPCAs is a different matter and handled elsewhere.
#endif
	}

fail:
	SG_RBTREE_NULLFREE(pCtx, prb_temp);
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
 * WRT the DAGLCA, we don't actually need to compute it.  We are
 * going to seriously lie to MERGE and trick it into doing an UPDATE
 * for us.
 *
 */
void sg_wc_tx__fake_merge__update__compute_lca(SG_context * pCtx, SG_mrg * pMrg)
{
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "FakeMergeUpdate: Skipping LCA(%s, %s)....\n",
							   pMrg->pszHid_StartingBaseline,
							   pMrg->pszHidTarget)  );
#else
	SG_UNUSED( pCtx );
	SG_UNUSED( pMrg );
#endif
}

void sg_wc_tx__fake_merge__revert_all__compute_lca(SG_context * pCtx, SG_mrg * pMrg)
{
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "FakeMergeRevertAll: Skipping LCA(%s, %s)....\n",
							   pMrg->pszHid_StartingBaseline,
							   pMrg->pszHidTarget)  );
#else
	SG_UNUSED( pCtx );
	SG_UNUSED( pMrg );
#endif
}
