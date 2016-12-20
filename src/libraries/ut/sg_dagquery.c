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
 * @file sg_dagquery.c
 *
 * @details An odd collection of routines to ask some hard DAG relationship questions.
 *
 */

#define SG_DOUBLE_CHECK__PATH_TO_ROOT 0 // isn't always correct since there are multiple paths to root

#define SG_DOUBLE_CHECK__CALC_DELTA   0

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

void SG_dagquery__how_are_dagnodes_related(SG_context * pCtx,
										   SG_repo * pRepo,
                                           SG_uint64 dagnum,
										   const char * pszHid1,
										   const char * pszHid2,
										   SG_bool bSkipDescendantCheck,
										   SG_bool bSkipAncestorCheck,
										   SG_dagquery_relationship * pdqRel)
{
	SG_dagnode * pdn1 = NULL;
	SG_dagnode * pdn2 = NULL;
	SG_dagfrag * pFrag = NULL;
	SG_dagquery_relationship dqRel = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
	SG_int32 gen1, gen2;
	SG_bool bFound;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NONEMPTYCHECK_RETURN(pszHid1);
	SG_NONEMPTYCHECK_RETURN(pszHid2);
	SG_NULLARGCHECK_RETURN(pdqRel);

	if (strcmp(pszHid1, pszHid2) == 0)
	{
		dqRel = SG_DAGQUERY_RELATIONSHIP__SAME;
		goto cleanup;
	}

	// fetch the dagnode for both HIDs.  this throws when the HID is not found.

	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, pszHid1, &pdn1)  );
	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, pszHid2, &pdn2)  );

	// we say that 2 nodes are either:
	// [1] ancestor/descendant of each other;
	// [2] or that they are peers (cousins) of each other (no matter
	//     how distant in the DAG).  (that have an LCA, but we don't
	//     care about it.)

	// get the generation of both dagnodes.  if they are the same, then they
	// cannot have an ancestor/descendant relationship and therefore must be
	// peers/cousins (we don't care how close/distant they are).

	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn1, &gen1)  );
	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn2, &gen2)  );
	if (gen1 == gen2)
	{
		dqRel = SG_DAGQUERY_RELATIONSHIP__PEER;
		goto cleanup;
	}

	// see if one is an ancestor of the other.  since we only have PARENT
	// edges in our DAG, we start with the deeper one and walk backwards
	// until we've visited all ancestors at the depth of the shallower one.
	//
	// i'm going to be lazy here and not reinvent a recursive-ish parent-edge
	// graph walker.  instead, i'm going to create a DAGFRAG using the
	// deeper one and request the generation difference as the "thickness".
	// in theory, if we have an ancestor/descendant relationship, the
	// shallower one should be in the END-FRINGE of the DAGFRAG.
	//
	// i'm going to pick an arbitrary direction "cs1 is R of cs2".

	SG_ERR_CHECK(  SG_dagfrag__alloc_transient(pCtx, dagnum, &pFrag)  );
	if (gen1 > gen2)		// cs1 is *DEEPER* than cs2
	{
		if (bSkipDescendantCheck)
		{
			dqRel = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
		}
		else
		{
			SG_ERR_CHECK(  SG_dagfrag__load_from_repo__one(pCtx, pFrag, pRepo, pszHid1, (gen1 - gen2))  );
			SG_ERR_CHECK(  SG_dagfrag__query(pCtx, pFrag, pszHid2, NULL, NULL, &bFound, NULL)  );

			if (bFound)			// pszHid2 is an ancestor of pszHid1.  READ pszHid1 is a descendent of pszHid2.
				dqRel = SG_DAGQUERY_RELATIONSHIP__DESCENDANT;
			else				// they are *distant* peers.
				dqRel = SG_DAGQUERY_RELATIONSHIP__PEER;
		}
		goto cleanup;
	}
	else
	{
		if (bSkipAncestorCheck)
		{
			dqRel = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
		}
		else
		{
			SG_ERR_CHECK(  SG_dagfrag__load_from_repo__one(pCtx, pFrag, pRepo, pszHid2, (gen2 - gen1))  );
			SG_ERR_CHECK(  SG_dagfrag__query(pCtx, pFrag, pszHid1, NULL, NULL, &bFound, NULL)  );

			if (bFound)			// pszHid1 is an ancestor of pszHid2.
				dqRel = SG_DAGQUERY_RELATIONSHIP__ANCESTOR;
			else				// they are *distant* peers.
				dqRel = SG_DAGQUERY_RELATIONSHIP__PEER;
		}
		goto cleanup;
	}

	/*NOTREACHED*/

cleanup:
	*pdqRel = dqRel;

fail:
	SG_DAGNODE_NULLFREE(pCtx, pdn1);
	SG_DAGNODE_NULLFREE(pCtx, pdn2);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
}

//////////////////////////////////////////////////////////////////

