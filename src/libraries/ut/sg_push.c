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

#include <sg.h>

#define TRACE_PUSH 0

typedef struct
{
	SG_repo* pRepo; // We don't own this.
	SG_sync_client* pSyncClient;
	SG_pathname* pTempPathname;
	SG_pathname* pFragballPathname;
    SG_fragball_writer* pfb;
	SG_sync_client_push_handle* pClientPushHandle;
	SG_vhash* pvhOtherRepoInfo;
	
	SG_vhash* pvhBranchesInPush; /* The push_add* routines keep track of what version control
								  * branches are being pushed, if any.  If at least one branch
								  * is in the push, locks and branches will be verified as the first 
								  * step of commit. */
	SG_rbtree* prbPotentialNewRemoteHeads;

	SG_bool bAdminOnly; /* True if repo ids don't match but admin ids do. */

	SG_uint32 countRoundtrips;
} _sg_push;

static void _sg_push__nullfree(SG_context* pCtx, _sg_push** ppMyPush)
{
	if (ppMyPush && *ppMyPush)
	{
		_sg_push* pMyPush = *ppMyPush;
		SG_PATHNAME_NULLFREE(pCtx, pMyPush->pTempPathname);
		SG_PATHNAME_NULLFREE(pCtx, pMyPush->pFragballPathname);

		SG_VHASH_NULLFREE(pCtx, pMyPush->pvhOtherRepoInfo);
		SG_VHASH_NULLFREE(pCtx, pMyPush->pvhBranchesInPush);
		
		SG_RBTREE_NULLFREE(pCtx, pMyPush->prbPotentialNewRemoteHeads);

		if (pMyPush->pSyncClient && pMyPush->pClientPushHandle)
			SG_ERR_IGNORE(  SG_sync_client__push_end(pCtx, pMyPush->pSyncClient, &pMyPush->pClientPushHandle)  );
		
		SG_SYNC_CLIENT_NULLFREE(pCtx, pMyPush->pSyncClient);
        SG_FRAGBALL_WRITER_NULLFREE(pCtx, pMyPush->pfb);

		SG_NULLFREE(pCtx, pMyPush);
		*ppMyPush = NULL;
	}
}

static void _push_init(
	SG_context* pCtx, 
	SG_repo* pThisRepo,
	const char* psz_remote_repo_spec, 
	const char* psz_username,
	const char* psz_password,
	SG_bool bFetchRemoteBranchInfo,
	SG_bool bAdminOnly,
	_sg_push** ppMyPush)
{
	_sg_push* pMyPush = NULL;
	char* pszThisRepoId = NULL;
	char* pszThisAdminId = NULL;
	char* pszThisHashMethod = NULL;
	const char* pszRefOtherRepoId = NULL;
	const char* pszRefOtherAdminId = NULL;
	const char* pszRefOtherHashMethod = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pMyPush)  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pMyPush->prbPotentialNewRemoteHeads)  );

	pMyPush->pRepo = pThisRepo;
	SG_ERR_CHECK(  SG_sync_client__open(pCtx, psz_remote_repo_spec, psz_username, psz_password, &pMyPush->pSyncClient)  );
    // The SG_FALSE below tells the other side we do NOT need the areas list
	SG_ERR_CHECK(  SG_sync_client__get_repo_info(pCtx, pMyPush->pSyncClient, 
		bFetchRemoteBranchInfo, SG_FALSE, &pMyPush->pvhOtherRepoInfo)  );
	pMyPush->countRoundtrips++;
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pMyPush->pvhOtherRepoInfo, SG_SYNC_REPO_INFO_KEY__REPO_ID, &pszRefOtherRepoId)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pMyPush->pvhOtherRepoInfo, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &pszRefOtherAdminId)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pMyPush->pvhOtherRepoInfo, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, &pszRefOtherHashMethod)  );

	if (!bAdminOnly)
	{
		SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pThisRepo, &pszThisRepoId)  );
		if (strcmp(pszThisRepoId, pszRefOtherRepoId) != 0)
			SG_ERR_THROW(SG_ERR_REPO_ID_MISMATCH);
	}
	else
		pMyPush->bAdminOnly = SG_TRUE;

	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pThisRepo, &pszThisAdminId)  );
	if (strcmp(pszThisAdminId, pszRefOtherAdminId) != 0)
		SG_ERR_THROW(SG_ERR_ADMIN_ID_MISMATCH);

	SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pThisRepo, &pszThisHashMethod)  );
	if (strcmp(pszThisHashMethod, pszRefOtherHashMethod) != 0)
		SG_ERR_THROW(SG_ERR_REPO_HASH_METHOD_MISMATCH);

	/* Start the push operation */
	SG_ERR_CHECK(  SG_sync_client__push_begin(pCtx, pMyPush->pSyncClient, &pMyPush->pTempPathname, &pMyPush->pClientPushHandle)  );
	pMyPush->countRoundtrips++;

	/* Create a fragball */
    {
        char buf_filename[SG_TID_MAX_BUFFER_LENGTH];
        SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_filename, sizeof(buf_filename))  );
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pMyPush->pFragballPathname, pMyPush->pTempPathname, buf_filename)  );
    }

	SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pThisRepo, pMyPush->pFragballPathname, SG_TRUE, 2, &pMyPush->pfb)  );

	SG_RETURN_AND_NULL(pMyPush, ppMyPush);

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszThisRepoId);
	SG_NULLFREE(pCtx, pszThisHashMethod);
	SG_NULLFREE(pCtx, pszThisAdminId);
	SG_ERR_IGNORE(  _sg_push__nullfree(pCtx, &pMyPush)  );
}

