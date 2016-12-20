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
 * @file sg_wc_tx__move_rename.c
 *
 * @details Handle a MOVE+RENAME.  This is the fully general
 * version of MOVE and/or RENAME that does both in 1 step.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * MOVE and/or RENAME in one step.
 *
 * The inputs are the full repo-paths of source and destinaion;
 * one currently exists and one will exist after the transaction
 * is applied.
 * 
 * This is the more general version of the API.  The classic
 * move-only and rename-only APIs use this to actually do things.
 * This routine is useful for internal use, because it might
 * cause user confusion (the same as most shell commands) if
 * presented as an full command line option.
 *
 * We optionally allow/disallow after-the-fact moves/renames. 
 *
 */
void SG_wc_tx__move_rename(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const char * pszInput_Src,
						   const char * pszInput_Dest,
						   SG_bool bNoAllowAfterTheFact)
{
	SG_string * pStringRepoPath_Src = NULL;
	SG_string * pStringRepoPath_Dest = NULL;
	SG_string * pStringRepoPath_DestDir = NULL;
	SG_string * pStringEntryname_Dest = NULL;
	sg_wc_liveview_item * pLVI_Src;			// we do not own this
	sg_wc_liveview_item * pLVI_Dest;		// we do not own this
	sg_wc_liveview_item * pLVI_DestDir;		// we do not own this
	char chDomain_Src;
	char chDomain_Dest;
	SG_bool bKnown_Src;
	SG_bool bKnown_Dest;
	SG_wc_status_flags xu_mask = SG_WC_STATUS_FLAGS__ZERO;

	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Src,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Src, &chDomain_Src)  );
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Dest,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Dest, &chDomain_Dest)  );

#if TRACE_WC_TX_MOVE_RENAME
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move_rename: source '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Src, chDomain_Src, SG_string__sz(pStringRepoPath_Src))  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move_rename: dest '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Dest, chDomain_Dest, SG_string__sz(pStringRepoPath_Dest))  );
#endif

	// do minimal validation here (just enough to print
	// better error messages while we still have access
	// to the user-input and the normalized domain-aware
	// repo-paths.  save the serious validation for the
	// "move-rename" code.

	// Fetch the LVI for the source item.  We expect this to
	// return UNCONTROLLED, MATCHED, LOST.  (We should not see
	// any DELETED items here.)  We get !bKnown for bogus paths.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_Src,
														  &bKnown_Src, &pLVI_Src)  );
	if (!bKnown_Src || (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_Src->scan_flags_Live)))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Source '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );
	if (pLVI_Src == pWcTx->pLiveViewItem_Root)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot MOVE/RENAME the root directory '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );

	// Try to lookup the complete destination.
	// 
	// In a normal operation, the destination should not exist,
	// so we should not be able to get an LVI for it -- unless
	// it is an after-the-fact operation.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_Dest,
														  &bKnown_Dest, &pLVI_Dest)  );
	if (bKnown_Dest)
	{
		if (pLVI_Src->pvhIssue
			&& (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
		{
			xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
			if (pLVI_Src->pLiveViewDir != pLVI_Dest->pLiveViewDir)
				xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION);
			if (strcmp(SG_string__sz(pLVI_Src->pStringEntryname),
					   SG_string__sz(pLVI_Dest->pStringEntryname)) != 0)
				xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME);
		}

		SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lvi_lvi(pCtx, pWcTx,
														  pLVI_Src,
														  pLVI_Dest,
														  bNoAllowAfterTheFact,
														  xu_mask)  );
	}
	else
	{
		// The complete destination does not exist.  Cut it up
		// into (parent,entryname) and try again.  Note that
		// we can not do this for gid- or tid-domain inputs
		// (because they are not pathnames).

		switch (chDomain_Dest)
		{
		case SG_WC__REPO_PATH_DOMAIN__G:
		case SG_WC__REPO_PATH_DOMAIN__T:
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "Cannot use domain '%c' repo-path for destination of move+rename: '%s' (%s)",
							 chDomain_Dest, pszInput_Dest, SG_string__sz(pStringRepoPath_Dest))  );

		default:
			SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
												  &pStringRepoPath_DestDir,
												  pStringRepoPath_Dest)  );
			SG_ERR_CHECK(  SG_repopath__remove_last(pCtx,
													pStringRepoPath_DestDir)  );
			SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx,
														   pStringRepoPath_DestDir)  );
			SG_ERR_CHECK(  SG_repopath__get_last(pCtx,
												 pStringRepoPath_Dest,
												 &pStringEntryname_Dest)  );

			SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lookup_destdir(pCtx, pWcTx,
																	 pszInput_Dest, // TODO 2012/08/14 should we pop off the final entryname on this?
																	 pStringRepoPath_DestDir,
																	 &pLVI_DestDir)  );

			if (pLVI_Src->pvhIssue
				&& (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
			{
				xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
				if (pLVI_Src->pLiveViewDir->uiAliasGidDir != pLVI_DestDir->uiAliasGid)
					xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION);
				if (strcmp(SG_string__sz(pLVI_Src->pStringEntryname),
						   SG_string__sz(pStringEntryname_Dest)) != 0)
					xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME);
			}

			SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lvi_lvi_sz(pCtx, pWcTx,
																 pLVI_Src,
																 pLVI_DestDir,
																 SG_string__sz(pStringEntryname_Dest),
																 bNoAllowAfterTheFact,
																 xu_mask)  );
			break;
		}
	}
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Dest);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir);
	SG_STRING_NULLFREE(pCtx, pStringEntryname_Dest);
}

