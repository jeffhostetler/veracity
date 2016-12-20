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

#define MyMain()				TEST_MAIN(u0107_hdb)
#define MyDcl(name)				u0107_hdb__##name
#define MyFn(name)				u0107_hdb__##name

#define u0107_LEFT32(p,n,s) (((SG_uint32) ((p)[n])) << (s)) 

#define u0107_UNPACK_32(i32,ptr) SG_STATEMENT(SG_byte*q=(SG_byte*)ptr;i32=\
                                 u0107_LEFT32(q,3,24)  \
                               | u0107_LEFT32(q,2,16)  \
                               | u0107_LEFT32(q,1,8)  \
                               | u0107_LEFT32(q,0,0);  \
                             )

void MyFn(test_multiple)(SG_context *pCtx)
{
	char buf[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPath1 = NULL;
    SG_hdb* pdb = NULL;
    SG_uint32 i = 0;
    SG_uint32 key_length = 8;
    SG_byte k[8];

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf, sizeof(buf), 10)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, buf)  );
    VERIFY_ERR_CHECK(  SG_hdb__create(pCtx, pPath1, (SG_uint8) key_length, 18, 4)  );
    printf("%s\n", SG_pathname__sz(pPath1));

    VERIFY_ERR_CHECK(  SG_hdb__open(pCtx, pPath1, 3, SG_TRUE, &pdb)  );

    SG_random_bytes(k, sizeof(k));

    i = 13;
    VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__ERROR)  );
    i = 16;
    VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__MULTIPLE)  );
    i = 21;
    VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__MULTIPLE)  );

    {
        SG_byte* p1 = NULL;
        SG_byte* p2 = NULL;
        SG_byte* p3 = NULL;
        SG_uint32 off1 = 0;
        SG_uint32 off2 = 0;
        SG_uint32 off3 = 0;

        VERIFY_ERR_CHECK(  SG_hdb__find(pCtx, pdb, k, 0, &p1, &off1)  );
        VERIFY_ERR_CHECK(  SG_hdb__find(pCtx, pdb, k, off1, &p2, &off2)  );
        VERIFY_ERR_CHECK(  SG_hdb__find(pCtx, pdb, k, off2, &p3, &off3)  );

        VERIFY_COND("", p1 != NULL);
        VERIFY_COND("", p2 != NULL);
        VERIFY_COND("", p3 != NULL);

        u0107_UNPACK_32(i, p1);
        VERIFY_COND("", 21 == i);

        u0107_UNPACK_32(i, p2);
        VERIFY_COND("", 16 == i);

        u0107_UNPACK_32(i, p3);
        VERIFY_COND("", 13 == i);

    }

    VERIFY_ERR_CHECK(  SG_hdb__close_free(pCtx, pdb)  );
    pdb = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath1);
}

void MyFn(test_simple)(SG_context *pCtx)
{
	char buf[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPath1 = NULL;
    SG_hdb* pdb = NULL;
    SG_uint32 count = 100000;
    SG_uint32 i = 0;
    SG_uint32 key_length = 8;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf, sizeof(buf), 10)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, buf)  );
    VERIFY_ERR_CHECK(  SG_hdb__create(pCtx, pPath1, (SG_uint8) key_length, 18, 4)  );
    printf("%s\n", SG_pathname__sz(pPath1));

    VERIFY_ERR_CHECK(  SG_hdb__open(pCtx, pPath1, count, SG_TRUE, &pdb)  );
    for (i=0; i<count; i++)
    {
        SG_byte k[8];

        SG_random_bytes(k, sizeof(k));

        VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__ERROR)  );
    }
    VERIFY_ERR_CHECK(  SG_hdb__close_free(pCtx, pdb)  );
    pdb = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath1);
}

