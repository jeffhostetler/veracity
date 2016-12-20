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
 * This module defines a "file_spec" object and associated functionality.
 *
 * A file_spec is basically a collection of include, exclude, and ignore patterns, and some flags.
 * A path can be matched against a filespec to see if it should be included in a given operation.
 * Exclude and ignore patterns behave identically.
 * They are separate because the use of ignores is often overridden by operations using the NO_IGNORES flag.
 * Includes behave more like "ExcludeEverythingExcept".
 * In other words: if a filespec has any include patterns, then paths only match the filespec if they match at least one of them.
 *
 * In the future, it is expected that a list of item patterns will be added to filespecs.
 * When this occurs, associated functionality to iterate/scan through matching files will also be added.
 * For now, callers are expected to iterate over potential files and run the matches themselves.
 */

#ifndef H_SG_FILE_SPEC_PROTOTYPES_H
#define H_SG_FILE_SPEC_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * This pointer can be used as an empty/blank filespec.
 * In other words: this filespec has no patterns added and no flags set.
 * Its purpose is to free callers from needing to allocate and free their own empty filespec.
 * Because it turns out to be rather common that a caller wants to provide an empty filespec.
 */
extern const SG_file_spec* gpBlankFilespec;

/**
 * Basic filespec memory management.
 */
void SG_file_spec__alloc(
	SG_context*    pCtx, //< [in] [out] Error and context info.
	SG_file_spec** pThis //< [out] The newly allocated filespec.
	);
void SG_file_spec__alloc__copy(
	SG_context*         pCtx,  //< [in] [out] Error and context info.
	const SG_file_spec* pThat, //< [in] The filespec to copy.
	SG_file_spec**      pThis  //< [out] The newly allocated filespec.
	);
void SG_file_spec__free(
	SG_context*         pCtx, //< [in] [out] Error and context info.
	const SG_file_spec* pThis //< [in] The filespec to free.
	);

/**
 * Allocation macros that track memory allocation file/line in debug builds.
 */
#if defined(DEBUG)
#define __SG_FILE_SPEC__ALLOC__(ppOutput, pVarName, AllocExpression)            \
	SG_STATEMENT(                                                               \
		SG_file_spec* pVarName = NULL;                                          \
		AllocExpression;                                                        \
		_sg_mem__set_caller_data(pVarName, __FILE__, __LINE__, "SG_file_spec"); \
		*(ppOutput) = pVarName;                                                 \
	)
#define SG_FILE_SPEC__ALLOC(pCtx, ppNew)              __SG_FILE_SPEC__ALLOC__(ppNew, pAlloc, SG_file_spec__alloc(pCtx, &pAlloc))
#define SG_FILE_SPEC__ALLOC__COPY(pCtx, pBase, ppNew) __SG_FILE_SPEC__ALLOC__(ppNew, pAlloc, SG_file_spec__alloc__copy(pCtx, pBase, &pAlloc))
#else
#define SG_FILE_SPEC__ALLOC(pCtx, ppNew)              SG_file_spec__alloc(pCtx, ppNew)
#define SG_FILE_SPEC__ALLOC__COPY(pCtx, pBase, ppNew) SG_file_spec__alloc__copy(pCtx, pBase, ppNew)
#endif

/**
 * Resets all data in a filespec back to its default state.
 */
void SG_file_spec__reset(
	SG_context*   pCtx, //< [in] [out] Error and context info.
	SG_file_spec* pThis //< [in] The filespec to reset.
	);

/**
 * Add a pattern to a filespec.
 */
void SG_file_spec__add_pattern__sz(
	SG_context*                pCtx,      //< [in] [out] Error and context info.
	SG_file_spec*              pThis,     //< [in] The filespec to add a pattern to.
	SG_file_spec__pattern_type eType,     //< [in] The type of pattern to add.
	const char*                szPattern, //< [in] The pattern to add to the filespec.
	SG_uint32                  uFlags     //< [in] Flags to associate with the pattern.
	);
