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
 * @file sg_wc_tx__liveview__fetch.c
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
 * Split the given live-repo-path into an array of entrynames.
 * We DO NOT confirm that they exist or are defined.
 *
 */
static void _parse_repo_path__split_string(SG_context * pCtx,
										   const SG_string * pStringLiveRepoPath,
										   SG_varray ** ppvaEntrynames)
{
	SG_varray * pvaEntrynames = NULL;
	const char * psz_0;
	SG_uint32 nrEntrynames;

	SG_ERR_CHECK(  sg_wc_path__split_string_into_varray(pCtx, pStringLiveRepoPath, &pvaEntrynames)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaEntrynames, &nrEntrynames)  );

	SG_ARGCHECK(  (nrEntrynames > 0), pStringLiveRepoPath  );

	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaEntrynames, 0, &psz_0)  );
	SG_ARGCHECK(  (psz_0[0] == '@'), pStringLiveRepoPath  );

	*ppvaEntrynames = pvaEntrynames;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaEntrynames);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch the LiveViewItem associated with the given repo-path.
 * This is a LIVE repo-path that refers to something IN-TX
 * and NOT a pre-tx path.
 *
 * This will allocate as necessary any parent LVDs and this LVI
 * and add them to the TX cache.
 *
 * Like many of the __find/__fetch/__get type routines, we
 * return a "bFound" rather than throwing.  I'm going to call
 * this "bKnown" here because I want to make a subtle distinction:
 * *Known* means that we know something about it -- the item could
 * be UNCONTROLLED (FOUND or IGNORED) or CONTROLLED (MARKED or LOST).
 * Present or not-present, but known about.  (And there could be
 * several levels of FOUND parent directories observed along the
 * way as long as we have done/could do all the scandirs required
 * to identify this item.
 *
 * We return !bKnown when the repo-path takes us off into the weeds.
 * 
 * You DO NOT own the returned pointer.
 *
 */
void sg_wc_tx__liveview__fetch_item(SG_context * pCtx, SG_wc_tx * pWcTx,
									const SG_string * pStringLiveRepoPath,
									SG_bool * pbKnown,
									sg_wc_liveview_item ** ppLVI)
{
	SG_varray * pvaEntrynames = NULL;
	sg_wc_liveview_dir * pLVD_prev;			// we do not own this
	sg_wc_liveview_item * pLVI_root = NULL;	// we do not own this
	sg_wc_liveview_item * pLVI_k = NULL;	// we do not own this
	SG_vector * pVecItemsFound = NULL;
	SG_string * pStringLiveRepoPath_prev = NULL;
	SG_uint32 nrEntrynames, k;
	const char * psz = SG_string__sz(pStringLiveRepoPath);

	// Create and/or get the cached LVI for "@/".
	// This ensures that we have a well-connected
	// top-down walk.

	SG_ERR_CHECK(  sg_wc_liveview_item__alloc__root_item(pCtx, pWcTx, &pLVI_root)  );

	// we should only be given repo-paths in the current/live domain.

	if ((psz[0] != '@') || (psz[1] != '/'))
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "FetchItem expected repo-path starting with '@/' but received '%s'.",
						 psz)  );

	if (psz[2]==0)		// if (strcmp(SG_string__sz(pStringLiveRepoPath), "@/") == 0)
	{
		*pbKnown = SG_TRUE;
		*ppLVI = pLVI_root;
		return;
	}

	// Since they might give us a repo-path that dives
	// into n controlled directories and then dives into
	// a bunch of uncontrolled directories, we cannot
	// assume that we can just map the repo-path to a
	// controlled-alias using the DB.  We have to do
	// this component by component and synthesize FOUND
	// directories and items as necessary.

	SG_ERR_CHECK(  _parse_repo_path__split_string(pCtx,
												  pStringLiveRepoPath,
												  &pvaEntrynames)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaEntrynames, &nrEntrynames)  );
	SG_ARGCHECK(  (nrEntrynames > 1), pStringLiveRepoPath  );

	// Build a partial repo-path as we dive so that
	// we can print nicer error messages.
	//
	// We always want to create it in the current/live '/' domain.
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLiveRepoPath_prev, "@/")  );

	// Fetch/Create the LVD for the root directory (the contents
	// of "@/").  This will force a scandir/readdir/sync the
	// first time we're called.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_root,
												 &pLVD_prev)  );

	// Begin tree-walk into the given repo-path and force
	// populate all of the intermediate directories.
	// This ensures that we *exact-match* the entryname[k]
	// at each level *and* forces us to synthesize full
	// info for any FOUND items/directories along the path.
	// (See the "TODO 2011/09/20 This routine..." in the
	// comment at the top of sg_wc_tx__prescan__fetch_row().)

	for (k=1; k<nrEntrynames; k++)
	{
		const char * pszEntryname_k;		// we do not own this
		SG_uint32 nrItemsFound;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaEntrynames, k, &pszEntryname_k)  );

		// Lookup entryname[k] in directory[k-1].  We do this
		// by entryname because we do not have an alias for it
		// yet.
		// 
		// The LVD won't have moved-away stuff, but it can
		// have DELETED items that we don't want to see
		// (because they are not unique).
		//
		// This find returns a vector of the matches because
		// entrynames within a directory are not necessarily
		// unique -- (suppose we did a sparse checkout and
		// then the user created an item with the same name)
		// (or if they '/bin/rm' deleted a file and then did
		// a mkdir).
		//
		// TODO 2011/09/20 Think about the set of flags we're
		// TODO            passing here.
		// TODO            Do we really want LOST items?

		SG_ERR_CHECK(  sg_wc_liveview_dir__find_item_by_entryname(pCtx,
																  pLVD_prev,
																  pszEntryname_k,
																  (
																	  SG_WC_LIVEVIEW_DIR__FIND_FLAGS__UNCONTROLLED
																	  |SG_WC_LIVEVIEW_DIR__FIND_FLAGS__RESERVED
																	  // not __deleted
																	  |SG_WC_LIVEVIEW_DIR__FIND_FLAGS__MATCHED
																	  |SG_WC_LIVEVIEW_DIR__FIND_FLAGS__LOST
																	  |SG_WC_LIVEVIEW_DIR__FIND_FLAGS__SPARSE
																	  ),
																  &pVecItemsFound)  );
		SG_ERR_CHECK(  SG_vector__length(pCtx, pVecItemsFound, &nrItemsFound)  );
		if (nrItemsFound == 0)
		{
			*pbKnown = SG_FALSE;
			*ppLVI = NULL;
			goto fail;
		}
		else if (nrItemsFound > 1)
		{
			SG_ERR_THROW2(  SG_ERR_WC_PATH_NOT_UNIQUE,
							(pCtx, "The directory '%s' contains multiple items with entryname '%s'.",
							 SG_string__sz(pStringLiveRepoPath_prev),
							 pszEntryname_k)  );
		}

		SG_ERR_CHECK(  SG_vector__get(pCtx, pVecItemsFound, 0, (void **)&pLVI_k)  );
		SG_VECTOR_NULLFREE(pCtx, pVecItemsFound);
		
		if (k+1 < nrEntrynames)
		{
			SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pStringLiveRepoPath_prev,
														 pszEntryname_k,
														 (pLVI_k->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY))  );
			if (pLVI_k->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
				SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_k, &pLVD_prev)  );
			else
				SG_ERR_THROW2(  SG_ERR_NOT_A_DIRECTORY,
								(pCtx, "'%s' (in path '%s')",
								 SG_string__sz(pStringLiveRepoPath_prev),
								 SG_string__sz(pStringLiveRepoPath))  );
		}
	}

	*pbKnown = SG_TRUE;
	*ppLVI = pLVI_k;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaEntrynames);
	SG_VECTOR_NULLFREE(pCtx, pVecItemsFound);
	SG_STRING_NULLFREE(pCtx, pStringLiveRepoPath_prev);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch the LiveViewDir associated with the given LiveViewItem.
 * (That is, pLVI refers to a directory item and we want to
 * request the contents of that directory.)
 *
 * This will look in the LVD TX cache and then if necessary
 * create one.  If there is a corresponding scandir/readdir
 * it will bind to it (and possibly cause a scandir/readdir
 * to happen).
 *
 * By forcing this routine to take a LVI as input, we can
 * verify that the LVD for the parent directory has already
 * been seen/fetched (since the LVIs are members of the LVD).
 * This ensures that we did a top-down tree-walk (probably
 * via sg_wc_tx__liveview__fetch_item(...RepoPath...)).
 *
 * So we are in-effect, requesting something one-level
 * deeper in the VC tree.
 * 
 * You DO NOT own the returned pointer.
 *
 */
