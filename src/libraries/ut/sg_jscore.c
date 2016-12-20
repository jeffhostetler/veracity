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
	SG_pathname * pPathToDispatchDotJS;
	SG_pathname * pPathToCore;
	SG_pathname * pPathToModules;
	SG_bool bSkipModules;

	SG_rbtree* prbJSMutexes; // contains _namedMutexes to be managed from JS
	SG_mutex mutexJsNamed; // serializes access to the not-thread-safe rbtree containing the named mutexes managed from JS

	JSRuntime * rt;
	JSContextCallback cb;
	JSFunctionSpec *shell_functions;

} * gpJSCoreGlobalState = NULL;

typedef struct  
{
	SG_mutex mutex;
	SG_uint32 count; // track the number of threads waiting on the named mutex. when 0, it can be removed from the rbtree.
} _namedMutex;

static void _sg_jscore_getpaths(SG_context *pCtx)
{
	char * szServerFiles = NULL;

	if (gpJSCoreGlobalState->pPathToDispatchDotJS)
		return;

	// Figure out and store relavant paths...
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_FILES, NULL, &szServerFiles, NULL)  );

	if (szServerFiles)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &gpJSCoreGlobalState->pPathToDispatchDotJS, szServerFiles)  );
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, gpJSCoreGlobalState->pPathToDispatchDotJS, "dispatch.js")  );

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &gpJSCoreGlobalState->pPathToCore, szServerFiles)  );
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, gpJSCoreGlobalState->pPathToCore, "core")  );

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &gpJSCoreGlobalState->pPathToModules, szServerFiles)  );
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, gpJSCoreGlobalState->pPathToModules, "modules")  );
	}

fail:
	SG_NULLFREE(pCtx, szServerFiles);
}

void SG_jscore__new_runtime(
	SG_context * pCtx,
	JSContextCallback cb,
	JSFunctionSpec *shell_functions,
	SG_bool bSkipModules,
	JSRuntime **ppRt
	)
{
	char * szSsjsMutable = NULL;

	if(gpJSCoreGlobalState != NULL)
		SG_ERR_THROW_RETURN(SG_ERR_ALREADY_INITIALIZED);

	SG_ERR_CHECK(  SG_alloc1(pCtx, gpJSCoreGlobalState)  );

	// Store this for later.
	gpJSCoreGlobalState->cb = cb;
	gpJSCoreGlobalState->shell_functions = shell_functions;
	gpJSCoreGlobalState->bSkipModules = bSkipModules;

	if (! bSkipModules)
		SG_ERR_CHECK(  _sg_jscore_getpaths(pCtx)  );

	SG_ERR_CHECK(  SG_mutex__init(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &gpJSCoreGlobalState->prbJSMutexes)  );

	// Start up SpiderMonkey.
	JS_SetCStringsAreUTF8();
	gpJSCoreGlobalState->rt = JS_NewRuntime(64L * 1024L * 1024L); // TODO decide the right size here
	if(gpJSCoreGlobalState->rt==NULL)
		SG_ERR_THROW2(SG_ERR_JS, (pCtx, "Failed to allocate JS Runtime"));

	if (ppRt)
		*ppRt = gpJSCoreGlobalState->rt;

	return;
fail:
	SG_NULLFREE(pCtx, szSsjsMutable);

	SG_PATHNAME_NULLFREE(pCtx, gpJSCoreGlobalState->pPathToDispatchDotJS);
	SG_PATHNAME_NULLFREE(pCtx, gpJSCoreGlobalState->pPathToModules);
}


///////////////////////////////////////////////////////////////////////////////

static JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub,  JS_PropertyStub,
	JS_PropertyStub,  JS_StrictPropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub,   JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static void sg_read_entire_file(
	SG_context* pCtx,
	const SG_pathname* pPath,
	char** ppbuf,
	SG_uint32* plen
	)
{
	SG_uint32 len32;
	SG_fsobj_type t;
	SG_byte* p = NULL;
	SG_bool bExists;
	SG_fsobj_type FsObjType;
	SG_fsobj_perms FsObjPerms;
	SG_file* pFile = NULL;

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, &FsObjType, &FsObjPerms)  );

	if (
		bExists
		&& (SG_FSOBJ_TYPE__REGULAR != FsObjType)
		)
	{
		SG_ERR_IGNORE(  SG_log__report_error(pCtx, "Unable to open file: %s.", SG_pathname__sz(pPath))  );
		
		SG_ERR_THROW_RETURN(SG_ERR_NOTAFILE);
	}

	if (bExists)
	{
		SG_uint64 len64;
		SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len64, &t)  );

		// TODO "len" is uint64 because we can have huge files, but
		// TODO our buffer is limited to uint32 (on 32bit systems).
		// TODO verify that len will fit in uint32.
		len32 = (SG_uint32)len64;

		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
	}
	else
	{
		SG_ERR_THROW_RETURN(SG_ERR_NOTAFILE);
		//len32 = 0;
	}

	if (len32 > 0)
	{
		SG_ERR_CHECK(  SG_alloc(pCtx, 1,len32+1,&p)  );
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile, len32, p, NULL)  );

		p[len32] = 0;

		*ppbuf = (char*) p;
		p = NULL;
		*plen = len32;
	}
	else
	{
		*ppbuf = NULL;
		*plen = 0;
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, p);
}

///////////////////////////////////////////////////////////////////////////////

static void _sg_jscore__install_modules(SG_context * pCtx, JSContext *cx, JSObject *glob,
	const SG_vhash * pServerConfig);

void SG_jscore__new_context(SG_context * pCtx, JSContext ** pp_cx, JSObject ** pp_glob,
	const SG_vhash * pServerConfig)
{
	JSContext * cx = NULL;
	JSObject * glob = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pp_cx);

	if(gpJSCoreGlobalState==NULL)
		SG_ERR_THROW2_RETURN(SG_ERR_UNINITIALIZED, (pCtx, "jscore has not been initialized"));

	if (gpJSCoreGlobalState->cb)
		JS_SetContextCallback(gpJSCoreGlobalState->rt, gpJSCoreGlobalState->cb);

	cx = JS_NewContext(gpJSCoreGlobalState->rt, 8192);
	if(cx==NULL)
		SG_ERR_THROW2_RETURN(SG_ERR_MALLOCFAILED, (pCtx, "Failed to allocate new JS context"));

	(void)JS_SetContextThread(cx);
	JS_BeginRequest(cx);

	JS_SetOptions(cx, JSOPTION_VAROBJFIX);
	JS_SetVersion(cx, JSVERSION_LATEST);
	JS_SetContextPrivate(cx, pCtx);

	glob = JS_NewCompartmentAndGlobalObject(cx, &global_class, NULL);
	if(glob==NULL)
		SG_ERR_THROW2(SG_ERR_JS, (pCtx, "Failed to create JavaScript global object for new JSContext."));
	if(!JS_InitStandardClasses(cx, glob))
		SG_ERR_THROW2(SG_ERR_JS, (pCtx, "JS_InitStandardClasses() failed."));
	if (gpJSCoreGlobalState->shell_functions)
		if (!JS_DefineFunctions(cx, glob, gpJSCoreGlobalState->shell_functions))
			SG_ERR_THROW2(SG_ERR_JS, (pCtx, "Failed to install shell functions"));

    SG_jsglue__set_sg_context(pCtx, cx);

	SG_ERR_CHECK(  SG_jsglue__install_scripting_api(pCtx, cx, glob)  );
	SG_ERR_CHECK(  SG_zing_jsglue__install_scripting_api(pCtx, cx, glob)  );

	if (! gpJSCoreGlobalState->bSkipModules)
	{
		_sg_jscore__install_modules(pCtx, cx, glob, pServerConfig);
		SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTAFILE);
	}

	*pp_cx = cx;
	*pp_glob = glob;

	return;
fail:
	if (cx)
	{
		JS_EndRequest(cx);
		JS_DestroyContext(cx);
	}
}

///////////////////////////////////////////////////////////////////////////////

static void _modulesInstalled(SG_context *pCtx, JSContext *cx, JSObject *glob, SG_bool *installed)
{
	jsval jv = JSVAL_VOID;

	SG_UNUSED(pCtx);

	*installed = SG_FALSE;

	if (JS_GetProperty(cx, glob, "vvModulesInstalled", &jv) && JSVAL_IS_BOOLEAN(jv))
	{
		*installed = JSVAL_TO_BOOLEAN(jv);
	}
}

static void _setModulesInstalled(SG_context *pCtx, JSContext *cx, JSObject *glob)
{
	jsval jv = BOOLEAN_TO_JSVAL(1);

	if (! JS_SetProperty(cx, glob, "vvModulesInstalled", &jv))
	{
		SG_ERR_THROW(SG_ERR_JS);
	}

fail:
	;
}

