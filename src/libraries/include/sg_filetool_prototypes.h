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
 * This module defines a "filetool" object and associated functionality.
 *
 * A filetool is a specification of a program/function that operates on one or more files.
 * Some tools are "built in" or "internal", which is to say they're compiled into the code.
 * Other tools are "external", meaning they execute an external program.
 *
 * This module is responsible for:
 * - Classifying files by type (such as text, binary, etc).
 * - The list of file types/classes and how types are defined in localsettings/config.
 * - The set of known tool types.
 * - The list of external tools and how they are defined in localsettings/config.
 * - Invoking tools using a set of arguments.
 *
 * There is currently no code for creating/manipulating/managing tools or file types.
 * It's expected that all of that work will occur via localsettings/config.
 * This module just finds/loads that information and puts it to use.
 *
 * In the future, it is hoped that functionality involving file types and
 * classification can be factored out into a seperate "filetype" module.
 *
 * This module is generally used as follows:
 * - Call get_class to determine the type/class of the file(s) you're working with.
 * - Call find_tool to allocate a tool for your needs.
 *   - To include some of your own internal tools in the search, implement and pass a "plugin" callback
 * - Build a vhash of arguments to pass to the tool.
 *   - The argument keys to use depends on the tool type.
 *   - See the types enumeration for more information.
 * - Call invoke_tool to run the tool.
 */

#ifndef H_SG_FILETOOL_PROTOTYPES_H
#define H_SG_FILETOOL_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Determines the class of a file (i.e. text, binary, etc).
 */
void SG_filetool__get_class(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const char*        szPath,    //< [in] The repo path or disk path of the file to get the class of.
	                              //<      If NULL, pDiskPath will be used.
	                              //<      This is passed separately because sometimes the actual file on disk will have a mangled name.
	const SG_pathname* pDiskPath, //< [in] If pattern-matching on szPath fails, we'll use this path to decide based on file contents.
	SG_repo*           pRepo,     //< [in] Repo to consider file classes from.
	                              //<      If NULL, then repo-specific file classes will not be considered.
	SG_string**        ppClass    //< [out] The class name of the given file/path.
	                              //<       Never returns NULL, returns SG_FILETOOL__CLASS__DEFAULT if no appropriate class was found.
	                              //<       Caller owns this.
	);

/**
 * Determines the default class for a file by examining its contents.
 *
 * This function is used by SG_filetool__get_class when pattern matching
 * on the file's path fails.  Generally you'll want to use that function to
 * determine the class of a file, this function just encapsulates the binary/text
 * detection algorithm.
 */
void SG_filetool__get_default_class(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const SG_pathname* pPath,  //< [in] Path of the file to determine the default class of.
	const char**       ppClass //< [out] The default class for the given file.
	                           //<       One of SG_FILETOOL__CLASS__*
	);

/**
 * Finds the most appropriate tool for a given set of circumstances:
 * - Type of tool needed.
 * - Context the tool is being used in (if the tool type uses contexts).
 * - Class of file the tool will be used on.
 */
void SG_filetool__find_tool(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	const char*         szType,    //< [in] The type of file tool to load.
	const char*         szContext, //< [in] The type-specific context to find a tool for.
	                               //<      Can be NULL to ignore context-specific tools (or if the type doesn't use contexts).
	const char*         szClass,   //< [in] The class of file to load a tool for.
	SG_repo*            pRepo,     //< [in] The repo to look up tools in.
	                               //<      If NULL, then only built-in and machine-wide tools will be considered.
	SG_string**         ppName     //< [out] The name of the found tool, or NULL if none was found.
	                               //<       Caller owns this.
	);

