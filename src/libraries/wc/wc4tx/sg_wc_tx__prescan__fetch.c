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
 * @file sg_wc_tx__prescan__fetch.c
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
 * Fetch pre-tx info for item associated with this alias-gid.
 * This will try to find it in the TX cache and then try to
 * look for it in in the working directory.
 *
 * We use this as the primary mechanism to drive scandir/readdir
 * (by alias rather than repo-path) so that we are sure to do an
 * exact-match on the entrynames as we see them (and not rely on
 * the filesystem to respect case, for example).
 *
 * You DO NOT own the returned pointer.
 *
 * TODO 2011/09/20 This routine (and __fetch_dir()) has a problem
 * TODO            that you can't always randomly request info
 * TODO            for a FOUND item inside a FOUND directory
 * TODO            unless you have already requested info for the
 * TODO            parent directory.  This routine takes the alias
 * TODO            and hits the database (if it can't find them in
 * TODO            the cache); FOUND items have temporary aliases
 * TODO            that aren't in the REFPC DB which aren't created
 * TODO            until first observed during a scan.
 * TODO
 * TODO            2011/10/18 I think the above comment is old because
 * TODO                       we no longer have the REFPC DB.
 * TODO
 * TODO            In practice, this shouldn't matter because most
 * TODO            calls should be driven by the __liveview__fetch_item()
 * TODO            and __liveview__fetch_dir() routines and they are
 * TODO            going to try to walk the given repo-path and
 * TODO            force the intermediate scans.
 *
 */
