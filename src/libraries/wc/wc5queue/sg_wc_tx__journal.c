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
 * @file sg_wc_tx__journal.c
 *
 * @details Accumulate a list of the commands/operations
 * during the QUEUE phase of the TX that we need/want to
 * perform on the working directory during the APPLY phase
 * of the TX.  These can be reported to the user (with some
 * pretty printing) as the VERBOSE/LOG and/or used as the
 * script for actually applying the changes.
 *
 * There are 2 journals that are built in parallel:
 * [1] pvecJournalStmts:
 *     the list of SQL statements that we need to do (in
 *     order).  these will bring the WC.DB upt to the
 *     state we have in the LVI/LVDs.
 * [2] pvaJournal:
 *     the script/log of operations.  this contains 2 types
 *     of items:
 *     [2a] informative ones (where the SQL peer took care
 *          of the details).
 *     [2b] non-sql operations (the actual move/rename/etc
 *          of items in the WD) needed to bring the WD in
 *          sync with the LVI/LVDs.
 *
 * TODO 2012/02/01 The pvaJournal does not have GIDs because
 * TODO            it didn't need them.  Think about maybe
 * TODO            adding them to the records.  This might
 * TODO            be helpful for unit testing.
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Create an "INSERT OR REPLACE INTO tbl_pc ..." statement
 * to record the current values of the pPcRow in the database.
 * We append this to the QUEUE of SQL STATEMENTS that we
 * will execute during the APPLY phase.
 *
 */
