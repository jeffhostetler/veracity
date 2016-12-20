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
 * We trick the MERGE code to do our homework.  It works with
 * the classic:
 *      A
 *     / \
 *    B   C
 *     \ /
 *      M
 *
 * But in our case we have the virgin baseline and the current
 * working directory (baseline+dirt).  (If there is a pending merge
 * we will also have an "other" cset, but that doesn't matter.)
 * Revert's job is to clean the dirt and make the WD look like
 * the baseline.
 *
 * So, using the 2-vs-1 rules of MERGE, we can do this:
 *
 *      WD
 *     /  \
 *   WD    B
 *     \  /
 *       B
 *
 * This may sound weird or over-kill, but the hard parts of REVERT
 * are not the fetching of original (unedited) content of the files
 * and optionally backing up the modified version; the hard part is
 * untangling the structural changes (moves/renames) in the proper order:
 * [] dealing with swap files when necessary,
 * [] detecting transient collisions among controlled items,
 * [] detecting hard collisions/conflicts between uncontrolled items
 *    present in the WD and items that need to be present after the revert.
 * These are things that MERGE already addresses.
 *
 * The following code must parallel the MERGE and UPDATE sequence of
 * events to ensure that everything gets set up correctly.
 * Note that there are some stemps here that are effectively NOOPs,
 * but I leave them here for referference.
 *
 */
void SG_wc_tx__revert_all(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  SG_bool bNoBackups,
						  SG_vhash ** ppvhStats)
{
	SG_mrg * pMrg = NULL;

	// ppvhStats is optional


	SG_ERR_CHECK(  SG_MRG__ALLOC(pCtx, pWcTx, NULL, &pMrg)  );
	pMrg->eFakeMergeMode = SG_FAKE_MERGE_MODE__REVERT;
	pMrg->bRevertNoBackups = bNoBackups;

	SG_ERR_CHECK(  sg_wc_tx__fake_merge__revert_all__get_starting_conditions(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__fake_merge__revert_all__determine_target_cset(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__fake_merge__revert_all__compute_lca(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__fake_merge__revert_all__load_csets(pCtx, pMrg)  );

	SG_ERR_CHECK(  sg_wc_tx__merge__compute_simple2(pCtx, pMrg)  );
	SG_ERR_CHECK(  SG_mrg_cset__compute_cset_dir_list(pCtx,pMrg->pMrgCSet_FinalResult)  );
	SG_ERR_CHECK(  SG_mrg_cset__check_dirs_for_collisions(pCtx, pMrg->pMrgCSet_FinalResult, pMrg)  );
	SG_ERR_CHECK(  SG_mrg_cset__make_unique_entrynames(pCtx,pMrg->pMrgCSet_FinalResult)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dir_list_as_hierarchy(pCtx, "FakeMergeRevert",
																	   pMrg->pMrgCSet_FinalResult)  );
#endif

	// TODO 2012/08/01 In theory, we SHOULD NOT have any files that need auto-merge
	// TODO            (because A & B are identical so C should always win without
	// TODO            the need for auto-merge).  This call is here to maintain the
	// TODO            parallel flow like MERGE and UPDATE.  We should assert that
	// TODO            we didn't have any.
	SG_ERR_CHECK(  SG_mrg__automerge_files(pCtx, pMrg, pMrg->pMrgCSet_FinalResult)  );

	SG_ERR_CHECK(  sg_wc_tx__merge__prepare_issue_table(pCtx, pMrg)  );
	SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan(pCtx, pMrg)  );

	// TODO 2012/08/01 Likewise, we should not generate any new ISSUES.  We should
	// TODO            assert that too.

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  _sg_wc_tx__merge__trace_msg__dump_raw_stats(pCtx, pMrg)  );
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pMrg->pWcTx->pvaJournal, "RevertAll:Journal")  );
#endif

	if (ppvhStats)
		SG_ERR_CHECK(  sg_wc_tx__merge__make_stats(pCtx, pMrg, ppvhStats)  );

	// If we reverted an actual merge, we need to drop the tne_L1 table
	// and the A and L1 rows from tbl_csets so that we no longer look like
	// we are in a merge.

	SG_ERR_CHECK(  sg_wc_tx__fake_merge__revert_all__cleanup_db(pCtx, pMrg)  );

fail:
	SG_MRG_NULLFREE(pCtx, pMrg);
}
