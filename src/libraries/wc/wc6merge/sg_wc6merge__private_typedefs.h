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
 * @file sg_wc6merge__private_typedefs.h
 * (formerly ut/sg_mrg__private_typedefs.h)
 *
 */

// TODO 2012/01/19 change all of the typedefs to have lower case
// TODO            prefixes now that they are completely private
// TODO            inside WC.

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC6MERGE__PRIVATE_TYPEDEFS_H
#define H_SG_WC6MERGE__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef enum _sg_fake_merge_mode
{
	SG_FAKE_MERGE_MODE__MERGE   = 0,
	SG_FAKE_MERGE_MODE__UPDATE  = 1,
	SG_FAKE_MERGE_MODE__REVERT  = 2
} sg_fake_merge_mode;

typedef struct _sg_mrg_cset_entry_collision		SG_mrg_cset_entry_collision;
typedef struct _sg_mrg_cset_entry_conflict		SG_mrg_cset_entry_conflict;
typedef struct _sg_mrg_cset_entry				SG_mrg_cset_entry;
typedef struct _sg_mrg_cset						SG_mrg_cset;
typedef struct _sg_mrg_cset_dir					SG_mrg_cset_dir;
typedef struct _sg_mrg							SG_mrg;

/**
 * We use the NEQ when we compare 2 entries for equality. It tells us what is different.
 */
typedef SG_uint32 SG_mrg_cset_entry_neq;
#define SG_MRG_CSET_ENTRY_NEQ__ALL_EQUAL			((SG_mrg_cset_entry_neq)0x00)

#define SG_MRG_CSET_ENTRY_NEQ__ATTRBITS				((SG_mrg_cset_entry_neq)0x01)
#define SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME			((SG_mrg_cset_entry_neq)0x02)
#define SG_MRG_CSET_ENTRY_NEQ__GID_PARENT			((SG_mrg_cset_entry_neq)0x04)

#define SG_MRG_CSET_ENTRY_NEQ__DELETED				((SG_mrg_cset_entry_neq)0x10)	// this one doesn't mix well with the others...

#define SG_MRG_CSET_ENTRY_NEQ__SYMLINK_HID_BLOB		((SG_mrg_cset_entry_neq)0x20)
#define SG_MRG_CSET_ENTRY_NEQ__FILE_HID_BLOB		((SG_mrg_cset_entry_neq)0x40)
#define SG_MRG_CSET_ENTRY_NEQ__SUBMODULE_HID_BLOB	((SG_mrg_cset_entry_neq)0x80)

//////////////////////////////////////////////////////////////////

/**
 * This structure is used to hold info for 1 entry (a file/symlink/directory);
 * this is basically a SG_treenode_entry converted into fields, but we also
 * know the GID of the parent directory.
 *
 * If we have a conflict for this entry, we also have a link to the conflict structure.
 */
struct _sg_mrg_cset_entry
{
	SG_mrg_cset *					pMrgCSet;				// the cset that we are a member of.  we do not own this.
	SG_mrg_cset_entry_conflict *	pMrgCSetEntryConflict;	// only set when we have a conflict.  we do not own this.
	SG_mrg_cset_entry_collision *	pMrgCSetEntryCollision;	// only set when we have a collision of some kind.  we do not own this.
	SG_string *						pStringEntryname;			// we own this
	SG_mrg_cset *					pMrgCSet_FileHidBlobInheritedFrom;	// only set on _clone_.  we do not own this.
	SG_mrg_cset *					pMrgCSet_CloneOrigin;	// we do not own this.

	SG_string *						pStringPortItemLog;		// private per-entry portability log.  only used in final-result.  we own this.
	SG_varray *						pvaPotentialPortabilityCollisions;	// only set when potential collisions reported.  we own this

	SG_uint64						uiAliasGid;		// WC alias of bufGid_Entry[].

	SG_int64						markerValue;			// data for caller to mark/unmark an entry
	SG_int64						attrBits;
	SG_wc_status_flags				statusFlags__using_wc;	// the initial statusFlags for the __using_wc() branch; (zero otherwise)
	SG_wc_port_flags				portFlagsObserved;
	SG_treenode_entry_type			tneType;

	SG_string *						pStringEntryname_OriginalBeforeMadeUnique;	// set to ref name if we needed to make a unique name.

	SG_bool							bForceParkBaselineInSwap;		// true when we have a final collision and want to force the baseline version to be parked in swap. (only set on baseline entries)

	char							bufGid_Parent[SG_GID_BUFFER_LENGTH];	// GID of the containing directory.
	char							bufGid_Entry[SG_GID_BUFFER_LENGTH];		// GID of this entry.