static void sg_wc_tx__journal__append__insert_pc_stmt(SG_context * pCtx,
													  SG_wc_tx * pWcTx,
													  const sg_wc_db__pc_row * pPcRow)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__pc__prepare_insert_stmt(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline, pPcRow, &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

static void sg_wc_tx__journal__append__delete_pc_stmt(SG_context * pCtx,
													  SG_wc_tx * pWcTx,
													  SG_uint64 uiAliasGid)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__pc__prepare_delete_stmt(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline, uiAliasGid, &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

static void sg_wc_tx__journal__append__clean_pc_but_leave_sparse_stmt(SG_context * pCtx,
																	  SG_wc_tx * pWcTx,
																	  SG_uint64 uiAliasGid)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__pc__prepare_clean_but_leave_sparse_stmt(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline, uiAliasGid, &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

static void sg_wc_tx__journal__append__insert_tne_stmt(SG_context * pCtx,
													   SG_wc_tx * pWcTx,
													   SG_uint64 uiAliasGid,
													   SG_uint64 uiAliasGid_Parent,
													   const SG_treenode_entry * pTNE)
{
	sqlite3_stmt * pStmt = NULL;

#if TRACE_WC_TNE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__journal__append__insert_tne_stmt:\n")  );
#endif

	SG_ERR_CHECK(  sg_wc_db__tne__prepare_insert_stmt(pCtx,
													  pWcTx->pDb, pWcTx->pCSetRow_Baseline,
													  uiAliasGid, uiAliasGid_Parent,
													  pTNE,
													  &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

static void sg_wc_tx__journal__append__delete_tne_stmt(SG_context * pCtx,
													   SG_wc_tx * pWcTx,
													   SG_uint64 uiAliasGid)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__tne__prepare_delete_stmt(pCtx,
													  pWcTx->pDb, pWcTx->pCSetRow_Baseline,
													  uiAliasGid,
													  &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * When SCANDIR/READDIR finds an uncontrolled item, it
 * immediately creates a TEMPORARY (gid, alias-gid) pair
 * and INSERTS it into the GID table (even when read-only).
 * (These temporary ids will be deleted from the GID table
 * when the TX is finished.)
 * 
 * When we ADD an item we need to QUEUE an SQL STATEMENT
 * to convert the TEMPORARY GID to a permanent GID.
 *
 */
static void sg_wc_tx__journal__append__toggle_gid_stmt(SG_context * pCtx,
													   SG_wc_tx * pWcTx,
													   const sg_wc_db__pc_row * pPcRow,
													   SG_bool bIsTmp)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__gid__prepare_toggle_tmp_stmt(pCtx,
														  pWcTx->pDb,
														  pPcRow->p_s->uiAliasGid,
														  bIsTmp,
														  &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 *
 *
 */
static void sg_wc_tx__journal__append__resolve_issue_stmt__sr(SG_context * pCtx,
															  SG_wc_tx * pWcTx,
															  SG_uint64 uiAliasGid,
															  SG_wc_status_flags statusFlags_x_xr_xu,
															  const SG_string * pStringResolve)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__issue__prepare_update_status__sr(pCtx,
															  pWcTx->pDb,
															  uiAliasGid,
															  statusFlags_x_xr_xu,
															  pStringResolve,
															  &pStmt)  );

	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

/**
 *
 *
 */
static void sg_wc_tx__journal__append__resolve_issue_stmt__s(SG_context * pCtx,
															 SG_wc_tx * pWcTx,
															 SG_uint64 uiAliasGid,
															 SG_wc_status_flags statusFlags_x_xr_xu)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__issue__prepare_update_status__s(pCtx,
															 pWcTx->pDb,
															 uiAliasGid,
															 statusFlags_x_xr_xu,
															 &pStmt)  );

	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

/**
 *
 *
 */
static void sg_wc_tx__journal__append__insert_issue_stmt(SG_context * pCtx,
														 SG_wc_tx * pWcTx,
														 SG_uint64 uiAliasGid,
														 SG_wc_status_flags statusFlags_x_xr_xu,
														 const SG_string * pStringIssue,
														 const SG_string * pStringResolve)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__issue__prepare_insert(pCtx,
												   pWcTx->pDb,
												   uiAliasGid,
												   statusFlags_x_xr_xu,
												   pStringIssue,
												   pStringResolve,
												   &pStmt)  );

	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

static void sg_wc_tx__journal__append__delete_issue_stmt(SG_context * pCtx,
														 SG_wc_tx * pWcTx,
														 SG_uint64 uiAliasGid)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__issue__prepare_delete(pCtx, pWcTx->pDb, uiAliasGid, &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Record QUEUED ADD in Journal.
 *
 * This consists of 3 parts:
 * [1] Add log entry to Journal.
 * [2] Add statement to SQL queue to take the TEMP status off of the alias-gid.
 * [3] Add statement to SQL queue to insert the PcRow.
 *
 */
void sg_wc_tx__journal__add(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const sg_wc_db__pc_row * pPcRow,
							const SG_string * pStringRepoPath)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__add: '%s'\n"),
							   SG_string__sz(pStringRepoPath))  );
#endif

	// [1]

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op", "add")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src", SG_string__sz(pStringRepoPath))  );

	// [2]

	SG_ERR_CHECK(  sg_wc_tx__journal__append__toggle_gid_stmt(pCtx, pWcTx, pPcRow, SG_FALSE)  );

	// [3]

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pPcRow)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Record QUEUED MOVE/RENAME in Journal.
 *
 * This consists of 2 parts:
 * [1] Add log entry to Journal
 * [2] Add statement to SQL queue to insert/update the PcRow.
 *
 *
 * bAfterTheFact means that the user is telling us about
 * it after the fact and we don't actually need to do
 * anything.
 *
 * bUseIntermediate means that the APPLY step will need
 * to use a temporary path (do a 2-step).
 *
 * bSrcIsSparse means that the user is moving/renaming a
 * SPARSE item (which we didn't populate) so we just need
 * to do the bookkeeping -- like bAfterTheFact.
 *
 */
void sg_wc_tx__journal__move_rename(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									const SG_string * pStringRepoPath_Src,
									const SG_string * pStringRepoPath_Dest,
									SG_bool bAfterTheFact,
									SG_bool bUseIntermediate)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse = SG_FALSE;

	// We allow limited support of moves/renames of sparse items.
	// In theory, they are just bookkeeping entries in the parent
	// directory/directories of the item.  That is, if the item is
	// itself sparse we don't actually have to call the fsobj code
	// to move/rename it on disk in the WD (since it isn't present);
	// we just need to update the LVI/LVDs.
	bSrcIsSparse = (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live));

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__move_rename: [after-the-fact %d][use-intermediate %d][src-sparse %d] '%s' --> '%s'\n"),
							   bAfterTheFact, bUseIntermediate, bSrcIsSparse,
							   SG_string__sz(pStringRepoPath_Src),
							   SG_string__sz(pStringRepoPath_Dest))  );
#endif

	// [1]

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op", "move_rename")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src", SG_string__sz(pStringRepoPath_Src))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "dest", SG_string__sz(pStringRepoPath_Dest))  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhRow, "after_the_fact", bAfterTheFact)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhRow, "use_intermediate", bUseIntermediate)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhRow, "src_sparse", bSrcIsSparse)  );

	// [2]

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Record QUEUED REMOVE in Journal.
 *
 * This consists of 2 parts:
 * [1] Add log entry to Journal.
 * [2] Add statement to SQL queue to insert the PcRow.
 *
 */
void sg_wc_tx__journal__remove(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   const sg_wc_db__pc_row * pPcRow,
							   SG_treenode_entry_type tneType,
							   const SG_string * pStringRepoPath,
							   SG_bool bIsUncontrolled,
							   const char * pszDisposition,
							   const SG_pathname * pPathBackup)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	const char * pszKey = NULL;
	SG_bool bFlatten = SG_FALSE;

