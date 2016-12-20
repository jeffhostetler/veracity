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
 * @file u0083_timestampcache.c
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_pendingtree.h"

//////////////////////////////////////////////////////////////////

/**
 * create a silly little file of line numbers.
 */
static void u0083__create_file__numbers(SG_context * pCtx,
										SG_pathname * pPath,
										const SG_uint32 countLines)
{
	SG_uint32 i;
	SG_file * pFile = NULL;

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0600, &pFile)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

//////////////////////////////////////////////////////////////////

#define HOW_REOPEN	1

static void u0083__touch_file(SG_context * pCtx,
							  SG_pathname * pPath,
							  int how)
{
	SG_file * pFile = NULL;

	switch (how)
	{
	default:
		SG_ASSERT(  (0)  );
		return;

	case HOW_REOPEN:
		VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDWR | SG_FILE_OPEN_EXISTING, 0600, &pFile)  );
		VERIFY_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, "xxx\n")  );
		break;
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

//////////////////////////////////////////////////////////////////

static void u0083__commit_all(SG_context * pCtx,
							  const SG_pathname* pPathWorkingDir)
{
	SG_ERR_CHECK_RETURN(  unittests_pendingtree__simple_commit(pCtx, pPathWorkingDir, NULL)  );
}

//////////////////////////////////////////////////////////////////

static void u0083__dump_diff(SG_context * pCtx,
							 const SG_pathname * pPathWorkingDir,
							 const char * pszMessage,
							 SG_uint32 * pNrChanges)
{
	SG_varray * pvaStatus = NULL;
	SG_uint32 nrFilesModified = 0;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_wc__status(pCtx,
								 pPathWorkingDir,
								 "@/",
								 WC__GET_DEPTH(SG_TRUE),	// bRecursive
								 SG_FALSE, // bListUnchanged
								 SG_FALSE, // bNoIgnores
								 SG_FALSE, // bNoTSC
								 SG_FALSE, // bListSparse
								 SG_FALSE, // bListReserved
								 SG_TRUE,  // bNoSort -- don't bother
								 &pvaStatus,
								 NULL)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus, &count)  );
	for (k=0; k<count; k++)
	{
		SG_vhash * pvhItem_k;
		SG_vhash * pvhStatus_k;
		SG_int64 i64;
		SG_wc_status_flags flags;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaStatus, k, &pvhItem_k)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, "status", &pvhStatus_k)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus_k, "flags", &i64)  );
		flags = ((SG_wc_status_flags)i64);

		if (flags & SG_WC_STATUS_FLAGS__T__FILE)
			if (flags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
				nrFilesModified++;
	}

	INFOP("dump_diff",("%s [computed %d / %d]", pszMessage, nrFilesModified, count));

	*pNrChanges = nrFilesModified;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

//////////////////////////////////////////////////////////////////

