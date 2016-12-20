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
 * See the prototypes header for additional information.
 */

#ifndef H_SG_FILETOOL_TYPEDEFS_H
#define H_SG_FILETOOL_TYPEDEFS_H

BEGIN_EXTERN_C;

/**
 * Definition of a filetool.
 */
typedef struct _SG_filetool SG_filetool;

/**
 * Prototype of a function that invokes/implements a file tool.
 */
typedef void (SG_filetool__invoker)(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const SG_filetool* pTool,        //< [in] The tool being invoked.
	const SG_vhash*    pTokenValues, //< [in] A list of name/value pairs to substitute into the tool's arguments.
	                                 //<      May be NULL if there are no values to substitute.
	SG_int32*          pResult       //< [out] The result of invoking the tool.
	                                 //<       SG_FILETOOL__RESULT__* or a type-specific result.
	                                 //<       This is NOT equal to the exit code of an external tool.
	                                 //<       External exit codes will have been translated to results.
	                                 //<       May be NULL if the exit code is unneeded.
	);

/**
 * Prototype of a function that looks up invokers for built-in file tools.
 */
typedef void (SG_filetool__plugin)(
	SG_context*            pCtx,      //< [in] [out] Error and context info.
	const char*            szType,    //< [in] The type of tool being looked up.
	const char*            szContext, //< [in] The context the tool is being looked up in.
	                                  //<      If NULL, then no specific context is being used.
	const char*            szName,    //< [in] The name of the tool being looked up.
	                                  //<      If NULL, a custom invoker for external tools is being requested.
	SG_filetool__invoker** ppInvoker  //< [out] The invoker that was found for the named tool.
	                                  //<       Set to NULL if no matching invoker was found.
	);

/**
 * List of built-in file classes.
 */
#define SG_FILETOOL__CLASS__BINARY SG_FILETOOLCONFIG__BINARY_CLASS
#define SG_FILETOOL__CLASS__TEXT   SG_FILETOOLCONFIG__TEXT_CLASS

/**
 * A tool flag for any external filetool.
 * Tools configured with this flag are invoked via system() rather than CreateProcess or fork/exec.
 * This is occasionally necessary for tools that are picky about their environment.
 * Using this flag causes the tool's configured stdin, stdout, and stderr bindings to be ignored.
 */
#define SG_FILETOOL__FLAG__SYSTEM_CALL "system-call" // invoke the tool via system() instead of CreateProcess/fork+exec

/*
 * List of built-in file tools.
 */
#define SG_FILETOOL__INTERNAL__NULL ":null" //< SG_filetool__invoke__null

/*
 * Standard exit codes.
 */
#define SG_FILETOOL__EXIT_CODE__ZERO    "0" //< Translates to "success"
#define SG_FILETOOL__EXIT_CODE__DEFAULT ""  //< Translates to "failure"

/**
 * Enumeration of standard result codes potentially used by any type of filetool.
 */
typedef enum SG_filetool__result
{
	// These result codes are valid on all types of filetool.
	SG_FILETOOL__RESULT__SUCCESS = 0,  //< Indicates that the tool completed successfully.
	                                   //< This should remain zero so that zero/non-zero success checks work.
	SG_FILETOOL__RESULT__FAILURE,      //< Indicates that the tool completed unsuccessfully.
	SG_FILETOOL__RESULT__ERROR,        //< Indicates that there was an error running the tool.
	SG_FILETOOL__RESULT__COUNT,        //< Number of valid values in this enum.

	// Result codes specific to a particular tool type should start here.
	// They will be defined by each individual type.
	SG_FILETOOL__RESULT__TYPE = 1 << 8 //< First type-specific result code.
}
SG_filetool__result;

/*
 * String representations of standard result codes.
 */
#define SG_FILETOOL__RESULT__SUCCESS__SZ "success" //< Translates to SG_FILETOOL__RESULT__SUCCESS
#define SG_FILETOOL__RESULT__FAILURE__SZ "failure" //< Translates to SG_FILETOOL__RESULT__FAILURE
#define SG_FILETOOL__RESULT__ERROR__SZ   "error"   //< String representation of SG_FILETOOL__RESULT__ERROR
                                                   //< Doesn't actually translate to a result code, because tools
                                                   //< can't return this, it indicates that the tool didn't even run.
#define SG_FILETOOL__RESULT__COUNT__SZ   "invalid" //< String representation of invalid result codes.

END_EXTERN_C;

#endif
