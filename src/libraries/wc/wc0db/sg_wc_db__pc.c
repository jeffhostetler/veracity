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
 * @file sg_wc_db__pc.c
 *
 * @details Routines associated with the PENDING STRUCTURAL CHANGES
 * table of the WC DB.  This table has info for the current/live working
 * directory *BUT* only for explicit structural changes (MOVES, RENAMES,
 * ADDS, DELETES) relative to the associated CSET; L0 ==> {tne_<hid>, pc_<hid>}
 *
 * It DOES NOT track implicit changes that we have to scan for
 * such as file-contents, attrbits.
 *
 * Update 2012/02/13: We do store in the table the content-HID for
 *                    non-directory items when MERGE is involved.
 *                    For example, so that we can tell if the item is
 *                    disposable or if it needs backup if the user
 *                    later deletes it before committing the merge).
 *
 * We also store info for SPARSE items (that we didn't populate
 * during CHECKOUT/UPDATE/etc).
 *
 * Update 2012/12/05: I'm adding optional columns for the dynamic
 *                    (content-hid, attrbits) fields for sparse items.
 *                    This is needed when we have a ADD-SPECIAL-U + SPARSE
 *                    item (becase the corresponding TNE table won't have
 *                    a row for this item to reference).  BUt it turns out
 *                    *that having these fields helps in other places too,
 *                     such as in a REVERT-ALL of a DELETED item that was
 *                     SPARSE before the delete.
 *
 * Update 2013/02/06: I'm adding the reference attrbits -- this is
 *                    primarily to remember their theoretical value.
 *                    The value we set the to (or should have set 
 *                    them to) during a MERGE (like hid-merge).
 *                    This is a problem because on Windows we can't
 *                    set the +x bit on an actual file.  When someone
 *                    calls __get_current_attrbits() we need to use
 *                    this reference value and AND/OR it with the
 *                    currently observed value.  See W3801.
 *                    For example, if on Windows you do a MERGE and
 *                    get an added-by-merge file with +x set, the
 *                    file in the WC won't have it set (because
 *                    Windows doesn't have that capability), so
 *                    when __get_current_attrbits() is called we need
 *                    to report that +x should be set.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_db__pc__create_table(SG_context * pCtx,
								sg_wc_db * pDb,
								const sg_wc_db__cset_row * pCSetRow)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE TABLE %s"
											   "  ("
											   "    alias_gid         INTEGER PRIMARY KEY REFERENCES tbl_gid( alias_gid ),"
											   "    alias_gid_parent  INTEGER NOT NULL    REFERENCES tbl_gid( alias_gid ),"
											   "    type              INTEGER NOT NULL,"
											   "    flags_net         INTEGER NOT NULL,"
											   "    entryname         VARCHAR NOT NULL,"
											   "    hid_merge         VARCHAR NULL,"
											   "    sparse_attrbits   INTEGER NULL,"
											   "    sparse_hid        VARCHAR NULL,"
											   "    ref_attrbits      INTEGER NULL"
											   "  )"),
											  pCSetRow->psz_pc_table_name)  );
}

/**
 * Create an INDEX on the above table.
 * We probably only need this for the L0 cset
 * since we drive everything from it.  If we
 * have tables for L1 (or others) we tend to
 * use them for random lookups from a loop or
 * something driven from L0, so we don't really
 * need indexes for them.  But we DO allow it.
 * (See the tricks that UPDATE plays with the
 * DB.)
 *
 */
void sg_wc_db__pc__create_index(SG_context * pCtx,
								sg_wc_db * pDb,
								const sg_wc_db__cset_row * pCSetRow)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	// This is not a UNIQUE index because I need to
	// have DELETED items in the table (to distinguish
	// between UNCHANGED items which won't have a row
	// because this is a delta table).

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE INDEX ndx_%s ON %s ( alias_gid_parent, entryname )"),
											  pCSetRow->psz_pc_table_name,
											  pCSetRow->psz_pc_table_name)  );
}

