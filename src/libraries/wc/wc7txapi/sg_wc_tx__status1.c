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
 * @file sg_wc_tx__status1.c
 *
 * @details Compute 1-rev-spec status.  That is, compute status using
 * the live/current WD and an ARBITRARY CSET.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include <sg_vv2__public_typedefs.h>
#include "sg_wc__public_prototypes.h"
#include <sg_vv2__public_prototypes.h>
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Deal with the 1-rev-spec STATUS for 1 item and accumulate
 * the result in pvaStatus.
 *
 * Like all other WC API verbs, we allow relative and absolute paths,
 * "@/..." live/current repo-paths, "@b/..." and "@c/..." extended
 * repo-paths to name an item as it was in either parent cset.
 *
 * We also allow, "@0/..." to name an item as it was in the cset
 * named in the rev-spec. (much like the 2-arg STATUS in VV2.)
 *
 */
static void _sg_wc_tx__status1(SG_context * pCtx,
							   sg_wc_tx__status1__data * pS1Data,
							   SG_varray * pvaStatus,
							   const char * pszInput,
							   SG_uint32 depth)
{
	SG_string * pStringRepoPath = NULL;
	char * pszGid_Allocated = NULL;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pS1Data );
	SG_NULLARGCHECK_RETURN( pvaStatus );
	// pszInput is optional -- if omitted, we assume "@/".

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pS1Data->pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ROOT,
														&pStringRepoPath, &chDomain)  );

#if TRACE_WC_TX_STATUS1
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__status1: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPath))  );
#endif

	if (chDomain == SG_VV2__REPO_PATH_DOMAIN__0)
	{
		SG_ERR_CHECK(  sg_wc_tx__status1__h(pCtx, pS1Data, pStringRepoPath, depth, pvaStatus)  );
	}
	else if (chDomain == 'g')
	{
		const char * pszGid = SG_string__sz(pStringRepoPath);
		pszGid++;	// skip over leading '@'
		SG_ERR_CHECK(  sg_wc_tx__status1__g(pCtx, pS1Data, pszGid, depth, pvaStatus)  );
	}
	else if (chDomain == '/')
	{
		// Since pS1Data already has a live/current repo-path-based index, we can
		// skip the lookup.
		SG_ERR_CHECK(  sg_wc_tx__status1__wc(pCtx, pS1Data, pStringRepoPath, depth, pvaStatus)  );
	}
	else // probably @b/... or @c/...
	{
		sg_wc_liveview_item * pLVI;			// we do not own this
		SG_bool bKnown;

		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pS1Data->pWcTx, pStringRepoPath,
															  &bKnown, &pLVI)  );
		if (!bKnown)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Unknown item '%s'.", SG_string__sz(pStringRepoPath))  );
		
		SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias3(pCtx, pS1Data->pWcTx->pDb, pLVI->uiAliasGid,
														  &pszGid_Allocated)  );
		SG_ERR_CHECK(  sg_wc_tx__status1__g(pCtx, pS1Data, pszGid_Allocated, depth, pvaStatus)  );
	}
			

#if TRACE_WC_TX_STATUS1
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__status1: computed status [depth %d][bListUnchanged %d][bNoIgnores %d][bNoTSC %d][bListSparse %d][bListReserved %d] on '%s':\n",
							   depth,
							   pS1Data->bListUnchanged, pS1Data->bNoIgnores, pS1Data->bNoTSC, pS1Data->bListSparse, pS1Data->bListReserved,
							   SG_string__sz(pStringRepoPath))  );
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pvaStatus, "")  );
#endif

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_NULLFREE(pCtx, pszGid_Allocated);
}

//////////////////////////////////////////////////////////////////

/**
 * Begin a "canonical" 1-rev-spec STATUS.  This is a VARRAY with one
 * row per reported item.  Each item is completely self-contained
 * and contains all of the status info for that item (unlike the
 * "classic" view).
 *
 * pszInput, if given, lists a single file or directory where we
 * should begin the tree-walk.  If null, we assume the entire tree.
 *
 * We accumulate all of the results into a single pvaStatus.
 * 
 * This status routine uses a pWcTx so that the results
 * reflect the in-progress transaction.
 *
 */
void SG_wc_tx__status1(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   const SG_rev_spec * pRevSpec,
					   const char * pszInput,
					   SG_uint32 depth,
					   SG_bool bListUnchanged,
					   SG_bool bNoIgnores,
					   SG_bool bNoTSC,
					   SG_bool bListSparse,
					   SG_bool bListReserved,
					   SG_bool bNoSort,
					   SG_varray ** ppvaStatus)
{
	sg_wc_tx__status1__data * pS1Data = NULL;
	SG_varray * pvaStatus = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pRevSpec );
	// pszInput is optional
	SG_NULLARGCHECK_RETURN( ppvaStatus );

	SG_ERR_CHECK(  sg_wc_tx__status1__setup(pCtx, pWcTx, pRevSpec,
											bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved,
											&pS1Data)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaStatus)  );

	SG_ERR_CHECK(  _sg_wc_tx__status1(pCtx, pS1Data, pvaStatus, pszInput, depth)  );

	// since we only have 1 input, we don't need to dedup the varray.

	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, pvaStatus)  );

	SG_RETURN_AND_NULL( pvaStatus, ppvaStatus );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_WC_TX__STATUS1__DATA__NULLFREE(pCtx, pS1Data);
}

//////////////////////////////////////////////////////////////////

/**
 * Begin a canonical 1-rev-spec STATUS when given one or more items using a SG_stringarray.
 * If psaInputs is NULL, assume the entire tree.
 * 
 * We accumulate all of the results into a single pvaStatus.
 * 
 * This status routine uses a pWcTx so that the results
 * reflect the in-progress transaction.
 *
 */
void SG_wc_tx__status1__stringarray(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									const SG_rev_spec * pRevSpec,
									const SG_stringarray * psaInputs,
									SG_uint32 depth,
									SG_bool bListUnchanged,
									SG_bool bNoIgnores,
									SG_bool bNoTSC,
									SG_bool bListSparse,
									SG_bool bListReserved,
									SG_bool bNoSort,
									SG_varray ** ppvaStatus)
{
	sg_wc_tx__status1__data * pS1Data = NULL;
	SG_varray * pvaStatus = NULL;
	SG_uint32 k, count;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pRevSpec );
	SG_NULLARGCHECK_RETURN( ppvaStatus );

	SG_ERR_CHECK(  sg_wc_tx__status1__setup(pCtx, pWcTx, pRevSpec,
											bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved,
											&pS1Data)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaStatus)  );

	if (psaInputs)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  _sg_wc_tx__status1(pCtx, pS1Data, pvaStatus, pszInput_k, depth)  );
		}

		// In case they said, "vv status foo foo foo" or something
		// like "vv status dirA/foo dirA" we need to de-dup the results
		// since we called the internal status with each input[k] and
		// just accumulated the results.
		SG_ERR_CHECK(  SG_vaofvh__dedup(pCtx, pvaStatus, "gid")  );

	}
	else
	{
		// if no args, assume "@/" for the STATUS.
		SG_ERR_CHECK(  _sg_wc_tx__status1(pCtx, pS1Data, pvaStatus, NULL, depth)  );

		// we don't need to de-dup since we simulated 1 argument.
	}

	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, pvaStatus)  );

	SG_RETURN_AND_NULL( pvaStatus, ppvaStatus );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_WC_TX__STATUS1__DATA__NULLFREE(pCtx, pS1Data);
}
