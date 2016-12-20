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
 * Do a "special-add" of this item.  The item is *NOT* present
 * in the baseline, but is present in the other branch.  There
 * are 2 reasons for this:
 *
 * [] if the item was also in the LCA,
 *       the item was deleted from the baseline.
 *
 *       we need to override/cancel the delete this because the
 *       other branch modified the item or (for a directory)
 *       moved something into it.

 * [] if the item was not in the LCA,
 *       the item first appears in the other branch.
 *
 *       we need to add it to the merge result.
 *
 * The distinctions here are mainly for better context messages.
 * 
 * The net effect is the same:
 * [] we need to ensure that there is an alias for the GID.
 * [] we need to cause a new LVI to be added to the LVD.
 * [] we need to queue/journal a request to populate the item
 *    in the WD.
 *
 *
 */
void sg_wc_tx__merge__queue_plan__add_special(SG_context * pCtx,
											  SG_mrg * pMrg,
											  const char * pszGid,
											  SG_mrg_cset_entry * pMrgCSetEntry_FinalResult)
{
	SG_mrg_cset_entry * pMrgCSetEntry_LCA;
	SG_string * pString_Parent = NULL;
	SG_string * pString_Prefix = NULL;
	SG_string * pString_ParkedEntryname = NULL;
	SG_uint32 k = 0;
	SG_bool bPresentInLCA;
	SG_wc_status_flags statusFlagsAddSpecialReason = SG_WC_STATUS_FLAGS__ZERO;

	switch (pMrg->eFakeMergeMode)
	{
	case SG_FAKE_MERGE_MODE__MERGE:
		statusFlagsAddSpecialReason = SG_WC_STATUS_FLAGS__S__MERGE_CREATED;
		break;

	case SG_FAKE_MERGE_MODE__UPDATE:
		statusFlagsAddSpecialReason = SG_WC_STATUS_FLAGS__S__UPDATE_CREATED;
		break;

	case SG_FAKE_MERGE_MODE__REVERT:
		// This should not happen.  REVERT should not attempt to do a special-add.
		// Anything that it needs to re-add should look like an undo-delete since,
		// by definition, it will be in the baseline.
	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "special-add of %s",
						 SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname))  );
	}

#if defined(WINDOWS)
	if (pMrgCSetEntry_FinalResult->tneType == SG_TREENODEENTRY_TYPE_SYMLINK)
	{
		// implicitly make posix-symlinks sparse on windows
		// since windows doesn't support them.  (FWIW Junction
		// points are different.)
		statusFlagsAddSpecialReason |= SG_WC_STATUS_FLAGS__A__SPARSE;
	}
#endif
	
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_LCA->prbEntries, pszGid,
								   &bPresentInLCA, (void **)&pMrgCSetEntry_LCA)  );

	// We want to add <entryname> to the desired parent directory.
	// Pass the repo-path as (<repo-path-parent>, <entryname>)
	// rather than as a single string.  Since we don't know (or
	// care about) the repo-path of the parent directory, create
	// a GID-domain repo-path or it.

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString_Parent, "@%s",
											pMrgCSetEntry_FinalResult->bufGid_Parent)  );

	// Use level-7 tx api to *try* to insert it in (<repo-path-parent>, <entryname>).
	// This will throw if we hit a transient collision.

	SG_wc_tx__add_special(pCtx, pMrg->pWcTx,
						  SG_string__sz(pString_Parent),
						  SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname),
						  pszGid,
						  pMrgCSetEntry_FinalResult->tneType,
						  pMrgCSetEntry_FinalResult->bufHid_Blob,
						  pMrgCSetEntry_FinalResult->attrBits,
						  statusFlagsAddSpecialReason);
	if ( ! SG_CONTEXT__HAS_ERR(pCtx))
		goto done_ok;

	if (( ! SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS))
		&& ( ! SG_context__err_equals(pCtx, SG_ERR_WC_PORT_DUPLICATE)))
		SG_ERR_RETHROW;
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "QueuePlanAddSpecial: port problem for [%s] parking it: %s\n",
							   pszGid,
							   SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname))  );
#endif

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

		SG_wc_tx__add_special(pCtx, pMrg->pWcTx,
							  "@/",
							  SG_string__sz(pString_ParkedEntryname),
							  pszGid,
							  pMrgCSetEntry_FinalResult->tneType,
							  pMrgCSetEntry_FinalResult->bufHid_Blob,
							  pMrgCSetEntry_FinalResult->attrBits,
							  statusFlagsAddSpecialReason);
		if ( ! SG_CONTEXT__HAS_ERR(pCtx))
			goto done_parked;

		if (( ! SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS))
			&& ( ! SG_context__err_equals(pCtx, SG_ERR_WC_PORT_DUPLICATE)))
			SG_ERR_RETHROW;
			
		SG_context__err_reset(pCtx);
	}

done_parked:
	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__PARKED;

done_ok:
	pMrgCSetEntry_FinalResult->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__APPLIED;
	if (bPresentInLCA)
		pMrg->nrOverrideDelete++;
	else
		pMrg->nrAddedByOther++;

fail:
	SG_STRING_NULLFREE(pCtx, pString_Parent);
	SG_STRING_NULLFREE(pCtx, pString_Prefix);
	SG_STRING_NULLFREE(pCtx, pString_ParkedEntryname);
}

