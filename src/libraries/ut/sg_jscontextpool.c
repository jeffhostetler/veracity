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

///////////////////////////////////////////////////////////////////////////////


static struct
{
	SG_vhash * pServerConfig;

	// Flag to create a new cx on every acquire() rather than keeping a pool of them.
	SG_bool ssjsMutable;

	// Members above this comment are not controlled by the mutex.
	// All others are.

	SG_mutex lock;

	// Please take care that all members below this comment be
	// protected by the mutex.

	SG_uint32 numContextsCheckedOut; // Number of SG_jscontexts currently in use by SG_uridispatch.
	SG_jscontext * pFirstAvailableContext; // Pointer to the first SG_jscontext NOT in use by SG_uridispatch.
} * gpJSContextPoolGlobalState = NULL;


static void _sg_jscontextpool__force_config_bool(SG_context * pCtx, SG_vhash * pConfig, const char * szSettingName, SG_bool * pValue)
{
	SG_bool has = SG_FALSE;
	SG_bool value = SG_FALSE;
	
	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(pConfig!=NULL);
	SG_ASSERT(szSettingName!=NULL);
	
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pConfig, szSettingName, &has)  );
	if(has)
	{
		SG_uint16 type_of_value = 0;
		SG_ERR_CHECK(  SG_vhash__typeof(pCtx, pConfig, szSettingName, &type_of_value)  );
		if(type_of_value==SG_VARIANT_TYPE_SZ)
		{
			const char *szValue = NULL;
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pConfig, szSettingName, &szValue)  );
			value = strcmp(szValue, "true")==0;
		}
		else if(type_of_value==SG_VARIANT_TYPE_BOOL)
		{
			SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pConfig, szSettingName, &value)  );
		}
	}
	SG_ERR_CHECK(  SG_vhash__update__bool(pCtx, pConfig, szSettingName, value)  );
	if(pValue!=NULL)
	{
		*pValue = value;
	}
	
	return;
fail:
	;
}


void SG_jscontextpool__init(SG_context * pCtx, const char * szApplicationRoot)
{
	if(gpJSContextPoolGlobalState != NULL)
		return;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, gpJSContextPoolGlobalState)  );

	SG_localsettings__get__collapsed_vhash(pCtx, "server", NULL, &gpJSContextPoolGlobalState->pServerConfig);
	if(SG_context__has_err(pCtx))
	{
		SG_log__report_error__current_error(pCtx);
		SG_context__err_reset(pCtx);
	}
	if(gpJSContextPoolGlobalState->pServerConfig==NULL)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &gpJSContextPoolGlobalState->pServerConfig)  );
	}

	SG_ERR_CHECK(  _sg_jscontextpool__force_config_bool(pCtx, gpJSContextPoolGlobalState->pServerConfig, "enable_diagnostics", NULL)  );
	SG_ERR_CHECK(  _sg_jscontextpool__force_config_bool(pCtx, gpJSContextPoolGlobalState->pServerConfig, "readonly", NULL)  );
	SG_ERR_CHECK(  _sg_jscontextpool__force_config_bool(pCtx, gpJSContextPoolGlobalState->pServerConfig, "remote_ajax_libs", NULL)  );
	SG_ERR_CHECK(  _sg_jscontextpool__force_config_bool(pCtx, gpJSContextPoolGlobalState->pServerConfig, "ssjs_mutable", &gpJSContextPoolGlobalState->ssjsMutable)  );

	if(szApplicationRoot==NULL)
		szApplicationRoot="";
	SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, gpJSContextPoolGlobalState->pServerConfig, "application_root", szApplicationRoot)  );

	// Start up SpiderMonkey.
	SG_jscore__new_runtime(pCtx, SG_jsglue__context_callback, NULL, SG_FALSE, NULL);

	//If jscore is already initialized, just move on.
	if (SG_context__err_equals(pCtx, SG_ERR_ALREADY_INITIALIZED))
	{
		SG_context__err_reset(pCtx);
	}

	SG_ERR_CHECK(  SG_mutex__init(pCtx, &gpJSContextPoolGlobalState->lock)  );

	return;