void sg_wc_tx__liveview__fetch_dir(SG_context * pCtx, SG_wc_tx * pWcTx,
								   const sg_wc_liveview_item * pLVI,
								   sg_wc_liveview_dir ** ppLVD)
{
	SG_string * pStringRefRepoPath = NULL;
	sg_wc_prescan_dir * pPrescanDir;		// we do not own this
	SG_bool bFoundInCache;

	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		SG_ERR_THROW2(  SG_ERR_NOT_A_DIRECTORY,
						(pCtx, "'%s'", SG_string__sz(pLVI->pStringEntryname))  );

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		// We NEVER want to look inside of a RESERVED directory.
		// Our caller should not have asked.
		//
		// This is more of an assert.
		SG_ERR_THROW2_RETURN(  SG_ERR_WC_RESERVED_ENTRYNAME,
							   (pCtx, "'%s'", SG_string__sz(pLVI->pStringEntryname))  );
	}

	SG_ERR_CHECK(  sg_wc_tx__cache__find_liveview_dir(pCtx, pWcTx,
													  pLVI->uiAliasGid,
													  &bFoundInCache, ppLVD)  );
#if TRACE_WC_TX_LVD
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "FetchDir: %s '%s' [Found LVD in cache %d][scan_flags_Live %x]\n",
								   SG_uint64_to_sz(pLVI->uiAliasGid,bufui64),
								   SG_string__sz(pLVI->pStringEntryname),
								   bFoundInCache,
								   pLVI->scan_flags_Live)  );
	}
