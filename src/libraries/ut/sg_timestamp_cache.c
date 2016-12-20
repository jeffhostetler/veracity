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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#if defined(DEBUG)
#	define TRACE_TIMESTAMP			0
#   define TRACE_TIMESTAMP_STATS	0
#else
#	define TRACE_TIMESTAMP			0
#   define TRACE_TIMESTAMP_STATS	0
#endif

//////////////////////////////////////////////////////////////////

static void sg_timestamp_data__alloc(SG_context * pCtx,
									 SG_timestamp_data ** ppTD_return,
									 SG_int64 mtime_ms,
									 SG_uint64 size,
									 const char * pszHid);

#if defined(DEBUG)
#define SG_TIMESTAMP_DATA__ALLOC(pCtx, ppNew, mtime_ms, size, pszHid) SG_STATEMENT( SG_timestamp_data * _pNew = NULL;                                      \
																			  sg_timestamp_data__alloc(pCtx, &_pNew, mtime_ms, size, pszHid);              \
																			  _sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_timestamp_data"); \
																			  *(ppNew) = _pNew;                                                      )
#else
#define SG_TIMESTAMP_DATA__ALLOC(pCtx, ppNew, mtime_ms, size, pszHid) sg_timestamp_data__alloc(pCtx, ppNew, mtime_ms, size, pszHid)
#endif

static void sg_timestamp_data__free(SG_context * pCtx, SG_timestamp_data * pThis);

#define SG_TIMESTAMP_DATA_NULLFREE(pCtx,p)        SG_STATEMENT(SG_context__push_level(pCtx);        sg_timestamp_data__free(pCtx, p);   SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx));SG_context__pop_level(pCtx);p=NULL;)

static void sg_timestamp_data__set_mtime_ms(SG_context * pCtx, SG_timestamp_data * pThis, SG_int64 nMtime_ms);

static void sg_timestamp_data__set_size(SG_context * pCtx, SG_timestamp_data * pThis, SG_uint64 size);

static void sg_timestamp_data__set_hid(SG_context * pCtx, SG_timestamp_data * pThis, const char * pszHid);

//////////////////////////////////////////////////////////////////

struct _sg_timestamp_data
{
	SG_int64		mtime_ms;
	SG_uint64		size;
	char			bufHid[SG_HID_MAX_BUFFER_LENGTH];
};

struct _sg_timestamp_cache
{
	SG_pathname *	pPathTempDir;
	SG_pathname *   pPathDrawer;
	SG_rbtreedb *   prbdb;
	SG_rbtree *     prbtree;

	SG_int64		mtime_ms__on_fs_when_save_started;

#if TRACE_TIMESTAMP_STATS
	SG_uint32		nr_loaded;
	SG_uint32		nr_find_calls;
	SG_uint32		nr_found;
	SG_uint32		nr_invalid_mtime_neq;
	SG_uint32		nr_valid_mtime_eq;
	SG_uint32		nr_unneeded_add_calls;
	SG_uint32		nr_add_calls;
	SG_uint32		nr_update_calls;
	SG_uint32		nr_remove_calls;
	SG_uint32		nr_unneeded_remove_calls;
	SG_uint32		nr_save_allow;
	SG_uint32		nr_save_omit;
#endif

	SG_bool			bIsDirty;

};

//////////////////////////////////////////////////////////////////

static SG_rbtreedb__fn_create_db _sg_timestamp_cache_rbtreedb__create_db;

#define LATEST_TIMESTAMP_SCHEMA_VERSION 1
static void _sg_timestamp_cache_rbtreedb__create_db(SG_context * pCtx, void * pVoidInst, sqlite3* psql)
{
	SG_string * pStringPragma = NULL;
	//SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_UNUSED( pVoidInst );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
								   "CREATE TABLE timestamp_cache"
								   "   ("
								   "     gid TEXT PRIMARY KEY,"
								   "     hid TEXT NOT NULL,"
								   "     mtime_ms INTEGER NOT NULL,"
								   "     size INTEGER NOT NULL"
								   "   )")  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringPragma)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringPragma, "PRAGMA user_version = %d;", LATEST_TIMESTAMP_SCHEMA_VERSION)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pStringPragma) )  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringPragma);
	return;
}

static SG_rbtreedb__fn_create_db _sg_timestamp_cache_rbtreedb__open_db;

