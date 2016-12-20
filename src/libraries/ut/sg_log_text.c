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
 * @file    SG_log_text.c
 * @details Implements the SG_log_text module.
 */

#include <sg.h>


/*
**
** Macros
**
*/

/**
 * Handy macro for unlocking a mutex in a fail label.
 * If the context is already in an error state, it ignores any error from unlocking the mutex.
 *
 * This legitimately uses SG_ERR_CHECK_RETURN, despite being in a fail block, 
 * because we're in common cleanup code and it's already verified that 
 * we're not in an error state.
 */
#define _ERR_UNLOCK(pCtx, pMutex)                                \
	if (SG_CONTEXT__HAS_ERR(pCtx))                               \
	{                                                            \
		SG_mutex__unlock__bare(pMutex);                          \
	}                                                            \
	else                                                         \
	{                                                            \
		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, pMutex)  ); \
	}


/*
**
** Types
**
*/

// forward declarations of our handler implementation functions
static SG_log__handler__init      _init_handler;
static SG_log__handler__operation _operation_handler;
static SG_log__handler__message   _message_handler;
static SG_log__handler__destroy   _destroy_handler;


/*
**
** Globals
**
*/

/**
 * Global instance of our handler.
 */
static const SG_log__handler gcFileLogHandler =
{
	_init_handler,
	_operation_handler,
	_message_handler,
	NULL,
	_destroy_handler,
};
const SG_log__handler* gpFileLogHandler = &gcFileLogHandler;

// default values for an SG_log_text__data structure
static const char* gszDefaultRegisterMessage     = "Log Started";
static const char* gszDefaultUnregisterMessage   = "Log Ended";
//static const char* gszDefaultDateTimeFormat      = "[%Y/%m/%d %H:%M:%S] ";
static const char* gszDefaultDateTimeFormat      = "[%s] ";
static const char* gszDefaultProcessThreadFormat = "[PID: %6s, Thread: %6s] ";
static const char* gszDefaultIndent              = "\t";
//static const char* gszDefaultElapsedTimeFormat   = "[%H:%M:%S.%l] ";
static const char* gszDefaultElapsedTimeFormat   = "[%8dms] ";
//static const char* gszDefaultCompletionFormat    = "[%3$3d%% (%1$3d/%2$3d)] ";
static const char* gszDefaultCompletionFormat    = "[(%3d/%3d) %3d%%] ";
static const char* gszDefaultOperationFormat     = "Operation: %s";
static const char* gszDefaultValueFormat         = "Value: %s=%s";
static const char* gszDefaultStepFormat          = "Step: %s";
static const char* gszDefaultStepMessage         = "Step(s) Finished";
static const char* gszDefaultResultFormat        = "Result: %d";
static const char* gszDefaultVerboseFormat       = "Verbose: %s";
static const char* gszDefaultInfoFormat          = "Info: %s";
static const char* gszDefaultWarningFormat       = "Warning: %s";
static const char* gszDefaultErrorFormat         = "Error: %s";


/*
**
** Internal Functions
**
*/

/**
 * Helper function for opening a file to write logged text to.
 * Automatically seeks to the end of the file so that text will be appended.
 * Optionally writes a header if the opened file is empty (or didn't exist before being opened).
 */
static void _open_file(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const SG_pathname* pPath,        //< [in] Path to the file to open.
	const char*        szHeader,     //< [in] String to write to the file, if it's currently empty or doesn't exist.
	                                 //<      May be NULL, if nothing needs to be written.
	SG_fsobj_perms     ePermissions, //< [in] Permissions for the file, if it must be created.
	SG_file**          ppFile        //< [out] The opened file.
	)
{
	SG_pathname* pFolder = NULL;
	SG_file*     pFile = NULL;
	SG_uint64    uSize = 0u;

	// make sure that the parent folder of the log file exists
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pFolder, pPath)  );
	SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pFolder)  );
	SG_fsobj__mkdir_recursive__pathname(pCtx, pFolder);
	SG_ERR_CHECK_CURRENT_DISREGARD(  SG_ERR_DIR_ALREADY_EXISTS  );

	// open the log file and seek to the end
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY, ePermissions, &pFile)  );
	SG_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, &uSize)  );

	if (uSize == 0u)
	{
		// If this is a new file, write UTF8 BOM.
		SG_ERR_CHECK(  SG_file__write(pCtx, pFile, sizeof(UTF8_BOM), UTF8_BOM, NULL)  );

		// If a header was specified, write it
		if (szHeader != NULL)
		{
			SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, szHeader)  );
		}
	}

	// return the file
	*ppFile = pFile;
	pFile = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFolder);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