#endif
	if (bFoundInCache)
		return;

	// I'd like to call sg_wc_tx__prescan__fetch_dir() at this
	// point and just let it do everything, but it will try to
	// hit the DB and will stumble over things FOUND or ADDED
	// in this TX.  So we do it the hard way.
	// 
	// See if the scandir/readdir code has already seen this
	// directory.

	SG_ERR_CHECK(  sg_wc_tx__cache__find_prescan_dir(pCtx, pWcTx, pLVI->uiAliasGid,
													 &bFoundInCache, &pPrescanDir)  );
#if TRACE_WC_TX_LVD
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "FetchDir: %s '%s' [Found SCANDIR in cache %d][scan_flags_Live %x]\n",
								   SG_uint64_to_sz(pLVI->uiAliasGid,bufui64),
								   SG_string__sz(pLVI->pStringEntryname),
								   bFoundInCache,
								   pLVI->scan_flags_Live)  );
	}
#endif
	if (bFoundInCache)
	{
		SG_ERR_CHECK(  SG_WC_LIVEVIEW_DIR__ALLOC__CLONE_FROM_PRESCAN(pCtx, pWcTx,
																	 pPrescanDir,
																	 ppLVD)  );
		return;
	}

	// If this item is associated with a scanrow, then we
	// know where the item was at the start of the TX (and
	// have already done a scandir/readdir on its initial
	// parent directory).  So we just need to request that
	// scandir go one level deeper.

	if (pLVI->pPrescanRow)
	{
		if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_ROOT(pLVI->scan_flags_Live))
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringRefRepoPath, "@/")  );
		}
		else
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pStringRefRepoPath,
												  pLVI->pPrescanRow->pPrescanDir_Ref->pStringRefRepoPath)  );
			SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx,
														 pStringRefRepoPath,
														 SG_string__sz(pLVI->pPrescanRow->pStringEntryname),
														 SG_TRUE)  );
		}

#if TRACE_WC_TX_LVD
		{
			SG_int_to_string_buffer bufui64;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "FetchDir: %s '%s' [scan_flags_Live %x] [ref_path %s]\n",
									   SG_uint64_to_sz(pLVI->uiAliasGid,bufui64),
									   SG_string__sz(pLVI->pStringEntryname),
									   pLVI->scan_flags_Live,
									   SG_string__sz(pStringRefRepoPath))  );
		}
