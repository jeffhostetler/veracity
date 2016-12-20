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
 * @file sg_wc_db__tne.c
 *
 * @details Routines associated with the (tne_L0, tne_L1, ...)
 * tables of the WC DB.
 *
 * WE ONLY KNOW ABOUT THE COMPLETE SET OF TREENODES AND
 * TREENODE_ENTRIES FROM A CHANGESET AND HOW THEY ARE EXPRESSED
 * IN A SQLITE TABLE.
 *
 * WE DO NOT KNOW ANYTHING ABOUT THE CURRENT STATE OF THE
 * WORKING DIRECTORY NOR OF ANY PENDING CHANGES.
 *
 * ALL "repo-paths" are relative to the named changeset.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne__create_named_table(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE TABLE %s"
											   "  ("
											   "    alias_gid        INTEGER PRIMARY KEY REFERENCES tbl_gid( alias_gid ),"
											   "    alias_gid_parent INTEGER NOT NULL    REFERENCES tbl_gid( alias_gid ),"
											   "    hid              VARCHAR NOT NULL,"
											   "    type             INTEGER NOT NULL,"
											   "    attrbits         INTEGER NOT NULL,"
											   "    entryname        VARCHAR NOT NULL"
											   "  )"),
											  pCSetRow->psz_tne_table_name)  );
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
void sg_wc_db__tne__create_index(SG_context * pCtx,
								 sg_wc_db * pDb,
								 const sg_wc_db__cset_row * pCSetRow)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );
	
	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE INDEX ndx_%s ON %s ( alias_gid_parent, entryname )"),
											  pCSetRow->psz_tne_table_name,
											  pCSetRow->psz_tne_table_name)  );
}

void sg_wc_db__tne__drop_named_table(SG_context * pCtx,
									 sg_wc_db * pDb,
									 const sg_wc_db__cset_row * pCSetRow)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__free_cached_statements(pCtx, pDb)  );

	// DROP INDEX is implicit when the referenced table is dropped.

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("DROP TABLE IF EXISTS %s"),
											  pCSetRow->psz_tne_table_name)  );
}

//////////////////////////////////////////////////////////////////

/**
 * BIND the fields for an INSERT or UPDATE a single row for this TreeNodeEntry (not
 * recursive) to the tne_L0 table.  Caller must compute the
 * GID aliases for the foreign keys and have already added
 * rows to tbl_gid as necessary.
 */
void sg_wc_db__tne__bind_insert(SG_context * pCtx,
								sqlite3_stmt * pStmt,
								SG_uint64 uiAliasGid,
								SG_uint64 uiAliasGidParent,
								const SG_treenode_entry * pTreenodeEntry)
{
	const char * pszHid = NULL;				// we do not own this
	const char * pszEntryname = NULL;		// we do not own this
	SG_int64 attrbits;
	SG_treenode_entry_type tneType;

	// pull all of the fields from the TNE.

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTreenodeEntry, &pszHid)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTreenodeEntry, &pszEntryname)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTreenodeEntry, &tneType)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pTreenodeEntry, &attrbits)  );

#if TRACE_WC_ATTRBITS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
				   "TNE: bind_insert (%d) %s\n",
				   ((SG_uint32)attrbits),
				   pszEntryname)  );
#endif

	SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
	SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(          pCtx, pStmt, 1, uiAliasGid)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(          pCtx, pStmt, 2, uiAliasGidParent)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text__transient(pCtx, pStmt, 3, pszHid)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(            pCtx, pStmt, 4, tneType)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(          pCtx, pStmt, 5, attrbits)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text__transient(pCtx, pStmt, 6, pszEntryname)  );

fail:
	return;
}