static void _sg_timestamp_cache_rbtreedb__open_db(SG_context * pCtx, void * pVoidInst, sqlite3* psql)
{
	//SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_int64 version = 0; 
	
	SG_UNUSED( pVoidInst );
	SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, psql, &version, "PRAGMA user_version;")  );

	if (version != LATEST_TIMESTAMP_SCHEMA_VERSION)
	{
		 SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DROP TABLE timestamp_cache;")  );
		 SG_ERR_CHECK(  _sg_timestamp_cache_rbtreedb__create_db(pCtx, pVoidInst, psql)  );
	}

fail:
	return;
}


static SG_rbtreedb__fn_initialize_insert_statement _sg_timestamp_cache_rbtreedb__initialize_insert_statement;

static void _sg_timestamp_cache_rbtreedb__initialize_insert_statement(SG_context * pCtx, void * pVoidInst, sqlite3* psql, sqlite3_stmt ** ppStmt)
{
	//SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_UNUSED( pVoidInst );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, ppStmt,
				"INSERT OR REPLACE INTO timestamp_cache (gid, hid, mtime_ms, size) VALUES (?, ?, ?, ?)")  );

fail:
	return;
}

static SG_rbtreedb__fn_bind_data _sg_timestamp_cache_rbtreedb__bind_data;

static void _sg_timestamp_cache_rbtreedb__bind_data(SG_context * pCtx, void * pVoidInst, sqlite3_stmt * pStmt, SG_bool * bDontInsertThisOne, const char* pszKey, void* pData)
{
	SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_timestamp_data * pTD = (SG_timestamp_data*) pData;
	SG_int64 mtime_ms_delta;
	SG_bool bAllowSave;

	SG_ASSERT(  (pTSC->mtime_ms__on_fs_when_save_started != 0)  );

	mtime_ms_delta = (pTSC->mtime_ms__on_fs_when_save_started - pTD->mtime_ms);

	// TODO 2010/08/15 Consider having a grace period here ( delta > x )
	// TODO            of 2 or 3 seconds where we basically require a
	// TODO            whole second (or 2) of time to have elapsed before
	// TODO            we really trust this timestamp.  I'm thinking of
	// TODO            FAT32 which only has 2-second resolution here.

	bAllowSave = (mtime_ms_delta > 0);
	
#if TRACE_TIMESTAMP
	{
		char buf_i64[40];

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "TimestampCache_Bind: [key %s] [mtime_ms_delta %s] %s\n",
								   pszKey,
								   SG_int64_to_sz(mtime_ms_delta, buf_i64),
								   ((bAllowSave) ? "ALLOW" : "OMIT"))  );
	}
#endif

#if TRACE_TIMESTAMP_STATS
	if (bAllowSave)
		pTSC->nr_save_allow++;
	else
		pTSC->nr_save_omit++;
#endif

	if (bAllowSave)
	{
		SG_ERR_CHECK(  sg_sqlite__bind_text( pCtx, pStmt, 1, pszKey)         );		// gid
		SG_ERR_CHECK(  sg_sqlite__bind_text( pCtx, pStmt, 2, pTD->bufHid)    );		// hid of file content
		SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 3, pTD->mtime_ms)  );		// mtime
		SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 4, pTD->size)  );		// size
	}

	*bDontInsertThisOne = ( !bAllowSave );

fail:
	return;
}

static SG_rbtreedb__fn_prepare_select_statement _sg_timestamp_cache_rbtreedb__prepare_select_statement;

static void _sg_timestamp_cache_rbtreedb__prepare_select_statement(SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL, sqlite3_stmt ** ppStmt)
{
	//SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_UNUSED( pVoidInst );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pSQL, ppStmt,
									  "SELECT gid, hid, mtime_ms, size FROM timestamp_cache;")  );

fail:
	return;
}

static SG_rbtreedb__fn_receive_select_row _sg_timestamp_cache_rbtreedb__receive_select_row;

