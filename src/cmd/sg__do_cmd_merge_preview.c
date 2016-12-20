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
 * @file sg__do_cmd_merge_preview.c
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

//////////////////////////////////////////////////////////////////

void do_cmd_merge_preview(SG_context * pCtx, SG_option_state * pOptSt)
{
	SG_repo * pRepo = NULL;
	
	SG_uint32 countRevSpecs = 0;
	SG_stringarray * psaRevSpecs = NULL;
	const char * const * ppszRevSpecs = NULL;
	
	SG_stringarray * psaNewChangesets = NULL;
	const char * const * ppszNewChangesets = NULL;
	SG_uint32 countNewChangesets = 0;
	
	char * pszHidBaseline = NULL;
	char * pszHidMergeTarget = NULL;
	SG_dagquery_relationship relationship;
	
	SG_vhash * pvhPileOfCleanBranches = NULL;
	SG_uint32 i = 0;
	
	countRevSpecs = 0;
	if (pOptSt->pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
		if(countRevSpecs>2)
			SG_ERR_THROW(SG_ERR_USAGE);
	}
	
	if(pOptSt->psz_repo!=NULL)
	{
		if(countRevSpecs==2)
		{
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pOptSt->psz_repo, &pRepo)  );
			SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_FALSE, &psaRevSpecs, NULL)  );
			SG_ERR_CHECK(  SG_stringarray__sz_array(pCtx, psaRevSpecs, &ppszRevSpecs)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, ppszRevSpecs[0], &pszHidBaseline)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, ppszRevSpecs[1], &pszHidMergeTarget)  );
			SG_STRINGARRAY_NULLFREE(pCtx, psaRevSpecs);
		}
		else
		{
			SG_ERR_THROW2(SG_ERR_USAGE, (pCtx, "When using the --repo option, you must provide both the BASELINE-REVSPEC and the OTHER-REVSPEC."));
		}
	}
	else
	{
		SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );
		if(countRevSpecs==2)
		{
			SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_FALSE, &psaRevSpecs, NULL)  );
			SG_ERR_CHECK(  SG_stringarray__sz_array(pCtx, psaRevSpecs, &ppszRevSpecs)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, ppszRevSpecs[0], &pszHidBaseline)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, ppszRevSpecs[1], &pszHidMergeTarget)  );
			SG_STRINGARRAY_NULLFREE(pCtx, psaRevSpecs);
		}
		else
		{
			SG_uint32 countBaselines = 0;
			SG_ERR_CHECK(  SG_wc__get_wc_parents__stringarray(pCtx, NULL, &psaRevSpecs)  );
			SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, psaRevSpecs, &ppszRevSpecs, &countBaselines)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, ppszRevSpecs[0], &pszHidBaseline)  );
			if(countBaselines==2)
			{
				SG_ERR_CHECK(  SG_STRDUP(pCtx, ppszRevSpecs[1], &pszHidMergeTarget)  );
			}
			else
			{
				SG_wc_merge_args merge_args;
				merge_args.pRevSpec = pOptSt->pRevSpec;
				merge_args.bNoAutoMergeFiles = SG_TRUE;	// doesn't matter
				merge_args.bComplainIfBaselineNotLeaf = SG_FALSE;	// doesn't matter
				SG_ERR_CHECK(  SG_wc__merge__compute_preview_target(pCtx, NULL, &merge_args, &pszHidMergeTarget)  );
			}
			SG_STRINGARRAY_NULLFREE(pCtx, psaRevSpecs);
		}
	}
	
	SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL,
		pszHidMergeTarget, pszHidBaseline,
		SG_FALSE, SG_FALSE,
		&relationship)  );
	if(relationship==SG_DAGQUERY_RELATIONSHIP__ANCESTOR || relationship==SG_DAGQUERY_RELATIONSHIP__SAME)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "The baseline already includes the merge target. No merge is needed.\n")  );
	}
	else
	{
		SG_ERR_CHECK(  SG_dagquery__find_new_since_common(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszHidBaseline, pszHidMergeTarget, &psaNewChangesets)  );
		SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, psaNewChangesets, &ppszNewChangesets, &countNewChangesets)  );
		
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPileOfCleanBranches)  );
		for(i=0; i<countNewChangesets; ++i)
		{
			SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx, SG_CS_STDOUT, pRepo, ppszNewChangesets[i], pvhPileOfCleanBranches, SG_TRUE, SG_FALSE)  );
		}
		
		if(relationship==SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\nFast-Forward Merge to '%s' brings in %i changeset%s.\n", pszHidMergeTarget, countNewChangesets, ((countNewChangesets==1)?"":"s"))  );
		}
		else
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\nMerge with '%s' brings in %i changeset%s.\n", pszHidMergeTarget, countNewChangesets, ((countNewChangesets==1)?"":"s"))  );
		}
	}

	SG_VHASH_NULLFREE(pCtx, pvhPileOfCleanBranches);
	SG_STRINGARRAY_NULLFREE(pCtx, psaNewChangesets);
	SG_NULLFREE(pCtx, pszHidBaseline);
	SG_NULLFREE(pCtx, pszHidMergeTarget);
	SG_REPO_NULLFREE(pCtx, pRepo);

	return;
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, psaNewChangesets);
	SG_STRINGARRAY_NULLFREE(pCtx, psaRevSpecs);
	SG_NULLFREE(pCtx, pszHidBaseline);
	SG_NULLFREE(pCtx, pszHidMergeTarget);
	SG_VHASH_NULLFREE(pCtx, pvhPileOfCleanBranches);
}
