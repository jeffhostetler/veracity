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
 * @file sg_wc_tx__status.c
 *
 * @details Return canonical status info for an item, a directory,
 * or the whole tree.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Deal with the status for 1 item and accumulate
 * the result in pvaStatus.
 *
 */
static void _sg_wc_tx__status(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  SG_varray * pvaStatus,
							  const char * pszInput,
							  SG_uint32 depth,
							  SG_bool bListUnchanged,
							  SG_bool bNoIgnores,
							  SG_bool bNoTSC,
							  SG_bool bListSparse,
							  SG_bool bListReserved,
							  SG_vhash ** ppvhLegend)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	char chDomain;
	SG_vhash * pvhCSets = NULL;
	SG_vhash * pvhLegend = NULL;
	const char * pszWasLabel_l = "Baseline (B)";
	const char * pszWasLabel_r = "Working";

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pvaStatus );
	// pszInput is optional -- if omitted, we assume "@/".
	// ppvhLegend is optional

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ROOT,
														&pStringRepoPath, &chDomain)  );

#if TRACE_WC_TX_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__status: '%s' normalized to [domain %c] '%s'\n",
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

	SG_ERR_CHECK(  sg_wc_tx__rp__status__lvi(pCtx,
											 pWcTx,
											 pLVI,
											 depth,
											 bListUnchanged,
											 bNoIgnores,
											 bNoTSC,
											 bListSparse,
											 bListReserved,
											 pszWasLabel_l, pszWasLabel_r,
											 pvaStatus)  );

#if TRACE_WC_TX_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__status: computed status [depth %d][bListUnchanged %d][bNoIgnores %d][bNoTSC %d][bListSparse %d][bListReserved %d] on '%s':\n",
							   depth,
							   bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved,
							   SG_string__sz(pStringRepoPath))  );
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pvaStatus, "")  );
#endif

	if (ppvhLegend)
	{
		const char * pszHid_x;

		SG_ERR_CHECK(  SG_wc_tx__get_wc_csets__vhash(pCtx, pWcTx, &pvhCSets)  );

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhLegend)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "A", &pszHid_x)  );
		if (pszHid_x)
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhLegend, "A", pszHid_x)  );

		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "L0", &pszHid_x)  );
		if (pszHid_x)
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhLegend, "B", pszHid_x)  );

		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "L1", &pszHid_x)  );
		if (pszHid_x)
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhLegend, "C", pszHid_x)  );

#if TRACE_WC_TX_STATUS
		SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx, pvhLegend, "Legend")  );
#endif

		*ppvhLegend = pvhLegend;
		pvhLegend = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	SG_VHASH_NULLFREE(pCtx, pvhCSets);
}

//////////////////////////////////////////////////////////////////

/**
 * Begin a "canonical" status.  This is a VARRAY with one
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
void SG_wc_tx__status(SG_context * pCtx,
					  SG_wc_tx * pWcTx,
					  const char * pszInput,
					  SG_uint32 depth,
					  SG_bool bListUnchanged,
					  SG_bool bNoIgnores,
					  SG_bool bNoTSC,
					  SG_bool bListSparse,
					  SG_bool bListReserved,
					  SG_bool bNoSort,
					  SG_varray ** ppvaStatus,
					  SG_vhash ** ppvhLegend)
{
	SG_varray * pvaStatus = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	// pszInput is optional
	SG_NULLARGCHECK_RETURN( ppvaStatus );
	// ppvhLegend is optional

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaStatus)  );

	SG_ERR_CHECK(  _sg_wc_tx__status(pCtx, pWcTx, pvaStatus, pszInput, depth, 
									 bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved,
									 ppvhLegend)  );

	// since we only have 1 input, we don't need to dedup the varray.

	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, pvaStatus)  );

	*ppvaStatus = pvaStatus;
	pvaStatus = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

//////////////////////////////////////////////////////////////////

/**
 * Begin a canonical STATUS when given one or more items using a SG_stringarray.
 * If psaInputs is NULL, assume the entire tree.
 * 
 * We accumulate all of the results into a single pvaStatus.
 * 
 * This status routine uses a pWcTx so that the results
 * reflect the in-progress transaction.
 *
 */
void SG_wc_tx__status__stringarray(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_stringarray * psaInputs,
								   SG_uint32 depth,
								   SG_bool bListUnchanged,
								   SG_bool bNoIgnores,
								   SG_bool bNoTSC,
								   SG_bool bListSparse,
								   SG_bool bListReserved,
								   SG_bool bNoSort,
								   SG_varray ** ppvaStatus,
								   SG_vhash ** ppvhLegend)
{
	SG_varray * pvaStatus = NULL;
	SG_uint32 k, count;
	SG_vhash * pvhLegend = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppvaStatus );
	// ppvhLegend is optional

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaStatus)  );

	if (psaInputs)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  _sg_wc_tx__status(pCtx, pWcTx, pvaStatus, pszInput_k, depth, 
											 bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved,
											 (((k==0) && ppvhLegend) ? &pvhLegend : NULL))  );
		}

		if (ppvhLegend)
		{
			*ppvhLegend = pvhLegend;
			pvhLegend = NULL;
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
		SG_ERR_CHECK(  _sg_wc_tx__status(pCtx, pWcTx, pvaStatus, NULL, depth, 
										 bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved,
										 ppvhLegend)  );

		// we don't need to de-dup since we simulated 1 argument.
	}

	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, pvaStatus)  );

	*ppvaStatus = pvaStatus;
	pvaStatus = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
}