/**
 * Builds a path from a base path and a filename that is based on a date.
 */
static void _build_date_path(
	SG_context*        pCtx,             //< [in] [out] Error and context info.
	const SG_pathname* pBasePath,        //< [in] Path to the folder that will contain the file.
	const char*        szFilenameFormat, //< [in] Format of the filename, passed three ints: year, month, mday.
	const SG_time*     pTime,            //< [in] The time to use when building the filename.
	SG_pathname**      ppFullPath        //< [out] The resulting full path.
	)
{
	SG_string* pFilename = NULL;

	SG_NULLARGCHECK(ppFullPath);

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pFilename, szFilenameFormat, pTime->year, pTime->month, pTime->mday)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, ppFullPath, pBasePath, SG_string__sz(pFilename))  );

fail:
	SG_STRING_NULLFREE(pCtx, pFilename);
	return;
}

/**
 * Writes out a message string.
 * It is formatted to include various status information, as configured in the log handler data.
 */
static void _write_format(
	SG_context*              pCtx,       //< [in] [out] Error and context info.
	SG_log_text__data*       pData,      //< [in] Our log handler data.
	const SG_log__operation* pOperation, //< [in] The operation that the written text is about.
	const char*              szText,     //< [in] The message text to write.
	SG_bool                  bError      //< [in] Whether or not the message text is considered error text.
	)
{
	SG_string* pText    = NULL;
	SG_uint32  uCount   = 0u;
	SG_uint32  uIndex   = 0u;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );

	// append the current date/time, if requested
	if (pData->szDateTimeFormat != NULL)
	{
		char     szBuffer[SG_TIME_FORMAT_LENGTH + 1];
		SG_int64 iLatest = 0;

		SG_ERR_CHECK(  SG_log__stack__get_time(pCtx, &iLatest)  );
		SG_ERR_CHECK(  SG_time__format_local__i64(pCtx, iLatest, szBuffer, SG_TIME_FORMAT_LENGTH + 1u)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pText, pData->szDateTimeFormat, szBuffer)  );
	}

	// append the current process/thread, if requested
	if (pData->szProcessThreadFormat != NULL)
	{
		SG_process_id cProcess;
		SG_thread_id  cThread;
		SG_int_to_string_buffer    szProcessBuffer;
		SG_thread_to_string_buffer szThreadBuffer;

		SG_ERR_CHECK(  SG_log__stack__get_threading(pCtx, &cProcess, &cThread)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pText, pData->szProcessThreadFormat, SG_uint64_to_sz((SG_uint64)cProcess, szProcessBuffer), SG_thread__sz(pCtx, cThread, szThreadBuffer))  );
	}

	// append one indent for each operation currently in progress
	// start at 1, because no indent is needed for the first operation
	SG_ERR_CHECK(  SG_log__stack__get_count(pCtx, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pText, "%s", pData->szIndent)  );
	}

	// append the elapsed time, if requested
	if (pOperation != NULL && pData->szElapsedTimeFormat != NULL)
	{
		SG_int64 iElapsed = 0;

		SG_ERR_CHECK(  SG_log__operation__get_time(pCtx, pOperation, NULL, NULL, &iElapsed)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pText, pData->szElapsedTimeFormat, iElapsed)  );
	}

	// append the completion percentage, if requested
	if (pOperation != NULL && pData->szCompletionFormat != NULL)
	{
		SG_uint32 uFinished = 0u;
		SG_uint32 uTotal    = 0u;
		SG_uint32 uPercent  = 0u;

		SG_ERR_CHECK(  SG_log__operation__get_progress(pCtx, pOperation, NULL, &uFinished, &uTotal, &uPercent, 0u)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pText, pData->szCompletionFormat, uFinished, uTotal, uPercent)  );
	}

	// append the given text and an EOL
	SG_ERR_CHECK(  SG_string__append__format(pCtx, pText, "%s%s", szText, SG_PLATFORM_NATIVE_EOL_STR)  );

	// write the line
	SG_ERR_CHECK(  pData->fWriter(pCtx, pData->pWriterData, pText, bError)  );

