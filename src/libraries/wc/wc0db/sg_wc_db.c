/*
Copyright 2011-2013 SourceGear, LLC

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
 * @file sg_wc_db.c
 *
 * @details Routines to build/maintain a SQLITE database for the
 * reference CSETs and the PENDING changes in the Working Copy.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#define MY_BUSY_TIMEOUT_MS      (30000)

//////////////////////////////////////////////////////////////////

void sg_wc_db__free(SG_context * pCtx, sg_wc_db * pDb)
{
	if (!pDb)
		return;

	SG_FILE_SPEC_NULLFREE(pCtx, pDb->pFilespecIgnores);
	SG_REPO_NULLFREE(pCtx, pDb->pRepo);
	SG_PATHNAME_NULLFREE(pCtx, pDb->pPathWorkingDirectoryTop);
	SG_WC_ATTRBITS_DATA__NULLFREE(pCtx, pDb->pAttrbitsData);
	SG_TIMESTAMP_CACHE_NULLFREE(pCtx, pDb->pTSC);

	if (pDb->psql)
		SG_ERR_IGNORE(  sg_wc_db__close_db(pCtx, pDb, SG_FALSE)  );

	SG_PATHNAME_NULLFREE(pCtx, pDb->pPathDB);
	
	SG_NULLFREE(pCtx, pDb);
}

void sg_wc_db__tx__free_cached_statements(SG_context * pCtx, sg_wc_db * pDb)
{
	if (!pDb)
		return;

	if (pDb->pSqliteStmt__get_issue != NULL)
	{
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pDb->pSqliteStmt__get_issue)  );
		pDb->pSqliteStmt__get_issue = NULL;
	}

	if (pDb->pSqliteStmt__get_gid_from_alias != NULL)
	{
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pDb->pSqliteStmt__get_gid_from_alias)  );
		pDb->pSqliteStmt__get_gid_from_alias = NULL;
	}

	if (pDb->prbCachedSqliteStmts__get_row_by_alias != NULL)
		SG_ERR_IGNORE(  SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pDb->prbCachedSqliteStmts__get_row_by_alias, (SG_free_callback *)sg_sqlite__finalize)  );
}

void sg_wc_db__close_db(SG_context * pCtx, sg_wc_db * pDb, SG_bool bDelete_DB)
{
	if (!pDb)
		return;

	SG_ERR_IGNORE(  sg_wc_db__tx__free_cached_statements(pCtx, pDb)  );

	if (pDb->psql)
	{
		sg_sqlite__close(pCtx, pDb->psql);
		pDb->psql = NULL;
		SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
	}

	if (bDelete_DB)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pDb->pPathDB)  );
		SG_PATHNAME_NULLFREE(pCtx, pDb->pPathDB);
	}
}

//////////////////////////////////////////////////////////////////

static void _db__pragmas(SG_context * pCtx, sg_wc_db * pDb, SG_bool bJournalMode_WAL)
{
	int rc;
	
	rc = sqlite3_busy_timeout(pDb->psql, MY_BUSY_TIMEOUT_MS);
	if (rc)
		SG_ERR_THROW(SG_ERR_SQLITE(rc));

	if (bJournalMode_WAL)
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "PRAGMA journal_mode=WAL")  );
	else
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "PRAGMA journal_mode=OFF")  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "PRAGMA foreign_key=ON")  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__open_db(SG_context * pCtx,
					   const SG_repo * pRepo,
					   const SG_pathname * pPathWorkingDirectoryTop,
					   SG_bool bWorkingDirPathFromCwd,
					   sg_wc_db ** ppDb)
{
	sg_wc_db * pDb = NULL;

	SG_NULLARGCHECK_RETURN( pPathWorkingDirectoryTop );
	SG_NULLARGCHECK_RETURN( ppDb );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pDb)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pDb->pPathWorkingDirectoryTop, pPathWorkingDirectoryTop)  );
	SG_ERR_CHECK(  SG_repo__open_repo_instance__copy(pCtx, pRepo, &pDb->pRepo)  );
	pDb->pFilespecIgnores = NULL;	// defer loading these until somebody needs them.
	pDb->pAttrbitsData = NULL;		// defer loading these until somebody needs them.
	pDb->pTSC = NULL;				// defer loading these until somebody needs them.
	pDb->pSqliteStmt__get_gid_from_alias = NULL;
	pDb->pSqliteStmt__get_issue = NULL;
	pDb->prbCachedSqliteStmts__get_row_by_alias = NULL;

	// Lookup the port mask at the beginning of the TX.
	SG_ERR_CHECK(  SG_wc_port__get_mask_from_config(pCtx, pDb->pRepo, pDb->pPathWorkingDirectoryTop,
													&pDb->portMask)  );

	pDb->bWorkingDirPathFromCwd = bWorkingDirPathFromCwd;

	SG_ERR_CHECK(  sg_wc_db__path__compute_pathname_in_drawer(pCtx, pDb)  );
	SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pDb->pPathDB, SG_SQLITE__SYNC__NORMAL, &pDb->psql)  );
	SG_ERR_CHECK(  _db__pragmas(pCtx, pDb, SG_TRUE)  );

	// Verify that we understand the DB version.

	SG_ERR_CHECK(  sg_wc_db__info__read_version(pCtx, pDb, NULL)  );

	*ppDb = pDb;
	return;

fail:
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_WC_DB_BUSY);
	SG_ERR_IGNORE(  sg_wc_db__close_db(pCtx, pDb, SG_FALSE)  );
	SG_WC_DB_NULLFREE(pCtx, pDb);
}

void sg_wc_db__create_db(SG_context * pCtx,
						 const SG_repo * pRepo,
						 const SG_pathname * pPathWorkingDirectoryTop,
						 SG_bool bWorkingDirPathFromCwd,
						 sg_wc_db ** ppDb)
{
	sg_wc_db * pDb = NULL;
	SG_bool bWeCreated = SG_FALSE;

	SG_NULLARGCHECK_RETURN( pPathWorkingDirectoryTop );
	SG_NULLARGCHECK_RETURN( ppDb );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pDb)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pDb->pPathWorkingDirectoryTop, pPathWorkingDirectoryTop)  );
	SG_ERR_CHECK(  SG_repo__open_repo_instance__copy(pCtx, pRepo, &pDb->pRepo)  );
	pDb->pFilespecIgnores = NULL;	// defer loading these until somebody needs them.
	pDb->pAttrbitsData = NULL;		// defer loading these until somebody needs them.
	pDb->pTSC = NULL;				// defer loading these until somebody needs them.
	pDb->pSqliteStmt__get_gid_from_alias = NULL;
	pDb->pSqliteStmt__get_issue = NULL;
	pDb->prbCachedSqliteStmts__get_row_by_alias = NULL;

	pDb->bWorkingDirPathFromCwd = bWorkingDirPathFromCwd;

	SG_ERR_CHECK(  sg_wc_db__path__compute_pathname_in_drawer(pCtx, pDb)  );

	// this throws if it already exists.
	SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pDb->pPathDB, SG_SQLITE__SYNC__NORMAL, &pDb->psql)  );
	bWeCreated = SG_TRUE;

	SG_ERR_CHECK(  _db__pragmas(pCtx, pDb, SG_FALSE)  );

	SG_ERR_CHECK(  sg_wc_db__tx__begin(pCtx, pDb, SG_TRUE)  );

	// create the fixed tables.

	SG_ERR_CHECK(  sg_wc_db__info__create_table(pCtx, pDb)  );
	SG_ERR_CHECK(  sg_wc_db__info__write_version(pCtx, pDb)  );
	SG_ERR_CHECK(  sg_wc_db__gid__create_table(pCtx, pDb)  );
	SG_ERR_CHECK(  sg_wc_db__csets__create_table(pCtx, pDb)  );
	SG_ERR_CHECK(  sg_wc_db__branch__create_table(pCtx, pDb)  );
	
	// defer creation of the {tne_L0,pc_L0} tables
	// for the baseline -- we don't know how the caller is
	// going to get the cset.

	// we leave the transaction open for the caller to
	// deal with.

	*ppDb = pDb;
	return;

fail:
	SG_ERR_IGNORE(  sg_wc_db__close_db(pCtx, pDb, bWeCreated)  );
	SG_WC_DB_NULLFREE(pCtx, pDb);
}

//////////////////////////////////////////////////////////////////

/**
 * Associate label (such as "L0") with the CSET with the given HID
 * and build the associated "tne" and optional "pc" tables for it.
 * And optionally populate the "tne" table with the contents of the CSET.
 *
 */