/**
 * Loads a file tool by its name.
 *
 * More about pResultMap:
 *
 * This parameter's main purpose is to allow the definition of type-specific
 * result conditions.  For example, mergetools may want to define results that
 * mean the user canceled the merge, or left the merged file in a conflicting state.
 * Each of these result conditions needs a string AND integer definition.  The
 * string definition is used in localsettings/config to allow users to map exit
 * codes to result strings, allowing users to configure a tool such that its exit
 * codes can be interpreted in a meaningful way.  The integer definition is used
 * by code that invokes the tool, because it's more convenient and efficient for
 * callers to check result integers rather than result strings.  To add these
 * type-specific results to a tool, add entries to this vhash that map each
 * result's string to its integer.
 *
 * In some circumstances, it may also be helpful to define type-specific exit codes.
 * For example, if all external tools of a particular type have standardized exit
 * codes, then they can also be included in this map so that all such tools don't
 * have to be configured individually in the localsettings/config to map the exit
 * codes to their equivalent result conditions.  To do this, add entries to this
 * vhash that map the lexical string representation of each exit code to the string
 * representation of the equivalent result.  For example, to map the exit code 5
 * to the tool-specific result "cancel", add an entry that maps "5" to "cancel".
 * (Due to the main purpose described above, the map should also contain an entry
 * that maps "cancel" to your type-specific integer value for it, which callers
 * will use to determine that the tool returned this result.)  The only reason
 * that the lexical string representations of exit codes are used as keys is
 * because we don't have a common associative container that uses integer keys.
 */
void SG_filetool__get_tool(
	SG_context*         pCtx,       //< [in] [out] Error and context info.
	const char*         szType,     //< [in] The type of file tool to load.
	const char*         szContext,  //< [in] The type-specific context to load the tool for.
	                                //<      Can be NULL if no specific context is being used.
	const char*         szName,     //< [in] The name of the tool to retrieve.
	SG_filetool__plugin fPlugin,    //< [in] Callback to use to find invokers for built-in tools implemented by the caller.
	                                //<      Can be NULL if no built-in tools are provided by the caller.
	SG_repo*            pRepo,      //< [in] The repo to look up tools in.
	                                //<      If NULL, then only built-in and machine-wide tools will be considered.
	const SG_vhash*     pResultMap, //< [in] Definition of type-specific result strings to apply to the tool.
	                                //<      Map result strings to their corresponding integer values.
	                                //<      May also contain exit code mappings, see function comments.
	SG_filetool**       ppTool      //< [out] The loaded tool, or NULL if none was found.
	                                //<       Caller owns this.
	);

/**
 * Gets a list of defined tools of a given type.
 */
void SG_filetool__list_tools(
	SG_context*      pCtx,   //< [in] [out] Error and context info.
	const char*      szType, //< [in] The type of tool to get a list of.
	SG_repo*         pRepo,  //< [in] The repo to include repo-specific tools from.
	                         //<      If NULL, then only built-in and machine-wide tools are included.
	SG_stringarray** pTools  //< [out] The list of tools.
	);

/**
 * Frees a filetool.
 */
void SG_filetool__free(
	SG_context*  pCtx, //< [in] [out] Error and context info.
	SG_filetool* pTool //< The tool to free.
	);

/**
 * Frees a filetool outside of the current error level.
 * Also NULLs the pointer after freeing.
 */
#define SG_FILETOOL_NULLFREE(pCtx, pTool)      \
	SG_STATEMENT(                              \
		SG_context__push_level(pCtx);          \
		SG_filetool__free(pCtx, pTool);        \
		SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); \
		SG_context__pop_level(pCtx);           \
		pTool = NULL;                          \
	)

/**
 * Gets the name of a tool.
 */
void SG_filetool__get_name(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	const SG_filetool* pTool, //< [in] Tool to get the name of.
	const char**       ppName //< [out] Name of the given tool.
	                          //<       Caller does NOT own this.
	);

/**
 * Gets the executable path of a tool.
 */
void SG_filetool__get_executable(
	SG_context*        pCtx,        //< [in] [out] Error and context info.
	const SG_filetool* pTool,       //< [in] Tool to get the executable path of.
	const char**       ppExecutable //< [out] Executable path of the given tool.
	                                //<       Set to NULL if the tool is built-in.
	                                //<       Caller does NOT own this.
	);

/**
 * Checks if a filetool's specification includes a particular boolean flag.
 *
 * The list of available flags depends on the type of filetool.  Check each tool
 * type's implementation for flag definitions that have meaning for that type of tool.
 * Flag names are case-sensitive.
 */
void SG_filetool__has_flag(
	SG_context*        pCtx,    //< [in] [out] Error and context info.
	const SG_filetool* pTool,   //< [in] The tool to check for a flag.
	const char*        szFlag,  //< [in] The flag to check for (case-sensitive).
	SG_bool*           pHasFlag //< [out] True if the tool has the given flag, false if it does not.
	);

