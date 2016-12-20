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
 * @file sg_wc_tx__begin.c
 *
 * @details A high-level transaction handle for manipulating
 * the working directory and the sqlite database in tandem.
 * (Not to be confused with the low-level sql tx stuff in
 * sg_wc_db__tx.c)
 *
 * A high-level TX allows the caller to request a sequence of
 * vv-level operations (for example, 'vv move a b c d dir' to
 * MOVE 4 items into a target directory -or- something like
 * an UPDATE -or- a PARTIAL-REVERT -or- a JS script).
 *
 * There are 2 problems we address:
 * [1] We'd like to be able to validate all of the arguments
 *     *before* we start munging the actual working directory
 *     on disk.  That is, we'd like to know that the 4th item
 *     isn't found before we start moving the first 3 and
 *     minimize the places where we can have an inconsistent
 *     view.  If done right, we can minimize the opportunities
 *     for getting a hard error from the filesystem and therefore
 *     opportunities to need to do any filesystem rollbacks.
 *
 *     By splitting a sequence of commands into:
 *     [a] queue/request all
 *     [b] apply them.
 *     we get a journal/plan of what we're doing and get
 "     the --test" and/or "--log" stuff almost for free.
 *
 * [2] No matter how we do it, we end up with an ambiguity
 *     in repo-paths.  If I do 4 moves/renames/whatever and
 *     they are all expressed in absolute terms, the user or
 *     owner of the transaction would be expecting the changes
 *     made in earlier steps to be reflected in later steps
 *     (ie. the rename of a parent directory) *as if* we had
 *     actually made the changes at each step.  *BUT* we need
 *     to inspect the current contents of the working directory
 *     but we haven't made the earlier changes yet.
 *
 *     So we potentially need 2 versions of each repo-path:
 *     [a] a fixed repo-path as of the beginning of the transaction;
 *     [b] an evolving repo-path that reflects the queued changes.
 *
 * During the QUEUE phase, the tbl_pc is not changed.  The tbl_pc
 * table serves as a reference for the state of the working directory
 * as of the start of the TX.  We sync the tne_L0, tbl_pc, and
 * scandir/readdir info to create the 'prescan' data as an in-memory
 * view.  The 'liveview' is an in-memory clone of the 'prescan' data
 * that can evolve as we queue/journal changes.  (Both the 'prescan'
 * and 'liveview' views are cached in memory.)  Only during the APPLY
 * phase do we use the 'liveview' and journal to alter the working
 * directory and the tbl_pc table.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc_tx__apply(SG_context * pCtx, SG_wc_tx * pWcTx)
{
	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK(  sg_wc_tx__run_apply(pCtx, pWcTx)  );

	SG_ERR_CHECK(  sg_wc_db__tx__commit(pCtx, pWcTx->pDb)  );

fail:
	return;
}

void SG_wc_tx__cancel(SG_context * pCtx, SG_wc_tx * pWcTx)
{
	SG_bool bInTx;

	if (!pWcTx)		// so we don't have to guard all calls in fail blocks.
		return;

	if (!pWcTx->pDb)	// so we don't have to guard all calls in fail blocks.
		return;

	sg_wc_db__tx__in_tx(pCtx, pWcTx->pDb, &bInTx);
	if (!bInTx)		// so we don't have to guard all calls in fail blocks.
		return;
	
	SG_ERR_CHECK(  sg_wc_db__tx__rollback(pCtx, pWcTx->pDb)  );

	// If MERGE or UPDATE generated any automerge temp files, we
	// should delete them.  We IGNORE the result because we don't
	// want a FS error to cause us to abort the rest of our abort.
	SG_ERR_IGNORE(  sg_wc_tx__merge__delete_automerge_tempfiles_on_abort(pCtx, pWcTx)  );

fail:
	return;
}

