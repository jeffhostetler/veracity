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

#define TRACE_PULL 0

/* This exists primarily as a convenience so policy/credential data can be added to it later. 
 * It's only used in this file, there's no opaque wrapper. */
typedef struct
{
	char* pszPullId;
	SG_staging* pStaging;
	SG_repo* pPullIntoRepo; // we don't own this
    SG_vhash* pvh_remote_repo_info;
	SG_bool bAdminOnly; /* True if repo ids don't match but admin ids do. */
    char* psz_remote_repo_spec;

	SG_uint32 countRoundtrips;
} sg_pull_instance_data;


/* This is the structure for which SG_pull is an opaque wrapper. */
typedef struct
{
	SG_sync_client* pSyncClient;
	SG_vhash* pvhFragballRequest;
	sg_pull_instance_data* pPullInstance;
} _sg_pull;

static void _free_instance_data(SG_context* pCtx,
								sg_pull_instance_data* pMe)
{
	if (pMe)
	{
		SG_NULLFREE(pCtx, pMe->pszPullId);
		SG_STAGING_NULLFREE(pCtx, pMe->pStaging);
        SG_VHASH_NULLFREE(pCtx, pMe->pvh_remote_repo_info);
        SG_NULLFREE(pCtx, pMe->psz_remote_repo_spec);
		SG_NULLFREE(pCtx, pMe);
	}
}
#define _NULLFREE_INSTANCE_DATA(pCtx,p) SG_STATEMENT(  _free_instance_data(pCtx, p); p=NULL;  )

static void _sg_pull__nullfree(SG_context* pCtx, _sg_pull** ppMyPull)
{
	if (ppMyPull && *ppMyPull)
	{
		_sg_pull* pMyPull = *ppMyPull;
		SG_VHASH_NULLFREE(pCtx, pMyPull->pvhFragballRequest);

		if (pMyPull->pPullInstance)
			SG_ERR_IGNORE(  SG_staging__cleanup(pCtx, &pMyPull->pPullInstance->pStaging)  );

		SG_ERR_IGNORE(  _free_instance_data(pCtx, pMyPull->pPullInstance)  );
		SG_SYNC_CLIENT_NULLFREE(pCtx, pMyPull->pSyncClient);
		SG_NULLFREE(pCtx, pMyPull);
		*ppMyPull = NULL;
	}
}


static void _buildPullStats(SG_context* pCtx, _sg_pull* pMyPull, const SG_vhash* pvhStatus, SG_vhash** ppvhStats)
{
	SG_vhash* pvhStats = NULL;
	SG_vhash* pvhRefCounts;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStatus, SG_SYNC_STATUS_KEY__COUNTS, &pvhRefCounts)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC__COPY(pCtx, &pvhStats, pvhRefCounts)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, pMyPull->pPullInstance->countRoundtrips)  );

	SG_RETURN_AND_NULL(pvhStats, ppvhStats);

	/* common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhStats);
}

/**
 * Creates a staging area and returns an initialized instance data structure.
 */
static void _pull_init(SG_context* pCtx, 
					   SG_repo* pPullIntoRepo,
					   const char* pszRemoteRepoSpec, // you must provide pszRemoteRepoSpec xor pRemoteRepo
					   SG_repo* pRemoteRepo,
					   const char* pszUsername,
					   const char* pszPassword,
					   SG_bool bAdminOnly,
					   _sg_pull** ppMyPull)
{
	char* pszThisRepoId = NULL;
	char* pszThisAdminId = NULL;
	char* pszThisHashMethod = NULL;
	const char* pszRefOtherRepoId = NULL;
	const char* pszRefOtherAdminId = NULL;
	const char* pszRefOtherHashMethod = NULL;

	sg_pull_instance_data* pMe = NULL;
	_sg_pull* pMyPull = NULL;
	
	SG_NULLARGCHECK_RETURN(pPullIntoRepo);
	SG_NULLARGCHECK_RETURN(ppMyPull);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pMyPull)  );
	SG_ERR_CHECK(  SG_alloc1(pCtx, pMe)  );
	pMyPull->pPullInstance = pMe;


	if (pszRemoteRepoSpec)
	{
		if (pRemoteRepo)
        {
			SG_ERR_THROW(SG_ERR_INVALIDARG);
        }
        SG_ERR_CHECK(  SG_strdup(pCtx, pszRemoteRepoSpec, &(pMe->psz_remote_repo_spec))  );
		SG_ERR_CHECK(  SG_sync_client__open(pCtx, pszRemoteRepoSpec, pszUsername, pszPassword, &pMyPull->pSyncClient)  );
	}
	else
	{
		if (!pRemoteRepo)
        {
			SG_ERR_THROW(SG_ERR_INVALIDARG);
        }
		SG_ERR_CHECK(  SG_sync_client__open__local(pCtx, pRemoteRepo, &pMyPull->pSyncClient)  );
	}

    // TODO in the line below, decide whether we need to request the area list or not
	SG_ERR_CHECK(  SG_sync_client__get_repo_info(pCtx, pMyPull->pSyncClient, SG_FALSE, SG_TRUE, &pMe->pvh_remote_repo_info)  );
	pMe->countRoundtrips++;
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pMe->pvh_remote_repo_info, SG_SYNC_REPO_INFO_KEY__REPO_ID, &pszRefOtherRepoId)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pMe->pvh_remote_repo_info, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &pszRefOtherAdminId)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pMe->pvh_remote_repo_info, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, &pszRefOtherHashMethod)  );

	pMe->pPullIntoRepo = pPullIntoRepo;
	if (!bAdminOnly)
	{
		SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pMe->pPullIntoRepo, &pszThisRepoId)  );
		if (strcmp(pszThisRepoId, pszRefOtherRepoId) != 0)
			SG_ERR_THROW(SG_ERR_REPO_ID_MISMATCH);
	}
	else
		pMyPull->pPullInstance->bAdminOnly = SG_TRUE;

	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pMe->pPullIntoRepo, &pszThisAdminId)  );
	if (strcmp(pszThisAdminId, pszRefOtherAdminId) != 0)
		SG_ERR_THROW(SG_ERR_ADMIN_ID_MISMATCH);

	SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pMe->pPullIntoRepo, &pszThisHashMethod)  );
	if (strcmp(pszThisHashMethod, pszRefOtherHashMethod) != 0)
		SG_ERR_THROW(SG_ERR_REPO_HASH_METHOD_MISMATCH);

	// Store pull id (which identifies the staging area) in instance data
	SG_ERR_CHECK(  SG_staging__create(pCtx, &pMe->pszPullId, &pMe->pStaging)  );

	SG_RETURN_AND_NULL(pMyPull, ppMyPull);

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszThisRepoId);
	SG_NULLFREE(pCtx, pszThisAdminId);
	SG_NULLFREE(pCtx, pszThisHashMethod);
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
}

