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

struct _dispatch
{
	const char * pszOp;
	void (*pfn)(SG_context * pCtx,
				SG_wc_tx * pWcTx,
				const SG_vhash * pvh);
};

static struct _dispatch gad[] =
{
	{ "move_rename",             sg_wc_tx__apply__move_rename },
	{ "add",                     sg_wc_tx__apply__add },

	// we distinguish between "remove_" and "undo_add_" in
	// the journal to be informative when printing it for
	// the user, but under the hood we do the same thing.

	{ "remove_directory",        sg_wc_tx__apply__remove_directory },
	{ "remove_file",             sg_wc_tx__apply__remove_file },
	{ "remove_symlink",          sg_wc_tx__apply__remove_symlink },

	{ "undo_add_directory",      sg_wc_tx__apply__remove_directory },
	{ "undo_add_file",           sg_wc_tx__apply__remove_file },
	{ "undo_add_symlink",        sg_wc_tx__apply__remove_symlink },

	// cancel the pended delete of an item.

	{ "undo_delete_directory",   sg_wc_tx__apply__undo_delete__directory },
	{ "undo_delete_file",        sg_wc_tx__apply__undo_delete__file },
	{ "undo_delete_symlink",     sg_wc_tx__apply__undo_delete__symlink },

	// restore a lost item.

	{ "undo_lost_directory",     sg_wc_tx__apply__undo_lost__directory },
	{ "undo_lost_file",          sg_wc_tx__apply__undo_lost__file },
	{ "undo_lost_symlink",       sg_wc_tx__apply__undo_lost__symlink },

	// store a blob for the item.
	// these are only used during COMMIT.

	{ "store_directory",         sg_wc_tx__apply__store_directory },
	{ "store_file",              sg_wc_tx__apply__store_file },
	{ "store_symlink",           sg_wc_tx__apply__store_symlink },

	// alter the tne_L0 and/or tbl_PC tables.
	// these are only used during COMMIT.

	{ "delete_tne_row",          sg_wc_tx__apply__delete_tne },
	{ "insert_tne_row",          sg_wc_tx__apply__insert_tne },
	{ "delete_pc_row",           sg_wc_tx__apply__delete_pc },
	{ "clean_pc_row_but_leave_sparse", sg_wc_tx__apply__clean_pc_but_leave_sparse },

	// alter the WD as directed by MERGE.

	{ "special_add_file",           sg_wc_tx__apply__add_special__file },
	{ "special_add_directory",      sg_wc_tx__apply__add_special__directory },
	{ "special_add_symlink",        sg_wc_tx__apply__add_special__symlink },
	// TODO special_add_subrepo

	{ "overwrite_file_from_file",   sg_wc_tx__apply__overwrite_file_from_file },
	{ "overwrite_file_from_repo",   sg_wc_tx__apply__overwrite_file_from_repo },
	{ "overwrite_symlink",          sg_wc_tx__apply__overwrite_symlink },

	{ "set_attrbits",               sg_wc_tx__apply__set_attrbits },

	{ "resolve_issue__s",           sg_wc_tx__apply__resolve_issue__s },
	{ "resolve_issue__sr",          sg_wc_tx__apply__resolve_issue__sr },
	{ "insert_issue",               sg_wc_tx__apply__insert_issue },
	{ "delete_issue",               sg_wc_tx__apply__delete_issue },

	// bookkeeping for revert-all delete of add-special item
	{ "kill_pc_row",                sg_wc_tx__apply__kill_pc_row },

};
	
static SG_varray_foreach_callback _apply_wd_cb;

static void _apply_wd_cb(SG_context * pCtx,
						 void * pVoid_Data,
						 const SG_varray * pva,
						 SG_uint32 ndx,
						 const SG_variant * pVariant)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoid_Data;
	SG_vhash * pvh;
	const char * pszOp;
	SG_uint32 k;

	SG_UNUSED( pva );
	SG_UNUSED( ndx );

	SG_ERR_CHECK_RETURN(  SG_variant__get__vhash(pCtx, pVariant, &pvh)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh, "op", &pszOp)  );

	for (k=0; k<SG_NrElements(gad); k++)
	{
		if (strcmp(gad[k].pszOp, pszOp) == 0)
		{
			SG_ERR_CHECK_RETURN(  (*gad[k].pfn)(pCtx, pWcTx, pvh)  );
			return;
		}
	}

	SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
						   (pCtx, "_apply_wd_cb: op '%s'", pszOp)  );
}

//////////////////////////////////////////////////////////////////

static SG_vector_foreach_callback _apply_db_cb;

static void _apply_db_cb(SG_context * pCtx,
						 SG_uint32 ndx,
						 void * pVoid_Assoc,
						 void * pVoid_Data)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoid_Data;
	sqlite3_stmt * pStmt = (sqlite3_stmt *)pVoid_Assoc;

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__apply: db step %d: %s\n",
							   ndx,
							   sqlite3_sql(pStmt))  );
#endif

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

	// Steal the prepared statement from the vector so that
	// we can finalize it now rather than waiting until the
	// vector is destroyed.  This might help some sqlite
	// table locking where some operations can fail if there
	// is an un-finalized select/insert/... statement hanging
	// around.

	SG_ERR_CHECK(  SG_vector__set(pCtx, pWcTx->pvecJournalStmts, ndx, NULL)  );
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * APPLY each command QUEUED in the TX JOURNAL.
 *
 * We DO NOT deal with the TX commit.
 *
 */
void sg_wc_tx__run_apply(SG_context * pCtx, SG_wc_tx * pWcTx)
{
#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__run_apply: start....\n"))  );
#endif

	SG_ERR_CHECK(  sg_wc_db__tx__assert(pCtx, pWcTx->pDb)  );

	if (pWcTx->bReadOnly)
		SG_ERR_THROW( SG_ERR_WC_CANNOT_APPLY_CHANGES_IN_READONLY_TX );

	// We have a (potential) problem here: we have to do 2 unrelated
	// things atomically.
	// 
	// [1] we need to modify records in the database to reflect the
	//     effects of the user's command.
	// [2] we need to modify the contents of the working directory
	//     on disk to actually do what they requested.
	//
	// BUT there's nothing stopping the user from concurrently
	// (in another window):
	// 
	// [a] directly altering the WD.
	// [b] altering both the WD and the database with another 'vv' command.
	// [c] doing a read-only operation (like 'vv status') on the WD and
	//     database.
	//
	// There's nothing we can do to stop [a].
	// We can guard against [b,c] using the existing SQL locks -- provided
	// we take sqlite's EXCLUSIVE lock before we do [1] or [2].

	SG_ERR_CHECK(  SG_vector__foreach(pCtx, pWcTx->pvecJournalStmts, _apply_db_cb, (void *)pWcTx)  );

	// re-arrange the working directory and/or store
	// blobs in the repo according to the plan.

	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pWcTx->pvaJournal, _apply_wd_cb, (void *)pWcTx)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__run_apply: finished....\n"))  );
#endif

fail:
	return;
}

					 
