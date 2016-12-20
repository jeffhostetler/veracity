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

/*
 * sg_dagwalker.h
 *
 *  Created on: Mar 1, 2010
 *      Author: jeremy
 */

#ifndef SG_DAGWALKER_H_
#define SG_DAGWALKER_H_

enum _sg_dagwalker_continue
{
	SG_DAGWALKER_CONTINUE__UNCONDITIONALLY		= 0,	// Continue walking the DAG until you fully run out of nodes.
	SG_DAGWALKER_CONTINUE__EMPTY_QUEUE			= 1,	// Don't walk the parents of the current node, but process the work queue
	SG_DAGWALKER_CONTINUE__STOP_NOW				= 2	// Stop walking the DAG immediately
};

typedef enum _sg_dagwalker_continue SG_dagwalker_continue;
typedef struct _opaque_dagwalker_token SG_dagwalker_token;

typedef void (SG_dagwalker__callback)(SG_context* pCtx, 
									SG_repo * pRepo, 
									void * pVoidData, 
									SG_dagnode * currentDagnode, 
									SG_rbtree * pDagnodeCache, 
									SG_dagwalker_continue * pContinue);

/**
 * Walk DAG from a single node.
 */
void SG_dagwalker__walk_dag_single(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	const char* pszStartWithNodeHid,
	SG_dagwalker__callback* callbackFunction,
	void* callbackData);

/**
 * Walk the dag from a single node, retain the dagnode cache.
 */
void SG_dagwalker__walk_dag_single__keep_cache(SG_context* pCtx,
								   SG_repo* pRepo,
								   SG_uint64 iDagNum,
								   const char* pszStartWithNodeHid,
								   SG_dagwalker__callback* callbackFunction,
								   void* callbackData,
								   SG_rbtree * prbtree_dagnode_cache,
								   SG_repo_fetch_dagnodes_handle * pFetchDagnodesHandle_input);

/**
 * Walk DAG from a list of nodes.
 */
void SG_dagwalker__walk_dag(
	SG_context * pCtx, 
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
	SG_stringarray * pStringArrayStartNodes, 
	SG_dagwalker__callback * callbackFunction, 
	void * callbackData,
	SG_dagwalker_token ** ppToken);

#endif /* SG_DAGWALKER_H_ */
