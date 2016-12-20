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
 * @file sg_mergetool.c
 *
 * @details Code to handle how we choose which merge-tool to use
 * to auto-merge or manually-merge a set of files.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

#include "sg_mergetool__builtin_skip.h"             // __INTERNAL__SKIP
#include "sg_mergetool__builtin_baseline_other.h"   // __INTERNAL__BASELINE, __INTERNAL__OTHER
#include "sg_mergetool__builtin_merge.h"            // __INTERNAL__MERGE_STRICT, __INTERNAL__MERGE_ANY_EOL
#include "sg_mergetool__builtin_diffmerge.h"        // __INTERNAL__DIFFMERGE


/*
**
** Types
**
*/

/**
 * Definition of an internal merge tool.
 */
typedef struct
{
	const char*           szName;    //< Unique name of the internal merge tool.
	const char*           szContext; //< Context that the tool is valid in, or NULL if valid in any context.
	SG_filetool__invoker* pInvoker;  //< Function that implements the tool.
}
_internal_tool;

/**
 * Definition of a default merge tool.
 */
typedef struct
{
	const char* szContext; //< The context that we're defining the default tool for.
	const char* szTool;    //< The name of the default tool in the context.
}
_default_tool;

/**
 * Prototype of functions that implement internal merge tools.
 */
typedef void (SG_mergetool__invoker)(
	SG_context*          pCtx,           //< [in] [out] Error and context info.
	const SG_pathname*   pAncestorPath,  //< [in] Path to the ancestor file for the merge.
	const SG_string*     pAncestorLabel, //< [in] Friendly label for the ancestor file, or NULL to use the path.
	const SG_pathname*   pBaselinePath,  //< [in] Path to the "baseline" file for the merge.
	const SG_string*     pBaselineLabel, //< [in] Friendly label for the "baseline" file, or NULL to use the path.
	const SG_pathname*   pOtherPath,     //< [in] Path to the "other" file for the merge.
	const SG_string*     pOtherLabel,    //< [in] Friendly label for the "other" file, or NULL to use the path.
	const SG_pathname*   pResultPath,    //< [in] Path to the result file for the merge.
	const SG_string*     pResultLabel,   //< [in] Friendly label for the "result" file, or NULL to use the path.
	SG_int32*            pStatus         //< [out] Result status returned by the tool.
	);

/**
 * Functions that translate filetool invocations into mergetool invocations.
 */
static void _invoke__skip         (SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* pExitCode);
static void _invoke__baseline     (SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* pExitCode);
static void _invoke__other        (SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* pExitCode);
static void _invoke__merge_strict (SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* pExitCode);
static void _invoke__merge_any_eol(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* pExitCode);
static void _invoke__diffmerge    (SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* pExitCode);


/*
**
** Globals
**
*/

// token keys used by merge tools
static const char* gszToken_AncestorPath  = "ANCESTOR";
static const char* gszToken_BaselinePath  = "BASELINE";
static const char* gszToken_OtherPath     = "OTHER";
static const char* gszToken_ResultPath    = "RESULT";
static const char* gszToken_AncestorLabel = "ANCESTOR_LABEL";
static const char* gszToken_BaselineLabel = "BASELINE_LABEL";
static const char* gszToken_OtherLabel    = "OTHER_LABEL";
static const char* gszToken_ResultLabel   = "RESULT_LABEL";

/**
 * List of all internal merge tools.
 */
static _internal_tool aInternalTools[] =
{
	{ SG_MERGETOOL__INTERNAL__SKIP,         NULL, _invoke__skip          },
	{ SG_MERGETOOL__INTERNAL__BASELINE,     NULL, _invoke__baseline      },
	{ SG_MERGETOOL__INTERNAL__OTHER,        NULL, _invoke__other         },
	{ SG_MERGETOOL__INTERNAL__MERGE_STRICT, NULL, _invoke__merge_strict  },
	{ SG_MERGETOOL__INTERNAL__MERGE,        NULL, _invoke__merge_any_eol },
	{ SG_MERGETOOL__INTERNAL__DIFFMERGE,    NULL, _invoke__diffmerge     },
};

/**
 * The hard-coded default tools for each merge context.
 */
