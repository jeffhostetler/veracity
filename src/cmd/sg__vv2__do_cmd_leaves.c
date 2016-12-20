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
 * @file sg__vv2__do_cmd_leaves.c
 *
 * @details Handle the 'vv leaves' command.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

/**
 * List the heads/tips/leaves.
 */
void vv2__do_cmd_leaves(SG_context * pCtx, 
						SG_option_state * pOptSt)
{
	SG_uint64 ui64DagNum = SG_DAGNUM__VERSION_CONTROL;
	SG_repo * pRepo = NULL;
	SG_varray * pvaHidLeaves = NULL;
    SG_vhash* pvh_pile = NULL;
	SG_uint32 countLeaves = 0;
	SG_uint32 k;

#if defined(DEBUG)
	if (pOptSt->bGaveDagNum)
		ui64DagNum = pOptSt->ui64DagNum;
#endif

	SG_ERR_CHECK(  SG_vv2__leaves(pCtx, pOptSt->psz_repo, ui64DagNum, &pvaHidLeaves)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaHidLeaves, &countLeaves)  );

#if defined(DEBUG)
	if (ui64DagNum != SG_DAGNUM__VERSION_CONTROL)
	{
		// can't use the history code to do fancy printout of non-vc dags...

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Found %d leaves:\n", countLeaves)  );
		for (k=0; k<countLeaves; k++)
		{
			const char * pszHid_k;

			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaHidLeaves, k, &pszHid_k)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", pszHid_k)  );
		}

		goto done;
	}
#endif

	// TODO 2012/06/29 This is a bit redundant.  The SG_vv2__leaves() code opened the repo
	// TODO            (named or default), looked up stuff, built the list, and returned it
	// TODO            to us.  Now *WE* have to open the repo so that we can print the
	// TODO            log-details for each CSET.  This happens in many places (not just here).
	// TODO
	// TODO            It'd be nice if we could maybe refactor __dump_log() and push
	// TODO            some of it to a lower level (in WC or VV2, rather than having
	// TODO            cmd_util know that much detail).

	if (pOptSt->psz_repo && *pOptSt->psz_repo)
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pOptSt->psz_repo, &pRepo)  );
	else
		SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );
	
    // we still ask for the branch pile so we can show the branch name in the output.

    SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );

	for (k=0; k<countLeaves; k++)
	{
		const char * pszHid_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaHidLeaves, k, &pszHid_k)  );
        SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx, SG_CS_STDOUT, pRepo, pszHid_k, pvh_pile, SG_FALSE, pOptSt->bVerbose)  );
	}

#if defined(DEBUG)
done:
	;
#endif

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VARRAY_NULLFREE(pCtx, pvaHidLeaves);
}
