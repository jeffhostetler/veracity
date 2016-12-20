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

#ifndef H_SG_WC_PRESCAN__PRIVATE_TYPEDEFS_H
#define H_SG_WC_PRESCAN__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Crude summary flags for sg_wc_prescan.  These flags are
 * only concerned with the results of the match-up in
 * the SCANDIR/READDIR.
 * 
 * TODO 2011/09/14 Since these are now used by both PRESCAN
 * TODO            and LIVEVIEW, we should move them out of
 * TODO            this file and rename them slightly.
 * 
 */
enum _sg_wc_prescan_flags
{
	SG_WC_PRESCAN_FLAGS__RESERVED						= 1,	// .sgdrawer/.sgcloset or something portability-wise-equivalent
	SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED		= 2,	// may be FOUND or IGNORED

	SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT				= 3,	// the '@/' directory cannot be structurally changed
	SG_WC_PRESCAN_FLAGS__CONTROLLED_INACTIVE_DELETED	= 4,	// scandir doesn't try to match it
	SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED		= 5,	// scandir matched observed item to known controlled item (includes recently ADDED/CREATED items)
	SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_LOST			= 6,	// scandir did not find an item in the directory to associate with the controlled item
	SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE		= 7,	// scandir doesn't try to match because we said it wasn't there
	SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN		= 8,	// added-by-merge after the scandir

};

typedef enum _sg_wc_prescan_flags sg_wc_prescan_flags;



#define SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(sf)	((sf) == SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED)
#define SG_WC_PRESCAN_FLAGS__IS_RESERVED(sf)					((sf) == SG_WC_PRESCAN_FLAGS__RESERVED)

#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED(sf)					((sf) >= SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT)

#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_ROOT(sf)				((sf) == SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT)
#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(sf)			((sf) == SG_WC_PRESCAN_FLAGS__CONTROLLED_INACTIVE_DELETED)
#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_MATCHED(sf)			((sf) == SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_MATCHED)
#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(sf)				((sf) == SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_LOST)
#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(sf)			((sf) == SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE)
#define SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_POSTSCAN(sf)			((sf) == SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN)

//////////////////////////////////////////////////////////////////

/**
 * A SCANROW represents the details for an item
 * AS WE SAW IT ON THE DISK IN THE WORKING DIRECTORY
 * combined with the PRE-TX (reference) state of the
 * item.
 *
 */
struct _sg_wc_prescan_row
{
	SG_uint64				uiAliasGid;			// may be permanent or tmp id

	sg_wc_prescan_dir *		pPrescanDir_Ref;		// we don't own this (back ptr)
	SG_string *				pStringEntryname;

	sg_wc_readdir__row *	pRD;			// only defined when observed by READDIR
	sg_wc_db__tne_row *		pTneRow;		// only defined when controlled
	sg_wc_db__pc_row *		pPcRow_Ref;		// only defined when controlled and in pc table at the start of the TX

	sg_wc_prescan_flags		scan_flags_Ref;
	SG_treenode_entry_type	tneType;
};

/**
 * A SCANDIR represents the details of the
 * contents of a directory.  This is a combination
 * of what we OBSERVED during READDIR and the PRE-TX
 * (reference) state.
 *
 */
struct _sg_wc_prescan_dir
{
	SG_uint64			uiAliasGidDir;

	SG_string *			pStringRefRepoPath;		// the computed pre-tx repo-path

	// We CANNOT use a
	//      "RBTREE<entryname> ==> <row>"
	// to address the matched-up scan-dir rows
	// but an entryname alone is not unique
	// because we could have (lost,"foo",file)
	// and (found,"foo",directory).
	// 
	// Also, because we allow moves/renames+deletes,
	// we could have many instances of (deleted,"foo",*)
	// in the directory, so we can't rely on entryname
	// here.
	//
	// So we use the alias-gid of the row to
	// index the container of rows.

	SG_rbtree_ui64 *	prb64PrescanRows;	// map[<alias-gid> ==> <sg_wc_prescan_row *>] we DO NOT own these

	// This set lists any items that were MOVED-OUT
	// of the directory (as of the time that the TX
	// started (in a previous TX)).

	SG_rbtree_ui64 *	prb64MovedOutItems;	// set[<alias-gid>]
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PRESCAN__PRIVATE_TYPEDEFS_H
