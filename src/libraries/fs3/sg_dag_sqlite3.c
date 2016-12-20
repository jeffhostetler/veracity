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
 * @file sg_dag_sqlite3.c
 *
 * @details PRIVATE Routines for the SQLITE3 implementation of the DAG
 *
 * Note: dagnode generation is now SG_int32 rather than SG_int64.
 */

//////////////////////////////////////////////////////////////////

#include "sg_fs3__private.h"

//////////////////////////////////////////////////////////////////
// sqlite3 has a way to spin and retry an operation if another
// process has a table/db locked.  we use the keep trying for
// upto n-milliseconds version.
//
// WARNING: As of 3.5.6 (at least), this only works some of the
// WARNING: time.  It seems to let COMMIT TRANSACTIONS routinely
// WARNING: work (during parallel runs of my
// WARNING:    testapps/dag_stress_1/add_nodes
// WARNING: test app.
// WARNING:
// WARNING: It *DID NOT* prevent the first INSERT (after the initial
// WARNING: BEGIN TRANSACTION) of the 2nd..nth parallel instance of the
// WARNING: test.  The builtin busy callback wasn't even used.
// WARNING:
// WARNING: The SQLITE3 docs say that statements can return SQLITE_BUSY
// WARNING: immediately (rather than waiting and retrying) *IF* they
// WARNING: can detect deadlock or other locking problems.  Installing
// WARNING: a busy callback reduces the odds of getting a BUSY, but
// WARNING: does not prevent it.

#define MY_BUSY_TIMEOUT_MS      (30000)

//////////////////////////////////////////////////////////////////

#define FAKE_PARENT			"*root*"

#define CURRENT_VERSION		1

#define BUF_LEN_TABLE_NAME (100 + SG_DAGNUM__BUF_MAX__HEX)
#define DAG_EDGES_TABLE_NAME "dag_edges_"
#define DAG_INFO_TABLE_NAME "dag_info_"
#define DAG_LEAVES_TABLE_NAME "dag_leaves_"
#define DAG_AUDITS_TABLE_NAME "dag_audits_"

//////////////////////////////////////////////////////////////////

static void _get_table_name(SG_context* pCtx, const char* pszTablePrefix, SG_uint64 iDagNum, char* bufTableName)
{
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_ERR_CHECK_RETURN(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );
	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx, bufTableName, BUF_LEN_TABLE_NAME, "%s_%s", pszTablePrefix, buf_dagnum)  );
}

void sg_dag_sqlite3__create_deferred_indexes(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum)
{
	char bufTableName[BUF_LEN_TABLE_NAME];

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE UNIQUE INDEX IF NOT EXISTS idx_pk_%s ON %s (child_id, parent_id)", bufTableName, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE INDEX IF NOT EXISTS %s_index_parent_id ON %s ( parent_id, child_id )", bufTableName, bufTableName)  );

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE UNIQUE INDEX IF NOT EXISTS %s_child_id ON %s ( child_id )", bufTableName, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE INDEX IF NOT EXISTS %s_generation ON %s ( generation )", bufTableName, bufTableName)  );

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, iDagNum, bufTableName)  );
	// TODO do we really need this index?  the leaves table is usually quite small
    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE UNIQUE INDEX IF NOT EXISTS idx_%s ON %s (child_id)", bufTableName, bufTableName)  );

fail:
	;
}

static void _create_dag_tables(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_uint32 flags)
{
	char bufTableName[BUF_LEN_TABLE_NAME];

	// The first table is the "dag_edges".  This will have one row for
	// each EDGE in the DAG.  These are child-->parent links.  Since
	// a child may have more than one parent (and likewise, a parent
	// may have more than one child), these individual columns are not
	// marked UNIQUE and are not the PRIMARY KEY.  Rather, we make the
	// PRIMARY KEY be the PAIR, so that each EDGE can only occur once.
	//
	// Naturally, the child_id must not be NULL because it represents
	// the HID of the CHANGESET/DAGNODE in question.  But the parent_id
	// *could* be NULL (because the initial checkin doesn't have a parent),
	// but we also make it NOT NULL because there are problems/disputes in
	// SQL with unique and null in multi-column constraints (see
	// http://www.sql.org/sql-database/postgresql/manual/ddl-constraints.html
	// sections 2.4.3 Unique Contraints and 2.4.4 Primary Keys) and will
	// have the C code use a special value to indicate no-parent.
	//
	// By creating an EDGE table with 2 columns (rather than a node table
	// with the parents packed in some kind of vector) we should be able to
	// search the DAG in both directions - find all parents of a node
	// and find all children of a node.  We currently believe that if
	// we always walk the graph starting at LEAF nodes, we'll only
	// need the former, but let's not rule out the latter.

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, ("CREATE TABLE %s"
		"  ("
		"    child_id VARCHAR NOT NULL,"
		"    parent_id VARCHAR NOT NULL"
		"  )"), bufTableName)  );

	// The second table is the "dag_leaves".  This will contain a row
	// for each LEAF node in the DAG.

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql,
		("CREATE TABLE %s"
		"  ("
		"    child_id VARCHAR NOT NULL"
		"    )"), bufTableName)  );

	// The third table is the "dag_info".  This will contain a row for
	// each DAGNODE and various information for it.

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql,
		("CREATE TABLE %s"
		"  ("
		"    instance_revno INTEGER PRIMARY KEY AUTOINCREMENT,"
		"    child_id VARCHAR NOT NULL,"
		"    generation INTEGER"
		"  )"), bufTableName)  );

    {
        char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
        SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );
        SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "INSERT INTO dags (dagnum) VALUES ('%s')", buf_dagnum)  );
    }

    if (!(flags & SG_REPO_TX_FLAG__CLONING))
    {
        SG_ERR_CHECK(  sg_dag_sqlite3__create_deferred_indexes(pCtx, psql, iDagNum)  );
    }

	/* fall through */
fail:
	;
}

static void _get_version(SG_context* pCtx, sqlite3* psql, SG_uint32* piVersion)
{
	SG_int32 iVersion;

	sg_sqlite__exec__va__int32(pCtx, psql, &iVersion, 
		"SELECT version FROM dag_storage_version");
	if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_ERROR)))
	{
		SG_ERR_DISCARD;
		*piVersion = 0;
	}
	else
	{
		SG_ERR_CHECK_RETURN_CURRENT;
		*piVersion = (SG_uint32)iVersion;
	}
}

//////////////////////////////////////////////////////////////////

void SG_dag_sqlite3__create(SG_context* pCtx, const SG_pathname* pPathnameSqlDb, sqlite3** ppNew)
{
	sqlite3 * psql = NULL;
	int rc;
	SG_bool bInTx = SG_FALSE;

    SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPathnameSqlDb, SG_SQLITE__SYNC__NORMAL, &psql)  );
    rc = sqlite3_busy_timeout(psql,MY_BUSY_TIMEOUT_MS);
	if (rc)
		SG_ERR_THROW(SG_ERR_SQLITE(rc));

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
		"CREATE TABLE dag_storage_version (version INTEGER)")  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, 
		"INSERT INTO dag_storage_version (version) VALUES (%u)", CURRENT_VERSION)  );

	/* Dag tables get created as they're needed.  A rew repository needs only
	   the table to list the dags that already exist, so we create that here.
	   As dags are added to this repo, rows are added to this table. */
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, 
		"CREATE TABLE dags (dagnum VARCHAR PRIMARY KEY)")  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	bInTx = SG_FALSE;

	*ppNew = psql;
	
	return;

fail:
    /* TODO delete the sqldb we just created?  Probably not */
	if (psql && bInTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );

    SG_ERR_IGNORE(  SG_dag_sqlite3__nullclose(pCtx, &psql)  );
}

void SG_dag_sqlite3__open(SG_context* pCtx, const SG_pathname* pPathnameSqlDb, sqlite3** ppNew)
{
	sqlite3 * p = NULL;
	int rc;
	SG_uint32 iCurrentVersion = 0;

	SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPathnameSqlDb,  SG_SQLITE__SYNC__NORMAL, &p)  );
	rc = sqlite3_busy_timeout(p,MY_BUSY_TIMEOUT_MS);
	if (rc)
		SG_ERR_THROW(SG_ERR_SQLITE(rc));

	SG_ERR_CHECK(  _get_version(pCtx, p, &iCurrentVersion)  );
	if (iCurrentVersion > CURRENT_VERSION)
	{
		SG_ERR_THROW2(SG_ERR_UNKNOWN_DAG_STORAGE_VERSION, 
			(pCtx, "sqlite dag storage version %u", iCurrentVersion)  );
	}

	*ppNew = p;

	return;

fail:
	SG_ERR_IGNORE(  SG_dag_sqlite3__nullclose(pCtx, &p)  );
}

//////////////////////////////////////////////////////////////////

void SG_dag_sqlite3__nullclose(SG_context* pCtx, sqlite3** ppsql)
{
    sqlite3* psql = NULL;

	SG_NULLARGCHECK_RETURN(ppsql);

    psql = *ppsql;
    if (!psql)
		return;

	sg_sqlite__close(pCtx, psql);

    *ppsql = NULL;

	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}


//////////////////////////////////////////////////////////////////

typedef struct {
    sqlite3* psql;
    SG_uint64 iDagNum;
    sqlite3_stmt* pStmt_not_leaf;
    sqlite3_stmt* pStmt_leaf;
    sqlite3_stmt* pStmt_edge;
    sqlite3_stmt* pStmt_info;
    SG_vhash* pvh_leaves;
    SG_uint32 flags;
} my_state;

static void free_my_state_stuff(
        SG_context* pCtx,
        my_state* pst
        )
{
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pst->pStmt_not_leaf)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pst->pStmt_leaf)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pst->pStmt_edge)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pst->pStmt_info)  );
    SG_VHASH_NULLFREE(pCtx, pst->pvh_leaves);

