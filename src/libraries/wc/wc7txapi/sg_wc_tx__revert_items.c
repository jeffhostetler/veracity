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
 * @file sg_wc_tx__revert_item.c
 *
 * @details Handle REVERT of individual item(s).  The goal here
 * is to restore one or more items to the state that they had in
 * the baseline.  We can either do a complete revert of an item
 * or partial revert for an item that has multiple changes.
 * For example, if you have a file that has been EDITED, MOVED,
 * and RENAMED, you might want to say to restore all three
 * aspects of the item or you might want to just discard the
 * edits.
 *
 * THIS IS A COMPLETELY DIFFERENT CODE PATH THAN REVERT-ALL
 * because REVERT-ALL has to deal with canceling any pending
 * merges and it has to compute the proper ordering of operations
 * to get everything back as it was.  Whereas a REVERT-ITEM can
 * just worry about doing a complete/partial UNDO on the named
 * item(s) and it is free to complain if the undo would cause
 * conflicts/collisions and stop.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

struct _ri_data
{
	SG_wc_tx *				pWcTx;
	SG_varray *				pvaStatus;
	SG_varray *				pvaParked;
	SG_varray *				pvaDeferred;
	SG_wc_status_flags		flagMaskCleaned;
	SG_wc_status_flags		flagMaskCleaned_Types;
	SG_wc_status_flags		flagMaskCleaned_Dirty;
	SG_bool					bNoBackups;
};

//////////////////////////////////////////////////////////////////

static void _ri__defer_undo_add_directory(SG_context * pCtx,
										  struct _ri_data * pData,
										  const char * pszGid,
										  const char * pszGidRepoPath,
										  const char * pszLiveRepoPathForErrorMessageOnly,
										  SG_bool bKeep)
{
	SG_vhash * pvhDeferred = NULL;	// we do not own this

	if (!pData->pvaDeferred)
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pData->pvaDeferred)  );

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pData->pvaDeferred, &pvhDeferred)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDeferred, "gid",              pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDeferred, "repo_path",        pszGidRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDeferred, "live_repo_path",   pszLiveRepoPathForErrorMessageOnly)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhDeferred, "keep", bKeep)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _ri__remember_park(SG_context * pCtx,
							   struct _ri_data * pData,
							   const char * pszGid,
							   const char * pszGidRepoPath,
							   const char * pszGidRepoPath_Parent,
							   const char * pszEntryname,
							   SG_wc_status_flags xu_mask)
{
	SG_vhash * pvhParked = NULL;	// we do not own this

	if (!pData->pvaParked)
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pData->pvaParked)  );

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pData->pvaParked, &pvhParked)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhParked, "gid",              pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhParked, "repo_path",        pszGidRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhParked, "repo_path_parent", pszGidRepoPath_Parent)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhParked, "name",             pszEntryname)  );
	if (xu_mask)
		SG_ERR_CHECK(  SG_vhash__add__int64( pCtx, pvhParked, "xu_mask",          xu_mask)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * We want to RESTORE something that was DELETED.
 *
 * If it was MOVED/RENAMED, we can either put it where
 * it was at the time of the DELETE or we can put it
 * where it was in the baseline, depending on the flags
 * passed in.  Since the actual __undo_delete() takes
 * the target location, we don't have to restore it and
 * then move/rename it.
 *
 * If the desired target location is in use or doesn't
 * exist yet, we "park" it and schedule an "unpark" later.
 *
 */
static void _ri__try_undo_delete(SG_context * pCtx,
								 struct _ri_data * pData,
								 SG_vhash * pvhItem,
								 SG_wc_status_flags bitsMatched)
{
	SG_uint64 uiAliasGid;
	SG_uint64 uiAliasGid_Parent;
	sg_wc_liveview_item * pLVI = NULL;				// we do not own this
	const char * pszEntryname = NULL;				// we do not own this
	SG_string * pStringGidRepoPath = NULL;
	SG_string * pStringGidRepoPath_Parent = NULL;
	const char * pszGid = NULL;
	SG_bool bKnown;
	SG_wc_status_flags xu_mask = SG_WC_STATUS_FLAGS__ZERO;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pStringGidRepoPath)  );

	SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvhItem, "alias", &uiAliasGid)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pWcTx, uiAliasGid, &bKnown, &pLVI)  );
	SG_ASSERT( (bKnown && pLVI) );

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__RENAMED)
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntryname)  );
	else
		pszEntryname = SG_string__sz(pLVI->pStringEntryname);

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__MOVED)
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_parent_alias(pCtx, pLVI, &uiAliasGid_Parent)  );
	else
		uiAliasGid_Parent = pLVI->pLiveViewDir->uiAliasGidDir;

	SG_ERR_CHECK(  sg_wc_db__path__alias_to_gid_repopath(pCtx, pData->pWcTx->pDb, uiAliasGid_Parent,
														 &pStringGidRepoPath_Parent)  );

	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
	{
		// A revert-item on an unresolved item, should implicitly
		// resolve the various choices.

		xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
		if (bitsMatched & SG_WC_STATUS_FLAGS__S__RENAMED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME);
		if (bitsMatched & SG_WC_STATUS_FLAGS__S__MOVED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION);
		if (bitsMatched & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__CONTENTS);
		if (bitsMatched & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES);
	}

	// We pass NULL for pArgs in __undo_delete() so that we get
	// the {hid,attr} values that this item had in the baseline,
	// so we don't need to look at any of the __C__ bits.
	//
	// We only pass xu_mask for the initial attempt to restore it.
	// If that fails and we have to park it, we park (move/rename)
	// if WITHOUT resolving it; later, during the unpark step, we
	// will deal with the implicit resolve using the xu_mask we
	// computed here.

	SG_wc_tx__undo_delete(pCtx, pData->pWcTx,
						  SG_string__sz(pStringGidRepoPath),
						  SG_string__sz(pStringGidRepoPath_Parent),
						  pszEntryname,
						  NULL,
						  xu_mask);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
