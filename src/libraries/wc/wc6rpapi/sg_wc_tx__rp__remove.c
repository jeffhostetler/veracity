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
 * @file sg_wc_tx__rp__remove.c
 *
 * @details Handle details of a 'vv remove'
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _dive(SG_context * pCtx,
				  SG_wc_tx * pWcTx,
				  sg_wc_liveview_item * pLVI,
				  SG_uint32 level,
				  SG_bool bForce,
				  SG_bool bNoBackups,
				  SG_bool bKeepInWC,
				  SG_bool bFlattenAddSpecialPlusRemove,
				  SG_uint32 * pnrImmediateChildrenKept);

static SG_rbtree_ui64_foreach_callback _dive_cb;

struct _dive_data
{
	SG_wc_tx *			pWcTx;
	SG_uint32			level;
	SG_bool				bForce;
	SG_bool				bNoBackups;
	SG_bool				bKeepInWC;
	SG_bool				bFlattenAddSpecialPlusRemove;

	SG_uint32			nrImmediateChildrenKept;
};

//////////////////////////////////////////////////////////////////

static void _can_do_nonrecursive_delete(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI,
										SG_bool bForce,
										SG_bool bKeepInWC,
										SG_bool bUndoAdd);

static SG_rbtree_ui64_foreach_callback _can_do_nonrecursive_delete_cb;

struct _nr_data
{
	SG_wc_tx *				pWcTx;
	sg_wc_liveview_item *	pLVI_Parent;
	SG_bool					bForce;
	SG_bool					bKeepInWC;
	SG_bool					bUndoAdd;
};

//////////////////////////////////////////////////////////////////

static void _handle_undo_added(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   sg_wc_liveview_item * pLVI,
							   SG_bool bForce,
							   SG_wc_status_flags statusFlags,
							   SG_bool * pbKept)
{
	// Treat the DELETE of an ADDED item as an "undo add"
	// rather than a real delete.  Here we just indicate
	// that we should not delete the actual item from disk
	// (regardless of whether it has been modified or not).
	// The point (for files and symlinks at least) is that
	// it has never been committed, so this may be the only
	// copy.
	//
	// Allow the DELETE of an ADDED+LOST item as an "undo add"
	// and don't worry about the item on disk (since it is
	// already missing).
	// 
	// If they say --force, we go ahead and delete it.
	// TODO 2011/10/27 I think that '--force' is too strong
	// TODO            and could be cut up into several types
	// TODO            force.

	const char * pszDisposition;

	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED) == 0)  );	// not possible
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED) == 0)  );	// not possible
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__R__RESERVED) == 0)  );			// not possible
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__FOUND) == 0)  );			// not possible
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED) == 0)  );		// not possible
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE) == 0)  );		// not possible

	if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
		pszDisposition = "noop";
	else if (bForce)
		pszDisposition = "delete";
	else
		pszDisposition = "keep";

	SG_ERR_CHECK(  sg_wc_tx__queue__undo_add(pCtx, pWcTx, pLVI, pszDisposition,
											 pbKept)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

struct _handle_undo_added_directory_data
{
	SG_wc_tx * pWcTx;
	sg_wc_liveview_item * pLVI_Dir;
};

static SG_rbtree_ui64_foreach_callback _handle_undo_added_directory_cb;

static void _handle_undo_added_directory_cb(SG_context * pCtx,
											SG_uint64 uiAliasGid,
											void * pVoid_LVI,
											void * pVoid_Data)
{
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;
	struct _handle_undo_added_directory_data * pData = (struct _handle_undo_added_directory_data *)pVoid_Data;
	SG_wc_status_flags statusFlags;
	SG_bool bNoIgnores;
	SG_bool bNoTSC;

	SG_UNUSED( uiAliasGid );

	// Compute the status for this item by itself; it does not dive (so
	// it does not look at stuff within (and besides we can't compute
	// whether the directory content is modified until we try a commit)).
	bNoIgnores = SG_FALSE;		// we want to be able to distinguish between IGNORED and FOUND.
	bNoTSC = SG_FALSE;			// we trust the TSC to have valid info.
	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pData->pWcTx, pLVI,
												bNoIgnores, bNoTSC,
												&statusFlags)  );
