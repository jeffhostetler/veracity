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
#include "unittests.h"

//////////////////////////////////////////////////////////////////
// we define a little trick here to prefix all global symbols (type,
// structures, functions) with our test name.  this allows all of
// the tests in the suite to be #include'd into one meta-test (without
// name collisions) when we do a GCOV run.

#define MyMain()				TEST_MAIN(u0062_str_utils)
#define MyDcl(name)				u0062_str_utils__##name
#define MyFn(name)				u0062_str_utils__##name


// Note u008_misc_utils also contains some tests of functions in sg_str_utils, since those functions used to be in sg_misc_utils.


typedef struct
{
	const char* szLeft;             //< Left string to compare.
	const char* szRight;            //< Right string to compare.
	SG_int32    iSensitiveResult;   //< Expected result for a case-sensitive comparison.
	SG_int32    iInsensitiveResult; //< Expected result for a case-insensitive comparison.
	// Note: the only thing that matters with the result variables is if they're >0, <0, or ==0.
}
MyDcl(test__strcmp__case);

static MyDcl(test__strcmp__case) MyDcl(test__strcmp__cases)[] = {
	{ "abc",  "abc",   0,  0 },
	{ "ABC",  "ABC",   0,  0 },
	{ "ABCD", "ABC",   1,  1 },
	{ "ABC",  "ABCD", -1, -1 },
	{ "ABC",  "abc",  -1,  0 },
	{ "abc",  "ABC",   1,  0 },
	{ "ABC",  "abz",  -1, -1 },
	{ "abc",  "ABZ",   1, -1 },
	{ "ABZ",  "abc",  -1,  1 },
	{ "abz",  "ABC",   1,  1 },
	{ "ABC",  "abcd", -1, -1 },
	{ "abc",  "ABCD",  1, -1 },
	{ "ABCD", "abc",  -1,  1 },
	{ "abcd", "ABC",   1,  1 },
	{ "abc",  NULL,    1,  1 },
	{ NULL,   "abc",  -1, -1 },
	{ NULL,   NULL,    0,  0 },
};

void MyFn(test__strcmp)(SG_context* pCtx)
{
	SG_uint32 uIndex = 0u;

	SG_UNUSED(pCtx);

	for (uIndex = 0u; uIndex < SG_NrElements(MyDcl(test__strcmp__cases)); ++uIndex)
	{
		const MyDcl(test__strcmp__case)* pCase   = MyDcl(test__strcmp__cases) + uIndex;
		SG_int32                         iResult = 0;

		if (pCase->szLeft == NULL || pCase->szRight == NULL)
		{
			continue;
		}

		iResult = strcmp(pCase->szLeft, pCase->szRight);
		if (pCase->iSensitiveResult < 0)
		{
			VERIFYP_COND("test__strcmp: Expected negative result", iResult < 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iSensitiveResult));
		}
		else if (pCase->iSensitiveResult > 0)
		{
			VERIFYP_COND("test__strcmp: Expected positive result", iResult > 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iSensitiveResult));
		}
		else
		{
			VERIFYP_COND("test__strcmp: Expected zero result", iResult == 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iSensitiveResult));
		}

		iResult = SG_stricmp(pCase->szLeft, pCase->szRight);
		if (pCase->iInsensitiveResult < 0)
		{
			VERIFYP_COND("test__strcmp: Expected negative result", iResult < 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iInsensitiveResult));
		}
		else if (pCase->iInsensitiveResult > 0)
		{
			VERIFYP_COND("test__strcmp: Expected positive result", iResult > 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iInsensitiveResult));
		}
		else
		{
			VERIFYP_COND("test__strcmp: Expected zero result", iResult == 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iInsensitiveResult));
		}
	}
}

