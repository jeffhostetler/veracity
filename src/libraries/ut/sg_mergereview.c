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
 * @file sg_mergereview.c
 *
 * @details
 *
 */

#include <sg.h>


///////////////////////////////////////////////////////////////////////////////

/* Implementation Notes... Defining Some Terms...

Paul Egli, August 2012

SG_mergereview walks the dag back from a given node and returns a good set of
data to construct a simplified view of the history of that node based on the
"merged-in" order of nodes. An example using hypothetical revno's looks like
this:

(95)
  | \
  |  (94)
  |    |
  |  (85)
  |
(90)
  |
(89)
  | \
  |  (88)
  |
(80)
  | \
  |  (79)
  |    | \
  |    |  (76)
  |    |    |
  |    |  (71)
  |    |    |
  |    |  (70)
  |    |
  |  (74)
  |    |
  |  (66)
  |    |
  |  (65)
  |
(78)
  | \
 ... ...

Walking back, whenever we encounter a vc merge (ie a merge node in the
VERSION_CONTROL dag), we do a best guess at which side of the merge was "brought
in" (or "new") and which side was "already there" (or "old"). The "new" side of
the merge gets indented to the right, and grouped together, even if this doesn't
follow revno (or even generation) ordering.

This is conceptually similar to the "merge-preview" command, but is looking in
review, not in preview. The idea also needs to be applied recursively when
another merge is met within the set of "new" nodes for a given merge.

Our best guess for "new" (and "old") nodes is decided as follows. A vc parent
by the same user is considered newer since the same user is likely the be the
one to merge in their own new work. Otherwise we order them by the highest revno
being the newest since it is more likely to have been received within the same
push (or pull) transaction as the merge.

Now, an insight into how this view of the history can be generated
programmatically comes from fact that the result is a tree. Thus the data we
generate will be referred to as "the tree".

While generating the tree, we need to jump around between the leaves, updating
them as new information comes in, and finalizing them in an order that doesn't
match the order they'll be displayed in. Thus we almost always walk more nodes
in the vc dag than we show results for.

Because the tree's parent/child relationships are the reverse of what they were
in the vc dag, we will use the more explicit terms "display parent" and "display
child" to try to avoid confusion with "vc child" and "vc parent" respectively.

The display parent of a node is the vc child of that node that gets a line drawn
to it in the display of the results. A node might have many vc children, but it
will only ever have one display parent. The only node that doesn't have exactly
one display parent is the root. (D'uh. It's a tree.)

A display child of a node is a vc parent of that node that gets a line drawn to
it in the display of the results. Some vc merges show both vc parents as display
children, such as node #79 the example. Others don't. For example, node #71
could be a vc merge of #70 and #66 and the above diagram would still be correct.
Node #66 still would not be a display child of #71, and (equivalently) #71 still
wouldn't be a display parent of #66. Some nodes, like #85 and #88, have no
display children, even though their vc parent(s) may be onscreen.

*/

//
// Node type for "the tree".
//

typedef struct _node_struct _node_t;

typedef struct _node_list_struct _node_list_t;
struct _node_list_struct
{
	_node_t ** p;
	SG_uint32 count;
	SG_uint32 countAllocated;
};

struct _node_struct
{
	SG_dagnode * pDagnode;
	SG_uint32 revno;
	const char * pszHidRef;

	// Changeset audits. Populated as needed (ie, to help decide which
	// side of a merge is "new" and which side is "old".)
	SG_varray * pAudits;

	// Csid-to-revno map of all VC parents. This is not actually used when
	// creating the tree--that's just when the most convenient time to fetch
	// this information is. (It's just part of making the results match SG_history
	// results.)
	SG_vhash * pVcParents;

	// A node is considered "pending" after it has been added to the
	// tree until its VC parents have been fetched.
	SG_bool isPending;
	
	_node_t * pDisplayParent;
	_node_list_t displayChildren;
};

static SG_bool _node__has_pending_children(_node_t * pNode)
{
	SG_uint32 i;
	for(i=0; i<pNode->displayChildren.count; ++i)
	{
		if(pNode->displayChildren.p[i]->isPending)
			return SG_TRUE;
	}
	return SG_FALSE;
}

static void _node__free_nonreusable_memory(SG_context * pCtx, _node_t * pNode)
{
	SG_ASSERT(pNode!=NULL);
	SG_DAGNODE_NULLFREE(pCtx, pNode->pDagnode);
	SG_VARRAY_NULLFREE(pCtx, pNode->pAudits);
	SG_VHASH_NULLFREE(pCtx, pNode->pVcParents);
}

