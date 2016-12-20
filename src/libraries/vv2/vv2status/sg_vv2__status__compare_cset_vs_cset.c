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
 * @file sg_vv2__status__compare_cset_vs_cset.c
 *
 * @details Private routine to compute cset-vs-cset status.
 *
 * The goal here is to fetch as little as possible of the
 * TREENODES for both CSETS to compute all of the changes.
 * Normally, we would need to load all of the treenodes in
 * both versions of the tree into 2 tables and then diff them.
 * But since the Content-HID of a directory can be used to
 * indicate if there are any changes *within* the directory,
 * we can short-circuit this and only load "interesting" nodes.
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

static void _td__fixup_od_inst_depth(SG_context * pCtx,
									 sg_vv2status * pST,
									 sg_vv2status_od * pOD,
									 SG_uint32 ndx)
{
	sg_vv2status_odi * pInst_self;
	sg_vv2status_od  * pOD_parent;
	sg_vv2status_odi * pInst_parent;
	SG_bool bFound;

	pInst_self = pOD->apInst[ndx];

	if (!pInst_self)			// TODO should this be an error since we only
		return;					// TODO get called explicitly for nodes we want (rather than in a blanket foreach)

	if (pInst_self->depthInTree != -1)		// already have the answer
		return;

	if (!*pInst_self->bufParentGid)			// if no parent, we are root
	{
		pInst_self->depthInTree = 1;
		return;
	}

	SG_ERR_CHECK(  SG_rbtree__find(pCtx,
								   pST->prbObjectDataAllocated,
								   pInst_self->bufParentGid,
								   &bFound,
								   (void **)&pOD_parent)  );
	SG_ASSERT_RELEASE_FAIL2(  (bFound),
					  (pCtx,
					   "Object [GID %s] parent [Gid %s] not in cache.\n",
					   pOD->bufGidObject,
					   pInst_self->bufParentGid)  );

	pInst_parent = pOD_parent->apInst[ndx];

#if 0 && defined(DEBUG)
	if (!pInst_parent)
	{
		// TODO remove this hack....
		// HACK there are a couple of cases where the parent isn't in the cache until
		// HACK i fix something in sg_pendingtree.  until then, fake a path so u0043
		// HACK doesn't crash.

		pInst_self->depthInTree = -2;
		return;
	}
#endif

	SG_ASSERT_RELEASE_FAIL2(  (pInst_parent),
					  (pCtx,
					   "Object [GID %s][ndx %d] parent [Gid %s] not in cache.\n",
					   pOD->bufGidObject, ndx,
					   pInst_self->bufParentGid)  );

	SG_ERR_CHECK(  _td__fixup_od_inst_depth(pCtx, pST, pOD_parent, ndx)  );

	pInst_self->depthInTree = pInst_parent->depthInTree + 1;

fail:
	return;
}

/**
 * Stuff instance data for a version of this file/folder
 * into the corresponding column.
 *
 */