/**
 * Based on the current contents of our staging area, add dagnodes until the DAGs connect. 
 * May do several roundtrips with the other repo.
 */
static void _add_dagnodes_until_connected(SG_context* pCtx, 
										  SG_vhash** ppvhStagingStatus, 
										  sg_pull_instance_data* pMe, 
										  SG_sync_client* pClient)
{
	SG_bool disconnected = SG_FALSE;
	SG_vhash* pvhRequest = NULL;
	char* pszFragballName = NULL;
	SG_pathname* pStagingPathname;
	
	SG_rev_spec* pRevSpec = NULL;
	SG_vhash* pvhRevSpec = NULL;
	SG_vhash* pvhRequestDags = NULL;
	SG_vhash* pvhRequestRevs = NULL;
	
	SG_vhash* pvhAllLeaves = NULL;
	SG_vhash* pvhDagLeaves = NULL;
	SG_rbtree* prbLeaves = NULL;
	SG_rbtree_iterator* pit = NULL;

	SG_repo_fetch_dagnodes_handle* pdh = NULL;
	SG_dagnode* pdn = NULL;

	SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, pMe->pszPullId, &pStagingPathname)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvhStagingStatus, SG_SYNC_STATUS_KEY__DAGS, &disconnected)  );
	while (disconnected)
	{

#if TRACE_PULL
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, *ppvhStagingStatus, "pull staging status")  );
#endif
		// There's at least one disconnected DAG.  
		
		// Create a fragball request vhash from the staging status vhash.
		SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhRequest, 2, NULL, NULL)  ); // 2 nodes: gens and dags

		{
			SG_vhash* pvhRefStatusDags;
			SG_vhash* pvhRefStatusDag;
			const char* pszRefDagnum;
			SG_uint32 countDags, i;

			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, *ppvhStagingStatus, SG_SYNC_STATUS_KEY__DAGS, &pvhRefStatusDags)  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefStatusDags, &countDags)  );
			SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhRequestDags, countDags, NULL, NULL)  ); // a node for each dag
			SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhAllLeaves, countDags, NULL, NULL)  ); // a node for each dag

			for (i = 0; i < countDags; i++)
			{
				/* For each disconnected DAG:
				 *  - create a rev spec, serialize the rev spec to a vhash, add that vhash to the request. 
				 *  - add leaves of that dag and their generation, so other repo can build a "best guess" dagfrag. */

				const char* pszRefHid;
				SG_uint32 countHids, j;
				SG_uint64 dagnum;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvhRefStatusDags, i, &pszRefDagnum, &pvhRefStatusDag)  );
				SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefStatusDag, &countHids)  );	
				SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, pszRefDagnum, &dagnum)  );

				SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
				SG_ERR_CHECK(  SG_rev_spec__set_dagnum(pCtx, pRevSpec, dagnum)  );
				for (j = 0; j < countHids; j++)
				{
					SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefStatusDag, j, &pszRefHid, NULL)  );
					SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRefHid)  );
				}
				SG_ERR_CHECK(  SG_rev_spec__to_vhash(pCtx, pRevSpec, SG_TRUE, &pvhRevSpec)  );
				SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhRequestDags, pszRefDagnum, &pvhRevSpec )  );
				SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);

				/* Add leaves and gens */
				{
					SG_uint32 countLeaves;
					SG_int32 gen;
					SG_bool b;

					SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pMe->pPullIntoRepo, dagnum, &prbLeaves)  );
					SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbLeaves, &countLeaves)  );
					SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhDagLeaves, countLeaves, NULL, NULL)  );

					SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pMe->pPullIntoRepo, dagnum, &pdh)  );
					SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbLeaves, &b, &pszRefHid, NULL)  );
					while (b)
					{
						SG_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pMe->pPullIntoRepo, pdh, pszRefHid, &pdn)  );
						SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
						SG_DAGNODE_NULLFREE(pCtx, pdn);
						SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhDagLeaves, pszRefHid, gen)  );
						SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &pszRefHid, NULL)  );
					}
					SG_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pMe->pPullIntoRepo, &pdh)  );
					SG_RBTREE_NULLFREE(pCtx, prbLeaves);
					SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

					SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhAllLeaves, pszRefDagnum, &pvhDagLeaves)  );
				}
			}

			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__DAGS, &pvhRequestDags)  );

			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__LEAVES, &pvhAllLeaves)  );
		}

		SG_ERR_CHECK(  SG_sync_client__pull_request_fragball(pCtx, pClient, pvhRequest, 
			SG_FALSE, pStagingPathname, &pszFragballName)  );
		pMe->countRoundtrips++;

		SG_VHASH_NULLFREE(pCtx, pvhRequest);

		SG_ERR_CHECK(  SG_staging__slurp_fragball(pCtx, pMe->pStaging, (const char*)pszFragballName)  );
		SG_NULLFREE(pCtx, pszFragballName);

		SG_VHASH_NULLFREE(pCtx, *ppvhStagingStatus);
		SG_ERR_CHECK(  SG_staging__check_status(pCtx, pMe->pStaging, pMe->pPullIntoRepo,
			SG_TRUE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, 
			ppvhStagingStatus)  );

		SG_ERR_CHECK(  SG_vhash__has(pCtx, *ppvhStagingStatus, SG_SYNC_STATUS_KEY__DAGS, &disconnected)  );

