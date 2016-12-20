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
 * @file u0081_varray.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_pendingtree.h"

//////////////////////////////////////////////////////////////////

static void _assertCount(
	 SG_context *pCtx,
	 const SG_varray *a,
	 SG_uint32 expected,
	 const char *desc)
{
	SG_uint32	count = 0;
	SG_string	*msg = NULL;

	VERIFY_ERR_CHECK(  SG_varray__count(pCtx, a, &count)  );

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &msg)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, msg, "%s: expected %u, got %u", 
										  desc, (unsigned)expected, (unsigned)count
										  )  
					);

	VERIFY_COND(SG_string__sz(msg), (count == expected));

fail:
	SG_STRING_NULLFREE(pCtx, msg);
}


static void _assertStringAt(
	 SG_context *pCtx,
	 const SG_varray *a,
	 SG_uint32 pos,
	 const char *expected)
{
	SG_string	*msg = NULL;
	const char *got = NULL;

	VERIFY_ERR_CHECK(  SG_varray__get__sz(pCtx, a, pos, &got)  );
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &msg)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, msg, "string at %u: expected '%s', got '%s'", 
										  (unsigned)pos, expected, got
										  )  
					);

	VERIFY_COND(SG_string__sz(msg), strcmp(got, expected) == 0);

fail:
	SG_STRING_NULLFREE(pCtx, msg);
}

void u0081__remove_only(SG_context *pCtx)  
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_only")  );

	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 1, "size before delete")  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 0)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 0, "size after delete")  );

fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

void u0081__remove_first(SG_context *pCtx)  
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_first 0")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_first 1")  );

	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 2, "size before delete")  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 0)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 1, "size after delete")  );
	VERIFY_ERR_CHECK(  _assertStringAt(pCtx, a, 0, "u0081__remove_first 1")  );

fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

void u0081__remove_last(SG_context *pCtx)  
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_last 0")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_last 1")  );

	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 2, "size before delete")  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 1)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 1, "size after delete")  );
	VERIFY_ERR_CHECK(  _assertStringAt(pCtx, a, 0, "u0081__remove_last 0")  );

fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

void u0081__remove_middle(SG_context *pCtx)  
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_middle 0")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_middle 1")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_middle 2")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_middle 3")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_middle 4")  );

	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 5, "size before delete")  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 1)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 4, "size after delete")  );
	VERIFY_ERR_CHECK(  _assertStringAt(pCtx, a, 0, "u0081__remove_middle 0")  );
	VERIFY_ERR_CHECK(  _assertStringAt(pCtx, a, 1, "u0081__remove_middle 2")  );
	VERIFY_ERR_CHECK(  _assertStringAt(pCtx, a, 2, "u0081__remove_middle 3")  );
	VERIFY_ERR_CHECK(  _assertStringAt(pCtx, a, 3, "u0081__remove_middle 4")  );

fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

void u0081__remove_all(SG_context *pCtx)  
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_all 0")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_all 1")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_all 2")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_all 3")  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_all 4")  );

	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 5, "size before delete")  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 1)  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 3)  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 0)  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 1)  );
	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 0)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, a, 0, "size after delete")  );

fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

void u0081__remove_past_end(SG_context *pCtx)  
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_past_end")  );

	SG_varray__remove(pCtx, a, 1);

	VERIFY_CTX_HAS_ERR("error expected", pCtx);
	VERIFY_CTX_ERR_EQUALS("out-of-range expected", pCtx, SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	SG_context__err_reset(pCtx);
fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

void  u0081__remove_last_twice(SG_context * pCtx)
{
	SG_varray *a = NULL;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &a)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, a, "u0081__remove_last_twice")  );

	VERIFY_ERR_CHECK(  SG_varray__remove(pCtx, a, 0)  );
	SG_varray__remove(pCtx, a, 0);

	VERIFY_CTX_HAS_ERR("error expected", pCtx);
	VERIFY_CTX_ERR_EQUALS("out-of-range expected", pCtx, SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	SG_context__err_reset(pCtx);
fail:
	SG_VARRAY_NULLFREE(pCtx, a);
}

