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

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__undo_delete__file(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh)
{
	const char * pszRepoPath;
	const char * pszGid;
	const char * pszHidBlob = NULL;
	const char * pszRepoPathTempFile = NULL;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_file * pFile = NULL;
	SG_pathname * pPathTempFile = NULL;
	SG_bool bMakeSparse = SG_FALSE;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "gid",            &pszGid)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__check__sz( pCtx, pvh, "hid",            &pszHidBlob)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__check__sz( pCtx, pvh, "tempfile",       &pszRepoPathTempFile)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "make_sparse",    &bMakeSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__undo_delete__file: '%s' (%s,%s) (sparse %d)\n"),
							   pszRepoPath,
							   ((pszHidBlob) ? pszHidBlob : "null"),
							   ((pszRepoPathTempFile) ? pszRepoPathTempFile : "null"),
							   bMakeSparse)  );
#endif

	if (!bMakeSparse)
	{
		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath,  &pPath)  );

		if (pszHidBlob)
		{
			SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
												   pPath,
												   SG_FILE_WRONLY|SG_FILE_CREATE_NEW,
												   0644,
												   &pFile)  );
			SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pWcTx->pDb->pRepo, pszHidBlob, pFile, NULL)  );
			SG_FILE_NULLCLOSE(pCtx, pFile);
		}
		else if (pszRepoPathTempFile)
		{
			SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPathTempFile,  &pPathTempFile)  );
			SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pPathTempFile, pPath, 0644)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "undo_delete %s", pszRepoPath)  );
		}
		
		SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );
	}

	// Since this item is new (as far as the baseline/WD are
	// concerned), it SHOULD NOT have an entry in the TimeStampCache.
	// And we DO NOT want to create one for it now.
	// 
	// Yes, we have the info to set it, but because of the clock
	// precision problem, we discard/don't-create TSC entries anytime
	// that we alter the contents or create a file.  (Only when we
	// observe/scan an entry after the clock-tick grace period do
	// we want to set the cache entry.)

	SG_ERR_CHECK(  sg_wc_db__timestamp_cache__remove(pCtx, pWcTx->pDb, pszGid)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathTempFile);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

void sg_wc_tx__apply__undo_delete__directory(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 const SG_vhash * pvh)
{
	const char * pszRepoPath;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_bool bMakeSparse = SG_FALSE;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	// "hid" field isn't needed
	// "tempfile" field isn't needed
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "make_sparse",    &bMakeSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__undo_delete__directory: '%s' (sparse %d)\n"),
							   pszRepoPath, bMakeSparse)  );
#endif

	if (!bMakeSparse)
	{
		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath,  &pPath)  );
		SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );
		SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void sg_wc_tx__apply__undo_delete__symlink(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_vhash * pvh)
{
	const char * pszRepoPath;
	const char * pszHidBlob;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_string * pStringLink = NULL;
	SG_byte * pBytes = NULL;
	SG_uint64 nrBytes;
	SG_bool bMakeSparse = SG_FALSE;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "hid",            &pszHidBlob)  );
	// "tempfile" field isn't needed
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "make_sparse",    &bMakeSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__undo_delete__symlink: '%s' (sparse %d)\n"),
							   pszRepoPath, bMakeSparse)  );
#endif

	if (!bMakeSparse)
	{
		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath,  &pPath)  );

		SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo, pszHidBlob, &pBytes, &nrBytes)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pStringLink, pBytes, (SG_uint32)nrBytes)  );
		SG_NULLFREE(pCtx, pBytes);

		SG_ERR_CHECK(  SG_fsobj__symlink(pCtx, pStringLink, pPath)  );

		SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pStringLink);
	SG_NULLFREE(pCtx, pBytes);
}