void MyFn(test__strcmp__null)(SG_context* pCtx)
{
	SG_uint32 uIndex = 0u;

	SG_UNUSED(pCtx);

	for (uIndex = 0u; uIndex < SG_NrElements(MyDcl(test__strcmp__cases)); ++uIndex)
	{
		const MyDcl(test__strcmp__case)* pCase   = MyDcl(test__strcmp__cases) + uIndex;
		SG_int32                         iResult = 0;

		iResult = SG_strcmp__null(pCase->szLeft, pCase->szRight);
		if (pCase->iSensitiveResult < 0)
		{
			VERIFYP_COND("test__strcmp: Expected negative result", iResult < 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iSensitiveResult));
		}
		else if (pCase->iSensitiveResult > 0)
		{
			VERIFYP_COND("test__strcmp: Expected positive result", iResult > 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iSensitiveResult));
		}
		else
		{
			VERIFYP_COND("test__strcmp: Expected zero result", iResult == 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iSensitiveResult));
		}

		iResult = SG_stricmp__null(pCase->szLeft, pCase->szRight);
		if (pCase->iInsensitiveResult < 0)
		{
			VERIFYP_COND("test__strcmp: Expected negative result", iResult < 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iInsensitiveResult));
		}
		else if (pCase->iInsensitiveResult > 0)
		{
			VERIFYP_COND("test__strcmp: Expected positive result", iResult > 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iInsensitiveResult));
		}
		else
		{
			VERIFYP_COND("test__strcmp: Expected zero result", iResult == 0, ("TestIndex(%d) Left(%s) Right(%s) Result(%d) Expected(%d)", uIndex, pCase->szLeft, pCase->szRight, iResult, pCase->iInsensitiveResult));
		}
	}
}

void MyFn(test__substring)(SG_context * pCtx)
{
	char * sz = NULL;

	VERIFY_ERR_CHECK(  SG_ascii__substring(pCtx, "abcd", 1, 2, &sz)  );
	VERIFY_COND(  "substring"  ,  strcmp(sz,"bc")==0  );

fail:
	SG_NULLFREE(pCtx, sz );
}

void MyFn(test__substring_to_end)(SG_context * pCtx)
{
	char * sz = NULL;

	VERIFY_ERR_CHECK(  SG_ascii__substring__to_end(pCtx, "abcd", 2, &sz)  );
	VERIFY_COND(  "substring_to_end"  ,  strcmp(sz,"cd")==0  );

fail:
	SG_NULLFREE(pCtx, sz );
}

void MyFn(test__int64_parse)(SG_context * pCtx)
{
	SG_int64 i = 0;

	VERIFY_ERR_CHECK_DISCARD(  SG_int64__parse(pCtx, &i, "1234")  );
	VERIFY_COND( "int64_parse"  ,  i==1234 );

	VERIFY_ERR_CHECK_DISCARD(  SG_int64__parse(pCtx, &i, "-1234")  );
	VERIFY_COND( "int64_parse"  ,  i==-1234 );

	//todo: test how invalid input is handled...
}

void MyFn(test__double_parse)(SG_context * pCtx)
{
	double x = 0;

	VERIFY_ERR_CHECK_DISCARD(  SG_double__parse(pCtx, &x, "1234")  );
	VERIFY_COND( "double_parse"  ,  x==1234 );

	VERIFY_ERR_CHECK_DISCARD(  SG_double__parse(pCtx, &x, "-1234")  );
	VERIFY_COND( "double_parse"  ,  x==-1234 );

	VERIFY_ERR_CHECK_DISCARD(  SG_double__parse(pCtx, &x, "0.0")  );
	VERIFY_COND( "double_parse"  ,  x==0 );

	VERIFY_ERR_CHECK_DISCARD(  SG_double__parse(pCtx, &x, "-1.5e27")  );
	VERIFY_COND( "double_parse"  ,  x==-1.5e27 );

	VERIFY_ERR_CHECK_DISCARD(  SG_double__parse(pCtx, &x, "1.5e-27")  );
	VERIFY_COND( "double_parse"  ,  x==1.5e-27 );

	//todo: test how invalid input is handled...
}

