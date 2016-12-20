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
 * @file sg__vvwc__do_cmd_diff.c
 *
 * @details Support the 'vv diff' command.
 * 
 * The 'vv diff' command has 3 different variants depending on
 * the number of REV-SPEC args given.
 * 
 * [0] baseline-cset vs live-wd
 * 
 * Usage:
 *
 * vv diff
 *       [--interactive]
 *       [--tool]
 *       [--nonrecursive]
 *       [item1 [item2 [...]]]
 *
 * Where 'item1', 'item2', ... are entrynames/pathnames/repo-paths.
 * If no items are given, we display all changes in the WD.
 * If one or more items are given, we only display info for them.
 * The --nonrecursive option only makes sense when given one or
 * more items.
 *
 * This version does not need --no-ignores since we don't need to
 * display uncontrolled items.
 *
 * [1] arbitrary cset-vs-live-wd
 *
 * vv diff
 *       [--interactive]
 *       [--tool]
 *       [--nonrecursive]
 *       <rev_spec_0>
 *       [item1 [item2 [...]]]
 *
 * Like [0], but use an arbitrary cset for the left side rather
 * than assuming the baseline.
 * 
 * [2] cset-vs-cset (historical) diff (no wd required)
 *
 * Usage:
 *
 * vv diff
 *       [--interactive]
 *       [--tool]
 *       [--repo=REPOSITORY]
 *       <rev_spec_0>
 *       <rev_spec_1>
 *       [file1 [file2 [...]]]
 *
 * If the REPOSITORY name is omitted, we'll try to get it
 * from the WD, if present.
 *
 * This version does not need --no-ignores since uncontrolled
 * items never appear in the history.
 *
 * When we are given a list of files, we ONLY compare them;
 * we DO NOT do a full diff of the tree.  (And we disregard
 * the --nonrecursive option since we only do files.)
 *
 * When no files are given, we do the normal full status/diff
 * of the tree.  WE DO NOT ALLOW --nonrecursive.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_vv2__public_typedefs.h>

#include <sg_wc__public_prototypes.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

/**
 * Pick thru the computed VARRAY build a subset VARRAY
 * containing just the dirty files.  That is, for an interactive
 * diff, we only show dirty files.  (In batch/patch mode, we show
 * everything.)
 *
 * We assume that the varray looks like:
 *
 * varray := [ { "status" : { "flags" : <int>,
 *                            ... },
 *               "path"   : <repo-path>,
 *               ... },
 *             ... ];
 *
 * Both a Canonical STATUS (pvaStatus) and a "DiffStep" (pvaDiffStep)
 * match this pattern.
 *
 * Return NULL if there aren't any.
 *
 */
static void _get_dirty_files(SG_context * pCtx,
							 const SG_varray * pva,
							 SG_varray ** ppvaDirtyFiles)
{
	SG_varray * pvaDirtyFiles = NULL;
	SG_uint32 k, nrItems;

	*ppvaDirtyFiles = NULL;

	if (!pva)
		return;
	SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &nrItems)  );
	if (nrItems == 0)
		return;

	for (k=0; k<nrItems; k++)
	{
		SG_vhash * pvhItem_k;			// we do not own this
		SG_vhash * pvhItemStatus_k;		// we do not own this
		SG_int64 i64;
		SG_wc_status_flags statusFlags;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, k, &pvhItem_k)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, "status", &pvhItemStatus_k)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhItemStatus_k, "flags", &i64)  );
		statusFlags = (SG_wc_status_flags)i64;

		if ((statusFlags & SG_WC_STATUS_FLAGS__T__FILE) == 0)
			continue;
		if ((statusFlags & (SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED
							|SG_WC_STATUS_FLAGS__S__ADDED
							|SG_WC_STATUS_FLAGS__S__DELETED
							|SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)) == 0)
			continue;

		if (!pvaDirtyFiles)
			SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaDirtyFiles)  );
		SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pvaDirtyFiles, pvhItem_k, NULL)  );
	}

	SG_RETURN_AND_NULL( pvaDirtyFiles, ppvaDirtyFiles );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaDirtyFiles);
}

//////////////////////////////////////////////////////////////////

/**
 * Prompt user for what to do on an individual item.
 *
 */
static void _do_prompt(SG_context * pCtx,
					   const char * pszPrompt,
					   const char * pszChoices,
					   char chDefault,
					   char * pchChoice)
{
	SG_string * pStringInput = NULL;

	while (1)
	{
		const char * pszInput;
		const char * p;

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s: ", pszPrompt)  );
		SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &pStringInput)  );
		pszInput = SG_string__sz(pStringInput);
		while ((*pszInput==' ') || (*pszInput=='\t'))
			pszInput++;
		if ((*pszInput==0) || (*pszInput=='\r') || (*pszInput=='\n'))
		{
			*pchChoice = chDefault;
			break;
		}
		p = strchr(pszChoices, *pszInput);
		if (p)
		{
			*pchChoice = *p;
			break;
		}
		SG_STRING_NULLFREE(pCtx, pStringInput);
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringInput);
}

