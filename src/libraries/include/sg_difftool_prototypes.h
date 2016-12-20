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
 * functionality specifically for finding and using diff tools.
 * It also implements several internal diff tools.
 * 
 * Created by PaulE, copied off of sg_mergetool* by Andy.
 * 
 * Note this is for showing a diff to the user, not finding out whether there
 * were changes. That's why __invoke() and __run() don't return any sort of
 * exit status or anything (other than pCtx itself).
 */

#ifndef H_SG_DIFFTOOL_PROTOTYPES_H
#define H_SG_DIFFTOOL_PROTOTYPES_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


/**
 * Check if a diff tool exists.
 */
void SG_difftool__exists(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	const char* szName,    //< [in] Name of the diff tool to check for.
	SG_repo*    pRepo,     //< [in] Repo to check for the diff tool in.
	                       //<      If NULL, then repo-specific tools will not be found.
	SG_bool*    pExists    //< [out] Set to SG_TRUE if the given tool was found, or SG_FALSE if it wasn't.
	);

/**
 * Generates a list of available diff tools.
 */
void SG_difftool__list(
	SG_context*      pCtx,  //< [in] [out] Error and context info.
	SG_repo*         pRepo, //< [in] The repo to include repo-specific tools from.
	                        //<      If NULL, then only built-in and machine-wide tools are included.
	SG_stringarray** pTools //< [out] A list of available diff tools.
	);

/**
 * Find the best diff tool for the given circumstances.
 */
void SG_difftool__select(
	SG_context*        pCtx,       //< [in] [out] Error and context info.
	const char*        szContext,    //< [in] The context invoking the diff (one of SG_DIFFTOOL__CONTEXT__*).
	const char*        szPath,     //< [in] The repo path or disk path of the file to select the tool for.
	                               //<      If NULL, pDiskPath will be used.
	                               //<      This is passed separately because sometimes the actual file on disk will have a mangled name.
	const SG_pathname* pDiskPath,  //< [in] If pattern-matching on szPath fails, we'll use this path to decide based on file contents.
	const char*        szOverride, //< [in] The name of a tool to use, overriding the one that would be selected automatically.
	                               //<      If non-NULL, this named tool is always used.
	SG_repo*           pRepo,      //< [in] Repo to look up externally defined tools in.
	                               //<      If NULL, then repo-specific tools will not be considered.
	SG_filetool**      ppTool      //< [out] The best tool that could be found for diffing.
	                               //<       Set to NULL if no suitable tool was found.
	);

/**
 * Invokes a diff tool to perform a diff.
 *
 * Map the tool's exit status back to our status.
 */
void SG_difftool__invoke(
	SG_context*        pCtx,       //< [in] [out] Error and context info.
	const SG_filetool* pTool,      //< [in] The tool to invoke to perform the diff.
	const SG_pathname* pFromPath,  //< [in] Path to the "from" file for the diff.
	const SG_string*   pFromLabel, //< [in] Friendly label for the "from" file, or NULL to use the path.
	const SG_pathname* pToPath,    //< [in] Path to the "to" file for the diff.
	const SG_string*   pToLabel,   //< [in] Friendly label for the "to" file, or NULL to use the path.
	SG_bool            bRightSideIsWritable, //< [in] Is the right side of the diff writable (live in the WD rather than a temp/historical view)
	SG_int32*          piResultCode //< [out] translated exit/result code from tool
	);

/**
 * Select and invoke a diff tool in one step.
 */
void SG_difftool__run(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const char*        szPath,       //< [in] The repo path or disk path of the file to select the tool for.
	                                 //<      If NULL, pFromPath will be used.
	                                 //<      This is passed separately because sometimes the actual file on disk will have a mangled name.
	SG_repo*           pRepo,        //< [in] Repo to look for defined tools in.
	const char*        szContext,    //< [in] The context invoking the diff (one of SG_DIFFTOOL__CONTEXT__*).
	const char*        szSuggestion, //< [in] The name of a tool that the caller suggests be used.
	const SG_pathname* pFromPath,    //< [in] Path to the "from" file for the diff.
	const SG_string*   pFromLabel,   //< [in] Friendly label for the "from" file, or NULL to use the path.
	const SG_pathname* pToPath,      //< [in] Path to the "to" file for the diff.
	const SG_string*   pToLabel,     //< [in] Friendly label for the "to" file, or NULL to use the path.
	SG_bool            bRightSideIsWritable, //< [in] Is the right side of the diff writable (live in the WD rather than a temp/historical view)
	SG_int32*          piResultCode,      //< [out] (optional) translated exit/result code from tool
	char **            ppszToolNameUsed   //< [out] (optional) name of the tool actually used
	);


SG_difftool__built_in_tool SG_difftool__built_in_tool__skip;
SG_difftool__built_in_tool SG_difftool__built_in_tool__textfilediff_strict;
SG_difftool__built_in_tool SG_difftool__built_in_tool__textfilediff_ignore_eols;
SG_difftool__built_in_tool SG_difftool__built_in_tool__textfilediff_ignore_whitespace;
SG_difftool__built_in_tool SG_difftool__built_in_tool__textfilediff_ignore_case;
SG_difftool__built_in_tool SG_difftool__built_in_tool__textfilediff_ignore_case_and_whitespace;

SG_difftool__built_in_tool SG_difftool__built_in_tool__exec_diffmerge;

void SG_difftool__check_result_code__throw(SG_context * pCtx,
										   SG_int64 i64Result,
										   const char * pszTool);

///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
