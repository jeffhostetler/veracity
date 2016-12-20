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
 * This module layers on top of "filetool" and adds functionality for calling diff tools.
 * See the prototypes header for additional information.
 * 
 * Created by PaulE, copied off of sg_mergetool* by Andy.
 */

#ifndef H_SG_DIFFTOOL_TYPEDEFS_H
#define H_SG_DIFFTOOL_TYPEDEFS_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


// the filetool "type" for all diff tools
#define SG_DIFFTOOL__TYPE "diff"

// diff tool contexts
#define SG_DIFFTOOL__CONTEXT__CONSOLE "console" //< invoked by command-line clients
#define SG_DIFFTOOL__CONTEXT__GUI     "gui"     //< invoked by GUI clients

//////////////////////////////////////////////////////////////////

/**
 * Possible result codes from invoking a difftool,
 * in addition to SG_FILETOOL__RESULT__*.
 */
typedef enum SG_difftool__result
{
	SG_DIFFTOOL__RESULT__SAME = SG_FILETOOL__RESULT__TYPE, //< Tool said they were the same.
	SG_DIFFTOOL__RESULT__DIFFERENT,                        //< Tool said they were different.
	SG_DIFFTOOL__RESULT__CANCEL,                           //< Tool canceled diff (such as for an unsupported (binary) file type)
}
SG_difftool__result;

/*
 * String representations of difftool result codes.
 */
#define SG_DIFFTOOL__RESULT__SAME__SZ       "same"        //< Translates to SG_DIFFTOOL__RESULT__SAME
#define SG_DIFFTOOL__RESULT__DIFFERENT__SZ  "different"   //< Translates to SG_DIFFTOOL__RESULT__DIFFERENT
#define SG_DIFFTOOL__RESULT__CANCEL__SZ     "cancel"      //< Translates to SG_DIFFTOOL__RESULT__CANCEL

//////////////////////////////////////////////////////////////////

typedef void (SG_difftool__built_in_tool)(
	SG_context * pCtx,

	// Allow pPathname0 or pPathname1, but not both, to be NULL, meaning empty file
	const SG_pathname * pPathname0,
	const SG_pathname * pPathname1,

	const SG_string * pLabel0,
	const SG_string * pLabel1,

	SG_bool bRightSideIsWritable,

	SG_string ** ppOutput, // Optional output parameter. If NULL, output will go to stdout.
	SG_int32 * piResultCode // SG_FILETOOL__RESULT__* or SG_DIFFTOOL__RESULT__*
	);


// The names of built-in diff tools are defined below.
// The ':' prefix is the current convention for internal tools,
// though nothing is preventing users from using that prefix in localsettings.

// don't actually run a diff, just return NOT_ATTEMPTED
#define SG_DIFFTOOL__INTERNAL__SKIP         ":skip"

// use complied-in C code in sg_diffcore
#define SG_DIFFTOOL__INTERNAL__DIFF_STRICT                     ":diff_strict"
#define SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_EOLS                ":diff"
#define SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_WHITESPACE          ":diff_ignore_whitespace"
#define SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_CASE                ":diff_ignore_case"
#define SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_CASE_AND_WHITESPACE ":diff_ignore_case_and_whitespace"

// built-in settings for DiffMerge.  this doesn't exactly match
// the usage of the other tools.  it will launch a GUI app rather
// that splatting the deltas to the console.
//
// WE DO NOT USE THE DiffMerge's BATCH HERE (partly because on Windows
// stdout is closed by the system before DiffMerge gets started).

#define SG_DIFFTOOL__INTERNAL__DIFFMERGE ":diffmerge"

///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