static void _sg_timestamp_cache_rbtreedb__receive_select_row(SG_context * pCtx, void * pVoidInst, sqlite3_stmt * pStmt, SG_rbtree * prb)
{
	SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_timestamp_data * pTD_allocated = NULL;
	SG_int64 mtime_ms;
	SG_uint64 size;
	const char * pszKey;
	const char * pszHid;
	SG_UNUSED( prb );

	pszKey = (const char *)sqlite3_column_text(pStmt, 0);		// the gid
	pszHid = (const char *)sqlite3_column_text(pStmt, 1);		// hid
	mtime_ms = sqlite3_column_int64(pStmt, 2);
	size = (SG_uint64) sqlite3_column_int64(pStmt, 3);

	SG_ERR_CHECK(  SG_TIMESTAMP_DATA__ALLOC(pCtx, &pTD_allocated, mtime_ms, size, pszHid)  );

#if TRACE_TIMESTAMP
	SG_ERR_CHECK(  SG_timestamp_data__dump_to_console(pCtx, pTD_allocated, pszKey, "TimestampCache_Loading")  );
#endif

	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pTSC->prbtree, pszKey, (void *)pTD_allocated)  );
	pTD_allocated = NULL;		// rbtree owns it now
		
#if TRACE_TIMESTAMP_STATS
	pTSC->nr_loaded++;
#endif

	// we do not set pTSC->bIsDirty.

	return;

fail:
	SG_ERR_IGNORE(  sg_timestamp_data__free(pCtx, pTD_allocated)  );
}

static SG_rbtreedb__fn_delete_all_rows _sg_timestamp_cache_rbtreedb__delete_all_rows;

static void _sg_timestamp_cache_rbtreedb__delete_all_rows(SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL)
{
	//SG_timestamp_cache * pTSC = (SG_timestamp_cache *)pVoidInst;
	SG_UNUSED( pVoidInst );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pSQL, "DELETE FROM timestamp_cache;")  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static SG_rbtreedb__fn_pointers * _sg_timestamp_cache__get_rbtreedb_functions(SG_context * pCtx)
{
	SG_rbtreedb__fn_pointers * pFnPtrs = NULL;

	SG_ERR_CHECK( SG_alloc1(pCtx, pFnPtrs) );

	pFnPtrs->fnPtr_create_db                   = _sg_timestamp_cache_rbtreedb__create_db;
	pFnPtrs->fnPtr_open_db                     = _sg_timestamp_cache_rbtreedb__open_db;
	pFnPtrs->fnPtr_initialize_insert_statement = _sg_timestamp_cache_rbtreedb__initialize_insert_statement;
	pFnPtrs->fnPtr_bind_data                   = _sg_timestamp_cache_rbtreedb__bind_data;
	pFnPtrs->fnPtr_prepare_select_statement    = _sg_timestamp_cache_rbtreedb__prepare_select_statement;
	pFnPtrs->fnPtr_receive_select_row          = _sg_timestamp_cache_rbtreedb__receive_select_row;
	pFnPtrs->fnPtr_delete_all_rows             = _sg_timestamp_cache_rbtreedb__delete_all_rows;

	return pFnPtrs;

fail:
	return NULL;
}

//////////////////////////////////////////////////////////////////

void SG_timestamp_cache__allocate_and_load(SG_context * pCtx,
										   SG_timestamp_cache ** ppTSC,
										   const SG_pathname * pPathWorkingDirectoryTop)
{
	SG_timestamp_cache * pTSC = NULL;

	SG_NULLARGCHECK_RETURN(pPathWorkingDirectoryTop);

#if TRACE_TIMESTAMP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TimestampCache_AllocateLoad: [%s]\n", SG_pathname__sz(pPathWorkingDirectoryTop))  );
#endif

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTSC)  );
	pTSC->bIsDirty = SG_FALSE;

	pTSC->mtime_ms__on_fs_when_save_started = 0;	// epoch in fs_clock means we couldn't get current fs time

	SG_ERR_CHECK(  SG_workingdir__get_temp_path(pCtx, pPathWorkingDirectoryTop, &pTSC->pPathTempDir)  );

	SG_ERR_CHECK(  SG_workingdir__get_drawer_path(pCtx, pPathWorkingDirectoryTop, &pTSC->pPathDrawer)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pTSC->pPathDrawer, "timestampcache.rbtreedb")  );
	SG_ERR_CHECK(  SG_rbtreedb__create_or_open(pCtx,
											   pTSC->pPathDrawer,
											   (void *)pTSC,
											   _sg_timestamp_cache__get_rbtreedb_functions(pCtx),
											   &pTSC->prbdb)  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pTSC->prbtree)  );
	SG_ERR_CHECK(  SG_rbtreedb__load_rbtree(pCtx, pTSC->prbdb, pTSC->prbtree)  );

