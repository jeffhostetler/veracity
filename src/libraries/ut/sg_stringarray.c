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

struct _sg_stringarray
{
	SG_uint32 count;
	SG_uint32 space;
	SG_strpool* pStrPool;
	const char** aStrings;
};


void SG_stringarray__alloc(
	SG_context* pCtx,
	SG_stringarray** ppThis,
	SG_uint32 space
	)
{
	SG_stringarray* pThis = NULL;

    if(space==0)
        space = 1;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pThis)  );

	pThis->space = space;

	SG_ERR_CHECK(  SG_alloc(pCtx, pThis->space, sizeof(const char *), &pThis->aStrings)  );
	SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pThis->pStrPool, pThis->space * 64)  );

	*ppThis = pThis;
	return;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pThis);
}

void SG_stringarray__free(SG_context * pCtx, SG_stringarray* pThis)
{
	if (!pThis)
	{
		return;
	}

	SG_NULLFREE(pCtx, pThis->aStrings);
	SG_STRPOOL_NULLFREE(pCtx, pThis->pStrPool);
	SG_NULLFREE(pCtx, pThis);
}

void SG_stringarray__add(
	SG_context* pCtx,
	SG_stringarray* psa,
	const char* psz
	)
{
	const char* pCopy = NULL;
	const char** new_aStrings;

	SG_NULLARGCHECK_RETURN( psa );
	SG_NULLARGCHECK_RETURN( psz );

	if ((1 + psa->count) > psa->space)
	{
        SG_uint32 new_space = psa->space * 2;
		SG_ERR_CHECK(  SG_alloc(pCtx, new_space, sizeof(const char *), &new_aStrings)  );
        memcpy((char **)new_aStrings, psa->aStrings, psa->count * sizeof(const char*));
        SG_NULLFREE(pCtx, psa->aStrings);
        psa->aStrings = new_aStrings;
        psa->space = new_space;
	}

	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, psa->pStrPool, psz, &pCopy)  );

	psa->aStrings[psa->count++] = pCopy;

fail:
	return;
}

void SG_stringarray__remove_all(
    SG_context* pCtx,
    SG_stringarray* psa,
    const char* psz,
    SG_uint32 * pNumOccurrencesRemoved
    )
{
    SG_uint32 numOccurrencesRemoved = 0;
    SG_uint32 i;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(psa);

	if (psz)
	{
		for(i=0;i<psa->count;++i)
		{
			if(strcmp(psa->aStrings[i],psz)==0)
				numOccurrencesRemoved += 1;
			else
				psa->aStrings[i-numOccurrencesRemoved] = psa->aStrings[i];
		}

	}
	else
	{
		numOccurrencesRemoved = psa->count;
	}

	psa->count -= numOccurrencesRemoved;
    
	if(pNumOccurrencesRemoved!=NULL)
        *pNumOccurrencesRemoved = numOccurrencesRemoved;
}

void SG_stringarray__pop(
	SG_context* pCtx,
	SG_stringarray* psa)
{
	SG_NULLARGCHECK_RETURN(psa);

	psa->count--;
}

void SG_stringarray__join(
	SG_context* pCtx,
	SG_stringarray* psa,
	const char* pszSeparator,
	SG_string** ppstrJoined)
{
	SG_string* pstr = NULL;
	SG_uint32 i;
	
	SG_NULLARGCHECK_RETURN(psa);
	SG_NULLARGCHECK_RETURN(ppstrJoined);

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr)  );
	
	for (i = 0; i < psa->count; i++)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, psa->aStrings[i])  );
		if ( pszSeparator && ((i + 1) < psa->count) )
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, pszSeparator)  );
	}
	
	*ppstrJoined = pstr;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_stringarray__get_nth(
	SG_context* pCtx,
	const SG_stringarray* psa,
	SG_uint32 n,
	const char** ppsz
	)
{

	SG_NULLARGCHECK_RETURN( psa );
	SG_NULLARGCHECK_RETURN( ppsz );

	if (n >= psa->count)
	{
		SG_ERR_THROW_RETURN( SG_ERR_ARGUMENT_OUT_OF_RANGE );
	}

	*ppsz = psa->aStrings[n];

}

void SG_stringarray__count(
	SG_context* pCtx,
	const SG_stringarray* psa,
	SG_uint32 * pCount)
{
    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(psa);
    SG_NULLARGCHECK_RETURN(pCount);
    *pCount = psa->count;
}

void SG_stringarray__sz_array(
	SG_context* pCtx,
	const SG_stringarray* psa,
	const char * const ** pppStrings)
{
    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(psa);
    SG_NULLARGCHECK_RETURN(pppStrings);
	*pppStrings = psa->aStrings;
}

