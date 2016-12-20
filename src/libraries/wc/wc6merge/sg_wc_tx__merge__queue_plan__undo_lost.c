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
 * We have a LOST item from the WC.  Restore it.
 * Ideally we should just put it back as it was
 * in the baseline and then let __queue_plan__peer
 * alter it, but that is more trouble than it's worth.
 * So we try to restore and alter it at the same time.
 * This is mainly to handle moves/renames with transient
 * collisions.
 *
 * This is a little different from a __undo_delete
 * because LOST state is a little different from DELETED
 * state.
 *
 */
void sg_wc_tx__merge__queue_plan__undo_lost(SG_context * pCtx,
											SG_mrg * pMrg,
											const char * pszGid,
											SG_mrg_cset_entry * pMrgCSetEntry_FinalResult)
{
	SG_string * pStringRepoPath_Src = NULL;
	SG_string * pStringRepoPath_DestDir = NULL;
	SG_string * pString_Prefix = NULL;
	SG_string * pString_ParkedEntryname = NULL;
	SG_uint32 k = 0;
	SG_wc_undo_delete_args undo_delete_args;

	memset(&undo_delete_args, 0, sizeof(undo_delete_args));
	undo_delete_args.attrbits = pMrgCSetEntry_FinalResult->attrBits;
	if (pMrgCSetEntry_FinalResult->bufHid_Blob[0])
		undo_delete_args.pszHidBlob = pMrgCSetEntry_FinalResult->bufHid_Blob;

	// Create a gid-domain repo-path for the item; this is just to identify the item.
	// we don't care where it was in the tree in the baseline.
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStringRepoPath_Src, "@%s", pszGid)  );

	// Create a gid-domain repo-path for the parent destination directory.
	// Normally this is the same as the baseline, unless the item was moved
	// in the other CSET.
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStringRepoPath_DestDir, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Parent)  );

	// Use level-7 tx api to *try* to insert it in (<repo-path-parent>, <entryname>).
	// This will throw if we hit a transient collision.  (The MERGE conflict/collision
	// detection code should have already dealt with hard problems (so for example,
	// <entryname> may already look like "foo~<gid7>", but there may be transient
	// issues.
	
	SG_wc_tx__undo_lost(pCtx, pMrg->pWcTx,
						SG_string__sz(pStringRepoPath_Src),
						SG_string__sz(pStringRepoPath_DestDir),
						SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname),
						&undo_delete_args,
						SG_WC_STATUS_FLAGS__ZERO);
	if ( ! SG_CONTEXT__HAS_ERR(pCtx))
		goto done_ok;

	if ( ! SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS))
		SG_ERR_RETHROW;

	SG_context__err_reset(pCtx);

	// There is a *transient* collision with the destination.
	// Create it in a "parked" state.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_Prefix)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pString_Prefix, ".park.")  );
	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pString_Prefix,
											  (const SG_byte *)pMrgCSetEntry_FinalResult->bufGid_Entry, 7)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_ParkedEntryname)  );

	while (1)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString_ParkedEntryname, "%s.%02d",
										  SG_string__sz(pString_Prefix), k++)  );

		SG_wc_tx__undo_lost(pCtx, pMrg->pWcTx,
							SG_string__sz(pStringRepoPath_Src),
							"@/",
							SG_string__sz(pString_ParkedEntryname),
							&undo_delete_args,
							SG_WC_STATUS_FLAGS__ZERO);
		if ( ! SG_CONTEXT__HAS_ERR(pCtx))
			goto done_parked;

		if ( ! SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS))
			SG_ERR_RETHROW;
			
		SG_context__err_reset(pCtx);
	}

done_parked:
	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__PARKED;

done_ok:
	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;
	pMrg->nrOverrideLost++;

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir);
	SG_STRING_NULLFREE(pCtx, pString_Prefix);
	SG_STRING_NULLFREE(pCtx, pString_ParkedEntryname);
}
