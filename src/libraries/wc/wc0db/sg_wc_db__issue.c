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
 * @file sg_wc_db__issue.c
 *
 * @details Routines associated with the tbl_issues table
 * of the WC DB.
 *
 * This table only exists in the DB when there is an active
 * MERGE.  This table is CREATED during a MERGE and should
 * be DROPPED/DELETED during a COMMIT and/or REVERT-all.
 *
 * 
 * There are 3 parts to each row in this table:
 *
 * [1] The ISSUES identified by MERGE for the item.
 *     This is a CONSTANT VHASH computed by MERGE.
 * 
 * [2] Choices/Values chosen by RESOLVE for the item.
 *     This is a VARIABLE VHASH with fields added
 *     to it as the user makes various choices.
 * 
 * [3] statusFlags_x_xr_xu -- the set of conflicts and
 *     choices.  This is the overall resolved/unresolved
 *     state "__X__" -- one of these bits will be set.
 *     We also have per-choice bits for each required
 *     choice.
 * 
 * 
 * WARNING: This table tries to avoid knowing any *current*
 * state about the items that is already known/stored in
 * tbl_pc.  We want this table to be a static dump of the
 * stuff that MERGE computed (so repo-paths of the inputs
 * from each leaf are here, but the repo-path of the final
 * result shouldn't be because it can change after the
 * merge).
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_db__issue__create_table(SG_context * pCtx,
								   sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(
		sg_sqlite__exec__va(
			pCtx, pDb->psql,
			("CREATE TABLE IF NOT EXISTS tbl_issue"
			 "  ("
			 "    alias_gid    INTEGER PRIMARY KEY REFERENCES tbl_gid( alias_gid ),"
			 "    issue_x_xr_xu INTEGER NOT NULL,"	// See statusFlags
			 "    issue_json   VARCHAR NOT NULL,"	// JSON of pvhIssue
			 "    resolve_json VARCHAR NULL"			// JSON of pvhResolve
			 "  )"))  );

fail:
	return;
}

void sg_wc_db__issue__drop_table(SG_context * pCtx,
								 sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pDb->psql,
									   ("DROP TABLE IF EXISTS tbl_issue"))  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Create an ISSUE in the tbl_issue table.
 * This should only be used by MERGE to register a conflict.
 *
 */
void sg_wc_db__issue__prepare_insert(SG_context * pCtx,
									 sg_wc_db * pDb,
									 SG_uint64 uiAliasGid,
									 SG_wc_status_flags statusFlags_x_xr_xu,
									 const SG_string * pStringIssue,
									 const SG_string * pStringResolve,
									 sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;
	const char * pszResolve = NULL;		// we do not own this

	SG_NULLARGCHECK_RETURN( pDb );
	SG_NULLARGCHECK_RETURN( pStringIssue );
	// pStringResolve is optional

	if (pStringResolve)
		pszResolve = SG_string__sz(pStringResolve);

#if TRACE_WC_ISSUE
	{
		SG_int_to_string_buffer bufui64_a;
		SG_int_to_string_buffer bufui64_b;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_insert: about to insert/replace: [alias %s][x_xr_xu %s]:\n",
								   SG_uint64_to_sz(uiAliasGid, bufui64_a),
								   SG_uint64_to_sz__hex(statusFlags_x_xr_xu, bufui64_b))  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_insert: issue_json: %s\n", SG_string__sz(pStringIssue))  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_insert: resolve_json: %s\n",
								   ((pszResolve && *pszResolve) ? pszResolve : "(null)"))  );
	}