static void _td__set_od_column(SG_context * pCtx,
							   sg_vv2status * pST,
							   sg_vv2status_od * pOD,
							   SG_uint32 ndx,
							   const SG_treenode_entry * pTreenodeEntry,
							   const sg_vv2status_od * pOD_parent)
{
	sg_vv2status_odi * pInst;
	const sg_vv2status_odi * pInst_other;
	SG_uint32 ndx_other;
	SG_treenode_entry_type tneType;
	SG_treenode_entry_type tneType_other;
	SG_bool bAlreadyPresent;

	SG_ASSERT_RELEASE_FAIL(  ((ndx == SG_VV2__OD_NDX_ORIG) || (ndx == SG_VV2__OD_NDX_DEST))  );
	ndx_other = ! ndx;		// we assume SG_VV2__OD_NDX_ORIG==0 and SG_VV2__OD_NDX_DEST==1.

	// See if we already have info for this GID in this column.
	// This would mean that the entry appears more than once in
	// the tree (which is a VERY BAD THING).

	bAlreadyPresent = (pOD->apInst[ndx] != NULL);
	SG_ASSERT_RELEASE_FAIL2(  (!bAlreadyPresent),
							  (pCtx,
							   "Object [GID %s] occurs in more than once place in this version [ndx %d] of the tree).",
							   pOD->bufGidObject,ndx)  );

	SG_ERR_CHECK(  sg_vv2__status__odi__alloc(pCtx, &pOD->apInst[ndx])  );

	pInst = pOD->apInst[ndx];
	pInst_other = pOD->apInst[ndx_other];			// the peer to this object in cset0-vs-cset1

	pInst->pTNE = pTreenodeEntry;

	// an object can NEVER change types (ie from file to symlink or folder)

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pInst->pTNE, &tneType)  );

	if (pInst_other)
	{
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pInst_other->pTNE, &tneType_other)  );
		SG_ASSERT_RELEASE_FAIL2(  (tneType == tneType_other),
								  (pCtx,
								   "Object [GID %s][ndx %d] changed type [from %d][to %d] vs [ndx_other %d]",
								   pOD->bufGidObject, ndx,
								   tneType, tneType_other,
								   ndx_other)  );
	}

	if (pOD_parent)
		SG_ERR_CHECK(  SG_strcpy(pCtx,
								 pInst->bufParentGid,
								 SG_NrElements(pInst->bufParentGid),
								 pOD_parent->bufGidObject)  );

//	SG_ASSERT(  (pInst->depthInTree != -1)  );
	SG_ERR_CHECK(  _td__fixup_od_inst_depth(pCtx,pST,pOD,ndx)  );

	if (!pInst_other)
		pOD->minDepthInTree = pInst->depthInTree;
	else
		pOD->minDepthInTree = SG_MIN(pInst->depthInTree,pInst_other->depthInTree);

	switch (tneType)
	{
	default:
		SG_ASSERT_RELEASE_FAIL2(  (0),
						  (pCtx,
						   "Object [GID %s][ndx %d] of unknown type [%d].",
						   pOD->bufGidObject,ndx,(SG_uint32)tneType)  );
		break;

	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		pInst->typeInst = SG_VV2__OD_TYPE_FILE;
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
		pInst->typeInst = SG_VV2__OD_TYPE_SYMLINK;
		break;

	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		pInst->typeInst = SG_VV2__OD_TYPE_UNSCANNED_FOLDER;
		if (!pInst_other)
		{
			// this is the first time that an instance has been populated for this
			// Object GID in either SG_VV2__OD_NDX_ORIG or SG_VV2__OD_NDX_DEST.
			//
			// add the folder to the work-queue so that a later iteration of the main
			// loop will process the contents *within* the folder -- IF NECESSARY.

			SG_ERR_CHECK(  sg_vv2__status__add_to_work_queue(pCtx,pST,pOD)  );
		}
		else if (pInst_other->typeInst == SG_VV2__OD_TYPE_UNSCANNED_FOLDER)
		{
			// we are the second column (of SG_VV2__OD_NDX_ORIG and SG_VV2__OD_NDX_DEST) that has been seen properly.
			// that is, the second column that we actually got the data directly from
			// the repo for.  the other side should have added an entry to the work-queue for
			// us.  short-circuit-eval if we can.

			SG_bool bCanShortCircuit;

			SG_ERR_CHECK(  sg_vv2__status__can_short_circuit_from_work_queue(pCtx,pOD,&bCanShortCircuit)  );
			if (bCanShortCircuit)
				SG_ERR_CHECK(  sg_vv2__status__remove_from_work_queue(pCtx,pST,pOD,pInst_other->depthInTree)  );
			else
				SG_ERR_CHECK(  sg_vv2__status__assert_in_work_queue(pCtx,pST,pOD,pInst_other->depthInTree)  );
		}
		else /* pInst_other->typeInst == SG_VV2__OD_TYPE_SCANNED_FOLDER */
		{
			// we are the second column -- and the other side has already been scanned.
			// this usually only happens when a folder is moved deeper/shallower into the
			// tree.
			//
			// force scan on this side so that the children of the other side don't look
			// artificially peerless.

			SG_ERR_CHECK(  sg_vv2__status__add_to_work_queue(pCtx,pST,pOD)  );
		}
		break;
	}

	return;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Find the treenode for the given HID in the allocated treenode cache or load it
 * from disk and put it in the cache.
 *
 * This can fail if the repo is sparse.
 *
 * The treenode cache owns this treenode.  Do not free it directly.
 */
