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
 * As COMMIT is finishing, we need to delete/drop
 * various info/tables from the WC.DB that MERGE
 * created to store intermediate results so that
 * it no longer looks like we are in a merge.
 *
 * THIS CODE IS TECHNICALLY PART OF wc6merge BUT
 * IT DOES NOT RUN DURING THE MERGE.  IT RUNS
 * MUCH LATER DURING COMMIT.  SO IT DOES NOT HAVE
 * ACCESS TO A pMrg.
 * 
 */
void sg_wc_tx__postmerge__commit_cleanup(SG_context * pCtx,
										 SG_wc_tx * pWcTx)
{
	SG_vector * pvecList = NULL;
	SG_uint32 k, kLimit;

	// Get a list of all of the involved CSETs.
	// Then deal with each.  We have to do this
	// in 2 parts because we can't DROP tables
	// inside a SELECT loop.
	SG_ERR_CHECK(  sg_wc_db__csets__get_all(pCtx, pWcTx->pDb, &pvecList)  );

	SG_ERR_CHECK(  SG_vector__length(pCtx, pvecList, &kLimit)  );
	for (k=0; k<kLimit; k++)
	{
		sg_wc_db__cset_row * pCSetRow_k;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pvecList, k, (void **)&pCSetRow_k)  );

		// "L0" is special and we must not delete/drop it.

		if (strcmp(pCSetRow_k->psz_label, "L0") == 0)
			continue;

		// non-L0 items were used to help describe the pending
		// merge.  these all need to go away.
		// 
		// [2] Drop the tne_X and if present the pc_X table.
		SG_ERR_CHECK(  sg_wc_db__tne__drop_named_table(pCtx, pWcTx->pDb, pCSetRow_k)  );
		SG_ERR_CHECK(  sg_wc_db__pc__drop_named_table(pCtx, pWcTx->pDb, pCSetRow_k)  );

		// [3] Delete the X row from tbl_csets.
		SG_ERR_CHECK(  sg_wc_db__csets__delete_row(pCtx, pWcTx->pDb, pCSetRow_k->psz_label)  );

		// [4] This may not strictly be necessary, but consistency
		//     is important.  When we delete the "L1" row from the
		//     tbl_csets table, we should also get rid of our cached
		//     copy of it.
		if (pWcTx->pCSetRow_Other && (strcmp(pCSetRow_k->psz_label, pWcTx->pCSetRow_Other->psz_label) == 0))
			SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pWcTx->pCSetRow_Other);
	}

fail:
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pvecList, (SG_free_callback *)sg_wc_db__cset_row__free);
}

										  
