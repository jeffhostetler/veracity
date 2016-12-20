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

// testsuite/unittests/u0005_string.c
// test basic string manipulation using SG_string.
//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "unittests.h"

//////////////////////////////////////////////////////////////////

void u0005_string__test_string_1(SG_context * pCtx)
{
	SG_string * p = NULL;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&p)  );
	VERIFY_COND( "test_simple_alloc", (SG_string__length_in_bytes(p) == 0) );

	VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,p,"Hello")  );
	VERIFY_COND( "test_simple_append", (SG_string__length_in_bytes(p) == 5) );
	VERIFY_COND( "test_simple_append", (strcmp(SG_string__sz(p),"Hello") == 0) );

	VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx, p," There")  );
	VERIFY_COND( "test_simple_append", (SG_string__length_in_bytes(p) == 11) );
	VERIFY_COND( "test_simple_append", (strcmp(SG_string__sz(p),"Hello There") == 0) );

	VERIFY_ERR_CHECK(  SG_string__insert__sz(pCtx,p,6,"XXX ")  );
	VERIFY_COND( "test_simple_insert", (SG_string__length_in_bytes(p) == 15) );
	VERIFY_COND( "test_simple_insert", (strcmp(SG_string__sz(p),"Hello XXX There") == 0) );

	VERIFY_ERR_CHECK(  SG_string__insert__sz(pCtx,p,0,"ZZZ ")  );
	VERIFY_COND( "test_simple_insert", (SG_string__length_in_bytes(p) == 19) );
	VERIFY_COND( "test_simple_insert", (strcmp(SG_string__sz(p),"ZZZ Hello XXX There") == 0) );

	VERIFY_ERR_CHECK(  SG_string__remove(pCtx,p,10,4)  );
	VERIFY_COND( "test_simple_delete", (SG_string__length_in_bytes(p) == 15) );
	VERIFY_COND( "test_simple_delete", (strcmp(SG_string__sz(p),"ZZZ Hello There") == 0) );

	VERIFY_ERR_CHECK(  SG_string__remove(pCtx,p,0,4)  );
	VERIFY_COND( "test_simple_delete", (SG_string__length_in_bytes(p) == 11) );
	VERIFY_COND( "test_simple_delete", (strcmp(SG_string__sz(p),"Hello There") == 0) );

fail:
	SG_STRING_NULLFREE(pCtx, p);
}

void u0005_string__test_string_2(SG_context * pCtx)
{
	SG_string * p1 = NULL;
	SG_string * p2 = NULL;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "Hello World!")  );
	VERIFY_COND("test alloc sz", (SG_string__length_in_bytes(p1) == 12));

	VERIFY_ERR_CHECK(  SG_string__insert__buf_len(pCtx,p1,6,(const SG_byte *)"XXX ZZZ",4)  );
	VERIFY_COND("test insert buf", (SG_string__length_in_bytes(p1) == 16));
	VERIFY_COND("test insert buf", (strcmp(SG_string__sz(p1),"Hello XXX World!") == 0));

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &p2, p1)  );
	VERIFY_COND("test copy string", (p2 != NULL));
	VERIFY_COND("test copy string", (SG_string__length_in_bytes(p2) == 16));
	VERIFY_COND("test copy string", (strcmp(SG_string__sz(p1),SG_string__sz(p2)) == 0));

fail:
	SG_STRING_NULLFREE(pCtx, p1);
	SG_STRING_NULLFREE(pCtx, p2);
}