#if TRACE_WC_TX_REMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "_handle_undo_added_directory_cb: considering child [status 0x%08x][scan_flags_live 0x%08x] %s\n",
							   (SG_uint32)statusFlags,
							   pLVI->scan_flags_Live,
							   SG_string__sz(pLVI->pStringEntryname))  );
#endif

	if (statusFlags & SG_WC_STATUS_FLAGS__S__MOVED)
	{
		// This child item was *MOVED-INTO* the directory that
		// we are trying to UNADD.
		//
		// TODO 2012/02/08 An alternative to consider is to go ahead
		// TODO            and let this child be deleted, but update
		// TODO            the parent-gid field in tbl_PC so that it
		// TODO            looks like this item was still in the baseline
		// TODO            parent directory when it was deleted.
		// TODO
		// TODO            See st_undo_added__P00015_01_b.js

		SG_ERR_THROW2(  SG_ERR_CANNOT_REMOVE_SAFELY,
						(pCtx, "'%s' (Cannot UNADD the directory because '%s' was moved into it.)",
						 SG_string__sz(pData->pLVI_Dir->pStringEntryname),
						 SG_string__sz(pLVI->pStringEntryname))  );
	}

fail:
	return;
}

/**
 * See if we can UNADD this directory.  We need this
 * to throw if there is any data in tbl_PC for this
 * directory that we can't get rid of.
 *
 * That is, the unadd of a directory will delete the
 * tbl_PC (add-directory) row for this directory. (as
 * opposed to a regular delete of a directory -- which
 * keeps the tbl_PC row and sets the delete-bit.)
 *
 * For example, if you have moved something into this
 * directory and then unadd this directory, the child
 * can (probably) be recursively deleted but we need
 * to keep track of the child-delete.
 *
 */
