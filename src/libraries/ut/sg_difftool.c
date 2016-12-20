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
 * @file sg_difftool.c
 *
 * @details Code to handle how we choose which diff-tool to use
 * to diff a set of files.
 * 
 * Created by PaulE, copied off of sg_mergetool* by Andy.
 *
 */

#include <sg.h>

///////////////////////////////////////////////////////////////////////////////


/*
**
** Types
**
*/

/**
 * Definition of an internal diff tool.
 */
typedef struct
{
	const char*           szName;    //< Unique name of the internal diff tool.
	SG_filetool__invoker* pInvoker;  //< Function that implements the tool.
}
_internal_tool;

/**
 * Functions that translate filetool invocations into difftool invocations.
 */
static SG_filetool__invoker _invoke__skip;
static SG_filetool__invoker _invoke__diff_strict;
static SG_filetool__invoker _invoke__diff_ignore_eols;
static SG_filetool__invoker _invoke__diff_ignore_whitespace;
static SG_filetool__invoker _invoke__diff_ignore_case;
static SG_filetool__invoker _invoke__diff_ignore_case_and_whitespace;
static SG_filetool__invoker _invoke__diffmerge;

/*
**
** Globals
**
*/

// token keys used by diff tools
static const char* gszToken_FromPath  = "FROM";
static const char* gszToken_ToPath    = "TO";
static const char* gszToken_FromLabel = "FROM_LABEL";
static const char* gszToken_ToLabel   = "TO_LABEL";

static const char* gszToken_RightSideIsWritable = "TO_WRITABLE";

/**
 * List of all internal diff tools.
 */
static _internal_tool aInternalTools[] =
{
	{ SG_DIFFTOOL__INTERNAL__SKIP,                            _invoke__skip                           },
	{ SG_DIFFTOOL__INTERNAL__DIFF_STRICT,                     _invoke__diff_strict                    },
	{ SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_EOLS,                _invoke__diff_ignore_eols               },
	{ SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_WHITESPACE,          _invoke__diff_ignore_whitespace         },
	{ SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_CASE,                _invoke__diff_ignore_case               },
	{ SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_CASE_AND_WHITESPACE, _invoke__diff_ignore_case_and_whitespace},
	{ SG_DIFFTOOL__INTERNAL__DIFFMERGE,                       _invoke__diffmerge                      },
};

/**
 * The hard-coded default tool for diff.
 */
static const char * _szDefaultTool = SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_EOLS;


/*
**
** Internal Functions
**
*/

/**
 * Invokes an internal difftool.
 * Basically translates the token values from a filetool call into the arguments for a difftool call.
 */
static void _invoke_built_in_tool(
	SG_context*                pCtx,         //< [in] [out] Error and context info.
	const SG_vhash*            pTokenValues, //< [in] Token/argument values passed to the tool.
	SG_difftool__built_in_tool fInvoker,     //< [in] The internal difftool implementation to invoke.
	SG_int32 * piResultCode                  //< [out] SG_FILETOOL__RESULT__ or SG_DIFFTOOL__RESULT__
	)
{
	SG_pathname* pFromPath  = NULL;
	SG_string*   pFromLabel = NULL;
	SG_pathname* pToPath    = NULL;
	SG_string*   pToLabel   = NULL;
	const char*  szTemp     = NULL;
	SG_bool bTemp = SG_FALSE;
	SG_bool bRightSideIsWritable = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTokenValues, gszToken_FromPath, &szTemp)  );
	if(szTemp)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pFromPath, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTokenValues, gszToken_FromLabel, &szTemp)  );
	if(szTemp)
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pFromLabel, szTemp)  );

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTokenValues, gszToken_ToPath, &szTemp)  );
	if(szTemp)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pToPath, szTemp)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTokenValues, gszToken_ToLabel, &szTemp)  );
	if(szTemp)
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pToLabel, szTemp)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pTokenValues, gszToken_RightSideIsWritable, &bTemp)  );
	if (bTemp)
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pTokenValues, gszToken_RightSideIsWritable, &bRightSideIsWritable)  );

	SG_ERR_CHECK(  fInvoker(pCtx, pFromPath, pToPath, pFromLabel, pToLabel, bRightSideIsWritable, NULL, piResultCode)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFromPath);
	SG_STRING_NULLFREE(pCtx, pFromLabel);
	SG_PATHNAME_NULLFREE(pCtx, pToPath);
	SG_STRING_NULLFREE(pCtx, pToLabel);
	return;
}