static void _node_list__uninit(_node_list_t * pList);

static void _node__nullfree(SG_context * pCtx, _node_t ** ppNode)
{
	if(ppNode==NULL || *ppNode==NULL)
		return;
	else
	{
		SG_uint32 i;
		_node_t * pNode = *ppNode;

		_node__free_nonreusable_memory(pCtx, pNode);

		for(i=0; i<pNode->displayChildren.count; ++i)
			_node__nullfree(pCtx, &pNode->displayChildren.p[i]);
		_node_list__uninit(&pNode->displayChildren);

		SG_NULLFREE(pCtx, pNode);
		*ppNode = NULL;
	}
}

//
// Definitions for _node_list, an ordered list container for nodes, used in several places.
//

static void _node_list__init(SG_context * pCtx, _node_list_t * pList, SG_uint32 countAllocateUpfront)
{
	SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, countAllocateUpfront, pList->p)  );
	pList->countAllocated = countAllocateUpfront;
}

static void _node_list__uninit(_node_list_t * pList)
{
	SG_free__no_ctx(pList->p);
	pList->p = NULL;
	pList->countAllocated = 0;
	pList->count = 0;
}

// Insert one or more nodes into a node list. Does not assume the list is taking ownership of the nodes.
static void _node_list__insert_at(SG_context * pCtx, _node_list_t * pList, SG_uint32 index, _node_t ** ppNewNodes, SG_uint32 countNewNodes)
{
	if (pList->countAllocated < pList->count+countNewNodes)
	{
		_node_t ** tmp = NULL;
		while(pList->countAllocated < pList->count+countNewNodes)
			pList->countAllocated *= 2;
		SG_ERR_CHECK_RETURN(  SG_allocN(pCtx, pList->countAllocated, tmp)  );
		(void)memmove(tmp, pList->p, pList->count*sizeof(pList->p[0]));
		SG_NULLFREE(pCtx, pList->p);
		pList->p = tmp;
	}
	
	if (index < pList->count)
		(void)memmove(&pList->p[index+countNewNodes], &pList->p[index], (pList->count-index)*sizeof(pList->p[0]));
	
	memcpy(&pList->p[index], ppNewNodes, countNewNodes*sizeof(ppNewNodes[0]));
	pList->count += countNewNodes;
}

// Add a single node to the end of a list. Assumes the list is taking ownership of the node, and nulls the caller's copy.
static void _node_list__append(SG_context * pCtx, _node_list_t * pList, _node_t ** ppNode)
{
	SG_ERR_CHECK_RETURN(  _node_list__insert_at(pCtx, pList, pList->count, ppNode, 1)  );
	*ppNode = NULL;
}

// Remove the node at a certain index in a list.
static void _node_list__remove_at(_node_list_t * pList, SG_uint32 index)
{
	--pList->count;
	if (index < pList->count)
		(void)memmove(&pList->p[index], &pList->p[index+1], (pList->count-index)*sizeof(pList->p[0]));
}

// Removes a certain node from a list and stores it in the output parameter.
static void _node_list__remove(_node_list_t * pList, _node_t * pNode, _node_t ** ppNode)
{
	SG_uint32 index = 0;
	while(index < pList->count && pList->p[index]!=pNode)
		++index;
	if(index < pList->count)
		_node_list__remove_at(pList, index);
	*ppNode = pNode;
}

static void _node_list__sort(_node_list_t * pList, SG_uint32 iBaseline)
{
	_node_t ** p = pList->p;
	SG_uint32 len = pList->count;

	// First move baseline node to front of list if applicable:
	if(iBaseline < pList->count)
	{
		if(iBaseline!=0)
		{
			_node_t * tmp = pList->p[iBaseline];
			pList->p[iBaseline] = pList->p[0];
			pList->p[0] = tmp;
		}
		++p;
		--len;
	}

	// Scalability not an issue. Just bubble sort the rest.
	if(len>1)
	{
		SG_uint32 outer, i;
		for(outer=0; outer<len-1; ++outer)
		{
			for(i=0; i < len-1-outer; ++i)
			{
				if(pList->p[i]->revno > pList->p[i+1]->revno)
				{
					_node_t * tmp = pList->p[i+1];
					pList->p[i+1] = pList->p[i];
					pList->p[i] = tmp;
				}
			}
		}
	}
}