#if 0 && TRACE_TIMESTAMP
	SG_ERR_CHECK(  SG_timestamp_cache__dump_to_console(pCtx, pTSC, "Allocate_and_Load")  );
#endif

	*ppTSC = pTSC;
	return;

fail:
	SG_TIMESTAMP_CACHE_NULLFREE(pCtx, pTSC);
}

//////////////////////////////////////////////////////////////////

/**
 * When we get ready to write the timestamp_cache to disk, we
 * have (mtime_ms, hid) values for each entry and we *believe*
 * that the HID correctly represents the content of the file
 * as of that time.  BUT, because of the limited accuracy of
 * "mtime_ms" (on filesystems that don't have sub-second resolution
 * those last 3 digits are 0 -- and on FAT we only have 2-second
 * resolution).  SO, it is possible for someone to modify the file
 * *after* we cache the value and *before* the clock ticks.  So
 * the file may have a different hash but the same mtime.
 *
 * This is not good.
 *
 * So we fetch the current clock and stash that in pTSC.  Then
 * when we are writing the rows to the rbtreedb we OMIT the ones
 * where the mtime is close enough to allow this ambiguity to
 * happen.
 *
 * We need to ask the filesystem for the current time in case
 * it is remotely mounted with clock skew issues and it allows
 * us to see what the resolution of mtime is on that filessystem.
 *
 * We create a temp file, get the mtime, and then delete it.
 */
static void _sg_timestamp_cache__get_fs_clock(SG_context * pCtx, SG_timestamp_cache * pTSC)
{
	SG_pathname * pPathTempFile = NULL;
	SG_file * pFile = NULL;
	SG_fsobj_stat stat;
	SG_bool bExistsTempDir;

	pTSC->mtime_ms__on_fs_when_save_started = 0;

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pTSC->pPathTempDir, &bExistsTempDir, NULL, NULL)  );
	if (!bExistsTempDir)
		SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pTSC->pPathTempDir)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPathTempFile, pTSC->pPathTempDir)  );
	SG_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, pPathTempFile, "timestamp.tmp")  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathTempFile, SG_FILE_RDWR | SG_FILE_CREATE_NEW, 0600, &pFile)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);
	
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPathTempFile, &stat)  );
	pTSC->mtime_ms__on_fs_when_save_started = stat.mtime_ms;

#if TRACE_TIMESTAMP
	{
		char buf_i64[40];

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "TimestampCache_Save: [fs_clock_ms %s]\n",
								   SG_int64_to_sz(pTSC->mtime_ms__on_fs_when_save_started, buf_i64))  );
	}
#endif

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	if (pPathTempFile)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathTempFile)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathTempFile);
	}
}

void SG_timestamp_cache__save(SG_context * pCtx, SG_timestamp_cache * pTSC)
{
	SG_NULLARGCHECK_RETURN(pTSC);
	SG_NULLARGCHECK_RETURN(pTSC->prbdb);
	SG_NULLARGCHECK_RETURN(pTSC->prbtree);

	SG_ERR_IGNORE(  _sg_timestamp_cache__get_fs_clock(pCtx, pTSC)  );
	if (pTSC->mtime_ms__on_fs_when_save_started == 0)
	{
		// we couldn't get the FS clock, so we don't know if any of
		// our cached times are good enough to prevent someone from
		// modifying a file *after* we are done here without changing
		// the mtime (at the accuracy that we have).  this would cause
		// our cached HID to be stale.
		//
		// so throw away all entries in the cache and write an empty
		// cache to disk.

#if TRACE_TIMESTAMP
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "TimestampCache_Save: Flushing all entries because fs_clock is zero\n")  );
#endif
		SG_ERR_CHECK(  SG_timestamp_cache__remove_all(pCtx, pTSC)  );
	}

	if (pTSC->bIsDirty)
		SG_ERR_CHECK(  SG_rbtreedb__store_rbtree(pCtx, pTSC->prbdb, pTSC->prbtree)  );

