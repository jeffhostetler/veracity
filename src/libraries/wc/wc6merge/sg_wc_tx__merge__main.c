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
 * @file sg_wc_tx__merge__main.c
 *
 * @details Handle details of a REGULAR (non-ff) MERGE.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Do a MERGE -- alter the contents of the WC to relect
 * the merger of the baseline changeset and the requested
 * changeset (in pMergeArgs->pRevSpec).
 *
 * This form of MERGE works with:
 *      [] ancestor cset,
 *      [] baseline cset + wd dirt,
 *      [] other cset.
 *
 *
 * TODO 2012/01/17 We allow them to specify the target changeset
 * TODO            to merge into the current baseline and if they
 * TODO            use a '--branch' argument we will make sure
 * TODO            that the target is a head, but otherwise we
 * TODO            don't.
 * TODO
 * TODO            Should we have an argument to allow/disallow
 * TODO            the target if it is not a head ?
 * TODO
 * TODO            Likewise, should we have an argument to allow/
 * TODO            disallow if the baseline is not a head (meaning
 * TODO            that they should maybe UPDATE then MERGE) ?
 * TODO
 * TODO            See W1163.
 * TODO
 * TODO            Furthermore, how would these 'require branch head'
 * TODO            options interact with the 'require baseline leaf'
 * TODO            option?
 *
 *
 * As a convenience we also optionally return the target/peer cset hid
 * since we have it on hand in pMrg.  At the level-7 layer, we don't
 * really need to do this since the caller holding the TX can just
 * call one of the parent() functions and get it.  I added it here
 * to help the layer-8 routines (since they have a --test option and
 * throw away intermediate results).
 * 
 */ 
void sg_wc_tx__merge__main(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const SG_wc_merge_args * pMergeArgs,
						   SG_vhash ** ppvhStats,
						   char ** ppszHidTarget)
{
	SG_mrg * pMrg = NULL;

	SG_NULLARGCHECK_RETURN( pMergeArgs );
	// ppvhStats is optional
	// ppszHidTarget is optional

	SG_ERR_CHECK(  SG_MRG__ALLOC(pCtx, pWcTx, pMergeArgs, &pMrg)  );
	pMrg->eFakeMergeMode = SG_FAKE_MERGE_MODE__MERGE;
	pMrg->bDisallowWhenDirty = pMergeArgs->bDisallowWhenDirty;
	
	SG_ERR_CHECK(  sg_wc_tx__merge__get_starting_conditions(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__merge__determine_target_cset(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__merge__compute_lca(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__merge__load_csets(pCtx, pMrg)  );

	SG_ERR_CHECK(  sg_wc_tx__merge__compute_simple2(pCtx, pMrg)  );
	SG_ERR_CHECK(  SG_mrg_cset__compute_cset_dir_list(pCtx,pMrg->pMrgCSet_FinalResult)  );
	SG_ERR_CHECK(  SG_mrg_cset__check_dirs_for_collisions(pCtx, pMrg->pMrgCSet_FinalResult, pMrg)  );
	SG_ERR_CHECK(  SG_mrg_cset__make_unique_entrynames(pCtx,pMrg->pMrgCSet_FinalResult)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dir_list_as_hierarchy(pCtx, "merge",
																	   pMrg->pMrgCSet_FinalResult)  );
#endif

	SG_ERR_CHECK(  SG_mrg__automerge_files(pCtx, pMrg, pMrg->pMrgCSet_FinalResult)  );

	SG_ERR_CHECK(  sg_wc_tx__merge__prepare_issue_table(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan(pCtx, pMrg)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dump_raw_stats(pCtx, pMrg)  );
#endif
	if (ppvhStats)
		SG_ERR_CHECK(  sg_wc_tx__merge__make_stats(pCtx, pMrg, ppvhStats)  );

	if (ppszHidTarget)
	{
		*ppszHidTarget = pMrg->pszHidTarget;
		pMrg->pszHidTarget = NULL;
	}

fail:
	SG_MRG_NULLFREE(pCtx, pMrg);
}

//////////////////////////////////////////////////////////////////

/**
 * Convenience routine to parallel the computation made in SG_wc_tx__merge__main()
 * to determine the OTHER/TARGET CSET that should be merged with the current
 * baseline **ASSUMING** it is given the same set of MERGE-ARGS.
 *
 * This will use the current baseline and/or current branch and determine
 * the single target -- either named explicitly with a --rev/--tag or
 * unique --branch or sniff the set of branch heads for a unique peer
 * that we should use.
 *
 * This will THROW for all the same reasons that SG_wc_tx__merge() does.
 * Such as if a unique peer/target cannot be found or if the WD is already
 * has a merge.
 *
 * This routine is ***only*** defined for a REAL-MERGE and not one of
 * the UPDATE/REVERT FAKE-MERGES.
 *
 */
void sg_wc_tx__merge__compute_preview_target(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 const SG_wc_merge_args * pMergeArgs,
											 char ** ppszHidTarget)
{
	SG_mrg * pMrg = NULL;

	SG_NULLARGCHECK_RETURN( pMergeArgs );
	SG_NULLARGCHECK_RETURN( ppszHidTarget );

	SG_ERR_CHECK(  SG_MRG__ALLOC(pCtx, pWcTx, pMergeArgs, &pMrg)  );
	pMrg->eFakeMergeMode = SG_FAKE_MERGE_MODE__MERGE;
	
	SG_ERR_CHECK(  sg_wc_tx__merge__get_starting_conditions(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__merge__determine_target_cset(pCtx, pMrg)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "MergePreviewTarget: %s\n", pMrg->pszHidTarget)  );
#endif

	// steal the fields we want from pMrg before we destroy it
	// rather than cloning them.

	*ppszHidTarget = pMrg->pszHidTarget;
	pMrg->pszHidTarget = NULL;

fail:
	SG_MRG_NULLFREE(pCtx, pMrg);
}
												