//
// General state for "the tree".
//

typedef struct _tree_struct _tree_t;
struct _tree_struct
{
	_node_t * pRoot;
	
	// List of nodes who have been visited in the vc dag and are in the tree,
	// but whose position in the tree has not been solidified. The list is kept
	// sorted by their current position in the tree, left to right.
	_node_list_t pending;

	// Pointer to the next node to be added to the results.
	_node_t * pNextResult;

	// How far pNextResult is from the left side of "the tree".
	SG_int32 indentLevel;
	
	// To save on memory allocations.
	_node_t * pFreeList;

	// Helpers to get data from the repo.
	SG_repo * pRepoRef;
	SG_repo_fetch_dagnodes_handle * pDagnodeFetcher;
};

static void _tree__add_new_node(SG_context * pCtx, _tree_t * pTree, _node_t * pDisplayParent, const char * pszHid, _node_t ** ppNewNodeRef);
static void _tree__uninit(SG_context * pCtx, _tree_t * pTree);

static void _tree__init(SG_context * pCtx, _tree_t * pTree, const char * szHidHead, SG_repo * pRepo)
{
	memset(pTree, 0, sizeof(*pTree));
	pTree->pRepoRef = pRepo;
	SG_ERR_CHECK_RETURN(  SG_repo__fetch_dagnodes__begin(pCtx, pTree->pRepoRef, SG_DAGNUM__VERSION_CONTROL, &pTree->pDagnodeFetcher)  );
	SG_ERR_CHECK(  _node_list__init(pCtx, &pTree->pending, 16)  );

	SG_ERR_CHECK(  _tree__add_new_node(pCtx, pTree, NULL, szHidHead, NULL)  );

	pTree->pending.p[0] = pTree->pRoot;
	pTree->pending.count = 1;

	pTree->pNextResult = pTree->pRoot;

	return;
fail:
	_tree__uninit(pCtx, pTree);
}

// Helper function for _tree__init__continuation()
static void _tree__add_subtoken(SG_context * pCtx, _tree_t * pTree, _node_t * pParent, SG_varray * pSubtoken)
{
	SG_int64 revno = 0;
	char * szHid = NULL;
	_node_t * pNodeRef = NULL;
	SG_uint32 count = 0;
	SG_uint32 i = 0;
	SG_uint16 lastElementType = 0;
	SG_varray * pSubsubtoken = NULL;
	SG_stringarray * psa = NULL;

	++pTree->indentLevel;

	SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pSubtoken, 0, &revno)  );
	SG_ERR_CHECK(  SG_repo__find_dagnode_by_rev_id(pCtx, pTree->pRepoRef, SG_DAGNUM__VERSION_CONTROL, (SG_uint32)revno, &szHid)  );
	SG_ERR_CHECK(  _tree__add_new_node(pCtx, pTree, pParent, szHid, &pNodeRef)  );
	pNodeRef->isPending = SG_FALSE;
	SG_NULLFREE(pCtx, szHid);

	SG_ERR_CHECK(  SG_varray__count(pCtx, pSubtoken, &count)  );
	SG_ERR_CHECK(  SG_varray__typeof(pCtx, pSubtoken, count-1, &lastElementType)  );
	if(lastElementType==SG_VARIANT_TYPE_VARRAY)
	{
		SG_ERR_CHECK(  SG_varray__get__varray(pCtx, pSubtoken, count-1, &pSubsubtoken)  );
		--count;
	}

	for(i=1; i<count; ++i)
	{
		_node_t * pRefChildNode = NULL;
		SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pSubtoken, i, &revno)  );
		if(i==1 && revno==0)
		{
			// Revno 0 in the as the first child means "Use the LCA for the baseline".
			SG_uint32 j = 0;

			if(pSubsubtoken!=NULL)
			{
				// LCA would not include nodes in the sub-token. Just error out.
				SG_ERR_THROW(SG_ERR_INVALIDARG);
			}

			if(count-2<2)
			{
				// This would be interpreted as merging a node with itself or with nothing.
				SG_ERR_THROW(SG_ERR_INVALIDARG);
			}

			SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count-2)  );
			for(j=2; j<count; ++j)
			{
				SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pSubtoken, j, &revno)  );
				SG_ERR_CHECK(  SG_repo__find_dagnode_by_rev_id(pCtx, pTree->pRepoRef, SG_DAGNUM__VERSION_CONTROL, (SG_uint32)revno, &szHid)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, szHid)  );
				SG_NULLFREE(pCtx, szHid);
			}
			SG_ERR_CHECK(  SG_dagquery__highest_revno_common_ancestor(pCtx, pTree->pRepoRef, SG_DAGNUM__VERSION_CONTROL, psa, &szHid)  );
			SG_STRINGARRAY_NULLFREE(pCtx, psa);
		}
		else
		{
			SG_ERR_CHECK(  SG_repo__find_dagnode_by_rev_id(pCtx, pTree->pRepoRef, SG_DAGNUM__VERSION_CONTROL, (SG_uint32)revno, &szHid)  );
		}
		SG_ERR_CHECK(  _tree__add_new_node(pCtx, pTree, pNodeRef, szHid, &pRefChildNode)  );
		pTree->pNextResult = pRefChildNode;
		SG_ERR_CHECK(  _node_list__append(pCtx, &pTree->pending, &pRefChildNode)  );
		SG_NULLFREE(pCtx, szHid);
	}

	if(pSubsubtoken!=NULL)
	{
		SG_ERR_CHECK(  _tree__add_subtoken(pCtx, pTree, pNodeRef, pSubsubtoken)  );
	}

	return;
