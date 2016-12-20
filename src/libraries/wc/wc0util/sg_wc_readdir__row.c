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
 * @file sg_wc_readdir__row.c
 *
 * @details Routines to manipulate a sg_wc_readdir__row.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Use the info we already have from the prescan to compute
 * the absolute path for the given item.  This is in the pre-tx
 * namespace.
 *
 */
static void _get_prescan_path(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  sg_wc_readdir__row * pRow,
							  SG_pathname ** ppPath)
{
	SG_pathname * pPath = NULL;

	SG_ASSERT_RELEASE_RETURN(  (pRow->pPrescanRow)  );
	SG_ASSERT_RELEASE_RETURN(  (pRow->pPrescanRow->pPrescanDir_Ref)  );
	SG_ASSERT_RELEASE_RETURN(  (pRow->pPrescanRow->pPrescanDir_Ref->pStringRefRepoPath)  );

	SG_ERR_CHECK(  sg_wc_db__path__repopath_to_absolute(pCtx,
														pWcTx->pDb, 
														pRow->pPrescanRow->pPrescanDir_Ref->pStringRefRepoPath,
														&pPath)  );
	SG_ERR_CHECK(  SG_pathname__append__from_string(pCtx,
													pPath,
													pRow->pStringEntryname)  );

	*ppPath = pPath;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

/**
 * Compute the ATTRBITS *based strictly on the info we
 * received from stat() during our READDIR.
 *
 */
void sg_wc_readdir__row__get_attrbits(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_readdir__row * pRow)
{
	SG_uint64 attrbits = SG_WC_ATTRBITS__ZERO;

	if (!pWcTx->pDb->pAttrbitsData)
		SG_ERR_CHECK_RETURN(  SG_wc_attrbits__get_masks_from_config(pCtx,
																	pWcTx->pDb->pRepo,
																	pWcTx->pDb->pPathWorkingDirectoryTop,
																	&pWcTx->pDb->pAttrbitsData)  );

	if (pRow->pAttrbits)
		return;

	// the readdir loop already gave us the stat()
	// data.  we deferred computing the actual bits until we needed
	// them (and we were sure that the mask data could be loaded).

	SG_ERR_CHECK_RETURN(  sg_wc_attrbits__compute_from_perms(pCtx,
															 pWcTx->pDb->pAttrbitsData,
															 pRow->tneType,
															 pRow->pfsStat->perms,
															 &attrbits)  );

	// Yeah, alloc a ui64 and stuff the attrbits value into it.
	// This may be a bit expensive, but lets us know whether we
	// have already computed the value (which can also be expensive).

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pRow->pAttrbits)  );
	*pRow->pAttrbits = attrbits;
}

//////////////////////////////////////////////////////////////////

/**
 * The code in sg_wc_readdir__row__get_content_hid() calls
 * one of the _get_content_hid__file__{without,using}_tsc()
 * and records the hid/size in pRow->{pszHidContent,sizeContent}
 * so that subsequent requests don't need to re-hash the file.
 * This works fine for simple a simple 'vv status' type request
 * (where we basically take a snapshot of the tree and report
 * what we saw).  It can have problems when we hold open a
 * longer TX, such as in tortoise.  For example, Explorer might
 * ask us about each item in a directory -- one by one.  We
 * don't want the overhead of a full TX for each item; it'd be
 * quicker to start a single somewhat longer running read-only
 * TX and use it to answer the file-by-file questions and then
 * discard the TX after some interval.
 *
 * Because our TX will hold a lock on the wc.db, moves/renames
 * can't happen in other processes; but they don't happen that
 * often and they will probably wait on the lock just fine.
 *
 * However, nothing prevents the user from editing a file during
 * our longer TX.  So we have code here to check/discard the
 * cached values if we detect that the file has changed since
 * we computed it.
 *
 * See st_A2653.js
 * 
 */
static void _validate_cached_content_hid_and_size(SG_context * pCtx,
												  SG_wc_tx * pWcTx,
												  sg_wc_readdir__row * pRow)
{
	SG_pathname * pPath = NULL;
	SG_fsobj_stat fsStat;

	SG_ERR_CHECK(  _get_prescan_path(pCtx, pWcTx, pRow, &pPath)  );
	// TODO 2012/11/01 I think I want this to throw if the file is suddenly missing.
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &fsStat)  );
	if ((fsStat.mtime_ms != pRow->pfsStat->mtime_ms) || (fsStat.size != pRow->pfsStat->size))
	{
#if 1 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "File Content changed during TX, flushing cache for row: %s\n",
								   SG_pathname__sz(pPath))  );