fail:
    ;
};

static void _my_is_known(
	SG_context* pCtx, 
    my_state* pst,
	const char * szHid, 
	sqlite3_stmt** ppStmt, /* [in/out] If you're going to call this repeatedly FOR THE SAME DAGNUM, you should re-use the statement. Otherwise specify NULL. */
	SG_bool * pbIsKnown, 
	SG_int32 * pGen, 
	SG_uint32* piRevno)
{
	// search the DAG_INFO table and see if the given HID is known.
	// this should be equivalent to searching for HID in the child_id column of DAG_EDGES.

	char bufTableName[BUF_LEN_TABLE_NAME];
	sqlite3_stmt * pStmtInfo = NULL;
	SG_int32 gen = -1;
	SG_uint32 revno = 0;
	SG_uint32 nrInfoRows = 0;
	int rc;
	SG_bool bTableExists = SG_TRUE;

	if (ppStmt == NULL || *ppStmt == NULL)
	{
		SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, pst->iDagNum, bufTableName)  );

		sg_sqlite__prepare(pCtx, pst->psql, &pStmtInfo,
			"SELECT instance_revno, generation FROM %s WHERE child_id = ?", bufTableName);

		/* We get SQLITE_ERROR if the dag tables don't exist yet.
		   If we confirm that the table doesn't exist, we return
		   the "no such node" values.  Otherwise we "throw." */
		if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_ERROR)))
		{
			SG_ERR_IGNORE(  SG_sqlite__table_exists(pCtx, pst->psql, bufTableName, &bTableExists)  );
			if (!bTableExists)
				SG_ERR_DISCARD;
		}
		SG_ERR_CHECK_CURRENT;
	}
	else
	{
		pStmtInfo = *ppStmt;
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmtInfo)  );
		bTableExists = SG_TRUE;
	}
	
	if (bTableExists)
	{
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmtInfo,1,szHid)  );

		while ((rc=sqlite3_step(pStmtInfo)) == SQLITE_ROW)
		{
			if (nrInfoRows == 0)
			{
				revno = (SG_uint32)sqlite3_column_int(pStmtInfo,0);
				gen = (SG_int32)sqlite3_column_int(pStmtInfo,1);
				nrInfoRows++;
			}
			else			// this can't happen because of primary key.
			{
				SG_ERR_THROW(SG_ERR_DAG_NOT_CONSISTENT);
			}
		}
		if (rc != SQLITE_DONE) 
		{
			SG_ERR_THROW(SG_ERR_SQLITE(rc));
		}
	}

	if (ppStmt)
	{
		*ppStmt = pStmtInfo;
		pStmtInfo= NULL;
	}
	else
		SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtInfo)  );

	*pbIsKnown = (nrInfoRows == 1);
	*pGen = gen;
	if (piRevno)
		*piRevno = revno;

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtInfo)  );
}

#if 0
static void _my_mark_as_not_leaf(SG_context* pCtx, my_state* pst, const char* pszidHid)
{
	// delete HID from the DAG_LEAVES TABLE.  this marks a DAGNODE as NOT a LEAF.
	// WE TRUST THE CALLER TO KNOW WHAT THEY'RE DOING.
	//
	// if the HID is not present in the DAG_LEAVES TABLE, we silently ignore
	// the request.

    if (!pst->pStmt_not_leaf)
    {
        char bufTableName[BUF_LEN_TABLE_NAME];

        SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, pst->iDagNum, bufTableName)  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pst->psql,&pst->pStmt_not_leaf,
            "DELETE FROM %s WHERE child_id = ?", bufTableName)  );
    }
    else
    {
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pst->pStmt_not_leaf)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pst->pStmt_not_leaf)  );
    }
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pst->pStmt_not_leaf,1,pszidHid)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pst->pStmt_not_leaf,SQLITE_DONE)  );

fail:
    ;
}
#endif

static void _my_mark_as_leaf(SG_context* pCtx, my_state* pst, const char* pszidHid)
{
	// insert HID as a row in DAG_LEAVES TABLE.  this marks the DAGNODE as a LEAF.
	// WE TRUST THE CALLER TO KNOW WHAT THEY'RE DOING.
	//
	// if the HID is already a LEAF, we silently ignore the request.

	char bufTableName[BUF_LEN_TABLE_NAME];

    if (!pst->pStmt_leaf)
    {
        SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, pst->iDagNum, bufTableName)  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pst->psql, &pst->pStmt_leaf,
            "INSERT OR IGNORE INTO %s (child_id) VALUES (?)", bufTableName)  );
    }
    else
    {
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pst->pStmt_leaf)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pst->pStmt_leaf)  );
    }
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pst->pStmt_leaf,1,pszidHid)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pst->pStmt_leaf,SQLITE_DONE)  );

fail:
    ;
}

static void _my_store_edge(
        SG_context* pCtx, 
        my_state* pst,
        const char * szHidChild, 
        const char * szHidParent
        )
{
	char bufTableName[BUF_LEN_TABLE_NAME];

	// insert edge.  we expect either SQLITE_DONE (mapped to SG_ERR_OK)
	// or an SQLITE_CONSTRAINT on a duplicate record.

	if (!pst->pStmt_edge)
	{
        SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, pst->iDagNum, bufTableName)  );

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pst->psql, &pst->pStmt_edge,
            "INSERT INTO %s (child_id, parent_id) VALUES (?, ?)", bufTableName)  );
	}
	else
	{
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pst->pStmt_edge)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pst->pStmt_edge)  );
	}
	
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pst->pStmt_edge,1,szHidChild)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pst->pStmt_edge,2,szHidParent)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pst->pStmt_edge,SQLITE_DONE)  );

fail:
    ;
}

static void _my_store_info(SG_context* pCtx, my_state* pst, const SG_dagnode * pDagnode)
{
	// add record to DAG_INFO table.

	const char* pszidHidChild = NULL;
	SG_int32 generation;

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pDagnode, &pszidHidChild)  );
	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pDagnode,&generation)  );

	// insert info.  we expect either SQLITE_DONE (mapped to SG_ERR_OK)
	// or an SQLITE_CONTRAINT on a duplicate record.

    if (!pst->pStmt_info)
    {
        char bufTableName[BUF_LEN_TABLE_NAME];

        SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, pst->iDagNum, bufTableName)  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pst->psql, &pst->pStmt_info,
            "INSERT INTO %s (child_id, generation) VALUES (?, ?)", bufTableName)  );
    }
    else
    {
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pst->pStmt_info)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pst->pStmt_info)  );
    }
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pst->pStmt_info,1,pszidHidChild)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pst->pStmt_info,2,generation)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pst->pStmt_info,SQLITE_DONE)  );

     return;

fail:
	    // if something failed, we should not reuse the prepared stmt
            SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pst->pStmt_info)  );
}

static void _my_store_initial_dagnode(SG_context* pCtx, my_state* pst, const SG_dagnode * pDagnode)
{
	// special case for the initial CHANGESET/DAGNODE.
	//
	// create fake edges from this dagnode to the root.
	// make this dagnode as a leaf.
	//
	// we hold the TX open as we create the EDGES and update the LEAVES, so
	// order doesn't really matter.
	//
	// WARNING: Due to the distributed nature of the system we ***CANNOT***
	// WARNING: insist that this node be the SINGLE ABSOLUTE/UNIQUE initial checkin.
	// WARNING: That is, the only dagnode that has no parents.  We have to
	// WARNING: allow for the possibility that someone created a repository
	// WARNING: and cloned it BEFORE the initial checkin.  Each instance of
	// WARNING: the repository would allow an initial checkin.  A PUSH/PULL
	// WARNING: between them would then give us a disconnected DAG....
	//
	// we store the DAG in a DAG_EDGES table with one
	// row for each EDGE from the DAGNODE to a parent
	// DAGNODE.
	//
	// Since this DAGNODE is the initial checkin, there
	// are no parents, and hence, no edges from it.
	//
	// We create a special parent "*root*" and make a
	// fake edge to it.  This keeps us from having to
	// rely on an SQL NULL value (which may or may not
	// work as expected when in a KEY).

	const char * szHidChild;

	// try to add record to DAG_INFO table.

	_my_store_info(pCtx, pst, pDagnode);
	if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_ERROR)))
	{
		/* Create the DAG tables if they don't yet exist. */

		SG_bool bExists = SG_TRUE;
		char bufTableName[BUF_LEN_TABLE_NAME];
		SG_ERR_IGNORE(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, pst->iDagNum, bufTableName)  );


		/* SQLITE_ERROR is pretty generic, so first we confirm that the table
		   does not exist. */
		SG_ERR_IGNORE(  SG_sqlite__table_exists(pCtx, pst->psql, bufTableName, &bExists)  );
		if (!bExists)
		{
			SG_ERR_DISCARD;

			SG_ERR_CHECK(  _create_dag_tables(pCtx, pst->psql, pst->iDagNum, pst->flags)  );
			_my_store_info(pCtx, pst, pDagnode);
		}
	}
	else if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_CONSTRAINT)))
	{
		// duplicate row constraint in DAG_INFO.  the DB already had this
		// record.
		//
		// we assume that it contains the DAG_EDGES records.
		//
		// we cannot make any statements about whether
		// or not this node is a leaf.
		//
		// return special already-exists error code; this error may
		// be ignored by caller.

		SG_ERR_RESET_THROW(SG_ERR_DAGNODE_ALREADY_EXISTS );
	}
	SG_ERR_CHECK_CURRENT;  // otherwise, we have an unknown SQL error.

	// we just added a *NEW* node to DAG_INFO, so we must have
	// a new (to us) dagnode.  continue adding stuff to the
	// other tables.

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pDagnode,&szHidChild)  );

	// try to add record to DAG_EDGES table.  if this fails because
	// an actual error --or-- because of a constraint (duplicate key)
	// error, something is wrong and we just give up.

	SG_ERR_CHECK(  _my_store_edge(pCtx, pst, szHidChild,FAKE_PARENT)  );

	// we juat added a *NEW* node to the DAG and, by definition,
	// no other node in the graph references us, so therefore
	// it is a leaf.

	//SG_ERR_CHECK(  _my_mark_as_leaf(pCtx, pst, szHidChild)  );
    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pst->pvh_leaves, szHidChild)  );

	return;