#if TRACE_WC_REVERT
		const char * pszInfo = NULL;
		SG_context__err_get_description(pCtx, &pszInfo);
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RevertItems:TryUndoDelete: '%s' into ('%s','%s'): [xu %02x] [err '%s']\n",
								   SG_string__sz(pStringGidRepoPath),
								   SG_string__sz(pStringGidRepoPath_Parent),
								   pszEntryname,
								   ((SG_uint32)((xu_mask & SG_WC_STATUS_FLAGS__XU__MASK) >> SGWC__XU_OFFSET)),
								   pszInfo)  );
#endif

		SG_context__err_reset(pCtx);
		SG_ERR_CHECK(  _ri__remember_park(pCtx, pData,
										  pszGid,
										  SG_string__sz(pStringGidRepoPath),
										  SG_string__sz(pStringGidRepoPath_Parent),
										  pszEntryname,
										  xu_mask)  );
		SG_ERR_CHECK(  sg_wc_tx__park__undo_delete(pCtx, pData->pWcTx, pszGid)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath_Parent);
}

/**
 * We want to RESTORE something that is currently LOST.
 *
 * Do for LOST items what the above code does for DELETED items.
 *
 */
static void _ri__try_undo_lost(SG_context * pCtx,
							   struct _ri_data * pData,
							   SG_vhash * pvhItem,
							   SG_wc_status_flags bitsMatched)
{
	SG_uint64 uiAliasGid;
	SG_uint64 uiAliasGid_Parent;
	sg_wc_liveview_item * pLVI = NULL;				// we do not own this
	const char * pszEntryname = NULL;				// we do not own this
	SG_string * pStringGidRepoPath = NULL;
	SG_string * pStringGidRepoPath_Parent = NULL;
	const char * pszGid = NULL;
	SG_bool bKnown;
	SG_wc_status_flags xu_mask = SG_WC_STATUS_FLAGS__ZERO;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pStringGidRepoPath)  );

	SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvhItem, "alias", &uiAliasGid)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pWcTx, uiAliasGid, &bKnown, &pLVI)  );
	SG_ASSERT( (bKnown && pLVI) );

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__RENAMED)
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntryname)  );
	else
		pszEntryname = SG_string__sz(pLVI->pStringEntryname);

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__MOVED)
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_parent_alias(pCtx, pLVI, &uiAliasGid_Parent)  );
	else
		uiAliasGid_Parent = pLVI->pLiveViewDir->uiAliasGidDir;

	SG_ERR_CHECK(  sg_wc_db__path__alias_to_gid_repopath(pCtx, pData->pWcTx->pDb, uiAliasGid_Parent,
														 &pStringGidRepoPath_Parent)  );

	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
	{
		// A revert-item on an unresolved item, should implicitly
		// resolve the various choices.

		xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
		if (bitsMatched & SG_WC_STATUS_FLAGS__S__RENAMED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME);
		if (bitsMatched & SG_WC_STATUS_FLAGS__S__MOVED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION);
		if (bitsMatched & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__CONTENTS);
		if (bitsMatched & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)
			xu_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES);
	}

	// We pass NULL for pArgs in __undo_lost() so that we get
	// the {hid,attr} values that this item had in the baseline,
	// so we don't need to look at any of the __C__ bits.

	SG_wc_tx__undo_lost(pCtx, pData->pWcTx,
						SG_string__sz(pStringGidRepoPath),
						SG_string__sz(pStringGidRepoPath_Parent),
						pszEntryname,
						NULL,
						xu_mask);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
#if TRACE_WC_REVERT
		const char * pszInfo = NULL;
		SG_context__err_get_description(pCtx, &pszInfo);
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RevertItems:TryUndoLost: '%s' into ('%s','%s'): [xu %02x] [err '%s']\n",
								   SG_string__sz(pStringGidRepoPath),
								   SG_string__sz(pStringGidRepoPath_Parent),
								   pszEntryname,
								   ((SG_uint32)((xu_mask & SG_WC_STATUS_FLAGS__XU__MASK) >> SGWC__XU_OFFSET)),
								   pszInfo)  );
#endif

		SG_context__err_reset(pCtx);
		SG_ERR_CHECK(  _ri__remember_park(pCtx, pData,
										  pszGid,
										  SG_string__sz(pStringGidRepoPath),
										  SG_string__sz(pStringGidRepoPath_Parent),
										  pszEntryname,
										  xu_mask)  );
		SG_ERR_CHECK(  sg_wc_tx__park__undo_lost(pCtx, pData->pWcTx, pszGid)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath_Parent);
}