/**
 * Invokes the "skip" internal difftool.
 */
static void _invoke__skip(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          piResultCode
	)
{
	SG_UNUSED(pTool);

	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__skip, piResultCode)  );
}

static void _invoke__diff_strict(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* piResultCode)
{
	SG_UNUSED(pTool);
	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__textfilediff_strict, piResultCode)  );
}
static void _invoke__diff_ignore_eols(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* piResultCode)
{
	SG_UNUSED(pTool);
	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__textfilediff_ignore_eols, piResultCode)  );
}
static void _invoke__diff_ignore_whitespace(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* piResultCode)
{
	SG_UNUSED(pTool);
	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__textfilediff_ignore_whitespace, piResultCode)  );
}
static void _invoke__diff_ignore_case(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* piResultCode)
{
	SG_UNUSED(pTool);
	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__textfilediff_ignore_case, piResultCode)  );
}
static void _invoke__diff_ignore_case_and_whitespace(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* piResultCode)
{
	SG_UNUSED(pTool);
	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__textfilediff_ignore_case_and_whitespace, piResultCode)  );
}
static void _invoke__diffmerge(SG_context* pCtx, const SG_filetool* pTool, const SG_vhash* pTokenValues, SG_int32* piResultCode)
{
	SG_UNUSED(pTool);
	SG_ERR_CHECK_RETURN(  _invoke_built_in_tool(pCtx, pTokenValues, SG_difftool__built_in_tool__exec_diffmerge, piResultCode)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Filetool "plugin" function that locates SG_filetool__invoker
 * functions for the various internal diff tools.
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
	SG_UNUSED(szContext);
	SG_NULLARGCHECK(ppInvoker);

	if (szName != NULL)
	{
		for (uIndex = 0u; uIndex < SG_NrElements(aInternalTools); ++uIndex)
		{
			const _internal_tool* pTool = &(aInternalTools[uIndex]);

			if(strcmp(pTool->szName, szName) == 0)
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

//////////////////////////////////////////////////////////////////

/*
**
** Public Functions
**
*/

void SG_difftool__exists(
	SG_context* pCtx,
	const char* szName,
	SG_repo*    pRepo,
	SG_bool*    pExists
	)
{
	SG_filetool* pTool = NULL;

	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(pExists);

	SG_ERR_CHECK(  SG_filetool__get_tool(pCtx, SG_DIFFTOOL__TYPE, NULL, szName, _filetool_plugin, pRepo, NULL, &pTool)  );
	*pExists = (pTool == NULL) ? SG_FALSE : SG_TRUE;

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return;
}

void SG_difftool__list(
	SG_context*      pCtx,
	SG_repo*         pRepo,
	SG_stringarray** ppTools
	)
{
	SG_stringarray* pTools = NULL;
	SG_uint32       uIndex = 0u;

	// add all the diff tools from config
	SG_ERR_CHECK(  SG_filetool__list_tools(pCtx, SG_DIFFTOOL__TYPE, pRepo, &pTools)  );

	// add all the internal diff tools
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

void SG_difftool__select(
	SG_context*        pCtx,
	const char*        szContext,
	const char*        szPath,
	const SG_pathname* pDiskPath,
	const char*        szOverride,
	SG_repo*           pRepo,
	SG_filetool**      ppTool
	)
{
	SG_string*   sClass = NULL;
	SG_string*   sTool  = NULL;
	SG_vhash*    pResultMap = NULL;
	SG_filetool* pTool  = NULL;

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
		SG_ERR_CHECK(  SG_filetool__find_tool(pCtx, SG_DIFFTOOL__TYPE, szContext, SG_string__sz(sClass), pRepo, &sTool)  );
		if (sTool == NULL)
		{
			// couldn't find a tool, use the default one
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sTool, _szDefaultTool)  );
		}
		SG_ASSERT(sTool != NULL);
	}

	// build a result map for difftool results
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pResultMap)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResultMap, SG_DIFFTOOL__RESULT__SAME__SZ,      SG_DIFFTOOL__RESULT__SAME)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResultMap, SG_DIFFTOOL__RESULT__DIFFERENT__SZ, SG_DIFFTOOL__RESULT__DIFFERENT)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pResultMap, SG_DIFFTOOL__RESULT__CANCEL__SZ,    SG_DIFFTOOL__RESULT__CANCEL)  );

	// look up the tool we found
	// TODO: Someday it would be nice to have a mechanism for verifying that the
	//       tool actually works with the class of the file that we're working with.
	//       Update 2013/01/18 The new SG_DIFFTOOL__RESULT__CANCEL can be used by the
	//       tool to signal that -- but that's after-the-fact, so it's not as good as
	//       a pre-check.
	SG_ERR_CHECK(  SG_filetool__get_tool(pCtx, SG_DIFFTOOL__TYPE, szContext, SG_string__sz(sTool), _filetool_plugin, pRepo, pResultMap, &pTool)  );
	if (pTool == NULL)
	{
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "No settings found for difftool: %s", SG_string__sz(sTool))  );
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

