/**
 * @copyright
 * 
 * Copyright (C) 2003, 2010-2013 SourceGear LLC. All rights reserved.
 * This file contains file-diffing-related source from subversion's
 * libsvn_delta, forked at version 0.20.0, which contained the
 * following copyright notice:
 *
 *      Copyright (c) 2000-2003 CollabNet.  All rights reserved.
 *      
 *      This software is licensed as described in the file COPYING, which
 *      you should have received as part of this distribution.  The terms
 *      are also available at http://subversion.tigris.org/license-1.html.
 *      If newer versions of this license are posted there, you may use a
 *      newer version instead, at your option.
 *      
 *      This software consists of voluntary contributions made by many
 *      individuals.  For exact contribution history, see the revision
 *      history and logs, available at http://subversion.tigris.org/.
 * 
 * @endcopyright
 * 
 * @file sg_filediff.c
 * 
 * @details
 * 
 */

#include <sg.h>

//////////////////////////////////////////////////////////////////

struct _sg_filediff_t // semi-public (outside SG_filediff treat as "void")
{
    SG_filediff_t * pNext;
    SG_diff_type type;
    SG_int32 start[3]; // Index into this array with an SG_filediff_doc_ndx enum
    SG_int32 length[3]; // Index into this array with an SG_filediff_doc_ndx enum
};


typedef struct __sg_diff__node _sg_diff__node_t;
struct __sg_diff__node
{
    _sg_diff__node_t * pParent;
    _sg_diff__node_t * pLeft;
    _sg_diff__node_t * pRight;

    void * pToken;
};

void _sg_diff__node__nullfree(_sg_diff__node_t ** ppNode)
{
    if(ppNode==NULL||*ppNode==NULL)
        return;
    _sg_diff__node__nullfree(&(*ppNode)->pLeft);
    _sg_diff__node__nullfree(&(*ppNode)->pRight);
    SG_free__no_ctx(*ppNode);
    *ppNode = NULL;
}


struct __sg_diff__tree
{
    _sg_diff__node_t * pRoot;
};
typedef struct __sg_diff__tree _sg_diff__tree_t;


typedef struct __sg_diff__position _sg_diff__position_t;
struct __sg_diff__position
{
    _sg_diff__position_t * pNext;
    _sg_diff__position_t * pPrev; // SourceGear-added
    _sg_diff__node_t * pNode;
    SG_int32 offset;
};


// "Longest Common Subsequence" -- See note at _sg_diff__lcs() function.
typedef struct __sg_diff__lcs_t _sg_diff__lcs_t;
struct __sg_diff__lcs_t
{
    _sg_diff__lcs_t * pNext;
    _sg_diff__position_t * positions[2];
    SG_int32 length;
};


// Manage memory for _sg_diff__position_t and _sg_diff__lcs_t by keeping
// a linked list of all allocated instances.
typedef struct __sg_diff__mempool _sg_diff__mempool;
struct __sg_diff__mempool
{
    union
    {
        _sg_diff__position_t position;
        _sg_diff__lcs_t lcs;
    } p;

    _sg_diff__mempool * pNext;
};

void _sg_diff__mempool__flush(_sg_diff__mempool * pMempool)
{
    if(pMempool==NULL)
        return;

    while(pMempool->pNext!=NULL)
    {
        _sg_diff__mempool * pNext = pMempool->pNext->pNext;
        SG_free__no_ctx(pMempool->pNext);
        pMempool->pNext = pNext;
    }
}

void _sg_diff__position_t__alloc(SG_context * pCtx, _sg_diff__mempool * pMempool, _sg_diff__position_t ** ppNew)
{
    _sg_diff__mempool * pMempoolNode = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pMempool);
    SG_NULLARGCHECK_RETURN(ppNew);

    SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pMempoolNode)  );
    pMempoolNode->pNext = pMempool->pNext;
    pMempool->pNext = pMempoolNode;

    *ppNew = &pMempoolNode->p.position;
}

void _sg_diff__lcs_t__alloc(SG_context * pCtx, _sg_diff__mempool * pMempool, _sg_diff__lcs_t ** ppNew)
{
    _sg_diff__mempool * pMempoolNode = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pMempool);
    SG_NULLARGCHECK_RETURN(ppNew);

    SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pMempoolNode)  );
    pMempoolNode->pNext = pMempool->pNext;
    pMempool->pNext = pMempoolNode;

    *ppNew = &pMempoolNode->p.lcs;
}


struct __sg_diff__snake_t
{
    SG_int32 y;
    _sg_diff__lcs_t * pLcs;
    _sg_diff__position_t * positions[2];
};
typedef struct __sg_diff__snake_t _sg_diff__snake_t;