#endif

		SG_ERR_CHECK(  SG_WC_PRESCAN_DIR__ALLOC__SCAN_AND_MATCH(pCtx, pWcTx,
																pLVI->uiAliasGid,
																pStringRefRepoPath,
																pLVI->scan_flags_Live,
																&pPrescanDir)  );
		SG_ERR_CHECK(  SG_WC_LIVEVIEW_DIR__ALLOC__CLONE_FROM_PRESCAN(pCtx, pWcTx,
																	 pPrescanDir,
																	 ppLVD)  );
		SG_STRING_NULLFREE(pCtx, pStringRefRepoPath);
		return;
	}

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_POSTSCAN(pLVI->scan_flags_Live))
	{
		// This directory is in the process of being added by MERGE.
		// We have an LVI for it, but it has not yet been created on
		// disk and none of the SCANDIR/READDIR code knows anything
		// about it yet.
		//
		// TODO 2012/02/01 We really need to rename the _prescan_flags_
		// TODO            because they are quite misleading now.

		SG_ERR_CHECK(  SG_WC_LIVEVIEW_DIR__ALLOC__ADD_SPECIAL(pCtx, pWcTx, pLVI->uiAliasGid, ppLVD)  );
		return;
	}

	// TODO 2011/09/20 Decide what to do here or if this case is even possible.

	SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
					(pCtx, "sg_wc_tx__liveview__fetch_dir: looking for LVD.")  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRefRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch a random LVI (loading/scanning/etc as necessary).
 *
 * There are times (such as when talking about the moved-from
 * directory of an item moved in an earlier TX) when we need
 * to get a LVI but don't have a repo-path on hand.
 *
 * We may or may not have already visited (during this TX)
 * this item and/or its parent directories.  We need to work
 * backwards from this alias-gid until we become connected to
 * the LVI/LVD caches (to compensate for moves/renames of
 * this and/or any parent directory relative to the start of
 * this TX).
 *
 */
void sg_wc_tx__liveview__fetch_random_item(SG_context * pCtx, SG_wc_tx * pWcTx,
										   SG_uint64 uiAliasGid,
										   SG_bool * pbKnown,
										   sg_wc_liveview_item ** ppLVI)
{
	SG_uint64 uiAliasGidParent;
	sg_wc_pctne__row * pPT = NULL;
	sg_wc_liveview_dir * pLVD_Parent;		// we do not own this
	sg_wc_liveview_item * pLVI;				// we do not own this
	sg_wc_liveview_item * pLVI_Parent;		// we do not own this
	SG_bool bFoundInCache, bFoundInDb, bFoundInDir, bKnown;

	if (!pWcTx->pLiveViewItem_Root)
	{
		sg_wc_liveview_item * pLVI_root;	// we do not own this

		// Make sure that the LVI for the root directory
		// is loaded before we start.  This ensures that
		// we have a backstop for the recursion below.

		SG_ERR_CHECK(  sg_wc_liveview_item__alloc__root_item(pCtx, pWcTx, &pLVI_root)  );
	}

	// See if it already known to the cache.

	SG_ERR_CHECK(  sg_wc_tx__cache__find_liveview_item(pCtx, pWcTx, uiAliasGid,
													   &bFoundInCache, &pLVI)  );
	if (bFoundInCache)
	{
		*pbKnown = SG_TRUE;
		*ppLVI = pLVI;
		goto fail;
	}

	// This item is NOT known to our (LVI) cache.
	// So this item has not be previously altered
	// in *this* TX.
	// It also means that this item's current
	// parent directory is NOT known to our LVD cache.
	// This means that there have not been any
	// operations *anywhere* below the parent directory.
	// (A change in a peer/sibling/nephew of this item
	// would have caused the top-down tree-walk.)
	//
	// So we can safely request this alias-gid from the
	// PC table directly and get the current
	// alias-gid-parent knowning that this item is still
	// in that directory (otherwise we'd already have a
	// LVI for it in our cache).
	//
	// WE DO NOT USE A SCANDIR/READDIR (nor do we use
	// the Prescan caches) because we just want the
	// immediate parent so that we can crawl back to
	// a known point in the LVI/LVD caches.

	SG_ERR_CHECK(  sg_wc_pctne__get_row_by_alias(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
												 uiAliasGid,
												 &bFoundInDb, &pPT)  );
	if (!bFoundInDb)
	{
		*pbKnown = SG_FALSE;
		*ppLVI = NULL;
		goto fail;
	}

	// Get the id of the current parent (as of the beginning
	// of the TX) and *RECURSE* and do a random lookup on it
	// and see if we can get connected to the LVI/LVD cache
	// with it.  If it returns a pLVI_Parent, the parent is
	// connected.

	SG_ERR_CHECK(  sg_wc_pctne__row__get_current_parent_alias(pCtx, pPT, &uiAliasGidParent)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAliasGidParent,
														 &bKnown, &pLVI_Parent)  );
	if (!bKnown)
	{
		*pbKnown = SG_FALSE;
		*ppLVI = NULL;
		goto fail;
	}

	// Force load the contents of the parent directory (causing
	// a scandir/readdir as necessary).  This will (hopefully)
	// cause a LVI for our item to be created.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_Parent, &pLVD_Parent)  );
	SG_ERR_CHECK(  sg_wc_liveview_dir__find_item_by_alias(pCtx, pLVD_Parent, uiAliasGid,
														  &bFoundInDir, &pLVI)  );
	if (!bFoundInDir)
	{
		*pbKnown = SG_FALSE;
		*ppLVI = NULL;
		goto fail;
	}

	*pbKnown = SG_TRUE;
	*ppLVI = pLVI;

