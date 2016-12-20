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
 * @file sg_wc__merge__public_typedefs.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC__MERGE__PUBLIC_TYPEDEFS_H
#define H_SG_WC__MERGE__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct sg_wc_merge_args
{
	const SG_rev_spec *			pRevSpec;		// how other branch is specified.  we do not own this
	
	SG_bool						bNoFFMerge;		// disallow fast-forward merges.
	SG_bool						bNoAutoMergeFiles;				// use :skip rather than any tool.
	SG_bool						bComplainIfBaselineNotLeaf;		// require baseline to be a leaf.
	SG_bool						bDisallowWhenDirty;
};

typedef struct sg_wc_merge_args SG_wc_merge_args;

//////////////////////////////////////////////////////////////////

/**
 * Conflict status flags for an entry.  These reflect a "structural"
 * problem with the entry, such as being moved to 2 different directories
 * or being renamed to 2 different names by different branches or being
 * changed in one branch and deleted in another.
 *
 * When one of these bits is set, we *WILL* have a SG_mrg_cset_entry_conflict
 * structure associated with the entry which has more of the details.
 *
 * These types of conflicts are tree-wide -- and indicate that we had a
 * problem constructing the result-cset version of the tree and maybe had
 * to make some compromises/substitutions just to get a sane view of the
 * tree.  For example, for an entry that was moved to 2 different locations
 * by different branches, we may just leave it in the location it was in
 * in the ancestor version of the tree and complain.
 */

typedef SG_uint32 SG_mrg_cset_entry_conflict_flags;

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO									((SG_mrg_cset_entry_conflict_flags)0x00000000)

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE						((SG_mrg_cset_entry_conflict_flags)0x00000001)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME						((SG_mrg_cset_entry_conflict_flags)0x00000002)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS					((SG_mrg_cset_entry_conflict_flags)0x00000004)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT				((SG_mrg_cset_entry_conflict_flags)0x00000010)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT				((SG_mrg_cset_entry_conflict_flags)0x00000020)

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT					((SG_mrg_cset_entry_conflict_flags)0x00000100)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN					((SG_mrg_cset_entry_conflict_flags)0x00000200)

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE				((SG_mrg_cset_entry_conflict_flags)0x00001000)

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE						((SG_mrg_cset_entry_conflict_flags)0x00010000)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME						((SG_mrg_cset_entry_conflict_flags)0x00020000)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS					((SG_mrg_cset_entry_conflict_flags)0x00040000)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT				((SG_mrg_cset_entry_conflict_flags)0x00100000)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT				((SG_mrg_cset_entry_conflict_flags)0x00200000)

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD				((SG_mrg_cset_entry_conflict_flags)0x01000000)	// need to run diff/merge tool to do auto-merge
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__NO_RULE			((SG_mrg_cset_entry_conflict_flags)0x02000000)	// no rule to handle file type (binary?); manual help required
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT	((SG_mrg_cset_entry_conflict_flags)0x04000000)	// auto-merge failed; manual help required
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_ERROR		((SG_mrg_cset_entry_conflict_flags)0x08000000)	// auto-merge failed; manual help required
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK			((SG_mrg_cset_entry_conflict_flags)0x10000000)	// auto-merge successful -- no text conflicts

// if you add a bit to the flags, update the msg table in sg_mrg__private_do_simple.h

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__UNDELETE__MASK						((SG_mrg_cset_entry_conflict_flags)0x00000fff)	// mask of _DELETE_vs_* and _DELETE_CAUSED_ORPHAN

#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK__NOT_OK		((SG_mrg_cset_entry_conflict_flags)0x0f000000)	// mask of _FILE_EDIT_ minus _OK bits
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK__MANUAL		((SG_mrg_cset_entry_conflict_flags)0x0d000000)	// mask of _FILE_EDIT_ bits (NO_RULE or AUTO_FAILED or ERROR)
#define SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK				((SG_mrg_cset_entry_conflict_flags)0x1f000000)	// mask of _FILE_EDIT_ bits

//////////////////////////////////////////////////////////////////

// The SG_MRG__MNEMONIC__ keys are used in the pvhIssue created by
// MERGE for each item with a conflict of some kind.  These are the
// keys in the pvhInput contained within the pvhIssue.
// 
// [] issue.input.A is a vhash that contains the details for the changeset
//    "playing the role of" the ancestor in the MERGE/UPDATE.
// [] issue.input.B contains the details for the original baseline *PLUS* any WD dirt.
// [] issue.input.C contains the details for the merged-in cset in the case of a regular MERGE
//    or the target cset in the case of an UPDATE.
//
// These keys define the "ROLE" of the changeset (or virtual changeset) in
// the overall MERGE/UPDATE.  This positional keying is needed by RESOLVE
// to know how to address/manipulate the data.
//
// Within the A/B/C vhash will be a field with a user-friendly label that
// RESOLVE can use to describe the cset. ("Ancestor" makes sense for a regular
// MERGE, but might be confusing for an UPDATE.)

#define SG_MRG_MNEMONIC__A			"A"
#define SG_MRG_MNEMONIC__B			"B"
#define SG_MRG_MNEMONIC__C			"C"

#define SG_MRG_MNEMONIC__M			"M"		// internal user only -- does not appear in issue.input

// These names will be added to the issue.input.* so that RESOLVE can
// properly label the choice and let use pick one.
//
// See W0993, W1449, W0341.

#define SG_MRG_ACCEPT_LABEL__A__MERGE	"ancestor"
#define SG_MRG_ACCEPT_LABEL__B__MERGE	"local"
#define SG_MRG_ACCEPT_LABEL__C__MERGE	"other"

#define SG_MRG_ACCEPT_LABEL__A__UPDATE	"old"
#define SG_MRG_ACCEPT_LABEL__B__UPDATE	"local"
#define SG_MRG_ACCEPT_LABEL__C__UPDATE	"new"

// TODO 2012/07/23 For REVERT we don't really need these since
// TODO            we're planning for revert to complain and
// TODO            stop before generating any issues.  So for now
// TODO            these are just placeholders.
#define SG_MRG_ACCEPT_LABEL__A__REVERT	"xxx_a"
#define SG_MRG_ACCEPT_LABEL__B__REVERT	"xxx_b"
#define SG_MRG_ACCEPT_LABEL__C__REVERT	"xxx_c"

#define SG_MRG_ACCEPT_LABEL__M		"merge"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC__MERGE__PUBLIC_TYPEDEFS_H
