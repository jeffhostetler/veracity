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
 * @file sg_wc_tx__apply__store_file.c
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
 * Try to store the contents of the file to the repo.
 *
 *
 * Note: There are a couple of ASSERTS in the code below
 * that complain about the file changing *during* the
 * commit:
 *     At the start of the COMMIT, we stat/hashed the file and/or
 *     used the TSC to get the HID of the content (to determine
 *     whether the file was dirty relative to the baseline).
 *    
 *     NOW we just copied the contents of the file to the repo
 *     and computed a hash on what we stored.
 *    
 *     Since the file can be edited behind our back (and at
 *     any time during our commit), those 2 values could be
 *     different if another process was able to modify the
 *     file in between those 2 points in time.
 *    
 *     So we always have the potential for a race condition.
 *     (Obscure? Yes, but aren't all race conditions?)
 *    
 *     TODO 2011/11/16 How hard do we want to complain about this?
 *     TODO            On one hand, we have a "slightly more dirty"
 *     TODO            file in the changeset.  So it doesn't matter.
 *     TODO
 *     TODO            On the other hand, they could have undone
 *     TODO            all of their edits, so that this file is now
 *     TODO            clean.  And if this were the only change, 
 *     TODO            this new found cleanliness could bubble-up
 *     TODO            and cause a trivial changeset to be created.
 *     TODO
 *     TODO            So for now, throw up.
 *
 */ 
void sg_wc_tx__apply__store_file(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const SG_vhash * pvh)
{
	SG_pathname * pPath = NULL;
	SG_file * pFile = NULL;
	const char * pszRepoPath;		// we do not own this
	const char * pszHidExpected;	// we do not own this
	char * pszHidObserved = NULL;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_int64 alias;
	SG_fsobj_stat statNow;
	SG_bool bKnown;
	SG_bool bDontBother_BlobEncoding;
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "src",   &pszRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "alias", &alias)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "hid",   &pszHidExpected)  );
	SG_ERR_CHECK(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__store_file: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, alias, &bKnown, &pLVI)  );
	SG_ASSERT( (bSrcIsSparse == SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live)) );
	if (bSrcIsSparse)
	{
		// We've been asked to store the contents of this file
		// and it is assumed that that will generate the same
		// HID that we were given.
		//
		// If the file is sparse (not populated) we have to
		// assume that we already have a blob in the repo for it.
		// That is, we don't currently allow arbitrary editing of
		// the target of a non-populated file.  (That is, MERGE
		// and RESOLVE might pick from a set of values and that's
		// fine, but if auto-merge is involved we shouldn't have
		// gotten here.)
		//
		// for sanity's sake verify that we already have this blob in the repo.
#if defined(DEBUG)
		SG_uint64 len = 0;
		SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pWcTx->pDb->pRepo, pszHidExpected,
												  SG_TRUE, NULL, NULL, NULL, &len, NULL)  );
#endif
		// so we don't need to do anything because we already
		// have a copy of this blob in the repo.
		return;
	}

	// TODO 2011/11/16 Consider using the file suffix to decide whether
	// TODO            or not to compress/encode the content of the file
	// TODO            as we store it.
	bDontBother_BlobEncoding = SG_FALSE;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx,
														   pWcTx->pDb,
														   pszRepoPath,
														   &pPath)  );

	// compare the stat() value computed during the STATUS
	// with the actual current values.
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &statNow)  );
	if ((statNow.mtime_ms != pLVI->pPrescanRow->pRD->pfsStat->mtime_ms)
		|| (statNow.size != pLVI->pPrescanRow->pRD->pfsStat->size))
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "The file '%s' changed during the commit.",
						 pszRepoPath)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath,
										   SG_FILE_LOCK | SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING,
										   SG_FSOBJ_PERMS__UNUSED,
										   &pFile)  );
	SG_ERR_CHECK(  SG_committing__add_file(pCtx,
										   pWcTx->pCommittingInProgress,
										   bDontBother_BlobEncoding,
										   pFile,
										   &pszHidObserved)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);

	// If the HID computed now differs from what we
	// thought it should be, we lost the race.

	if (strcmp(pszHidObserved, pszHidExpected) != 0)
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "The file '%s' changed during the commit.",
						 pszRepoPath)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, pszHidObserved);
}