fail:
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Internal worker for _variant_to_string.
 * Appends to an existing string rather than returning a fresh one.
 * Allows optimization in the case of arrays and hashes.
 */
static void _variant_to_string__append(
	SG_context*       pCtx,     //< [in] [out] Error and context info.
	const SG_variant* pVariant, //< [in] The variant to append to a loggable string.
	SG_string*        sString   //< [in] [out] String to append the variant's value to.
	)
{
	switch (pVariant->type)
	{
	case SG_VARIANT_TYPE_NULL:
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, "null")  );
			break;
		}
	case SG_VARIANT_TYPE_INT64:
		{
			SG_int_to_string_buffer aBuffer;
			SG_ERR_CHECK(  SG_string__append__format(pCtx, sString, "%s", SG_int64_to_sz(pVariant->v.val_int64, aBuffer))  );
			break;
		}
	case SG_VARIANT_TYPE_DOUBLE:
		{
			SG_ERR_CHECK(  SG_string__append__format(pCtx, sString, "%f", pVariant->v.val_double)  );
			break;
		}
	case SG_VARIANT_TYPE_BOOL:
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, (pVariant->v.val_bool == SG_FALSE ? "false" : "true"))  );
			break;
		}
	case SG_VARIANT_TYPE_SZ:
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, pVariant->v.val_sz)  );
			break;
		}
	case SG_VARIANT_TYPE_VHASH:
		{
			SG_uint32 uCount = 0u;
			SG_uint32 uIndex = 0u;

			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, "{")  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pVariant->v.val_vhash, &uCount)  );
			for (uIndex = 0u; uIndex < uCount; ++uIndex)
			{
				const char*       szKey  = NULL;
				const SG_variant* pValue = NULL;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pVariant->v.val_vhash, uIndex, &szKey, &pValue)  );
				SG_ERR_CHECK(  SG_string__append__format(pCtx, sString, "%s=", szKey)  );
				SG_ERR_CHECK(  _variant_to_string__append(pCtx, pValue, sString)  );
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, ",")  );
			}
			SG_ERR_CHECK(  SG_string__remove(pCtx, sString, SG_string__length_in_bytes(sString)-1u, 1u)  );
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, "}")  );

			break;
		}
	case SG_VARIANT_TYPE_VARRAY:
		{
			SG_uint32 uCount = 0u;
			SG_uint32 uIndex = 0u;

			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, "[")  );
			SG_ERR_CHECK(  SG_varray__count(pCtx, pVariant->v.val_varray, &uCount)  );
			for (uIndex = 0u; uIndex < uCount; ++uIndex)
			{
				const SG_variant* pValue = NULL;

				SG_ERR_CHECK(  SG_varray__get__variant(pCtx, pVariant->v.val_varray,uIndex, &pValue)  );
				SG_ERR_CHECK(  _variant_to_string__append(pCtx, pValue, sString)  );
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, ",")  );
			}
			SG_ERR_CHECK(  SG_string__remove(pCtx, sString, SG_string__length_in_bytes(sString)-1u, 1u)  );
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sString, "]")  );

			break;
		}
	default:
		SG_ARGCHECK(SG_FALSE, pVariant);
	}

fail:
	return;
}

/**
 * Converts an SG_variant into a string suitable for being logged.
 * We purposefully don't use SG_variant__to_string here, because that function
 * is implemented with a jsonwriter and so munges values such that they can be
 * used in JSON.  We want to log values as literally as possible.
 */
static void _variant_to_string(
	SG_context*       pCtx,     //< [in] [out] Error and context info.
	const SG_variant* pVariant, //< [in] The variant to convert to a loggable string.
	SG_string**       ppString  //< [out] A loggable string containing the variant's value.
	)
{
	SG_string* sString = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sString)  );
	SG_ERR_CHECK(  _variant_to_string__append(pCtx, pVariant, sString)  );

	*ppString = sString;
	sString = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sString);
	return;
}

