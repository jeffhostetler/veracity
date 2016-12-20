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
 * @file sg_wc_db__csets.c
 *
 * @details Routines associated with the CSETS table of the WC DB.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * The "tbl_csets" table contains a list of the HIDs of
 * the CSets active in the WC.
 *
 * This table contains a map of something like [ 'L0' ==> '<hid_cset>' ].
 *
 */

//////////////////////////////////////////////////////////////////

void sg_wc_db__csets__create_table(SG_context * pCtx, sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, pDb->psql,
											  ("CREATE TABLE tbl_csets"
											   "  ("
											   "    label          VARCHAR PRIMARY KEY,"
											   "    hid_cset       VARCHAR NOT NULL,"
											   "    tne_table_name VARCHAR NOT NULL,"
											   "    pc_table_name  VARCHAR NOT NULL,"
											   "    hid_super_root VARCHAR NOT NULL"
											   "  )"))  );
}

/**
 * We are given a label (like 'L0') for a CSET.
 * Use that to create the names of the (TNE,PC) tables
 * for this CSET.
 *
 */
void sg_wc_db__csets__insert(SG_context * pCtx,
							 sg_wc_db * pDb,
							 const char * pszLabel,
							 const char * pszHidCSet,
							 const char * pszHidSuperRoot)
{
	sqlite3_stmt * pStmt = NULL;
	SG_string * pString_tne = NULL;
	SG_string * pString_pc = NULL;
	char * pszTid = NULL;

	SG_NONEMPTYCHECK_RETURN( pszLabel );
	SG_NONEMPTYCHECK_RETURN( pszHidCSet );
	SG_NONEMPTYCHECK_RETURN( pszHidSuperRoot );

	// NOTE 2012/05/11 During the initial WC implementation, I had 2 fixed
	// NOTE            tables in the DB for the baseline "tne_L0" and the
	// NOTE            set of pended changes "tbl_pc".  During a MERGE, we
	// NOTE            added "tne_L1" (with no assocated _pc table).  (We
	// NOTE            kept the "tne_L1" table around until the merge is
	// NOTE            either committed or reverted.)
	// NOTE
	// NOTE            I am getting rid of the fixed table names for the
	// NOTE            "tne" and "pc" tables associated with a CSET.
	// NOTE
	// NOTE            The tbl_csets contains the mapping from symbolic
	// NOTE            CSET name (such as "L0" or "L1") to the actual SQL
	// NOTE            table names for the associated "tne" and "pc" tables.
	// NOTE
	// NOTE            All code that talks to sg_wc_db__tne__
	// NOTE            or sg_wc_db__pc__ passes a pCSetRow (allocated using
	// NOTE            a symbolic name and) that hides the table names.
	// NOTE
	// NOTE            Normally, these 2 or 3 tables are relatively fixed
	// NOTE            (since the baseline or the number of parents of the
	// NOTE            WD doesn't change except under certain circumstances).
	// NOTE
	// NOTE            [1] COMMIT updates the baseline (afterwards "L0"
	// NOTE                means the new baseline); it in-place updates the
	// NOTE                contents of the L0-associated tables (and drops
	// NOTE                the L1-associated tables, if present).
	// NOTE
	// NOTE            [2] MERGE creates the "L1" row in tbl_csets and creates
	// NOTE                the L1-associated tables as necessary.
	// NOTE
	// NOTE            [3] UPDATE is special.  In addition to re-arranging
	// NOTE                the contents of the WD, UPDATE needs to change the
	// NOTE                baseline ("L0" is now associated with this CSET)
	// NOTE                which means that the L0-associated "tne" table needs
	// NOTE                to be rebuilt to match the contents of the new CSET
	// NOTE                and the "pc" table (which represents the changes
	// NOTE                between the "tne" table and the current state of
	// NOTE                WD) needs re-expressed as a delta RELATIVE TO THE
	// NOTE                NEW VERSION of L0-tne.
	// NOTE
	// NOTE                As a side-effect of the new one-legged merge
	// NOTE                approach used by UPDATE, we already have the "tne"
	// NOTE                table for the new L0 in a table (the "L1" table
	// NOTE                used in the merge) and can easily create the "pc"
	// NOTE                table from the pMrg structure.  So at the end of
	// NOTE                UPDATE, we just need to rebind mapping in the
	// NOTE                "L0" row in tbl_csets to point to the new tables
	// NOTE                (and drop the original L0-associated "tne" and "pc"
	// NOTE                tables.
	// NOTE
	// NOTE                This also means that we should be able to do a
	// NOTE                'vv update --test --status' or 'vv update --test --diff'
	// NOTE                and then throw it all away without messing up.
	// NOTE
	// NOTE
	// NOTE            So for all of this to work, we need the "tne" and
	// NOTE            "pc" table names to not be fixed and not necessarily
	// NOTE            associated-with or tied-to a particular CSET HID.
	// NOTE            The row in tbl_csets is the only one to know what the
	// NOTE            binding/mapping is.
	// NOTE
	// NOTE            So I'm going to give the pair (and their assoicated
	// NOTE            indexes) a TID-based name.

	SG_ERR_CHECK(  SG_tid__alloc2(pCtx, &pszTid, 10)  );
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString_tne, "tne_%s", pszTid)  );
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pString_pc,  "pc_%s",  pszTid)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "wc_db__csets: insert '%s'==>'%s' (%s,%s) (%s)\n",
							   pszLabel,
							   pszHidCSet,
							   SG_string__sz(pString_tne),
							   SG_string__sz(pString_pc),
							   pszHidSuperRoot)  );