/**
 * Invokes a tool.
 */
void SG_filetool__invoke(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const SG_filetool* pTool,        //< [in] The tool to invoke.
	const SG_vhash*    pTokenValues, //< [in] Values for placeholder tokens in the tool's arguments.
	                                 //<      See SG_filetool__replace_tokens for more info.
	                                 //<      May be NULL if there are no values to substitute.
	SG_int32*          pResult       //< [out] The result of invoking the tool.
	                                 //<       SG_FILETOOL__RESULT__* or a type-specific result.
	                                 //<       This is NOT equal to the exit code of an external tool.
	                                 //<       External exit codes will have been translated to results.
	                                 //<       May be NULL if not needed.
	);

/**
 * An internal filetool that does nothing and always succeeds.
 */
void SG_filetool__invoke__null(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pResult
	);

/**
 * An SG_filetool__invoker that invokes an external program.
 *
 * This function is mainly useful within custom external invokers,
 * when you just want to do something before/after the program is run.
 * Just do the extra thing yourself and call this function to handle executing the program.
 *
 * This function also handles translating exit codes from the external
 * tool into a suitable result code.
 */
void SG_filetool__invoke__external(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pResult
	);

/**
 * Replaces tokens embedded in a string with actual values from a hash.
 *
 * Tokens can be embedded in several different configuration settings for a tool, mainly arguments.
 * These tokens are replaced with actual data when the tool is invoked.
 * An obvious example of a token is FILENAME which would be replaced with the actual filename to pass to the tool.
 * This function does the actual work of replacing tokens with values in an arbitrary string.
 *
 * The pValues argument contains the values that need to replace tokens.
 * The keys are the names of tokens, and the values determine how that token should be replaced.
 * The keys are token names only, with no additional punctuation or syntax as they
 * would be used in a tool's configuration settings.  For example, if a tool's arguments
 * contain "--filename=@FILENAME@", you want to pass the value to swap in for @FILENAME@
 * under the key "FILENAME" without the @ symbols.  What those symbols mean and how they
 * are embedded into arguments is entirely a filetool implementation detail that is only
 * important when working with config settings.  Callers of filetool should never have to
 * deal with them.
 *
 * Filetool currently supports tokens with the following value types:
 *
 * String:  A token with a string value results in a straight-forward replacement of
 *          "@TOKEN_NAME@" with the string value associated with "TOKEN_NAME".
 * Integer: A token with an integer value is converted to a string and then treated
 *          as if it were a string value.
 * Boolean: A token with a boolean value is used in config settings like this:
 *          "@TOKEN_NAME|TRUE|FALSE@".  If the token's value is true then
 *          the entire token is replaced by TRUE, and likewise if the value is
 *          false then it is replaced with FALSE.  This allows each tool to determine
 *          the substitution string when a token is true or false.  Either TRUE or FALSE
 *          can also be empty, and if FALSE is empty the token can be shorted to
 *          "@TOKEN_NAME|TRUE@" (whereas if TRUE is empty, you need to leave a blank
 *          string in its place like this: "@TOKEN_NAME||FALSE@").  An example of this
 *          is be a read-only token, that might be used like this: "@READONLY|--read-only@",
 *          meaning that if the READONLY token is true, then the --read-only flag is
 *          passed to the tool, and if it's false then it isn't.
 */
void SG_filetool__replace_tokens(
	SG_context*     pCtx,    //< [in] [out] Error and context info.
	const SG_vhash* pValues, //< [in] Mapping of token names to replacement values.
	                         //<      May be NULL if there are no token values to substitute.
	const char*     szInput, //< [in] The string to replace tokens in.
	SG_string**     ppOutput //< [out] A copy of szInput with all tokens replaced by their corresponding values.
	);

/**
 * Function to simply fetch the path where SourceGear Diffmerge exists on the machine.
 */
void SG_filetool__get_builtin_diffmerge_exe(
	SG_context * pCtx,          //< [in] [out] Error and context info.
	SG_pathname ** ppPathCmd    //< [out] Path to diffmerge executable. Caller owns it.
	);

END_EXTERN_C;

#endif