	char							bufHid_Blob[SG_HID_MAX_BUFFER_LENGTH];		// not set for directories
};

//////////////////////////////////////////////////////////////////

/**
 * This structure is used to gather all of the entries present in a directory.
 *
 * We only use this when we need to see the version control tree in a hierarchial
 * manner -- as opposed to the flat view that we use most of the time.
 *
 * Our prbEntry_Children contains a list of all of the entries that are present
 * in the directory;  we borrow these pointers from our SG_mrg_cset->prbEntries.
 *
 * We hold a reference to the cset that we are a member of.
 */
struct _sg_mrg_cset_dir
{
	SG_mrg_cset *					pMrgCSet;						// the cset that we are a member of.                           we do not own this.
	SG_mrg_cset_entry *				pMrgCSetEntry_Self;				// back-link to our cset entry.    we do not own this.
	SG_rbtree *						prbEntry_Children;				// map[<gid-entry> --> SG_mrg_cset_entry *]            we DO NOT own these; we borrow from the SG_mrg_cset.prbEntries
	SG_wc_port *					pPort;							// we own this.
	SG_rbtree *						prbCollisions;					// map[<entryname> --> SG_mrg_cset_entry_collision *]  we own this and the values within it.

	char							bufGid_Entry_Self[SG_GID_BUFFER_LENGTH];	// GID of this entry.
};

//////////////////////////////////////////////////////////////////

enum _sg_mrg_cset_origin
{
	SG_MRG_CSET_ORIGIN__ZERO		= 0,	// unset
	SG_MRG_CSET_ORIGIN__LOADED		= 1,	// a real CSET that was loaded from the REPO.
	SG_MRG_CSET_ORIGIN__VIRTUAL		= 2,	// a synthetic CSET that we build during the merge computation.
};

typedef enum _sg_mrg_cset_origin	SG_mrg_cset_origin;

/**
 * This structure is used to hold everything that we know about a single CHANGE-SET.
 *
 * We start with a FLAT LIST of all of the entries in this version of the entire
 * version control tree.  Each of the SG_mrg_cset_entry's are stored in the .prbEntries.
 *
 * Later, we *MAY* build a hierarchial view of the tree using SG_mrg_cset_dir's
 * and .prgDirs.
 *
 * We do remember some fields to help name/find the root node to help with some things.
 *
 */
struct _sg_mrg_cset
{
	SG_string *						pStringCSetLabel;		// L0, L1, ... for internal use (tbl_csets, tne_*, pc_* tables)
	char *							pszMnemonicName;		// STRUCTURAL mnemonic name for where this "virtual" cset fits in the merge (needed to help RESOLVE).
	SG_string *						pStringAcceptLabel;		// user-friendly label for this version for RESOLVE-accept.

	SG_rbtree *						prbEntries;				// map[<gid-entry> --> SG_mrg_cset_entry *]   we own these
	SG_mrg_cset_entry *				pMrgCSetEntry_Root;		// we do not own this. (borrowed from prbEntries assoc-value)

	SG_rbtree *						prbDirs;				// map[<gid-entry> --> SG_mrg_cset_dir *]     we own these -- only defined when needed
	SG_mrg_cset_dir *				pMrgCSetDir_Root;		// we do not own this.  root of hierarchial view -- only defined when actually needed

	SG_rbtree *						prbConflicts;			// map[<gid-entry> --> SG_mrg_cset_entry_conflict *]	we own these -- only defined when needed

	SG_rbtree *						prbDeletes;				// map[<gid-entry> --> SG_mrg_cset_entry * ancestor]	we do not own these -- only defined when needed

	char							bufGid_Root[SG_GID_BUFFER_LENGTH];				// GID of the actual-root directory.
	char							bufHid_CSet[SG_HID_MAX_BUFFER_LENGTH];			// HID of this CSET.  (will be empty for virtual CSETs)

	SG_mrg_cset_origin				origin;
	SG_daglca_node_type				nodeType;				// LEAF or SPCA or LCA -- only valid when (origin == SG_MRG_CSET_ORIGIN__LOADED)
};

//////////////////////////////////////////////////////////////////

