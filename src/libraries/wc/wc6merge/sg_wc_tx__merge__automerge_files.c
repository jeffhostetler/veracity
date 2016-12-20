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

struct _amf_data
{
	SG_mrg *				pMrg;
	SG_mrg_cset *			pMrgCSet;
};

typedef struct _amf_data _amf_data;

//////////////////////////////////////////////////////////////////

/**
 * When doing a MERGE we run the "merge" version of the
 * tool (as opposed to the "resolve" version of the tool).
 * Internal merge tools are fully automatic.  External
 * merge tools may (gnu diff3) or may not (diffmerge) be.
 * We recommend that they only install manual tools in the
 * resolve phase, but there is nothing to enforce that.
 * 
 * If the merge-phase tool is fully automatic, then the
 * merge-result file is fully generated and is DISPOSABLE
 * if they later revert the merge.
 *
 * If the merge-phase tool asked them for help (or we can't
 * tell if it did), then we say that the merge-result file
 * is not disposable.
 * 
 */
static void _sg_mrg__is_automatic_filetool(SG_context * pCtx,
										   SG_filetool * pMergeTool,
										   SG_bool * pbIsResultFileDisposable)
{
	// TODO 2011/03/03 Consider adding a flag to the actual filetool
	// TODO            indicate if it is interactive/automatic.
	// TODO
	// TODO            For now we just assume that internal tools
	// TODO            are automatic and external ones are not.

	const char * pszExe = NULL;
	SG_bool bInternal;

	SG_ERR_CHECK_RETURN(  SG_filetool__get_executable(pCtx, pMergeTool, &pszExe)  );
	bInternal = ((pszExe == NULL) || (*pszExe == 0));

	*pbIsResultFileDisposable = (bInternal);
}

/**
 * If auto-merge was successful or generated a conflict file
 * and the auto-merge tool is fully automatic, I want to declare
 * that the merge-result file is "disposable".
 *
 * If for example, the user does a REVERT we don't really need
 * to make a backup of the content.
 *
 */
static void _sg_mrg__compute_automerge_hid(SG_context * pCtx,
										   SG_mrg * pMrg,
										   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict)
{
	SG_file * pFile = NULL;
	SG_bool bIsFullyAutomatic = SG_FALSE;
	SG_bool bCreatedResultFile;

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx,
											  pMrgCSetEntryConflict->pPathTempFile_Result,
											  &bCreatedResultFile, NULL, NULL)  );
	if (bCreatedResultFile)
	{
		if (pMrgCSetEntryConflict->pMergeTool)
		{
			// We get called when __AUTO_OK or __AUTO_CONFLICT.
			// 
			// When we generated a result file (whether good or with
			// conflict markers) remember the HID so that we can
			// write it to the issue.automerge_generated_hid so that
			// STATUS and/or MSTATUS can give better answers).

			SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
												   pMrgCSetEntryConflict->pPathTempFile_Result,
												   SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING,
												   SG_FSOBJ_PERMS__UNUSED,
												   &pFile)  );
			SG_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx,
																  pMrg->pWcTx->pDb->pRepo,
																  pFile,
																  &pMrgCSetEntryConflict->pszHidGenerated)  );
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "_sg_mrg__compute_automerge_hid: %s %s\n",
									   pMrgCSetEntryConflict->pszHidGenerated,
									   SG_pathname__sz(pMrgCSetEntryConflict->pPathTempFile_Result))  );
#endif

			// If the merge tool is fully automatic (and we could delete the
			// file on a REVERT without backing it upt), set the "disposable" HID.

			SG_ERR_CHECK(  _sg_mrg__is_automatic_filetool(pCtx,
														  pMrgCSetEntryConflict->pMergeTool,
														  &bIsFullyAutomatic)  );
			if (bIsFullyAutomatic)
			{
				SG_ERR_CHECK(  SG_strdup(pCtx,
										 pMrgCSetEntryConflict->pszHidGenerated,
										 &pMrgCSetEntryConflict->pszHidDisposable)  );
			}
		}
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

//////////////////////////////////////////////////////////////////

