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

#ifndef H_SG_VV2__STATUS__PRIVATE_TYPEDEFS_H
#define H_SG_VV2__STATUS__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef struct _sg_vv2status     sg_vv2status;
typedef struct _sg_vv2status_od  sg_vv2status_od;
typedef struct _sg_vv2status_odi sg_vv2status_odi;
typedef enum   _sg_vv2status_odt sg_vv2status_odt;
typedef struct _sg_vv2status_cset_column sg_vv2status_cset_column;

#define SG_VV2__OD_NDX_ORIG 0
#define SG_VV2__OD_NDX_DEST 1

enum _sg_vv2status_odt
{
	SG_VV2__OD_TYPE_UNPOPULATED			= 0x00,		// a void (possibly temporarily)
	SG_VV2__OD_TYPE_FILE				= 0x01,		// a regular file
	SG_VV2__OD_TYPE_SYMLINK				= 0x02,		// a symlink
	SG_VV2__OD_TYPE_UNSCANNED_FOLDER	= 0x04,		// a folder (something that we may or may not have to dive into)
	SG_VV2__OD_TYPE_SCANNED_FOLDER		= 0x08,		// a folder that we have already dived into
	// subrepo
};


struct _sg_vv2status_odi			// an instance of an object in a version of the tree
{
	const SG_treenode_entry *	pTNE;			// we DO NOT own this

	SG_int32					depthInTree;	// only used for SG_VV2__OD_NDX_ORIG,SG_VV2__OD_NDX_DEST during scan, -1 when undefined/unknown
	sg_vv2status_odt			typeInst;

	char						bufParentGid[SG_GID_BUFFER_LENGTH];
};


struct _sg_vv2status_od
{
	sg_vv2status_odi *			apInst[2];			// SG_VV2__OD_NDX_ORIG and SG_VV2__OD_NDX_DEST
	SG_string *					pStringRefRepoPath;
	SG_string *					pStringCurRepoPath;
	SG_wc_status_flags			statusFlags;
	SG_int32					minDepthInTree;
	char						bufGidObject[SG_GID_BUFFER_LENGTH];
};


struct _sg_vv2status_cset_column
{
	char				bufHidTreeNodeRoot[SG_HID_MAX_BUFFER_LENGTH];
	char				bufHidCSet[SG_HID_MAX_BUFFER_LENGTH];
};

//////////////////////////////////////////////////////////////////

struct _sg_vv2status
{
	SG_repo *			pRepo;

	// we keep a rbtree of the tree-nodes that we load from disk so that we can make
	// sure that we free them all without having to worry about them when unwinding
	// our stack (and because the tree-node-entries are actually inside the tree-nodes)
	// and because a treenode might be referenced by both changesets and we don't want
	// to load it twice from disk.
	//
	// We now have the option to be given this treenode-cache so that we can share it.
	// This is usefull when doing an MSTATUS because we need to do several related
	// pairwise diffs/statuses.
	//
	// So this cache owns the treenodes within, but this structure does not own the cache.

	SG_rbtree *			prbTreenodesAllocated;		// map [hidTN --> pTreenode] we own pTreenodes

	// per-cset data is stored in a "column" for each.  these are ordered because
	// the user is expecting us to report in LEFT-vs-RIGHT or OLDER-vs-NEWER terms.

	sg_vv2status_cset_column aCSetColumns[2];			// SG_VV2__OD_NDX_ORIG and SG_VV2__OD_NDX_DEST

	// as we explore both trees, we must sync up files/folders by Object GID and
	// see how the corresponding entries compare.  note that we must do this in
	// Object GID space rather than pathname space so that we can detect moves and
	// renames and etc.
	//
	// this rbtree contains a ROW for each Object GID that we have observed.
	// this tree contains all of the ROWs that we have allocated.

	SG_rbtree *			prbObjectDataAllocated;		// map[gidObject --> pObjectData] we own pObjectData

	// we build work-queue containing a depth-sorted list of FOLDERS that
	// we ***MAY*** need to dive into.
	//
	// i say "may" here, because there are times when we can short-circuit
	// the full treewalk.  because of the bubble-up effect on the Blob HIDs
	// of sub-folders, we may not have to walk a part of the 2 versions of
	// the tree when we can detect that they are identical from the HIDs.
	//
	// as we process the work-queue ObjectData row-by-row, we remove an entry
	// as we process it and we (conditionally) add to the work-queue any
	// sub-folders that it may have.
	//
	// processing continues until we have removed all entries from the
	// work-queue.  this represents a kind of closure on the tree scan.
	//
	// we sort this by depth-in-the-tree so that we are more likely to scan
	// across (like a breadth-first search) rather than fully populating
	// one version before visiting the other version (like in a depth-first
	// treewalk).
	//
	// note: this does not prevent us from "accidentially" scanning one side
	// of an identical sub-folder pair.  in the case where a folder is moved
	// deeper in the tree (but otherwise unchanged), we will scan the shallower
	// side (thinking it is peerless) before seeing it on the other side.  in
	// this case, we go ahead and repeat the scan in the deeper version so that
	// all the (identical) children get populated on both sides (so that they
	// don't look like additions/deletions).  these will get pruned out later
	// with the regular identical-pair filter.

	SG_rbtree *			prbWorkQueue;		// map[(depth,gidObject) --> pObjectData] we borrow pointers

	//////////////////////////////////////////////////////////////////
	// Fields for reporting new canonical status using domain-prefixed
	// repo-paths and per-cset sub-sections.

	char chDomain_0;
	char chDomain_1;

	char * pszLabel_0;
	char * pszLabel_1;

	char * pszWasLabel_0;
	char * pszWasLabel_1;

};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV2__STATUS__PRIVATE_TYPEDEFS_H
