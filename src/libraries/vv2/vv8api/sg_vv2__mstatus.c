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

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Compute a HISTORICAL MSTATUS from a given changeset M,
 * its 2 parents, and the computed ancestor.
 *
 * We DO HAVE nor use a WD (which means that we DO NOT KNOW about
 * wc.db nor gid-aliases nor know about any current baseline).
 *
 * The version here compares the peers in the classic diamond
 * and returns a single canonical status vhash (augmented with
 * merge-related status codes):
 * 
 *      A
 *     / \.
 *    B   C
 *     \ /
 *      M
 *
 * Where we are given M in the rev-spec.
 *
 *
 * If M does not have 2 parents (and fallback is allowed)
 * return:
 *
 *       A
 *       |
 *       M
 *
 * 
 * It does not take any (argc,argv) to limit the result and it
 * does not do any filtering (unlike the original treediff code).
 *
 * If the repo-name is omitted, we will sniff for a WD and get
 * the fields we need from it.
 *
 *
 * You own the returned pvaStatus and pvhLegend.
 * 
 */
void SG_vv2__mstatus(SG_context * pCtx,
					 const char * pszRepoName,
					 const SG_rev_spec * pRevSpec,
					 SG_bool bNoFallback,
					 SG_bool bNoSort,
					 SG_varray ** ppvaStatus,
					 SG_vhash ** ppvhLegend)
{
	SG_vhash * pvhWcInfo = NULL;
	SG_repo * pRepo = NULL;
	char * pszHid_M = NULL;
	
	// pszRepoName is optional (defaults to WD if present)
	SG_NULLARGCHECK_RETURN( pRevSpec );
	SG_NULLARGCHECK_RETURN( ppvaStatus );
	// ppvhLegend is optional

	if (!pszRepoName || !*pszRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );
	}
	
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &pszHid_M, NULL)  );

	SG_ERR_CHECK(  sg_vv2__mstatus__main(pCtx, pRepo, pszHid_M, bNoFallback, bNoSort, ppvaStatus, ppvhLegend)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, pszHid_M);
}