static void _handle_undo_added_directory(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 sg_wc_liveview_item * pLVI,
										 SG_bool bForce,
										 SG_wc_status_flags statusFlags,
										 SG_bool * pbKept)
{
	sg_wc_liveview_dir * pLVD;		// we do not own this
	struct _handle_undo_added_directory_data data;

	data.pWcTx = pWcTx;
	data.pLVI_Dir = pLVI;

	// sniff each child and see if there is anything that
	// would prevent us from unadding this directory.
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
	SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _handle_undo_added_directory_cb, &data)  );

	// if nothing above threw up....
	SG_ERR_CHECK(  _handle_undo_added(pCtx, pWcTx, pLVI, bForce, statusFlags, pbKept)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _remove_file(SG_context * pCtx,
						 SG_wc_tx * pWcTx,
						 sg_wc_liveview_item * pLVI,
						 SG_bool bForce,
						 SG_bool bNoBackups,
						 SG_bool bKeepInWC,
						 SG_bool bFlattenAddSpecialPlusRemove,
						 SG_wc_status_flags statusFlags,
						 SG_bool * pbKept)
{
	SG_string * pStringRepoPath = NULL;
	SG_bool bContentChanged;
	const char * pszDisposition = NULL;
	char * pszHid_owned = NULL;
	SG_uint32 nrBits = 0;

	SG_ASSERT(  (statusFlags & SG_WC_STATUS_FLAGS__T__FILE)  );
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED) == 0)  );	// caller already checked this

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)          nrBits++;
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)  nrBits++;
	if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED) nrBits++;
	if (nrBits > 1)
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,	// TODO 2012/07/26 Perhaps SG_ERR_ASSERT ?
						(pCtx, "More than one of __S__ADDED, __S__MERGE_CREATED, and __S__UPDATE_CREATED set for %s.",
						 SG_string__sz(pLVI->pStringEntryname))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		// An honest ADD that the user did and we can undo.
		SG_ERR_CHECK(  _handle_undo_added(pCtx, pWcTx, pLVI, bForce, statusFlags, pbKept)  );
		goto done;
	}
	else if (statusFlags & (SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED))
	{
		// MERGE or UPDATE created this item in the WD so there is no
		// baseline version of this file to look at.
		//
		// But we stashed the content-hid of the file in tbl_PC
		// for just such an event.  So we can see if the current
		// version in the WD has been edited and decide whether
		// the file is disposable or should be backed-up.

		const char * pszHidMerge = NULL;

		if (pLVI->pPrescanRow && pLVI->pPrescanRow->pPcRow_Ref)
			pszHidMerge = pLVI->pPrescanRow->pPcRow_Ref->pszHidMerge;
		else if (pLVI->pPcRow_PC)
			pszHidMerge = pLVI->pPcRow_PC->pszHidMerge;

		if (pszHidMerge)
		{
			SG_bool bNoTSC = SG_FALSE;		// we trust the TSC.

			SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx, pLVI, pWcTx, bNoTSC, &pszHid_owned, NULL)  );
			bContentChanged = (strcmp(pszHid_owned, pszHidMerge) != 0);
		}
		else
		{
			// This should really be an assert, but I'm in a good mood.
			// Play it safe and claim is is dirty and back it up.
			bContentChanged = SG_TRUE;
		}
		
		// fall thru and let the normal file disposition happen
		// as we remove the item from our data structures.
		// i'm wanting to say that bKeepInWC should be turned off,
		// but i don't have a good reason for that.
	}
	else
	{
		bContentChanged = ((statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED) != 0);
	}

	if (bContentChanged)
	{
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__FOUND) == 0)  );		// no baseline content to diff
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED) == 0)  );	// no baseline content to diff
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__LOST) == 0)  );		// no current content to diff
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE) == 0)  );	// not populated, could not have been edited
		// so we know that the file is CONTROLLED and PRESENT and MODIFIED in the WD.

		if (bKeepInWC)
		{
			// Remove it from version control, but leave the file on the disk.
			// 
			// EVENTUALLY, this file will have status FOUND/IGNORED (in
			// a ***NEW*** LVI).  The current LVI remains to hold the DELETE status
			// with the existing GID.  The new LVI may or may not be contained
			// in the same directory; if the parent directory is also being
			// removed, it to will have a ***NEW*** peer LVI.
			pszDisposition = "keep";
		}
		else if (!bForce)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
																	  &pStringRepoPath)  );
			SG_ERR_THROW2(  SG_ERR_CANNOT_REMOVE_SAFELY,
							(pCtx, "File has been modified: '%s'", SG_string__sz(pStringRepoPath))  );
		}
		else if (bNoBackups)
		{
			// Remove it from version control and remove it from the disk.
			// DO NOT back it up.
			pszDisposition = "delete";
		}
		else
		{
			// The want a backup file for the item.
			pszDisposition = "backup";
		}
	}
	else
	{
		if (statusFlags & (SG_WC_STATUS_FLAGS__A__SPARSE | SG_WC_STATUS_FLAGS__U__LOST))
		{
			// The file is CONTROLLED, but not PRESENT on the disk.
			pszDisposition = "noop";
		}
		else if (bKeepInWC)
		{
			pszDisposition = "keep";
		}
		else if ( ((statusFlags & SG_WC_STATUS_FLAGS__U__FOUND) != 0) && !bForce )
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
																	  &pStringRepoPath)  );
			SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
							(pCtx, "'%s'", SG_string__sz(pStringRepoPath))  );
		}
		else
		{
			pszDisposition = "delete";
		}
	}
	
	SG_ERR_CHECK(  sg_wc_tx__queue__remove(pCtx, pWcTx, pLVI, pszDisposition,
										   bFlattenAddSpecialPlusRemove,
										   pbKept)  );

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_NULLFREE(pCtx, pszHid_owned);
}

/**
 * See if we can queue delete the for directory.  This is
 * subject to the usual (bForce, bKeepInWC, statusFlags).
 *
 * An added complication, we also need to know how many
 * items *within* the directory were left in place and
 * *maybe* reject it.
 *
 * For example, (when not-force and not-keep) if a child
 * was ADDED and the recursive delete would try to do the
 * "undo added" trick and leave the file in the directory.
 * So we can't delete the directory.
 *
 * On the other hand, if "--keep" is set we're not going
 * to acutally remove the items from disk, so it doesn't
 * matter how much stuff we left inside the directory on
 * disk.
 *
 * The "nrImmediateChildrenKept" value indicates how many
 * things we left inside the directory on disk.
 *
 */