#endif
	
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("INSERT OR REPLACE INTO tbl_csets"
									   "  ("
									   "    label,"
									   "    hid_cset,"
									   "    tne_table_name,"
									   "    pc_table_name,"
									   "    hid_super_root"
									   "  )"
									   "  VALUES (?, ?, ?, ?, ?)"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszLabel)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszHidCSet  )  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 3, SG_string__sz(pString_tne))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 4, SG_string__sz(pString_pc))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 5, pszHidSuperRoot  )  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	SG_STRING_NULLFREE(pCtx, pString_tne);
	SG_STRING_NULLFREE(pCtx, pString_pc);
	SG_NULLFREE(pCtx, pszTid);
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_STRING_NULLFREE(pCtx, pString_tne);
	SG_STRING_NULLFREE(pCtx, pString_pc);
	SG_NULLFREE(pCtx, pszTid);
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__csets__update__replace_all(SG_context * pCtx,
										  sg_wc_db * pDb,
										  const char * pszLabel,
										  const char * pszHidCSet,
										  const char * psz_tne,
										  const char * psz_pc,
										  const char * pszHidSuperRoot)
{
	sqlite3_stmt * pStmt = NULL;

	SG_NONEMPTYCHECK_RETURN( pszLabel );
	SG_NONEMPTYCHECK_RETURN( pszHidCSet );
	SG_NONEMPTYCHECK_RETURN( psz_tne );
	SG_NONEMPTYCHECK_RETURN( psz_pc );
	SG_NONEMPTYCHECK_RETURN( pszHidSuperRoot );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "wc_db__csets: update '%s'==>'%s' (%s,%s) (%s)\n",
							   pszLabel,
							   pszHidCSet,
							   psz_tne,
							   psz_pc,
							   pszHidSuperRoot)  );
#endif
	
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("UPDATE tbl_csets"
									   "  SET hid_cset = ?,"
									   "      tne_table_name = ?,"
									   "      pc_table_name = ?,"
									   "      hid_super_root = ?"
									   "  WHERE label = ?"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszHidCSet  )  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, psz_tne)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 3, psz_pc)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 4, pszHidSuperRoot  )  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 5, pszLabel)  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__csets__update__cset_hid(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszLabel,
									   const char * pszHidCSet,
									   const char * pszHidSuperRoot)
{
	sqlite3_stmt * pStmt = NULL;
	
	SG_NONEMPTYCHECK_RETURN( pszLabel );
	SG_NONEMPTYCHECK_RETURN( pszHidCSet );
	SG_NONEMPTYCHECK_RETURN( pszHidSuperRoot );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "wc_db__csets: update '%s'==>'%s' (%s)\n",
							   pszLabel,
							   pszHidCSet,
							   pszHidSuperRoot)  );
#endif
	
	SG_ERR_CHECK_RETURN(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("UPDATE tbl_csets"
									   "  SET hid_cset = ?,"
									   "      hid_super_root = ?"
									   "  WHERE label = ?"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszHidCSet  )  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszHidSuperRoot  )  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 3, pszLabel)  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__cset_row__free(SG_context * pCtx, sg_wc_db__cset_row * pCSetRow)
{
	if (!pCSetRow)
		return;

	SG_NULLFREE(pCtx, pCSetRow->psz_label);
	SG_NULLFREE(pCtx, pCSetRow->psz_hid_cset);
	SG_NULLFREE(pCtx, pCSetRow->psz_tne_table_name);
	SG_NULLFREE(pCtx, pCSetRow->psz_pc_table_name);
	SG_NULLFREE(pCtx, pCSetRow->psz_hid_super_root);

	SG_NULLFREE(pCtx, pCSetRow);
}
	
static void _db__cset_row__alloc_from_select(SG_context * pCtx,
											 sg_wc_db__cset_row ** ppCSetRow,
											 sqlite3_stmt * pStmt)
{
	sg_wc_db__cset_row * pCSetRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pCSetRow)  );

	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 0), &pCSetRow->psz_label)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 1), &pCSetRow->psz_hid_cset)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 2), &pCSetRow->psz_tne_table_name)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 3), &pCSetRow->psz_pc_table_name)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char *)sqlite3_column_text(pStmt, 4), &pCSetRow->psz_hid_super_root)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "wc_db__csets: select found '%s'==>'%s' (%s,%s) (%s)\n",
							   pCSetRow->psz_label,
							   pCSetRow->psz_hid_cset,
							   pCSetRow->psz_tne_table_name,
							   pCSetRow->psz_pc_table_name,
							   pCSetRow->psz_hid_super_root)  );