void u0005_string__test_string_3(SG_context * pCtx)
{
	SG_string * p1 = NULL;
	const char * pb = NULL;
	SG_uint32 k,len;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "ZZ")  );
	len = 2;

	// insert enough stuff into middle of string to force the realloc code to happen.
	// we don't know the chunk size, so we don't know exactly when this will happen,
	// so we keep inserting until the underlying buffer pointer changes.
	// we do this in a loop so that we get 4 reallocs.

	for (k=0; k<4; k++)
	{
		pb = SG_string__sz(p1);
		VERIFY_COND("test_string_3", (pb != NULL));

		while (SG_string__sz(p1) == pb)
		{
			VERIFY_ERR_CHECK(  SG_string__insert__sz(pCtx,p1,1,"abcdefghij")  );
			len += 10;

			//VERIFY_COND("insert abcdefghij", (SG_string__length_in_bytes(p1) == len));
		}

		INFOP("insert abcdefghij", ("realloc happened at length %d",len));
	}

	// verify that that string looks like what we built.

	VERIFY_COND("insert abcdefghij", (SG_string__length_in_bytes(p1) == len));
	VERIFY_COND("insert abcdefghij", (SG_string__length_in_bytes(p1) == strlen(SG_string__sz(p1))));

	pb = SG_string__sz(p1);
	VERIFY_COND("test_string_3", (pb != NULL));
	VERIFY_COND("test_string_3", (pb[0] == 'Z'));
	for (k=1; k<len-1; k+=10)
		VERIFY_COND("test_string_3", (strncmp(&pb[k],"abcdefghij",10) == 0));
	VERIFY_COND("test_string_3", (pb[len-1] == 'Z'));

fail:
	SG_STRING_NULLFREE(pCtx, p1);
}

void u0005_string__test_replace_with_advance(SG_context * pCtx)
{
	SG_string * p1 = NULL;
	SG_uint32 nrFound = 0;

	SG_byte aBytesZZ[] = { 'Z', 'Z' };
	SG_byte aBytesYY[] = { 'Y', 'Y' };
	SG_byte aBytesYYZZ[] = { 'Y', 'Y', 'Z', 'Z' };
	SG_byte aBytesX[] = { 'X' };

	// test replacing the 'ZZ' with 'YY' in steps.  and we begin and end
	// string with the pattern to be replaced.
	// this also test replacing with same length buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "ZZxxZZxxZZxxZZ")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 1));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxZZxxZZxxZZ") == 0));

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,2,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxYYxxYYxxZZ") == 0));

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 1));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxYYxxYYxxYY") == 0));

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 0));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxYYxxYYxxYY") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replacing 'ZZ' where we end string in middle of pattern

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxAAxxAAxxZ")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 0));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxAAxxAAxxZ") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replacing 'ZZ' where we don't begin or end string with pattern.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxAAxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 1));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxAAxxYYxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace 'ZZ' with 'YYZZ' to ensure that we only scan the original content.
	// this also tests replacing with longer buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxZZxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYYZZ,4,2,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxYYZZxxYYZZxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace 'ZZ' with 'X' to test replacing with shorter buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxZZxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesX,1,SG_UINT32_MAX,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxXxxXxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace 'ZZ' with NULL to test replacing with shorter buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxZZxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,NULL,0,2,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxxxxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace when we don't have anything in the string.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &p1)  );
	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesX,1,SG_UINT32_MAX,SG_TRUE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 0));
	VERIFY_COND("test_replace", (SG_string__length_in_bytes(p1) == 0));

fail:
	SG_STRING_NULLFREE(pCtx, p1);
}