//////////////////////////////////////////////////////////////////

/**
 * We want to UNDO the ADD or ADD-SPECIAL of an item.
 * 
 * Since we are being driven by the results of a STATUS,
 * we want this to be a NONRECURSIVE un-add.  That is, if
 * STATUS told us that we have an ADDED directory, it probably
 * also told us about everything within it -- whether ADDED or
 * MOVED into the directory.  We need to deal with the children
 * individually (per the flagMask) before we deal with the
 * directory (so that if one of the children has a change that
 * we are not reverting, we can abort the revert of the directory
 * if necessary).
 *
 * Also since STATUS probably gave us a top-down view of the
 * tree, we need to be prepared for our initial attempt to remove
 * a directory to fail; we try to defer it until the rest of the
 * items have been processed.
 *
 *
 * For plain ADDED items, we expect to use the __undo_add()
 * portion of the remove-safely code to do the dirty work so
 * that they should become FOUND items.
 *
 * For ADD-SPECIAL items, we expect the remove-safely code to
 * actually remove it from the WD (and leaving an item with
 * both "ADDED-SPECIAL + REMOVED" status).
 * 
 */
static void _ri__try_undo_add(SG_context * pCtx,
							  struct _ri_data * pData,
							  SG_vhash * pvhItem,
							  SG_wc_status_flags bitsMatched)
{
	SG_string * pStringGidRepoPath = NULL;
	const char * pszLiveRepoPathForErrorMessageOnly = NULL;	// see WARNING below
	const char * pszGid = NULL;
	SG_uint64 uiAliasGid;
	sg_wc_liveview_item * pLVI = NULL;				// we do not own this
	SG_bool bKnown;
	SG_bool bKeep;
	SG_bool bForceFiles;

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		bKeep = SG_TRUE;
		bForceFiles = SG_FALSE;
	}
	else if (bitsMatched & (SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED))
	{
		bKeep = SG_FALSE;
		bForceFiles = SG_TRUE;
	}
	else
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pStringGidRepoPath)  );

	SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvhItem, "alias", &uiAliasGid)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pWcTx, uiAliasGid, &bKnown, &pLVI)  );
	SG_ASSERT( (bKnown && pLVI) );

	switch (pLVI->tneType)
	{
	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		// TODO 2012/08/13 Not sure what bForce should be.  Don't want
		// TODO            us to do too much and accidentally delete
		// TODO            an uncontrolled item within the directory.
		SG_wc_tx__remove(pCtx, pData->pWcTx,
						 SG_string__sz(pStringGidRepoPath),
						 SG_TRUE,				// bNonRecursive
						 SG_FALSE,				// bForce
						 pData->bNoBackups,		// bNoBackups
						 bKeep);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
#if TRACE_WC_REVERT
			const char * pszInfo = NULL;
			SG_context__err_get_description(pCtx, &pszInfo);
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "RevertItems:TryUndoAddDirectory: '%s': [err '%s']\n",
									   SG_string__sz(pStringGidRepoPath),
									   pszInfo)  );
#endif
			SG_context__err_reset(pCtx);

			// We could not safely convert this directory from ADDED
			// to FOUND status.  This probably means that the directory
			// still contains controlled items.  It could be that they
			// will be moved-out or un-added when our outer loop gets
			// to them in the pvaStatus, so we need to defer the un-add
			// on this directory until we have processed them.  (Or it
			// could be that the offending child was excluded from the
			// scope of the revert and we need to stop and complain;
			// we can't which without an expensive computation, so we
			// always defer it and deal with it later.)
			//
			// WARNING: We only use pszLiveRepoPath for the throw2()
			// WARNING: if we can't un-add it.  This path is "live" as
			// WARNING: of when we computed the STATUS and probably
			// WARNING: reflects the item as the user saw it when they
			// WARNING: the command; but because of all the IN-TX
			// WARNING: operations performed by our caller, it is no
			// WARNING: guaranteed to be the "actual" live repo-path.
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path",
											 &pszLiveRepoPathForErrorMessageOnly)  );
			SG_ERR_CHECK(  _ri__defer_undo_add_directory(pCtx, pData,
														 pszGid,
														 SG_string__sz(pStringGidRepoPath),
														 pszLiveRepoPathForErrorMessageOnly,
														 bKeep)  );
		}
		break;

	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
	case SG_TREENODEENTRY_TYPE_SYMLINK:
		SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pData->pWcTx,
										SG_string__sz(pStringGidRepoPath),
										SG_TRUE,				// bNonRecursive
										bForceFiles,			// bForce
										pData->bNoBackups,		// bNoBackups
										bKeep)  );
		break;

	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Unknown type. [tne %d] %s",
						 (SG_uint32)pLVI->tneType,
						 SG_string__sz(pStringGidRepoPath))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * We know that the item does not need to be created or destroyed;
 * we just need to undo some of the changes.
 *
 */
