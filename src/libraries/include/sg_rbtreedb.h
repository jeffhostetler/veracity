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

#ifndef SG_RBTREEDB_H_
#define SG_RBTREEDB_H_

//////////////////////////////////////////////////////////////////

/**
 * SG_rbtreedb: An rbtree-db is a generic class that allows you to
 * load and store an in-memory rbtree to a sqlite file on disk.
 *
 * Think of this as the base-class of a rbtree serialization;
 * in that the base class doesn't know anything about the contents
 * of the records stored in the rbtree (just like SG_rbtree doesn't
 * know).  This class knows how to manipulate the sqlite file and
 * drive the various loops and deal with transactions.
 *
 * In fact, this rbtree-db class does not even have a handle to
 * the rbtree.  Both the rbtree and rbtree-db are peers and
 * should be owned by a wrapper class that manipulates both of them.
 *
 * To use a rbtree-db the caller must provide a structure containing
 * a set of callback functions to help import/export each record.
 */
typedef struct _sg_rbtree_db_is_undefined SG_rbtreedb;

//////////////////////////////////////////////////////////////////

typedef void (SG_rbtreedb__fn_create_db)                   (SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL);
typedef void (SG_rbtreedb__fn_open_db)					   (SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL);
typedef void (SG_rbtreedb__fn_initialize_insert_statement) (SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL, sqlite3_stmt** ppStmt);
typedef void (SG_rbtreedb__fn_bind_data)                   (SG_context * pCtx, void * pVoidInst, sqlite3_stmt * pStmt, SG_bool * bSkipThisOne, const char * pszKey, void * pData);
typedef void (SG_rbtreedb__fn_prepare_select_statement)    (SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL, sqlite3_stmt ** ppStmt);
typedef void (SG_rbtreedb__fn_receive_select_row)          (SG_context * pCtx, void * pVoidInst, sqlite3_stmt * pStmt, SG_rbtree * prb);
typedef void (SG_rbtreedb__fn_delete_all_rows)             (SG_context * pCtx, void * pVoidInst, sqlite3 * pSQL);

typedef struct
{
	SG_rbtreedb__fn_create_db *                     fnPtr_create_db;
	SG_rbtreedb__fn_open_db *                       fnPtr_open_db;
	SG_rbtreedb__fn_initialize_insert_statement *   fnPtr_initialize_insert_statement;
	SG_rbtreedb__fn_bind_data *                     fnPtr_bind_data;
	SG_rbtreedb__fn_prepare_select_statement *      fnPtr_prepare_select_statement;
	SG_rbtreedb__fn_receive_select_row *            fnPtr_receive_select_row;
	SG_rbtreedb__fn_delete_all_rows *               fnPtr_delete_all_rows;
	
} SG_rbtreedb__fn_pointers;

//////////////////////////////////////////////////////////////////

void SG_rbtreedb__close_free(SG_context * pCtx,
							 SG_rbtreedb * pThis);

/**
 * Create or Open a rbtreedb on disk and prepare to load it into memory
 * or write to it.
 *
 * You must provide the set of callback functions to deal with each
 * record.
 *
 * pVoidInst is opaque data that will be passed to each callback.
 *
 * You must call __close_free() when finished to ensure that the sqlite
 * file is properly closed/cleaned up.
 */
void SG_rbtreedb__create_or_open(SG_context * pCtx,
								 const SG_pathname * pPathDbFile,
								 void * pVoidInst,
								 SG_rbtreedb__fn_pointers * fnPtrs,
								 SG_rbtreedb ** ppDB_return);

/**
 * Begin a transaction and (using the _insert_ and _bind_ callbacks)
 * cause each record in the peer rbtree to potentially be written to
 * the disk.
 */
void SG_rbtreedb__store_rbtree(
	SG_context* pCtx,
	SG_rbtreedb* pThis,
	SG_rbtree * prbt);

#if 0 // not currently needed
/**
 * Use a special transaction to potentially write one record to the disk.
 */
void SG_rbtreedb__store_single(
	SG_context* pCtx,
	SG_rbtreedb* pThis,
	SG_rbtree * prbt,
	const char * pszKey);
#endif

/**
 * Begin a transaction and (using the _select_ callbacks) cause the
 * on-disk sqlite db to be loaded into the peer rbtree.
 */
void SG_rbtreedb__load_rbtree(
	SG_context* pCtx,
	SG_rbtreedb* pThis,
	SG_rbtree * prbTree);

#if 0 // not currently needed
/**
 * Begin a special transaction to delete all rows/records from the
 * on-disk sqlite db.
 */
void SG_rbtreedb__reset(
	SG_context* pCtx,
	SG_rbtreedb* pThis);
#endif

//////////////////////////////////////////////////////////////////

#endif /* SG_RBTREEDB_H_ */