void SG_file_spec__add_pattern__buflen(
	SG_context*                pCtx,      //< [in] [out] Error and context info.
	SG_file_spec*              pThis,     //< [in] The filespec to add a pattern to.
	SG_file_spec__pattern_type eType,     //< [in] The type of pattern to add.
	const char*                szPattern, //< [in] The pattern to add to the filespec.
	SG_uint32                  uLength,   //< [in] Length of the pattern.
	SG_uint32                  uFlags     //< [in] Flags to associate with the pattern.
	);
void SG_file_spec__add_pattern__string(
	SG_context*                pCtx,     //< [in] [out] Error and context info.
	SG_file_spec*              pThis,    //< [in] The filespec to add a pattern to.
	SG_file_spec__pattern_type eType,    //< [in] The type of pattern to add.
	SG_string*                 pPattern, //< [in] The pattern to add to the filespec.
	SG_uint32                  uFlags    //< [in] Flags to associate with the pattern.
	);

/**
 * Add several patterns to a filespec.
 */
void SG_file_spec__add_patterns__array(
	SG_context*                pCtx,       //< [in] [out] Error and context info.
	SG_file_spec*              pThis,      //< [in] The filespec to add patterns to.
	SG_file_spec__pattern_type eType,      //< [in] The type of pattern to add.
	const char* const*         ppPatterns, //< [in] The array of patterns to add.
	SG_uint32                  uCount,     //< [in] The number of patterns in the array.
	SG_uint32                  uFlags      //< [in] Flags to associate with the patterns.
	);
void SG_file_spec__add_patterns__stringarray(
	SG_context*                pCtx,      //< [in] [out] Error and context info.
	SG_file_spec*              pThis,     //< [in] The filespec to add patterns to.
	SG_file_spec__pattern_type eType,     //< [in] The type of pattern to add.
	const SG_stringarray*      pPatterns, //< [in] The stringarray of patterns to add.
	SG_uint32                  uFlags     //< [in] Flags to associate with the patterns.
	);

void SG_file_spec__add_patterns__varray(
	SG_context * pCtx,
	SG_file_spec * pThis,
	SG_file_spec__pattern_type eType,
	const SG_varray * pva,
	SG_uint32 uFlags
	);

void SG_file_spec__add_patterns__file(
	SG_context*                pCtx,       //< [in] [out] Error and context info.
	SG_file_spec*              pThis,      //< [in] The filespec to add patterns to.
	SG_file_spec__pattern_type eType,      //< [in] The type of pattern to add.
	const SG_pathname *        pPathname,  //< [in] The filename to read the patterns from.  Must be a text file with one pattern per line.
	SG_uint32                  uFlags      //< [in] Flags to associate with the patterns.
	);
void SG_file_spec__add_patterns__file__sz(
	SG_context * pCtx,
	SG_file_spec * pThis,
	SG_file_spec__pattern_type eType,
	const char * pszFilename,
	SG_uint32 uFlags);

/**
 * Loads ignores from a repo into a filespec.
 */
void SG_file_spec__load_ignores(
	SG_context*    pCtx,  //< [in] [out] Error and context info.
	SG_file_spec*  pThis, //< [in] The filespec to read the ignores into.
	SG_repo*       pRepo, //< [in] The repo to load the ignores from.
	const SG_pathname * pPathWorkingDirTop, //< [in] optional.
	SG_uint32      uFlags //< [in] Flags to associate with the ignores.
	);

/**
 * Clears all of the patterns of a single type from the filespec.
 */
void SG_file_spec__clear_patterns(
	SG_context*   pCtx,  //< [in] [out] Error and context info.
	SG_file_spec* pThis, //< [in] The filespec to clear patterns from.
	SG_uint32     uTypes //< [in] SG_file_spec__match_pattern(s) to clear.
	);

/**
 * Checks if a filespec contains any patterns of a given type or types.
 */