void SG_stringarray__sz_array_and_count(
	SG_context* pCtx,
	const SG_stringarray* psa,
	const char * const ** pppStrings,
	SG_uint32 * pCount)
{
    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(psa);
    SG_NULLARGCHECK_RETURN(pppStrings);
    SG_NULLARGCHECK_RETURN(pCount);
	*pppStrings = psa->aStrings;
    *pCount = psa->count;
}

void SG_stringarray__to_rbtree_keys(
	SG_context* pCtx,
	const SG_stringarray* psa,
	SG_rbtree** pprb)
{
	SG_uint32 i, count;
	SG_rbtree* prb = NULL;

	SG_NULLARGCHECK_RETURN(psa);
	SG_NULLARGCHECK_RETURN(pprb);

	count = psa->count;
	if (count)
	{
		SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, &prb, count, NULL)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psa->aStrings[i])  );
		}
	}

	*pprb = prb;
	prb = NULL;

	/* common cleanup */
fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
}

void SG_stringarray__to_rbtree_keys__dedup(
	SG_context* pCtx,
	const SG_stringarray* psa,
	SG_rbtree** pprb)
{
	SG_uint32 i, count;
	SG_rbtree* prb = NULL;

	SG_NULLARGCHECK_RETURN(psa);
	SG_NULLARGCHECK_RETURN(pprb);

	count = psa->count;
	if (count)
	{
		SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, &prb, count, NULL)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_rbtree__update(pCtx, prb, psa->aStrings[i])  );
		}
	}

	*pprb = prb;
	prb = NULL;

	/* common cleanup */
fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
}

/**
 * Create a copy of the stringarray and de-dup it
 * and PRESERVE THE ORDER of the strings.
 *
 */
void SG_stringarray__alloc_copy_dedup(SG_context * pCtx,
									  const SG_stringarray * psaInput,
									  SG_stringarray ** ppsaNew)
{
	SG_stringarray * psaNew = NULL;
	SG_rbtree * prb = NULL;
	SG_uint32 k, nrInput = 0;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInput, &nrInput)  );
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaNew, nrInput)  );
	for (k=0; k<nrInput; k++)
	{
		const char * psz_k = NULL;
		SG_bool bFound_k;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInput, k, &psz_k)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb, psz_k, &bFound_k, NULL)  );
		if (!bFound_k)
		{
			SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaNew, psz_k)  );
			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psz_k)  );
		}
	}

	SG_RETURN_AND_NULL(psaNew, ppsaNew);

fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
	SG_STRINGARRAY_NULLFREE(pCtx, psaNew);
}

void SG_stringarray__to_ihash_keys(
	SG_context* pCtx,
	const SG_stringarray* psa,
	SG_ihash** ppih)
{
	SG_uint32 i, count;
	SG_ihash* pih = NULL;

	SG_NULLARGCHECK_RETURN(psa);
	SG_NULLARGCHECK_RETURN(ppih);

	count = psa->count;
	if (count)
	{
		SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pih)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih, psa->aStrings[i], 0)  );
		}
	}

	*ppih = pih;
	pih = NULL;

	/* common cleanup */
fail:
	SG_IHASH_NULLFREE(pCtx, pih);
}


// TODO bad idea.  If you need to search a stringarray, you
// shouldn't be using a stringarray.
void SG_stringarray__find(
	SG_context * pCtx,
	const SG_stringarray * psa,
	const char * pszToFind,
	SG_uint32 ndxStart,
	SG_bool * pbFound,
	SG_uint32 * pNdxFound)
{
	SG_uint32 k;

	SG_NULLARGCHECK_RETURN(psa);
	SG_NONEMPTYCHECK_RETURN(pszToFind);
	SG_NULLARGCHECK_RETURN(pbFound);
	// pNdxFound is optional

	for (k=ndxStart; k<psa->count; k++)
	{
		if (strcmp(psa->aStrings[k], pszToFind) == 0)
		{
			*pbFound = SG_TRUE;
			if (pNdxFound)
				*pNdxFound = k;
			return;
		}
	}

	*pbFound = SG_FALSE;
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_stringarray_debug__dump_to_console(SG_context * pCtx,
										   const SG_stringarray * psa,
										   const char * pszLabel)
{
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "StringArrayDump: %s\n",
							   ((pszLabel && *pszLabel) ? pszLabel : ""))  );
	if (psa)
	{
		SG_uint32 k;

		for (k=0; k < psa->count; k++)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "    [%d/%d]: %s\n",
									   k+1, psa->count,
									   ((psa->aStrings[k] && *psa->aStrings[k])
										? psa->aStrings[k] : "(null)"))  );
	}
}
#endif