#endif

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT INTO tbl_issue"
									   "  ("
									   "    alias_gid,"
									   "    issue_x_xr_xu,"
									   "    issue_json,"
									   "    resolve_json"
									   "  )"
									   "  VALUES (?, ?, ?, ?)"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 2, statusFlags_x_xr_xu)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text__transient( pCtx, pStmt, 3, SG_string__sz(pStringIssue))  );
	if (pszResolve && *pszResolve)
		SG_ERR_CHECK(  sg_sqlite__bind_text__transient( pCtx, pStmt, 4, pszResolve)  );
	else
		SG_ERR_CHECK(  sg_sqlite__bind_null( pCtx, pStmt, 4)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Update the resolution status of the ISSUE.
 * This should only be used by DELETE or RESOLVE
 * to MARK an item RESOLVED.
 *
 * This version only updates the issueStatus field.
 *
 * WE DO NOT ALTER THE resolve_json FIELD.
 *
 * You might use this version in DELETE (where we *IMPLICITLY*
 * mark a to-be-deleted item as RESOLVED) and don't have any
 * additional details to provide.
 *
 * Can also be used by RESOLVE if you don't care
 * to fill in the pvhResolve details.  (You probably
 * don't want to use this.)
 *
 */
void sg_wc_db__issue__prepare_update_status__s(SG_context * pCtx,
											   sg_wc_db * pDb,
											   SG_uint64 uiAliasGid,
											   SG_wc_status_flags statusFlags_x_xr_xu,
											   sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_ISSUE
	{
		SG_int_to_string_buffer bufui64_a;
		SG_int_to_string_buffer bufui64_b;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_update_status__s: about to update: [alias %s][x_xr_xu %s]:\n",
								   SG_uint64_to_sz(uiAliasGid, bufui64_a),
								   SG_uint64_to_sz__hex(statusFlags_x_xr_xu, bufui64_b))  );
	}
#endif	

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("UPDATE tbl_issue"
									   "  SET"
									   "    issue_x_xr_xu = ?"
									   "  WHERE"
									   "    alias_gid = ?"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, statusFlags_x_xr_xu)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 2, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

/**
 * Update the resolution status *and* the RESOLVE data of
 * the ISSUE.  This should only be used by RESOLVE to update
 * an item.
 *
 * Since an item/issue can have multiple conflict bits set
 * (for example, a MOVE and a RENAME and a EDIT), this routine
 * can be called once as each choice is made, if that makes
 * sense.  BUT IT IS UP TO YOU TO FETCH THE CURRENT STATUS
 * AND -OR- IN WHATEVER NEW/ADDITIONAL RESOLUTION BITS IN THE
 * THE VALUE YOU PASS INTO THIS ROUTINE.  This routine just
 * believes whatever you tell it.
 *
 * WE ALWAYS UPDATE BOTH THE issue_status AND resolve_json
 * FIELDS.  So if you pass a null string, we'll NULL the
 * resolve_json field.
 *
 *
 * NOTE: We have to use __bind_text__transient() because we
 * are queueing a string in the statement and we don't own it.
 * 
 */
void sg_wc_db__issue__prepare_update_status__sr(SG_context * pCtx,
												sg_wc_db * pDb,
												SG_uint64 uiAliasGid,
												SG_wc_status_flags statusFlags_x_xr_xu,
												const SG_string * pStringResolve,
												sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;
	const char * pszResolve = ((pStringResolve) ? SG_string__sz(pStringResolve) : NULL);

#if TRACE_WC_ISSUE
	{
		SG_int_to_string_buffer bufui64_a;
		SG_int_to_string_buffer bufui64_b;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_update_status__sr: about to update: [alias %s][x_xr_xu %s]:\n",
								   SG_uint64_to_sz(uiAliasGid, bufui64_a),
								   SG_uint64_to_sz__hex(statusFlags_x_xr_xu, bufui64_b))  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_update_status__sr: resolve_json: %s\n",
								   ((pszResolve && *pszResolve) ? pszResolve : "(NULL)"))  );
	}
#endif	

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("UPDATE tbl_issue"
									   "  SET"
									   "    issue_x_xr_xu = ?,"
									   "    resolve_json = ?"
									   "  WHERE alias_gid = ?"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, statusFlags_x_xr_xu)  );
	if (pszResolve && *pszResolve)
		SG_ERR_CHECK(  sg_sqlite__bind_text__transient( pCtx, pStmt, 2, pszResolve)  );
	else
		SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt, 2)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 3, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__issue__prepare_delete(SG_context * pCtx,
									 sg_wc_db * pDb,
									 SG_uint64 uiAliasGid,
									 sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_ISSUE
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__issue__prepare_delete: about to delete: [alias %s]\n",
								   SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}
#endif	

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM tbl_issue WHERE alias_gid = ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );

}

//////////////////////////////////////////////////////////////////

