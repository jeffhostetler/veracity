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
 * This module layers on top of "filetool" and adds functionality for calling merge tools.
 * See the prototypes header for additional information.
 */

#ifndef H_SG_MERGETOOL_TYPEDEFS_H
#define H_SG_MERGETOOL_TYPEDEFS_H

BEGIN_EXTERN_C;

// the filetool "type" for all merge tools
#define SG_MERGETOOL__TYPE "merge"

// merge tool contexts
#define SG_MERGETOOL__CONTEXT__MERGE   "merge"   // invoked during a "merge" command / API call
#define SG_MERGETOOL__CONTEXT__RESOLVE "resolve" // invoked during a "resolve" command / API call

// merge tool flags
#define SG_MERGETOOL__FLAG__PREMAKE_RESULT     "premake-result"     // make an empty result file before invoking the tool
#define SG_MERGETOOL__FLAG__ANCESTOR_OVERWRITE "ancestor-overwrite" // tool overwrites the ancestor file instead of making a new result file

/**
 * Possible result codes from invoking a mergetool,
 * in addition to SG_FILETOOL__RESULT__*.
 */
typedef enum SG_mergetool__result
{
	SG_MERGETOOL__RESULT__CONFLICT = SG_FILETOOL__RESULT__TYPE, //< Indicates that conflicts still exist in the result.
	SG_MERGETOOL__RESULT__CANCEL,                               //< Indicates that the merge was canceled.
}
SG_mergetool__result;

/*
 * String representations of mergetool result codes.
 */
#define SG_MERGETOOL__RESULT__CONFLICT__SZ "conflict" //< Translates to SG_MERGETOOL__RESULT__CONFLICT
#define SG_MERGETOOL__RESULT__CANCEL__SZ   "cancel"   //< Translates to SG_MERGETOOL__RESULT__CANCEL

// The names of internal merge tools are defined below.
// The ':' prefix is the current convention for internal tools,
// though nothing is preventing users from using that prefix in localsettings.
#define SG_MERGETOOL__INTERNAL__SKIP         ":skip"              // don't actually run a merge, just return SG_MERGETOOL__RESULT__CANCEL
#define SG_MERGETOOL__INTERNAL__MERGE_STRICT ":merge_strict"      // use compiled-in C code in sg_diffcore with SG_TEXTFILEDIFF_OPTION__STRICT_EOL
#define SG_MERGETOOL__INTERNAL__MERGE        ":merge"             // use complied-in C code in sg_diffcore with SG_TEXTFILEDIFF_OPTION__REGULAR
#define SG_MERGETOOL__INTERNAL__BASELINE     ":baseline"          // pretend to merge, but just take my version
#define SG_MERGETOOL__INTERNAL__OTHER        ":other"             // pretend to merge, but just take the other version
#define SG_MERGETOOL__INTERNAL__DIFFMERGE    ":diffmerge"         // use compiled-in C code that knows how to exec diffmerge.

END_EXTERN_C;

#endif