void _sg_diff__tree_insert_token(
    SG_context * pCtx,
    _sg_diff__tree_t * pTree,
    const SG_filediff_vtable * pVtable,
    void * pDiffBaton,
    void * pToken,
    _sg_diff__node_t ** ppResult)
{
    _sg_diff__node_t * pParent = NULL;
    _sg_diff__node_t ** ppNode = NULL;
    _sg_diff__node_t * pNewNode = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pTree);
    SG_NULLARGCHECK_RETURN(pVtable);
    SG_NULLARGCHECK_RETURN(ppResult);

    ppNode = &pTree->pRoot;
    while(*ppNode!=NULL)
    {
        SG_int32 rv;
        rv = pVtable->token_compare(pDiffBaton, (*ppNode)->pToken, pToken);
        if (rv == 0)
        {
            // Discard the token
            if (pVtable->token_discard != NULL)
                pVtable->token_discard(pDiffBaton, pToken);

            *ppResult = *ppNode;
            return;
        }
        else if (rv > 0)
        {
            pParent = *ppNode;
            ppNode = &pParent->pLeft;
        }
        else
        {
            pParent = *ppNode;
            ppNode = &pParent->pRight;
        }
    }

    // Create a new node
    {
        SG_ERR_CHECK(  SG_alloc1(pCtx, pNewNode)  );
        pNewNode->pParent = pParent;
        pNewNode->pLeft = NULL;
        pNewNode->pRight = NULL;
        pNewNode->pToken = pToken;

        *ppNode = pNewNode;
        *ppResult = pNewNode;
    }

    return;
fail:
    ;
}

void _sg_diff__get_tokens(
    SG_context * pCtx,
    _sg_diff__mempool * pMempool,
    _sg_diff__tree_t * pTree,
    const SG_filediff_vtable * pVtable,
    void * pDiffBaton,
    SG_filediff_datasource datasource,
    _sg_diff__position_t ** ppPositionList)
{
    _sg_diff__position_t * pStartPosition = NULL;
    _sg_diff__position_t * pPosition = NULL;
    _sg_diff__position_t ** ppPosition;
    _sg_diff__position_t * pPrev = NULL;
    _sg_diff__node_t * pNode;
    void * pToken;
    SG_int32 offset;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(ppPositionList);

    SG_ERR_CHECK(  pVtable->datasource_open(pCtx, pDiffBaton, datasource)  );

    ppPosition = &pStartPosition;
    offset = 0;
    while (1)
    {
        SG_ERR_CHECK(  pVtable->datasource_get_next_token(pCtx, pDiffBaton, datasource, &pToken)  );
        if (pToken == NULL)
            break;

        offset++;
        SG_ERR_CHECK(  _sg_diff__tree_insert_token(pCtx, pTree, pVtable, pDiffBaton, pToken, &pNode)  );

        // Create a new position
        SG_ERR_CHECK(  _sg_diff__position_t__alloc(pCtx, pMempool, &pPosition)  );
        pPosition->pNext = NULL;
        pPosition->pNode = pNode;
        pPosition->offset = offset;
        pPosition->pPrev = pPrev;
        pPrev = pPosition;
  
        *ppPosition = pPosition;
        ppPosition = &pPosition->pNext;
    }

    *ppPosition = pStartPosition;

    if (pStartPosition)
        pStartPosition->pPrev = pPosition;

    SG_ERR_CHECK(  pVtable->datasource_close(pCtx, pDiffBaton, datasource)  );

    *ppPositionList = pPosition;

    // NOTE: for some strange reason we return the postion_list
    // NOTE: pointing to the last position in the datasource
    // NOTE: (so pl->pNext gives the beginning of the datasource).

    return;
fail:
    ;
}

void _sg_diff__snake(SG_context * pCtx, _sg_diff__mempool *pMempool, SG_int32 k, _sg_diff__snake_t *fp, int idx)
{
    _sg_diff__position_t * startPositions[2] = {NULL, NULL};
    _sg_diff__position_t * positions[2] = {NULL, NULL};
    _sg_diff__lcs_t * pLcs = NULL;
    _sg_diff__lcs_t * pPreviousLcs = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(fp);

    if (fp[k - 1].y + 1 > fp[k + 1].y)
    {
        startPositions[0] = fp[k - 1].positions[0];
        startPositions[1] = fp[k - 1].positions[1]->pNext;

        pPreviousLcs = fp[k - 1].pLcs;
    }
    else
    {
        startPositions[0] = fp[k + 1].positions[0]->pNext;
        startPositions[1] = fp[k + 1].positions[1];

        pPreviousLcs = fp[k + 1].pLcs;
    }

    // ### Optimization, skip all positions that don't have matchpoints
    // ### anyway. Beware of the sentinel, don't skip it!

    positions[0] = startPositions[0];
    positions[1] = startPositions[1];

    while (positions[0]->pNode == positions[1]->pNode)
    {
        positions[0] = positions[0]->pNext;
        positions[1] = positions[1]->pNext;
    }

    if (positions[1] != startPositions[1])
    {
        SG_ERR_CHECK(  _sg_diff__lcs_t__alloc(pCtx, pMempool, &pLcs)  );

        pLcs->positions[idx] = startPositions[0];
        pLcs->positions[1-idx] = startPositions[1];
        pLcs->length = positions[1]->offset - startPositions[1]->offset;

        pLcs->pNext = pPreviousLcs;
        fp[k].pLcs = pLcs;
    }
    else
    {
        fp[k].pLcs = pPreviousLcs;
    }

    fp[k].positions[0] = positions[0];
    fp[k].positions[1] = positions[1];

    fp[k].y = positions[1]->offset;

    return;
fail:
    ;
}

_sg_diff__lcs_t * _sg_diff__lcs_reverse(_sg_diff__lcs_t * pLcs)
{
    _sg_diff__lcs_t * pIter = pLcs;
    _sg_diff__lcs_t * pIterPrev = NULL;
    while(pIter!=NULL)
    {
        _sg_diff__lcs_t * pIterNext = pIter->pNext;
        pIter->pNext = pIterPrev;

        pIterPrev = pIter;
        pIter = pIterNext;
    }
    pLcs = pIterPrev;

    return pLcs;
}