/**
 * Fetch a copy of each version of the file (ancestor and each
 * branch/leaf) into our private temp-dir.
 *
 * These are written to disk to allow an external mergetool to
 * use them and write the merge-result to the same directory.
 *
 * Even if we select the internal the builtin core diff3 engine for
 * the automerge, we still want them written to temp files in case
 * of conflicts and/or so that the user can re-merge them using
 * a GUI tool during a RESOLVE.
 *
 * We do all of this (even the merge-result file) in a private
 * temp directory so that 'vv merge --test' can completely run
 * without trashing/modifying their working directory.
 *
 */
static void _sg_mrg__fetch_file_versions_into_temp_files(SG_context * pCtx,
														 SG_mrg * pMrg,
														 SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict)
{
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey_k;
	const SG_vector * pVec_k;
	SG_uint32 nrUnique;
	SG_bool bFound;

	SG_ERR_CHECK(  SG_rbtree__count(pCtx, pMrgCSetEntryConflict->prbUnique_File_HidBlob, &nrUnique)  );
	if (nrUnique != 2)
		SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
							   (pCtx, "TODO figure out how to layout temp files when more than 2 versions of the file.")  );

	// create and record pathnames for the 3 inputs and the 1 result file.

	SG_ERR_CHECK(  _sg_mrg__create_pathname_for_conflict_file(pCtx, pMrg, pMrgCSetEntryConflict,
															  pMrg->pMrgCSet_LCA->pszMnemonicName,
															  &pMrgCSetEntryConflict->pPathTempFile_Ancestor)  );
	SG_ERR_CHECK(  _sg_mrg__create_pathname_for_conflict_file(pCtx, pMrg, pMrgCSetEntryConflict,
															  pMrg->pMrgCSet_Baseline->pszMnemonicName,
															  &pMrgCSetEntryConflict->pPathTempFile_Baseline)  );
	SG_ERR_CHECK(  _sg_mrg__create_pathname_for_conflict_file(pCtx, pMrg, pMrgCSetEntryConflict,
															  pMrg->pMrgCSet_Other->pszMnemonicName,
															  &pMrgCSetEntryConflict->pPathTempFile_Other)  );
	SG_ERR_CHECK(  _sg_mrg__create_pathname_for_conflict_file(pCtx, pMrg, pMrgCSetEntryConflict,
															  pMrg->pMrgCSet_FinalResult->pszMnemonicName,
															  &pMrgCSetEntryConflict->pPathTempFile_Result)  );

	// fetch the 3 input versions of the file into temp.
	// 
	// for the "baseline" version, we copy the file as it
	// is in the WD rather than doing a fresh fetch; in the
	// event that the file is dirty (and they allowed a dirty
	// merge), we get the current content of the file.  and in
	// the case that it is clean, we get the same result as if
	// we had done our own fetch.

	SG_ERR_CHECK(  _sg_mrg__export_to_temp_file(pCtx, pMrg,
												pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor->bufHid_Blob,
												pMrgCSetEntryConflict->pPathTempFile_Ancestor)  );

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pMrgCSetEntryConflict->prbUnique_File_HidBlob, &bFound, &pszKey_k, (void **)&pVec_k)  );
	while (bFound)
	{
		if (strcmp(pszKey_k, pMrgCSetEntryConflict->pMrgCSetEntry_Baseline->bufHid_Blob) == 0)
		{
			SG_ERR_CHECK(  _sg_mrg__copy_wc_to_temp_file(pCtx, pMrg,
														 pMrgCSetEntryConflict->pMrgCSetEntry_Baseline,
														 pMrgCSetEntryConflict->pPathTempFile_Baseline)  );
		}
		else
		{
			SG_ERR_CHECK(  _sg_mrg__export_to_temp_file(pCtx, pMrg,
														pszKey_k,
														pMrgCSetEntryConflict->pPathTempFile_Other)  );
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &bFound, &pszKey_k, (void **)&pVec_k)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

/**
 * Look at the entryname and/or the file content of the ancestor
 * version of the file and select the best mergetool for it.
 *
 * Update the mergetool name field in the conflict.
 */
static void _sg_mrg__select_mergetool(SG_context * pCtx,
									  SG_mrg * pMrg,
									  SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict)
{
	SG_ASSERT(  ((pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK)
				 == SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD)  );
	SG_ASSERT( (pMrgCSetEntryConflict->pMrgCSetEntry_Composite->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE) );

	// use the pathname and contents of the ANCESTOR version of the file
	// to choose the mergetool.
	//
	// this is an arbitrary choice, but it simplifies things.
	// 
	// Most of time, it won't matter, but if there was a rename in one or more
	// of the leaves/branches, then we have to decide which of the new names
	// should be used. and if there is an unresolved-divergent-rename, then
	// we also have to worry about that.  so just use the ancestor name to
	// guide the lookup.
	//
	// likewise we use the ancestor version of the file content (for "#!"
	// (shebang) style matches), if appropriate.
	//
	// Also by using the ancestor entryname, we don't have to worry so much
	// about whether the user has resolved the rename conflict before or
	// after they resolve the file-content conflict.
	//
	// this may return null if there is no match.

	SG_ERR_CHECK(  SG_mergetool__select(pCtx,
										SG_MERGETOOL__CONTEXT__MERGE,
										NULL,
										pMrgCSetEntryConflict->pPathTempFile_Ancestor,
										((pMrg->pMergeArgs && pMrg->pMergeArgs->bNoAutoMergeFiles)
										 ? SG_MERGETOOL__INTERNAL__SKIP
										 : NULL),
										pMrg->pWcTx->pDb->pRepo,
										&pMrgCSetEntryConflict->pMergeTool)  );

fail:
	return;
}

static void _sg_mrg__invoke_mergetool(SG_context * pCtx,
									  SG_mrg * pMrg,
									  SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict)
{
	SG_varray * pvaExternal = NULL;
	SG_int32 statusMergeTool = SG_FILETOOL__RESULT__COUNT;
	SG_bool  resultExists = SG_FALSE;

	if (pMrgCSetEntryConflict->pMergeTool == NULL)
	{
		// no mergetool defined/appropriate ==> "No Rule"

		pMrgCSetEntryConflict->flags &= ~SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD;
		pMrgCSetEntryConflict->flags |=  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__NO_RULE;
		goto done;
	}

	SG_ERR_CHECK(
		SG_mergetool__invoke(pCtx, pMrgCSetEntryConflict->pMergeTool,
							 pMrgCSetEntryConflict->pPathTempFile_Ancestor, pMrg->pMrgCSet_LCA->pStringAcceptLabel,
							 pMrgCSetEntryConflict->pPathTempFile_Baseline, pMrg->pMrgCSet_Baseline->pStringAcceptLabel,
							 pMrgCSetEntryConflict->pPathTempFile_Other,    pMrg->pMrgCSet_Other->pStringAcceptLabel,
							 pMrgCSetEntryConflict->pPathTempFile_Result,   pMrg->pMrgCSet_FinalResult->pStringAcceptLabel,
							 &statusMergeTool, &resultExists)  );

	if (statusMergeTool == SG_FILETOOL__RESULT__SUCCESS && resultExists == SG_TRUE)
	{
		pMrgCSetEntryConflict->flags &= ~SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD;
		pMrgCSetEntryConflict->flags |=  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK;
		SG_ERR_CHECK(  _sg_mrg__compute_automerge_hid(pCtx, pMrg, pMrgCSetEntryConflict)  );
	}
	else if (statusMergeTool == SG_MERGETOOL__RESULT__CONFLICT && resultExists == SG_TRUE)
	{
		pMrgCSetEntryConflict->flags &= ~SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD;
		pMrgCSetEntryConflict->flags |=  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT;
		// If the merge tool managed to spit out a conflict file (with <<< ||| >>> markers),
		// we'll install it in the WD (for historical reasons like other products).  If so,
		// we still want to compute the disposable-HID of the conflict file (so that we can
		// tell if it is disposable should they decide to revert).
		SG_ERR_CHECK(  _sg_mrg__compute_automerge_hid(pCtx, pMrg, pMrgCSetEntryConflict)  );
	}
	else if (statusMergeTool != SG_MERGETOOL__RESULT__CANCEL)
	{
		pMrgCSetEntryConflict->flags &= ~SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD;
		pMrgCSetEntryConflict->flags |=  SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_ERROR;
		// I'm going to assume that in these cases no merge result file is generated, so
		// we don't need to compute the disposable-HID.
		//
		// TODO 2012/03/01 Should we assert(!resultExists) and/or delete it if it does
		// TODO            (assuming that we always use a temp file for the result) ?
	}

done:
	;
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaExternal);
}

