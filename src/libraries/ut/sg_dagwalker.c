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





//////////////////////////////////////////////////////////////////

static void _dw_work_queue__insert(SG_context * pCtx,
								SG_rbtree * pRbTree, SG_dagnode * pDagnode)
{
	// insert an entry into sorted work-queue for the my_data structure
	// associated with a dagnode.
	//
	// the work-queue contains a list of the dagnodes that we need
	// to examine.  we seed it with the set of n leaves.  it then
	// evolves as we walk the parent links in each node.  it maintains
	// the "active frontier" of the dag that we are exploring.
	//
	// to avoid lots of O(n^2) and worse problems, we maintain the
	// queue sorted by dagnode depth/generation in the dag.  this
	// allows us to visit nodes in an efficient order.
	//
	// the sort key looks like <-generation>.<hid> "%08x.%s"
	//
	// Note: we don't need the second part of the key to be
	// the HID, it could be a static monotonically increasing
	// sequence number.  (anything to give us a unique value.)
	// I'm going to use the HID for debugging.

	char bufSortKey[SG_HID_MAX_BUFFER_LENGTH + 20];
	const char * szHid;
	SG_int32 genDagnode = 0;
	SG_bool bAlreadyThere = SG_FALSE;
	if (pDagnode)
	{
		SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx,pDagnode,&szHid)  );
		SG_ERR_CHECK_RETURN(  SG_dagnode__get_generation(pCtx, pDagnode, &genDagnode)  );
		SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
										 bufSortKey,sizeof(bufSortKey),
										 "%08x.%s", (-genDagnode), szHid)  );
	}
	else
	{
		// ensure that the null root node gets appended to the end of the list.
		// (because -0 is 0, the above generation trick doesn't work)
		szHid = "*root*";
		SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
										 bufSortKey,sizeof(bufSortKey),
										 "z.%s", szHid)  );
	}
	
	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pRbTree, bufSortKey, &bAlreadyThere, NULL)  );
	if (bAlreadyThere == SG_FALSE)
		SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx,
											pRbTree,
											bufSortKey,
											pDagnode)  );
}

static void _dw_cache__insert__dagnode(SG_context * pCtx,
								SG_rbtree * pRbTree, SG_dagnode * pDagnode)
{
	const char * szHid = NULL;
	if (pDagnode)
	{
		//SG_bool bAlreadyThere = SG_FALSE;
		SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx, pDagnode, &szHid)  );
		//SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pRbTree, szHid, &bAlreadyThere, NULL)  );
		//if (bAlreadyThere == SG_FALSE)
			SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx,
												pRbTree,
												szHid,
												pDagnode)  );

	}
}


static void _dw_work_queue__remove_first(SG_context * pCtx,
									  SG_rbtree * pRbTree,
									  SG_bool * pbFound,
									  void ** ppData)	// you do not own this
{
	// remove the first element in the work-queue.
	// this should be one of the deepest nodes in the frontier
	// of the dag that we have yet to visit.
	//
	// you don't own the returned pData (because the work-queue
	// borrowed it from the fetched-queue).

	// TODO we should add a SG_rbtree__remove__first__with_assoc().
	// TODO that would save a full lookup.

	const char * szKey;
	void * pData;
	SG_bool bFound = SG_FALSE;

	SG_ERR_CHECK_RETURN(  SG_rbtree__iterator__first(pCtx,
													 NULL,pRbTree,
													 &bFound,
													 &szKey,
													 (void**)&pData)  );
	if (bFound)
		SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx,
												pRbTree,szKey)  );

	*pbFound = bFound;
	*ppData = ((bFound) ? pData : NULL);
}