/**
 * Implementation of SG_log__handler__init.
 */
static void _init_handler(
	SG_context* pCtx,
	void*       pThis
	)
{
	SG_log_text__data* pData = (SG_log_text__data*)pThis;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pData->fWriter);

	// initialize the writer
	SG_ERR_CHECK(  pData->fWriter(pCtx, pData->pWriterData, NULL, SG_TRUE)  );

	// write our register message, if we have one
	if (pData->szRegisterMessage != NULL)
	{
		SG_ERR_CHECK(  _write_format(pCtx, pData, NULL, pData->szRegisterMessage, SG_FALSE)  );
	}

fail:
	return;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__PUSHED.
 */
static void _operation_handler__pushed(
	SG_context*              pCtx,
	SG_log_text__data*       pData,
	const SG_log__operation* pOperation
	)
{
	SG_string* pText = NULL;
	const char* szDescription = NULL;

	if (pData->szOperationFormat == NULL)
	{
		return;
	}

	SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, &szDescription, NULL, NULL)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pText, pData->szOperationFormat, szDescription)  );
	SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, SG_string__sz(pText), SG_FALSE)  );

fail:
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__DESCRIBED.
 */
static void _operation_handler__described(
	SG_context*              pCtx,
	SG_log_text__data*       pData,
	const SG_log__operation* pOperation
	)
{
	SG_string*  pText         = NULL;
	const char* szDescription = NULL;

	if (pData->szOperationFormat == NULL)
	{
		return;
	}

	SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, &szDescription, NULL, NULL)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pText, pData->szOperationFormat, szDescription)  );
	SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, SG_string__sz(pText), SG_FALSE)  );

fail:
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__VALUE_SET.
 */
static void _operation_handler__value_set(
	SG_context*              pCtx,
	SG_log_text__data*       pData,
	const SG_log__operation* pOperation
	)
{
	SG_string*        pText        = NULL;
	const char*       szName       = NULL;
	const SG_variant* pValue       = NULL;
	SG_uint32         uFlags       = SG_LOG__FLAG__NONE;
	SG_string*        pValueString = NULL;

	if (pData->szValueFormat == NULL)
	{
		return;
	}

	SG_ERR_CHECK(  SG_log__operation__get_value__latest(pCtx, pOperation, &szName, &pValue, &uFlags)  );
	if ((pData->bLogVerboseValues == SG_FALSE) && (uFlags & SG_LOG__FLAG__VERBOSE))
	{
		return;
	}

	SG_ERR_CHECK(  _variant_to_string(pCtx, pValue, &pValueString)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pText, pData->szValueFormat, szName, SG_string__sz(pValueString))  );
	SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, SG_string__sz(pText), SG_FALSE)  );

fail:
	SG_STRING_NULLFREE(pCtx, pValueString);
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__STEP_DESCRIBED.
 */
static void _operation_handler__step_described(
	SG_context*              pCtx,
	SG_log_text__data*       pData,
	const SG_log__operation* pOperation
	)
{
	SG_string* pText = NULL;
	const char* szDescription = NULL;

	if (pData->szStepFormat == NULL)
	{
		return;
	}

	SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, NULL, &szDescription, NULL)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pText, pData->szStepFormat, szDescription)  );
	SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, SG_string__sz(pText), SG_FALSE)  );

fail:
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__STEPS_FINISHED.
 */
static void _operation_handler__steps_finished(
	SG_context*              pCtx,
	SG_log_text__data*       pData,
	const SG_log__operation* pOperation
	)
{
	SG_UNUSED(pOperation);

	if (pData->szStepMessage != NULL)
	{
		SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, pData->szStepMessage, SG_FALSE)  );
	}

fail:
	return;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__COMPLETED.
 */