void sg_wc_db__tne__bind_insert_and_step(SG_context * pCtx,
										 sqlite3_stmt * pStmt,
										 SG_uint64 uiAliasGid,
										 SG_uint64 uiAliasGidParent,
										 const SG_treenode_entry * pTreenodeEntry)
{
	SG_ERR_CHECK(  sg_wc_db__tne__bind_insert(pCtx, pStmt, uiAliasGid, uiAliasGidParent, pTreenodeEntry)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

fail:
	return;
}

/**
 * INSERT or UPDATE this TreeNodeEntry and if a directory,
 * dive into it and INSERT/UPDATE the contents of the directory.
 */
void sg_wc_db__tne__insert_recursive(SG_context * pCtx,
									 sg_wc_db * pDb,
									 const sg_wc_db__cset_row * pCSetRow,
									 sqlite3_stmt * pStmt,
									 SG_uint64 uiAliasGidParent,
									 SG_uint64 uiAliasGid,
									 const SG_treenode_entry * pTreenodeEntry)
{
	SG_treenode * pTreenode = NULL;
	SG_uint32 nrEntries, k;
	SG_treenode_entry_type tneType;

	SG_ERR_CHECK(  sg_wc_db__tne__bind_insert_and_step(pCtx, pStmt,
													   uiAliasGid,
													   uiAliasGidParent,
													   pTreenodeEntry)  );
	
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTreenodeEntry, &tneType)  );
	if (tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		const char * pszHid = NULL;				// we do not own this

		// dive into the contents of the directory and load them.

		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTreenodeEntry, &pszHid)  );
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pDb->pRepo, pszHid, &pTreenode)  );
	
		SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &nrEntries)  );
		for (k=0; k<nrEntries; k++)
		{
			const char * pszGid_k = NULL;							// we do not own this
			const SG_treenode_entry * pTreenodeEntry_k = NULL;		// we do not own this
			SG_uint64 uiAliasGid_k = 0;

			SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx,
																	pTreenode,
																	k,
																	&pszGid_k,
																	&pTreenodeEntry_k)  );

			SG_ERR_CHECK(  sg_wc_db__gid__insert(pCtx, pDb, pszGid_k)  );
			SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, pszGid_k, &uiAliasGid_k)  );

			SG_ERR_CHECK(  sg_wc_db__tne__insert_recursive(pCtx, pDb, pCSetRow, pStmt,
														   uiAliasGid, uiAliasGid_k,
														   pTreenodeEntry_k)  );
		}
	}
	
fail:
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
}

/**
 * Load the SuperRoot TreeNode from the Repo and completely
 * populate the tbl_L[k] table.
 *
 * WE DO NOT DROP/RECREATE THE TABLE BEFORE STARTING.
 * The caller should have prep'd the table if they want that.
 *
 * We DO NOT put the super-root in the tne_L0 table; we
 * start the tne_L0 table with the actual root "@/" (aka "@b/").
 * (Info on the super-root can be found in the tbl_csets.)
 * 
 */
void sg_wc_db__tne__process_super_root(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow,
									   sqlite3_stmt * pStmt,
									   const char * pszHidSuperRoot)
{
	SG_treenode * pTreenodeSuperRoot = NULL;
	const char * pszGidRoot = NULL;							// we don't own this
	const SG_treenode_entry * pTreenodeEntryRoot = NULL;	// we don't own this
	SG_uint64 uiAliasGidNull = 0;
	SG_uint64 uiAliasGidRoot = 0;

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx,
											   pDb->pRepo,
											   pszHidSuperRoot,
											   &pTreenodeSuperRoot)  );

#if defined(DEBUG)
	{
		// verify we have a well-formed super-root.  that is, the super-root treenode
		// should have exactly 1 treenode-entry -- the "@" (aka "@b/") directory.

		SG_uint32 nrEntries;
		
		SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenodeSuperRoot, &nrEntries)  );
		if (nrEntries != 1)
			SG_ERR_THROW(  SG_ERR_MALFORMED_SUPERROOT_TREENODE  );
	}
#endif

	SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx,
															pTreenodeSuperRoot,0,
															&pszGidRoot,
															&pTreenodeEntryRoot)  );
#if defined(DEBUG)
	{
		const char * pszEntrynameRoot = NULL;					// we don't own this
		SG_treenode_entry_type tneTypeRoot;

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTreenodeEntryRoot, &pszEntrynameRoot)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTreenodeEntryRoot, &tneTypeRoot)  );
		// we set the root's entryname to "@" (not "@b") for
		// historical reasons and because that is what is in
		// the treenode. (We don't want it to look like
		// a rename if/when the corresponding row is created
		// in the tbl_PC table (where it should have "@").)
		if ( (strcmp(pszEntrynameRoot,"@") != 0) || (tneTypeRoot != SG_TREENODEENTRY_TYPE_DIRECTORY) )
			SG_ERR_THROW(  SG_ERR_MALFORMED_SUPERROOT_TREENODE  );
	}
#endif

	// alias for null-root has already been added.
	SG_ERR_CHECK(  sg_wc_db__gid__insert(pCtx, pDb, pszGidRoot)  );
	SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, SG_WC_DB__GID__NULL_ROOT, &uiAliasGidNull)  );
	SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, pszGidRoot,               &uiAliasGidRoot)  );

	SG_ERR_CHECK(  sg_wc_db__tne__insert_recursive(pCtx, pDb, pCSetRow, pStmt,
												   uiAliasGidNull, uiAliasGidRoot,
												   pTreenodeEntryRoot)  );