#if TRACE_TIMESTAMP_STATS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "TimestampCache_Save: %s -- Stats [Loaded %d] [Find %d / %d] [Valid NEQ/EQ %d / %d] [Add/Update/Dup %d / %d / %d] [Remove/Unk %d / %d] [save allow/omit/total %d / %d / %d]\n",
							   ((pTSC->bIsDirty)
								? "Wrote Dirty Cache to Disk"
								: "Skipped Write of Clean Cache"),
							   pTSC->nr_loaded,
							   pTSC->nr_found, pTSC->nr_find_calls,
							   pTSC->nr_invalid_mtime_neq, pTSC->nr_valid_mtime_eq,
							   pTSC->nr_add_calls, pTSC->nr_update_calls, pTSC->nr_unneeded_add_calls,
							   pTSC->nr_remove_calls, pTSC->nr_unneeded_remove_calls,
							   pTSC->nr_save_allow, pTSC->nr_save_omit, (pTSC->nr_save_allow + pTSC->nr_save_omit))  );
	pTSC->nr_loaded = 0;
	pTSC->nr_find_calls = 0;
	pTSC->nr_found = 0;
	pTSC->nr_invalid_mtime_neq = 0;
	pTSC->nr_valid_mtime_eq = 0;
	pTSC->nr_unneeded_add_calls = 0;
	pTSC->nr_add_calls = 0;
	pTSC->nr_update_calls = 0;
	pTSC->nr_remove_calls = 0;
	pTSC->nr_unneeded_remove_calls = 0;
	pTSC->nr_save_allow = 0;
	pTSC->nr_save_omit = 0;
#endif

	pTSC->bIsDirty = SG_FALSE;

fail:
	return;
}

void SG_timestamp_cache__free(SG_context * pCtx, SG_timestamp_cache * pTSC)
{
	if (!pTSC)
		return;

#if TRACE_TIMESTAMP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TimestampCache_Free: [dirty %c]\n", ((pTSC->bIsDirty) ? 'Y' : 'N'))  );
//	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__dump_to_console(pCtx, pTSC, "Dump in TimestampCache_Free")  );
#endif

	SG_PATHNAME_NULLFREE( pCtx, pTSC->pPathTempDir );
	SG_PATHNAME_NULLFREE( pCtx, pTSC->pPathDrawer );
	SG_RBTREEDB_NULLFREE( pCtx, pTSC->prbdb );
	SG_RBTREE_NULLFREE_WITH_ASSOC( pCtx, pTSC->prbtree, (SG_free_callback *)sg_timestamp_data__free);
	SG_NULLFREE( pCtx, pTSC );
}

//////////////////////////////////////////////////////////////////

static void sg_timestamp_cache__find_data(SG_context * pCtx, SG_timestamp_cache * pTSC, const char * pszKey,
										  SG_bool * pbFound, SG_timestamp_data ** ppData)
{
	SG_NULLARGCHECK_RETURN(pTSC);
	SG_NULLARGCHECK_RETURN(pTSC->prbtree);
	SG_NONEMPTYCHECK_RETURN(pszKey);
	SG_NULLARGCHECK_RETURN(pbFound);
	SG_NULLARGCHECK_RETURN(ppData);

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pTSC->prbtree, pszKey, pbFound, (void **)ppData)  );

#if 0 && TRACE_TIMESTAMP
	if (*pbFound)
		SG_ERR_CHECK(  SG_timestamp_data__dump_to_console(pCtx, (*ppData), pszKey, "TimestampCache_Find")  );
	else
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%40s: [key %s] not found in cache.\n",
								   "TimestampCache_Find",
								   pszKey)  );
#endif