static void _remove_directory(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  sg_wc_liveview_item * pLVI,
							  SG_uint32 nrImmediateChildrenKept,
							  SG_bool bForce,
							  SG_bool bKeepInWC,
							  SG_bool bFlattenAddSpecialPlusRemove,
							  SG_wc_status_flags statusFlags,
							  SG_bool * pbKept)
{
	SG_string * pStringRepoPath = NULL;
	const char * pszDisposition = NULL;
	SG_uint32 nrBits = 0;

	SG_ASSERT(  (statusFlags & SG_WC_STATUS_FLAGS__T__DIRECTORY)  );
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED) == 0)  );	// caller already checked this

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)          nrBits++;
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)  nrBits++;
	if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED) nrBits++;
	if (nrBits > 1)
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,	// TODO 2012/07/26 Perhaps SG_ERR_ASSERT ?
						(pCtx, "More than one of __S__ADDED, __S__MERGE_CREATED, and __S__UPDATE_CREATED set for %s.",
						 SG_string__sz(pLVI->pStringEntryname))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		SG_ERR_CHECK(  _handle_undo_added_directory(pCtx, pWcTx, pLVI, bForce, statusFlags, pbKept)  );
		goto done;
	}
	else if (statusFlags & (SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED))
	{
		// MERGE created this item in the WD.
		//
		// We DO NOT cache the content HID in the tbl_PC table.
		//
		// fall thru and let the normal disposition happen
		// as we remove the item.  i'm wanting to say that
		// bKeepInWC should be turned off, but i don't have
		// a good reason for that.
	}

	if (statusFlags & (SG_WC_STATUS_FLAGS__A__SPARSE | SG_WC_STATUS_FLAGS__U__LOST))
	{
		// The dir is CONTROLLED, but not PRESENT on the disk.
		SG_ASSERT(  (nrImmediateChildrenKept == 0)  );
		pszDisposition = "noop";
	}
	else if (bKeepInWC)
	{
		pszDisposition = "keep";
	}
	else if ( ((statusFlags & SG_WC_STATUS_FLAGS__U__FOUND) != 0) && !bForce )
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
																  &pStringRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "'%s'", SG_string__sz(pStringRepoPath))  );
	}
	else
	{
		pszDisposition = "delete";
	}
	
	SG_ERR_CHECK(  sg_wc_tx__queue__remove(pCtx, pWcTx, pLVI, pszDisposition,
										   bFlattenAddSpecialPlusRemove,
										   pbKept)  );

done:
	if ((nrImmediateChildrenKept > 0) && (*pbKept == SG_FALSE))
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
																  &pStringRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_CANNOT_REMOVE_SAFELY,
						(pCtx, "Directory not empty: '%s'", SG_string__sz(pStringRepoPath))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

static void _remove_symlink(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							sg_wc_liveview_item * pLVI,
							SG_bool bForce,
							SG_bool bKeepInWC,
							SG_bool bFlattenAddSpecialPlusRemove,
							SG_wc_status_flags statusFlags,
							SG_bool * pbKept)
{
	SG_string * pStringRepoPath = NULL;
	SG_bool bContentChanged;
	const char * pszDisposition = NULL;
	char * pszHid_owned = NULL;
	SG_uint32 nrBits = 0;

	SG_ASSERT(  (statusFlags & SG_WC_STATUS_FLAGS__T__SYMLINK)  );
	SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED) == 0)  );	// caller already checked this

#if defined(WINDOWS)
	// Since we can't populate symlinks on Windows, they
	// will always be marked SPARSE.
	SG_ASSERT(  (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)  );
	// TODO 2012/11/26 See W3729 for notes on a sparse symlink that MERGE/RESOLVE/COMMIT
	// TODO            needs to deal with.
