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
 * @file sg_wc_tx__rp__add.c
 *
 * @details Handle details of a 'vv add'
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Try to ADD this item -- by itself.
 * If the item is a directory, we DO NOT
 * worry about the contents.
 *
 */
static void _try_to_add_it(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   sg_wc_liveview_item * pLVI)
{
	sg_wc_liveview_item * pLVI_parent;		// we do not own this
	SG_wc_status_flags flags_parent;
	SG_bool bFound_parent;
	SG_bool bDisregardUncontrolledItems;
	
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_ROOT(pLVI->scan_flags_Live))
	{
		// They are trying to ADD the root directory.
		// Not sure that is it possible to actually get
		// here, but if we need to make sure we don't
		// try to inspect the parent directory.
	}
	else
	{
		// When looking back at the parent directory from
		// the given item, we don't need to qualify the
		// parent as FOUND vs IGNORED.  That stuff only
		// needs to happen when diving and evaluating a
		// list of arguments to add.  Here we are looking
		// at the item and need only to see if the parent
		// is controlled before we can allow the item to
		// become controlled.
		SG_bool bNoIgnores_ParentDontCare = SG_TRUE;
		// Also because the parent is a directory (and the
		// TSC only holds info for files) the TSC flag does
		// not matter.
		SG_bool bNoTSC_DoesNotApply = SG_TRUE;

		// Fetch the LVI of the parent directory and
		// get its status.  We require that the parent
		// be controlled and in good standing before
		// we allow an item to be added to it.

		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx,
															 pLVI->pLiveViewDir->uiAliasGidDir,
															 &bFound_parent,
															 &pLVI_parent)  );
		// The parent should be found because we have a LVI for this item.
		SG_ASSERT_RELEASE_FAIL(  (bFound_parent)  );
		SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI_parent,
													bNoIgnores_ParentDontCare,
													bNoTSC_DoesNotApply,
													&flags_parent)  );
		// This isn't possible because we did said DontCare.
		SG_ASSERT_RELEASE_FAIL(  ((flags_parent & SG_WC_STATUS_FLAGS__U__IGNORED) == 0)  );
		// The following aren't possible because a FOUND LVI
		// would not have been found within a missing/deleted
		// directory.
		SG_ASSERT_RELEASE_FAIL(  ((flags_parent & SG_WC_STATUS_FLAGS__U__LOST) == 0)  );
		SG_ASSERT_RELEASE_FAIL(  ((flags_parent & SG_WC_STATUS_FLAGS__S__DELETED) == 0)  );

		if (flags_parent & SG_WC_STATUS_FLAGS__A__SPARSE)
		{
			// TODO 2011/10/19 If the parent directory is sparse (not-populated)
			// TODO            we cannot add something to it.  They will need to
			// TODO            backfill more of the view first; we don't automatically
			// TODO            do that.
			// TODO
			// TODO            Actually, I'm not sure that this case is possible.
			// TODO            I mean before we can ADD it they have to have created it
			// TODO            on disk (and the parent directory) and if they did that
			// TODO            the parent directory would have gotten flagged for as duplicate
			// TODO            (one entry in the LVD with status FOUND and one entry
			// TODO            with status SPARSE).
		}

		if (flags_parent & SG_WC_STATUS_FLAGS__U__FOUND)
		{
			// The parent directory is not controlled.  Force
			// ADD it.  This wll recursively bubble back up if
			// necessary to add containing parent directories.
			// 
			// The ADD of the parent must not be recursive
			// because we don't want peers of this item to get
			// added; we just want the parent chain.
			//
			// We need the parent add to happen regardless of
			// whether it would have been ignored in a regular
			// top-down add.

			SG_ERR_CHECK(  sg_wc_tx__rp__add__lvi(pCtx, pWcTx, pLVI_parent, 0, SG_TRUE)  );
			SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI_parent,
														bNoIgnores_ParentDontCare,
														bNoTSC_DoesNotApply,
														&flags_parent)  );
			SG_ASSERT_RELEASE_FAIL(  ((flags_parent & SG_WC_STATUS_FLAGS__S__ADDED) != 0)  );
		}

		// See if we could ADD the entryname of this item to the
		// parent directory.  (Obviously, the file system thinks
		// it can because SCANDIR/READDIR found it.)  We are also
		// looking for PORTABILITY problems.
		//
		// We pass pLVI as the src to get it excluded from the
		// collider.
		//
		// We set bDisregardUncontrolledItems because we only want
		// to see if the name we want to add potentially collides
		// with something already under version control -- and not
		// any dirt they happen to have in the WD.

		bDisregardUncontrolledItems = SG_TRUE;
		SG_ERR_CHECK(  sg_wc_liveview_dir__can_add_new_entryname(pCtx,
																 pWcTx->pDb,
																 pLVI->pLiveViewDir,
																 pLVI,
																 NULL,
																 SG_string__sz(pLVI->pStringEntryname),
																 pLVI->tneType,
																 bDisregardUncontrolledItems)  );
	}

	// QUEUE the ADD (both in the in-memory LiveView and in the Journal).

	SG_ERR_CHECK(  sg_wc_tx__queue__add(pCtx, pWcTx, pLVI)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

struct _dive_data
{
	SG_wc_tx * pWcTx;
	SG_uint32 depth;
	SG_bool bNoIgnores;
};

static SG_rbtree_ui64_foreach_callback _dive_cb;

static void _dive_cb(SG_context * pCtx,
					 SG_uint64 uiAliasGid_k,
					 void * pVoid_LVI_k,
					 void * pVoid_DiveData)
{
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI_k;
	struct _dive_data * pDiveData = (struct _dive_data *)pVoid_DiveData;

	SG_UNUSED( uiAliasGid_k );

#if TRACE_WC_TX_ADD
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__rp__add: considering nested add of '%s' [scan_flags_Live %x]\n",
							   SG_string__sz(pLVI->pStringEntryname),
							   pLVI->scan_flags_Live)  );
