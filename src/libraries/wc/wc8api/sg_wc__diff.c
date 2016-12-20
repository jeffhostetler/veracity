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
 * Open a pRepo handle on the REPO associated with the indicated WC.
 * We have to do this the hard way because we are not using
 * SG_wc_tx__alloc__begin() like all/most of the other layer-8 API
 * routines.
 *
 */
static void _get_repo(SG_context * pCtx,
					  const SG_pathname * pPathWc,
					  SG_repo ** ppRepo)
{
	const SG_pathname * pPath;
	SG_pathname * pPathCwd = NULL;
	SG_repo * pRepo = NULL;
	SG_string * pStringRepoDescriptorName = NULL;

	if (pPathWc)
	{
		pPath = pPathWc;
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathCwd)  );
		SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathCwd)  );
		pPath = pPathCwd;
	}
	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath, NULL, &pStringRepoDescriptorName, NULL)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pStringRepoDescriptorName), &pRepo)  );

	SG_RETURN_AND_NULL(pRepo, ppRepo);

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_STRING_NULLFREE(pCtx, pStringRepoDescriptorName);
}

//////////////////////////////////////////////////////////////////

/**
 * Do actual DIFF of a file and either splat to console or launch
 * an external tool.  The input here is data output from __diff__setup().
 * This routine is intended to be called outside of a TX.
 *
 */
void SG_wc__diff__run(SG_context * pCtx,
					  const SG_pathname * pPathWc,
					  const SG_vhash * pvhDiffItem,
					  SG_vhash ** ppvhResultCodes)
{
	const char * pszGid = NULL;
	const char * pszRepoPath = NULL;
	const char * pszToolName = NULL;
	const char * pszToolContext = NULL;
	const char * pszPathFrom = NULL;
	const char * pszPathTo = NULL;
	const char * pszLabelFrom = NULL;
	const char * pszLabelTo = NULL;
	SG_pathname * pPathFrom = NULL;
	SG_pathname * pPathTo = NULL;
	SG_string * pStringLabelFrom = NULL;
	SG_string * pStringLabelTo = NULL;
	SG_bool bIsTmp_right = SG_FALSE;
	SG_vhash * pvhResultCodes = NULL;
	SG_int32 iResultCode;
	SG_repo * pRepo = NULL;

	// ppvhResultCodes is optional

	SG_ERR_CHECK(  _get_repo(pCtx, pPathWc, &pRepo)  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhDiffItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhDiffItem, "path", &pszRepoPath)  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhDiffItem, "tool", &pszToolName)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhDiffItem, "context", &pszToolContext)  );

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhDiffItem, "left_abs_path", &pszPathFrom)  );
	if (pszPathFrom)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathFrom, pszPathFrom)  );

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhDiffItem, "right_abs_path", &pszPathTo)  );
	if (pszPathTo)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTo, pszPathTo)  );

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhDiffItem, "left_label", &pszLabelFrom)  );
	if (pszLabelFrom)
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLabelFrom, pszLabelFrom)  );

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhDiffItem, "right_label", &pszLabelTo)  );
	if (pszLabelTo)
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLabelTo, pszLabelTo)  );

	SG_ERR_CHECK(  SG_vhash__check__bool(pCtx, pvhDiffItem, "right_is_tmp", &bIsTmp_right)  );

	SG_ERR_CHECK(  SG_difftool__run(pCtx, pszRepoPath, pRepo, pszToolContext, pszToolName,
									pPathFrom, pStringLabelFrom,
									pPathTo, pStringLabelTo,
									(!bIsTmp_right),
									&iResultCode, NULL)  );

	if (ppvhResultCodes)
	{
		SG_vhash * pvhDetails;		// we do not own this.

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResultCodes)  );
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhResultCodes, pszGid, &pvhDetails)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "tool", pszToolName)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhDetails, "result", (SG_int64)iResultCode)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "repopath", pszRepoPath)  );

		SG_RETURN_AND_NULL(pvhResultCodes, ppvhResultCodes);
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathFrom);
	SG_PATHNAME_NULLFREE(pCtx, pPathTo);
	SG_STRING_NULLFREE(pCtx, pStringLabelFrom);
	SG_STRING_NULLFREE(pCtx, pStringLabelTo);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

/**
 * This is a 1-step diff.  It does the STATUS and holds the TX open
 * (but releases the locks) so that the individual diffs can be done
 * before the temp files are deleted (when the TX is freed).
 *
 */
void SG_wc__diff__throw(SG_context * pCtx,
						const SG_pathname * pPathWc,
						const SG_rev_spec * pRevSpec,
						const SG_stringarray * psa_args,
						SG_uint32 nDepth,
						SG_bool bNoIgnores,
						SG_bool bNoTSC,
						SG_bool bInteractive,
						const char * psz_tool)
{
	SG_wc_tx * pWcTx = NULL;
	SG_varray * pvaDiffSteps = NULL;
	SG_vhash * pvhResultCodes = NULL;

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__diff__setup__stringarray(pCtx, pWcTx, pRevSpec,
													  psa_args, nDepth,
													  bNoIgnores,
													  bNoTSC,
													  SG_FALSE,	// bNoSort,
													  bInteractive,
													  psz_tool,
													  &pvaDiffSteps)  );
	// rollback/cancel the TX to release the SQL locks,
	// but don't free it yet (because that will auto-delete
	// the session temp files that we are using for the left
	// sides).
	SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, pWcTx)  );

	if (pvaDiffSteps)
	{
		SG_uint32 k, nrItems;
		SG_vhash * pvhItem = NULL;
		SG_bool bHasTool = SG_FALSE;
		const char * pszGid = NULL;

		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaDiffSteps, &nrItems)  );

		for (k = 0; k < nrItems; k++)
		{
			SG_vhash * pvhResult = NULL;	// we do not own this
			SG_int64 i64Result = 0;
			const char * pszTool;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaDiffSteps, k, &pvhItem)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItem, "tool", &bHasTool)  );
			if (!bHasTool)
				continue;

			SG_ERR_CHECK(  SG_wc__diff__run(pCtx, pPathWc, pvhItem, &pvhResultCodes)  );
			if (!pvhResultCodes)	// this should be an assert(pvhResultCodes)
				continue;

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhResultCodes, pszGid, &pvhResult)  );
			if (!pvhResult)			// this should be an assert(pvhResult)
				continue;

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResult, "tool", &pszTool)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhResult, "result", &i64Result)  );

			SG_ERR_CHECK(  SG_difftool__check_result_code__throw(pCtx, i64Result, pszTool)  );
		}
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaDiffSteps);
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_VHASH_NULLFREE(pCtx, pvhResultCodes);
}