static void _buildPushStats(SG_context* pCtx, _sg_push* pMyPush, const SG_vhash* pvhStatus, SG_vhash** ppvhStats)
{
	SG_vhash* pvhStats = NULL;
	SG_vhash* pvhRefCounts;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStatus, SG_SYNC_STATUS_KEY__COUNTS, &pvhRefCounts)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC__COPY(pCtx, &pvhStats, pvhRefCounts)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, pMyPush->countRoundtrips)  );

	SG_RETURN_AND_NULL(pvhStats, ppvhStats);

	/* common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhStats);
}

static void _check_potential_new_heads(
	SG_context* pCtx,
	const SG_rbtree* prbPotentialNewRemoteHeads,
	const SG_vhash* pvhNewNodes)
{
	char buf[SG_DAGNUM__BUF_MAX__HEX];
	SG_bool b;
	SG_rbtree_iterator* pit = NULL;

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VERSION_CONTROL, buf, sizeof(buf))  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhNewNodes, buf, &b)  );
	if (b)
	{
		SG_vhash* pvhRefVcDag;
		SG_bool bNext;
		const char* pszRefHidPotential = NULL;
		const char* pszRefBranchName = NULL;

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhNewNodes, buf, &pvhRefVcDag)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbPotentialNewRemoteHeads, &bNext, &pszRefHidPotential, (void**)&pszRefBranchName)  );
		while (bNext)
		{
			SG_bool bIsNew;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefVcDag, pszRefHidPotential, &bIsNew)  );
			if (bIsNew)
				SG_ERR_THROW2(  SG_ERR_PUSH_WOULD_CREATE_NEW_HEADS,  (pCtx, "%s", pszRefBranchName)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &bNext, &pszRefHidPotential, (void**)&pszRefBranchName)  );
		}

		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	}

	return;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

static void _add_dagnodes_until_connected(SG_context* pCtx, 
										  SG_bool bSkipBranchAmbiguityCheck,
										  SG_vhash** ppvh_status, 
										  _sg_push* pMyPush)
{
	SG_bool disconnected = SG_FALSE;
	SG_pathname* pPath_fragball = NULL;
	SG_rbtree* prb_missing_nodes = NULL;
	SG_dagfrag* pFrag = NULL;
	SG_rbtree_iterator* pit = NULL;
    SG_fragball_writer* pfb = NULL;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__DAGS, &disconnected)  );
	while (disconnected)
	{
		SG_vhash* pvhRefUnknownDagnodes = NULL;
		SG_uint32 i, count_dagnums;

		// There's at least one dag with connection problems.
        {
            char buf_filename[SG_TID_MAX_BUFFER_LENGTH];
            SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_filename, sizeof(buf_filename))  );
            SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_fragball, pMyPush->pTempPathname, buf_filename)  );
        }

        SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pMyPush->pRepo, pPath_fragball, SG_TRUE, 2, &pfb)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__DAGS, &pvhRefUnknownDagnodes)  );

		// For each dag, get the unknown nodes.
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefUnknownDagnodes, &count_dagnums)  );
		for (i=0; i<count_dagnums; i++)
		{
			const char* pszRefDagNum = NULL; 
			SG_uint32 iMissingFringeNodeCount;
			const SG_variant* pvRefMissingNodes = NULL;
			SG_vhash* pvhRefMissingNodes = NULL;

			// Get the dag's missing node vhash.
			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefUnknownDagnodes, i, &pszRefDagNum, &pvRefMissingNodes)  );

			SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pvRefMissingNodes, &pvhRefMissingNodes)  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefMissingNodes, &iMissingFringeNodeCount)  );
			if (iMissingFringeNodeCount > 0)
			{
				SG_uint64 iDagnum;
				SG_vhash* pvhRefAllLeaves;
				SG_vhash* pvhRefDagLeaves = NULL;
				SG_bool bHas;

				SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, pszRefDagNum, &iDagnum)  );
				SG_ERR_CHECK(  SG_vhash__get_keys_as_rbtree(pCtx, pvhRefMissingNodes, &prb_missing_nodes)  );

				// Get leaves of other repo
				SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__LEAVES, &bHas)  );
				if (bHas)
				{
					SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__LEAVES, &pvhRefAllLeaves)  );
					SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefAllLeaves, pszRefDagNum, &bHas)  );
					if (bHas)
						SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefAllLeaves, pszRefDagNum, &pvhRefDagLeaves)  );
				}

				SG_ERR_CHECK(  SG_sync__build_best_guess_dagfrag(pCtx, pMyPush->pRepo, iDagnum, 
					prb_missing_nodes, pvhRefDagLeaves, &pFrag)  );
				if (pFrag)
					SG_ERR_CHECK(  SG_fragball__write__frag(pCtx, pfb, pFrag)  );
				
				SG_RBTREE_NULLFREE(pCtx, prb_missing_nodes);
				SG_DAGFRAG_NULLFREE(pCtx, pFrag);
			}
		}

		SG_VHASH_NULLFREE(pCtx, *ppvh_status);

		SG_ERR_CHECK(  SG_sync_client__push_add(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, 
			SG_FALSE, &pPath_fragball, ppvh_status)  );
		pMyPush->countRoundtrips++;

		SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__DAGS, &disconnected)  );

#if TRACE_PUSH
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, *ppvh_status, "connecting dags loop status")  );
#endif
        SG_FRAGBALL_WRITER_NULLFREE(pCtx, pfb);
	}

	/* If we're checking for new remote heads, check for new nodes */
	if (!bSkipBranchAmbiguityCheck)
	{
		SG_bool b;
		SG_vhash* pvhRefNewNodes;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &b)  );
		if (b)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &pvhRefNewNodes)  );
			SG_ERR_CHECK(  _check_potential_new_heads(pCtx, pMyPush->prbPotentialNewRemoteHeads, pvhRefNewNodes)  );
		}
	}

	return;

fail:
    if (pPath_fragball)
    {
        SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_fragball)  );
        SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    }
	SG_RBTREE_NULLFREE(pCtx, prb_missing_nodes);
	SG_VHASH_NULLFREE(pCtx, *ppvh_status);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_FRAGBALL_WRITER_NULLFREE(pCtx, pfb);
}