static void _ri__try_undo_changes(SG_context * pCtx,
								  struct _ri_data * pData,
								  SG_vhash * pvhItem,
								  SG_wc_status_flags bitsMatched)
{
	SG_uint64 uiAliasGid;
	SG_uint64 uiAliasGid_Parent;
	sg_wc_liveview_item * pLVI = NULL;				// we do not own this
	const char * pszEntryname = NULL;				// we do not own this
	SG_string * pStringGidRepoPath = NULL;
	SG_string * pStringGidRepoPath_Parent = NULL;
	const char * pszHidContent = NULL;
	SG_uint64 attrbits = 0;
	const char * pszGid = NULL;
	SG_bool bKnown;
	SG_wc_status_flags xu1_mask = SG_WC_STATUS_FLAGS__ZERO;
	SG_wc_status_flags xu2_mask = SG_WC_STATUS_FLAGS__ZERO;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pStringGidRepoPath)  );

	SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvhItem, "alias", &uiAliasGid)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pWcTx, uiAliasGid, &bKnown, &pLVI)  );
	SG_ASSERT( (bKnown && pLVI) );

	//////////////////////////////////////////////////////////////////
	// There are 2 independent parts here:
	// [1] {hid,attr}
	// [2] {move,rename}
	//
	// Let's restore [1] before we attempt [2] because [1] is completely
	// self-contained and cannot have a collision/conflict.
	//
	// If [2] fails because of a conflict, we can "park" it for later
	// knowning that [1] has already been handled.
	//////////////////////////////////////////////////////////////////

	// [1] fetch {hid,attr}.  Note that the __overwrite_ routines
	//     take both args, so we only have to explictly deal with
	//     {attr} when the hid doesn't need to be restored.

	if (bitsMatched & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pData->pWcTx, &attrbits)  );
		xu1_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES);
	}
	else
		SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits(pCtx, pLVI, pData->pWcTx, &attrbits)  );

	if (bitsMatched & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
	{
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pData->pWcTx, SG_FALSE, &pszHidContent)  );
		xu1_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__CONTENTS);
		
		if (pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		{
			SG_ERR_CHECK(  SG_wc_tx__overwrite_file_from_repo(pCtx, pData->pWcTx,
															  SG_string__sz(pStringGidRepoPath),
															  pszHidContent, attrbits,
															  (!pData->bNoBackups),
															  xu1_mask)  );
		}
		else if (pLVI->tneType == SG_TREENODEENTRY_TYPE_SYMLINK)
		{
			SG_ERR_CHECK(  SG_wc_tx__overwrite_symlink_from_repo(pCtx, pData->pWcTx,
																 SG_string__sz(pStringGidRepoPath),
																 pszHidContent, attrbits,
																 xu1_mask)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "Unknown type. [tne %d] %s",
							 (SG_uint32)pLVI->tneType,
							 SG_string__sz(pStringGidRepoPath))  );
		}
	}
	else
	{
		if (bitsMatched & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)
			SG_ERR_CHECK(  SG_wc_tx__set_attrbits(pCtx, pData->pWcTx,
												  SG_string__sz(pStringGidRepoPath),
												  attrbits, xu1_mask)  );
	}

	//////////////////////////////////////////////////////////////////

	// [2] {moved,renamed}.  TRY to do the move/rename.  If we get a
	// collision/conflict, park this item.

	if ((bitsMatched & (SG_WC_STATUS_FLAGS__S__RENAMED | SG_WC_STATUS_FLAGS__S__MOVED)) == 0)
		goto done;
	
	if (bitsMatched & SG_WC_STATUS_FLAGS__S__RENAMED)
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntryname)  );
	else
		pszEntryname = SG_string__sz(pLVI->pStringEntryname);

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__MOVED)
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_parent_alias(pCtx, pLVI, &uiAliasGid_Parent)  );
	else
		uiAliasGid_Parent = pLVI->pLiveViewDir->uiAliasGidDir;

	SG_ERR_CHECK(  sg_wc_db__path__alias_to_gid_repopath(pCtx, pData->pWcTx->pDb, uiAliasGid_Parent,
														 &pStringGidRepoPath_Parent)  );

	if (pLVI->pvhIssue && (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__UNRESOLVED))
	{
		// A revert-item on an unresolved item, should implicitly
		// resolve the various choices.  Here we only deal with
		// the existence/name/location choices for the move/rename
		// call below.  (The content/attr choices should have been
		// handled by the overwrite calls.)

		xu2_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
		if (bitsMatched & SG_WC_STATUS_FLAGS__S__RENAMED)
			xu2_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME);
		if (bitsMatched & SG_WC_STATUS_FLAGS__S__MOVED)
			xu2_mask |= (pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION);
	}

	SG_wc_tx__move_rename3(pCtx, pData->pWcTx,
						   SG_string__sz(pStringGidRepoPath),
						   SG_string__sz(pStringGidRepoPath_Parent),
						   pszEntryname,
						   SG_FALSE,
						   xu2_mask);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
#if TRACE_WC_REVERT
		const char * pszInfo = NULL;
		SG_context__err_get_description(pCtx, &pszInfo);
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RevertItems:TryUndoChange: '%s' into ('%s','%s'): [xu %02x] [err '%s']\n",
								   SG_string__sz(pStringGidRepoPath),
								   SG_string__sz(pStringGidRepoPath_Parent),
								   pszEntryname,
								   ((SG_uint32)((xu2_mask & SG_WC_STATUS_FLAGS__XU__MASK) >> SGWC__XU_OFFSET)),
								   pszInfo)  );

