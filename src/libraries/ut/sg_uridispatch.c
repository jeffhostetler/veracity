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

#include <sg.h>
#include <ctype.h>

#include "sg_js_safeptr__private.h"

static struct
{
	// List of included (available) or excluded (unavailable) repos. Only one of these will be non-null.
	SG_vhash * pInclude;
	SG_vhash * pExclude;

	SG_bool enableDiagnostics;
	SG_uint32 debugDelay;

	// We own this pointer, but don't really use it ourselves. We are simply
	// the ones responsible for figuring out its value and make sure its memory
	// persists for the lifetime of the server. sg_jscontextpool has a pointer
	// to it, and the http server (mongoose, IIS, etc) might also have one.
	SG_string * pApplicationRoot;

	SG_log_text__data cLogData;
	SG_log_text__writer__daily_path__data cLogFileWriterData;

} * gpUridispatchGlobalState = NULL;

SG_bool _sg_uridispatch__debug_remote_shutdown = SG_FALSE;

static void _register_server_log_handler(
	SG_context*        pCtx,
	SG_log_text__data* pLogData,
	SG_log_text__writer__daily_path__data * pLogFileWriterData
	)
{
	char * szLogLevel = NULL;
	char * szLogPath  = NULL;
	SG_uint32	logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;

	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(pLogData!=NULL);
	SG_ASSERT(pLogFileWriterData!=NULL);

	// build the log file path
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_PATH, NULL, &szLogPath, NULL)  );

	// get the configured log level
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_LEVEL, NULL, &szLogLevel, NULL)  );

	// register the handler
	SG_ERR_CHECK(  SG_log_text__set_defaults(pCtx, pLogData)  );
	pLogData->fWriter             = SG_log_text__writer__daily_path;
	pLogData->pWriterData         = pLogFileWriterData;
	pLogData->szRegisterMessage   = "\\---- veracity server started logging ----";
	pLogData->szUnregisterMessage = "\\---- veracity server stopped logging ----";
	if (szLogLevel != NULL)
	{
		if (SG_stricmp(szLogLevel, "quiet") == 0)
		{
			pLogData->bLogVerboseOperations = SG_FALSE;
			pLogData->bLogVerboseValues     = SG_FALSE;
			pLogData->szVerboseFormat       = NULL;
			pLogData->szInfoFormat          = NULL;
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__QUIET;
		}
		else if (SG_stricmp(szLogLevel, "normal") == 0)
		{
			pLogData->bLogVerboseOperations = SG_FALSE;
			pLogData->bLogVerboseValues     = SG_FALSE;
			pLogData->szVerboseFormat = NULL;
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__NORMAL;
		}
//		else if (SG_stricmp(szLogLevel, "verbose") == 0)
//		{
//		}
	}
	logFileFlags |= SG_LOG__FLAG__DETAILED_MESSAGES;
	SG_ERR_CHECK(  SG_log_text__writer__daily_path__set_defaults(pCtx, pLogFileWriterData)  );
	pLogFileWriterData->bReopen          = SG_FALSE;
	if (szLogPath != NULL)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pLogFileWriterData->pBasePath, szLogPath)  );
	else
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__LOG_DIRECTORY(pCtx, &pLogFileWriterData->pBasePath)  );
	pLogFileWriterData->szFilenameFormat = "veracity-%d-%02d-%02d.log";
	SG_ERR_CHECK(  SG_log_text__register(pCtx, pLogData, NULL, logFileFlags)  );

	SG_NULLFREE(pCtx, szLogLevel);
	SG_NULLFREE(pCtx, szLogPath);

	return;
fail:
	SG_NULLFREE(pCtx, szLogLevel);
	SG_NULLFREE(pCtx, szLogPath);
	SG_PATHNAME_NULLFREE(pCtx, pLogFileWriterData->pBasePath);
	return;
}

static void _unregister_server_log_handler(
	SG_context * pCtx,
	SG_log_text__data * pLogData,
	SG_log_text__writer__daily_path__data * pLogFileWriterData
	)
{
	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(pLogData!=NULL);
	SG_ASSERT(pLogFileWriterData!=NULL);

	SG_ERR_IGNORE(  SG_log_text__unregister(pCtx, pLogData)  );
	SG_PATHNAME_NULLFREE(pCtx, pLogFileWriterData->pBasePath);
}

void SG_uridispatch__init(
	SG_context * pCtx,
	const char ** ppApplicationRoot
	)
{
	char * szConfigSetting = NULL;

	SG_pathname * pPathCwd = NULL;
	SG_string * pWdRepoName = NULL;

	SG_varray * pList = NULL;

	if(gpUridispatchGlobalState != NULL)
		return;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, gpUridispatchGlobalState)  );

	// configure and register our global log handler
	if(ppApplicationRoot!=NULL) // (But not if we're the 'vv serve' internal server, since vv itself will be logging already.)
		SG_ERR_CHECK(  _register_server_log_handler(pCtx, &gpUridispatchGlobalState->cLogData, &gpUridispatchGlobalState->cLogFileWriterData)  );

	// Determine the "Application Root" and store it in a variable that we can let others use for
	// the lifetime of the server.
	if(ppApplicationRoot!=NULL)
	{
		SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_APPLICATIONROOT, NULL, &szConfigSetting, NULL)  );
		if(szConfigSetting!=NULL)
		{
			// As an example of what APPLICATION_ROOT is in a url, the url for a repo called "my_repo" is:
			// 
			//     "http://server:port" + APPLICATION_ROOT + "/repos/my_repo"
			// 
			// Ie, it MUST start with a slash AND NOT end with a slash (... N.B.: Or it can be an empty string).

			const char * pAppRoot = szConfigSetting; // the APPLICATION_ROOT, minus the beginning slash (if there is one)
			SG_int32 len = 0;

			while(pAppRoot[0]=='/')
				++pAppRoot;

			len = SG_STRLEN(pAppRoot);
			while(len>0 && pAppRoot[len-1]=='/')
				--len;

			if(len>0)
			{
				SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &gpUridispatchGlobalState->pApplicationRoot, "/")  );
				SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, gpUridispatchGlobalState->pApplicationRoot, (const SG_byte *)pAppRoot, len)  );
			}
			else
				SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &gpUridispatchGlobalState->pApplicationRoot, "")  );

			SG_NULLFREE(pCtx, szConfigSetting);
		}
		else
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &gpUridispatchGlobalState->pApplicationRoot, "/veracity")  );
		*ppApplicationRoot = SG_string__sz(gpUridispatchGlobalState->pApplicationRoot);
	}
	else
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &gpUridispatchGlobalState->pApplicationRoot, "")  );

	// Get the enable_diagnostics flag.
	gpUridispatchGlobalState->enableDiagnostics = SG_FALSE;
	SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_ENABLE_DIAGNOSTICS, NULL, &szConfigSetting, NULL);
	if(!SG_context__has_err(pCtx) && szConfigSetting!=NULL)
		gpUridispatchGlobalState->enableDiagnostics = (SG_stricmp(szConfigSetting, "true")==0);
	if(SG_context__has_err(pCtx))
	{
		SG_log__report_error__current_error(pCtx);
		SG_context__err_reset(pCtx);
	}
	SG_NULLFREE(pCtx, szConfigSetting);

	// Get the debug delay.
	gpUridispatchGlobalState->debugDelay = 0;
	SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_DEBUG_DELAY, NULL, &szConfigSetting, NULL);
	if(!SG_context__has_err(pCtx) && szConfigSetting!=NULL)
		SG_uint32__parse__strict(pCtx, &gpUridispatchGlobalState->debugDelay, szConfigSetting);
	if(SG_context__has_err(pCtx))
	{
		SG_log__report_error__current_error(pCtx);
		SG_context__err_reset(pCtx);
	}
	SG_NULLFREE(pCtx, szConfigSetting);

	// Start up the js context pool.
	SG_ERR_CHECK(  SG_jscontextpool__init(pCtx, SG_string__sz(gpUridispatchGlobalState->pApplicationRoot))  );

	// Figure available repos.
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathCwd, ".")  );
		SG_workingdir__find_mapping(pCtx, pPathCwd, NULL, &pWdRepoName, NULL);
		if(!SG_context__has_err(pCtx))
		{
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &gpUridispatchGlobalState->pInclude)  );
			SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, gpUridispatchGlobalState->pInclude, SG_string__sz(pWdRepoName), SG_TRUE)  );
			SG_STRING_NULLFREE(pCtx, pWdRepoName);
		}
		else
		{
			SG_uint32 len = 0;
			SG_uint32 i = 0;
			const char * sz = NULL;
			
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOT_A_WORKING_COPY);

			SG_ERR_CHECK(  SG_localsettings__get__varray(pCtx, SG_LOCALSETTING__SERVER_REPO_INCLUDES, NULL, &pList, NULL)  );
			if(pList!=NULL)
			{
				SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &gpUridispatchGlobalState->pInclude)  );
				SG_ERR_CHECK(  SG_varray__count(pCtx, pList, &len)  );
				for(i=0; i<len; ++i)
				{
					SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pList, i, &sz)  );
					SG_vhash__add__bool(pCtx, gpUridispatchGlobalState->pInclude, sz, SG_TRUE);
					SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_VHASH_DUPLICATEKEY);
				}
				SG_VARRAY_NULLFREE(pCtx, pList);
			}

			SG_ERR_CHECK(  SG_localsettings__get__varray(pCtx, SG_LOCALSETTING__SERVER_REPO_EXCLUDES, NULL, &pList, NULL)  );
			if(pList!=NULL)
			{
				if(gpUridispatchGlobalState->pInclude!=NULL)
				{
					SG_ERR_CHECK(  SG_varray__count(pCtx, pList, &len)  );
					for(i=0; i<len; ++i)
					{
						SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pList, i, &sz)  );
						SG_vhash__remove(pCtx, gpUridispatchGlobalState->pInclude, sz);
						SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_VHASH_KEYNOTFOUND);
					}
					SG_VARRAY_NULLFREE(pCtx, pList);
				}
				else
				{
					SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &gpUridispatchGlobalState->pExclude)  );
					SG_ERR_CHECK(  SG_varray__count(pCtx, pList, &len)  );
					for(i=0; i<len; ++i)
					{
						SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pList, i, &sz)  );
						SG_vhash__add__bool(pCtx, gpUridispatchGlobalState->pExclude, sz, SG_TRUE);
						SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_VHASH_DUPLICATEKEY);
					}
					SG_VARRAY_NULLFREE(pCtx, pList);
				}
			}
		}
		SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	}

	return;