//////////////////////////////////////////////////////////////////

static void _throw_if_sparse(SG_context * pCtx,
							 SG_mrg * pMrg,
							 SG_mrg_cset_entry * pMrgCSetEntry)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;
	SG_bool bKnown;
	SG_bool bIsSparse;

	// If the file needs merging but is sparse, we need to complain.
	// We know the HID of the file (sparse-hid in tbl_PC), so it isn't
	// a problem to populate it, but at some point we probably need to
	// make it un-sparse so that the result can be placed in the WD
	// (not counting that rare case when we already have a blob with
	// the same HID as the merge-result).  But this is not the right
	// place to make it un-sparse -- suppose the parent directory is
	// also sparse.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry->uiAliasGid,
														 &bKnown, &pLVI)  );
	bIsSparse = SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live);
	if (bIsSparse)
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pMrg->pWcTx, pLVI,
																  &pStringRepoPath)  );
		SG_ERR_THROW2(  SG_ERR_WC_IS_SPARSE,
						(pCtx, "The file '%s' needs to be merged, but is sparse.",
						 SG_string__sz(pStringRepoPath))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * We get called once for each conflict in SG_mrg_cset.prbConflicts.
 * We may be a structural conflict or a TBD file-content conflict.
 * Ignore the former and try to automerge the contents of the latter.
 */
static SG_rbtree_foreach_callback _try_automerge_files;

static void _try_automerge_files(SG_context * pCtx,
								 const char * pszKey_Gid_Entry,
								 void * pVoidAssocData_MrgCSetEntryConflict,
								 void * pVoid_Data)
{
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = (SG_mrg_cset_entry_conflict *)pVoidAssocData_MrgCSetEntryConflict;
	_amf_data * pAmfData = (_amf_data *)pVoid_Data;

	SG_UNUSED(pszKey_Gid_Entry);

	if ((pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD) == 0)
		return;

	SG_ERR_CHECK(  _throw_if_sparse(pCtx, pAmfData->pMrg, pMrgCSetEntryConflict->pMrgCSetEntry_Baseline)  );

	SG_ERR_CHECK(  _sg_mrg__fetch_file_versions_into_temp_files(pCtx, pAmfData->pMrg, pMrgCSetEntryConflict)  );

	SG_ERR_CHECK(  _sg_mrg__select_mergetool(pCtx, pAmfData->pMrg, pMrgCSetEntryConflict)  );
	SG_ERR_CHECK(  _sg_mrg__invoke_mergetool(pCtx, pAmfData->pMrg, pMrgCSetEntryConflict)  );

	// leave the final merge-result and the temp-files in the temp-dir.

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_mrg__automerge_files(SG_context * pCtx, SG_mrg * pMrg, SG_mrg_cset * pMrgCSet)
{
	_amf_data amfData;

	SG_NULLARGCHECK_RETURN(pMrg);
	SG_NULLARGCHECK_RETURN(pMrgCSet);

	// if there were no conflicts *anywhere* in the result-cset, we don't have to do anything.

	if (!pMrgCSet->prbConflicts)
		return;

	// visit each entry in the result-cset and do any auto-merges that we can.

	amfData.pMrg = pMrg;
	amfData.pMrgCSet = pMrgCSet;

	SG_ERR_CHECK_RETURN(  SG_rbtree__foreach(pCtx,pMrgCSet->prbConflicts,_try_automerge_files,&amfData)  );
}