void MyFn(test_underguess)(SG_context *pCtx)
{
	char buf[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPath1 = NULL;
    SG_hdb* pdb = NULL;
    SG_uint32 count = 1000;
    SG_uint32 i = 0;
    SG_uint32 key_length = 8;
    SG_vhash* pvh1 = NULL;
    SG_string* pstr1 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf, sizeof(buf), 10)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, buf)  );
    VERIFY_ERR_CHECK(  SG_hdb__create(pCtx, pPath1, (SG_uint8) key_length, 18, 4)  );
    printf("%s\n", SG_pathname__sz(pPath1));

    VERIFY_ERR_CHECK(  SG_hdb__open(pCtx, pPath1, count, SG_TRUE, &pdb)  );
    for (i=0; i<2*count; i++)
    {
        SG_byte k[8];

        SG_random_bytes(k, sizeof(k));

        VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__ERROR)  );
    }
    VERIFY_ERR_CHECK(  SG_hdb__close_free(pCtx, pdb)  );
    pdb = NULL;

    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath1, 
                SG_HDB_TO_VHASH__HEADER | SG_HDB_TO_VHASH__STATS,
                &pvh1)  );
    SG_STRING__ALLOC(pCtx, &pstr1); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh1, pstr1); 
    printf("%s\n", SG_string__sz(pstr1)); 
    SG_VHASH_NULLFREE(pCtx, pvh1);
    SG_STRING_NULLFREE(pCtx, pstr1);

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath1);
}

void MyFn(test_overguess)(SG_context *pCtx)
{
	char buf[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPath1 = NULL;
    SG_hdb* pdb = NULL;
    SG_uint32 count = 1000;
    SG_uint32 i = 0;
    SG_uint32 key_length = 8;
    SG_vhash* pvh1 = NULL;
    SG_string* pstr1 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf, sizeof(buf), 10)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, buf)  );
    VERIFY_ERR_CHECK(  SG_hdb__create(pCtx, pPath1, (SG_uint8) key_length, 18, 4)  );
    printf("%s\n", SG_pathname__sz(pPath1));

    VERIFY_ERR_CHECK(  SG_hdb__open(pCtx, pPath1, count, SG_TRUE, &pdb)  );
    for (i=0; i<count/2; i++)
    {
        SG_byte k[8];

        SG_random_bytes(k, sizeof(k));

        VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__ERROR)  );
    }
    VERIFY_ERR_CHECK(  SG_hdb__close_free(pCtx, pdb)  );
    pdb = NULL;

    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath1, 
                SG_HDB_TO_VHASH__HEADER | SG_HDB_TO_VHASH__STATS,
                &pvh1)  );
    SG_STRING__ALLOC(pCtx, &pstr1); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh1, pstr1); 
    printf("%s\n", SG_string__sz(pstr1)); 
    SG_VHASH_NULLFREE(pCtx, pvh1);
    SG_STRING_NULLFREE(pCtx, pstr1);

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath1);
}

void MyFn(test_complicated)(SG_context *pCtx)
{
	char buf[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPath1 = NULL;
	SG_pathname* pPath2 = NULL;
    SG_hdb* pdb = NULL;
    SG_uint32 count = 100000;
    SG_uint32 i = 0;
    SG_uint32 key_length = 8;
    SG_vhash* pvh1 = NULL;
    SG_vhash* pvh2 = NULL;
    SG_int64 t1, t2;
    SG_string* pstr1 = NULL;
    SG_string* pstr2 = NULL;
    SG_string* pstr3 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf, sizeof(buf), 10)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, buf)  );
    VERIFY_ERR_CHECK(  SG_hdb__create(pCtx, pPath1, (SG_uint8) key_length, 16, 4)  );
    printf("%s\n", SG_pathname__sz(pPath1));

    VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t1)  );
    VERIFY_ERR_CHECK(  SG_hdb__open(pCtx, pPath1, count, SG_FALSE, &pdb)  );
    for (i=0; i<count; i++)
    {
        SG_byte k[8];

        SG_random_bytes(k, sizeof(k));

        VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__ERROR)  );
#if 0
        {
        SG_byte* p = NULL;
        int cmp;
        VERIFY_ERR_CHECK(  SG_hdb__find(pCtx, pdb, k, &p)  );
        cmp = memcmp(p, (SG_byte*) &i, 4);
        VERIFY_COND("", 0 == cmp);
        }