static void _isValidJsFile(SG_context *pCtx, const char *filename, SG_bool *valid) 
{
	SG_NULLARGCHECK_RETURN(filename);

	*valid = (filename[0] >= 'a' && filename[0] <= 'z') || (filename[0] >= 'A' && filename[0] <= 'Z');
}   

static void _loadModuleDir(SG_context *pCtx, const SG_pathname *path, const char *modname, JSContext *cx, JSObject *glob)
{
	SG_rbtree * pJavascriptFiles = NULL;
	SG_rbtree_iterator * pJavascriptFile = NULL;
	const char *szJavascriptFile = NULL; 
	SG_pathname *pJavascriptFilePath = NULL;
	SG_bool ok = SG_FALSE;
	SG_bool valid = SG_FALSE;
	char *psz_js = NULL; // free
	SG_uint32 len_js = 0;
	jsval rval = JSVAL_VOID;
	
	SG_ERR_CHECK(  SG_dir__list(pCtx, path, NULL, NULL, ".js", &pJavascriptFiles)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pJavascriptFile, pJavascriptFiles, &ok, &szJavascriptFile, NULL)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pJavascriptFilePath, path)  );

	while(ok)
	{
		SG_ERR_CHECK(  _isValidJsFile(pCtx, szJavascriptFile, &valid)  );

		if (valid)
		{
			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pJavascriptFilePath, szJavascriptFile)  );

			SG_ERR_CHECK(  sg_read_entire_file(pCtx, pJavascriptFilePath, &psz_js, &len_js)  );
			if(!JS_EvaluateScript(cx, glob, psz_js, len_js, szJavascriptFile, 1, &rval))
			{
				SG_ERR_CHECK_CURRENT;
				SG_ERR_THROW2(SG_ERR_JS, (pCtx, "An error occurred loading %s javascript file '%s'", modname, szJavascriptFile));
			}
			SG_NULLFREE(pCtx, psz_js);
			SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pJavascriptFilePath)  );
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pJavascriptFile, &ok, &szJavascriptFile, NULL)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pJavascriptFilePath);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pJavascriptFile);
	SG_RBTREE_NULLFREE(pCtx, pJavascriptFiles);
	SG_NULLFREE(pCtx, psz_js);
}

static void _sg_jscore__install_modules(SG_context * pCtx, JSContext *cx, JSObject *glob,
	const SG_vhash *pServerConfig)
{
	SG_pathname *pModuleDirPath = NULL;
	SG_rbtree_iterator *pModuleDir = NULL;
	SG_rbtree *pModules = NULL;
	const char *szModuleDir = NULL;
	SG_bool ok = SG_FALSE;
	SG_uint32 len_js = 0;
	jsval rval;
	char *psz_js = NULL;
	jsval fo = JSVAL_VOID;
	JSObject * jsoServerConfig = NULL;

	if (gpJSCoreGlobalState->bSkipModules)
		return;

	SG_ERR_CHECK(  _modulesInstalled(pCtx, cx, glob, &ok)  );

	if (ok)
		return;

	if (! gpJSCoreGlobalState->pPathToDispatchDotJS)
		return;
	
	SG_ERR_CHECK(  _setModulesInstalled(pCtx, cx, glob)  );

	SG_ERR_CHECK(  sg_read_entire_file(pCtx, gpJSCoreGlobalState->pPathToDispatchDotJS, &psz_js, &len_js)  );
	if(!JS_EvaluateScript(cx, glob, psz_js, len_js, "dispatch.js", 1, &rval))
	{
		SG_ERR_CHECK_CURRENT;
		SG_ERR_THROW2(SG_ERR_JS, (pCtx, "An error occurred evaluating dispatch.js!"));
	}
	SG_NULLFREE(pCtx, psz_js);

	// Call init function in dispatch.js
	if(pServerConfig)
	{
		jsval arg;
		JSBool js_ok;
		jsval rval2;

		SG_JS_NULL_CHECK(  jsoServerConfig = JS_NewObject(cx, NULL, NULL, NULL)  );
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pServerConfig, jsoServerConfig)  );
		arg = OBJECT_TO_JSVAL(jsoServerConfig);
		js_ok = JS_CallFunctionName(cx, glob, "init", 1, &arg, &rval2);
		SG_ERR_CHECK_CURRENT;
		if(!js_ok)
			SG_ERR_THROW2(SG_ERR_JS, (pCtx, "An error occurred initializing JavaScript framework: call to JavaScript init() failed"));
		
		jsoServerConfig = NULL;
	}

	// Load core.
	SG_ERR_CHECK(  _loadModuleDir(pCtx, gpJSCoreGlobalState->pPathToCore, "core", cx, glob)  );

	// Load modules.

	SG_ERR_CHECK(  SG_dir__list(pCtx, gpJSCoreGlobalState->pPathToModules, NULL, NULL, NULL, &pModules)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pModuleDir, pModules, &ok, &szModuleDir, NULL)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pModuleDirPath, gpJSCoreGlobalState->pPathToModules)  );
	while(ok)
	{
		if (szModuleDir[0] != '.')
		{
			SG_fsobj_stat fss;

			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pModuleDirPath, szModuleDir)  );

			SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pModuleDirPath, &fss)  );

			if (fss.type & SG_FSOBJ_TYPE__DIRECTORY)  // dot paths?
			{
				SG_ERR_CHECK(  _loadModuleDir(pCtx, pModuleDirPath, szModuleDir, cx, glob)  );
			}

			SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pModuleDirPath)  );
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pModuleDir, &ok, &szModuleDir, NULL)  );
	}
												
	if (! JS_LookupProperty(cx, glob, "initModules", &fo))
	{
		SG_ERR_CHECK_CURRENT;
		SG_ERR_THROW2(SG_ERR_JS, (pCtx, "lookup of initModules() failed"));
	}

	if (!JSVAL_IS_VOID(fo))
		if (! JS_CallFunctionName(cx, glob, "initModules", 0, NULL, &rval))
			{
				SG_ERR_CHECK_CURRENT;
				SG_ERR_THROW2(SG_ERR_JS, (pCtx, "Call to initModules() failed"));
			}