void sg_wc_tx__prescan__fetch_row(SG_context * pCtx, SG_wc_tx * pWcTx,
								  SG_uint64 uiAliasGid,
								  sg_wc_prescan_row ** ppPrescanRow)
{
	sg_wc_pctne__row * pPT = NULL;
	sg_wc_prescan_dir * pPrescanDirParent;		// we don't own this
	SG_uint64 uiAliasGidParent;
	SG_bool bFoundInCache, bFoundInDb, bFoundInDir;

	if (uiAliasGid == pWcTx->uiAliasGid_Root)
	{
		SG_ERR_CHECK(  sg_wc_prescan_row__alloc__row_root(pCtx, pWcTx, ppPrescanRow)  );
		return;
	}

	// if the row and/or the parent directory has already
	// been scanned, the row will be in the TX cache.

	SG_ERR_CHECK(  sg_wc_tx__cache__find_prescan_row(pCtx, pWcTx, uiAliasGid,
													 &bFoundInCache, ppPrescanRow)  );
	if (bFoundInCache)
		return;

	// hit the DB and see if this alias is under control
	// (this also gives us the alias of the parent directory).

	SG_ERR_CHECK(  sg_wc_pctne__get_row_by_alias(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
												 uiAliasGid,
												 &bFoundInDb, &pPT)  );
	if (!bFoundInDb)
		SG_ERR_THROW(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL  );

	// force a scandir/readdir of the parent directory.
	// this causes the scandir and all of the scanrows
	// to be added to the TX cache.  (This will recurse
	// back up to the root directory as necessary.)

	SG_ERR_CHECK(  sg_wc_pctne__row__get_current_parent_alias(pCtx, pPT, &uiAliasGidParent)  );
	SG_ERR_CHECK(  sg_wc_tx__prescan__fetch_dir(pCtx, pWcTx, uiAliasGidParent, &pPrescanDirParent)  );

	// it's a little quicker to use the directory's private
	// view of the scanrows rather than the full scanrow
	// cache (over everything in the TX) to find the item.

	SG_ERR_CHECK(  sg_wc_prescan_dir__find_row_by_alias(pCtx, pPrescanDirParent,
														uiAliasGid,
														&bFoundInDir, ppPrescanRow)  );
	// by construction, there should be a row with
	// this alias in this directory.  that doesn't
	// mean it is still on disk.
	//
	// TODO 2011/09/19 think about asserting __IS_CONTROLLED
	// TODO            on the row.
	SG_ASSERT_RELEASE_FAIL(  (bFoundInDir)  );

fail:
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch pre-tx data for the contents of this directory.
 * This will try to find the scandir results in the TX cache
 * and then try to create it by doing a scandir/readdir in
 * the working directory.
 *
 * We use this as the primary mechanism to drive scandir/readdir
 * (by alias rather than repo-path) so that we are sure to do an
 * exact-match on the entrynames as we see them (and not rely on
 * the filesystem to respect case, for example).
 *
 * You DO NOT own the returned pointer.
 *
 */
void sg_wc_tx__prescan__fetch_dir(SG_context * pCtx, SG_wc_tx * pWcTx,
								  SG_uint64 uiAliasGid,
								  sg_wc_prescan_dir ** ppPrescanDir)
{
	SG_string * pStringRefRepoPath = NULL;
	sg_wc_pctne__row * pPT = NULL;
	sg_wc_prescan_dir * pPrescanDirParent;		// we don't own this
	sg_wc_prescan_row * pPrescanRow;			// we don't own this
	SG_uint64 uiAliasGidParent;
	SG_bool bFoundInCache, bFoundInDb, bFoundInDir;

	SG_ERR_CHECK(  sg_wc_tx__cache__find_prescan_dir(pCtx, pWcTx, uiAliasGid,
													 &bFoundInCache, ppPrescanDir)  );
	if (bFoundInCache)
		return;
	
	if (uiAliasGid == pWcTx->uiAliasGid_Root)
	{
		// since the root directory "@/" cannot be moved/renamed
		// (nor can anything above it), we don't need to worry
		// about the pre-tx vs in-tx differences in the repo-path
		// for it.

		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringRefRepoPath, "@/")  );

		SG_ERR_CHECK(  sg_wc_prescan_dir__alloc__scan_and_match(pCtx, pWcTx,
																uiAliasGid,
																pStringRefRepoPath,
																SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT,
																ppPrescanDir)  );
	}
	else
	{
		// hit the DB and see if this alias is under control
		// (this also gives us the alias of the parent directory).

		SG_ERR_CHECK(  sg_wc_pctne__get_row_by_alias(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
													 uiAliasGid,
													 &bFoundInDb, &pPT)  );
		if (!bFoundInDb)
			SG_ERR_THROW(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL  );

		// see if the parent directory has already been scanned
		// and/or force it to be loaded.  this causes the scandir
		// and all of the scanrows to be added to the TX cache.
		// (This will recurse back up to the root directory as
		// necessary.)

		SG_ERR_CHECK(  sg_wc_pctne__row__get_current_parent_alias(pCtx, pPT, &uiAliasGidParent)  );
		SG_ERR_CHECK(  sg_wc_tx__prescan__fetch_dir(pCtx, pWcTx, uiAliasGidParent, &pPrescanDirParent)  );

		// it's a little quicker to use the directory's private
		// view of the scanrows rather than the full scanrow
		// cache (over everything in the TX) to find the item.

		SG_ERR_CHECK(  sg_wc_prescan_dir__find_row_by_alias(pCtx, pPrescanDirParent,
															uiAliasGid,
															&bFoundInDir, &pPrescanRow)  );
		// by construction, there should be a row with
		// this alias in the parent directory.  but that
		// doesn't mean it is still on disk -- just that
		// we know about it.

		SG_ASSERT_RELEASE_FAIL(  (bFoundInDir)  );
		SG_ASSERT_RELEASE_FAIL(  (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED(pPrescanRow->scan_flags_Ref))  );
		SG_ASSERT_RELEASE_FAIL(  (pPrescanRow->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)  );

		if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_MATCHED(pPrescanRow->scan_flags_Ref))
		{
			// Construct the reference-repo-path for this directory
			// and then try to do a scandir/readdir on it.  We construct
			// the repo-path this way to ensure we get an *EXACT* match.
			//
			// Since we are __MATCHED, we know that the case of the
			// directory on disk exactly matches what were expecting
			// it to be.  (This lets us avoid the problems caused
			// when stat()/exists() lie to us because of case
			// insenstivity.)

			SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
												  &pStringRefRepoPath,
												  pPrescanDirParent->pStringRefRepoPath)  );
			SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx,
														 pStringRefRepoPath,
														 SG_string__sz(pPrescanRow->pStringEntryname),
														 SG_TRUE)  );
			
			SG_ERR_CHECK(  sg_wc_prescan_dir__alloc__scan_and_match(pCtx, pWcTx,
																	uiAliasGid,
																	pStringRefRepoPath,
																	pPrescanRow->scan_flags_Ref,
																	ppPrescanDir)  );
		}
		else
		{
			// TODO 2011/09/19 what to do for a deleted or lost directory....
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRefRepoPath);
}