void SG_difftool__invoke(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_pathname* pFromPath,
	const SG_string*   pFromLabel,
	const SG_pathname* pToPath,
	const SG_string*   pToLabel,
	SG_bool            bRightSideIsWritable,
	SG_int32 * piResultCode
	)
{
	SG_tempfile* pTempFile    = NULL;
	SG_vhash*    pTokenValues = NULL;
	SG_int32     iResultCode  = 0;

	SG_NULLARGCHECK_RETURN(pTool);
	SG_ARGCHECK(pFromPath != NULL || pToPath != NULL, pFromPath+pToPath);

	// create an empty temp file for missing paths
	// that way a non-existant file will show up as empty in a difftool
	if (pFromPath == NULL)
	{
		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFile)  );
		SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFile)  );
		pFromPath = pTempFile->pPathname;
	}
	if (pToPath == NULL)
	{
		SG_ASSERT(pTempFile == NULL);
		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFile)  );
		SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFile)  );
		pToPath = pTempFile->pPathname;
	}

	// bundle all of the arguments up into a hash
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTokenValues)  );
	if (pFromPath != NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_FromPath,  SG_pathname__sz(pFromPath))  );
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_FromLabel, SG_pathname__sz(pFromPath))  );
	}
	if (pToPath != NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_ToPath,    SG_pathname__sz(pToPath))  );
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_ToLabel,   SG_pathname__sz(pToPath))  );
	}
	if (pFromLabel != NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_FromLabel, SG_string__sz(pFromLabel))  );
	}
	if (pToLabel != NULL)
	{
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pTokenValues, gszToken_ToLabel,   SG_string__sz(pToLabel))  );
	}
	SG_ERR_CHECK(  SG_vhash__update__bool(pCtx, pTokenValues, gszToken_RightSideIsWritable, bRightSideIsWritable)  );

	// invoke the tool
	SG_ERR_CHECK(  SG_filetool__invoke(pCtx, pTool, pTokenValues, &iResultCode)  );

	if (iResultCode == SG_FILETOOL__RESULT__ERROR)
	{
		const char * pszName = NULL;
		SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &pszName)  );
		SG_ERR_CHECK(  SG_log__report_error(pCtx, "An error occurred launching the diff tool %s", pszName)  );
	}

	if (piResultCode)
		*piResultCode = iResultCode;

fail:
	SG_VHASH_NULLFREE(pCtx, pTokenValues);
	if (pTempFile != NULL)
	{
		SG_ERR_IGNORE(  SG_tempfile__delete(pCtx, &pTempFile)  );
	}
	return;
}

void SG_difftool__run(
	SG_context*        pCtx,
	const char*        szPath,
	SG_repo*           pRepo,
	const char*        szContext,
	const char*        szSuggestion,
	const SG_pathname* pFromPath,
	const SG_string*   pFromLabel,
	const SG_pathname* pToPath,
	const SG_string*   pToLabel,
	SG_bool            bRightSideIsWritable,
	SG_int32*          piResultCode,
	char **            ppszToolNameUsed
	)
{
	SG_filetool* pTool = NULL;

	if(pFromPath==NULL && pToPath==NULL)
		SG_ERR_THROW2_RETURN(SG_ERR_INVALIDARG, (pCtx, "pFromPath and pToPath cannot both be NULL"));

	if(pFromPath!=NULL)
		SG_ERR_CHECK(  SG_difftool__select(pCtx, szContext, szPath, pFromPath, szSuggestion, pRepo, &pTool)  );
	else
		SG_ERR_CHECK(  SG_difftool__select(pCtx, szContext, szPath, pToPath, szSuggestion, pRepo, &pTool)  );

	SG_ERR_CHECK(  SG_difftool__invoke(pCtx, pTool, pFromPath, pFromLabel, pToPath, pToLabel, bRightSideIsWritable,
									   piResultCode)  );

	if (ppszToolNameUsed)
	{
		const char * pszName = NULL;
		SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &pszName)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszName, ppszToolNameUsed)  );
	}

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return;
}