static void _add_blobs_until_done(SG_context* pCtx, 
								  SG_vhash** ppvh_status,
								  _sg_push* pMyPush) 
{
	SG_bool need_blobs = SG_FALSE;
	SG_pathname* pPath_fragball = NULL;
	SG_bool bPopOp = SG_FALSE;
	SG_uint32 i;
    SG_fragball_writer* pfb = NULL;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__BLOBS, &need_blobs)  );

	for(i=0; need_blobs; i++)
	{
		SG_vhash* pvh;

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__BLOBS, &pvh)  );
        {
            char buf_filename[SG_TID_MAX_BUFFER_LENGTH];
            SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_filename, sizeof(buf_filename))  );
            SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_fragball, pMyPush->pTempPathname, buf_filename)  );
        }

        SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pMyPush->pRepo, pPath_fragball, SG_TRUE, 2, &pfb)  );
		SG_ERR_CHECK(  SG_sync__add_blobs_to_fragball(pCtx, pfb, pvh)  );
		SG_VHASH_NULLFREE(pCtx, *ppvh_status);

		if (i)
			SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Uploading user data", SG_LOG__FLAG__NONE)  );
		else
			SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Uploading changesets", SG_LOG__FLAG__NONE)  );
		bPopOp = SG_TRUE;

		SG_ERR_CHECK(  SG_sync_client__push_add(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, 
			SG_TRUE, &pPath_fragball, ppvh_status)  );
		pMyPush->countRoundtrips++;

		SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		bPopOp = SG_FALSE;

#if TRACE_PUSH
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, *ppvh_status, "blob addition loop status")  );
#endif

		SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvh_status, SG_SYNC_STATUS_KEY__BLOBS, &need_blobs)  );
        SG_FRAGBALL_WRITER_NULLFREE(pCtx, pfb);
	}

    if (pPath_fragball)
    {
        SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_fragball)  );
        SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    }

	return;

fail:
	if (bPopOp)
    {
		SG_log__pop_operation(pCtx);
    }
    if (pPath_fragball)
    {
        SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_fragball)  );
        SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    }
	SG_VHASH_NULLFREE(pCtx, *ppvh_status);
    SG_FRAGBALL_WRITER_NULLFREE(pCtx, pfb);
}

static void _add_all_leaves(
	SG_context*  pCtx,
    SG_fragball_writer* pfb
	)
{
    SG_repo* pRepo = NULL;
	SG_uint32  i;
	SG_uint32  count_dagnums = 0;
	SG_uint64* paDagNums     = NULL;
	SG_rbtree* prb_leaves    = NULL;

    SG_ERR_CHECK(  SG_fragball__get_repo(pCtx, pfb, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );

	for (i=0; i<count_dagnums; i++)
	{
		SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, paDagNums[i], &prb_leaves)  );
		SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pfb, paDagNums[i], prb_leaves)  );
		SG_RBTREE_NULLFREE(pCtx, prb_leaves);
	}

fail:
	SG_NULLFREE(pCtx, paDagNums);
	SG_RBTREE_NULLFREE(pCtx, prb_leaves);
}

static void _get_all_current_branches(SG_context* pCtx, SG_repo* pRepo, SG_vhash** ppvhBranches)
{
	SG_vhash* pvhBranchPile = NULL;
	SG_bool bHas;

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhBranchPile)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhBranchPile, "branches", &bHas)  );
	if (bHas)
	{
		SG_vhash* pvhRefBranchesInPush = NULL;
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhBranchPile, "branches", &pvhRefBranchesInPush)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC__COPY(pCtx, ppvhBranches, pvhRefBranchesInPush)  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
}

static void _check_branch_ambiguity(
	SG_context* pCtx,
	_sg_push* pMyPush)
{
	SG_bool bRemoteInfoIncludesBranches = SG_FALSE;
	SG_vhash* pvhSourceBranches = NULL;

	/* If branch data wasn't retrieved in our initial round-trip with the server 
	 * (when SG_sync_remote__get_repo_info gets called, in _push_init), these checks are going to
	 * succeed but mean nothing. So don't do that. Currently, push_all and push_begin both call 
	 * _push_init requesting the full details, so it's safe. */
	
#if TRACE_PUSH
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pMyPush->pvhOtherRepoInfo, "dest repo info")  );
#endif

	/* If there are no version control branches in the push, there's nothing to do. */
	{
		SG_uint32 count;

		if (!pMyPush->pvhBranchesInPush)
			return;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pMyPush->pvhBranchesInPush, &count)  );
		if (!count)
			return;
	}