static _default_tool aDefaultTools[] =
{
	{ SG_MERGETOOL__CONTEXT__MERGE,   SG_MERGETOOL__INTERNAL__MERGE },
	{ SG_MERGETOOL__CONTEXT__RESOLVE, SG_MERGETOOL__INTERNAL__DIFFMERGE },
};


/*
**
** Internal Functions
**
*/

/**
 * Invokes an internal mergetool.
 * Basically translates the token values from a filetool call into the arguments for a mergetool call.
 */
static void _invoke(
	SG_context*           pCtx,         //< [in] [out] Error and context info.
	const SG_vhash*       pTokenValues, //< [in] Token/argument values passed to the tool.
	SG_mergetool__invoker fInvoker,     //< [in] The internal mergetool implementation to invoke.
	SG_int32*             pExitCode     //< [out] The exit code returned by the mergetool.
	)
{
	SG_pathname*        pAncestorPath  = NULL;
	SG_string*          pAncestorLabel = NULL;
	SG_pathname*        pBaselinePath  = NULL;
	SG_string*          pBaselineLabel = NULL;
	SG_pathname*        pOtherPath     = NULL;
	SG_string*          pOtherLabel    = NULL;
	SG_pathname*        pResultPath    = NULL;
	SG_string*          pResultLabel   = NULL;
	SG_int32            eStatus        = SG_FILETOOL__RESULT__COUNT;
	const char*         szTemp         = NULL;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_AncestorPath, &szTemp)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pAncestorPath, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_AncestorLabel, &szTemp)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pAncestorLabel, szTemp)  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_BaselinePath, &szTemp)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pBaselinePath, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_BaselineLabel, &szTemp)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pBaselineLabel, szTemp)  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_OtherPath, &szTemp)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pOtherPath, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_OtherLabel, &szTemp)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pOtherLabel, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_ResultPath, &szTemp)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pResultPath, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTokenValues, gszToken_ResultLabel, &szTemp)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pResultLabel, szTemp)  );

	SG_ERR_CHECK(  (fInvoker)(pCtx,
							  pAncestorPath, pAncestorLabel,
							  pBaselinePath, pBaselineLabel,
							  pOtherPath,    pOtherLabel,
							  pResultPath,   pResultLabel,
							  &eStatus)  );

	*pExitCode = eStatus;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pAncestorPath);
	SG_STRING_NULLFREE(pCtx, pAncestorLabel);
	SG_PATHNAME_NULLFREE(pCtx, pBaselinePath);
	SG_STRING_NULLFREE(pCtx, pBaselineLabel);
	SG_PATHNAME_NULLFREE(pCtx, pOtherPath);
	SG_STRING_NULLFREE(pCtx, pOtherLabel);
	SG_PATHNAME_NULLFREE(pCtx, pResultPath);
	SG_STRING_NULLFREE(pCtx, pResultLabel);
	return;
}

/**
 * Invokes the "skip" internal mergetool.
 */
static void _invoke__skip(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pExitCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke(pCtx, pTokenValues, _sg_mergetool__builtin_skip, pExitCode)  );
}

/**
 * Invokes the "baseline" internal mergetool.
 */
static void _invoke__baseline(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pExitCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke(pCtx, pTokenValues, _sg_mergetool__builtin_baseline, pExitCode)  );
}

/**
 * Invokes the "other" internal mergetool.
 *
 * This tool copies the OTHER version to the RESULT.
 */
static void _invoke__other(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pExitCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke(pCtx, pTokenValues, _sg_mergetool__builtin_other, pExitCode)  );
}

/**
 * Invokes the "merge_strict" internal mergetool.
 */
static void _invoke__merge_strict(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pExitCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke(pCtx, pTokenValues, _sg_mergetool__builtin_merge__strict, pExitCode)  );
}

/**
 * Invokes the "merge" internal mergetool.
 */
static void _invoke__merge_any_eol(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pExitCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke(pCtx, pTokenValues, _sg_mergetool__builtin_merge__any_eol, pExitCode)  );
}

/**
 * Invokes the "diffmerge" internal mergetool.
 */
static void _invoke__diffmerge(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pExitCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke(pCtx, pTokenValues, _sg_mergetool__builtin_diffmerge, pExitCode)  );
}