static void u0081__appendcopy__varray(SG_context* pCtx)
{
	SG_varray* pThis  = NULL;
	SG_varray* pArg   = NULL;
	SG_varray* pCopy  = NULL;
	SG_bool    bEqual = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pArg)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pArg, 0u, "size of arg before population")  );
	VERIFY_ERR_CHECK(  SG_varray__append__null(pCtx, pArg)  );
	VERIFY_ERR_CHECK(  SG_varray__append__bool(pCtx, pArg, SG_TRUE)  );
	VERIFY_ERR_CHECK(  SG_varray__append__int64(pCtx, pArg, 2)  );
	VERIFY_ERR_CHECK(  SG_varray__append__double(pCtx, pArg, 3.0)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pArg, "value 4")  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pArg, 5u, "size of arg after population")  );

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pThis)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, 0u, "size of this before test")  );
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__varray(pCtx, pThis, pArg, NULL)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, 1u, "size of this after test")  );

	VERIFY_ERR_CHECK(  SG_varray__get__varray(pCtx, pThis, 0u, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != pArg);
	VERIFY_ERR_CHECK(  SG_varray__equal(pCtx, pArg, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

fail:
	SG_VARRAY_NULLFREE(pCtx, pThis);
	SG_VARRAY_NULLFREE(pCtx, pArg);
}

static void u0081__appendcopy__varray__badargs(SG_context* pCtx)
{
	SG_varray* pArray = NULL;

	SG_VARRAY__ALLOC(pCtx, &pArray);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__appendcopy__varray(pCtx, NULL,   pArray, NULL), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__appendcopy__varray(pCtx, pArray, NULL, NULL),   SG_ERR_INVALIDARG  );

	SG_VARRAY_NULLFREE(pCtx, pArray);
}

static void u0081__appendcopy__vhash(SG_context* pCtx)
{
	SG_varray* pThis  = NULL;
	SG_vhash*  pArg   = NULL;
	SG_vhash*  pCopy  = NULL;
	SG_bool    bEqual = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pArg)  );
	VERIFY_ERR_CHECK(  SG_vhash__add__null(pCtx, pArg, "null")  );
	VERIFY_ERR_CHECK(  SG_vhash__add__bool(pCtx, pArg, "bool", SG_TRUE)  );
	VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pArg, "int64", 2)  );
	VERIFY_ERR_CHECK(  SG_vhash__add__double(pCtx, pArg, "double", 3.0)  );
	VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pArg, "string", "value 4")  );

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pThis)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, 0u, "size of this before test")  );
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pThis, pArg, NULL)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, 1u, "size of this after test")  );

	VERIFY_ERR_CHECK(  SG_varray__get__vhash(pCtx, pThis, 0u, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != pArg);
	VERIFY_ERR_CHECK(  SG_vhash__equal(pCtx, pArg, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

fail:
	SG_VARRAY_NULLFREE(pCtx, pThis);
	SG_VHASH_NULLFREE(pCtx, pArg);
}

static void u0081__appendcopy__vhash__badargs(SG_context* pCtx)
{
	SG_varray* pArray = NULL;
	SG_vhash*  pHash  = NULL;

	SG_VARRAY__ALLOC(pCtx, &pArray);
	SG_VHASH__ALLOC(pCtx, &pHash);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__appendcopy__vhash(pCtx, NULL,   pHash, NULL), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__appendcopy__vhash(pCtx, pArray, NULL, NULL),  SG_ERR_INVALIDARG  );

	SG_VARRAY_NULLFREE(pCtx, pArray);
	SG_VHASH_NULLFREE(pCtx, pHash);
}

static void u0081__appendcopy__variant(SG_context* pCtx)
{
	SG_varray*        pThis  = NULL;
	SG_uint32         uCount = 0u;
	SG_variant        oVariant;
	const SG_variant* pCopy  = NULL;
	SG_bool           bEqual = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pThis)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, uCount, "initial size")  );

	oVariant.type = SG_VARIANT_TYPE_NULL;
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pThis, &oVariant)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, ++uCount, "size after NULL")  );
	VERIFY_ERR_CHECK(  SG_varray__get__variant(pCtx, pThis, uCount-1, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != &oVariant);
	VERIFY_ERR_CHECK(  SG_variant__equal(pCtx, &oVariant, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

	oVariant.type = SG_VARIANT_TYPE_BOOL;
	oVariant.v.val_bool = SG_TRUE;
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pThis, &oVariant)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, ++uCount, "size after NULL")  );
	VERIFY_ERR_CHECK(  SG_varray__get__variant(pCtx, pThis, uCount-1, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != &oVariant);
	VERIFY_ERR_CHECK(  SG_variant__equal(pCtx, &oVariant, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

	oVariant.type = SG_VARIANT_TYPE_INT64;
	oVariant.v.val_int64 = 2;
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pThis, &oVariant)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, ++uCount, "size after NULL")  );
	VERIFY_ERR_CHECK(  SG_varray__get__variant(pCtx, pThis, uCount-1, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != &oVariant);
	VERIFY_ERR_CHECK(  SG_variant__equal(pCtx, &oVariant, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

	oVariant.type = SG_VARIANT_TYPE_DOUBLE;
	oVariant.v.val_double = 3.0;
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pThis, &oVariant)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, ++uCount, "size after NULL")  );
	VERIFY_ERR_CHECK(  SG_varray__get__variant(pCtx, pThis, uCount-1, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != &oVariant);
	VERIFY_ERR_CHECK(  SG_variant__equal(pCtx, &oVariant, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

	oVariant.type = SG_VARIANT_TYPE_SZ;
	oVariant.v.val_sz = "value 4";
	VERIFY_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pThis, &oVariant)  );
	VERIFY_ERR_CHECK(  _assertCount(pCtx, pThis, ++uCount, "size after NULL")  );
	VERIFY_ERR_CHECK(  SG_varray__get__variant(pCtx, pThis, uCount-1, &pCopy)  );
	VERIFY_COND("copy is NULL", pCopy != NULL);
	VERIFY_COND("copy is same pointer as original", pCopy != &oVariant);
	VERIFY_ERR_CHECK(  SG_variant__equal(pCtx, &oVariant, pCopy, &bEqual)  );
	VERIFY_COND("copy isn't equal to original", bEqual == SG_TRUE);

fail:
	SG_VARRAY_NULLFREE(pCtx, pThis);
}