#if defined(DEBUG)
	if (pPathBackup)
	{
		SG_ASSERT(  (tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)  );
	}
#endif

	switch (tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		pszKey = "remove_file";
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		pszKey = "remove_directory";
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		pszKey = "remove_symlink";
		break;

//	case SG_TREENODEENTRY_TYPE_SUBREPO:
//		pszKey = "remove_subrepo";
//		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		break;
	}

	// The pvaJournal provides 2 services:
	// [a] a list of operations for --test/--verbose.
	// [b] a list of operations to perform on the WD.
	//
	// When the disposition is "keep", we do not actually
	// remove it from the disk.

	if (bIsUncontrolled && (strcmp(pszDisposition,"keep")==0))
	{
		// When we have a:
		//     "vv remove <uncontrolled-item> --keep"
		// or more likely:
		//     "vv remove <directory-containing-uncontrolled-item> --keep"
		// we're not really doing anything to the uncontrolled
		// item, neither [a] nor [b], so don't bother logging it
		// in the journal.
	}
	else
	{
#if TRACE_WC_TX_JRNL
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__journal__remove: '%s' [disp %16s] '%s'\n"),
								   pszKey,
								   pszDisposition,
								   SG_string__sz(pStringRepoPath))  );
#endif

		// [1]

		SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          pszKey)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "disposition", pszDisposition)  );
		if (pPathBackup)
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "backup_path", SG_pathname__sz(pPathBackup))  );

		if (pPcRow->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__FLATTEN)
			bFlatten = SG_TRUE;
		SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "flatten", bFlatten)  );
	}
	
	// [2]
	//
	// We silently support deleting FOUND/IGNORED items as a convenience
	// inside a larger delete.  But we don't want PC records for uncontrolled
	// items in the the tbl_PC table.

	if (!bIsUncontrolled)
	{
		if (bFlatten)
		{
			// An ADD-SPECIAL-* and a REMOVE cancel each other out
			// and need to be removed from the PC table (in certain
			// contexts liks a REVERT-ALL where we don't want to see
			// them in future STATUS commands).
			SG_ERR_CHECK(  sg_wc_tx__queue__kill_pc_row(pCtx, pWcTx, pPcRow->p_s->uiAliasGid)  );
		}
		else
		{
			SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pPcRow)  );
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Record QUEUED UNDO-ADD in Journal.
 *
 * This consists of 3 parts:
 * [1] Add log entry to Journal.
 * [2] Add statement to SQL queue to delete the PcRow.
 * [3] Return the alias-gid back to have TEMP status.
 *
 */
void sg_wc_tx__journal__undo_add(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const sg_wc_db__pc_row * pPcRow,
								 SG_treenode_entry_type tneType,
								 const SG_string * pStringRepoPath,
								 const char * pszDisposition)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	const char * pszKey = NULL;

	switch (tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		pszKey = "undo_add_file";
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		pszKey = "undo_add_directory";
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		pszKey = "undo_add_symlink";
		break;

//	case SG_TREENODEENTRY_TYPE_SUBREPO:
//		pszKey = "undo_add_subrepo";
//		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		break;
	}

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__undo_add: '%s' [disp %20s] '%s'\n"),
							   pszKey,
							   pszDisposition,
							   SG_string__sz(pStringRepoPath))  );
#endif

	// [1]

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          pszKey)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "entryname",   pPcRow->p_s->pszEntryname)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "disposition", pszDisposition)  );

	// [2]

	SG_ERR_CHECK(  sg_wc_tx__journal__append__delete_pc_stmt(pCtx, pWcTx, pPcRow->p_s->uiAliasGid)  );

	// [3]

	SG_ERR_CHECK(  sg_wc_tx__journal__append__toggle_gid_stmt(pCtx, pWcTx, pPcRow, SG_TRUE)  );


fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Record a STORE BLOB command in the JOURNAL.
 *
 * This is only to get the blob added to the repo
 * during the APPLY phase.
 * 
 * We do not know anything about SQL.
 * 
 */
void sg_wc_tx__journal__store_blob(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const char * pszKey,
								   const char * pszRepoPath,
								   sg_wc_liveview_item * pLVI,
								   SG_int64 alias_gid,
								   const char * pszHid,
								   SG_uint64 sizeContent)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse = ((pLVI) ? SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live) : SG_FALSE);

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__store_blob: %s '%s' '%s' [sparse %d]\n"),
							   pszKey, pszRepoPath, pszHid, bSrcIsSparse)  );