#if TRACE_TIMESTAMP_STATS
	pTSC->nr_find_calls++;
	if (*pbFound)
		pTSC->nr_found++;
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_timestamp_cache__add(SG_context * pCtx,
							 SG_timestamp_cache * pTSC,
							 const char * pszGid,
							 SG_int64 mtime_ms,
							 SG_uint64 size,
							 const char * pszHid)
{
	SG_timestamp_data * pTD_allocated = NULL;
	SG_timestamp_data * pTD_existing = NULL;	// we do not own this
	SG_bool bFound;

	SG_ERR_CHECK(  sg_timestamp_cache__find_data(pCtx, pTSC, pszGid, &bFound, &pTD_existing)  );
	if (bFound)
	{
		SG_bool bEqual = SG_FALSE;

		SG_ERR_CHECK(  SG_timestamp_data__is_equal_to(pCtx, pTD_existing, mtime_ms, size, pszHid, &bEqual)  );
		if (bEqual)
		{
#if TRACE_TIMESTAMP_STATS
			pTSC->nr_unneeded_add_calls++;
#endif
		}
		else
		{
#if TRACE_TIMESTAMP
			SG_ERR_CHECK(  SG_timestamp_data__dump_to_console(pCtx, pTD_existing, pszGid, "TimestampCache_Update: old")  );
#endif

			SG_ERR_CHECK(  sg_timestamp_data__set_mtime_ms(pCtx, pTD_existing, mtime_ms)  );
			SG_ERR_CHECK(  sg_timestamp_data__set_size(pCtx, pTD_existing, size)  );
			SG_ERR_CHECK(  sg_timestamp_data__set_hid(pCtx, pTD_existing, pszHid)  );

#if TRACE_TIMESTAMP
			SG_ERR_CHECK(  SG_timestamp_data__dump_to_console(pCtx, pTD_existing, pszGid, "TimestampCache_Update: new")  );
#endif

#if TRACE_TIMESTAMP_STATS
			pTSC->nr_update_calls++;
#endif

			pTSC->bIsDirty = SG_TRUE;
		}
	}
	else
	{
		SG_ERR_CHECK(  SG_TIMESTAMP_DATA__ALLOC(pCtx, &pTD_allocated, mtime_ms, size, pszHid)  );

#if TRACE_TIMESTAMP
		SG_ERR_CHECK(  SG_timestamp_data__dump_to_console(pCtx, pTD_allocated, pszGid, "TimestampCache_Add")  );
#endif

		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pTSC->prbtree, pszGid, (void *)pTD_allocated)  );
		pTD_allocated = NULL;		// rbtree owns it now
		
#if TRACE_TIMESTAMP_STATS
		pTSC->nr_add_calls++;
#endif

		pTSC->bIsDirty = SG_TRUE;
	}
	
	return;

fail:
	SG_TIMESTAMP_DATA_NULLFREE(pCtx, pTD_allocated);
}

void SG_timestamp_cache__is_valid(SG_context * pCtx,
								  SG_timestamp_cache * pTSC,
								  const char * pszGid,
								  SG_int64 mtime_ms_now,
								  SG_uint64 size_now,
								  SG_bool * pbValid,
								  const SG_timestamp_data ** ppData)
{
	SG_timestamp_data * pData = NULL;
	SG_bool bFound;

	SG_ERR_CHECK(  sg_timestamp_cache__find_data(pCtx, pTSC, pszGid, &bFound, &pData)  );
	if (!bFound)
	{
#if TRACE_TIMESTAMP
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%40s: [key %s] not found in cache.\n",
								   "TimestampCache_IsValid",
								   pszGid)  );
#endif

		*pbValid = SG_FALSE;
		return;
	}

	if (mtime_ms_now > pData->mtime_ms || size_now != pData->size)
	{
		// stat() reports that the file is newer value than what we have cached, or the size has changed.
		// we claim that the file is dirty and our cached value are stale.

#if TRACE_TIMESTAMP
		char buf_i64_current[40];
		char buf_i64_cached[40];

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%40s: [key %s] not valid: [current %s] > [cached %s].\n",
								   "TimestampCache_IsValid",
								   pszGid,
								   SG_int64_to_sz(mtime_ms_now, buf_i64_current),
								   SG_int64_to_sz(pData->mtime_ms, buf_i64_cached))  );
#endif

#if TRACE_TIMESTAMP_STATS
		pTSC->nr_invalid_mtime_neq++;
#endif

		*pbValid = SG_FALSE;
		return;
	}
	else if (mtime_ms_now < pData->mtime_ms)
	{
		// stat() reports that the file is *OLDER* than we thought it was.
		// this should not happen unless someone has explicitly set the mtime
		// back.  if this is the case, all bets are off.  invalidate the cache.

#if TRACE_TIMESTAMP
		char buf_i64_current[40];
		char buf_i64_cached[40];

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%40s: [key %s] invalid: [current %s] < [cached %s].\n",
								   "TimestampCache_IsValid",
								   pszGid,
								   SG_int64_to_sz(mtime_ms_now, buf_i64_current),
								   SG_int64_to_sz(pData->mtime_ms, buf_i64_cached))  );
#endif