#if TRACE_PULL
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, *ppvhStagingStatus, "pull staging status")  );
#endif
	}

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pStagingPathname);
	SG_VHASH_NULLFREE(pCtx, *ppvhStagingStatus);
	SG_VHASH_NULLFREE(pCtx, pvhRequest);
	SG_NULLFREE(pCtx, pszFragballName);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_VHASH_NULLFREE(pCtx, pvhRevSpec);
	SG_VHASH_NULLFREE(pCtx, pvhRequestDags);
	SG_VHASH_NULLFREE(pCtx, pvhRequestRevs);
	SG_VHASH_NULLFREE(pCtx, pvhAllLeaves);
	SG_VHASH_NULLFREE(pCtx, pvhDagLeaves);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	if (pdh && pMe && pMe->pPullIntoRepo)
		SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pMe->pPullIntoRepo, &pdh)  );
}

/**
 * Based on the current contents of our staging area, request all the blobs we need to complete the pull.
 * May do several roundtrips with the other repo.
 */
static void _add_blobs_until_done(SG_context* pCtx, 
								  _sg_pull* pMyPull) 
{
	SG_staging* pStaging = pMyPull->pPullInstance->pStaging;
	SG_sync_client* pClient = pMyPull->pSyncClient;
	sg_pull_instance_data* pMe = pMyPull->pPullInstance;
	
	SG_bool need_blobs = SG_FALSE;
	SG_vhash* pvhFragballRequest = NULL;
	char* pszFragballName = NULL;
	SG_pathname* pStagingPathname = NULL;
	SG_vhash* pvhStagingStatus = NULL;

	SG_bool bPopOp = SG_FALSE;
	SG_uint32 i;

	SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pMe->pPullIntoRepo,
		SG_FALSE, SG_FALSE, SG_TRUE, SG_TRUE, SG_FALSE, 
		&pvhStagingStatus)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhStagingStatus, SG_SYNC_STATUS_KEY__BLOBS, &need_blobs)  );
	
	if (need_blobs)
		SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, pMe->pszPullId, &pStagingPathname)  );

	for (i = 0; need_blobs; i++)
	{
		pvhFragballRequest = pvhStagingStatus;
		pvhStagingStatus = NULL;

        SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Downloading blobs", SG_LOG__FLAG__NONE)  );
		bPopOp = SG_TRUE;

		SG_ERR_CHECK(  SG_sync_client__pull_request_fragball(pCtx, pClient, pvhFragballRequest, SG_TRUE, pStagingPathname, &pszFragballName)  );
		pMe->countRoundtrips++;
		SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		bPopOp = SG_FALSE;

		SG_VHASH_NULLFREE(pCtx, pvhFragballRequest);

		SG_ERR_CHECK(  SG_staging__slurp_fragball(pCtx, pStaging, (const char*)pszFragballName)  );
		SG_NULLFREE(pCtx, pszFragballName);

		SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pMe->pPullIntoRepo,
			SG_FALSE, SG_FALSE, SG_TRUE, SG_TRUE, SG_FALSE, 
			&pvhStagingStatus)  );

#if TRACE_PULL
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStagingStatus, "pull staging status")  );
#endif

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhStagingStatus, SG_SYNC_STATUS_KEY__BLOBS, &need_blobs)  );
	}

	/* fall through */
fail:
	if (bPopOp)
		SG_log__pop_operation(pCtx);
	SG_PATHNAME_NULLFREE(pCtx, pStagingPathname);
	SG_VHASH_NULLFREE(pCtx, pvhStagingStatus);
	SG_VHASH_NULLFREE(pCtx, pvhFragballRequest);
	SG_NULLFREE(pCtx, pszFragballName);
}

/**
 * Pulls the requested dagnodes into our staging area.
 * If pMyPull->pvhFragballRequest is NULL, the leaves of every dag in the remote repo are pulled.
 */
static void _add_dagnodes(
	SG_context*        pCtx,
	_sg_pull*		   pMyPull,
	SG_vhash**         ppStatus
	)
{
	sg_pull_instance_data* pMe  = pMyPull->pPullInstance;
	SG_staging*        pStaging = pMe->pStaging;
	SG_sync_client*    pClient  = pMyPull->pSyncClient;

	SG_pathname* pStagingPathname = NULL;
	char*              pszFragballName  = NULL;
	SG_vhash*          pvhStatus        = NULL;

	SG_NULLARGCHECK_RETURN(pStaging);
	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppStatus);

	SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, pMe->pszPullId, &pStagingPathname)  );

	SG_sync_client__pull_request_fragball(pCtx, pClient, pMyPull->pvhFragballRequest, 
		SG_FALSE, pStagingPathname, &pszFragballName);
	if(SG_context__err_equals(pCtx, SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT))
	{
		SG_ERR_RESET_THROW2(SG_ERR_PULL_FAILED, (pCtx, 
			"The branch cannot be pulled from the requested repository because the repository is "
			"missing the latest changeset(s) that it already knows should be in the branch."));
	}
	else
		SG_ERR_CHECK_CURRENT;
	pMyPull->pPullInstance->countRoundtrips++;

	SG_ERR_CHECK(  SG_staging__slurp_fragball(pCtx, pStaging, pszFragballName)  );

	SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pMe->pPullIntoRepo,
		SG_TRUE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, 
		&pvhStatus)  );
#if TRACE_PULL
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStatus, "pull staging status")  );
#endif

	*ppStatus = pvhStatus;
	pvhStatus = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pStagingPathname);
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
	SG_NULLFREE(pCtx, pszFragballName);
}

/**
 * Pulls the requested dagnodes into our staging area, then roundtrips adding dagnodes until
 * the DAGS connect.
 * A NULL pMyPull->pvhFragballRequest will pull all missing dagnodes from every DAG.
 */
