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
 * @file sg_wc__merge.c
 *
 * @details MERGE a changeset with the current changeset
 * reflected in the WC.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

/**
 * ***TRY** to do a REGULAR MERGE -- alter the contents of
 * the WC to relect the merger of the baseline changeset
 * and the requested changeset.
 *
 * We completely manage the TX.  If successful, we return
 * all the results that the API caller expects.  (I say this
 * because if we detect that a ff-merge is required, we just
 * throw an error and discard the TX because the caller will
 * need to set up a different one with different args.)
 *
 * The guts-proper of a REGULAR MERGE is now in layer-6 in
 * wc6merge and can be completely contained within this TX.
 * However, I've removed the layer-7 API for MERGE because
 * a ff-merge cannot be done in layer-7 and we don't want
 * to force every caller to have to deal with this.
 *
 */
void _try_regular_merge(SG_context * pCtx,
						const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						const SG_wc_merge_args * pMergeArgs,
						SG_bool bTest,		// --test is a layer-8-only flag.
						SG_varray ** ppvaJournal,
						SG_vhash ** ppvhStats,
						char ** ppszHidTarget,
						SG_varray ** ppvaStatus)
{
	// ppvaJournal is optional
	// ppvhStats is optional
	// ppszHidTarget is optional
	// ppvaStatus is optional

	SG_wc_tx * pWcTx = NULL;
	SG_varray * pvaJournalAllocated = NULL;
	SG_vhash * pvhStatsAllocated = NULL;
	SG_varray * pvaStatusAllocated = NULL;
	char * pszHidTarget = NULL;
		
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_FALSE)  );

	SG_ERR_CHECK(  sg_wc_tx__merge__main(pCtx, pWcTx, pMergeArgs, &pvhStatsAllocated,
										 ((ppszHidTarget) ? &pszHidTarget : NULL))  );

	if (ppvaJournal)
		SG_ERR_CHECK(  SG_WC_TX__JOURNAL__ALLOC_COPY(pCtx, pWcTx, &pvaJournalAllocated)  );

	if (ppvaStatus)
		SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pWcTx,
										NULL, SG_INT32_MAX,
										SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE,
										&pvaStatusAllocated,
										NULL)  );	// TODO 2012/06/26 what about legend?

	if (bTest)
		SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	else
		SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );

	SG_ERR_CHECK(  SG_wc_tx__free(pCtx, pWcTx)  );

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
		*ppszHidTarget = pszHidTarget;
		pszHidTarget = NULL;
	}

	if (ppvaStatus)
	{
		*ppvaStatus = pvaStatusAllocated;
		pvaStatusAllocated = NULL;
	}

	return;

fail:
	if (pWcTx)
	{
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
		SG_WC_TX__NULLFREE(pCtx, pWcTx);
	}
	SG_VARRAY_NULLFREE(pCtx, pvaJournalAllocated);
	SG_VHASH_NULLFREE(pCtx, pvhStatsAllocated);
	SG_NULLFREE(pCtx, pszHidTarget);
	SG_VARRAY_NULLFREE(pCtx, pvaStatusAllocated);
}

//////////////////////////////////////////////////////////////////

/**
 * Try to do a REGULAR and optionally fall-back to a FAST-FORWARD merge. 
 *
 */
void SG_wc__merge(SG_context * pCtx,
				  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				  const SG_wc_merge_args * pMergeArgs,
				  SG_bool bTest,		// --test is a layer-8-only flag.
				  SG_varray ** ppvaJournal,
				  SG_vhash ** ppvhStats,
				  char ** ppszHidTarget,
				  SG_varray ** ppvaStatus)
{
	// ppvaJournal is optional
	// ppvhStats is optional
	// ppszHidTarget is optional
	// ppvaStatus is optional

	_try_regular_merge(pCtx, pPathWc, pMergeArgs, bTest,
					   ppvaJournal, ppvhStats, ppszHidTarget, ppvaStatus);

	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_MERGE_TARGET_IS_DESCENDANT_OF_BASELINE))
		{
			if (!pMergeArgs->bNoFFMerge)
			{
				SG_context__err_reset(pCtx);
				SG_ERR_CHECK_RETURN(  sg_wc_tx__ffmerge__main(pCtx, pPathWc, pMergeArgs, bTest,
															  ppvaJournal, ppvhStats, ppszHidTarget, ppvaStatus)  );
			}
			else
			{
				SG_ERR_RETHROW_RETURN;
			}
		}
		else
		{
			SG_ERR_RETHROW_RETURN;
		}
	}
}

/**
 * Use this routine to compute the goal/target/peer cset
 * should be for a merge using the given WD state and the
 * set of selector args in pMergeArgs IN THOSE CASES WHERE
 * you don't actually want any of the test/stats/status/journal
 * info.
 *
 */
void SG_wc__merge__compute_preview_target(SG_context * pCtx,
										  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
										  const SG_wc_merge_args * pMergeArgs,
										  char ** ppszHidTarget)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  sg_wc_tx__merge__compute_preview_target(pCtx, pWcTx, pMergeArgs, ppszHidTarget)  )  );
}