fail:
	SG_NULLFREE(pCtx, szConfigSetting);

	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_STRING_NULLFREE(pCtx, pWdRepoName);

	SG_VHASH_NULLFREE(pCtx, gpUridispatchGlobalState->pInclude);
	SG_VHASH_NULLFREE(pCtx, gpUridispatchGlobalState->pExclude);
	SG_STRING_NULLFREE(pCtx, gpUridispatchGlobalState->pApplicationRoot);
	SG_NULLFREE(pCtx, gpUridispatchGlobalState);
}

void sg_uridispatch__query_repo_availability(SG_context * pCtx, const char * szRepoName, SG_bool * pbIsAvailable)
{
	SG_bool dummy = SG_TRUE;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(szRepoName);
	SG_NULLARGCHECK_RETURN(pbIsAvailable);

	if(gpUridispatchGlobalState==NULL)
	{
		*pbIsAvailable = SG_TRUE;
	}
	else if(gpUridispatchGlobalState->pInclude!=NULL)
	{
		SG_vhash__get__bool(pCtx, gpUridispatchGlobalState->pInclude, szRepoName, &dummy);
		if(SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
		{
			*pbIsAvailable = SG_FALSE;
			SG_context__err_reset(pCtx);
		}
		else if(!SG_context__has_err(pCtx))
			*pbIsAvailable = SG_TRUE;
		else
			SG_ERR_CHECK_RETURN_CURRENT;
	}
	else if(gpUridispatchGlobalState->pExclude!=NULL)
	{
		SG_vhash__get__bool(pCtx, gpUridispatchGlobalState->pExclude, szRepoName, &dummy);
		if(SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
		{
			*pbIsAvailable = SG_TRUE;
			SG_context__err_reset(pCtx);
		}
		else if(!SG_context__has_err(pCtx))
			*pbIsAvailable = SG_FALSE;
		else
			SG_ERR_CHECK_RETURN_CURRENT;
	}
	else
	{
		*pbIsAvailable = SG_TRUE;
	}

}

/**
 * This routine will tell you if a repository is allowed to be created or deleted remotely.
 * 
 * We disallow creation/deletion if any repository includes or excludes are configured, or
 * if the server was started inside a working folder.
 */
void sg_uridispatch__repo_addremove_allowed(SG_context* pCtx, SG_bool* pbAllowed)
{
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pbAllowed);

	if (gpUridispatchGlobalState)
	{
		SG_uint32 count = 0;

		if (gpUridispatchGlobalState->pInclude)
		{
			/* On startup, the working copy's respository is added to this vhash if the server
			 * was started inside a working folder. See SG_uridispatch__init(). */
			SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, gpUridispatchGlobalState->pInclude, &count)  );
			if (count)
			{
				*pbAllowed = SG_FALSE;
				return;
			}
		}
		if (gpUridispatchGlobalState->pExclude)
		{
			SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, gpUridispatchGlobalState->pExclude, &count)  );
			if (count)
			{
				*pbAllowed = SG_FALSE;
				return;
			}
		}
	}

	*pbAllowed = SG_TRUE;
}

///////////////////////////////////////////////////////////////////////////////

struct _sg_uridispatchcontext
{
	SG_context * pCtx;

	// These two members provide a simple mechanism for returning plaintext error responses without requiring SSJS.
	const char * szErrorStatusCode; // Eg: "413 Request Entity Too Large."
	const char * szErrorMessage; // If left NULL, the szErrorStatusCode will be sent as the message body as well.
	SG_string * pErrorString; // String we own. Sometimes szErrorMessage points to it. Sometimes not.

	SG_bool requestMethodIsReallyHEAD;

	SG_jscontext * pJs;
	JSObject * requestObject;
	JSObject * requestHandlers; // Object with callback functions to handle incoming message body.
	JSObject * responseObject;
	SG_bool jsobjectsRooted;

	SG_uint64 requestLength;
	SG_uint64 requestLength_chunked;
	char * szStatusCode; // Status code returned by SSJS.
	SG_uint64 responseLength;
	SG_uint64 responseLength_chunked;

	SG_string * pIncomingJson;
	SG_tempfile * pIncomingFile;
	SG_cbuffer * pOutgoingChunk;
};

// For when all else fails. This is not a valid dispatch context, so don't try to access its members.
SG_uridispatchcontext * URIDISPATCHCONTEXT__MALLOC_FAILED = (SG_uridispatchcontext*)(void*)&URIDISPATCHCONTEXT__MALLOC_FAILED;
#define SZ_MALLOC_FAILED "Memory allocation failure."

SG_uridispatchcontext * URIDISPATCHCONTEXT__NO_pCtx_PROVIDED = (SG_uridispatchcontext*)(void*)&URIDISPATCHCONTEXT__NO_pCtx_PROVIDED;
#define SZ_NO_pCtx_PROVIDED "An unknown error has occurred."