#endif
		*pRow->pfsStat = fsStat;	// structure copy
		SG_NULLFREE(pCtx, pRow->pszHidContent);
		pRow->sizeContent = 0;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

static void _get_content_hid__file__without_tsc(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												sg_wc_readdir__row * pRow)
{
	SG_pathname * pPath = NULL;

	SG_ERR_CHECK(  _get_prescan_path(pCtx, pWcTx, pRow, &pPath)  );
	SG_ERR_CHECK(  sg_wc_compute_file_hid(pCtx, pWcTx, pPath, &pRow->pszHidContent, &pRow->sizeContent)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void _get_content_hid__file__using_tsc(SG_context * pCtx,
											  SG_wc_tx * pWcTx,
											  sg_wc_readdir__row * pRow)
{
	char * pszGid = NULL;
	const SG_timestamp_data * pTSData = NULL;		// we do not own this
	SG_bool bValid;

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb,
													 pRow->pPrescanRow->uiAliasGid,
													 &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__timestamp_cache__is_valid(pCtx, pWcTx->pDb,
													   pszGid, pRow->pfsStat,
													   &bValid, &pTSData)  );

#if TRACE_WC_READDIR
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "GetContentHid: [cache valid %d] %s\n",
							   bValid, SG_string__sz(pRow->pStringEntryname))  );
#endif

	if (bValid)
	{
		const char * pszHidRef;	// we do not own this
		SG_ERR_CHECK(  SG_timestamp_data__get_hid__ref(pCtx, pTSData, &pszHidRef)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidRef, &pRow->pszHidContent)  );
		SG_ERR_CHECK(  SG_timestamp_data__get_size(pCtx, pTSData, &pRow->sizeContent)  );
	}
	else
	{
		SG_ERR_CHECK(  _get_content_hid__file__without_tsc(pCtx, pWcTx, pRow)  );
		SG_ERR_CHECK(  sg_wc_db__timestamp_cache__set(pCtx, pWcTx->pDb,
													  pszGid, pRow->pfsStat,
													  pRow->pszHidContent)  );
	}

fail:
	SG_NULLFREE(pCtx, pszGid);
}

static void _get_content_hid__symlink(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_readdir__row * pRow)
{
	SG_pathname * pPath = NULL;

	SG_ERR_CHECK(  _get_prescan_path(pCtx, pWcTx, pRow, &pPath)  );

	SG_ERR_CHECK(  SG_fsobj__readlink(pCtx, pPath, &pRow->pStringSymlinkTarget)  );
	SG_ERR_CHECK(  SG_repo__alloc_compute_hash__from_string(pCtx,
															pWcTx->pDb->pRepo,
															pRow->pStringSymlinkTarget,
															&pRow->pszHidContent)  );
	pRow->sizeContent = (SG_uint64)SG_string__length_in_bytes(pRow->pStringSymlinkTarget);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Get the content HID of the item as it currently exists
 * in the working directory.  Optionally also return the
 * size of the item.
 *
 * If 'bNoTSC' we disregard the TimeStampCache and always
 * compute it the hard way.
 *
 * You DO NOT own the returned result.
 * 
 */
void sg_wc_readdir__row__get_content_hid(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 sg_wc_readdir__row * pRow,
										 SG_bool bNoTSC,
										 const char ** ppszHid,
										 SG_uint64 * pSize)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pRow );
	// ppszHid is optional
	// pSize is optional

	if (pRow->pszHidContent)
		SG_ERR_CHECK(  _validate_cached_content_hid_and_size(pCtx, pWcTx, pRow)  );

	if (pRow->pszHidContent)
	{
		// an earlier call to us computed the answer
		// and our cached value is still valid.
	}
	else
	{
		switch (pRow->tneType)
		{
		case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
			if (bNoTSC)
				SG_ERR_CHECK(  _get_content_hid__file__without_tsc(pCtx, pWcTx, pRow)  );
			else
				SG_ERR_CHECK(  _get_content_hid__file__using_tsc(pCtx, pWcTx, pRow)  );
			break;

		case SG_TREENODEENTRY_TYPE_DIRECTORY:
			// The content-HID of a directory isn't computed or available
			// until after the changeset is committed.
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "Cannot request content HID of a live directory. '%s'",
							 SG_string__sz(pRow->pStringEntryname))  );

		case SG_TREENODEENTRY_TYPE_SYMLINK:
			SG_ERR_CHECK(  _get_content_hid__symlink(pCtx, pWcTx, pRow)  );
			break;

		case SG_TREENODEENTRY_TYPE_SUBMODULE:
		default:
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED );
			break;

		case SG_TREENODEENTRY_TYPE__DEVICE:	// a FIFO, BlkDev, etc.
			// These will NEVER be under version control; the row is just
			// being maintained to remember the entryname for collision
			// purposes.
			pRow->pszHidContent = NULL;
			pRow->sizeContent = 0;
			break;
		}
	}

	if (ppszHid)
		*ppszHid = pRow->pszHidContent;
	if (pSize)
		*pSize = pRow->sizeContent;