fail:
	return;
}

static void _my_store_dagnode_with_parents(SG_context* pCtx, my_state* pst, const SG_dagnode * pDagnode)
{
	// store non-initial CHANGESET/DAGNODE.
	//
	// we store an EDGE from child to each parent.
	// we mark the parents as not leaves.
	// we mark the child as a leaf.
	//
	// we hold the TX open as we create the EDGES and update the LEAVES, so
	// order doesn't really matter.

	const char * szHidChild;
	SG_uint32 k, kLimit;
	const char** paParents = NULL;

	// try to add record to DAG_INFO table.

	_my_store_info(pCtx, pst, pDagnode);
	if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_CONSTRAINT)))
	{
		// duplicate row constraint in DAG_INFO.  the DB already had this
		// record.
		//
		// we assume that it contains the DAG_EDGES records.
		//
		// we cannot make any statements about whether
		// or not this node is a leaf.
		//
		// return special already-exists error code; this error may
		// be ignored by caller.

		SG_ERR_RESET_THROW(SG_ERR_DAGNODE_ALREADY_EXISTS);
	}
	SG_ERR_CHECK_CURRENT;  // otherwise, we have an unknown SQL error.

	// we just added a *NEW* node to DAG_INFO, so we must have
	// a new (to us) dagnode.  continue adding stuff to the
	// other tables.

	SG_ERR_CHECK_RETURN(  SG_dagnode__get_parents__ref(pCtx, pDagnode,&kLimit,&paParents)  );

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pDagnode,&szHidChild)  );

	// We want to insert a unique <child,parent> pair into the
	// DAG_EDGES table for each parent.

	for (k=0; k<kLimit; k++)
	{
#if 0 // this check is redundant
		SG_bool bIsParentKnown = SG_FALSE;
		SG_int32 genUnused;

		// Because the DAG is essentially append-only and we want to avoid
		// having a sparse DAG, let's require that all of the parents already
		// be in the DAG before we allow the dagnode to be added.

		SG_ERR_CHECK(  _my_is_known(pCtx, pst, paParents[k], NULL, &bIsParentKnown, &genUnused, NULL)  );
		if (!bIsParentKnown)
		{
			SG_ERR_THROW(  SG_ERR_CANNOT_CREATE_SPARSE_DAG  );
		}
#endif
		// try to add record to DAG_EDGES table.  if this fails because
		// an actual error --or-- because of a constraint (duplicate key)
		// error, something is wrong and we just give up.

		SG_ERR_CHECK(  _my_store_edge(pCtx, pst, szHidChild,paParents[k])  );

		// we just added a *NEW* edge from child->parent to the DAG and, by
		// definition, the parent node is no longer a leaf.

		//SG_ERR_CHECK(  _my_mark_as_not_leaf(pCtx, pst, paParents[k])  );
        {
            SG_bool b_was_a_leaf = SG_FALSE;
            SG_ERR_CHECK(  SG_vhash__remove_if_present(pCtx, pst->pvh_leaves, paParents[k], &b_was_a_leaf)  );
        }
	}

	// we just added a *NEW* node to the DAG and, by definition, no other node
	// in the graph references us, so therefore, it is a leaf.

	//SG_ERR_CHECK(  _my_mark_as_leaf(pCtx, pst, szHidChild)  );
    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pst->pvh_leaves, szHidChild)  );

	paParents = NULL;

fail:
    ;
}

//////////////////////////////////////////////////////////////////

static void SG_dag_sqlite3__store_dagnode(SG_context* pCtx, my_state* pst, const SG_dagnode * pDagnode)
{
	// Callers should have an open sqlite transaction and rollback if any error is returned.
	// Ian TODO: Except SG_ERR_DAGNODE_ALREADY_EXISTS, which will go away?

	SG_uint32 nrParents;

	SG_NULLARGCHECK_RETURN(pst);
	SG_NULLARGCHECK_RETURN(pDagnode);

	SG_ERR_CHECK(  SG_dagnode__count_parents(pCtx, pDagnode, &nrParents)  );

	if (nrParents == 0)
    {
		SG_ERR_CHECK(  _my_store_initial_dagnode(pCtx, pst, pDagnode)  );
    }
	else
    {
		SG_ERR_CHECK(  _my_store_dagnode_with_parents(pCtx, pst, pDagnode)  );
    }

	return;

fail:
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}

//////////////////////////////////////////////////////////////////

static void _my_fetch_leaves(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned);

void SG_dag_sqlite3__find_new_dagnodes_since(
	SG_context* pCtx,
    sqlite3* psql,
	SG_uint64 iDagNum,
    SG_varray* pva_starting_leafset,
    SG_ihash** ppih
	)
{
	char buf_table_info[BUF_LEN_TABLE_NAME];
	char buf_table_edges[BUF_LEN_TABLE_NAME];
	sqlite3_stmt* pStmt = NULL;
	int rc;
    SG_ihash* pih_before = NULL;
    SG_ihash* pih_since = NULL;
    my_state st;
    SG_uint32 count_starting_leafset = 0;
    SG_uint32 count_current_leaves = 0;
    SG_uint32 i = 0;
    SG_rbtree* prb_leaves = NULL;

    memset(&st, 0, sizeof(st));
    st.psql = psql;
    st.iDagNum = iDagNum;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_starting_leafset, &count_starting_leafset)  );
    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pih_since)  );

	SG_ERR_CHECK(  _my_fetch_leaves(pCtx, psql,iDagNum,&prb_leaves)  );
    SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_leaves, &count_current_leaves)  );
    if (count_current_leaves == count_starting_leafset)
    {
        SG_bool b_same_leaves = SG_TRUE;

        for (i=0; i<count_starting_leafset; i++)
        {
            const char* psz_id = NULL;
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_starting_leafset, i, &psz_id)  );
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_leaves, psz_id, &b_has, NULL)  );
            if (!b_has)
            {
                b_same_leaves = SG_FALSE;
                break;
            }
        }

        if (b_same_leaves)
        {
            goto done;
        }
    }
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, buf_table_info)  );
	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, buf_table_edges)  );

    //SG_VARRAY_STDERR(pva_starting_leafset);
    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pih_before)  );
    for (i=0; i<count_starting_leafset; i++)
    {
        const char* psz_id = NULL;
        SG_bool b_ok = SG_FALSE;
        SG_int32 gen = -1;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_starting_leafset, i, &psz_id)  );

        SG_ERR_CHECK(  _my_is_known(pCtx, &st, psz_id, NULL, &b_ok, &gen, NULL)  );
        if (!b_ok)
        {
            SG_ERR_THROW(SG_ERR_NOT_FOUND);
        }
        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_before, psz_id, gen)  );
    }

    SG_ERR_CHECK(  free_my_state_stuff(pCtx, &st)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT e.child_id, e.parent_id, i.generation FROM %s e INNER JOIN %s i ON (i.child_id=e.child_id) ORDER BY i.generation DESC",
        buf_table_edges,
        buf_table_info
        )  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * psz_node = (const char*)sqlite3_column_text(pStmt,0);
        const char * psz_parent = (const char*)sqlite3_column_text(pStmt,1);
        SG_int32 gen = (SG_int32) sqlite3_column_int(pStmt, 2);
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK(  SG_ihash__has(pCtx, pih_before, psz_node, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK(  SG_ihash__update__int64(pCtx, pih_before, psz_parent, gen)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_ihash__update__int64(pCtx, pih_since, psz_node, gen)  );
        }
    }

	if ( (rc != SQLITE_DONE) )
    {
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

done:
    *ppih = pih_since;
    pih_since = NULL;

	/* fall through */

fail:
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &st)  );
    SG_IHASH_NULLFREE(pCtx, pih_since);
    SG_IHASH_NULLFREE(pCtx, pih_before);
	if (pStmt)
    {
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
}