#endif

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)          nrBits++;
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)  nrBits++;
	if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED) nrBits++;
	if (nrBits > 1)
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,	// TODO 2012/07/26 Perhaps SG_ERR_ASSERT ?
						(pCtx, "More than one of __S__ADDED, __S__MERGE_CREATED, and __S__UPDATE_CREATED set for %s.",
						 SG_string__sz(pLVI->pStringEntryname))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		// An honest ADD that the user did and we can undo.
		SG_ERR_CHECK(  _handle_undo_added(pCtx, pWcTx, pLVI, bForce, statusFlags, pbKept)  );
		goto done;
	}
	else if (statusFlags & (SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED))
	{
		// MERGE created this item in the WD.

		if (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)
		{
			bContentChanged = SG_FALSE;
		}
		else
		{
			// Grab the content HID that we cached in the tbl_PC.
			// If the merge happened in a TX prior to this one, we
			// can get it from pPrescanRow->pPcRow_Ref (and pPcRow_PC
			// won't exist unless there is aleady a pending change
			// to this item in this TX).  On the other hand, the
			// pPrescanRow won't exist if the merge happened earlier
			// in this TX (and the merge will have created the pPcRow_PC).

			const char * pszHidMerge = NULL;

			if (pLVI->pPrescanRow && pLVI->pPrescanRow->pPcRow_Ref)
				pszHidMerge = pLVI->pPrescanRow->pPcRow_Ref->pszHidMerge;
			else if (pLVI->pPcRow_PC)
				pszHidMerge = pLVI->pPcRow_PC->pszHidMerge;

			if (pszHidMerge)
			{
				SG_bool bNoTSC = SG_FALSE;		// we trust the TSC.

				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx, pLVI, pWcTx, bNoTSC, &pszHid_owned, NULL)  );
				bContentChanged = (strcmp(pszHid_owned, pszHidMerge) != 0);
			}
			else
			{
				// This should really be an assert, but I'm in a good mood.
				// Play it safe and claim is is dirty and back it up.
				bContentChanged = SG_TRUE;
			}
		}

		// fall thru and let the normal disposition happen
		// as we remove the item.  i'm wanting to say that
		// bKeepInWC should be turned off, but i don't have
		// a good reason for that.

#if defined(WINDOWS)
		// Since we can't populate symlinks on Windows, they
		// will always be marked SPARSE.  And you can't modify
		// some that isn't there.  So on Windows, we'll never
		// see a changed symlink.
		SG_ASSERT(  (bContentChanged == SG_FALSE)  );
		// TODO 2012/11/26 See W3729 for notes on a sparse symlink that MERGE/RESOLVE/COMMIT
		// TODO            needs to deal with.
#endif
	}
	else
	{
		bContentChanged = ((statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED) != 0);

#if defined(WINDOWS)
		// Since we can't populate symlinks on Windows, they
		// will always be marked SPARSE.  And you can't modify
		// some that isn't there.  So on Windows, we'll never
		// see a changed symlink.
		SG_ASSERT(  (bContentChanged == SG_FALSE)  );
		// TODO 2012/11/26 See W3729 for notes on a sparse symlink that MERGE/RESOLVE/COMMIT
		// TODO            needs to deal with.
#endif
	}

	if (bContentChanged)
	{
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__LOST) == 0)  );		// no current content to diff
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__FOUND) == 0)  );		// no baseline content to diff
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED) == 0)  );	// no baseline content to diff
		SG_ASSERT(  ((statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE) == 0)  );	// not populated, can't be edited
		// so we know that the file is CONTROLLED and PRESENT and MODIFIED in the WD.

		if (bKeepInWC)
		{
			// Remove it from version control, but leave the symlink on the disk.
			// 
			// EVENTUALLY, this item will have status FOUND/IGNORED (in
			// a ***NEW*** LVI).  The current LVI remains to hold the DELETE status
			// with the existing GID.  The new LVI may or may not be contained
			// in the same directory; if the parent directory is also being
			// removed, it to will have a ***NEW*** peer LVI.
			pszDisposition = "keep";
		}
		else if (!bForce)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
																	  &pStringRepoPath)  );
			SG_ERR_THROW2(  SG_ERR_CANNOT_REMOVE_SAFELY,
							(pCtx, "Symlink has been modified: '%s'", SG_string__sz(pStringRepoPath))  );
		}
		else
		{
			// We DO NOT offer to backup symlinks.  (We could, I just didn't
			// see any reason to offer this service, since they don't change
			// that often.)
			// 
			// Remove it from version control and remove it from the disk.
			pszDisposition = "delete";
		}
	}
	else
	{
		if (statusFlags & (SG_WC_STATUS_FLAGS__A__SPARSE | SG_WC_STATUS_FLAGS__U__LOST))
		{
			// The symlink is CONTROLLED, but not PRESENT on the disk.
			pszDisposition = "noop";
		}
		else if (bKeepInWC)
		{
			pszDisposition = "keep";
		}
		else if ( ((statusFlags & SG_WC_STATUS_FLAGS__U__FOUND) != 0) && !bForce )
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
																	  &pStringRepoPath)  );
			SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
							(pCtx, "'%s'", SG_string__sz(pStringRepoPath))  );
		}
		else
		{
			pszDisposition = "delete";
		}
	}
	
	SG_ERR_CHECK(  sg_wc_tx__queue__remove(pCtx, pWcTx, pLVI, pszDisposition,
										   bFlattenAddSpecialPlusRemove,
										   pbKept)  );

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_NULLFREE(pCtx, pszHid_owned);
}