static void _operation_handler__completed(
	SG_context*              pCtx,
	SG_log_text__data*       pData,
	const SG_log__operation* pOperation
	)
{
	SG_string* pText = NULL;

	if (pData->szResultFormat != NULL)
	{
		SG_error eResult = SG_ERR_OK;

		SG_ERR_CHECK(  SG_log__operation__get_result(pCtx, pOperation, &eResult)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pText, pData->szResultFormat, eResult)  );
		SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, SG_string__sz(pText), SG_FALSE)  );
	}
	else if (pData->szStepMessage != NULL)
	{
		SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, pData->szStepMessage, SG_FALSE)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Implementation of SG_log__handler__operation.
 */
static void _operation_handler(
	SG_context*              pCtx,
	void*                    pThis,
	const SG_log__operation* pOperation,
	SG_log__operation_change eChange,
	SG_bool*                 pCancel
	)
{
	SG_log_text__data* pData  = (SG_log_text__data*)pThis;
	SG_uint32          uFlags = 0u;

	SG_NULLARGCHECK(pCancel);

	*pCancel = SG_FALSE;

	SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, NULL, NULL, &uFlags)  );
	if ((pData->bLogVerboseOperations == SG_FALSE) && (uFlags & SG_LOG__FLAG__VERBOSE))
	{
		return;
	}

	switch (eChange)
	{
	case SG_LOG__OPERATION__PUSHED:
		SG_ERR_CHECK(  _operation_handler__pushed(pCtx, pData, pOperation)  );
		break;
	case SG_LOG__OPERATION__DESCRIBED:
		SG_ERR_CHECK(  _operation_handler__described(pCtx, pData, pOperation)  );
		break;
	case SG_LOG__OPERATION__VALUE_SET:
		SG_ERR_CHECK(  _operation_handler__value_set(pCtx, pData, pOperation)  );
		break;
	case SG_LOG__OPERATION__STEP_DESCRIBED:
		SG_ERR_CHECK(  _operation_handler__step_described(pCtx, pData, pOperation)  );
		break;
	case SG_LOG__OPERATION__STEPS_FINISHED:
		SG_ERR_CHECK(  _operation_handler__steps_finished(pCtx, pData, pOperation)  );
		break;
	case SG_LOG__OPERATION__COMPLETED:
		SG_ERR_CHECK(  _operation_handler__completed(pCtx, pData, pOperation)  );
		break;
	default:
		break;
	}

fail:
	return;
}

/**
 * Implementation of SG_log__handler__message.
 */
static void _message_handler(
	SG_context*                    pCtx,
	void*                          pThis,
	const SG_log__operation*       pOperation,
	SG_log__message_type           eType,
	const SG_string*               pMessage,
	SG_bool*                       pCancel
	)
{
	SG_log_text__data* pData    = (SG_log_text__data*)pThis;
	const char*        szFormat = NULL;
	SG_string*         pText    = NULL;
	SG_bool            bError   = SG_FALSE;
	SG_uint32          uFlags   = 0u;

	SG_UNUSED(pOperation);
	SG_NULLARGCHECK(pCancel);

	*pCancel = SG_FALSE;

	switch (eType)
	{
	case SG_LOG__MESSAGE__VERBOSE:
		szFormat = pData->szVerboseFormat;
		bError   = SG_FALSE;
		break;
	case SG_LOG__MESSAGE__INFO:
		szFormat = pData->szInfoFormat;
		bError   = SG_FALSE;
		break;
	case SG_LOG__MESSAGE__WARNING:
		szFormat = pData->szWarningFormat;
		bError   = SG_TRUE;
		break;
	case SG_LOG__MESSAGE__ERROR:
		szFormat = pData->szErrorFormat;
		bError   = SG_TRUE;
		break;
	default:
		break;
	}

	if (pOperation != NULL)
	{
		SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, NULL, NULL, &uFlags)  );
		if ((pData->bLogVerboseOperations == SG_FALSE) && (uFlags & SG_LOG__FLAG__VERBOSE) && (bError == SG_FALSE))
		{
			return;
		}
	}
	if (pData->bLogVerboseOperations == SG_FALSE && bError == SG_FALSE)
		return;

	if (szFormat != NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pText)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pText, szFormat, SG_string__sz(pMessage))  );
		SG_ERR_CHECK(  _write_format(pCtx, pData, pOperation, SG_string__sz(pText), bError)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pText);
	return;
}