void u0005_string__test_replace_without_advance(SG_context * pCtx)
{
	SG_string * p1 = NULL;
	SG_uint32 nrFound = 0;

	SG_byte aBytesZZ[] = { 'Z', 'Z' };
	SG_byte aBytesYY[] = { 'Y', 'Y' };
	SG_byte aBytesX[] = { 'X' };
	SG_byte aBytesSDS[] = { '/', '.', '/' };
	SG_byte aBytesS[] = { '/' };
	SG_byte aBytesLong[] = { 'L', 'o', 'n', 'g' };
	SG_byte aBytesLonger[] = { 'L', 'o', 'n', 'g', 'e', 'r' };

	// test replacing the 'ZZ' with 'YY' in steps.  and we begin and end
	// string with the pattern to be replaced.
	// this also test replacing with same length buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "ZZxxZZxxZZxxZZ")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 1));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxZZxxZZxxZZ") == 0));

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,2,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxYYxxYYxxZZ") == 0));

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 1));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxYYxxYYxxYY") == 0));

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 0));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"YYxxYYxxYYxxYY") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replacing 'ZZ' where we end string in middle of pattern

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxAAxxAAxxZ")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 0));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxAAxxAAxxZ") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replacing 'ZZ' where we don't begin or end string with pattern.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxAAxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesYY,2,1,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 1));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxAAxxYYxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace '/./' with '/' to ensure that we scan the replaced content.
	// this also tests replacing with longer buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxx/./xx/./././xxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesSDS,3,aBytesS,1,SG_UINT32_MAX,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 4));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxx/xx/xxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace 'ZZ' with 'X' to test replacing with shorter buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxZZxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesX,1,SG_UINT32_MAX,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxXxxXxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace 'ZZ' with NULL to test replacing with shorter buffer.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "AAxxZZxxZZxxAA")  );

	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,NULL,0,2,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1),"AAxxxxxxAA") == 0));

	SG_STRING_NULLFREE(pCtx, p1);

	// test replace when we don't have anything in the string.

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&p1)  );
	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesZZ,2,aBytesX,1,SG_UINT32_MAX,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 0));
	VERIFY_COND("test_replace", (SG_string__length_in_bytes(p1) == 0));

	// be careful when replacing with a longer string which is prefixed by the original text
	SG_STRING_NULLFREE(pCtx, p1);

	nrFound = 0;
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &p1, "Long becomes Longer")  );
	VERIFY_ERR_CHECK(  SG_string__replace_bytes(pCtx,p1,aBytesLong,sizeof(aBytesLong),aBytesLonger,sizeof(aBytesLonger),5,SG_FALSE,&nrFound)  );
	VERIFY_COND("test_replace", (nrFound == 2));
	VERIFY_COND("test_replace", (strcmp(SG_string__sz(p1), "Longer becomes Longerer") == 0));

fail:
	SG_STRING_NULLFREE(pCtx, p1);
}

void u0005_string__test_overlap(SG_context * pCtx)
{
	// verify that we correctly detect and fail when given overlapping strings

	SG_string * pString = NULL;
	const char * sz = NULL;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );

	VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pString,"abcdefghijklmnopqrstuvwxyz")  );

	// these should fail.

	sz = SG_string__sz(pString);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__set__sz(pCtx,pString,sz),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__set__sz(pCtx,pString,sz+10),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__set__sz(pCtx,pString,sz+25),SG_ERR_OVERLAPPINGBUFFERS);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__set__buf_len(pCtx,pString,(SG_byte *)sz,(SG_uint32)1),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__set__buf_len(pCtx,pString,(SG_byte *)sz-1,(SG_uint32)2),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__set__buf_len(pCtx,pString,(SG_byte *)sz+25,(SG_uint32)1),SG_ERR_OVERLAPPINGBUFFERS);

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__append__string(pCtx,pString,pString),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__append__sz(pCtx,pString,sz),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__append__sz(pCtx,pString,sz+10),SG_ERR_OVERLAPPINGBUFFERS);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_string__append__buf_len(pCtx,pString,(SG_byte *)sz,(SG_uint32)10),SG_ERR_OVERLAPPINGBUFFERS);

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

void u0005_string__test_sprintf(SG_context * pCtx)
{
	SG_string * pString = NULL;
	char * szExpected;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );

	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx,pString,"Format %s %d","abc",5)  );
	VERIFYP_COND("sprintf",(strcmp(SG_string__sz(pString),"Format abc 5") == 0),
			("sprintf result[%s] expected[%s]",SG_string__sz(pString),"Format abc 5"));

	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx,pString,
							 "%s %s %s %s %s %s %s %s %s %s",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz",
							 "abcdefghijklmnopqrstuvwxyz")  );
	szExpected = ("abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" " "
				  "abcdefghijklmnopqrstuvwxyz" );
	VERIFYP_COND("sprintf",(strcmp(SG_string__sz(pString),szExpected) == 0),
			("sprintf result[%s] expected[%s]",SG_string__sz(pString),szExpected));

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

void u0005_string__test_nogrow(SG_context * pCtx)
{
	const SG_uint32 TEST_SIZE = 10;

	SG_string * pString = NULL;
	SG_uint32 avail;
	SG_uint32 i;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pString, TEST_SIZE)  );
	VERIFY_COND("initial reserved length incorrect", TEST_SIZE == SG_string__bytes_allocated(pString));

	avail = SG_string__bytes_avail_before_grow(pString);
	VERIFY_COND("non-sensical bytes available reported", avail <= TEST_SIZE - 1);

	for (i = 0; i < avail; i++)
	{
		VERIFY_ERR_CHECK(  SG_string__append__sz__no_grow(pCtx, pString, "X")  );
	}

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_string__append__sz__no_grow(pCtx, pString, "X")  ,  SG_ERR_BUFFERTOOSMALL  );

	VERIFY_COND("final reserved length incorrect", TEST_SIZE == SG_string__bytes_allocated(pString));

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}


