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
 * @file sg_wc_tx__resolve__merge__inline1.h
 *
 * @details Merge-related internal functions that I found in the
 * PendingTree version of sg_resolve.c
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__MERGE__INLINE1_H
#define H_SG_WC_TX__RESOLVE__MERGE__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Selects and invokes a mergetool to run on two given values in a choice.
 *
 * TODO 2012/06/08 In this function the terms "Ancestor", "Baseline", and "Other"
 * TODO            should be replaced with "Center", "Left", and "Right" because
 * TODO            we are passed in the args and are just using them as positional
 * TODO            parameters -- we do/should not do anything with more knowledge
 * TODO            than that -- because the user gets to choose what leaves are
 * TODO            used, right?
 * 
 */
static void _merge__run_tool(
	SG_context*         pCtx,            //< [in] [out] Error and context info.
	SG_resolve__value*  pAncestor,       //< [in] The value to use as the common ancestor.
	SG_resolve__value*  pBaseline,       //< [in] The value to use as the baseline parent.
	SG_resolve__value*  pOther,          //< [in] The value to use as the other parent.
	const char*         szResultLabel,   //< [in] Label to use for the result file.
	const SG_pathname*  pResultPath,     //< [in] Full temp path to use for the result file.
	const char*         szSuggestedTool, //< [in] A suggested tool name to use, or NULL if it doesn't matter.
	SG_int32*           pResult,         //< [out] The result of running the mergetool.
	                                     //<       One of SG_MERGETOOL__RESULT__* or SG_FILETOOL__RESULT__*
	SG_bool*            pResultExists,   //< [out] Whether or not a merge file was generated.
	char**              ppToolName       //< [out] Name of the merge tool that was used.
	                                     //<       NULL if the merge was unsuccessful.
	)
{
	SG_string * pStringHypotheticalRepoPath = NULL;
	SG_filetool* pTool              = NULL;
	const char*  szToolName         = NULL;
	SG_string*   sAncestorLabel     = NULL;
	SG_string*   sBaselineLabel     = NULL;
	SG_string*   sOtherLabel        = NULL;
	SG_string*   sResultLabel       = NULL;
	SG_int32     iResult            = SG_FILETOOL__RESULT__COUNT;
	SG_bool      bResultExists      = SG_FALSE;
	SG_pathname * pPathAncestor = NULL;
	SG_pathname * pPathBaseline = NULL;
	SG_pathname * pPathOther    = NULL;

	SG_NULLARGCHECK(pAncestor);
	SG_NULLARGCHECK(pAncestor->pVariantFile);
	SG_NULLARGCHECK(pBaseline);
	SG_NULLARGCHECK(pBaseline->pVariantFile);
	SG_NULLARGCHECK(pOther);
	SG_NULLARGCHECK(pOther->pVariantFile);
	SG_NULLARGCHECK(szResultLabel);
	SG_NULLARGCHECK(pResultPath);
	SG_NULLARGCHECK(pResult);
	SG_NULLARGCHECK(pResultExists);
	SG_NULLARGCHECK(ppToolName);
	SG_ARGCHECK(pAncestor->pChoice == pBaseline->pChoice, pAncestor+pBaseline);
	SG_ARGCHECK(pAncestor->pChoice == pOther->pChoice, pAncestor+pOther);

	// Note: We are passing pItem here.  It is the same regardless of which
	// Note: value/choice we are using here (which means that there is only
	// Note: one hypothetical-repo-path to consider).
	SG_ERR_CHECK(  _resolve__util__compute_hypothetical_repo_path_for_tool_selection(pCtx,
																					 pAncestor->pChoice->pItem,
																					 &pStringHypotheticalRepoPath)  );

	// Arbitrarily choose one of the 3 versions of the file (probably in temp)
	// and get its pathname.  This will let SG_mergetool__select() sniff it for
	// magic-numbers and etc.

	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pAncestor, &pPathAncestor)  );
	SG_ERR_CHECK(  SG_mergetool__select(pCtx,
										SG_MERGETOOL__CONTEXT__RESOLVE,
										SG_string__sz(pStringHypotheticalRepoPath),
										pPathAncestor,
										szSuggestedTool,
										pAncestor->pChoice->pItem->pResolve->pWcTx->pDb->pRepo,
										&pTool)  );
	if (pTool == NULL)
	{
		SG_ERR_THROW2(SG_ERR_RESOLVE_TOOL_NOT_FOUND,
					  (pCtx, "No merge tool found for: %s",
					   SG_string__sz(pStringHypotheticalRepoPath))  );
	}
	SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &szToolName)  );

	// build all the paths and labels we need
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sAncestorLabel, pAncestor->szLabel)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sBaselineLabel, pBaseline->szLabel)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sOtherLabel, pOther->szLabel)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sResultLabel, szResultLabel)  );

	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pBaseline, &pPathBaseline)  );
	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pOther,    &pPathOther)  );

	// invoke the tool
	SG_ERR_CHECK(  SG_mergetool__invoke(pCtx, pTool,
										pPathAncestor, sAncestorLabel,
										pPathBaseline, sBaselineLabel,
										pPathOther,    sOtherLabel,
										pResultPath,   sResultLabel,
										&iResult,
										&bResultExists)  );

	// return the results
	SG_ERR_CHECK(  SG_strdup(pCtx, szToolName, ppToolName)  );
	*pResult = iResult;
	*pResultExists = bResultExists;

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	SG_STRING_NULLFREE(pCtx, sAncestorLabel);
	SG_STRING_NULLFREE(pCtx, sBaselineLabel);
	SG_STRING_NULLFREE(pCtx, sOtherLabel);
	SG_STRING_NULLFREE(pCtx, sResultLabel);
	SG_PATHNAME_NULLFREE(pCtx, pPathAncestor);
	SG_PATHNAME_NULLFREE(pCtx, pPathBaseline);
	SG_PATHNAME_NULLFREE(pCtx, pPathOther);
	SG_STRING_NULLFREE(pCtx, pStringHypotheticalRepoPath);
}

/**
 * Adds/updates a value in its choice's list of values.
 * If a value with the same label already exists, it is replaced.
 * Otherwise, the new value is appended.
 */
static void _merge__update_values(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_resolve__value* pValue //< [in] The value to add to its choice's list.
	)
{
	SG_resolve__choice* pChoice   = pValue->pChoice;
	SG_uint32           uIndex    = 0u;
	SG_resolve__value*  pOldValue = NULL;

	SG_NULLARGCHECK(pValue);

	// look up any value that already exists at the same label
	SG_ERR_CHECK(  SG_vector__find__first(pCtx, pChoice->pValues, _value__vector_predicate__match_label, (void*)pValue->szLabel, &uIndex, (void**)&pOldValue)  );
	if (pOldValue != NULL)
	{
		// make sure it's a manually generated value
		// this should have been verified before we were called
		SG_ASSERT(pOldValue->pszMnemonic != NULL || pOldValue->bAutoMerge == SG_FALSE);

		// make sure it's not the choice's current result
		// this should also have been verified before we were called
		SG_ASSERT(pOldValue != pChoice->pMergeableResult);

		// remove the old value so that it is effectively replaced with the new one
		// Note: We are removing the old value and appending the new one,
		//       rather than simply overwriting the value in its old position.
		//       This is to ensure that a value's parents always come before
		//       it in the list of values.
		SG_ERR_CHECK(  SG_vector__remove__index(pCtx, pChoice->pValues, uIndex, _value__sg_action__free)  );
		pOldValue = NULL; // old value was freed by the remove
	}

	// append the new value
	SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__MERGE__INLINE1_H