fail:
	SG_TREENODE_NULLFREE(pCtx, pTreenodeSuperRoot);
}

/**
 * DROP and CREATE a fresh tbl_L[k] table and populate it with
 * the contents of the CSET with the given SuperRoot HID.
 */
void sg_wc_db__tne__populate_named_table(SG_context * pCtx,
										 sg_wc_db * pDb,
										 const sg_wc_db__cset_row * pCSetRow,
										 const char * pszHidSuperRoot,
										 SG_bool bCreateIndex)
{
    sqlite3_stmt * pStmt = NULL;

	// drop/create the TNE table for this cset.

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_wc_db__tne__drop_named_table(pCtx, pDb, pCSetRow)  );
	SG_ERR_CHECK(  sg_wc_db__tne__create_named_table(pCtx, pDb, pCSetRow)  );

	// start populating the table pair with the entire CSET.

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT INTO %s"
									   "  ("
									   "    alias_gid,"
									   "    alias_gid_parent,"
									   "    hid,"
									   "    type,"
									   "    attrbits,"
									   "    entryname"
									   "  )"
									   "  VALUES"
									   "  (?, ?, ?, ?, ?, ?)"),
									  pCSetRow->psz_tne_table_name)  );

	SG_ERR_CHECK(  sg_wc_db__tne__process_super_root(pCtx, pDb, pCSetRow, pStmt, pszHidSuperRoot)  );

	if (bCreateIndex)
		SG_ERR_CHECK(  sg_wc_db__tne__create_index(pCtx, pDb, pCSetRow)  );

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	return;

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne_row__free(SG_context * pCtx, sg_wc_db__tne_row * pTneRow)
{
	if (!pTneRow)
		return;

	SG_WC_DB__STATE_STRUCTURAL__NULLFREE(pCtx, pTneRow->p_s);
	SG_WC_DB__STATE_DYNAMIC__NULLFREE(pCtx, pTneRow->p_d);

	SG_NULLFREE(pCtx, pTneRow);
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void sg_wc_db__debug__tne_row__print(SG_context * pCtx, sg_wc_db__tne_row * pTneRow)
{
	SG_int_to_string_buffer bufui64_a;
	SG_int_to_string_buffer bufui64_b;
		
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db:tne_row: [gid %s][par %s][hid %s] %s\n",
							   SG_uint64_to_sz(pTneRow->p_s->uiAliasGid,       bufui64_a),
							   SG_uint64_to_sz(pTneRow->p_s->uiAliasGidParent, bufui64_b),
							   pTneRow->p_d->pszHid,
							   pTneRow->p_s->pszEntryname)  );
}
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne_row__alloc(SG_context * pCtx, sg_wc_db__tne_row ** ppTneRow)
{
	sg_wc_db__tne_row * pTneRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTneRow)  );
	SG_ERR_CHECK(  sg_wc_db__state_structural__alloc(pCtx, &pTneRow->p_s)  );
	SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc(pCtx, &pTneRow->p_d)  );

	*ppTneRow = pTneRow;
	return;

fail:
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch a row from the tne_L0 table.
 */
void sg_wc_db__tne__get_row_by_alias(SG_context * pCtx,
									 sg_wc_db * pDb,
									 const sg_wc_db__cset_row * pCSetRow,
									 SG_uint64 uiAliasGid,
									 SG_bool * pbFound,
									 sg_wc_db__tne_row ** ppTneRow)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__tne_row * pTneRow = NULL;
	int rc;

	// use <alias_gid> to fetch row of tne_L0

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid_parent,"	// 0
									   "    hid,"				// 1
									   "    type,"				// 2
									   "    attrbits,"			// 3
									   "    entryname"			// 4
									   "  FROM %s"
									   "  WHERE alias_gid = ?"),
									  pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );
	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
	{
		SG_int_to_string_buffer bufui64;

		if ((rc == SQLITE_DONE) && pbFound)
		{
			*pbFound = SG_FALSE;
			*ppTneRow = NULL;
			goto done;
		}

		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:%s can't find tne row for alias %s.",
						 pCSetRow->psz_tne_table_name,
						 SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}

	SG_ERR_CHECK(  sg_wc_db__tne_row__alloc(pCtx, &pTneRow)  );

	pTneRow->p_s->uiAliasGid       = uiAliasGid;
	pTneRow->p_s->uiAliasGidParent = (SG_uint64)sqlite3_column_int64(pStmt, 0);
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 1), &pTneRow->p_d->pszHid)  );
	pTneRow->p_s->tneType          = (SG_uint32)sqlite3_column_int(pStmt, 2);
	pTneRow->p_d->attrbits         = (SG_uint64)sqlite3_column_int64(pStmt, 3);
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 4), &pTneRow->p_s->pszEntryname)  );
	
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "sg_wc_db__tne__get_row_by_alias: found:\n")  );
	SG_ERR_IGNORE(  sg_wc_db__debug__tne_row__print(pCtx, pTneRow)  );