void SG_dag_sqlite3__find_dag_path(
	SG_context* pCtx,
    sqlite3* psql,
	SG_uint64 iDagNum,
    const char* psz_id_min,
    const char* psz_id_max,
    SG_varray** ppva
	)
{
	char buf_table_info[BUF_LEN_TABLE_NAME];
	char buf_table_edges[BUF_LEN_TABLE_NAME];
	sqlite3_stmt* pStmt = NULL;
	int rc;
    SG_vhash* pvh_nodes = NULL;
    SG_vhash* pvh_gens = NULL;
    SG_bool b_ok = SG_FALSE;
    SG_int32 gen_min = -1;
    SG_int32 gen_max = -1;
    SG_varray* pva = NULL;
    my_state st;

    memset(&st, 0, sizeof(st));
    st.psql = psql;
    st.iDagNum = iDagNum;

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, buf_table_info)  );
	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, buf_table_edges)  );

    b_ok = SG_FALSE;
	SG_ERR_CHECK(  _my_is_known(pCtx, &st, psz_id_max, NULL, &b_ok, &gen_max, NULL)  );
    if (!b_ok)
    {
		SG_ERR_THROW(SG_ERR_NOT_FOUND);
    }

    if (1 == gen_max)
    {
        SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, psz_id_max)  );
        goto done;
    }

    if (psz_id_min)
    {
        b_ok = SG_FALSE;
        SG_ERR_CHECK(  _my_is_known(pCtx, &st, psz_id_min, NULL, &b_ok, &gen_min, NULL)  );
        if (!b_ok)
        {
            SG_ERR_THROW(SG_ERR_NOT_FOUND);
        }
    }
    else
    {
        gen_min = 1;
    }

    SG_ERR_CHECK(  free_my_state_stuff(pCtx, &st)  );

    SG_ASSERT(gen_min < gen_max);

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT e.child_id, e.parent_id, i.generation FROM %s e INNER JOIN %s i ON (i.child_id=e.child_id) WHERE (i.generation >= %d AND i.generation <= %d) ORDER BY i.generation ASC",
        buf_table_edges,
        buf_table_info, 
        gen_min, gen_max
        )  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_nodes)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_gens)  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * psz_node = (const char*)sqlite3_column_text(pStmt,0);
        const char * psz_parent = (const char*)sqlite3_column_text(pStmt,1);
        SG_int32 gen = (SG_int32) sqlite3_column_int(pStmt, 2);

        SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvh_gens, psz_node, (SG_int64) gen)  );

        if (
                (gen == gen_max) 
                && (0 != strcmp(psz_node, psz_id_max))
                )
        {
            continue;
        }

        if (
                psz_id_min
                && (gen == gen_min) 
                && (0 == strcmp(psz_node, psz_id_min))
           )
        {
            SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_nodes, psz_node, "")  );
        }
        else if (
                    !psz_id_min
                    && (1 == gen)
                )
        {
            SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_nodes, psz_node, "")  );
        }
        else
        {
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_nodes, psz_parent, &b_has)  );
            if (b_has)
            {
                const char* psz_other_parent = NULL;

                // my parent is there, so we can add me
                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_nodes, psz_node, &psz_other_parent)  );
                if (psz_other_parent)
                {
                    // I'm already present
                    SG_int32 gen_this_parent = -1;
                    SG_int32 gen_other_parent = -1;

                    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_gens, psz_parent, &gen_this_parent)  );
                    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_gens, psz_other_parent, &gen_other_parent)  );
                    if (gen_this_parent < gen_other_parent)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_nodes, psz_node, psz_parent)  );
                    }
                }
                else
                {
                    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_nodes, psz_node, psz_parent)  );
                }
            }
        }
    }

	if ( (rc != SQLITE_DONE) )
    {
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    {
        const char* psz_cur = psz_id_max;

        SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
        while (1)
        {
            const char* psz_parent = NULL;

            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, psz_cur)  );
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_nodes, psz_cur, &psz_parent)  );
            if (!psz_parent)
            {
                SG_VARRAY_NULLFREE(pCtx, pva);
                break;
            }
            if (!psz_parent[0])
            {
                break;
            }
            psz_cur = psz_parent;
        }
    }

done:
    if (!psz_id_min)
    {
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, "")  );
    }
    *ppva = pva;
    pva = NULL;

	/* fall through */
fail:
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &st)  );
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VHASH_NULLFREE(pCtx, pvh_nodes);
    SG_VHASH_NULLFREE(pCtx, pvh_gens);
	if (pStmt)
    {
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
}

typedef struct  
{
	SG_uint64 iDagnum;
	sqlite3_stmt* pStmtKnown;
	sqlite3_stmt* pStmtEdge;
} _fetch_dagnodes_handle;

static void _fetch_dagnode(
	SG_context* pCtx, 
    my_state* pst,
	const char* pszidHidChangeset, 
	sqlite3_stmt** ppStmtKnown,		/* [in/out] If you're going to call this again WITH THE SAME DAGNUM, passing back this statement will be faster. Otherwise pass NULL. */
	sqlite3_stmt** ppStmtEdge,		/* [in/out] If you're going to call this again WITH THE SAME DAGNUM, passing back this statement will be faster. Otherwise pass NULL. */
	SG_dagnode ** ppNewDagnode)
{
	// fetch all rows in the DAG_EDGE table that have the
	// given HID as the child_id field.  package up all
	// of the parent HIDs into a freshly allocate DAGNODE.

	// since dagnodes are constant once created (immutable), we
	// don't really have to worry about transaction consistency
	// when the caller is trying to read more than one dagnode
	// into memory.  (each fetch_dagnode can be in its own TX.)
	//
	// HOWEVER, we do need a TX to make sure that that we a
	// consistent view of the EDGES (all or none) (i think).

	int rc;
	SG_dagnode * pNewDagnode = NULL;
	sqlite3_stmt * pStmtEdge = NULL;
#ifdef DEBUG
	SG_bool bHasFakeParent = SG_FALSE;
#endif
	SG_bool bIsKnownInfo = SG_FALSE;
	SG_uint32 sumParents = 0;
	SG_int32 generation = -1;
	SG_uint32 revno = 0;
	char bufTableName[BUF_LEN_TABLE_NAME];
    SG_rbtree* prb_parents = NULL;

	SG_NULLARGCHECK_RETURN(pst);
	SG_NULLARGCHECK_RETURN(pszidHidChangeset);
	SG_NULLARGCHECK_RETURN(ppNewDagnode);

	//////////////////////////////////////////////////////////////////
	// begin transaction for doing a consistent SELECT.
	// we only look at the DAG_EDGES table.
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// fetch info from the DAG_INFO table.

	SG_ERR_CHECK(  _my_is_known(pCtx, pst, pszidHidChangeset, ppStmtKnown, &bIsKnownInfo, &generation, &revno)  );
	if (!bIsKnownInfo)
	{
		// we didn't find a row in the dag_info table.
		// perhaps they gave us a bogus Changeset HID.
		// or perhaps we have a problem in the DAG DB.

		SG_ERR_THROW(SG_ERR_NOT_FOUND);
	}

	SG_ERR_CHECK(  SG_dagnode__alloc(pCtx, &pNewDagnode, pszidHidChangeset, generation, revno)  );

	//////////////////////////////////////////////////////////////////
	// fetch all DAG_EDGE rows with having this child_id.
	//
	// we don't need to worry about having SQL sort the edges because
	// we now use a rbtree to store the list of parents in memory and
	// it will always be sorted.

	if (ppStmtEdge == NULL || *ppStmtEdge == NULL)
	{
		SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, pst->iDagNum, bufTableName)  );
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pst->psql, &pStmtEdge,
			"SELECT parent_id FROM %s WHERE child_id = ?", bufTableName)  );
	}
	else
	{
		pStmtEdge = *ppStmtEdge;
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmtEdge)  );
	}
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmtEdge,1,pszidHidChangeset)  );

	// read each parent row.

	while ((rc=sqlite3_step(pStmtEdge)) == SQLITE_ROW)
	{
		const char * szHidParent = (const char *)sqlite3_column_text(pStmtEdge,0);

		// if parent_id is the FAKE_PARENT then we think that this child_id
		// is the initial checkin/commit -- BUT WE SHOULD VERIFY THIS.  we
		// let the loop continue and see if there are any other parents in
		// the SELECT result set.
#ifdef DEBUG
		if (strcmp(szHidParent,FAKE_PARENT) == 0)
			bHasFakeParent = SG_TRUE;
		else
#else
		if (strcmp(szHidParent,FAKE_PARENT) != 0)
#endif
		{
            if (!prb_parents)
            {
                SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_parents)  );
            }

			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb_parents, szHidParent)  );
			sumParents++;
		}
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	if (ppStmtEdge == NULL)
		SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtEdge)  );
	else
	{
		*ppStmtEdge = pStmtEdge;
		pStmtEdge = NULL;
	}

    if (prb_parents)
    {
        SG_ERR_CHECK(  SG_dagnode__set_parents__rbtree(pCtx, pNewDagnode, prb_parents)  );
        SG_RBTREE_NULLFREE(pCtx, prb_parents);
    }

	SG_ERR_CHECK(  SG_dagnode__freeze(pCtx, pNewDagnode)  );

#ifdef DEBUG
	//////////////////////////////////////////////////////////////////
	// begin consistency checks
	//////////////////////////////////////////////////////////////////
	{
		SG_uint32 nrParents;

		if (bHasFakeParent)
			SG_ASSERT( (sumParents == 0)  &&  "Cannot have FAKE_PARENT and real parents." );
		else
			SG_ASSERT( (sumParents > 0)  && "Must have either FAKE_PARENT or at least one real parent.");

		SG_ERR_CHECK(  SG_dagnode__count_parents(pCtx, pNewDagnode,&nrParents)  );

		// We did a unique insert of each parent HID into the DAGNODEs idset.
		// So if everything is in order, the DAGNODE should have the
		// same number of parents as we found in the DB.  Since we have
		// a PRIMARY KEY on the <child,parent> pair on the disk, this shouldn't happen.

		SG_ASSERT( (nrParents == sumParents) && "Possible duplicate in list of real parents.");
	}

	//////////////////////////////////////////////////////////////////
	// end of consistency checks
	//////////////////////////////////////////////////////////////////
#endif

	*ppNewDagnode = pNewDagnode;
	return;

fail:
    SG_RBTREE_NULLFREE(pCtx, prb_parents);
	SG_DAGNODE_NULLFREE(pCtx, pNewDagnode);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmtEdge)  );
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY),SG_ERR_DB_BUSY);
}

void SG_dag_sqlite3__fetch_dagnode(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, const char* pszidHidChangeset, SG_dagnode ** ppNewDagnode)
{
    my_state st;

    memset(&st, 0, sizeof(st));
    st.psql = psql;
    st.iDagNum = iDagNum;
	SG_ERR_CHECK(  _fetch_dagnode(pCtx, &st, pszidHidChangeset, NULL, NULL, ppNewDagnode)  );

fail:
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &st)  );
}

