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
 * @file sg_vv2__status.c
 *
 * @details Compute a HISTORICAL STATUS (either complete or partial)
 * between 2 changesets.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

struct _my_data
{
	SG_vhash * pvhWcInfo;
	SG_repo * pRepo;
	SG_stringarray * psaHids;

	const char * pszHid_0;	// borrowed from psaHids
	const char * pszHid_1;	// borrowed from psaHids

	SG_bool bGivenRepoName;

};

//////////////////////////////////////////////////////////////////

static void _my__free_data(SG_context * pCtx,
						   struct _my_data * pData)
{
	if (!pData)
		return;

	SG_VHASH_NULLFREE(pCtx, pData->pvhWcInfo);
	SG_REPO_NULLFREE(pCtx, pData->pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, pData->psaHids);

	// Don't actually free pData since it is on the stack.
}

//////////////////////////////////////////////////////////////////
	
/**
 * use { pszRepoName, pRevSpec } to get the HIDs of the 2 csets.
 *
 */ 
static void _my__get_cset_hids(SG_context * pCtx,
							   struct _my_data * pData,
							   const char * pszRepoName,
							   const SG_rev_spec * pRevSpec)
{
	SG_uint32 nrHids;
	// should 'vv {diff,status} -b master foo.txt' work if master has 2 heads?
	SG_bool bAllowAmbiguousBranches = SG_FALSE;

	pData->bGivenRepoName = (pszRepoName && *pszRepoName);
	if (!pData->bGivenRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.
		SG_wc__get_wc_info(pCtx, NULL, &pData->pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pData->pvhWcInfo, "repo", &pszRepoName)  );
	}

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pData->pRepo)  );
	SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pData->pRepo, pRevSpec,
											  bAllowAmbiguousBranches,
											  &pData->psaHids, NULL)  );
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, pData->psaHids, &nrHids)  );
	if (nrHids != 2)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Two changesets are required for this operation.")  );
	SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pData->psaHids, 0, &pData->pszHid_0)  );
	SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pData->psaHids, 1, &pData->pszHid_1)  );

	if (strcmp(pData->pszHid_0, pData->pszHid_1) == 0)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Both revision args refer to the same changeset '%s'.",
						 pData->pszHid_0)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * A somewhat public API (for use by WC, but not really exported
 * beyond that).  (The official public API parses its various args
 * and calls us; the WC code already has the args isolated.)
 *
 * This does a complete (non-filtered) status.
 *
 */
void SG_vv2__status__repo(SG_context * pCtx,
						  SG_repo * pRepo,
						  SG_rbtree * prbTreenodeCache,
						  const char * pszHid_0,
						  const char * pszHid_1,
						  char chDomain_0,
						  char chDomain_1,
						  const char * pszLabel_0,
						  const char * pszLabel_1,
						  const char * pszWasLabel_0,
						  const char * pszWasLabel_1,
						  SG_bool bNoSort,
						  SG_varray ** ppvaStatus,
						  SG_vhash ** ppvhLegend)
{
	SG_NULLARGCHECK_RETURN( pRepo );
	// prbTreenodeCache is optional
	SG_NONEMPTYCHECK_RETURN( pszHid_0 );
	SG_NONEMPTYCHECK_RETURN( pszHid_1 );
	// chDomain_* and pszLabel_* are validated in sg_vv2__status__alloc().
	SG_NULLARGCHECK_RETURN( ppvaStatus );
	// ppvhLegend is optional
	
	SG_ERR_CHECK(  sg_vv2__status__main(pCtx, pRepo, prbTreenodeCache,
										pszHid_0, pszHid_1,
										chDomain_0, chDomain_1,
										pszLabel_0, pszLabel_1,
										pszWasLabel_0, pszWasLabel_1,
										bNoSort,
										ppvaStatus, ppvhLegend)  );

fail:
	return;
}

/**
 * Compute the historical status betwen cset-0 and cset-1.
 * This is the official public API for an historical status.
 * 
 */
void SG_vv2__status(SG_context * pCtx,
					const char * pszRepoName,
					const SG_rev_spec * pRevSpec,
					const SG_stringarray * psaInputs,
					SG_uint32 depth,
					SG_bool bNoSort,
					SG_varray ** ppvaStatus,
					SG_vhash ** ppvhLegend)
{
	struct _my_data data;
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;
	const char * pszWasLabel_l = "Changeset (0)";
	const char * pszWasLabel_r = "Changeset (1)";

	memset(&data, 0, sizeof(data));

	// pszRepoName is optional (defaults to WD if present)
	SG_NULLARGCHECK_RETURN( pRevSpec );
	// pasInputs is optional (defaults to complete status)
	SG_NULLARGCHECK_RETURN( ppvaStatus );
	// ppvhLegend is optional

	SG_ERR_CHECK(  _my__get_cset_hids(pCtx, &data, pszRepoName, pRevSpec)  );

	if (!psaInputs)		// do a complete status and be done with it.
	{
		if (depth != SG_INT32_MAX)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Non-recursive option requires one or more files/folders.")  );

		SG_ERR_CHECK(  SG_vv2__status__repo(pCtx, data.pRepo, NULL,
											data.pszHid_0, data.pszHid_1,
											SG_VV2__REPO_PATH_DOMAIN__0, SG_VV2__REPO_PATH_DOMAIN__1,
											SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__B,
											pszWasLabel_l, pszWasLabel_r,
											bNoSort,
											&pvaStatus,
											((ppvhLegend) ? &pvhLegend : NULL))  );
	}
	else				// do filtered status
	{
		SG_ERR_CHECK(  sg_vv2__filtered_status(pCtx, data.pRepo, NULL,
											   data.pszHid_0, data.pszHid_1,
											   (!data.bGivenRepoName),
											   psaInputs, depth,
											   SG_VV2__REPO_PATH_DOMAIN__0,  SG_VV2__REPO_PATH_DOMAIN__1,
											   SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__B,
											   pszWasLabel_l, pszWasLabel_r,
											   bNoSort,
											   &pvaStatus,
											   ((ppvhLegend) ? &pvhLegend : NULL))  );
	}

	SG_RETURN_AND_NULL( pvaStatus, ppvaStatus );
	SG_RETURN_AND_NULL( pvhLegend, ppvhLegend );

fail:
	_my__free_data(pCtx, &data);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
}