static void _sg_wc_db__issue__get_issue(SG_context * pCtx,
										sg_wc_db * pDb,
										SG_uint64 uiAliasGid,
										SG_bool * pbFound,
										SG_wc_status_flags * pStatusFlags_x_xr_xu,
										SG_vhash ** ppvhIssue,
										SG_vhash ** ppvhResolve)
{
	SG_vhash * pvhIssueAllocated = NULL;
	SG_vhash * pvhResolveAllocated = NULL;
	int rc;
	SG_wc_status_flags statusFlags_x_xr_xu;

	SG_NULLARGCHECK_RETURN( pbFound );
	// pStatusFlags_x_xr_xu is optional	
	// ppvhIssue is optional
	// ppvhResolve is optional

	//Caching the prepared statement is a pretty huge performance win.
	if (pDb->pSqliteStmt__get_issue == NULL)
	{
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pDb->pSqliteStmt__get_issue,
									  ("SELECT"
									   "    issue_x_xr_xu,"
									   "    issue_json,"
									   "    resolve_json"
									   "  FROM tbl_issue"
									   "  WHERE alias_gid = ?"))  );
	}
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pDb->pSqliteStmt__get_issue, 1, uiAliasGid)  );

	if ((rc=sqlite3_step(pDb->pSqliteStmt__get_issue)) != SQLITE_ROW)
	{
		if (rc == SQLITE_DONE)
		{
			*pbFound = SG_FALSE;
			if (pStatusFlags_x_xr_xu)
				*pStatusFlags_x_xr_xu = SG_WC_STATUS_FLAGS__ZERO;
			if (ppvhIssue)
				*ppvhIssue = NULL;
			if (ppvhResolve)
				*ppvhResolve = NULL;
			goto done;
		}
		else
		{
			SG_int_to_string_buffer bufui64;
			SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
							(pCtx, "sg_wc_db:tbl_issue can't find alias %s.",
							 SG_uint64_to_sz(uiAliasGid, bufui64))  );
		}
	}

	statusFlags_x_xr_xu = (SG_wc_status_flags)sqlite3_column_int64(pDb->pSqliteStmt__get_issue, 0);
	if (ppvhIssue)
		SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx,
													  &pvhIssueAllocated, 
													  (const char *)sqlite3_column_text(pDb->pSqliteStmt__get_issue, 1))  );
	if (ppvhResolve)
	{
		if (sqlite3_column_type(pDb->pSqliteStmt__get_issue, 2) == SQLITE_NULL)
			pvhResolveAllocated = NULL;
		else
		{
			const char * pszResolveJSON = (const char *)sqlite3_column_text(pDb->pSqliteStmt__get_issue, 2);
			if (pszResolveJSON && *pszResolveJSON)
				SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhResolveAllocated, pszResolveJSON)  );
		}
	}

	*pbFound = SG_TRUE;
	if (pStatusFlags_x_xr_xu)
		*pStatusFlags_x_xr_xu = statusFlags_x_xr_xu;
	if (ppvhIssue)
		*ppvhIssue = pvhIssueAllocated;
	if (ppvhResolve)
		*ppvhResolve = pvhResolveAllocated;

	SG_ERR_CHECK(  sg_sqlite__reset(pCtx,  pDb->pSqliteStmt__get_issue) );
	SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pDb->pSqliteStmt__get_issue)  );
	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhIssueAllocated);
	SG_VHASH_NULLFREE(pCtx, pvhResolveAllocated);
done:
	if (pDb->pSqliteStmt__get_issue)
	{
		SG_ERR_IGNORE(  sg_sqlite__reset(pCtx,  pDb->pSqliteStmt__get_issue) );
		SG_ERR_IGNORE(  sg_sqlite__clear_bindings(pCtx, pDb->pSqliteStmt__get_issue)  );
	}
}

