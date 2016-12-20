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
 * @file sg_wc_db__gid.c
 *
 * @details Routines associated with the GID table in the WC DB.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * The "tbl_gid" table contains a map[ alias <=> gid ].
 * This is used by foreign keys in the actual data tables
 * to refer to a gid without wasting all the space.  No
 * rocket science here, but more of a terminology problem.
 * I'm going to use the term "alias" rather than the typical
 * "id" field because we have too many gids and hids as it
 * is now.
 *
 * Note that the entire WC DB is private to a
 * specific working directory instance and can be
 * deleted/recreated at any time, so these aliases
 * are only valid for the life of this DB.
 *
 * We ACCUMULATE gids/aliases as an append-only table
 * because items under version control aren't deleted
 * that often.
 *
 */

//////////////////////////////////////////////////////////////////

void sg_wc_db__gid__create_table(SG_context * pCtx, sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE TABLE tbl_gid"
											   "  ("
											   "    alias_gid INTEGER PRIMARY KEY AUTOINCREMENT,"
											   "    gid       VARCHAR NOT NULL,"
											   "    tmp       INTEGER NOT NULL"
											   "  )"))  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE UNIQUE INDEX index_gid ON tbl_gid ( gid )"))  );

	// put (0,"*undefined*") in the table as a GID string so that we
	// have a mapping for a fictional temporary id (for FOUND and
	// IGNORED items until we decide they need a real/permanent id).

	SG_ERR_CHECK_RETURN(  sg_wc_db__gid__insert_known(pCtx, pDb, SG_WC_DB__ALIAS_GID__UNDEFINED, SG_WC_DB__GID__UNDEFINED)  );

	// put (1,"*null*)" in the table as a GID string so that we have a mapping
	// for a fictional parent of the root directory.  this lets us
	// keep the non-null constraint on the column and avoid having
	// to special case it in the SQL.

	SG_ERR_CHECK_RETURN(  sg_wc_db__gid__insert_known(pCtx, pDb, SG_WC_DB__ALIAS_GID__NULL_ROOT, SG_WC_DB__GID__NULL_ROOT)  );
}

/**
 * Insert a row into the "tbl_gid" table WITH A FIXED AND KNOWN ALIAS.
 * We only use this for the special pseudo-gids.  This overrides the
 * auto-increment feature on the alias_gid field.
 */
void sg_wc_db__gid__insert_known(SG_context * pCtx,
								 sg_wc_db * pDb,
								 SG_uint64 uiAliasGid,
								 const char * pszGid)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO tbl_gid ( alias_gid, gid, tmp ) VALUES (?, ?, 0)"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,  pStmt, 2, pszGid)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

static void _sg_wc_db__gid__insert(SG_context * pCtx,
								   sg_wc_db * pDb,
								   const char * pszGid,
								   SG_bool bIsTmp)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR IGNORE INTO tbl_gid ( gid, tmp ) VALUES ( ?, ? )"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszGid)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int( pCtx, pStmt, 2, bIsTmp)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void sg_wc_db__gid__insert(SG_context * pCtx,
						   sg_wc_db * pDb,
						   const char * pszGid)
{
	SG_ERR_CHECK_RETURN(  _sg_wc_db__gid__insert(pCtx, pDb, pszGid, SG_FALSE)  );
}

/**
 * Generate a NEW GID, add it to the tbl_gid, and
 * return the alias to it.
 *
 */
void sg_wc_db__gid__insert_new(SG_context * pCtx,
							   sg_wc_db * pDb,
							   SG_bool bIsTmp,
							   SG_uint64 * puiAliasGidNew)
{
	char bufGid[SG_GID_BUFFER_LENGTH+1];

	SG_ERR_CHECK_RETURN(  SG_gid__generate(pCtx, bufGid, sizeof(bufGid))  );
	SG_ERR_CHECK_RETURN(  _sg_wc_db__gid__insert(pCtx, pDb, bufGid, bIsTmp)  );
	SG_ERR_CHECK_RETURN(  sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, bufGid, puiAliasGidNew)  );

	if (bIsTmp)
		pDb->nrTmpGids++;
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__gid__prepare_toggle_tmp_stmt(SG_context * pCtx,
											sg_wc_db * pDb,
											SG_uint64 uiAliasGid,
											SG_bool bIsTmp,
											sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_DB
	{
		SG_int_to_string_buffer bufui64;

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "sg_wc_db__gid__prepare_toggle_tmp_stmt: %s --> %c\n",
								   SG_uint64_to_sz(uiAliasGid, bufui64),
								   ((bIsTmp) ? 'T' : 'F'))  );
	}
