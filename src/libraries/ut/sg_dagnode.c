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
 * @file sg_dagnode.c
 *
 * @details Routines for manipulating DAGNODES.
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

/**
 * SG_dagnode is a structure containing information on a single
 * DAGNODE.  A DAGNODE contains the essence of the child/parent
 * relationships in a single CHANGESET.  That is, a DAGNODE is a
 * collection of DAG EDGES (from child to parent).
 *
 * We have 2 notions of a DAGNODE:
 * [1] The raw relationships (EDGES) that are stored in the
 *     actual DAG by the REPO.
 * [2] The memory object that contains the edges and additional
 *     useful information to allow the graph to be efficiently
 *     walked.
 *
 * NOTE: the dagnode's HID is now a buffer inside the dagnode
 *       rather than another allocated buffer.
 * NOTE: the generation is now a SG_int32 rather than a SG_int64.
 *
 * TODO Re-order these fields in the structure so that the pointers
 * TODO are grouped together at the top and the buffer is at the bottom.
 * TODO This will make them a little smaller in memory.
 */
struct SG_dagnode
{
    char*                   m_psz_hid;
    char**                  m_parents;
	SG_int32				m_generation;	// the depth of the DAGNODE in the DAG (must be signed).
	SG_uint32				m_instanceRevNo;
    SG_uint8                m_count_parents;
	SG_uint8				m_bIsFrozen;
};

/*

   Note that the struct above does not have a dagnum in it.
   Here is a snippet from an email from 4 March 2010 which
   discusses the reasons why:


   I think this is by design.

   Having the dagnum in every dagnode just seems
   redundant.  Every dagnode in a dag (or in a frag
   thereof) has the same dagnum.

   Most of the purpose of a dagnode (as opposed to
   a changeset) is to be as small as possible.

   In the C representation of a dagnode, it would
   take 4 more bytes to store it, and it doesn't
   really help anything.  We need dagnodes in memory
   to be as small as possible.

   dagnodes get serialized during push/pull, so
   having the dagnum in every dagnode's json would
   be really wasteful.

   So, I would argue against putting the dagnum in
   any C representation of a dagnode.

   However, you note that sg_repo__fetch_dagnode
   doesn't take a dagnum.  It is reasonable to ask
   if it should.

   If it did, then it could fail when it gets a
   dagnode id which is not in the given dagnum.
   Since it does not have this parameter, certain
   operations will fail later than they could
   if the parameter were present.

   In fact, fetch_dagnode used to have this parameter.
   I took it out for two reasons:

   1.  Using the parameter required the WHERE clause
   to have an AND portion, and this is a performance
   hit on every single dagnode retrieval, even though
   it only helps on the incredibly rare error case.

   2.  The dagnum isn't always known by the caller.
   Some operations want to fetch a dagnode by its
   id without concern over what dag it is in.  For
   example, drawing a picture of a dag is valid for
   any dagnum, so all I need is an id.  I don't really
   care if the given dagnode is in any particular dag.

   */


#define MY_INVALID_GENERATION(g)	((g) <= 0)

//////////////////////////////////////////////////////////////////

void SG_dagnode__alloc(SG_context* pCtx, SG_dagnode ** ppNew, const char* pszidHidChangeset, SG_int32 gen, SG_uint32 revno)
{
	SG_dagnode * pNew = NULL;

	SG_NULLARGCHECK(ppNew);
	SG_NULLARGCHECK(pszidHidChangeset);
	SG_ARGCHECK(!MY_INVALID_GENERATION(gen), gen);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pNew)  );

	SG_ERR_CHECK(  SG_strdup(pCtx,pszidHidChangeset,&pNew->m_psz_hid)  );

	pNew->m_bIsFrozen = SG_FALSE;
	pNew->m_generation = gen;
	pNew->m_instanceRevNo = revno;

	*ppNew = pNew;
	return;

fail:
	SG_DAGNODE_NULLFREE(pCtx, pNew);
}

void SG_dagnode__free(SG_context * pCtx, SG_dagnode * pdn)
{
    SG_uint32 i = 0;
	if (!pdn)
		return;

    SG_NULLFREE(pCtx, pdn->m_psz_hid);
    for (i=0; i<pdn->m_count_parents; i++)
    {
        SG_NULLFREE(pCtx, pdn->m_parents[i]);
    }
    SG_NULLFREE(pCtx, pdn->m_parents);

	SG_NULLFREE(pCtx, pdn);
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__get_id(SG_context* pCtx, const SG_dagnode * pdn, char** ppszidHidChangeset)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(ppszidHidChangeset);

	SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, pdn->m_psz_hid, ppszidHidChangeset)  );
}