static void u0083__test1(SG_context * pCtx, const SG_pathname * pPathTopDir)
{
	char bufName_repo[SG_TID_MAX_BUFFER_LENGTH];
	char bufCSet_I[SG_HID_MAX_BUFFER_LENGTH];
	char bufCSet_A[SG_HID_MAX_BUFFER_LENGTH];
	SG_pathname * pPathTestRoot = NULL;
	SG_pathname * pPathWorkingDir_wd1 = NULL;
	SG_pathname * pPath_wd1_file1 = NULL;
	SG_pathname * pPath_wd1_file2 = NULL;
	SG_pathname * pPath_wd1_file3 = NULL;
	SG_pathname * pPathWorkingDir_wd2 = NULL;
	SG_pathname * pPath_wd2_file1 = NULL;
	SG_pathname * pPath_wd2_file2 = NULL;
	SG_pathname * pPath_wd2_file3 = NULL;

	SG_fsobj_stat stat_A_wd1_1;
	SG_fsobj_stat stat_A_wd1_2;
	SG_fsobj_stat stat_A_wd1_3;
	SG_fsobj_stat stat_A_wd1_dirty_1;
	SG_fsobj_stat stat_A_wd1_dirty_2;
	SG_fsobj_stat stat_A_wd1_dirty_3;
	SG_fsobj_stat stat_A_wd2_1;
	SG_fsobj_stat stat_A_wd2_2;
	SG_fsobj_stat stat_A_wd2_3;
	SG_fsobj_stat stat_A_wd2_dirty_1;
	SG_fsobj_stat stat_A_wd2_dirty_2;
	SG_fsobj_stat stat_A_wd2_dirty_3;

	char buf_i64_a[40];
	char buf_i64_b[40];

	SG_uint32 nrChanges_wd1;
	SG_uint32 nrChanges_wd2;

	//////////////////////////////////////////////////////////////////
	// create <repo_name>  This is a unique TID to be the logical name of the repo.

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName_repo, sizeof(bufName_repo), 6)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathTestRoot, pPathTopDir, bufName_repo)  );

	// create working copy in <topdir>/<repo_name>/wd_1

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir_wd1, pPathTestRoot, "wd_1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd1_file1,     pPathWorkingDir_wd1, "file1.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd1_file2,     pPathWorkingDir_wd1, "file2.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd1_file3,     pPathWorkingDir_wd1, "file3.txt")  );

	INFO2("test1 working copy [1]", SG_pathname__sz(pPathWorkingDir_wd1));
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathWorkingDir_wd1)  );

	// create working copy in <topdir>/<repo_name>/wd_2

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir_wd2, pPathTestRoot, "wd_2")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd2_file1,     pPathWorkingDir_wd2, "file1.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd2_file2,     pPathWorkingDir_wd2, "file2.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd2_file3,     pPathWorkingDir_wd2, "file3.txt")  );

	INFO2("test1 working copy [2]", SG_pathname__sz(pPathWorkingDir_wd2));
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathWorkingDir_wd2)  );

	//////////////////////////////////////////////////////////////////
	// create a repo using <repo_name> in wd_1.
	// the initial (implied) commit will be known as CSET_I.

	VERIFY_ERR_CHECK(  _ut_pt__new_repo(pCtx,bufName_repo, pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx,pPathWorkingDir_wd1,bufCSet_I,sizeof(bufCSet_I))  );

	//////////////////////////////////////////////////////////////////
	// Using wd_1, add a collection of stuff and create CSET_A.

	INFOP("begin_building_A",("begin_building_A [%s]","wd_1"));
	VERIFY_ERR_CHECK(  u0083__create_file__numbers(pCtx, pPath_wd1_file1, 20)  );
	VERIFY_ERR_CHECK(  u0083__create_file__numbers(pCtx, pPath_wd1_file2, 40)  );
	VERIFY_ERR_CHECK(  u0083__create_file__numbers(pCtx, pPath_wd1_file3, 60)  );
	INFOP("committing_A",("committing_A"));
	VERIFY_ERR_CHECK(  _ut_pt__addremove_param(pCtx,pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  u0083__commit_all(pCtx,pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx,pPathWorkingDir_wd1,bufCSet_A,sizeof(bufCSet_A))  );
	INFOP("CSET_A",("CSET_A %s",bufCSet_A));

	//////////////////////////////////////////////////////////////////
	// get the official mtimes for each of the files in CSET_A.

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file1, &stat_A_wd1_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file2, &stat_A_wd1_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file3, &stat_A_wd1_3)  );

	//////////////////////////////////////////////////////////////////
	// touch some of the files in wd_1 and see if pendingtree notices that they are dirty.

	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd1_file1, HOW_REOPEN)  );
	SG_sleep_ms(1000);
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd1_file2, HOW_REOPEN)  );
	SG_sleep_ms(1000);
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd1_file3, HOW_REOPEN)  );

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file1, &stat_A_wd1_dirty_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file2, &stat_A_wd1_dirty_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file3, &stat_A_wd1_dirty_3)  );

	VERIFY_ERR_CHECK(  u0083__dump_diff(pCtx, pPathWorkingDir_wd1, "WD1 Should have dirty files.", &nrChanges_wd1)  );
	INFOP("timestamps1", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd1_file1), SG_int64_to_sz(stat_A_wd1_1.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd1_dirty_1.mtime_ms, buf_i64_b))  );
	INFOP("timestamps1", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd1_file2), SG_int64_to_sz(stat_A_wd1_2.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd1_dirty_2.mtime_ms, buf_i64_b))  );
	INFOP("timestamps1", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd1_file3), SG_int64_to_sz(stat_A_wd1_3.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd1_dirty_3.mtime_ms, buf_i64_b))  );

	VERIFY_COND("STATUS of wd_1", (nrChanges_wd1 == 3));
	
	//////////////////////////////////////////////////////////////////
	// Use wd_2 to do a fresh get/checkout/clone as of CSET_A

	INFO2("////////////////////////////////////////////////////////////////","Begin Part 2");
	INFO2("test1 working copy [2]", SG_pathname__sz(pPathWorkingDir_wd2));
	VERIFY_ERR_CHECK(  unittests_workingdir__create_and_checkout(pCtx,bufName_repo,pPathWorkingDir_wd2,bufCSet_A)  );

	//////////////////////////////////////////////////////////////////
	// get the official mtimes that clone/checkout put on the copies.

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file1, &stat_A_wd2_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file2, &stat_A_wd2_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file3, &stat_A_wd2_3)  );

	//////////////////////////////////////////////////////////////////
	// touch some of the files in wd_2 and see if pendingtree notices that they are dirty.

	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd2_file1, HOW_REOPEN)  );
	SG_sleep_ms(1000);
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd2_file2, HOW_REOPEN)  );
	SG_sleep_ms(1000);
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd2_file3, HOW_REOPEN)  );

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file1, &stat_A_wd2_dirty_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file2, &stat_A_wd2_dirty_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file3, &stat_A_wd2_dirty_3)  );

	VERIFY_ERR_CHECK(  u0083__dump_diff(pCtx, pPathWorkingDir_wd2, "WD2 Should have dirty files.", &nrChanges_wd2)  );
	INFOP("timestamps2", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd2_file1), SG_int64_to_sz(stat_A_wd2_1.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd2_dirty_1.mtime_ms, buf_i64_b))  );
	INFOP("timestamps2", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd2_file2), SG_int64_to_sz(stat_A_wd2_2.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd2_dirty_2.mtime_ms, buf_i64_b))  );
	INFOP("timestamps2", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd2_file3), SG_int64_to_sz(stat_A_wd2_3.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd2_dirty_3.mtime_ms, buf_i64_b))  );

	VERIFY_COND("STATUS of wd_2", (nrChanges_wd2 == 3));

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTestRoot);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir_wd1);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd1_file1);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd1_file2);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd1_file3);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir_wd2);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd2_file1);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd2_file2);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd2_file3);
}