#if TRACE_PUSH
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pMyPush->pvhBranchesInPush, "branches in push")  );
#endif

	/******************
	* Check branches
	*
	* This woefully complicated code is tested by st_push_vc_branches.
	******************/

	/* Might need these in the loops below. Let's grab them just once. */
	SG_ERR_CHECK(  _get_all_current_branches(pCtx, pMyPush->pRepo, &pvhSourceBranches)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pMyPush->pvhOtherRepoInfo, "branches", &bRemoteInfoIncludesBranches)  );
	if (bRemoteInfoIncludesBranches)
	{
		SG_vhash* pvhRefBranchInfo;
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pMyPush->pvhOtherRepoInfo, "branches", &pvhRefBranchInfo)  );
			
		/* It looks weird that "branches" repeats here, but that's correct. */
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranchInfo, "branches", &bRemoteInfoIncludesBranches)  );
		if (bRemoteInfoIncludesBranches)
		{
			SG_vhash* pvhRefBranches;
			SG_uint32 i, countBranchesInPush;
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefBranchInfo, "branches", &pvhRefBranches)  );

			/* Inside pvhRefBranches, we've got a vhash for each named branch, which looks like this:
			 * 
			 * "master" :
			 * {
			 *   "records" :
			 *	 {
			 *	   "75238f06fc7526966a3ef107b59fb2959df2970c" : "f44db3aac6e2ffbb8c4ff1e9b185051761a07112"
			 *	 },
			 *	 "values" :
			 *	 {
			 *	   "f44db3aac6e2ffbb8c4ff1e9b185051761a07112" : "75238f06fc7526966a3ef107b59fb2959df2970c"
			 *	 }
			 * }
			 */

			
			/*
			 * For each branch we push, our head(s) of that branch must be descendants of 
			 * existing heads in the destination repository.
			 */
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pMyPush->pvhBranchesInPush, &countBranchesInPush)  );
			for (i = 0; i < countBranchesInPush; i++)
			{
				const char* pszThisBranchName;
				SG_bool bDestHasBranch;
				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pMyPush->pvhBranchesInPush, i, &pszThisBranchName, NULL)  );
				SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranches, pszThisBranchName, &bDestHasBranch)  );
				if (bDestHasBranch)
				{
					SG_vhash* pvhRefSourceBranch;
					SG_vhash* pvhRefSourceBranchVals;
					SG_uint32 countSourceBranchVals;
					SG_bool destBranchesAreOldNews = SG_FALSE;

					SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhSourceBranches, pszThisBranchName, &pvhRefSourceBranch)  );
					SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefSourceBranch, "values", &pvhRefSourceBranchVals)  );
					SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefSourceBranchVals, &countSourceBranchVals)  );

					/* If the branch is unambiguous in our repo (it has just one head), we can take the following shortcut:
					 * 
					 * If the leaf node of the remote _branches_ DAG is present in the source branches DAG, then the user
					 * has already dealt with the branch ambiguity and we ignore all these subsequent checks. 
					 * 
					 * This also allows "re-inventing" a branch somewhere on a dagnode with no relationship to the original,
					 * so long as there's only one head, as described in X9177.*/
					if (countSourceBranchVals == 1)
					{
						SG_bool bHasLeaf = SG_FALSE;
						const char* pszRefHidLeafRemoteBranchDag;
						SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranchInfo, "leaf", &bHasLeaf)  );
						if (bHasLeaf)
						{
							SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhRefBranchInfo, "leaf", &pszRefHidLeafRemoteBranchDag)  );
							SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pMyPush->pRepo, pszRefHidLeafRemoteBranchDag, &destBranchesAreOldNews)  );
						}
					}
					
					if (!destBranchesAreOldNews)
					{
						SG_vhash* pvhRefThisBranchInDest;
						SG_vhash* pvhRefDestBranchVals;
						SG_uint32 j, countDestBranchVals;

						SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefBranches, pszThisBranchName, &pvhRefThisBranchInDest)  );
						SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefThisBranchInDest, "values", &pvhRefDestBranchVals)  );
						SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefDestBranchVals, &countDestBranchVals)  );

						/* Loop over the heads of this branch in the SOURCE repo:
						 * Each branch head we push must be a either a descendant of, or the same as, 
						 * a destination head of the same branch. */
						for (j = 0; j < countSourceBranchVals; j++)
						{
							const char* pszRefHidSourceBranchHead;
							SG_uint32 k;
							SG_dagquery_relationship dqRel;
							SG_bool bSrcHeadExistsInSrc = SG_FALSE;
							SG_bool bSourceHeadDefinitelyWillBeNew = SG_TRUE;

							SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefSourceBranchVals, j, &pszRefHidSourceBranchHead, NULL)  );
							SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pMyPush->pRepo, pszRefHidSourceBranchHead, &bSrcHeadExistsInSrc)  );

							/* If the changeset doesn't exist locally, we won't be pushing it. So we need not check it. */
							if (bSrcHeadExistsInSrc)
							{
								/* The source changeset must be a descendant of a branch head in the destination repo, 
								 * or it must match a destination head. */
								for (k = 0; k < countDestBranchVals; k++)
								{
									SG_bool bDestHeadExistsInSrc = SG_FALSE;
									const char* pszRefHidDestBranchHead;

									SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefDestBranchVals, k, &pszRefHidDestBranchHead, NULL)  );

									/* It's okay if there exists a remote head for this branch that we don't have, 
									 * as long as one of the following is true:
									 * - the source head is the same as the destination head
									 * - the source head is an ancestor of the destination head
									 * - the source head is a descendant of the destination head
									 * 
									 * To check this, first we need to know if we have the destination head changeset in the source repo. */
									SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pMyPush->pRepo, pszRefHidDestBranchHead, &bDestHeadExistsInSrc)  );
									if (bDestHeadExistsInSrc)
									{
										/* We do have the changeset. So we can check for the acceptable relationships. */
										SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pMyPush->pRepo, SG_DAGNUM__VERSION_CONTROL,
											pszRefHidDestBranchHead, pszRefHidSourceBranchHead, SG_FALSE, SG_FALSE, &dqRel)  );

										if (dqRel == SG_DAGQUERY_RELATIONSHIP__SAME 
											|| dqRel == SG_DAGQUERY_RELATIONSHIP__ANCESTOR
											|| dqRel == SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
										{
											bSourceHeadDefinitelyWillBeNew = SG_FALSE;
									
											SG_rbtree__remove(pCtx, pMyPush->prbPotentialNewRemoteHeads, pszRefHidSourceBranchHead);
											SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOT_FOUND);
									
											break;
										}
									}
									else
									{
										/* We don't have the current head of the destination repo, so we can't check its relationship
										 * to our current head. It may or may not be a new head in the destination, and we won't know
										 * until we start connecting dags, later.  So we save it here, and the DAG connecting code
										 * will remove it if it determines that the node is not new in the destination. */
										SG_ERR_CHECK(  SG_rbtree__update__with_pooled_sz(pCtx, pMyPush->prbPotentialNewRemoteHeads, 
											pszRefHidSourceBranchHead, pszThisBranchName)  );
										bSourceHeadDefinitelyWillBeNew = SG_FALSE;
									}
								}

								if (bSourceHeadDefinitelyWillBeNew)
									SG_ERR_THROW2( SG_ERR_PUSH_WOULD_CREATE_NEW_HEADS, (pCtx, "%s", pszThisBranchName) ); 
							} // dest branches are not old news

						} // for each source branch value
					}

				} // destination has this branch
				else
				{
					/* The destination does not have this branch. Ensure it's not ambiguous locally. */

					SG_vhash* pvhRefThisSourceBranch;
					SG_vhash* pvhRefThisSourceBranchValues;
					SG_uint32 countThisSourceBranchValues;

					SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhSourceBranches, pszThisBranchName, &pvhRefThisSourceBranch)  );
					SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefThisSourceBranch, "values", &pvhRefThisSourceBranchValues)  );
					SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefThisSourceBranchValues, &countThisSourceBranchValues)  );
					if (countThisSourceBranchValues > 1)
						SG_ERR_THROW2( SG_ERR_PUSH_WOULD_CREATE_NEW_HEADS, (pCtx, "%s", pszThisBranchName) );
				} // destination does not have this branch
			} // for each branch in push
		} // dest has any branches
	} // other repo info we retrieved includes branch info

	/* Common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhSourceBranches);
}

static void _cleanup_temp_dir(
	SG_context* pCtx,
	_sg_push* pMyPush)
{
	/* When committing to a local repo, there's only one temp/staging directory and SG_sync_remote cleans it up.
	   When committing to a remote repo, SG_sync_remote still removes the remote staging folder, but we have our 
	   own local temp directory that's ours to clean up. */
	if (pMyPush && pMyPush->pTempPathname)
	{
		SG_bool bExists;
		SG_ERR_CHECK_RETURN(  SG_fsobj__exists__pathname(pCtx, pMyPush->pTempPathname, &bExists, NULL, NULL)  );
		if (bExists)
			SG_ERR_CHECK_RETURN(  SG_fsobj__rmdir_recursive__pathname(pCtx, pMyPush->pTempPathname)  );
		SG_PATHNAME_NULLFREE(pCtx, pMyPush->pTempPathname);
	}
}