/**
 * Filetool "plugin" function that locates SG_filetool__invoker
 * functions for the various internal merge tools.
 */
static void _filetool_plugin(
	SG_context*            pCtx,      //< [in] [out] Error and context info.
	const char*            szType,    //< [in] Type of the tool being looked up.
	const char*            szContext, //< [in] The context the tool is being looked up in.
	                                  //<      If NULL, then no specific context is being used.
	const char*            szName,    //< [in] Name of the tool being searched for.
	                                  //<      If NULL, then a custom external tool invoker is being searched for.
	SG_filetool__invoker** ppInvoker  //< [out] Invoker function for the given tool.
	                                  //<       Set to NULL if the tool wasn't found.
	)
{
	SG_uint32             uIndex     = 0u;
	SG_filetool__invoker* pInvoker   = NULL;

	SG_NULLARGCHECK(szType);
	SG_NULLARGCHECK(ppInvoker);

	if (szName != NULL)
	{
		for (uIndex = 0u; uIndex < SG_NrElements(aInternalTools); ++uIndex)
		{
			const _internal_tool* pTool = &(aInternalTools[uIndex]);

			if (
				strcmp(pTool->szName, szName) == 0
				&& (
					pTool->szContext == NULL
					|| szContext == NULL
					|| strcmp(pTool->szContext, szContext) == 0
					)
				)
			{
				pInvoker = pTool->pInvoker;
				break;
			}
		}
	}

	*ppInvoker = pInvoker;
	pInvoker = NULL;

fail:
	return;
}


/*
**
** Public Functions
**
*/

void SG_mergetool__exists(
	SG_context* pCtx,
	const char* szContext,
	const char* szName,
	SG_repo*    pRepo,
	SG_bool*    pExists
	)
{
	SG_filetool* pTool = NULL;

	SG_NULLARGCHECK(szContext);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(pExists);

	SG_ERR_CHECK(  SG_filetool__get_tool(pCtx, SG_MERGETOOL__TYPE, szContext, szName, _filetool_plugin, pRepo, NULL, &pTool)  );
	*pExists = (pTool == NULL) ? SG_FALSE : SG_TRUE;

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return;
}

void SG_mergetool__list(
	SG_context*      pCtx,
	SG_repo*         pRepo,
	SG_stringarray** ppTools
	)
{
	SG_stringarray* pTools = NULL;
	SG_uint32       uIndex = 0u;

	// add all the merge tools from config
	SG_ERR_CHECK(  SG_filetool__list_tools(pCtx, SG_MERGETOOL__TYPE, pRepo, &pTools)  );

	// add all the internal merge tools
	for (uIndex = 0u; uIndex < SG_NrElements(aInternalTools); ++uIndex)
	{
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, pTools, aInternalTools[uIndex].szName)  );
	}

	// return the result
	*ppTools = pTools;
	pTools = NULL;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pTools);
	return;
}

