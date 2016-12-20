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
 * @file sg_vector.c
 *
 * @details A growable vector to store pointers.  This is a peer of RBTREE
 * that lets us store an array of pointers.  These are like VHASH and VARRAY.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

static const SG_uint32 MIN_CHUNK_SIZE = 10;

struct _SG_vector
{
	SG_uint32		m_uiSizeInUse;		// currently used
	SG_uint32		m_uiAllocated;		// amount allocated
	SG_uint32		m_uiChunkSize;		// realloc delta
	void **			m_array;			// an array of pointers
};

//////////////////////////////////////////////////////////////////

static void _sg_vector__grow(SG_context* pCtx, SG_vector * pVector, SG_uint32 additionalSpaceNeeded)
{
	SG_uint32 totalSpaceNeeded;
	SG_uint32 totalSpaceRounded;
	void ** pNewArray = NULL;

	SG_NULLARGCHECK_RETURN(pVector);

	totalSpaceNeeded = pVector->m_uiSizeInUse + additionalSpaceNeeded;
	if (totalSpaceNeeded <= pVector->m_uiAllocated)
		return;

    // TODO This grow function is REALLY slow when given a small chunk size
    // and when trying to insert a large number of items.  Consider a doubling
    // grow function instead of an additive one.

    totalSpaceRounded = ((totalSpaceNeeded + pVector->m_uiChunkSize - 1) / pVector->m_uiChunkSize) * pVector->m_uiChunkSize;

	SG_ERR_CHECK_RETURN(  SG_alloc(pCtx, totalSpaceRounded,sizeof(void *),&pNewArray)  );

	if (pVector->m_array)
	{
		if (pVector->m_uiSizeInUse > 0)
			memmove(pNewArray,pVector->m_array,pVector->m_uiSizeInUse*sizeof(void *));
		SG_NULLFREE(pCtx, pVector->m_array);
	}

	pVector->m_array = pNewArray;
	pVector->m_uiAllocated = totalSpaceRounded;

	SG_ASSERT_RELEASE_RETURN(  (pVector->m_uiSizeInUse <= pVector->m_uiAllocated)  );
}

void SG_vector__reserve(SG_context* pCtx, SG_vector * pVector, SG_uint32 sizeNeeded)
{
	SG_NULLARGCHECK_RETURN(pVector);

	if (sizeNeeded > pVector->m_uiSizeInUse)
	{
		SG_uint32 k;

		SG_ERR_CHECK_RETURN(  _sg_vector__grow(pCtx, pVector, (sizeNeeded - pVector->m_uiSizeInUse))  );

		for (k=pVector->m_uiSizeInUse; k<sizeNeeded; k++)
			pVector->m_array[k] = 0;
		pVector->m_uiSizeInUse = sizeNeeded;
	}

	SG_ASSERT_RELEASE_RETURN(  (pVector->m_uiSizeInUse <= pVector->m_uiAllocated)  );
}

//////////////////////////////////////////////////////////////////

void SG_vector__alloc(SG_context* pCtx, SG_vector ** ppVector, SG_uint32 suggestedInitialSize)
{
	SG_vector * pVector = NULL;

	SG_NULLARGCHECK_RETURN(ppVector);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pVector)  );

	pVector->m_uiSizeInUse = 0;
	pVector->m_uiAllocated = 0;
	pVector->m_uiChunkSize = MIN_CHUNK_SIZE;
	pVector->m_array = NULL;

	SG_ERR_CHECK(  _sg_vector__grow(pCtx, pVector,suggestedInitialSize)  );

	*ppVector = pVector;

	return;

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
}

