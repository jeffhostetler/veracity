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
 * @file sg_vv_verbs__heads.h
 *
 * @details Routines to perform most of 'vv heads'.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_VERBS__HEADS_H
#define H_SG_VV_VERBS__HEADS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
void SG_vv_verbs__heads__repo(SG_context * pCtx, 
        SG_repo * pRepo,
		const SG_stringarray* psaRequestedBranchNames,
		SG_bool bAll,
		SG_rbtree ** pprbLeafHidsToOutput,
		SG_rbtree ** pprbNonLeafHidsToOutput)
{
	SG_rbtree* prbLeaves = NULL;
	SG_vhash* pvhPileBranches = NULL;
	SG_rbtree* prbRequestedBranchNames = NULL;
	SG_rbtree_iterator* pit = NULL;

	SG_vhash* pvhRefBranches = NULL;
	SG_vhash* pvhRefClosedBranches = NULL;

	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx,pRepo,SG_DAGNUM__VERSION_CONTROL,&prbLeaves)  );

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPileBranches)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhPileBranches, "branches", &pvhRefBranches)  );

	if (psaRequestedBranchNames)
	{
		const char* pszRefName;
		SG_bool bOk = SG_FALSE;

		/* GetOpt puts these in a stringarray. But we'll look them up in a loop below, so we copy them to an rbtree. */
		SG_ERR_CHECK(  SG_stringarray__to_rbtree_keys(pCtx, psaRequestedBranchNames, &prbRequestedBranchNames)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbRequestedBranchNames, &bOk, &pszRefName, NULL)  );
		while (bOk)
		{
			SG_bool bHas = SG_FALSE;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranches, pszRefName, &bHas)  );
			if (!bHas)
				SG_ERR_THROW2(SG_ERR_BRANCH_NOT_FOUND, (pCtx, "%s", pszRefName));
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &bOk, &pszRefName, NULL)  );
		}
	}
    

	{
		SG_bool bHasClosedBranches;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhPileBranches, "closed", &bHasClosedBranches)  );
		if (bHasClosedBranches)
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhPileBranches, "closed", &pvhRefClosedBranches)  );
	}

	/* Collect the HIDs of changesets we want to output. 
	 * We keep leaf and non-leaf changesets in separate lists because we want to display leaves first.
	 * We keep the lists in rbtrees to easily prevent duplicates, otherwise a changeset belonging to two branches could appear twice. */
	{
		SG_uint32 i, countAllBranches;
		
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefBranches, &countAllBranches)  );
		SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, pprbLeafHidsToOutput, countAllBranches, NULL)  );
		SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, pprbNonLeafHidsToOutput, countAllBranches, NULL)  );

		for (i = 0; i < countAllBranches; i++)
		{
			const SG_variant* pv = NULL;
			const char* pszRefBranchName = NULL;
			SG_vhash* pvh_branch_info = NULL;
			SG_vhash* pvh_branch_values = NULL;
			SG_uint32 count_branch_values = 0;
			SG_uint32 j = 0;
			SG_bool bBranchIsClosed = SG_FALSE;
			SG_bool bBranchRequested = SG_TRUE;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefBranches, i, &pszRefBranchName, &pv)  );

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefClosedBranches, pszRefBranchName, &bBranchIsClosed)  );
			if (prbRequestedBranchNames)
				SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbRequestedBranchNames, pszRefBranchName, &bBranchRequested, NULL)  );
			
			if (bBranchRequested && (bAll || !bBranchIsClosed))
			{
				SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_branch_info)  );
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
				SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_branch_values)  );

				for (j=0; j<count_branch_values; j++)
				{
					const char* pszRefHid = NULL;
					SG_bool bIsLeaf;
					SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, j, &pszRefHid, NULL)  );
					SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbLeaves, pszRefHid, &bIsLeaf, NULL)  );
					if (bIsLeaf)
						SG_ERR_CHECK(  SG_rbtree__update(pCtx, *pprbLeafHidsToOutput, pszRefHid)  );
					else
						SG_ERR_CHECK(  SG_rbtree__update(pCtx, *pprbNonLeafHidsToOutput, pszRefHid)  );
				}
			}
		}
	}

fail:
    SG_VHASH_NULLFREE(pCtx, pvhPileBranches);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	SG_RBTREE_NULLFREE(pCtx, prbRequestedBranchNames);
}

void SG_vv_verbs__heads(SG_context * pCtx, 
        const char* psz_repo,
		const SG_stringarray* psaRequestedBranchNames,
		SG_bool bAll,
		SG_rbtree ** prbLeafHidsToOutput,
		SG_rbtree ** prbNonLeafHidsToOutput)
{

	SG_repo* pRepo = NULL;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );
	
	SG_ERR_CHECK(  SG_vv_verbs__heads__repo(pCtx, pRepo, psaRequestedBranchNames, bAll, prbLeafHidsToOutput, prbNonLeafHidsToOutput)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

//////////////////////////////////////////////////////////////////


END_EXTERN_C;

#endif//H_SG_VV_VERBS__HEADS_H