/**
 * Do diff of an individual item.
 * When WC-based, we have a "DiffStep" vhash.
 * When historical, we have an item from a pvaStatus.
 * 
 */
static void _do_diff1(SG_context * pCtx,
					  SG_bool bWC,
					  const SG_option_state * pOptSt,
					  const SG_vhash * pvhItem,
					  SG_uint32 * piResult)
{
	SG_string * pStringGidRepoPath = NULL;
	SG_vhash * pvhResultCodes = NULL;
	SG_stringarray * psa1 = NULL;
	const char * pszGid;
	SG_int64 i64Result = 0;
	SG_string * pStringErr = NULL;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );

	if (bWC)
	{
		SG_pathname * pPathWc = NULL;
		SG_bool bHasTool = SG_FALSE;

		// With the __diff__setup() and __diff__run() changes, we have already
		// examined the items during the __setup() step and recorded a tool for
		// the *FILE* that have changed content.  So if "tool" isn't set in the
		// DiffStep/Item, we don't need to diff it -- it could be a structural
		// change, a non-file, a found item, etc.
		//
		// we do not use SG_wc__diff__throw() because we already have the diff info
		// and we want to control the result-code processing below.

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItem, "tool", &bHasTool)  );
		if (bHasTool)
			SG_ERR_CHECK(  SG_wc__diff__run(pCtx, pPathWc, pvhItem, &pvhResultCodes)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringGidRepoPath)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringGidRepoPath, "@%s", pszGid)  );

		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa1, 1)  );
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa1, SG_string__sz(pStringGidRepoPath))  );
		// we do not use the __throw() version of this routine so we can control
		// result-code processing below.
		SG_ERR_CHECK(  SG_vv2__diff_to_stream(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec,
											  psa1, 0,
											  SG_FALSE,		// bNoSort
											  SG_TRUE,		// bInteractive,
											  pOptSt->psz_tool,
											  &pvhResultCodes)  );
	}

	if (pvhResultCodes)
	{
		SG_vhash * pvhResult;				// we do not own this
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhResultCodes, pszGid, &pvhResult)  );
		if (pvhResult)
		{
			const char * pszTool;

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResult, "tool", &pszTool)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhResult, "result", &i64Result)  );

			SG_difftool__check_result_code__throw(pCtx, i64Result, pszTool);
			if (SG_context__has_err(pCtx))
			{
				SG_context__err_to_string(pCtx, SG_FALSE, &pStringErr);
				SG_context__err_reset(pCtx);
				SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDERR, SG_string__sz(pStringErr))  );

				// eat the tool error. the result code is set.
			}
		}
	}

	if (piResult)
		*piResult = (SG_uint32)i64Result;

fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
	SG_VHASH_NULLFREE(pCtx, pvhResultCodes);
	SG_STRINGARRAY_NULLFREE(pCtx, psa1);
	SG_STRING_NULLFREE(pCtx, pStringErr);
}

/**
 * Iterate over all of the dirty files and prompt before diffing.
 * pvaDirtyFiles can either be a STATUS or a "DiffStep" varray.
 * 
 */