/**
 * Implementation of SG_log__handler__destroy.
 */
static void _destroy_handler(
	SG_context* pCtx,
	void*       pThis
	)
{
	SG_log_text__data* pData = (SG_log_text__data*)pThis;

	// write our unregister message, if we have one
	if (pData->szUnregisterMessage != NULL)
	{
		SG_ERR_CHECK(  _write_format(pCtx, pData, NULL, pData->szUnregisterMessage, SG_FALSE)  );
	}

	// destroy the writer
	SG_ERR_CHECK(  pData->fWriter(pCtx, pData->pWriterData, NULL, SG_FALSE)  );

fail:
	return;
}

/**
 * Implements the "init" functionality of the path writer.
 */
void _path__init(
	SG_context*                      pCtx,
	SG_log_text__writer__path__data* pThis
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pThis->pPath);

	pThis->pFile = NULL;
	SG_ERR_CHECK(  SG_mutex__init(pCtx, &pThis->cMutex)  );

	// we'll lazily open the file on first write

fail:
	return;
}

/**
 * Implements the "destroy" functionality of the path writer.
 */
void _path__destroy(
	SG_context*                      pCtx,
	SG_log_text__writer__path__data* pThis
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ASSERT(pThis->pFile == NULL || pThis->bReopen == SG_FALSE);

	SG_FILE_NULLCLOSE(pCtx, pThis->pFile);
	SG_ERR_CHECK(  SG_mutex__destroy(&pThis->cMutex)  );

fail:
	return;
}

/**
 * Implements the "write" functionality of the path writer.
 */
void _path__write(
	SG_context*                      pCtx,
	SG_log_text__writer__path__data* pThis,
	const SG_string*                 pText,
	SG_bool                          bError
	)
{
	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__LOGGING);
	SG_NULLARGCHECK_RETURN(pThis);
	SG_NULLARGCHECK(pText);
	SG_UNUSED(bError);

	// make sure only one thread can write at a time
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &pThis->cMutex)  );

	// open the file if it's not already open
	if (pThis->pFile == NULL)
	{
		SG_ERR_CHECK(  _open_file(pCtx, pThis->pPath, pThis->szHeader, pThis->ePermissions, &pThis->pFile)  );
	}

	// write our text to the file
	SG_ERR_CHECK(  SG_file__write__string(pCtx, pThis->pFile, pText)  );

fail:
	// if we're reopening the file each time, then close it now that we're finished
	if (pThis->bReopen == SG_TRUE)
	{
		SG_FILE_NULLCLOSE(pCtx, pThis->pFile);
	}

	// let the next thread in
	_ERR_UNLOCK(pCtx, &pThis->cMutex);

	SG_httprequestprofiler__stop();
	return;
}

/**
 * Implements the "init" functionality of the daily path writer.
 */
void _daily_path__init(
	SG_context*                            pCtx,
	SG_log_text__writer__daily_path__data* pThis
	)
{
	SG_pathname* pPath        = NULL;
	SG_time      cCurrentTime;

	// initialize
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pThis->pBasePath);
	SG_NULLARGCHECK(pThis->szFilenameFormat);
	SG_ARGCHECK(pThis->uCurrentYday > 365u,  pThis->uCurrentYday);
	SG_ARGCHECK(pThis->pCurrentPath == NULL, pThis->pCurrentPath);
	SG_ARGCHECK(pThis->pCurrentFile == NULL, pThis->pCurrentFile);

	// initialize our mutex
	SG_ERR_CHECK(  SG_mutex__init(pCtx, &pThis->cMutex)  );

	// get the current time
	SG_ERR_CHECK(  SG_time__local_time(pCtx, &cCurrentTime)  );

	// build our file path
	// store it in a temporary variable for now
	// that way it's cleaned up in our fail label if an error occurs later in the init process
	// once we know our initialization has fully succeeded, we'll claim it
	SG_ERR_CHECK(  _build_date_path(pCtx, pThis->pBasePath, pThis->szFilenameFormat, &cCurrentTime, &pPath)  );

	// store the yday used to build the path
	// so that we know when to change files
	pThis->uCurrentYday = cCurrentTime.yday;

	// we'll lazily open the file on first write

	// move the temp path to our actual data
	pThis->pCurrentPath = pPath;
	pPath = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;
}