/**
 * Abort the effects of a __begin__create().
 *
 * That is, we will *try* to DELETE the DB, the DRAWER, and WD
 * if they were created by __begin__create().
 *
 * This is like a __cancel() on steroids.
 *
 * It will free the given pWcTx.
 *
 */ 
void SG_wc_tx__abort_create_and_free(SG_context * pCtx, SG_wc_tx ** ppWcTx)
{
	SG_wc_tx * pWcTx;
	SG_pathname * pPathWorkingDirectoryTop = NULL;
	SG_pathname * pPathDrawer = NULL;
	SG_bool bWeCreated_WD = SG_FALSE;
	SG_bool bWeCreated_WD_Contents = SG_FALSE;

	if (!ppWcTx)
		return;

	pWcTx = *ppWcTx;
	*ppWcTx = NULL;

	if (!pWcTx)
		return;

#if TRACE_WC_ABORT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "AbortCreate: [WD %d][WDc %d][Drawer %d][DB %d] %s\n",
							   pWcTx->bWeCreated_WD,
							   pWcTx->bWeCreated_WD_Contents,
							   pWcTx->bWeCreated_Drawer,
							   pWcTx->bWeCreated_DB,
							   SG_pathname__sz(pWcTx->pDb->pPathWorkingDirectoryTop))  );
#endif

	if (pWcTx->bWeCreated_WD || pWcTx->bWeCreated_WD_Contents)
	{
		SG_ERR_IGNORE(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPathWorkingDirectoryTop, pWcTx->pDb->pPathWorkingDirectoryTop)  );
		bWeCreated_WD = pWcTx->bWeCreated_WD;
		bWeCreated_WD_Contents = pWcTx->bWeCreated_WD_Contents;
	}
	if (pWcTx->bWeCreated_Drawer)
		SG_ERR_IGNORE(  SG_workingdir__get_drawer_path( pCtx, pWcTx->pDb->pPathWorkingDirectoryTop, &pPathDrawer)  );

	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_ERR_IGNORE(  sg_wc_db__close_db(pCtx, pWcTx->pDb, pWcTx->bWeCreated_DB)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);

	if (pPathDrawer)
	{
		SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathDrawer)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathDrawer);
	}
	if (pPathWorkingDirectoryTop)
	{
		if (bWeCreated_WD)
			SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPathWorkingDirectoryTop, SG_TRUE)  );
		else if (bWeCreated_WD_Contents)
			SG_ERR_IGNORE(  SG_fsobj__rmdir_contents_recursive(pCtx, pPathWorkingDirectoryTop, SG_TRUE)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDirectoryTop);
	}
}

//////////////////////////////////////////////////////////////////

static void _my_sqlite__finalize(SG_context * pCtx, sqlite3_stmt * pStmt)
{
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__free(SG_context * pCtx, SG_wc_tx * pWcTx)
{
	if (!pWcTx)
		return;

#if 0 && defined(DEBUG)
	if (pWcTx->pDb)
	{
		// because __free routines are usually called inside an SG_ERR_IGNORE()
		// we normally will never see any errors that this routine would throw.
		// so this is a little sanity check to make sure that the TX was either
		// committed or rolled back.

		SG_bool bInTx;

		sg_wc_db__tx__in_tx(pCtx, pWcTx->pDb, &bInTx);
		if (bInTx)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "SG_wc_tx__free: still in open TX.\n")  );
	}