void SG_dag_sqlite3__fetch_dagnodes__begin(SG_context* pCtx, SG_uint64 iDagnum, SG_dag_sqlite3_fetch_dagnodes_handle** ppHandle)
{
	_fetch_dagnodes_handle* pHandle = NULL;

	SG_NULLARGCHECK_RETURN(ppHandle);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pHandle)  );
	pHandle->iDagnum = iDagnum;

	*ppHandle = (SG_dag_sqlite3_fetch_dagnodes_handle*)pHandle;

	return;

fail:
	SG_NULLFREE(pCtx, pHandle);
}

void SG_dag_sqlite3__fetch_dagnodes__one(
	SG_context* pCtx, 
	sqlite3* psql, 
	SG_dag_sqlite3_fetch_dagnodes_handle* pHandle,
	const char* pszidHidChangeset, 
	SG_dagnode** ppNewDagnode)
{
	_fetch_dagnodes_handle* pMyHandle = (_fetch_dagnodes_handle*)pHandle;
    my_state st;

    memset(&st, 0, sizeof(st));
    st.psql = psql;
    st.iDagNum = pMyHandle->iDagnum;
	SG_ERR_CHECK(  _fetch_dagnode(pCtx, &st, pszidHidChangeset, &pMyHandle->pStmtKnown, &pMyHandle->pStmtEdge, ppNewDagnode)  );

fail:
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &st)  );
}

void SG_dag_sqlite3__fetch_dagnodes__end(SG_context* pCtx, SG_dag_sqlite3_fetch_dagnodes_handle** ppHandle)
{
	_fetch_dagnodes_handle* pHandle = (_fetch_dagnodes_handle*)*ppHandle;
	if (pHandle)
	{
		SG_ERR_CHECK_RETURN(  sg_sqlite__nullfinalize(pCtx, &pHandle->pStmtKnown)  );
		SG_ERR_CHECK_RETURN(  sg_sqlite__nullfinalize(pCtx, &pHandle->pStmtEdge)  );
		SG_NULLFREE(pCtx, *ppHandle);
	}
}

//////////////////////////////////////////////////////////////////

static void _my_fetch_leaves(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned)
{
	// do the actual fetch_leaves assuming that a TX is already open.

	char bufTableName[BUF_LEN_TABLE_NAME];
	int rc;
	sqlite3_stmt * pStmt = NULL;
	SG_rbtree * pIdset = NULL;
	SG_bool bTableExists = SG_TRUE;

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(ppIdsetReturned);

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pIdset)  );

	// prepare SELECT to fetch all rows from DAG_LEAVES.
	//
	// we don't need to worry about having SQL sort the leaves because
	// we now use a rbtree to store the list in memory.
	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, iDagNum, bufTableName)  );
	sg_sqlite__prepare(pCtx, psql, &pStmt, "SELECT child_id FROM %s", bufTableName);

	/* We get SQLITE_ERROR if the dag tables don't exist yet.
	   If we confirm that the table doesn't exist, we return
	   the "no such node" values.  Otherwise we throw. */
	if (SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_ERROR)))
	{
		SG_ERR_IGNORE(  SG_sqlite__table_exists(pCtx, psql, bufTableName, &bTableExists)  );
		if (!bTableExists)
			SG_ERR_DISCARD;
	}
	SG_ERR_CHECK_CURRENT;

	if (bTableExists)
	{
		// read each leaf row.

		while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
		{
			const char * szHid = (const char *)sqlite3_column_text(pStmt,0);

			SG_ERR_CHECK(  SG_rbtree__add(pCtx, pIdset,szHid)  );
		}
		if (rc != SQLITE_DONE)
		{
			SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
		}
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

#if 1
	//////////////////////////////////////////////////////////////////
	// begin consistency checks
	//////////////////////////////////////////////////////////////////
	{
		// TODO what kind of asserts should we put on the data that we found?
		// TODO should we verify the leaf count is > 0 when there are edges present?
	}
	//////////////////////////////////////////////////////////////////
	// end consistency checks
	//////////////////////////////////////////////////////////////////
#endif

	*ppIdsetReturned = pIdset;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, pIdset);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}

void SG_dag_sqlite3__find_by_prefix(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, const char* psz_hid_prefix, SG_rbtree** pprb_results)
{
	char bufTableName[BUF_LEN_TABLE_NAME];
	int rc;
	sqlite3_stmt * pStmt = NULL;
    SG_rbtree* prb = NULL;
    char buf_prefix_plus_one[SG_HID_MAX_BUFFER_LENGTH];

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(psz_hid_prefix);
	SG_NULLARGCHECK_RETURN(pprb_results);

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );
    SG_ERR_CHECK(  SG_strcpy(pCtx, buf_prefix_plus_one, sizeof(buf_prefix_plus_one), psz_hid_prefix)  );
    
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );
	
	SG_hex__fake_add_one(pCtx, buf_prefix_plus_one);
	
	/* If the provided prefix isn't hex, return no results. */
	if (SG_context__err_equals(pCtx, SG_ERR_INVALIDARG))
	{
		SG_ERR_DISCARD;
		*pprb_results = prb;
		return;
	}
	SG_ERR_CHECK_CURRENT;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT child_id FROM %s WHERE ( (child_id >= ?) AND (child_id < ?) )", 
		bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_hid_prefix)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, buf_prefix_plus_one)  );

    //
	// read each row.
	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char * szHid = (const char *)sqlite3_column_text(pStmt,0);

		SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb,szHid)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *pprb_results = prb;

	return;

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}

void SG_dag_sqlite3__find_by_rev_id(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_uint32 iRevisionId, char** ppsz_hid)
{
	char bufTableName[BUF_LEN_TABLE_NAME];
	SG_string* pstrResult = NULL;

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(ppsz_hid);

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va__string(pCtx, psql, &pstrResult,
		"SELECT child_id FROM %s WHERE (instance_revno=%u)", bufTableName, iRevisionId)  );

	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrResult, (SG_byte**)ppsz_hid, NULL)  );

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstrResult);
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_DONE), SG_ERR_NOT_FOUND);
}

void SG_dag_sqlite3__list_dags(SG_context* pCtx, sqlite3* psql, SG_uint32* piCount, SG_uint64** paDagNums)
{
	int rc;
	sqlite3_stmt * pStmt = NULL;
    SG_uint32 space = 0;
    SG_uint64* pResults = NULL;
    SG_uint64* pGrowResults = NULL;
    SG_uint32 count = 0;

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(piCount);
	SG_NULLARGCHECK_RETURN(paDagNums);

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT dagnum FROM dags ORDER BY dagnum ASC")  );

    space = 32;
	SG_ERR_CHECK(  SG_allocN(pCtx,space,pResults)  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_dagnum = (const char*) sqlite3_column_text(pStmt,0);
        SG_uint64 d = 0;

        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &d)  );

        if ((count + 1) > space)
        {
            space *= 2;
			SG_ERR_CHECK(  SG_allocN(pCtx,space,pGrowResults)  );

            memcpy(pGrowResults, pResults, count * sizeof(SG_uint64));
            SG_NULLFREE(pCtx, pResults);
            pResults = pGrowResults;
            pGrowResults = NULL;
        }

        pResults[count++] = d;
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *piCount = count;
    *paDagNums = pResults;

	return;

fail:
    SG_NULLFREE(pCtx, pResults);
    SG_NULLFREE(pCtx, pGrowResults);
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}

void SG_dag_sqlite3__fetch_leaves(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned)
{
	// fetch HIDs of all LEAVES from the DAG_LEAVES table
	// and assemble them into a freshly allocated IDSET.

	// because leaves get added and deleted as nodes are added,
	// we hold a TX to make sure that we get a consistent view
	// of the leaves.
	//
	// let's agree here that we will give the caller a consistent
	// view of the leaves as of a particular point in time.  they
	// should start with this list and reference dagnodes and edges
	// with confidence -- a hid in the set of leaves implies that
	// it is already present (and complete) it the edges.
	//
	// if another process is adding nodes (and updating the leaves),
	// it won't affect this process - until we are called again to
	// get a new copy of the leaves.

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(ppIdsetReturned);

	//////////////////////////////////////////////////////////////////
	// begin transaction for doing a consistent SELECT.
	// we only look at the DAG_EDGES table.
	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  _my_fetch_leaves(pCtx, psql,iDagNum,ppIdsetReturned)  );

	return;

fail:
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}

void SG_dag_sqlite3__fetch_dagnode_children(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, const char * pszDagnodeID, SG_rbtree ** ppIdsetReturned)
{
	char bufTableName[BUF_LEN_TABLE_NAME];
	int rc;
	sqlite3_stmt * pStmt = NULL;
	SG_rbtree * pIdset = NULL;

	// fetch HIDs of all LEAVES from the DAG_LEAVES table
	// and assemble them into a freshly allocated IDSET.

	// because children get added and deleted as nodes are added,
	// we hold a TX to make sure that we get a consistent view
	// of the leaves.
	//
	// let's agree here that we will give the caller a consistent
	// view of the children as of a particular point in time.  they
	// should start with this list and reference dagnodes and edges
	// with confidence -- a hid in the set of children implies that
	// it is already present (and complete) it the edges.
	//
	// if another process is adding nodes (and updating the children),
	// it won't affect this process - until we are called again to
	// get a new copy of the leaves.

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(ppIdsetReturned);

	//////////////////////////////////////////////////////////////////
	// begin transaction for doing a consistent SELECT.
	// we only look at the DAG_EDGES table.
	//////////////////////////////////////////////////////////////////

	// do the actual fetch_children assuming that a TX is already open.

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pIdset)  );

	// prepare SELECT to fetch all rows from DAG_LEAVES.
	//
	// we don't need to worry about having SQL sort the leaves because
	// we now use a rbtree to store the list in memory.

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT child_id FROM %s WHERE parent_id = ?", bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt,1,pszDagnodeID)  );
	// read each row.

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char * szHid = (const char *)sqlite3_column_text(pStmt,0);

		SG_ERR_CHECK(  SG_rbtree__add(pCtx, pIdset,szHid)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

#if 1
	//////////////////////////////////////////////////////////////////
	// begin consistency checks
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// end consistency checks
	//////////////////////////////////////////////////////////////////
#endif

	*ppIdsetReturned = pIdset;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, pIdset);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