void _SG_uridispatchcontext__nullfree(SG_uridispatchcontext ** ppDispatchContext)
{
	SG_context * pCtx = NULL;
	SG_uridispatchcontext * pDispatchContext = NULL;

	if(ppDispatchContext==NULL || *ppDispatchContext==NULL)
		return;

	if(*ppDispatchContext==URIDISPATCHCONTEXT__MALLOC_FAILED || *ppDispatchContext==URIDISPATCHCONTEXT__NO_pCtx_PROVIDED)
	{
		*ppDispatchContext = NULL;
		return;
	}

	pDispatchContext = *ppDispatchContext;
	pCtx = pDispatchContext->pCtx;
	SG_context__err_reset(pCtx);

	SG_STRING_NULLFREE(pCtx, pDispatchContext->pErrorString);

	SG_jscontext__resume(pDispatchContext->pJs);

	// There might not be a JS context if there was an error loading one of the 
	// SSJS files (i.e. a syntax error)
	if(pDispatchContext->pJs!=NULL && pDispatchContext->jsobjectsRooted)
	{
		jsval rval;
		if(pDispatchContext->requestHandlers!=NULL)
		{
			jsval args[1];
			args[0] = OBJECT_TO_JSVAL(pDispatchContext->requestObject);
			(void)JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onRequestAborted", SG_NrElements(args), args, &rval);
			SG_context__err_reset(pCtx);
		}
		if(pDispatchContext->requestObject!=NULL)
		{
			(void)JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->requestObject, "close", 0, NULL, &rval);
			SG_context__err_reset(pCtx);
		}
		if(pDispatchContext->responseObject!=NULL)
		{
			jsval n = JSVAL_NULL;
			(void)JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "finalize", 0, NULL, &rval);
			SG_context__err_reset(pCtx);
			(void)JS_SetProperty(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "finalize", &n);
		}

		(void)JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->pJs->glob, "onDispatchContextNullfree", 0, NULL, &rval);
		SG_context__err_reset(pCtx);

		(void)JS_RemoveObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestObject);
		(void)JS_RemoveObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestHandlers);
		(void)JS_RemoveObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->responseObject);

		SG_ERR_IGNORE(  SG_jscontext__release(pCtx, &pDispatchContext->pJs)  );
	}
	else if(pDispatchContext->pJs!=NULL)
	{
		// Unlikely this will ever happen. We were able to acquire a JSContext but unable to root our objects.
		SG_ERR_IGNORE(  SG_jscontext__release(pCtx, &pDispatchContext->pJs)  );
	}

	SG_NULLFREE(pCtx, pDispatchContext->szStatusCode);
	SG_STRING_NULLFREE(pCtx, pDispatchContext->pIncomingJson);
	if(pDispatchContext->pIncomingFile!=NULL)
	{
		if(pDispatchContext->pIncomingFile->pFile!=NULL)
			SG_FILE_NULLCLOSE(pCtx, pDispatchContext->pIncomingFile->pFile);

		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pDispatchContext->pIncomingFile->pPathname)  );

		SG_PATHNAME_NULLFREE(pCtx, pDispatchContext->pIncomingFile->pPathname);
		SG_NULLFREE(pCtx, pDispatchContext->pIncomingFile);
	}
	SG_cbuffer__nullfree(&pDispatchContext->pOutgoingChunk);

	SG_NULLFREE(pCtx, pDispatchContext);

	*ppDispatchContext = NULL;
}


///////////////////////////////////////////////////////////////////////////////

static void _sg_uridispatch__log_pCtx_error_and_fetch_response_text_into_szErrorMessage(SG_context * pCtx, SG_uridispatchcontext * pDispatchContext)
{
	SG_error err;

	SG_log__report_error__current_error(pCtx); // Log the error.

	pDispatchContext->szErrorStatusCode = "500 Internal Server Error"; 

	if(gpUridispatchGlobalState->enableDiagnostics)
		err = SG_context__err_to_string(pCtx, SG_TRUE, &pDispatchContext->pErrorString);
	else
	{
		SG_context__push_level(pCtx);
		SG_STRING__ALLOC__SZ(pCtx, &pDispatchContext->pErrorString, "An error occurred processing the request. The error has been recorded in the server's log file.\nFor further information, contact the server's administrator.");
		(void)SG_context__get_err(pCtx, &err);
		SG_context__pop_level(pCtx);
	}

	if(err==SG_ERR_MALLOCFAILED && SG_context__err_equals(pCtx, SG_ERR_MALLOCFAILED))
		pDispatchContext->szErrorMessage = SZ_MALLOC_FAILED;
	else if(pDispatchContext->pErrorString==NULL)
		pDispatchContext->szErrorMessage = "An unknown error has occurred.";
	else
		pDispatchContext->szErrorMessage = SG_string__sz(pDispatchContext->pErrorString);
}

static void _sg_uridispatch__log_pCtx_error_if_necessary_and_prepare_http_response(SG_context * pCtx, SG_uridispatchcontext * pDispatchContext)
{
	SG_ASSERT(pCtx && SG_context__has_err(pCtx));
	SG_ASSERT(pDispatchContext!=NULL);

	// Extract error information from pCtx.
	if(SG_context__err_equals(pCtx, SG_ERR_HTTP_400_BAD_REQUEST) || SG_context__err_equals(pCtx, SG_ERR_HTTP_413_REQUEST_ENTITY_TOO_LARGE))
	{
		SG_error err;
		const char * szMessage = NULL;

		// Client error. Don't log it to server log--just report it to them.

		if(SG_context__err_equals(pCtx, SG_ERR_HTTP_413_REQUEST_ENTITY_TOO_LARGE))
			pDispatchContext->szErrorStatusCode = "413 Request Entity Too Large";
		else
			pDispatchContext->szErrorStatusCode = "400 Bad Request";

		err = SG_context__err_get_description(pCtx, &szMessage);
		if(SG_IS_OK(err) && szMessage!=NULL)
		{
			SG_context__push_level(pCtx);
			SG_STRING__ALLOC__SZ(pCtx, &pDispatchContext->pErrorString, szMessage);
			(void)SG_context__get_err(pCtx, &err);
			SG_context__pop_level(pCtx);
			if(SG_IS_OK(err))
				pDispatchContext->szErrorMessage = SG_string__sz(pDispatchContext->pErrorString);
		}
	}
	else
	{
		_sg_uridispatch__log_pCtx_error_and_fetch_response_text_into_szErrorMessage(pCtx, pDispatchContext);
	}

	// We've extracted the error information. Now reset pCtx.
	SG_context__err_reset(pCtx);

	// Construct SSJS responseObject, if possible.
	if(pDispatchContext->pJs)
	{
		JSString * jsstrErrorStatusCode;
		JSString * jsstrErrorMessage;
		jsval args[3];
		JSBool ok;
		jsval rval;

		SG_jscontext__resume(pDispatchContext->pJs);

		if(pDispatchContext->responseObject!=NULL)
			SG_ERR_IGNORE(  SG_log__report_warning(pCtx,"Discarding previously generated response and replacing it with an error response.")  );

		jsstrErrorStatusCode = JS_NewStringCopyZ(pDispatchContext->pJs->cx, pDispatchContext->szErrorStatusCode);
		jsstrErrorMessage = JS_NewStringCopyZ(pDispatchContext->pJs->cx, pDispatchContext->szErrorMessage);
		if(jsstrErrorStatusCode && jsstrErrorMessage)
		{
			args[0] = STRING_TO_JSVAL(jsstrErrorStatusCode);
			args[1] = OBJECT_TO_JSVAL(pDispatchContext->requestObject);
			args[2] = STRING_TO_JSVAL(jsstrErrorMessage);
			ok = JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->pJs->glob, "errorResponse", SG_NrElements(args), args, &rval);
			if(ok && !SG_context__has_err(pCtx) && JSVAL_IS_NONNULL_OBJECT(rval))
			{
				pDispatchContext->responseObject = JSVAL_TO_OBJECT(rval);
				pDispatchContext->szErrorStatusCode = NULL;
				pDispatchContext->szErrorMessage = NULL;
			}
			else
				SG_context__err_reset(pCtx);
		}

		SG_jscontext__suspend(pDispatchContext->pJs);
	}
}