//////////////////////////////////////////////////////////////////
// _sg_diff__lcs_juggle() -- juggle the lcs list to eliminate
// minor items.  the lcs algorithm is greedy in that it will
// accept a one line match as a "common" sequence and then create
// another item for the next portion of a large change (a line
// of code containing only a '{' or '}', for example).  if the
// gap between two "common" portions ends with the same text as
// is in the first "common" portion, it is sort of arbitrary
// whether we use the first "common" portion or the one in the
// gap.  but by selecting the one at the end of the gap, we can
// coalesce the item with the following one (and append the gap
// onto the prior node).
SG_bool _sg_diff__lcs_juggle(_sg_diff__lcs_t * pLcsHead)
{
    _sg_diff__lcs_t * pLcs = pLcsHead;
    SG_bool bDidJuggling = SG_FALSE;
    while (pLcs && pLcs->pNext)
    {
        SG_int32 lenGap0;
        SG_int32 lenGap1;
        SG_int32 ndxGap;
        SG_int32 k;

        _sg_diff__lcs_t * pLcsNext = pLcs->pNext;

        _sg_diff__position_t * pGapTail = NULL;
        _sg_diff__position_t * pBegin = NULL;
        _sg_diff__position_t * pEnd = NULL;

        // if the next lcs is length zero, we are on the last one and can't do anything.
        if (pLcsNext->length == 0)
            goto NoShift;

        // compute the gap between this sequence and the next one.
        lenGap0 = (pLcsNext->positions[0]->offset - pLcs->positions[0]->offset - pLcs->length);
        lenGap1 = (pLcsNext->positions[1]->offset - pLcs->positions[1]->offset - pLcs->length);

        // if both gaps are zero we're probably at the end.
        // whether we are or not, we don't have anything to do.
        if ((lenGap0==0) && (lenGap1==0))
            goto NoShift;

        // if both are > 0, this is not a minor item and
        // we shouldn't mess with it.
        if ((lenGap0>0) && (lenGap1>0))
            goto NoShift;

        // compute the index (0 or 1) of the one which has
        // the gap past the end of the common portion.  the
        // other is contiguous with the next lcs.
        ndxGap = (lenGap1 > 0);

        // get the end of the gap and back up the length of common part.
        for (pGapTail=pLcsNext->positions[ndxGap],k=0; (k<pLcs->length); pGapTail=pGapTail->pPrev, k++)
        {
        }

        // see if the common portion (currently prior to
        // the gap) is exactly repeated at the end of the
        // gap.
        for (k=0, pBegin=pLcs->positions[ndxGap], pEnd=pGapTail; k < pLcs->length; k++, pBegin=pBegin->pNext, pEnd=pEnd->pNext)
        {
            if (pBegin->pNode != pEnd->pNode)
                goto NoShift;
        }
        
        // we can shift the common portion to the end of the gap
        // and push the first common portion and the remainder
        // of the gap onto the tail of the previous lcs.
        pLcs->positions[ndxGap] = pGapTail;

        // now, we have 2 contiguous lcs's (pLcs and pLcsNext) that
        // are contiguous on both parts.  so pLcsNext is redundant.
        // we just add the length of pLcsNext to pLcs and remove
        // pLcsNext from the list.
        //
        // but wait, the last thing in the lcs list is a "sentinal",
        // a zero length lcs item that we can't delete.

        if (pLcsNext->length)
        {
            pLcs->length += pLcsNext->length;
            pLcs->pNext = pLcsNext->pNext;
            bDidJuggling = SG_TRUE;
        }
        
        // let's go back and try this again on this lcs and see
        // if it has overlap with the next one.
        continue;

    NoShift:
        pLcs=pLcs->pNext;
    }

    return bDidJuggling;
}