///////////////////////////////////////////////////////////////////////////////

void SG_difftool__built_in_tool__skip(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string ** ppOutput,
	SG_int32 * piResultCode)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pPathname0);
	SG_UNUSED(pPathname1);
	SG_UNUSED(pLabel0);
	SG_UNUSED(pLabel1);
	SG_UNUSED( bRightSideIsWritable );

	if(ppOutput!=NULL)
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, ppOutput)  ); // Return an empty string.

	*piResultCode = SG_DIFFTOOL__RESULT__CANCEL;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _sg_difftool__perform_textfilediff(
    SG_context * pCtx,
    SG_textfilediff_options diffOptions,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
    SG_string **ppOutput,
	SG_int32 * piResultCode)
{
    SG_textfilediff_t * pDiff = NULL;
    SG_string * pOutput = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_ARGCHECK_RETURN(pPathname0!=NULL || pPathname1!=NULL, pPathname1);

    SG_ERR_CHECK(  SG_textfilediff(pCtx, pPathname0, pPathname1, diffOptions, &pDiff)  );
    SG_ERR_CHECK(  SG_textfilediff__output_unified__string(pCtx, pDiff, pLabel0, pLabel1, 3, &pOutput)  );

    SG_TEXTFILEDIFF_NULLFREE(pCtx, pDiff);

	if(ppOutput!=NULL)
		*ppOutput = pOutput;
	else
	{
		SG_ASSERT(pOutput!=NULL);
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s", SG_string__sz(pOutput))  );
		SG_STRING_NULLFREE(pCtx, pOutput);
	}

	*piResultCode = SG_FILETOOL__RESULT__SUCCESS;
    return;
fail:
    SG_TEXTFILEDIFF_NULLFREE(pCtx, pDiff);
    SG_STRING_NULLFREE(pCtx, pOutput);
}

void SG_difftool__built_in_tool__textfilediff_strict(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string **ppOutput,
	SG_int32 * piResultCode)
{
	SG_UNUSED( bRightSideIsWritable );

    SG_ERR_CHECK_RETURN(  _sg_difftool__perform_textfilediff(
        pCtx,
        SG_TEXTFILEDIFF_OPTION__STRICT_EOL,
        pPathname0, pPathname1, pLabel0, pLabel1, ppOutput, piResultCode)  );
}

void SG_difftool__built_in_tool__textfilediff_ignore_eols(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string **ppOutput,
	SG_int32 * piResultCode)
{
	SG_UNUSED( bRightSideIsWritable );

    SG_ERR_CHECK_RETURN(  _sg_difftool__perform_textfilediff(
        pCtx,
        SG_TEXTFILEDIFF_OPTION__REGULAR,
        pPathname0, pPathname1, pLabel0, pLabel1, ppOutput, piResultCode)  );
}

void SG_difftool__built_in_tool__textfilediff_ignore_whitespace(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string **ppOutput,
	SG_int32 * piResultCode)
{
	SG_UNUSED( bRightSideIsWritable );

    SG_ERR_CHECK_RETURN(  _sg_difftool__perform_textfilediff(
        pCtx,
        SG_TEXTFILEDIFF_OPTION__IGNORE_WHITESPACE,
        pPathname0, pPathname1, pLabel0, pLabel1, ppOutput, piResultCode)  );
}

void SG_difftool__built_in_tool__textfilediff_ignore_case(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string **ppOutput,
	SG_int32 * piResultCode)
{
	SG_UNUSED( bRightSideIsWritable );

    SG_ERR_CHECK_RETURN(  _sg_difftool__perform_textfilediff(
        pCtx,
        SG_TEXTFILEDIFF_OPTION__STRICT_EOL|SG_TEXTFILEDIFF_OPTION__IGNORE_CASE,
        pPathname0, pPathname1, pLabel0, pLabel1, ppOutput, piResultCode)  );
}

void SG_difftool__built_in_tool__textfilediff_ignore_case_and_whitespace(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string **ppOutput,
	SG_int32 * piResultCode)
{
	SG_UNUSED( bRightSideIsWritable );

    SG_ERR_CHECK_RETURN(  _sg_difftool__perform_textfilediff(
        pCtx,
        SG_TEXTFILEDIFF_OPTION__IGNORE_CASE|SG_TEXTFILEDIFF_OPTION__IGNORE_WHITESPACE,
        pPathname0, pPathname1, pLabel0, pLabel1, ppOutput, piResultCode)  );
}