void sg_wc_db__pc__drop_named_table(SG_context * pCtx,
									sg_wc_db * pDb,
									const sg_wc_db__cset_row * pCSetRow)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__free_cached_statements(pCtx, pDb)  );

	// DROP INDEX is implicit when referenced table is dropped.

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("DROP TABLE IF EXISTS %s"),
											  pCSetRow->psz_pc_table_name)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__pc__insert_self_immediate(SG_context * pCtx,
										 sg_wc_db * pDb,
										 const sg_wc_db__cset_row * pCSetRow,
										 const sg_wc_db__pc_row * pPcRow)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__pc__prepare_insert_stmt(pCtx, pDb, pCSetRow, pPcRow, &pStmt)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void sg_wc_db__pc__prepare_insert_stmt(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow,
									   const sg_wc_db__pc_row * pPcRow,
									   sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__pc__prepare_insert_stmt: about to insert/replace into: %s\n",
							   pCSetRow->psz_pc_table_name)  );
	SG_ERR_IGNORE(  sg_wc_db__debug__pc_row__print(pCtx, pPcRow)  );
#endif
#if 0 && defined(DEBUG)
	{
		SG_bool bSparse_flags = ((pPcRow->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE) != 0);
		SG_bool bSparse_d     = (pPcRow->p_d_sparse != NULL);
		SG_ASSERT_RELEASE_FAIL2( (bSparse_flags == bSparse_d),
								 (pCtx, "pc__prepare_insert_stmt: [%d %d] %s",
								  bSparse_flags, bSparse_d, pPcRow->p_s->pszEntryname) );
		if (pPcRow->p_d_sparse)
			SG_ASSERT_RELEASE_FAIL2( ((pPcRow->p_d_sparse->attrbits == 0) || (pPcRow->p_d_sparse->attrbits == 1)),
									 (pCtx, "pc__prepare_insert_stmt: attrbits crazy: %s",
									  pPcRow->p_s->pszEntryname)  );
	}
#endif

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ASSERT_RELEASE_FAIL(  ((pPcRow->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__INVALID) == 0)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO %s"
									   "  ("
									   "    alias_gid,"         // 1
									   "    alias_gid_parent,"  // 2
									   "    type,"              // 3
									   "    flags_net,"         // 4
									   "    entryname,"         // 5
									   "    hid_merge,"         // 6 -- optional
									   "    sparse_attrbits,"   // 7 -- optional
									   "    sparse_hid,"        // 8 -- optional
									   "    ref_attrbits"       // 9
									   "  )"
									   "  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"),
									  pCSetRow->psz_pc_table_name)  );

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,     pStmt, 1, pPcRow->p_s->uiAliasGid)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,     pStmt, 2, pPcRow->p_s->uiAliasGidParent)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,       pStmt, 3, pPcRow->p_s->tneType)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,     pStmt, 4, pPcRow->flags_net)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text__transient(pCtx,      pStmt, 5, pPcRow->p_s->pszEntryname)  );

	if (pPcRow->pszHidMerge && *pPcRow->pszHidMerge)
		SG_ERR_CHECK(  sg_sqlite__bind_text__transient(pCtx,  pStmt, 6, pPcRow->pszHidMerge)  );
	else
		SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx,  pStmt, 6)  );

	if (pPcRow->p_d_sparse)
		SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 7, pPcRow->p_d_sparse->attrbits)  );
	else
		SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx,  pStmt, 7)  );

	if (pPcRow->p_d_sparse && pPcRow->p_d_sparse->pszHid && *pPcRow->p_d_sparse->pszHid)
		SG_ERR_CHECK(  sg_sqlite__bind_text__transient(pCtx,  pStmt, 8, pPcRow->p_d_sparse->pszHid)  );
	else
		SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx,  pStmt, 8)  );

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 9, pPcRow->ref_attrbits)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__pc__prepare_delete_stmt(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow,
									   SG_uint64 uiAliasGid,
									   sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_DB
	{
		SG_int_to_string_buffer bufui64;
		
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__pc__prepare_delete_stmt: about to delete from %s: %s\n",
								   pCSetRow->psz_pc_table_name,
								   SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}
#endif

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM %s WHERE alias_gid = ?"),
									  pCSetRow->psz_pc_table_name)  );

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * During a COMMIT we move info for any dirty rows into the tne table
 * (and remove them from the pc table) as the changes become part of
 * the resulting changeset.
 *
 * This routine is used when we are committing a dirty+sparse item
 * (such as the move/rename of a sparse item).  Our caller has already
 * updated the tne table and is now asking us to "fix up" the row in
 * the pc table.
 *
 * Basically we need to update the row and remove any indication of
 * dirt (since it will be clean relative to the new baseline), but
 * leave the item marked SPARSE.
 *
 * There is an assumption here that when we commit an item that
 * we completely commit it -- that is, we don't currently support
 * committing the rename, but not the move of an item.
 *
 */