// Calculate the Longest Common Subsequence between two datasources.
// This function is what makes the diff code tick.
// 
// The LCS algorithm implemented here is described by Sun Wu,
// Udi Manber and Gene Meyers in "An O(NP) Sequence Comparison Algorithm"
// 
// The "Juggle" routine is original to the SourceGear version.
// 
// NOTE: Use position lists and tree to compute LCS using graph
// NOTE: algorithm described in the paper.
// NOTE:
// NOTE: This produces a "LCS list".  Where each "LCS" represents
// NOTE: a length and an offset in source-a and source-c where they
// NOTE: are identical.
// NOTE:
// NOTE: "Gaps" (in my terminology) are implicit between LCS's and
// NOTE: represent places where they are different.  A Gap is said
// NOTE: to exist when the offset in the next LCS is greater than
// NOTE: the offset in this LCS plus the length for a particular
// NOTE: source.
// NOTE:
// NOTE: From the LCS list, they compute the "diff list" which
// NOTE: represents the changes (and is basically what we see as the
// NOTE: output of the "diff" command).
void _sg_diff__lcs(
    SG_context * pCtx,
    _sg_diff__mempool * pMempool,
    _sg_diff__position_t * pPositionList1,
    _sg_diff__position_t * pPositionList2,
    _sg_diff__lcs_t ** ppResult)
{
    int idx;
    SG_int32 length[2];
    _sg_diff__snake_t *fp = NULL;
    _sg_diff__snake_t *fp0 = NULL;
    SG_int32 d;
    SG_int32 p = 0;
    _sg_diff__lcs_t * pLcs = NULL;

    _sg_diff__position_t sentinelPositions[2];
    _sg_diff__node_t     sentinelNodes[2];

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(ppResult);

    // Since EOF is always a sync point we tack on an EOF link with sentinel positions
    SG_ERR_CHECK(  _sg_diff__lcs_t__alloc(pCtx, pMempool, &pLcs)  );
    SG_ERR_CHECK(  _sg_diff__position_t__alloc(pCtx, pMempool, &pLcs->positions[0])  );
    pLcs->positions[0]->offset = pPositionList1 ? pPositionList1->offset + 1 : 1;
    SG_ERR_CHECK(  _sg_diff__position_t__alloc(pCtx, pMempool, &pLcs->positions[1])  );
    pLcs->positions[1]->offset = pPositionList2 ? pPositionList2->offset + 1 : 1;
    pLcs->length = 0;
    pLcs->pNext = NULL;

    if (pPositionList1 == NULL || pPositionList2 == NULL)
    {
        *ppResult = pLcs;
        return;
    }

    // Calculate length of both sequences to be compared
    length[0] = pPositionList1->offset - pPositionList1->pNext->offset + 1;
    length[1] = pPositionList2->offset - pPositionList2->pNext->offset + 1;
    idx = length[0] > length[1] ? 1 : 0;

    SG_ERR_CHECK(  SG_allocN(pCtx, (length[0]+length[1]+3), fp)  );
    memset(fp, 0, sizeof(*fp)*(length[0]+length[1]+3));
    fp0 = fp;
    fp += length[idx] + 1;

    sentinelPositions[idx].pNext = pPositionList1->pNext;
    pPositionList1->pNext = &sentinelPositions[idx];
    sentinelPositions[idx].offset = pPositionList1->offset + 1;

    sentinelPositions[1-idx].pNext = pPositionList2->pNext;
    pPositionList2->pNext = &sentinelPositions[1-idx];
    sentinelPositions[1-idx].offset = pPositionList2->offset + 1;

    sentinelPositions[0].pNode = &sentinelNodes[0];
    sentinelPositions[1].pNode = &sentinelNodes[1];

    d = length[1-idx] - length[idx];

    // k = -1 will be the first to be used to get previous position
    // information from, make sure it holds sane data
    fp[-1].positions[0] = sentinelPositions[0].pNext;
    fp[-1].positions[1] = &sentinelPositions[1];

    p = 0;
    do
    {
        SG_int32 k;

        // Forward
        for (k = -p; k < d; k++)
            SG_ERR_CHECK(  _sg_diff__snake(pCtx, pMempool, k, fp, idx)  );

        for (k = d + p; k >= d; k--)
            SG_ERR_CHECK(  _sg_diff__snake(pCtx, pMempool, k, fp, idx)  );

        p++;
    }
    while (fp[d].positions[1] != &sentinelPositions[1]);

    pLcs->pNext = fp[d].pLcs;
    pLcs = _sg_diff__lcs_reverse(pLcs);

    pPositionList1->pNext = sentinelPositions[idx].pNext;
    pPositionList2->pNext = sentinelPositions[1-idx].pNext;

    // run our juggle algorithm on the LCS list to try to
    // eliminate minor annoyances -- this has the effect of
    // joining large chunks that were split by a one line
    // change -- where it is arbitrary whether the one line
    // is handled before or after the second large chunk.
    //
    // we let this run until it reaches (a kind of transitive)
    // closure.  this is required because the concatenation
    // tends to cascade backwards -- that is, the tail of a
    // large change get joined to the prior change and then that
    // gets joined to the prior change (see filediffs/d176_*).
    //
    // TODO figure out how to do the juggle in one pass.
    // it's possible that we could do this in one trip if we
    // had back-pointers in the lcs list or if we tried it
    // before the list is (un)reversed.  but i'll save that
    // for a later experiment.  it generally only runs once,
    // but i've seen it go 4 times for a large block (with
    // several blank or '{' lines) (on d176).  it is linear
    // in the number of lcs'es and since we now have back
    // pointers on the position list, it's probably not worth
    // the effort (or the amount of text in this comment :-).

    while(_sg_diff__lcs_juggle(pLcs))
    {
    }

    SG_NULLFREE(pCtx, fp0);

    *ppResult = pLcs;

    return;
fail:
    SG_NULLFREE(pCtx, fp0);
}