/*	SG_string__split__sz_asciichar should not return more than the
 *  max-requested segments
 */
void u0005_string__test_safe_split(SG_context * pCtx)
{
	const SG_uint32 MAX_WANTED = 5;
	const char * strWith10Parts = "1/2/3/4/5/6/7/8/9/10";
	const char sep = '/';
	SG_uint32 partsReturned = 0;
	char **parts = NULL;

	VERIFY_ERR_CHECK(  SG_string__split__sz_asciichar(pCtx, strWith10Parts, sep, MAX_WANTED, &parts, &partsReturned)  );

	VERIFY_COND("returned more parts than requested", MAX_WANTED == partsReturned);

fail:
	SG_STRINGLIST_NULLFREE(pCtx, parts, MAX_WANTED);
}

typedef struct u0005__case__find__sz
{
	const char* szString;
	SG_uint32   uStart;
	SG_bool     bRange;
	SG_bool     bError;
	const char* szSubstring;
	SG_uint32   uIndex;
}
u0005__case__find__sz;

static u0005__case__find__sz u0005__tests__find__sz[] =
{
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, "This",              0u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, "is",                2u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, " is ",              4u },
	{ "This is a string.",  2u, SG_TRUE,  SG_FALSE, "is",                2u },
	{ "This is a string.",  3u, SG_TRUE,  SG_FALSE, "is",                5u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, "a",                 8u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, "string",           10u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, "s",                 3u },
	{ "This is a string.",  3u, SG_TRUE,  SG_FALSE, "s",                 3u },
	{ "This is a string.",  4u, SG_TRUE,  SG_FALSE, "s",                 6u },
	{ "This is a string.",  7u, SG_TRUE,  SG_FALSE, "s",                10u },
	{ "This is a string.", 11u, SG_TRUE,  SG_FALSE, "s",      SG_UINT32_MAX },
	{ "This is a string.", 17u, SG_TRUE,  SG_TRUE,  "s",                 0u },
	{ "This is a string.", 20u, SG_TRUE,  SG_TRUE,  "s",                 0u },

	{ "This is a string.",  0u, SG_FALSE, SG_FALSE, "This",              0u },
	{ "This is a string.",  0u, SG_FALSE, SG_FALSE, "is",                2u },
	{ "This is a string.",  0u, SG_FALSE, SG_FALSE, " is ",              4u },
	{ "This is a string.",  2u, SG_FALSE, SG_FALSE, "is",                2u },
	{ "This is a string.",  3u, SG_FALSE, SG_FALSE, "is",                5u },
	{ "This is a string.",  0u, SG_FALSE, SG_FALSE, "a",                 8u },
	{ "This is a string.",  0u, SG_FALSE, SG_FALSE, "string",           10u },
	{ "This is a string.",  0u, SG_FALSE, SG_FALSE, "s",                 3u },
	{ "This is a string.",  3u, SG_FALSE, SG_FALSE, "s",                 3u },
	{ "This is a string.",  4u, SG_FALSE, SG_FALSE, "s",                 6u },
	{ "This is a string.",  7u, SG_FALSE, SG_FALSE, "s",                10u },
	{ "This is a string.", 11u, SG_FALSE, SG_FALSE, "s",      SG_UINT32_MAX },
	{ "This is a string.", 17u, SG_FALSE, SG_FALSE, "s",      SG_UINT32_MAX },
	{ "This is a string.", 20u, SG_FALSE, SG_FALSE, "s",      SG_UINT32_MAX },
};

