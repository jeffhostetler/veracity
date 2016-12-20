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
 * @file sg_wc_tx__merge__alloc.c
 *
 * @details Deal with alloc/free of SG_mrg.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_mrg__free(SG_context * pCtx, SG_mrg * pMrg)
{
	if (!pMrg)
		return;

	//////////////////////////////////////////////////////////////////

	pMrg->pMergeArgs = NULL;			// we do not own this
	pMrg->pWcTx = NULL;					// we do not own this

	//////////////////////////////////////////////////////////////////

	SG_VHASH_NULLFREE(pCtx, pMrg->pvhPile);
	SG_NULLFREE(pCtx, pMrg->pszHid_StartingBaseline);
	SG_NULLFREE(pCtx, pMrg->pszBranchName_Starting);
	SG_NULLFREE(pCtx, pMrg->pszHidTarget);

	//////////////////////////////////////////////////////////////////

	SG_DAGLCA_NULLFREE(pCtx,pMrg->pDagLca);
	SG_MRG_CSET_NULLFREE(pCtx, pMrg->pMrgCSet_LCA);
	SG_MRG_CSET_NULLFREE(pCtx, pMrg->pMrgCSet_Baseline);
	SG_MRG_CSET_NULLFREE(pCtx, pMrg->pMrgCSet_Other);
	SG_MRG_CSET_NULLFREE(pCtx,pMrg->pMrgCSet_FinalResult);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx,pMrg->prbCSets_Aux,(SG_free_callback *)SG_mrg_cset__free);

	SG_VECTOR_I64_NULLFREE(pCtx, pMrg->pVecRevertAllKillList);

	SG_NULLFREE(pCtx, pMrg);
}

//////////////////////////////////////////////////////////////////

void SG_mrg__alloc(SG_context * pCtx,
				   SG_wc_tx * pWcTx,
				   const SG_wc_merge_args * pMergeArgs,
				   SG_mrg ** ppMrg_New)
{
	SG_mrg * pMrg = NULL;

	SG_NULLARGCHECK_RETURN(ppMrg_New);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx,pMrg)  );
	pMrg->pWcTx = pWcTx;			// we do not own this
	pMrg->pMergeArgs = pMergeArgs;	// we do not own this

	// Defer allocating pMrg->prbCSets_Aux until needed

	*ppMrg_New = pMrg;
}


