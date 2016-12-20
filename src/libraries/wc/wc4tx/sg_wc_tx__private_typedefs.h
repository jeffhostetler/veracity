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
 * @flie sg_wc_tx__private_typedefs.h
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
 *     So we potentially need 2 versions of each repo-path
 *     (and potentially the contents of the directory):
 *     [a] PRESCAN: the FIXED repo-path and directory content
 *         as of the beginning of the transaction;
 *     [b] LIVEVIEW: the EVOLVING repo-path and directory
 *         content that reflects the queued changes.
 *
 * We use the "temp_refpc" table to compute [a] and the
 * "tbl_pc" table to compute [b] paths.
 *
 * Also, because READDIR and SCAN_AND_MATCH are somewhat
 * expensive, we maintain a cache of the directories that
 * we have actually visited.  On the PRESCAN side, these
 * are constant anyway.
 * 
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__PRIVATE_TYPEDEFS_H
#define H_SG_WC_TX__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct _sg_wc_tx__lock_info_for_status
{
	char *			psz_lock_branch_name;
	SG_vhash *		pvh_locks_in_branch;
	char *			psz_userid_for_lock_reporting_in_status;
};

struct _sg_wc_tx
{
	sg_wc_db *				pDb;
	sg_wc_db__cset_row *	pCSetRow_Baseline;		// of baseline L0
	sg_wc_db__cset_row *	pCSetRow_Other;			// only set if we have a pending merge

	// We maintain a cache of the disk scans.
	// These are read-only views of the individual
	// directories within the working directory as
	// we saw them at the BEGINNING of the transaction.
	SG_rbtree_ui64 *		prb64PrescanDirCache;		// map[<alias-gid-dir> ==> <sg_wc_prescan_dir *>] we own these
	SG_rbtree_ui64 *		prb64PrescanRowCache;		// map[<alias-gid>     ==> <sg_wc_prescan_row *>] we own these

	SG_rbtree_ui64 *		prb64LiveViewDirCache;	// map[<alias-gid-dir> ==> sg_wc_liveview_dir  *>] we own these
	SG_rbtree_ui64 *		prb64LiveViewItemCache;	// map[<alias-gid-dir> ==> sg_wc_liveview_item *>] we own these


	SG_uint64				uiAliasGid_Root;		// alias of "@/"
	sg_wc_prescan_row *		pPrescanRow_Root;		// we DO NOT own this
	sg_wc_liveview_item *	pLiveViewItem_Root;		// we DO NOT own this

	// With the addition of LOCK information to STATUS,
	// we cache a copy of the current lock data in the
	// and the user-id in the TX.  We defer actually
	// loading them until the first call into STATUS.
	struct _sg_wc_tx__lock_info_for_status * pLockInfoForStatus;

	// We maintain a journal of steps in the
	// transaction that we are to perform.
	// There are 2 parts to this:
	// [1] Actually doing each of the changes to the live working directory.
	//     This is a varray/vhash of the details.  This contains enough info
	//     for --test/--verbose output to the user *and* for us to do the work.
	// [2] Actually updating the database to officially pend the change.
	//     For this we, build a list of the necessary SQLITE3 statements
	//     as we identify them during the QUEUE phase.  We can then rip
	//     thru them in order during the APPLY phase.
	SG_varray *				pvaJournal;
	SG_vector *				pvecJournalStmts;		// vec[sqlite3_stmt *] we own these

	// A Read-Only Transaction can be used for
	// things like STATUS. This lets the caller
	// do the TX setup once and do multiple
	// queries while realizing the savings from
	// the caches we maintain.
	//
	// It also means that all STATUS-type verbs
	// are built upon a TX (rather than a private
	// DB handle) so that these routines can
	// report on the live/in-progress status
	// during a writable TX).
	SG_bool					bReadOnly;

	// Did we create the WD and DB during this TX?
	// We might use this during a rollback/abort
	// to delete stuff rather than leaving things
	// half-baked after a checkout or init.
	SG_bool					bWeCreated_WD;
	SG_bool					bWeCreated_WD_Contents;
	SG_bool					bWeCreated_Drawer;
	SG_bool					bWeCreated_DB;

	//////////////////////////////////////////////////////////////////
	// The following fields are used to keep track
	// of any RESOLVE commands issued during this TX.

	SG_resolve *			pResolve;

	//////////////////////////////////////////////////////////////////
	// The following fields are ***ONLY*** used during a COMMIT.

	char *					pszHid_superroot_content_commit;
	SG_treenode *			pTN_superroot_commit;
	SG_committing *			pCommittingInProgress;

	//////////////////////////////////////////////////////////////////
	// The following fields are used for a per-TX temp directory
	// (needed by things like DIFF).  This is independent (at least
	// for now) of the temp-dir that MERGE creates.

	SG_pathname *			pPathSessionTempDir;

};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__PRIVATE_TYPEDEFS_H