#endif
	
	if (pbFound)
		*pbFound = SG_TRUE;
	*ppTneRow = pTneRow;
	return;

fail:
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow);
done:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne__get_row_by_gid(SG_context * pCtx,
								   sg_wc_db * pDb,
								   const sg_wc_db__cset_row * pCSetRow,
								   const char * pszGid,
								   SG_bool * pbFound,
								   sg_wc_db__tne_row ** ppTneRow)
{
	SG_uint64 uiAliasGid = 0;

	SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pDb, pszGid, &uiAliasGid)  );
	SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_alias(pCtx, pDb, pCSetRow,
												   uiAliasGid,
												   pbFound, ppTneRow)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Iterate over all items that list the given gid as
 * their parent directory.
 *
 * Warning: if you give a bogus directory gid, we can't
 * tell -- our SELECT just won't find any.
 *
 */
void sg_wc_db__tne__foreach_in_dir_by_parent_alias(SG_context * pCtx,
												   sg_wc_db * pDb,
												   const sg_wc_db__cset_row * pCSetRow,
												   SG_uint64 uiAliasGidParent,
												   sg_wc_db__tne__foreach_cb * pfn_cb,
												   void * pVoidData)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__tne_row * pTneRow = NULL;
	int rc;

	SG_ARGCHECK_RETURN(  (uiAliasGidParent != SG_WC_DB__ALIAS_GID__UNDEFINED), uiAliasGidParent  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid,"			// 0
									   "    hid,"				// 1
									   "    type,"				// 2
									   "    attrbits,"			// 3
									   "    entryname"			// 4
									   "  FROM %s"
									   "  WHERE (alias_gid_parent = ?)"),
									  pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGidParent)  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		SG_ERR_CHECK(  sg_wc_db__tne_row__alloc(pCtx, &pTneRow)  );

		pTneRow->p_s->uiAliasGid       = (SG_uint64)sqlite3_column_int64(pStmt, 0);
		pTneRow->p_s->uiAliasGidParent = uiAliasGidParent;
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 1), &pTneRow->p_d->pszHid)  );
		pTneRow->p_s->tneType          = (SG_uint32)sqlite3_column_int(pStmt, 2);
		pTneRow->p_d->attrbits         = (SG_uint64)sqlite3_column_int64(pStmt, 3);
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 4), &pTneRow->p_s->pszEntryname)  );

		// pass the tne_row by address so that the caller can steal it if they want to.
		SG_ERR_CHECK(  (*pfn_cb)(pCtx, pVoidData, &pTneRow)  );

		SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow);
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow);
}

//////////////////////////////////////////////////////////////////

/**
 * Find the alias-gid for the root "@/" (aka "@b/").  Unlike the null-root,
 * this is not a compile-time constant.  (perhaps it should be.)
 *
 * Since neither the GID nor repo-path of the root directory
 * can ever change, the TX-layer caller can just ask us for
 * the alias of the root directory and not have to bother with
 * TNE/PC and/or the prescan/liveview stuff.
 *
 * We could/should use sg_wc_db__tne__foreach_in_dir_by_parent_alias()
 * and pass __NULL_ROOT and set up a callback to get the first result
 * rather than duplicating parts of that routine, but that felt like
 * too much trouble.  (But we could also verify that there is exactly
 * one row and it has the correct entryname.)
 * 
 */