static void _td__load_treenode(SG_context * pCtx, sg_vv2status * pST, const char * szHid, const SG_treenode ** ppTreenode)
{
	SG_treenode * pTreenodeAllocated = NULL;
	SG_treenode * pTreenode;
	SG_bool bFound;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pST->prbTreenodesAllocated,szHid,&bFound,(void **)&pTreenode)  );
	if (!bFound)
	{
#if TRACE_VV2_STATUS
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "LoadTreenode: Cache Miss: %s\n", szHid)  );
#endif

		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pST->pRepo,szHid,&pTreenodeAllocated)  );
		pTreenode = pTreenodeAllocated;
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pST->prbTreenodesAllocated,szHid,pTreenodeAllocated)  );
		pTreenodeAllocated = NULL;		// rbtree now owns the thing we just allocated.
	}
	else
	{
#if TRACE_VV2_STATUS
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "LoadTreenode: Cache Hit:  %s\n", szHid)  );
#endif
	}

	*ppTreenode = pTreenode;
	return;

fail:
	SG_TREENODE_NULLFREE(pCtx, pTreenodeAllocated);
}

//////////////////////////////////////////////////////////////////

/**
 * Find the sg_vv2status_od (ROW) for this Object GID from the allocated cache or
 * create one for it.
 *
 * The cache owns this pointer; do not free it directly.
 */
static void _td__load_object_data(SG_context * pCtx,
								  sg_vv2status * pST,
								  const char * szGidObject,
								  sg_vv2status_od ** ppObjectData,
								  SG_bool * pbCreated)
{
	sg_vv2status_od * pODAllocated = NULL;
	sg_vv2status_od * pOD;
	SG_bool bFound;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pST->prbObjectDataAllocated,szGidObject,&bFound,(void **)&pOD)  );
	if (!bFound)
	{
		SG_ERR_CHECK(  sg_vv2__status__od__alloc(pCtx, szGidObject,&pODAllocated)  );
		pOD = pODAllocated;
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pST->prbObjectDataAllocated,szGidObject,pODAllocated)  );
		pODAllocated = NULL;		// rbtree now owns the thing we just allocated
	}

	*ppObjectData = pOD;
	if (pbCreated)
		*pbCreated = !bFound;

	return;

fail:
	SG_ERR_IGNORE(  sg_vv2__status__od__free(pCtx, pODAllocated)  );
}

//////////////////////////////////////////////////////////////////

/**
 * load the cset from the repo and remember the HID of the cset in our tables.
 */
static void _td__load_changeset(SG_context * pCtx, sg_vv2status * pST, const char * szHidCSet, SG_uint32 ndx)
{
	char * pszTreeRootHid = NULL;
	SG_ASSERT_RELEASE_FAIL(  ((ndx == SG_VV2__OD_NDX_ORIG) || (ndx == SG_VV2__OD_NDX_DEST))  );

	// load changeset from disk and put it in our cset cache.

	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pST->pRepo,szHidCSet,&pszTreeRootHid)  );
	
	// remember the changeset's root tree node.
	SG_ERR_CHECK(  SG_strcpy(pCtx, 
							pST->aCSetColumns[ndx].bufHidTreeNodeRoot,
							SG_NrElements(pST->aCSetColumns[ndx].bufHidTreeNodeRoot),
							pszTreeRootHid)  );

	// remember the CSet's HID in this column.
	SG_ERR_CHECK(  SG_strcpy(pCtx,
							 pST->aCSetColumns[ndx].bufHidCSet,
							 SG_NrElements(pST->aCSetColumns[ndx].bufHidCSet),
							 szHidCSet)  );

