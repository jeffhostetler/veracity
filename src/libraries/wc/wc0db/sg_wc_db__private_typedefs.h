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

#ifndef H_SG_WC_DB__PRIVATE_TYPEDEFS_H
#define H_SG_WC_DB__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

enum _sg_wc_db__path__import_flags
{
	SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR   = 0,
	SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ROOT    = 1,
	SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_CWD     = 2,
};

typedef enum _sg_wc_db__path__import_flags sg_wc_db__path__import_flags;

//////////////////////////////////////////////////////////////////

struct _sg_wc_db
{
	SG_pathname * pPathWorkingDirectoryTop;
	SG_pathname * pPathDB;
	
	// If false, the wc_tx initiator specified the path to the working dir.
	// If true, we derived it from the cwd.
	SG_bool bWorkingDirPathFromCwd; 

	// We cache a set of the IGNORES settings.
	// This is only loaded when needed.
	SG_file_spec *			pFilespecIgnores;

	// This contains a set of bit masks for the ATTRBITS
	// that are defined and usable on this platform.
	SG_wc_attrbits_data * pAttrbitsData;

	// The TimeStampCache (TSC) is a table of (gid,hid,mtime,size,...)
	// for FILES in the working directory.  We use it to try to
	// avoid re-computing hashes during STATUS when we can prove
	// we already know the answer.
	//
	// In the original PendingTree (ptnode) implementation, the TSC
	// was stored as an rbtreedb *beside* the wd.json file.  During
	// the PendingTree-2-WC re-write I was planning to bringing the
	// TSC into the WC db as a plain table, but I'm going to leave
	// it independent for the moment.
	//
	// TODO 2011/10/13 See W5659 and W1544.
	SG_timestamp_cache * pTSC;

	SG_repo * pRepo;
	sqlite3 * psql;
	SG_uint32 txCount;
	SG_uint32 nrTmpGids;

	sqlite3_stmt * pSqliteStmt__get_issue;
	sqlite3_stmt * pSqliteStmt__get_gid_from_alias;
	SG_rbtree * prbCachedSqliteStmts__get_row_by_alias;

	SG_wc_port_flags portMask;	// set when we open/being the TX
};

typedef struct _sg_wc_db sg_wc_db;

struct _sg_wc_db__cset_row
{
	char * psz_label;
	char * psz_hid_cset;
	char * psz_tne_table_name;
	char * psz_pc_table_name;
	char * psz_hid_super_root;
};

typedef struct _sg_wc_db__cset_row sg_wc_db__cset_row;

//////////////////////////////////////////////////////////////////

/**
 * I want to divide the properties of a TreeNodeEntry
 * (as stored in a TreeNode within a ChangeSet) into
 * 3 groups:
 * 
 * [1] fixed -- gid, tneType
 * [2] structural -- parent-gid, entryname
 * [3] dynamic -- hid, attrbits,
 *
 * These are stored within the tne_L0 table (as we got
 * them from the TNE).
 *
 * In the working directory, structural properties can
 * be changed using explicit commands, such as ADD, DELETE,
 * MORE, and RENAME.  (No scanning required.)  We model
 * them in the tbl_pc table.
 *
 * In the working directory, dynamic properties
 * are "discovered" by scanning at various times.
 * That is, if the user edits a file we have to be
 * able to detect that there are changes to the content
 * because we don't require them to notify us about
 * edits.
 * 
 */

/**
 * Capture [1] and [2].
 */
struct sg_wc_db__state_structural
{
	SG_uint64 uiAliasGid;
	SG_uint64 uiAliasGidParent;
	char * pszEntryname;
	SG_treenode_entry_type tneType;
};

typedef struct sg_wc_db__state_structural sg_wc_db__state_structural;

/**
 * Capture [3].
 *
 * ATTRBITS are always defined, but not all bits are
 * necessarily defined on all platforms. For example,
 * the 'x' bit on Windows.  We will define a platform
 * mask such that we can always look at the complete
 * set of defined bits, but only allow the current
 * platform to (implicitly via /bin/chmod) alter the
 * bits defined on this platform *and* only look at
 * the defined bits when computing status.
 * 
 */
