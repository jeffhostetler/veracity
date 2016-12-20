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
 * @file u0082_rbtreedb.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_pendingtree.h"

//////////////////////////////////////////////////////////////////

#if 0 // not currently needed
/**
 * create a silly little file of line numbers.
 */
static void u0082__create_file__numbers(SG_context * pCtx,
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

static void u0082__commit_all(SG_context * pCtx,
							  const SG_pathname* pPathWorkingDir)
{
	SG_ERR_CHECK_RETURN(  unittests_pendingtree__simple_commit(pCtx, pPathWorkingDir, NULL)  );
}
#endif

//////////////////////////////////////////////////////////////////

void u0082__test1(SG_context * pCtx, const SG_pathname * pPathTopDir)
{
	char bufName_repo[SG_TID_MAX_BUFFER_LENGTH];
	char bufCSet_I[SG_HID_MAX_BUFFER_LENGTH];
	SG_pathname * pPathTestRoot = NULL;
	SG_pathname * pPathWorkingDir_wd1 = NULL;
	SG_timestamp_cache * pTSC = NULL;
	const SG_timestamp_data * pTD;
	SG_uint32 count;
	SG_int64 clock;
	SG_int64 clock_past;
	SG_bool bValid;

	// WANRING: These tests use the timestamp_cache but are actually
	// WARNING: intended to be testing the underlying rbtreedb.
	// WARNING: So for this test I don't care about the actual cache values,
	// WARNING: but rather only that the entries get created/saved/restored
	// WARNING: correctly.

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName_repo, sizeof(bufName_repo), 6)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathTestRoot, pPathTopDir, bufName_repo)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir_wd1, pPathTestRoot, "wd_1")  );
	INFO2("working copy [1]", SG_pathname__sz(pPathWorkingDir_wd1));

	// get the current time and subtract 5 minutes so we don't have to worry
	// about filtering effects introducted by the timestamp_cache.  also make
	// sure it looks like we have sub-second resolution.

	VERIFY_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &clock)  );
	clock_past = clock - 5 * 60 * 1000;
	if ((clock_past % 1000) == 0)
		clock_past++;

	// set up basic repo and wd.  (create CSET_I)

	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx,pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  _ut_pt__new_repo(pCtx,bufName_repo, pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx,pPathWorkingDir_wd1,bufCSet_I,sizeof(bufCSet_I))  );

	// reload the timestamp_cache from disk and verify.
	// the cache should be empty because there are no files.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__allocate_and_load(pCtx, &pTSC, pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 0 entries.", (count == 0));

	// create some cache entries.  The mtimes should be old enough
	// that we won't get any filtering during the save/restore.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0001", clock_past, 0, "00001")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0002", clock_past, 0, "00002")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0003", clock_past, 0, "00003")  );
	
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 3 entries.", (count == 3));

	// save the cache to disk and reload.  all 3 entries should be present.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__save(pCtx, pTSC)  );
	SG_TIMESTAMP_CACHE_NULLFREE(pCtx, pTSC);
	VERIFY_ERR_CHECK(  SG_timestamp_cache__allocate_and_load(pCtx, &pTSC, pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 3 entries.", (count == 3));
	
	// flush the in-memory entries only.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__remove_all(pCtx, pTSC)  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 0 entries.", (count == 0));

	// discard dirty cache from memory and reload from disk.
	// we should still have the 3 entries.

	SG_TIMESTAMP_CACHE_NULLFREE(pCtx, pTSC);
	VERIFY_ERR_CHECK(  SG_timestamp_cache__allocate_and_load(pCtx, &pTSC, pPathWorkingDir_wd1)  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 3 entries.", (count == 3));

	// flush the in-memory entries by name and then save to disk.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__remove(pCtx, pTSC, "g0001")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__remove(pCtx, pTSC, "g0002")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__remove(pCtx, pTSC, "g0003")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 0 entries.", (count == 0));
	VERIFY_ERR_CHECK(  SG_timestamp_cache__save(pCtx, pTSC)  );
	SG_TIMESTAMP_CACHE_NULLFREE(pCtx, pTSC);

	// reload and verify empty cache.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__allocate_and_load(pCtx, &pTSC, pPathWorkingDir_wd1)  );
	VERIFY_COND("Cache should have 0 entries.", (count == 0));
	VERIFY_ERR_CHECK(  SG_timestamp_cache__save(pCtx, pTSC)  );
	
	// create some new entries (saving some)

	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0011", clock_past, 0, "00001")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0022", clock_past, 0, "00002")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__save(pCtx, pTSC)  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0033", clock_past, 0, "00003")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__add(pCtx, pTSC, "g0044", clock_past, 0, "00004")  );
	VERIFY_ERR_CHECK(  SG_timestamp_cache__count(pCtx, pTSC, &count)  );
	VERIFY_COND("Cache should have 4 entries.", (count == 4));

	//////////////////////////////////////////////////////////////////
	// test lookups using fake clock data.
	//////////////////////////////////////////////////////////////////

	// pretend that the current mtime on a file is 5 minutes ago
	// which matches the value in the cache, so these should be valid.

	VERIFY_ERR_CHECK(  SG_timestamp_cache__is_valid(pCtx, pTSC, "g0011", clock_past, 0, &bValid, &pTD)  );
	VERIFY_COND("is g0011 valid", (bValid));
	VERIFY_ERR_CHECK(  SG_timestamp_cache__is_valid(pCtx, pTSC, "g0033", clock_past, 0, &bValid, &pTD)  );
	VERIFY_COND("is g0033 valid", (bValid));

	// pretend that the current mtime on a file is now.  and since we cached the time 5 minutes ago, our cache is stale.
	VERIFY_ERR_CHECK(  SG_timestamp_cache__is_valid(pCtx, pTSC, "g0022", clock, 0, &bValid, &pTD)  );
	VERIFY_COND("is g0022 not valid", (!bValid));

	// pretend that the current mtime on a file is 15 minutes ago.  and since we cached the time 5 minutes
	// ago, our cache is confused.
	VERIFY_ERR_CHECK(  SG_timestamp_cache__is_valid(pCtx, pTSC, "g0022", clock_past - (10 * 60 * 1000), 0, &bValid, &pTD)  );
	VERIFY_COND("is g0022 not valid", (!bValid));

	// lookup something that isn't in the cache.
	VERIFY_ERR_CHECK(  SG_timestamp_cache__is_valid(pCtx, pTSC, "g00xx", clock_past, 0, &bValid, &pTD)  );
	VERIFY_COND("is g00xx not valid", (!bValid));

	VERIFY_ERR_CHECK(  SG_timestamp_cache__save(pCtx, pTSC)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTestRoot);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir_wd1);
	SG_TIMESTAMP_CACHE_NULLFREE(pCtx, pTSC);
}

//////////////////////////////////////////////////////////////////

TEST_MAIN(u0082_rbtreedb)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 6)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPathTopDir)  );

	BEGIN_TEST(  u0082__test1(pCtx, pPathTopDir)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);

	TEMPLATE_MAIN_END;
}