#endif

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",    pszKey)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",   pszRepoPath)  );
	// TODO 2012/05/24 should we write 'gid' rather than 'alias' to the journal?
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "alias", alias_gid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",   pszHid)  );
	// 2012/03/28 The content size is informative.  We don't really need this
	//            when adding the actual blob HID to the CSET/Committing; I
	//            thought it might be nice to have if/when we print the journal
	//            during verbose outout.  This value may be 0 if the caller doesn't
	//            know it or it can't be computed yet or isn't well defined.
	SG_ERR_CHECK(  SG_vhash__add__int64(    pCtx, pvhRow, "size",  sizeContent)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "src_sparse",     bSrcIsSparse)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__commit__delete_tne(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_string * pStringRepoPath,
										   SG_int64 alias_gid)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__commit__delete_tne: '%s'\n"),
							   SG_string__sz(pStringRepoPath))  );
#endif

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          "delete_tne_row")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
	// TODO 2012/05/24 should we write 'gid' rather than 'alias' to the journal?
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "alias",       alias_gid)  );

	// Remove the row from the tne_L0 table, if it exists.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__delete_tne_stmt(pCtx, pWcTx, alias_gid)  );

fail:
	return;
}

void sg_wc_tx__journal__commit__insert_tne(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_string * pStringRepoPath,
										   SG_int64 alias_gid,
										   SG_int64 alias_gid_parent,
										   const SG_treenode_entry * pTNE)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__commit__insert_tne: '%s'\n"),
							   SG_string__sz(pStringRepoPath))  );
#endif

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          "insert_tne_row")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
	// TODO 2012/05/24 should we write 'gid' rather than 'alias' to the journal?
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "alias",       alias_gid)  );

	// Insert/Update the row in the tne_L0 table.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_tne_stmt(pCtx,
															  pWcTx,
															  alias_gid,
															  alias_gid_parent,
															  pTNE)  );

fail:
	return;
}

void sg_wc_tx__journal__commit__delete_pc(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  const SG_string * pStringRepoPath,
										  SG_int64 alias_gid)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__commit__delete_pc: '%s'\n"),
							   SG_string__sz(pStringRepoPath))  );
#endif

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          "delete_pc_row")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
	// TODO 2012/05/24 should we write 'gid' rather than 'alias' to the journal?
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "alias",       alias_gid)  );

	// Remove the row from the pc_L0 table, if it exists.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__delete_pc_stmt(pCtx, pWcTx, alias_gid)  );

fail:
	return;
}

/**
 * Clean the dirty bits on the pc row for this item, but
 * leave it marked sparse (and present in the pc table).
 *
 */
void sg_wc_tx__journal__commit__clean_pc_but_leave_sparse(SG_context * pCtx,
														  SG_wc_tx * pWcTx,
														  const SG_string * pStringRepoPath,
														  SG_int64 alias_gid)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__commit__clean_pc_but_leave_sparse: '%s'\n"),
							   SG_string__sz(pStringRepoPath))  );
#endif

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          "clean_pc_row_but_leave_sparse")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
	// TODO 2012/05/24 should we write 'gid' rather than 'alias' to the journal?
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "alias",       alias_gid)  );

	// delete the change bits from the row in the pc_L0 table, but leave it marked sparse.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__clean_pc_but_leave_sparse_stmt(pCtx, pWcTx, alias_gid)  );

fail:
	return;
}
													
//////////////////////////////////////////////////////////////////

/**
 * Record QUEUED SPECIAL-ALL in Journal.  This is, for example,
 * for items created by MERGE or UPDATE.
 *
 */
void sg_wc_tx__journal__add_special(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									const char * pszHidBlob,
									SG_int64 attrbits)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	const char * pszKey = NULL;	// we don't own this
	char * pszGid = NULL;
	SG_string * pStringRepoPath = NULL;
	SG_bool bSrcIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
	
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pStringRepoPath)  );


#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

	switch (pLVI->tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		pszKey = "special_add_file";
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		pszKey = "special_add_directory";
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		pszKey = "special_add_symlink";
		break;

//	case SG_TREENODEENTRY_TYPE_SUBREPO:
//		pszKey = "special_add_subrepo";
//		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		break;
	}

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__add_special: '%s' '%s' [sparse %d]\n"),
							   pszKey,
							   SG_string__sz(pStringRepoPath),
							   bSrcIsSparse)  );
#endif

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pWcTx->pDb,
													 pLVI->pPcRow_PC->p_s->uiAliasGid,
													 &pszGid)  );

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             pszKey)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",            pszHidBlob)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "src_sparse",     bSrcIsSparse)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, pvhRow, pvhRow)  );

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_NULLFREE(pCtx, pszGid);
}

void sg_wc_tx__journal__overwrite_file_from_file(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI,
												 const SG_string * pStringRepoPath,
												 const char * pszGid,
												 const SG_pathname * pPathTemp,
												 SG_int64 attrbits)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