void SG_vector__alloc__copy(SG_context* pCtx, const SG_vector* pInputVector, SG_copy_callback fCopyCallback, SG_free_callback fFreeCallback, SG_vector** ppOutputVector)
{
	SG_vector* pVector = NULL;
	SG_uint32  uIndex  = 0u;

	SG_NULLARGCHECK(pInputVector);
	SG_NULLARGCHECK(ppOutputVector);

	// allocate memory for the vector structure
	SG_ERR_CHECK(  SG_alloc1(pCtx, pVector)  );

	// initialize its data
	pVector->m_uiSizeInUse = 0u;
	pVector->m_uiAllocated = 0u;
	pVector->m_uiChunkSize = pInputVector->m_uiChunkSize;
	pVector->m_array       = NULL;

	// grow the vector to the same size as the input vector
	SG_ERR_CHECK(  _sg_vector__grow(pCtx, pVector, pInputVector->m_uiAllocated)  );

	// copy the items from the existing vector into the new one
	if (fCopyCallback == NULL)
	{
		memcpy(pVector->m_array, pInputVector->m_array, pInputVector->m_uiSizeInUse * sizeof(void*));
		pVector->m_uiSizeInUse = pInputVector->m_uiSizeInUse;
	}
	else
	{
		for (uIndex = 0u; uIndex < pInputVector->m_uiSizeInUse; ++uIndex)
		{
			SG_ERR_CHECK(  fCopyCallback(pCtx, pInputVector->m_array[uIndex], &(pVector->m_array[uIndex]))  );
			pVector->m_uiSizeInUse++;
		}
	}

	// return the new vector
	*ppOutputVector = pVector;
	return;

fail:
	if (pVector != NULL)
	{
		// if we have a free callback, we need to call it on any items copied so far with the copy callback
		if (fFreeCallback != NULL && fCopyCallback != NULL)
		{
			for (uIndex = 0u; uIndex < pVector->m_uiSizeInUse; ++uIndex)
			{
				// to check if we've copied an item into the new vector:
				// 1) check to see if its non-NULL
				//    if it's NULL, then we haven't copied it yet
				// 2) check to see if its got a different value than the vector we're copying
				//    if it has the same value, then the copy is shallow and there's nothing to free
				if (pVector->m_array[uIndex] != NULL && pVector->m_array[uIndex] != pInputVector->m_array[uIndex])
				{
					fFreeCallback(pCtx, pVector->m_array[uIndex]);
				}
			}
		}

		// free the output vector
		SG_VECTOR_NULLFREE(pCtx, pVector);
	}
}

void SG_vector__free(SG_context * pCtx, SG_vector * pVector)
{
	if (!pVector)
		return;

	// since we don't know what the array of pointers are, we do not free them.
	// the caller is responsible for this.

	SG_NULLFREE(pCtx, pVector->m_array);

	pVector->m_uiSizeInUse = 0;
	pVector->m_uiAllocated = 0;
	SG_NULLFREE(pCtx, pVector);
}

void SG_vector__free__with_assoc(SG_context * pCtx, SG_vector * pVector, SG_free_callback * pcbFreeValue)
{
	SG_uint32 k, kLimit;

	if (!pVector)
		return;

	if (pVector->m_array)
	{
		kLimit = pVector->m_uiSizeInUse;
		for (k=0; k<kLimit; k++)
		{
			void * p = pVector->m_array[k];
			if (p)
			{
				(*pcbFreeValue)(pCtx, p);
				pVector->m_array[k] = NULL;
			}
		}
	}

	SG_vector__free(pCtx, pVector);
}

//////////////////////////////////////////////////////////////////

void SG_vector__append(SG_context* pCtx, SG_vector * pVector, void * pValue, SG_uint32 * pIndexReturned)
{
	SG_uint32 k;

	SG_NULLARGCHECK_RETURN(pVector);

	// insert value at array[size_in_use]

	if (pVector->m_uiSizeInUse >= pVector->m_uiAllocated)
	{
		SG_ERR_CHECK_RETURN(  _sg_vector__grow(pCtx, pVector,1)  );
		SG_ASSERT( (pVector->m_uiSizeInUse < pVector->m_uiAllocated)  &&  "vector grow failed"  );
	}

	k = pVector->m_uiSizeInUse++;

	pVector->m_array[k] = pValue;
	if (pIndexReturned)
		*pIndexReturned = k;
}

