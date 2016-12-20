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

void sg_vv2__history__get_starting_changesets(
	SG_context * pCtx,
	SG_repo * pRepo,
	const SG_rev_spec* pRevSpec,
	SG_stringarray ** ppsaHidChangesets,
	SG_stringarray ** ppsaHidChangesetsMissing,
	SG_bool * pbRecommendDagWalk,
	SG_bool * pbLeaves)
{
	SG_uint32 countRevSpecs = 0;
	SG_stringarray* psaHidChangesets = NULL;
	SG_stringarray* psaHidChangesetsMissing = NULL;
	SG_rbtree* prbHidChangesets = NULL;

	*pbLeaves = SG_FALSE;

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs)
	{
		SG_uint32 countBranches = 0;
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo__dedup(pCtx, pRepo, pRevSpec, SG_TRUE,
														 &psaHidChangesets, &psaHidChangesetsMissing)  );
		SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, pRevSpec, &countBranches)  );
		if (countBranches)
			*pbRecommendDagWalk = SG_TRUE;
	}
	else //Assume that they want all history.  Fill the leaves.
	{
		SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &prbHidChangesets)  );
		SG_ERR_CHECK(  SG_rbtree__to_stringarray__keys_only(pCtx, prbHidChangesets, &psaHidChangesets)  );
		*pbRecommendDagWalk = SG_FALSE; //If we are looking at all leaves, we can use the faster
										//list method, instead of querying.
		*pbLeaves = SG_TRUE;
	}

	SG_RETURN_AND_NULL(psaHidChangesets, ppsaHidChangesets);
	SG_RETURN_AND_NULL(psaHidChangesetsMissing, ppsaHidChangesetsMissing);
	
fail:
	SG_RBTREE_NULLFREE(pCtx, prbHidChangesets);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesets);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesetsMissing);
}

void sg_vv2__history__lookup_gids_by_repopaths(
	SG_context * pCtx,
	SG_repo * pRepo,
	const char * psz_dagnode_hid,
	const SG_stringarray * psaArgs,    // If present these must be full repo-paths.
	SG_stringarray ** ppStringArrayGIDs)
{
	SG_stringarray * pStringArrayGIDs = NULL;
	char * pszHidForRoot = NULL;
	SG_uint32 count_args;
	SG_uint32 i = 0;

	char * pszSearchItemGID = NULL;
	SG_treenode * pTreeNodeRoot = NULL;
	SG_treenode_entry * ptne = NULL;

	SG_ERR_CHECK( SG_stringarray__count(pCtx, psaArgs, &count_args)  );
	SG_ERR_CHECK( SG_STRINGARRAY__ALLOC(pCtx, &pStringArrayGIDs, count_args) );
	SG_ERR_CHECK( SG_changeset__tree__find_root_hid(pCtx, pRepo, psz_dagnode_hid, &pszHidForRoot) );
	SG_ERR_CHECK( SG_treenode__load_from_repo(pCtx, pRepo, pszHidForRoot, &pTreeNodeRoot) );
	for (i = 0; i < count_args; i++)
	{
		const char * pszArg_i;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaArgs, i, &pszArg_i)  );
		if ((pszArg_i[0] == '@') && (pszArg_i[1] == 'g'))
		{
			SG_bool bValid;
			SG_ERR_CHECK(  SG_gid__verify_format(pCtx, &pszArg_i[1], &bValid)  );
			if (!bValid)
				SG_ERR_THROW2(  SG_ERR_INVALIDARG,
								(pCtx, "Argument '%s' must be an absolute repo-path or full gid.", pszArg_i)  );
			SG_ERR_CHECK( SG_stringarray__add(pCtx, pStringArrayGIDs, &pszArg_i[1]) );
		}
		else if ((pszArg_i[0] == '@') && (pszArg_i[1] == '/'))
		{
			SG_ERR_CHECK( SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreeNodeRoot, pszArg_i,
																  &pszSearchItemGID, &ptne) );
			if (pszSearchItemGID == NULL)
				SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
								(pCtx, "Argument '%s' not found.", pszArg_i)  );
			SG_ERR_CHECK( SG_stringarray__add(pCtx, pStringArrayGIDs, pszSearchItemGID) );
			SG_NULLFREE(pCtx, pszSearchItemGID);
			SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "Argument '%s' must be full repo-path.", pszArg_i)  );
		}
	}
	SG_RETURN_AND_NULL(pStringArrayGIDs, ppStringArrayGIDs);

fail:
	SG_NULLFREE(pCtx, pszHidForRoot);
	SG_TREENODE_NULLFREE(pCtx, pTreeNodeRoot);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
	SG_NULLFREE(pCtx, pszSearchItemGID);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
}

