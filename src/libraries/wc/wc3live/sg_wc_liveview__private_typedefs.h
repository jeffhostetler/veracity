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
 * @file sg_wc_liveview__private_typedefs.h
 *
 * @details "liveview_dir" and "liveview_item" are used to represent
 * the "live" (in-tx) view of a directory and/or item.  This is a
 * superset of the info we get from SCANDIR/READDIR (which represent
 * the pre-tx view).
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_LIVEVIEW__PRIVATE_TYPEDEFS_H
#define H_SG_WC_LIVEVIEW__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef SG_uint32 sg_wc_commit_flags;
#define SG_WC_COMMIT_FLAGS__ZERO							((sg_wc_commit_flags)0x00)
#define SG_WC_COMMIT_FLAGS__PARTICIPATING					((sg_wc_commit_flags)0x01)
#define SG_WC_COMMIT_FLAGS__DIR_CONTENTS_PARTICIPATING		((sg_wc_commit_flags)0x02)
#define SG_WC_COMMIT_FLAGS__BUBBLE_UP						((sg_wc_commit_flags)0x04)
#define SG_WC_COMMIT_FLAGS__MOVED_OUT_OF_MARKED_DIRECTORY	((sg_wc_commit_flags)0x10)
#define SG_WC_COMMIT_FLAGS__MOVED_INTO_MARKED_DIRECTORY		((sg_wc_commit_flags)0x20)

//////////////////////////////////////////////////////////////////

typedef struct _sg_wc_liveview_item_dynamic sg_wc_liveview_item_dynamic;
typedef struct _sg_wc_liveview_item sg_wc_liveview_item;
typedef struct _sg_wc_liveview_dir  sg_wc_liveview_dir;

//////////////////////////////////////////////////////////////////

/**
 * This structure is used to help remember QUEUED
 * changes to dynamic fields on this item.  It is
 * intended to help support multiple operations on
 * an item during a single TX.
 *
 * For example, suppose you want to do a MERGE and
 * a DIFF in the same TX.  MERGE may need to OVERWRITE
 * the contents of a file in the WD.  Normally it would
 * QUEUE the OVERWRITE and then APPLY the TX; the file
 * not actually written until the APPLY phase.  A DIFF
 * normally wants to READ a HISTORICAL/BASELINE and
 * the CURRENT version of the file to print the diffs
 * (using an external tool perhaps).  If you want to
 * able to do a
 *      [BEGIN-TX, MERGE, DIFF, CANCEL-TX]
 * operation, we need to keep track of the QUEUEd
 * changes to the file so that we can lie to DIFF when
 * it asks for the file handle/pathname because the
 * output from MERGE hasn't been moved into place yet.
 *
 * You can think of these fields as the last known values
 * for the item currently in the journal.
 *
 * WE DO NOT OWN ANY OF THESE VHASHES -- THEY ARE
 * BORROWED FROM THE JOURNAL.
 * 
 */
struct _sg_wc_liveview_item_dynamic
{
	const SG_vhash *	pvhContent;			// see 'hid' or 'file' or 'target' fields
	const SG_vhash *	pvhAttrbits;		// see 'attrbits' field
};

//////////////////////////////////////////////////////////////////

/**
 * A LiveViewItem is like a PrescanRow but with a little
 * more info to reflect the in-tx stuff.
 *
 * THIS IS THE MOST IMPORTANT STRUCTURE IN ALL OF WC.
 * ANY/ALL OPERATIONS ON AN ITEM MUST GO THROUGH THIS.
 *
 */
struct _sg_wc_liveview_item
{
	SG_uint64					uiAliasGid;			// might be to permanent or tmp id

	sg_wc_liveview_dir *		pLiveViewDir;		// we don't own this (back ptr)
	SG_string *					pStringEntryname;

	const sg_wc_prescan_row *	pPrescanRow;		// we do not own this

	sg_wc_db__pc_row *			pPcRow_PC;			// in-tx changes; see also pLVI->pPrescanRow->pPcRow_Ref.

	sg_wc_prescan_flags			scan_flags_Live;
	SG_treenode_entry_type		tneType;

	SG_vhash *					pvhIssue;			// present if there is a MERGE/RESOLVE ISSUE on the item.  This is CONSTANT.
	SG_vhash *					pvhSavedResolutions;	// present if RESOLVE has been used on the item.  *IF* pvhIssue.
	SG_wc_status_flags			statusFlags_x_xr_xu;	// resolve-state and choices *IF* pvhIssue.

	// The following fields are ***ONLY*** used during a COMMIT.

	char *						pszHid_dir_content_commit;	// always remember the HID computed by _dive_ (regardless of whether pLVD->pTN_commit is needed)
	sg_wc_commit_flags			commitFlags;
	SG_wc_status_flags			statusFlags_commit;
	SG_bool						bDirHidModified_commit;

	// The following field is cached during a STATUS.
	// Call sg_wc_tx__liveview__compute_baseline_repo_path()
	// to compute/get it.

	SG_string *					pStringBaselineRepoPath;

	// The following fields are used during an UPDATE as a way to
	// transition the baseline during of the item as we get ready
	// to do the "change of basis".  That is, when we queue up
	// everything in the update and then want to call status on
	// the in-progress stuff, we need to point the item's baseline
	// fields at L1 temporarily -- because we don't actually change
	// the readdir/scandir/reference fields and/or actually do the
	// change of basis until after the apply.
	//
	// This is intended to only affect the __get_original_ and
	// __get_baseline_repo_path calls.

	sg_wc_db__tne_row *			pTneRow_AlternateBaseline;
	SG_string *					pStringAlternateBaselineRepoPath;

	// The following fields are used during the QUEUE phase
	// to capture changes to dynamic fields that have been
	// QUEUED but not APPLIED to the working version of this
	// item.

	sg_wc_liveview_item_dynamic queuedOverwrites;

};

struct _sg_wc_liveview_dir
{
	SG_uint64					uiAliasGidDir;

	const sg_wc_prescan_dir *	pPrescanDir;			// we do not own this

	SG_rbtree_ui64 *			prb64LiveViewItems;		// map[<alias-gid> ==> <sg_wc_liveview_item *>] we do not own these

	// This is the set of any items moved-out of this
	// directory (as of this point in the TX).

	SG_rbtree_ui64 *			prb64MovedOutItems;		// set[<alias-gid>]

	// The following fields are ***ONLY*** used during a COMMIT.

	SG_wc_status_flags			statusFlagsParticipatingUnion_commit;	// TODO 2011/11/23 do we really need this
	SG_treenode *				pTN_commit;
};

//////////////////////////////////////////////////////////////////

typedef SG_uint32 sg_wc_liveview_dir__find_flags;
#define SG_WC_LIVEVIEW_DIR__FIND_FLAGS__RESERVED		0x0001
#define SG_WC_LIVEVIEW_DIR__FIND_FLAGS__UNCONTROLLED	0x0002
#define SG_WC_LIVEVIEW_DIR__FIND_FLAGS__DELETED			0x0010
#define SG_WC_LIVEVIEW_DIR__FIND_FLAGS__MATCHED			0x0020
#define SG_WC_LIVEVIEW_DIR__FIND_FLAGS__LOST			0x0040
#define SG_WC_LIVEVIEW_DIR__FIND_FLAGS__SPARSE			0x0080

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_LIVEVIEW__PRIVATE_TYPEDEFS_H
