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
 * @file sg_wc_tx__commit__apply.c
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

void sg_wc_tx__commit__apply(SG_context * pCtx,
							 sg_commit_data * pCommitData)
{
	const char * pszHidSuperRoot = NULL;	// we do not own this
	SG_uint32 k;

	// begin creating the CSET.

	SG_ERR_CHECK(  SG_committing__alloc(pCtx,
										&pCommitData->pWcTx->pCommittingInProgress,
										pCommitData->pWcTx->pDb->pRepo,
										SG_DAGNUM__VERSION_CONTROL,
										&pCommitData->auditRecord,
										SG_CSET_VERSION__CURRENT)  );
	for (k=0; k<pCommitData->nrParents; k++)
	{
		const char * pszHidParent_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx,
										  pCommitData->pvaParents,
										  k,
										  &pszHidParent_k)  );
		SG_ERR_CHECK(  SG_committing__add_parent(pCtx,
												 pCommitData->pWcTx->pCommittingInProgress,
												 pszHidParent_k)  );
	}

	SG_ERR_CHECK(  SG_committing__tree__set_root(pCtx,
												 pCommitData->pWcTx->pCommittingInProgress,
												 pCommitData->pWcTx->pszHid_superroot_content_commit)  );

	// Apply the journaled stuff.
	//
	// The journal contains the following types of commands:
	// 
	// [1] store content blobs for all new/modified (participating)
	//     files and symlinks.
	//
	// [1a] add details to pCommittingInProgress (and the new
	//      changeset) for each item in [1].
	// 
	// [2] store new/modified TREENODE content blobs for participating
	//     *and/or* bubble-up directories.
	//
	// [2a] add details to pCommittingInProgress (and the new
	//      changeset) for each item in [2].
	//
	// [3] database commands to update the tne_L0 and pc_L0 tables
	//     in wc.db to reflect their future (non-dirty or less-dirty)
	//     state.
	//
	// note that we have pre-verified everything, so
	// we should not get any errors here -- unless
	// there's a disk space issue or something like
	// that.
	//
	// if we do take a hard error here and we abort
	// the cset, we may have a few unreferenced blobs
	// in the repo, but that is ok:
	// () parts [1] and [2] are read-only from the
	//    point of view of the WD.
	// () part [3] is covered by the TX that we will abort.
	//
	// We use the internal version of APPLY that does not
	// auto-commit (in the sql sense) because we need to
	// update the tbl_csets table with the new HID of the
	// new baseline in the same TX but we won't know what
	// HID value is until after SG_committing__end().

	SG_ERR_CHECK(  sg_wc_tx__run_apply(pCtx, pCommitData->pWcTx)  );

	// write the actual changeset and dagnode.  this computes the resulting
	// changeset's HID.  this eats any duplicate changeset/dagnode warning
	// silently continues if the changeset/dagnode already exists in the
	// repo and returns handles to the existing ones.
	//
	// this is an old-style call that frees the pCommitting upon success.

	SG_ERR_CHECK(  SG_committing__end(pCtx,
									  pCommitData->pWcTx->pCommittingInProgress,
									  &pCommitData->pChangeset_VC,
									  &pCommitData->pDagnode_VC)  );
	pCommitData->pWcTx->pCommittingInProgress = NULL;
	SG_ERR_CHECK(  SG_dagnode__get_id(pCtx,
									  pCommitData->pDagnode_VC,
									  &pCommitData->pszHidChangeset_VC)  );
	
	if (pCommitData->nrParents > 1)
	{
		// If there was a MERGE, do non-queued (but still in-TX) DELETE
		// of the CSet info we had for the other parents/ancestor/etc.

		SG_ERR_CHECK(  sg_wc_tx__postmerge__commit_cleanup(pCtx, pCommitData->pWcTx)  );
	}

	// The new VC changeset is written to the repo and we know its HID.
	// The (hidden) tx within pCommitting is completed.  But this new
	// changeset is still unreferenced by the WD (and our pWcTx TX is
	// still open).
	//
	// Do a non-queued (but still in-TX) INSERT/REPLACE to point
	// tne_L0 at the new changeset.
	//
	// This updates the HID stored in the row in tbl_csets; it DOES NOT
	// change the mappings to which "tne" and "pc" tables are referenced.

	SG_ERR_CHECK(  SG_changeset__tree__get_root(pCtx,
												pCommitData->pChangeset_VC,
												&pszHidSuperRoot)  );
	SG_ERR_CHECK(  sg_wc_db__csets__update__cset_hid(pCtx, pCommitData->pWcTx->pDb, "L0",
													 pCommitData->pszHidChangeset_VC,
													 pszHidSuperRoot)  );
	// Since we just changed the contents of the row in tbl_csets, we
	// should re-fetch it and refresh pWcTx->pCSetRow_Baseline.  Granted
	// nothing probably follows this in the TX, but it is better to have
	// a consistent view.
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCommitData->pWcTx->pCSetRow_Baseline);
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pCommitData->pWcTx->pDb, "L0",
													 NULL, &pCommitData->pWcTx->pCSetRow_Baseline)  );

	// We return with the TX still open.
	// As far as we are concerned everything
	// is ready for the sql-commit on wc.db.

fail:
	return;
}