static void _walk_dag(SG_context* pCtx,
					  SG_repo* pRepo,
					  SG_uint64 iDagNum,
					  SG_dagwalker__callback* callbackFunction,
					  void* callbackData,
					  SG_rbtree* prbtree_work_queue,
					  SG_rbtree* prbtree_dagnode_cache,
					  SG_bool bRemoveDagnodes_from_cache,
					  SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle_input,
					  SG_dagwalker_token ** ppToken)
{
	SG_bool bFound = SG_TRUE;
	SG_dagwalker_continue dwContinue = SG_DAGWALKER_CONTINUE__UNCONDITIONALLY;
	SG_dagnode* pdn = NULL;
	SG_bool bFoundInCache = SG_FALSE;
	SG_dagnode* pDagnode = NULL;
	const char * pszHid = NULL;
	SG_vhash * pvh_token = NULL;
	SG_varray * pvarray_queue = NULL;
	SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle = NULL;

	//We're going to be fetching a lot of dagnodes.
	//Doing this will make things faster.
	if (pFetchDagnodesHandle_input == NULL)
	{
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, iDagNum, &pFetchDagnodesHandle)  );
	}
	else
	{
		pFetchDagnodesHandle = pFetchDagnodesHandle_input;
	}
	while (bFound)
	{
		SG_ERR_CHECK(  _dw_work_queue__remove_first(pCtx,prbtree_work_queue,&bFound,(void**)&pdn)  );
		if ((bFound == SG_TRUE) && (dwContinue != SG_DAGWALKER_CONTINUE__STOP_NOW))
		{
			SG_ERR_CHECK(  (callbackFunction)(pCtx, pRepo, callbackData, pdn, prbtree_dagnode_cache, &dwContinue)  );;
			if (dwContinue == SG_DAGWALKER_CONTINUE__UNCONDITIONALLY)
			{
                SG_uint32 count_parents = 0;
                const char** parents = NULL;
				SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pdn, &count_parents, &parents)  );
				if (parents != NULL)
				{
                    SG_uint32 i = 0;

                    for (i=0; i<count_parents; i++)
                    {
						SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbtree_dagnode_cache, parents[i], &bFoundInCache, (void**)&pDagnode) );
						if (bFoundInCache == SG_FALSE)
						{
							SG_ERR_CHECK(  SG_REPO__FETCH_DAGNODES__ONE(pCtx, pRepo, pFetchDagnodesHandle, parents[i], &pDagnode)  );
							SG_ERR_CHECK(  _dw_cache__insert__dagnode(pCtx, prbtree_dagnode_cache, pDagnode)  );
						}
						SG_ERR_CHECK(  _dw_work_queue__insert(pCtx, prbtree_work_queue, pDagnode)  );
                    }
				}
			}
			else if (dwContinue == SG_DAGWALKER_CONTINUE__EMPTY_QUEUE)
			{
				// At this point we've skipped the parents the callback asked us to skip.
				// We set this to continue unconditionally to correctly process the work queue.
				dwContinue = SG_DAGWALKER_CONTINUE__UNCONDITIONALLY;
			}
			else if (dwContinue == SG_DAGWALKER_CONTINUE__STOP_NOW)
			{
				SG_uint32 nQueueLength = 0;
				//Add this node's parents to the queue, so that they appear in the token
				//that we return.

                SG_uint32 count_parents = 0;
                const char** parents = NULL;
				SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pdn, &count_parents, &parents)  );
				if (parents != NULL)
				{
                    SG_uint32 i = 0;
                    
                    for (i=0; i<count_parents; i++)
                    {
						SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbtree_dagnode_cache, parents[i], &bFoundInCache, NULL) );
						if (bFoundInCache == SG_FALSE)
						{
							SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, iDagNum, parents[i], &pDagnode)  );
							SG_ERR_CHECK(  _dw_work_queue__insert(pCtx, prbtree_work_queue, pDagnode)  );
							SG_ERR_CHECK(  _dw_cache__insert__dagnode(pCtx, prbtree_dagnode_cache, pDagnode)  );
						}
                    }
				}

				SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbtree_work_queue, &nQueueLength)  );
				//If there are more dagnodes to walk (we are exiting before the 
				//end of the dag walk, try to put the queue in the token.
				//Then they can continue the dagwalk at a later time.
				if (nQueueLength > 0 && ppToken && *ppToken == NULL)
				{
					SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvarray_queue)  );
				}
			
			}
			
			if (bRemoveDagnodes_from_cache == SG_TRUE)
			{
				SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pdn,&pszHid)  );
				SG_ERR_CHECK(  SG_rbtree__remove(pCtx, prbtree_dagnode_cache, pszHid)  );
				SG_DAGNODE_NULLFREE(pCtx, pdn);  //Free the dagnode.  Once we walk through it once, we won't walk through it again.
			}
		}
		else if (bFound == SG_TRUE && dwContinue == SG_DAGWALKER_CONTINUE__STOP_NOW)
		{
			//Our callback told us to stop. Just free everything
			SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pdn,&pszHid)  );
			//Add it to the queue.
			if (pvarray_queue != NULL)
			{
				SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvarray_queue, pszHid)  );
			}

			if (bRemoveDagnodes_from_cache == SG_TRUE)
			{
				SG_ERR_CHECK(  SG_rbtree__remove(pCtx, prbtree_dagnode_cache, pszHid)  );
				SG_DAGNODE_NULLFREE(pCtx, pdn);  //Free the dagnode.  Once we walk through it once, we won't walk through it again.
			}
		}
	}
	if (pvarray_queue != NULL)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_token)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_token, "queue", &pvarray_queue)  );
		*ppToken = (SG_dagwalker_token*)pvh_token;
	}
	if (pFetchDagnodesHandle_input == NULL)
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pFetchDagnodesHandle)  );
	return;

fail:
	if (pFetchDagnodesHandle_input == NULL)
		SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pFetchDagnodesHandle)  );
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}