void SG_mergetool__select(
	SG_context*        pCtx,
	const char*        szContext,
	const char*        szPath,
	const SG_pathname* pDiskPath,
	const char*        szOverride,
	SG_repo*           pRepo,
	SG_filetool**      ppTool
	)
{
	SG_string*   sClass     = NULL;
	SG_string*   sTool      = NULL;
	SG_vhash*    pResultMap = NULL;
	SG_filetool* pTool      = NULL;

	SG_NULLARGCHECK(szContext);
	SG_NULLARGCHECK(pDiskPath);
	SG_NULLARGCHECK(ppTool);

	if (szOverride != NULL)
	{
		// use the tool that was explicitly specified
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sTool, szOverride)  );
	}
	else
	{
		// get the class of the given file
		SG_ERR_CHECK(  SG_filetool__get_class(pCtx, szPath, pDiskPath, pRepo, &sClass)  );

		// try to find a tool in the local settings for this class
		SG_ERR_CHECK(  SG_filetool__find_tool(pCtx, SG_MERGETOOL__TYPE, szContext, SG_string__sz(sClass), pRepo, &sTool)  );
		if (sTool == NULL)
		{
			SG_uint32 uIndex = 0u;

			// couldn't find a tool, use the default one for this context
			for (uIndex = 0u; uIndex < SG_NrElements(aDefaultTools); ++uIndex)
			{
				if (strcmp(szContext, aDefaultTools[uIndex].szContext) == 0)
				{
					SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sTool, aDefaultTools[uIndex].szTool)  );
					break;
				}
			}
			if (sTool == NULL)
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "No merge tool found in context '%s' for file: %s", szContext, SG_pathname__sz(pDiskPath))  );
			}
		}
		SG_ASSERT(sTool != NULL);
	}

	// build a result map for merge tool results
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pResultMap)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResultMap, SG_MERGETOOL__RESULT__CONFLICT__SZ, SG_MERGETOOL__RESULT__CONFLICT)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResultMap, SG_MERGETOOL__RESULT__CANCEL__SZ, SG_MERGETOOL__RESULT__CANCEL)  );

	// look up the tool we found
	// TODO: Someday it would be nice to have a mechanism for verifying that the
	//       tool actually works with the class of the file that we're working with
	//       and the context we're trying to use it in.
	SG_ERR_CHECK(  SG_filetool__get_tool(pCtx, SG_MERGETOOL__TYPE, szContext, SG_string__sz(sTool), _filetool_plugin, pRepo, pResultMap, &pTool)  );
	if (pTool == NULL)
	{
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "No settings found for mergetool: %s", SG_string__sz(sTool))  );
	}

	// return the tool we found
	*ppTool = pTool;
	pTool = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sClass);
	SG_STRING_NULLFREE(pCtx, sTool);
	SG_VHASH_NULLFREE(pCtx, pResultMap);
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return;
}