static void _sg_uridispatch__get_response_headers_from_SSJS_responseObject(SG_context * pCtx, SG_uridispatchcontext * pDispatchContext, const char ** ppStatusCode, SG_uint64 * pContentLength, SG_vhash ** ppResponseHeaders)
{
	JSBool ok;
	jsval statusCode;
	jsval headersVal;
	JSObject * headersObject;
	jsval contentLengthVal;
	char *szContentLength = NULL;

	SG_vhash * pResponseHeaders = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(pDispatchContext!=NULL);
	SG_ASSERT(ppStatusCode!=NULL);
	SG_ASSERT(pContentLength!=NULL);
	SG_ASSERT(ppResponseHeaders!=NULL);

	SG_jscontext__resume(pDispatchContext->pJs);

	ok = JS_GetProperty(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "statusCode", &statusCode);
	if(!ok || !JSVAL_IS_STRING(statusCode))
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Unable to retrieve Status Code from response object!"));
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, pDispatchContext->pJs->cx, JSVAL_TO_STRING(statusCode), &pDispatchContext->szStatusCode)  );

	ok = JS_GetProperty(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "headers", &headersVal);
	if(!ok || !JSVAL_IS_NONNULL_OBJECT(headersVal))
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Unable to retrieve headers from response object!"));
	headersObject = JSVAL_TO_OBJECT(headersVal);
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, pDispatchContext->pJs->cx, headersObject, &pResponseHeaders)  );

	ok = JS_GetProperty(pDispatchContext->pJs->cx, headersObject, "Content-Length", &contentLengthVal);
	if(ok && JSVAL_IS_INT(contentLengthVal))
		pDispatchContext->responseLength = JSVAL_TO_INT(contentLengthVal);
	else if(ok && JSVAL_IS_STRING(contentLengthVal))
	{
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, pDispatchContext->pJs->cx, JSVAL_TO_STRING(contentLengthVal), &szContentLength)  );
		SG_ERR_CHECK(  SG_uint64__parse__strict(pCtx, &pDispatchContext->responseLength, szContentLength)  );
		SG_NULLFREE(pCtx, szContentLength);
	}
	else
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Error retrieving Content-Length from response headers (value missing or of the wrong type?)."));
	SG_ERR_CHECK(  SG_vhash__remove(pCtx, pResponseHeaders, "Content-Length")  );

	*ppStatusCode = pDispatchContext->szStatusCode;
	*pContentLength = pDispatchContext->responseLength;
	*ppResponseHeaders = pResponseHeaders;

	SG_jscontext__suspend(pDispatchContext->pJs);

	return;
fail:
	SG_jscontext__suspend(pDispatchContext->pJs);

	SG_VHASH_NULLFREE(pCtx, pResponseHeaders);
	SG_NULLFREE(pCtx, szContentLength);
}

///////////////////////////////////////////////////////////////////////////////

	static void nextLine(
		SG_context *pCtx,
		SG_string *line,
		const char *start,
		const char **pNext,
		SG_bool *lfOnly
		)
	{
		const char *next = start;

		while ((*next) && (*next!= '\r') && (*next != '\n'))
		{
			++next;
		}

		SG_ERR_CHECK(  SG_string__set__buf_len(	pCtx, line, (const SG_byte *)start, 
												(SG_uint32)((next - start) * sizeof(char))
											  )
				    );

		if (*next == '\n')
		{
			++next;
			*lfOnly = SG_TRUE;
		}
		else
		{
			*lfOnly = SG_FALSE;
			++next;

			if (*next == '\n')
				++next;
		}

		*pNext = next;

	fail:
		;
	}

	static void _stringTrim(
		SG_context *pCtx,
		SG_string *str)
	{
		const char *firstNonWhiteSpace = NULL;
		const char *lastNonWhiteSpace = NULL;
		const char *st = SG_string__sz(str);
		SG_string *tstr = NULL;

		while (*st)
		{
			if (! isspace(*st))
			{
				if (firstNonWhiteSpace == NULL)
					firstNonWhiteSpace = st;
				lastNonWhiteSpace = st;
			}

			++st;
		}

		if ((firstNonWhiteSpace != NULL) && (lastNonWhiteSpace > firstNonWhiteSpace))
		{
			if ((*firstNonWhiteSpace == '"') && (*lastNonWhiteSpace == '"'))
			{
				++firstNonWhiteSpace;
				--lastNonWhiteSpace;
			}

			if (firstNonWhiteSpace > lastNonWhiteSpace)
				firstNonWhiteSpace = lastNonWhiteSpace = NULL;
		}

		if (firstNonWhiteSpace == NULL)
			SG_ERR_CHECK(  SG_string__clear(pCtx, str)  );
		else
		{
			SG_ERR_CHECK(  SG_string__alloc__buf_len(pCtx, &tstr, (const SG_byte *)firstNonWhiteSpace, 
														  (SG_uint32)(lastNonWhiteSpace + 1 - firstNonWhiteSpace) * sizeof(char)  )
							   );
			SG_ERR_CHECK(  SG_string__set__string(pCtx, str, tstr)  );
		}

fail:
		SG_STRING_NULLFREE(pCtx, tstr);
	}

	static void _parseHeaderChunk(
		SG_context *pCtx,
		const char *chunk,
		SG_string *vari,
		SG_string *value)
	{
		SG_uint32 pos;

		SG_ERR_CHECK(  SG_ascii__find__char(pCtx, chunk, '=', &pos)  );

		if (pos == SG_UINT32_MAX)
		{
			SG_ERR_CHECK(  SG_string__set__sz(pCtx, vari, chunk)  );
			SG_ERR_CHECK(  SG_string__clear(pCtx, value)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_string__set__buf_len(pCtx, vari, (const SG_byte *)chunk, pos * sizeof(char))  );
			SG_ERR_CHECK(  SG_string__set__sz(pCtx, value, chunk + pos + 1)  );
		}

		SG_ERR_CHECK(  _stringTrim(pCtx, vari)  );
		SG_ERR_CHECK(  _stringTrim(pCtx, value)  );

fail:
		;
	}