return;

}

void SG_dag_sqlite3__count_all_dagnodes(
	SG_context* pCtx,
	sqlite3* pSql,
	SG_uint64 iDagNum,
    SG_uint32* pi
    )
{
	char bufTableName[BUF_LEN_TABLE_NAME];
    SG_int32 count = 0;

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pSql, (SG_int32*)&count,
		"SELECT COUNT(child_id) FROM %s",
		bufTableName)  );

    *pi = (SG_uint32) count;

fail:
    ;
}

void SG_dag_sqlite3__fetch_dagnode_ids(
	SG_context* pCtx,
    sqlite3* psql,
	SG_uint64 dagnum,
    SG_int32 gen_min,
    SG_int32 gen_max,
    SG_ihash** ppih
    )
{
	char bufTableName[BUF_LEN_TABLE_NAME];
    char buf_where[256];
	sqlite3_stmt* pStmt = NULL;
	int rc;
    SG_ihash* pih = NULL;

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, dagnum, bufTableName)  );

    if (gen_min < 0)
    {
        if (gen_max < 0)
        {
            buf_where[0] = 0;
        }
        else
        {
            SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx, buf_where, sizeof(buf_where), "WHERE generation <= %d", gen_max)  );
        }
    }
    else
    {
        if (gen_max < 0)
        {
            SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx, buf_where, sizeof(buf_where), "WHERE generation >= %d", gen_min)  );
        }
        else
        {
            SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx, buf_where, sizeof(buf_where), "WHERE generation >= %d AND generation <= %d", gen_min, gen_max)  );
        }
    }

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT child_id, generation FROM %s "
        "%s "
		"ORDER BY generation DESC ",
        bufTableName,
        buf_where
        )  );

    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pih)  );
    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * szHid = (const char*)sqlite3_column_text(pStmt,0);
        SG_int32 gen = (SG_int32) sqlite3_column_int(pStmt, 1);

        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih, szHid, (SG_int64) gen)  );
    }

	if ( (rc != SQLITE_DONE) )
    {
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppih = pih;
    pih = NULL;

	/* fall through */
fail:
	if (pStmt)
    {
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
    SG_IHASH_NULLFREE(pCtx, pih);
}

void SG_dag_sqlite3__fetch_all_dagnodes(
	SG_context* pCtx,
	sqlite3* pSql,
	SG_uint64 iDagNum,
    SG_stringarray* psa
    )
{
	char bufTableName[BUF_LEN_TABLE_NAME];
	sqlite3_stmt* pStmt = NULL;
	int rc;

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pSql, &pStmt,
		"SELECT child_id FROM %s "
		//"ORDER BY instance_revno DESC ",
		"ORDER BY generation ASC ",
        //index rebuild needs these in generational order
        bufTableName)  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * szHid = (const char*)sqlite3_column_text(pStmt,0);

        SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, szHid)  );
    }

	if ( (rc != SQLITE_DONE) )
    {
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	/* fall through */
fail:
	if (pStmt)
    {
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
}

void sg_fs3_get_fragball_stuff(
	SG_context* pCtx,
	sqlite3* psql,
	SG_uint64 dagnum,
    const char* psz_tid
    )
{
	char bufTableName[BUF_LEN_TABLE_NAME];

#if 0
	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, dagnum, bufTableName)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, ("CREATE TABLE %s.%s"
		"  ("
		"    child_id VARCHAR NOT NULL,"
		"    parent_id VARCHAR NOT NULL"
		"  )"), psz_tid, bufTableName)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "INSERT INTO %s.%s (child_id, parent_id) SELECT child_id, parent_id FROM %s", psz_tid, bufTableName, bufTableName)  );
#endif

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, dagnum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql,
		("CREATE TABLE %s.%s"
		"  ("
		"    child_id VARCHAR NOT NULL,"
		"    generation INTEGER"
		"  )"), psz_tid, bufTableName)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "INSERT INTO %s.%s (child_id, generation) SELECT child_id, generation FROM %s", psz_tid, bufTableName, bufTableName)  );

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_AUDITS_TABLE_NAME, dagnum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql,
		("CREATE TABLE %s.%s"
		"  ("
		"    csid VARCHAR NOT NULL,"
		"    userid VARCHAR NOT NULL,"
		"    timestamp INTEGER NOT NULL"
		"  )"), psz_tid, bufTableName)  );

    {
        SG_int_to_string_buffer buf_dagnum;
        SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "INSERT INTO %s.%s (csid,userid,timestamp) SELECT csid,userid,timestamp FROM audits WHERE dagnum=%s", psz_tid, bufTableName, SG_uint64_to_sz(dagnum, buf_dagnum))  );
    }

	/* fall through */

fail:
    ;
}

void SG_dag_sqlite3__fetch_chrono_dagnode_list(
	SG_context* pCtx,
	sqlite3* pSql,
	SG_uint64 iDagNum,
	SG_uint32 iStartRevNo,
	SG_uint32 iNumRevsRequested,
	SG_uint32* piNumRevsReturned,
	char*** ppaszHidsReturned)
{
	char bufTableName[BUF_LEN_TABLE_NAME];
	char** paszHidsReturned = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_uint32 iRowsAvail = 0;
	SG_uint32 iRowsReturned = 0;
	int rc;
	SG_uint32 bufLen = SG_HID_MAX_BUFFER_LENGTH;

	SG_NULLARGCHECK_RETURN(piNumRevsReturned);
	SG_NULLARGCHECK_RETURN(ppaszHidsReturned);

	if (!iStartRevNo)
		iStartRevNo = SG_UINT32_MAX;

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufTableName)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pSql, (SG_int32*)&iRowsAvail,
		"SELECT COUNT(instance_revno) FROM %s "
		"WHERE instance_revno <= %u",
		bufTableName, iStartRevNo)  );

	if ( (iNumRevsRequested == 0) || (iNumRevsRequested > iRowsAvail) )
		iNumRevsRequested = iRowsAvail;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pSql, &pStmt,
		"SELECT child_id FROM %s "
		"WHERE instance_revno <= %u "
		"ORDER BY instance_revno DESC "
		"LIMIT %u", bufTableName, iStartRevNo, iNumRevsRequested)  );

	SG_ERR_CHECK(  SG_alloc(pCtx, iNumRevsRequested, bufLen, &paszHidsReturned)  );
	{
		char* pszDest = (char*)paszHidsReturned;
		while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
		{
			const char * szHid = (const char*)sqlite3_column_text(pStmt,0);
			SG_ERR_CHECK(  SG_strcpy(pCtx, (char*)pszDest, bufLen, szHid)  );
			pszDest += bufLen;
			iRowsReturned++;
		}
	}
	if ( (rc != SQLITE_DONE) )
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );

	*piNumRevsReturned = iRowsReturned;
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_RETURN_AND_NULL(paszHidsReturned, ppaszHidsReturned);

	/* fall through */
fail:
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_NULLFREE(pCtx, paszHidsReturned);
}

//////////////////////////////////////////////////////////////////

static void _my_compute_leaves(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned)
{
	// let SQL compute the set of leaves using some fancy/expensive black magic.
	// the caller must free the idset returned.
	//
	// scan the DAG_EDGES TABLE and find all the child_id's that do not
	// appear in the parent_id column.  these are effectively leaves.
	//
	// we require the caller to be in a transaction so that our results
	// come from the same consistent view of the DB as their other results.

	char bufTableName[BUF_LEN_TABLE_NAME];
	int rc;
	sqlite3_stmt * pStmt = NULL;
	SG_rbtree * pIdset = NULL;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pIdset)  );

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
									  "SELECT DISTINCT child_id FROM %s"
									  "    WHERE child_id NOT IN"
									  "        (SELECT DISTINCT parent_id FROM %s)", bufTableName, bufTableName)  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char * szHid = (const char *)sqlite3_column_text(pStmt,0);

		SG_ERR_CHECK(  SG_rbtree__add(pCtx, pIdset,szHid)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	*ppIdsetReturned = pIdset;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, pIdset);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx,pStmt)  );
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY),SG_ERR_DB_BUSY);
}

static void _my_is_sparse(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_bool * pbIsSparse)
{
	// let SQL compute whether the DAG is SPARSE using some fancy/expensive black magic.
	//
	// sparse - a DAG is SPARSE if there are nodes missing between
	//          known nodes and the ROOT.  assuming edges point from
	//          child to parent.
	//
	//          if it is SPARSE, then we cannot walk from every LEAF
	//          all the way to the ROOT.
	//
	// note that this computation only reflects the current snapshot
	// of the DAG that we have.  since we don't know the complete
	// graph or if we know the complete graph, we see if we have a
	// node that has a parent that we don't know about.
	//
	// that is, scan the DAG_EDGES TABLE and see if there is a referenced parent
	// that does not appear as a child.
	//
	// we require the caller to be in a transaction so that our results
	// come from the same consistent view of the DB as their other results.

	char bufTableName[BUF_LEN_TABLE_NAME];
	sqlite3_stmt * pStmt = NULL;
	SG_int32 result;

	SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql,&pStmt,
									  "SELECT EXISTS"
									  "    (SELECT parent_id FROM %s"
									  "         WHERE parent_id != ?"
									  "           AND parent_id NOT IN"
									  "                   (SELECT child_id FROM %s)"
									  "         LIMIT 1)", bufTableName, bufTableName)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt,1,FAKE_PARENT)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt,SQLITE_ROW)  );

	result = (SG_int32)sqlite3_column_int(pStmt,0);
	*pbIsSparse = (result == 1);

	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx,pStmt)  );
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY),SG_ERR_DB_BUSY);
}