#endif

	if (pWcTx->pCommittingInProgress)
	{
		SG_ERR_IGNORE(  SG_committing__abort(pCtx, pWcTx->pCommittingInProgress)  );
		pWcTx->pCommittingInProgress = NULL;
	}

	SG_VARRAY_NULLFREE(pCtx, pWcTx->pvaJournal);
	// The list of JournalStmts must be freed before we close/free the DB.
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pWcTx->pvecJournalStmts, (SG_free_callback *)_my_sqlite__finalize);

	if (pWcTx->pDb)
	{
		SG_ERR_IGNORE(  sg_wc_db__close_db(pCtx, pWcTx->pDb, SG_FALSE)  );
		SG_WC_DB_NULLFREE(pCtx, pWcTx->pDb);
	}

	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pWcTx->pCSetRow_Baseline);
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pWcTx->pCSetRow_Other);

	SG_RBTREE_UI64_NULLFREE_WITH_ASSOC(pCtx, pWcTx->prb64PrescanDirCache, (SG_free_callback *)sg_wc_prescan_dir__free);
	SG_RBTREE_UI64_NULLFREE_WITH_ASSOC(pCtx, pWcTx->prb64PrescanRowCache, (SG_free_callback *)sg_wc_prescan_row__free);
	SG_RBTREE_UI64_NULLFREE_WITH_ASSOC(pCtx, pWcTx->prb64LiveViewDirCache, (SG_free_callback *)sg_wc_liveview_dir__free);
	SG_RBTREE_UI64_NULLFREE_WITH_ASSOC(pCtx, pWcTx->prb64LiveViewItemCache, (SG_free_callback *)sg_wc_liveview_item__free);

	if (pWcTx->pLockInfoForStatus)
	{
		SG_NULLFREE(      pCtx, pWcTx->pLockInfoForStatus->psz_lock_branch_name);
		SG_VHASH_NULLFREE(pCtx, pWcTx->pLockInfoForStatus->pvh_locks_in_branch);
		SG_NULLFREE(      pCtx, pWcTx->pLockInfoForStatus->psz_userid_for_lock_reporting_in_status);
		SG_NULLFREE(      pCtx, pWcTx->pLockInfoForStatus);
	}

	SG_RESOLVE_NULLFREE(pCtx, pWcTx->pResolve);

	SG_NULLFREE(pCtx, pWcTx->pszHid_superroot_content_commit);
	SG_TREENODE_NULLFREE(pCtx, pWcTx->pTN_superroot_commit);

	if (pWcTx->pPathSessionTempDir)
	{
		// we may or may not be able to delete the tmp dir (they may be visiting it in another window)
		// so we have to allow this to fail and not mess up the real context.

		SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pWcTx->pPathSessionTempDir)  );
		SG_PATHNAME_NULLFREE(pCtx, pWcTx->pPathSessionTempDir);
	}

	SG_NULLFREE(pCtx, pWcTx);
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__alloc__begin(SG_context * pCtx,
							SG_wc_tx ** ppWcTx,
							const SG_pathname * pPathGiven,
							SG_bool bReadOnly)
{
	const SG_pathname * pPath;
	SG_pathname * pPathCwd = NULL;
	SG_pathname * pPathWorkingDirectoryTop = NULL;
	SG_repo * pRepo = NULL;
	SG_string * pStringRepoDescriptorName = NULL;
	char * pszGidAnchorDirectory = NULL;
	SG_wc_tx * pWcTx = NULL;
	SG_bool bHaveL1;

	SG_NULLARGCHECK_RETURN( ppWcTx );
	// pPathGiven is optional

	if (pPathGiven)
	{
		pPath = pPathGiven;
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathCwd)  );
		SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathCwd)  );
		pPath = pPathCwd;
	}
	
	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath,
											   &pPathWorkingDirectoryTop,
											   &pStringRepoDescriptorName,
											   &pszGidAnchorDirectory)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx,
											   SG_string__sz(pStringRepoDescriptorName),
											   &pRepo)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pWcTx)  );
	pWcTx->bReadOnly = bReadOnly;
	pWcTx->bWeCreated_WD          = SG_FALSE;
	pWcTx->bWeCreated_WD_Contents = SG_FALSE;
	pWcTx->bWeCreated_Drawer      = SG_FALSE;
	pWcTx->bWeCreated_DB          = SG_FALSE;

	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64PrescanDirCache)  );
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64PrescanRowCache)  );

	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64LiveViewDirCache)  );
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64LiveViewItemCache)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pWcTx->pvaJournal)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pWcTx->pvecJournalStmts, 100)  );
	
	SG_ERR_CHECK(  sg_wc_db__open_db(pCtx, pRepo, pPathWorkingDirectoryTop, (pPathGiven == NULL), &pWcTx->pDb)  );
	SG_ERR_CHECK(  sg_wc_db__tx__begin(pCtx, pWcTx->pDb, SG_TRUE)  );
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pWcTx->pDb, "L0",
													 NULL,	// let it throw if row L0 not present.
													 &pWcTx->pCSetRow_Baseline)  );

	SG_ERR_CHECK(  sg_wc_db__tne__get_alias_of_root(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
													&pWcTx->uiAliasGid_Root)  );

	// See if we have a pending merge in the WD.
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pWcTx->pDb, "L1",
													 &bHaveL1,
													 &pWcTx->pCSetRow_Other)  );

	// Defer creating session temp dir until needed.
	pWcTx->pPathSessionTempDir = NULL;

	// Defer creating/loading lock data for status until needed.

	*ppWcTx = pWcTx;
	pWcTx = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDirectoryTop);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRING_NULLFREE(pCtx, pStringRepoDescriptorName);
	SG_NULLFREE(pCtx, pszGidAnchorDirectory);
	SG_WC_TX__NULLFREE(pCtx, pWcTx);	// use normal destructor since we didn't create anything
}