void u0005_string__test__find__sz(SG_context* pCtx)
{
	SG_uint32  uIndex  = 0u;
	SG_string* sString = NULL;

	for (uIndex = 0u; uIndex < SG_NrElements(u0005__tests__find__sz); ++uIndex)
	{
		const u0005__case__find__sz* pTest   = u0005__tests__find__sz + uIndex;
		SG_uint32                    uResult = 0u;

		VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sString, pTest->szString)  );
		if (pTest->bRange == SG_TRUE && pTest->bError == SG_TRUE)
		{
			SG_string__find__sz(pCtx, sString, pTest->uStart, pTest->bRange, pTest->szSubstring, &uResult);
			VERIFYP_CTX_ERR_EQUALS("SG_string__find__sz", pCtx, SG_ERR_INDEXOUTOFRANGE, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Substring(%s) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szSubstring, pTest->uIndex, uResult));
			SG_context__err_reset(pCtx);
		}
		else
		{
			VERIFY_ERR_CHECK(  SG_string__find__sz(pCtx, sString, pTest->uStart, pTest->bRange, pTest->szSubstring, &uResult)  );
			VERIFYP_COND("SG_string__find__sz", uResult == pTest->uIndex, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Substring(%s) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szSubstring, pTest->uIndex, uResult));
		}
		SG_STRING_NULLFREE(pCtx, sString);
	}

fail:
	SG_STRING_NULLFREE(pCtx, sString);
}

void u0005_string__test__find__string(SG_context* pCtx)
{
	SG_uint32  uIndex     = 0u;
	SG_string* sString    = NULL;
	SG_string* sSubstring = NULL;

	for (uIndex = 0u; uIndex < SG_NrElements(u0005__tests__find__sz); ++uIndex)
	{
		const u0005__case__find__sz* pTest   = u0005__tests__find__sz + uIndex;
		SG_uint32                    uResult = 0u;

		VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sString, pTest->szString)  );
		VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sSubstring, pTest->szSubstring)  );
		if (pTest->bRange == SG_TRUE && pTest->bError == SG_TRUE)
		{
			SG_string__find__string(pCtx, sString, pTest->uStart, pTest->bRange, sSubstring, &uResult);
			VERIFYP_CTX_ERR_EQUALS("SG_string__find__string", pCtx, SG_ERR_INDEXOUTOFRANGE, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Substring(%s) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szSubstring, pTest->uIndex, uResult));
			SG_context__err_reset(pCtx);
		}
		else
		{
			VERIFY_ERR_CHECK(  SG_string__find__string(pCtx, sString, pTest->uStart, pTest->bRange, sSubstring, &uResult)  );
			VERIFYP_COND("SG_string__find__string", uResult == pTest->uIndex, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Substring(%s) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szSubstring, pTest->uIndex, uResult));
		}
		SG_STRING_NULLFREE(pCtx, sString);
		SG_STRING_NULLFREE(pCtx, sSubstring);
	}

fail:
	SG_STRING_NULLFREE(pCtx, sString);
	SG_STRING_NULLFREE(pCtx, sSubstring);
}

typedef struct u0005__case__find__char
{
	const char* szString;
	SG_uint32   uStart;
	SG_bool     bRange;
	SG_bool     bError;
	char        scChar;
	SG_uint32   uIndex;
}
u0005__case__find__char;