#endif
		SG_context__err_reset(pCtx);

		// park it somewhere.  we don't care where since we always
		// refer to it with a gid-domain repo-path.  Then remember
		// that this item needed parking and where we wanted it to
		// be (also by gid-repo-path).
		SG_ERR_CHECK(  _ri__remember_park(pCtx, pData,
										  pszGid,
										  SG_string__sz(pStringGidRepoPath),
										  SG_string__sz(pStringGidRepoPath_Parent),
										  pszEntryname,
										  xu2_mask)  );
		SG_ERR_CHECK(  sg_wc_tx__park__move_rename(pCtx, pData->pWcTx, pszGid)  );
	}

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath_Parent);
}

//////////////////////////////////////////////////////////////////

/**
 * Set up to restore a single dirty item.
 * (Provided it has dirt in the dimension
 * that we want to restore.)
 *
 */
static void _ri__item_k(SG_context * pCtx,
						struct _ri_data * pData,
						SG_vhash * pvhItem)
{
	SG_vhash * pvhItemStatus;	// we do not own this
	SG_wc_status_flags statusFlags;
	SG_wc_status_flags bitsMatched;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );
	SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvhItemStatus, "flags", &statusFlags)  );

#if TRACE_WC_REVERT
	{
		const char * pszRepoPath;
		SG_string * pString_1 = NULL;
		SG_string * pString_2 = NULL;

		// using the "path" from the pvhItem is fine for debug-only purposes.
		SG_ERR_IGNORE(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszRepoPath)  );
		SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags(pCtx, pData->flagMaskCleaned, "Mask", &pString_1)  );
		SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags(pCtx, statusFlags,            "Obs1", &pString_2)  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RevertItems: visiting dirty item: %s\n\t%s\n\t%s\n",
								   pszRepoPath,
								   SG_string__sz(pString_1),
								   SG_string__sz(pString_2))  );
		SG_STRING_NULLFREE(pCtx, pString_1);
		SG_STRING_NULLFREE(pCtx, pString_2);
	}
#endif

	if ((statusFlags & pData->flagMaskCleaned_Types) == 0)
	{
		// we are not restoring items of this type.
		return;
	}
	if ((statusFlags & pData->flagMaskCleaned_Dirty) == 0)
	{
		// we are not restoring any of the categories
		// of dirt that this item has.
		return;
	}

	if ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
		&& ((pData->flagMaskCleaned & SG_WC_STATUS_FLAGS__S__DELETED) == 0))
	{
		// if the item is actually deleted *AND* we are NOT restoring
		// deleted items, then it doesn't matter if the item was moved
		// or renamed before it was deleted, we won't be restoring it
		// so we don't need to worry about UNDO-RENAME, UNDO-MOVE, or
		// bits from any other category of dirt.
		return;
	}
	if ((statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
		&& ((pData->flagMaskCleaned & SG_WC_STATUS_FLAGS__U__LOST) == 0))
	{
		// likewise
		return;
	}
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
	{
		// See W4647.  ADDED-BY-MERGE items are somewhat unique
		// because they were not present in the baseline.  And
		// REVERT-ITEMS is defined as restoring the baseline
		// state of an item (at least currently).
		//
		// So we can either:
		// [] we can delete it or
		// [] leave it untouched.
		// 

		if ((pData->flagMaskCleaned & SG_WC_STATUS_FLAGS__S__MERGE_CREATED) == 0)
		{
			// We are doing a partial-revert-item (such as only
			// RENAMES and the __S__MERGE_CREATED bit was not set).
			// So we leave the item as is.
			//
			// We DO NOT need/bother to look at any of the other
			// bits because they are baseline-relative (and we
			// did a STATUS rather than an MSTATUS so we don't
			// know about renames-relative-to-other, for example).
			return;
		}
		
		// We are doing a full-revert-item (reverting all
		// types of changes), so we want to delete it.
		// 
		// If it is already deleted, we're done.

		if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
			return;

		SG_ERR_CHECK(  _ri__try_undo_add(pCtx, pData, pvhItem,
										 SG_WC_STATUS_FLAGS__S__MERGE_CREATED)  );
		return;
	}
	if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
	{
		// See W6552.  ADDED-BY-UPDATE items are like the ADDED-BY-MERGE
		// items in that they are not present in the update's target cset
		// (but were present in the update source cset).  And a REVERT-ITEM
		// on this item can just delete (subject to the backup rules).

		if ((pData->flagMaskCleaned & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED) == 0)
		{
			// A partial revert-items did not include this item.
			return;
		}

		if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
			return;

		SG_ERR_CHECK(  _ri__try_undo_add(pCtx, pData, pvhItem,
										 SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)  );
		return;
	}

	// The set of categories of dirt that we want to restore.
	bitsMatched = (statusFlags & pData->flagMaskCleaned_Dirty);
	SG_ASSERT(  (bitsMatched != 0)  );

	// We have a bit of a chicken-and-egg problem here.
	// If the restore needs to undelete things, it needs
	// to work top-down.  If the retore needs to unadd
	// things, it needs to work bottom-up.  If there are
	// items within an added/deleted directory, we need
	// to handle them before/after we deal with the
	// directory so that we don't throw parent-not-found
	// or directory-not-empty type errors.
	//
	// And since this is (probably) a PARTIAL REVERT (unless
	// they said "@/") it is possible that item might need
	// to be created in a directory that isn't present or
	// isn't going to be present.
	//
	// For example, if you do:
	//     vv move   foo/bar xyz
	//     vv remove foo
	//     vv revert xyz/bar
	// we want to undo the move of bar, but the original
	// parent directory "foo" isn't part of the set of
	// things being reverted.
	//
	// Since this is a partial revert, we have to let
	// this throw and (maybe) ask them to include "foo"
	// in the revert set.
	//
	// (And if they do 'vv revert xyz/bar foo' we need
	// to juggle the ordering so that foo exists before
	// we want to un-move "bar". (sigh))
	//
	// That is, even with the QUEUE/APPLY machinery,
	// we can't un-move "bar" until we un-delete "foo".
	// 
	// So we try to QUEUE what we need to do to this item
	// and if we get a conflict, we park it in a temporary
	// location and/or defer it and try it again later
	// (presumably after we have deleted or moved the
	// other thing out of the way).

	if (bitsMatched & SG_WC_STATUS_FLAGS__S__DELETED)
	{
		// ADDED+DELETED shouldn't be possible (since they are
		// different from special-added items).
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__S__ADDED) == 0)  );
		SG_ERR_CHECK(  _ri__try_undo_delete(pCtx, pData, pvhItem, bitsMatched)  );
	}
	else if (bitsMatched & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		// ADDED items should become FOUND items after the REVERT.
		// However, if it was ADDED+LOST, it should just go away completely.
		SG_ERR_CHECK(  _ri__try_undo_add(pCtx, pData, pvhItem, bitsMatched)  );
	}
	else if (bitsMatched & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _ri__try_undo_lost(pCtx, pData, pvhItem, bitsMatched)  );
	}
	else
	{
		SG_ERR_CHECK(  _ri__try_undo_changes(pCtx, pData, pvhItem, bitsMatched)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Make 1 trip thru the set of deferred directories
 * and try to un-add each.  This will fail for non-leaf
 * items, but we keep going as a complete trip should
 * touch each leaf (and potentially making the parent
 * directory of each a leaf in the next pass).
 *
 * Yes, this is very crude/inefficient from a big-O()
 * point of view, but 'n' should be small.
 *
 */
static void _ri__pdd_pass(SG_context * pCtx,
						  struct _ri_data * pData,
						  SG_uint32 * pNrProcessed)
{
	SG_uint32 k, count;
	SG_uint32 nrProcessed = 0;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaDeferred, &count)  );
#if TRACE_WC_REVERT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "RevertItems:PDD:Pass [starting count %d]:\n",
							   count)  );