void SG_dagnode__get_id_ref(SG_context* pCtx, const SG_dagnode * pdn, const char** ppszidHidChangeset)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(ppszidHidChangeset);

	*ppszidHidChangeset = pdn->m_psz_hid;
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__get_revno(SG_context* pCtx, const SG_dagnode * pdn, SG_uint32* piRevno)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(piRevno);

	*piRevno = pdn->m_instanceRevNo;
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__set_parents__rbtree(SG_context* pCtx, SG_dagnode * pdn, SG_rbtree* prb_parents)
{
	SG_rbtree_iterator* pit = NULL;
	SG_bool b = SG_FALSE;
    SG_uint32 count_parents = 0;
    const char* psz = NULL;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_parents, &count_parents)  );
    pdn->m_count_parents = (SG_uint8) count_parents;
	SG_ERR_CHECK(  SG_allocN(pCtx,pdn->m_count_parents,pdn->m_parents)  );

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_parents, &b, &psz, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_strdup(pCtx, psz, &(pdn->m_parents[i]))  );
        i++;
        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz, NULL)  );
    }

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

void SG_dagnode__set_parents__2(SG_context* pCtx, SG_dagnode * pdn, const char* psz1, const char* psz2)
{
    int cmp = 0;

	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(psz1);
	SG_NULLARGCHECK_RETURN(psz2);

	if (pdn->m_bIsFrozen)
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_WHILE_FROZEN);


    pdn->m_count_parents = 2;
	SG_ERR_CHECK(  SG_allocN(pCtx,pdn->m_count_parents,pdn->m_parents)  );

    cmp = strcmp(psz1, psz2);
    if (cmp < 0)
    {
        SG_ERR_CHECK(  SG_strdup(pCtx, psz1, &(pdn->m_parents[0]))  );
        SG_ERR_CHECK(  SG_strdup(pCtx, psz2, &(pdn->m_parents[1]))  );
    }
    else
    {
        SG_ERR_CHECK(  SG_strdup(pCtx, psz2, &(pdn->m_parents[0]))  );
        SG_ERR_CHECK(  SG_strdup(pCtx, psz1, &(pdn->m_parents[1]))  );
    }

fail:
    // TODO
    ;
}

void SG_dagnode__set_parent(SG_context* pCtx, SG_dagnode * pdn, const char* psz)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(psz);

	if (pdn->m_bIsFrozen)
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_WHILE_FROZEN);

    pdn->m_count_parents = 1;
	SG_ERR_CHECK(  SG_allocN(pCtx,pdn->m_count_parents,pdn->m_parents)  );
    SG_ERR_CHECK(  SG_strdup(pCtx, psz, &(pdn->m_parents[0]))  );

fail:
    // TODO
    ;
}

void SG_dagnode__set_parents__varray(SG_context* pCtx, SG_dagnode * pdn, SG_varray* pva_parents)
{
    SG_uint32 count_parents = 0;
    SG_uint32 i = 0;
    SG_rbtree* prb = NULL;

	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(pva_parents);

	if (pdn->m_bIsFrozen)
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_WHILE_FROZEN);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count_parents)  );
    
    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &prb)  );
    for (i=0; i<count_parents; i++)
    {
        const char* psz = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz)  );
        SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psz)  );
    }

    SG_ERR_CHECK(  SG_dagnode__set_parents__rbtree(pCtx, pdn, prb)  );

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
}

void SG_dagnode__count_parents(SG_context* pCtx, const SG_dagnode * pdn, SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(pCount);

    *pCount = pdn->m_count_parents;
}

void SG_dagnode__get_parents__ref(
	SG_context* pCtx,
	const SG_dagnode * pdn,
	SG_uint32* piCount,
	const char*** pppResult
	)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(piCount);
	SG_NULLARGCHECK_RETURN(pppResult);

    *piCount = pdn->m_count_parents;
    *pppResult = (const char**)pdn->m_parents;
}