fail:
	SG_NULLFREE(pCtx, pszTreeRootHid);
	return;
}

/**
 * Process the changeset associated with the given NDX.
 * Get the super-root treenode and queue up the
 * user's actual "@" root directory for processing.
 */
static void _td__process_changeset(SG_context * pCtx, sg_vv2status * pST, SG_uint32 ndx)
{
	const SG_treenode * pTreenodeSuperRoot;
	const char * szGidObjectRoot = NULL;
	const char * szEntrynameRoot;
	const SG_treenode_entry * pTreenodeEntryRoot;
	SG_treenode_entry_type tneTypeRoot;
	SG_uint32 nrEntries;
	sg_vv2status_od * pODRoot;

	// lookup the HID of the super-root treenode from the changeset.

	SG_ASSERT_RELEASE_FAIL(  ((ndx == SG_VV2__OD_NDX_ORIG) || (ndx == SG_VV2__OD_NDX_DEST))  );

	if (!pST->aCSetColumns[ndx].bufHidTreeNodeRoot || !*pST->aCSetColumns[ndx].bufHidTreeNodeRoot)
		SG_ERR_THROW(  SG_ERR_MALFORMED_SUPERROOT_TREENODE  );

	// use the super-root treenode HID to load the treenode from disk
	// and add to the treenode cache (or just fetch it from the cache
	// if already present).

	SG_ERR_CHECK(  _td__load_treenode(pCtx, pST,pST->aCSetColumns[ndx].bufHidTreeNodeRoot,&pTreenodeSuperRoot)  );

	// verify we have a well-formed super-root.  that is, the super-root treenode
	// should have exactly 1 treenode-entry -- the "@" directory.

	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenodeSuperRoot,&nrEntries)  );
	if (nrEntries != 1)
		SG_ERR_THROW(  SG_ERR_MALFORMED_SUPERROOT_TREENODE  );
	SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx,
													   pTreenodeSuperRoot,0,
													   &szGidObjectRoot,&pTreenodeEntryRoot)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTreenodeEntryRoot,&szEntrynameRoot)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTreenodeEntryRoot,&tneTypeRoot)  );
	if ( (strcmp(szEntrynameRoot,"@") != 0) || (tneTypeRoot != SG_TREENODEENTRY_TYPE_DIRECTORY) )
		SG_ERR_THROW(  SG_ERR_MALFORMED_SUPERROOT_TREENODE  );

	// add/lookup row in the Object GID map.
	// and bind the column to our instance data.
	// since the user's root doesn't have a parent, we set the Parent Object GID to NULL.

	SG_ERR_CHECK(  _td__load_object_data(pCtx, pST,szGidObjectRoot,&pODRoot,NULL)  );
	SG_ERR_CHECK(  _td__set_od_column(pCtx, pST,pODRoot,ndx,pTreenodeEntryRoot,NULL)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * sg_vv2status_od is a ROW containing one or more sg_vv2status_odi (instances) that each
 * contain a version of a sub-directory.  Here we are asked to load
 * the treenode for the sub-directory (using the treenode-entry blob HID)
 * and scan the contents.
 *
 * During the (directory) scan, we add ROWS to the sg_vv2status_od rows for each
 * file/sub-directory contained within the given sub-directory.
 *
 * We read the treenode entry from the repo, so we can only be called
 * on SG_VV2__OD_NDX_ORIG and SG_VV2__OD_NDX_DEST.
 *
 * We do NOT go recursive.  Our job is to simply process the contents
 * of this directory.
 *
 * We only do this for one instance; only the caller knows if the peer
 * should be scanned too.
 */
static void _td__scan_folder_for_column(SG_context * pCtx,
										sg_vv2status * pST,
										sg_vv2status_od * pOD,
										SG_uint32 ndx)
{
	const SG_treenode * pTreenode;
	SG_uint32 kEntry, nrEntries;
	const char * szGidObject_child_k = NULL;
	const SG_treenode_entry * pTreenodeEntry_child_k;
	const char * pszHidContent;
	sg_vv2status_od * pOD_child_k;

	SG_ASSERT_RELEASE_FAIL(  ((ndx == SG_VV2__OD_NDX_ORIG) || (ndx == SG_VV2__OD_NDX_DEST))  );

	if (pOD->apInst[ndx]->typeInst != SG_VV2__OD_TYPE_UNSCANNED_FOLDER)
		return;

	pOD->apInst[ndx]->typeInst = SG_VV2__OD_TYPE_SCANNED_FOLDER;

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pOD->apInst[ndx]->pTNE, &pszHidContent)  );
	SG_ERR_CHECK(  _td__load_treenode(pCtx, pST, pszHidContent, &pTreenode)  );

	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode,&nrEntries)  );
	for (kEntry=0; kEntry<nrEntries; kEntry++)
	{
		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx,
														   pTreenode,
														   kEntry,
														   &szGidObject_child_k,
														   &pTreenodeEntry_child_k)  );
		SG_ERR_CHECK(  _td__load_object_data(pCtx, pST,szGidObject_child_k,&pOD_child_k,NULL)  );
		SG_ERR_CHECK(  _td__set_od_column(pCtx, pST,pOD_child_k,ndx,pTreenodeEntry_child_k,pOD)  );

	}