static void _push_commit(
	SG_context* pCtx,
	_sg_push* pMyPush,
	const char* pszRootOperationDescription,
	SG_bool bSkipBranchAmbiguityCheck,
	SG_vhash** ppvhStats)
{
	SG_vhash* pvh_status = NULL;
	SG_vhash* pvhFinalPrecommitStatusForStats = NULL;

    if (pMyPush->pfb)
    {
        SG_ERR_CHECK(  SG_fragball_writer__close(pCtx, pMyPush->pfb)  );
    }

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, 
		pszRootOperationDescription ? pszRootOperationDescription : "Pushing", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 4u, NULL)  );

	/* Push the initial fragball */
	/* Then check the status and use it to send more dagnodes until the dags connect */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Looking for changes")  );

	if (!bSkipBranchAmbiguityCheck)
		SG_ERR_CHECK(  _check_branch_ambiguity(pCtx, pMyPush)  );

	SG_ERR_CHECK(  SG_sync_client__push_add(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, SG_FALSE, 
		&pMyPush->pFragballPathname, &pvh_status)  );
	pMyPush->countRoundtrips++;
#if TRACE_PUSH
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_status, "_push_commit status after first round-trip")  );
#endif

	SG_ERR_CHECK(  _add_dagnodes_until_connected(pCtx, bSkipBranchAmbiguityCheck, &pvh_status, pMyPush)  );

	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* All necessary dagnodes are in the frag at this point.  Add the blobs the server said it was missing. */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Transferring data")  );
	SG_ERR_CHECK(  _add_blobs_until_done(pCtx, &pvh_status, pMyPush)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	if (ppvhStats)
	{
		SG_ERR_CHECK(  SG_sync_client__push_add(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, 
			SG_FALSE, NULL, &pvhFinalPrecommitStatusForStats)  );
		pMyPush->countRoundtrips++;
	}

	SG_VHASH_NULLFREE(pCtx, pvh_status);

	/* commit */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Committing changes")  );
	SG_ERR_CHECK(  SG_sync_client__push_commit(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, &pvh_status)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

#if TRACE_PUSH
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_status, "_push_commit status after commit")  );
#endif

	/* cleanup */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Cleaning up")  );
	SG_ERR_CHECK(  SG_sync_client__push_end(pCtx, pMyPush->pSyncClient, &pMyPush->pClientPushHandle)  );
	pMyPush->countRoundtrips++;

	SG_ERR_CHECK(  _cleanup_temp_dir(pCtx, pMyPush)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	if (ppvhStats)
		SG_ERR_CHECK(  _buildPushStats(pCtx, pMyPush, pvhFinalPrecommitStatusForStats, ppvhStats)  );

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_status);
	SG_VHASH_NULLFREE(pCtx, pvhFinalPrecommitStatusForStats);

	SG_ERR_IGNORE(  _cleanup_temp_dir(pCtx, pMyPush)  );

	SG_log__pop_operation(pCtx);
}

void SG_push__all(
	SG_context* pCtx, 
	SG_repo* pSrcRepo, 
	const char* psz_remote_repo_spec, 
	const char* psz_username,
	const char* psz_password,
	SG_bool bSkipBranchAmbiguityCheck,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats)
{
	_sg_push* pMyPush = NULL;
	SG_vhash* pvhBranchesInPush = NULL;

	SG_NULLARGCHECK(pSrcRepo);
	SG_NULLARGCHECK(psz_remote_repo_spec);

	SG_ERR_CHECK(  _push_init(pCtx, pSrcRepo, psz_remote_repo_spec, psz_username, psz_password, SG_TRUE, SG_FALSE, &pMyPush)  );

	SG_ERR_CHECK(  _add_all_leaves(pCtx, pMyPush->pfb)  );

	/* We're pushing everything, including the version control area, so we need to add all branches 
	 * and verify them. The verification sorts out branches we haven't actually changed. */
	if (!bSkipBranchAmbiguityCheck)
		SG_ERR_CHECK(  _get_all_current_branches(pCtx, pMyPush->pRepo, &pMyPush->pvhBranchesInPush)  );

	SG_ERR_CHECK(  _push_commit(pCtx, pMyPush, NULL, bSkipBranchAmbiguityCheck, ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPush->pvhOtherRepoInfo, ppvhAccountInfo)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_push__nullfree(pCtx, &pMyPush)  );
	SG_VHASH_NULLFREE(pCtx, pvhBranchesInPush);
}