static void u0081__appendcopy__variant__badargs(SG_context* pCtx)
{
	SG_varray* pArray = NULL;
	SG_variant oVariant;

	SG_VARRAY__ALLOC(pCtx, &pArray);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__appendcopy__variant(pCtx, NULL,   &oVariant), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__appendcopy__variant(pCtx, pArray, NULL),      SG_ERR_INVALIDARG  );

	SG_VARRAY_NULLFREE(pCtx, pArray);
}

static void u0081__find(SG_context* pCtx)
{
	SG_varray* pThis      = NULL;
	SG_variant oVariant;
	SG_bool    bContains  = SG_FALSE;
	SG_bool    bContains2 = SG_FALSE;
	SG_uint32  uIndex     = 1234u;
	SG_uint32  uIndex2    = 1234u;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__append__null(pCtx, pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__append__bool(pCtx, pThis, SG_TRUE)  );
	VERIFY_ERR_CHECK(  SG_varray__append__int64(pCtx, pThis, 2)  );
	VERIFY_ERR_CHECK(  SG_varray__append__double(pCtx, pThis, 3.0)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pThis, "value 4")  );

	// verify finding NULL values
	oVariant.type = SG_VARIANT_TYPE_NULL;
	VERIFY_ERR_CHECK(  SG_varray__find(pCtx, pThis, &oVariant, &bContains, &uIndex)  );
	VERIFY_ERR_CHECK(  SG_varray__find__null(pCtx, pThis, &bContains2, &uIndex2)  );
	VERIFY_COND("Expected value not found.", bContains == SG_TRUE);
	VERIFY_COND("Value found at unexpected index.", uIndex == 0u);
	VERIFY_COND("Variant search and explicit search got different results.", bContains == bContains2);
	VERIFY_COND("Variant search and explicit search got different results.", uIndex == uIndex2);