void SG_dagquery__find_descendant_heads(SG_context * pCtx,
										SG_repo * pRepo,
                                        SG_uint64 iDagNum,
										const char * pszHidStart,
										SG_bool bStopIfMultiple,
										SG_dagquery_find_head_status * pdqfhs,
										SG_rbtree ** pprbHeads)
{
	SG_rbtree * prbLeaves = NULL;
	SG_rbtree * prbHeadsFound = NULL;
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey_k = NULL;
	SG_bool b;
	SG_dagquery_find_head_status dqfhs;
	SG_dagquery_relationship dqRel;
	SG_uint32 nrFound;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NONEMPTYCHECK_RETURN(pszHidStart);
	SG_NULLARGCHECK_RETURN(pdqfhs);
	SG_NULLARGCHECK_RETURN(pprbHeads);

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbHeadsFound)  );

	// fetch a list of all of the LEAVES in the DAG.
	// this rbtree only contains keys; no assoc values.

	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, iDagNum, &prbLeaves)  );

	// if the starting point is a leaf, then we are done (we don't care how many
	// other leaves are in the rbtree because none will be a child of ours because
	// we are a leaf).

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbLeaves, pszHidStart, &b, NULL)  );
	if (b)
	{
		SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbHeadsFound, pszHidStart)  );

		dqfhs = SG_DAGQUERY_FIND_HEAD_STATUS__IS_LEAF;
		goto done;
	}

	// inspect each leaf and qualify it; put the ones that pass
	// into the list of actual heads.

	nrFound = 0;
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, prbLeaves, &b, &pszKey_k, NULL)  );
	while (b)
	{
		// is head[k] a descendant of start?
		SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, iDagNum, pszKey_k, pszHidStart,
															 SG_FALSE, // we care about descendants, so don't skip
															 SG_TRUE,  // we don't care about ancestors, so skip them
															 &dqRel)  );

		if (dqRel == SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
		{
			nrFound++;

			if (bStopIfMultiple && (nrFound > 1))
			{
				// they wanted a unique answer and we've found too many answers
				// (which they won't be able to use anyway) so just stop and
				// return the status.  (we delete prbHeadsFound because it is
				// incomplete and so that they won't be tempted to use it.)

				SG_RBTREE_NULLFREE(pCtx, prbHeadsFound);
				dqfhs = SG_DAGQUERY_FIND_HEAD_STATUS__MULTIPLE;
				goto done;
			}

			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbHeadsFound, pszKey_k)  );
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &b, &pszKey_k, NULL)  );
	}

	switch (nrFound)
	{
	case 0:
		// this should NEVER happen.  we should always be able to find a
		// leaf/head for a node.
		//
		// TODO the only case where this might happen is if named branches
		// TODO cause the leaf to be disqualified.  so i'm going to THROW
		// TODO here rather than ASSERT.

		SG_ERR_THROW2(  SG_ERR_DAG_NOT_CONSISTENT,
						(pCtx, "Could not find head/leaf for changeset [%s]", pszHidStart)  );
		break;

	case 1:
		dqfhs = SG_DAGQUERY_FIND_HEAD_STATUS__UNIQUE;
		break;

	default:
		dqfhs = SG_DAGQUERY_FIND_HEAD_STATUS__MULTIPLE;
		break;
	}

done:
	*pprbHeads = prbHeadsFound;
	prbHeadsFound = NULL;

	*pdqfhs = dqfhs;

fail:
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_RBTREE_NULLFREE(pCtx, prbHeadsFound);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

void SG_dagquery__find_single_descendant_head(SG_context * pCtx,
											  SG_repo * pRepo,
                                              SG_uint64 dagnum,
											  const char * pszHidStart,
											  SG_dagquery_find_head_status * pdqfhs,
											  char * bufHidHead,
											  SG_uint32 lenBufHidHead)
{
	SG_rbtree * prbHeads = NULL;
	const char * pszKey_0;
	SG_dagquery_find_head_status dqfhs;
	SG_bool b;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NONEMPTYCHECK_RETURN(pszHidStart);
	SG_NULLARGCHECK_RETURN(pdqfhs);
	SG_NULLARGCHECK_RETURN(bufHidHead);

	SG_ERR_CHECK(  SG_dagquery__find_descendant_heads(pCtx, pRepo, dagnum, pszHidStart, SG_TRUE, &dqfhs, &prbHeads)  );

	switch (dqfhs)
	{
	case SG_DAGQUERY_FIND_HEAD_STATUS__IS_LEAF:
	case SG_DAGQUERY_FIND_HEAD_STATUS__UNIQUE:
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, NULL, prbHeads, &b, &pszKey_0, NULL)  );
		SG_ASSERT(  (b)  );
		SG_ERR_CHECK(  SG_strcpy(pCtx, bufHidHead, lenBufHidHead, pszKey_0)  );
		break;

	case SG_DAGQUERY_FIND_HEAD_STATUS__MULTIPLE:
	default:
		// don't throw, just return the status
		bufHidHead[0] = 0;
		break;
	}

	*pdqfhs = dqfhs;

fail:
	SG_RBTREE_NULLFREE(pCtx, prbHeads);
}

typedef struct
{
    SG_uint64 dagnum;
	const char* pszNewNodeHid;
	const char* pszOldNodeHid;
	SG_rbtree* prbNewNodeHids;
} new_since_context;


static void _dagquery__new_since__callback(SG_context* pCtx, 
										   SG_repo *pRepo, 
										   void *pVoidData, 
										   SG_dagnode *pCurrentDagnode, 
										   SG_rbtree* pDagnodeCache, 
										   SG_dagwalker_continue* pContinue)
{
	SG_dagquery_relationship rel;
	new_since_context* pcb = (new_since_context*)pVoidData;
	const char* szCurrentNodeHid = NULL;

	SG_UNUSED(pDagnodeCache);

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pCurrentDagnode, &szCurrentNodeHid)  );

	if (strcmp(szCurrentNodeHid, pcb->pszOldNodeHid) == 0)
	{
		*pContinue = SG_DAGWALKER_CONTINUE__EMPTY_QUEUE;
		return;
	}

	// TODO: Consider looking at revision numbers to make this faster.
	SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, pcb->dagnum, szCurrentNodeHid, pcb->pszOldNodeHid, SG_FALSE, SG_TRUE, &rel)  );
	if (rel != SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
	{
		SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, pcb->dagnum, szCurrentNodeHid, pcb->pszOldNodeHid, SG_TRUE, SG_FALSE, &rel)  );
		if (rel == SG_DAGQUERY_RELATIONSHIP__ANCESTOR)
		{
			*pContinue = SG_DAGWALKER_CONTINUE__EMPTY_QUEUE;
			return;
		}
	}

	SG_ERR_CHECK(  SG_rbtree__add(pCtx, pcb->prbNewNodeHids, szCurrentNodeHid)  );

fail:
	;
}

