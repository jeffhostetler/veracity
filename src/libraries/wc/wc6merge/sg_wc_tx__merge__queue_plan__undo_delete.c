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
 * The item has a pending delete in the WC.
 * It was present in the baseline, but has
 * been deleted from the WC.  The other cset
 * has made changes to it (relative to the
 * ancestor) and we want to override/cancel
 * the delete.
 *
 * This is very similar to __add_special(),
 * but the delete was done in the WC rather
 * than in the baseline-proper.
 *
 * TODO 2012/03/12 I'm going to claim that we
 * TODO            DO NOT restore the baseline
 * TODO            version of the file and then
 * TODO            auto-merge it with the other
 * TODO            cset.
 * TODO
 * TODO            I'm going to claim that if you
 * TODO            delete it in your WC and do a
 * TODO            dirty merge that we try to
 * TODO            behave as if you had committed
 * TODO            the WC first.  If you had, the
 * TODO            ADD-SPECIAL code would not do
 * TODO            an auto-merge.
 *
 */
void sg_wc_tx__merge__queue_plan__undo_delete(SG_context * pCtx,
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
	SG_bool bIsPortFlags = SG_FALSE;
	SG_bool bIsRevertCollision = SG_FALSE;

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
	// This will throw if we hit a transient collision.  The MERGE conflict/collision
	// detection code should have already dealt with hard problems. So for example,
	// <entryname> may already look like "foo~<gid7>", but there may be transient
	// issues.  A transient issue is something like:
	//      vv rename foo bar
	//      vv rename xyz FOO
	//      vv revert --all
	// where there is a transient collision on 'foo'.  If we unwind everything in
	// the right order, we don't need to park things.  But since computing the
	// "correct" order is hard (and driving loops by GID doesn't help).  So if we
	// park 'bar' whenever we see it (regardless of order), then we can unwind
	// everything without order dependencies.
	//
	// Things get a little more messy now that REVERT-ALL is using the merge engine
	// to do things.  Consider:
	//      vv remove foo
	//      date > foo
	//      vv add foo        (optional step)
	//      vv revert --all
	// The merge-proper code says that we should delete the new foo and undelete
	// the original one.  AND DOES NOT REPORT A COLLISION.  (Yes, we have a
	// transient/ordering issue, but not a final/hard collision.)  So the merge
	// engine thinks we can do it.  ***BUT*** REVERT tries hard to not delete
	// uncontrolled items, so the delete of the new foo is done with "keep".
	// So when we try to restore the old foo here, we get an unexpected hard
	// collision.  I've expanded the park-it rules below to include this case.
	// This defers the hard error until the "unpark" phase (which needs to be
	// able to handle --force options). W6339, W9411.
	
	SG_wc_tx__undo_delete(pCtx, pMrg->pWcTx,
						  SG_string__sz(pStringRepoPath_Src),
						  SG_string__sz(pStringRepoPath_DestDir),
						  SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname),
						  &undo_delete_args,
						  SG_WC_STATUS_FLAGS__ZERO);
	if ( ! SG_CONTEXT__HAS_ERR(pCtx))
		goto done_ok;

	bIsPortFlags = SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS);
	bIsRevertCollision = ((pMrg->eFakeMergeMode == SG_FAKE_MERGE_MODE__REVERT)
						  && SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS));
	if (!bIsPortFlags && !bIsRevertCollision)
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

		SG_wc_tx__undo_delete(pCtx, pMrg->pWcTx,
							  SG_string__sz(pStringRepoPath_Src),
							  "@/",
							  SG_string__sz(pString_ParkedEntryname),
							  &undo_delete_args,
							  SG_WC_STATUS_FLAGS__ZERO);
		if ( ! SG_CONTEXT__HAS_ERR(pCtx))
			goto done_parked;

		bIsPortFlags = SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS);
		bIsRevertCollision = ((pMrg->eFakeMergeMode == SG_FAKE_MERGE_MODE__REVERT)
							  && SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS));
		if (!bIsPortFlags && !bIsRevertCollision)
			SG_ERR_RETHROW;
			
		SG_context__err_reset(pCtx);
	}

done_parked:
	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__PARKED;

done_ok:
	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;
	pMrg->nrOverrideDelete++;	// (bPresentInLCA) is implied.

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir);
	SG_STRING_NULLFREE(pCtx, pString_Prefix);
	SG_STRING_NULLFREE(pCtx, pString_ParkedEntryname);
}