//////////////////////////////////////////////////////////////////

/**
 * Queue diving REMOVE of an item when given a LVI
 *
 */
void sg_wc_tx__rp__remove__lvi(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   sg_wc_liveview_item * pLVI,
							   SG_uint32 level,
							   SG_bool bNonRecursive,
							   SG_bool bForce,
							   SG_bool bNoBackups,
							   SG_bool bKeepInWC,
							   SG_bool bFlattenAddSpecialPlusRemove,
							   SG_bool * pbKept)
{
	SG_wc_status_flags statusFlags;
	SG_bool bNoIgnores;
	SG_bool bNoTSC;

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		// We NEVER want to delete a Reserved item (as part of a
		// version control operation).  That is, no 'vv <verb> ...'
		// command should try to touch it.
		//
		// If we get here because we are in a recursive rmdir,
		// all we can do is say that we kept it in the WD.
		// If the caller is doing a "--keep", bKeepInWC will
		// be set.  If the parent directory has ADDED status,
		// it will try to do an undo-add (and bKeepInWC may or
		// may not be set).  Either way, all we can do is indicate
		// that we did not/will not do a __queue_remove() on it
		// and let the caller decide if that is going to be a
		// problem.  (In theory, an undo-add or a --keep on the
		// parent directory could succeed.)
		*pbKept = SG_TRUE;
		goto done;
	}

	// Compute the status for this item by itself; it does not dive (so
	// it does not look at stuff within (and besides we can't compute
	// whether the directory content is modified until we try a commit)).
	bNoIgnores = SG_FALSE;		// we want to be able to distinguish between IGNORED and FOUND.
	bNoTSC = SG_FALSE;			// we trust the TSC to have valid info.
	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
												bNoIgnores, bNoTSC,
												&statusFlags)  );
#if TRACE_WC_TX_REMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__rp__remove: considering [status 0x%08x][scan_flags_live 0x%08x][level %d] %s\n",
							   (SG_uint32)statusFlags,
							   pLVI->scan_flags_Live,
							   level,
							   SG_string__sz(pLVI->pStringEntryname))  );
#endif

#if defined(DEBUG)
	// I'm just documenting that we don't care about these bits.
	// That is, deleting a moved/renamed/chmod'ed item is fine;
	// our "safe delete" code is only really concerned with
	// modified content.
	statusFlags &= ~SG_WC_STATUS_FLAGS__S__RENAMED;
	statusFlags &= ~SG_WC_STATUS_FLAGS__S__MOVED;
	statusFlags &= ~SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;

	if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND | SG_WC_STATUS_FLAGS__U__IGNORED))
	{
		// sg_wc_tx__rp__remove() checks for uncontrolled items at top level.
		SG_ASSERT( (level > 0) );
	}