struct sg_wc_db__state_dynamic
{
	SG_uint64 attrbits;
	char * pszHid;
};

typedef struct sg_wc_db__state_dynamic sg_wc_db__state_dynamic;

/**
 * A TNE_ROW contains [1], [2], and [3].
 */
struct sg_wc_db__tne_row
{
	sg_wc_db__state_structural * p_s;
	sg_wc_db__state_dynamic    * p_d;
};

typedef struct sg_wc_db__tne_row sg_wc_db__tne_row;

//////////////////////////////////////////////////////////////////

/**
 * Prototype typedef for a callback function to receive
 * a tne_row from a SELECT on the tne_L0 table.
 *
 * The callback may steal the tne_row it it wants to.
 */
typedef void (sg_wc_db__tne__foreach_cb)(SG_context * pCtx, void * pVoidData, sg_wc_db__tne_row ** ppTneRow);

//////////////////////////////////////////////////////////////////

/**
 * The "flags_net" flags are associated with a row in the
 * in the tbl_pc table.  This indicates the "net" structural
 * status for the item.  That is, how the item differs from
 * the baseline.
 *
 * More than one bit may be set for an item.
 *
 * TODO 2011/08/30 Pay attention to MERGE_CREATED + DELETED case for
 * TODO            a "user delete after added by merge".
 *
 */
enum _sg_wc_db__pc_row__flags_net
{
	SG_WC_DB__PC_ROW__FLAGS_NET__ZERO			= 0x0000,
	SG_WC_DB__PC_ROW__FLAGS_NET__ADDED			= 0x0001,	// user created a new item
	SG_WC_DB__PC_ROW__FLAGS_NET__DELETED		= 0x0002,
	SG_WC_DB__PC_ROW__FLAGS_NET__RENAMED		= 0x0004,
	SG_WC_DB__PC_ROW__FLAGS_NET__MOVED			= 0x0008,
	SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE			= 0x0010,	// we did not populate it during checkout
	SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_M	= 0x0020,	// MERGE  "creates" items in the WD that were added in the other branch
	SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U	= 0x0040,	// UPDATE "re-creates" items in the WD to override deletes in target

	SG_WC_DB__PC_ROW__FLAGS_NET__FLATTEN		= 0x4000,	// don't report "add-special + removed" (see revert-all)
	SG_WC_DB__PC_ROW__FLAGS_NET__INVALID		= 0x8000,	// placeholder/memory-only value used by UNDO_ADD during TX.
};

typedef enum _sg_wc_db__pc_row__flags_net sg_wc_db__pc_row__flags_net;

//////////////////////////////////////////////////////////////////

/**
 * ["pc" == "pending/pended change"]
 * 
 * A row from the tbl_pc table describes the CURRENT
 * state of the [2] fields for this item.
 *
 * Note that tbl_pc is a list of deltas and will only contain
 * an item if the user has modified it in a structural
 * way.  So a request to fetch one of these from the
 * tbl_pc table *MAY* return a not-found if the item
 * hasn't been moved/renamed/etc -or- it might silently
 * substitute the corresponding data from the TNE.
 * 
 * This structure is a wrapper for the raw row data
 * in the table.
 *
 * This structure DOES NOT contain a repo-path because
 * we can't compute it directly from the row by itself
 * (or from the corresponding row in the tne_L0 table).
 * (A parent directory may be moved/renamed in the
 * working directory, so we need to dynamically compute
 * the repo-path.)
 *
 * ALSO Note that the pc_row *does not* contain any
 * of the dynamic fields.  These *always* have to be
 * computed on the fly by sniffing (because the user
 * is always free to change them behind our back)
 * *even if* we queued a set_attrbits(), overwrite_file...().
 * 
 * NOTE 2012/12/05 There is an edge-case in UPDATE
 * NOTE            where the PC row needs to keep a
 * NOTE            set of the dynamic fields.  For example,
 * NOTE            when we have an ADD-SPECIAL-U item
 * NOTE            that is also SPARSE in the WD.  A sparse
 * NOTE            item cannot be edited (since it isn't
 * NOTE            present in the filesystem) but it can be
 * NOTE            moved/renamed, so it will/can have a PC
 * NOTE            row in the DB.  Normally, it should reference
 * NOTE            the dynamic fields in the backing TNE table,
 * NOTE            but in the case of an ADD-SPECIAL-U item
 * NOTE            there isn't one.  This shows up if UPDATE
 * NOTE            finds a structural conflict and needs to
 * NOTE            preserve the item in the WD.
 * NOTE            See st_sparse_05b.js
 * 
 */