/**
 * This structure is used to represent what we know about a conflict
 * on a single entry.  We know the SG_mrg_cset_entry for the entry
 * as it appeared in the ancestor.  And we know the corresponding
 * entries in 2 or more branches where changes were made.  The
 * vectors only contain the (entry, neq-flags) of the branches
 * where there was a change.
 *
 * We use this structure to bind the ancestor entry with the
 * correpsonding entry in the branches so that a caller (such as
 * the command line client or gui) can decide how to present the
 * info and let the user choose how to resolve it.  (handwave)
 *
 * Note that we may have a combination of issues.
 * For example, we may have 2 branches that renamed an entry to
 * different values.  Or that renamed it to the same new name.
 * The former requires help.  The latter we can just eat and treat
 * like a simple rename in only 1 branch (and maybe even avoid
 * generating this conflict record).  But, we might have a combination
 * of issues, for example a double rename and a double move.  If one
 * is a duplicate and one is divergent, then we need to be able to
 * present the whole thing and let the resolve the divergent part
 * and still handle the duplicate part (with or without showing
 * them it).
 *
 * Granted, moves, renames, and attrbit changes are rare.
 * This most likely to be seen when 2 or more branches modify
 * the contents of a file.  In this case, we just know that the
 * contents changes -- we need to run something like gnu-diff3 or
 * DiffMerge to tell us if the files can be auto-merged or if we
 * need help.  (And cache the auto-merge result if successful.)
 * We use this structure to also hold the content-merge state
 * so that when we actually populate the WD, we know what to do
 * with the content.
 *
 * We accumulate these (potential and actual) conflicts in the
 * SG_mrg_cset in a rbtree.
 *
 * Note that when a branch/leaf deletes an entry, we don't have
 * an entry to put in the pVec_MrgCSetEntries vector, so we have
 * a second vector listing the branches/leaves where it was deleted.
 */

struct _sg_mrg_cset_entry_conflict
{
	SG_mrg_cset *						pMrgCSet;						// the virtual cset that we are building as we compute the merge.  we do not own this
	SG_mrg_cset_entry *					pMrgCSetEntry_Ancestor;			// the ancestor entry (owned by the ancestor cset).  we do not own this
	SG_mrg_cset_entry *					pMrgCSetEntry_Baseline;			// the bseline entry (when available).  we do not own this
	SG_vector *							pVec_MrgCSetEntry_Changes;		// vec[SG_mrg_cset_entry *]		we own the vector, but not the pointers within (they belong to their respective csets).
	SG_vector_i64 *						pVec_MrgCSetEntryNeq_Changes;	// vec[SG_mrg_cset_entry_neq]	we own the vector.
	SG_vector *							pVec_MrgCSet_Deletes;			// vec[SG_mrg_cset *]			we own the vector, but not the pointers within.
	SG_mrg_cset_entry *					pMrgCSetEntry_Composite;		// the merged composite entry.  we DO NOT own this.

	SG_rbtree *							prbUnique_AttrBits;				// map[#attrbits --> vec[SG_mrg_cset_entry *]]  we own vector, but not the pointers within.
	SG_rbtree *							prbUnique_Entryname;			// map[entryname --> vec[SG_mrg_cset_entry *]]  we own vector, but not the pointers within.
	SG_rbtree *							prbUnique_GidParent;			// map[gid       --> vec[SG_mrg_cset_entry *]]  we own vector, but not the pointers within.
	SG_rbtree *							prbUnique_Symlink_HidBlob;		// map[hid       --> vec[SG_mrg_cset_entry *]]  we own vector, but not the pointers within.
	SG_rbtree *							prbUnique_File_HidBlob;			// map[hid       --> vec[SG_mrg_cset_entry *]]  we own vector, but not the pointers within.
	SG_rbtree *							prbUnique_Submodule_HidBlob;	// map[hid       --> vec[SG_mrg_cset_entry *]]  we own vector, but not the pointers within.

	SG_vector *							pVec_MrgCSetEntry_OtherDirsInCycle;	// vec[SG_mrg_cset_entry *] we own the vector, but not the pointers within.
	SG_string *							pStringPathCycleHint;

	SG_pathname *						pPathTempDirForFile;			// temp-dir for the versions of this file
	SG_filetool *						pMergeTool;						// only set when needed

	SG_mrg_cset_entry_conflict_flags	flags;

	SG_pathname *						pPathTempFile_Ancestor;			// Path to the ANCESTOR version of file in tmp (only when __DIVERGENT_FILE_EDIT__).
	SG_pathname *						pPathTempFile_Result;			// location of the temp-file for the merge-result.
	SG_pathname *						pPathTempFile_Baseline;			// location of the temp-file for the baseline (L0) version.
	SG_pathname *						pPathTempFile_Other;			// location of the temp-file for the other (L1) version to merge.

	char *								pszHidGenerated;	// HID of auto-merge (may or may not be disposable)
	char *								pszHidDisposable;	// HID of auto-merged *WHEN FULLY AUTOMATIC AND DISPOSABLE*
};

//////////////////////////////////////////////////////////////////

