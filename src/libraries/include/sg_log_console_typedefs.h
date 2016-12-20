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
 * @file    SG_log_console_typedefs.h
 * @details Typedefs for the SG_log_console module.
 *          See the prototypes header for more information.
 */

#ifndef H_SG_LOG_CONSOLE_TYPEDEFS_H
#define H_SG_LOG_CONSOLE_TYPEDEFS_H

BEGIN_EXTERN_C;

/**
 * Handler that writes progress messages to SG_console.
 *
 * TODO: Check if this handler works while sglib is not initialized.
 *       Potential issues include: SG_file, SG_string, SG_console
 */
extern const SG_log__handler* gpConsoleLogHandler;

/**
 * Data used by gpFileLogHandler.
 * Pass an instance as the pHandlerData when registering gpFileLogHandler.
 * Fill out the configuration section(s) as appropriate first.
 * The user/caller still owns any pointers that they set.
 * All pointers set by the user/caller must remain valid as long as the handler is registered.
 * Use SG_log_text__set_defaults to setup default values.
 */
typedef struct _sg_log_console__data
{
	// options configuration (optional, set to defaults using SG_log_console__set_defaults)
	SG_bool bLogVerboseOperations; //< If false, then verbose operations will not be logged (defaults to true).

	// output configuration (optional, set to defaults using SG_log_console__set_defaults)
	// If any of these are NULL, then the corresponding data will not be logged at all.
	const char* szStepFormat;              //< Format for step descriptions (passed a single string).
	const char* szCompletionFormat;        //< Format for step completion 
	                                       //  (args: string step description, int finished steps, 
	                                       //  int total steps, string unit description, int percentage).

	const char* szOpStackSeparator;        // String separating operations on the stack when printed to the console. No replacement parameters.

	SG_bool bSilenced;
}
SG_log_console__data;

END_EXTERN_C;

#endif