struct sg_wc_db__pc_row
{
	sg_wc_db__pc_row__flags_net		flags_net;
	sg_wc_db__state_structural *	p_s;
	char *							pszHidMerge;	// optional non-dir content-hid from L1 in merge
	sg_wc_db__state_dynamic *		p_d_sparse;		// only defined when sparse
	SG_uint64						ref_attrbits;	// 
};

typedef struct sg_wc_db__pc_row sg_wc_db__pc_row;

//////////////////////////////////////////////////////////////////

/**
 * Prototype for a callback function to receive a
 * pc_row from a SELECT on the tbl_pc table.
 *
 * The callback may steal the pc_row if it wants to.
 */
typedef void (sg_wc_db__pc__foreach_cb)(SG_context * pCtx, void * pVoidData, sg_wc_db__pc_row ** ppPcRow);

//////////////////////////////////////////////////////////////////

/**
 * Prototype for a callback function to receive a
 * pc_row from a SELECT on the CSET table.
 *
 * The callback may steal the row if it wants to.
 */
typedef void (sg_wc_db__csets__foreach_cb)(SG_context * pCtx, void * pVoidData, sg_wc_db__cset_row ** ppCSetRow);

//////////////////////////////////////////////////////////////////

/**
 * We define (1,"*null*") as a pseudo-gid to represent
 * the null parent directory of the root of the tree.
 * This may also be called the "superroot".
 * 
 * This is used for an internal TREENODE that serves
 * as the parent of the TREENODEENTRY for "@".
 * 
 */
#define SG_WC_DB__ALIAS_GID__NULL_ROOT		1
#define SG_WC_DB__GID__NULL_ROOT			"*null*"

/**
 * We define (0,"*undefined*") as a pseudo-gid to be
 * used for items NOT under version control.  There are
 * times when we need to create a data structure for a
 * FOUND or IGNORED item and need to set the alias and/or
 * gid fields (either temporarily or until we decide we
 * need to add or otherwise do something with it).  The
 * original pendingtree code would synthesize a temporary
 * GID for uncontrolled items (for the duration of a scandir
 * or status, for example).  That doesn't work so well when
 * with a separate GID table as we'd accumulate a lot of
 * cruft ids.
 *
 */
#define SG_WC_DB__ALIAS_GID__UNDEFINED		0
#define SG_WC_DB__GID__UNDEFINED			"*undefined*"

//////////////////////////////////////////////////////////////////

#if 1
// TODO            See W8312.
/**
 * Prototype for a callback function to receive an ISSUE
 * from the tbl_issue table.  Each ISSUE represents a
 * conflict that MERGE identified.
 *
 * The callback may steal the pvhIssue or pvhResolve
 * if it wants to.
 *
 */
typedef void (sg_wc_db__issue__foreach_cb)(SG_context * pCtx,
										   void * pVoidData,
										   SG_uint64 uiAliasGid,
										   SG_wc_status_flags statusFlags_x_xr_xu,
										   SG_vhash ** ppvhIssue,
										   SG_vhash ** ppvhResolve);
#endif

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_DB__PRIVATE_TYPEDEFS_H