/**
 * Collision status for entries within a directory.  These reflect
 * real collisions as reported by SG_wc_port.
 *
 * Since SG_mrg_entry_conflict is used to bind multiple versions of the
 * same entry together and a collision indicates that different entries
 * map to the same entryname, collision problems DO NOT cause a
 * SG_mrg_cset_entry_conflict to be generated, rather they get a
 * SG_mrg_cset_entry_collision.
 *
 * Ideally, all colliding entries are the same.  We just accumulate them
 * in the order that we found them (probably GID-ENTRY order, but that
 * doesn't matter).
 */
struct _sg_mrg_cset_entry_collision
{
	SG_vector *		pVec_MrgCSetEntry;		// vec[SG_mrg_cset_entry *] the entries in the collision.  we own the vector, but do not own the values.
};

//////////////////////////////////////////////////////////////////

/**
 * The main structure for driving a version control tree merge.
 *
 * We give it a PENDING-TREE so that we can find the associated WD,
 * REPO, and BASELINE.
 */
struct _sg_mrg
{
	//////////////////////////////////////////////////////////////////
	// Input arguments and initial conditions for new WC-based version.
	//////////////////////////////////////////////////////////////////

	const SG_wc_merge_args *	pMergeArgs;		// we do not own this

	SG_wc_tx *					pWcTx;			// we do not own this
	SG_vhash *					pvhPile;

	// details about the state of the WD before we started.
	char *						pszHid_StartingBaseline;
	char *						pszBranchName_Starting;

	// details about how they requested/named the target changeset.
	char *						pszHidTarget;		// aka the Other CSet
	char *						pszBranchNameTarget;
//	SG_bool						bRequestedByBranch;
//	SG_bool						bRequestedByRev;

	SG_uint32					nrHexDigitsInHIDs;	// number of hex digits in a HID for HIDs in this repo.

	sg_fake_merge_mode		eFakeMergeMode;
	SG_bool					bRevertNoBackups;

	SG_bool					bDisallowWhenDirty;		// only used when SG_FAKE_MERGE_MODE__MERGE or __UPDATE.

	//////////////////////////////////////////////////////////////////
	// fields to let us remember the set of input Leaves (L0, L1, ...)
	// and the DAG-LCA that we compute.  and a map of all of the CSets
	// that we have loaded (both for leaves and the various ancestors).
	// We also remember which leaf is is the baseline.

	SG_daglca *				pDagLca;
	SG_mrg_cset *			pMrgCSet_LCA;			// link to LCA                                we own this
	SG_mrg_cset *			pMrgCSet_Baseline;		// link to Baseline (when member of merge)    we own this
	SG_mrg_cset *			pMrgCSet_Other;			// link to Target/Other Branch                we own this
	SG_rbtree *				prbCSets_Aux;			// map[<hid-cset> --> SG_mrg_cset *]          we own these

	//////////////////////////////////////////////////////////////////
	// We also remember the CSet of final merge.  This is an IN-MEMORY-ONLY
	// representation of the results of the merge and reflects what the
	// WD *WILL EVENTUALLY* look like *IF* the merge is actually applied.
	// We can use this to dump a preview and/or to do the APPLY.

	SG_mrg_cset *			pMrgCSet_FinalResult;

	//////////////////////////////////////////////////////////////////
	// Accumulate the stats for the 'vv merge' summary report.
	// This summarizes the stuff we needed to do to the baseline/WD
	// to make it look like the final-result.

	SG_uint32				nrUnchanged;
	SG_uint32				nrChanged;
	SG_uint32				nrDeleted;
	SG_uint32				nrAddedByOther;		// if we're being sloppy, we can
	SG_uint32				nrOverrideDelete;	// combine these 2 (see __add_special).
	SG_uint32				nrOverrideLost;
	SG_uint32				nrAutoMerged;
	SG_uint32				nrResolved;
	SG_uint32				nrUnresolved;

	//////////////////////////////////////////////////////////////////
	// During a REVERT-ALL we sometimes need to kill items that
	// are (or will be) ADDED-SPECIAL + REMOVED.  Normally, we
	// keep these rows in the pc_L0 table so that STATUS can report
	// them, but after a REVERT we just want them to go away.
	SG_vector_i64 *			pVecRevertAllKillList;

};

//////////////////////////////////////////////////////////////////

#define _SG_MRG__PLAN__MARKER__FLAGS__ZERO			0x0000

#define _SG_MRG__PLAN__MARKER__FLAGS__PARKED		0x0001
#define _SG_MRG__PLAN__MARKER__FLAGS__APPLIED		0x0080

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6MERGE__PRIVATE_TYPEDEFS_H
