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
 * @file u0108_bitvector.c
 *
 * @details tests for SG_bitvector
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"

//////////////////////////////////////////////////////////////////

static void _u0108__set_bits_from_string(SG_context * pCtx,
										 SG_bitvector * pBitVector,
										 const char * psz)
{
	SG_uint32 kLimit = SG_STRLEN(psz);
	SG_uint32 k;
	SG_uint32 lenInUse;
	char * psz_debug = NULL;

	VERIFY_ERR_CHECK(  SG_bitvector__zero(pCtx, pBitVector)  );
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("len", (lenInUse == 0));

	for (k=0; k<kLimit; k++)
	{
		VERIFY_ERR_CHECK(  SG_bitvector__set_bit(pCtx,
												 pBitVector,
												 k,
												 (psz[k] == '1'))  );
	}

	for (k=0; k<kLimit; k++)
	{
		SG_bool b_k;

		VERIFY_ERR_CHECK(  SG_bitvector__get_bit(pCtx,
												 pBitVector,
												 k,
												 SG_FALSE,
												 &b_k)  );
		VERIFY_COND("bit", (b_k == (psz[k] == '1')));
	}

	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("lenInUse", (lenInUse == kLimit));

	VERIFY_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pBitVector, &psz_debug)  );
	VERIFY_COND("debug", (strcmp(psz_debug, psz) == 0));

	//INFOP("set_bits",("resulting bitvector '%s'", psz_debug));

fail:
	SG_NULLFREE(pCtx, psz_debug);
}

static void _u0108__append_bits_from_string(SG_context * pCtx,
											SG_bitvector * pBitVector,
											const char * psz)
{
	SG_uint32 kLimit = SG_STRLEN(psz);
	SG_uint32 k;
	SG_uint32 lenInUse;
	char * psz_debug = NULL;

	VERIFY_ERR_CHECK(  SG_bitvector__zero(pCtx, pBitVector)  );
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("lenInUse", (lenInUse == 0));

	for (k=0; k<kLimit; k++)
	{
		VERIFY_ERR_CHECK(  SG_bitvector__append_bit(pCtx,
													pBitVector,
													(psz[k] == '1'),
													NULL)  );
	}

	for (k=0; k<kLimit; k++)
	{
		SG_bool b_k;

		VERIFY_ERR_CHECK(  SG_bitvector__get_bit(pCtx,
												 pBitVector,
												 k,
												 SG_FALSE,
												 &b_k)  );
		VERIFY_COND("bit", (b_k == (psz[k] == '1')));
	}

	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("lenInUse", (lenInUse == kLimit));

	VERIFY_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pBitVector, &psz_debug)  );
	VERIFY_COND("debug", (strcmp(psz_debug, psz) == 0));

	//INFOP("append_bits",("resulting bitvector '%s'", psz_debug));

fail:
	SG_NULLFREE(pCtx, psz_debug);
}

static void _u0108__test_from_string(SG_context * pCtx,
									 SG_uint32 kInitialSize,
									 const char * pszPattern)
{
	SG_bitvector * pBitVector = NULL;
	SG_uint32 lenInUse;
	SG_bool b;

	INFOP("test_from_string", ("[initial size %d][pattern '%s']", kInitialSize, pszPattern));

	// pre-allocate suggested space in vector.
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector, kInitialSize)  );

	// but that does not actually cause any bits to be in use.
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("lenInUse", (lenInUse == 0));

	// set various random bits in vector (causing it to grow as necessary).
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector, pszPattern)  );

	// try again using append.
	VERIFY_ERR_CHECK(  _u0108__append_bits_from_string(pCtx, pBitVector, pszPattern)  );

	// get a random bit way past the end of the defined vector
	// verify that it silently supplies the value suggested.

	VERIFY_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pBitVector, 1000000, SG_FALSE, &b)  );
	VERIFY_COND("get_past_end", (b == SG_FALSE));
	VERIFY_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pBitVector, 1000000, SG_TRUE, &b)  );
	VERIFY_COND("get_past_end", (b == SG_TRUE));

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector);
}

static void _u0108__test_alloc_copy(SG_context * pCtx,
									SG_uint32 kInitialSize,
									const char * pszPattern)
{
	SG_bitvector * pBitVector = NULL;
	SG_bitvector * pBitVector_copy = NULL;
	char * psz_debug_copy = NULL;
	SG_uint32 lenInUse;
	SG_uint32 lenInUse_copy;

	INFOP("test_alloc_copy", ("[initial size %d][pattern '%s']", kInitialSize, pszPattern));

	// pre-allocate suggested space in vector.
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector, kInitialSize)  );

	// but that does not actually cause any bits to be in use.
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("lenInUse", (lenInUse == 0));

	// set various random bits in vector (causing it to grow as necessary).
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector, pszPattern)  );

	// allocate a copy of the bitvector.
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC__COPY(pCtx, &pBitVector_copy, pBitVector)  );
	
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector,      &lenInUse     )  );
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector_copy, &lenInUse_copy)  );
	VERIFY_COND("lenInUse_copy", (lenInUse_copy == lenInUse));

	VERIFY_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pBitVector_copy, &psz_debug_copy)  );
	VERIFY_COND("debug", (strcmp(psz_debug_copy, pszPattern) == 0));

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector);
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_copy);
	SG_NULLFREE(pCtx, psz_debug_copy);
}