void sg_wc_db__tne__get_alias_of_root(SG_context * pCtx,
									  sg_wc_db * pDb,
									  const sg_wc_db__cset_row * pCSetRow,
									  SG_uint64 * puiAliasGid_Root)
{
	sqlite3_stmt * pStmt = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid"			// 0
									   "  FROM %s"
									   "  WHERE (alias_gid_parent = ?)"),
									  pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, SG_WC_DB__ALIAS_GID__NULL_ROOT)  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
	{
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:%s can't find tne row for '@/' (aka '@b/').",
						 pCSetRow->psz_tne_table_name)  );
	}

	*puiAliasGid_Root = (SG_uint64)sqlite3_column_int64(pStmt, 0);

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne__prepare_insert_stmt(SG_context * pCtx,
										sg_wc_db * pDb,
										sg_wc_db__cset_row * pCSetRow,
										SG_uint64 uiAliasGid,
										SG_uint64 uiAliasGid_Parent,
										const SG_treenode_entry * pTNE,
										sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	// Note: This pStmt is used by wc5queue/wc5apply so we can't
	//       cache this statement and reset/re-bind the fields
	//       like we do during a __load_named_cset().
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO %s"
									   "  ("
									   "    alias_gid,"
									   "    alias_gid_parent,"
									   "    hid,"
									   "    type,"
									   "    attrbits,"
									   "    entryname"
									   "  )"
									   "  VALUES"
									   "  (?, ?, ?, ?, ?, ?)"),
									  pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  sg_wc_db__tne__bind_insert(pCtx,
											  pStmt,
											  uiAliasGid,
											  uiAliasGid_Parent,
											  pTNE)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void sg_wc_db__tne__prepare_delete_stmt(SG_context * pCtx,
										sg_wc_db * pDb,
										sg_wc_db__cset_row * pCSetRow,
										SG_uint64 uiAliasGid,
										sqlite3_stmt ** ppStmt)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM %s WHERE alias_gid = ?"),
									  pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGid)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Get the TNE Row for the (uiAliasGirParent, pszEntryname) pair.
 * This should not have any of the entryname ambiguity problems
 * that such a request on the live view might have.
 *
 */
void sg_wc_db__tne__get_row_by_parent_alias_and_entryname(SG_context * pCtx,
														  sg_wc_db * pDb,
														  const sg_wc_db__cset_row * pCSetRow,
														  SG_uint64 uiAliasGidParent,
														  const char * pszEntryname,
														  SG_bool * pbFound,
														  sg_wc_db__tne_row ** ppTneRow)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__tne_row * pTneRow = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    alias_gid,"			// 0
									   "    hid,"				// 1
									   "    type,"				// 2
									   "    attrbits"			// 3
									   "  FROM %s"
									   "  WHERE alias_gid_parent = ?"
									   "    AND entryname = ?"),
									  pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, uiAliasGidParent)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,  pStmt, 2, pszEntryname)  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
	{
		SG_int_to_string_buffer bufui64;

		if ((rc == SQLITE_DONE) && pbFound)
		{
			*pbFound = SG_FALSE;
			*ppTneRow = NULL;
			goto done;
		}

		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:%s can't find tne row for parent alias %s and entryname '%s'.",
						 pCSetRow->psz_tne_table_name,
						 SG_uint64_to_sz(uiAliasGidParent, bufui64),
						 pszEntryname)  );
	}

	SG_ERR_CHECK(  sg_wc_db__tne_row__alloc(pCtx, &pTneRow)  );

	pTneRow->p_s->uiAliasGid       = (SG_uint64)sqlite3_column_int64(pStmt, 0);
	pTneRow->p_s->uiAliasGidParent = uiAliasGidParent;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 1), &pTneRow->p_d->pszHid)  );
	pTneRow->p_s->tneType          = (SG_uint32)sqlite3_column_int(pStmt, 2);
	pTneRow->p_d->attrbits         = (SG_uint64)sqlite3_column_int64(pStmt, 3);
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszEntryname, &pTneRow->p_s->pszEntryname)  );

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "sg_wc_db__tne__get_row_by_parent_alias_and_entryname: found:\n")  );
	SG_ERR_IGNORE(  sg_wc_db__debug__tne_row__print(pCtx, pTneRow)  );
#endif

	if (pbFound)
		*pbFound = SG_TRUE;
	*ppTneRow = pTneRow;
	return;

fail:
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow);
done:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

/**
 * Take a domain-specific/relative repo-path and get
 * the GID ALIAS of the item.
 *
 * THIS IS STRICLTY BASED UPON THE FIXED CHANGESET
 * THAT WE ALREADY HAVE IN THE tne_* TABLE.  It does
 * not know about or account for any pending changes
 * in the WD; that is, it DOES NOT know about tbl_PC.
 *
 * We DO NOT know if the given domain is appropriate
 * for the given pCSetRow.  That is upto the caller.
 * For example, we expect them to map:
 *     'b' ==> "L0"
 *     'c' ==> "L1"
 * but we don't enforce that here.
 *
 */