//////////////////////////////////////////////////////////////////

/**
 * Formerly SG_vv_utils__lookup_wd_info().
 * 
 * I needed to move this to here and make it static because
 * we are in a fragile state because we don't have a pWcTx
 * handle yet and we're not yet sure whether we can create
 * one because we don't yet know anything about the CWD
 * and whether it *is* a WD or is *inside* a WD.
 *
 * Our caller is trying to decide if it can create a new WD
 * at the given path.
 *
 */
static void _lookup_wd_info(SG_context * pCtx,
							const SG_pathname * pPathAnywhereWithinWD,
							SG_bool * pbInWD,
							SG_bool * pbUnderVC,
							SG_pathname ** ppPathRoot,
							SG_string ** ppStringRepoName,
							char ** ppszGidEntry)
{
	SG_wc_tx * pWcTx_temp = NULL;
	SG_pathname * pPathRoot = NULL;
	SG_string * pStringRepoName = NULL;
	char * pszGidEntry = NULL;

	// TODO 2010/10/25 Figure out what ANCHOR is and if we need to do anything with it.

	SG_NULLARGCHECK_RETURN( pPathAnywhereWithinWD );
	SG_NULLARGCHECK_RETURN( pbInWD );
	SG_NULLARGCHECK_RETURN( pbUnderVC );

	// TODO 2010/10/25 We really should fix SG_workingdir__find_mapping to throw _NOT_A_WORKING_COPY
	// TODO            rather than just _NOT_FOUND.

	SG_workingdir__find_mapping(pCtx, pPathAnywhereWithinWD, &pPathRoot, &pStringRepoName, NULL);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (!SG_context__err_equals(pCtx, SG_ERR_NOT_A_WORKING_COPY))
			SG_ERR_RETHROW;

		SG_context__err_reset(pCtx);

		*pbInWD = SG_FALSE;
		*pbUnderVC = SG_FALSE;
	}
	else
	{
		// the given pathname is somewhere within a Working Directory.

		*pbInWD = SG_TRUE;

		// see if the entry is under version control (rather than
		// being a found or ignored item).
		//
		// we carefully try to bootstrap a pWcTx since we know something now.
		// we ***ASSUME*** that we are not already in SG_wc_tx__alloc__begin().
		// create a temporary TX and try to fetch this item's GID.  We do this
		// the long way because the macros and layer-8 assume CWD and we want
		// to base it from the root we just computed.

		SG_ERR_CHECK(  SG_wc_tx__alloc__begin(pCtx, &pWcTx_temp, pPathRoot, SG_TRUE)  );
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx_temp, SG_pathname__sz(pPathAnywhereWithinWD), &pszGidEntry)  );

		*pbUnderVC = (pszGidEntry[0] == 'g');	// uncontrolled items have 't' prefix.
	}

	if (ppPathRoot)
	{
		*ppPathRoot = pPathRoot;
		pPathRoot = NULL;
	}
	
	if (ppStringRepoName)
	{
		*ppStringRepoName = pStringRepoName;
		pStringRepoName = NULL;
	}

	if (ppszGidEntry)
	{
		*ppszGidEntry = pszGidEntry;
		pszGidEntry = NULL;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathRoot);
	SG_STRING_NULLFREE(pCtx, pStringRepoName);
	SG_NULLFREE(pCtx, pszGidEntry);
	SG_WC_TX__NULLFREE(pCtx, pWcTx_temp);
}