#if TRACE_WC_TX_JRNL
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_tx__journal__overwrite_file_from_file: [%s, '%s'] %s\n",
								   SG_pathname__sz(pPathTemp),
								   SG_uint64_to_sz__hex(attrbits, bufui64),
								   SG_string__sz(pStringRepoPath))  );
	}
#endif
		
	if (bSrcIsSparse)
	{
		// This is more of an assert.  We can't allow the caller to overwrite
		// the contents of a file with a temp file when it is sparse, because
		// the {content-hid,attrbits} of a sparse file implicitly reference
		// the backing tbl_TNE and must refer to a blob in the repo.

		SG_ERR_THROW2(  SG_ERR_WC_IS_SPARSE,
						(pCtx, "Cannot overwrite file '%s' because it is sparse.",
						 SG_string__sz(pStringRepoPath))  );
	}

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             "overwrite_file_from_file")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "file",           SG_pathname__sz(pPathTemp))  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, pvhRow, pvhRow)  );

	// TODO 2013/02/07 For W3801 should this call __append__insert_pc_stmt()
	// TODO            when the given attrbits are different from what we
	// TODO            have stored ?

fail:
	return;
}

void sg_wc_tx__journal__overwrite_file_from_repo(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI,
												 const SG_string * pStringRepoPath,
												 const char * pszGid,
												 const char * pszHidBlob,
												 SG_int64 attrbits,
												 const SG_pathname * pPathBackup)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
	SG_int64 orig_attrbits;

#if defined(DEBUG)
	if (pPathBackup)
	{
		SG_ASSERT(  (pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)  );
		SG_ASSERT(  (bSrcIsSparse == SG_FALSE)  );
	}
#endif

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pWcTx, (SG_uint64 *)&orig_attrbits)  );
	
#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__journal__overwrite_file_from_repo: [hid %s] [attr %d-->%d] [backup %s] %s [sparse %d]\n",
							   pszHidBlob,
							   ((SG_uint32)orig_attrbits), ((SG_uint32)attrbits),
							   ((pPathBackup) ? SG_pathname__sz(pPathBackup) : "(null)"),
							   SG_string__sz(pStringRepoPath),
							   bSrcIsSparse)  );
#endif
		
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             "overwrite_file_from_repo")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",            pszHidBlob)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );
	if (pPathBackup)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "backup_path",    SG_pathname__sz(pPathBackup))  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "src_sparse",     bSrcIsSparse)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, pvhRow, pvhRow)  );

	if (bSrcIsSparse)
		SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );
	else if (attrbits != orig_attrbits)
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__overwrite_ref_attrbits(pCtx, pLVI, attrbits)  );
		SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );
	}

fail:
	return;
}

/**
 * Overwrite symlink {target,attrbits} using an arbitrary value.
 * This might come from a known value such as from 'vv resolve accept baseline...'
 * on a divergent-symlink conflict where we already have {hid,blob} in
 * the repo.  Or it might come from a totally new value, such as
 * 'vv resolve accept working...' where the user manually changed the
 * symlink target.  In the latter case we may not already have {hid,blob}
 * in the repo.
 *
 * Normally this does not matter because the repo doesn't need to contain
 * {hid,blob} until we COMMIT.  But with our new support for SPARSE items
 * and the p_d_sparse dynamic data in tbl_PC (which only stores the HID
 * but not the target), we need to ensure that {hid,blob} gets created
 * now **DURING THE CURRENT APPLY OPERATION** rather than waiting until
 * the COMMIT.  This deviates from the normal model in that we don't usually
 * create repo blobs until the COMMIT.  If the user actually does do a
 * COMMIT after the current operation, the blob we be there for them.
 * If the user does a REVERT-ALL, REVERT-ITEM, RESOLVE, or otherwise
 * retargets this sparse symlink, the {hid,blob} that we create here
 * will just be unreferenced in their repo -- a little wasteful, but
 * quite tiny.
 *
 */
void sg_wc_tx__journal__overwrite_symlink(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  sg_wc_liveview_item * pLVI,
										  const SG_string * pStringRepoPath,
										  const char * pszGid,
										  const char * pszNewTarget,
										  SG_int64 attrbits,
										  const char * pszHid)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse;

	bSrcIsSparse = (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live));

	// if (bSrcIsSparse)
	// {
	//     Since we don't track dynamic (non-structural) changes in tbl_PC
	//     and the user can't change a symlink target on a sparse item (which
	//     would require them to manually operate on a symlink that isn't
	//     populated), we can't/shouldn't get any requests on sparse symlinks
	//     *UNLESS* we are in an UPDATE/MERGE/REVERT/RESOLVE.  And since we don't
	//     do auto-merges on symlink targets, the only calls here will
	//     should have a well-known target value for the symlink (in the
	//     L0 or L1 tbl TNE).
	//     But I'm going to be nice and let it pass thru since we can always
	//     compute HID and store a new blob if needed.
	//
	//     So we queue/journal the new hid/target info (and update the in-tx pLVI),
	//     but tell APPLY to not bother SG_fsobj__symlink()).
	// }

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