void SG_dagquery__new_since(SG_context * pCtx,
							SG_repo* pRepo,
							SG_uint64 iDagNum,
							const char* pszOldNodeHid,
							const char* pszNewNodeHid,
							SG_rbtree** pprbNewNodeHids)
{
	SG_dagquery_relationship rel;
	new_since_context cbCtx;

    cbCtx.dagnum = iDagNum;
	cbCtx.prbNewNodeHids = NULL;
	cbCtx.pszNewNodeHid = pszNewNodeHid;
	cbCtx.pszOldNodeHid = pszOldNodeHid;

	SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, iDagNum, pszNewNodeHid, pszOldNodeHid, SG_FALSE, SG_FALSE, &rel)  );

	if (rel != SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
		SG_ERR_THROW2_RETURN(SG_ERR_UNSPECIFIED, (pCtx, "pszNewNodeHid must be a descendant of pszOldNodeHid"));

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &cbCtx.prbNewNodeHids)  );

	SG_ERR_CHECK(  SG_dagwalker__walk_dag_single(pCtx, pRepo, iDagNum, pszNewNodeHid, _dagquery__new_since__callback, (void*)&cbCtx)  );

	SG_RETURN_AND_NULL(cbCtx.prbNewNodeHids, pprbNewNodeHids);

	/* Fall through to common cleanup */
fail:
	SG_RBTREE_NULLFREE(pCtx, cbCtx.prbNewNodeHids);
}

///////////////////////////////////////////////////////////////////////////////

// Below are definitions used by SG_dagquery__find_new_since_common().

// Defines used for isAncestorOf values.
#define _ANCESTOR_OF_NEW 0x01
#define _ANCESTOR_OF_OLD 0x02
//_ANCESTOR_OF_BOTH would be 0x03, but we don't actually need the symbol.

struct _fnsc_work_queue_item_struct
{
	// The dagnode to be processed.
	SG_dagnode * pDagnode;
	
	// The work queue is kept sorted by revno and we always process the
	// highest revno first. This information is stored in the dagnode, but
	// the SG_dagnode structure is opaque, so we store it here as well.
	SG_uint32 revno;
	
	// Which of the two nodes ("New" and "Old") this node is an ancestor of.
	SG_byte isAncestorOf;
};
typedef struct _fnsc_work_queue_item_struct _fnsc_work_queue_item_t;

#define _FNSC_WORK_QUEUE_INIT_LENGTH (16)
struct _fnsc_work_queue_struct
{
	// The list of work queue items, kept sorted by revno. Note: we will be
	// removing from the back (so that we don't have to memmove) and we
	// always want to process the highest revno first, thus it is kept in
	// ascending order.
	_fnsc_work_queue_item_t * p;
	SG_uint32 length;
	SG_uint32 allocatedLength;
	
	// We need to stop walking back when no more nodes on the queue are ancestors
	// of "New" only. We'll keep track of how there are in this separate
	// variable so we don't have to scan the list after each iteration.
	SG_uint32 numAncestorsOfNewOnTheQueue;
	
	// SG_dagnode__get_parents__ref() gives us hids when what we really want
	// are revno's. This is a simple hid-to-revno map of everything currently
	// on the queue. Since it tells us whether a dagnode is already on the
	// queue or not, this also means we don't have to double-fetch dagnodes.
	SG_rbtree * pRevnoCache;
};
typedef struct _fnsc_work_queue_struct _fnsc_work_queue_t;

// Add a new dagnode to the work queue, having already determined that it needs
// to be added at a particular location in the list.
static void _fnsc_work_queue__insert_at(
	SG_context * pCtx,
	_fnsc_work_queue_t * pWorkQueue,
	SG_uint32 index,
	const char * pszHid,
	SG_uint32 revno,
	SG_dagnode ** ppDagnode, // Takes ownership and nulls the caller's copy.
	SG_byte isAncestorOf
	)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx, pWorkQueue->pRevnoCache, pszHid, ((char*)NULL)+revno)  );
	
	if (pWorkQueue->length==pWorkQueue->allocatedLength)
	{
		_fnsc_work_queue_item_t * tmp = NULL;
		SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, pWorkQueue->allocatedLength*2, tmp)  );
		(void)memmove(tmp, pWorkQueue->p,
			pWorkQueue->length*sizeof(_fnsc_work_queue_item_t));
		SG_NULLFREE(pCtx, pWorkQueue->p);
		pWorkQueue->p = tmp;
		pWorkQueue->allocatedLength *= 2;
	}
	
	if (index < pWorkQueue->length)
	{
		(void)memmove(&pWorkQueue->p[index+1], &pWorkQueue->p[index],
			(pWorkQueue->length-index)*sizeof(_fnsc_work_queue_item_t));
	}
	
	pWorkQueue->p[index].revno = revno;
	pWorkQueue->p[index].isAncestorOf = isAncestorOf;
	pWorkQueue->p[index].pDagnode = *ppDagnode;
	
	*ppDagnode = NULL;
	
	++pWorkQueue->length;
	if(isAncestorOf==_ANCESTOR_OF_NEW)
	{
		++pWorkQueue->numAncestorsOfNewOnTheQueue;
	}
}

// Add a dagnode to the work queue if it's not already on it. If it is already
// on it, this might tell us new information about whether it is a descendent of
// "New" or "Old", so update it with the new isAncestorOf information.
static void _fnsc_work_queue__insert(
	SG_context * pCtx,
	_fnsc_work_queue_t * pWorkQueue,
	const char * pszHid,
	SG_uint64 dagnum,
	SG_repo * pRepo,
	SG_byte isAncestorOf
	)
{
	SG_bool alreadyInTheQueue = SG_FALSE;
	SG_dagnode * pDagnode = NULL;
	SG_uint32 i;
	SG_uint32 revno = 0;
	char * revno__p = NULL;
	
	// First we check the cache. This will tell us whether the item is
	// already on the queue, and if so what its revno is.
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pWorkQueue->pRevnoCache, pszHid, &alreadyInTheQueue, (void**)&revno__p)  );
	if(alreadyInTheQueue)
	{
		revno = (SG_uint32)(revno__p - (char*)NULL);
	}
	else
	{
		SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, pszHid, &pDagnode)  );
		SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pDagnode, &revno)  );
	}
	
	i = pWorkQueue->length;
	while(i>0 && pWorkQueue->p[i-1].revno > revno)
		--i;
	
	if (i>0 && pWorkQueue->p[i-1].revno == revno)
	{
		SG_ASSERT(alreadyInTheQueue);
		if (pWorkQueue->p[i-1].isAncestorOf==_ANCESTOR_OF_NEW && isAncestorOf!=_ANCESTOR_OF_NEW)
			--pWorkQueue->numAncestorsOfNewOnTheQueue;
		pWorkQueue->p[i-1].isAncestorOf |= isAncestorOf; // OR in the new isAncestorOfs
	}
	else
	{
		SG_ASSERT(pDagnode!=NULL);
		SG_ERR_CHECK(  _fnsc_work_queue__insert_at(pCtx, pWorkQueue, i, pszHid, revno, &pDagnode, isAncestorOf)  );
	}
	
	return;