//////////////////////////////////////////////////////////////////

/**
 * Convert the given (absolute, relative, or empty) path
 * to an actual pathname and see if it is already
 * contained within an outer WD and/or is already under
 * version control in an outer WD.  We do not immediately
 * stat() it because it may not actually exist on disk yet.
 *
 * You own the returned pathname.
 * 
 */
static void _validate_path(SG_context * pCtx,
						   const char * pszPath,
						   SG_bool bRequireNewDirOrEmpty,
						   SG_pathname ** ppPathAllocated)
{
	SG_pathname * pPathAllocated = NULL;
	SG_pathname * pPathOuterWD = NULL;
	SG_string * pStringOuterRepoName = NULL;
	char * pszGidEntry = NULL;
	SG_bool bAlreadyInWD = SG_FALSE;
	SG_bool bUnderVC = SG_FALSE;

	SG_ERR_CHECK(  SG_vv_utils__folder_arg_to_path(pCtx, pszPath, &pPathAllocated)  );
	SG_ERR_CHECK(  _lookup_wd_info(pCtx, pPathAllocated,
								   &bAlreadyInWD,
								   &bUnderVC,
								   &pPathOuterWD,
								   &pStringOuterRepoName,
								   &pszGidEntry)  );
	if (bUnderVC)
	{
		// the pathname is already under version control in
		// an outer WD.  if we were to create a WD/submodule
		// here, we would be stealing the directory from the
		// outer WD.  the outer would see this (and everything
		// within it) as LOST.

		SG_ERR_THROW2(  SG_ERR_ENTRY_ALREADY_UNDER_VERSION_CONTROL,
						(pCtx, "The path '%s' is already under version control in the working copy rooted at '%s'.",
						 SG_pathname__sz(pPathAllocated),
						 SG_pathname__sz(pPathOuterWD))  );
	}

	// CHECKOUT wants to create a *NEW* directory or populate an *EMPTY*
	// directory so that we don't accidentally trash their work.
	// This is different from an INIT where we want to be able to let
	// ADDREMOVE adopt the contents of the directory.

	if (bRequireNewDirOrEmpty)
	{
		SG_ERR_CHECK(  SG_vv_utils__verify_dir_empty(pCtx, pPathAllocated)  );
	}
	else
	{
		SG_fsobj_type fsType = SG_FSOBJ_TYPE__UNSPECIFIED;
		SG_bool bExists = SG_FALSE;

		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathAllocated, &bExists, &fsType, NULL)  );
		if (bExists && (fsType != SG_FSOBJ_TYPE__DIRECTORY))
		{
			SG_ERR_THROW2(  SG_ERR_NOT_A_DIRECTORY,
							(pCtx, "Cannot create a WD here; the path '%s' already exists and is not a directory.",
							 SG_pathname__sz(pPathAllocated))  );
		}
	}

	// If the given path is logically inside another WD,
	// we don't want to allow it.

	if (bAlreadyInWD)
		SG_ERR_THROW2(  SG_ERR_WORKING_DIRECTORY_IN_USE,
						(pCtx, "The directory '%s' is already part of the working copy rooted at '%s'.",
						 SG_pathname__sz(pPathAllocated),
						 SG_pathname__sz(pPathOuterWD))  );

	*ppPathAllocated = pPathAllocated;
	pPathAllocated = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathAllocated);
	SG_PATHNAME_NULLFREE(pCtx, pPathOuterWD);
	SG_STRING_NULLFREE(pCtx, pStringOuterRepoName);
	SG_NULLFREE(pCtx, pszGidEntry);
}

