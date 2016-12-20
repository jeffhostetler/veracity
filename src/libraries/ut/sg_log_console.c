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
 * @file    SG_log_console.c
 * @details Implements the SG_log_console module.
 */

#include <sg.h>

/*
**
** Types
**
*/

// forward declarations of our handler implementation functions
static SG_log__handler__operation _operation_handler;
static SG_log__handler__silenced _silenced_handler;
/*
**
** Globals
**
*/

/**
 * Global instance of our handler.
 */
static const SG_log__handler gcConsoleLogHandler =
{
	NULL,
	_operation_handler,
	NULL,
	_silenced_handler,
	NULL,
};
const SG_log__handler* gpConsoleLogHandler = &gcConsoleLogHandler;

// default values for an SG_log_console__data structure
static const char* gszDefaultStepFormat              = " [%s...]";
static const char* gszDefaultCompletionFormat        = " [%d/%d%s: %d%%]";

static const char* gszDefaultOpStackSeparator        = " => ";

/*
**
** Internal Functions
**
*/

static void _writer__std(
	SG_context*      pCtx,
	const char*      pszText
)
{
	if (pszText != NULL)
	{
		SG_ERR_CHECK_RETURN(  SG_console__raw(pCtx, SG_CS_STDERR, pszText)  );
		SG_ERR_CHECK_RETURN(  SG_console__flush(pCtx, SG_CS_STDERR)  );
	}
}

static void _append_ops(
	SG_context* pCtx, 
	SG_log_console__data* pData,
	const SG_vector* pvOps,
	SG_string* pstrWrite)
{
	SG_uint32 i;
	SG_uint32 len = 0;

	SG_ERR_CHECK(  SG_vector__length(pCtx, pvOps, &len)  );
	for (i = 0; i < len; i++)
	{
		const SG_log__operation* pOperation = NULL;
		const char* szText   = NULL;
		const char* szStep   = NULL;

		SG_uint32 uFinished = 0u;
		SG_uint32 uTotal    = 0u;
		SG_uint32 uPercent  = 0u;
		const char* szUnit = NULL;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pvOps, i, (void**)&pOperation)  );
		if (!pOperation)
			continue;

		SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, &szText, &szStep, NULL)  );

		// Append the stack separator if we're not first.
		if (i > 0)
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrWrite, pData->szOpStackSeparator)  );

		SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrWrite, "%s", szText)  );

		// Append completion data 
		if (pData->szCompletionFormat != NULL || pData->szStepFormat != NULL)
		{
			SG_ERR_CHECK(  SG_log__operation__get_progress(pCtx, pOperation, &szUnit, &uFinished, &uTotal, &uPercent, 0u)  );

			if (uTotal > 0 && pData->szCompletionFormat)
			{
				char bufStepUnit[256];

				// We have progress info, and we're configured to output that kind of message.

				// Put leading space on the unit description
				if (szUnit)
					SG_ERR_CHECK(  SG_sprintf_truncate(pCtx, bufStepUnit, sizeof(bufStepUnit), " %s", szUnit)  );
				else
					bufStepUnit[0] = 0;

				SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrWrite, pData->szCompletionFormat, 
					uFinished, uTotal, bufStepUnit, uPercent)  );
			}
		}

		if ( i == (len-1))
		{
			// We're the top of the operation stack

			if (szStep != NULL)
			{
				// We have a step description
				if (!uTotal && szStep && pData->szStepFormat)
				{
					// We have only a step description, and we're configured to output that kind of message.
					SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrWrite, pData->szStepFormat, szStep)  );
				}
				else if (uTotal > 0 && pData->szCompletionFormat)
				{
					// We have progress info and a step description, so we'll append the step description at the end.
					SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrWrite, " ")  );
					SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrWrite, szStep)  );
				}
			}

			if ( (szStep && uTotal) || (!szStep && !uTotal) )
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrWrite, "...")  );
		}
	}

fail:
	;
}

static void _pad_to_console_width(
        SG_context* pCtx,
        SG_string* pstrWrite
        )
{
    SG_uint32 maxWidth = SG_console__get_width() - 1;
    SG_uint32 contentLen = SG_string__length_in_bytes(pstrWrite);

    /* If the message is too wide, show the right-most stuff */
    if (contentLen > maxWidth)
    {
        SG_ERR_CHECK(  SG_string__remove(pCtx, pstrWrite, 0, contentLen - maxWidth + 2)  );
        SG_ERR_CHECK(  SG_string__insert__sz(pCtx, pstrWrite, 0, "..")  );
    }
    /* If the message is smaller than the console's width, append spaces so the previous message is fully overwritten. */
    else if (contentLen < maxWidth)
    {
        SG_uint32 spaces_needed = maxWidth - contentLen;
        while (spaces_needed)
        {
            SG_uint32 now = spaces_needed;
            if (spaces_needed > 80)
            {
                now = 80;
            }
            SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pstrWrite, (SG_byte*) "                                                                                ", now)  );
            spaces_needed -= now;
        }
    }

fail:
    ;
}

/**
 * Writes out a message string.
 * It is formatted to include various status information, as configured in the log handler data.
 */
static void _write_console_format(
	SG_context*              pCtx,      //< [in] [out] Error and context info.
	SG_log_console__data*    pData,     //< [in] Our log handler data.
	const char*              szEpilogue,//< [in] Optionally write something at the end of the line.
    const char*              szFinal
	)
{
	SG_vector* pvOps = NULL;
	SG_string* pstrWrite = NULL;

	SG_ERR_CHECK(  SG_log__stack__get_operations(pCtx, &pvOps)  );
	if (!pvOps)
		return;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrWrite)  );
	SG_ERR_CHECK(  _append_ops(pCtx, pData, pvOps, pstrWrite)  );

	if (szEpilogue)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrWrite, szEpilogue)  );

    SG_ERR_CHECK(  _pad_to_console_width(pCtx, pstrWrite)  );

	// write the line
	SG_ERR_CHECK(  _writer__std(pCtx, "\r")  );
	SG_ERR_CHECK(  _writer__std(pCtx, SG_string__sz(pstrWrite))  );

    if (szFinal)
    {
        SG_ERR_CHECK(  _writer__std(pCtx, szFinal)  );
    }