void SG_dagwalker__walk_dag_single(SG_context* pCtx,
								   SG_repo* pRepo,
								   SG_uint64 iDagNum,
								   const char* pszStartWithNodeHid,
								   SG_dagwalker__callback* callbackFunction,
								   void* callbackData)
{
	SG_dagnode* pdn = NULL;
	SG_rbtree * prbtree_work_queue = NULL;
	SG_rbtree * prbtree_dagnode_cache = NULL;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbtree_work_queue)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbtree_dagnode_cache)  );

	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, iDagNum, pszStartWithNodeHid, &pdn)  );
	SG_ERR_CHECK(  _dw_work_queue__insert(pCtx, prbtree_work_queue, pdn)  );
	SG_ERR_CHECK(  _dw_cache__insert__dagnode(pCtx, prbtree_dagnode_cache, pdn)  );
	pdn = NULL;

	SG_ERR_CHECK(  _walk_dag(pCtx, pRepo, iDagNum, callbackFunction, callbackData, prbtree_work_queue, prbtree_dagnode_cache, SG_TRUE, NULL, NULL)  );

	/* fall through */
fail:
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_RBTREE_NULLFREE(pCtx, prbtree_dagnode_cache);
	SG_RBTREE_NULLFREE(pCtx, prbtree_work_queue);
}

void SG_dagwalker__walk_dag_single__keep_cache(SG_context* pCtx,
								   SG_repo* pRepo,
								   SG_uint64 iDagNum,
								   const char* pszStartWithNodeHid,
								   SG_dagwalker__callback* callbackFunction,
								   void* callbackData,
								   SG_rbtree * prbtree_dagnode_cache,
								   SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle_input
								   )
{
	SG_dagnode* pdn = NULL;
	SG_rbtree * prbtree_work_queue = NULL;
	SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle = NULL;
	SG_bool bInCache = SG_FALSE;

	//We're going to be fetching a lot of dagnodes.
	//Doing this will make things faster.
	if (pFetchDagnodesHandle_input == NULL)
	{
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, iDagNum, &pFetchDagnodesHandle)  );
	}
	else
	{
		pFetchDagnodesHandle = pFetchDagnodesHandle_input;
	}
	
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbtree_work_queue)  );

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbtree_dagnode_cache, pszStartWithNodeHid, &bInCache, (void**)&pdn)  );
	if (bInCache == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_REPO__FETCH_DAGNODES__ONE(pCtx, pRepo, pFetchDagnodesHandle, pszStartWithNodeHid, &pdn)  );
		SG_ERR_CHECK(  _dw_cache__insert__dagnode(pCtx, prbtree_dagnode_cache, pdn)  );
	}
	SG_ERR_CHECK(  _dw_work_queue__insert(pCtx, prbtree_work_queue, pdn)  );
	
	
	SG_ERR_CHECK(  _walk_dag(pCtx, pRepo, iDagNum, callbackFunction, callbackData, prbtree_work_queue, prbtree_dagnode_cache, SG_FALSE, pFetchDagnodesHandle, NULL)  );

	if (pFetchDagnodesHandle_input == NULL)
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pFetchDagnodesHandle)  );
	/* fall through */
fail:
	if (pFetchDagnodesHandle_input == NULL)
		SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pFetchDagnodesHandle)  );
	SG_RBTREE_NULLFREE(pCtx, prbtree_work_queue);	
}

void SG_dagwalker__walk_dag(SG_context * pCtx, SG_repo * pRepo, SG_uint64 iDagNum, SG_stringarray * pStringArrayStartNodes, SG_dagwalker__callback * callbackFunction, void * callbackData, SG_dagwalker_token ** ppToken)
{
	SG_uint32 SearchVectorCount =  0;
	SG_uint32 SearchVectorIndex = 0;
	const char * pStrCurrentDagNode = NULL;
	SG_dagnode* pdn = NULL;
	SG_rbtree * prbtree_work_queue = NULL;
	SG_rbtree * prbtree_dagnode_cache = NULL;

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayStartNodes, &SearchVectorCount )  );
	if (SearchVectorCount == 0)
		return;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbtree_work_queue)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbtree_dagnode_cache)  );

	//Look up the dagnodes for the start nodes.
	for (SearchVectorIndex = 0; SearchVectorIndex < SearchVectorCount; SearchVectorIndex++)
	{
		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArrayStartNodes, SearchVectorIndex, &pStrCurrentDagNode)  );
		SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, iDagNum, pStrCurrentDagNode, &pdn)  );
		SG_ERR_CHECK(  _dw_work_queue__insert(pCtx, prbtree_work_queue, pdn)  );
		SG_ERR_CHECK(  _dw_cache__insert__dagnode(pCtx, prbtree_dagnode_cache, pdn)  );
        pdn = NULL;
	}

	SG_ERR_CHECK(  _walk_dag(pCtx, pRepo, iDagNum, callbackFunction, callbackData, prbtree_work_queue, prbtree_dagnode_cache, SG_TRUE, NULL, ppToken)  );

	SG_RBTREE_NULLFREE(pCtx, prbtree_dagnode_cache);
	SG_RBTREE_NULLFREE(pCtx, prbtree_work_queue);

	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, prbtree_dagnode_cache);
	SG_RBTREE_NULLFREE(pCtx, prbtree_work_queue);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}