fail:
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
}

//////////////////////////////////////////////////////////////////

/**
 * Compute the "live repo path" for the given LVI.
 * This is the *CURRENT* location of the item at
 * this point in the TX.  THIS PATH IS ONLY GUARANTEED
 * TO BE VALID UNTIL THE NEXT MOVE/RENAME/WHATEVER
 * IN THIS TX.
 *
 * We crawl thru the LVI/LVD caches and construct
 * the current repo-path.  By construction, for us
 * to have a valid LVI means that the entire tree
 * between this LVI and the root has already been
 * visited and is in the caches.
 * 
 * You own the returned string.
 *
 * TODO 2011/10/03 Build a cache in the pWcTx of these as
 * TODO            we compute them.  (Flushing it on any
 * TODO            move/rename.)
 *
 * If the item does not have a live (detination-relative)
 * repo-path, we SILENTLY SUBSTITUTE the baseline-relative
 * repo-path.  If don't have a baseline version (because it
 * was added by merge), silently substitute the other-relative
 * repo-path.  TODO note problem caused by UPDATE.
 * 
 */
void sg_wc_tx__liveview__compute_live_repo_path(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												sg_wc_liveview_item * pLVI,
												SG_string ** ppStringLiveRepoPath)
{
	SG_string * pString = NULL;
	char * pszGid = NULL;
	sg_wc_liveview_item * pLVI_Parent;		// we do not own this
	SG_bool bFoundParentInCache;
	sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;

	if (pLVI == pWcTx->pLiveViewItem_Root)
	{
		// use the '/' domain for current/live repo-paths
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppStringLiveRepoPath,
											SG_WC__REPO_PATH_PREFIX__LIVE)  );	// "@/"
		return;
	}

	SG_ASSERT_RELEASE_FAIL( (pLVI->uiAliasGid != pWcTx->uiAliasGid_Root) );
	SG_ASSERT_RELEASE_FAIL( (pLVI->pLiveViewDir) );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_flags_net(pCtx, pLVI, &flags_net)  );
	
	if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__DELETED)
	{
		// The item has been DELETED.  It no longer owns a "live" repo-path.

		if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_M)
		{
			// They DELETED an item that MERGE created.  So there isn't
			// a defined "live" or "baseline" repo-path.  Use tbl_L1 to
			// get repo-path for it (as it was in L1 and with whatever
			// domain we are using for L1).  See W3520.
			//
			// TODO 2012/05/15 Verify that this makes sense after an UPDATE. W1733.

			SG_ERR_CHECK(  sg_wc_db__tne__get_extended_repo_path_from_alias(pCtx,
																			pWcTx->pDb,
																			pWcTx->pCSetRow_Other,
																			pLVI->uiAliasGid,
																			SG_WC__REPO_PATH_DOMAIN__C,
																			&pString)  );
		}
		else if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U)
		{
			// UPDATE re-created the item (that was deleted in the update target)
			// because of a conflict.
			//
			// The item DOES NOT have a baseline path (remember after the update
			// is finished the baseline is set to the target cset).  The item
			// DOES NOT own a live repo-path.
			//
			// TODO 2012/05/24 Not sure what we should report here.  For now, just
			// TODO            return a GID-domain repo-path.
			// TODO
			// TODO            This needs to be revisited because we are in a recursive
			// TODO            call and we don't want to get something like @g123..../file
			// TODO            (or do we?)
			// TODO
			// TODO            See W7521.
			
			SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pString, "@%s", pszGid)  );
		}
		else
		{
			const SG_string * pStringBaselineRepoPath;	// we do not own this

			// a DELETE of a normal ADD should have done an UNDO-ADD causing
			// this item to be FOUND rather than DELETE+ADD.
			SG_ASSERT(  ((flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADDED) == 0)  );

			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_baseline_repo_path(pCtx, pWcTx, pLVI,
																		  &pStringBaselineRepoPath)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pString, pStringBaselineRepoPath)  );
		}
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_tx__cache__find_liveview_item(pCtx, pWcTx,
														   pLVI->pLiveViewDir->uiAliasGidDir,
														   &bFoundParentInCache, &pLVI_Parent)  );
		SG_ASSERT_RELEASE_FAIL( (bFoundParentInCache) );

		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Parent, &pString)  );
		SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pString,
													 SG_string__sz(pLVI->pStringEntryname),
													 (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY))  );
	}

	*ppStringLiveRepoPath = pString;
	pString = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_NULLFREE(pCtx, pszGid);
}