void MyFn(test__int64_to_string)(SG_UNUSED_PARAM(SG_context * pCtx))
{
	SG_int_to_string_buffer s;
	SG_UNUSED(pCtx);

	VERIFY_COND("", strcmp("1234",SG_int64_to_sz(1234,s))==0 );
	VERIFY_COND("", strcmp("-1234",SG_int64_to_sz(-1234,s))==0 );
	VERIFY_COND("", strcmp("0",SG_int64_to_sz(0,s))==0 );
	VERIFY_COND("", strcmp("9223372036854775807",SG_int64_to_sz(SG_INT64_MAX,s))==0 );
	VERIFY_COND("", strcmp("-9223372036854775808",SG_int64_to_sz(SG_INT64_MIN,s))==0 );
	VERIFY_COND( "" , SG_int64_to_sz(-1,NULL)==NULL );

	VERIFY_COND("", strcmp("1234",SG_uint64_to_sz(1234,s))==0 );
	VERIFY_COND("", strcmp("0",SG_uint64_to_sz(0,s))==0 );
	VERIFY_COND("", strcmp("18446744073709551615",SG_uint64_to_sz(SG_UINT64_MAX,s))==0 );
	VERIFY_COND("", SG_uint64_to_sz(12345678,NULL)==NULL );

	VERIFY_COND( "" , strcmp("0000000000001234",SG_uint64_to_sz__hex(0x1234,s))==0 );
	VERIFY_COND( "" , strcmp("0000000000000000",SG_uint64_to_sz__hex(0,s))==0 );
	VERIFY_COND( "" , strcmp("ffffffffffffffff",SG_uint64_to_sz__hex(SG_UINT64_MAX,s))==0 );
}

void MyFn(test__sprintf_truncate)(SG_context * pCtx)
{
	char buf[25];

	VERIFY_ERR_CHECK(  SG_sprintf_truncate(pCtx, buf, sizeof(buf),
		"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890")  );
	VERIFY_COND("simple truncated buffer contents incorrect", 0==strcmp(buf, "123456789012345678901234"));

	VERIFY_ERR_CHECK(  SG_sprintf_truncate(pCtx, buf, sizeof(buf),
		"%d %d %d", 2147483647,2147483647,2147483647)  );
	VERIFY_COND("formatted, truncated buffer contents incorrect", 0==strcmp(buf, "2147483647 2147483647 21"));

fail:
	return;
}

typedef struct
{
	SG_uint32   uDestinationSize; //< Size of the destination buffer to use.
	const char* szSourceString;   //< String to copy into the destination buffer.
	SG_uint32   uSourceCount;     //< Number of characters to copy from the string.
	SG_error    uExpectedError;   //< Expected error code.
}
MyDcl(test__strncpy__case);

static MyDcl(test__strncpy__case) MyDcl(test__strncpy__cases)[] = {
	{ 11u, "test string", 11u, SG_ERR_BUFFERTOOSMALL }, // count == source, dest == count
	{ 12u, "test string", 11u, SG_ERR_OK },             // count == source, dest == count + 1
	{  5u, "test string", 11u, SG_ERR_BUFFERTOOSMALL }, // count == source, dest <  count
	{ 20u, "test string", 11u, SG_ERR_OK },             // count == source, dest >  count
	{  5u, "test string",  5u, SG_ERR_BUFFERTOOSMALL }, // count <  source, dest == count
	{  6u, "test string",  5u, SG_ERR_OK },             // count <  source, dest == count + 1
	{  2u, "test string",  5u, SG_ERR_BUFFERTOOSMALL }, // count <  source, dest <  count
	{ 20u, "test string",  5u, SG_ERR_OK },             // count <  source, dest >  count
	{ 20u, "test string", 20u, SG_ERR_OK },             // count >  source, dest == count
	{ 21u, "test string", 20u, SG_ERR_OK },             // count >  source, dest == count + 1
	{ 11u, "test string", 20u, SG_ERR_BUFFERTOOSMALL }, // count >  source, dest == source
	{ 12u, "test string", 20u, SG_ERR_OK },             // count >  source, dest == source + 1
	{ 10u, "test string", 20u, SG_ERR_BUFFERTOOSMALL }, // count >  source, dest <  count, dest <  source
	{ 15u, "test string", 20u, SG_ERR_OK },             // count >  source, dest <  count, dest >  source
	{ 30u, "test string", 20u, SG_ERR_OK },             // count >  source, dest >  count
};