static void _parseMimeFile(
	SG_context *pCtx, 
	const SG_pathname *rawpathname, 
	SG_string **ppFilepath, 
	SG_string **ppFilename, 
	SG_string **ppMimeType)
{
	SG_string *filepath = NULL;
	SG_string *filename = NULL;
	SG_string *separator = NULL;
	SG_string *line = NULL;
	const char *next = NULL;
	const char *start = NULL;
	SG_uint32 len = 0;
	SG_bool	lfOnly = SG_FALSE;
	SG_bool done = SG_FALSE;
	const SG_byte *sepPos = NULL;
	const SG_byte *bufEnd = NULL;
	SG_uint32 dataBytes = 0;
	SG_pathname *tempDir = NULL;
	SG_pathname *fn = NULL;
	SG_string *uploadfn = NULL;
	SG_file *f = NULL;
	SG_uint32 written = 0;
	SG_vhash *lineParts = NULL;
	SG_string *prefix = NULL;
	SG_string *suffix = NULL;
	char **strchunks = NULL;
	SG_string *vari = NULL;
	SG_string *value = NULL;
	SG_string *contentType = NULL;
	SG_uint32	nchunks = 0;
	SG_string *buffer = NULL;
	SG_file *data = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &buffer)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, rawpathname, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, SG_FSOBJ_PERMS__UNUSED, &data)  );
	SG_ERR_CHECK(  SG_file__read_utf8_file_into_string(pCtx, data, buffer)  );
	SG_FILE_NULLCLOSE(pCtx, data);

	// get buffer from somewhere
	start = next = SG_string__sz(buffer);

	bufEnd = (SG_byte *)start + SG_string__length_in_bytes(buffer);
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &line)  );

	SG_ERR_CHECK(  nextLine(pCtx, line, start, &next, &lfOnly)  );

	len = (SG_uint32)(next - start);

	SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &separator, len + 3)  );

	if (lfOnly)
		SG_string__set__sz(pCtx, separator, "\n");
	else
		SG_string__set__sz(pCtx, separator, "\r\n");

	SG_ERR_CHECK(  SG_string__append__string(pCtx, separator, line)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &vari)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &value)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &prefix)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &suffix)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &uploadfn, "test.dat")  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &contentType, "application/octet-stream")  );

	while (! done)
		{
			start = next;
			SG_ERR_CHECK(  nextLine(pCtx, line, start, &next, &lfOnly)  );

			if (SG_string__length_in_bytes(line) == 0)
				done = SG_TRUE;
			else
				{
					SG_uint32	pos = SG_UINT32_MAX;
					SG_ERR_CHECK(  SG_ascii__find__char(pCtx, SG_string__sz(line), ':', &pos)  );

					if (pos == SG_UINT32_MAX)
						continue;

					SG_ERR_CHECK(  SG_string__set__buf_len(pCtx, prefix, (const SG_byte *)SG_string__sz(line), pos * (SG_uint32)sizeof(char))  );
					SG_ERR_CHECK(  SG_string__set__sz(pCtx, suffix, SG_string__sz(line) + pos + 1)  );

					if (strcmp("Content-Disposition", SG_string__sz(prefix))==0)
						{
							SG_uint32 i = 0;

							SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &lineParts)  );

							SG_ERR_CHECK(  SG_string__split__asciichar(pCtx, suffix, ';', 10, &strchunks, &nchunks)  );

							for ( i = 0; i < nchunks; ++i )
								{
									SG_ERR_CHECK(  _parseHeaderChunk(pCtx, strchunks[i], vari, value)  );

									if (strcmp("filename", SG_string__sz(vari))==0)
										{
											SG_ERR_CHECK(  SG_string__set__string(pCtx, uploadfn, value)  );
										}
								}

							// Content-Disposition:  form-data; name="file"; filename="testAttachFileToWorkitem.txt"
							// Content-Type: text/plain

							for ( i = 0; i < nchunks; ++i )
								{
									SG_ERR_CHECK(  SG_NULLFREE(pCtx, strchunks[i])  );
								}
							SG_NULLFREE(pCtx, strchunks);
						
							SG_VHASH_NULLFREE(pCtx, lineParts);
						}
					else if (strcmp("Content-Type", SG_string__sz(prefix))==0)
						{
							SG_ERR_CHECK(  _stringTrim(pCtx, suffix)  );
							SG_ERR_CHECK(  SG_string__set__string(pCtx, contentType, suffix)  );
						}
				}
		}

	start = next;

	if (bufEnd > (const SG_byte *)start)
		SG_ERR_CHECK(  SG_findInBuf(pCtx, (const SG_byte *)start, (SG_uint32)(bufEnd - (const SG_byte *)start),
									(const SG_byte *)SG_string__sz(separator), SG_string__length_in_bytes(separator),
									&sepPos)
					   );
	else
		SG_ERR_THROW2(SG_ERR_HTTP_400_BAD_REQUEST, (pCtx, "Error parsing mime file."));

	if (sepPos == NULL)
		sepPos = bufEnd;

	dataBytes = (SG_uint32)(sepPos - (const SG_byte *)start);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &tempDir)  );
	SG_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, tempDir, "attach")  );
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, tempDir)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &fn, tempDir, SG_string__sz(uploadfn))  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, fn, SG_FILE_WRONLY | SG_FILE_OPEN_OR_CREATE | SG_FILE_TRUNC, 0600, &f)  );

	SG_ERR_CHECK(  SG_file__write(pCtx, f, dataBytes, (const SG_byte *)start, &written)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &f)  );
		
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &filename, SG_pathname__sz(fn))  );
	*ppFilepath = filename;
	filename = NULL;
	*ppFilename = uploadfn;
	uploadfn = NULL;
	*ppMimeType = contentType;
	contentType = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, filepath);
	SG_PATHNAME_NULLFREE(pCtx, fn);
	SG_STRING_NULLFREE(pCtx, uploadfn);
	SG_STRING_NULLFREE(pCtx, contentType);
	SG_STRING_NULLFREE(pCtx, buffer);
	SG_FILE_NULLCLOSE(pCtx, data);

	if (strchunks != NULL)
	{
		SG_uint32 i;

		for ( i = 0; i < nchunks; ++i )
		{
			SG_ERR_CHECK(  SG_NULLFREE(pCtx, strchunks[i])  );
		}
		SG_NULLFREE(pCtx, strchunks);
	}

	SG_STRING_NULLFREE(pCtx, line);

	SG_STRING_NULLFREE(pCtx, contentType);
	SG_STRING_NULLFREE(pCtx, vari);
	SG_STRING_NULLFREE(pCtx, value);
	SG_VHASH_NULLFREE(pCtx, lineParts);
	SG_STRING_NULLFREE(pCtx, prefix);
	SG_STRING_NULLFREE(pCtx, suffix);
	SG_FILE_NULLCLOSE(pCtx, f);
	SG_STRING_NULLFREE(pCtx, uploadfn);
	SG_PATHNAME_NULLFREE(pCtx, tempDir);
	SG_PATHNAME_NULLFREE(pCtx, fn);
	SG_STRING_NULLFREE(pCtx, separator);
}

static void _sg_uridispatch__on_after_last_chunk_received(SG_context * pCtx, SG_uridispatchcontext * pDispatchContext)
{
	SG_vhash * pVhash = NULL;
	SG_varray * pVarray = NULL;
	SG_string *mimeType = NULL;
	SG_string *filename = NULL;
	SG_string *filepath = NULL;

	SG_ASSERT(pDispatchContext!=NULL);

	if(pDispatchContext->pIncomingJson!=NULL)
	{
		const char * pJson = NULL;
		SG_uint32 lenJson = 0;

		SG_int32 i = 0;
		double d = 0;

		jsval args[2];
		JSBool ok = SG_FALSE;
		jsval rval = JSVAL_VOID;

		SG_jscontext__resume(pDispatchContext->pJs);

		args[0] = OBJECT_TO_JSVAL(pDispatchContext->requestObject);
		args[1] = JSVAL_VOID; // args[1] is the argument for the value represented by the json

		pJson = SG_string__sz(pDispatchContext->pIncomingJson);
		lenJson = SG_string__length_in_bytes(pDispatchContext->pIncomingJson);
		while(*pJson==' ' || *pJson=='\t' || *pJson==SG_CR || *pJson==SG_LF)
		{
			++pJson;
			--lenJson;
		}

		if(lenJson==4 && memcmp(pJson, "null", 4)==0)
		{
			args[1] = JSVAL_NULL;
		}
		else if(lenJson==5 && memcmp(pJson, "false", 5)==0)
		{
			args[1] = JSVAL_FALSE;
		}
		else if(lenJson==4 && memcmp(pJson, "true", 4)==0)
		{
			args[1] = JSVAL_TRUE;
		}

		if(JSVAL_IS_VOID(args[1]))
		{
			SG_int32__parse__strict__buf_len(pCtx, &i, pJson, lenJson);
			if(!SG_context__has_err(pCtx))
				args[1] = INT_TO_JSVAL(i);
			else
				SG_context__err_reset(pCtx);
		}

		if(JSVAL_IS_VOID(args[1]))
		{
			SG_double__parse(pCtx, &d, pJson);
			if(!SG_context__has_err(pCtx))
				args[1] = DOUBLE_TO_JSVAL(d);
			else
				SG_context__err_reset(pCtx);
		}

		if(JSVAL_IS_VOID(args[1]))
		{
			SG_veither__parse_json__buflen(pCtx, pJson, lenJson, &pVhash, &pVarray);
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_JSONPARSER_SYNTAX);
			if(!SG_context__has_err(pCtx) && pVhash!=NULL)
			{
				JSObject * jso = NULL;
				SG_JS_NULL_CHECK(  jso = JS_NewObject(pDispatchContext->pJs->cx, NULL, NULL, NULL)  );
				SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, pDispatchContext->pJs->cx, pVhash, jso)  );
				SG_VHASH_NULLFREE(pCtx, pVhash);
				args[1] = OBJECT_TO_JSVAL(jso);
			}
			else if(!SG_context__has_err(pCtx) && pVarray!=NULL)
			{
				JSObject * jso = NULL;
				SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(pDispatchContext->pJs->cx, 0, NULL)  );
				SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, pDispatchContext->pJs->cx, pVarray, jso)  );
				SG_VARRAY_NULLFREE(pCtx, pVarray);
				args[1] = OBJECT_TO_JSVAL(jso);
			}
			else
				SG_context__err_reset(pCtx);
		}

		if(JSVAL_IS_VOID(args[1]))
			SG_ERR_THROW2(SG_ERR_HTTP_400_BAD_REQUEST, (pCtx, "Invalid JSON."));
		else
		{
			SG_STRING_NULLFREE(pCtx, pDispatchContext->pIncomingJson);

			ok = JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onJsonReceived", 2, args, &rval);
			pDispatchContext->requestHandlers = NULL; // Null this out so that onRequestAborted does not get called after onJsonReceived.
			SG_ERR_CHECK_CURRENT;
			if(!ok)
				SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to onJsonReceived() failed"));

			if(JSVAL_IS_VOID(rval))
				pDispatchContext->responseObject = NULL;
			else if(JSVAL_IS_NONNULL_OBJECT(rval))
				pDispatchContext->responseObject = JSVAL_TO_OBJECT(rval);
			else
				SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to onJsonReceived() returned an invalid value."));
		}

		SG_jscontext__suspend(pDispatchContext->pJs);
	}
	else if(pDispatchContext->pIncomingFile!=NULL)
	{
		JSString * strFilePath = NULL;
		JSString *strMimeType = NULL;
		JSString *strFileName = NULL;
		JSBool ok = SG_FALSE;
		jsval rval = JSVAL_VOID;
		SG_bool has = SG_FALSE;

		SG_FILE_NULLCLOSE(pCtx, pDispatchContext->pIncomingFile->pFile);

		SG_jscontext__resume(pDispatchContext->pJs);

		ok = JS_HasProperty(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onMimeFileReceived_", &has);
		has = ok && has;

		if (has)
		{
			jsval args[4];
			SG_ERR_CHECK(  _parseMimeFile(pCtx, pDispatchContext->pIncomingFile->pPathname, &filepath, &filename, &mimeType)  );
			SG_JS_NULL_CHECK(  strFilePath = JS_NewStringCopyZ(pDispatchContext->pJs->cx, SG_string__sz(filepath))  );
			SG_JS_NULL_CHECK(  strMimeType = JS_NewStringCopyZ(pDispatchContext->pJs->cx, SG_string__sz(mimeType))  );
			SG_JS_NULL_CHECK(  strFileName = JS_NewStringCopyZ(pDispatchContext->pJs->cx, SG_string__sz(filename))  );
			SG_STRING_NULLFREE(pCtx, filepath);
			SG_STRING_NULLFREE(pCtx, filename);
			SG_STRING_NULLFREE(pCtx, mimeType);

			SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Calling SSJS onMimeFileReceived().")  );
			args[0] = OBJECT_TO_JSVAL(pDispatchContext->requestObject);
			args[1] = STRING_TO_JSVAL(strFilePath);
			args[2] = STRING_TO_JSVAL(strFileName);
			args[3] = STRING_TO_JSVAL(strMimeType);
			ok = JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onMimeFileReceived", 4, args, &rval);
			pDispatchContext->requestHandlers = NULL; // Null this out so that onRequestAborted does not get called after onMimeFileReceived.
			SG_ERR_CHECK_CURRENT;
			if(!ok)
				SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to onMimeFileReceived() failed"));

			if(JSVAL_IS_VOID(rval))
				pDispatchContext->responseObject = NULL;
			else if(JSVAL_IS_NONNULL_OBJECT(rval))
				pDispatchContext->responseObject = JSVAL_TO_OBJECT(rval);
			else
				SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to onMimeFileReceived() returned an invalid value."));

			pDispatchContext->responseObject = JSVAL_TO_OBJECT(rval);
		}
		else
		{
			jsval args[2];
			SG_JS_NULL_CHECK(  strFilePath = JS_NewStringCopyZ(pDispatchContext->pJs->cx, SG_pathname__sz(pDispatchContext->pIncomingFile->pPathname))  );

			args[0] = OBJECT_TO_JSVAL(pDispatchContext->requestObject);
			args[1] = STRING_TO_JSVAL(strFilePath);
			ok = JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onFileReceived", 2, args, &rval);
			pDispatchContext->requestHandlers = NULL; // Null this out so that onRequestAborted does not get called after onFileReceived.
			SG_ERR_CHECK_CURRENT;
			if(!ok)
				SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to onFileReceived() failed"));

			if(JSVAL_IS_VOID(rval))
				pDispatchContext->responseObject = NULL;
			else if(JSVAL_IS_NONNULL_OBJECT(rval))
				pDispatchContext->responseObject = JSVAL_TO_OBJECT(rval);
			else
				SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to onFileReceived() returned an invalid value."));
		}

		SG_jscontext__suspend(pDispatchContext->pJs);
	}

	return;
fail:
	SG_jscontext__suspend(pDispatchContext->pJs);

	SG_VHASH_NULLFREE(pCtx, pVhash);
	SG_VARRAY_NULLFREE(pCtx, pVarray);
	SG_STRING_NULLFREE(pCtx, mimeType);
	SG_STRING_NULLFREE(pCtx, filename);
	SG_STRING_NULLFREE(pCtx, filepath);
}