#endif

	// WARNING: These structural change bits are ***NOT*** mutually exclusive.
	// WARNING: It is possible to have more than one set.  For example, the
	// WARNING: user could "vv add foo; /bin/rm foo" and we'd have status
	// WARNING: ADDED and LOST.

	if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
	{
		// item has already been taken care of by an earlier command (pre-TX)
		// or perhaps by an earlier item in the arglist (in-TX).  so it has
		// DELETED status *and* does not appear on the disk.
		// 
		// We *ASSUME* that if we have a DELETED directory, that any content
		// has already been deleted too.  That is, if already DELETED, then
		// we ***DO NOT*** need to dive into the LVD and mess with the LVI
		// contained within.

		goto done;
	}

	// TODO 2011/10/27 This can probably be flattened/refactored.
	// TODO            Most of the code in _remove_{file,directory,symlink}()
	// TODO            is duplicated in each.

	if (pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
	{
		SG_ERR_CHECK(  _remove_file(pCtx, pWcTx, pLVI, bForce, bNoBackups, bKeepInWC,
									bFlattenAddSpecialPlusRemove,
									statusFlags, pbKept)  );
	}
	else if (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		// if this item is a directory, dive in and try to delete
		// everything within it first.  even if the directory is
		// lost or sparse, we still need to dive to set the state
		// on each child.
		//
		// for some of the children we will know immediately that
		// we can't delete it and therefore can't delete the directory.
		// but there are cases (combinations of arg bits) that we
		// have to wait for.

		SG_uint32 nrImmediateChildrenKept = 0;

		if (bNonRecursive)
		{
			// If the top-level said to do a non-recursive delete, we
			// sniff the contents of the directory and throw if there
			// is anything there that shouldn't be deleted.
			SG_bool bUndoAdd = ((statusFlags & SG_WC_STATUS_FLAGS__S__ADDED) != 0);
			SG_ERR_CHECK(  _can_do_nonrecursive_delete(pCtx, pWcTx, pLVI, bForce, bKeepInWC, bUndoAdd)  );
		}

		SG_ERR_CHECK(  _dive(pCtx, pWcTx, pLVI, level, bForce, bNoBackups, bKeepInWC, bFlattenAddSpecialPlusRemove,
							 &nrImmediateChildrenKept)  );
		SG_ERR_CHECK(  _remove_directory(pCtx, pWcTx, pLVI,
										 nrImmediateChildrenKept,
										 bForce, bKeepInWC, bFlattenAddSpecialPlusRemove,
										 statusFlags, pbKept)  );
	}
	else if (pLVI->tneType == SG_TREENODEENTRY_TYPE_SYMLINK)
	{
		SG_ERR_CHECK(  _remove_symlink(pCtx, pWcTx, pLVI, bForce, bKeepInWC,
									   bFlattenAddSpecialPlusRemove,
									   statusFlags, pbKept)  );
	}
	else
	{
		// TODO 2011/10/25 subrepos.....

		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Remove on subrepo....")  );
	}
	
done:
	;
fail:
	return;
}