void SG_dag_sqlite3__check_consistency(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum)
{
	// I consider this to be a DEBUG routine.  But I'm putting it
	// in the VTABLE, so I'm not #ifdef'ing it because I want
	// the size of the VTABLE to be constant.
	//
	// Run a series of consistency checks on the DAG DB.
	// We do this in our own transaction so that all queries are
	// consistent.
	//
	// we do a rollback when we're done because we don't want to
	// actually make any changes.

	SG_rbtree * pIdsetAsStored = NULL;
	SG_rbtree * pIdsetAsComputed = NULL;
	SG_bool bAreEqual;
	SG_bool bIsSparse = SG_TRUE;

	//////////////////////////////////////////////////////////////////
	// begin transaction for doing a consistent SELECT.
	//////////////////////////////////////////////////////////////////

	// Test #1:
	// [] Read the set of leaves as stored in the DAG_LEAVES table.
	// [] Have SQL compute the set of leaves from the DAG_EDGES table
	//    (this may be O(N) or O(N^2) expensive).
	// [] Compare the 2 sets.  This can be efficient since both rbtrees
	//    are sorted, but is still expensive.

	SG_ERR_CHECK(  _my_fetch_leaves(pCtx, psql,iDagNum,&pIdsetAsStored)  );
	SG_ERR_CHECK(  _my_compute_leaves(pCtx, psql,iDagNum,&pIdsetAsComputed)  );
	SG_ERR_CHECK(  SG_rbtree__compare__keys_only(pCtx, pIdsetAsStored,pIdsetAsComputed,&bAreEqual,NULL,NULL,NULL)  );
	if (!bAreEqual)
	{
		SG_ERR_THROW(  SG_ERR_DAG_NOT_CONSISTENT  );
	}

	// Test #2:
	// [] Verify that the DAG is not sparse.

	SG_ERR_CHECK(  _my_is_sparse(pCtx, psql,iDagNum,&bIsSparse)  );
	if (bIsSparse)
	{
		SG_ERR_THROW(  SG_ERR_DAG_NOT_CONSISTENT  );
	}

	// Test #3:
	// [] Verify that revision numbers are all in parent->child order.
	//    This is to say: a child dagnode should always have a higher
	//    revision number than its parent.
	{
		SG_int32 count;
		char bufInfoTableName[BUF_LEN_TABLE_NAME];
		char bufEdgesTableName[BUF_LEN_TABLE_NAME];

		SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, iDagNum, bufInfoTableName)  );
		SG_ERR_CHECK(  _get_table_name(pCtx, DAG_EDGES_TABLE_NAME, iDagNum, bufEdgesTableName)  );
		
		SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, psql, &count,
			(
				"SELECT COUNT(*) FROM %s parent "
				"INNER JOIN %s edge ON parent.child_id = edge.parent_id "
				"INNER JOIN %s child ON edge.child_id = child.child_id "
				"WHERE parent.instance_revno > child.instance_revno"
			),
			bufInfoTableName, bufEdgesTableName, bufInfoTableName)  );

		if (count)
		{
            char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
            SG_ERR_CHECK_RETURN(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );
			SG_ERR_THROW2(  SG_ERR_DAG_NOT_CONSISTENT,
				(pCtx, "DAG %s has %d records with out-of-order revision numbers.", buf_dagnum, count)  );
		}
	}
	

	// TODO have a test that verifies no loops in the dag using the generation
	// TODO field.  it ought to be possible to write an SQL statement that does
	// TODO something like:
	// TODO
	// TODO     for each edge in dag_edges ( child_id, parent_id )
	// TODO        join with dag_info on child_id, giving ( child_generation )
	// TODO        join with dag_info on parent_id, giving ( parent_generation )
	// TODO        count rows with ( child_generation <= parent_generation )
	// TODO
	// TODO any such rows would indicate a possible loop in the dag.

	// TODO Any other tests...?

	SG_RBTREE_NULLFREE(pCtx, pIdsetAsStored);
	SG_RBTREE_NULLFREE(pCtx, pIdsetAsComputed);
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, pIdsetAsStored);
	SG_RBTREE_NULLFREE(pCtx, pIdsetAsComputed);
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY),SG_ERR_DB_BUSY);
}

//////////////////////////////////////////////////////////////////

struct _state_is_connected
{
    my_state st;
	SG_bool bConnected;
	SG_rbtree ** pprbMissing;
};

static void _my_foreach_end_fringe_callback__sqlite3(SG_context* pCtx, const char * szHid, void * pStateVoid)
{
	// a SG_dagfrag__foreach_end_fringe_callback.

	SG_int32 genFetched;
	SG_bool bIsKnown;
	struct _state_is_connected * pStateIsConnected = (struct _state_is_connected *)pStateVoid;

	SG_ERR_CHECK_RETURN(  _my_is_known(pCtx, &pStateIsConnected->st, szHid, NULL, &bIsKnown, &genFetched, NULL)  );
	if (bIsKnown)
		return;

	// our dag does not know about this HID.
	pStateIsConnected->bConnected = SG_FALSE;

	if (!pStateIsConnected->pprbMissing)		// they don't care about which node is unknown
		return;

	if (!*pStateIsConnected->pprbMissing)		// we deferred creating the rbtree until we actually needed it.
		SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, pStateIsConnected->pprbMissing)  );

	// add this HID to the list and return OK so that the iterator does the whole set.

	SG_rbtree__add(pCtx, *pStateIsConnected->pprbMissing,szHid);
}

static void _my_is_connected(SG_context* pCtx,
							 sqlite3* psql,
							 const SG_dagfrag * pFrag,
							 SG_bool* pbConnected,
							 SG_rbtree ** ppIdsetReturned)
{
	// iterate over the set of dagnodes in the end-fringe in the fragment
	// and see if we already know about all of them.

	SG_rbtree * prbMissing = NULL;		// defer allocating it until we actually need it and if requested.
	struct _state_is_connected state_is_connected;

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(pFrag);
	SG_NULLARGCHECK_RETURN(pbConnected);

    memset(&state_is_connected, 0, sizeof(state_is_connected));
	state_is_connected.st.psql = psql;
	SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &state_is_connected.st.iDagNum)  );
	state_is_connected.pprbMissing = &prbMissing;
	state_is_connected.bConnected = SG_TRUE; // Default to true. The callback only changes this to false
										   // because a single unknown/disconnected node makes the whole
										   // frag disconnected.

	// iterate over all end-fringe nodes in the fragment.  optionally build a list
	// of unknown nodes.

	SG_ERR_CHECK(  SG_dagfrag__foreach_end_fringe(pCtx, pFrag,_my_foreach_end_fringe_callback__sqlite3,&state_is_connected)  );

	if (!state_is_connected.bConnected)			// callback allocated a list to put at least one item
	{
		SG_ASSERT(prbMissing);
        if (ppIdsetReturned)
        {
            *ppIdsetReturned = prbMissing;	// caller now owns the list
        }
        else
        {
            SG_RBTREE_NULLFREE(pCtx, prbMissing);
        }
		*pbConnected = SG_FALSE;
	}
	else
	{
		// all nodes were found, so we are connectable.
		*pbConnected = SG_TRUE;
	}

    SG_ERR_CHECK(  free_my_state_stuff(pCtx, &state_is_connected.st)  );

	return;

fail:
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &state_is_connected.st)  );
	SG_RBTREE_NULLFREE(pCtx, prbMissing);
}

struct _state_member
{
    my_state st;
	sqlite3_stmt* pstmt;
	SG_stringarray ** ppsa_new;
    SG_bool b_not_really;
    SG_uint32 steps_total;
    SG_uint32 steps_sofar;
};

static void _my_insert_member_callback__sqlite3(SG_context* pCtx,
												const SG_dagnode * pDagnode,
												SG_dagfrag_state qs,
												void * pCtxVoid)
{
	// a SG_dagfrag__foreach_member_callback

	struct _state_member * pCtxMember = (struct _state_member *)pCtxVoid;
	const char * szHid = NULL;

	SG_UNUSED(qs);

	SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx, pDagnode,&szHid)  );

    if (pCtxMember->steps_total)
    {
        pCtxMember->steps_sofar++;
        if (
                (0 == (pCtxMember->steps_sofar % 10))
                || (pCtxMember->steps_sofar >= pCtxMember->steps_total)
           )
        {
            SG_ERR_CHECK_RETURN(  SG_log__set_finished(pCtx, (SG_uint32) pCtxMember->steps_sofar)  );
        }
    }

    if (pCtxMember->b_not_really)
    {
        SG_int32 generation = -1;
        SG_bool bIsKnownInfo = SG_FALSE;

        SG_ERR_CHECK_RETURN(  _my_is_known(pCtx, &pCtxMember->st, szHid, &pCtxMember->pstmt, &bIsKnownInfo, &generation, NULL)  );
        if (bIsKnownInfo)
        {
            // dag node is already known

            return;
        }
    }
    else
    {
        SG_dag_sqlite3__store_dagnode(pCtx, &pCtxMember->st, pDagnode);
        if (SG_context__err_equals(pCtx, SG_ERR_DAGNODE_ALREADY_EXISTS))
        {
            // we cannot precompute the amount of overlap between the frag and our dag,
            // so we expect to get duplicates.  silently ignore them.
			SG_context__err_reset(pCtx);
            return;
        }

        if (SG_context__err_equals(pCtx, SG_ERR_CANNOT_CREATE_SPARSE_DAG))
        {
            // this should not happen because we already checked the end-fringe and we
            // assumed that we have a connected graph before we started.  therefore, we
            // must be missing a node in the middle of the graph.
            //
            // return this error code to avoid confusing caller.
            //
            // TODO would it be helpful to also return the conflicting HID?
			SG_ERR_RESET_THROW_RETURN(SG_ERR_DAG_NOT_CONSISTENT);
        }

        if (SG_CONTEXT__HAS_ERR(pCtx)) return;
    }

	// we added a "new to us" node.

	if (!*pCtxMember->ppsa_new)			// we deferred creating the stringarray until we actually needed it.
    {
		SG_ERR_CHECK_RETURN(  SG_STRINGARRAY__ALLOC(pCtx, pCtxMember->ppsa_new, 10)  );
    }

	SG_ERR_CHECK_RETURN(  SG_stringarray__add(pCtx, *pCtxMember->ppsa_new,szHid)  );
}