/**
 * For items present in the baseline, compute
 * the repo-path as it was in the baseline.
 *
 * If the item was not present in the baseline,
 * just return NULL.
 *
 * You DO NOT own the returned string (since it
 * will be constant, we can cache it in the LVI
 * and re-use it).
 *
 * If the item does not have a baseline-relative repo-path
 * we DO NOT substitute a live/destination-relative repo-path.
 * 
 */
void sg_wc_tx__liveview__compute_baseline_repo_path(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_liveview_item * pLVI,
													const SG_string ** ppStringBaselineRepoPath)
{
	const char * pszEntrynameInBaseline;			// we do not own this
	sg_wc_liveview_item * pLVI_ParentInBaseline;	// we do not own this
	const SG_string * pStringParent;				// we do not own this
	SG_uint64 uiAliasGidParentInBaseline;
	SG_bool bKnownInBaseline;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI );
	SG_NULLARGCHECK_RETURN( ppStringBaselineRepoPath );

	*ppStringBaselineRepoPath = NULL;

	// if the item is uncontrolled (found or ignored)
	// then it wasn't present in the baseline.

	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
		return;

	if (pLVI->pStringAlternateBaselineRepoPath)
	{
		// if we have a temporary/alternate baseline (UPDATE is in-progres)
		// silently substitute it.
		*ppStringBaselineRepoPath = pLVI->pStringAlternateBaselineRepoPath;
		return;
	}

	// TODO 2012/02/14 If add_special, think about returning
	// TODO            an L1-based domain repo-path.  See W3520.
	// TODO            (We still probably need this even with
	// TODO            the alternate stuff above.)

	// if no TNE row, it wasn't present in the baseline.
	// For example, a pending ADD.  (There might be other
	// ways for this to happen, such as after a MERGE.)
	if (!pLVI->pPrescanRow || !pLVI->pPrescanRow->pTneRow)
		return;

	// we cache the result (because we tend to call this
	// routine on a series of files and recurse up to the
	// root).

	if (pLVI->pStringBaselineRepoPath)
	{
		*ppStringBaselineRepoPath = pLVI->pStringBaselineRepoPath;
		return;
	}

	if (pLVI == pWcTx->pLiveViewItem_Root)
	{
		// use the 'b' domain for baseline-based repo-paths.
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pLVI->pStringBaselineRepoPath,
											SG_WC__REPO_PATH_PREFIX__B)  );
		*ppStringBaselineRepoPath = pLVI->pStringBaselineRepoPath;
		return;
	}

	SG_ASSERT_RELEASE_FAIL( (pLVI->uiAliasGid != pWcTx->uiAliasGid_Root) );
	SG_ASSERT_RELEASE_FAIL( (pLVI->pLiveViewDir) );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntrynameInBaseline)  );
	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_parent_alias(pCtx, pLVI, &uiAliasGidParentInBaseline)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAliasGidParentInBaseline,
														 &bKnownInBaseline, &pLVI_ParentInBaseline)  );
	if (!bKnownInBaseline)
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "Could not find baseline parent LVI for '%s'.", pszEntrynameInBaseline)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_baseline_repo_path(pCtx, pWcTx, pLVI_ParentInBaseline,
																  &pStringParent)  );
	if (!pStringParent)
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "Could not compute baseline repo path for '%s'.", pszEntrynameInBaseline)  );
	SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
											SG_string__sz(pStringParent),
											pszEntrynameInBaseline,
											(pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
											&pLVI->pStringBaselineRepoPath)  );
	*ppStringBaselineRepoPath = pLVI->pStringBaselineRepoPath;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * We were given an "@a/..." repo-path and need to look it up in
 * the common ancestor -- assuming we have a pending merge.
 *
 */