#if TRACE_WC_TX_JRNL
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_tx__journal__overwrite_symlink: [==>%s, %s, '%s'] %s [sparse %d]\n",
								   pszNewTarget,
								   pszHid,
								   SG_uint64_to_sz__hex(attrbits, bufui64),
								   SG_string__sz(pStringRepoPath),
								   bSrcIsSparse)  );
	}
#endif
		
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             "overwrite_symlink")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "target",         pszNewTarget)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",            pszHid)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "src_sparse",     bSrcIsSparse)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, pvhRow, pvhRow)  );

	if (bSrcIsSparse)
		SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );

fail:
	return;
}

/**
 * Overwrite the target of the symlink using a known-to-be-in-the-repo {hid,blob}.
 *
 */
void sg_wc_tx__journal__overwrite_symlink_from_repo(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_liveview_item * pLVI,
													const SG_string * pStringRepoPath,
													const char * pszGid,
													SG_int64 attrbits,
													const char * pszHidBlob)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse;
	SG_string * pStringLink = NULL;
	SG_byte * pBytes = NULL;
	SG_uint64 nrBytes;

	// Go ahead and convert the HID into the actual text of the target.
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo, pszHidBlob,
												   &pBytes, &nrBytes)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pStringLink, pBytes, (SG_uint32)nrBytes)  );

	bSrcIsSparse = (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live));

	// if (bSrcIsSparse)
	// {
	//     Since we don't track dynamic (non-structural) changes in tbl_PC
	//     and the user can't change a symlink target on a sparse item (which
	//     would require them to manually operate on a symlink that isn't
	//     populated), we can't/shouldn't get any requests on sparse symlinks
	//     *UNLESS* we are in an UPDATE/MERGE/REVERT.  And since we don't
	//     do auto-merges on symlink targets, the only calls here will
	//     should have a well-known target value for the symlink (in the
	//     L0 or L1 tbl TNE).
	//
	//     So we queue/journal the new hid/target info (and update the in-tx pLVI),
	//     but tell APPLY to not bother SG_fsobj__symlink()).
	// }

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

#if TRACE_WC_TX_JRNL
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_tx__journal__overwrite_symlink_from_repo: [==>%s, %s, '%s'] %s [sparse %d]\n",
								   SG_string__sz(pStringLink),
								   pszHidBlob,
								   SG_uint64_to_sz__hex(attrbits, bufui64),
								   SG_string__sz(pStringRepoPath),
								   bSrcIsSparse)  );
	}
#endif
		
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             "overwrite_symlink")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "target",         SG_string__sz(pStringLink))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",            pszHidBlob)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "src_sparse",     bSrcIsSparse)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, pvhRow, pvhRow)  );

	if (bSrcIsSparse)
		SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringLink);
	SG_NULLFREE(pCtx, pBytes);
}

/**
 * Queue a stand-alone chmod to an item.
 *
 */
void sg_wc_tx__journal__set_attrbits(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 sg_wc_liveview_item * pLVI,
									 const SG_string * pStringRepoPath,
									 SG_int64 attrbits)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	SG_bool bSrcIsSparse;
	SG_int64 orig_attrbits;

	bSrcIsSparse = (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live));

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pWcTx, (SG_uint64 *)&orig_attrbits)  );

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__journal__set_attrbits: [attr %d-->%d] %s [sparse %d]\n",
							   ((SG_uint32)orig_attrbits), ((SG_uint32)attrbits),
							   SG_string__sz(pStringRepoPath), bSrcIsSparse)  );
#endif
		
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",          "set_attrbits")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",         SG_string__sz(pStringRepoPath))  );
	// TODO 2012/05/24 should we write 'gid' to journal?
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",    attrbits)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "src_sparse",  bSrcIsSparse)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, NULL, pvhRow)  );

	if (bSrcIsSparse)
		SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );
	else if (attrbits != orig_attrbits)
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__overwrite_ref_attrbits(pCtx, pLVI, attrbits)  );
		SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );
	}

fail:
	return;
}