SG_uridispatchcontext * SG_uridispatch__begin_request(
	SG_context * pCtx,
	const char * szRequestMethod,
	const char * szUri,
	const char * szQueryString,
	const char * szScheme,
	SG_vhash ** ppHeaders
	)
{
	SG_uridispatchcontext * pDispatchContext = NULL;

	SG_vhash * pRequestObject = NULL;
	SG_vhash * pHeadersRef = NULL;

	jsval args[1];
	JSBool ok;
	jsval rval;
	JSObject * jso = NULL;
	jsval statusCode;
	JSBool has = JS_FALSE;

	if(pCtx==NULL || gpUridispatchGlobalState==NULL)
		return URIDISPATCHCONTEXT__NO_pCtx_PROVIDED;

	if(SG_context__err_equals(pCtx, SG_ERR_MALLOCFAILED))
		return URIDISPATCHCONTEXT__MALLOC_FAILED;

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__C_BEGIN_REQUEST);

	if(SG_context__has_err(pCtx))
		goto fail;
	
	SG_alloc1(pCtx, pDispatchContext);

	if(SG_context__err_equals(pCtx, SG_ERR_MALLOCFAILED))
	{
		SG_httprequestprofiler__stop();
		return URIDISPATCHCONTEXT__MALLOC_FAILED;
	}

	if(SG_context__has_err(pCtx))
		goto fail;

	pDispatchContext->pCtx = pCtx;

	if(szQueryString!=NULL)
		SG_ERR_IGNORE(  SG_log__report_verbose(pCtx, "Dispatching %s %s?%s", szRequestMethod, szUri, szQueryString)  );
	else
		SG_ERR_IGNORE(  SG_log__report_verbose(pCtx, "Dispatching %s %s", szRequestMethod, szUri)  );

	if(gpUridispatchGlobalState->debugDelay>0)
		SG_sleep_ms(gpUridispatchGlobalState->debugDelay);

	SG_ERR_CHECK(  SG_jscontext__acquire(pCtx, &pDispatchContext->pJs)  );
	if(!JS_AddObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestObject))
	{
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "JS_AddRoot() failed!"));
	}
	if(!JS_AddObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestHandlers))
	{
		(void)JS_RemoveObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestObject);
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "JS_AddRoot() failed!!"));
	}
	if(!JS_AddObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->responseObject))
	{
		(void)JS_RemoveObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestObject);
		(void)JS_RemoveObjectRoot(pDispatchContext->pJs->cx, &pDispatchContext->requestHandlers);
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "JS_AddRoot() failed!!!"));
	}
	pDispatchContext->jsobjectsRooted = SG_TRUE;

	if(strcmp(szRequestMethod,"HEAD")==0)
	{
		szRequestMethod = "GET";
		pDispatchContext->requestMethodIsReallyHEAD = SG_TRUE;
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pRequestObject)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pRequestObject, "requestMethod", szRequestMethod)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pRequestObject, "uri", szUri)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pRequestObject, "queryString", ((szQueryString!=NULL)?szQueryString:""))  );
	if(szScheme!=NULL)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pRequestObject, "scheme", szScheme)  );
	if(ppHeaders!=NULL && *ppHeaders!=NULL)
	{
		pHeadersRef = *ppHeaders;
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pRequestObject, "headers", ppHeaders)  );
	}

	SG_JS_NULL_CHECK(  pDispatchContext->requestObject = JS_NewObject(pDispatchContext->pJs->cx, NULL, NULL, NULL)  );
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, pDispatchContext->pJs->cx, pRequestObject, pDispatchContext->requestObject)  );
	
	args[0] = OBJECT_TO_JSVAL(pDispatchContext->requestObject);
	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JAVASCRIPT_DISPATCH);
	ok = JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->pJs->glob, "dispatch", SG_NrElements(args), args, &rval);
	SG_httprequestprofiler__stop();
	SG_ERR_CHECK_CURRENT;
	if(!ok)
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "Call to dispatch() failed for request to %s", szUri));
	if(!JSVAL_IS_NONNULL_OBJECT(rval))
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "dispatch() returned a non-object for request to %s", szUri));

	jso = JSVAL_TO_OBJECT(rval);

	if( JS_GetProperty(pDispatchContext->pJs->cx, jso, "statusCode", &statusCode) && JSVAL_IS_STRING(statusCode) )
	{
		pDispatchContext->responseObject = jso;
	}
	else
	{
		const SG_variant * pContentLength;

		pDispatchContext->requestHandlers = jso;

		SG_vhash__get__variant(pCtx, pHeadersRef, "Content-Length", &pContentLength);
		if(!SG_context__has_err(pCtx))
		{
			if(pContentLength->type == SG_VARIANT_TYPE_SZ)
			{
				SG_uint64__parse__strict(pCtx, &pDispatchContext->requestLength, pContentLength->v.val_sz);
				if(SG_context__has_err(pCtx))
				{
					SG_context__err_reset(pCtx);
					SG_ERR_THROW2(SG_ERR_HTTP_400_BAD_REQUEST, (pCtx, "Invalid Content-Length"));
				}
			}
			else if(pContentLength->type == SG_VARIANT_TYPE_INT64)
				pDispatchContext->requestLength = pContentLength->v.val_int64;
			else
				SG_ERR_THROW2(SG_ERR_HTTP_400_BAD_REQUEST, (pCtx, "Invalid Content-Length"));
		}
		else
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_VHASH_KEYNOTFOUND);

		if( JS_HasProperty(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onJsonReceived_", &has) && has )
		{
			if(pDispatchContext->requestLength+1>SG_UINT32_MAX)
				SG_ERR_THROW(SG_ERR_HTTP_413_REQUEST_ENTITY_TOO_LARGE);
			else
				SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pDispatchContext->pIncomingJson, (SG_uint32)pDispatchContext->requestLength+1)  );
		}
		else if( JS_HasProperty(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onFileReceived_", &has) && has )
		{
			SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Receiving uploaded file.")  );
			SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pDispatchContext->pIncomingFile)  );
		}
		else if( JS_HasProperty(pDispatchContext->pJs->cx, pDispatchContext->requestHandlers, "onMimeFileReceived_", &has) && has )
		{
			SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Receiving uploaded (mime) file.")  );
			SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pDispatchContext->pIncomingFile)  );
		}
		else if(pDispatchContext->requestLength==0 && strcmp(szRequestMethod,"GET")==0)
			SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "No response generated for request to %s", szUri));
		else
			SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "No callback function provided to receive %s request to %s", szRequestMethod, szUri));
	}

	SG_VHASH_NULLFREE(pCtx, pRequestObject);

	SG_jscontext__suspend(pDispatchContext->pJs);

	SG_httprequestprofiler__stop();
	return pDispatchContext;
