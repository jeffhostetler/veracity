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
 * @file u0055_vector.c
 *
 * @details A simple test to exercise SG_vector.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"

//////////////////////////////////////////////////////////////////
// we define a little trick here to prefix all global symbols (type,
// structures, functions) with our test name.  this allows all of
// the tests in the suite to be #include'd into one meta-test (without
// name collisions) when we do a GCOV run.

#define MyMain()				TEST_MAIN(u0055_vector)
#define MyDcl(name)				u0055_vector__##name
#define MyFn(name)				u0055_vector__##name

//////////////////////////////////////////////////////////////////

void MyFn(test1)(SG_context * pCtx)
{
	SG_vector * pVec = NULL;
	SG_uint32 k, ndx, len;
	void * pValue;
	SG_uint32 variable_1 = 0;
	SG_uint32 variable_2 = 0;
#define ADDR_1(k)		((void *)((&variable_1)+k))
#define ADDR_2(k)		((void *)((&variable_2)+k))

	VERIFY_ERR_CHECK(  SG_vector__alloc(pCtx,&pVec,20)  );

	VERIFY_ERR_CHECK(  SG_vector__length(pCtx,pVec,&len)  );
	VERIFY_COND("test1",(len==0));

	for (k=0; k<100; k++)
	{
		// fabricate a bogus pointer from a constant and stuff it into the vector.

		VERIFY_ERR_CHECK(  SG_vector__append(pCtx,pVec,ADDR_1(k),&ndx)  );
		VERIFY_COND("append",(ndx==k));
	}
	for (k=0; k<100; k++)
	{
		VERIFY_ERR_CHECK(  SG_vector__get(pCtx,pVec,k,&pValue)  );
		VERIFY_COND("get1",(pValue == ADDR_1(k)));
	}

	for (k=0; k<100; k++)
	{
		VERIFY_ERR_CHECK(  SG_vector__set(pCtx,pVec,k,ADDR_2(k))  );
	}
	for (k=0; k<100; k++)
	{
		VERIFY_ERR_CHECK(  SG_vector__get(pCtx,pVec,k,&pValue)  );
		VERIFY_COND("get2",(pValue == ADDR_2(k)));
	}

	VERIFY_ERR_CHECK(  SG_vector__length(pCtx,pVec,&len)  );
	VERIFY_COND("test1",(len==100));

	VERIFY_ERR_CHECK(  SG_vector__clear(pCtx,pVec)  );

	VERIFY_ERR_CHECK(  SG_vector__length(pCtx,pVec,&len)  );
	VERIFY_COND("test1",(len==0));

	// fall thru to common cleanup

fail:
	SG_VECTOR_NULLFREE(pCtx, pVec);
}

void MyFn(alloc__copy__shallow)(SG_context * pCtx)
{
	static const SG_uint32 uSize = 100u;
	
	SG_vector* pVector  = NULL;
	SG_vector* pCopy    = NULL;
	SG_uint32  uIndex   = 0u;
	SG_uint32  uOutput1 = 0u;
	SG_uint32  uOutput2 = 0u;
	void*      pOutput1 = NULL;
	void*      pOutput2 = NULL;

	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, uSize)  );

	// add some random stack data to the vector
	for (uIndex = 0u; uIndex < uSize; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, &uIndex + uIndex, &uOutput1)  );
		VERIFY_COND("Added item has unexpected index.", uOutput1 == uIndex);
	}

	// copy the vector
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC__COPY(pCtx, pVector, NULL, NULL, &pCopy)  );

	// verify that the copy matches the original
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uOutput1)  );
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pCopy,   &uOutput2)  );
	VERIFY_COND("Copied vector's length doesn't match added item count.", uOutput1 == uSize);
	VERIFY_COND("Copied vector's length doesn't match original.", uOutput1 == uOutput2);
	for (uIndex = 0u; uIndex < uOutput1; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_vector__get(pCtx, pVector, uIndex, &pOutput1)  );
		VERIFY_ERR_CHECK(  SG_vector__get(pCtx, pCopy,   uIndex, &pOutput2)  );
		VERIFYP_COND("Copied vector's value doesn't match original.", pOutput1 == pOutput2, ("index(%d", uIndex));
	}

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
	SG_VECTOR_NULLFREE(pCtx, pCopy);
}

