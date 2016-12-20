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
 *
 * @file sg_jsglue__private_sg_curl.h
 *
 * @details Routines implementing sg.curl
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_JSGLUE__PRIVATE_SG_CURL_H
#define H_SG_JSGLUE__PRIVATE_SG_CURL_H

BEGIN_EXTERN_C;

// Curl option setting helper methods
static void _sg_jsglue_curl__get_headers(SG_context * pCtx, SG_curl * pCurl, SG_vhash * pOpts, struct curl_slist ** ppHeaderList);
static void _sg_jsglue_curl__set_curl_options(SG_context * pCtx, SG_curl * pCurl, SG_vhash * pOpts);
static void _sg_jsglue_curl__set_curl_option(
	SG_context*       pCtx,        //< [in] [out] Error and context info.
	void*             pCallerData, //< [in] The SG_curl instance to set the option in.
	const SG_vhash*   pHash,       //< [in] The hash being iterated.
	const char*       szKey,       //< [in] The current key in the hash being iterated.
	const SG_variant* pValue       //< [in] The current value in the hash being iterated.
	);

////////////////////////////////////////////////////////////////////

static JSClass curl_response__class = {
    "curl_response",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
	
};

////////////////////////////////////////////////////////////////////

/**
 * Options that can be passed to and of the curl methods
 *  
 *	opt name	js_type			valid arguments
 *		description 
 *	"headers" 		<array>		...
 *		array containing any special http headers to be sent with the request
 *	"cookies"		<string>	"NAME=VALUE" or "NAME=VALUE; NAME2=VALUE2"
 *		Set a string to pass as the cookies for this request
 *	"auth_type"		<string>	("basic", "digest", "digest-ie", "gss", "ntlm", "ntlm-wb", "any")
 *		HTTP auth method the server expects
 *	"username"		<string>	...
 *		Username to be used for HTTP auth
 *	"password"		<string>	...
 *		Password to be used for HTTP auth
 *	"follow"		<boolean>
 *		Tells the curl library to follow and Location: header that the server sends as part of an HTTP header
 *
 */
	 
/**
 * sg.curl.get( url, options = {} )
 *
 * TODO 01/03/12 Add an option to (optionally) download the request body to a 
 *				 file.
 */

SG_JSGLUE_METHOD_PROTOTYPE(curl, get)
{
	
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * szArg0 = NULL;
	SG_curl * pCurl = NULL;
	SG_vhash * pOpts = NULL;
	SG_string * pstrResponse = NULL;
	SG_string * pstrRespHeaders = NULL;
	struct curl_slist* pHeaderList = NULL;
	jsval jv;
	SG_int32 httpResponseCode = 0;
	JSObject * jso = NULL;
	
	SG_JS_BOOL_CHECK( argc == 1 || argc == 2);
	
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szArg0)  );

	
	SG_ERR_CHECK(  SG_curl__alloc(pCtx, &pCurl)  );
	
    SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_URL, szArg0)  );
	
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPGET, 1)  );
    
	if (argc == 2)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[1])  );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[1]), &pOpts)  );
		SG_ERR_CHECK( _sg_jsglue_curl__set_curl_options(pCtx, pCurl, pOpts) );
		SG_ERR_CHECK( _sg_jsglue_curl__get_headers(pCtx, pCurl, pOpts, &pHeaderList) );
		SG_VHASH_NULLFREE(pCtx, pOpts);
	}
		
	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pCurl, pstrResponse)  );

	SG_ERR_CHECK( SG_curl__record_headers(pCtx, pCurl) );
	
	SG_ERR_CHECK(  SG_curl__perform(pCtx, pCurl)  );
	
    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &curl_response__class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	
	// Set the response body
    JSVAL_FROM_SZ(jv, SG_string__sz(pstrResponse));
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "body", &jv)  );
	
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	
	// Set the HTTP status code
	SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_RESPONSE_CODE, &httpResponseCode)  );
	jv = INT_TO_JSVAL(httpResponseCode);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "http_status", &jv)  );
	
	// Set the response headers
	SG_ERR_CHECK( SG_curl__get_response_headers(pCtx, pCurl, &pstrRespHeaders));
    JSVAL_FROM_SZ(jv, SG_string__sz(pstrRespHeaders));
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "headers", &jv)  );
	
	SG_STRING_NULLFREE(pCtx, pstrRespHeaders);
	
	// Do some cleanup
	SG_CURL_HEADERS_NULLFREE(pCtx, pHeaderList);
	SG_CURL_NULLFREE(pCtx, pCurl);
	SG_NULLFREE(pCtx, szArg0);
	
    return JS_TRUE;
	
fail:
	SG_VHASH_NULLFREE(pCtx, pOpts);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_STRING_NULLFREE(pCtx, pstrRespHeaders);
	SG_CURL_HEADERS_NULLFREE(pCtx, pHeaderList);
	SG_CURL_NULLFREE(pCtx, pCurl);
	SG_NULLFREE(pCtx, szArg0);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;

}