/**
 * Begin a TX where we are CREATING (rather than just OPENING)
 * the drawer/db (used by CHECKOUT and INIT).
 *
 */
void SG_wc_tx__alloc__begin__create(SG_context * pCtx,
									SG_wc_tx ** ppWcTx,
									const char * pszRepoName,
									const char * pszPath,
									SG_bool bRequireNewDirOrEmpty,
									const char * pszHidCSet)
{
	SG_pathname * pPathWorkingDirectoryTop = NULL;
	SG_repo * pRepo = NULL;
	SG_changeset * pcs = NULL;
	SG_wc_tx * pWcTx = NULL;
	SG_fsobj_type fsType = SG_FSOBJ_TYPE__UNSPECIFIED;
	SG_bool bExists = SG_FALSE;
	SG_bool bWeCreated_Drawer = SG_FALSE;

	SG_NULLARGCHECK_RETURN( ppWcTx );
	SG_NONEMPTYCHECK_RETURN( pszRepoName );
	// pszPath is optional
	SG_NONEMPTYCHECK_RETURN( pszHidCSet );

	SG_ERR_CHECK(  _validate_path(pCtx, pszPath, bRequireNewDirOrEmpty, &pPathWorkingDirectoryTop)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );

	// before we actually create anything on disk, make sure
	// that we have a valid changeset HID.
	SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, pszHidCSet, &pcs)  );
	SG_CHANGESET_NULLFREE(pCtx, pcs);

	// We now believe that we have enough info to at least
	// populate the working directory.
	//
	// Create the root directory.  But we defer setting ATTRBITS
	//     on the root directory until later when
	//     we start populating it.
	// Create the drawer inside it.
	// Set the drawer version.
	// Create the repo.json repo-descriptor.
	// Set branch.

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathWorkingDirectoryTop, &bExists, &fsType, NULL)  );
	if (!bExists)
		SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPathWorkingDirectoryTop)  );
	SG_ERR_CHECK(  SG_workingdir__verify_drawer_exists(pCtx, pPathWorkingDirectoryTop,
													   NULL, &bWeCreated_Drawer)  );
	SG_ERR_CHECK(  SG_workingdir__set_mapping(pCtx, pPathWorkingDirectoryTop, pszRepoName, NULL)  );

	// Create the SQL DB and start a TX.

	SG_ERR_CHECK(  SG_alloc1(pCtx, pWcTx)  );
	pWcTx->bReadOnly = SG_FALSE;
	pWcTx->bWeCreated_WD = (!bExists);		// remember if we had to create it in case we need to throw up later.
	pWcTx->bWeCreated_WD_Contents = ((!bExists) || bRequireNewDirOrEmpty);
	pWcTx->bWeCreated_Drawer = bWeCreated_Drawer;

	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64PrescanDirCache)  );
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64PrescanRowCache)  );

	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64LiveViewDirCache)  );
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pWcTx->prb64LiveViewItemCache)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pWcTx->pvaJournal)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pWcTx->pvecJournalStmts, 100)  );
	
	// Specifying SG_FALSE for bWorkingDirPathFromCwd here is kind of a hack. This flag controls
	// whether callers can specify relative paths for operations in this working copy transaction. 
	// SG_FALSE is safe and arguably correct, because we ALWAYS specify a path when creating a working 
	// copy. However it's valid to specify e.g. "." for a new WC path, which would make any relative 
	// paths specified later perfectly valid. If we ever have callers who expect this to work, we can 
	// change this to check for those cases. Today, this is valid and safe.
	SG_ERR_CHECK(  sg_wc_db__create_db(pCtx, pRepo, pPathWorkingDirectoryTop, SG_FALSE, &pWcTx->pDb)  );
	pWcTx->bWeCreated_DB = SG_TRUE;

	SG_ERR_CHECK(  sg_wc_db__tx__assert(pCtx, pWcTx->pDb)  );

	// Load the complete contents of requested baseline into
	// the tne_L0 table in SQL and hook up the various pointers
	// in pWcTx.
	// 
	// We let caller deal with actually populating the WD
	// and/or the TSC.

	SG_ERR_CHECK(  sg_wc_db__load_named_cset(pCtx, pWcTx->pDb, "L0", pszHidCSet,
											 SG_TRUE, SG_TRUE, SG_TRUE)  );
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pWcTx->pDb, "L0",
													 NULL,		// let it throw if no "L0" row.
													 &pWcTx->pCSetRow_Baseline)  );
	SG_ERR_CHECK(  sg_wc_db__tne__get_alias_of_root(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
													&pWcTx->uiAliasGid_Root)  );

	pWcTx->pCSetRow_Other = NULL;		// a fresh checkout cannot have a pending merge.

	// TODO 2011/10/17 Should we write a note to the journal about
	// TODO            'checked out baseline...' ?
	// TODO
	// TODO            Note that the 'pvecJournalStmts' is not used
	// TODO            when we load the tne_L0 table; we just do it
	// TODO            immediately. (It's still part of the overall TX,
	// TODO            but we don't use the QUEUE/APPLY model for it.
	// TODO            (Because it was written before the Q/A model was
	// TODO            written *and* because if we get an error or want
	// TODO            the do the '--test' thing, we can just delete the
	// TODO            whole DB.))

	*ppWcTx = pWcTx;

	SG_CHANGESET_NULLFREE(pCtx, pcs);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDirectoryTop);
	SG_REPO_NULLFREE(pCtx, pRepo);
	return;