fail:
	SG_VHASH_NULLFREE(pCtx, pRequestObject);

	if (pDispatchContext!=NULL)
		SG_jscontext__suspend(pDispatchContext->pJs);

	_sg_uridispatch__log_pCtx_error_if_necessary_and_prepare_http_response(pCtx, pDispatchContext);

	SG_httprequestprofiler__stop();
	return pDispatchContext;
}

void SG_uridispatch__chunk_request_body(
	SG_uridispatchcontext * pDispatchContext,
	const SG_byte * pBuffer,
	SG_uint32 bufferLength
	)
{
	SG_context * pCtx = NULL; // Caller does not explicitly pass in pCtx. Extract it from the dispatch context.

	SG_ASSERT(pDispatchContext!=NULL && pDispatchContext!=URIDISPATCHCONTEXT__MALLOC_FAILED && pDispatchContext!=URIDISPATCHCONTEXT__NO_pCtx_PROVIDED);

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__C_CHUNK_REQUEST);

	pCtx = pDispatchContext->pCtx;
	if(SG_context__has_err(pCtx))
		goto fail;

	SG_NULLARGCHECK(pBuffer);
	SG_ARGCHECK(bufferLength>0, bufferLength);

	if((SG_uint64)bufferLength + pDispatchContext->requestLength_chunked > pDispatchContext->requestLength)
	{
		SG_ERR_IGNORE(  SG_log__report_info(pCtx, "Caller gave us too many bytes for the request body.")  );
		bufferLength = (SG_uint32)(pDispatchContext->requestLength-pDispatchContext->requestLength_chunked);
	}

	SG_ASSERT(pDispatchContext->pJs!=NULL);
	SG_jscontext__resume(pDispatchContext->pJs);

	if(pDispatchContext->pIncomingJson!=NULL)
	{
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Handling incoming JSON.")  );

		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pDispatchContext->pIncomingJson, pBuffer, bufferLength)  );
		pDispatchContext->requestLength_chunked += bufferLength;
	}
	else if(pDispatchContext->pIncomingFile!=NULL)
	{
		SG_ERR_CHECK(  SG_file__write(pCtx, pDispatchContext->pIncomingFile->pFile, bufferLength, pBuffer, NULL)  );
		pDispatchContext->requestLength_chunked += bufferLength;
	}
	else
		SG_ERR_THROW(SG_ERR_ASSERT); // Should not happen

	SG_jscontext__suspend(pDispatchContext->pJs);

	SG_httprequestprofiler__stop();
	return;
fail:
	SG_jscontext__suspend(pDispatchContext->pJs);

	_sg_uridispatch__log_pCtx_error_if_necessary_and_prepare_http_response(pCtx, pDispatchContext);

	SG_httprequestprofiler__stop();
	return;
}

SG_bool SG_uridispatch__get_response_headers(SG_uridispatchcontext * pDispatchContext, const char ** ppStatusCode, SG_uint64 * pContentLength, SG_vhash ** ppResponseHeaders)
{
	SG_context * pCtx = NULL; // Caller does not explicitly pass in pCtx. Extract it from the dispatch context.

	SG_ASSERT(pDispatchContext!=NULL);
	SG_ASSERT(ppStatusCode!=NULL);
	SG_ASSERT(pContentLength!=NULL);
	SG_ASSERT(ppResponseHeaders!=NULL);

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__C_GET_RESPONSE_HEADERS);

	if(pDispatchContext==URIDISPATCHCONTEXT__MALLOC_FAILED)
	{
		*ppStatusCode = "500 Internal Server Error";
		*pContentLength = SG_STRLEN(SZ_MALLOC_FAILED);
		*ppResponseHeaders = NULL;
		SG_httprequestprofiler__stop();
		return SG_TRUE;
	}
	if(pDispatchContext==URIDISPATCHCONTEXT__NO_pCtx_PROVIDED)
	{
		*ppStatusCode = "500 Internal Server Error";
		*pContentLength = SG_STRLEN(SZ_NO_pCtx_PROVIDED);
		*ppResponseHeaders = NULL;
		SG_httprequestprofiler__stop();
		return SG_TRUE;
	}

	pCtx = pDispatchContext->pCtx;

	if(SG_context__has_err(pCtx))
	{
		// Probably shouldn't happen. This would mean the caller of SG_uridispatch dirtied our pCtx underneath us.
		_sg_uridispatch__log_pCtx_error_if_necessary_and_prepare_http_response(pCtx, pDispatchContext);
	}
	else if(pDispatchContext->requestLength_chunked>=pDispatchContext->requestLength)
	{
		_sg_uridispatch__on_after_last_chunk_received(pCtx, pDispatchContext);
		if(SG_context__has_err(pCtx))
			_sg_uridispatch__log_pCtx_error_if_necessary_and_prepare_http_response(pCtx, pDispatchContext);
	}

	SG_ASSERT(pCtx!=NULL);

	if(pDispatchContext->responseObject!=NULL)
	{
		jsval rval;
		jsval args[1];
		SG_jscontext__resume(pDispatchContext->pJs);
		args[0] = OBJECT_TO_JSVAL(pDispatchContext->responseObject);
		(void)JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->pJs->glob, "onBeforeSendResponse", SG_NrElements(args), args, &rval);
		SG_context__err_reset(pCtx);
		SG_jscontext__suspend(pDispatchContext->pJs);

		// We have a responseObject from SSJS. Extract the headers information.
		_sg_uridispatch__get_response_headers_from_SSJS_responseObject(pCtx, pDispatchContext, ppStatusCode, pContentLength, ppResponseHeaders);

		// If the previous call succeeded, we are done. Headers have been returned to the caller.
		if(!SG_context__has_err(pCtx))
		{
			SG_httprequestprofiler__stop();
			return SG_TRUE;
		}

		// Othewise... Failed. Discard that response, and construct a new one with the error.
		_sg_uridispatch__log_pCtx_error_if_necessary_and_prepare_http_response(pCtx, pDispatchContext);

		// Then get the headers.
		_sg_uridispatch__get_response_headers_from_SSJS_responseObject(pCtx, pDispatchContext, ppStatusCode, pContentLength, ppResponseHeaders);
		if(!SG_context__has_err(pCtx))
		{
			SG_httprequestprofiler__stop();
			return SG_TRUE;
		}

		// And if it didn't succeed this time, fall back to a plaintext response handled entirely by plain old C code.
		_sg_uridispatch__log_pCtx_error_and_fetch_response_text_into_szErrorMessage(pCtx, pDispatchContext);
		SG_context__err_reset(pCtx);
	}

	if(pDispatchContext->szErrorStatusCode!=NULL || pDispatchContext->szErrorMessage!=NULL)
	{
		if(pDispatchContext->szErrorStatusCode==NULL)
			pDispatchContext->szErrorStatusCode = "500 Internal Server Error";
		if(pDispatchContext->szErrorMessage==NULL)
			pDispatchContext->szErrorMessage = pDispatchContext->szErrorStatusCode;
		
		pDispatchContext->responseLength = SG_STRLEN(pDispatchContext->szErrorMessage);

		*ppStatusCode = pDispatchContext->szErrorStatusCode;
		*pContentLength = pDispatchContext->responseLength;
		*ppResponseHeaders = NULL;
		SG_httprequestprofiler__stop();
		return SG_TRUE;
	}
	else if(pDispatchContext->requestLength_chunked>=pDispatchContext->requestLength)
	{
		*ppStatusCode = "200 OK";
		*pContentLength = pDispatchContext->responseLength = 0;
		*ppResponseHeaders = NULL;
		SG_httprequestprofiler__stop();
		return SG_TRUE;
	}
	else
	{
		SG_httprequestprofiler__stop();
		return SG_FALSE;
	}
}