static u0005__case__find__char u0005__tests__find__char[] =
{
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'T',            0u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'h',            1u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'i',            2u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 's',            3u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, ' ',            4u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'a',            8u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 't',           11u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'r',           12u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'n',           14u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, 'g',           15u },
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, '.',           16u },

	{ "This is a string.",  2u, SG_TRUE,  SG_FALSE, 'i',            2u },
	{ "This is a string.",  3u, SG_TRUE,  SG_FALSE, 'i',            5u },
	{ "This is a string.",  5u, SG_TRUE,  SG_FALSE, 'i',            5u },
	{ "This is a string.",  6u, SG_TRUE,  SG_FALSE, 'i',           13u },
	{ "This is a string.", 13u, SG_TRUE,  SG_FALSE, 'i',           13u },
	{ "This is a string.", 14u, SG_TRUE,  SG_FALSE, 'i', SG_UINT32_MAX },

	{ "This is a string.",  3u, SG_TRUE,  SG_FALSE, 's',            3u },
	{ "This is a string.",  4u, SG_TRUE,  SG_FALSE, 's',            6u },
	{ "This is a string.",  6u, SG_TRUE,  SG_FALSE, 's',            6u },
	{ "This is a string.",  7u, SG_TRUE,  SG_FALSE, 's',           10u },
	{ "This is a string.", 10u, SG_TRUE,  SG_FALSE, 's',           10u },
	{ "This is a string.", 11u, SG_TRUE,  SG_FALSE, 's', SG_UINT32_MAX },

	{ "This is a string.",  4u, SG_TRUE,  SG_FALSE, ' ',            4u },
	{ "This is a string.",  5u, SG_TRUE,  SG_FALSE, ' ',            7u },
	{ "This is a string.",  7u, SG_TRUE,  SG_FALSE, ' ',            7u },
	{ "This is a string.",  8u, SG_TRUE,  SG_FALSE, ' ',            9u },
	{ "This is a string.",  9u, SG_TRUE,  SG_FALSE, ' ',            9u },
	{ "This is a string.", 10u, SG_TRUE,  SG_FALSE, ' ', SG_UINT32_MAX },

	{ "This is a string.", 17u, SG_TRUE,  SG_TRUE,  ' ',            0u },
	{ "This is a string.", 20u, SG_TRUE,  SG_TRUE,  ' ',            0u },
	{ "This is a string.", 17u, SG_FALSE, SG_FALSE, ' ', SG_UINT32_MAX },
	{ "This is a string.", 20u, SG_FALSE, SG_FALSE, ' ', SG_UINT32_MAX },
};

void u0005_string__test__find__char(SG_context* pCtx)
{
	SG_uint32  uIndex  = 0u;
	SG_string* sString = NULL;

	for (uIndex = 0u; uIndex < SG_NrElements(u0005__tests__find__char); ++uIndex)
	{
		const u0005__case__find__char* pTest   = u0005__tests__find__char + uIndex;
		SG_uint32                      uResult = 0u;

		VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sString, pTest->szString)  );
		if (pTest->bRange == SG_TRUE && pTest->bError == SG_TRUE)
		{
			SG_string__find__char(pCtx, sString, pTest->uStart, pTest->bRange, pTest->scChar, &uResult);
			VERIFYP_CTX_ERR_EQUALS("SG_string__find__char", pCtx, SG_ERR_INDEXOUTOFRANGE, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Char(%c) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->scChar, pTest->uIndex, uResult));
			SG_context__err_reset(pCtx);
		}
		else
		{
			VERIFY_ERR_CHECK(  SG_string__find__char(pCtx, sString, pTest->uStart, pTest->bRange, pTest->scChar, &uResult)  );
			VERIFYP_COND("SG_string__find__char", uResult == pTest->uIndex, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Char(%c) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->scChar, pTest->uIndex, uResult));
		}
		SG_STRING_NULLFREE(pCtx, sString);
	}

fail:
	SG_STRING_NULLFREE(pCtx, sString);
}

typedef struct u0005__case__find_any__char
{
	const char* szString;
	SG_uint32   uStart;
	SG_bool     bRange;
	SG_bool     bError;
	const char* szChars;
	SG_uint32   uIndex;
	char        scChar;
}
u0005__case__find_any__char;

static u0005__case__find_any__char u0005__tests__find_any__char[] =
{
	{ "This is a string.",  0u, SG_TRUE,  SG_FALSE, "is ",            2u, 'i' },
	{ "This is a string.",  2u, SG_TRUE,  SG_FALSE, "is ",            2u, 'i' },
	{ "This is a string.",  3u, SG_TRUE,  SG_FALSE, "is ",            3u, 's' },
	{ "This is a string.",  4u, SG_TRUE,  SG_FALSE, "is ",            4u, ' ' },
	{ "This is a string.",  5u, SG_TRUE,  SG_FALSE, "is ",            5u, 'i' },
	{ "This is a string.",  6u, SG_TRUE,  SG_FALSE, "is ",            6u, 's' },
	{ "This is a string.",  7u, SG_TRUE,  SG_FALSE, "is ",            7u, ' ' },
	{ "This is a string.",  8u, SG_TRUE,  SG_FALSE, "is ",            9u, ' ' },
	{ "This is a string.",  9u, SG_TRUE,  SG_FALSE, "is ",            9u, ' ' },
	{ "This is a string.", 10u, SG_TRUE,  SG_FALSE, "is ",           10u, 's' },
	{ "This is a string.", 11u, SG_TRUE,  SG_FALSE, "is ",           13u, 'i' },
	{ "This is a string.", 13u, SG_TRUE,  SG_FALSE, "is ",           13u, 'i' },
	{ "This is a string.", 14u, SG_TRUE,  SG_FALSE, "is ", SG_UINT32_MAX,  0  },

	{ "This is a string.", 17u, SG_TRUE,  SG_TRUE,  "is ",             0,  0  },
	{ "This is a string.", 20u, SG_TRUE,  SG_TRUE,  "is ",             0,  0  },
	{ "This is a string.", 17u, SG_FALSE, SG_FALSE, "is ", SG_UINT32_MAX,  0  },
	{ "This is a string.", 20u, SG_FALSE, SG_FALSE, "is ", SG_UINT32_MAX,  0  },
};