fail:
	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
}

static void _fnsc_work_queue__pop(
	SG_context * pCtx,
	_fnsc_work_queue_t * pWorkQueue,
	SG_dagnode ** ppDagnode,
	const char ** ppszHidRef,
	SG_byte * isAncestorOf
	)
{
	const char * pszHidRef = NULL;
	SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx, pWorkQueue->p[pWorkQueue->length-1].pDagnode, &pszHidRef)  );
	SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx, pWorkQueue->pRevnoCache, pszHidRef)  );
	--pWorkQueue->length;
	if(pWorkQueue->p[pWorkQueue->length].isAncestorOf==_ANCESTOR_OF_NEW)
		--pWorkQueue->numAncestorsOfNewOnTheQueue;
	
	*ppDagnode = pWorkQueue->p[pWorkQueue->length].pDagnode;
	*ppszHidRef = pszHidRef;
	*isAncestorOf = pWorkQueue->p[pWorkQueue->length].isAncestorOf;
}

void SG_dagquery__find_new_since_common(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_uint64 dagnum,
	const char * pszOldNodeHid,
	const char * pszNewNodeHid,
	SG_stringarray ** ppResults
	)
{
	_fnsc_work_queue_t workQueue = {NULL, 0, 0, 0, NULL};
	SG_uint32 i;
	SG_dagnode * pDagnode = NULL;
	SG_stringarray * pResults = NULL;
	
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK(pRepo);
	SG_NONEMPTYCHECK(pszOldNodeHid);
	SG_NONEMPTYCHECK(pszNewNodeHid);
	SG_NULLARGCHECK(ppResults);
	
	SG_ERR_CHECK(  SG_allocN(pCtx, _FNSC_WORK_QUEUE_INIT_LENGTH, workQueue.p)  );
	workQueue.allocatedLength = _FNSC_WORK_QUEUE_INIT_LENGTH;
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &workQueue.pRevnoCache)  );
	
	SG_ERR_CHECK(  _fnsc_work_queue__insert(pCtx, &workQueue, pszOldNodeHid, dagnum, pRepo, _ANCESTOR_OF_OLD)  );
	SG_ERR_CHECK(  _fnsc_work_queue__insert(pCtx, &workQueue, pszNewNodeHid, dagnum, pRepo, _ANCESTOR_OF_NEW)  );
	
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pResults, 32)  );
	while(workQueue.numAncestorsOfNewOnTheQueue > 0)
	{
		const char * pszHidRef = NULL;
		SG_byte isAncestorOf = 0;
		
		SG_ERR_CHECK(  _fnsc_work_queue__pop(pCtx, &workQueue, &pDagnode, &pszHidRef, &isAncestorOf)  );
		if (isAncestorOf==_ANCESTOR_OF_NEW)
			SG_ERR_CHECK(  SG_stringarray__add(pCtx, pResults, pszHidRef)  );
		
		{
			SG_uint32 count_parents = 0;
			const char** parents = NULL;
			SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pDagnode, &count_parents, &parents)  );
			for(i=0; i<count_parents; ++i)
				SG_ERR_CHECK(  _fnsc_work_queue__insert(pCtx, &workQueue, parents[i], dagnum, pRepo, isAncestorOf)  );
		}
		
		SG_DAGNODE_NULLFREE(pCtx, pDagnode);
	}
	
	for(i=0; i<workQueue.length; ++i)
		SG_DAGNODE_NULLFREE(pCtx, workQueue.p[i].pDagnode);
	SG_NULLFREE(pCtx, workQueue.p);
	SG_RBTREE_NULLFREE(pCtx, workQueue.pRevnoCache);
	
	*ppResults = pResults;

	return;
fail:
	for(i=0; i<workQueue.length; ++i)
		SG_DAGNODE_NULLFREE(pCtx, workQueue.p[i].pDagnode);
	SG_NULLFREE(pCtx, workQueue.p);
	SG_RBTREE_NULLFREE(pCtx, workQueue.pRevnoCache);

	SG_DAGNODE_NULLFREE(pCtx, pDagnode);

	SG_STRINGARRAY_NULLFREE(pCtx, pResults);
}

///////////////////////////////////////////////////////////////////////////////

// Below are definitions used by SG_dagquery__highest_revno_common_ancestor().

struct _hrca_work_queue_item_struct
{
	// The dagnode to be processed.
	SG_dagnode * pDagnode;
	
	// The work queue is kept sorted by revno and we always process the
	// highest revno first. This information is stored in the dagnode, but
	// the SG_dagnode structure is opaque, so we store it here as well.
	SG_uint32 revno;

	SG_bitvector * pIsAncestorOf;
};
typedef struct _hrca_work_queue_item_struct _hrca_work_queue_item_t;

#define _HRCA_WORK_QUEUE_INIT_LENGTH (16)
struct _hrca_work_queue_struct
{
	// The list of work queue items, kept sorted by revno. Note: we will be
	// removing from the back (so that we don't have to memmove) and we
	// always want to process the highest revno first, thus it is kept in
	// ascending order.
	_hrca_work_queue_item_t * p;
	SG_uint32 length;
	SG_uint32 allocatedLength;
	
	// SG_dagnode__get_parents__ref() gives us hids when what we really want
	// are revno's. This is a simple hid-to-revno map of everything currently
	// on the queue. Since it tells us whether a dagnode is already on the
	// queue or not, this also means we don't have to double-fetch dagnodes.
	SG_rbtree * pRevnoCache;
};
typedef struct _hrca_work_queue_struct _hrca_work_queue_t;