void _sg_diff__diff(SG_context * pCtx, _sg_diff__lcs_t *pLcs, SG_int32 originalStart, SG_int32 modifiedStart, SG_bool want_common, SG_filediff_t ** ppResult)
{
    // Morph a _sg_lcs_t into a _sg_filediff_t.
    SG_filediff_t * pResult = NULL;
    SG_filediff_t ** ppNext = &pResult;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(ppResult);

    while (1)
    {
        if( (originalStart < pLcs->positions[0]->offset) || (modifiedStart < pLcs->positions[1]->offset) )
        {
            SG_ERR_CHECK(  SG_alloc1(pCtx, *ppNext)  );

            (*ppNext)->type = SG_DIFF_TYPE__DIFF_MODIFIED;
            (*ppNext)->start[SG_FILEDIFF_DOC_NDX__ORIGINAL] = originalStart - 1;
            (*ppNext)->length[SG_FILEDIFF_DOC_NDX__ORIGINAL] = pLcs->positions[0]->offset - originalStart;
            (*ppNext)->start[SG_FILEDIFF_DOC_NDX__MODIFIED] = modifiedStart - 1;
            (*ppNext)->length[SG_FILEDIFF_DOC_NDX__MODIFIED] = pLcs->positions[1]->offset - modifiedStart;
            (*ppNext)->start[SG_FILEDIFF_DOC_NDX__LATEST] = 0;
            (*ppNext)->length[SG_FILEDIFF_DOC_NDX__LATEST] = 0;

            ppNext = &(*ppNext)->pNext;
        }

        // Detect the EOF
        if (pLcs->length == 0)
            break;

        originalStart = pLcs->positions[0]->offset;
        modifiedStart = pLcs->positions[1]->offset;

        if (want_common)
        {
            SG_ERR_CHECK(  SG_alloc1(pCtx, *ppNext)  );

            (*ppNext)->type = SG_DIFF_TYPE__COMMON;
            (*ppNext)->start[SG_FILEDIFF_DOC_NDX__ORIGINAL] = originalStart - 1;
            (*ppNext)->length[SG_FILEDIFF_DOC_NDX__ORIGINAL] = pLcs->length;
            (*ppNext)->start[SG_FILEDIFF_DOC_NDX__MODIFIED] = modifiedStart - 1;
            (*ppNext)->length[SG_FILEDIFF_DOC_NDX__MODIFIED] = pLcs->length;
            (*ppNext)->start[SG_FILEDIFF_DOC_NDX__LATEST] = 0;
            (*ppNext)->length[SG_FILEDIFF_DOC_NDX__LATEST] = 0;

            ppNext = &(*ppNext)->pNext;
        }

        originalStart += pLcs->length;
        modifiedStart += pLcs->length;

        pLcs = pLcs->pNext;
    }

    *ppNext = NULL;

    *ppResult = pResult;

    return;
fail:
    ;
}