static void _add_dagnodes_and_connect(
	SG_context*            pCtx,
	_sg_pull*			   pMyPull
	)
{
	SG_vhash* pvhStatus = NULL;

	SG_ERR_CHECK(  _add_dagnodes(pCtx, pMyPull, &pvhStatus)  );
	SG_ERR_CHECK(  _add_dagnodes_until_connected(pCtx, &pvhStatus, pMyPull->pPullInstance, pMyPull->pSyncClient)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
}

/**
 * 
 */
static void _do_pull(
	SG_context* pCtx,
	_sg_pull* pMyPull,
	const char* pszRootOperationDescription,
	SG_vhash** ppvhStats)
{
	sg_pull_instance_data* pMe = pMyPull->pPullInstance;
	SG_vhash* pvhFinalPrecommitStatusForStats = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, 
		pszRootOperationDescription ? pszRootOperationDescription : "Pulling", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 5u, NULL)  );

	SG_NULLARGCHECK(pMyPull);

	/* Request a fragball containing leaves of every dag */
	/* Then check the status and use it to request more dagnodes until the dags connect */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Looking for changes")  );
	SG_ERR_CHECK(  _add_dagnodes_and_connect(pCtx, pMyPull)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* All necessary dagnodes are in the frag at this point.  Now get remaining missing blobs. */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Transferring data")  );
	SG_ERR_CHECK(  _add_blobs_until_done(pCtx, pMyPull)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	if (ppvhStats)
	{
		SG_ERR_CHECK(  SG_staging__check_status(pCtx, pMe->pStaging, pMe->pPullIntoRepo, 
			SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, SG_TRUE, &pvhFinalPrecommitStatusForStats)  );
	}

	/* commit */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Committing changes")  );
	SG_ERR_CHECK(  SG_staging__commit(pCtx, pMe->pStaging, pMe->pPullIntoRepo)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* cleanup */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Cleaning up")  );
	SG_ERR_CHECK(  SG_staging__cleanup(pCtx, &pMe->pStaging)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* auto-merge zing dags */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Merging databases")  );
	SG_ERR_CHECK(  SG_zing__auto_merge_all_dags(pCtx, pMe->pPullIntoRepo)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	if (ppvhStats)
		SG_ERR_CHECK(  _buildPullStats(pCtx, pMyPull, pvhFinalPrecommitStatusForStats, ppvhStats)  );

	/* fall through */
fail:
	SG_log__pop_operation(pCtx);
	SG_VHASH_NULLFREE(pCtx, pvhFinalPrecommitStatusForStats);
}

#if defined(SG_IOS)
static void _pull_leaves_for_every_dag(
	SG_context*        pCtx,
	_sg_pull*		   pMyPull,
	SG_vhash**         ppStatus,
    char**             ppsz_fragball_leaves
	)
{
	sg_pull_instance_data* pMe  = pMyPull->pPullInstance;
	SG_staging*        pStaging = pMe->pStaging;
	SG_sync_client*    pClient  = pMyPull->pSyncClient;

	SG_pathname* pStagingPathname = NULL;
	char*              pszFragballName  = NULL;
	SG_vhash*          pvhStatus        = NULL;

	SG_NULLARGCHECK_RETURN(pStaging);
	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppStatus);

	SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, pMe->pszPullId, &pStagingPathname)  );

	SG_sync_client__pull_request_fragball(pCtx, pClient, pMyPull->pvhFragballRequest, 
		SG_FALSE, pStagingPathname, &pszFragballName);
	if(SG_context__err_equals(pCtx, SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT))
	{
		SG_ERR_RESET_THROW2(SG_ERR_PULL_FAILED, (pCtx, 
			"The branch cannot be pulled from the requested repository because the repository is "
			"missing the latest changeset(s) that it already knows should be in the branch."));
	}
	else
		SG_ERR_CHECK_CURRENT;
	pMyPull->pPullInstance->countRoundtrips++;

	SG_ERR_CHECK(  SG_staging__slurp_fragball(pCtx, pStaging, pszFragballName)  );

	SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pMe->pPullIntoRepo,
		SG_TRUE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, 
		&pvhStatus)  );
#if TRACE_PULL
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStatus, "pull staging status")  );
#endif

	*ppStatus = pvhStatus;
	pvhStatus = NULL;

    *ppsz_fragball_leaves = pszFragballName;
    pszFragballName = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pStagingPathname);
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
	SG_NULLFREE(pCtx, pszFragballName);
}

void _pull_since_last_time(
	SG_context* pCtx,
	_sg_pull* pMyPull,
    SG_vhash** ppvh_since
    )
{
    SG_vhash* pvh_request = NULL;
	sg_pull_instance_data* pMe  = pMyPull->pPullInstance;
	SG_staging*        pStaging = pMe->pStaging;
	SG_sync_client*    pClient  = pMyPull->pSyncClient;

	SG_pathname* pStagingPathname = NULL;
	char*              pszFragballName  = NULL;

	SG_NULLARGCHECK_RETURN(pStaging);
	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppvh_since);

    SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, pMe->pszPullId, &pStagingPathname)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_request)  );
    SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_request, SG_SYNC_STATUS_KEY__SINCE, ppvh_since)  );

    SG_ERR_CHECK(  SG_sync_client__pull_request_fragball(pCtx, pClient, pvh_request, 
        SG_TRUE, pStagingPathname, &pszFragballName)  );

    pMyPull->pPullInstance->countRoundtrips++;

    SG_ERR_CHECK(  SG_staging__slurp_fragball(pCtx, pStaging, pszFragballName)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pStagingPathname);
	SG_NULLFREE(pCtx, pszFragballName);
    SG_VHASH_NULLFREE(pCtx, pvh_request);
}

static void _leaf_scan__one_frag(
	SG_context* pCtx,
    SG_vhash* pvh_frag,
    SG_vhash* pvh_leaves
	)
{
	SG_uint64 dagnum;
	SG_dagfrag* pFrag = NULL;
	SG_bool bSkipDag = SG_FALSE;
    SG_varray* pva_members = NULL;

	SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
	SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &dagnum)  );

	SG_ERR_CHECK(  SG_zing__missing_hardwired_template(pCtx, dagnum, &bSkipDag)  );
	if (!bSkipDag)
	{
        char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

		SG_ERR_CHECK_RETURN(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );

        SG_ERR_CHECK(  SG_dagfrag__get_members__not_including_the_fringe(pCtx, pFrag, &pva_members)  );

        SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_leaves, buf_dagnum, &pva_members)  );
	}

	/* fall through */
