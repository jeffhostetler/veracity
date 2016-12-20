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
 * @file sg_wc_tx__rp__move_rename.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Return TRUE if pLVI_1 is an ancestor of pLVI_2.
 *
 */
static void _is_ancestor(SG_context * pCtx,
						 SG_wc_tx * pWcTx,
						 const sg_wc_liveview_item * pLVI_1,
						 const sg_wc_liveview_item * pLVI_2,
						 SG_bool * pbIsAncestor)
{
	sg_wc_liveview_item * pLVI_2_parent;	// we do not own this
	SG_bool bFoundInCache;

	while (pLVI_2)
	{
		if (pLVI_1 == pLVI_2)
		{
			*pbIsAncestor = SG_TRUE;
			return;
		}

		if (!pLVI_2->pLiveViewDir)
			break;

		// since we are walking back up the tree, we
		// know that the items for the parent directories
		// have already been loaded.  (we could just call
		// sg_wc_tx__liveview__fetch_random_item() and not
		// worry about it.)
		SG_ERR_CHECK_RETURN(  sg_wc_tx__cache__find_liveview_item(pCtx, pWcTx,
														   pLVI_2->pLiveViewDir->uiAliasGidDir,
														   &bFoundInCache, &pLVI_2_parent)  );
		SG_ASSERT_RELEASE_RETURN( (bFoundInCache) );

		pLVI_2 = pLVI_2_parent;
	}

	*pbIsAncestor = SG_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * MOVE and/or RENAME the item associated with pLVI_Src
 * to the location associated with (pLVI_DestDir,Entryname).
 *
 */
void sg_wc_tx__rp__move_rename__lvi_lvi_sz(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   sg_wc_liveview_item * pLVI_Src,
										   sg_wc_liveview_item * pLVI_DestDir,
										   const char * pszNewEntryname,
										   SG_bool bNoAllowAfterTheFact,
										   SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src_Computed = NULL;
	SG_string * pStringRepoPath_DestDir_Computed = NULL;
	SG_string * pStringRepoPath_Dest_Computed = NULL;
	sg_wc_liveview_item * pLVI_Dest;		// we do not own this
	sg_wc_liveview_dir * pLVD_DestDir;		// we do not own this
	SG_bool bLostSrc;
	SG_bool bKnown_Dest;
	SG_bool bSrcIsSparse = SG_FALSE;
	SG_bool bDestDirIsSparse = SG_FALSE;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI_Src );
	SG_NULLARGCHECK_RETURN( pLVI_DestDir );
	SG_NONEMPTYCHECK_RETURN( pszNewEntryname );

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Src,
															  &pStringRepoPath_Src_Computed)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_DestDir,
															  &pStringRepoPath_DestDir_Computed)  );

	if (pLVI_Src == pWcTx->pLiveViewItem_Root)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot MOVE/RENAME the root directory '%s'.",
						 SG_string__sz(pStringRepoPath_Src_Computed))  );
	
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI_Src->scan_flags_Live))
	{
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "The item '%s' has already been REMOVED.",
						 SG_string__sz(pStringRepoPath_Src_Computed))  );
	}

	// If the source item and/or the destination directory is sparse
	// (not populated), we want to *try* to allow the move/rename
	// within limits.  The move/rename of a sparse item is basically
	// a bookkeeping operation (since the item doesn't actually exist
	// on disk in the WD).
	bSrcIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI_Src->scan_flags_Live);
	bDestDirIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI_DestDir->scan_flags_Live);
	if (bSrcIsSparse)
	{
		// The src-parent-directory may be sparse or present.
		// The destination-parent-directory may be sparse or present.
	}
	else
	{
		// We can assert that the src-parent-directory is not sparse
		// because a present item cannot be inside a sparse directory.
		// 
		// So we only have to guard that we are not trying move this
		// present item into a sparse destination directory.
		//
		// TODO 2012/11/26 Technically we *could* move a present item into a
		// TODO            sparse destination directory *IF* it is unmodified
		// TODO            (so all we have to do is the move-bookkeeping and
		// TODO            then "make it sparse" (set a bit and delete it)).
		// TODO            But I don't want to do that today.
		
		if (bDestDirIsSparse)
			SG_ERR_THROW2(  SG_ERR_WC_IS_SPARSE,
							(pCtx, "Cannot move non-sparse item '%s' into sparse directory '%s'.",
							 SG_string__sz(pStringRepoPath_Src_Computed),
							 SG_string__sz(pStringRepoPath_DestDir_Computed))  );
	}

	bLostSrc = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI_Src->scan_flags_Live);
	if (bLostSrc && bNoAllowAfterTheFact)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx,
						 ("The item '%s' is currently LOST and"
						  " after-the-fact operations were disallowed."),
						 SG_string__sz(pStringRepoPath_Src_Computed))  );

	// Compute the live-repo-path of the intended final destination.
	// And use it to search for an existing item.  This is a bit
	// expensive (as we could fetch the LVD of pLVI_DestDir and then
	// do a FIND), but let's just use the regular __fetch_item (which
	// needs to start from scratch on the repo-path), because there's
	// a lot of code in there to handle various edge cases that I
	// don't want to repeat here.
	// 
	// In a normal operation, the destination should not exist,
	// so we should not be able to get an LVI for it -- unless
	// it is an after-the-fact operation.

	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
										  &pStringRepoPath_Dest_Computed,
										  pStringRepoPath_DestDir_Computed)  );
	SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx,
												 pStringRepoPath_Dest_Computed,
												 pszNewEntryname,
												 SG_FALSE)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item(pCtx, pWcTx,
												  pStringRepoPath_Dest_Computed,
												  &bKnown_Dest, &pLVI_Dest)  );
	if (bKnown_Dest)
	{
		if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI_Dest->scan_flags_Live))
			SG_ERR_THROW2(  SG_ERR_WC_RESERVED_ENTRYNAME,
							(pCtx, "The destination '%s' is reserved.",
							 SG_string__sz(pStringRepoPath_Dest_Computed))  );

		SG_ERR_CHECK(  sg_wc_tx__rp__move_rename__lvi_lvi(pCtx, pWcTx,
														  pLVI_Src,
														  pLVI_Dest,
														  bNoAllowAfterTheFact,
														  xu_mask)  );
	}
	else
	{
		// The destination is unknown.  This is the normal case --
		// the new entryname does not yet exist in the parent directory.

		if (bLostSrc)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, ("The item '%s' is currently LOST and the"
									" destination '%s' does not refer to a"
									" FOUND item for an after-the-fact operation."),
							 SG_string__sz(pStringRepoPath_Src_Computed),
							 SG_string__sz(pStringRepoPath_Dest_Computed))  );

		// So the source is known and the destination is unknown.
		// Confirm that the destination's parent directory is "well-defined"
		// and capable of receiving the new entryname.  (We don't want a MOVE
		// to start synthesizing/ADDING intermediate parent directories, for
		// example.)

		if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI_DestDir->scan_flags_Live))
			SG_ERR_THROW2(  SG_ERR_WC_RESERVED_ENTRYNAME,
							(pCtx, "The destination parent directory '%s'.",
							 SG_string__sz(pStringRepoPath_DestDir_Computed))  );

		if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_DestDir->scan_flags_Live))
			SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
							(pCtx, "The destination parent directory '%s'.",
							 SG_string__sz(pStringRepoPath_DestDir_Computed))  );

		if ((pLVI_Src->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
			&& pLVI_Src->pLiveViewDir->uiAliasGidDir != pLVI_DestDir->uiAliasGid)
		{
			// We have a DIRECTORY MOVE or MOVE+RENAME.  Verify that they are
			// are not trying to move the directory into a child of itself.
			// For example:  vv move a/b/c a/b/c/d/e/f

			SG_bool bIsAncestor = SG_FALSE;

			SG_ERR_CHECK(  _is_ancestor(pCtx, pWcTx, pLVI_Src, pLVI_DestDir, &bIsAncestor)  );
			if (bIsAncestor)
				SG_ERR_THROW2(  SG_ERR_CANNOT_MOVE_INTO_SUBFOLDER,
								(pCtx, "Cannot move '%s' deeper into itself '%s'.",
								 SG_string__sz(pStringRepoPath_Src_Computed),
								 SG_string__sz(pStringRepoPath_DestDir_Computed))  );
		}

		// See if the new entryname can be added to the parent directory;
		// that is, is it portable?  This may cause a scandir/readdir.
		//
		// We pass SG_TREENODEENTRY_TYPE__INVALID rather than passing
		// the tneType because we are not ADDing this item and do
		// not want it to throw for a symlink, for example.

		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_DestDir, &pLVD_DestDir)  );
		SG_ERR_CHECK(  sg_wc_liveview_dir__can_add_new_entryname(pCtx,
																 pWcTx->pDb,
																 pLVD_DestDir,
																 pLVI_Src,
																 NULL,
																 pszNewEntryname,
																 SG_TREENODEENTRY_TYPE__INVALID,
																 SG_FALSE)  );

		// Update the in-memory LiveView and write to the journal.

		SG_ERR_CHECK(  sg_wc_tx__queue__move_rename__normal(pCtx, pWcTx,
															pStringRepoPath_Src_Computed,
															pStringRepoPath_Dest_Computed,
															pLVI_Src,
															pLVD_DestDir,
															pszNewEntryname,
															xu_mask)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src_Computed);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir_Computed);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Dest_Computed);
}