static void MyFn(copy_uint32)(SG_context* pCtx, void* pInput, void** pOutput)
{
	SG_uint32* pInputInt  = (SG_uint32*)pInput;
	SG_uint32* pOutputInt = NULL;

	VERIFY_ERR_CHECK(  SG_alloc1(pCtx, pOutputInt)  );
	*pOutputInt = *pInputInt;

	*pOutput = pOutputInt;

fail:
	return;
}

static void MyFn(free_uint32)(SG_context* pCtx, void* pValue)
{
	VERIFY_ERR_CHECK(  SG_free(pCtx, pValue)  );

fail:
	return;
}

void MyFn(alloc__copy__deep)(SG_context * pCtx)
{
	static const SG_uint32 uSize = 100u;
	
	SG_vector* pVector  = NULL;
	SG_vector* pCopy    = NULL;
	SG_uint32  uIndex   = 0u;
	SG_uint32  uOutput1 = 0u;
	SG_uint32  uOutput2 = 0u;
	void*      pOutput1 = NULL;
	void*      pOutput2 = NULL;

	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, uSize)  );

	// add some allocated data to the vector
	for (uIndex = 0u; uIndex < uSize; ++uIndex)
	{
		SG_uint32* pValue = NULL;
		VERIFY_ERR_CHECK(  SG_alloc1(pCtx, pValue)  );
		*pValue = uIndex;
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, pValue, &uOutput1)  );
		VERIFY_COND("Added item has unexpected index.", uOutput1 == uIndex);
	}

	// copy the vector
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC__COPY(pCtx, pVector, MyFn(copy_uint32), MyFn(free_uint32), &pCopy)  );

	// verify that the copy matches the original
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uOutput1)  );
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pCopy,   &uOutput2)  );
	VERIFY_COND("Copied vector's length doesn't match added item count.", uOutput1 == uSize);
	VERIFY_COND("Copied vector's length doesn't match original.", uOutput1 == uOutput2);
	for (uIndex = 0u; uIndex < uOutput1; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_vector__get(pCtx, pVector, uIndex, &pOutput1)  );
		VERIFY_ERR_CHECK(  SG_vector__get(pCtx, pCopy,   uIndex, &pOutput2)  );
		VERIFYP_COND("Copied vector's pointer value matches original after deep copy.", pOutput1 != pOutput2, ("index(%d)", uIndex));
		uOutput1 = *((SG_uint32*)pOutput1);
		uOutput2 = *((SG_uint32*)pOutput2);
		VERIFYP_COND("Copied vector's pointed-to value doesn't match original after deep copy.", uOutput1 == uOutput2, ("index(%d)", uIndex));
	}

fail:
	SG_context__push_level(pCtx);
	SG_vector__free__with_assoc(pCtx, pVector, MyFn(free_uint32));
	SG_vector__free__with_assoc(pCtx, pCopy,   MyFn(free_uint32));
	SG_context__pop_level(pCtx);
}

void MyFn(clear__with_assoc)(SG_context * pCtx)
{
	static const SG_uint32 uSize = 100u;
	
	SG_vector* pVector = NULL;
	SG_uint32  uIndex  = 0u;
	SG_uint32  uOutput = 0u;

	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, uSize)  );

	// add some allocated data to the vector
	for (uIndex = 0u; uIndex < uSize; ++uIndex)
	{
		SG_uint32* pValue = NULL;
		VERIFY_ERR_CHECK(  SG_alloc1(pCtx, pValue)  );
		*pValue = uIndex;
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, pValue, &uOutput)  );
		VERIFY_COND("Added item has unexpected index.", uOutput == uIndex);
	}

	// verify that the length is what we expect
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uOutput)  );
	VERIFY_COND("Vector's length doesn't match added item count.", uOutput == uSize);

	// clear the vector using our free callback
	VERIFY_ERR_CHECK(  SG_vector__clear__with_assoc(pCtx, pVector, MyFn(free_uint32))  );
	// if we get memory leaks, then the callback wasn't properly called to free the elements

	// verify that the vector is now empty
	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uOutput)  );
	VERIFY_COND("Vector's length is non-zero after being cleared.", uOutput == 0u);

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
}