fail:
	SG_NULLFREE(pCtx, szHid);
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

static void _tree__init__continuation(SG_context * pCtx, _tree_t * pTree, SG_varray * pContinuationToken, SG_repo * pRepo)
{
	memset(pTree, 0, sizeof(*pTree));
	pTree->pRepoRef = pRepo;
	SG_ERR_CHECK_RETURN(  SG_repo__fetch_dagnodes__begin(pCtx, pTree->pRepoRef, SG_DAGNUM__VERSION_CONTROL, &pTree->pDagnodeFetcher)  );
	SG_ERR_CHECK(  _node_list__init(pCtx, &pTree->pending, 16)  );

	SG_ERR_CHECK(  _tree__add_subtoken(pCtx, pTree, NULL, pContinuationToken)  );

	if(pTree->pNextResult->pDisplayParent->displayChildren.count==1)
		--pTree->indentLevel;

	return;
fail:
	_tree__uninit(pCtx, pTree);
}

static void _tree__uninit(SG_context * pCtx, _tree_t * pTree)
{
	if(pTree->pDagnodeFetcher!=NULL)
	{
		SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pTree->pRepoRef, &pTree->pDagnodeFetcher)  );
	}
	_node_list__uninit(&pTree->pending);
	_node__nullfree(pCtx, &pTree->pRoot);
	_node__nullfree(pCtx, &pTree->pFreeList);
}

static void _tree__add_new_node(SG_context * pCtx, _tree_t * pTree, _node_t * pDisplayParent, const char * pszHid, _node_t ** ppNewNodeRef)
{
	// Adds a new node to the tree. The new node is considered 'pending', though
	// it not actually added to the pending list at this point.
	_node_t * pNode = NULL;
	
	// Get the memory.
	if(pTree->pFreeList==NULL)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pNode)  );
		SG_ERR_CHECK(  _node_list__init(pCtx, &pNode->displayChildren, 2)  );
	}
	else
	{
		pNode = pTree->pFreeList;
		pTree->pFreeList = pNode->displayChildren.p[0];
		pNode->displayChildren.count = 0;
	}
	
	if(ppNewNodeRef!=NULL)
		*ppNewNodeRef = pNode;
	
	// Populate the node.
	pNode->pDisplayParent = pDisplayParent;
	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pTree->pRepoRef, pTree->pDagnodeFetcher, pszHid, &pNode->pDagnode)  );
	SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pNode->pDagnode, &pNode->revno)  );
	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pNode->pDagnode, &pNode->pszHidRef)  );
	pNode->isPending = SG_TRUE;
	
	// Add the node to its display parent.
	if(pDisplayParent!=NULL)
	{
		SG_ERR_CHECK(  _node_list__append(pCtx, &pDisplayParent->displayChildren, &pNode)  );
	}
	else
	{
		SG_ASSERT(pTree->pRoot==NULL);
		pTree->pRoot = pNode;
	}
	
	return;
fail:
	_node__nullfree(pCtx, &pNode);
}