fail:
	return;
}

/**
 * The work-queue contains folders that we need to visit.  We remove the first
 * entry in the queue and scan single/pair of directories and decide what to do
 * with each of the entries within them.  This may cause new items to be added
 * to the work-queue (for sub-directories), so we can't use a normal iterator.
 *
 * By "scan", I mean load the treeenode from the REPO and look at each of the
 * entries.  We do not have anything to do with the WD or pending-tree.
 *
 * We loop until we reach a kind of closure where we have visited as much of
 * the 2 versions of the tree as we need to.
 */
static void _td__compute_closure(SG_context * pCtx, sg_vv2status * pST)
{
	sg_vv2status_od * pOD = NULL;
	SG_bool bFound = SG_FALSE;

	while (1)
	{
		// remove the first item in the work-queue.  since this is depth-in-the-tree
		// sorted, this will be the shallowest thing that we haven't completely
		// scanned yet.
		//
		// we can have 1 or 2 columns populated.  (and because the user can
		// move a folder deeper in the tree) we don't know if a peerless one is
		// a temporary thing or not.)

		SG_ERR_CHECK_RETURN(  sg_vv2__status__remove_first_from_work_queue(pCtx,pST,&bFound,&pOD)  );
		if (!bFound)
			return;

		if (pOD->apInst[SG_VV2__OD_NDX_ORIG])
			SG_ERR_CHECK_RETURN(  _td__scan_folder_for_column(pCtx,pST,pOD,SG_VV2__OD_NDX_ORIG)  );

		if (pOD->apInst[SG_VV2__OD_NDX_DEST])
			SG_ERR_CHECK_RETURN(  _td__scan_folder_for_column(pCtx,pST,pOD,SG_VV2__OD_NDX_DEST)  );
	}
}

//////////////////////////////////////////////////////////////////

void sg_vv2__status__compare_cset_vs_cset(SG_context * pCtx,
										  sg_vv2status * pST,
										  const char * pszHidCSet_0,
										  const char * pszHidCSet_1)
{
	SG_ERR_CHECK(  _td__load_changeset(pCtx, pST, pszHidCSet_0, SG_VV2__OD_NDX_ORIG)  );
	SG_ERR_CHECK(  _td__load_changeset(pCtx, pST, pszHidCSet_1, SG_VV2__OD_NDX_DEST)  );
	SG_ERR_CHECK(  _td__process_changeset(pCtx, pST, SG_VV2__OD_NDX_ORIG)  );
	SG_ERR_CHECK(  _td__process_changeset(pCtx, pST, SG_VV2__OD_NDX_DEST)  );
	SG_ERR_CHECK(  _td__compute_closure(pCtx, pST)  );

fail:
	return;
}
