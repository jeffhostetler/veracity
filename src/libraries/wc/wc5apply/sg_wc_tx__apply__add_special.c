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

/**
 * Fetch a file from the repo into the WD.
 * This is used, for example, by MERGE when
 * the other branch has created a file that
 * needs to be added to the merge-result.
 *
 */
void sg_wc_tx__apply__add_special__file(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh)
{
	const char * pszRepoPath;
	const char * pszGid;
	const char * pszHidBlob;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_file * pFile = NULL;
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "gid",            &pszGid)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "hid",            &pszHidBlob)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__add_special__file: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	if (bSrcIsSparse)
		return;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath,  &pPath)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
										   pPath,
										   SG_FILE_WRONLY|SG_FILE_CREATE_NEW,
										   0644,
										   &pFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pWcTx->pDb->pRepo, pszHidBlob, pFile, NULL)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);

	SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );

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
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

/**
 * Fetch a directory from the repo into the WD.
 * (Actually this just means we need to create
 * a directory with the given attrbits.)
 *
 */
void sg_wc_tx__apply__add_special__directory(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 const SG_vhash * pvh)
{
	const char * pszRepoPath;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	// "hid" field isn't needed
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__add_special__directory: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	if (bSrcIsSparse)
		return;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath,  &pPath)  );

	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );

	SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Fetch a symlink from the repo into the WD.
 *
 */
void sg_wc_tx__apply__add_special__symlink(SG_context * pCtx,
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
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "hid",            &pszHidBlob)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__add_special__symlink: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	if (bSrcIsSparse)
		return;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath,  &pPath)  );

	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo, pszHidBlob, &pBytes, &nrBytes)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pStringLink, pBytes, (SG_uint32)nrBytes)  );
	SG_NULLFREE(pCtx, pBytes);

	SG_ERR_CHECK(  SG_fsobj__symlink(pCtx, pStringLink, pPath)  );

	SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pStringLink);
	SG_NULLFREE(pCtx, pBytes);
}

//////////////////////////////////////////////////////////////////

/**
 * Overwrite the contents of a file in the WD
 * with the contents of the given file.  We use
 * this after a merge, for example, to install
 * an automerge result file into the WD.
 *
 */
void sg_wc_tx__apply__overwrite_file_from_file(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const SG_vhash * pvh)
{
	const char * pszRepoPath;
	const char * pszFile;
	const char * pszGid;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_pathname * pPathFileToCopy = NULL;
	SG_bool bSameFile;

	// TODO 2012/12/20 put src_sparse value in this vhash and verify !sparse....

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "gid",            &pszGid)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "file",           &pszFile)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__overwrite_file_from_file: '%s'\n"),
							   pszRepoPath)  );
#endif

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathFileToCopy, pszFile)  );

	// WARNING We DO NOT do backups here.  We don't have
	// WARNING enough information at this level to know
	// WARNING whether we should and/or where it should
	// WARNING be placed.
	//
	// Secretly delete the WD file and copy over the
	// given file.
	//
	// [] I say "secretly" because I'm not telling the WC
	//    layer that we're doing this.  Our goal here is
	//    only to replace the contents of the file.
	//
	// [] I say "copy" rather than move/rename because the
	//    new RESOLVE code wants to keep the automerge
	//    result in the temp directory with the other
	//    versions of the file.

	//////////////////////////////////////////////////////////////////
	// One last sanity check.  If both pathnames refer
	// to the SAME FILE, WE SKIP THE ACTUAL COPY
	// so that we don't accidentally truncate the file.
	// 
	// I'm not sure that this case is possible unless
	// maybe if RESOLVE does an 'accept working'.
	//
	// (Ideally we should have done the shortcut inside
	// RESOLVE code, but it doesn't hurt to double-check
	// here.)

	SG_ERR_CHECK(  SG_pathname__equal_ignoring_final_slashes(pCtx,
															 pPathFileToCopy,
															 pPath,
															 &bSameFile)  );
	if (!bSameFile)
	{
		SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
		SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pPathFileToCopy, pPath, 0644)  );

		// Since we altered the content of the file, flush the
		// TimeStampCache entry for this item.

		SG_ERR_CHECK(  sg_wc_db__timestamp_cache__remove(pCtx, pWcTx->pDb, pszGid)  );
	}
	
	SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );


fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathFileToCopy);
}

/**
 * Overwrite the contents of a file in the WD
 * with the contents of a file in the repo.  We
 * use this after a merge, for example, to
 * update the file when it was modified in the
 * other branch.
 *
 */