#endif

	*ppCSetRow = pCSetRow;
	return;

fail:
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__csets__get_row_by_label(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszLabel,
									   SG_bool * pbFound,
									   sg_wc_db__cset_row ** ppCSetRow)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__cset_row * pCSetRow = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    label,"
									   "    hid_cset,"
									   "    tne_table_name,"
									   "    pc_table_name,"
									   "    hid_super_root"
									   "  FROM tbl_csets"
									   "  WHERE label = ?"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszLabel)  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
	{
		if ((rc == SQLITE_DONE) && pbFound)
		{
			*pbFound = SG_FALSE;
			*ppCSetRow = NULL;
			goto done;
		}
		
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_csets can't get row for label '%s'.", pszLabel)  );
	}

	SG_ERR_CHECK(  _db__cset_row__alloc_from_select(pCtx, &pCSetRow, pStmt)  );

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	if (pbFound)
		*pbFound = SG_TRUE;
	*ppCSetRow = pCSetRow;
	return;

fail:
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
done:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void sg_wc_db__csets__get_row_by_hid_cset(SG_context * pCtx,
										  sg_wc_db * pDb,
										  const char * pszHidCSet,
										  sg_wc_db__cset_row ** ppCSetRow)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__cset_row * pCSetRow = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    label,"
									   "    hid_cset,"
									   "    tne_table_name,"
									   "    pc_table_name,"
									   "    hid_super_root"
									   "  FROM tbl_csets"
									   "  WHERE hid_cset = ?"))  );

	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszHidCSet)  );

	if ((rc=sqlite3_step(pStmt)) != SQLITE_ROW)
		SG_ERR_THROW2(  SG_ERR_SQLITE(rc),
						(pCtx, "sg_wc_db:tbl_csets can't get row for cset '%s'.", pszHidCSet)  );

	SG_ERR_CHECK(  _db__cset_row__alloc_from_select(pCtx, &pCSetRow, pStmt)  );

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	*ppCSetRow = pCSetRow;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
}

//////////////////////////////////////////////////////////////////

/**
 * Iterate over all of the CSETS present in the WC.
 *
 */
void sg_wc_db__csets__foreach_in_wc(SG_context * pCtx,
									sg_wc_db * pDb,
									sg_wc_db__csets__foreach_cb * pfn_cb,
									void * pVoidData)
{
	sqlite3_stmt * pStmt = NULL;
	sg_wc_db__cset_row * pCSetRow = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("SELECT"
									   "    label,"
									   "    hid_cset,"
									   "    tne_table_name,"
									   "    pc_table_name,"
									   "    hid_super_root"
									   "  FROM tbl_csets"
									   "  ORDER BY label"))  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		SG_ERR_CHECK(  _db__cset_row__alloc_from_select(pCtx, &pCSetRow, pStmt)  );

		// pass the cset row by address so that the cb can steal it if they want to.
		SG_ERR_CHECK(  (*pfn_cb)(pCtx, pVoidData, &pCSetRow)  );
		SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
}

//////////////////////////////////////////////////////////////////

#if 0 // not currently needed
/**
 * Delete all rows from tbl_csets.
 *
 */
void sg_wc_db__csets__delete_all(SG_context * pCtx,
								 sg_wc_db * pDb)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM tbl_csets"))  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}
#endif

#if 0 // not currently needed
/**
 * Delete all rows except for the named one from tbl_csets.
 *
 */
void sg_wc_db__csets__delete_all_except_one(SG_context * pCtx,
											sg_wc_db * pDb,
											const char * pszLabelOfOneToKeep)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM tbl_csets WHERE label != ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszLabelOfOneToKeep)  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}
#endif

/**
 * Delete the named row from tbl_csets.
 *
 */
void sg_wc_db__csets__delete_row(SG_context * pCtx,
								 sg_wc_db * pDb,
								 const char * pszLabel)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pDb->psql, &pStmt,
									  ("DELETE FROM tbl_csets WHERE label = ?"))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszLabel)  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

static sg_wc_db__csets__foreach_cb _get_all__cb;

static void _get_all__cb(SG_context * pCtx,
						 void * pVoidData,
						 sg_wc_db__cset_row ** ppCSetRow)
{
	SG_vector * pvecList = (SG_vector *)pVoidData;

	SG_ERR_CHECK(  SG_vector__append(pCtx, pvecList, (*ppCSetRow), NULL)  );
	*ppCSetRow = NULL;

fail:
	return;
}

/**
 * Return the set of all CSETs currently involved.
 *
 */
void sg_wc_db__csets__get_all(SG_context * pCtx,
							  sg_wc_db * pDb,
							  SG_vector ** ppvecList)
{
	SG_vector * pvecList = NULL;

	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pvecList, 10)  );
	SG_ERR_CHECK(  sg_wc_db__csets__foreach_in_wc(pCtx, pDb, _get_all__cb, pvecList)  );

	*ppvecList = pvecList;
	return;

fail:
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pvecList, (SG_free_callback *)sg_wc_db__cset_row__free);
}