void SG_mergetool__invoke(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_pathname* pAncestorPath,
	const SG_string*   pAncestorLabel,
	const SG_pathname* pBaselinePath,
	const SG_string*   pBaselineLabel,
	const SG_pathname* pOtherPath,
	const SG_string*   pOtherLabel,
	const SG_pathname* pResultPath,
	const SG_string*   pResultLabel,
	SG_int32*          pResultCode,
	SG_bool*           pResultExists
	)
{
	SG_bool       bExists            = SG_FALSE;
	SG_vhash*     pTokenValues       = NULL;
	SG_bool       bAncestorOverwrite = SG_FALSE;
	SG_int64      iBeforeTime        = 0;
	SG_bool       bPremakeResult     = SG_FALSE;
	SG_int32      iResultCode        = 0;
	SG_bool       bResultExists      = SG_FALSE;
	SG_fsobj_stat cStat;

	SG_NULLARGCHECK(pTool);
	SG_NULLARGCHECK(pAncestorPath);
	SG_NULLARGCHECK(pBaselinePath);
	SG_NULLARGCHECK(pOtherPath);
	SG_NULLARGCHECK(pResultPath);

	// TODO 2010/09/17 Consider stat'ing the 3 input files and make sure
	// TODO            that they exist and throw and request that the caller
	// TODO            re-populate them.  MERGE doesn't need this, but RESOLVE
	// TODO            might.

	// delete any already existing result file
	// this ensures that we can check later that this invocation produced a result
	SG_ERR_CHECK_RETURN(  SG_fsobj__exists__pathname(pCtx, pResultPath, &bExists, NULL, NULL)  );
	if (bExists == SG_TRUE)
	{
		// Note: This could theoretically be optimized in the case where
		//       bPremakeResult would be true.  In that case we could not delete the
		//       result file if it is already zero length, rather than deleting it
		//       and then re-creating it.  Seems like more hassle and complication
		//       than it's worth at the moment, though.
		// Note: Could also probably be optimized in the case where
		//       bAncestorOverwrite would be true.  In that case, we could just let
		//       SG_fsobj__copy_file overwrite the existing result file.
		SG_ERR_CHECK_RETURN(  SG_fsobj__remove__pathname(pCtx, pResultPath)  );
	}

	// check what to do about a result file
	SG_ERR_CHECK(  SG_filetool__has_flag(pCtx, pTool, SG_MERGETOOL__FLAG__PREMAKE_RESULT, &bPremakeResult)  );
	SG_ERR_CHECK(  SG_filetool__has_flag(pCtx, pTool, SG_MERGETOOL__FLAG__ANCESTOR_OVERWRITE, &bAncestorOverwrite)  );
	if (bAncestorOverwrite == SG_TRUE)
	{
		SG_fsobj_stat cStat;

		// we need to copy the ancestor to the result so it can be overwritten
		SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pAncestorPath, pResultPath, 0644)  );

		// record its current modified time so we'll know if the tool modified it later
		SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pResultPath, &cStat)  );
		iBeforeTime = cStat.mtime_ms;
	}
	else if (bPremakeResult == SG_TRUE)
	{
		SG_file* pResultFile = NULL;

		// we need to premake an empty result file
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pResultPath, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0644, &pResultFile)  );
		SG_FILE_NULLCLOSE(pCtx, pResultFile);
	}

	// bundle all of the arguments up into a hash
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTokenValues)  );
	SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_AncestorPath, SG_pathname__sz(pAncestorPath))  );
	SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_BaselinePath, SG_pathname__sz(pBaselinePath))  );
	SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_OtherPath,    SG_pathname__sz(pOtherPath))  );
	SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_ResultPath,   SG_pathname__sz(pResultPath))  );
	if (pAncestorLabel == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_AncestorLabel, SG_pathname__sz(pAncestorPath))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_AncestorLabel, SG_string__sz(pAncestorLabel))  );
	}
	if (pBaselineLabel == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_BaselineLabel, SG_pathname__sz(pBaselinePath))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_BaselineLabel, SG_string__sz(pBaselineLabel))  );
	}
	if (pOtherLabel == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_OtherLabel,    SG_pathname__sz(pOtherPath))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_OtherLabel,    SG_string__sz(pOtherLabel))  );
	}
	if (pResultLabel == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_ResultLabel,   SG_pathname__sz(pResultPath))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_ResultLabel,   SG_string__sz(pResultLabel))  );
	}

	// invoke the tool
	SG_ERR_CHECK(  SG_filetool__invoke(pCtx, pTool, pTokenValues, &iResultCode)  );

	// check if it produced an output file
	SG_ERR_CHECK(  SG_fsobj__exists_stat__pathname(pCtx, pResultPath, &bResultExists, &cStat)  );
	if (
		   bResultExists == SG_TRUE
		&& (
			   (iResultCode == SG_FILETOOL__RESULT__ERROR)
			|| (bAncestorOverwrite == SG_TRUE && cStat.mtime_ms <= iBeforeTime)
			|| (bPremakeResult == SG_TRUE && cStat.size == 0)
			)
		)
	{
		// There's a result file, but either:
		// 1) the tool errored out, so we can't trust what's in it
		// 2) we created it earlier and the tool didn't modify it
		// Therefore the tool didn't really generate any output,
		// so get rid of the file.
		SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pResultPath)  );
		bResultExists = SG_FALSE;
	}

	// return the results
	if (pResultCode != NULL)
	{
		*pResultCode = iResultCode;
	}
	if (pResultExists != NULL)
	{
		*pResultExists = bResultExists;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pTokenValues);
	return;
}

void SG_mergetool__run(
	SG_context*          pCtx,
	SG_repo*             pRepo,
	const char*          szContext,
	const char*          szSuggestion,
	const SG_pathname*   pAncestorPath,
	const SG_string*     pAncestorLabel,
	const SG_pathname*   pBaselinePath,
	const SG_string*     pBaselineLabel,
	const SG_pathname*   pOtherPath,
	const SG_string*     pOtherLabel,
	const SG_pathname*   pResultPath,
	const SG_string*     pResultLabel,
	SG_int32*            pResultCode,
	SG_bool*             pResultExists
	)
{
	SG_filetool* pTool = NULL;

	SG_NULLARGCHECK(szContext);
	SG_NULLARGCHECK(pAncestorPath);
	SG_NULLARGCHECK(pBaselinePath);
	SG_NULLARGCHECK(pOtherPath);
	SG_NULLARGCHECK(pResultPath);

	SG_ERR_CHECK(  SG_mergetool__select(pCtx, szContext, NULL, pAncestorPath, szSuggestion, pRepo, &pTool)  );
	SG_ERR_CHECK(  SG_mergetool__invoke(pCtx,
										pTool,
										pAncestorPath, pAncestorLabel,
										pBaselinePath, pBaselineLabel,
										pOtherPath,    pOtherLabel,
										pResultPath,   pResultLabel,
										pResultCode,   pResultExists)  );

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return;
}