/**
 * MOVE and/or RENAME the item associated with pLVI_Src
 * to (replace) the item current known as pLVI_Dest.
 *
 */
void sg_wc_tx__rp__move_rename__lvi_lvi(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI_Src,
										sg_wc_liveview_item * pLVI_Dest,
										SG_bool bNoAllowAfterTheFact,
										SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src_Computed = NULL;
	SG_string * pStringRepoPath_Dest_Computed = NULL;
	SG_bool bLostSrc;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI_Src );
	SG_NULLARGCHECK_RETURN( pLVI_Dest );

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Src,
															  &pStringRepoPath_Src_Computed)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Dest,
															  &pStringRepoPath_Dest_Computed)  );

	if (pLVI_Src == pWcTx->pLiveViewItem_Root)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot MOVE/RENAME the root directory '%s'.",
						 SG_string__sz(pStringRepoPath_Src_Computed))  );

	if (pLVI_Dest == pWcTx->pLiveViewItem_Root)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot MOVE/RENAME the root directory '%s'.",
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );
	
	if (pLVI_Src == pLVI_Dest)
		SG_ERR_THROW2(  SG_ERR_NO_EFFECT,
						(pCtx, "The MOVE/RENAME of '%s' to '%s' would have no effect.",
						 SG_string__sz(pStringRepoPath_Src_Computed),
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );

	bLostSrc = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI_Src->scan_flags_Live);
	if (bLostSrc && bNoAllowAfterTheFact)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx,
						 ("The item '%s' is currently LOST and"
						  " after-the-fact operations were disallowed."),
						 SG_string__sz(pStringRepoPath_Src_Computed))  );

	// If the destination repo-path refers to something that already
	// has a GID (probably MATCHED or LOST) so we can't do it.
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED(pLVI_Dest->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_WC__ITEM_ALREADY_EXISTS,
						(pCtx,
						 "The destination '%s' already exists under version control.",
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );

	// So the destination repo-path refers to a FOUND (or IGNORED) item.
	//
	// We *might* be able to do an after-the-fact operation.

	if (!bLostSrc)
		SG_ERR_THROW2(  SG_ERR_WC__ITEM_ALREADY_EXISTS,
						(pCtx,
						 ("The destination '%s' already exists."),
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );

	// For an after-the-fact operation, both items must
	// be of the same type -- can't secretly change a
	// file into a directory.
	if (pLVI_Src->tneType != pLVI_Dest->tneType)
		SG_ERR_THROW2(  SG_ERR_WC__ITEM_ALREADY_EXISTS,
						(pCtx,
						 ("The destination '%s' already exists and has a different type."),
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI_Src->scan_flags_Live))
	{
		// The source item is sparse (non-populated).  So after-the-fact
		// doesn't matter since they could not have moved/renamed a
		// non-present item without our help.  And therefore, the
		// existing destination must refer to a different item.
		//
		// TODO 2012/11/26 It is possible that we might want to let
		// TODO            this slide for a directory.  (I just don't
		// TODO            want to get into dealing with the item's
		// TODO            content at this point.)

		SG_ERR_THROW2(  SG_ERR_WC_IS_SPARSE,
						(pCtx, "The item '%s' is sparse and the destination '%s' already exists.",
						 SG_string__sz(pStringRepoPath_Src_Computed),
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );
	}

	// (In the after-the-fact case) We don't have to worry about
	// them trying to move a directory into a child of itself
	// because the file system couldn't have done that.

	// Make sure that the new name is (collision-wise) portable.
	// Yes, I know that they already did the move/rename and
	// therefore that the new entryname is already present in
	// the working directory, *BUT* that doesn't mean that we
	// can respect it.  On Linux, suppose that they already have
	// a "foo" under version control and they did a "/bin/mv bar FOO"
	// and are now telling us to do a "vv move bar FOO".  Both files
	// can exist in the file system, but we may not want to allow it.
	//
	// We pass SG_TREENODEENTRY_TYPE__INVALID rather than passing
	// the tneType because we are not ADDing this item and do
	// not want it to throw for a symlink, for example.

	SG_ERR_CHECK(  sg_wc_liveview_dir__can_add_new_entryname(pCtx,
															 pWcTx->pDb,
															 pLVI_Dest->pLiveViewDir,
															 pLVI_Src,
															 pLVI_Dest,
															 SG_string__sz(pLVI_Dest->pStringEntryname),
															 SG_TREENODEENTRY_TYPE__INVALID,
															 SG_FALSE)  );

	// Everything is good for an after-the-fact operation.
	// Update the in-memory LiveView and write to the journal.

	SG_ERR_CHECK(  sg_wc_tx__queue__move_rename__after_the_fact(pCtx, pWcTx,
																pStringRepoPath_Src_Computed,
																pStringRepoPath_Dest_Computed,
																pLVI_Src,
																pLVI_Dest,
																xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src_Computed);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Dest_Computed);
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__move_rename__lookup_destdir(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const char * pszInput_DestDir,
											   const SG_string * pStringRepoPath_DestDir,
											   sg_wc_liveview_item ** ppLVI_DestDir)
{
	sg_wc_liveview_item * pLVI_DestDir = NULL;	// we do not own this
	SG_bool bKnown_DestDir = SG_FALSE;

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_DestDir,
														  &bKnown_DestDir, &pLVI_DestDir)  );
	if (!bKnown_DestDir)
		SG_ERR_THROW2(  SG_ERR_WC__MOVE_DESTDIR_NOT_FOUND,
						(pCtx, "'%s' (%s)",
						 pszInput_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );

	else if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_DestDir->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_WC__MOVE_DESTDIR_UNCONTROLLED,
						(pCtx, "'%s' (%s)",
						 pszInput_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );

	else if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI_DestDir->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_WC__MOVE_DESTDIR_PENDING_DELETE,
						(pCtx, "'%s' (%s)",
						 pszInput_DestDir, SG_string__sz(pStringRepoPath_DestDir))  );

	*ppLVI_DestDir = pLVI_DestDir;

fail:
	return;
}