void sg_wc_tx__apply__overwrite_file_from_repo(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const SG_vhash * pvh)
{
	const char * pszRepoPath;
	const char * pszHidBlob;
	const char * pszGid;
	const char * pszBackupPath = NULL;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_file * pFile = NULL;
	SG_pathname * pPathBackup = NULL;
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "gid",            &pszGid)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(   pCtx, pvh, "hid",            &pszHidBlob)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK(  SG_vhash__check__sz( pCtx, pvh, "backup_path",    &pszBackupPath)  );
	SG_ERR_CHECK(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

	if (pszBackupPath)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathBackup, pszBackupPath)  );

#if TRACE_WC_TX_APPLY
	if (pPathBackup)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__apply__overwrite_file_from_repo: '%s' [backup '%s'][hid %s][sparse %d]\n"),
								   pszRepoPath, SG_pathname__sz(pPathBackup), pszHidBlob, bSrcIsSparse)  );
	else
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__apply__overwrite_file_from_repo: '%s' [backup no][hid %s][sparse %d]\n"),
								   pszRepoPath, pszHidBlob, bSrcIsSparse)  );
#endif

	if (bSrcIsSparse)
		goto done;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );

	if (pPathBackup)
	{
		SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath, pPathBackup)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
	}
	
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
										   pPath,
										   SG_FILE_WRONLY|SG_FILE_CREATE_NEW,
										   0644,
										   &pFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pWcTx->pDb->pRepo, pszHidBlob, pFile, NULL)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);

	// Since we altered the content of the file, flush the
	// TimeStampCache entry for this item.

	SG_ERR_CHECK(  sg_wc_db__timestamp_cache__remove(pCtx, pWcTx->pDb, pszGid)  );

	SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );

done:
	;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathBackup);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

//////////////////////////////////////////////////////////////////

/**
 * We get requested from either __journal__overwrite_symlink()
 * or __journal__overwrite_symlink_from_repo().  We are now given
 * both HID and TARGET.
 *
 */
void sg_wc_tx__apply__overwrite_symlink(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh)
{
	const char * pszRepoPath;
	const char * pszTarget = NULL;
	const char * pszHidExpected = NULL;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_string * pStringLink = NULL;
	SG_bool bSrcIsSparse;
	SG_repo_tx_handle* pRepoTx = NULL;
	char * pszHidComputed = NULL;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "target",         &pszTarget)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "hid",            &pszHidExpected)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__overwrite_symlink: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	if (bSrcIsSparse)
	{
		// See note above __journal__overwrite_symlink().  When sparse
		// we want to ensure that {hid,target} is in the repo because
		// the p_d_sparse dynamic data fields in tbl_PC only record HID
		// and not the target/blob and we don't have any other way of
		// recording what the target is of this sparse (non-populated)
		// symlink is.  This runs the risk of having an unreferenced blob
		// if they later change the target again before they commit,
		// but that is ok.

		SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pWcTx->pDb->pRepo, &pRepoTx)  );
		SG_ERR_CHECK(  SG_repo__store_blob_from_memory(pCtx, pWcTx->pDb->pRepo, pRepoTx,
													   SG_TRUE,
													   (const SG_byte *)pszTarget,
													   SG_STRLEN(pszTarget),
													   &pszHidComputed)  );
		SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pWcTx->pDb->pRepo, &pRepoTx)  );

		if (strcmp(pszHidComputed, pszHidExpected) != 0)
			SG_ERR_THROW2(  SG_ERR_ASSERT,
							(pCtx, "Computed a different HID (%s) for symlink '%s' than expected (%s).",
							 pszHidComputed, pszRepoPath, pszHidExpected)  );

		// Since the symlink is sparse, we don't need to update the target
		// value in the WD.
	}
	else
	{
		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );

		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLink, pszTarget)  );

		// We can't replace a symlink target without deleting and recreating it.

		SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
		SG_ERR_CHECK(  SG_fsobj__symlink(pCtx, pStringLink, pPath)  );

		SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pStringLink);
	if (pRepoTx)
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pWcTx->pDb->pRepo, &pRepoTx)  );
	SG_NULLFREE(pCtx, pszHidComputed);
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__set_attrbits(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_vhash * pvh)
{
	const char * pszRepoPath;
	SG_int64 attrbits;
	SG_pathname * pPath = NULL;
	SG_bool bSrcIsSparse;

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(   pCtx, pvh, "src",            &pszRepoPath)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, "attrbits",       &attrbits)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__bool( pCtx, pvh, "src_sparse",     &bSrcIsSparse)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__set_attrbits: '%s' [src-sparse %d]\n"),
							   pszRepoPath, bSrcIsSparse)  );
#endif

	if (bSrcIsSparse)
		return;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );
	SG_ERR_CHECK(  SG_attributes__bits__apply(pCtx, pPath, attrbits)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

