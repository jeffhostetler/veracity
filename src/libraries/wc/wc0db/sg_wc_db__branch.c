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

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * The "tbl_branch" table contains a SINGLE ROW with the name
 * of the attached branch (if set).
 *
 * If the table does not have a row or the name value in that
 * row is null or empty, we say the WD is detached.
 *
 */

#define ID_KEY    0

//////////////////////////////////////////////////////////////////

void sg_wc_db__branch__create_table(SG_context * pCtx,
									sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE TABLE tbl_branch"
											   "  ("
											   "    id   INTEGER PRIMARY KEY," 
											   "    name VARCHAR NULL"
											   "  )"))  );
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch the current branch if attached.
 * Return NULL if detached.
 *
 */
void sg_wc_db__branch__get_branch(SG_context * pCtx,
								  sg_wc_db * pDb,
								  char ** ppszBranchName)
{
	sqlite3_stmt * pStmt = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    name"	// 0
									   "  FROM tbl_branch"
									   "  WHERE id = ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, ID_KEY)  );
	rc = sqlite3_step(pStmt);
	switch (rc)
	{
	case SQLITE_ROW:
		if (sqlite3_column_type(pStmt, 0) == SQLITE_NULL)
			*ppszBranchName = NULL;
		else
			SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 0), ppszBranchName)  );
		break;

	case SQLITE_DONE:
		*ppszBranchName = NULL;
		break;

	default:
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_branch can't get branch name.")  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__branch__get_branch: %s\n",
							   ((*ppszBranchName) ? (*ppszBranchName) : "<detached>"))  );
#endif

	return;

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

/**
 * Attach/tie the WD to the given branch.
 * 
 * We optionally validate the name (so that they can attach to
 * old pre-2.0 branches that allowed punctuation characters).
 *
 * We optionally verify that the branch name does/does not exist
 * (corresponding to --attach or --attach-new).
 *
 * We don't care one way or the other and only provide these
 * options as a service to the caller.  You probably don't need
 * to use them since the caller should have already validated/verified
 * all this before while they were parsing the user's input.
 *
 */
void sg_wc_db__branch__attach(SG_context * pCtx,
							  sg_wc_db * pDb,
							  const char * pszBranchName,
							  SG_vc_branches__check_attach_name__flags flags,
							  SG_bool bValidateName)
{
	sqlite3_stmt * pStmt = NULL;
	char * pszNormalizedBranchName = NULL;	// however, we always normalize the name

	SG_NONEMPTYCHECK_RETURN( pszBranchName );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__branch__attach: [flags %d][validate %d] %s\n",
							   (SG_uint32)flags, bValidateName, pszBranchName)  );
#endif

	SG_ERR_CHECK(  SG_vc_branches__check_attach_name(pCtx, pDb->pRepo, pszBranchName,
													 flags, bValidateName,
													 &pszNormalizedBranchName)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO tbl_branch ( id, name ) VALUES ( ?, ? )"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt, 1, ID_KEY)  );
	// probably unnecessary since __check_attach_name should have thrown.
	if (pszNormalizedBranchName && *pszNormalizedBranchName)
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszNormalizedBranchName)  );
	else
		SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt, 2)  );
	
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_NULLFREE(pCtx, pszNormalizedBranchName);
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_NULLFREE(pCtx, pszNormalizedBranchName);
}

void sg_wc_db__branch__detach(SG_context * pCtx,
							  sg_wc_db * pDb)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__branch__detach:\n")  );
#endif

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO tbl_branch ( id, name ) VALUES ( ?, ? )"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt, 1, ID_KEY)  );
	SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt, 2)  );
	
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