fail:
	SG_VHASH_NULLFREE(pCtx, gpJSContextPoolGlobalState->pServerConfig);
	SG_NULLFREE(pCtx, gpJSContextPoolGlobalState);
}


///////////////////////////////////////////////////////////////////////////////

static void _sg_jscontext__create(SG_context * pCtx, SG_jscontext ** ppJs)
{
	SG_jscontext * pJs = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppJs);

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSCONTEXT_CREATION);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pJs)  );

	SG_ERR_CHECK(  SG_jscore__new_context(pCtx, &pJs->cx, &pJs->glob, gpJSContextPoolGlobalState->pServerConfig)  );
	pJs->isInARequest = SG_TRUE;

	*ppJs = pJs;

	SG_httprequestprofiler__stop();

	return;
fail:
	SG_NULLFREE(pCtx, pJs);

	SG_httprequestprofiler__stop();
}


///////////////////////////////////////////////////////////////////////////////

void SG_jscontext__acquire(SG_context * pCtx, SG_jscontext ** ppJs)
{
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppJs);

	if(gpJSContextPoolGlobalState->ssjsMutable)
	{
		_sg_jscontext__create(pCtx, ppJs);
		return;
	}

	SG_ERR_CHECK_RETURN(  SG_mutex__lock(pCtx, &gpJSContextPoolGlobalState->lock)  );

	if(gpJSContextPoolGlobalState->pFirstAvailableContext!=NULL)
	{
		SG_jscontext * pJs = gpJSContextPoolGlobalState->pFirstAvailableContext;

		gpJSContextPoolGlobalState->pFirstAvailableContext = pJs->pNextAvailableContext;
		pJs->pNextAvailableContext = NULL;

		++gpJSContextPoolGlobalState->numContextsCheckedOut;

		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &gpJSContextPoolGlobalState->lock)  );

		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING);
		(void)JS_SetContextThread(pJs->cx);
		JS_BeginRequest(pJs->cx);
		pJs->isInARequest = SG_TRUE;
		SG_httprequestprofiler__stop();

		JS_SetContextPrivate(pJs->cx, pCtx);

		*ppJs = pJs;
	}
	else
	{
		++gpJSContextPoolGlobalState->numContextsCheckedOut;
		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &gpJSContextPoolGlobalState->lock)  );

		_sg_jscontext__create(pCtx, ppJs);

		if(SG_context__has_err(pCtx) || *ppJs==NULL)
		{
			/* Use the version of the mutex routines that doesn't touch pCtx,
			because we're already in an error state. */
			SG_mutex__lock__bare(&gpJSContextPoolGlobalState->lock);
			--gpJSContextPoolGlobalState->numContextsCheckedOut;
			SG_mutex__unlock__bare(&gpJSContextPoolGlobalState->lock);
		}
	}
}

void SG_jscontext__release(SG_context * pCtx, SG_jscontext ** ppJs)
{
	if(ppJs==NULL || *ppJs==NULL)
		return;

	if(gpJSContextPoolGlobalState->ssjsMutable)
	{
		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING);
		JS_EndRequest((*ppJs)->cx);
		JS_DestroyContext((*ppJs)->cx);
		SG_httprequestprofiler__stop();

		if(SG_context__has_err(pCtx)) // An error was produced during GC...
		{
			SG_log__report_error__current_error(pCtx);
			SG_context__err_reset(pCtx);
		}

		SG_NULLFREE(pCtx, (*ppJs));
	}
	else
	{
		SG_jscontext * pJs = *ppJs;

		JS_MaybeGC(pJs->cx);

		JS_SetContextPrivate(pJs->cx, NULL); // Clear out the old pCtx pointer.

		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING);
		JS_EndRequest(pJs->cx);
		pJs->isInARequest = SG_FALSE;
		(void)JS_ClearContextThread(pJs->cx);
		SG_httprequestprofiler__stop();

		SG_ERR_CHECK_RETURN(  SG_mutex__lock(pCtx, &gpJSContextPoolGlobalState->lock)  );

		pJs->pNextAvailableContext = gpJSContextPoolGlobalState->pFirstAvailableContext;
		gpJSContextPoolGlobalState->pFirstAvailableContext = pJs;
		--gpJSContextPoolGlobalState->numContextsCheckedOut;

		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &gpJSContextPoolGlobalState->lock)  );

		*ppJs = NULL;
	}
}