fail:
    SG_VARRAY_NULLFREE(pCtx, pva_members);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
}

static void _leaf_scan__version_2(
	SG_context* pCtx,
	SG_file* pfb,
    SG_vhash* pvh_leaves
	)
{
    SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(pfb);

	while (1)
	{
		SG_vhash* pvh_frag = NULL;
		SG_uint64 offset_begin_record = 0;
        SG_uint64 iPos = 0;
        const char* psz_op = NULL;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pfb, &offset_begin_record)  );
		SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfb, &pvh)  );
		if (!pvh)
		{
			break;
		}
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &psz_op)  );
		if (0 == strcmp(psz_op, "frag"))
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "frag", &pvh_frag)  );

            SG_ERR_CHECK(  _leaf_scan__one_frag(pCtx, pvh_frag, pvh_leaves)  );
		}
		else if (0 == strcmp(psz_op, "audits"))
        {
        }
		else if (0 == strcmp(psz_op, "blob"))
		{
			SG_int64 i64;
			SG_uint64 len_encoded  = 0;

			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_encoded", &i64)  );
			len_encoded = (SG_uint64) i64;

			/* seek ahead to skip the blob */
			SG_ERR_CHECK(  SG_file__tell(pCtx, pfb, &iPos)  );
			SG_ERR_CHECK(  SG_file__seek(pCtx, pfb, iPos + len_encoded)  );
		}
		else
		{
			SG_ERR_THROW(  SG_ERR_FRAGBALL_INVALID_OP  );
		}
		SG_VHASH_NULLFREE(pCtx, pvh);
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void _scan_fragball_for_leaves(
	SG_context* pCtx,
    SG_pathname* pPath,
    SG_vhash** ppvh
	)
{
	SG_vhash* pvh_version = NULL;
	const char* psz_version = NULL;
	SG_uint32 version = 0;
	SG_file* pfb = NULL;
    SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(pPath);

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pfb)  );

	SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfb, &pvh_version)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_version, "version", &psz_version)  );
	version = atoi(psz_version);
	SG_VHASH_NULLFREE(pCtx, pvh_version);

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
	switch (version)
	{
		case 2:
			SG_ERR_CHECK(  _leaf_scan__version_2(pCtx, pfb, pvh)  );
			break;

		default:
			SG_ERR_THROW(  SG_ERR_FRAGBALL_INVALID_VERSION  );
			break;
	}

	SG_ERR_CHECK(  SG_file__close(pCtx, &pfb)  );

    *ppvh = pvh;
    pvh = NULL;

fail:
	SG_FILE_NULLCLOSE(pCtx, pfb);
	SG_VHASH_NULLFREE(pCtx, pvh_version);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

static void _save_leaves_for_next_time(
	SG_context* pCtx,
	_sg_pull* pMyPull,
    const char* psz_fragball_name
    )
{
    SG_vhash* pvh_since = NULL;
    SG_pathname* pPath = NULL;
	sg_pull_instance_data* pMe = pMyPull->pPullInstance;

	SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, pMe->pszPullId, &pPath)  );
    SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, psz_fragball_name)  );

    SG_ERR_CHECK(  _scan_fragball_for_leaves(pCtx, pPath, &pvh_since)  );

    SG_ERR_CHECK(  SG_closet__write_remote_leaves(pCtx, pMe->psz_remote_repo_spec, pvh_since)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_since);
}

