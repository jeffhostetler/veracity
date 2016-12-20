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

#define MY_BUSY_TIMEOUT_MS      (30000)

typedef struct
{
	sqlite3 *						psql;
	SG_rbtreedb__fn_pointers *		fnPtrs;
	void *							pVoidInst;	// we do not own this.

} _rbtreedb_handle;

//////////////////////////////////////////////////////////////////////////

void SG_rbtreedb__close_free(SG_context* pCtx, SG_rbtreedb* pThis)
{
	_rbtreedb_handle* pMe = (_rbtreedb_handle*)pThis;

	if (pMe)
	{
		SG_ERR_CHECK_RETURN(  sg_sqlite__close(pCtx, pMe->psql)  );
		SG_NULLFREE(pCtx, pMe->fnPtrs);
		pMe->pVoidInst = NULL;			// we do not own this.
		SG_NULLFREE(pCtx, pMe);
	}
}

//////////////////////////////////////////////////////////////////

static void sg_rbtreedb__create(SG_context * pCtx,
								const SG_pathname * pPathDbFile,
								void * pVoidInst,
								SG_rbtreedb__fn_pointers * fnPtrs,
								SG_rbtreedb** pprbtreedb)
{
	_rbtreedb_handle* pMe = NULL;
	SG_rbtreedb* prbtreedb = NULL;
	SG_bool bCreated = SG_FALSE;
	SG_bool bInTx = SG_FALSE;

	SG_NULLARGCHECK_RETURN(fnPtrs);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pMe)  );
	prbtreedb = (SG_rbtreedb*)pMe;
	pMe->fnPtrs = fnPtrs;
	pMe->pVoidInst = pVoidInst;

	SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPathDbFile, SG_SQLITE__SYNC__NORMAL, &pMe->psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "PRAGMA journal_mode=WAL")  );
	bCreated = SG_TRUE;
	sqlite3_busy_timeout(pMe->psql, MY_BUSY_TIMEOUT_MS);

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION;")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  fnPtrs->fnPtr_create_db(pCtx, pVoidInst, pMe->psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION;")  );
	bInTx = SG_FALSE;

	SG_RETURN_AND_NULL(prbtreedb, pprbtreedb);

	/* fall through */
fail:
	if (bInTx && SG_CONTEXT__HAS_ERR(pCtx) && pMe && pMe->psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION;")  );
	SG_RBTREEDB_NULLFREE(pCtx, prbtreedb);
	if (bCreated && SG_CONTEXT__HAS_ERR(pCtx)) // If we created the database but we're failing, clean it up.
		SG_ERR_IGNORE(  SG_fsobj__rmdir__pathname(pCtx, pPathDbFile)  );
}

static void sg_rbtreedb__open(SG_context * pCtx,
							  const SG_pathname * pPathDbFile,
							  void * pVoidInst,
							  SG_rbtreedb__fn_pointers * fnPtrs,
							  SG_rbtreedb ** pprbtreedb)
{
	_rbtreedb_handle* pMe = NULL;
	SG_rbtreedb* prbtreedb = NULL;
	SG_bool bInTx = SG_FALSE;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pMe)  );
	prbtreedb = (SG_rbtreedb*)pMe;
	pMe->fnPtrs = fnPtrs;
	pMe->pVoidInst = pVoidInst;

	SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPathDbFile, SG_SQLITE__SYNC__NORMAL, &pMe->psql)  );
	sqlite3_busy_timeout(pMe->psql, MY_BUSY_TIMEOUT_MS);

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION;")  );
	bInTx = SG_TRUE;
	
	SG_ERR_CHECK(  fnPtrs->fnPtr_open_db(pCtx, pVoidInst, pMe->psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION;")  );
	bInTx = SG_FALSE;

	SG_RETURN_AND_NULL(prbtreedb, pprbtreedb);

	/* fall through */
fail:
	if (bInTx && SG_CONTEXT__HAS_ERR(pCtx) && pMe && pMe->psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION;")  );
	SG_RBTREEDB_NULLFREE(pCtx, prbtreedb);
}

void SG_rbtreedb__create_or_open(SG_context * pCtx,
								 const SG_pathname * pPathDbFile,
								 void * pVoidInst,
								 SG_rbtreedb__fn_pointers * fnPtrs,
								 SG_rbtreedb ** ppDB_return)
{
	SG_bool exists;
	SG_rbtreedb* pDb = NULL;
	SG_NULLARGCHECK_RETURN(fnPtrs);

	SG_ERR_CHECK_RETURN(  SG_fsobj__exists__pathname(pCtx, pPathDbFile, &exists, NULL, NULL)  );
	if (exists)
	{
		SG_ERR_CHECK(  sg_rbtreedb__open(pCtx, pPathDbFile, pVoidInst, fnPtrs, &pDb)  );
	}
	else
	{
		SG_ERR_CHECK(  sg_rbtreedb__create(pCtx, pPathDbFile, pVoidInst, fnPtrs, &pDb)  );
	}

	SG_RETURN_AND_NULL(pDb, ppDB_return);

	return;

fail:
	SG_RBTREEDB_NULLFREE(pCtx, pDb);
}

void SG_rbtreedb__store_rbtree(
	SG_context* pCtx,
	SG_rbtreedb* pThis,
	SG_rbtree * prbt)
{
	_rbtreedb_handle* pMe = (_rbtreedb_handle*)pThis;
	sqlite3_stmt* pStmt = NULL;
	SG_bool bInTx = SG_FALSE;
	SG_rbtree_iterator * prbtIt = NULL;
	SG_bool bContinue = SG_FALSE;
	const char * pszKey = NULL;
	void * pData = NULL;
	SG_bool bSkipThisOne = SG_FALSE;

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION;")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_delete_all_rows(pCtx, pMe->pVoidInst, pMe->psql)  );

	// Iterate over the rbtree, inserting each entry.

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &prbtIt, prbt, &bContinue, &pszKey, (void**)&pData)  );
	SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_initialize_insert_statement(pCtx, pMe->pVoidInst, pMe->psql, &pStmt)  );
	while (bContinue)
	{
		SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_bind_data(pCtx, pMe->pVoidInst, pStmt, &bSkipThisOne, pszKey, pData)  );
		if (bSkipThisOne == SG_FALSE)
			SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
		SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, prbtIt, &bContinue, &pszKey, (void**)&pData)  );
	}
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION;")  );
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, prbtIt);
	bInTx = SG_FALSE;
	return;

