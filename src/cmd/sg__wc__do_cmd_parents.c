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
 * @file sg__wc__do_cmd_parents.c
 *
 * @details Handle the 'vv parents' command.
 * Print info for the parents of the current WD.
 *
 * THIS DOES NOT DO ANY HISTORICAL LOOKUP.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void wc__do_cmd_parents(SG_context * pCtx,
						const SG_option_state * pOptSt)
{
	SG_repo * pRepo = NULL;
    SG_vhash * pvhPileOfCleanBranches = NULL;
	SG_varray * pvaParents = NULL;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_wc__get_wc_parents__varray(pCtx, NULL, &pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &count)  );

	SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );
    SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPileOfCleanBranches)  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "Parents of pending changes in working copy:\n" )  );
	for (k=0; k<count; k++)
	{
		const char * pszHid_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, k, &pszHid_k)  );
		SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx, SG_CS_STDOUT, pRepo, pszHid_k,
											 pvhPileOfCleanBranches, SG_FALSE,
											 pOptSt->bVerbose)  );
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
	SG_REPO_NULLFREE(pCtx, pRepo);
    SG_VHASH_NULLFREE(pCtx, pvhPileOfCleanBranches);
}