#endif
    }
    VERIFY_ERR_CHECK(  SG_hdb__close_free(pCtx, pdb)  );
    pdb = NULL;
    VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t2)  );

    printf("%d ms\n", (int)(t2 - t1));

    // stats
    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath1, 
                SG_HDB_TO_VHASH__HEADER | SG_HDB_TO_VHASH__STATS,
                &pvh1)  );
    SG_STRING__ALLOC(pCtx, &pstr1); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh1, pstr1); 
    printf("%s\n", SG_string__sz(pstr1)); 
    SG_VHASH_NULLFREE(pCtx, pvh1);
    SG_STRING_NULLFREE(pCtx, pstr1);

    // rehash to table 2 with 18 bits
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf, sizeof(buf), 10)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath2, buf)  );
    VERIFY_ERR_CHECK(  SG_hdb__rehash(pCtx, pPath1, 18, pPath2)  );
    printf("%s\n", SG_pathname__sz(pPath2));

    // stats
    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath2, 
                SG_HDB_TO_VHASH__HEADER | SG_HDB_TO_VHASH__STATS,
                &pvh2)  );
    SG_STRING__ALLOC(pCtx, &pstr2); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh2, pstr2); 
    printf("%s\n", SG_string__sz(pstr2)); 
    SG_VHASH_NULLFREE(pCtx, pvh2);
    SG_STRING_NULLFREE(pCtx, pstr2);

    // grab pairs and compare
    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath1, 
                SG_HDB_TO_VHASH__PAIRS,
                &pvh1)  );
    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath2, 
                SG_HDB_TO_VHASH__PAIRS,
                &pvh2)  );
    SG_STRING__ALLOC(pCtx, &pstr1); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh1, pstr1); 
    SG_STRING__ALLOC(pCtx, &pstr2); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh2, pstr2); 
    VERIFY_COND("", 0 == strcmp(SG_string__sz(pstr1), SG_string__sz(pstr2)));
    SG_VHASH_NULLFREE(pCtx, pvh1);
    SG_STRING_NULLFREE(pCtx, pstr1);
    SG_VHASH_NULLFREE(pCtx, pvh2);
    SG_STRING_NULLFREE(pCtx, pstr2);

    // remember state of file1
    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath1, 
                SG_HDB_TO_VHASH__HEADER | SG_HDB_TO_VHASH__STATS | SG_HDB_TO_VHASH__BUCKETS | SG_HDB_TO_VHASH__PAIRS,
                &pvh1)  );
    SG_STRING__ALLOC(pCtx, &pstr1); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh1, pstr1); 
    SG_VHASH_NULLFREE(pCtx, pvh1);

    // now add some stuff
    VERIFY_ERR_CHECK(  SG_hdb__open(pCtx, pPath1, 100, SG_TRUE, &pdb)  );
    for (i=0; i<100; i++)
    {
        SG_byte k[8];

        SG_random_bytes(k, sizeof(k));

        VERIFY_ERR_CHECK(  SG_hdb__put(pCtx, pdb, k, (SG_byte*) &i, SG_HDB__ON_COLLISION__ERROR)  );
    }
    // but then rollback
    VERIFY_ERR_CHECK(  SG_hdb__rollback_close_free(pCtx, pdb)  );
    pdb = NULL;

    // fetch state of file1
    VERIFY_ERR_CHECK(  SG_hdb__to_vhash(
                pCtx, 
                pPath1, 
                SG_HDB_TO_VHASH__HEADER | SG_HDB_TO_VHASH__STATS | SG_HDB_TO_VHASH__BUCKETS | SG_HDB_TO_VHASH__PAIRS,
                &pvh1)  );
    SG_STRING__ALLOC(pCtx, &pstr3); 
    SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh1, pstr3); 
    SG_VHASH_NULLFREE(pCtx, pvh1);

    // the states should match exactly, since we rolled back the changes
    VERIFY_COND("", 0 == strcmp(SG_string__sz(pstr1), SG_string__sz(pstr3)));
    SG_STRING_NULLFREE(pCtx, pstr1);
    SG_STRING_NULLFREE(pCtx, pstr3);


fail:
    SG_STRING_NULLFREE(pCtx, pstr1);
    SG_STRING_NULLFREE(pCtx, pstr2);
    SG_STRING_NULLFREE(pCtx, pstr3);
    SG_VHASH_NULLFREE(pCtx, pvh1);
    SG_VHASH_NULLFREE(pCtx, pvh2);
    SG_PATHNAME_NULLFREE(pCtx, pPath1);
    SG_PATHNAME_NULLFREE(pCtx, pPath2);
}


MyMain()
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  MyFn(test_simple)(pCtx)  );
	BEGIN_TEST(  MyFn(test_underguess)(pCtx)  );
	BEGIN_TEST(  MyFn(test_overguess)(pCtx)  );
	BEGIN_TEST(  MyFn(test_complicated)(pCtx)  );
	BEGIN_TEST(  MyFn(test_multiple)(pCtx)  );

	TEMPLATE_MAIN_END;
}

#undef MyMain
#undef MyDcl
#undef MyFn