// Add a new dagnode to the work queue, having already determined that it needs
// to be added at a particular location in the list.
static void _hrca_work_queue__insert_at(
	SG_context * pCtx,
	_hrca_work_queue_t * pWorkQueue,
	SG_uint32 index,
	const char * pszHid,
	SG_uint32 revno,
	SG_dagnode ** ppDagnode, // Takes ownership and nulls the caller's copy.
	SG_bitvector * pIsAncestorOf
	)
{
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pWorkQueue->pRevnoCache, pszHid, ((char*)NULL)+revno)  );

	if (pWorkQueue->length==pWorkQueue->allocatedLength)
	{
		_hrca_work_queue_item_t * tmp = NULL;
		SG_ERR_CHECK(  SG_allocN(pCtx, pWorkQueue->allocatedLength*2, tmp)  );
		(void)memmove(tmp, pWorkQueue->p,
			pWorkQueue->length*sizeof(_hrca_work_queue_item_t));
		SG_NULLFREE(pCtx, pWorkQueue->p);
		pWorkQueue->p = tmp;
		pWorkQueue->allocatedLength *= 2;
	}

	if (index < pWorkQueue->length)
	{
		(void)memmove(&pWorkQueue->p[index+1], &pWorkQueue->p[index],
			(pWorkQueue->length-index)*sizeof(_hrca_work_queue_item_t));
	}

	pWorkQueue->p[index].revno = revno;
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC__COPY(pCtx, &pWorkQueue->p[index].pIsAncestorOf, pIsAncestorOf)  );
	pWorkQueue->p[index].pDagnode = *ppDagnode;

	*ppDagnode = NULL;

	++pWorkQueue->length;

	return;
fail:
	;
}

// Add a dagnode to the work queue if it's not already on it. If it is already
// on it update it with the new pIsAncestorOf information.
static void _hrca_work_queue__insert(
	SG_context * pCtx,
	_hrca_work_queue_t * pWorkQueue,
	const char * pszHid,
	SG_repo * pRepo,
	SG_repo_fetch_dagnodes_handle * pDagnodeFetcher,
	SG_bitvector * pIsAncestorOf
	)
{
	SG_bool alreadyInTheQueue = SG_FALSE;
	SG_dagnode * pDagnode = NULL;
	SG_uint32 i;
	SG_uint32 revno = 0;
	char * revno__p = NULL;

	// First we check the cache. This will tell us whether the item is
	// already on the queue, and if so what its revno is.
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pWorkQueue->pRevnoCache, pszHid, &alreadyInTheQueue, (void**)&revno__p)  );
	if(alreadyInTheQueue)
	{
		revno = (SG_uint32)(revno__p - (char*)NULL);
	}
	else
	{
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pRepo, pDagnodeFetcher, pszHid, &pDagnode)  );
		SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pDagnode, &revno)  );
	}

	i = pWorkQueue->length;
	while(i>0 && pWorkQueue->p[i-1].revno > revno)
		--i;

	if (i>0 && pWorkQueue->p[i-1].revno == revno)
	{
		SG_ASSERT(alreadyInTheQueue);
		// OR in the new pIsAncestorOf
		SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pWorkQueue->p[i-1].pIsAncestorOf, pIsAncestorOf)  );
	}
	else
	{
		SG_ASSERT(pDagnode!=NULL);
		SG_ERR_CHECK(  _hrca_work_queue__insert_at(pCtx, pWorkQueue, i, pszHid, revno, &pDagnode, pIsAncestorOf)  );
	}

	return;
fail:
	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
}

static void _hrca_work_queue__pop(
	SG_context * pCtx,
	_hrca_work_queue_t * pWorkQueue,
	SG_dagnode ** ppDagnode,
	const char ** ppszHidRef,
	SG_bitvector ** ppIsAncestorOf
	)
{
	const char * pszHidRef = NULL;
	SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx, pWorkQueue->p[pWorkQueue->length-1].pDagnode, &pszHidRef)  );
	SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx, pWorkQueue->pRevnoCache, pszHidRef)  );
	--pWorkQueue->length;
	
	*ppDagnode = pWorkQueue->p[pWorkQueue->length].pDagnode;
	*ppszHidRef = pszHidRef;
	*ppIsAncestorOf = pWorkQueue->p[pWorkQueue->length].pIsAncestorOf;
}

void SG_dagquery__highest_revno_common_ancestor(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_uint64 dagnum,
	const SG_stringarray * pInputNodeHids,
	char ** ppOutputNodeHid
	)
{
	const char * const * paszInputNodeHids = NULL;
	SG_uint32 countInputNodes = 0;
	SG_repo_fetch_dagnodes_handle * pDagnodeFetcher = NULL;
	_hrca_work_queue_t workQueue = {NULL, 0, 0, NULL};
	SG_uint32 i;
	SG_dagnode * pDagnode = NULL;
	const char * pszHidRef = NULL;
	SG_bitvector * pIsAncestorOf = NULL;
	SG_uint32 countIsAncestorOf = 0;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(pInputNodeHids);
	SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, pInputNodeHids, &paszInputNodeHids, &countInputNodes)  );
	SG_ARGCHECK(countInputNodes>0, pInputNodeHids);
	SG_NULLARGCHECK(ppOutputNodeHid);

	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, dagnum, &pDagnodeFetcher)  );

	SG_ERR_CHECK(  SG_allocN(pCtx, _HRCA_WORK_QUEUE_INIT_LENGTH, workQueue.p)  );
	workQueue.allocatedLength = _HRCA_WORK_QUEUE_INIT_LENGTH;
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &workQueue.pRevnoCache)  );

	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pIsAncestorOf, countInputNodes)  );
	for(i=0; i<countInputNodes; ++i)
	{
		SG_ERR_CHECK(  SG_bitvector__zero(pCtx, pIsAncestorOf)  );
		SG_ERR_CHECK(  SG_bitvector__set_bit(pCtx, pIsAncestorOf, i, SG_TRUE)  );
		SG_ERR_CHECK(  _hrca_work_queue__insert(pCtx, &workQueue, paszInputNodeHids[i], pRepo, pDagnodeFetcher, pIsAncestorOf)  );
	}
	SG_BITVECTOR_NULLFREE(pCtx, pIsAncestorOf);

	SG_ERR_CHECK(  _hrca_work_queue__pop(pCtx, &workQueue, &pDagnode, &pszHidRef, &pIsAncestorOf)  );
	SG_ERR_CHECK(  SG_bitvector__count_set_bits(pCtx, pIsAncestorOf, &countIsAncestorOf)  );
	while(countIsAncestorOf < countInputNodes)
	{
		SG_uint32 count_parents = 0;
		const char** parents = NULL;
		SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pDagnode, &count_parents, &parents)  );
		for(i=0; i<count_parents; ++i)
			SG_ERR_CHECK(  _hrca_work_queue__insert(pCtx, &workQueue, parents[i], pRepo, pDagnodeFetcher, pIsAncestorOf)  );
		
		SG_DAGNODE_NULLFREE(pCtx, pDagnode);
		SG_BITVECTOR_NULLFREE(pCtx, pIsAncestorOf);

		SG_ERR_CHECK(  _hrca_work_queue__pop(pCtx, &workQueue, &pDagnode, &pszHidRef, &pIsAncestorOf)  );
		SG_ERR_CHECK(  SG_bitvector__count_set_bits(pCtx, pIsAncestorOf, &countIsAncestorOf)  );
	}

	SG_ERR_CHECK(  SG_strdup(pCtx, pszHidRef, ppOutputNodeHid)  );

	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
	SG_BITVECTOR_NULLFREE(pCtx, pIsAncestorOf);

	for(i=0; i<workQueue.length; ++i)
	{
		SG_DAGNODE_NULLFREE(pCtx, workQueue.p[i].pDagnode);
		SG_BITVECTOR_NULLFREE(pCtx, workQueue.p[i].pIsAncestorOf);
	}
	SG_NULLFREE(pCtx, workQueue.p);
	SG_RBTREE_NULLFREE(pCtx, workQueue.pRevnoCache);

	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pDagnodeFetcher)  );

	return;