void _sg_diff3__diff3(
	SG_context * pCtx,
	_sg_diff__lcs_t * pLcs_om, _sg_diff__lcs_t * pLcs_ol,

	_sg_diff__position_t * pos[3], //< Position lists are a linked list of lines in the file.
	                               //  We'll run throught the files, using these pointers to
	                               //  keep track of our current position.

	SG_filediff_t ** ppResult)
{
	SG_filediff_t * pResult = NULL;
	SG_filediff_t ** ppNext = &pResult;

	SG_enumeration ndx;
	const SG_enumeration ORIGINAL = SG_FILEDIFF_DOC_NDX__ORIGINAL; // File #0 is the original file
	const SG_enumeration MODIFIED = SG_FILEDIFF_DOC_NDX__MODIFIED; // File #1, called "modified" is the first modified version.
	const SG_enumeration LATEST   = SG_FILEDIFF_DOC_NDX__LATEST;   // File #2, called "latest" is the second modified version.
	// Note that the "latest" file is at index [1] in pLcs_ol. So to avoid getting it confused with the actual "modified"
	// file, we use the [1] subscript instead of [MODIFIED] when accessing it in pLcs_ol.

	_sg_diff__position_t eof[3];

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppResult);

	// Insert a position in each file to represent the EOF, so that we can actually tell when we've reached it.
	for(ndx=0; ndx<3; ++ndx)
	{
		if(pos[ndx]!=NULL)
		{
			pos[ndx] = pos[ndx]->pNext; // (The lists come in pointing to the last node, so advance it one.)

			eof[ndx].pNode = NULL;
			eof[ndx].offset = pos[ndx]->pPrev->offset+1;
			eof[ndx].pNext = pos[ndx];
			eof[ndx].pPrev = pos[ndx]->pPrev;

			eof[ndx].pNext->pPrev = &eof[ndx];
			eof[ndx].pPrev->pNext = &eof[ndx];
		}
		else
		{
			pos[ndx] = &eof[ndx];

			eof[ndx].pNode = NULL;
			eof[ndx].offset = 1;
			eof[ndx].pNext = &eof[ndx];
			eof[ndx].pPrev = &eof[ndx];
		}
	}

	// There's a special zero-length node at the end of the LCMs that indicates the EOF itself as being part of the LCM.
	// We rely on this node to exist, zero length, even if the EOF is part of a larger run of equal lines at the end of the file.
	while( pLcs_om->length>0 || pLcs_ol->length>0
		// After the EOF is pointed to by the 'pLcs's, we still might need to "catch up" to it by including a change at the end of the file.
		// Let's see:
		|| pos[MODIFIED]->offset < pLcs_om->positions[MODIFIED]->offset
		|| pos[ORIGINAL]->offset < pLcs_om->positions[ORIGINAL]->offset
		|| pos[ORIGINAL]->offset < pLcs_ol->positions[ORIGINAL]->offset
		|| pos[LATEST]->offset   < pLcs_ol->positions[1]->offset
	)
	{
		// We alternate between accounting for the next modification in the file (modified in
		// either version, or perhaps a conflict) followed by the next group of unchanged lines.

		// First the modification.
		{
			SG_bool changes_in_modified = SG_FALSE;
			SG_bool changes_in_latest = SG_FALSE;
			SG_bool same_changes_both_sides = SG_TRUE;

			SG_int32 start[3];
			for(ndx=0; ndx<3; ++ndx)
				start[ndx] = pos[ndx]->offset;

			while( pos[MODIFIED]->offset < pLcs_om->positions[MODIFIED]->offset
				|| pos[ORIGINAL]->offset < pLcs_om->positions[ORIGINAL]->offset
				|| pos[ORIGINAL]->offset < pLcs_ol->positions[ORIGINAL]->offset
				|| pos[LATEST]->offset   < pLcs_ol->positions[1]->offset        )
			{
				if(pos[MODIFIED]->offset >= pLcs_om->positions[MODIFIED]->offset && pos[ORIGINAL]->offset >= pLcs_om->positions[ORIGINAL]->offset)
				{
					while(
						(pos[ORIGINAL]->offset < pLcs_ol->positions[ORIGINAL]->offset) // False when "deleted in 'latest'" no longer true.
						&&
						(pos[ORIGINAL]->offset < pLcs_om->positions[ORIGINAL]->offset+pLcs_om->length) // False when "untouched in 'modified'" no longer true.
					)
					{
						// (lines deleted in "latest", but left untouched in "modified")
						changes_in_latest = SG_TRUE;
						same_changes_both_sides = SG_FALSE;
						pos[ORIGINAL] = pos[ORIGINAL]->pNext;
						pos[MODIFIED] = pos[MODIFIED]->pNext;

					}
					if(pos[ORIGINAL]->offset==pLcs_om->positions[ORIGINAL]->offset+pLcs_om->length && pLcs_om->length>0)
						pLcs_om = pLcs_om->pNext;
				}
				else if(pos[LATEST]->offset >= pLcs_ol->positions[1]->offset && pos[ORIGINAL]->offset >= pLcs_ol->positions[ORIGINAL]->offset)
				{
					while(
						(pos[ORIGINAL]->offset < pLcs_om->positions[ORIGINAL]->offset) // False when "deleted in 'latest'" no longer true.
						&&
						(pos[ORIGINAL]->offset < pLcs_ol->positions[ORIGINAL]->offset+pLcs_ol->length) // False when "untouched in 'modified'" no longer true.
					)
					{
						// (lines deleted in "modified", but left untouched in "latest")
						changes_in_modified = SG_TRUE;
						same_changes_both_sides = SG_FALSE;
						pos[ORIGINAL] = pos[ORIGINAL]->pNext;
						pos[LATEST] = pos[LATEST]->pNext;
					}
					if(pos[ORIGINAL]->offset==pLcs_ol->positions[ORIGINAL]->offset+pLcs_ol->length && pLcs_ol->length>0)
						pLcs_ol = pLcs_ol->pNext;
				}

				while( pos[MODIFIED]->offset < pLcs_om->positions[MODIFIED]->offset
					&& pos[LATEST]->offset < pLcs_ol->positions[1]->offset)
				{
					// (lines added in both "modified" and "latest" (may or may not be equal, we'll see))
					changes_in_modified = SG_TRUE;
					changes_in_latest = SG_TRUE;
					same_changes_both_sides &= (pos[MODIFIED]->pNode == pos[LATEST]->pNode);
					pos[MODIFIED] = pos[MODIFIED]->pNext;
					pos[LATEST] = pos[LATEST]->pNext;
				}
				while(pos[MODIFIED]->offset < pLcs_om->positions[1]->offset)
				{
					// (lines added in "modified" (no potential match in "latest"))
					changes_in_modified = SG_TRUE;
					same_changes_both_sides = SG_FALSE;
					pos[MODIFIED] = pos[MODIFIED]->pNext;
				}
				while(pos[LATEST]->offset < pLcs_ol->positions[1]->offset)
				{
					// (lines added in "latest" (no potential match in "modified"))
					changes_in_latest = SG_TRUE;
					same_changes_both_sides = SG_FALSE;
					pos[LATEST] = pos[LATEST]->pNext;
				}

				while( pos[ORIGINAL]->offset < pLcs_om->positions[ORIGINAL]->offset
					&& pos[ORIGINAL]->offset < pLcs_ol->positions[ORIGINAL]->offset)
				{
					// (lines deleted in both "modified" and "latest")
					changes_in_modified = SG_TRUE;
					changes_in_latest = SG_TRUE;
					pos[ORIGINAL] = pos[ORIGINAL]->pNext;
				}
			}

			// We should always enter this 'if' unless we were at the beginning of the file
			// and there were no modifications to the first line.
			SG_ASSERT(changes_in_modified || changes_in_latest || pResult==NULL);
			if(changes_in_modified || changes_in_latest)
			{
				SG_ERR_CHECK(  SG_alloc1(pCtx, *ppNext)  );

				if(same_changes_both_sides)
					(*ppNext)->type = SG_DIFF_TYPE__DIFF_COMMON;
				else if(!changes_in_latest)
					(*ppNext)->type = SG_DIFF_TYPE__DIFF_MODIFIED;
				else if(!changes_in_modified)
					(*ppNext)->type = SG_DIFF_TYPE__DIFF_LATEST;
				else
					(*ppNext)->type = SG_DIFF_TYPE__CONFLICT;

				for(ndx=0; ndx<3; ++ndx)
				{
					(*ppNext)->start[ndx] = start[ndx];
					(*ppNext)->length[ndx] = pos[ndx]->offset - start[ndx];
				}

				ppNext = &(*ppNext)->pNext;
			}
		}

		// Done with the modified lines.
		// We are now at the begining of a group of matching/unchanged/"common" lines (or at the EOF, which is also considered a match).

		SG_ASSERT( (pLcs_om->length==0) == (pLcs_ol->length==0) ); // (Note: a regular subsequence should never match the special EOF subsequence)

		// Add the common lines to our merge result.
		if(pLcs_om->length>0) // (but only if we're not at the EOF)
		{
			SG_int32 length = SG_MIN(
				pLcs_om->positions[ORIGINAL]->offset + pLcs_om->length - pos[ORIGINAL]->offset,
				pLcs_ol->positions[ORIGINAL]->offset + pLcs_ol->length - pos[ORIGINAL]->offset);

			SG_ERR_CHECK(  SG_alloc1(pCtx, *ppNext)  );
			(*ppNext)->type = SG_DIFF_TYPE__COMMON;
			for(ndx=0; ndx<3; ++ndx)
			{
				(*ppNext)->start[ndx] = pos[ndx]->offset;
				(*ppNext)->length[ndx] = length;
			}

			ppNext = &(*ppNext)->pNext;

			// And don't forget to advance all the pointers...
			if( ( pos[ORIGINAL]->offset - pLcs_om->positions[ORIGINAL]->offset ) + length == pLcs_om->length )
				pLcs_om = pLcs_om->pNext;

			if( ( pos[ORIGINAL]->offset - pLcs_ol->positions[ORIGINAL]->offset ) + length == pLcs_ol->length )
				pLcs_ol = pLcs_ol->pNext;

			while(length>0)
			{
				for(ndx=0; ndx<3; ++ndx)
					pos[ndx] = pos[ndx]->pNext;
				--length;
			}
		}
	}

	*ppResult = pResult;

	return;