fail:
	SG_ERR_IGNORE(sg_sqlite__finalize(pCtx, pStmt));
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, prbtIt);
	if (bInTx && pMe && pMe->psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION;")  );
}

#if 0 // not currently needed
void SG_rbtreedb__store_single(
	SG_context* pCtx,
	SG_rbtreedb* pThis,
	SG_rbtree * prbt,
	const char * pszKey)
{
	_rbtreedb_handle* pMe = (_rbtreedb_handle*)pThis;
	sqlite3_stmt* pStmt = NULL;
	SG_bool bInTx = SG_FALSE;
	SG_bool bContinue = SG_FALSE;
	SG_bool bSkipThisOne = SG_FALSE;
	void * pData = NULL;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbt, pszKey, &bContinue, (void**)&pData)  );
	if (bContinue)
	{
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION;")  );
		bInTx = SG_TRUE;

		// Insert object

		SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_initialize_insert_statement(pCtx, pMe->pVoidInst, pMe->psql, &pStmt)  );
		SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_bind_data(pCtx, pMe->pVoidInst, pStmt, &bSkipThisOne, pszKey, pData)  );
		if (bSkipThisOne == SG_FALSE)
			SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

		SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION;")  );
		bInTx = SG_FALSE;
	}
	return;

fail:
	SG_ERR_IGNORE(sg_sqlite__finalize(pCtx, pStmt));
	if (bInTx && pMe && pMe->psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION;")  );
}
#endif

void SG_rbtreedb__load_rbtree(
	SG_context* pCtx,
	SG_rbtreedb* pThis,
	SG_rbtree * prbTree)
{
	_rbtreedb_handle* pMe = (_rbtreedb_handle*)pThis;
	sqlite3_stmt* pStmt = NULL;
	SG_int32 rc;

	SG_NULLARGCHECK_RETURN(pMe);
	SG_NULLARGCHECK_RETURN(prbTree);

	SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_prepare_select_statement(pCtx, pMe->pVoidInst, pMe->psql, &pStmt)  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_receive_select_row(pCtx, pMe->pVoidInst, pStmt, prbTree)  );
	}

fail:
	SG_ERR_IGNORE(sg_sqlite__finalize(pCtx, pStmt));
}

#if 0 // not currently needed
void SG_rbtreedb__reset(
	SG_context* pCtx,
	SG_rbtreedb* pThis)
{
	_rbtreedb_handle* pMe = (_rbtreedb_handle*)pThis;
	sqlite3_stmt* pStmt = NULL;
	SG_bool bInTx = SG_FALSE;

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION;")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  pMe->fnPtrs->fnPtr_delete_all_rows(pCtx, pMe->pVoidInst, pMe->psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION;")  );
	bInTx = SG_FALSE;
	return;

fail:
	SG_ERR_IGNORE(sg_sqlite__finalize(pCtx, pStmt));
	if (bInTx && pMe && pMe->psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION;")  );
}
#endif
