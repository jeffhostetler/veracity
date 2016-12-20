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
 * @file sg_wc__update.c
 *
 * @details UPDATE the WC to a difference CSET.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Do an UPDATE -- alter the contents of the WD to reflect
 * a different CHANGESET.  In theory, the resulting WD (and
 * .sgdrawer) should look like a fresh checkout of the goal
 * changeset in this directory.
 *
 * We have (potentially) 3 views of the tree to deal with:
 * [1] The baseline.
 * [2] The changes between the baseline and the goal changeset.
 * [3] Any dirt/changes in the WD (relative to the baseline)
 *     we need to deal with.  WD-dirt falls into these
 *     categories:
 *     [3a] Controlled items that have been modified in
 *          the WD and should be "carried forward".
 *          [3a1] And not modified in the goal.
 *          [3a2] But also modified in the goal.
 *     [3b] Uncontrolled items (found/ignored) in the WD.
 * 
 * TODO To be continued....
 *
 * We do this in a level-8-api routine and DO NOT provide
 * a level-7-txapi routine.  This is because we need to
 * alter things outside of the DB during/after the update
 * so we need complete control of the TX.
 *
 * 
 * UPDATE deviates from the normal API model because it
 * must both do something *TO* the {tne,pc} and WD like
 * MERGE ***AND*** it must change the baseline like COMMIT
 * (but *without* recording anything in the repo like
 * COMMIT).  This is another reason that it is a level-8
 * API only.
 *
 * We OPTIONALLY return the JOURNAL, the merge-stats, and
 * a full status as of the end of the projected update.
 *
 */
void SG_wc__update(SG_context * pCtx,
				   const SG_pathname* pPathWc,
				   const SG_wc_update_args * pUpdateArgs,
				   SG_bool bTest,	// --test is a layer-8-only flag so don't put it in SG_wc_update_args.
				   SG_varray ** ppvaJournal,
				   SG_vhash ** ppvhStats,
				   SG_varray ** ppvaStatus,
				   char ** ppszHidChosen)
{
	sg_update_data updateData;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhStats = NULL;
	SG_varray * pvaStatus = NULL;
	SG_bool bDidNothing = SG_FALSE;
	SG_bool bWroteBranch = SG_FALSE;

	memset(&updateData, 0, sizeof(updateData));
	updateData.pUpdateArgs = pUpdateArgs;

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &updateData.pWcTx, pPathWc, SG_FALSE)  );

	SG_ERR_CHECK(  sg_wc_tx__update__determine_target_cset(pCtx, &updateData)  );

	SG_ERR_CHECK(  sg_wc_tx__update__fake_merge(pCtx, &updateData,
												((ppvhStats) ? &pvhStats : NULL),
												((ppvaStatus) ? &pvaStatus : NULL),
												&bDidNothing)  );

	// Attach or detach to/from the branch as requested.
	SG_ERR_CHECK(  sg_wc_tx__update__attach(pCtx, &updateData, &bWroteBranch)  );

	if (ppvaJournal)
		SG_ERR_CHECK(  SG_WC_TX__JOURNAL__ALLOC_COPY(pCtx, updateData.pWcTx, &pvaJournal)  );

	if (bTest)
	{
		SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, updateData.pWcTx)  );
	}
	else if (bDidNothing)
	{
		// UPDATE did not need to alter the WD, but we may have
		// set a new/different BRANCH and since the branch is now
		// stored within the wc.db, we have to commit it to get
		// that one field written.
		//
		// Otherwise, we don't need to alter the wc.db.
		//
		// In both cases, we DO commit the TSC.

		if (bWroteBranch)
			SG_ERR_CHECK(  sg_wc_db__tx__commit(pCtx, updateData.pWcTx->pDb)  );
		else
			SG_ERR_CHECK(  sg_wc_db__tx__rollback(pCtx, updateData.pWcTx->pDb)  );
	}
	else
	{
		// We can't just call SG_wc_tx__apply() because it both
		// *runs* the plan/journal *and* does a SQL-commit. we
		// need to slip in some things in some things between
		// those 2 steps.
		//
		// We need to run the plan/journal, do a change of basis
		// (dropping some tables), and then do the SQL-commit.

		SG_ERR_CHECK(  sg_wc_tx__update__apply(pCtx, &updateData)  );
		// bWroteBranch doesn't matter here since we're commiting anyway.
		SG_ERR_CHECK(  sg_wc_db__tx__commit(pCtx, updateData.pWcTx->pDb)  );
	}

	SG_WC_TX__NULLFREE(pCtx, updateData.pWcTx);

	if (ppszHidChosen)
	{
		*ppszHidChosen = updateData.pszHidChosen;
		updateData.pszHidChosen = NULL;
	}

	if (ppvaJournal)
	{
		*ppvaJournal = pvaJournal;
		pvaJournal = NULL;
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
	if (updateData.pWcTx)
	{
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, updateData.pWcTx)  );
		SG_WC_TX__NULLFREE(pCtx, updateData.pWcTx);
	}

	SG_VHASH_NULLFREE( pCtx, updateData.pvhPile);
	SG_VARRAY_NULLFREE(pCtx, updateData.pvaStatusVV2);
	SG_VARRAY_NULLFREE(pCtx, updateData.pvaStatusWC);
	SG_NULLFREE(       pCtx, updateData.pszHid_StartingBaseline);
	SG_NULLFREE(       pCtx, updateData.pszBranchName_Starting);
	SG_NULLFREE(       pCtx, updateData.pszHidTarget);
	SG_NULLFREE(       pCtx, updateData.pszBranchNameTarget);
	SG_NULLFREE(       pCtx, updateData.pszNormalizedAttach);
	SG_NULLFREE(       pCtx, updateData.pszNormalizedAttachNew);
	SG_REV_SPEC_NULLFREE(pCtx, updateData.pRevSpecChosen);
	SG_NULLFREE(       pCtx, updateData.pszHidChosen);
	SG_NULLFREE(       pCtx, updateData.pszAttachChosen);
	SG_VHASH_NULLFREE( pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}
