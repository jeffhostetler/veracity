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
 * @file sg_vv2__find_cset.c
 *
 * @details Routine to convert various cset-prefix/rev-nr/tag/whatever
 * into a full CSet HID.
 *
 * Formerly: sg_vv_utils__rev_arg_to_csid__no_wd.h
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

/**
 * Use <<rev-spec>> to get a single cset and its associated branch.
 * Fallback and use "master" or single unique head when <<rev-spec>>
 * is empty.
 *
 * THIS IS NOT SUITABLE FOR USE WHEN WE ARE TAKING SOME INFO FROM
 * THE WD (because in that case we should fallback to the current
 * baseline and/or attached branch).
 *
 * We don't public the __repo() form because we don't have enough
 * info to decide if/how to fallback; our caller knows that.
 *
 */
static void _find_cset__repo(SG_context * pCtx,
							 SG_repo * pRepo,
							 const SG_rev_spec * pRevSpec,
							 char ** ppszHidCSet,
							 char ** ppsz_branch_name)
{
	SG_uint32 countRevSpec = 0;
	SG_rev_spec * pRevSpec_Allocated = NULL;

	SG_NULLARGCHECK_RETURN( pRepo );
	SG_NULLARGCHECK_RETURN( ppszHidCSet );
	// ppsz_branch_name is optional

	if (pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpec)  );
		if (countRevSpec > 1)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "At most one revision specifier (e.g. rev, tag, branch) may be used.")  );
		if (countRevSpec == 0)
			pRevSpec = NULL;
	}

	if (!pRevSpec)
	{
		// TODO 2012/07/09 Should we take an argument saying whether we
		// TODO            want this full fallback to master?

		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_Allocated)  );
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec_Allocated, SG_VC_BRANCHES__DEFAULT)  );
		pRevSpec = pRevSpec_Allocated;
	}
	
	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, SG_TRUE, ppszHidCSet, ppsz_branch_name)  );

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_Allocated);
}

static void _get_from_wc_info(SG_context * pCtx,
							  const SG_vhash * pvhWcInfo,
							  char ** ppszHidCSet,
							  char ** ppsz_branch_name)
{
	// When no --repo and no <<rev-spec>> (and there was a WD present)
	// the WD already has the various answers.

	SG_varray * pvaParents = NULL;			// we do not own this
	char * pszHid_Allocated = NULL;
	char * pszBranch_Allocated = NULL;
	const char * pszHid_Ref = NULL;
	const char * pszBranch_Ref = NULL;
	SG_uint32 nrParents = 0;

	SG_NULLARGCHECK_RETURN( ppszHidCSet );
	// ppsz_branch_name is optional

	SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhWcInfo, "parents", &pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &nrParents)  );
	if (nrParents != 1)
		SG_ERR_THROW(  SG_ERR_WC_HAS_MULTIPLE_PARENTS  );

	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 0, &pszHid_Ref)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHid_Ref, &pszHid_Allocated)  );

	if (ppsz_branch_name)
	{
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhWcInfo, "branch", &pszBranch_Ref)  );
		if (pszBranch_Ref)
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszBranch_Ref, &pszBranch_Allocated)  );
	}

	if (ppszHidCSet)
	{
		*ppszHidCSet = pszHid_Allocated;
		pszHid_Allocated = NULL;
	}
	if (ppsz_branch_name)
	{
		*ppsz_branch_name = pszBranch_Allocated;
		pszBranch_Allocated = NULL;
	}

fail:
	SG_NULLFREE(pCtx, pszHid_Allocated);
	SG_NULLFREE(pCtx, pszBranch_Allocated);
}

/**
 * Get the CSET and (optionally) the associated BRANCH for the
 * (repo-name, <<rev-spec>>) pair.  We do this with all of the
 * expected fallbacks.
 *
 * We take care of expanding rev-prefixs, tag-lookups, and etc.
 *
 * Return both the full HID of the desired CSET *and* (optionally)
 * the name of the branch that it is in (if it is in one).
 * 
 */
void SG_vv2__find_cset(SG_context * pCtx,
					   const char * pszRepoName,
					   const SG_rev_spec * pRevSpec,
					   char ** ppszHidCSet,
					   char ** ppsz_branch_name)
{
	SG_vhash * pvhWcInfo = NULL;
	SG_repo * pRepo = NULL;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to WD if present and tied or single parent)

	if (!pszRepoName || !*pszRepoName)
	{
		SG_uint32 countRevSpec = 0;

		// If they didn't give us a RepoName, try to get it from the WD.
		// And if no <<rev-spec>> given, get the current baseline.

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );

		if (pRevSpec)
			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpec)  );
		if (countRevSpec == 0)
		{
			SG_ERR_CHECK(  _get_from_wc_info(pCtx, pvhWcInfo, ppszHidCSet, ppsz_branch_name)  );
			goto done;
		}
	}

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  _find_cset__repo(pCtx, pRepo, pRevSpec, ppszHidCSet, ppsz_branch_name)  );

done:
	;
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
}