#if TRACE_TIMESTAMP_STATS
		pTSC->nr_invalid_mtime_neq++;
#endif

		*pbValid = SG_FALSE;
		return;
	}
	else	// the mtimes are equal, and the sizes are the same.
	{
		// Since we now only write the cache entries
		// that we are confident about, we can assume
		// that the clock had ticked enough when we wrote
		// the cache that we don't need to worry about
		// the lack of sub-second resolution and the
		// user being able to modify a file and it getting
		// the same mtime *after* we have recorded the
		// mtime (and the old HID) in the cache.

#if TRACE_TIMESTAMP
		{
			char buf_i64_current[40];

			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%40s: [key %s] valid [current %s].\n",
									   "TimestampCache_IsValid",
									   pszGid,
									   SG_int64_to_sz(mtime_ms_now, buf_i64_current))  );
		}
#endif

#if TRACE_TIMESTAMP_STATS
		pTSC->nr_valid_mtime_eq++;
#endif

		*pbValid = SG_TRUE;

		if (ppData)
			*ppData = pData;

		return;
	}

fail:
	return;
}

void SG_timestamp_cache__remove(SG_context * pCtx,
								SG_timestamp_cache * pTSC,
								const char * pszGid)
{
	SG_timestamp_data * pTD = NULL;

	SG_NULLARGCHECK_RETURN(pTSC);
	SG_NONEMPTYCHECK_RETURN(pszGid);

	SG_rbtree__remove__with_assoc(pCtx, pTSC->prbtree, pszGid, (void **)&pTD);

	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND) == SG_FALSE)
			SG_ERR_RETHROW_RETURN;

		SG_context__err_reset(pCtx);

#if TRACE_TIMESTAMP
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%40s: [key %s] not found.\n", "TimestampCache_Remove", pszGid)  );
#endif

#if TRACE_TIMESTAMP_STATS
		pTSC->nr_unneeded_remove_calls++;
#endif
	}
	else
	{
#if TRACE_TIMESTAMP
		SG_ERR_CHECK_RETURN(  SG_timestamp_data__dump_to_console(pCtx, pTD, pszGid, "TimestampCache_Remove")  );
#endif

		SG_TIMESTAMP_DATA_NULLFREE(pCtx, pTD);

#if TRACE_TIMESTAMP_STATS
		pTSC->nr_remove_calls++;
#endif

		pTSC->bIsDirty = SG_TRUE;
	}
}

void SG_timestamp_cache__remove_all(SG_context * pCtx,
									SG_timestamp_cache * pTSC)
{
	SG_uint32 count;

	SG_NULLARGCHECK_RETURN(pTSC);

	SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx, pTSC->prbtree, &count)  );

#if TRACE_TIMESTAMP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TimestampCache_RemoveAll: [count %d].\n", count)  );
#endif

	if (count > 0)
	{
		SG_RBTREE_NULLFREE_WITH_ASSOC( pCtx, pTSC->prbtree, (SG_free_callback *)sg_timestamp_data__free);
		SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, &pTSC->prbtree)  );

#if TRACE_TIMESTAMP_STATS
		pTSC->nr_remove_calls += count;
#endif

		pTSC->bIsDirty = SG_TRUE;
	}
}

void SG_timestamp_cache__count(SG_context * pCtx,
							   SG_timestamp_cache * pTSC,
							   SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN(pTSC);
	SG_NULLARGCHECK_RETURN(pCount);

	SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx, pTSC->prbtree, pCount)  );
}

//////////////////////////////////////////////////////////////////

static void sg_timestamp_data__alloc(SG_context * pCtx,
									 SG_timestamp_data ** ppTD_return,
									 SG_int64 mtime_ms,
									 SG_uint64 size,
									 const char * pszHid)
{
	SG_timestamp_data * pTD = NULL;

	SG_NULLARGCHECK_RETURN( ppTD_return );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTD)  );

	pTD->mtime_ms = mtime_ms;
	pTD->size = size;
	SG_ERR_CHECK(  SG_strcpy(pCtx, pTD->bufHid, sizeof(pTD->bufHid), pszHid)  );

	*ppTD_return = pTD;
	return;

fail:
	SG_TIMESTAMP_DATA_NULLFREE(pCtx, pTD);
}