static void _u0108__test_assign(SG_context * pCtx,
								SG_uint32 kInitialSize,
								const char * pszPattern)
{
	SG_bitvector * pBitVector = NULL;
	SG_bitvector * pBitVector_copy = NULL;
	char * psz_debug_copy = NULL;
	SG_uint32 lenInUse;
	SG_uint32 lenInUse_copy;

	INFOP("test_assign", ("[initial size %d][pattern '%s']", kInitialSize, pszPattern));

	// pre-allocate suggested space in vector.
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector, kInitialSize)  );

	// but that does not actually cause any bits to be in use.
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector, &lenInUse)  );
	VERIFY_COND("lenInUse", (lenInUse == 0));

	// set various random bits in vector (causing it to grow as necessary).
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector, pszPattern)  );

	// allocate an empty bitvector.
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_copy, 0)  );

	// do basic bitvector assignment:  lvalue = rvalue
	VERIFY_ERR_CHECK(  SG_bitvector__assign__bv__eq__bv(pCtx, pBitVector_copy, pBitVector)  );
	
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector,      &lenInUse     )  );
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector_copy, &lenInUse_copy)  );
	VERIFY_COND("lenInUse_copy", (lenInUse_copy == lenInUse));

	VERIFY_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pBitVector_copy, &psz_debug_copy)  );
	VERIFY_COND("debug", (strcmp(psz_debug_copy, pszPattern) == 0));

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector);
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_copy);
	SG_NULLFREE(pCtx, psz_debug_copy);
}

static void _u0108__test_assign_or_eq(SG_context * pCtx,
									  SG_uint32 kInitialSize,
									  const char * pszPattern_1,
									  const char * pszPattern_2)
{
	SG_bitvector * pBitVector_1 = NULL;
	SG_bitvector * pBitVector_2 = NULL;
	char * psz_debug_after = NULL;
	SG_uint32 strlen_1 = SG_STRLEN(pszPattern_1);
	SG_uint32 strlen_2 = SG_STRLEN(pszPattern_2);
	SG_uint32 lenInUse_after;
	SG_uint32 expected_length;
	SG_uint32 k;

	INFOP("test_assign_or_eq", ("[initial size %d][pattern_1 '%s'][pattern_2 '%s']", kInitialSize, pszPattern_1, pszPattern_2));

	// allocate lvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_1, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_1, pszPattern_1)  );

	// allocate rvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_2, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_2, pszPattern_2)  );

	// do:  lvalue |= rvalue

	VERIFY_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pBitVector_1, pBitVector_2)  );
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector_1, &lenInUse_after)  );

	expected_length = SG_MAX( strlen_1, strlen_2 );
	VERIFYP_COND("lenInUse_after", (lenInUse_after == expected_length), ("[lenInUse_after %d][expected_length %d][strlen_1 %d][strlen_2 %d]", lenInUse_after, expected_length, strlen_1, strlen_2));

	VERIFY_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pBitVector_1, &psz_debug_after)  );
	//INFOP("or_eq",("resulting bitvector '%s'", psz_debug_after));

	for (k=0; k<lenInUse_after; k++)
	{
		SG_bool b1_before, b1_after, b2;

		if (k < strlen_1)
			b1_before = (pszPattern_1[k] == '1');
		else
			b1_before = SG_FALSE;

		if (k < strlen_2)
			b2 = (pszPattern_2[k] == '1');
		else
			b2 = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pBitVector_1, k, SG_FALSE, &b1_after)  );

		VERIFYP_COND("l |= r", (b1_after == (b1_before | b2)), ("bit[%d] %d != (%d | %d)",
																k, b1_after, b1_before, b2));
	}

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_1);
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_2);
	SG_NULLFREE(pCtx, psz_debug_after);
}