static void _tree__move_node(SG_context * pCtx, _node_t * pNode, _node_t * pNewParent)
{
	_node_t * pNodeAsOurs = NULL; // Same as pNode, but we own the memory.
	
	// remove from old parent
	_node_list__remove(&pNode->pDisplayParent->displayChildren, pNode, &pNodeAsOurs);
	
	// add to new parent
	SG_ERR_CHECK(  _node_list__append(pCtx, &pNewParent->displayChildren, &pNodeAsOurs)  );
	pNode->pDisplayParent = pNewParent;
	
	return;
fail:
	_node__nullfree(pCtx, &pNodeAsOurs);
}

static void _user_match_found(SG_context * pCtx, SG_repo * pRepo, _node_t *pNode1, _node_t * pNode2, SG_bool *matchFound)
{
	SG_uint32 countAudits1 = 0;
	SG_uint32 countAudits2 = 0;
	SG_uint32 iAudit1;

	if(pNode1->pAudits==NULL)
	{
		SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pNode1->pszHidRef, &pNode1->pAudits)  );
	}
	if(pNode2->pAudits==NULL)
	{
		SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pNode2->pszHidRef, &pNode2->pAudits)  );
	}


	SG_ERR_CHECK(  SG_varray__count(pCtx, pNode1->pAudits, &countAudits1)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pNode2->pAudits, &countAudits2)  );
	for(iAudit1=0; iAudit1<countAudits1; ++iAudit1)
	{
		SG_vhash * pAuditRef = NULL;
		SG_uint32 iAudit2;

		const char * szAudit1 = NULL;
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pNode1->pAudits, iAudit1, &pAuditRef)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pAuditRef, "userid", &szAudit1)  );

		for(iAudit2=0; iAudit2<countAudits2; ++iAudit2)
		{
			const char * szAudit2 = NULL;
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pNode2->pAudits, iAudit2, &pAuditRef)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pAuditRef, "userid", &szAudit2)  );
			if(strcmp(szAudit1, szAudit2)==0)
			{
				*matchFound = SG_TRUE;
				return;
			}
		}
	}

	*matchFound = SG_FALSE;

	return;
fail:
	;
}

static void _tree__process_next_pending_item(SG_context * pCtx, _tree_t * pTree, SG_vhash * pMergeBaselines)
{
	SG_uint32 i;
	
	_node_t * pNode = NULL; // The node we are processing.
	SG_uint32 iNode = 0; // Index of pNode in the 'pending' list.
	
	SG_uint32 countVcParents = 0;
	const char ** paszVcParentHids = NULL;
	SG_uint32 iVcParent;
	
	// The first pending node that needs to be processed is always the one with
	// the highest revno. Find it in the list.
	for(i=1; i < pTree->pending.count; ++i)
	{
		if(pTree->pending.p[i]->revno > pTree->pending.p[iNode]->revno)
			iNode = i;
	}
	pNode = pTree->pending.p[iNode];
	
	// Load in the node's display children/vc parents.
	SG_ASSERT(pNode->displayChildren.count==0);
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pNode->pVcParents)  );
	SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pNode->pDagnode, &countVcParents, &paszVcParentHids)  );
	for(iVcParent=0; iVcParent<countVcParents; ++iVcParent)
	{
		// Each vc parent is a candidate display child.
		const char * pszHidCandidate = paszVcParentHids[iVcParent];
		_node_t * pNodeRef = NULL;
		
		// Scan through the list of 'pending' nodes to see if we have already
		// fetched this one...
		SG_uint32 iCandidate = pTree->pending.count;
		for(i=0; i < pTree->pending.count && iCandidate==pTree->pending.count; ++i)
		{
			if(strcmp(pTree->pending.p[i]->pszHidRef, pszHidCandidate)==0)
			{
				iCandidate = i;
				pNodeRef = pTree->pending.p[i];
			}
		}
		
		if(iCandidate == pTree->pending.count)
		{
			// Node was not found. Add it new.
			SG_ERR_CHECK(  _tree__add_new_node(pCtx, pTree, pNode, pszHidCandidate, &pNodeRef)  );
		}
		else if(iCandidate > iNode)
		{
			// Node was found further to the right in the tree. Steal it.
			SG_ERR_CHECK(  _tree__move_node(pCtx, pTree->pending.p[iCandidate], pNode)  );
			
			// Also, remove it from the pending list. (It gets re-added later.)
			_node_list__remove_at(&pTree->pending, iCandidate);
		}
		else
		{
			// Node was found further to the left. Do nothing.
		}

		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pNode->pVcParents, pszHidCandidate, pNodeRef->revno)  );
	}
	
	// We have all this node's display children (still pending--they could get
	// stolen later). Now we need to sort them.
	
	if(pNode->displayChildren.count>1)
	{
		// First we pick one to go on the far left, if one stands out as most likely
		// to be the "old"/baseline node into which the others were "brought in".
		SG_uint32 iBaseline = pNode->displayChildren.count;
		
		// Allow the caller to have hand-picked the baseline node:
		if(pMergeBaselines!=NULL)
		{
			SG_int_to_string_buffer sz;

			SG_int64 baseline = 0;
			SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pMergeBaselines, SG_int64_to_sz(pNode->revno, sz), &baseline)  );
			if(baseline!=0)
			{
				for(i=0; i<pNode->displayChildren.count; ++i)
				{
					if(pNode->displayChildren.p[i]->revno==(SG_uint32)baseline)
					{
						iBaseline = i;
						break;
					}
				}
			}
		}
		
		if(iBaseline == pNode->displayChildren.count)
		{
			// No baseline node from the user. See if there's one unique node whose
			// user *doesn't* match.
			for(i=0; i<pNode->displayChildren.count; ++i)
			{
				SG_bool match = SG_FALSE;
				SG_ERR_CHECK(  _user_match_found(pCtx, pTree->pRepoRef, pNode->displayChildren.p[i], pNode, &match)  );
				if(!match)
				{
					if(iBaseline == pNode->displayChildren.count)
					{
						iBaseline = i;
					}
					else
					{
						// Whoops. "Nevermind."
						iBaseline = pNode->displayChildren.count;
						break;
					}
				}
			}
		}
		
		// Finally, sort
		_node_list__sort(&pNode->displayChildren, iBaseline);
	}
	
	// In the 'pending' list, replace this node with its children.
	if(pNode->displayChildren.count == 0)
		_node_list__remove_at(&pTree->pending, iNode);
	else
	{
		pTree->pending.p[iNode] = pNode->displayChildren.p[0];
		if(pNode->displayChildren.count > 1)
		{
			SG_ERR_CHECK(  _node_list__insert_at(pCtx,
				&pTree->pending,
				iNode+1,
				&pNode->displayChildren.p[1],
				pNode->displayChildren.count-1)  );
		}
	}
	
	// This node is no longer pending.
	pNode->isPending = SG_FALSE;

	return;