void SG_dagnode__is_parent(SG_context* pCtx, const SG_dagnode * pdn, const char* pszidHidTest, SG_bool * pbIsParent)
{
    SG_uint32 i=0;

	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(pszidHidTest);
	SG_NULLARGCHECK_RETURN(pbIsParent);

	*pbIsParent = SG_FALSE;

	if (!pdn->m_count_parents)			// no parents, so no.
		return;

    for (i=0; i<pdn->m_count_parents; i++)
    {
	// TODO m_parents is sorted
        if (0 == strcmp(pszidHidTest, pdn->m_parents[i]))
        {
            *pbIsParent = SG_TRUE;
            return;
        }
    }

    *pbIsParent = SG_FALSE;
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__freeze(SG_context* pCtx, SG_dagnode * pdn)
{
	SG_NULLARGCHECK_RETURN(pdn);

	SG_ARGCHECK_RETURN(!MY_INVALID_GENERATION(pdn->m_generation), pdn->m_generation);	// must set generation before freezing

	pdn->m_bIsFrozen = SG_TRUE;
}

void SG_dagnode__is_frozen(SG_context* pCtx, const SG_dagnode * pdn, SG_bool * pbFrozen)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(pbFrozen);

	*pbFrozen = pdn->m_bIsFrozen;
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__equal(SG_context* pCtx, const SG_dagnode * pdn1, const SG_dagnode * pdn2, SG_bool * pbEqual)
{
    SG_uint32 i = 0;
	SG_bool bEqual;

	SG_NULLARGCHECK_RETURN(pdn1);
	SG_NULLARGCHECK_RETURN(pdn2);
	SG_NULLARGCHECK_RETURN(pbEqual);

	if (pdn1 == pdn2)
	{
		*pbEqual = SG_TRUE;
		return;
	}

	bEqual = (0 == strcmp(pdn1->m_psz_hid,pdn2->m_psz_hid));
	if (!bEqual)
	{
		*pbEqual = SG_FALSE;
		return;
	}

	if (!pdn1->m_bIsFrozen  ||  !pdn2->m_bIsFrozen)
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_UNLESS_FROZEN);

	if (pdn1->m_generation != pdn2->m_generation)
	{
		*pbEqual = SG_FALSE;
		return;
	}

	if ((pdn1->m_count_parents==0) && (pdn2->m_count_parents==0))
	{
		*pbEqual = SG_TRUE;
		return;
	}
	if (pdn1->m_count_parents != pdn2->m_count_parents)
	{
		*pbEqual = SG_FALSE;
		return;
	}

    for (i=0; i<pdn1->m_count_parents; i++)
    {
        if (0 != strcmp(pdn1->m_parents[i], pdn2->m_parents[i]))
        {
            *pbEqual = SG_FALSE;
            return;
        }
    }

	*pbEqual = SG_TRUE;
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__get_generation(SG_context* pCtx, const SG_dagnode * pdn, SG_int32 * pGen)
{
	SG_NULLARGCHECK_RETURN(pdn);
	SG_NULLARGCHECK_RETURN(pGen);

	*pGen = pdn->m_generation;
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_dagnode_debug__dump(SG_context* pCtx,
							const SG_dagnode * pdn,
							SG_uint32 nrDigits,
							SG_uint32 indent,
							SG_string * pStringOutput)
{
	char buf[4000];

	// we can abbreviate the full IDs.

	nrDigits = SG_MIN(nrDigits, SG_HID_MAX_BUFFER_LENGTH);
	nrDigits = SG_MAX(nrDigits, 4);

	// create:
	//     Dagnode[addr]: <child_id> <gen> [<parent_id>...]

	// Ian said: It seems a little strange to ignore all these errors, but return codes were all
	//           ignored before we did error conversion, so this is functionally equivalent.
	//
	// Jeff said: Yeah, this is just a debug dump routine, so I was ignoring errors.  Most of
	//            the code deals with formatting a line of text and appending it to the output
	//            string.  These shouldn't have any problems.  It is possible for the rbtree
	//            to give us problems (such as a bad vector of parents), these will cause us to
	//            crash and that is OK in a debug routine.  Also, our caller probably (should
	//            have) called us in an _IGNORE() block.

	SG_ERR_IGNORE(  SG_sprintf(pCtx,buf,SG_NrElements(buf),"%*cDagnode[%p]: ", indent,' ', pdn)  );
	SG_ERR_IGNORE(  SG_string__append__sz(pCtx, pStringOutput,buf)  );

	if (pdn->m_bIsFrozen)
		SG_ERR_IGNORE(  SG_string__append__buf_len(pCtx, pStringOutput,(SG_byte *)(pdn->m_psz_hid),nrDigits)  );
	else
		SG_ERR_IGNORE(  SG_string__append__sz(pCtx, pStringOutput,"<unk>")  );

	SG_ERR_IGNORE(  SG_sprintf(pCtx,buf,SG_NrElements(buf)," gen=%d",pdn->m_generation)  );
	SG_ERR_IGNORE(  SG_string__append__sz(pCtx, pStringOutput,buf)  );

	if (pdn->m_parents)
	{
		SG_uint32 k, nrParents;
		const char ** paHidParents = NULL;

		SG_ERR_IGNORE(  SG_dagnode__get_parents__ref(pCtx,pdn,&nrParents,&paHidParents)  );
		for (k=0; k<nrParents; k++)
		{
			SG_ERR_IGNORE(  SG_string__append__sz(pCtx, pStringOutput," ")  );
			SG_ERR_IGNORE(  SG_string__append__buf_len(pCtx, pStringOutput,(SG_byte*)(paHidParents[k]),nrDigits)  );
		}
	}

	SG_ERR_IGNORE(  SG_string__append__sz(pCtx, pStringOutput,"\n")  );
}
#endif//DEBUG

//////////////////////////////////////////////////////////////////
// We serialize a dagnode to a vhash.  It has the following form:
//
// vhash "Dagnode" {
//   KEY_VERSION : string = 1
//   KEY_HID     : string HID
//   KEY_GEN     : integer GENERATION
//   KEY_PARENTS : (optional) varray [
//     string HID
//     string HID
//     ...
//   ]
// }

#define KEY_VERSION			"ver"
#define KEY_HID				"hid"
#define KEY_REVNO			"revno"
#define KEY_GEN				"gen"
#define KEY_PARENTS			"parents"
#define KEY_WHO 			"who"
#define KEY_WHEN 			"when"

#define GUESS_NR_KEYS		4

#define VALUE_VERSION		"1"

//////////////////////////////////////////////////////////////////

void SG_dagnode__to_vhash__shared(SG_context* pCtx,
								  const SG_dagnode * pDagnode,
								  SG_vhash * pvhShared,
								  SG_vhash ** ppvhNew)
{
	SG_vhash * pvh = NULL;

	SG_NULLARGCHECK_RETURN(pDagnode);
	SG_NULLARGCHECK_RETURN(ppvhNew);

	if (!pDagnode->m_bIsFrozen)
		SG_ERR_THROW_RETURN(SG_ERR_INVALID_UNLESS_FROZEN);

	SG_ERR_CHECK(  SG_VHASH__ALLOC__SHARED(pCtx,&pvh,GUESS_NR_KEYS,pvhShared)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx,pvh,KEY_VERSION,VALUE_VERSION)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx,pvh,KEY_HID,pDagnode->m_psz_hid)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx,pvh,KEY_REVNO,pDagnode->m_instanceRevNo)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx,pvh,KEY_GEN,(SG_int64)pDagnode->m_generation)  );

	if (pDagnode->m_count_parents)
	{
        SG_varray * pvaParents = NULL;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx,pvh,KEY_PARENTS,&pvaParents)  );
        for (i=0; i<pDagnode->m_count_parents; i++)
        {
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvaParents, pDagnode->m_parents[i])  );
        }
	}

	*ppvhNew = pvh;
	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
}