//////////////////////////////////////////////////////////////////

void _match_string(
	SG_context* pCtx,
	SG_uint32   uIndex,
	void*       pData,
	void*       pUserData,
	SG_bool*    pMatch
	)
{
	const char* szItemData  = (const char*)pData;
	const char* szMatchData = (const char*)pUserData;

	SG_UNUSED(pCtx);
	SG_UNUSED(uIndex);

	if (strcmp(szItemData, szMatchData) == 0)
	{
		*pMatch = SG_TRUE;
	}
	else
	{
		*pMatch = SG_FALSE;
	}
}

typedef struct
{
	SG_uint32   uMiddleIndex;
	SG_uint32   uExpectedIndex;
	const char* szExpectedValue;
}
_verify_value_data;

void _verify_value(
	SG_context* pCtx,
	SG_uint32   uIndex,
	void*       pData,
	void*       pUserData
	)
{
	_verify_value_data* pValueData = (_verify_value_data*)pUserData;

	SG_UNUSED(pCtx);

	VERIFY_COND("Value found at wrong index.", uIndex == pValueData->uExpectedIndex);
	VERIFY_COND("Wrong value found.", strcmp((const char*)pData, pValueData->szExpectedValue) == 0);

	pValueData->uExpectedIndex += pValueData->uMiddleIndex;
}

void MyFn(find)(SG_context* pCtx)
{
	// the test data contains the same set of elements twice in the same order
	static const char* aData[] = {
		"abc",
		"123",
		"456",
		"def",
		"abc",
		"123",
		"456",
		"def",
	};
	static const SG_uint32 uMiddle = SG_NrElements(aData) / 2u;

	SG_vector* pVector = NULL;
	SG_uint32  uIndex  = 0u;
	SG_bool    bFound  = SG_FALSE;

	// create a vector containing our test data
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, 5u)  );
	for (uIndex = 0u; uIndex < SG_NrElements(aData); ++uIndex)
	{
		SG_uint32 uAddIndex = 0u;

		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, (void*)aData[uIndex], &uAddIndex)  );
		VERIFY_COND("Test value added to wrong index.", uIndex == uAddIndex);
	}

	// make sure we find each of the values somewhere in the vector
	for (uIndex = 0u; uIndex < uMiddle; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)aData[uIndex], &bFound)  );
		VERIFY_COND("Expected value not found.", bFound == SG_TRUE);
	}

	// look for a couple items we know aren't in the vector
	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, NULL, &bFound)  );
	VERIFY_COND("Unexpected value found.", bFound == SG_FALSE);
	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)pVector, &bFound)  );
	VERIFY_COND("Unexpected value found.", bFound == SG_FALSE);
	VERIFY_ERR_CHECK(  SG_vector__contains(pCtx, pVector, (void*)&uIndex, &bFound)  );
	VERIFY_COND("Unexpected value found.", bFound == SG_FALSE);

	// make sure we find each of the values in their first location correctly
	for (uIndex = 0u; uIndex < uMiddle; ++uIndex)
	{
		SG_uint32 uFoundIndex = 0u;
		void*     pFoundData  = NULL;

		VERIFY_ERR_CHECK(  SG_vector__find__first(pCtx, pVector, _match_string, (void*)aData[uIndex], &uFoundIndex, &pFoundData)  );

		VERIFY_COND("Value found at wrong index.", uIndex == uFoundIndex);
		VERIFY_COND("Wrong value found.", strcmp(aData[uIndex], (const char*)pFoundData) == 0);

		// make sure we don't crash if we don't want either output
		VERIFY_ERR_CHECK(  SG_vector__find__first(pCtx, pVector, _match_string, (void*)aData[uIndex], NULL, NULL)  );
	}

	// make sure we find each of the values in their last location correctly
	for (uIndex = uMiddle; uIndex < SG_NrElements(aData); ++uIndex)
	{
		SG_uint32 uFoundIndex = 0u;
		void*     pFoundData  = NULL;

		VERIFY_ERR_CHECK(  SG_vector__find__last(pCtx, pVector, _match_string, (void*)aData[uIndex], &uFoundIndex, &pFoundData)  );

		VERIFY_COND("Value found at wrong index.", uIndex == uFoundIndex);
		VERIFY_COND("Wrong value found.", strcmp(aData[uIndex], (const char*)pFoundData) == 0);

		// make sure we don't crash if we don't want either output
		VERIFY_ERR_CHECK(  SG_vector__find__last(pCtx, pVector, _match_string, (void*)aData[uIndex], NULL, NULL)  );
	}

	// make sure we find each value at each of its locations correctly
	for (uIndex = 0u; uIndex < uMiddle; ++uIndex)
	{
		_verify_value_data cData;
		cData.szExpectedValue = aData[uIndex];
		cData.uExpectedIndex  = uIndex;
		cData.uMiddleIndex    = uMiddle;

		VERIFY_ERR_CHECK(  SG_vector__find__all(pCtx, pVector, _match_string, (void*)aData[uIndex], _verify_value, (void*)&cData)  );
	}

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
}