static void _do_gui_diffs(SG_context * pCtx,
						  SG_bool bWC,
						  const SG_option_state * pOptSt,
						  const SG_varray * pvaDirtyFiles,
						  SG_uint32 * pNrErrors)
{
	SG_uint32 k, nrItems;
	SG_uint32 nrErrors = 0;
	
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaDirtyFiles, &nrItems)  );
	if (nrItems == 1)	// if only 1 item, no fancy prompt required.
	{
		SG_vhash * pvhItem_0;			// we do not own this
		SG_uint32 iResult = 0;
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaDirtyFiles, 0, &pvhItem_0)  );
		SG_ERR_CHECK(  _do_diff1(pCtx, bWC, pOptSt, pvhItem_0, &iResult)  );

		switch (iResult)
		{
		default:
		case SG_FILETOOL__RESULT__SUCCESS:
		case SG_DIFFTOOL__RESULT__SAME:
		case SG_DIFFTOOL__RESULT__DIFFERENT:
			break;

		case SG_DIFFTOOL__RESULT__CANCEL:
		case SG_FILETOOL__RESULT__FAILURE:
		case SG_FILETOOL__RESULT__ERROR:
			nrErrors++;
			break;
		}
	}
	else
	{
		k = 0;
		while (1)
		{
			SG_vhash * pvhItem;			// we do not own this
			const char * pszRepoPath;
			char chChoice = 'd';
			SG_uint32 iResult;
		
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaDirtyFiles, k, &pvhItem)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszRepoPath)  );

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n[%d/%d] %s:\n", k+1, nrItems, pszRepoPath)  );
			if (k == 0)
				SG_ERR_CHECK(  _do_prompt(pCtx, "(d)iff (n)ext (q)uit", "dnq", 'd', &chChoice)  );
			else if (k+1 == nrItems)
				SG_ERR_CHECK(  _do_prompt(pCtx, "(d)iff (p)rev (q)uit", "dpq", 'd', &chChoice)  );
			else
				SG_ERR_CHECK(  _do_prompt(pCtx, "(d)iff (n)ext (p)rev (q)uit", "dnpq", 'd', &chChoice)  );

			switch (chChoice)
			{
			case 'd':
				SG_ERR_CHECK(  _do_diff1(pCtx, bWC, pOptSt, pvhItem, &iResult)  );
				switch (iResult)
				{
				default:
				case SG_FILETOOL__RESULT__SUCCESS:
				case SG_DIFFTOOL__RESULT__SAME:
				case SG_DIFFTOOL__RESULT__DIFFERENT:
					// advance to next pair of files or finish.
					if (k+1 == nrItems)
						goto done;
					k++;
					break;

				case SG_DIFFTOOL__RESULT__CANCEL:
				case SG_FILETOOL__RESULT__FAILURE:
				case SG_FILETOOL__RESULT__ERROR:
					nrErrors++;
					// stay on this pair of files (so that they see the
					// error message and this filename again).
					break;
				}
				break;

			case 'n':
				k++;
				break;

			case 'p':
				k--;
				break;

			default:
			case 'q':
				goto done;
			}
		}
	}

done:
	*pNrErrors = nrErrors;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _s2__do_cset_vs_cset(SG_context * pCtx,
								 const SG_option_state * pOptSt,
								 const SG_stringarray * psaArgs,
								 SG_uint32 * pNrErrors)
{
	SG_varray * pvaStatus = NULL;
	SG_varray * pvaStatusDirtyFiles = NULL;
	SG_stringarray * psa1 = NULL;
	SG_string * pStringGidRepoPath = NULL;
	SG_string * pStringErr = NULL;
	SG_uint32 nrErrors = 0;

	SG_ERR_CHECK(  SG_vv2__status(pCtx,
								  pOptSt->psz_repo,
								  pOptSt->pRevSpec,
								  psaArgs,
								  WC__GET_DEPTH(pOptSt),
								  SG_FALSE, // bNoSort
								  &pvaStatus, NULL)  );
	if (pvaStatus)
	{
		if (pOptSt->bInteractive)
		{
			// Filter list down to just modified files and show them one-by-one.
			SG_ERR_CHECK(  _get_dirty_files(pCtx, pvaStatus, &pvaStatusDirtyFiles)  );
			if (pvaStatusDirtyFiles)
				SG_ERR_CHECK(  _do_gui_diffs(pCtx, SG_FALSE, pOptSt, pvaStatusDirtyFiles, &nrErrors)  );
		}
		else
		{
			SG_uint32 k, nrItems;

			// Print the changes with PATCH-like headers.
			// Accumulate any tool errors.
			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus, &nrItems)  );
			for (k=0; k<nrItems; k++)
			{
				SG_vhash * pvhItem;
				const char * pszGid = NULL;

				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaStatus, k, &pvhItem)  );

				// TODO 2013/02/22 Our pvhItem has all of the details for the diff,
				// TODO            but we don't yet have a public API to let it be
				// TODO            used as is.  So we build a @gid repo-path and
				// TODO            run the old historical diff code on a 1-item array
				// TODO            containing this @gid.
				// TODO
				// TODO            We should fix this to just pass down the pvhItem
				// TOOD            so that it doesn't have to repeat the status lookup.

				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
				SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringGidRepoPath)  );
				SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringGidRepoPath, "@%s", pszGid)  );

				SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa1, 1)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa1, SG_string__sz(pStringGidRepoPath))  );

				SG_vv2__diff_to_stream__throw(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec,
											  psa1, 0,
											  SG_TRUE,		// bNoSort -- doesn't matter only 1 item in list
											  SG_FALSE,		// bInteractive,
											  pOptSt->psz_tool);
				// Don't throw the error from the tool.  Just print it on STDERR
				// and remember that we had an error so that don't stop showing
				// the diffs just because we stumble over a changed binary file
				// or mis-configured tool, for example.
				if (SG_context__has_err(pCtx))
				{
					SG_context__err_to_string(pCtx, SG_FALSE, &pStringErr);
					SG_context__err_reset(pCtx);
					SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDERR, SG_string__sz(pStringErr))  );
					SG_STRING_NULLFREE(pCtx, pStringErr);

					nrErrors++;
				}

				SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
				SG_STRINGARRAY_NULLFREE(pCtx, psa1);
			}
		}
	}

	*pNrErrors = nrErrors;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VARRAY_NULLFREE(pCtx, pvaStatusDirtyFiles);
	SG_STRINGARRAY_NULLFREE(pCtx, psa1);
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringErr);
}