fail:
	return;
}

void sg_wc_readdir__row__get_content_hid__owned(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												sg_wc_readdir__row * pRow,
												SG_bool bNoTSC,
												char ** ppszHid,
												SG_uint64 * pSize)
{
	const char * pszHid_ref = NULL;

	SG_ERR_CHECK_RETURN(  sg_wc_readdir__row__get_content_hid(pCtx, pWcTx, pRow, bNoTSC, &pszHid_ref, pSize)  );
	// TODO 2012/11/16 handle null pszHid_ref if tneType is __DEVICE ?
	SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, pszHid_ref, ppszHid)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Get the current symlink target of the item as it currently exists
 * in the working directory.
 *
 */
void sg_wc_readdir__row__get_content_symlink_target(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_readdir__row * pRow,
													const SG_string ** ppString)
{
	if (pRow->tneType != SG_TREENODEENTRY_TYPE_SYMLINK)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "GetContentSymlinkTarget: '%s' is not a symlink.",
						 SG_string__sz(pRow->pStringEntryname))  );

	SG_ERR_CHECK(  sg_wc_readdir__row__get_content_hid(pCtx, pWcTx, pRow, SG_FALSE, NULL, NULL)  );

	*ppString = pRow->pStringSymlinkTarget;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Convert the system pfsStat->type into a TneType.
 * 
 */
static void _set_tne_type(SG_context * pCtx,
						  sg_wc_readdir__row * pRow,
						  const SG_pathname * pPathDir)
{
	SG_fsobj_type t_fsobj = pRow->pfsStat->type;

	if (t_fsobj == SG_FSOBJ_TYPE__DIRECTORY)
	{
		// TODO 2011/08/17 Use (pPathDir,pRow->pStringEntryname)
		// TODO            to sniff inside the directory and look
		// TODO            for a sub-repo (aka sub-module).

		pRow->tneType = SG_TREENODEENTRY_TYPE_DIRECTORY;
	}
	else if (t_fsobj == SG_FSOBJ_TYPE__REGULAR)
	{
		pRow->tneType = SG_TREENODEENTRY_TYPE_REGULAR_FILE;
	}
	else if (t_fsobj == SG_FSOBJ_TYPE__SYMLINK)
	{
		pRow->tneType = SG_TREENODEENTRY_TYPE_SYMLINK;
	}
	else if (t_fsobj == SG_FSOBJ_TYPE__DEVICE)
	{
		pRow->tneType = SG_TREENODEENTRY_TYPE__DEVICE;
	}
	else
	{
		SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
							   (pCtx, "_set_tne_type: [t 0x%x] %s / %s",
								t_fsobj,
								SG_pathname__sz(pPathDir),
								SG_string__sz(pRow->pStringEntryname))  );
	}
}

//////////////////////////////////////////////////////////////////

void sg_wc_readdir__row__free(SG_context * pCtx, sg_wc_readdir__row * pRow)
{
	if (!pRow)
		return;

	SG_STRING_NULLFREE(pCtx, pRow->pStringEntryname);
	SG_FSOBJ_STAT_NULLFREE(pCtx, pRow->pfsStat);

	SG_NULLFREE(pCtx, pRow->pAttrbits);
	SG_NULLFREE(pCtx, pRow->pszHidContent);
	SG_STRING_NULLFREE(pCtx, pRow->pStringSymlinkTarget);

	SG_NULLFREE(pCtx, pRow);
}

void sg_wc_readdir__row__alloc(SG_context * pCtx,
							   sg_wc_readdir__row ** ppRow,
							   const SG_pathname * pPathDir,
							   const SG_string * pStringEntryname,
							   const SG_fsobj_stat * pfsStat)
{
	sg_wc_readdir__row * pRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pRow)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pRow->pStringEntryname, pStringEntryname)  );
	SG_ERR_CHECK(  SG_FSOBJ_STAT__ALLOC__COPY(pCtx, &pRow->pfsStat, pfsStat)  );

	// We DO NOT store the full pathname to the item in the row
	// (because parent directories can be moved/renamed and we
	// don't want to track that in all children).  We only use
	// it when converting the fsobj_type into a tne_type (because
	// we may have to sniff for a sub-repo (sub-module)).
	SG_ERR_CHECK(  _set_tne_type(pCtx, pRow, pPathDir)  );

	// Defer computing the ATTRBITS until someone actually needs them.
	pRow->pAttrbits = NULL;

	// Defer computing the CONTENT-HID until someone actually needs it.
	pRow->pszHidContent = NULL;
	pRow->sizeContent = 0;
	pRow->pStringSymlinkTarget = NULL;

	*ppRow = pRow;
	return;