void SG_push__begin(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_remote_repo_spec,
	const char* psz_username,
	const char* psz_password,
	SG_push** ppPush)
{
	_sg_push* pMyPush = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK(psz_remote_repo_spec);
	SG_NULLARGCHECK_RETURN(ppPush);

	SG_ERR_CHECK(  _push_init(pCtx, pRepo, psz_remote_repo_spec, psz_username, psz_password, SG_TRUE, SG_FALSE, &pMyPush)  );
	
	*ppPush = (SG_push*)pMyPush;

	return;
	
fail:
	SG_ERR_IGNORE(  _sg_push__nullfree(pCtx, &pMyPush)  );
}

void SG_push__add(
	SG_context* pCtx, 
	SG_push* pPush,
	SG_uint64 iDagnum,
	SG_rbtree* prbDagnodes)
{
	_sg_push* pMyPush = NULL;
	SG_uint32 countDagnums;
	SG_uint32 i;
	SG_uint64* dagnums;
	SG_bool isDagnumValid = SG_FALSE;
	SG_rbtree* prbLeaves = NULL;

	SG_NULLARGCHECK_RETURN(pPush);
	pMyPush = (_sg_push*)pPush;

	// Verify that the requested dagnum exists
	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pMyPush->pRepo, &countDagnums, &dagnums)  );
	for (i = 0; i < countDagnums; i++)
	{
		if (dagnums[i] == iDagnum)
		{
			isDagnumValid = SG_TRUE;
			break;
		}
	}
	if (!isDagnumValid)
	{
		char buf[SG_DAGNUM__BUF_MAX__HEX];
		SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagnum, buf, sizeof(buf))  );
		SG_ERR_THROW2(SG_ERR_NO_SUCH_DAG, (pCtx, "%s", buf));
	}

	/* Note that we don't look at the provided dagnum and dagnodes and try to add the appropriate 
	 * branches to pMyPush->pvhBranchesInPush. You shouldn't use this routine to add version control
	 * dagnodes. */

	if (prbDagnodes)
	{
		/* Add specified nodes to the initial fragball */
		SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, iDagnum, prbDagnodes)  );
	}
	else
	{
		/* No specific nodes were provided, so add the leaves */
		SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pMyPush->pRepo, iDagnum, &prbLeaves)  );
		SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, iDagnum, prbLeaves)  );
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, dagnums);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);

}

/**
 * Based on the version control dagnodes specified by an SG_rev_spec, add to the list of branches
 * that will be checked in the branch and lock check before commit.
 */
static void _add_to_branch_check_list_based_on_rev_spec(
	SG_context* pCtx, 
	_sg_push* pMyPush, 
	SG_rev_spec* pRevSpec, 
	SG_rbtree** pprbDagnodes)
{
	SG_uint32 countRevSpecs = 0;
	SG_stringarray* psaDagnodes = NULL;
	SG_rbtree* prbDagnodes = NULL;
	SG_stringarray* psaBranches = NULL;
	SG_stringarray* psaMissingInThisBranch = NULL;
	SG_vhash* pvhBranchPile = NULL;

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );

	/* If we're adding all dagnodes, add the list of all current branches to our list of branches in the push. */
	if (!countRevSpecs)
	{
		if (!pMyPush->pvhBranchesInPush)
		{
			/* If there are already branches in the list, just free the whole thing. 
			 * We're adding them all in one shot, below. 
			 * This is easier and probably faster than adding only those that are missing. */
			SG_VHASH_NULLFREE(pCtx, pMyPush->pvhBranchesInPush);
		}
		SG_ERR_CHECK(  _get_all_current_branches(pCtx, pMyPush->pRepo, &pMyPush->pvhBranchesInPush)  );
	}
	else
	{
		/* Specific dagnodes are being added to the push, so we only want to check their branches and locks. */
		SG_uint32 countBranchSpecs;

		if (!pMyPush->pvhBranchesInPush)
			SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pMyPush->pvhBranchesInPush, countRevSpecs, NULL, NULL)  );

		// we pass (&psaMissingInThisBranch) to silently omit the heads of
		// this branch that are not present in the local repo. See W6015.
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pMyPush->pRepo, pRevSpec, SG_TRUE, &psaDagnodes, &psaMissingInThisBranch)  );
		SG_STRINGARRAY_NULLFREE(pCtx, psaMissingInThisBranch);

		SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, pRevSpec, &countBranchSpecs)  );
		if (countBranchSpecs == countRevSpecs)
		{
			/* Only branches are specified. Add exactly those branches. */
			SG_uint32 countAllBranches, i;

			SG_ERR_CHECK(  SG_rev_spec__branches(pCtx, pRevSpec, &psaBranches)  );
			SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaBranches, &countAllBranches)  );
			for (i = 0; i < countAllBranches; i++)
			{
				const char* pszRefBranch;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaBranches, i, &pszRefBranch)  );
				SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pMyPush->pvhBranchesInPush, pszRefBranch)  );
			}
		}
		else
		{
			/* At least one rev spec is not a branch. Loop over all specified hids.
			 * If any hid is the tip of a branch, add that branch to the list. */

			SG_ERR_CHECK(  _get_all_current_branches(pCtx, pMyPush->pRepo, &pvhBranchPile)  );
			if (pvhBranchPile)
			{
				SG_bool bHas;

				SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhBranchPile, "values", &bHas)  );
				if (bHas)
				{
					SG_uint32 i, countDagnodes;
					const char* pszRefHid;
					SG_vhash* pvhRefBranchVals;

					SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhBranchPile, "values", &pvhRefBranchVals)  );

					SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaDagnodes, &countDagnodes)  );
					for (i = 0; i < countDagnodes; i++)
					{
						SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaDagnodes, i, &pszRefHid)  );
						SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranchVals, pszRefHid, &bHas)  );
						if (bHas)
						{
							/* A specified dagnode is the tip of at least one branch. 
							 * Add all of its branches to the list. */
							
							SG_uint32 j, countBranches;
							SG_vhash* pvhRefThisBranchVal;
							const char* pszRefBranch;

							SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefBranchVals, pszRefHid, &pvhRefThisBranchVal)  );
							SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefThisBranchVal, &countBranches)  );
							for (j = 0; j < countBranches; countBranches++)
							{
								SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefThisBranchVal, j, &pszRefBranch, NULL)  );
								SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pMyPush->pvhBranchesInPush, pszRefBranch)  );
							}
						}
					}
				}
			}
		}

		SG_ERR_CHECK(  SG_stringarray__to_rbtree_keys(pCtx, psaDagnodes, pprbDagnodes)  );
	}

	/* Common cleanup */
fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaDagnodes);
	SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
	SG_STRINGARRAY_NULLFREE(pCtx, psaBranches);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	SG_STRINGARRAY_NULLFREE(pCtx, psaMissingInThisBranch);
}

void SG_push__add__area__version_control(
	SG_context* pCtx, 
	SG_push* pPush,
	SG_rev_spec* pRevSpec)
{
	_sg_push* pMyPush = NULL;
	SG_uint32 countDagnums;
	SG_uint32 i;
	SG_uint64* dagnums = NULL;
	SG_rbtree* prbLeaves = NULL;
	SG_rbtree* prbDagnodes = NULL;

	SG_NULLARGCHECK_RETURN(pPush);
	pMyPush = (_sg_push*)pPush;

	SG_ERR_CHECK(  _add_to_branch_check_list_based_on_rev_spec(pCtx, pMyPush, pRevSpec, &prbDagnodes)  );

	/* Add all DAGs that belong to the version control area to the push. */
	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pMyPush->pRepo, &countDagnums, &dagnums)  );
	for (i = 0; i < countDagnums; i++)
	{
		if (SG_DAGNUM__GET_AREA_ID(dagnums[i]) == SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__VERSION_CONTROL))
		{
            if (
                    (SG_DAGNUM__VERSION_CONTROL == dagnums[i])
                    && prbDagnodes
               )
            {
                /* Add specified nodes to the initial fragball */
                SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, dagnums[i], prbDagnodes)  );
            }
            else
            {
                SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pMyPush->pRepo, dagnums[i], &prbLeaves)  );
                SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, dagnums[i], prbLeaves)  );
                SG_RBTREE_NULLFREE(pCtx, prbLeaves);
            }
		}
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, dagnums);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
}

void SG_push__add__admin(
	SG_context* pCtx, 
	SG_push* pPush)
{
	_sg_push* pMyPush = NULL;
	SG_uint32 countDagnums;
	SG_uint32 i;
	SG_uint64* dagnums = NULL;
	SG_rbtree* prbLeaves = NULL;
	SG_rbtree* prbDagnodes = NULL;

	SG_NULLARGCHECK_RETURN(pPush);
	pMyPush = (_sg_push*)pPush;

	/* Add all administrative DAGs to the push. */
	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pMyPush->pRepo, &countDagnums, &dagnums)  );
	for (i = 0; i < countDagnums; i++)
	{
		if (SG_DAGNUM__IS_ADMIN(dagnums[i]))
		{
			if (
				(SG_DAGNUM__VERSION_CONTROL == dagnums[i])
				&& prbDagnodes
				)
			{
				/* Add specified nodes to the initial fragball */
				SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, dagnums[i], prbDagnodes)  );
			}
			else
			{
				SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pMyPush->pRepo, dagnums[i], &prbLeaves)  );
				SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, dagnums[i], prbLeaves)  );
				SG_RBTREE_NULLFREE(pCtx, prbLeaves);
			}
		}
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, dagnums);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
}

void SG_push__add__area(
	SG_context* pCtx, 
	SG_push* pPush,
    const char* psz_area_name
    )
{
	_sg_push* pMyPush = NULL;
	SG_uint32 countDagnums;
	SG_uint32 i;
	SG_uint64* dagnums = NULL;
	SG_rbtree* prbLeaves = NULL;
    SG_uint64 area_id = 0;
    SG_vhash* pvh_areas = NULL;

	SG_NULLARGCHECK_RETURN(pPush);
	pMyPush = (_sg_push*)pPush;

	SG_ERR_CHECK(  SG_area__list(pCtx, pMyPush->pRepo, &pvh_areas)  );
	SG_vhash__get__uint64(pCtx, pvh_areas, psz_area_name, &area_id);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_REPLACE2(SG_ERR_VHASH_KEYNOTFOUND, SG_ERR_AREA_NOT_FOUND, (pCtx, "%s", psz_area_name));
		SG_ERR_CHECK_CURRENT;
	}


	if ( area_id == SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__VERSION_CONTROL) )
	{
		/* Add the list of all current branches to our list of branches in the push. */
		if (!pMyPush->pvhBranchesInPush)
		{
			/* If there are already branches in the list, just free the whole thing. 
			 * We're adding them all in one shot, below. 
			 * It's easier and probably faster than adding only those that are missing. */
			SG_VHASH_NULLFREE(pCtx, pMyPush->pvhBranchesInPush);
		}
		SG_ERR_CHECK(  _get_all_current_branches(pCtx, pMyPush->pRepo, &pMyPush->pvhBranchesInPush)  );
	}	
	
	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pMyPush->pRepo, &countDagnums, &dagnums)  );
	for (i = 0; i < countDagnums; i++)
	{
		if (SG_DAGNUM__GET_AREA_ID(dagnums[i]) == area_id)
		{
            SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pMyPush->pRepo, dagnums[i], &prbLeaves)  );
            SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, dagnums[i], prbLeaves)  );
            SG_RBTREE_NULLFREE(pCtx, prbLeaves);
		}
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, dagnums);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
    SG_VHASH_NULLFREE(pCtx, pvh_areas);
}

