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
 * @file sg_wc_tx__resolve__diff__inline2.h
 *
 * @details SG_resolve__diff related routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__DIFF__INLINE2_H
#define H_SG_WC_TX__RESOLVE__DIFF__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__diff_values(
	SG_context*        pCtx,
	SG_resolve__value* pLeft,
	SG_resolve__value* pRight,
	const char*        szToolContext,
	const char*        szTool,
	char**             ppTool
	)
{
	SG_resolve__choice* pChoice          = pLeft->pChoice;
	SG_bool             bMergeable       = SG_FALSE;
	SG_string *         pStringHypotheticalRepoPath = NULL;
	SG_filetool*        pTool            = NULL;
	SG_string*          pLeftLabel       = NULL;
	SG_string*          pRightLabel      = NULL;
	SG_pathname *       pPathLeft        = NULL;
	SG_pathname *       pPathRight       = NULL;

	SG_NULLARGCHECK(pLeft);
	SG_NULLARGCHECK(pRight);
	SG_NULLARGCHECK(szToolContext);
	SG_ARGCHECK(pLeft->pChoice == pRight->pChoice, pLeft+pRight);

	// make sure this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	SG_ARGCHECK(bMergeable == SG_TRUE, pLeft|pRight);
	SG_ASSERT(pLeft->pVariantFile != NULL && pRight->pVariantFile != NULL);

	// Note: We are passing pItem here.  It is the same regardless of which
	// Note: value/choice we are using here (which means that there is only
	// Note: one hypothetical-repo-path to consider).
	SG_ERR_CHECK(  _resolve__util__compute_hypothetical_repo_path_for_tool_selection(pCtx,
																					 pLeft->pChoice->pItem,
																					 &pStringHypotheticalRepoPath)  );

	// pPathLeft will contain the TEMP path where MERGE put a copy.
	// This might be used to sniff the file for magic-numbers and
	// determine its type.
	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pLeft, &pPathLeft)  );
	
	SG_ERR_CHECK(  SG_difftool__select(pCtx,
									   szToolContext,
									   SG_string__sz(pStringHypotheticalRepoPath),
									   pPathLeft,
									   szTool,
									   pChoice->pItem->pResolve->pWcTx->pDb->pRepo,
									   &pTool)  );
	if (pTool == NULL)
	{
		SG_ERR_THROW2(SG_ERR_RESOLVE_TOOL_NOT_FOUND,
					  (pCtx, "No diff tool found for: %s",
					   SG_string__sz(pStringHypotheticalRepoPath))  );
	}

	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pRight, &pPathRight)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pLeftLabel, pLeft->szLabel)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pRightLabel, pRight->szLabel)  );

	// invoke the selected tool.
	SG_ERR_CHECK(  SG_difftool__invoke(pCtx,
									   pTool,
									   pPathLeft, pLeftLabel,
									   pPathRight, pRightLabel,
									   SG_FALSE, NULL)  );

	// return name of the selected tool
	if (ppTool != NULL)
	{
		const char* szToolName = NULL;

		SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &szToolName)  );
		SG_ERR_CHECK(  SG_strdup(pCtx, szToolName, ppTool)  );
	}

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	SG_STRING_NULLFREE(pCtx, pLeftLabel);
	SG_STRING_NULLFREE(pCtx, pRightLabel);
	SG_PATHNAME_NULLFREE(pCtx, pPathLeft);
	SG_PATHNAME_NULLFREE(pCtx, pPathRight);
	SG_STRING_NULLFREE(pCtx, pStringHypotheticalRepoPath);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__DIFF__INLINE2_H