#endif

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("UPDATE tbl_gid SET tmp = ? WHERE alias_gid = ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int( pCtx, pStmt, 1, bIsTmp)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 2, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__gid__get_alias_from_gid(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszGid,
									   SG_uint64 * puiAliasGid)
{
	sqlite3_stmt * pStmt = NULL;
	SG_uint64 uiAliasGid = 0;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT alias_gid FROM tbl_gid WHERE gid = ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1,
										((pszGid && *pszGid) ? pszGid : SG_WC_DB__GID__NULL_ROOT))  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_gid can't find gid %s.", pszGid)  );
	
	uiAliasGid = (SG_uint64)sqlite3_column_int64(pStmt, 0);
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	*puiAliasGid = uiAliasGid;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void sg_wc_db__gid__get_gid_from_alias(SG_context * pCtx,
									   sg_wc_db * pDb,
									   SG_uint64 uiAliasGid,
									   char ** ppszGid)
{
	char * pszGid = NULL;
	int rc;

	// TODO 2011/08/01 Should this check for "*null*" and offer to return NULL ?

	//Caching the prepared statement is a pretty huge performance win.
	if (pDb->pSqliteStmt__get_gid_from_alias == NULL)
	{
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pDb->pSqliteStmt__get_gid_from_alias,
									  ("SELECT gid FROM tbl_gid WHERE alias_gid = ?"))  );
	}
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pDb->pSqliteStmt__get_gid_from_alias, 1, uiAliasGid)  );

	if ((rc=sqlite3_step(pDb->pSqliteStmt__get_gid_from_alias)) != SQLITE_ROW)
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_gid can't find alias %s.",
						 SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}

	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pDb->pSqliteStmt__get_gid_from_alias, 0), &pszGid)  );
	SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pDb->pSqliteStmt__get_gid_from_alias)  );
	SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pDb->pSqliteStmt__get_gid_from_alias)  );

	*ppszGid = pszGid;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__reset(pCtx, pDb->pSqliteStmt__get_gid_from_alias)  );
	SG_ERR_IGNORE(  sg_sqlite__clear_bindings(pCtx, pDb->pSqliteStmt__get_gid_from_alias)  );
	SG_NULLFREE(pCtx, pszGid);
}

void sg_wc_db__gid__get_gid_from_alias2(SG_context * pCtx,
										sg_wc_db * pDb,
										SG_uint64 uiAliasGid,
										char ** ppszGid,
										SG_bool * pbIsTmp)
{
	sqlite3_stmt * pStmt = NULL;
	char * pszGid = NULL;
	int rc;
	SG_bool bIsTmp;

	// TODO 2011/08/01 Should this check for "*null*" and offer to return NULL ?

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT gid,tmp FROM tbl_gid WHERE alias_gid = ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_gid can't find alias %s.",
						 SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}

	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 0), &pszGid)  );
	bIsTmp = (sqlite3_column_int(pStmt, 1) != 0);
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	*ppszGid = pszGid;
	*pbIsTmp = bIsTmp;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_NULLFREE(pCtx, pszGid);
}

void sg_wc_db__gid__get_gid_from_alias3(SG_context * pCtx,
										sg_wc_db * pDb,
										SG_uint64 uiAliasGid,
										char ** ppszGid)
{
	char * pszGid = NULL;
	SG_bool bIsTmp;

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias2(pCtx, pDb, uiAliasGid, &pszGid, &bIsTmp)  );
	if (bIsTmp)
	{
		SG_ASSERT( (pszGid[0] == 'g') );
		pszGid[0] = 't';
	}

	SG_RETURN_AND_NULL(pszGid, ppszGid);

fail:
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__gid__get_or_insert_alias_from_gid(SG_context * pCtx,
												 sg_wc_db * pDb,
												 const char * pszGid,
												 SG_uint64 * puiAliasGid)
{
	SG_bool bNotAlreadyPresent;
	
	sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, pszGid, puiAliasGid);
	bNotAlreadyPresent = SG_CONTEXT__HAS_ERR(pCtx);

	if (bNotAlreadyPresent)
	{
		SG_context__err_reset(pCtx);

		SG_ERR_CHECK(  sg_wc_db__gid__insert(pCtx, pDb, pszGid)  );
		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, pszGid, puiAliasGid)  );
	}

#if TRACE_WC_GID
	{
		SG_int_to_string_buffer bufui64;

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "GID: get_or_insert [%s] ==> %s [new %d]\n",
								   pszGid,
								   SG_uint64_to_sz(*puiAliasGid,bufui64),
								   bNotAlreadyPresent)  );
	}
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__gid__delete_all_tmp(SG_context * pCtx,
								   sg_wc_db * pDb)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__gid__delete_all_tmp: had %d tmp gids.\n",
							   pDb->nrTmpGids)  );
#endif

	if (pDb->nrTmpGids == 0)
		return;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM tbl_gid WHERE tmp != 0"))  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	pDb->nrTmpGids = 0;

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}