fail:
	;
}

static void _tree__generate_continuation_token(SG_context * pCtx, _tree_t * pTree, SG_varray * pContinuationToken)
{
	SG_varray * pCurrent = pContinuationToken;
	_node_t * p;
	SG_bool done = SG_FALSE;
	for(p=pTree->pRoot; p!=pTree->pNextResult; p=p->displayChildren.p[p->displayChildren.count-1])
	{
		if(p->displayChildren.count > 1){
			SG_uint32 i;
			SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pCurrent, p->revno)  );
			for(i=0; i < p->displayChildren.count-1; ++i)
			{
				SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pCurrent, p->displayChildren.p[i]->revno)  );
			}
			if(p->displayChildren.p[p->displayChildren.count-1]==pTree->pNextResult)
			{
				SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pCurrent, pTree->pNextResult->revno)  );
				done = SG_TRUE;
			}
			else
			{
				SG_varray * pNext = NULL;
				SG_ERR_CHECK(  SG_varray__appendnew__varray(pCtx, pCurrent, &pNext)  );
				pCurrent = pNext;
			}
		}
	}

	if(!done)
	{
		SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pCurrent, pTree->pNextResult->pDisplayParent->revno)  );
		SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pCurrent, pTree->pNextResult->revno)  );
	}

	return;
fail:
	;
}

