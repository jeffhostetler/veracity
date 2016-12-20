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
 * To be called after we have computed the in-memory merge and
 * have the MERGE-GOAL in pMrgCSet_FinalResult.
 *
 * Our job is to figure out the sequence of disk operations
 * required to convert the current WD into the FinalResult.
 *
 * We use the in-progress TX to queue/journal the steps
 * required (and to track the transient repo-paths).
 *
 */
void sg_wc_tx__merge__queue_plan(SG_context * pCtx,
								 SG_mrg * pMrg)
{
	// set the markers to 0 on all of the entries in the baseline
	// and final-result csets so that we know which ones we have
	// visited and which ones are peerless and the transient status
	// as we build the plan.

	SG_ERR_CHECK(  SG_mrg_cset__set_all_markers(pCtx,pMrg->pMrgCSet_FinalResult, _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );
	SG_ERR_CHECK(  SG_mrg_cset__set_all_markers(pCtx,pMrg->pMrgCSet_Baseline,    _SG_MRG__PLAN__MARKER__FLAGS__ZERO)  );

	// for each entry in the final-result, look at it and the
	// corresponding entry in the baseline and decide what needs
	// to be done to the WD and the pendingtree.  add this to the
	// plan.

	SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__entry(pCtx, pMrg)  );

	// queue the insert of ISSUE/RESOLVE data for each item with
	// a conflict of some kind.  we need to do this after we have
	// dealt with the __queue_plan__entry() so that the LVI Cache
	// is fully populated (especialy for __add_special items).

	SG_ERR_CHECK(  sg_wc_tx__merge__prepare_issues(pCtx, pMrg)  );

	// delete anything that was in the baseline and
	// not in the final-result.

	SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__delete(pCtx, pMrg)  );

	// run thru the final-result one more time and un-park
	// anything that had to be parked because of *transient*
	// collisions (with items that were going to be moved/
	// renamed/deleted but hadn't been processed yet).

	SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__unpark(pCtx, pMrg)  );

	// If REVERT-ALL added items to the kill list, remove them from
	// SQL now.

	SG_ERR_CHECK(  sg_wc_tx__merge__queue_plan__kill_list(pCtx, pMrg)  );

fail:
	return;
}