/**
 * Implements the "destroy" functionality of the daily path writer.
 */
void _daily_path__destroy(
	SG_context*                            pCtx,
	SG_log_text__writer__daily_path__data* pThis
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ASSERT(pThis->pCurrentFile == NULL || pThis->bReopen == SG_FALSE);

	SG_FILE_NULLCLOSE(pCtx, pThis->pCurrentFile);
	SG_PATHNAME_NULLFREE(pCtx, pThis->pCurrentPath);
	SG_ERR_CHECK(  SG_mutex__destroy(&pThis->cMutex)  );
	SG_STRING_NULLFREE(pCtx, pThis->pDefaultHeader);

fail:
	return;
}

/**
 * Implements the "write" functionality of the daily path writer.
 */
void _daily_path__write(
	SG_context*                            pCtx,
	SG_log_text__writer__daily_path__data* pThis,
	const SG_string*                       pText,
	SG_bool                                bError
	)
{
	SG_time cCurrentTime;

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__LOGGING);
	SG_NULLARGCHECK_RETURN(pThis);
	SG_NULLARGCHECK(pText);
	SG_UNUSED(bError);

	// make sure only one thread can write at a time
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &pThis->cMutex)  );

	// get the current time
	SG_ERR_CHECK(  SG_time__local_time(pCtx, &cCurrentTime)  );

	// check if the day changed from last time we logged something
	if (cCurrentTime.yday != pThis->uCurrentYday)
	{
		// close the current file and free the current path
		SG_FILE_NULLCLOSE(pCtx, pThis->pCurrentFile);
		SG_PATHNAME_NULLFREE(pCtx, pThis->pCurrentPath);

		// build a new path
		SG_ERR_CHECK(  _build_date_path(pCtx, pThis->pBasePath, pThis->szFilenameFormat, &cCurrentTime, &pThis->pCurrentPath)  );

		// store the new current day
		pThis->uCurrentYday = cCurrentTime.yday;
	}

	// open the file if it's not already open
	if (pThis->pCurrentFile == NULL)
	{
		SG_ERR_CHECK(  _open_file(pCtx, pThis->pCurrentPath, pThis->szHeader, pThis->ePermissions, &pThis->pCurrentFile)  );
	}

	// write our text to the file
	SG_ERR_CHECK(  SG_file__write__string(pCtx, pThis->pCurrentFile, pText)  );

fail:
	// if we're reopening the file each time, then close it now that we're finished
	if (pThis->bReopen == SG_TRUE)
	{
		SG_FILE_NULLCLOSE(pCtx, pThis->pCurrentFile);
	}

	// let the next thread in
	_ERR_UNLOCK(pCtx, &pThis->cMutex);

	SG_httprequestprofiler__stop();
	return;
}


/*
**
** Public Functions
**
*/

void SG_log_text__set_defaults(
	SG_context*        pCtx,
	SG_log_text__data* pData
	)
{
	SG_UNUSED(pCtx);

	pData->fWriter               = NULL;
	pData->pWriterData           = NULL;
	pData->bLogVerboseOperations = SG_TRUE;
	pData->bLogVerboseValues     = SG_TRUE;
	pData->szRegisterMessage     = gszDefaultRegisterMessage;
	pData->szUnregisterMessage   = gszDefaultUnregisterMessage;
	pData->szDateTimeFormat      = gszDefaultDateTimeFormat;
	pData->szProcessThreadFormat = gszDefaultProcessThreadFormat;
	pData->szIndent              = gszDefaultIndent;
	pData->szElapsedTimeFormat   = gszDefaultElapsedTimeFormat;
	pData->szCompletionFormat    = gszDefaultCompletionFormat;
	pData->szOperationFormat     = gszDefaultOperationFormat;
	pData->szValueFormat         = gszDefaultValueFormat;
	pData->szStepFormat          = gszDefaultStepFormat;
	pData->szStepMessage         = gszDefaultStepMessage;
	pData->szResultFormat        = gszDefaultResultFormat;
	pData->szVerboseFormat       = gszDefaultVerboseFormat;
	pData->szInfoFormat          = gszDefaultInfoFormat;
	pData->szWarningFormat       = gszDefaultWarningFormat;
	pData->szErrorFormat         = gszDefaultErrorFormat;
}