void sg_wc_tx__journal__undo_delete(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									const SG_string * pStringRepoPath_Dest,
									const char * pszHidBlob,
									SG_int64 attrbits,
									const SG_string * pStringRepoPathTempFile)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	const char * pszKey = NULL;	// we don't own this
	char * pszGid = NULL;
	SG_bool bMakeSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath_Dest);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

	switch (pLVI->tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		pszKey = "undo_delete_file";
		SG_ASSERT(  ((pszHidBlob && *pszHidBlob) || (pStringRepoPathTempFile))  );
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		pszKey = "undo_delete_directory";
		SG_ASSERT(  (pszHidBlob == NULL)  );
		SG_ASSERT(  (pStringRepoPathTempFile == NULL)  );
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		pszKey = "undo_delete_symlink";
		SG_ASSERT(  (pszHidBlob && *pszHidBlob)  );
		SG_ASSERT(  (pStringRepoPathTempFile == NULL)  );
		break;

//	case SG_TREENODEENTRY_TYPE_SUBREPO:
//		pszKey = "undo_delete_subrepo";
//		SG_ASSERT(  (pszHidBlob && *pszHidBlob)  );
//		SG_ASSERT(  (pStringRepoPathTempFile == NULL)  );
//		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		break;
	}

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__undo_delete: '%s' '%s' [bMakeSparse %d]\n"),
							   pszKey,
							   SG_string__sz(pStringRepoPath_Dest),
							   bMakeSparse)  );
#endif

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pWcTx->pDb,
													 pLVI->pPcRow_PC->p_s->uiAliasGid,
													 &pszGid)  );

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             pszKey)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath_Dest))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	if (pszHidBlob)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",            pszHidBlob)  );
	if (pStringRepoPathTempFile)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "tempfile",   SG_string__sz(pStringRepoPathTempFile))  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhRow, "make_sparse",    bMakeSparse)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, ((pszHidBlob) ? pvhRow : NULL), pvhRow)  );

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );

fail:
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__undo_lost(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  sg_wc_liveview_item * pLVI,
								  const SG_string * pStringRepoPath_Dest,
								  const char * pszHidBlob,
								  SG_int64 attrbits,
								  const SG_string * pStringRepoPathTempFile)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	const char * pszKey = NULL;	// we don't own this
	char * pszGid = NULL;

#if defined(DEBUG)
	{
		// non current/live repo-paths should have been
		// converted by layers 6 & 7.
		const char * psz = SG_string__sz(pStringRepoPath_Dest);
		SG_ASSERT_RELEASE_FAIL( ((psz) && (psz[0]=='@') && (psz[1]==SG_WC__REPO_PATH_DOMAIN__LIVE)) );
	}
#endif

	switch (pLVI->tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		pszKey = "undo_lost_file";
		SG_ASSERT(  ((pszHidBlob && *pszHidBlob) || (pStringRepoPathTempFile))  );
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		pszKey = "undo_lost_directory";
		SG_ASSERT(  (pszHidBlob == NULL)  );
		SG_ASSERT(  (pStringRepoPathTempFile == NULL)  );
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		pszKey = "undo_lost_symlink";
		SG_ASSERT(  (pszHidBlob && *pszHidBlob)  );
		SG_ASSERT(  (pStringRepoPathTempFile == NULL)  );
		break;

//	case SG_TREENODEENTRY_TYPE_SUBREPO:
//		pszKey = "undo_lost_subrepo";
//		SG_ASSERT(  (pszHidBlob && *pszHidBlob)  );
//		SG_ASSERT(  (pStringRepoPathTempFile == NULL)  );
//		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		break;
	}

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__undo_lost: '%s' '%s'\n"),
							   pszKey,
							   SG_string__sz(pStringRepoPath_Dest))  );
#endif

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
													 pWcTx->pDb,
													 pLVI->pPcRow_PC->p_s->uiAliasGid,
													 &pszGid)  );

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",             pszKey)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",            SG_string__sz(pStringRepoPath_Dest))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",            pszGid)  );
	if (pszHidBlob)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "hid",            pszHidBlob)  );
	if (pStringRepoPathTempFile)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "tempfile",   SG_string__sz(pStringRepoPathTempFile))  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "attrbits",       attrbits)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__remember_last_overwrite(pCtx, pLVI, ((pszHidBlob) ? pvhRow : NULL), pvhRow)  );

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_pc_stmt(pCtx, pWcTx, pLVI->pPcRow_PC)  );

fail:
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

/**
 * Update the overall resolved status and the specific resolve-info
 * (even if null).
 *
 */
void sg_wc_tx__journal__resolve_issue__sr(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  SG_uint64 uiAliasGid,
										  SG_wc_status_flags statusFlags_x_xr_xu,
										  const SG_string * pStringResolve)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__journal__resolve_issue__sr: [x_xr_xu %s][%s]\n"),
								   SG_uint64_to_sz__hex(statusFlags_x_xr_xu, bufui64),
								   ((pStringResolve) ? SG_string__sz(pStringResolve) : ""))  );
	}