void SG_vector__pop_back(SG_context * pCtx, SG_vector * pVector, void ** ppValue)
{
    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pVector);

    if(ppValue!=NULL)
        *ppValue = pVector->m_array[pVector->m_uiSizeInUse-1];

    --pVector->m_uiSizeInUse;
}

//////////////////////////////////////////////////////////////////

void SG_vector__get(SG_context* pCtx, const SG_vector * pVector, SG_uint32 k, void ** ppValue)
{
	SG_NULLARGCHECK_RETURN(pVector);
	SG_NULLARGCHECK_RETURN(ppValue);

	SG_ARGCHECK_RETURN(k < pVector->m_uiSizeInUse, pVector->m_uiSizeInUse);

	*ppValue = pVector->m_array[k];
}

void SG_vector__set(SG_context* pCtx, SG_vector * pVector, SG_uint32 k, void * pValue)
{
	SG_NULLARGCHECK_RETURN(pVector);

	SG_ARGCHECK_RETURN(k < pVector->m_uiSizeInUse, pVector->m_uiSizeInUse);

	pVector->m_array[k] = pValue;
}

//////////////////////////////////////////////////////////////////

void SG_vector__clear(SG_context* pCtx, SG_vector * pVector)
{
	SG_NULLARGCHECK_RETURN(pVector);

	pVector->m_uiSizeInUse = 0;

	if (pVector->m_array && (pVector->m_uiAllocated > 0))
		memset(pVector->m_array,0,(pVector->m_uiAllocated * sizeof(void *)));
}

void SG_vector__clear__with_assoc(SG_context * pCtx, SG_vector * pVector, SG_free_callback * pcbFreeValue)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK_RETURN(pVector);

	for (uIndex = 0u; uIndex < pVector->m_uiSizeInUse; ++uIndex)
	{
		if (pVector->m_array[uIndex] != NULL)
		{
			(*pcbFreeValue)(pCtx, pVector->m_array[uIndex]);
			pVector->m_array[uIndex] = NULL;
		}
	}

	SG_vector__clear(pCtx, pVector);
}

//////////////////////////////////////////////////////////////////

void SG_vector__length(SG_context* pCtx, const SG_vector * pVector, SG_uint32 * pLength)
{
	SG_NULLARGCHECK_RETURN(pVector);
	SG_NULLARGCHECK_RETURN(pLength);

	*pLength = pVector->m_uiSizeInUse;
}

//////////////////////////////////////////////////////////////////

void SG_vector__setChunkSize(SG_context* pCtx, SG_vector * pVector, SG_uint32 size)
{
	SG_NULLARGCHECK_RETURN(pVector);

	if (size < MIN_CHUNK_SIZE)
		size = MIN_CHUNK_SIZE;

	pVector->m_uiChunkSize = size;
}

//////////////////////////////////////////////////////////////////

void SG_vector__foreach(SG_context * pCtx,
						SG_vector * pVector,
						SG_vector_foreach_callback * pfn,
						void * pVoidCallerData)
{
	SG_uint32 k, kLimit;

	if (!pVector)
		return;

	if (!pVector->m_array)
		return;

	kLimit = pVector->m_uiSizeInUse;
	for (k=0; k<kLimit; k++)
	{
		void * p = pVector->m_array[k];			// this may be null if you haven't put anything in this cell

		SG_ERR_CHECK_RETURN(  (*pfn)(pCtx,k,p,pVoidCallerData)  );
	}
}

//////////////////////////////////////////////////////////////////