static void _fetch_A(SG_context * pCtx, SG_wc_tx * pWcTx,
					 const SG_string * pStringRepoPath,
					 SG_bool * pbKnown,
					 sg_wc_liveview_item ** ppLVI)
{
	sg_wc_db__cset_row * pCSetRowA = NULL;
	SG_string * pStr = NULL;
	const char * pszInput;
	SG_bool bHaveCSet;
	SG_treenode * ptnSuperRoot = NULL;
	SG_treenode_entry * ptne = NULL;
	char * pszGid = NULL;
	SG_uint64 uiAlias;

	*pbKnown = SG_FALSE;
	*ppLVI = NULL;

	// map "@a/..." to "@/...".
	pszInput = SG_string__sz(pStringRepoPath);
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStr, "@%s", &pszInput[2])  );

	// lookup the details of the ancestor cset and try to find this item within it.
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pWcTx->pDb, "A", &bHaveCSet, &pCSetRowA)  );
	if (!bHaveCSet)		// This should have been weeded out by our caller.
		SG_ERR_THROW(  SG_ERR_NOT_IN_A_MERGE  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pWcTx->pDb->pRepo, pCSetRowA->psz_hid_super_root, &ptnSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pWcTx->pDb->pRepo, ptnSuperRoot, SG_string__sz(pStr),
														   &pszGid, &ptne)  );
	if (!pszGid)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Item '%s' not found in ancestor changeset '%s'.",
						 SG_string__sz(pStr), pCSetRowA->psz_hid_cset)  );

	// map the GID to an alias in WC so that we can look it up relative to WC.
	sg_wc_db__gid__get_alias_from_gid(pCtx, pWcTx->pDb, pszGid, &uiAlias);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		// This can happen if the file was in the ancestor, but not in either
		// parent *and* this is a fresh WC so that the tbl_gid never had an
		// opportunity to accumulate the gid/alias for it.  This doesn't matter
		// because the item isn't present in either parent, so we won't have
		// an LVI for it.
		SG_context__err_reset(pCtx);
	}
	else
	{
		// This lookup can fail if neither parent knows anything about this
		// item.  This can happen if the item has been deleted in both branches
		// *between* the ancestor and both parents.
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAlias, pbKnown, ppLVI)  );
	}

	if (!*pbKnown)
	{
		// TODO 2013/01/15 There is one edge-case that I can't currently handle.  If the item
		// TODO            was present in the ancestor and not present in either parent, then
		// TODO            there won't be an LVI (or row in tbl_L0, tbl_PC, or tbl_L1) for it.
		// TODO            Normally __fetch_item__domain() returns (false,null) when something
		// TODO            isn't found and the callers in wc7txapi use that to throw _NOT_FOUND
		// TODO            and say it is an unknown/bogus item.  I want to distinguish this case
		// TODO            so that the generic message doesn't confuse the user.
		// TODO
		// TODO            For example, a 'vv status @a/foo' should ideally give a "Removed"
		// TODO            status for the item (and match a 'vv status -r ancestor @0/foo'),
		// TODO            but we have enough info right here.
		//
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_PRESENT_IN_PARENTS,
						(pCtx, "'%s'", pszInput)  );
	}

fail:
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRowA);
	SG_STRING_NULLFREE(pCtx, pStr);
	SG_TREENODE_NULLFREE(pCtx, ptnSuperRoot);
	SG_NULLFREE(pCtx, pszGid);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch the GID/alias/LVI for the named item.
 * The repo-path may refer to one of the valid
 * domains for a current/live repo-path.
 *
 * We assume that the given repo-path has already
 * been normalized/validated using
 * sg_wc_db__path__anything_to_repopath().
 * 
 */
void sg_wc_tx__liveview__fetch_item__domain(SG_context * pCtx, SG_wc_tx * pWcTx,
											const SG_string * pStringRepoPath,
											SG_bool * pbKnown,
											sg_wc_liveview_item ** ppLVI)
{
	SG_uint64 uiAlias;
	const char * psz;
	char * pszGidTemp = NULL;
	char chDomain;
#if TRACE_WC_TX_LVD
	SG_string * pStringRepoPath_Computed = NULL;
#endif

	SG_NULLARGCHECK( pStringRepoPath );
	SG_NULLARGCHECK( ppLVI );

	psz = SG_string__sz(pStringRepoPath);
	SG_ASSERT_RELEASE_FAIL( (psz[0]=='@') );
	chDomain = psz[1];

	switch (chDomain)
	{
	case SG_WC__REPO_PATH_DOMAIN__LIVE:	// '/' normal current/live-repo-path
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item(pCtx, pWcTx, pStringRepoPath, pbKnown, ppLVI)  );
		break;

	case SG_WC__REPO_PATH_DOMAIN__G:	// a GID. find the associated item whereever it happens to be in the WD.
		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pWcTx->pDb, &psz[1], &uiAlias)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAlias, pbKnown, ppLVI)  );