void MyFn(test__strncpy__run_test_cases)(SG_context* pCtx)
{
	char*     szDestinationString = NULL;
	SG_uint32 uIndex              = 0u;

	for (uIndex = 0u; uIndex < SG_NrElements(MyDcl(test__strncpy__cases)); ++uIndex)
	{
		const MyDcl(test__strncpy__case)* pCase         = MyDcl(test__strncpy__cases) + uIndex;
		SG_uint32                         uOutputLength = 0u;

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			uIndex = uIndex;
		}

		// calculate the expected output length
		uOutputLength = SG_MIN(SG_STRLEN(pCase->szSourceString), pCase->uSourceCount);
		uOutputLength = SG_MIN(uOutputLength, pCase->uDestinationSize - 1);

		// setup the destination buffer
		VERIFYP_COND("strncpy__run_test_cases: szDest is non-NULL at start of test", szDestinationString == NULL, ("index(%d)", uIndex));
		VERIFY_ERR_CHECK(  SG_alloc(pCtx, sizeof(char), pCase->uDestinationSize, &szDestinationString)  );
		memset(szDestinationString, 0, pCase->uDestinationSize);

		// run the strncpy
		VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_strncpy(pCtx, szDestinationString, pCase->uDestinationSize, pCase->szSourceString, pCase->uSourceCount), pCase->uExpectedError  );

		// make sure the expected number of characters were copied
		VERIFYP_COND("strncpy__run_test_cases: destination string length incorrect", SG_STRLEN(szDestinationString) == uOutputLength, ("index(%d)", uIndex));

		// make sure the correct characters were copied
		// note: characters are copied even if the destination buffer is too small,
		//       the string is simply truncated to fit
		VERIFYP_COND("strncpy__run_test_cases: destination string incorrect", strncmp(pCase->szSourceString, szDestinationString, uOutputLength) == 0, ("index(%d)", uIndex));

		// free the destination buffer
		VERIFY_ERR_CHECK(  SG_free(pCtx, szDestinationString)  );
		szDestinationString = NULL;
	}

fail:
	SG_NULLFREE(pCtx, szDestinationString);
}

void MyFn(test__strncpy__badargs)(SG_context* pCtx)
{
	static const char*     szString = "test string";
	static const SG_uint32 uSize    = 15u;           // > strlen(szString)
	static const SG_uint32 uCount   = 5u;            // < strlen(szString)

	char szBuffer[15u];

	// pass various invalid arguments
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_strncpy(pCtx, NULL,     uSize, szString, uCount), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_strncpy(pCtx, szBuffer, 0u,    szString, uCount), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_strncpy(pCtx, szBuffer, uSize, NULL,     uCount), SG_ERR_INVALIDARG  );

	// make sure copying zero characters results in an empty destination
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szString)  );
	VERIFY_ERR_CHECK(  SG_strncpy(pCtx, szBuffer, uSize, szString, 0u)  );
	VERIFY_COND("strncpy__badargs: destination has non-zero length after copying blank string", SG_STRLEN(szBuffer) == 0u);

fail:
	return;
}