static void _tree__add_next_node_to_results(SG_context * pCtx, _tree_t * pTree, SG_varray * pResults, SG_uint32 * pCountResults, SG_bool * pbLastResultWasIndented)
{
	SG_vhash * pResult = NULL;
	SG_varray * pChildren = NULL;
	SG_uint32 i;
	SG_varray * pTmp = NULL;

	// Add pTree->pNextResult to results list.
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pResults, &pResult)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResult, "revno", pTree->pNextResult->revno)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pResult, "changeset_id", pTree->pNextResult->pszHidRef)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pResult, "parents", &pTree->pNextResult->pVcParents)  );
	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pResult, "displayChildren", &pChildren)  );
	for(i=0; i<pTree->pNextResult->displayChildren.count; ++i)
		SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pChildren, pTree->pNextResult->displayChildren.p[i]->revno)  );
	if(pTree->pNextResult->displayChildren.count>1 && pTree->indentLevel>0)
	{
		SG_varray * pContinuationToken = NULL;
		SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pResult, "continuationToken", &pContinuationToken)  );
		SG_ERR_CHECK(  _tree__generate_continuation_token(pCtx, pTree, pContinuationToken)  );
	}
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResult, "indent", pTree->indentLevel)  );
	*pbLastResultWasIndented = (pTree->indentLevel > 0);
	if(pTree->pNextResult->pDisplayParent!=NULL)
	{
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResult, "displayParent", pTree->pNextResult->pDisplayParent->revno)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResult, "indexInParent", pTree->pNextResult->pDisplayParent->displayChildren.count)  );
		SG_ASSERT(*pbLastResultWasIndented);
	}
	else
	{
		SG_ASSERT(!*pbLastResultWasIndented);
	}
	++(*pCountResults);

	// Advance pTree->pNextResult pointer to next result.
	while(pTree->pNextResult->displayChildren.count==0 && pTree->pNextResult!=pTree->pRoot)
	{
		// We have already added this node and all children to the results.
		// Free the memory that will not be reused, and then put the node
		// on the "free list".
		_node__free_nonreusable_memory(pCtx, pTree->pNextResult);
		pTree->pNextResult->displayChildren.p[0] = pTree->pFreeList;
		pTree->pNextResult->displayChildren.count = 1;
		pTree->pFreeList = pTree->pNextResult;

		// Move back up up in the tree.
		pTree->pNextResult = pTree->pNextResult->pDisplayParent;

		if(pTree->pNextResult!=NULL)
		{
			// The node we just freed... Remove it from its display parent too.
			--pTree->pNextResult->displayChildren.count;

			// All children but the leftmost one are indented by 1 from the parent.
			if(pTree->pNextResult->displayChildren.count>0)
				--pTree->indentLevel;
		}
	}
	if(pTree->pNextResult->displayChildren.count>0)
	{
		SG_uint32 i = pTree->pNextResult->displayChildren.count-1;
		pTree->pNextResult = pTree->pNextResult->displayChildren.p[i];
		if(i>=1)
			++pTree->indentLevel;
	}
	else
	{
		pTree->pNextResult = NULL;
	}

	// If we advanced past root...
	if(pTree->pNextResult==NULL || pTree->pNextResult==pTree->pRoot->displayChildren.p[0])
	{
		// Out with the old root, in with the new...

		_node__free_nonreusable_memory(pCtx, pTree->pRoot);
		pTree->pRoot->displayChildren.p[0] = pTree->pFreeList;
		pTree->pRoot->displayChildren.count = 1;
		pTree->pFreeList = pTree->pRoot;

		pTree->pRoot = pTree->pNextResult;
		if(pTree->pRoot!=NULL)
			pTree->pRoot->pDisplayParent = NULL;
	}

	return;
fail:
	SG_VARRAY_NULLFREE(pCtx, pTmp);
}

//
// Main functions.
//