void MyFn(_free_simple_value)(
	SG_context* pCtx,
	void*       pValue
	)
{
	SG_free(pCtx, pValue);
}

void MyFn(_verify_size)(
	SG_context* pCtx,
	SG_vector*  pVector,
	SG_uint32   uExpected
	)
{
	SG_uint32 uSize = 0u;

	VERIFY_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uSize)  );
	VERIFYP_COND("Incorrect size.", uSize == uExpected, ("Expected(%u) Actual(%u)", uExpected, uSize));

fail:
	return;
}

void MyFn(_verify_offset_values)(
	SG_context* pCtx,
	SG_vector*  pVector,
	SG_uint32   uBegin,
	SG_uint32   uEnd,
	SG_int32    iOffset
	)
{
	SG_uint32 uIndex = 0u;

	for (uIndex = uBegin; uIndex < uEnd; ++uIndex)
	{
		SG_uint32* pValue    = NULL;
		SG_uint32  uExpected = uIndex + iOffset;

		VERIFY_ERR_CHECK(  SG_vector__get(pCtx, pVector, uIndex, (void**)&pValue)  );
		VERIFYP_COND("Incorrect value.", *pValue == uExpected, ("Expected(%u) Actual (%u) Index(%u)", uExpected, *pValue, uIndex));
	}

fail:
	return;
}

// Specification for a single item in a test vector.
typedef struct
{
	const char* szValue; //< Value to add to the test vector.
	SG_uint32   uCount;  //< Number of times to add the value to the test vector.
}
test_item;

// Specification of a simple test vector.
static test_item gaTestItems[] =
{
	{ "abc", 3 },
	{ "def", 2 },
	{ "123", 1 },
	{ "456", 2 },
};

// Generates a vector from gaTestItems.
void MyFn(_create_test_vector)(
	SG_context* pCtx,
	SG_vector** ppVector,
	SG_uint32*  pSize
	)
{
	SG_vector* pVector        = NULL;
	SG_uint32  uIndex         = 0u;
	SG_uint32  uExpectedTotal = 0u;
	SG_uint32  uPass          = 0u;
	SG_uint32  uPassTotal     = 1u;
	SG_uint32  uActualTotal   = 0u;

	// check how many big the final vector will be
	for (uIndex = 0u; uIndex < SG_NrElements(gaTestItems); ++uIndex)
	{
		test_item* pTestItem = gaTestItems + uIndex;
		uExpectedTotal += pTestItem->uCount;
	}

	// allocate the vector
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, uExpectedTotal)  );
	
	// add the items in passes rather than each item sequentially
	// this way values appended multiple times won't be sequential
	while (uPassTotal > 0u)
	{
		uPassTotal = 0u;
		for (uIndex = 0u; uIndex < SG_NrElements(gaTestItems); ++uIndex)
		{
			test_item* pTestItem = gaTestItems + uIndex;

			if (pTestItem->uCount > uPass)
			{
				VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, (void*)pTestItem->szValue, NULL)  );
				uPassTotal += 1u;
			}
		}
		uActualTotal += uPassTotal;
		uPass        += 1u;
	}
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uExpectedTotal)  );
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uActualTotal)  );

	// return the vector
	if (ppVector != NULL)
	{
		*ppVector = pVector;
		pVector = NULL;
	}
	if (pSize != NULL)
	{
		*pSize = uActualTotal;
	}

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
}