void MyFn(test__sz__trim)(SG_context* pCtx)
{        
     SG_uint32 len = 0;
     char* sz_trimmed_val = NULL;

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
     VERIFY_ERR_CHECK( SG_sz__trim(pCtx, "foo", &len, &sz_trimmed_val)  );    
     VERIFY_COND("trimmed string incorrect", 0==strcmp(sz_trimmed_val, "foo"));
     VERIFY_COND("trimmed string length incorrect", len == 3);
     SG_NULLFREE(pCtx, sz_trimmed_val);

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
     VERIFY_ERR_CHECK( SG_sz__trim(pCtx, "foo  ", &len, &sz_trimmed_val)  );    
     VERIFY_COND("trimmed trailing string incorrect", 0==strcmp(sz_trimmed_val, "foo"));
     VERIFY_COND("trimmed trailing string length incorrect", len == 3);
     SG_NULLFREE(pCtx, sz_trimmed_val);

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
     VERIFY_ERR_CHECK( SG_sz__trim(pCtx, "   foo", &len, &sz_trimmed_val)  );    
     VERIFY_COND("trimmed leading string incorrect", 0==strcmp(sz_trimmed_val, "foo"));
     VERIFY_COND("trimmed leading string length incorrect", len == 3);
     SG_NULLFREE(pCtx, sz_trimmed_val);

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
     VERIFY_ERR_CHECK( SG_sz__trim(pCtx, "   foo   ", &len, &sz_trimmed_val)  );    
     VERIFY_COND("trimmed leading and trailing string incorrect", 0==strcmp(sz_trimmed_val, "foo"));
     VERIFY_COND("trimmed leading and trailing string length incorrect", len == 3);
     SG_NULLFREE(pCtx, sz_trimmed_val);

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
     VERIFY_ERR_CHECK( SG_sz__trim(pCtx, "  ", &len, &sz_trimmed_val)  );    
     VERIFY_COND("trimmed leading and trailing string incorrect", sz_trimmed_val == NULL);
     VERIFY_COND("trimmed leading and trailing string length incorrect", len == 0);
     SG_NULLFREE(pCtx, sz_trimmed_val);   

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
	 VERIFY_ERR_CHECK( SG_sz__trim(pCtx, "", &len, &sz_trimmed_val)  );
	 VERIFY_COND("trimmed of empty string yields null", sz_trimmed_val==NULL);
	 VERIFY_COND("trimmed of empty string yields zero length", len==0);
	 sz_trimmed_val = NULL;

	 len = 0xdeadbeef;
	 sz_trimmed_val = (char *)((size_t)0xdeadbeef);
	 VERIFY_ERR_CHECK( SG_sz__trim(pCtx, NULL, &len, &sz_trimmed_val)  );
	 VERIFY_COND("trimmed of null string yields null", sz_trimmed_val==NULL);
	 VERIFY_COND("trimmed of null string yields zero length", len==0);
	 sz_trimmed_val = NULL;

fail:
     SG_NULLFREE(pCtx, sz_trimmed_val);
     return;

}

MyMain()
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  MyFn(test__strcmp)(pCtx)  );
	BEGIN_TEST(  MyFn(test__strcmp__null)(pCtx)  );
	BEGIN_TEST(  MyFn(test__substring)(pCtx)  );
	BEGIN_TEST(  MyFn(test__substring_to_end)(pCtx)  );
	BEGIN_TEST(  MyFn(test__int64_parse)(pCtx)  );
	BEGIN_TEST(  MyFn(test__double_parse)(pCtx)  );
	BEGIN_TEST(  MyFn(test__int64_to_string)(pCtx)  );
	BEGIN_TEST(  MyFn(test__sprintf_truncate)(pCtx)  );
	BEGIN_TEST(  MyFn(test__strncpy__run_test_cases)(pCtx)  );
	BEGIN_TEST(  MyFn(test__strncpy__badargs)(pCtx)  );
    BEGIN_TEST(  MyFn(test__sz__trim)(pCtx)  );

	TEMPLATE_MAIN_END;
}

#undef MyMain
#undef MyDcl
#undef MyFn