#if TRACE_WC_TX_LVD
		if (*pbKnown)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, *ppLVI, &pStringRepoPath_Computed)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "FetchItem: [domain %c] '%s' ==> '%s'\n",
									   chDomain,
									   SG_string__sz(pStringRepoPath),
									   SG_string__sz(pStringRepoPath_Computed))  );
		}
#endif
		break;

	case SG_WC__REPO_PATH_DOMAIN__T:	// a temporary GID for a found/ignored item *WHICH IS ONLY VALID DURING THE SAME TX*.
		SG_ERR_CHECK(  SG_STRDUP(pCtx, &psz[1], &pszGidTemp)  );
		pszGidTemp[0] = SG_WC__REPO_PATH_DOMAIN__G;
		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pWcTx->pDb, pszGidTemp, &uiAlias)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAlias, pbKnown, ppLVI)  );
#if TRACE_WC_TX_LVD
		if (*pbKnown)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, *ppLVI, &pStringRepoPath_Computed)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "FetchItem: [domain %c] '%s' ==> '%s'\n",
									   chDomain,
									   SG_string__sz(pStringRepoPath),
									   SG_string__sz(pStringRepoPath_Computed))  );
		}
#endif
		break;
		
	case SG_WC__REPO_PATH_DOMAIN__B:
		// a baseline-relative repo-path.
		// find GID and forward map to item in WD.
		SG_ERR_CHECK(  sg_wc_db__tne__get_alias_from_extended_repo_path(pCtx,
																		pWcTx->pDb,
																		pWcTx->pCSetRow_Baseline,
																		psz,
																		pbKnown, &uiAlias)  );
		if (!*pbKnown)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Could not find '%s' in baseline.", psz)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAlias, pbKnown, ppLVI)  );
#if TRACE_WC_TX_LVD
		if (*pbKnown)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, *ppLVI, &pStringRepoPath_Computed)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "FetchItem: [domain %c] '%s' ==> '%s'\n",
									   chDomain,
									   SG_string__sz(pStringRepoPath),
									   SG_string__sz(pStringRepoPath_Computed))  );
		}
#endif
		break;

	case SG_WC__REPO_PATH_DOMAIN__C:
		// an other-branch (post-merge) relative repo-path.
		// find GID and forward map to item in WD.
		// TODO 2012/02/14 See W3250.
		if (!pWcTx->pCSetRow_Other)
			SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
							(pCtx,
							 "Domain '%c' repo-paths are only defined when there is a pending merge: %s",
							 chDomain,
							 psz)  );
		
		SG_ERR_CHECK(  sg_wc_db__tne__get_alias_from_extended_repo_path(pCtx,
																		pWcTx->pDb,
																		pWcTx->pCSetRow_Other,
																		psz,
																		pbKnown, &uiAlias)  );
		if (!*pbKnown)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Could not find '%s' in other changeset.", psz)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, uiAlias, pbKnown, ppLVI)  );
#if TRACE_WC_TX_LVD
		if (*pbKnown)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, *ppLVI, &pStringRepoPath_Computed)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "FetchItem: [domain %c] '%s' ==> '%s'\n",
									   chDomain,
									   SG_string__sz(pStringRepoPath),
									   SG_string__sz(pStringRepoPath_Computed))  );
		}
#endif
		break;

	case SG_WC__REPO_PATH_DOMAIN__A:
		// a reference to a repo-path as it was in the ancestor.
		// find GID and forward map to item in WD.
		if (!pWcTx->pCSetRow_Other)
			SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
							(pCtx,
							 "Domain '%c' repo-paths are only defined when there is a pending merge: %s",
							 chDomain,
							 psz)  );
		SG_ERR_CHECK(  _fetch_A(pCtx, pWcTx, pStringRepoPath, pbKnown, ppLVI)  );
#if TRACE_WC_TX_LVD
		if (*pbKnown)
		{
			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, *ppLVI, &pStringRepoPath_Computed)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "FetchItem: [domain %c] '%s' ==> '%s'\n",
									   chDomain,
									   SG_string__sz(pStringRepoPath),
									   SG_string__sz(pStringRepoPath_Computed))  );
		}
#endif
		break;

	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Unsupported repo-path-domain '%s'.", psz)  );
	}

fail:
#if TRACE_WC_TX_LVD
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Computed);
#endif
	SG_NULLFREE(pCtx, pszGidTemp);
	return;
}