void MyFn(remove__index)(SG_context* pCtx)
{
	static const SG_uint32 uSize = 20u;

	SG_vector* pVector = NULL;
	SG_uint32  uIndex  = 0u;
	SG_uint32  uCount  = uSize;

	// create some test data
	// uSize elements, each with an SG_uint32* whose value equals its index
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, uSize)  );
	for (uIndex = 0u; uIndex < uSize; ++uIndex)
	{
		SG_uint32* pValue = NULL;
		SG_alloc1(pCtx, pValue);
		*pValue = uIndex;
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, pValue, NULL)  );
	}
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uCount)  );

	// remove the last index and verify
	VERIFY_ERR_CHECK(  SG_vector__remove__index(pCtx, pVector, uSize - 1u, MyFn(_free_simple_value))  );
	uCount -= 1u;
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uCount)  );
	VERIFY_ERR_CHECK(  MyFn(_verify_offset_values)(pCtx, pVector, 0u, uCount, 0)  );

	// remove the first index and verify
	VERIFY_ERR_CHECK(  SG_vector__remove__index(pCtx, pVector, 0u, MyFn(_free_simple_value))  );
	uCount -= 1u;
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uCount)  );
	VERIFY_ERR_CHECK(  MyFn(_verify_offset_values)(pCtx, pVector, 0u, uCount, 1)  );

	// remove a middle index and verify
	uIndex = uCount / 2u;
	VERIFY_ERR_CHECK(  SG_vector__remove__index(pCtx, pVector, uIndex, MyFn(_free_simple_value))  );
	uCount -= 1u;
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uCount)  );
	VERIFY_ERR_CHECK(  MyFn(_verify_offset_values)(pCtx, pVector, 0u, uIndex, 1)  );
	VERIFY_ERR_CHECK(  MyFn(_verify_offset_values)(pCtx, pVector, uIndex, uCount, 2)  );

fail:
	SG_vector__free__with_assoc(pCtx, pVector, MyFn(_free_simple_value));
	return;
}

void MyFn(remove__value)(SG_context* pCtx)
{
	SG_vector* pVector = NULL;
	SG_uint32  uSize   = 0u;
	SG_uint32  uIndex  = 0u;

	// create a test vector
	VERIFY_ERR_CHECK(  MyFn(_create_test_vector)(pCtx, &pVector, &uSize)  );

	// run through each item and remove it by value
	// verify that we remove the number of values expected
	// also verify that the resulting size is as expected
	for (uIndex = 0u; uIndex < SG_NrElements(gaTestItems); ++uIndex)
	{
		test_item* pTestItem = gaTestItems + uIndex;
		SG_uint32  uRemoved  = 0u;

		VERIFY_ERR_CHECK(  SG_vector__remove__value(pCtx, pVector, (void*)pTestItem->szValue, &uRemoved, NULL)  );
		VERIFYP_COND("Removed wrong number of items.", uRemoved == pTestItem->uCount, ("Value(%s) Expected(%u) Removed(%u)", pTestItem->szValue, pTestItem->uCount, uRemoved));
		uSize -= uRemoved;
		VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, uSize)  );
	}

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
	return;
}

/**
 * A predicate that matches any value whose index is evenly divisible by a given number.
 */
void MyFn(remove__if__predicate)(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the current item.
	void*       pValue,    //< [in] The value of the current item.
	void*       pUserData, //< [in] The number (SG_uint32*) divide the current index by.
	SG_bool*    pMatch     //< [out] Whether or not the item matches.
	)
{
	SG_uint32* pData = (SG_uint32*)pUserData;

	SG_UNUSED(pCtx);
	SG_UNUSED(pValue);

	if ( (uIndex % *pData) == 0u )
	{
		*pMatch = SG_TRUE;
	}
	else
	{
		*pMatch = SG_FALSE;
	}
}