fail:
	for(i=0; i<workQueue.length; ++i)
	{
		SG_DAGNODE_NULLFREE(pCtx, workQueue.p[i].pDagnode);
		SG_BITVECTOR_NULLFREE(pCtx, workQueue.p[i].pIsAncestorOf);
	}
	SG_NULLFREE(pCtx, workQueue.p);
	SG_RBTREE_NULLFREE(pCtx, workQueue.pRevnoCache);

	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
	SG_BITVECTOR_NULLFREE(pCtx, pIsAncestorOf);

	if(pDagnodeFetcher!=NULL)
	{
		SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pDagnodeFetcher)  );
	}
}

///////////////////////////////////////////////////////////////////////////////

void SG_repo__dag__find_direct_path_from_root(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid,
        SG_varray** ppva
        )
{
    SG_varray* new_pva = NULL;
#if SG_DOUBLE_CHECK__PATH_TO_ROOT
    SG_varray* old_pva = NULL;
    SG_dagnode* pdn = NULL;
    char* psz_cur = NULL;
    SG_string* pstr1 = NULL;
    SG_string* pstr2 = NULL;
#endif

    SG_ERR_CHECK(  SG_repo__find_dag_path(pCtx, pRepo, dagnum, NULL, psz_csid, &new_pva)  );

#if SG_DOUBLE_CHECK__PATH_TO_ROOT
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &old_pva)  );
    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_csid, &psz_cur)  );
    while (1)
    {
        SG_uint32 count_parents = 0;
        const char** a_parents = NULL;

        SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, psz_cur, &pdn)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, old_pva, psz_cur)  );
        SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pdn, &count_parents, &a_parents)  );
        if (0 == count_parents)
        {
            break;
        }
        SG_NULLFREE(pCtx, psz_cur);
        SG_ERR_CHECK(  SG_STRDUP(pCtx, a_parents[0], &psz_cur)  );
        SG_DAGNODE_NULLFREE(pCtx, pdn);
    }
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, old_pva, "")  );

    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr1)  );
    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr2)  );
    SG_ERR_CHECK(  SG_varray__to_json(pCtx, old_pva, pstr1)  );
    SG_ERR_CHECK(  SG_varray__to_json(pCtx, new_pva, pstr2)  );
    if (0 != strcmp(SG_string__sz(pstr1), SG_string__sz(pstr2)))
    {
        // a failure here isn't actually ALWAYS bad.  there can be more than one path
        // to root.

        fprintf(stderr, "old way:\n");
        SG_VARRAY_STDERR(old_pva);
        fprintf(stderr, "new way:\n");
        SG_VARRAY_STDERR(new_pva);

        SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
    }
#endif

    *ppva = new_pva;
    new_pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, new_pva);
#if SG_DOUBLE_CHECK__PATH_TO_ROOT
    SG_STRING_NULLFREE(pCtx, pstr1);
    SG_STRING_NULLFREE(pCtx, pstr2);
    SG_VARRAY_NULLFREE(pCtx, old_pva);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_NULLFREE(pCtx, psz_cur);
#endif
}

void SG_repo__dag__find_direct_backward_path(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid_from,
        const char* psz_csid_to,
        SG_varray** ppva
        )
{
    SG_ERR_CHECK_RETURN(  SG_repo__find_dag_path(pCtx, pRepo, dagnum, psz_csid_to, psz_csid_from, ppva)  );
}