static void _do_pull__using_since(
	SG_context* pCtx,
	_sg_pull* pMyPull,
	SG_vhash** ppvhStats)
{
	sg_pull_instance_data* pMe = pMyPull->pPullInstance;
	SG_vhash* pvhFinalPrecommitStatusForStats = NULL;
    SG_vhash* pvhStatus = NULL;
    char* psz_fragball_leaves = NULL;
    SG_vhash* pvh_since = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Pulling", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 5u, NULL)  );

	SG_NULLARGCHECK(pMyPull);

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Looking for changes")  );

    /* Try to use the SINCE information */
    SG_ERR_CHECK(  SG_closet__read_remote_leaves(pCtx, pMe->psz_remote_repo_spec, &pvh_since)  );

    if (pvh_since)
    {
        //fprintf(stderr, "\npvh_since, as read from the closet:\n");
        //SG_VHASH_STDERR(pvh_since);
        SG_ERR_CHECK(  _pull_since_last_time(pCtx, pMyPull, &pvh_since)  );

        // TODO we should have enough info to construct the leaf set.  instead, we
        // just request another (very small) fragball
        SG_ERR_CHECK(  _pull_leaves_for_every_dag(pCtx, pMyPull, &pvhStatus, &psz_fragball_leaves)  );
    }
    else
    {
        /* Request a fragball containing leaves of every dag */
        SG_ERR_CHECK(  _pull_leaves_for_every_dag(pCtx, pMyPull, &pvhStatus, &psz_fragball_leaves)  );

        /* Then check the status and use it to request more dagnodes until the dags connect */
        SG_ERR_CHECK(  _add_dagnodes_until_connected(pCtx, &pvhStatus, pMyPull->pPullInstance, pMyPull->pSyncClient)  );
        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

        /* All necessary dagnodes are in the frag at this point.  Now get remaining missing blobs. */
        SG_ERR_CHECK(  SG_log__set_step(pCtx, "Transferring data")  );
        SG_ERR_CHECK(  _add_blobs_until_done(pCtx, pMyPull)  );
        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }

	if (ppvhStats)
	{
		SG_ERR_CHECK(  SG_staging__check_status(pCtx, pMe->pStaging, pMe->pPullIntoRepo, 
			SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, &pvhFinalPrecommitStatusForStats)  );
        //fprintf(stderr, "\nfinal check_status:\n");
        //SG_VHASH_STDERR(pvhFinalPrecommitStatusForStats);
	}

	/* commit */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Committing changes")  );
	SG_ERR_CHECK(  SG_staging__commit(pCtx, pMe->pStaging, pMe->pPullIntoRepo)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    /* Remember the leaves so we can use SINCE on the next pull */
    SG_ERR_CHECK(  _save_leaves_for_next_time(pCtx, pMyPull, psz_fragball_leaves)  );

	/* cleanup */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Cleaning up")  );
	SG_ERR_CHECK(  SG_staging__cleanup(pCtx, &pMe->pStaging)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* auto-merge zing dags */
	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Merging databases")  );
	SG_ERR_CHECK(  SG_zing__auto_merge_all_dags(pCtx, pMe->pPullIntoRepo)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    //fprintf(stderr, "\nRound trips: %d\n", pMe->countRoundtrips);

	if (ppvhStats)
    {
		SG_ERR_CHECK(  _buildPullStats(pCtx, pMyPull, pvhFinalPrecommitStatusForStats, ppvhStats)  );
    }

	/* fall through */
fail:
	SG_log__pop_operation(pCtx);
    SG_VHASH_NULLFREE(pCtx, pvh_since);
	SG_VHASH_NULLFREE(pCtx, pvhFinalPrecommitStatusForStats);
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
    SG_NULLFREE(pCtx, psz_fragball_leaves);
}

void SG_pull__all__using_since(
	SG_context* pCtx, 
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats
    )
{
	_sg_pull* pMyPull = NULL;

	SG_ERR_CHECK(  _pull_init(pCtx, pRepoPullInto, pszRemoteRepoSpec, NULL, pszUsername, pszPassword, SG_FALSE, &pMyPull)  );
	SG_ERR_CHECK(  _do_pull__using_since(pCtx, pMyPull, ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPull->pPullInstance->pvh_remote_repo_info, ppvhAccountInfo)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
}
#endif

void SG_pull__all(
	SG_context* pCtx, 
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats)
{
	_sg_pull* pMyPull = NULL;

	SG_ERR_CHECK(  _pull_init(pCtx, pRepoPullInto, pszRemoteRepoSpec, NULL, pszUsername, pszPassword, SG_FALSE, &pMyPull)  );
	SG_ERR_CHECK(  _do_pull(pCtx, pMyPull, NULL, ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPull->pPullInstance->pvh_remote_repo_info, ppvhAccountInfo)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
}

void SG_pull__begin(
	SG_context* pCtx,
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_pull** ppPull)
{
	_sg_pull* pMyPull = NULL;

	SG_NULLARGCHECK_RETURN(ppPull);

	SG_ERR_CHECK(  _pull_init(pCtx, pRepoPullInto, pszRemoteRepoSpec, NULL, pszUsername, pszPassword, SG_FALSE, &pMyPull)  );

	*ppPull = (SG_pull*)pMyPull;
	pMyPull = NULL;

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
}

void SG_pull__add(
	SG_context* pCtx,
	SG_pull* pPull,
	SG_rev_spec** ppAddRevSpec)
{
	_sg_pull* pMyPull = NULL;

	SG_uint64 iDagnum;
	char bufDagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_bool bDagnumRequestExists = SG_FALSE;
	
	/* These need to be freed. */
	SG_vhash* pvhDags = NULL;
	SG_vhash* pvhNewRevSpec = NULL;

	/* These don't. */
	SG_vhash* pvhRefRevSpec = NULL;
	SG_vhash* pvhDagsRef = NULL;
	SG_bool bSkipDag = SG_FALSE;
	
	SG_rev_spec* pNewRevSpec = NULL;

	SG_NULLARGCHECK_RETURN(pPull);
	SG_NULL_PP_CHECK_RETURN(ppAddRevSpec);

	pMyPull = (_sg_pull*)pPull;
	if (!pMyPull->pvhFragballRequest)
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pMyPull->pvhFragballRequest)  );

	SG_ERR_CHECK(  SG_rev_spec__get_dagnum(pCtx, *ppAddRevSpec, &iDagnum)  );

	SG_ERR_CHECK(  SG_zing__missing_hardwired_template(pCtx, iDagnum, &bSkipDag)  );
	if (!bSkipDag)
	{
		SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagnum, bufDagnum, sizeof(bufDagnum))  );

		/* Get the existing rev spec, if there is one. */
		{
			SG_bool found = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pMyPull->pvhFragballRequest, SG_SYNC_STATUS_KEY__DAGS, &found)  );
			if (found)
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pMyPull->pvhFragballRequest, SG_SYNC_STATUS_KEY__DAGS, &pvhDagsRef)  );
			else
			{
				SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhDags)  );
				pvhDagsRef = pvhDags;
				SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pMyPull->pvhFragballRequest, SG_SYNC_STATUS_KEY__DAGS, &pvhDags)  );
			}
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhDagsRef, bufDagnum, &bDagnumRequestExists)  );
			if (bDagnumRequestExists)
			{
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhDagsRef, bufDagnum, &pvhRefRevSpec)  );
				SG_ERR_CHECK(  SG_rev_spec__from_vash(pCtx, pvhRefRevSpec, &pNewRevSpec)  );
			}
		}

		/* If there's an existing rev spec, merge the new one into it. */
		if (pNewRevSpec)
		{
			SG_ERR_CHECK(  SG_rev_spec__merge(pCtx, pNewRevSpec, ppAddRevSpec)  );
		}
		else
		{
			/* Otherwise, we'll just add the provided rev spec as-is. */
			pNewRevSpec = *ppAddRevSpec;
			*ppAddRevSpec = NULL;
		}

		/* Put new rev spec into fragball request vhash. Replace the old contents if there were any. */
		if (bDagnumRequestExists)
			SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvhDagsRef, bufDagnum)  );
		SG_ERR_CHECK(  SG_rev_spec__to_vhash(pCtx, pNewRevSpec, SG_TRUE, &pvhNewRevSpec)  );
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhDagsRef, bufDagnum, &pvhNewRevSpec)  );
	}

	/* common cleanup */