fail:
	SG_NULLFREE(pCtx, psz_js);
	SG_PATHNAME_NULLFREE(pCtx, pModuleDirPath);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pModuleDir);
	SG_RBTREE_NULLFREE(pCtx, pModules);
}


void SG_jscore__check_module_dags(SG_context *pCtx, JSContext *cx, JSObject *glob, const char *reponame)
{
	jsval args[2];
	JSBool js_ok;
	jsval rval;
	JSString *pjs;
	jsval fo = JSVAL_VOID;

	if (gpJSCoreGlobalState->bSkipModules || (! gpJSCoreGlobalState->pPathToModules))
		return;

	SG_ERR_CHECK(  _sg_jscore__install_modules(pCtx, cx, glob, NULL)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, reponame))  );

	args[0] = STRING_TO_JSVAL(pjs);

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(gpJSCoreGlobalState->pPathToModules)))  );

	args[1] = STRING_TO_JSVAL(pjs);

	if (! JS_LookupProperty(cx, glob, "checkModuleDags", &fo))
	{
		SG_ERR_CHECK_CURRENT;
		SG_ERR_THROW2(SG_ERR_JS, (pCtx, "lookup of checkModuleDags failed"));
	}

	if (!JSVAL_IS_VOID(fo))
	{
		js_ok = JS_CallFunctionName(cx, glob, "checkModuleDags", SG_NrElements(args), args, &rval);
		SG_ERR_CHECK_CURRENT;
		if(!js_ok)
			SG_ERR_THROW2(SG_ERR_JS, (pCtx, "An error occurred initializing modules: call to JavaScript checkModuleDags() failed"));
	}

 fail:
	;
}



///////////////////////////////////////////////////////////////////////////////

void _free_js_mutex_cb(SG_context* pCtx, void * pVoidData)
{
	if (pVoidData)
	{
		_namedMutex* p = (_namedMutex*)pVoidData;
		SG_ERR_IGNORE(  SG_mutex__unlock(pCtx, &p->mutex)  );
		SG_NULLFREE(pCtx, p);
	}
}