fail:
	SG_FILEDIFF_NULLFREE(pCtx, pResult);
}


//////////////////////////////////////////////////////////////////
// The Main Events
//////////////////////////////////////////////////////////////////

void SG_filediff(SG_context * pCtx, const SG_filediff_vtable * pVtable, void * pDiffBaton, SG_filediff_t ** ppDiff)
{
    _sg_diff__tree_t tree;
    _sg_diff__mempool mempool;
    _sg_diff__position_t * positionLists[2] = {NULL, NULL};
    _sg_diff__lcs_t * pLcs = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pVtable);
    SG_NULLARGCHECK_RETURN(ppDiff);

    tree.pRoot = NULL;
    SG_zero(mempool);

    // Insert the data into the tree
    SG_ERR_CHECK(  _sg_diff__get_tokens(pCtx, &mempool, &tree, pVtable, pDiffBaton, SG_FILEDIFF_DATASOURCE__ORIGINAL, &positionLists[0])  );
    SG_ERR_CHECK(  _sg_diff__get_tokens(pCtx, &mempool, &tree, pVtable, pDiffBaton, SG_FILEDIFF_DATASOURCE__MODIFIED, &positionLists[1])  );

    // The cool part is that we don't need the tokens anymore.
    // Allow the app to clean them up if it wants to.
    if(pVtable->token_discard_all!=NULL)
        pVtable->token_discard_all(pDiffBaton);

    _sg_diff__node__nullfree(&tree.pRoot);

    // Get the lcs
    SG_ERR_CHECK(  _sg_diff__lcs(pCtx, &mempool, positionLists[0], positionLists[1], &pLcs)  );

    // Produce the diff
    SG_ERR_CHECK(  _sg_diff__diff(pCtx, pLcs, 1, 1, SG_TRUE, ppDiff)  );

    _sg_diff__mempool__flush(&mempool);

    return;
fail:
    _sg_diff__node__nullfree(&tree.pRoot);
    _sg_diff__mempool__flush(&mempool);
}

void SG_filediff3(SG_context * pCtx, const SG_filediff_vtable * pVtable, void * pDiffBaton, SG_filediff_t ** ppDiff)
{
    _sg_diff__tree_t tree;
    _sg_diff__mempool mempool;
    _sg_diff__position_t * positionLists[3] = {NULL, NULL, NULL};
    _sg_diff__lcs_t * pLcs_om = NULL;
    _sg_diff__lcs_t * pLcs_ol = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pVtable);
    SG_NULLARGCHECK_RETURN(ppDiff);

    tree.pRoot = NULL;
    SG_zero(mempool);

    // Insert the data into the tree
    SG_ERR_CHECK(  _sg_diff__get_tokens(pCtx, &mempool, &tree, pVtable, pDiffBaton, SG_FILEDIFF_DATASOURCE__ORIGINAL, &positionLists[0])  );
    SG_ERR_CHECK(  _sg_diff__get_tokens(pCtx, &mempool, &tree, pVtable, pDiffBaton, SG_FILEDIFF_DATASOURCE__MODIFIED, &positionLists[1])  );
    SG_ERR_CHECK(  _sg_diff__get_tokens(pCtx, &mempool, &tree, pVtable, pDiffBaton, SG_FILEDIFF_DATASOURCE__LATEST, &positionLists[2])  );

    // Get rid of the tokens, we don't need them to calc the diff
    if(pVtable->token_discard_all!=NULL)
        pVtable->token_discard_all(pDiffBaton);

    _sg_diff__node__nullfree(&tree.pRoot);

    // Get the lcs for original-modified and original-latest
    SG_ERR_CHECK(  _sg_diff__lcs(pCtx, &mempool, positionLists[0], positionLists[1], &pLcs_om)  );
    SG_ERR_CHECK(  _sg_diff__lcs(pCtx, &mempool, positionLists[0], positionLists[2], &pLcs_ol)  );

    // Produce a merged diff
    SG_ERR_CHECK(  _sg_diff3__diff3(pCtx, pLcs_om, pLcs_ol, positionLists, ppDiff)  );

    _sg_diff__mempool__flush(&mempool);

    return;