//////////////////////////////////////////////////////////////////

void SG_dagnode__alloc__from_vhash(SG_context* pCtx,
								   SG_dagnode ** ppNew,
								   const SG_vhash * pvhDagnode)
{
	const char * szVersion;
	const char * szHid;
	SG_uint32 revno;
	SG_int64 gen64;
	SG_bool bHasParents;
	SG_dagnode * pdn = NULL;
	SG_bool bFound = SG_FALSE;

	SG_NULLARGCHECK_RETURN(ppNew);
	SG_NULLARGCHECK_RETURN(pvhDagnode);

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx,pvhDagnode,KEY_VERSION,&szVersion)  );
	if (strcmp(szVersion,"1") == 0)
	{
		// handle dagnode that was serialized by software compiled with VALUE_VERSION == 1.
		
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx,pvhDagnode,KEY_HID,&szHid)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx,pvhDagnode,KEY_GEN,&gen64)  );
		
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhDagnode, KEY_REVNO, &bFound)  );
		if (bFound)
			SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvhDagnode,KEY_REVNO,&revno)  );
		else
			revno = 0;

		SG_ERR_CHECK(  SG_dagnode__alloc(pCtx,&pdn, szHid, (SG_int32)gen64, revno)  );

		SG_ERR_CHECK(  SG_vhash__has(pCtx,pvhDagnode,KEY_PARENTS,&bHasParents)  );
		if (bHasParents)
		{
            SG_uint32 count_parents = 0;
            SG_uint32 i = 0;
            SG_varray * pva_parents = NULL;

			SG_ERR_CHECK(  SG_vhash__get__varray(pCtx,pvhDagnode,KEY_PARENTS,&pva_parents)  );
			SG_ERR_CHECK(  SG_varray__count(pCtx,pva_parents,&count_parents)  );
            pdn->m_count_parents = (SG_uint8) count_parents;
            SG_ERR_CHECK(  SG_allocN(pCtx,pdn->m_count_parents,pdn->m_parents)  );
            for (i=0; i<pdn->m_count_parents; i++)
            {
                const char* psz = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz)  );
                SG_ERR_CHECK(  SG_strdup(pCtx, psz, &(pdn->m_parents[i]))  );
            }
		}

		SG_ERR_CHECK(  SG_dagnode__freeze(pCtx, pdn)  );

		*ppNew = pdn;
		return;
	}
	else
	{
		SG_ERR_THROW(  SG_ERR_DAGNODE_DESERIALIZATION_VERSION  );
	}

fail:
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}