void u0005_string__test__find_any__char(SG_context* pCtx)
{
	SG_uint32  uIndex  = 0u;
	SG_string* sString = NULL;

	for (uIndex = 0u; uIndex < SG_NrElements(u0005__tests__find_any__char); ++uIndex)
	{
		const u0005__case__find_any__char* pTest   = u0005__tests__find_any__char + uIndex;
		SG_uint32                          uResult = 0u;
		char                               scChar  = 0;

		VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sString, pTest->szString)  );
		if (pTest->bRange == SG_TRUE && pTest->bError == SG_TRUE)
		{
			SG_string__find_any__char(pCtx, sString, pTest->uStart, pTest->bRange, pTest->szChars, &uResult, &scChar);
			VERIFYP_CTX_ERR_EQUALS("SG_string__find_any__char: expected error", pCtx, SG_ERR_INDEXOUTOFRANGE, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Chars(%s) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szChars, pTest->uIndex, uResult));
			SG_context__err_reset(pCtx);
		}
		else
		{
			VERIFY_ERR_CHECK(  SG_string__find_any__char(pCtx, sString, pTest->uStart, pTest->bRange, pTest->szChars, &uResult, &scChar)  );
			VERIFYP_COND("SG_string__find_any__char: index mismatch", uResult == pTest->uIndex, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Chars(%s) ExpectedFoundIndex(%u) ActualFoundIndex(%u)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szChars, pTest->uIndex, uResult));
			if (uResult != SG_UINT32_MAX)
			{
				VERIFYP_COND("SG_string__find_any__char: char mismatch", scChar == pTest->scChar, ("TestIndex(%u) String(%s) Start(%u) Range(%d) Error(%d) Chars(%s) ExpectedFoundChar(%c) ActualFoundChar(%c)", uIndex, pTest->szString, pTest->uStart, pTest->bRange, pTest->bError, pTest->szChars, pTest->scChar, scChar));
			}
		}
		SG_STRING_NULLFREE(pCtx, sString);
	}

fail:
	SG_STRING_NULLFREE(pCtx, sString);
}

TEST_MAIN(u0005_string)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0005_string__test_string_1(pCtx)  );
	BEGIN_TEST(  u0005_string__test_string_2(pCtx)  );
	BEGIN_TEST(  u0005_string__test_string_3(pCtx)  );
	BEGIN_TEST(  u0005_string__test_replace_with_advance(pCtx)  );
	BEGIN_TEST(  u0005_string__test_replace_without_advance(pCtx)  );
	BEGIN_TEST(  u0005_string__test_overlap(pCtx)  );
	BEGIN_TEST(  u0005_string__test_sprintf(pCtx)  );
	BEGIN_TEST(  u0005_string__test_nogrow(pCtx)  );
	BEGIN_TEST(  u0005_string__test_safe_split(pCtx)  );
	BEGIN_TEST(  u0005_string__test__find__sz(pCtx)  );
	BEGIN_TEST(  u0005_string__test__find__string(pCtx)  );
	BEGIN_TEST(  u0005_string__test__find__char(pCtx)  );
	BEGIN_TEST(  u0005_string__test__find_any__char(pCtx)  );

	TEMPLATE_MAIN_END;
}