void sg_wc_db__issue__get_issue(SG_context * pCtx,
								sg_wc_db * pDb,
								SG_uint64 uiAliasGid,
								SG_bool * pbFound,
								SG_wc_status_flags * pStatusFlags_x_xr_xu,
								SG_vhash ** ppvhIssue,
								SG_vhash ** ppvhResolve)
{
	// Since the tbl_issue table is only defined when needed (after a merge),
	// we guard the SELECT because we get a "no such table" error from sqlite.

	_sg_wc_db__issue__get_issue(pCtx, pDb, uiAliasGid, pbFound, pStatusFlags_x_xr_xu, ppvhIssue, ppvhResolve);

	if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_ERROR)))
	{
		// TODO 2012/04/18 There are other reasons why we could get this error.
		// TODO            Check the message/codes for 'no such table'.

		SG_context__err_reset(pCtx);

		*pbFound = SG_FALSE;
		if (pStatusFlags_x_xr_xu)
			*pStatusFlags_x_xr_xu = SG_WC_STATUS_FLAGS__ZERO;
		if (ppvhIssue)
			*ppvhIssue = NULL;
		if (ppvhResolve)
			*ppvhResolve = NULL;
	}
	else
	{
		SG_ERR_CHECK_RETURN_CURRENT;
	}
}

//////////////////////////////////////////////////////////////////

#if 1
// TODO 2012/06/06 I want to deprecate this interface.
// TODO
// TODO            Callers wanting to iterate over the set of issues
// TODO            should do a regular/full STATUS and then visit
// TODO            the rows with an __X__ bit set.  This ensures that
// TODO            they get a full/current view of the tree.  Callers
// TODO            that use the SQL method get a view as of the START
// TODO            of the TX (which is fine if you don't/can't need
// TODO            to chain multiple operations in the same TX) but
// TODO            has the potential to be misleading.  Remember that
// TODO            *during* a MERGE or UPDATE, we QUEUE requests to
// TODO            insert/delete/update rows in this table rather than
// TODO            doing it immediately.
// TODO
// TODO            See W8312.
static void _sg_wc_db__issue__foreach(SG_context * pCtx,
									  sg_wc_db * pDb,
									  sg_wc_db__issue__foreach_cb * pfn_cb,
									  void * pVoidData)
{
	sqlite3_stmt * pStmt = NULL;
	int rc;
	SG_uint64 uiAliasGid;
	SG_wc_status_flags statusFlags_x_xr_xu;
	SG_vhash * pvhIssue = NULL;
	SG_vhash * pvhResolve = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid,"
									   "    issue_x_xr_xu,"
									   "    issue_json,"
									   "    resolve_json"
									   "  FROM tbl_issue"))  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		uiAliasGid = (SG_uint64)sqlite3_column_int64(pStmt, 0);
		statusFlags_x_xr_xu = (SG_wc_status_flags)sqlite3_column_int64(pStmt, 1);
		SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhIssue,
													  (const char *)sqlite3_column_text(pStmt, 2))  );
		if (sqlite3_column_type(pStmt, 3) == SQLITE_NULL)
			pvhResolve = NULL;
		else
		{
			const char * pszResolveJSON = (const char *)sqlite3_column_text(pStmt, 3);
			if (pszResolveJSON && *pszResolveJSON)
				SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhResolve, pszResolveJSON)  );
		}

		// pass pvhIssue and pvhResolve by address so that the callback can
		// steal them if it wants to.

		SG_ERR_CHECK(  (*pfn_cb)(pCtx, pVoidData,
								 uiAliasGid, statusFlags_x_xr_xu,
								 &pvhIssue, &pvhResolve)  );
		SG_VHASH_NULLFREE(pCtx, pvhIssue);
		SG_VHASH_NULLFREE(pCtx, pvhResolve);
	}
	if (rc != SQLITE_DONE)
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_VHASH_NULLFREE(pCtx, pvhIssue);
	SG_VHASH_NULLFREE(pCtx, pvhResolve);
}

void sg_wc_db__issue__foreach(SG_context * pCtx,
							  sg_wc_db * pDb,
							  sg_wc_db__issue__foreach_cb * pfn_cb,
							  void * pVoidData)
{
	// Iterate over the entire tbl_issue table.
	// 
	// Since the tbl_issue table is only defined when needed (after a merge),
	// we guard the SELECT because we get a "no such table" error from sqlite.
	//
	// Silently eat that error so that it just looks to our callers like there
	// are no issues.

	_sg_wc_db__issue__foreach(pCtx, pDb, pfn_cb, pVoidData);

	if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_ERROR)))
	{
		// TODO 2012/04/18 There are other reasons why we could get this error.
		// TODO            Check the message/codes for 'no such table'.

		SG_context__err_reset(pCtx);
	}
	else
	{
		SG_ERR_CHECK_RETURN_CURRENT;
	}
}
#endif // see W8312