//////////////////////////////////////////////////////////////////

void SG_difftool__built_in_tool__exec_diffmerge(
    SG_context * pCtx,
    const SG_pathname * pPathname0,
    const SG_pathname * pPathname1,
    const SG_string * pLabel0,
    const SG_string * pLabel1,
	SG_bool bRightSideIsWritable,
    SG_string ** ppOutput,
	SG_int32 * piResultCode     //< [out] SG_FILETOOL__RESULT__ or SG_DIFFTOOL__RESULT__
)
{
	SG_pathname * pPathCmd = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_file * pNull = NULL;
	SG_exit_status childExitStatus;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pPathname0);
	SG_NULLARGCHECK_RETURN(pPathname1);

	SG_ERR_CHECK(  SG_filetool__get_builtin_diffmerge_exe(pCtx, &pPathCmd)  );

	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx,&pArgVec)  );

	if(pLabel0!=NULL)
	{
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-t1")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_string__sz(pLabel0))  );
	}

	if(pLabel1!=NULL)
	{
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-t2")  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_string__sz(pLabel1))  );
	}

	if (!bRightSideIsWritable)
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-ro2")  );

	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathname0))  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathname1)  )  );

	// exec diffmerge and wait for exit status.

#if defined(MAC) || defined(LINUX)
	SG_ERR_CHECK(  SG_file__open__null_device(pCtx, SG_FILE_RDONLY, &pNull)  );
#else
	SG_UNUSED(pNull);
#endif
	SG_ERR_CHECK(  SG_exec__exec_sync__files(pCtx, SG_pathname__sz(pPathCmd), pArgVec, NULL, NULL, pNull, &childExitStatus)  );
	SG_FILE_NULLCLOSE(pCtx, pNull);

	// The ":diffmerge" tool is a little unique.  We claim it is a "built-in" tool,
	// but then we secretly exec() it.  This tool was defined *BEFORE* we had the
	// SG_filetools and SG_localsettings machinery.  They now define a "diffmerge"
	// tool that allows for user-configuration and so that we can treat DIffMerge
	// like any other third-party tool.  The definition of ":diffmerge" here allows
	// for a minimal/absolute default fall-back (and would let us easily lookup
	// stuff in the registry, for example).
	//
	// Because the internal tools don't go through the general sg_filetool.c:_translate_exit_code()
	// machinery (that we only use for external tools), we do the mapping here.
	//
	// The following is equivalent to:
	//     "filetools/diff/:diffmerge/exit/0 ==> success"
	//     "filetools/diff/:diffmerge/exit/1 ==> different"	(DiffMerge only returns this in batch mode.)
	// with anything else mapping to "failure".

	switch (childExitStatus)
	{
	case 0:
		*piResultCode = SG_FILETOOL__RESULT__SUCCESS;
		break;

	case 1:
		*piResultCode = SG_DIFFTOOL__RESULT__DIFFERENT;
		break;

	default:
		*piResultCode = SG_FILETOOL__RESULT__FAILURE;
		break;
	}

	if(ppOutput!=NULL)
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, ppOutput)  ); // Return an empty string.

fail:
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_PATHNAME_NULLFREE(pCtx, pPathCmd);
	SG_FILE_NULLCLOSE(pCtx, pNull);
}

//////////////////////////////////////////////////////////////////

void SG_difftool__check_result_code__throw(SG_context * pCtx,
										   SG_int64 i64Result,
										   const char * pszTool)
{
	switch (i64Result)
	{
	default:
	case SG_FILETOOL__RESULT__SUCCESS:
	case SG_DIFFTOOL__RESULT__SAME:
	case SG_DIFFTOOL__RESULT__DIFFERENT:
		break;

	case SG_DIFFTOOL__RESULT__CANCEL:
		SG_ERR_THROW2(  SG_ERR_DIFFTOOL_CANCELED,
						(pCtx, "The files are different, but the '%s' tool cannot display them.", pszTool)  );

	case SG_FILETOOL__RESULT__FAILURE:
		SG_ERR_THROW2(  SG_ERR_EXTERNAL_TOOL_ERROR,
						(pCtx, "The tool '%s' reported an error.", pszTool)  );

	case SG_FILETOOL__RESULT__ERROR:
		SG_ERR_THROW2(  SG_ERR_EXTERNAL_TOOL_ERROR,
						(pCtx, "There was an error launching tool '%s'.", pszTool)  );
	}

fail:
	return;
}