void sg_wc_db__tne__get_alias_from_extended_repo_path(SG_context * pCtx,
													  sg_wc_db * pDb,
													  const sg_wc_db__cset_row * pCSetRow,
													  const char * pszBaselineRepoPath,
													  SG_bool * pbFound,
													  SG_uint64 * puiAliasGid)
{
	SG_varray * pva = NULL;
	sg_wc_db__tne_row * pTneRow_k = NULL;
	SG_uint64 uiAlias;
	SG_uint32 k, count;

	*pbFound = SG_FALSE;
	*puiAliasGid = 0;

	// TODO 2012/01/04 For now, require that an extended-prefix be
	// TODO            present in the repo-path.
	// TODO
	// TODO            We may relax this to allow a '/' current/live
	// TODO            domain repo-path eventually.

	SG_ASSERT_RELEASE_FAIL( ((pszBaselineRepoPath[0]=='@')
							 && (pszBaselineRepoPath[1]!='/')) );

	SG_ERR_CHECK(  SG_repopath__split_into_varray(pCtx, pszBaselineRepoPath, &pva)  );

	// the root directory should be "@b" and be contained in pva[0].
	// we have a direct line to its alias.
	SG_ERR_CHECK(  sg_wc_db__tne__get_alias_of_root(pCtx, pDb, pCSetRow, &uiAlias)  );
	
	SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
	for (k=1; k<count; k++)
	{
		const char * pszEntryname_k;
		SG_bool bFound_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, k, &pszEntryname_k)  );
		SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_parent_alias_and_entryname(pCtx, pDb, pCSetRow,
																			uiAlias, pszEntryname_k,
																			&bFound_k, &pTneRow_k)  );
		if (!bFound_k)
			goto fail;

		uiAlias = pTneRow_k->p_s->uiAliasGid;

		SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow_k);
	}

	*pbFound = SG_TRUE;
	*puiAliasGid = uiAlias;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow_k);
}

/**
 * Convert an alias into a domain-specific repo-path
 * relative to this cset.  This is based *STRICTLY*
 * upon the content of this tne_* table.  We DO NOT
 * know anything about pending changes or the tbl_pc
 * table.
 *
 * We also DO NOT know anything about the association
 * of domain and which tne_* table to use; that is the
 * responsibility of the caller.
 *
 * For example, it is usually the case that we have the
 * domain mapping:
 *     'b' ==> "L0"
 *     'c' ==> "L1"
 * but we don't assume that.
 *
 */
void sg_wc_db__tne__get_extended_repo_path_from_alias(SG_context * pCtx,
													  sg_wc_db * pDb,
													  const sg_wc_db__cset_row * pCSetRow,
													  SG_uint64 uiAliasGid,
													  char chDomain,
													  SG_string ** ppStringRepoPath)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_db__tne_row * pTneRow = NULL;

	SG_NULLARGCHECK_RETURN( pDb );
	SG_NULLARGCHECK_RETURN( pCSetRow );
	SG_ARGCHECK_RETURN( (strchr(("abcdef"        // 'g' is reserved
								 "hijklmnopqrs"  // 't' is reserved
								 "uvwxyz"        // '/' is reserved
								 "0123456789"),
								chDomain)), chDomain );
	SG_NULLARGCHECK_RETURN( ppStringRepoPath );

	SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_alias(pCtx, pDb, pCSetRow,
												   uiAliasGid,
												   NULL, &pTneRow)  );
	SG_ASSERT(  (pTneRow)  );

	if (pTneRow->p_s->uiAliasGidParent == SG_WC_DB__ALIAS_GID__NULL_ROOT)
	{
		SG_ASSERT_RELEASE_FAIL( (strcmp(pTneRow->p_s->pszEntryname, "@")==0) );

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringRepoPath)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringRepoPath, "@%c/", chDomain)  );
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_db__tne__get_extended_repo_path_from_alias(pCtx, pDb, pCSetRow,
																		pTneRow->p_s->uiAliasGidParent,
																		chDomain,
																		&pStringRepoPath)  );
		SG_ASSERT( pStringRepoPath );
		SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pStringRepoPath,
													 pTneRow->p_s->pszEntryname,
													 (pTneRow->p_s->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY))  );
	}
	
	*ppStringRepoPath = pStringRepoPath;
	pStringRepoPath = NULL;

fail:
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}