#if SG_DOUBLE_CHECK__CALC_DELTA
static void loop_innards_make_delta(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_varray* pva_path,
    SG_uint32 i_path_step,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
    )
{
    SG_changeset* pcs = NULL;

    const char* psz_csid_cur = NULL;
    const char* psz_csid_parent = NULL;
    SG_vhash* pvh_changes = NULL;
    SG_vhash* pvh_one_parent_changes = NULL;
    SG_vhash* pvh_cs_add = NULL;
    SG_vhash* pvh_cs_remove = NULL;

    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_path, i_path_step, &psz_csid_cur)  );
    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_path, i_path_step + 1, &psz_csid_parent)  );
    SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, psz_csid_cur, &pcs)  );

    SG_ERR_CHECK(  SG_changeset__db__get_changes(pCtx, pcs, &pvh_changes)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_changes, psz_csid_parent, &pvh_one_parent_changes)  );

    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_parent_changes, "add", &pvh_cs_remove)  );
    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_parent_changes, "remove", &pvh_cs_add)  );

    if (pvh_cs_add)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_cs_add, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_hid_rec = NULL;
            SG_bool b = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_cs_add, i, &psz_hid_rec, NULL)  );
            SG_ERR_CHECK(  SG_vhash__remove_if_present(pCtx, pvh_remove, psz_hid_rec, &b)  );
            if (!b)
            {
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_add, psz_hid_rec)  );
            }
        }
    }

    if (pvh_cs_remove)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_cs_remove, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_hid_rec = NULL;
            SG_bool b = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_cs_remove, i, &psz_hid_rec, NULL)  );
            SG_ERR_CHECK(  SG_vhash__remove_if_present(pCtx, pvh_add, psz_hid_rec, &b)  );
            if (!b)
            {
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_remove, psz_hid_rec)  );
            }
        }
    }


fail:
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

static void old_SG_db__make_delta_from_path(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 dagnum,
    SG_varray* pva_path,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
    )
{
    SG_uint32 count = 0;
    SG_uint32 i_path_step = 0;
    SG_uint32 count_ops = 0;

    SG_UNUSED(dagnum);
    SG_NULLARGCHECK_RETURN(pRepo);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_path, &count)  );
    SG_ASSERT(count >= 2);

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Collecting deltas", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count-1, "changesets")  );

    for (i_path_step=0; i_path_step<(count-1); i_path_step++)
    {
        SG_ERR_CHECK(  loop_innards_make_delta(pCtx, pRepo, pva_path, i_path_step, pvh_add, pvh_remove)  );
        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    count_ops--;

fail:
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}
#endif

static void SG_db__make_delta_from_path(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 dagnum,
    SG_varray* pva_path,
    SG_uint32 flags,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
    )
{
#if SG_DOUBLE_CHECK__CALC_DELTA
    SG_int64 t1 = -1;
    SG_int64 t2 = -1;
    SG_vhash* new_pvh_add = NULL;
    SG_vhash* new_pvh_remove = NULL;
    SG_vhash* old_pvh_add = NULL;
    SG_vhash* old_pvh_remove = NULL;
    SG_string* old_pstr = NULL;
    SG_string* new_pstr = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &new_pvh_add, pvh_add)  );
    SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &new_pvh_remove, pvh_remove)  );
    SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &old_pvh_add, pvh_add)  );
    SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &old_pvh_remove, pvh_remove)  );

    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t1)  );
    SG_ERR_CHECK(  old_SG_db__make_delta_from_path(
                pCtx,
                pRepo,
                dagnum,
                pva_path,
                old_pvh_add,
                old_pvh_remove
                )  );
    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t2)  );
    {
        SG_uint32 path_length = 0;
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_path, &path_length)  );
        fprintf(stderr, "make_delta_from_path (%d)\n", path_length);
    }
    fprintf(stderr, "  time old %d ms\n", (int) (t2 - t1));

    SG_ERR_CHECK(  SG_vhash__sort(pCtx, old_pvh_add, SG_FALSE, SG_vhash_sort_callback__increasing)  );
    SG_ERR_CHECK(  SG_vhash__sort(pCtx, old_pvh_remove, SG_FALSE, SG_vhash_sort_callback__increasing)  );

    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t1)  );
    SG_ERR_CHECK(  SG_repo__dbndx__make_delta_from_path(
                pCtx,
                pRepo,
                dagnum,
                pva_path,
                0,
                new_pvh_add,
                new_pvh_remove
                )  );
    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t2)  );
    fprintf(stderr, "  time new %d ms\n", (int) (t2 - t1));
    SG_ERR_CHECK(  SG_vhash__sort(pCtx, new_pvh_add, SG_FALSE, SG_vhash_sort_callback__increasing)  );
    SG_ERR_CHECK(  SG_vhash__sort(pCtx, new_pvh_remove, SG_FALSE, SG_vhash_sort_callback__increasing)  );

    SG_ERR_CHECK(  SG_string__alloc(pCtx, &old_pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, old_pvh_add, old_pstr)  );
    SG_ERR_CHECK(  SG_string__alloc(pCtx, &new_pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, new_pvh_add, new_pstr)  );

    if (0 != strcmp(SG_string__sz(old_pstr), SG_string__sz(new_pstr)))
    {
        fprintf(stderr, "oldway:\n");
        SG_VHASH_STDERR(old_pvh_add);
        fprintf(stderr, "new:\n");
        SG_VHASH_STDERR(new_pvh_add);

        SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
    }

    SG_STRING_NULLFREE(pCtx, old_pstr);
    SG_STRING_NULLFREE(pCtx, new_pstr);

    SG_ERR_CHECK(  SG_string__alloc(pCtx, &old_pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, old_pvh_remove, old_pstr)  );
    SG_ERR_CHECK(  SG_string__alloc(pCtx, &new_pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, new_pvh_remove, new_pstr)  );

    if (0 != strcmp(SG_string__sz(old_pstr), SG_string__sz(new_pstr)))
    {
        fprintf(stderr, "oldway:\n");
        SG_VHASH_STDERR(old_pvh_remove);
        fprintf(stderr, "new:\n");
        SG_VHASH_STDERR(new_pvh_remove);

        SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
    }
#endif

#if SG_DOUBLE_CHECK__CALC_DELTA
    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t1)  );
#endif
    SG_ERR_CHECK(  SG_repo__dbndx__make_delta_from_path(
                pCtx,
                pRepo,
                dagnum,
                pva_path,
                flags,
                pvh_add,
                pvh_remove
                )  );
#if SG_DOUBLE_CHECK__CALC_DELTA
    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t2)  );
    fprintf(stderr, "  time NEW %d ms\n", (int) (t2 - t1));
#endif

fail:
#if SG_DOUBLE_CHECK__CALC_DELTA
    SG_STRING_NULLFREE(pCtx, old_pstr);
    SG_STRING_NULLFREE(pCtx, new_pstr);

    SG_VHASH_NULLFREE(pCtx, old_pvh_add);
    SG_VHASH_NULLFREE(pCtx, old_pvh_remove);
    SG_VHASH_NULLFREE(pCtx, new_pvh_add);
    SG_VHASH_NULLFREE(pCtx, new_pvh_remove);
