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
 * @file sg_wc_tx__update__fake_merge.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Setup for a one-legged MERGE using the guts of the normal MERGE engine.
 * The flow here must match, in spirit, the steps in SG_wc_tx__merge().
 *
 */
void sg_wc_tx__update__fake_merge(SG_context * pCtx,
								  sg_update_data * pUpdateData,
								  SG_vhash ** ppvhStats,
								  SG_varray ** ppvaStatus,
								  SG_bool * pbDidNothing)
{
	SG_wc_merge_args mergeArgs;
	SG_mrg * pMrg = NULL;
	SG_vhash * pvhStats = NULL;
	SG_varray * pvaStatus = NULL;

	memset(&mergeArgs, 0, sizeof(mergeArgs));
	mergeArgs.pRevSpec                   = pUpdateData->pRevSpecChosen;
	mergeArgs.bNoAutoMergeFiles          = pUpdateData->pUpdateArgs->bNoAutoMerge;
	mergeArgs.bComplainIfBaselineNotLeaf = SG_FALSE;
	mergeArgs.bDisallowWhenDirty         = pUpdateData->pUpdateArgs->bDisallowWhenDirty;

	SG_ERR_CHECK(  SG_MRG__ALLOC(pCtx, pUpdateData->pWcTx, &mergeArgs, &pMrg)  );
	pMrg->eFakeMergeMode = SG_FAKE_MERGE_MODE__UPDATE;
	pMrg->bDisallowWhenDirty = pUpdateData->pUpdateArgs->bDisallowWhenDirty;
	
	SG_ERR_CHECK(  sg_wc_tx__fake_merge__update__get_starting_conditions(pCtx, pMrg,
																		 pUpdateData->pszHid_StartingBaseline,
																		 pUpdateData->pszBranchName_Starting)  );
	SG_ERR_CHECK(  sg_wc_tx__fake_merge__update__determine_target_cset(pCtx, pMrg, pUpdateData->pszHidChosen)  );

	if (strcmp(pMrg->pszHidTarget, pMrg->pszHid_StartingBaseline) == 0)
	{
		*pbDidNothing = SG_TRUE;

		// The requested target of the UPDATE is the same as the baseline.
		// Nothing to do.  We check this at this layer (rather than in
		// SG_wc__update()) so that we can easily fake up the stats/journal
		// if the caller wanted them.
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "FakeMerge: nothing to do -- target==baseline: %s\n",
								   pMrg->pszHidTarget)  );
#endif
		if (ppvhStats)
			SG_ERR_CHECK(  sg_wc_tx__merge__make_stats(pCtx, pMrg, &pvhStats)  );
	}
	else
	{
		*pbDidNothing = SG_FALSE;

		SG_ERR_CHECK(  sg_wc_tx__fake_merge__update__compute_lca(pCtx, pMrg)  );
		SG_ERR_CHECK(  sg_wc_tx__fake_merge__update__load_csets(pCtx, pMrg)  );

		SG_ERR_CHECK(  sg_wc_tx__merge__compute_simple2(pCtx, pMrg)  );
		SG_ERR_CHECK(  SG_mrg_cset__compute_cset_dir_list(pCtx,pMrg->pMrgCSet_FinalResult)  );
		SG_ERR_CHECK(  SG_mrg_cset__check_dirs_for_collisions(pCtx, pMrg->pMrgCSet_FinalResult, pMrg)  );
		SG_ERR_CHECK(  SG_mrg_cset__make_unique_entrynames(pCtx,pMrg->pMrgCSet_FinalResult)  );

#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dir_list_as_hierarchy(pCtx, "FakeMergeUpdate",
																		   pMrg->pMrgCSet_FinalResult)  );
#endif

		SG_ERR_CHECK(  SG_mrg__automerge_files(pCtx, pMrg, pMrg->pMrgCSet_FinalResult)  );

		SG_ERR_CHECK(  sg_wc_tx__merge__prepare_issue_table(pCtx, pMrg)  );
		SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan(pCtx, pMrg)  );

#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dump_raw_stats(pCtx, pMrg)  );
		SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pMrg->pWcTx->pvaJournal, "Update:Journal")  );
#endif

		if (ppvhStats)
			SG_ERR_CHECK(  sg_wc_tx__merge__make_stats(pCtx, pMrg, &pvhStats)  );

		SG_ERR_CHECK(  sg_wc_tx__fake_merge__update__build_pc(pCtx, pMrg)  );

		if (ppvaStatus)
			SG_ERR_CHECK(  sg_wc_tx__fake_merge__update__status(pCtx, pMrg, &pvaStatus)  );

		// TODO 2012/05/14 write debug/test routine to verify that the net-net of
		// TODO            { tne_L0 + pc_L0 } and { tne_L1 + pc_L1 } are equivalent
		// TODO            before we do the "change of basis".
	}

	if (ppvhStats)
	{
		*ppvhStats = pvhStats;
		pvhStats = NULL;
	}

	if (ppvaStatus)
	{
		*ppvaStatus = pvaStatus;
		pvaStatus = NULL;
	}

fail:
	SG_MRG_NULLFREE(pCtx, pMrg);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}