//////////////////////////////////////////////////////////////////

void SG_dag_sqlite3__check_frag(SG_context* pCtx,
								sqlite3* psql,
								SG_dagfrag * pFrag,
								SG_bool * pbConnected,
								SG_rbtree ** ppIdsetMissing,
								SG_stringarray ** ppsa_new
                                )
{
	struct _state_member ctx_member;
    SG_bool b_pop = SG_FALSE;

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(pFrag);
	SG_NULLARGCHECK_RETURN(pbConnected);
	SG_NULLARGCHECK_RETURN(ppIdsetMissing);
	SG_NULLARGCHECK_RETURN(ppsa_new);

	*ppIdsetMissing = NULL;
	*ppsa_new = NULL;

	SG_ERR_CHECK_RETURN(  _my_is_connected(pCtx, psql, pFrag, pbConnected, ppIdsetMissing)  );
	if (SG_FALSE == *pbConnected)
		return;

    memset(&ctx_member, 0, sizeof(ctx_member));
	ctx_member.st.psql = psql;
	SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &ctx_member.st.iDagNum)  );
	ctx_member.ppsa_new = ppsa_new;
    ctx_member.b_not_really = SG_TRUE;
	ctx_member.pstmt = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Checking dagfrag", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
	SG_ERR_CHECK(  SG_dagfrag__foreach_member(pCtx, pFrag,_my_insert_member_callback__sqlite3,(void *)&ctx_member)  );
    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &ctx_member.pstmt)  );
    SG_ERR_CHECK(  free_my_state_stuff(pCtx, &ctx_member.st)  );

	return;

fail:
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &ctx_member.st)  );
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &ctx_member.pstmt)  );
}

static void get_leaves_into_vhash(
    SG_context* pCtx,
    sqlite3* psql,
    SG_uint64 dagnum,
    SG_vhash** ppvh
    )
{
    char bufTableName[BUF_LEN_TABLE_NAME];
    SG_bool b_exists = SG_FALSE;
    SG_vhash* pvh = NULL;
    sqlite3_stmt* pStmt = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

    SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, dagnum, bufTableName)  );
    SG_ERR_CHECK(  SG_sqlite__table_exists(pCtx, psql, bufTableName, &b_exists)  );
    if (b_exists)
    {
        int rc = -1;

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "SELECT child_id FROM %s", bufTableName)  );
        while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
        {
            const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh, psz_csid)  );
        }
        if (rc != SQLITE_DONE)
        {
            SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
        }
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    }

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
}

static void store_leaves_from_vhash(
    SG_context* pCtx,
    sqlite3* psql,
    SG_uint64 dagnum,
    SG_vhash* pvh_leaves,
    my_state* pst
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    char bufTableName[BUF_LEN_TABLE_NAME];

    SG_ERR_CHECK(  _get_table_name(pCtx, DAG_LEAVES_TABLE_NAME, dagnum, bufTableName)  );
    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "DELETE FROM %s", bufTableName)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_leaves, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_csid = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_leaves, i, &psz_csid, NULL)  );
        SG_ERR_CHECK(  _my_mark_as_leaf(pCtx, pst, psz_csid)  );
    }

fail:
    ;
}

void SG_dag_sqlite3__insert_frag(SG_context* pCtx,
								 sqlite3* psql,
								 SG_dagfrag * pFrag,
								 SG_rbtree ** ppIdsetMissing,
								 SG_stringarray ** ppsa_new,
                                 SG_uint32 flags
                                 )
{
	// The caller of this routine should manage a sqlite transaction.
	//
	// try to insert the contents of this fragment into our dag.
	// this takes 2 steps:
	// [1] determine if fragment is "connected" to our dag.  that is,
	//     if we already know about everything in the end-fringe so
	//     that the resulting graph is completely connected (and we
	//     assume that the current graph is completely connected before
	//     we start).
	// [2] add individual dagnodes in ancestor order.
	//
	// if [1] reports a disconnected graph, we return SG_ERR_CANNOT_CREATE_SPARSE_DAG
	// and an rbtree of the nodes we are missing.
	//
	// during [2] we accumulate the HID of nodes that were "new to us"
	// and return that.
	//
	// the caller must free both idsets.

	struct _state_member state_member;
	SG_bool bConnected = SG_FALSE;
    SG_bool b_pop = SG_FALSE;
	
	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(pFrag);
	SG_NULLARGCHECK_RETURN(ppIdsetMissing);
	SG_NULLARGCHECK_RETURN(ppsa_new);

	*ppIdsetMissing = NULL;
	*ppsa_new = NULL;

	SG_ERR_CHECK_RETURN(  _my_is_connected(pCtx, psql, pFrag, &bConnected, ppIdsetMissing)  );
	if (!bConnected)
    {
#if 0
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_rbtree__to_vhash__keys_only(pCtx, *ppIdsetMissing, &pvh)  );
        SG_VHASH_STDERR(pvh);
        SG_VHASH_NULLFREE(pCtx, pvh);
#endif
		SG_ERR_THROW_RETURN(SG_ERR_CANNOT_CREATE_SPARSE_DAG);
    }

    memset(&state_member, 0, sizeof(state_member));
    state_member.st.flags = flags;
	state_member.st.psql = psql;
	SG_ERR_CHECK_RETURN(  SG_dagfrag__get_dagnum(pCtx, pFrag, &state_member.st.iDagNum)  );
	state_member.ppsa_new = ppsa_new;
    state_member.b_not_really = SG_FALSE;
	state_member.pstmt = NULL;

    SG_ERR_CHECK(  get_leaves_into_vhash(pCtx, psql, state_member.st.iDagNum, &state_member.st.pvh_leaves)  );

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing DAG", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;
    SG_ERR_CHECK(  SG_dagfrag__dagnode_count(pCtx, pFrag, &state_member.steps_total)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, state_member.steps_total, "nodes")  );
	SG_ERR_CHECK(  SG_dagfrag__foreach_member(pCtx, pFrag, _my_insert_member_callback__sqlite3, (void *)&state_member)  );
    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

    SG_ERR_CHECK(  store_leaves_from_vhash(pCtx, psql, state_member.st.iDagNum, state_member.st.pvh_leaves, &state_member.st)  );

#if 0
    SG_ERR_CHECK(  SG_dag_sqlite3__check_consistency(pCtx, psql, state_member.st.iDagNum)  );
#endif

fail:
    SG_ERR_IGNORE(  free_my_state_stuff(pCtx, &state_member.st)  );
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
}

//////////////////////////////////////////////////////////////////

static FN__sg_fetch_dagnode my_fetch_dagnodes;

struct my_fetch_data
{
    sqlite3* psql;
    SG_dag_sqlite3_fetch_dagnodes_handle* pfdh;
};

static void my_fetch_dagnodes(SG_context* pCtx, void * pCtxFetchDagnode, SG_uint64 iDagNum, const char * szHidDagnode, SG_dagnode ** ppDagnode)
{
    struct my_fetch_data* pfd = (struct my_fetch_data*) pCtxFetchDagnode;

    SG_UNUSED(iDagNum);

	SG_ERR_CHECK_RETURN(  SG_dag_sqlite3__fetch_dagnodes__one(pCtx, pfd->psql, pfd->pfdh, szHidDagnode, ppDagnode)  );
}

void SG_dag_sqlite3__get_lca(
	SG_context* pCtx,
    sqlite3* psql,
    SG_uint64 iDagNum,
    const SG_rbtree* prbNodes,
	SG_daglca ** ppDagLca
    )
{
	SG_daglca * pDagLca = NULL;
    struct my_fetch_data fd;

    fd.psql = psql;
    fd.pfdh = NULL;
    SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnodes__begin(pCtx, iDagNum, &fd.pfdh)  );

	SG_ERR_CHECK(  SG_daglca__alloc(pCtx, &pDagLca,iDagNum,my_fetch_dagnodes,&fd)  );
	SG_ERR_CHECK(  SG_daglca__add_leaves(pCtx, pDagLca,prbNodes)  );
	SG_ERR_CHECK(  SG_daglca__compute_lca(pCtx, pDagLca)  );

    SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnodes__end(pCtx, &fd.pfdh)  );

	*ppDagLca = pDagLca;

    return;

fail:
    if (fd.pfdh)
    {
        SG_ERR_IGNORE(  SG_dag_sqlite3__fetch_dagnodes__end(pCtx, &fd.pfdh)  );
    }
	SG_DAGLCA_NULLFREE(pCtx, pDagLca);
}