#endif

	k = 0;
	while ((count > 0) && (k < count))
	{
		SG_vhash * pvh_k;				// we do not own this
		const char * pszGidRepoPath;	// we do not own this
		SG_bool bKeep;
		
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaDeferred, k, &pvh_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_k, "repo_path", &pszGidRepoPath)  );
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_k, "keep", &bKeep)  );

		SG_wc_tx__remove(pCtx, pData->pWcTx, pszGidRepoPath,
						 SG_TRUE,				// bNonRecursive
						 SG_FALSE,				// bForce
						 pData->bNoBackups,		// bNoBackups
						 bKeep);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
#if TRACE_WC_REVERT
			const char * pszInfo = NULL;
			SG_context__err_get_description(pCtx, &pszInfo);
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "RevertItems:PDD:Pass: Error on '%s': [err '%s']\n",
									   pszGidRepoPath,
									   pszInfo)  );
#endif
			SG_context__err_reset(pCtx);
			k++;
		}
		else
		{
#if TRACE_WC_REVERT
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "RevertItems:PDD:Pass: Processed '%s'\n",
									   pszGidRepoPath)  );
#endif
			SG_ERR_CHECK(  SG_varray__remove(pCtx, pData->pvaDeferred, k)  );
			count--;
			nrProcessed++;
		}
	}

	*pNrProcessed = nrProcessed;

fail:
	return;
}

/**
 * Try to do the un-add on the set of directories
 * that we had to defer.  Because the un-add of each
 * individual directory will complain if it isn't empty
 * *and* we don't have a good ordering *and* we don't
 * know whether the error is due to something that is
 * later in our set, we iterate until we get them all
 * or until we get a hard error.
 *
 * For example.  Consider:
 *     vv add bbb bbb/ccc bbb/ccc/ddd
 *     vv revert bbb
 *
 * One could argue that we could just reverse-sort the
 * set (by live-repo-path) before we begin so that we'd
 * always see 'ddd' before 'ccc' and 'bbb', but i'm not
 * sure that that really helps if there are messy moves/renames
 * that are also being undone.  That is, I think we need
 * this anyway.
 *
 */
static void _ri__process_deferred_directories(SG_context * pCtx,
											  struct _ri_data * pData)
{
	SG_uint32 nrProcessed = 0;
	SG_uint32 count = 0;