#endif

	// [1] Add log entry to the Journal

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",           "resolve_issue__sr")  );
	// I'm going to omit 'src' because there are some times when we can't
	// compute it and we don't really need it.  (Such as after an ADDED-by-UPDATE
	// followed by a DELETED-by-ACCEPT. (Remember we are running *after* the delete
	// has been QUEUED and pLVI updated) and there isn't an 'other' branch from
	// which to get an alternate.
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "x_xr_xu",      statusFlags_x_xr_xu)  );
	if (pStringResolve && (SG_string__length_in_bytes(pStringResolve) > 0))
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "resolve",  SG_string__sz(pStringResolve))  );

	// [2] Add statement to SQL queue to update the row in tbl_issues.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__resolve_issue_stmt__sr(pCtx,
																	 pWcTx,
																	 uiAliasGid,
																	 statusFlags_x_xr_xu,
																	 pStringResolve)  );

fail:
	return;
}

/**
 * Update the overall resolved status AND DO NOT ALTER the resolve-info.
 *
 */
void sg_wc_tx__journal__resolve_issue__s(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 SG_uint64 uiAliasGid,
										 SG_wc_status_flags statusFlags_x_xr_xu)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__journal__resolve_issue__s: [x_xr_xu %s]\n"),
								   SG_uint64_to_sz__hex(statusFlags_x_xr_xu, bufui64))  );
	}
#endif

	// [1] Add log entry to the Journal

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",           "resolve_issue__s")  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "x_xr_xu",      statusFlags_x_xr_xu)  );

	// [2] Add statement to SQL queue to update the row in tbl_issues.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__resolve_issue_stmt__s(pCtx,
																	pWcTx,
																	uiAliasGid,
																	statusFlags_x_xr_xu)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * 
 *
 */
void sg_wc_tx__journal__insert_issue(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 const SG_string * pStringRepoPath,
									 SG_uint64 uiAliasGid,
									 SG_wc_status_flags statusFlags_x_xr_xu,	// the initial __X__, __XR__, and __XU__ flags.
									 const SG_string * pStringIssue,
									 const SG_string * pStringResolve)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	{
		SG_int_to_string_buffer buf64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__journal__insert_issue: %s '%s' [%s] [%s]\n"),
								   SG_uint64_to_sz__hex(statusFlags_x_xr_xu, buf64),
								   SG_string__sz(pStringRepoPath),
								   SG_string__sz(pStringIssue),
								   ((pStringResolve) ? SG_string__sz(pStringResolve) : ""))  );
	}
#endif

	// [1] Add log entry to the Journal

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",           "insert_issue")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src",          SG_string__sz(pStringRepoPath))  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhRow, "x_xr_xu",      statusFlags_x_xr_xu)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "issue",        SG_string__sz(pStringIssue))  );
	if (pStringResolve && (SG_string__length_in_bytes(pStringResolve) > 0))
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "resolve",  SG_string__sz(pStringResolve))  );

	// [2] Add statement to SQL queue to update the row in tbl_issues.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__insert_issue_stmt(pCtx,
																pWcTx,
																uiAliasGid,
																statusFlags_x_xr_xu,
																pStringIssue,
																pStringResolve)  );

fail:
	return;
}

void sg_wc_tx__journal__delete_issue(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 SG_uint64 uiAliasGid,
									 const char * pszGid,
									 const char * pszRepoPathTempDir)
{
	SG_vhash * pvhRow = NULL;	// we don't own this

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "sg_wc_tx__journal__delete_issue: [gid %s] '%s'\n",
							   pszGid, ((pszRepoPathTempDir) ? pszRepoPathTempDir : "(null)"))  );
#endif

	// [1] Add log entry to the Journal

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",               "delete_issue")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "gid",              pszGid)  );
	if (pszRepoPathTempDir)		// only present for file-edit conflicts
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "repopath_tempdir", pszRepoPathTempDir)  );

	// [2] Add statement to SQL queue to update the row in tbl_issues.

	SG_ERR_CHECK(  sg_wc_tx__journal__append__delete_issue_stmt(pCtx, pWcTx, uiAliasGid)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__kill_pc_row(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									SG_uint64 uiAliasGid)
{
	SG_vhash * pvhRow = NULL;	// we don't own this
	char * pszGid = NULL;
	SG_string * pStringGidRepoPath = NULL;
	const char * pszKey = "kill_pc_row";

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, uiAliasGid, &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pStringGidRepoPath)  );
	

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__journal__kill_pc_row: '%s'\n"),
							   SG_string__sz(pStringGidRepoPath))  );
#endif

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pWcTx->pvaJournal, &pvhRow)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "op",  pszKey)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhRow, "src", SG_string__sz(pStringGidRepoPath))  );

	SG_ERR_CHECK(  sg_wc_tx__journal__append__delete_pc_stmt(pCtx, pWcTx, uiAliasGid)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
	SG_NULLFREE(pCtx, pszGid);
}