//////////////////////////////////////////////////////////////////

/**
 * Compute DIFF on (baseline or arbitrary cset vs WC) and either
 * splat to console or launch a GUI tool for each.
 *
 */
static void _s01__do_cset_vs_wc(SG_context * pCtx,
								const SG_option_state * pOptSt,											 
								const SG_stringarray * psaArgs,
								SG_uint32 * pNrErrors)
{
	SG_wc_tx * pWcTx = NULL;
	SG_varray * pvaDiffSteps = NULL;
	SG_varray * pvaDiffSteps_DirtyFiles = NULL;
	SG_pathname * pPathWc = NULL;
	SG_uint32 nrErrors = 0;

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__diff__setup__stringarray(pCtx, pWcTx, pOptSt->pRevSpec,
													  psaArgs,
													  WC__GET_DEPTH(pOptSt),
													  SG_FALSE, // bNoIgnores
													  SG_FALSE,	// bNoTSC
													  SG_FALSE,	// bNoSort,
													  pOptSt->bInteractive,
													  pOptSt->psz_tool,
													  &pvaDiffSteps)  );
	// rollback/cancel the TX to release the SQL locks,
	// but don't free it yet (because that will auto-delete
	// the session temp files that we are using for the left
	// sides).
	//
	// This is like SG_wc__diff__throw() but we want to control
	// the diff-loop so we can optionally do the interactive prompt.

	SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, pWcTx)  );

	if (pvaDiffSteps)
	{
		if (pOptSt->bInteractive)
		{
			SG_ERR_CHECK(  _get_dirty_files(pCtx, pvaDiffSteps, &pvaDiffSteps_DirtyFiles)  );
			if (pvaDiffSteps_DirtyFiles)
				SG_ERR_CHECK(  _do_gui_diffs(pCtx, SG_TRUE, pOptSt, pvaDiffSteps_DirtyFiles, &nrErrors)  );
		}
		else
		{
			SG_uint32 k, nrItems;

			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaDiffSteps, &nrItems)  );
			for (k=0; k<nrItems; k++)
			{
				SG_vhash * pvhItem;
				const char * pszHeader = NULL;
				SG_uint32 iResult;

				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaDiffSteps, k, &pvhItem)  );
				SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhItem, "header", &pszHeader)  );
				if (pszHeader)
					SG_ERR_IGNORE(  SG_console__raw(pCtx, SG_CS_STDOUT, pszHeader)  );
				SG_ERR_CHECK(  _do_diff1(pCtx, SG_TRUE, pOptSt, pvhItem, &iResult)  );

				switch (iResult)
				{
				default:
				case SG_FILETOOL__RESULT__SUCCESS:
				case SG_DIFFTOOL__RESULT__SAME:
				case SG_DIFFTOOL__RESULT__DIFFERENT:
					break;

				case SG_DIFFTOOL__RESULT__CANCEL:
				case SG_FILETOOL__RESULT__FAILURE:
				case SG_FILETOOL__RESULT__ERROR:
					nrErrors++;
					break;
				}
			}
		}
	}

	*pNrErrors = nrErrors;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaDiffSteps);
	SG_VARRAY_NULLFREE(pCtx, pvaDiffSteps_DirtyFiles);
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
}

//////////////////////////////////////////////////////////////////

/**
 * Main 'vv diff' entry point.  We decide which variant is intended.
 *
 */
void vvwc__do_cmd_diff(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs,
					   SG_uint32 * pNrErrors)
{
	SG_uint32 nrRevSpecs = 0;
	SG_uint32 nrErrors = 0;

	if (pOptSt->pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &nrRevSpecs)  );

	switch (nrRevSpecs)
	{
	case 0:
	case 1:
		SG_ERR_CHECK(  _s01__do_cset_vs_wc(pCtx, pOptSt, psaArgs, &nrErrors)  );
		break;

	case 2:
		SG_ERR_CHECK(  _s2__do_cset_vs_cset(pCtx, pOptSt, psaArgs, &nrErrors)  );
		break;

	default:
		SG_ERR_THROW( SG_ERR_USAGE );
	}

	*pNrErrors = nrErrors;

fail:
	return;
}