static void _u0108__test_assign_and_eq_not(SG_context * pCtx,
										   SG_uint32 kInitialSize,
										   const char * pszPattern_1,
										   const char * pszPattern_2)
{
	SG_bitvector * pBitVector_1 = NULL;
	SG_bitvector * pBitVector_2 = NULL;
	char * psz_debug_after = NULL;
	SG_uint32 strlen_1 = SG_STRLEN(pszPattern_1);
	SG_uint32 strlen_2 = SG_STRLEN(pszPattern_2);
	SG_uint32 lenInUse_after;
	SG_uint32 expected_length;
	SG_uint32 k;

	INFOP("test_assign_and_eq_not", ("[initial size %d][pattern_1 '%s'][pattern_2 '%s']", kInitialSize, pszPattern_1, pszPattern_2));

	// allocate lvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_1, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_1, pszPattern_1)  );

	// allocate rvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_2, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_2, pszPattern_2)  );

	// do:  lvalue &= ~rvalue

	VERIFY_ERR_CHECK(  SG_bitvector__assign__bv__and_eq__not__bv(pCtx, pBitVector_1, pBitVector_2)  );
	VERIFY_ERR_CHECK(  SG_bitvector__length(pCtx, pBitVector_1, &lenInUse_after)  );

	expected_length = SG_MAX( strlen_1, strlen_2 );
	VERIFYP_COND("lenInUse_after", (lenInUse_after == expected_length), ("[lenInUse_after %d][expected_length %d][strlen_1 %d][strlen_2 %d]", lenInUse_after, expected_length, strlen_1, strlen_2));

	VERIFY_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pBitVector_1, &psz_debug_after)  );
	//INFOP("and_eq_not",("resulting bitvector '%s'", psz_debug_after));

	for (k=0; k<lenInUse_after; k++)
	{
		SG_bool b1_before, b1_after, b2;

		if (k < strlen_1)
			b1_before = (pszPattern_1[k] == '1');
		else
			b1_before = SG_FALSE;

		if (k < strlen_2)
			b2 = (pszPattern_2[k] == '1');
		else
			b2 = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pBitVector_1, k, SG_FALSE, &b1_after)  );

		VERIFYP_COND("l |= r", (b1_after == (b1_before & ~b2)), ("bit[%d] %d != (%d & ~%d)",
																k, b1_after, b1_before, b2));
	}

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_1);
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_2);
	SG_NULLFREE(pCtx, psz_debug_after);
}

//////////////////////////////////////////////////////////////////

static void _u0108__test_operator_eq_eq(SG_context * pCtx,
										SG_uint32 kInitialSize,
										const char * pszPattern_1,
										const char * pszPattern_2)
{
	SG_bitvector * pBitVector_1 = NULL;
	SG_bitvector * pBitVector_2 = NULL;
	SG_uint32 strlen_1 = SG_STRLEN(pszPattern_1);
	SG_uint32 strlen_2 = SG_STRLEN(pszPattern_2);
	SG_uint32 max_length = SG_MAX( strlen_1, strlen_2 );
	SG_uint32 k;
	SG_bool bEqual;
	SG_bool bTestEqual;

	INFOP("test_operator_eq_eq", ("[initial size %d][pattern_1 '%s'][pattern_2 '%s']", kInitialSize, pszPattern_1, pszPattern_2));

	// allocate lvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_1, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_1, pszPattern_1)  );

	// allocate rvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_2, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_2, pszPattern_2)  );

	// do:  bEqual = (lvalue == rvalue);

	VERIFY_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__bv(pCtx, pBitVector_1, pBitVector_2, &bEqual)  );

	bTestEqual = SG_TRUE;
	for (k=0; k<max_length; k++)
	{
		SG_bool b1, b2;

		if (k < strlen_1)
			b1 = (pszPattern_1[k] == '1');
		else
			b1 = SG_FALSE;

		if (k < strlen_2)
			b2 = (pszPattern_2[k] == '1');
		else
			b2 = SG_FALSE;

		if (b1 != b2)
		{
			bTestEqual = SG_FALSE;
			break;
		}
	}

	VERIFYP_COND("l == r", (bEqual == bTestEqual), ("[bEqual %d][bTestEqual %d]", bEqual, bTestEqual));

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_1);
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_2);
}

//////////////////////////////////////////////////////////////////

static void _u0108__test_operator_eq_eq_zero(SG_context * pCtx,
											 SG_uint32 kInitialSize,
											 const char * pszPattern_1)
{
	SG_bitvector * pBitVector_1 = NULL;
	SG_uint32 strlen_1 = SG_STRLEN(pszPattern_1);
	SG_uint32 k;
	SG_bool bEqual;
	SG_bool bTestEqual;

	INFOP("test_operator_eq_eq_zero", ("[initial size %d][pattern_1 '%s']", kInitialSize, pszPattern_1));

	// allocate lvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_1, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_1, pszPattern_1)  );

	// do:  bEqual = (lvalue == '0...');

	VERIFY_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__zero(pCtx, pBitVector_1, &bEqual)  );

	bTestEqual = SG_TRUE;
	for (k=0; k<strlen_1; k++)
	{
		SG_bool b1 = (pszPattern_1[k] == '1');

		if (b1)
		{
			bTestEqual = SG_FALSE;
			break;
		}
	}

	VERIFYP_COND("l == 0", (bEqual == bTestEqual), ("[bEqual %d][bTestEqual %d]", bEqual, bTestEqual));

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_1);
}

