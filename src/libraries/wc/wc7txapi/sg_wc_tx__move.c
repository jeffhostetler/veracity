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
 * @file sg_wc_tx__move.c
 *
 * @details Handle a plain MOVE (without RENAME).
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * The classic interface for a MOVE.
 *
 * The inputs are:
 * [] the item to be moved.
 * [] the destination directory.
 *
 * This API corresponds to the usual command line usage where you
 * don't repeat the entryname in the destination.  It is a simple
 * MOVE and not a MOVE+RENAME.  This is an arbitrary choice because
 * the underlying layers allow both to be done in one step.
 *
 * We optionally allow/disallow after-the-fact MOVES.
 *
 */
void SG_wc_tx__move(SG_context * pCtx,
					SG_wc_tx * pWcTx,
					const char * pszInput_Src,
					const char * pszInput_DestDir,
					SG_bool bNoAllowAfterTheFact)
{
	SG_string * pStringRepoPath_Src = NULL;
	SG_string * pStringRepoPath_DestDir = NULL;
	sg_wc_liveview_item * pLVI_Src;			// we do not own this
	sg_wc_liveview_item * pLVI_DestDir;		// we do not own this
	char chDomain_Src;
	char chDomain_DestDir;
	SG_bool bKnown_Src;
	SG_wc_status_flags xu_mask = SG_WC_STATUS_FLAGS__ZERO;

	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Src,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Src, &chDomain_Src)  );
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_DestDir,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_DestDir, &chDomain_DestDir)  );

#if TRACE_WC_TX_MOVE_RENAME
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move: source '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Src, chDomain_Src, SG_string__sz(pStringRepoPath_Src))  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__move: destdir '%s' normalized to [domain %c] '%s'\n",
							   pszInput_DestDir, chDomain_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );
#endif

	// do minimal validation here (just enough to print
	// better error messages while we still have access
	// to the user-input and the normalized domain-aware
	// repo-paths.  save the serious validation for the
	// "move-rename" code.

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

	if (pLVI_Src->pvhIssue && (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
	{
		xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
		xu_mask |= (pLVI_Src->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION);
	}
		
	// since both the source and destination inputs/repo-paths
	// could be GID- or baseline- domain strings, we can't
	// use the actual text in either.  we have to speak in
	// LVIs and the current entryname in the LVI.

	SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lvi_lvi_sz(pCtx, pWcTx,
														 pLVI_Src,
														 pLVI_DestDir,
														 SG_string__sz(pLVI_Src->pStringEntryname),
														 bNoAllowAfterTheFact,
														 xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir);
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__move__stringarray(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const SG_stringarray * psaInputs,
								 const char * pszInput_DestDir,
								 SG_bool bNoAllowAfterTheFact)
{
	SG_uint32 k, count;

	if (psaInputs)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  SG_wc_tx__move(pCtx, pWcTx, pszInput_k, pszInput_DestDir, bNoAllowAfterTheFact)  );
		}
	}
	else
	{
		// if no args, we DO NOT assume "@/" for a MOVE.
		// (caller should have thrown USAGE if apropriate.)
		
		SG_NULLARGCHECK_RETURN( psaInputs );
	}

fail:
	return;
}