fail:
	SG_REV_SPEC_NULLFREE(pCtx, pNewRevSpec);
	SG_VHASH_NULLFREE(pCtx, pvhDags);
	SG_VHASH_NULLFREE(pCtx, pvhNewRevSpec);
}

static void _add_dagnum_to_request(SG_context* pCtx, SG_uint64 iDagnum, SG_vhash** ppvhFragballRequest)
{
	char bufDagnum[SG_DAGNUM__BUF_MAX__HEX];

	/* These need to be freed. */
	SG_vhash* pvhDags = NULL;
	SG_vhash* pvhFragballRequest = NULL;

	/* These don't. */
	SG_vhash* pvhDagsRef = NULL;
	SG_vhash* pvhFragballRequestRef = NULL;

	SG_bool bSkipDag = SG_FALSE;

	SG_NULLARGCHECK_RETURN(ppvhFragballRequest);

	if (*ppvhFragballRequest)
		pvhFragballRequestRef = *ppvhFragballRequest;
	else
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhFragballRequest)  );
		pvhFragballRequestRef = pvhFragballRequest;
	}

	SG_ERR_CHECK(  SG_zing__missing_hardwired_template(pCtx, iDagnum, &bSkipDag)  );
	if (!bSkipDag)
	{
		SG_bool found = SG_FALSE;

		SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagnum, bufDagnum, sizeof(bufDagnum))  );

		/* Get the dags container node into pvhDagsRev. */
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhFragballRequestRef, SG_SYNC_STATUS_KEY__DAGS, &found)  );
		if (found)
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhFragballRequestRef, SG_SYNC_STATUS_KEY__DAGS, &pvhDagsRef)  );
		else
		{
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhDags)  );
			pvhDagsRef = pvhDags;
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhFragballRequestRef, SG_SYNC_STATUS_KEY__DAGS, &pvhDags)  );
		}

		/* Add a null value for this dagnum, indicating we want it all. */
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvhDagsRef, bufDagnum)  );
	}

	*ppvhFragballRequest = pvhFragballRequestRef;
	pvhFragballRequest = NULL;

	/* common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhDags);
	SG_VHASH_NULLFREE(pCtx, pvhFragballRequest);
}

void SG_pull__add__dagnum(SG_context* pCtx, SG_pull* pPull, SG_uint64 iDagnum)
{
	_sg_pull* pMyPull = (_sg_pull*)pPull;

	SG_NULLARGCHECK_RETURN(pPull);
	SG_ARGCHECK_RETURN(iDagnum,iDagnum);

	SG_ERR_CHECK_RETURN(  _add_dagnum_to_request(pCtx, iDagnum, &pMyPull->pvhFragballRequest)  );
}

void SG_pull__commit(
	SG_context* pCtx,
	SG_pull** ppPull,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats
    )
{
	_sg_pull* pMyPull = NULL;

	SG_NULL_PP_CHECK(ppPull);
	pMyPull = (_sg_pull*)*ppPull;

	SG_ERR_CHECK(  _do_pull(pCtx, pMyPull, NULL, ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPull->pPullInstance->pvh_remote_repo_info, ppvhAccountInfo)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, (_sg_pull**)ppPull)  );
}

void SG_pull__abort(
	SG_context* pCtx,
	SG_pull** ppPull)
{
	_sg_pull** ppMyPull = (_sg_pull**)ppPull;

	SG_NULL_PP_CHECK_RETURN(ppPull);

	SG_ERR_CHECK_RETURN(  _sg_pull__nullfree(pCtx, ppMyPull)  );
}

void SG_pull__list_incoming_vc(
	SG_context* pCtx,
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_history_result** ppInfo,
	SG_vhash** ppvhStats)
{
	SG_string* pstrOperation = NULL;
	_sg_pull* pMyPull = NULL;
	SG_vhash* pvhStatus = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrOperation)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrOperation, "Incoming from ")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrOperation, pszRemoteRepoSpec)  );
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, SG_string__sz(pstrOperation), SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 2u, NULL)  );

	SG_NULLARGCHECK(pRepoPullInto);
	SG_NULLARGCHECK(pszRemoteRepoSpec);

	SG_ERR_CHECK(  _pull_init(pCtx, pRepoPullInto, pszRemoteRepoSpec, NULL, pszUsername, pszPassword, SG_FALSE, &pMyPull)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Looking for changes")  );
	SG_ERR_CHECK(  _add_dagnum_to_request(pCtx, SG_DAGNUM__VERSION_CONTROL, &pMyPull->pvhFragballRequest)  );
	SG_ERR_CHECK(  _add_dagnodes_and_connect(pCtx, pMyPull)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Fetching changeset descriptions")  );
	{
		sg_pull_instance_data* pMe = pMyPull->pPullInstance;
		SG_bool b = SG_FALSE;
		SG_vhash* pvhRequest = NULL;

		SG_ERR_CHECK(  SG_staging__check_status(pCtx, pMe->pStaging, pMe->pPullIntoRepo,
			SG_FALSE, SG_TRUE, SG_FALSE, SG_FALSE, (ppvhStats != NULL), &pvhStatus)  );

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhStatus, SG_SYNC_STATUS_KEY__NEW_NODES, &b)  );
		if (b)
		{
			/* There are incoming nodes.  Get their info. */
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStatus, SG_SYNC_STATUS_KEY__NEW_NODES, &pvhRequest)  );
#if TRACE_PULL
			SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhRequest, "incoming status")  );
#endif
			SG_ERR_CHECK(  SG_sync_client__get_dagnode_info(pCtx, pMyPull->pSyncClient, pvhRequest, ppInfo)  );
		}
	}
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

 	if (ppvhStats)
	{
 		SG_ERR_CHECK(  _buildPullStats(pCtx, pMyPull, pvhStatus, ppvhStats)  );
	}

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
	SG_log__pop_operation(pCtx);
	SG_STRING_NULLFREE(pCtx, pstrOperation); // must be after the operation pop
}