void SG_jscontext__suspend(SG_jscontext * pJs)
{
	if(pJs!=NULL && pJs->isInARequest)
	{
		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING);
		JS_EndRequest(pJs->cx);
		pJs->isInARequest = SG_FALSE;
		SG_httprequestprofiler__stop();
	}
}

void SG_jscontext__resume(SG_jscontext * pJs)
{
	if(pJs!=NULL && !pJs->isInARequest)
	{
		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING);
		JS_BeginRequest(pJs->cx);
		pJs->isInARequest = SG_TRUE;
		SG_httprequestprofiler__stop();
	}
}


///////////////////////////////////////////////////////////////////////////////

void SG_jscontextpool__teardown(SG_context * pCtx)
{
	if(gpJSContextPoolGlobalState!=NULL)
	{
		SG_ERR_CHECK_RETURN(  SG_mutex__lock(pCtx, &gpJSContextPoolGlobalState->lock)  );

		// Wait until all outstanding SG_jscontexts have been released. Don't try to terminate
		// early, otherwise we have a race condition on our hands: If the outstanding SG_jscontext
		// tries to perform any JavaScript operations before the app terminates, but after we have
		// called JS_Shutdown(), we'll end up crashing on exit. This of course is worse than
		//  making the user hit Ctrl-C again to do a hard shutdown if it's taking too long.
		if(gpJSContextPoolGlobalState->numContextsCheckedOut > 0)
		{
			SG_ERR_IGNORE(  SG_log__report_warning(pCtx, "Waiting on %d SG_jscontexts that are still in use.", gpJSContextPoolGlobalState->numContextsCheckedOut)  );
			while(gpJSContextPoolGlobalState->numContextsCheckedOut > 0)
			{
				SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &gpJSContextPoolGlobalState->lock)  );
				SG_sleep_ms(10);
				SG_ERR_CHECK_RETURN(  SG_mutex__lock(pCtx, &gpJSContextPoolGlobalState->lock)  );
			}
		}

		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &gpJSContextPoolGlobalState->lock)  );
		SG_mutex__destroy(&gpJSContextPoolGlobalState->lock);

		while(gpJSContextPoolGlobalState->pFirstAvailableContext!=NULL)
		{
			SG_jscontext * pJs = gpJSContextPoolGlobalState->pFirstAvailableContext;
			gpJSContextPoolGlobalState->pFirstAvailableContext = pJs->pNextAvailableContext;

			(void)JS_SetContextThread(pJs->cx);
			JS_BeginRequest(pJs->cx);
			JS_SetContextPrivate(pJs->cx, pCtx);
			JS_DestroyContextNoGC(pJs->cx);

			if(SG_context__has_err(pCtx)) // An error was produced during GC...
			{
				SG_log__report_error__current_error(pCtx);
				SG_context__err_reset(pCtx);
			}

			SG_NULLFREE(pCtx, pJs);
		}

		SG_VHASH_NULLFREE(pCtx, gpJSContextPoolGlobalState->pServerConfig);
		SG_NULLFREE(pCtx, gpJSContextPoolGlobalState);
	}
}


///////////////////////////////////////////////////////////////////////////////