fail:
    _sg_diff__node__nullfree(&tree.pRoot);
    _sg_diff__mempool__flush(&mempool);
}

void SG_filediff__free(SG_context * pCtx, SG_filediff_t * pDiff)
{
    SG_ASSERT(pCtx!=NULL);
    while(pDiff!=NULL)
    {
        SG_filediff_t * pNext = pDiff->pNext;
        SG_NULLFREE(pCtx, pDiff);
        pDiff = pNext;
    }
}


//////////////////////////////////////////////////////////////////
// Utility Functions
//////////////////////////////////////////////////////////////////

SG_bool SG_filediff__contains_conflicts(SG_filediff_t * pDiff)
{
    while(pDiff!=NULL)
    {
        if(pDiff->type==SG_DIFF_TYPE__CONFLICT)
            return SG_TRUE;
        pDiff = pDiff->pNext;
    }
    return SG_FALSE;
}

SG_bool SG_filediff__contains_diffs(SG_filediff_t * pDiff)
{
    while(pDiff!=NULL)
    {
        if(pDiff->type!=SG_DIFF_TYPE__COMMON)
            return SG_TRUE;
        pDiff = pDiff->pNext;
    }
    return SG_FALSE;
}


//////////////////////////////////////////////////////////////////
// Displaying Diffs
//////////////////////////////////////////////////////////////////

void SG_filediff__output(SG_context * pCtx, const SG_filediff_output_functions * pVtable, void * pOutputBaton, SG_filediff_t * pDiff)
{
    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pVtable);

    while(pDiff!=NULL)
    {
        SG_filediff_output_function output_function = NULL;
        switch(pDiff->type)
        {
        case SG_DIFF_TYPE__COMMON:        output_function = pVtable->output_common; break;
        case SG_DIFF_TYPE__DIFF_COMMON:   output_function = pVtable->output_diff_common; break;
        case SG_DIFF_TYPE__DIFF_MODIFIED: output_function = pVtable->output_diff_modified; break;
        case SG_DIFF_TYPE__DIFF_LATEST:   output_function = pVtable->output_diff_latest; break;
        case SG_DIFF_TYPE__CONFLICT:
            if (pVtable->output_conflict != NULL)
            {
                SG_ERR_CHECK(  pVtable->output_conflict(
                    pCtx, pOutputBaton,
                    pDiff->start[SG_FILEDIFF_DOC_NDX__ORIGINAL], pDiff->length[SG_FILEDIFF_DOC_NDX__ORIGINAL],
                    pDiff->start[SG_FILEDIFF_DOC_NDX__MODIFIED], pDiff->length[SG_FILEDIFF_DOC_NDX__MODIFIED],
                    pDiff->start[SG_FILEDIFF_DOC_NDX__LATEST], pDiff->length[SG_FILEDIFF_DOC_NDX__LATEST])  );
            }
            break;
        default:
            break;
        }

        if(output_function!=NULL)
        {
            SG_ERR_CHECK(  output_function(
                pCtx,
                pOutputBaton,
                pDiff->start[SG_FILEDIFF_DOC_NDX__ORIGINAL], pDiff->length[SG_FILEDIFF_DOC_NDX__ORIGINAL],
                pDiff->start[SG_FILEDIFF_DOC_NDX__MODIFIED], pDiff->length[SG_FILEDIFF_DOC_NDX__MODIFIED],
                pDiff->start[SG_FILEDIFF_DOC_NDX__LATEST], pDiff->length[SG_FILEDIFF_DOC_NDX__LATEST])  );
        }

        pDiff = pDiff->pNext;
    }

    return;
fail:
    ;
}

void SG_filediff__get_next(SG_context * pCtx, SG_filediff_t * pDiff, SG_filediff_t ** ppNextDiff)
{
	SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pDiff);
	SG_NULLARGCHECK_RETURN(ppNextDiff);

	*ppNextDiff = pDiff->pNext;
}

void SG_filediff__get_details(SG_context * pCtx, 
	SG_filediff_t * pDiff, 
	SG_diff_type * pDiffType,
	SG_int32 * pStart1,
	SG_int32 * pStart2,
	SG_int32 * pStart3,
	SG_int32 * pLen1,
	SG_int32 * pLen2,
	SG_int32 * pLen3)
{
	SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pDiff);
	
	if (pDiffType)
		*pDiffType = pDiff->type;
	if (pStart1)
		*pStart1 = pDiff->start[0];
	if (pStart2)
		*pStart2 = pDiff->start[1];
	if (pStart3)
		*pStart3 = pDiff->start[2];

	if (pLen1)
		*pLen1 = pDiff->length[0];
	if (pLen2)
		*pLen2 = pDiff->length[1];
	if (pLen3)
		*pLen3 = pDiff->length[2];
}