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
 * @file sg_wc_tx__apply__store_symlink.c
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

void sg_wc_tx__apply__store_symlink(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									const SG_vhash * pvh)
{
	SG_pathname * pPath = NULL;
	SG_string * pStringSymlink = NULL;
	const char * pszRepoPath;		// we do not own this
	const char * pszHidExpected;	// we do not own this
	char * pszHidObserved = NULL;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_int64 alias;
	SG_bool bKnown;
	SG_bool bDontBother_BlobEncoding;
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "src",   &pszRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "alias", &alias)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "hid",   &pszHidExpected)  );
	SG_ERR_CHECK(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__store_symlink: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pWcTx, alias, &bKnown, &pLVI)  );

	SG_ASSERT( (bSrcIsSparse == SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live)) );
	if (bSrcIsSparse)
	{
		// We've been asked to store the target of the symlink ***during a COMMIT***
		// and are given the *Expected-HID* (and we need to get the actual target
		// from the WD) and it is assumed that that will generate the same HID
		// that we were given.
		//
		// However, if the symlink is sparse (not populated) we can't do __readlink()
		// to get the (current) target.  So we have to
		// assume that we already have a blob in the repo for it.
		//
		// Since sparse items now have p_d_sparse dynamic data in tbl_PC, we assume
		// that whoever last modified the content of the symlink and set p_d_sparse->pszHid
		// also recorded the blob we need to be present now. (See __apply__overwrite_symlink())
		//
		// for sanity's sake verify that we already have this blob in the repo.

		SG_uint64 len = 0;
		SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pWcTx->pDb->pRepo, pszHidExpected,
												  SG_TRUE, NULL, NULL, NULL, &len, NULL)  );

		// so we don't need to do anything because we already
		// have a copy of this blob in the repo.
		return;
	}

	// We never bother compressing/encoding the symlink content
	// since it is so short.
	bDontBother_BlobEncoding = SG_TRUE;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx,
														   pWcTx->pDb,
														   pszRepoPath,
														   &pPath)  );

	SG_ERR_CHECK(  SG_fsobj__readlink(pCtx, pPath, &pStringSymlink)  );
	SG_ERR_CHECK(  SG_committing__add_bytes__string(pCtx,
													pWcTx->pCommittingInProgress,
													pStringSymlink,
													bDontBother_BlobEncoding,
													&pszHidObserved)  );

	// See note in __apply__store_file() about race condition.
	// If the HID computed now differs from what we thought
	// it should be, we lost the race.

	if (strcmp(pszHidObserved, pszHidExpected) != 0)
		SG_ERR_THROW2(  SG_ERR_ASSERT,
						(pCtx, "The symlink '%s' changed during the commit.",
						 pszRepoPath)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pStringSymlink);
	SG_NULLFREE(pCtx, pszHidObserved);
}