fail:
	SG_CHANGESET_NULLFREE(pCtx, pcs);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDirectoryTop);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_ERR_IGNORE(  SG_wc_tx__abort_create_and_free(pCtx, &pWcTx)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Return COPIES of the REPO and/or WD TOP.
 * 
 * These will be cleaned up versions of the
 * info given to __alloc__begin() or
 * __alloc__begin__create().
 *
 * You own both of the returned values.
 *
 */
void SG_wc_tx__get_repo_and_wd_top(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   SG_repo ** ppRepo,
								   SG_pathname ** ppPathWorkingDirectoryTop)
{
	SG_repo * pRepoCopy = NULL;
	SG_pathname * pPathCopy = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	// ppRepo is optional
	// ppPathWorkingDirectoryTop is optional

	if (ppRepo)
		SG_ERR_CHECK(  SG_repo__open_repo_instance__copy(pCtx, pWcTx->pDb->pRepo, &pRepoCopy)  );
	if (ppPathWorkingDirectoryTop)
		SG_ERR_CHECK(  SG_pathname__alloc__copy(pCtx, &pPathCopy, pWcTx->pDb->pPathWorkingDirectoryTop)  );

	if (ppRepo)
		*ppRepo = pRepoCopy;
	if (ppPathWorkingDirectoryTop)
		*ppPathWorkingDirectoryTop = pPathCopy;
	return;

fail:
	SG_REPO_NULLFREE(pCtx, pRepoCopy);
	SG_PATHNAME_NULLFREE(pCtx, pPathCopy);
}