void SG_uridispatch__chunk_response_body(SG_uridispatchcontext ** ppDispatchContext, const SG_byte ** ppBuffer, SG_uint32 * pBufferLength)
{
	SG_context * pCtx = NULL;
	SG_uridispatchcontext * pDispatchContext = NULL;

	JSBool ok;
	jsval rval;
	SG_safeptr* psp = NULL;

	SG_ASSERT(ppDispatchContext!=NULL && *ppDispatchContext!=NULL);
	pDispatchContext = *ppDispatchContext;

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__C_CHUNK_RESPONSE);

	if(pDispatchContext==URIDISPATCHCONTEXT__MALLOC_FAILED)
	{
		if(ppBuffer!=NULL && pBufferLength!=NULL)
		{
			*ppBuffer = (const SG_byte*)SZ_MALLOC_FAILED;
			*pBufferLength = SG_STRLEN(SZ_MALLOC_FAILED);
		}
		*ppDispatchContext = NULL;
		SG_httprequestprofiler__stop();
		return;
	}
	if(pDispatchContext==URIDISPATCHCONTEXT__NO_pCtx_PROVIDED)
	{
		if(ppBuffer!=NULL && pBufferLength!=NULL)
		{
			*ppBuffer = (const SG_byte*)SZ_NO_pCtx_PROVIDED;
			*pBufferLength = SG_STRLEN(SZ_NO_pCtx_PROVIDED);
		}
		*ppDispatchContext = NULL;
		SG_httprequestprofiler__stop();
		return;
	}

	pCtx = pDispatchContext->pCtx;
	SG_ASSERT(pCtx!=NULL && !SG_context__has_err(pCtx));

	SG_NULLARGCHECK(ppBuffer);
	SG_NULLARGCHECK(pBufferLength);

	if(pDispatchContext->requestMethodIsReallyHEAD || pDispatchContext->responseLength_chunked>=pDispatchContext->responseLength)
	{
		if(pDispatchContext->responseLength_chunked>pDispatchContext->responseLength)
		{
			SG_int_to_string_buffer chunked;
			SG_int_to_string_buffer total;
			SG_ERR_IGNORE(  SG_log__report_error(pCtx, "Response sent more data than specified in the Content-Length header (%s bytes rather than %s).",
				SG_uint64_to_sz(pDispatchContext->responseLength_chunked, chunked),
				SG_uint64_to_sz(pDispatchContext->responseLength, total))  );
		}

		if(pDispatchContext->responseObject != NULL)
		{
			SG_jscontext__resume(pDispatchContext->pJs);

			if(pDispatchContext->responseLength_chunked>=pDispatchContext->responseLength)
			{
				jsval n = JSVAL_NULL; // In dispatch.js we claim that finalize() will only be called if onFinish() isn't.
				(void)JS_SetProperty(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "finalize", &n);
				
				(void)JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "onFinish", 0, NULL, &rval);
				SG_context__err_reset(pCtx);
				pDispatchContext->responseObject = NULL;
			}
		}

		*ppBuffer = NULL;
		*pBufferLength = 0;
		_SG_uridispatchcontext__nullfree(ppDispatchContext);
		SG_httprequestprofiler__stop();
		return;
	}

	if(pDispatchContext->szErrorMessage!=NULL)
	{
		*ppBuffer = (const SG_byte*)pDispatchContext->szErrorMessage;
		*pBufferLength = (SG_uint32)pDispatchContext->responseLength;

		pDispatchContext->responseLength_chunked += *pBufferLength;
		SG_httprequestprofiler__stop();
		return;
	}

	SG_ASSERT(pDispatchContext->pJs!=NULL);
	SG_jscontext__resume(pDispatchContext->pJs);

	ok = JS_CallFunctionName(pDispatchContext->pJs->cx, pDispatchContext->responseObject, "onChunk", 0, NULL, &rval);
	SG_ERR_CHECK_CURRENT;
	if(!ok)
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "onChunk() callback failed"));

	if(!JSVAL_IS_NONNULL_OBJECT(rval))
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "onChunk() callback returned something that wasn't a JSObject"));

	SG_cbuffer__nullfree(&pDispatchContext->pOutgoingChunk); // Don't leak the previous buffer :)

	psp = (SG_safeptr*)JS_GetPrivate(pDispatchContext->pJs->cx, JSVAL_TO_OBJECT(rval));
	SG_ERR_CHECK(  SG_safeptr__unwrap__cbuffer(pCtx, psp, &pDispatchContext->pOutgoingChunk)  );
	SG_SAFEPTR_NULLFREE(pCtx, psp);

	*ppBuffer = pDispatchContext->pOutgoingChunk->pBuf;
	*pBufferLength = pDispatchContext->pOutgoingChunk->len;

	pDispatchContext->responseLength_chunked += *pBufferLength;

	SG_jscontext__suspend(pDispatchContext->pJs);

	SG_httprequestprofiler__stop();
	return;
fail:
	// By now there is pretty much nothing we can do to send the error information
	// to the client, since the response headers have already been sent. Just log
	// the error to the server's log file and silently fail to send the rest of the
	// response.

	SG_log__report_error__current_error(pCtx);

	SG_SAFEPTR_NULLFREE(pCtx, psp);

	if(ppBuffer!=NULL)
		*ppBuffer = NULL;
	if(pBufferLength!=NULL)
		*pBufferLength = 0;

	_SG_uridispatchcontext__nullfree(ppDispatchContext);
	SG_httprequestprofiler__stop();
	return;
}

void SG_uridispatch__abort(SG_uridispatchcontext ** ppDispatchContext)
{
	_SG_uridispatchcontext__nullfree(ppDispatchContext);
}

///////////////////////////////////////////////////////////////////////////////

void SG_uridispatch__global_cleanup(SG_context * pCtx)
{
	if(gpUridispatchGlobalState!=NULL)
	{
		SG_jscontextpool__teardown(pCtx);
		SG_ERR_IGNORE(  _unregister_server_log_handler(pCtx, &gpUridispatchGlobalState->cLogData, &gpUridispatchGlobalState->cLogFileWriterData)  );

		SG_VHASH_NULLFREE(pCtx, gpUridispatchGlobalState->pInclude);
		SG_VHASH_NULLFREE(pCtx, gpUridispatchGlobalState->pExclude);

		SG_STRING_NULLFREE(pCtx, gpUridispatchGlobalState->pApplicationRoot);
		SG_NULLFREE(pCtx, gpUridispatchGlobalState);
	}
}