/**
* Other Proposed curl methods
*
* sg.curl.post( url, post_data, options = {})
*
* sg.curl.put( url, file_handle, options = {})
* 
* sg.curl.delete( url, option = {})
*/


static void _sg_jsglue_curl__get_headers(SG_context * pCtx, SG_curl * pCurl, SG_vhash * pOpts, struct curl_slist ** ppHeaderList)
{
	const SG_variant * pvHeaders = NULL;
	
	SG_vhash__get__variant(pCtx, pOpts, "headers", &pvHeaders);
	
	// If we don't have any headers, bail
	if(SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
	{
		SG_context__err_reset(pCtx);
		return;
	}
	
	if (pvHeaders->type == SG_VARIANT_TYPE_VARRAY)
	{
		SG_varray * pvaHeaders = NULL;
		SG_ERR_CHECK_RETURN( SG_variant__get__varray(pCtx, pvHeaders, &pvaHeaders) );
		SG_ERR_CHECK_RETURN( SG_curl__set_headers_from_varray(pCtx, pCurl, pvaHeaders, ppHeaderList) ); 
	}

}

static void _sg_jsglue_curl__set_curl_options(SG_context * pCtx, SG_curl * pCurl, SG_vhash * pOpts)
{
	// iterate through each value in the options hash and if it is a expected option
	// set the approriate curl flag
	SG_ERR_CHECK_RETURN(  SG_vhash__foreach(pCtx, pOpts, _sg_jsglue_curl__set_curl_option, pCurl)  );
}

static void _sg_jsglue_curl__set_curl_option(
	SG_context*       pCtx,        //< [in] [out] Error and context info.
	void*             pCallerData, //< [in] The SG_curl instance to set the option in.
	const SG_vhash*   pHash,       //< [in] The hash being iterated.
	const char*       szKey,       //< [in] The current key in the hash being iterated.
	const SG_variant* pValue       //< [in] The current value in the hash being iterated.
	)
{
	SG_curl* pCurl = (SG_curl*)pCallerData;
	
	SG_NULLARGCHECK_RETURN(pCallerData);
	SG_UNUSED(pHash);
	SG_NULLARGCHECK_RETURN(szKey);
	SG_NULLARGCHECK_RETURN(pValue);
	
	if (strcmp("auth_type", szKey) == 0)
	{
		const char* psz = NULL;
		SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pValue, &psz)  );
		if (strcmp("basic", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC)  );
		}
		else if (strcmp("digest", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		}
		else if (strcmp("digest-ie", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST_IE)  );
		}
		else if (strcmp("gss", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, CURLAUTH_GSSNEGOTIATE)  );
		}
		else if (strcmp("ntlm", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM)  );
		}
		else if (strcmp("any", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY)  );
		}
		else if (strcmp("any-safe", psz) == 0)
		{
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANYSAFE)  );
		}
	}
	else if(strcmp("cookies", szKey) == 0)
	{
		const char* psz = NULL;
		SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pValue, &psz)  );
		SG_ERR_CHECK_RETURN(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_COOKIE, psz)  );
	}
	
	else if(strcmp("username", szKey) == 0)
	{
		const char* psz = NULL;
		SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pValue, &psz)  );
		SG_ERR_CHECK_RETURN(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_USERNAME, psz)  );
	}
	else if(strcmp("password", szKey) == 0)
	{		
		const char* psz = NULL;
		SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pValue, &psz)  );
		SG_ERR_CHECK_RETURN(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_PASSWORD, psz)  );
	}
	else if(strcmp("follow", szKey) == 0)
	{
		SG_bool b = SG_FALSE;
		SG_ERR_CHECK_RETURN(  SG_variant__get__bool(pCtx, pValue, &b)  );
		if (b)
			SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_FOLLOWLOCATION, 1)  );
	}
	else
	{
		// Discard any unknown options
	}	
}

//////////////////////////////////////////////////////////////////

static JSClass sg_curl__class = {
    "sg_curl",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec sg_curl__methods[] = {
	{ "get",    SG_JSGLUE_METHOD_NAME(curl, get),  1,0},
	{ NULL, NULL, 0, 0}
};


/*
 * These properties & methods are available on a curl_response class.
 */
static JSPropertySpec curl_response__properties[] = {
    {NULL,0,0,NULL,NULL}
};

static JSFunctionSpec curl_response__methods[] = {
	{ NULL, NULL, 0, 0}
};

//////////////////////////////////////////////////////////////////

/**
 * This function defines the curl_response class
 */	
static void sg_jsglue__install_sg_curl(SG_context * pCtx, JSContext * cx, JSObject * glob)
{
    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &curl_response__class,
            NULL, /* no constructor */
            0, /* nargs */
            curl_response__properties,
            curl_response__methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );
fail:
	// Nothing to do.  I think???
	return;
}


END_EXTERN_C;

#endif//H_SG_JSGLUE__PRIVATE_SG_CURL_H
