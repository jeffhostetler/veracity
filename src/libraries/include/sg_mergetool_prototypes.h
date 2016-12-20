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

/*
 * This module layers on top of "filetool" and adds
 * functionality specifically for finding and using merge tools.
 * It also implements several internal merge tools.
 */

#ifndef H_SG_MERGETOOL_PROTOTYPES_H
#define H_SG_MERGETOOL_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Check if a merge tool exists.
 */
void SG_mergetool__exists(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	const char* szContext, //< [in] Merge context to check for a tool for (AUTO or MANUAL).
	const char* szName,    //< [in] Name of the merge tool to check for.
	SG_repo*    pRepo,     //< [in] Repo to check for the merge tool in.
	                       //<      If NULL, then repo-specific tools will not be found.
	SG_bool*    pExists    //< [out] Set to SG_TRUE if the given tool was found, or SG_FALSE if it wasn't.
	);

/**
 * Generates a list of available merge tools.
 */
void SG_mergetool__list(
	SG_context*      pCtx,  //< [in] [out] Error and context info.
	SG_repo*         pRepo, //< [in] The repo to include repo-specific tools from.
	                        //<      If NULL, then only built-in and machine-wide tools are included.
	SG_stringarray** pTools //< [out] A list of available merge tools.
	);

/**
 * Find the best merge tool for the given circumstances.
 */
void SG_mergetool__select(
	SG_context*        pCtx,       //< [in] [out] Error and context info.
	const char*        szContext,  //< [in] The context of the merge (MERGE or RESOLVE).
	const char*        szPath,     //< [in] The repo path or disk path of the file to select the tool for (usually the ancestor).
	                               //<      If NULL, pDiskPath will be used.
	                               //<      This is passed separately because sometimes the actual file on disk will have a mangled name.
	const SG_pathname* pDiskPath,  //< [in] Actual file on disk (mangled filename or not) to examine for looking up an appropriate tool.
	const char*        szOverride, //< [in] The name of a tool to use, overriding the one that would be selected automatically.
	                               //<      If non-NULL, this named tool is always used.
	SG_repo*           pRepo,      //< [in] Repo to look up externally defined tools in.
	                               //<      If NULL, then repo-specific tools will not be considered.
	SG_filetool**      ppTool      //< [out] The tool selected for the specified circumstances.
	                               //<       Set to NULL if no suitable tool was found.
	);

/**
 * Invokes a merge tool to perform a merge.
 *
 * Map the tool's exit status back to our status.
 */
void SG_mergetool__invoke(
	SG_context*          pCtx,           //< [in] [out] Error and context info.
	const SG_filetool*   pTool,          //< [in] The tool to invoke to perform the merge.
	const SG_pathname*   pAncestorPath,  //< [in] Path to the ancestor file for the merge.
	const SG_string*     pAncestorLabel, //< [in] Friendly label for the ancestor file, or NULL to use the path.
	const SG_pathname*   pBaselinePath,  //< [in] Path to the "baseline" file for the merge.
	const SG_string*     pBaselineLabel, //< [in] Friendly label for the "baseline" file, or NULL to use the path.
	const SG_pathname*   pOtherPath,     //< [in] Path to the "other" file for the merge.
	const SG_string*     pOtherLabel,    //< [in] Friendly label for the "other" file, or NULL to use the path.
	const SG_pathname*   pResultPath,    //< [in] Path to the result file for the merge.
	const SG_string*     pResultLabel,   //< [in] Friendly label for the "result" file, or NULL to use the path.
	SG_int32*            pResultCode,    //< [out] Result returned by the tool.
	                                     //<       One of SG_MERGETOOL__RESULT__* or SG_FILETOOL__RESULT__*
	                                     //<       May be NULL if not needed.
	SG_bool*             pResultExists   //< [out] Whether or not the tool produced a result file at pResultPath.
	                                     //<       If this is true, caller is responsible for the file.
	                                     //<       May be NULL if not needed.
	);

/**
 * Select and invoke a merge tool in one step.
 */
void SG_mergetool__run(
	SG_context*          pCtx,           //< [in] [out] Error and context info.
	SG_repo*             pRepo,          //< [in] Repo to look for defined tools in.
	const char*          szContext,      //< [in] The context of the merge (AUTO or MANUAL).
	const char*          szSuggestion,   //< [in] The name of a tool that the caller suggests be used.
	const SG_pathname*   pAncestorPath,  //< [in] Path to the ancestor file for the merge.
	const SG_string*     pAncestorLabel, //< [in] Friendly label for the ancestor file, or NULL to use the path.
	const SG_pathname*   pBaselinePath,  //< [in] Path to the "baseline" file for the merge.
	const SG_string*     pBaselineLabel, //< [in] Friendly label for the "baseline" file, or NULL to use the path.
	const SG_pathname*   pOtherPath,     //< [in] Path to the "other" file for the merge.
	const SG_string*     pOtherLabel,    //< [in] Friendly label for the "other" file, or NULL to use the path.
	const SG_pathname*   pResultPath,    //< [in] Path to the result file for the merge.
	const SG_string*     pResultLabel,   //< [in] Friendly label for the "result" file, or NULL to use the path.
	SG_int32*            pResultCode,    //< [out] Result returned by the tool.
	                                     //<       One of SG_MERGETOOL__RESULT__* or SG_FILETOOL__RESULT__*
	SG_bool*             pResultExists   //< [out] Whether or not the tool produced a result file at pResultPath.
	                                     //<       If this is true, caller is responsible for the file.
	);

END_EXTERN_C;

#endif
