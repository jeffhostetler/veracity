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
 * @file    SG_log_text_typedefs.h
 * @details Typedefs for the SG_log_text module.
 *          See the prototypes header for more information.
 */

#ifndef H_SG_LOG_TEXT_TYPEDEFS_H
#define H_SG_LOG_TEXT_TYPEDEFS_H

BEGIN_EXTERN_C;

/**
 * Handler that logs progress to a FILE*.
 *
 * TODO: Check if this handler works while sglib is not initialized.
 *       Potential issues include: SG_file, SG_string, SG_console
 */
extern const SG_log__handler* gpFileLogHandler;

/**
 * Type of a function that writes text to its destination.
 * One of these must be provided in each handler's config.
 * Several built-in writers are available in the prototypes header.
 */
typedef void SG_log_text__writer(
	SG_context*      pCtx,        //< [in] [out] Error and context information.
	void*            pWriterData, //< [in] Data specific to the writer.
	const SG_string* pText,       //< [in] The text to write.
	                              //<      If NULL, then the writer is being initialized or destroyed (check bError to determine which).
	SG_bool          bError       //< [in] Whether or not the text should be considered an error.
	                              //<      This is for things like choosing STDERR vs STDOUT.
	                              //<      If szText is NULL, then this will be true for initialization or false for destruction.
	);

/**
 * A macro for implementing an SG_log_text__writer function in terms of
 * separate init, destroy, and write functions.
 *
 * Name:            Name of the writer function to implement.
 * DataType:        Type to cast pWriterData to before passing to other functions.
 * InitFunction:    void Function(SG_context*, DataType*)
 *                  Called when pText is NULL and bError is SG_TRUE.
 * DestroyFunction: void Function(SG_context*, DataType*)
 *                  Called when pText is NULL and bError is SG_FALSE.
 * WriteFunction:   void Function(SG_context*, DataType*, const SG_string*, SG_bool)
 *                  Called when pText is non-NULL.
 */
#define SG_LOG_TEXT__IMPLEMENT_WRITER(Name, DataType, InitFunction, DestroyFunction, WriteFunction) \
void Name(                                                                                          \
	SG_context*      pCtx,                                                                          \
	void*            pData,                                                                         \
	const SG_string* pText,                                                                         \
	SG_bool          bError                                                                         \
)                                                                                                   \
{                                                                                                   \
	DataType* pThis = NULL;                                                                         \
	SG_NULLARGCHECK_RETURN(pData);                                                                  \
	pThis = (DataType*)pData;                                                                       \
	if (pText == NULL)                                                                              \
	{                                                                                               \
		if (bError == SG_TRUE)                                                                      \
		{                                                                                           \
			SG_ERR_CHECK_RETURN(  InitFunction(pCtx, pThis)  );                                     \
		}                                                                                           \
		else                                                                                        \
		{                                                                                           \
			SG_ERR_CHECK_RETURN(  DestroyFunction(pCtx, pThis)  );                                  \
		}                                                                                           \
	}                                                                                               \
	else                                                                                            \
	{                                                                                               \
		SG_ERR_CHECK_RETURN(  WriteFunction(pCtx, pThis, pText, bError)  );                         \
	}                                                                                               \
}


/**
 * Data used by gpFileLogHandler.
 * Pass an instance as the pHandlerData when registering gpFileLogHandler.
 * Fill out the configuration section(s) as appropriate first.
 * The user/caller still owns any pointers that they set.
 * All pointers set by the user/caller must remain valid as long as the handler is registered.
 * Use SG_log_text__set_defaults to setup default values.
 */
typedef struct _sg_log_text__data
{
	// target configuration (required)
	SG_log_text__writer* fWriter;     //< The function to use to write text.
	                                  //< Several built-in writers are available below.
	                                  //< It is invalid for this to be NULL when the handler is registered.
	                                  //< Defaults to SG_log_text__writer__std.
	void*                pWriterData; //< Data to pass to the writer for its use.
	                                  //< Type/contents depends on the writer being used.
	                                  //< Defaults to NULL, which is only valid for some writers.

	// options configuration (optional, set to defaults using SG_log_text__set_defaults)
	SG_bool bLogVerboseOperations; //< If false, then verbose operations will not be logged (defaults to true).
	SG_bool bLogVerboseValues;     //< If false, then verbose values will not be logged (defaults to true).

	// output configuration (optional, set to defaults using SG_log_text__set_defaults)
	// If any of these are NULL, then the corresponding data will not be logged at all.
	const char* szRegisterMessage;     //< String to log when the handler is registered.
	const char* szUnregisterMessage;   //< String to log when the handler is unregistered.
	const char* szDateTimeFormat;      //< Format for date/time information (passed a single string: formatted local time).
	const char* szProcessThreadFormat; //< Format for process/thread IDs (passed two strings: process ID, thread ID).
	const char* szIndent;              //< String to use as a single indent.
	const char* szElapsedTimeFormat;   //< Format for elapsed time (passed a single int: milliseconds).
	const char* szCompletionFormat;    //< Format for step completion (passed three ints: finished steps, total steps, percentage).
	const char* szOperationFormat;     //< Format for operation descriptions (passed a single string).
	const char* szValueFormat;         //< Format for operation values (passed two strings: name, value).
	const char* szStepFormat;          //< Format for step descriptions (passed a single string).
	const char* szStepMessage;         //< Message displayed when steps are finished.
	const char* szResultFormat;        //< Format for operation results (passed a single SG_error).
	const char* szVerboseFormat;       //< Format for verbose messages (passed a single string).
	const char* szInfoFormat;          //< Format for info messages (passed a single string).
	const char* szWarningFormat;       //< Format for warning messages (passed a single string).
	const char* szErrorFormat;         //< Format for error messages (passed a single string).
}
SG_log_text__data;

