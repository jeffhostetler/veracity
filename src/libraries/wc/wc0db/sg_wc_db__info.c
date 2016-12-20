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
 * @file sg_wc_db__info.c
 *
 * @details Routines associated with the INFO table of the WC DB.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * The "tbl_info" table contains a SINGLE ROW with the DB VERSION
 * number.
 *
 */

#define DB_VERSION			(3)

//////////////////////////////////////////////////////////////////

void sg_wc_db__info__create_table(SG_context * pCtx, sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE TABLE tbl_info ( version INTEGER PRIMARY KEY )"))  );
}

void sg_wc_db__info__write_version(SG_context * pCtx, sg_wc_db * pDb)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO tbl_info ( version ) VALUES (%d)"),
									  DB_VERSION)  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void sg_wc_db__info__read_version(SG_context * pCtx, sg_wc_db * pDb, SG_uint32 * pDbVersion)
{
	sqlite3_stmt * pStmt = NULL;
	int rc;
	SG_uint32 count;
	SG_uint32 version_found;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT COUNT(*) FROM tbl_info"))  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_info can't get count.")  );
	count = (SG_uint32)sqlite3_column_int(pStmt, 0);
	if (count != 1)
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "sg_wc_db:tbl_info has %d rows.", count)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT version FROM tbl_info"))  );
	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_info can't get version.")  );
	version_found = (SG_uint32)sqlite3_column_int(pStmt, 0);
	if (version_found != DB_VERSION)
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "sg_wc_db:tbl_info has version %d.", version_found)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	if (pDbVersion)
		*pDbVersion = version_found;
	return;
		
fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}
