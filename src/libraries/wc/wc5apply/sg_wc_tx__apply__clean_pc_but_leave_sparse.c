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
 * @file sg_wc_tx__apply__clean_pc_but_leave_sparse.c
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
 * Delete the changes in a PC ROW but leave it marked sparse
 * in the tbl_PC table in the wc.db.
 * 
 * The existing row (if it exists) in the tbl_PC
 * indicates that this item has a structural change
 * (add/delete/move/rename/etc) and describes how the
 * item has changed since the baseline (stored in tne_L0).
 * 
 * During a COMMIT, the caller is saying that all of
 * the changes have been accounted for in the future
 * changeset and this item should appear as unchanged
 * relative to the new baseline.  BUT the item is sparse
 * (not populated in the WD) and we need to continue to
 * remember that.
 *
 * So we want the PC ROW to be updated to clear the
 * various dirty bits, but keep the sparse bit as we
 * transition the wc.db.
 *
 */
void sg_wc_tx__apply__clean_pc_but_leave_sparse(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												const SG_vhash * pvh)
{
#if TRACE_WC_TX_APPLY
	const char * pszRepoPath;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh, "src", &pszRepoPath)  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__clean_pc_but_leave_sparse: '%s'\n"),
							   pszRepoPath)  );
#else
	SG_UNUSED( pCtx );
	SG_UNUSED( pvh );
#endif

	SG_UNUSED( pWcTx );

	// we don't actually have anything here.
	// the journal record was more for the verbose log.
	// the actual work of updating the SQL will be done
	// in the parallel journal-stmt.

}