void SG_jscore__mutex__lock(
	SG_context* pCtx,
	const char* pszName)
{
	SG_bool bExists = SG_FALSE;
	_namedMutex* pFreeThisNamedMutex = NULL;
	_namedMutex* pNamedMutex = NULL;

	SG_ASSERT(gpJSCoreGlobalState);

	/* We always acquire the rbtree mutex first, then the specific named mutex. A deadlock is impossible. */
	//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Waiting for JS mutex manager in LOCK.")  );
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, gpJSCoreGlobalState->prbJSMutexes, pszName, &bExists, (void**)&pNamedMutex)  );
	if (!bExists)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pFreeThisNamedMutex)  );
		pNamedMutex = pFreeThisNamedMutex;
		pNamedMutex->count = 0;
		SG_ERR_CHECK(  SG_mutex__init(pCtx, &pNamedMutex->mutex)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, gpJSCoreGlobalState->prbJSMutexes, pszName, pNamedMutex)  );
		pFreeThisNamedMutex = NULL;
	}
	pNamedMutex->count++; // Cannot be touched unless you hold mutexJsNamed. We do here.
	if (pNamedMutex->count > 10)
		SG_ERR_CHECK(  SG_log__report_info(pCtx, "%u threads are waiting for named JS mutex: %s", pNamedMutex->count-1, pszName)  );

	/* We deliberately unlock the rbtree mutex before locking the named mutex.
	 * We want to hold the lock on the rbtree for as little time as possible. Any subsequent
	 * attempts to lock the same name will yield the correct named mutex and correctly block
	 * on it below, without blocking access to the name management rbtree. */
	SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
	//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Released JS mutex manager in LOCK.")  );

	//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Waiting for named JS mutex: %s", pszName)  );
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &pNamedMutex->mutex)  );
	//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Acquired named JS mutex: %s", pszName)  );

	return;

fail:
	SG_ERR_IGNORE(  SG_mutex__unlock(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
	SG_NULLFREE(pCtx, pFreeThisNamedMutex);
}

void SG_jscore__mutex__unlock(
	SG_context* pCtx,
	const char* pszName)
{
	SG_bool bExists = SG_FALSE;
	_namedMutex* pNamedMutex = NULL;

	SG_ASSERT(gpJSCoreGlobalState);

	/* We always acquire the rbtree mutex first, then the specific named mutex. A deadlock is impossible. */
	//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Waiting for JS mutex manager in UNLOCK.")  );
	SG_ERR_CHECK(  SG_mutex__lock(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, gpJSCoreGlobalState->prbJSMutexes, pszName, &bExists, (void**)&pNamedMutex)  );
	if (bExists)
	{
		SG_ASSERT(pNamedMutex);

		//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Releasing named JS mutex: %s", pszName)  );
		SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &pNamedMutex->mutex)  );

		pNamedMutex->count--; // Cannot be touched unless you hold mutexJsNamed. We do here.
		if ( 0 == pNamedMutex->count )
		{
			//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Nobody else is waiting. Removing named JS mutex: %s", pszName)  );
			SG_ERR_CHECK(  SG_rbtree__remove(pCtx, gpJSCoreGlobalState->prbJSMutexes, pszName)  );
			SG_NULLFREE(pCtx, pNamedMutex);
		}
		//else if (pNamedMutex->count > 1) // Cannot be touched unless you hold mutexJsNamed. We do here.
			//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "%u threads are waiting for named JS mutex: %s", pNamedMutex->count-1, pszName)  );

		/* We deliberately unlock the rbtree mutex after the named mutex.
		   Creating a new mutex with the same name can't be allowed until this one is released, 
		   because they are logically the same lock. 
		   Unlocking doesn't block on anything and should be fast. */
		SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
		//SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Released JS mutex manager in UNLOCK.")  );
	}
	else
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND, (pCtx, "Named mutex: %s", pszName)  );

	return;

fail:
	SG_ERR_IGNORE(  SG_mutex__unlock(pCtx, &gpJSCoreGlobalState->mutexJsNamed)  );
}


///////////////////////////////////////////////////////////////////////////////


void SG_jscore__shutdown(SG_context * pCtx)
{
	if(gpJSCoreGlobalState!=NULL)
	{
		if (gpJSCoreGlobalState->rt)
			JS_DestroyRuntime(gpJSCoreGlobalState->rt);
		JS_ShutDown();

		SG_PATHNAME_NULLFREE(pCtx, gpJSCoreGlobalState->pPathToDispatchDotJS);
		SG_PATHNAME_NULLFREE(pCtx, gpJSCoreGlobalState->pPathToCore);
		SG_PATHNAME_NULLFREE(pCtx, gpJSCoreGlobalState->pPathToModules);
		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, gpJSCoreGlobalState->prbJSMutexes, _free_js_mutex_cb);
		SG_NULLFREE(pCtx, gpJSCoreGlobalState);
	}
}

///////////////////////////////////////////////////////////////////////////////