//////////////////////////////////////////////////////////////////

static void _u0108__test_operator_and_eq_eq_zero(SG_context * pCtx,
												 SG_uint32 kInitialSize,
												 const char * pszPattern_1,
												 const char * pszPattern_2)
{
	SG_bitvector * pBitVector_1 = NULL;
	SG_bitvector * pBitVector_2 = NULL;
	SG_uint32 strlen_1 = SG_STRLEN(pszPattern_1);
	SG_uint32 strlen_2 = SG_STRLEN(pszPattern_2);
	SG_uint32 max_length = SG_MAX( strlen_1, strlen_2 );
	SG_uint32 k;
	SG_bool bEqual;
	SG_bool bTestEqual;

	INFOP("test_operator_and_eq_eq_zero", ("[initial size %d][pattern_1 '%s'][pattern_2 '%s']", kInitialSize, pszPattern_1, pszPattern_2));

	// allocate lvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_1, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_1, pszPattern_1)  );

	// allocate rvalue
	VERIFY_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pBitVector_2, kInitialSize)  );
	VERIFY_ERR_CHECK(  _u0108__set_bits_from_string(pCtx, pBitVector_2, pszPattern_2)  );

	// do:  bEqual = ((lvalue & rvalue) == '0...');

	VERIFY_ERR_CHECK(  SG_bitvector__operator__bv__and__bv__eq_eq__zero(pCtx, pBitVector_1, pBitVector_2, &bEqual)  );

	bTestEqual = SG_TRUE;
	for (k=0; k<max_length; k++)
	{
		SG_bool b1, b2;

		if (k < strlen_1)
			b1 = (pszPattern_1[k] == '1');
		else
			b1 = SG_FALSE;

		if (k < strlen_2)
			b2 = (pszPattern_2[k] == '1');
		else
			b2 = SG_FALSE;

		if ((b1 & b2) != SG_FALSE)
		{
			bTestEqual = SG_FALSE;
			break;
		}
	}

	VERIFYP_COND("((l & r) == 0)", (bEqual == bTestEqual), ("[bEqual %d][bTestEqual %d]", bEqual, bTestEqual));

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_1);
	SG_BITVECTOR_NULLFREE(pCtx, pBitVector_2);
}

//////////////////////////////////////////////////////////////////

void u0108_bitvector__basic(SG_context * pCtx, SG_uint32 kInitialSize)
{
	// A somewhat random 64 bit pattern
#define B64 "1010101010101010" "0000000000000000" "1111000011110000" "0000111100001111"

	char * apsz[] = { ("1"),
					  ("01"),
					  ("1001"),
					  ("0"),
					  ("000000000"),
					  (B64),		// a 64-bit string
					  (B64 "1"),	// a 65-bit string
					  (B64 B64),	// a 128-bit string
					  (B64 B64 "0"),
					  ("1" B64),
					  ("0" B64),
					  (B64 B64 B64 B64 B64 B64 B64 B64 B64 B64)
					};
	SG_uint32 kLimit = SG_NrElements(apsz);
	SG_uint32 k, j;

	for (k=0; k<kLimit; k++)
	{
		VERIFY_ERR_CHECK(  _u0108__test_from_string(pCtx, kInitialSize, apsz[k])  );
		VERIFY_ERR_CHECK(  _u0108__test_alloc_copy(pCtx, kInitialSize, apsz[k])  );
		VERIFY_ERR_CHECK(  _u0108__test_assign(pCtx, kInitialSize, apsz[k])  );
		VERIFY_ERR_CHECK(  _u0108__test_operator_eq_eq_zero(pCtx, kInitialSize, apsz[k])  );

		for (j=0; j<kLimit; j++)
		{
			VERIFY_ERR_CHECK(  _u0108__test_assign_or_eq(pCtx, kInitialSize, apsz[k], apsz[j])  );
			VERIFY_ERR_CHECK(  _u0108__test_assign_and_eq_not(pCtx, kInitialSize, apsz[k], apsz[j])  );
			VERIFY_ERR_CHECK(  _u0108__test_operator_eq_eq(pCtx, kInitialSize, apsz[k], apsz[j])  );
			VERIFY_ERR_CHECK(  _u0108__test_operator_and_eq_eq_zero(pCtx, kInitialSize, apsz[k], apsz[j])  );
		}
	}

fail:
	;
}

//////////////////////////////////////////////////////////////////

TEST_MAIN(u0108_bitvector)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0108_bitvector__basic(pCtx, 0)  );
//	BEGIN_TEST(  u0108_bitvector__basic(pCtx, 1)  );

	TEMPLATE_MAIN_END;
}