void sg_wc_db__pc__prepare_clean_but_leave_sparse_stmt(SG_context * pCtx,
													   sg_wc_db * pDb,
													   const sg_wc_db__cset_row * pCSetRow,
													   SG_uint64 uiAliasGid,
													   sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_DB
	{
		SG_int_to_string_buffer bufui64;
		
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db__pc__prepare_clean_but_leave_sparse_stmt: about to clean+sparse %s: %s\n",
								   pCSetRow->psz_pc_table_name,
								   SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}
#endif

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("UPDATE %s"
									   "   SET"
									   "     flags_net = ?,"
									   "     hid_merge = ?"
									   "   WHERE"
									   "     alias_gid = ?"),
									  pCSetRow->psz_pc_table_name)  );

	// keep the existing {sparse_attrbits,sparse_hid} fields in the table.

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE)  );
	SG_ERR_CHECK(  sg_sqlite__bind_null( pCtx, pStmt, 2)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 3, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * The "tbl_pc" table is a delta table.  It only contains rows for items
 * that have been structurally changed in the working directory
 * (BY EXPLICIT COMMAND FROM THE USER).  This includes ADD, DELETE,
 * MOVE, RENAME.  So if an item under version control has not been
 * changed, there will NOT be a record in this table.
 *
 * GET requests on this table in GID (or equivalently GID-ALIAS)
 * space respect the delta-nature and return (false,null) when a
 * record isn't present.  That is, GID-based requests DO NOT do
 * the fallback-to-the-baseline-version trick; that is for a
 * higher layer.
 *
 * WARNING: We WILL return a row for a PENDED DELETE.
 *
 * Note: we also contain info for SPARSE items (that weren't populated
 * in the WD).
 * 
 */
void sg_wc_db__pc__get_row_by_alias(SG_context * pCtx,
									sg_wc_db * pDb,
									const sg_wc_db__cset_row * pCSetRow,
									SG_uint64 uiAliasGid,
									SG_bool * pbFound,
									sg_wc_db__pc_row ** ppPcRow)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__pc_row * pPcRow = NULL;
	int rc;
	SG_bool bFound = SG_FALSE;

	//Because all of this statement can be based off different tables,
	//we cache a prepared statement for each table that is requested.
	if (pDb->prbCachedSqliteStmts__get_row_by_alias == NULL)
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pDb->prbCachedSqliteStmts__get_row_by_alias)  );

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pDb->prbCachedSqliteStmts__get_row_by_alias, pCSetRow->psz_pc_table_name, &bFound, (void**)&pStmt) );
	if (bFound == SG_FALSE)
	{
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid_parent,"	// 0
									   "    type,"				// 1
									   "    flags_net,"			// 2
									   "    entryname,"			// 3
									   "    hid_merge,"			// 4
									   "    sparse_attrbits,"	// 5
									   "    sparse_hid,"		// 6
									   "    ref_attrbits"       // 7
									   "  FROM %s"
									   "  WHERE alias_gid = ?"),
									  pCSetRow->psz_pc_table_name)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pDb->prbCachedSqliteStmts__get_row_by_alias, pCSetRow->psz_pc_table_name, pStmt)  );
	}

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
	{
		SG_int_to_string_buffer bufui64;

		if ((rc == SQLITE_DONE) && pbFound)
		{
			*pbFound = SG_FALSE;
			*ppPcRow = NULL;
			goto done;
		}
		
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:%s can't find row for alias %s.",
						 pCSetRow->psz_pc_table_name,
						 SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}

	SG_ERR_CHECK(  sg_wc_db__pc_row__alloc(pCtx, &pPcRow)  );

	pPcRow->p_s->uiAliasGid = uiAliasGid;
	pPcRow->p_s->uiAliasGidParent = (SG_uint64)sqlite3_column_int64(pStmt, 0);
	pPcRow->p_s->tneType = (SG_uint32)sqlite3_column_int(pStmt, 1);
	pPcRow->flags_net = (SG_uint64)sqlite3_column_int64(pStmt, 2);
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 3), &pPcRow->p_s->pszEntryname)  );
	if (sqlite3_column_type(pStmt, 4) != SQLITE_NULL)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 4), &pPcRow->pszHidMerge)  );

	if (sqlite3_column_type(pStmt, 5) != SQLITE_NULL)
	{
		SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc(pCtx, &pPcRow->p_d_sparse)  );
		pPcRow->p_d_sparse->attrbits = (SG_uint64)sqlite3_column_int64(pStmt, 5);

		if (sqlite3_column_type(pStmt, 6) != SQLITE_NULL)
			SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 6), &pPcRow->p_d_sparse->pszHid)  );
	}

	// get the value of the 'ref_attrbutes' field.
	// I added the ref_attrbits column to fix W3801
	// so it should be non-null WD's created or
	// touched by a build after the fix.
	//
	// for WD's last touched by a version of VV prior
	// to this fix, we will get a NULL field (because
	// we did an ALTER TABLE during the OPEN).

	if (sqlite3_column_type(pStmt, 7) != SQLITE_NULL)
	{
		pPcRow->ref_attrbits = (SG_uint64)sqlite3_column_int64(pStmt, 7);
#if TRACE_WC_ATTRBITS
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "PC_get_row_by_alias: ref_attrbits set to ref_attrbits=%d for '%s'.\n",
								   (SG_uint32)(pPcRow->ref_attrbits),
								   pPcRow->p_s->pszEntryname)  );
