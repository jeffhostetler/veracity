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

#ifndef H_SG_JSCORE_PROTOTYPES_H
#define H_SG_JSCORE_PROTOTYPES_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


/**
 * Wrapper around JS_NewRuntime().
 * 
 * Throws SG_ERR_ALREADY_INITIALIZED if it has already been called before.
 */
void SG_jscore__new_runtime(
	SG_context * pCtx,               //< [in] [out]

	JSContextCallback cb,            //< [in] JSContextCallback to use for all 'JSContext's.
	JSFunctionSpec *shell_functions, //< [in] Shell functions to use (optional).

	SG_bool bSkipModules,			 //< [in] Don't try to load js from server/files

	JSRuntime **rt                   //< [out] Newly created JavaScript runtime.
	);

/**
 * Wrapper around JS_DestroyRuntime() followed by JS_Shutdown().
 */
void SG_jscore__shutdown(
	SG_context * pCtx //< [in] N.B.: Input parameter only, since called by
	                  //  SG_lib__global_cleanup(). Will always return in an
	                  //  error-free state.
	);

/**
 * Wrapper around JS_NewContext(), also including:
 *  - association with current thread with JS_SetContextThread(),
 *  - starting of a new request with JS_BeginRequest(),
 *  - creation of global object with JS_NewCompartmentAndGlobalObject(),
 *  - and of course loading in all of veracity's "js glue".
 * 
 * Yes, this means that cx is already in a request when handed to you.
 * 
 * Note we do not also provide a wrapper for JS_EndRequest(),
 * JS_ClearContextThread(), or JS_DestroyContext(), so you are responsible for
 * calling directly into the JSAPI however you want hereafter for any further
 * use of and the tearing down of the output parameters.
 * 
 * Currently this function installs more glue than it should, because it
 * installs the entire "web glue" needed by the http server to handle http
 * requests. This should be moved out to sg_jscontextpool.
 */
void SG_jscore__new_context(
	SG_context * pCtx, //< [in] [out] Error and context info.
	                   //  Note that on success, the returned JSContext will have a reference to
	                   //  this pCtx, so subsequent calls into the JSAPI can potentially affect it.

	JSContext ** cx,   //< [out] Newly created JavaScript context.
	JSObject ** glob,  //< [out] Newly instantiated global object for cx.

	// The following parameter is specific to the http server and should be
	// removed from this function. Unfortunately that will require some work.
	const SG_vhash * pServerConfig
	);

/**
 * 
 */
void SG_jscore__check_module_dags(
	SG_context * pCtx,
	JSContext *cx,
	JSObject *glob,
	const char *reponame
	);

///////////////////////////////////////////////////////////////////////////////

void SG_jscore__mutex__lock(
	SG_context* pCtx,
	const char* pszName);

void SG_jscore__mutex__unlock(
	SG_context* pCtx,
	const char* pszName);

END_EXTERN_C;

#endif