fail:
	SG_VECTOR_NULLFREE(pCtx, pvOps);
	SG_STRING_NULLFREE(pCtx, pstrWrite);
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__COMPLETED.
 */
static void _operation_handler__completed(
	SG_context*              pCtx,
	SG_log_console__data*    pData
	)
{
	SG_uint32 stackSize = 0;
	const char* szTempStepFormat = NULL;
    SG_string* pstr = NULL;

	/* If this is the last operation on the stack, clear the completion percent, 
	   write "Done," and start a new line. */
	SG_ERR_CHECK(  SG_log__stack__get_count(pCtx, &stackSize)  );
	if (stackSize == 1)
	{
        SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr)  );
        SG_ERR_CHECK(  _pad_to_console_width(pCtx, pstr)  );
        SG_ERR_CHECK(  _writer__std(pCtx, "\r")  );
        SG_ERR_CHECK(  _writer__std(pCtx, SG_string__sz(pstr))  );
        SG_ERR_CHECK(  _writer__std(pCtx, "\r")  );
        SG_STRING_NULLFREE(pCtx, pstr);
	}
    else
    {
        /* Clear the previous step's description, if there was one, by temporarily removing the format string. */
        szTempStepFormat = pData->szStepFormat;
        pData->szStepFormat = NULL;

        SG_ERR_CHECK(  _write_console_format(pCtx, pData, NULL, NULL)  );
    }

	/* Fall through */

fail:
	// Restore normal step format
    SG_STRING_NULLFREE(pCtx, pstr);
	pData->szStepFormat = szTempStepFormat;
}

/**
 * Implementation of SG_log__handler__operation, SG_LOG__OPERATION__STEPS_FINISHED.
 */
static void _operation_handler__steps_finished(
	SG_context*              pCtx,
	SG_log_console__data*    pData
	)
{
	const char* szTempStepFormat = NULL;

	// Clear the step's description, if there was one, by temporarily removing the format string.
	szTempStepFormat = pData->szStepFormat;
	pData->szStepFormat = NULL;

	SG_ERR_CHECK(  _write_console_format(pCtx, pData, NULL, NULL)  );

	/* Fall through */
fail:
	// Restore normal step format.
	pData->szStepFormat = szTempStepFormat;
}

/**
 * Implementation of SG_log__handler__silenced.
 */
static void _silenced_handler(
	SG_context* pCtx,
	void*       pThis,
	SG_bool     bSilenced
	)
{
	SG_log_console__data* pData  = (SG_log_console__data*)pThis;
	pData->bSilenced = bSilenced;
	SG_UNUSED(pCtx);
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
	SG_log_console__data* pData  = (SG_log_console__data*)pThis;
	SG_uint32          uFlags = 0u;

	SG_NULLARGCHECK(pCancel);

	*pCancel = SG_FALSE;

	if (pData->bSilenced)
		return;

	SG_ERR_CHECK(  SG_log__operation__get_basic(pCtx, pOperation, NULL, NULL, &uFlags)  );
	if ((pData->bLogVerboseOperations == SG_FALSE) && (uFlags & SG_LOG__FLAG__VERBOSE))
	{
		return;
	}

	switch (eChange)
	{
	case SG_LOG__OPERATION__PUSHED:
		SG_ERR_CHECK_RETURN(  _write_console_format(pCtx, pData, NULL, NULL)  );
		break;
	case SG_LOG__OPERATION__DESCRIBED:
		SG_ERR_CHECK_RETURN(  _write_console_format(pCtx, pData, NULL, NULL)  );
		break;
	case SG_LOG__OPERATION__VALUE_SET:
		/* do nothing */
		break;
	case SG_LOG__OPERATION__STEP_DESCRIBED:
		SG_ERR_CHECK_RETURN(  _write_console_format(pCtx, pData, NULL, NULL)  );
		break;
	case SG_LOG__OPERATION__STEPS_FINISHED:
		SG_ERR_CHECK(  _operation_handler__steps_finished(pCtx, pData)  );
		break;
	case SG_LOG__OPERATION__COMPLETED:
		SG_ERR_CHECK(  _operation_handler__completed(pCtx, pData)  );
		break;
	default:
		break;
	}

fail:
	return;
}

/*
**
** Public Functions
**
*/

void SG_log_console__set_defaults(
	SG_context*        pCtx,
	SG_log_console__data* pData
	)
{
	SG_UNUSED(pCtx);

	pData->bLogVerboseOperations = SG_TRUE;
	
	pData->szCompletionFormat        = gszDefaultCompletionFormat;
	pData->szStepFormat              = gszDefaultStepFormat;

	pData->szOpStackSeparator = gszDefaultOpStackSeparator;
}

void SG_log_console__register(
	SG_context*				pCtx,
	SG_log_console__data*	pData,
	SG_log__stats*			pStats,
	SG_uint32				uFlags
	)
{
	SG_ERR_CHECK(  SG_log__register_handler(pCtx, gpConsoleLogHandler, (void*)pData, pStats, uFlags)  );

fail:
	return;
}

void SG_log_console__unregister(
	SG_context*        pCtx,
	SG_log_console__data* pData
	)
{
	SG_ERR_CHECK(  SG_log__unregister_handler(pCtx, gpConsoleLogHandler, (void*)pData)  );

fail:
	return;
}