#endif
	}
	else if (pPcRow->p_d_sparse)
	{
		// if the file is sparse, the sparse_attrbits field
		// has the same info as (what should be in the ref_attrbits
		// field.
		pPcRow->ref_attrbits = pPcRow->p_d_sparse->attrbits;
#if TRACE_WC_ATTRBITS
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "PC_get_row_by_alias: ref_attrbits set to sparse_attrbits=%d for '%s'.\n",
								   (SG_uint32)(pPcRow->ref_attrbits),
								   pPcRow->p_s->pszEntryname)  );
#endif
	}
	else
	{
		// we could look in the tne-entry for it, but
		// it won't be present for added-by-merge and
		// in cases where the attrbits were modified
		// by the merge -- which was the original reason
		// for W3801.
		//
		// just set it to 0 -- this might be the wrong
		// answer, but we'll only get here while still
		// suffering from the need to do the ALTER TABLE;
		// as soon as they update/merge/checkout/etc
		// the ref_attrbits field will be properly set.
		pPcRow->ref_attrbits = 0;
#if TRACE_WC_ATTRBITS
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "PC_get_row_by_alias: ref_attrbits set to 0 for '%s'.\n",
								   pPcRow->p_s->pszEntryname)  );
#endif
	}

    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
	SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__pc__get_row_by_alias: in %s found:\n",
							   pCSetRow->psz_pc_table_name)  );
	SG_ERR_IGNORE(  sg_wc_db__debug__pc_row__print(pCtx, pPcRow)  );
#endif

#if 0 && defined(DEBUG)
	{
		SG_bool bIsSparse_flags = ((pPcRow->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE) != 0);
		SG_bool bIsSparse_d     = (pPcRow->p_d_sparse != NULL);
		SG_ASSERT_RELEASE_FAIL2(  (bIsSparse_flags == bIsSparse_d),
								  (pCtx, "Expect sparse settings to match [%d %d] %s",
								   bIsSparse_flags, bIsSparse_d, pPcRow->p_s->pszEntryname)  );
		if (bIsSparse_d)
			SG_ASSERT_RELEASE_FAIL2( ((pPcRow->p_d_sparse->attrbits == 0) || (pPcRow->p_d_sparse->attrbits == 1)),
									 (pCtx, "Crazy attrbits: %s", pPcRow->p_s->pszEntryname)  );
	}
#endif

	if (pbFound)
		*pbFound = SG_TRUE;
	*ppPcRow = pPcRow;
	return;

fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
done:
    SG_ERR_IGNORE(  sg_sqlite__reset(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch all of the rows in the tbl_pc table that currently have
 * the given GID as a parent.
 *
 * WARNING: Pending DELETES are still in the table (with a flag bit
 * set) AND WE DO RETURN THEM HERE.  So be careful about how you
 * collect these in a container (because there could be another
 * entry with the same name as a deleted entry).
 *
 * We DO NOT inspect the state of the parent directory.  (We don't let
 * a DELETE of the parent directory affect this query (we assume that
 * the delete of the parent properly handled any items within it).
 *
 */
void sg_wc_db__pc__foreach_in_dir_by_parent_alias(SG_context * pCtx,
												  sg_wc_db * pDb,
												  const sg_wc_db__cset_row * pCSetRow,
												  SG_uint64 uiAliasGidParent,
												  sg_wc_db__pc__foreach_cb * pfn_cb,
												  void * pVoidData)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__pc_row * pPcRow = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid,"		// 0
									   "    type,"			// 1
									   "    flags_net,"		// 2
									   "    entryname,"		// 3
									   "    hid_merge,"		// 4
									   "    sparse_attrbits,"	// 5
									   "    sparse_hid,"		// 6
									   "    ref_attrbits"		// 7
									   "  FROM %s"
									   "  WHERE alias_gid_parent = ?"),
									  pCSetRow->psz_pc_table_name)  );

	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGidParent)  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		SG_ERR_CHECK(  sg_wc_db__pc_row__alloc(pCtx, &pPcRow)  );

		pPcRow->p_s->uiAliasGid = (SG_uint64)sqlite3_column_int64(pStmt, 0);
		pPcRow->p_s->uiAliasGidParent = uiAliasGidParent;
		pPcRow->p_s->tneType = (SG_uint32)sqlite3_column_int(pStmt, 1);
		pPcRow->flags_net = (SG_uint64)sqlite3_column_int64(pStmt, 2);
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 3), &pPcRow->p_s->pszEntryname)  );
		if (sqlite3_column_type(pStmt, 4) != SQLITE_NULL)
			SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 4), &pPcRow->pszHidMerge)  );

		if (sqlite3_column_type(pStmt, 5) != SQLITE_NULL)
		{
			SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc(pCtx, &pPcRow->p_d_sparse)  );
			pPcRow->p_d_sparse->attrbits = (SG_uint64)sqlite3_column_int64(pStmt, 5);

			if (sqlite3_column_type(pStmt, 6) != SQLITE_NULL)
				SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 6), &pPcRow->p_d_sparse->pszHid)  );
		}

		if (sqlite3_column_type(pStmt, 7) != SQLITE_NULL)
		{
			pPcRow->ref_attrbits = (SG_uint64)sqlite3_column_int64(pStmt, 7);
#if TRACE_WC_ATTRBITS
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "PC_foreach_in_dir_by_parent_alias: ref_attrbits set to ref_attrbits=%d for '%s'.\n",
									   (SG_uint32)(pPcRow->ref_attrbits),
									   pPcRow->p_s->pszEntryname)  );
#endif
		}
		else if (pPcRow->p_d_sparse)
		{
			pPcRow->ref_attrbits = pPcRow->p_d_sparse->attrbits;
#if TRACE_WC_ATTRBITS
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "PC_foreach_in_dir_by_parent_alias: ref_attrbits set to sparse_attrbits=%d for '%s'.\n",
									   (SG_uint32)(pPcRow->ref_attrbits),
									   pPcRow->p_s->pszEntryname)  );
#endif
		}
		else
		{
			pPcRow->ref_attrbits = 0;
#if TRACE_WC_ATTRBITS
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "PC_foreach_in_dir_by_parent_alias: ref_attrbits set to 0 for '%s'.\n",
									   pPcRow->p_s->pszEntryname)  );
#endif
		}

#if TRACE_WC_DB
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "sg_wc_db__pc__foreach_in_dir_by_parent_alias: in %s found:\n",
									   pCSetRow->psz_pc_table_name)  );
			SG_ERR_IGNORE(  sg_wc_db__debug__pc_row__print(pCtx, pPcRow)  );
		}
#endif
#if 0 && defined(DEBUG)
		{
			SG_bool bIsSparse_flags = ((pPcRow->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE) != 0);
			SG_bool bIsSparse_d     = (pPcRow->p_d_sparse != NULL);
			SG_ASSERT_RELEASE_FAIL2(  (bIsSparse_flags == bIsSparse_d),
									  (pCtx, "Expect sparse settings to match [%d %d] %s",
									   bIsSparse_flags, bIsSparse_d, pPcRow->p_s->pszEntryname)  );
			if (bIsSparse_d)
				SG_ASSERT_RELEASE_FAIL2( ((pPcRow->p_d_sparse->attrbits == 0) || (pPcRow->p_d_sparse->attrbits == 1)),
										 (pCtx, "Crazy attrbits: %s", pPcRow->p_s->pszEntryname)  );
		}
#endif

		// pass the pc_row by address so that the caller can steal it if they want to.
		SG_ERR_CHECK(  (*pfn_cb)(pCtx, pVoidData, &pPcRow)  );

		SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
}