static void _dive_cb(SG_context * pCtx,
					 SG_uint64 uiAliasGid,
					 void * pVoid_LVI,
					 void * pVoid_Data)
{
	struct _dive_data * pDiveData = (struct _dive_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;
	SG_bool bKept = SG_FALSE;

	SG_UNUSED( uiAliasGid );

	SG_ERR_CHECK(  sg_wc_tx__rp__remove__lvi(pCtx,
											 pDiveData->pWcTx,
											 pLVI,
											 pDiveData->level+1,
											 SG_FALSE,		// non-recursive doesn't trickle down
											 pDiveData->bForce,
											 pDiveData->bNoBackups,
											 pDiveData->bKeepInWC,
											 pDiveData->bFlattenAddSpecialPlusRemove,
											 &bKept)  );
	if (bKept)
		pDiveData->nrImmediateChildrenKept++;

fail:
	return;
}

static void _dive(SG_context * pCtx,
				  SG_wc_tx * pWcTx,
				  sg_wc_liveview_item * pLVI,
				  SG_uint32 level,
				  SG_bool bForce,
				  SG_bool bNoBackups,
				  SG_bool bKeepInWC,
				  SG_bool bFlattenAddSpecialPlusRemove,
				  SG_uint32 * pnrImmediateChildrenKept)
{
	// Since we now have a TX with QUEUE and APPLY phases
	// built into the TX, we don't need to explicitly
	// manage a pre-compute check ourselves.  Just dive
	// and try to queue the delete of stuff within the
	// directory and if all that succeeds, delete the
	// directory itself.

	sg_wc_liveview_dir * pLVD;		// we do not own this
	struct _dive_data dive_data;

	// Get the contents of the directory (implicitly causing
	// scandir/readdir as necessary).

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
	dive_data.pWcTx = pWcTx;
	dive_data.level = level;
	dive_data.bForce = bForce;
	dive_data.bNoBackups = bNoBackups;
	dive_data.bKeepInWC = bKeepInWC;
	dive_data.bFlattenAddSpecialPlusRemove = bFlattenAddSpecialPlusRemove;
	dive_data.nrImmediateChildrenKept = 0;

	SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _dive_cb, &dive_data)  );

	*pnrImmediateChildrenKept = dive_data.nrImmediateChildrenKept;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _can_do_nonrecursive_delete_cb(SG_context * pCtx,
										   SG_uint64 uiAliasGid,
										   void * pVoid_LVI,
										   void * pVoid_Data)
{
	struct _nr_data * pData = (struct _nr_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;
	SG_string * pStringRepoPath = NULL;
	SG_bool bReject = SG_FALSE;

	SG_UNUSED( uiAliasGid );

	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		if (pData->bForce || pData->bKeepInWC)
		{
			// Don't reject it.
		}
		else
		{
			bReject = SG_TRUE;
		}
	}
	else if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		if (pData->bKeepInWC || pData->bUndoAdd)
		{
			// Don't reject it, since the parent directory is not actually
			// going to be deleted (either an un-add or --keep).
		}
		else
		{
			// We never assist them in deleting a reserved item
			// because they are never under version control (and
			// we don't want to silently kill them as part of a
			// larger force-remove).
			bReject = SG_TRUE;
		}
	}
	else if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI->scan_flags_Live))
	{
		// Don't reject it.
	}
	else
	{
		bReject = SG_TRUE;
	}

#if TRACE_WC_TX_REMOVE
	if (bReject)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RemoveNonRecursive: Rejecting [scan_flags_live 0x%08x] %s\n",
								   pLVI->scan_flags_Live,
								   SG_string__sz(pLVI->pStringEntryname))  );
	else
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "RemoveNonRecursive: Allowing [scan_flags_live 0x%08x] %s\n",
								   pLVI->scan_flags_Live,
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif

	if (bReject)
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pData->pWcTx,
																  pData->pLVI_Parent,
																  &pStringRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_CANNOT_REMOVE_SAFELY,
						(pCtx, "Cannot do non-recursive delete of '%s' because of '%s'.",
						 SG_string__sz(pStringRepoPath),
						 SG_string__sz(pLVI->pStringEntryname))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

static void _can_do_nonrecursive_delete(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI,
										SG_bool bForce,
										SG_bool bKeepInWC,
										SG_bool bUndoAdd)
{
	// See if the directory is either empty or only contains
	// (the ghosts of) deleted items.  That is, we DO NOT want
	// to delete any currently-controlled items within this
	// directory if we were to delete this directory.
	//
	// If either bForce or bKeepInWC are set, we don't let
	// FOUND items affect the answer.
	//
	// If either bKeepInWC or bUndoAdd, we don't let any
	// RESERVED items found within the directory affect the
	// answer.

	sg_wc_liveview_dir * pLVD;		// we do not own this
	struct _nr_data data;

	memset(&data, 0, sizeof(data));
	data.pWcTx = pWcTx;
	data.pLVI_Parent = pLVI;
	data.bForce = bForce;
	data.bKeepInWC = bKeepInWC;
	data.bUndoAdd = bUndoAdd;

	// Get the contents of the directory (implicitly causing
	// scandir/readdir as necessary).

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
	SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _can_do_nonrecursive_delete_cb, &data)  );

fail:
	return;
}