void SG_vector__find__first(
	SG_context*          pCtx,
	SG_vector*           pVector,
	SG_vector_predicate* fPredicate,
	void*                pUserData,
	SG_uint32*           pFoundIndex,
	void**               ppFoundData
	)
{
	SG_uint32 uFoundIndex = pVector->m_uiSizeInUse;
	void*     pFoundData  = NULL;

	if (pVector != NULL && pVector->m_array != NULL)
	{
		SG_uint32 uIndex = 0u;

		for (uIndex = 0u; uIndex < pVector->m_uiSizeInUse; ++uIndex)
		{
			void*   pValue = pVector->m_array[uIndex];
			SG_bool bMatch = SG_FALSE;
			
			SG_ERR_CHECK(  fPredicate(pCtx, uIndex, pValue, pUserData, &bMatch)  );

			if (bMatch == SG_TRUE)
			{
				uFoundIndex = uIndex;
				pFoundData  = pValue;
				break;
			}
		}
	}

	if (pFoundIndex != NULL)
	{
		*pFoundIndex = uFoundIndex;
	}

	if (ppFoundData != NULL)
	{
		*ppFoundData = pFoundData;
		pFoundData = NULL;
	}

fail:
	return;
}

void SG_vector__find__last(
	SG_context*          pCtx,
	SG_vector*           pVector,
	SG_vector_predicate* fPredicate,
	void*                pUserData,
	SG_uint32*           pFoundIndex,
	void**               ppFoundData
	)
{
	SG_uint32 uFoundIndex = pVector->m_uiSizeInUse;
	void*     pFoundData  = NULL;

	if (pVector != NULL && pVector->m_array != NULL)
	{
		SG_uint32 uIndex = pVector->m_uiSizeInUse;

		while (uIndex > 0u)
		{
			void*   pValue = NULL;
			SG_bool bMatch = SG_FALSE;

			--uIndex;
			pValue = pVector->m_array[uIndex];

			SG_ERR_CHECK(  fPredicate(pCtx, uIndex, pValue, pUserData, &bMatch)  );

			if (bMatch == SG_TRUE)
			{
				uFoundIndex = uIndex;
				pFoundData  = pValue;
				break;
			}
		}
	}

	if (pFoundIndex != NULL)
	{
		*pFoundIndex = uFoundIndex;
	}

	if (ppFoundData != NULL)
	{
		*ppFoundData = pFoundData;
		pFoundData = NULL;
	}

fail:
	return;
}

