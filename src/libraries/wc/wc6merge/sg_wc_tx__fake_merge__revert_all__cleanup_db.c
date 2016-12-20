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
 * The (QUEUE phase of the) REVERT-ALL was successful.  If the WD
 * was in a merge prior to the revert, we need to alter the wc.db
 * so that it no longer looks like we are.  That is, make it appear
 * as the current baseline is the ONLY parent.
 *
 * (This code is for REVERT-ALL only and doesn't really have a peer
 * in MERGE-proper or UPDATE.  The closest peer is in the post-COMMIT
 * code -- see sg_wc_tx__commit__apply().)
 *
 */
void sg_wc_tx__fake_merge__revert_all__cleanup_db(SG_context * pCtx,
												  SG_mrg * pMrg)
{
	// DROP the tne_L1 table and the A and L1 rows from tbl_csets.
	SG_ERR_CHECK(  sg_wc_tx__postmerge__commit_cleanup(pCtx, pMrg->pWcTx)  );

	// We do not need to delete any auto-merge temp files here.
	// The code to queue/apply the individual delete-issue operations
	// takes care of that.

fail:
	return;
}