void SG_file_spec__has_pattern(
	SG_context*         pCtx,       //< [in] [out] Error and context info.
	const SG_file_spec* pThis,      //< [in] The filespec to check.
	SG_uint32           uTypes,     //< [in] SG_file_spec__match_pattern(s) to check for.
	SG_bool*            pHasPattern //< [out] Set to SG_TRUE if the filespec contains at least one pattern of any given type, or SG_FALSE otherwise.
	);

/**
 * Get/set flags on a filespec.
 */
void SG_file_spec__get_flags(
	SG_context*         pCtx,  //< [in] [out] Error and context info.
	const SG_file_spec* pThis, //< [in] The filespec to get flags from.
	SG_uint32*          pFlags //< [out] Filled with the filespec's flags value.
	);
void SG_file_spec__set_flags(
	SG_context*   pCtx,     //< [in] [out] Error and context info.
	SG_file_spec* pThis,    //< [in] The filespec to set flags on.
	SG_uint32     uFlags,   //< [in] Flags to set to the filespec.
	SG_uint32*    pOldFlags //< [out] Filled with the old flags value (optional).
	);
void SG_file_spec__modify_flags(
	SG_context*   pCtx,         //< [in] [out] Error and context info.
	SG_file_spec* pThis,        //< [in] The filespec to modify flags on.
	SG_uint32     uAddFlags,    //< [in] Flags to add to the filespec.
	SG_uint32     uRemoveFlags, //< [in] Flags to remove from the filespec.
	SG_uint32*    pOldFlags,    //< [out] Filled with the old flags value (optional).
	SG_uint32*    pNewFlags     //< [out] Filled with the new flags value (optional).
	);

/**
 * Check if a path matches a filespec.
 */
void SG_file_spec__match_path(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	const SG_file_spec* pThis,   //< [in] The filespec to match against.
	const char*         szPath,  //< [in] The path to match.
	SG_uint32           uFlags,  //< [in] Flags for optional matching behavior.
	SG_bool*            pMatches //< [out] Set to SG_TRUE if the path matches, or SG_FALSE if it doesn't.
	);

/**
 * Check if a path matches an individual pattern.
 */
void SG_file_spec__match_pattern(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	const char* szPattern, //< [in] The pattern to match a filename against.
	const char* szPath,    //< [in] The path to match against the pattern.
	SG_uint32   uFlags,    //< [in] Flags for optional matching behavior.
	SG_bool*    pMatches   //< [out] Set to SG_TRUE if the path matches, or SG_FALSE if it doesn't.
	);

/**
 * Check if a path matches a filespec.
 *
 * This function is only for backward compatibility with the current implementation of pendingtree,
 * and should become unnecessary once scanning functionality is moved into this module.
 * New code that's not part of pendingtree should not use this function.
 *
 * Note that this function uses a slightly different algorithm than match_path.
 * See the result enumeration values for more information.
 */
void SG_file_spec__should_include(
	SG_context*                 pCtx,   //< [in] [out] Error and context info.
	const SG_file_spec*         pThis,  //< [in] The filespec to match against.
	const char*                 szPath, //< [in] The path to match.
	SG_uint32                   uFlags, //< [in] Flags for optional matching behavior.
	SG_file_spec__match_result* pResult //< [out] Set to the result of the match.
	);

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_file_spec_debug__dump_to_console(SG_context * pCtx,
										 const SG_file_spec * pFilespec,
										 const char * pszLabel);
#endif

//////////////////////////////////////////////////////////////////

void SG_file_spec__count_patterns(SG_context * pCtx,
								  const SG_file_spec * pFilespec,
								  SG_file_spec__pattern_type eType,
								  SG_uint32 * pCount);

void SG_file_spec__get_nth_pattern(SG_context * pCtx,
								   const SG_file_spec * pFilespec,
								   SG_file_spec__pattern_type eType,
								   SG_uint32 uIndex,
								   const char ** ppszPattern,
								   SG_uint32 * puFlags);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