/**
 * Static initializer for SG_log_text__data.
 */
#define SG_LOG_TEXT__DATA__INITIALIZER {NULL,NULL,SG_FALSE,SG_FALSE,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}

/**
 * Data used by the SG_log_text__writer__path writer.
 * Initialize with SG_log_text__writer__path__set_defaults.
 */
typedef struct _sg_log_text__writer__path__data
{
	// configuration
	SG_pathname*   pPath;        //< The path to write to.  Defaults to NULL.
	                             //< Must be specified by the user, NULL is an invalid value.
	SG_bool        bReopen;      //< If true, the file will be opened and closed for each write.
	                             //< If false, the file will be held open as long as the handler is registered.
	                             //< Defaults to false.
	const char*    szHeader;     //< String to write at the start of the file, or NULL to not write anything.
	                             //< This will never be written to a file that already contains data.
	                             //< Defaults to NULL.
	SG_fsobj_perms ePermissions; //< Permissions to assign to the log file, if the writer creates it.
	                             //< Defaults to 0644.

	// state
	SG_file*     pFile;  //< Handle to the file.
	                     //< This is always NULL until the first write.
	                     //< If bReopen is true, then this is also NULL while not actively being written to.
	                     //< Internal use only.
	SG_mutex     cMutex; //< Controls access to our write function.
	                     //< Internal use only.
}
SG_log_text__writer__path__data;

/**
 * Static initializer for SG_log_text__writer__path__data.
 */
// not sure what to initialize an SG_mutex to.. hopefully nobody will need this
//#define SG_LOG_TEXT__WRITER__PATH__DATA__INITIALIZER {NULL,SG_FALSE,NULL,NULL,?}

/**
 * Data used by the SG_log_text__writer__daily_path writer.
 * Initialize with SG_log_text__writer__daily_path__set_defaults.
 */
typedef struct _sg_log_text__writer__daily_path__data
{
	// configuration
	SG_pathname*   pBasePath;        //< Path of the folder to store output files in.  Defaults to NULL.
	                                 //< Must be specified by the user, NULL is an invalid value.
	const char*    szFilenameFormat; //< Format string for filenames (passed three ints: year, month, day).  Defaults to NULL.
	                                 //< Must be specified by the user, NULL is an invalid value.
	SG_bool        bReopen;          //< If true, the current file will be opened and closed for each write.
	                                 //< If false, the current file will be held open as long as the handler is registered.
	                                 //< Defaults to false.
	const char*    szHeader;         //< String to write at the start of the file, or NULL to not write anything.
	                                 //< This will never be written to a file that already contains data.
	                                 //< Defaults to NULL.
	SG_fsobj_perms ePermissions;     //< Permissions to assign to log files that the writer creates.
	                                 //< Defaults to 0644.

	// state
	SG_uint32    uCurrentYday; //< SG_time.yday that pCurrentPath was constructed from.
	                           //< Internal use only.
	SG_pathname* pCurrentPath; //< Path of the current file.
	                           //< Internal use only.
	SG_file*     pCurrentFile; //< Handle to the current file.
	                           //< This is always NULL until the first write.
	                           //< If bReopen is true, then this is also NULL while not actively being written to.
	                           //< Internal use only.
	SG_mutex     cMutex;       //< Controls access to our write function.
	                           //< Internal use only.
	SG_string*   pDefaultHeader;//< Default Header. szHeader references this unless overridden.
}
SG_log_text__writer__daily_path__data;

/**
 * Static initializer for SG_log_text__writer__daily_path__data.
 */
// not sure what to initialize an SG_mutex to.. hopefully nobody will need this
//#define SG_LOG_TEXT__WRITER__DAILY_PATH__DATA__INITIALIZER {NULL,NULL,SG_FALSE,NULL,0u,NULL,NULL,?}

END_EXTERN_C;

#endif