void MyFn(remove__if)(SG_context* pCtx)
{
	static SG_uint32   uItemCount    = 20u;
	static const char* szItemValue   = "abc";
	static SG_uint32   uStartDivisor = 5u;

	SG_vector* pVector  = NULL;
	SG_uint32  uIndex   = 0u;
	SG_uint32  uCount   = uItemCount;
	SG_uint32  uDivisor = 0u;
	SG_uint32  uRemoved = 0u;

	// create a vector with test data
	VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, 10u)  );
	for (uIndex = 0u; uIndex < uItemCount; ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_vector__append(pCtx, pVector, (void*)szItemValue, NULL)  );
	}

	// run tests, removing every Nth, then every (N-1)th item, etc
	// the last iteration (every 1th item) will remove every remaining item
	for (uDivisor = uStartDivisor; uDivisor > 0u; --uDivisor)
	{
		SG_uint32 uExpected = uCount / uDivisor;

		VERIFY_ERR_CHECK(  SG_vector__remove__if(pCtx, pVector, MyFn(remove__if__predicate), (void*)&uDivisor, &uRemoved, NULL)  );
		VERIFYP_COND("Wrong number of items removed.", uRemoved == uExpected, ("Removed(%u) Expected(%u)", uRemoved, uExpected));

		uCount -= uExpected;
	}
	VERIFY_COND("Expected size should be zero.", uCount == 0u);

	// verify that the vector is empty
	VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pVector, 0u)  );

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
	return;
}

void MyFn(match_value__append)(SG_context* pCtx)
{
	SG_vector* pVector = NULL;
	SG_uint32  uSize   = 0u;
	SG_uint32  uIndex  = 0u;
	SG_vector* pTarget = NULL;

	// create a test vector
	VERIFY_ERR_CHECK(  MyFn(_create_test_vector)(pCtx, &pVector, &uSize)  );

	// run through each test item and copy all indices with that value to another vector
	// verify that the other vector receives the correct number of items
	for (uIndex = 0u; uIndex < SG_NrElements(gaTestItems); ++uIndex)
	{
		test_item* pTestItem = gaTestItems + uIndex;

		// allocate a target vector
		VERIFY_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pTarget, uSize)  );

		// find all values that match the current item
		// add them to the target vector
		VERIFY_ERR_CHECK(  SG_vector__find__all(pCtx, pVector, SG_vector__predicate__match_value, (void*)pTestItem->szValue, SG_vector__callback__append, pTarget)  );

		// verify the size of the target vector
		VERIFY_ERR_CHECK(  MyFn(_verify_size)(pCtx, pTarget, pTestItem->uCount)  );

		// free the target vector
		SG_VECTOR_NULLFREE(pCtx, pTarget);
		pTarget = NULL;
	}

fail:
	SG_VECTOR_NULLFREE(pCtx, pVector);
	SG_VECTOR_NULLFREE(pCtx, pTarget);
	return;
}

//////////////////////////////////////////////////////////////////

MyMain()
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  MyFn(test1)(pCtx)  );
	BEGIN_TEST(  MyFn(alloc__copy__shallow)(pCtx)  );
	BEGIN_TEST(  MyFn(alloc__copy__deep)(pCtx)  );
	BEGIN_TEST(  MyFn(clear__with_assoc)(pCtx)  );
	BEGIN_TEST(  MyFn(find)(pCtx)  );
	BEGIN_TEST(  MyFn(remove__index)(pCtx)  );
	BEGIN_TEST(  MyFn(remove__value)(pCtx)  );
	BEGIN_TEST(  MyFn(remove__if)(pCtx)  );
	BEGIN_TEST(  MyFn(match_value__append)(pCtx)  );

	TEMPLATE_MAIN_END;
}

//////////////////////////////////////////////////////////////////

#undef MyMain
#undef MyDcl
#undef MyFn