static void sg_timestamp_data__free(SG_context * pCtx, SG_timestamp_data * pThis)
{
	if (!pThis)
		return;

//#if TRACE_TIMESTAMP
//	SG_ERR_CHECK_RETURN(  SG_timestamp_data__dump_to_console(pCtx, pThis, "(unknown)", "TimestampData_Free")  );
//#endif

	SG_NULLFREE(pCtx, pThis);
}

static void sg_timestamp_data__set_mtime_ms(SG_context * pCtx, SG_timestamp_data * pThis, SG_int64 nMtime_ms)
{
	SG_NULLARGCHECK_RETURN(pThis);

	pThis->mtime_ms = nMtime_ms;
}

static void sg_timestamp_data__set_size(SG_context * pCtx, SG_timestamp_data * pThis, SG_uint64 nSize)
{
	SG_NULLARGCHECK_RETURN(pThis);

	pThis->size = nSize;
}

void SG_timestamp_data__get_mtime_ms(SG_context * pCtx, const SG_timestamp_data * pThis, SG_int64 * pnMtime_ms)
{
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK_RETURN(pThis);

	*pnMtime_ms = pThis->mtime_ms;
}

void SG_timestamp_data__get_size(SG_context * pCtx, const SG_timestamp_data * pThis, SG_uint64 * pSize)
{
	SG_NULLARGCHECK_RETURN(pThis);
	SG_NULLARGCHECK_RETURN(pSize);

	*pSize = pThis->size;
}

static void sg_timestamp_data__set_hid(SG_context * pCtx, SG_timestamp_data * pThis, const char * pszHid)
{
	SG_NULLARGCHECK_RETURN(pThis);
	SG_NONEMPTYCHECK_RETURN(pszHid);

	SG_ERR_CHECK_RETURN(  SG_strcpy(pCtx, pThis->bufHid, sizeof(pThis->bufHid), pszHid)  );
}

void SG_timestamp_data__get_hid__ref(SG_context * pCtx, const SG_timestamp_data * pThis, const char ** ppszHid)
{
	SG_NULLARGCHECK_RETURN(pThis);
	SG_NULLARGCHECK_RETURN(ppszHid);

	*ppszHid = pThis->bufHid;
}

void SG_timestamp_data__is_equal_to(SG_context * pCtx,
									const SG_timestamp_data * pThis,
									SG_int64 mtime_ms, SG_uint64 size, const char * pszHid,
									SG_bool * pbEqual)
{
	SG_NULLARGCHECK_RETURN(pThis);
	SG_NONEMPTYCHECK_RETURN(pszHid);

	*pbEqual = ((pThis->mtime_ms == mtime_ms)  && (pThis->size == size) &&  (strcmp(pThis->bufHid, pszHid) == 0));
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_timestamp_data__dump_to_console(SG_context * pCtx, const SG_timestamp_data * pData, const char * pszKey, const char * pszMessage)
{
	char buf_i64_1[40];
	char buf_i64_2[40];

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "%40s: [key %65s] [pData %p] [mtime %s] [size %s] [hid %s]\n",
							   ((pszMessage) ? pszMessage : ""),
							   ((pszKey) ? pszKey : ""),
							   pData,
							   (SG_int64_to_sz(pData->mtime_ms, buf_i64_1)),
							   (SG_uint64_to_sz(pData->size, buf_i64_2)),
							   pData->bufHid)  );
}

void SG_timestamp_cache__dump_to_console(SG_context * pCtx,
										 const SG_timestamp_cache * pTSC,
										 const char * pszMessage)
{
	SG_rbtree_iterator * pIter = NULL;
	const SG_timestamp_data * pTD;
	const char * pszGid;
	SG_uint32 count;
	SG_bool bOK;

	SG_NULLARGCHECK_RETURN(pTSC);
	// pszMessage is optional

	SG_ERR_CHECK(  SG_rbtree__count(pCtx, pTSC->prbtree, &count)  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "TimestampCache_Dump: [count %d] [%s]\n",
							   count,
							   ((pszMessage) ? pszMessage : ""))  );

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pTSC->prbtree, &bOK, &pszGid, (void **)&pTD)  );
	while (bOK)
	{
		SG_ERR_CHECK(  SG_timestamp_data__dump_to_console(pCtx, pTD, pszGid, "\t")  );

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &bOK, &pszGid, (void **)&pTD)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}
#endif
