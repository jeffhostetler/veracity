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
 * Construct a pathname to a personal TEMP directory within the WD
 * and hidden in .sgdrawer.  This directory will be used for the various
 * versions of a single file/item so that we can use external tools
 * during the AUTOMERGE and/or RESOLVE.
 * 
 * We want this temp directory to be local to the WD (so that files can be
 * moved/linked rather than copied -- both for input to diff3 and for the
 * auto-merge result -- IF THAT IS APPROPRIATE.  (And so that we don't have
 * to create a bunch of trash in the user's WD.)
 *
 * We want this TEMP directory to be for the duration of the MERGE/RESOLVE/
 * COMMIT effort.  That is, if MERGE needs to create a set of Ancestor/Mine/
 * Other files for an unresolved file conflict, RESOLVE will know where to
 * find them and COMMIT will clean up the mess.  This allows RESOLVE to have
 * a "redo-merge" option with a different tool, for example.  It also allows
 * us to keep the trash out of the working directory -- both for neatness
 * and to avoid having baggage if there is also a move/rename for the file.
 * 
 * We create a pathname and store it in the conflict structure
 * so that all versions of this file in this conflict can use it.
 *
 * The directory on disk should be deleted when this item has been committed
 * or reverted and the pvhIssue has been deleted.  (I'm not talking about
 * resolve here.)
 * 
 */
static void _sg_mrg__ensure_temp_dir_for_file_conflict(SG_context * pCtx,
													   SG_mrg * pMrg,
													   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict)
{
	SG_string * pString = NULL;
	const SG_byte * pszGid = (const SG_byte *)pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufGid_Entry;

	if (pMrgCSetEntryConflict->pPathTempDirForFile)
		return;

	// create something like: "<sgtemp>/<gid7>_YYYYMMDD_<k>/"

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pString, pszGid, 7)  );
	SG_ERR_CHECK(  SG_workingdir__generate_and_create_temp_dir_for_purpose(pCtx, pMrg->pWcTx->pDb->pPathWorkingDirectoryTop,
																		   SG_string__sz(pString),
																		   &pMrgCSetEntryConflict->pPathTempDirForFile)  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

/**
 * Create a pathname in the per-file temp-dir for one
 * version of the file.
 *
 * We use the ancestor version of the entryname
 * to avoid issues with pending renames.
 * 
 */
void _sg_mrg__create_pathname_for_conflict_file(SG_context * pCtx,
												SG_mrg * pMrg,
												SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
												const char * pszPrefix,
												SG_pathname ** ppPathReturned)
{
	SG_pathname * pPath = NULL;
	SG_string * pString = NULL;

	SG_ERR_CHECK(  _sg_mrg__ensure_temp_dir_for_file_conflict(pCtx, pMrg, pMrgCSetEntryConflict)  );

	// create something like: "<sgtemp>/<gid7>_YYYYMMDD_<k>/<prefix>~<entryname>"

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "%s~%s",
									  pszPrefix,
									  SG_string__sz(pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->pStringEntryname))  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath,
												   pMrgCSetEntryConflict->pPathTempDirForFile,
												   SG_string__sz(pString))  );

	SG_STRING_NULLFREE(pCtx, pString);
	*ppPathReturned = pPath;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

/**
 * Export the contents of the given file entry (which reflects a specific version)
 * into a temp file so that an external tool can use it.
 *
 * You own the returned pathname and the file on disk.
 */
void _sg_mrg__export_to_temp_file(SG_context * pCtx, SG_mrg * pMrg,
								  const char * pszHidBlob,
								  const SG_pathname * pPathTempFile)
{
	SG_file * pFile = NULL;
	SG_bool bExists;

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx,pPathTempFile,&bExists,NULL,NULL)  );
	if (bExists)
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "Skipping export of [blob %s] to [%s]\n",pszHidBlob,SG_pathname__sz(pPathTempFile))  );
#endif
	}
	else
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "Exporting          [blob %s] to [%s]\n",pszHidBlob,SG_pathname__sz(pPathTempFile))  );
#endif

		// Ideally, when we create this TEMP file it should be read-only.
		// Afterall, it does represent a historical version of the file and
		// it should only be used as INPUT to whatever merge tool the user
		// has configured.  So it should be read-only.  This might allow
		// a GUI merge tool to show locks/whatever and/or prevent accidental
		// editing of these files.
		//
		// However, this can cause an "Access is Denied" error on Windows
		// when we get ready to delete the contents of the TEMP directory.
		// We'll deal with that there rather than here.

		SG_ERR_CHECK(  SG_file__open__pathname(pCtx,pPathTempFile,SG_FILE_WRONLY|SG_FILE_CREATE_NEW,0400,&pFile)  );
		SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pMrg->pWcTx->pDb->pRepo, pszHidBlob,pFile,NULL)  );
		SG_FILE_NULLCLOSE(pCtx,pFile);
	}

	return;

fail:
	if (pFile)			// only if **WE** created the file, do we try to delete it on error.
	{
		SG_FILE_NULLCLOSE(pCtx,pFile);
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx,pPathTempFile)  );
	}
}

void _sg_mrg__copy_wc_to_temp_file(SG_context * pCtx, SG_mrg * pMrg,
								   SG_mrg_cset_entry * pMrgCSetEntry,
								   const SG_pathname * pPathTempFile)
{
	sg_wc_liveview_item * pLVI;
	SG_string * pStringRepoPath = NULL;
	SG_pathname * pPathInWC = NULL;
	SG_bool bExists;
	SG_bool bKnown;

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx,pPathTempFile,&bExists,NULL,NULL)  );
	if (bExists)
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "Skipping copy of WC version to [%s]\n",
								   SG_pathname__sz(pPathTempFile))  );
#endif
		return;
	}

	// Find the absolute path of the WD version of the file.
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry->uiAliasGid,
														 &bKnown, &pLVI)  );
	SG_ASSERT_RELEASE_FAIL(  (bKnown)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pMrg->pWcTx, pLVI,
															  &pStringRepoPath)  );
	SG_ERR_CHECK(  sg_wc_db__path__repopath_to_absolute(pCtx, pMrg->pWcTx->pDb,
														pStringRepoPath,
														&pPathInWC)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "Copying WC version [%s] to [%s]\n",
							   SG_string__sz(pStringRepoPath),
							   SG_pathname__sz(pPathTempFile))  );
#endif

	// Ideally, when we create this TEMP file it should be read-only.
	// Afterall, it does represent a historical version of the file and
	// it should only be used as INPUT to whatever merge tool the user
	// has configured.  So it should be read-only.  This might allow
	// a GUI merge tool to show locks/whatever and/or prevent accidental
	// editing of these files.
	//
	// However, this can cause an "Access is Denied" error on Windows
	// when we get ready to delete the contents of the TEMP directory.
	// We'll deal with that there rather than here.

	SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pPathInWC, pPathTempFile, 0400)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathInWC);
}