void SG_log_text__register(
	SG_context*        pCtx,
	SG_log_text__data* pData,
	SG_log__stats*     pStats,
	SG_uint32          uFlags
	)
{
	SG_ERR_CHECK(  SG_log__register_handler(pCtx, gpFileLogHandler, (void*)pData, pStats, uFlags)  );

fail:
	return;
}

void SG_log_text__unregister(
	SG_context*        pCtx,
	SG_log_text__data* pData
	)
{
	SG_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpFileLogHandler, (void*)pData)  );

fail:
	return;
}

void SG_log_text__writer__std(
	SG_context*      pCtx,
	void*            pData,
	const SG_string* pText,
	SG_bool          bError
	)
{
	SG_console_stream eStream;

	SG_UNUSED(pData);

	if (pText == NULL)
	{
		return;
	}

	if (bError == SG_TRUE)
	{
		eStream = SG_CS_STDERR;
	}
	else
	{
		eStream = SG_CS_STDOUT;
	}

	SG_ERR_CHECK(  SG_console(pCtx, eStream, "%s", SG_string__sz(pText))  );
	SG_ERR_CHECK(  SG_console__flush(pCtx, eStream)  );

fail:
	return;
}

void SG_log_text__writer__file(
	SG_context*      pCtx,
	void*            pData,
	const SG_string* pText,
	SG_bool          bError
	)
{

	//This method seems to be only called from the concurrency test.  It doesn't go through the normal
	//mutex locking stuff
	SG_file* pFile = NULL;

	SG_NULLARGCHECK(pData);
	SG_UNUSED(bError);

	pFile = (SG_file*)pData;

	if (pText == NULL)
	{
		return;
	}
	
	SG_ERR_CHECK(  SG_file__write__string(pCtx, pFile, pText)  );
	SG_ERR_CHECK(  SG_file__fsync(pCtx, pFile)  );

fail:
	return;
}

SG_LOG_TEXT__IMPLEMENT_WRITER(SG_log_text__writer__path, SG_log_text__writer__path__data, _path__init, _path__destroy, _path__write);

void SG_log_text__writer__path__set_defaults(
	SG_context*                      pCtx,
	SG_log_text__writer__path__data* pData
	)
{
	SG_UNUSED(pCtx);

	pData->pPath        = NULL;
	pData->bReopen      = SG_FALSE;
	pData->szHeader     = NULL;
	pData->ePermissions = 0644;
	pData->pFile        = NULL;
}

SG_LOG_TEXT__IMPLEMENT_WRITER(SG_log_text__writer__daily_path, SG_log_text__writer__daily_path__data, _daily_path__init, _daily_path__destroy, _daily_path__write);

void SG_log_text__writer__daily_path__set_defaults(
	SG_context*                            pCtx,
	SG_log_text__writer__daily_path__data* pData
	)
{
	SG_string * pDefaultHeader = NULL;
	const char * szVersion = NULL;

	SG_ERR_CHECK(  SG_lib__version(pCtx, &szVersion)  );

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pDefaultHeader,
		"SourceGear Veracity" SG_PLATFORM_NATIVE_EOL_STR
		"Version %s" SG_PLATFORM_NATIVE_EOL_STR
		SG_PLATFORM_NATIVE_EOL_STR,
		szVersion)  );

	pData->pBasePath        = NULL;
	pData->szFilenameFormat = NULL;
	pData->bReopen          = SG_FALSE;
	pData->szHeader         = SG_string__sz(pDefaultHeader);
	pData->ePermissions     = 0644;
	pData->uCurrentYday     = 1000u; // something outside the range of valid yday values
	pData->pCurrentPath     = NULL;
	pData->pCurrentFile     = NULL;
	pData->pDefaultHeader   = pDefaultHeader;

	return;
fail:
	SG_STRING_NULLFREE(pCtx, pDefaultHeader);
}