// a macro that verifies that we find an expected value at an expected index
#define CHECK_FOUND(VARIANT_TYPE, VARIANT_VAR, VARIANT_SEARCH, EXPECTED_VALUE, EXPECTED_INDEX)        \
	oVariant.type = VARIANT_TYPE;                                                                      \
	oVariant.v.VARIANT_VAR = EXPECTED_VALUE;                                                           \
	VERIFY_ERR_CHECK(  SG_varray__find(pCtx, pThis, &oVariant, &bContains, &uIndex)  );                \
	VERIFY_ERR_CHECK(  VARIANT_SEARCH(pCtx, pThis, EXPECTED_VALUE, &bContains2, &uIndex2)  );          \
	VERIFY_COND("Expected value not found.", bContains == SG_TRUE);                                    \
	VERIFY_COND("Value found at unexpected index.", uIndex == EXPECTED_INDEX);                         \
	VERIFY_COND("Variant search and explicit search got different results.", bContains == bContains2); \
	VERIFY_COND("Variant search and explicit search got different results.", uIndex == uIndex2);

// a macro that verifies that we don't find a value
#define CHECK_NOT_FOUND(VARIANT_TYPE, VARIANT_VAR, VARIANT_SEARCH, SEARCH_VALUE, ARRAY_SIZE)          \
	oVariant.type = VARIANT_TYPE;                                                                      \
	oVariant.v.VARIANT_VAR = SEARCH_VALUE;                                                             \
	VERIFY_ERR_CHECK(  SG_varray__find(pCtx, pThis, &oVariant, &bContains, &uIndex)  );                \
	VERIFY_ERR_CHECK(  VARIANT_SEARCH(pCtx, pThis, SEARCH_VALUE, &bContains2, &uIndex2)  );            \
	VERIFY_COND("Unexpected value found.", bContains == SG_FALSE);                                     \
	VERIFY_COND("Index not set to varray size when value not found.", uIndex == ARRAY_SIZE);           \
	VERIFY_COND("Variant search and explicit search got different results.", bContains == bContains2); \
	VERIFY_COND("Variant search and explicit search got different results.", uIndex == uIndex2);

// a macro that verifies that we can find one value and not find another value
#define CHECK_FIND(VARIANT_TYPE, VARIANT_VAR, VARIANT_SEARCH, EXPECTED_VALUE, EXPECTED_INDEX, UNEXPECTED_VALUE, ARRAY_SIZE) \
	CHECK_FOUND(VARIANT_TYPE, VARIANT_VAR, VARIANT_SEARCH, EXPECTED_VALUE, EXPECTED_INDEX)                                  \
	CHECK_NOT_FOUND(VARIANT_TYPE, VARIANT_VAR, VARIANT_SEARCH, UNEXPECTED_VALUE, ARRAY_SIZE)

	// verify finding bool, int64, double, and sz values
	CHECK_FIND(SG_VARIANT_TYPE_BOOL, val_bool, SG_varray__find__bool, SG_TRUE, 1u, SG_FALSE, 5u);
	CHECK_FIND(SG_VARIANT_TYPE_INT64, val_int64, SG_varray__find__int64, 2, 2u, 5, 5u);
	CHECK_FIND(SG_VARIANT_TYPE_DOUBLE, val_double, SG_varray__find__double, 3.0, 3u, 5.0, 5u);
	CHECK_FIND(SG_VARIANT_TYPE_SZ, val_sz, SG_varray__find__sz, "value 4", 4u, "value 5", 5u);

#undef CHECK_FOUND
#undef CHECK_NOT_FOUND
#undef CHECK_FIND

fail:
	SG_VARRAY_NULLFREE(pCtx, pThis);
}

static void u0081__find__optargs(SG_context* pCtx)
{
	SG_varray* pThis     = NULL;
	SG_variant oVariant;
	SG_bool    bContains = SG_FALSE;
	SG_uint32  uIndex    = 1234u;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__append__null(pCtx, pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__append__bool(pCtx, pThis, SG_TRUE)  );
	VERIFY_ERR_CHECK(  SG_varray__append__int64(pCtx, pThis, 2)  );
	VERIFY_ERR_CHECK(  SG_varray__append__double(pCtx, pThis, 3.0)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pThis, "value 4")  );

	oVariant.type = SG_VARIANT_TYPE_BOOL;
	oVariant.v.val_bool = SG_TRUE;
	VERIFY_ERR_CHECK(  SG_varray__find(pCtx, pThis, &oVariant, NULL, &uIndex)  );
	VERIFY_COND("Value found at unexpected index.", uIndex == 1u);
	VERIFY_ERR_CHECK(  SG_varray__find(pCtx, pThis, &oVariant, &bContains, NULL)  );
	VERIFY_COND("Expected value not found.", bContains == SG_TRUE);