void SG_pull__add__area(
	SG_context* pCtx,
	SG_pull* pPull,
    const char* psz_area_name
    )
{
	_sg_pull* pMe = (_sg_pull*) pPull;
    SG_vhash* pvh_dags = NULL;
    SG_vhash* pvh_areas = NULL;
    SG_uint64 area_id = 0;
    SG_uint32 count_dags = 0;
    SG_uint32 i_dag = 0;
	SG_rev_spec* pRevSpec = NULL;

	SG_NULLARGCHECK_RETURN(pPull);

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pMe->pPullInstance->pvh_remote_repo_info, "areas", &pvh_areas)  );

	SG_vhash__get__uint64(pCtx, pvh_areas, psz_area_name, &area_id);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_REPLACE2(SG_ERR_VHASH_KEYNOTFOUND, SG_ERR_AREA_NOT_FOUND, (pCtx, "%s", psz_area_name));
		SG_ERR_CHECK_CURRENT;
	}

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pMe->pPullInstance->pvh_remote_repo_info, "dags", &pvh_dags)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dags, &count_dags)  );
    for (i_dag=0; i_dag<count_dags; i_dag++)
    {
        const char* psz_dagnum = NULL;
        SG_uint64 dagnum = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dags, i_dag, &psz_dagnum, NULL)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
        if (SG_DAGNUM__GET_AREA_ID(dagnum) == area_id)
        {
            SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, dagnum)  );
        }
    }

fail:
    SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

void SG_pull__add__area__version_control(
	SG_context* pCtx,
	SG_pull* pPull,
	SG_rev_spec** ppRevSpec
    )
{
    SG_uint64 area_id = SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__VERSION_CONTROL);
	_sg_pull* pMe = (_sg_pull*) pPull;
    SG_vhash* pvh_dags = NULL;
    SG_uint32 count_dags = 0;
    SG_uint32 i_dag = 0;

	SG_NULLARGCHECK_RETURN(pPull);
	
	if (ppRevSpec && *ppRevSpec)
	{
		/* If specific version control nodes were requested, add them. */
		SG_uint64 iDagnum;
		SG_ERR_CHECK(  SG_rev_spec__get_dagnum(pCtx, *ppRevSpec, &iDagnum)  );
		if (iDagnum != SG_DAGNUM__VERSION_CONTROL)
			SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "revision spec's dagnum is not version control"));

		SG_ERR_CHECK(  SG_pull__add(pCtx, pPull, ppRevSpec)  );
	}
	else
	{
		/* Otherwise add the whole dag. */
		SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__VERSION_CONTROL)  );
	}
    

	/* Now add all the other dags that are in the version control area. */
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pMe->pPullInstance->pvh_remote_repo_info, "dags", &pvh_dags)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dags, &count_dags)  );
    for (i_dag=0; i_dag<count_dags; i_dag++)
    {
        const char* psz_dagnum = NULL;
        SG_uint64 dagnum = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dags, i_dag, &psz_dagnum, NULL)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
        if (SG_DAGNUM__GET_AREA_ID(dagnum) == area_id)
        {
            if (SG_DAGNUM__VERSION_CONTROL != dagnum)
            {
				SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, dagnum)  );
            }
        }
    }

fail:
    ;
}

void SG_pull__add__admin(
	SG_context* pCtx,
	SG_pull* pPull
	)
{
	_sg_pull* pMe = (_sg_pull*) pPull;
	SG_vhash* pvh_dags = NULL;
	SG_uint32 count_dags = 0;
	SG_uint32 i_dag = 0;

	SG_NULLARGCHECK_RETURN(pPull);

	/* Add all the remote dags that have their admin bit set. */
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pMe->pPullInstance->pvh_remote_repo_info, "dags", &pvh_dags)  );
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dags, &count_dags)  );
	for (i_dag = 0; i_dag < count_dags; i_dag++)
	{
		const char* psz_dagnum = NULL;
		SG_uint64 dagnum = 0;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dags, i_dag, &psz_dagnum, NULL)  );
		SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
		if (SG_DAGNUM__IS_ADMIN(dagnum))
			SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, dagnum)  );
	}

fail:
	;
}

void SG_pull__admin(
	SG_context* pCtx, 
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats)
{
	_sg_pull* pMyPull = NULL;

	SG_NULLARGCHECK_RETURN(pRepoPullInto);
	SG_NULLARGCHECK_RETURN(pszRemoteRepoSpec);

	SG_ERR_CHECK(  _pull_init(pCtx, pRepoPullInto, pszRemoteRepoSpec, NULL, pszUsername, pszPassword, SG_TRUE, &pMyPull)  );
	SG_ERR_CHECK(  SG_pull__add__admin(pCtx, (SG_pull*)pMyPull)  );
	SG_ERR_CHECK(  _do_pull(pCtx, pMyPull, "Pulling user database", ppvhStats)  );

	if (ppvhAccountInfo)
		SG_ERR_CHECK(  SG_sync__get_account_info__from_repo_info(pCtx, pMyPull->pPullInstance->pvh_remote_repo_info, ppvhAccountInfo)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
}

void SG_pull__admin__local(
	SG_context* pCtx, 
	SG_repo* pRepoDest, 
	SG_repo* pRepoSrc,
	SG_vhash** ppvhStats)
{
	_sg_pull* pMyPull = NULL;

	SG_NULLARGCHECK_RETURN(pRepoDest);
	SG_NULLARGCHECK_RETURN(pRepoSrc);

	SG_ERR_CHECK(  _pull_init(pCtx, pRepoDest, NULL, pRepoSrc, NULL, NULL, SG_TRUE, &pMyPull)  );
	SG_ERR_CHECK(  SG_pull__add__admin(pCtx, (SG_pull*)pMyPull)  );
	SG_ERR_CHECK(  _do_pull(pCtx, pMyPull, "Pulling user database", ppvhStats)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  _sg_pull__nullfree(pCtx, &pMyPull)  );
}
