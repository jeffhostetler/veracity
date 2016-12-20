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

void sg_wc_tx__update__apply(SG_context * pCtx,
							 sg_update_data * pUpdateData)
{
	// We use the internal version of APPLY that does not auto-commit
	// (in the SQL sense) because we need to squeeze in a few things.
	//
	// This will run the plan and change the pc_L0 table and change
	// the contents of the WD to match the result of the merge/update.

	SG_ERR_CHECK(  sg_wc_tx__run_apply(pCtx, pUpdateData->pWcTx)  );

	//////////////////////////////////////////////////////////////////
	// Now do the "change of basis".  because of the DROP TABLES, we
	// needed to wait until all of the prepared statements in the
	// QUEUE were processed.
	//////////////////////////////////////////////////////////////////

	// drop the SQL tables associated with the old baseline { tne_L0, pc_L0 } tables.

	SG_ERR_CHECK(  sg_wc_db__tne__drop_named_table(pCtx, pUpdateData->pWcTx->pDb, pUpdateData->pWcTx->pCSetRow_Baseline)  );
	SG_ERR_CHECK(  sg_wc_db__pc__drop_named_table(pCtx,  pUpdateData->pWcTx->pDb, pUpdateData->pWcTx->pCSetRow_Baseline)  );
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pUpdateData->pWcTx->pCSetRow_Baseline);

	// steal pWcTx->pCSetRow_Other and convert it in-place.  This contents
	// of this should match the data currently in row "L1" in tbl_csets.
	// re-write "L0" row in tbl_csets to refer to the new baseline { tne_L1, pc_L1 } tables.
	// delete "L1" row from tbl_csets.

	pUpdateData->pWcTx->pCSetRow_Baseline = pUpdateData->pWcTx->pCSetRow_Other;
	pUpdateData->pWcTx->pCSetRow_Other = NULL;
	pUpdateData->pWcTx->pCSetRow_Baseline->psz_label[1] = '0';	// change "L1" to "L0" in-place

	SG_ERR_CHECK(  sg_wc_db__csets__update__replace_all(pCtx, pUpdateData->pWcTx->pDb, "L0",
														pUpdateData->pWcTx->pCSetRow_Baseline->psz_hid_cset,
														pUpdateData->pWcTx->pCSetRow_Baseline->psz_tne_table_name,
														pUpdateData->pWcTx->pCSetRow_Baseline->psz_pc_table_name,
														pUpdateData->pWcTx->pCSetRow_Baseline->psz_hid_super_root)  );
	SG_ERR_CHECK(  sg_wc_db__csets__delete_row(pCtx, pUpdateData->pWcTx->pDb, "L1")  );

	// delete the "A" row from tbl_csets.

	SG_ERR_CHECK(  sg_wc_db__csets__delete_row(pCtx, pUpdateData->pWcTx->pDb, "A")  );

fail:
	return;
}