void SG_vector__find__all(
	SG_context*                 pCtx,
	SG_vector*                  pVector,
	SG_vector_predicate*        fPredicate,
	void*                       pPredicateData,
	SG_vector_foreach_callback* fCallback,
	void*                       pCallbackData
	)
{
	if (pVector != NULL && pVector->m_array != NULL)
	{
		SG_uint32 uIndex = 0u;

		for (uIndex = 0u; uIndex < pVector->m_uiSizeInUse; ++uIndex)
		{
			void*   pValue = pVector->m_array[uIndex];
			SG_bool bMatch = SG_FALSE;
			
			SG_ERR_CHECK(  fPredicate(pCtx, uIndex, pValue, pPredicateData, &bMatch)  );

			if (bMatch == SG_TRUE)
			{
				SG_ERR_CHECK(  fCallback(pCtx, uIndex, pValue, pCallbackData)  );
			}
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_vector__contains(
	SG_context* pCtx,
	SG_vector*  pVector,
	void*       pData,
	SG_bool*    pContains
	)
{
	void* pFound = NULL;

	SG_NULLARGCHECK(pVector);
	SG_NULLARGCHECK(pContains);

	SG_ERR_CHECK(  SG_vector__find__first(pCtx, pVector, SG_vector__predicate__match_value, pData, NULL, &pFound)  );

	if (pFound == NULL)
	{
		*pContains = SG_FALSE;
	}
	else
	{
		*pContains = SG_TRUE;
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_vector__remove__index(
	SG_context*       pCtx,
	SG_vector*        pVector,
	SG_uint32         uIndex,
	SG_free_callback* fDestructor
	)
{
	SG_NULLARGCHECK(pVector);
	SG_NULLARGCHECK(pVector->m_array);
	SG_ARGCHECK(uIndex < pVector->m_uiSizeInUse, pVector->m_uiSizeInUse);

	// free the value at the given index
	if (fDestructor != NULL)
	{
		SG_ERR_CHECK(  fDestructor(pCtx, pVector->m_array[uIndex])  );
	}

	// copy all the elements after the given index on top of the given index
	memcpy(pVector->m_array + uIndex, pVector->m_array + uIndex + 1u, (pVector->m_uiSizeInUse - uIndex - 1u) * sizeof(void*));

	// blank out the last value, which we'll no longer use
	pVector->m_array[pVector->m_uiSizeInUse - 1u] = NULL;

	// decrement our count
	pVector->m_uiSizeInUse -= 1u;

fail:
	return;
}

void SG_vector__remove__value(
	SG_context*       pCtx,
	SG_vector*        pVector,
	void*             pValue,
	SG_uint32*        pCount,
	SG_free_callback* fDestructor
	)
{
	SG_ERR_CHECK_RETURN(  SG_vector__remove__if(pCtx, pVector, SG_vector__predicate__match_value, pValue, pCount, fDestructor)  );
}

void SG_vector__remove__if(
	SG_context*          pCtx,
	SG_vector*           pVector,
	SG_vector_predicate* fPredicate,
	void*                pUserData,
	SG_uint32*           pCount,
	SG_free_callback*    fDestructor
	)
{
	void** ppOldArray = NULL;

	if (pVector != NULL && pVector->m_array != NULL)
	{
		SG_uint32 uIndex     = 0u;
		void**    ppNewArray = NULL;
		SG_uint32 uNewCount  = 0u;

		// allocate a new array to copy kept values into
		SG_ERR_CHECK(  SG_alloc(pCtx, pVector->m_uiAllocated, sizeof(void*), &ppNewArray)  );

		// run through each item in the current array
		for (uIndex = 0u; uIndex < pVector->m_uiSizeInUse; ++uIndex)
		{
			void*   pValue = pVector->m_array[uIndex];
			SG_bool bMatch = SG_FALSE;
			
			// check if we should remove it
			SG_ERR_CHECK(  fPredicate(pCtx, uIndex, pValue, pUserData, &bMatch)  );
			if (bMatch == SG_TRUE)
			{
				// we're removing it
				// call the supplied destructor
				if (fDestructor != NULL)
				{
					SG_ERR_CHECK(  fDestructor(pCtx, pValue)  );
				}
			}
			else
			{
				// we're keeping it, copy it to the new array
				ppNewArray[uNewCount] = pValue;
				uNewCount += 1u;
			}
		}

		// swap the new array into the vector
		ppOldArray = pVector->m_array;
		pVector->m_array = ppNewArray;
		ppNewArray = NULL;

		// return the number of items removed if they want it
		if (pCount != NULL)
		{
			*pCount = pVector->m_uiSizeInUse - uNewCount;
		}

		// update the size of the vector to match the new array
		pVector->m_uiSizeInUse = uNewCount;
	}

fail:
	SG_NULLFREE(pCtx, ppOldArray);
	return;
}

//////////////////////////////////////////////////////////////////

void SG_vector__predicate__match_value(
	SG_context* pCtx,
	SG_uint32   uIndex,
	void*       pValue,
	void*       pUserData,
	SG_bool*    pMatch
	)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(uIndex);

	if (pValue == pUserData)
	{
		*pMatch = SG_TRUE;
	}
	else
	{
		*pMatch = SG_FALSE;
	}
}

void SG_vector__callback__append(
	SG_context* pCtx,
	SG_uint32   uIndex,
	void*       pValue,
	void*       pUserData
	)
{
	SG_vector* pVector = (SG_vector*)pUserData;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pUserData);

	SG_ERR_CHECK(  SG_vector__append(pCtx, pVector, pValue, NULL)  );

fail:
	return;
}
