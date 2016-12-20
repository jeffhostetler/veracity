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
 * @file sg_wc_tx__get_item_dirstatus_flags.c
 *
 * @details Return status-flags for a single directory *AND*
 * an indication of whether there are changes *WITHIN* the directory.
 *
 * Note that we do not have a status __C__ bit for directories
 * to indicate that the contents of the directory contains changes;
 * In a historical (vv2) context, we could return such a bit if the
 * HIDs of the directory TNs are different, but we cannot do this
 * (efficiently) for WC because we do not compute the TN HID until
 * we commit it.
 *
 * The only real way to see if the directory contains pending changes
 * is to do a full recursive status starting at the directory and
 * see if it returns any rows (not counting the directory itself).
 *
 * Since the intended use of this is to let Tortoise decorate
 * directories, doing such a full status seems too expensive.
 * The following is an attempt to have a light-weight status that
 * knows that a single change within the directory is sufficient
 * to say that the directory contents have changes and not bother
 * computing how many and what they are.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _ds_lvd(SG_context * pCtx,
					SG_wc_tx * pWcTx,
					sg_wc_liveview_dir * pLVD,
					SG_bool * pbDirContainsChanges)
{
	SG_uint64 uiAlias_Child;
	SG_rbtree_ui64_iterator * pIter = NULL;
	sg_wc_liveview_item * pLVI_Child;		// we do not own this
	SG_bool bDirContainsChanges = SG_FALSE;
	SG_bool bOK;

	// If there are *any* "moved-out" items, we can just stop
	// without computing any item statuses or HIDs.
	if (pLVD->prb64MovedOutItems)
	{
		SG_uint32 nrMovedOut;
		SG_ERR_CHECK(  SG_rbtree_ui64__count(pCtx, pLVD->prb64MovedOutItems, &nrMovedOut)  );
		if (nrMovedOut > 0)
		{
			bDirContainsChanges = SG_TRUE;
			goto done;
		}
	}

	// Iterate over the set of things "still-in" the directory
	// and see if any have changes.
	//
	// NOTE 2012/11/26 We do not filter out sparse items because
	// NOTE            we now allow moves/renames/deletes of sparse items.

	SG_ERR_CHECK(  SG_rbtree_ui64__iterator__first(pCtx, &pIter, pLVD->prb64LiveViewItems,
												   &bOK, &uiAlias_Child, (void**)&pLVI_Child)  );
	while (bOK)
	{
		SG_wc_status_flags sf;

		SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI_Child, SG_FALSE, SG_FALSE, &sf)  );
		if (sf & (SG_WC_STATUS_FLAGS__U__FOUND
				  | SG_WC_STATUS_FLAGS__U__IGNORED
				  | SG_WC_STATUS_FLAGS__R__RESERVED))
		{
			// uncontrolled or reserved -- don't count it.
		}
		else if (sf & SG_WC_STATUS_FLAGS__U__LOST)
		{
			bDirContainsChanges = SG_TRUE;
		}
		else if (sf & (SG_WC_STATUS_FLAGS__S__MASK | SG_WC_STATUS_FLAGS__C__MASK))
		{
			bDirContainsChanges = SG_TRUE;
		}
		else if (pLVI_Child->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			sg_wc_liveview_dir * pLVD_Child;		// we do not own this.
			SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_Child, &pLVD_Child)  );
			SG_ERR_CHECK(  _ds_lvd(pCtx, pWcTx, pLVD_Child, &bDirContainsChanges)  );
		}
		
		if (bDirContainsChanges)
			break;

		SG_ERR_CHECK(  SG_rbtree_ui64__iterator__next(pCtx, pIter,
													  &bOK, &uiAlias_Child, (void**)&pLVI_Child)  );
	}

done:
	*pbDirContainsChanges = bDirContainsChanges;

fail:
	SG_RBTREE_UI64_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__get_item_dirstatus_flags(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const char * pszInput,
										SG_wc_status_flags * pStatusFlagsDir,
										SG_vhash ** ppvhProperties,
										SG_bool * pbDirContainsChanges)
{
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	SG_bool bDirContainsChanges = SG_FALSE;
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	char chDomain;
	SG_bool bKnown;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );
	SG_NULLARGCHECK_RETURN( pStatusFlagsDir );
	// ppvhProperties is optional
	SG_NULLARGCHECK_RETURN( pbDirContainsChanges );

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &chDomain)  );

#if TRACE_WC_TX_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__get_item_dirstatus_flags: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPath))  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath,
														  &bKnown, &pLVI)  );
	if (!bKnown)
	{
		// We only get this if the path is completely bogus and
		// took us off into the weeds (as opposed to reporting
		// something just not-controlled).
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown item '%s'.", SG_string__sz(pStringRepoPath))  );
	}

	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Provided path '%s' does not name a directory.", pszInput)  );

	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI, SG_FALSE, SG_FALSE, &statusFlags)  );
	if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND
					   | SG_WC_STATUS_FLAGS__U__IGNORED
					   | SG_WC_STATUS_FLAGS__R__RESERVED))
	{
		// uncontrolled or reserved -- don't dive.
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
	{
		// probably can't happen since tortoise wouldn't have ask us.
		// say yes because the contents (if any) are also lost.
		bDirContainsChanges = SG_TRUE;
	}
	else
	{
		// NOTE 2012/11/26 We do dive on sparse directories because we now
		// NOTE            allow moves/renames/deletes of items within them.

		sg_wc_liveview_dir * pLVD;		// we do not own this.
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
		SG_ERR_CHECK(  _ds_lvd(pCtx, pWcTx, pLVD, &bDirContainsChanges)  );
	}

	if (ppvhProperties)
		SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, statusFlags, ppvhProperties)  );

	*pStatusFlagsDir = statusFlags;
	*pbDirContainsChanges = bDirContainsChanges;
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

