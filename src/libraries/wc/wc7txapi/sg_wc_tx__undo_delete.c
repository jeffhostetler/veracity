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
 * Try to un-delete the requested item.
 *
 * pszInput_Src should be baseline-relative/domain
 * (or gid-domain) relative.  (Since we never address
 * deleted items using a LIVE repo-path.)
 *
 * pszInput_Dest and pszEntryname_Dest are optional.
 * If omitted, we try to un-delete the item where it
 * was in the baseline.  If either/both given, we
 * try to do an implicit move/rename as we are
 * restoring it.
 *
 * If pArgs is given, we will try to restore the
 * item using the given values rather than defaulting
 * to the baseline values.
 *
 * We will implicitly resolve any choices set in xu_mask;
 * see __XU__ flags.
 * 
 */
void SG_wc_tx__undo_delete(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const char * pszInput_Src,
						   const char * pszInput_DestDir,
						   const char * pszEntryname_Dest,
						   const SG_wc_undo_delete_args * pArgs,
						   SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src = NULL;
	SG_string * pStringRepoPath_DestDir = NULL;
	sg_wc_liveview_item * pLVI_Src = NULL;			// we do not own this
	sg_wc_liveview_item * pLVI_DestDir = NULL;		// we do not own this
	char chDomain_Src;
	char chDomain_DestDir;
	SG_bool bKnown_Src;
	SG_bool bKnown_DestDir;
	
	SG_NULLARGCHECK_RETURN( pWcTx );
	// pszInput_Src is checked below
	// pszInput_DestDir is optional
	// pszEntryname_Dest is optional
	// pArgs is optional

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Src,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Src, &chDomain_Src)  );
#if TRACE_WC_UNDO_DELETE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__undo_delete: source '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Src, chDomain_Src, SG_string__sz(pStringRepoPath_Src))  );
#endif

	if (pszInput_DestDir)
	{
		SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_DestDir,
															SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
															&pStringRepoPath_DestDir, &chDomain_DestDir)  );

#if TRACE_WC_UNDO_DELETE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "SG_wc_tx__undo_delete: destdir '%s' normalized to [domain %c] '%s'\n",
								   pszInput_DestDir, chDomain_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );
#endif
	}
	
	switch (chDomain_Src)
	{
	case SG_WC__REPO_PATH_DOMAIN__LIVE:
		// we never address deleted items using a live repo-path
		// (because a deleted item does not own a pathname).
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot use domain '%c' repo-path to specify item to un-delete: '%s' (%s)",
						 chDomain_Src, pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );

	default:
		break;
	}

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_Src,
														  &bKnown_Src, &pLVI_Src)  );
	if (!bKnown_Src || SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_Src->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Source '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI_Src->scan_flags_Live) == SG_FALSE)
		SG_ERR_THROW2(  SG_ERR_WC__ITEM_ALREADY_EXISTS,
						(pCtx, "Source '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );

	if (pStringRepoPath_DestDir)
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_DestDir,
															  &bKnown_DestDir, &pLVI_DestDir)  );
		if (!bKnown_DestDir || SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_DestDir->scan_flags_Live))
			SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
							(pCtx, "The destination parent directory '%s' (%s).",
							 pszInput_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );
	}
	else // assume it should be restored in the same parent directory.
	{
		// TODO 2012/03/12 We could use either the *CURRENT* or *BASELINE*
		// TODO            value of the parent GID here.  The question is do
		// TODO            we want an undo to put it where it was when we
		// TODO            pended the delete or where it was originally in
		// TODO            this baseline ?
		// TODO
		// TODO            For now, let's assume the current (last known) value.
		SG_bool bKnown = SG_FALSE;
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx,
															 pLVI_Src->pLiveViewDir->uiAliasGidDir,
															 &bKnown, &pLVI_DestDir)  );
		SG_ASSERT_RELEASE_FAIL2(  (bKnown),
								  (pCtx, "Looking up '%s'", SG_string__sz(pStringRepoPath_Src))  );
	}

	if (!pszEntryname_Dest || !*pszEntryname_Dest)
	{
		// TODO 2012/03/12 Likewise, we could use the *CURRENT* or *BASELINE*
		// TODO            value for the entryname here.
		pszEntryname_Dest = SG_string__sz(pLVI_Src->pStringEntryname);
	}

	SG_ERR_CHECK(  sg_wc_tx__rp__undo_delete__lvi_lvi_sz(pCtx, pWcTx,
														 pLVI_Src,
														 pLVI_DestDir,
														 pszEntryname_Dest,
														 pArgs,
														 xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir);
}