//////////////////////////////////////////////////////////////////

/**
 * MOVE and/or RENAME in one step where the destination
 * is only known as (new-GID-parent, new-entryname).
 * That is, where the caller doesn't know the current
 * repo-path of the destination directory and they'd
 * like to do something like "@<gid>/<entryname>" if
 * we allowed paths in GID-domain repo-paths.
 *
 */
void SG_wc_tx__move_rename2(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput_Src,
							const char * pszInput_DestDir,
							const char * pszEntryname_Dest,
							SG_bool bNoAllowAfterTheFact)
{
	SG_ERR_CHECK_RETURN(  SG_wc_tx__move_rename3(pCtx, pWcTx,
												 pszInput_Src,
												 pszInput_DestDir, pszEntryname_Dest,
												 bNoAllowAfterTheFact,
												 SG_WC_STATUS_FLAGS__ZERO)  );
}

void SG_wc_tx__move_rename3(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput_Src,
							const char * pszInput_DestDir,
							const char * pszEntryname_Dest,
							SG_bool bNoAllowAfterTheFact,
							SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src = NULL;
	SG_string * pStringRepoPath_DestDir = NULL;
	sg_wc_liveview_item * pLVI_Src;			// we do not own this
	sg_wc_liveview_item * pLVI_DestDir;		// we do not own this
	char chDomain_Src;
	char chDomain_DestDir;
	SG_bool bKnown_Src;

	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Src,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Src, &chDomain_Src)  );
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_DestDir,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_DestDir, &chDomain_DestDir)  );

#if TRACE_WC_TX_MOVE_RENAME
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move_rename2: source '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Src, chDomain_Src, SG_string__sz(pStringRepoPath_Src))  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move_rename2: destdir '%s' normalized to [domain %c] '%s'\n",
							   pszInput_DestDir, chDomain_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move_rename2: dest '%s'\n",
							   pszEntryname_Dest)  );
#endif

	if ((strchr(pszEntryname_Dest, '/')  != NULL)
		|| (strchr(pszEntryname_Dest, '\\')  != NULL)
		|| (pszEntryname_Dest[0] == '@'))
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx,
								"The destination of a RENAME must be a simple entryname, not a pathname: '%s'",
								pszEntryname_Dest)  );

	// do minimal validation here (just enough to print
	// better error messages while we still have access
	// to the user-input and the normalized domain-aware
	// repo-paths.  save the serious validation for the
	// "move-rename" code.

	// Fetch the LVI for the source item.  We expect this to
	// return UNCONTROLLED, MATCHED, LOST.  (We should not see
	// any DELETED items here.)  We get !bKnown for bogus paths.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_Src,
														  &bKnown_Src, &pLVI_Src)  );
	if (!bKnown_Src || (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_Src->scan_flags_Live)))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Source '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );
	if (pLVI_Src == pWcTx->pLiveViewItem_Root)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot MOVE/RENAME the root directory '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );

	SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lookup_destdir(pCtx, pWcTx,
															 pszInput_DestDir,
															 pStringRepoPath_DestDir,
															 &pLVI_DestDir)  );
	SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lvi_lvi_sz(pCtx, pWcTx,
														 pLVI_Src,
														 pLVI_DestDir,
														 pszEntryname_Dest,
														 bNoAllowAfterTheFact,
														 xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir);
}