void SG_push__commit(
	SG_context* pCtx,
	SG_push** ppPush,
	SG_bool bSkipBranchAmbiguityCheck,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats
    )
{
	_sg_push* pMyPush = NULL;

	SG_NULL_PP_CHECK_RETURN(ppPush);
	pMyPush = (_sg_push*)*ppPush;

	SG_ERR_CHECK(  _push_commit(pCtx, pMyPush, NULL, bSkipBranchAmbiguityCheck, ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPush->pvhOtherRepoInfo, ppvhAccountInfo)  );

	SG_ERR_CHECK(  _sg_push__nullfree(pCtx, &pMyPush)  );
	*ppPush = NULL;

fail:
	return;
}

void SG_push__abort(
	SG_context* pCtx,
	SG_push** ppPush)
{
	_sg_push* pMyPush = NULL;

	SG_NULL_PP_CHECK_RETURN(ppPush);
	pMyPush = (_sg_push*)*ppPush;

	/* Errors on cleanup are deliberately ignored here. If this is called, something has gone
	 * wrong or we've been interrupted at an arbitrary point in the push. This is the "do the best 
	 * you can to clean everything up" routine, so we attempt to clean everything up. */

    if (pMyPush->pfb)
    {
        SG_ERR_IGNORE(  SG_fragball_writer__close(pCtx, pMyPush->pfb)  );
    }

	SG_ERR_IGNORE(  SG_sync_client__push_end(pCtx, pMyPush->pSyncClient, &pMyPush->pClientPushHandle)  );

	if (pMyPush->pTempPathname)
    {
		SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pMyPush->pTempPathname)  );
    }

	SG_ERR_IGNORE(  _sg_push__nullfree(pCtx, &pMyPush)  );
	*ppPush = NULL;
}

void SG_push__list_outgoing_vc(
	SG_context* pCtx, 
	SG_repo* pSrcRepo, 
	const char* psz_remote_repo_spec, 
	const char* psz_username,
	const char* psz_password,
	SG_history_result** ppInfo,
	SG_vhash** ppvhStats)
{
	SG_string* pstrOperation = NULL;
	_sg_push* pMyPush = NULL;
	SG_rbtree* prbVcLeaves = NULL;
	SG_vhash* pvh_status = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrOperation)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrOperation, "Outgoing to ")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrOperation, psz_remote_repo_spec)  );
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, SG_string__sz(pstrOperation), SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 2u, NULL)  );

	SG_NULLARGCHECK(pSrcRepo);
	SG_NULLARGCHECK(psz_remote_repo_spec);

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Looking for changes")  );
	SG_ERR_CHECK(  _push_init(pCtx, pSrcRepo, psz_remote_repo_spec, psz_username, psz_password, SG_FALSE, SG_FALSE, &pMyPush)  );

	/* Outgoing only shows version control changes, so add only the leaves of the version control DAG to the push. */
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pMyPush->pRepo, SG_DAGNUM__VERSION_CONTROL, &prbVcLeaves)  );
	SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pMyPush->pfb, SG_DAGNUM__VERSION_CONTROL, prbVcLeaves)  );

	SG_ERR_CHECK(  SG_sync_client__push_add(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, SG_FALSE, 
		&pMyPush->pFragballPathname, &pvh_status)  );
	pMyPush->countRoundtrips++;
#if TRACE_PUSH
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_status, "outgoing status after first round-trip")  );
#endif
	SG_ERR_CHECK(  _add_dagnodes_until_connected(pCtx, SG_TRUE, &pvh_status, pMyPush)  );
	SG_VHASH_NULLFREE(pCtx, pvh_status);
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* An add with no fragball does a complete status check */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Building changeset descriptions")  );
	SG_ERR_CHECK(  SG_sync_client__push_add(pCtx, pMyPush->pSyncClient, pMyPush->pClientPushHandle, SG_FALSE, NULL, &pvh_status)  );
	pMyPush->countRoundtrips++;
#if TRACE_PUSH
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_status, "outgoing status")  );
#endif
	{
		SG_bool b = SG_FALSE;
		SG_vhash* pvhRequest = NULL;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &b)  );
		if (b)
		{
			/* There are outgoing nodes.  Get their info. */
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &pvhRequest)  );
			SG_ERR_CHECK(  SG_sync_remote__get_dagnode_info(pCtx, pSrcRepo, pvhRequest, ppInfo)  );
			pMyPush->countRoundtrips++;
		}
	}
    if (pMyPush->pfb)
    {
        SG_ERR_CHECK(  SG_fragball_writer__close(pCtx, pMyPush->pfb)  );
    }

	SG_ERR_CHECK(  SG_sync_client__push_end(pCtx, pMyPush->pSyncClient, &pMyPush->pClientPushHandle)  );
	pMyPush->countRoundtrips++;
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	if (ppvhStats)
		SG_ERR_CHECK(  _buildPushStats(pCtx, pMyPush, pvh_status, ppvhStats)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_push__nullfree(pCtx, &pMyPush)  );
	SG_RBTREE_NULLFREE(pCtx, prbVcLeaves);
	SG_VHASH_NULLFREE(pCtx, pvh_status);
	SG_log__pop_operation(pCtx);
	SG_STRING_NULLFREE(pCtx, pstrOperation); // must be after the operation pop
}

void SG_push__admin(
	SG_context* pCtx, 
	SG_repo* pSrcRepo, 
	const char* psz_remote_repo_spec, 
	const char* psz_username,
	const char* psz_password,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats)
{
	_sg_push* pMyPush = NULL;
	SG_vhash* pvhBranchesInPush = NULL;

	SG_NULLARGCHECK(pSrcRepo);
	SG_NULLARGCHECK(psz_remote_repo_spec);

	SG_ERR_CHECK(  _push_init(pCtx, pSrcRepo, psz_remote_repo_spec, psz_username, psz_password, SG_FALSE, SG_TRUE, &pMyPush)  );

	SG_ERR_CHECK(  SG_push__add__admin(pCtx, (SG_push*)pMyPush)  );

	SG_ERR_CHECK(  _push_commit(pCtx, pMyPush, "Pushing user database", SG_TRUE, ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPush->pvhOtherRepoInfo, ppvhAccountInfo)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_push__nullfree(pCtx, &pMyPush)  );
	SG_VHASH_NULLFREE(pCtx, pvhBranchesInPush);
}
