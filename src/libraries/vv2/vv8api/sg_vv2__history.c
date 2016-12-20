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
 * @file sg_vv2__history.c
 *
 * @details Compute HISTORY for either the whole repo or for a
 * selected subset.  We do not require a WD to be present, but
 * if the REPO arg is omitted, we will try to use the current
 * WD if available.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void SG_vv2__history(SG_context * pCtx,
					 const SG_vv2_history_args * pHistoryArgs,
					 SG_bool* pbHasResult,
					 SG_vhash** ppvhBranchPile,
					 SG_history_result ** ppHistoryResult)
{
	SG_vhash * pvhBranchPile = NULL;
	SG_history_result * pHistoryResult = NULL;
	SG_bool bHasResult = SG_FALSE;

	SG_NULLARGCHECK_RETURN( pHistoryArgs );
	// pbHasResult is optional
	// ppvhBranchPile is optional
	SG_NULLARGCHECK_RETURN( ppHistoryResult );

	// TODO 2012/07/03 Figure out what the purpose/meaning of "leaves" is.
	// TODO            The "leaves" argument is only passed to the __working_folder()
	// TODO            version of HISTORY.  When "((count(psa_branches)==0) && !bLeaves)"
	// TODO            it will use the WD's current branch (as if it were given to us
	// TODO            using (one or more) "branch" args).

	if (pHistoryArgs->bListAll)
	{
		// When bListAll, we shouldn't get anythin pRevSpec_single_revisions.
		// 
		// TODO 2012/07/05 I'd like to move this variable inside here rather
		// TODO            than force all the callers to split up the args.
		SG_ASSERT_RELEASE_RETURN( (pHistoryArgs->pRevSpec_single_revisions==NULL) );
	}

	if (pHistoryArgs->pszRepoDescriptorName && *pHistoryArgs->pszRepoDescriptorName)
	{
		if (pHistoryArgs->bLeaves)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "The 'leaves' option cannot be used with the 'repo' option.")  );

		SG_ERR_CHECK(  sg_vv2__history__repo(pCtx,
											 pHistoryArgs->pszRepoDescriptorName,
											 pHistoryArgs->psaSrcArgs,
											 pHistoryArgs->pRevSpec,
											 pHistoryArgs->pRevSpec_single_revisions,
											 pHistoryArgs->pszUser,
											 pHistoryArgs->pszStamp,
											 pHistoryArgs->nResultLimit,
											 pHistoryArgs->bHideObjectMerges,
											 pHistoryArgs->nFromDate,
											 pHistoryArgs->nToDate,
											 pHistoryArgs->bListAll,
											 SG_FALSE,
											 ((pbHasResult)    ? &bHasResult    : NULL),
											 ((ppvhBranchPile) ? &pvhBranchPile : NULL),
											 &pHistoryResult,
											 NULL)  );
	}
	else
	{
		SG_bool bDetectCurrentBranch = (!pHistoryArgs->bLeaves);

		SG_ERR_CHECK(  sg_vv2__history__working_folder(pCtx,
													   pHistoryArgs->psaSrcArgs,
													   pHistoryArgs->pRevSpec,
													   pHistoryArgs->pRevSpec_single_revisions,
													   pHistoryArgs->pszUser,
													   pHistoryArgs->pszStamp,
													   bDetectCurrentBranch,
													   pHistoryArgs->nResultLimit,
													   pHistoryArgs->bHideObjectMerges,
													   pHistoryArgs->nFromDate,
													   pHistoryArgs->nToDate,
													   pHistoryArgs->bListAll,
													   ((pbHasResult)    ? &bHasResult    : NULL),
													   ((ppvhBranchPile) ? &pvhBranchPile : NULL),
													   &pHistoryResult,
													   NULL)  );
	}

	if (pHistoryResult)
	{
		if (pHistoryArgs->bReverse)
			SG_ERR_CHECK(  SG_history_result__reverse(pCtx, pHistoryResult)  );
	}

	if (pbHasResult)
		*pbHasResult = bHasResult;
	if (ppvhBranchPile)
	{
		*ppvhBranchPile = pvhBranchPile;
		pvhBranchPile = NULL;
	}
	*ppHistoryResult = pHistoryResult;
	pHistoryResult = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResult);
}