#endif

	// We were told to ADD the parent directory and
	// recursively add the contents of that directory.

	// Since we are in a FOREACH we will see both
	// present/active and and non-present/inactive/deleted
	// items.  We need to weed the inactive ones out so
	// that we don't try to add them.

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI->scan_flags_Live))
		return;

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI->scan_flags_Live))
		return;

	SG_ERR_CHECK_RETURN(  sg_wc_tx__rp__add__lvi(pCtx,
												 pDiveData->pWcTx,
												 pLVI,
												 pDiveData->depth,
												 pDiveData->bNoIgnores)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__add__lvi(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							sg_wc_liveview_item * pLVI,
							SG_uint32 depth,
							SG_bool bNoIgnores)
{
	SG_string * pString = NULL;
	SG_wc_status_flags flags;
	// because we are ADDING things, any FILES we touch
	// are new and therefore are not in the TSC, so it
	// doesn't matter.
	SG_bool bNoTSC_DoesNotApply = SG_TRUE;

	// Go ahead and compute the full status flags for this item.
	// This will consistently do the classification of FOUND vs
	// IGNORED vs whatever.

	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx,
												pLVI,
												bNoIgnores,
												bNoTSC_DoesNotApply,
												&flags)  );

#if TRACE_WC_TX_ADD
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__rp__add: considering '%s' [status 0x%08x]\n",
							   SG_string__sz(pLVI->pStringEntryname),
							   (SG_uint32)flags)  );
#endif

	if (flags & SG_WC_STATUS_FLAGS__R__RESERVED)
		return;

	if (flags & SG_WC_STATUS_FLAGS__T__DEVICE)
	{
		SG_ASSERT(  (flags & (SG_WC_STATUS_FLAGS__U__FOUND|SG_WC_STATUS_FLAGS__U__IGNORED))  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pString)  );
		SG_ERR_THROW2(  SG_ERR_UNSUPPORTED_DEVICE_SPECIAL_FILE,
						(pCtx, "%s", SG_string__sz(pString))  );
	}

	if (flags & SG_WC_STATUS_FLAGS__U__IGNORED)
		return;

	if (flags & SG_WC_STATUS_FLAGS__U__LOST)
		return;

	// If the item is FOUND, we try to add it -- by itself.
	// We don't worry about recursion at this point.

	if (flags & SG_WC_STATUS_FLAGS__U__FOUND)
		SG_ERR_CHECK(  _try_to_add_it(pCtx, pWcTx, pLVI)  );

	// For both FOUND and CONTROLLED (active, non-LOST) directories,
	// we want to dive in and ADD their contents (if they requested it).

	if (flags & SG_WC_STATUS_FLAGS__T__DIRECTORY)
	{
		if (depth > 0)
		{
			sg_wc_liveview_dir * pLVD;		// we do not own this
			struct _dive_data dive_data;

#if TRACE_WC_TX_ADD
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "SG_wc_tx__add: diving into '%s'\n",
									   SG_string__sz(pLVI->pStringEntryname))  );
#endif

			// Get the contents of the directory (implicitly causing
			// scandir/readdir as necessary).

			SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
			dive_data.pWcTx = pWcTx;
			dive_data.depth = depth - 1;
			dive_data.bNoIgnores = bNoIgnores;
			SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _dive_cb, &dive_data)  );
		}
	}

	// Silently disregard items already under version control.
	// This lets them do 'vv add *' without getting a bunch of
	// complaints about duplicate adds.

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}