fail:
	SG_VARRAY_NULLFREE(pCtx, pThis);
}

static void u0081__find__badargs(SG_context* pCtx)
{
	SG_varray* pArray = NULL;
	SG_variant oVariant;
	SG_bool    bBool  = SG_FALSE;
	SG_uint32  uIndex = 0u;

	SG_VARRAY__ALLOC(pCtx, &pArray);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__find(pCtx, NULL,   &oVariant, &bBool, &uIndex), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__find(pCtx, pArray, NULL,      &bBool, &uIndex), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__find(pCtx, pArray, NULL,      NULL,   NULL),    SG_ERR_INVALIDARG  );

	SG_VARRAY_NULLFREE(pCtx, pArray);
}

static void u0081__alloc__copy(SG_context* pCtx)
{
	SG_varray* pThis  = NULL;
	SG_varray* pCopy  = NULL;
	SG_bool    bEqual = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__append__null(pCtx, pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__append__bool(pCtx, pThis, SG_TRUE)  );
	VERIFY_ERR_CHECK(  SG_varray__append__int64(pCtx, pThis, 2)  );
	VERIFY_ERR_CHECK(  SG_varray__append__double(pCtx, pThis, 3.0)  );
	VERIFY_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pThis, "value 4")  );

	VERIFY_ERR_CHECK(  SG_VARRAY__ALLOC__COPY(pCtx, &pCopy, pThis)  );
	VERIFY_ERR_CHECK(  SG_varray__equal(pCtx, pThis, pCopy, &bEqual)  );
	VERIFY_COND("copied array doesn't equal original", bEqual == SG_TRUE);

fail:
	SG_VARRAY_NULLFREE(pCtx, pThis);
	SG_VARRAY_NULLFREE(pCtx, pCopy);
}

static void u0081__alloc__copy__badargs(SG_context* pCtx)
{
	SG_varray* pThis = NULL;
	SG_varray* pCopy = NULL;

	SG_VARRAY__ALLOC(pCtx, &pThis);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__alloc__copy(pCtx, NULL,   pThis), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_varray__alloc__copy(pCtx, &pCopy, NULL),  SG_ERR_INVALIDARG  );
	VERIFY_COND("copying NULL array created output", pCopy == NULL);

	SG_VARRAY_NULLFREE(pCtx, pThis);
	SG_VARRAY_NULLFREE(pCtx, pCopy);
}

TEST_MAIN(u0081_varray)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0081__remove_only(pCtx)  );
	BEGIN_TEST(  u0081__remove_first(pCtx)  );
	BEGIN_TEST(  u0081__remove_last(pCtx)  );
	BEGIN_TEST(  u0081__remove_middle(pCtx)  );
	BEGIN_TEST(  u0081__remove_all(pCtx)  );
	BEGIN_TEST(  u0081__remove_past_end(pCtx)  );
	BEGIN_TEST(  u0081__remove_last_twice(pCtx)  );
	BEGIN_TEST(  u0081__appendcopy__varray(pCtx)  );
	BEGIN_TEST(  u0081__appendcopy__varray__badargs(pCtx)  );
	BEGIN_TEST(  u0081__appendcopy__vhash(pCtx)  );
	BEGIN_TEST(  u0081__appendcopy__vhash__badargs(pCtx)  );
	BEGIN_TEST(  u0081__appendcopy__variant(pCtx)  );
	BEGIN_TEST(  u0081__appendcopy__variant__badargs(pCtx)  );
	BEGIN_TEST(  u0081__find(pCtx)  );
	BEGIN_TEST(  u0081__find__optargs(pCtx)  );
	BEGIN_TEST(  u0081__find__badargs(pCtx)  );
	BEGIN_TEST(  u0081__alloc__copy(pCtx)  );
	BEGIN_TEST(  u0081__alloc__copy__badargs(pCtx)  );

	TEMPLATE_MAIN_END;
}

#undef AA
#undef MY_VERIFY_IN_SECTION
#undef MY_VERIFY_MOVED_FROM
#undef MY_VERIFY_NAMES
#undef MY_VERIFY_REPO_PATH
#undef MY_VERIFY__IN_SECTION__REPO_PATH