	SG_ERR_CHECK(  _ri__pdd_pass(pCtx, pData, &nrProcessed)  );
	while (nrProcessed > 0)
		SG_ERR_CHECK(  _ri__pdd_pass(pCtx, pData, &nrProcessed)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaDeferred, &count)  );
	if (count > 0)
	{
		SG_vhash * pvh_k;				// we do not own this
		const char * pszLiveRepoPathForErrorMessageOnly;	// we do not own this
		
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaDeferred, 0, &pvh_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_k, "live_repo_path",
										 &pszLiveRepoPathForErrorMessageOnly)  );

		// TODO 2012/08/13 I'd like to make a better error message here.
		// TODO            Perhaps also list the first item still in the
		// TODO            directory.
		SG_ERR_THROW2(  SG_ERR_DIR_NOT_EMPTY,
						(pCtx, "%s",
						 pszLiveRepoPathForErrorMessageOnly)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _ri__unpark_k(SG_context * pCtx,
						  struct _ri_data * pData,
						  SG_vhash * pvhParked)
{
	const char * pszGidRepoPath;
	const char * pszGidRepoPath_Parent;
	const char * pszEntryname;
	SG_wc_status_flags xu_mask = SG_WC_STATUS_FLAGS__ZERO;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhParked, "repo_path",        &pszGidRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhParked, "repo_path_parent", &pszGidRepoPath_Parent)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhParked, "name",             &pszEntryname)  );
	SG_ERR_CHECK(  SG_vhash__check__uint64(pCtx, pvhParked, "xu_mask", (SG_uint64 *)&xu_mask)  );

#if TRACE_WC_REVERT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "RevertItems:Unparking: '%s' into ('%s','%s') [xu %02x]\n",
							   pszGidRepoPath,
							   pszGidRepoPath_Parent,
							   pszEntryname,
							   ((SG_uint32)((xu_mask & SG_WC_STATUS_FLAGS__XU__MASK) >> SGWC__XU_OFFSET)))  );
#endif

	SG_wc_tx__move_rename3(pCtx, pData->pWcTx,
						   pszGidRepoPath,
						   pszGidRepoPath_Parent,
						   pszEntryname,
						   SG_TRUE,
						   xu_mask);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
#if TRACE_WC_REVERT
		{
			const char * pszInfo = NULL;
			SG_context__err_get_description(pCtx, &pszInfo);
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "RevertItems:Unpark Error on '%s' [xu %02x] [err '%s']\n",
									   pszEntryname,
									   ((SG_uint32)((xu_mask & SG_WC_STATUS_FLAGS__XU__MASK) >> SGWC__XU_OFFSET)),
									   pszInfo)  );
		}
#endif

		// Relabel the error when we can so it looks like
		// an "item was in the way" message rather than
		// one of the low-level steps (that the user wouldn't
		// know about).

		if (SG_context__err_equals(   pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS)
			|| SG_context__err_equals(pCtx, SG_ERR_WC_PORT_FLAGS)
			// TODO 2012/08/13 Other cases?
			)
		{
			SG_context__err_reset(pCtx);
			SG_ERR_THROW2(  SG_ERR_WC_ITEM_BLOCKS_REVERT,
							(pCtx, "%s", pszEntryname)  );
		}
		else if (SG_context__err_equals(   pCtx, SG_ERR_WC__MOVE_DESTDIR_NOT_FOUND)
				 || SG_context__err_equals(pCtx, SG_ERR_WC__MOVE_DESTDIR_UNCONTROLLED)
				 || SG_context__err_equals(pCtx, SG_ERR_WC__MOVE_DESTDIR_PENDING_DELETE))
		{
			SG_context__err_reset(pCtx);
			SG_ERR_THROW2(  SG_ERR_WC__REVERT_BLOCKED_BY_DESTDIR,
							(pCtx, "%s", pszEntryname)  );
		}
		else
		{
			SG_ERR_RETHROW;
		}
	}

fail:
	return;
}

/**
 * Try to "unpark" each item that we "parked".
 * Parking helps avoid order-dependencies, but
 * still can't prevent a hard error.
 *
 */
static void _ri__unpark(SG_context * pCtx,
						struct _ri_data * pData)
{
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaParked, &count)  );
	for (k=0; k<count; k++)
	{
		SG_vhash * pvhParked_k;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaParked, k, &pvhParked_k)  );
		SG_ERR_CHECK(  _ri__unpark_k(pCtx, pData, pvhParked_k)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _ri__main(SG_context * pCtx,
					  struct _ri_data * pData)
{
	SG_uint32 k, count;

	//////////////////////////////////////////////////////////////////
	// WARNING: The REVERT-ITEMS command should really just
	// take 1 item and maybe not take a RECURSIVE option so
	// that it can just undo one item's changes and quit.
	// 
	// But it is nice to be able to undo all of the edits in
	// a particular sub-directory at once.  However, things
	// get messy when they want to undo all of the MOVES/RENAMES
	// within a directory.  We ARE NOT going to do the full
	// collision/conflict/park tricks for these.  Either they
	// can do one at a time or use the --all option, so I'm
	// not going to fight very hard here.  But I will do it
	// depth-first so we might get lucky.
	//
	// Things get weird if they un-move/un-rename and give
	// 2 or 3 args on the command line.  Do each of them refer
	// to the tree as of the beginning of the command or is the
	// 2nd one relative to the shape of the tree after the first
	// is reverted?
	//
	// TODO 2012/08/06 I'm going to assume the former.
	//////////////////////////////////////////////////////////////////
	
	// The STATUS call sorted the items by repo-path.
	// TODO 2012/08/13 Not sure it matters how/if we sort them.

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaStatus, &count)  );
	for (k=0; k<count; k++)
	{
		SG_vhash * pvhItem_k;			// we do not own this

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaStatus, k, &pvhItem_k)  );
		SG_ERR_CHECK(  _ri__item_k(pCtx, pData, pvhItem_k)  );
	}

	if (pData->pvaDeferred)
	{
		SG_ERR_CHECK(  _ri__process_deferred_directories(pCtx, pData)  );
	}

	if (pData->pvaParked)
	{
		SG_ERR_CHECK(  _ri__unpark(pCtx, pData)  );
	}