fail:
	SG_WC_READDIR__ROW__NULLFREE(pCtx, pRow);
}

//////////////////////////////////////////////////////////////////

/**
 * Allocate a special READDIR_ROW for the ROOT directory.
 *
 */
void sg_wc_readdir__row__alloc__row_root(SG_context * pCtx,
										 sg_wc_readdir__row ** ppRow,
										 const SG_pathname * pPathWorkingDirectoryTop)
{
	sg_wc_readdir__row * pRow = NULL;
	SG_fsobj_stat statRoot;

	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPathWorkingDirectoryTop, &statRoot)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pRow)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pRow->pStringEntryname, "@")  );
	SG_ERR_CHECK(  SG_FSOBJ_STAT__ALLOC__COPY(pCtx, &pRow->pfsStat, &statRoot)  );

	// We DO NOT use _set_tne_type() for this because the answer
	// if obvious *and* we don't want it sniffing and incorrectly
	// thinking that we are a submodule.
	pRow->tneType = SG_TREENODEENTRY_TYPE_DIRECTORY;

	// Defer computing the ATTRBITS until someone actually needs them.
	pRow->pAttrbits = NULL;

	// Defer computing the CONTENT-HID until someone actually needs it.
	pRow->pszHidContent = NULL;
	pRow->sizeContent = 0;
	pRow->pStringSymlinkTarget = NULL;

	*ppRow = pRow;
	return;

fail:
	SG_WC_READDIR__ROW__NULLFREE(pCtx, pRow);
}

//////////////////////////////////////////////////////////////////

void sg_wc_readdir__row__alloc__copy(SG_context * pCtx,
									 sg_wc_readdir__row ** ppRow,
									 const sg_wc_readdir__row * pRow_Input)
{
	sg_wc_readdir__row * pRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pRow)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pRow->pStringEntryname, pRow_Input->pStringEntryname)  );
	SG_ERR_CHECK(  SG_FSOBJ_STAT__ALLOC__COPY(pCtx, &pRow->pfsStat, pRow_Input->pfsStat)  );
	pRow->tneType = pRow_Input->tneType;

	pRow->pPrescanRow = NULL;		// let caller fill this in if it wants it linked

	if (pRow_Input->pAttrbits)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pRow->pAttrbits)  );
		*pRow->pAttrbits = *pRow_Input->pAttrbits;
	}

	if (pRow_Input->pszHidContent)
	{
		SG_ERR_CHECK(  SG_strdup(pCtx, pRow_Input->pszHidContent, &pRow->pszHidContent)  );
		pRow->sizeContent = pRow_Input->sizeContent;
	}

	if (pRow_Input->pStringSymlinkTarget)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pRow->pStringSymlinkTarget, pRow_Input->pStringSymlinkTarget)  );
	}

	*ppRow = pRow;
	return;

fail:
	SG_WC_READDIR__ROW__NULLFREE(pCtx, pRow);
}

//////////////////////////////////////////////////////////////////

/**
 * compute HID and size of arbitrary file with the given pathname.
 * this DOES NOT know anything about any of the QUEUE/APPLY tricks.
 *
 */
void sg_wc_compute_file_hid(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const SG_pathname * pPath,
							char ** ppszHid,
							SG_uint64 * pui64Size)
{
	SG_file * pFile = NULL;
	SG_fsobj_stat statNow;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pPath );
	SG_NULLARGCHECK_RETURN( ppszHid );
	// pui64Size is optional

	if (pui64Size)
	{
		SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &statNow)  );
		*pui64Size = statNow.size;
	}

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath,
										   SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING,
										   SG_FSOBJ_PERMS__UNUSED,
										   &pFile)  );
	SG_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx, pWcTx->pDb->pRepo, pFile, ppszHid)  );

#if TRACE_WC_READDIR
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "ComputeFileHid: [%s] %s\n", *ppszHid, SG_pathname__sz(pPath))  );
#endif

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

void sg_wc_compute_file_hid__sz(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const char * pszFile,
								char ** ppszHid,
								SG_uint64 * pui64Size)
{
	SG_pathname * pPath = NULL;

	SG_NONEMPTYCHECK_RETURN( pszFile );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, pszFile)  );
	SG_ERR_CHECK(  sg_wc_compute_file_hid(pCtx, pWcTx, pPath, ppszHid, pui64Size)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}