#endif
    ;
}

void SG_repo__db__calc_delta(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid_from,
        const char* psz_csid_to,
        SG_uint32 flags,
        SG_vhash** ppvh_add,
        SG_vhash** ppvh_remove
        )
{
    SG_dagnode* pdn_from = NULL;
    SG_dagnode* pdn_to = NULL;
    SG_int32 gen_from = -1;
    SG_int32 gen_to = -1;
    SG_varray* pva_direct_backward_path = NULL;
    SG_varray* pva_direct_forward_path = NULL;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;
    SG_rbtree* prb_temp = NULL;
    SG_daglca* plca = NULL;
    char* psz_csid_ancestor = NULL;

    SG_NULLARGCHECK_RETURN(psz_csid_from);
    SG_NULLARGCHECK_RETURN(psz_csid_to);
    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(ppvh_add);
    SG_NULLARGCHECK_RETURN(ppvh_remove);

    SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, psz_csid_from, &pdn_from)  );
    SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn_from, &gen_from)  );
    SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, psz_csid_to, &pdn_to)  );
    SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn_to, &gen_to)  );

    if (gen_from > gen_to)
    {
        SG_ERR_CHECK(  SG_repo__dag__find_direct_backward_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    psz_csid_from,
                    psz_csid_to,
                    &pva_direct_backward_path
                    )  );
        if (pva_direct_backward_path)
        {
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_add)  );
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_remove)  );
            SG_ERR_CHECK(  SG_db__make_delta_from_path(
                        pCtx,
                        pRepo,
                        dagnum,
                        pva_direct_backward_path,
                        flags,
                        pvh_add,
                        pvh_remove
                        )  );
        }
    }
    else if (gen_from < gen_to)
    {
        SG_ERR_CHECK(  SG_repo__dag__find_direct_backward_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    psz_csid_to,
                    psz_csid_from,
                    &pva_direct_forward_path
                    )  );
        if (pva_direct_forward_path)
        {
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_add)  );
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_remove)  );
            SG_ERR_CHECK(  SG_db__make_delta_from_path(
                        pCtx,
                        pRepo,
                        dagnum,
                        pva_direct_forward_path,
                        flags,
                        pvh_remove,
                        pvh_add
                        )  );
        }
    }

    if (!pvh_add && !pvh_remove)
    {
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_temp)  );
        SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb_temp,psz_csid_from)  );
        SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb_temp,psz_csid_to)  );
        SG_ERR_CHECK(  SG_repo__get_dag_lca(pCtx,pRepo,dagnum,prb_temp,&plca)  );
        {
            const char* psz_hid = NULL;
            SG_daglca_node_type node_type = 0;
            SG_int32 gen = -1;

            SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx,
                                                      NULL,
                                                      plca,
                                                      SG_FALSE,
                                                      &psz_hid,
                                                      &node_type,
                                                      &gen,
                                                      NULL)  );
            SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid, &psz_csid_ancestor)  );
        }

        SG_ERR_CHECK(  SG_repo__dag__find_direct_backward_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    psz_csid_from,
                    psz_csid_ancestor,
                    &pva_direct_backward_path
                    )  );
        SG_ERR_CHECK(  SG_repo__dag__find_direct_backward_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    psz_csid_to,
                    psz_csid_ancestor,
                    &pva_direct_forward_path
                    )  );
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_add)  );
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_remove)  );
        SG_ERR_CHECK(  SG_db__make_delta_from_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    pva_direct_backward_path,
                    flags,
                    pvh_add,
                    pvh_remove
                    )  );
        SG_ERR_CHECK(  SG_db__make_delta_from_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    pva_direct_forward_path,
                    flags,
                    pvh_remove,
                    pvh_add
                    )  );
    }

    *ppvh_add = pvh_add;
    pvh_add = NULL;

    *ppvh_remove = pvh_remove;
    pvh_remove = NULL;

fail:
    SG_NULLFREE(pCtx, psz_csid_ancestor);
    SG_RBTREE_NULLFREE(pCtx, prb_temp);
    SG_DAGLCA_NULLFREE(pCtx, plca);
    SG_VHASH_NULLFREE(pCtx, pvh_add);
    SG_VHASH_NULLFREE(pCtx, pvh_remove);
    SG_VARRAY_NULLFREE(pCtx, pva_direct_backward_path);
    SG_VARRAY_NULLFREE(pCtx, pva_direct_forward_path);
    SG_DAGNODE_NULLFREE(pCtx, pdn_from);
    SG_DAGNODE_NULLFREE(pCtx, pdn_to);
}

void SG_repo__db__calc_delta_from_root(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid_to,
        SG_uint32 flags,
        SG_vhash** ppvh
        )
{
    SG_varray* pva_direct_forward_path = NULL;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;

    SG_NULLARGCHECK_RETURN(psz_csid_to);
    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(ppvh);

    SG_ERR_CHECK(  SG_repo__dag__find_direct_path_from_root(
                pCtx,
                pRepo,
                dagnum,
                psz_csid_to,
                &pva_direct_forward_path
                )  );
    if (pva_direct_forward_path)
    {
        SG_uint32 count_remove = 0;

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_add)  );
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_remove)  );
        SG_ERR_CHECK(  SG_db__make_delta_from_path(
                    pCtx,
                    pRepo,
                    dagnum,
                    pva_direct_forward_path,
                    flags,
                    pvh_remove,
                    pvh_add
                    )  );

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_remove, &count_remove)  );
        if (count_remove)
        {
            SG_uint32 i = 0;

            // TODO it would be nice to have a batch remove so vhash would only have to rehash once
            for (i=0; i<count_remove; i++)
            {
                const char* psz = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_remove, i, &psz, NULL)  );
                SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_add, psz)  );
            }
        }
    }

    *ppvh = pvh_add;
    pvh_add = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_add);
    SG_VHASH_NULLFREE(pCtx, pvh_remove);
    SG_VARRAY_NULLFREE(pCtx, pva_direct_forward_path);
}