//////////////////////////////////////////////////////////////////

static void u0083__test2(SG_context * pCtx, const SG_pathname * pPathTopDir)
{
	char bufName_repo[SG_TID_MAX_BUFFER_LENGTH];
	char bufCSet_I[SG_HID_MAX_BUFFER_LENGTH];
	char bufCSet_A[SG_HID_MAX_BUFFER_LENGTH];
	SG_pathname * pPathTestRoot = NULL;
	SG_pathname * pPathWorkingDir_wd1 = NULL;
	SG_pathname * pPath_wd1_file1 = NULL;
	SG_pathname * pPath_wd1_file2 = NULL;
	SG_pathname * pPath_wd1_file3 = NULL;
	SG_pathname * pPathWorkingDir_wd2 = NULL;
	SG_pathname * pPath_wd2_file1 = NULL;
	SG_pathname * pPath_wd2_file2 = NULL;
	SG_pathname * pPath_wd2_file3 = NULL;

	SG_fsobj_stat stat_A_wd1_1;
	SG_fsobj_stat stat_A_wd1_2;
	SG_fsobj_stat stat_A_wd1_3;
	SG_fsobj_stat stat_A_wd1_dirty_1;
	SG_fsobj_stat stat_A_wd1_dirty_2;
	SG_fsobj_stat stat_A_wd1_dirty_3;
	SG_fsobj_stat stat_A_wd2_1;
	SG_fsobj_stat stat_A_wd2_2;
	SG_fsobj_stat stat_A_wd2_3;
	SG_fsobj_stat stat_A_wd2_dirty_1;
	SG_fsobj_stat stat_A_wd2_dirty_2;
	SG_fsobj_stat stat_A_wd2_dirty_3;

	char buf_i64_a[40];
	char buf_i64_b[40];

	SG_uint32 nrChanges_wd1;
	SG_uint32 nrChanges_wd2;

	//////////////////////////////////////////////////////////////////
	// create <repo_name>  This is a unique TID to be the logical name of the repo.

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName_repo, sizeof(bufName_repo), 6)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathTestRoot, pPathTopDir, bufName_repo)  );

	// create working copy in <topdir>/wd_1

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir_wd1, pPathTestRoot, "wd_1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd1_file1,     pPathWorkingDir_wd1, "file1.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd1_file2,     pPathWorkingDir_wd1, "file2.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd1_file3,     pPathWorkingDir_wd1, "file3.txt")  );

	INFO2("test1 working copy [1]", SG_pathname__sz(pPathWorkingDir_wd1));
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathWorkingDir_wd1)  );

	// create working copy in <topdir>/wd_2

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir_wd2, pPathTestRoot, "wd_2")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd2_file1,     pPathWorkingDir_wd2, "file1.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd2_file2,     pPathWorkingDir_wd2, "file2.txt")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_wd2_file3,     pPathWorkingDir_wd2, "file3.txt")  );

	INFO2("test1 working copy [2]", SG_pathname__sz(pPathWorkingDir_wd2));
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathWorkingDir_wd2)  );

	//////////////////////////////////////////////////////////////////
	// create a repo using <repo_name> and <topdir>/wd_1.
	// the initial (implied) commit will be known as CSET_I.

	VERIFY_ERR_CHECK(  _ut_pt__new_repo(pCtx,bufName_repo, pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx,pPathWorkingDir_wd1,bufCSet_I,sizeof(bufCSet_I))  );

	//////////////////////////////////////////////////////////////////
	// Using wd_1, add a collection of stuff and create CSET_A.

	INFOP("begin_building_A",("begin_building_A [%s]","wd_1"));
	VERIFY_ERR_CHECK(  u0083__create_file__numbers(pCtx, pPath_wd1_file1, 20)  );
	VERIFY_ERR_CHECK(  u0083__create_file__numbers(pCtx, pPath_wd1_file2, 40)  );
	VERIFY_ERR_CHECK(  u0083__create_file__numbers(pCtx, pPath_wd1_file3, 60)  );
	INFOP("committing_A",("committing_A"));
	VERIFY_ERR_CHECK(  _ut_pt__addremove_param(pCtx,pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  u0083__commit_all(pCtx,pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx,pPathWorkingDir_wd1,bufCSet_A,sizeof(bufCSet_A))  );
	INFOP("CSET_A",("CSET_A %s",bufCSet_A));

	//////////////////////////////////////////////////////////////////
	// get the official mtimes for each of the files in CSET_A.

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file1, &stat_A_wd1_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file2, &stat_A_wd1_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file3, &stat_A_wd1_3)  );

	//////////////////////////////////////////////////////////////////
	// touch some of the files in wd_1 and see if pendingtree notices that they are dirty.

	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd1_file1, HOW_REOPEN)  );
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd1_file2, HOW_REOPEN)  );
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd1_file3, HOW_REOPEN)  );

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file1, &stat_A_wd1_dirty_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file2, &stat_A_wd1_dirty_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd1_file3, &stat_A_wd1_dirty_3)  );

	VERIFY_ERR_CHECK(  u0083__dump_diff(pCtx, pPathWorkingDir_wd1, "WD1 Should have dirty files.", &nrChanges_wd1)  );
	INFOP("timestamps1", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd1_file1), SG_int64_to_sz(stat_A_wd1_1.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd1_dirty_1.mtime_ms, buf_i64_b))  );
	INFOP("timestamps1", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd1_file2), SG_int64_to_sz(stat_A_wd1_2.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd1_dirty_2.mtime_ms, buf_i64_b))  );
	INFOP("timestamps1", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd1_file3), SG_int64_to_sz(stat_A_wd1_3.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd1_dirty_3.mtime_ms, buf_i64_b))  );

	VERIFY_COND("STATUS of wd_1", (nrChanges_wd1 == 3));
	
	//////////////////////////////////////////////////////////////////
	// Use wd_2 to do a fresh get/checkout/clone as of CSET_A

	INFO2("////////////////////////////////////////////////////////////////","Begin Part 2");
	INFO2("test1 working copy [2]", SG_pathname__sz(pPathWorkingDir_wd2));
	VERIFY_ERR_CHECK(  unittests_workingdir__create_and_checkout(pCtx,bufName_repo,pPathWorkingDir_wd2,bufCSet_A)  );

	//////////////////////////////////////////////////////////////////
	// get the official mtimes that clone/checkout put on the copies.

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file1, &stat_A_wd2_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file2, &stat_A_wd2_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file3, &stat_A_wd2_3)  );

	//////////////////////////////////////////////////////////////////
	// touch some of the files in wd_2 and see if pendingtree notices that they are dirty.

	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd2_file1, HOW_REOPEN)  );
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd2_file2, HOW_REOPEN)  );
	VERIFY_ERR_CHECK(  u0083__touch_file(pCtx, pPath_wd2_file3, HOW_REOPEN)  );

	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file1, &stat_A_wd2_dirty_1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file2, &stat_A_wd2_dirty_2)  );
	VERIFY_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath_wd2_file3, &stat_A_wd2_dirty_3)  );

	VERIFY_ERR_CHECK(  u0083__dump_diff(pCtx, pPathWorkingDir_wd2, "WD2 Should have dirty files.", &nrChanges_wd2)  );
	INFOP("timestamps2", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd2_file1), SG_int64_to_sz(stat_A_wd2_1.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd2_dirty_1.mtime_ms, buf_i64_b))  );
	INFOP("timestamps2", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd2_file2), SG_int64_to_sz(stat_A_wd2_2.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd2_dirty_2.mtime_ms, buf_i64_b))  );
	INFOP("timestamps2", ("%s: [official %s] [dirty %s]", SG_pathname__sz(pPath_wd2_file3), SG_int64_to_sz(stat_A_wd2_3.mtime_ms, buf_i64_a), SG_int64_to_sz(stat_A_wd2_dirty_3.mtime_ms, buf_i64_b))  );

	VERIFY_COND("STATUS of wd_2", (nrChanges_wd2 == 3));

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTestRoot);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir_wd1);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd1_file1);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd1_file2);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd1_file3);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir_wd2);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd2_file1);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd2_file2);
	SG_PATHNAME_NULLFREE(pCtx, pPath_wd2_file3);
}

//////////////////////////////////////////////////////////////////

TEST_MAIN(u0083_timestampcache)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 6)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPathTopDir)  );

	BEGIN_TEST(  u0083__test1(pCtx, pPathTopDir)  );
	BEGIN_TEST(  u0083__test2(pCtx, pPathTopDir)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);

	TEMPLATE_MAIN_END;
}