fail:
	return;
}
						
//////////////////////////////////////////////////////////////////

/**
 * Strip out bits from the given flagMask that
 * don't make any sense for a revert.
 *
 */
static void _ri__set_flag_mask(SG_context * pCtx,
							   struct _ri_data * pData,
							   SG_wc_status_flags flagMask)
{
	SG_wc_status_flags mask = SG_WC_STATUS_FLAGS__ZERO;
	SG_wc_status_flags mask_types;
	SG_wc_status_flags mask_dirty;

	mask  = flagMask;

	// We DO NOT include the __A__, __E__, __M__, or __X__ bits.
	// We DO NOT include the __R__ bits.

	mask &= (SG_WC_STATUS_FLAGS__T__MASK
			 |SG_WC_STATUS_FLAGS__U__MASK
			 |SG_WC_STATUS_FLAGS__S__MASK
			 |SG_WC_STATUS_FLAGS__C__MASK);

	mask &= ~(SG_WC_STATUS_FLAGS__T__BOGUS
			  |SG_WC_STATUS_FLAGS__U__FOUND
			  |SG_WC_STATUS_FLAGS__U__IGNORED);

	mask_types = (mask & SG_WC_STATUS_FLAGS__T__MASK);
	mask_dirty = (mask & ~SG_WC_STATUS_FLAGS__T__MASK);
	
	if (mask_types == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "The RevertItem mask will not match any types.")  );

	if (mask_dirty == SG_WC_STATUS_FLAGS__ZERO)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "The RevertItems mask will not match any changes.")  );

	pData->flagMaskCleaned = mask;
	pData->flagMaskCleaned_Types = mask_types;
	pData->flagMaskCleaned_Dirty = mask_dirty;
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__revert_item(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput,
							SG_uint32 depth,
							SG_wc_status_flags flagMask,
							SG_bool bNoBackups)
{
	struct _ri_data data;
	
	data.pWcTx = pWcTx;
	data.pvaStatus = NULL;
	data.pvaParked = NULL;
	data.pvaDeferred = NULL;
	data.flagMaskCleaned = 0;
	data.flagMaskCleaned_Types = 0;
	data.flagMaskCleaned_Dirty = 0;
	data.bNoBackups = bNoBackups;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );

	SG_ERR_CHECK(  _ri__set_flag_mask(pCtx, &data, flagMask)  );

	// Unlike ADDREMOVE, we DO NOT want to assume "@/" when
	// no inputs are given.  (I think.)  That is, a 'vv revert'
	// and 'vv revert @' are different than 'vv revert --all'
	// because of the need to clean up the parents.

	SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pWcTx, pszInput, depth,
									SG_FALSE, SG_TRUE, SG_FALSE,
									SG_FALSE, SG_FALSE, SG_FALSE,
									&data.pvaStatus, NULL)  );
	SG_ERR_CHECK(  _ri__main(pCtx, &data)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatus);
	SG_VARRAY_NULLFREE(pCtx, data.pvaParked);
	SG_VARRAY_NULLFREE(pCtx, data.pvaDeferred);
}

void SG_wc_tx__revert_items__stringarray(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 const SG_stringarray * psaInputs,
										 SG_uint32 depth,
										 SG_wc_status_flags flagMask,
										 SG_bool bNoBackups)
{
	struct _ri_data data;
	SG_uint32 count = 0;

	data.pWcTx = pWcTx;
	data.pvaStatus = NULL;
	data.pvaParked = NULL;
	data.pvaDeferred = NULL;
	data.flagMaskCleaned = 0;
	data.flagMaskCleaned_Types = 0;
	data.flagMaskCleaned_Dirty = 0;
	data.bNoBackups = bNoBackups;

	SG_NULLARGCHECK_RETURN( pWcTx );

	// Unlike ADDREMOVE, we DO NOT want to assume "@/" when
	// no inputs are given.  (I think.)  That is, a 'vv revert'
	// and 'vv revert @/' (aka "cd <root>; vv revert .") are
	// different than 'vv revert --all'
	// because of the need to clean up the parents.

	if (psaInputs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
	if (count == 0)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "RevertItems requires at least one item.")  );

	SG_ERR_CHECK(  _ri__set_flag_mask(pCtx, &data, flagMask)  );
	SG_ERR_CHECK(  SG_wc_tx__status__stringarray(pCtx, pWcTx, psaInputs, depth,
												 SG_FALSE, SG_TRUE, SG_FALSE,
												 SG_FALSE, SG_FALSE, SG_FALSE,
												 &data.pvaStatus, NULL)  );

	SG_ERR_CHECK(  _ri__main(pCtx, &data)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatus);
	SG_VARRAY_NULLFREE(pCtx, data.pvaParked);
	SG_VARRAY_NULLFREE(pCtx, data.pvaDeferred);
}
