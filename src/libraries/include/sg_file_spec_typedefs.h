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
 * A file_spec describes a set of files using a combination of glob-like patterns.
 */

#ifndef H_SG_FILE_SPEC_TYPEDEFS_H
#define H_SG_FILE_SPEC_TYPEDEFS_H

BEGIN_EXTERN_C;

/**
 * Definition of a filespec.
 */
typedef struct _SG_file_spec SG_file_spec;

/**
 * Flags that can be used to affect various filespec functions.
 */
typedef enum
{
	// Flags that specify how to match a path against a pattern.
	SG_FILE_SPEC__MATCH_NULL_PATTERN        = 1 << 0, //< Consider a NULL pattern to always match.
	SG_FILE_SPEC__MATCH_NULL_FILENAME       = 1 << 1, //< Consider a NULL filename to always match.
	SG_FILE_SPEC__MATCH_ANYWHERE            = 1 << 2, //< Consider pattern(s) to have an implicit "**/" prefix.
	SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY = 1 << 3, //< Consider pattern(s) to have an implicit "/**" suffix.
	SG_FILE_SPEC__MATCH_REPO_ROOT           = 1 << 4, //< Consider pattern(s) to have an implicit "@/" prefix.
	SG_FILE_SPEC__MATCH_TRAILING_SLASH      = 1 << 5, //< Consider pattern(s) to have an implicit and optional "/" suffix.

	// Handy combinations of above flags.
	SG_FILE_SPEC__MATCH_NULL      = SG_FILE_SPEC__MATCH_NULL_PATTERN |
	                                SG_FILE_SPEC__MATCH_NULL_FILENAME,
	SG_FILE_SPEC__MATCH_REPO_PATH = SG_FILE_SPEC__MATCH_REPO_ROOT |
	                                SG_FILE_SPEC__MATCH_TRAILING_SLASH,

	// Flags that specify how to match a path against a filespec.
	SG_FILE_SPEC__NO_INCLUDES = 1 << 6, //< Don't consider the filespec's include patterns.
	SG_FILE_SPEC__NO_EXCLUDES = 1 << 7, //< Don't consider the filespec's exclude patterns.
	SG_FILE_SPEC__NO_IGNORES  = 1 << 8, //< Don't consider the filespec's ignore patterns.
	SG_FILE_SPEC__NO_RESERVED = 1 << 9, //< Don't consider the filespec's reserved patterns.

	// Flags for memory management of added patterns.
	// Default behavior is for the filespec to make its own private copy of each pattern.
	SG_FILE_SPEC__SHALLOW_COPY = 1 << 10, //< File_spec should not make a private copy of the pattern, just copy the pointer.
	SG_FILE_SPEC__OWN_MEMORY   = 1 << 11, //< When used with SHALLOW_COPY, filespec will also assume ownership of the pointer's memory.

	//////////////////////////////////This flag is now commented out      ////////////////
	//////////////////////////////////It was causing errors with ignore patterns which started with @/ ///////////////
	////////////////////////////////// See J9736
	// Backwards-compatibility flags.
	// These flags are only for backward compatibility with the current implementation of pendingtree,
	// and should become unnecessary once scanning functionality is moved into this module.
	// New code that's not part of pendingtree should not use these flags.
	//SG_FILE_SPEC__CONVERT_REPO_PATH = 1 << 12, //< Convert the incoming repo path into a CWD-relative local path before matching.
	                                           //< Only the should_include function respects this flag.
	                                           //< This flag cannot be used unless set_pendingtree has previously been called with a (still) valid pendingtree.
}
SG_file_spec__flags;

/**
 * An enumeration of the various types of patterns used in filespecs.
 */
typedef enum
{
	SG_FILE_SPEC__PATTERN__INCLUDE = 1 << 0, //< Matching filenames will match at least one of these patterns.
	SG_FILE_SPEC__PATTERN__EXCLUDE = 1 << 1, //< Matching filenames will not match any of these patterns.
	SG_FILE_SPEC__PATTERN__IGNORE  = 1 << 2, //< Matching filenames will not match any of these patterns.
	SG_FILE_SPEC__PATTERN__RESERVE = 1 << 3, //< Matching filenames will not match any of these patterns.
	SG_FILE_SPEC__PATTERN__LAST    = 1 << 4, //< Last value in this enumeration.  Used to terminate loops over the values.

	SG_FILE_SPEC__PATTERN__ALL     = 0xF     //< All pattern types.
}
SG_file_spec__pattern_type;

/**
 * An enumeration of possible results from matching a path against a filespec.
 * TODO: These are only bit-flags because pendingtree currently wants to use them as such.
 *       Once pendingtree is refactored, it should be safe (and preferable) to make them sequential.
 */
typedef enum
{
	SG_FILE_SPEC__RESULT__MAYBE    = 0x00, //< MAYBE: The path didn't match any patterns, but there are include patterns.
	                                       //<        If the path was a file, caller should consider this a FALSE result.
	                                       //<        If the path was a directory, caller should consider this a TRUE result if any child paths match, or FALSE if none do.
	SG_FILE_SPEC__RESULT__RESERVED = 0x04, //< FALSE: The path matched a RESERVED pattern.
	SG_FILE_SPEC__RESULT__EXCLUDED = 0x01, //< FALSE: The path matched an EXCLUDE pattern.
	SG_FILE_SPEC__RESULT__IGNORED  = 0x02, //< FALSE: The path matched an IGNORE pattern, but no INCLUDE or EXCLUDE patterns.
	SG_FILE_SPEC__RESULT__INCLUDED = 0x10, //< TRUE: The path matched an INCLUDE pattern, but no EXCLUDE patterns.
	SG_FILE_SPEC__RESULT__IMPLIED  = 0x20, //< TRUE: The path didn't match anything and there are no includes, so the path matches implicitly.
}
SG_file_spec__match_result;

END_EXTERN_C;

#endif