void sg_wc_db__load_named_cset(SG_context * pCtx,
							   sg_wc_db * pDb,
							   const char * pszLabel,
							   const char * pszHidCSet,
							   SG_bool bPopulateTne,
							   SG_bool bCreateTneIndex,
							   SG_bool bCreatePC)
{
	char * pszHidSuperRoot = NULL;
	sg_wc_db__cset_row * pCSetRow = NULL;

	SG_NULLARGCHECK_RETURN( pDb );
	SG_NONEMPTYCHECK_RETURN( pszLabel );
	SG_NONEMPTYCHECK_RETURN( pszHidCSet );

	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx,
													 pDb->pRepo,
													 pszHidCSet,
													 &pszHidSuperRoot)  );

	SG_ERR_CHECK(  sg_wc_db__tx__assert(pCtx, pDb)  );

	// insert/update a row with (label,hid) in tbl_csets
	// and then fetch the row so that we get the current
	// mappings to SQL tables.

	SG_ERR_CHECK(  sg_wc_db__csets__insert(pCtx, pDb, pszLabel, pszHidCSet, pszHidSuperRoot)  );
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pDb, pszLabel, NULL, &pCSetRow)  );

	if (bPopulateTne)
	{
#if TRACE_WC_TNE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__load_named_cset: %s %s %d %d %d\n",
								   pszLabel, pszHidCSet, bPopulateTne, bCreateTneIndex, bCreatePC)  );
#endif

		SG_ERR_CHECK(  sg_wc_db__tne__populate_named_table(pCtx, pDb, pCSetRow, pszHidSuperRoot, bCreateTneIndex)  );
		if (bCreatePC)
		{
			SG_ERR_CHECK(  sg_wc_db__pc__create_table(pCtx, pDb, pCSetRow)  );
			SG_ERR_CHECK(  sg_wc_db__pc__create_index(pCtx, pDb, pCSetRow)  );
		}
	}

fail:
	SG_NULLFREE(pCtx, pszHidSuperRoot);
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
}

//////////////////////////////////////////////////////////////////


