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
 * @file sg_wc_tx__queue__remove.c
 *
 * @details Manipulate the LVI/LVD caches to effect an REMOVE.
 * This is part of the QUEUE phase of the TX where we accumulate
 * the changes and look for problems.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * QUEUE an REMOVE operation.
 *
 * I decided to let this __queue__ api NOT take
 * a repo-path as an argument and let it compute
 * it from the cache.  This saves the caller from
 * having to do pathname juggling in the middle of
 * the recursive REMOVE code.
 *
 * We do allow uncontrolled (FOUND/IGNORED) items
 * here so that the caller can do something like
 * 'vv remove --force <directory>' when the controlled
 * directory contains trash that they don't care about.
 *
 * Schedule/queue the remove of an item and IMPLICITLY
 * mark it resolved if it has an ISSUE.
 * 
 */
void sg_wc_tx__queue__remove(SG_context * pCtx,
							 SG_wc_tx * pWcTx,
							 sg_wc_liveview_item * pLVI,
							 const char * pszDisposition,
							 SG_bool bFlattenAddSpecialPlusRemove,
							 SG_bool * pbKept)
{
	SG_string * pStringRepoPath = NULL;
	SG_pathname * pPathBackup = NULL;
	char * pszGid = NULL;

	// Note: 2012/01/14 As part of the changes to make deleted items
	// Note:            report status using a baseline-relative
	// Note:            repo-path, we must compute the repo-path
	// Note:            *before* we mark it deleted in the pLVI so
	// Note:            that we still get a live-/destination-relative
	// Note:            repo-path and not a baseline-relative one.
	// Note:            (We want something like "@/foo..." not "@b/foo...".)

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pStringRepoPath)  );
#if defined(DEBUG)
	{
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz[0]=='@') && (psz[1]=='/')) );
	}
#endif

	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
	{
		// We've decided that when an item is deleted by the user,
		// we will *IMPLICITLY* also mark it resolved.  This avoids
		// problems where the user deletes something (probably
		// outside of RESOLVE) and still can't commit because of
		// unresolved issues (that they can't easily address/name).
		//
		// For W0960 and W3903 I'm only going to set the overall
		// resolved status and leave the existing (possibly queued)
		// resolve-info in SQL.  This lets us continue to remember
		// any "merge2" type values so that the user can re-resolve
		// and have all of the current choices; if we delete the
		// resolve-info here and they want to restore it, they would
		// be limited to the stock set of cset-relative choices.

		SG_wc_status_flags xr    = (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags xu2xr = (((pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK) >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags sf    = (SG_WC_STATUS_FLAGS__X__RESOLVED | xr | xu2xr);

#if defined(DEBUG)
		if (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)
		{
			SG_ASSERT_RELEASE_FAIL(  (xr    != 0)  );
			SG_ASSERT_RELEASE_FAIL(  (xu2xr == 0)  );
		}
#endif

		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pWcTx, pLVI, sf)  );
	}

	// update pLVI to have deleted status
	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__remove(pCtx, pLVI, bFlattenAddSpecialPlusRemove)  );

	if ((pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		&& (strcmp(pszDisposition, "backup") == 0))
	{
		const char * pszOriginalEntryname = NULL;

		SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszOriginalEntryname)  );
		SG_ERR_CHECK(  sg_wc_db__path__generate_backup_path(pCtx, pWcTx->pDb, pszGid, pszOriginalEntryname,
															&pPathBackup)  );
#if TRACE_WC_TX_REMOVE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("QueueRemoveFile: scheduling backup of '%s' as '%s'.\n"),
								   SG_string__sz(pStringRepoPath),
								   SG_pathname__sz(pPathBackup))  );
#endif
	}

	// add stuff to journal to do:
	// [1] if controlled, update the database.
	// [2] if item is present, deal with it.
	SG_ERR_CHECK(  sg_wc_tx__journal__remove(pCtx,
											 pWcTx,
											 pLVI->pPcRow_PC,
											 pLVI->tneType,
											 pStringRepoPath,
											 (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live)),
											 pszDisposition,
											 pPathBackup)  );

	if (pbKept)
		*pbKept = (strcmp(pszDisposition,"keep") == 0);

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathBackup);
	SG_NULLFREE(pCtx, pszGid);
}