static void _sg_mergereview(SG_context * pCtx, _tree_t * pTree, SG_int32 singleMergeReview, SG_bool firstChunk, SG_vhash * pMergeBaselines, SG_uint32 resultLimit, SG_varray ** ppResults, SG_uint32 * pCountResults, SG_varray ** ppContinuationToken)
{
	SG_varray * pResults = NULL;
	SG_uint32 countResults = 0;
	SG_bool lastResultWasIndented = SG_FALSE;

	SG_varray * pContinuationToken = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(pTree!=NULL);
	SG_ASSERT(ppResults!=NULL);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pResults)  );
	
	while(
			// Stop if we have have reached the result limit.
			! (countResults >= resultLimit)

			// Stop if we have completed a "single merge review" that was asked for.
			&& ! (pTree->indentLevel == 0 && singleMergeReview && (countResults>1 || !firstChunk))

			// Stop if we've walked all the way back to root of the vc dag.
			&& ! (pTree->pNextResult==NULL)

		)
	{
		// Process the next item in the pending list.
		// Note that this may cause any number of results to become available.
		SG_ERR_CHECK(  _tree__process_next_pending_item(pCtx, pTree, pMergeBaselines)  );

		// Fetch results until we don't need anymore or there's none available.
		while(

			// Stop if we have have reached the result limit.
			! (countResults >= resultLimit)

			// Stop if we have completed a "single merge review" that was asked for.
			&& ! (pTree->indentLevel == 0 && singleMergeReview && (countResults>1 || !firstChunk))

			// Stop if we've walked all the way back to root of the vc dag.
			&& ! (pTree->pNextResult==NULL)

			// Stop if the next node is in a pending state.
			// (Can only happen when we were starting from a "continuation token".)
			&& ! (pTree->pNextResult->isPending)

			// Stop if the next node has any display children in a pending state.
			&& ! (_node__has_pending_children(pTree->pNextResult))

			)
		{
			SG_ERR_CHECK(  _tree__add_next_node_to_results(pCtx, pTree, pResults, &countResults, &lastResultWasIndented)  );
		}
	}

	if(countResults<resultLimit && lastResultWasIndented)
	{
		SG_ASSERT(pTree->pNextResult!=NULL); // VC root will never be indented.
		SG_ERR_CHECK(  _tree__add_next_node_to_results(pCtx, pTree, pResults, &countResults, &lastResultWasIndented)  );
	}

	if(ppContinuationToken!=NULL)
	{
		if(pTree->pNextResult==NULL)
		{
			*ppContinuationToken = NULL;
		}
		else
		{
			SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pContinuationToken)  );
			if(pTree->pNextResult==pTree->pRoot)
			{
				SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pContinuationToken, pTree->pNextResult->pszHidRef)  );
			}
			else
			{
				SG_ERR_CHECK(  _tree__generate_continuation_token(pCtx, pTree, pContinuationToken)  );
			}
			*ppContinuationToken = pContinuationToken;
		}
	}

	*ppResults = pResults;

	if(pCountResults != NULL)
		*pCountResults = countResults;

	return;
fail:
	SG_VARRAY_NULLFREE(pCtx, pResults);
	SG_VARRAY_NULLFREE(pCtx, pContinuationToken);
}

void SG_mergereview(SG_context * pCtx, SG_repo * pRepo, const char * szHidHead, SG_bool singleMergeReview, SG_vhash * pMergeBaselines, SG_uint32 resultLimit, SG_varray ** ppResults, SG_uint32 * pCountResults, SG_varray ** ppContinuationToken)
{
	_tree_t tree;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK(pRepo);
	SG_NONEMPTYCHECK(szHidHead);
	if(resultLimit==0)
		resultLimit = SG_UINT32_MAX;
	SG_NULLARGCHECK(ppResults);

	SG_ERR_CHECK(  _tree__init(pCtx, &tree, szHidHead, pRepo)  );

	SG_ERR_CHECK(  _sg_mergereview(pCtx, &tree, singleMergeReview, SG_TRUE, pMergeBaselines, resultLimit, ppResults, pCountResults, ppContinuationToken)  );

	_tree__uninit(pCtx, &tree);

	return;
fail:
	_tree__uninit(pCtx, &tree);
}

void SG_mergereview__continue(SG_context * pCtx, SG_repo * pRepo, SG_varray * pContinuationToken, SG_bool singleMergeReview, SG_vhash * pMergeBaselines, SG_uint32 resultLimit, SG_varray ** ppResults, SG_uint32 * pCountResults, SG_varray ** ppContinuationToken)
{
	_tree_t tree;
	SG_uint16 firstVariantType;

	SG_zero(tree);

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(pContinuationToken);
	if(resultLimit==0)
		resultLimit = SG_UINT32_MAX;
	SG_NULLARGCHECK(ppResults);

	SG_ERR_CHECK(  SG_varray__typeof(pCtx, pContinuationToken, 0, &firstVariantType)  );
	if(firstVariantType==SG_VARIANT_TYPE_SZ)
	{
		const char * szHead = NULL;
		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pContinuationToken, 0, &szHead)  );
		SG_ERR_CHECK(  _tree__init(pCtx, &tree, szHead, pRepo)  );
	}
	else
	{
		SG_ERR_CHECK(  _tree__init__continuation(pCtx, &tree, pContinuationToken, pRepo)  );
	}


	SG_ERR_CHECK(  _sg_mergereview(pCtx, &tree, singleMergeReview, SG_FALSE, pMergeBaselines, resultLimit, ppResults, pCountResults, ppContinuationToken)  );

	_tree__uninit(pCtx, &tree);

	return;
fail:
	_tree__uninit(pCtx, &tree);
}
