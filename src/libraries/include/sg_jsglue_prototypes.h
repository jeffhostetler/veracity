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
 * @file sg_jsglue_prototypes.h
 *
 */

#ifndef H_SG_JSGLUE_PROTOTYPES_H
#define H_SG_JSGLUE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

#define SG_OPT_NONRECURSIVE "nonrecursive";
#define SG_OPT_TEST "test";
#define SG_OPT_FORCE "force";
#define SG_OPT_STAMP "stamp";
#define SG_OPT_MESSGE "message";
#define SG_OPT_INCLUDE "include";
#define SG_OPT_EXCLUDE "exclude"

void SG_jsglue__np__required__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint32* pi
    );

void SG_jsglue__np__optional__varray(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_varray** ppva
    );

void SG_jsglue__np__required__varray(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_varray** ppva
    );

void SG_jsglue__np__required__vhash(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_vhash** ppvh
    );

void SG_jsglue__np__required__sz(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    const char** ppsz
    );

void SG_jsglue__np__required__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_bool* p
    );

void SG_jsglue__np__optional__sz(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    const char* psz_def,
    const char** ppsz
    );

void SG_jsglue__np__optional__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_bool def,
    SG_bool* p
    );

void SG_jsglue__np__optional__uint64(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint64 def,
    SG_uint64* p
    );

void SG_jsglue__np__optional__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint32 def,
    SG_uint32* p
    );

void SG_jsglue__np__optional__int32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_int32 def,
    SG_int32* p
    );

void SG_jsglue__np__anything_not_got_is_invalid(
        SG_context* pCtx,
        const char* psz_func,
        SG_vhash* pvh_args,
        SG_vhash* pvh_got
        );


SG_context * SG_jsglue__get_clean_sg_context(JSContext * cx);

void SG_jsglue__set_sg_context(SG_context * pCtx, JSContext * cx);

JSBool SG_jsglue__context_callback(JSContext * cx, uintN contextOp);

void SG_jsglue__error_reporter(JSContext * cx, const char * pszMessage, JSErrorReport * pJSErrorReport);

void SG_jsglue__report_sg_error(SG_context * pCtx, JSContext * cx);

void SG_jsglue__install_scripting_api(SG_context * pCtx, JSContext *cx, JSObject *obj);

SG_js_safeptr * SG_jsglue__get_object_private(JSContext * cx, JSObject * obj);

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Utility functions used by methods and properties.
//These are also used by sg_zing_jsglue and the "web glue" (sg_uridispatch.c).
void sg_jsglue__copy_stringarray_into_jsobject(SG_context * pCtx, JSContext* cx, SG_stringarray* psa, JSObject* obj);
void sg_jsglue__copy_varray_into_jsobject(SG_context * pCtx, JSContext* cx, SG_varray* pva, JSObject* obj);
void sg_jsglue__copy_vhash_into_jsobject(SG_context * pCtx, JSContext* cx, const SG_vhash* pvh, JSObject* obj);
void sg_jsglue__jsobject_to_vhash(SG_context * pCtx, JSContext* cx, JSObject* obj, SG_vhash** ppvh);
void sg_jsglue__jsobject_to_varray(SG_context * pCtx, JSContext* cx, JSObject* obj, SG_varray** ppva);
void sg_jsglue__jsval_to_json(SG_context * pCtx, JSContext* cx, jsval obj, SG_bool bPretty, SG_string ** ppstr);
void sg_jsglue__jsstring_to_sz(SG_context *pCtx, JSContext *cx, JSString *str, char **ppOut);
SG_bool sg_jsglue_get_equals_flag(const char* psz_flag, const char* psz_match, const char** psz_value);
void sg_jsglue_get_pattern_array(SG_context * pCtx, const char* psz_patterns, SG_stringarray** psaPatterns);


/**
 * SUSPEND_REQUEST_ERR_CHECK() is like SG_ERR_CHECK, but suspends the JS Request
 * before evaluating the expression and resumes the request afterwards.
 */
#define SUSPEND_REQUEST_ERR_CHECK(expr) SG_STATEMENT(\
	jsrefcount rc; \
	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
	rc = JS_SuspendRequest(cx); \
	SG_httprequestprofiler__stop(); \
	expr; \
	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
	JS_ResumeRequest(cx, rc); \
	SG_httprequestprofiler__stop(); \
	SG_ERR_CHECK_CURRENT; \
)

/**
 * SUSPEND_REQUEST_ERR_CHECK2() is the same as the SUSPEND_REQUEST_ERR_CHECK, but
 * allows you to tell the Http Request Profiler a category of operation to use for
 * all time spend on evaluating the expression.
 */
#define SUSPEND_REQUEST_ERR_CHECK2(expr, category) SG_STATEMENT(\
	jsrefcount rc; \
	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
	rc = JS_SuspendRequest(cx); \
	SG_httprequestprofiler__switch(category); \
	expr; \
	SG_httprequestprofiler__switch(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
	JS_ResumeRequest(cx, rc); \
	SG_httprequestprofiler__stop(); \
	SG_ERR_CHECK_CURRENT; \
)

/**
 * Macros to suspend and resume a request that will span multiple ERR_CHECKS. Ie:
 * 
 *     SG_JS_SUSPENDREQUEST()
 *     REQUEST_SUSPENDED_ERR_CHECK( ... );
 *     REQUEST_SUSPENDED_ERR_CHECK( ... );
 *     ...
 *     SG_JS_RESUMEREQUEST()
 * 
 * Do not use a regular SG_ERR_CHECK inbtween SG_JS_SUSPENDREQUEST and SG_JS_RESUMEREQUEST!!!!!
 */
#define SG_JS_SUSPENDREQUEST() \
	SG_STATEMENT__BEGIN\
		jsrefcount rc; \
		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
		rc = JS_SuspendRequest(cx); \
		SG_httprequestprofiler__stop();
#define REQUEST_SUSPENDED_ERR_CHECK(expr) \
		SG_STATEMENT( \
			expr; \
			if(SG_CONTEXT__HAS_ERR(pCtx)) \
			{ \
				SG_context__err_stackframe_add(pCtx,__FILE__,__LINE__); \
				SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
				JS_ResumeRequest(cx, rc); \
				SG_httprequestprofiler__stop(); \
				goto fail; \
			} \
		)
#define SG_JS_RESUMEREQUEST() \
		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING); \
		JS_ResumeRequest(cx, rc); \
		SG_httprequestprofiler__stop(); \
	SG_STATEMENT__END

/**
 * SG_JS_BOOL_CHECK() is used to wrap JS_ statements that fail by returning JS_FALSE.
 */
#define SG_JS_BOOL_CHECK(expr) SG_STATEMENT(\
	JSBool sg_js_bool_check_ok;\
	sg_js_bool_check_ok = (expr);\
	SG_ERR_CHECK_CURRENT;\
	if (!sg_js_bool_check_ok)\
	{\
		SG_context__err(pCtx,SG_ERR_JS,__FILE__,__LINE__,#expr);\
		goto fail;\
	}\
)

/**
 * SG_JS_NULL_CHECK() is used to wrap JS_ statements that fail by returning NULL.
 */
#define SG_JS_NULL_CHECK(expr) SG_STATEMENT(\
	void * sg_js_null_check_ok;\
	sg_js_null_check_ok = (void*)(expr);\
	SG_ERR_CHECK_CURRENT;\
	if (!sg_js_null_check_ok)\
	{\
		SG_context__err(pCtx,SG_ERR_JS,__FILE__,__LINE__,#expr);\
		goto fail;\
	}\
)

/**
 * JSVAL_FROM_SZ() effectively makes a call to JS_NewStringCopyZ(), wrapped in an SG_JS_NULL_CHECK().
 * Then, if successful, it uses STRING_TO_JSVAL() to store the result in a jsval.
 * 
 * NB: The first parameter "dest" is an lval.
 * 
 * Also note we use a hardcoded "cx" for the JSContext parameter to JS_StringCopyZ().
 *
 * WARNING: If you get an error report that JS_NewStringCopyZ() failed, your
 * WARNING: _src_ string probably does not look like UTF8 data.
 * 
 */
#define JSVAL_FROM_SZ(_dest_, _src_) SG_STATEMENT(\
	JSString * js_coppied_string;\
	js_coppied_string = JS_NewStringCopyZ(cx, (_src_));\
	SG_ERR_CHECK_CURRENT;\
	if (!js_coppied_string)\
	{\
		SG_context__err(pCtx,SG_ERR_JS,__FILE__,__LINE__, "JS_NewStringCopyZ() failed.");\
		goto fail;\
	}\
	(_dest_) = STRING_TO_JSVAL(js_coppied_string);\
)




// "JSVAL_IS_OBJECT(v) returns true if v is either an object or JSVAL_NULL.
// This indicates that it is safe to call JSVAL_TO_OBJECT(v) to convert v to
// type JSObject *. Be sure to check for JSVAL_NULL separately if necessary.
// If you only want to determine whether a value v is an object but not null,
// use !JSVAL_IS_PRIMITIVE(v)."
// -- https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference/JSVAL_IS_OBJECT
#define JSVAL_IS_NONNULL_OBJECT(x) (!JSVAL_IS_PRIMITIVE(x))


//////////////////////////////////////////////////////////////////
void SG_jsglue__np__required__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint32* pi
    );

void SG_jsglue__np__required__varray(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_varray** ppva
    );

void SG_jsglue__np__required__vhash(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_vhash** ppvh
    );

void SG_jsglue__np__required__sz(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    const char** ppsz
    );

void SG_jsglue__np__required__sz_or_varray_of_sz(SG_context * pCtx,
												 SG_vhash * pvh_args,
												 SG_vhash * pvh_got,
												 const char * psz_key,
												 const char ** ppsz,
												 SG_varray ** ppva);

void SG_jsglue__np__optional__sz_or_varray_of_sz(SG_context * pCtx,
												 SG_vhash * pvh_args,
												 SG_vhash * pvh_got,
												 const char * psz_key,
												 const char ** ppsz,
												 SG_varray ** ppva);

void SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(SG_context * pCtx,
															  SG_vhash * pvh_args,
															  SG_vhash * pvh_got,
															  const char * psz_key,
															  SG_stringarray ** ppsa);

void SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(SG_context * pCtx,
															  SG_vhash * pvh_args,
															  SG_vhash * pvh_got,
															  const char * psz_key,
															  SG_stringarray ** ppsa);

void SG_jsglue__np__required__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_bool* p
    );

void SG_jsglue__np__optional__sz(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    const char* psz_def,
    const char** ppsz
    );

void SG_jsglue__np__optional__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_bool def,
    SG_bool* p
    );

void SG_jsglue__np__optional__uint64(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint64 def,
    SG_uint64* p
    );

void SG_jsglue__np__optional__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint32 def,
    SG_uint32* p
    );

void SG_jsglue__np__optional__int32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_int32 def,
    SG_int32* p
    );

void SG_jsglue__np__optional__vhash(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
	SG_vhash * pvh_default,
    SG_vhash** ppvh
    );

void SG_jsglue__np__anything_not_got_is_invalid(
	SG_context* pCtx,
	const char* psz_func,
	SG_vhash* pvh_args,
	SG_vhash* pvh_got
	);

void SG_jsglue__np__optional__when(SG_context * pCtx,
								   SG_vhash * pvh_args,
								   SG_vhash * pvh_got,
								   const char * psz_key,
								   SG_int64 def,
								   SG_int64 * piWhen);

void SG_jsglue__np__required__when(SG_context * pCtx,
								   SG_vhash * pvh_args,
								   SG_vhash * pvh_got,
								   const char * psz_key,
								   SG_int64 * piWhen);

//////////////////////////////////////////////////////////////////

/**
 * We cannot control the prototypes of methods and properties;
 * these are defined by SpiderMonkey (JSNative and JSPropertyOp).
 *
 * Note that they do not take a SG_Context.  Methods and properties
 * will have to extract it from the js-context-private data.
 */
#define SG_JSGLUE_METHOD_NAME(class, name) sg_jsglue__method__##class##__##name
#define SG_JSGLUE_METHOD_PROTOTYPE(class, name) static JSBool SG_JSGLUE_METHOD_NAME(class, name)(JSContext *cx, uintN argc, jsval *vp)

#define SG_JSGLUE_PROPERTY_NAME(class, name) sg_jsglue__property__##class##__##name
#define SG_JSGLUE_PROPERTY_PROTOTYPE(class, name) static JSBool SG_JSGLUE_PROPERTY_NAME(class, name)(JSContext *cx, JSObject *obj, jsid id, jsval *vp)

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_JSGLUE_PROTOTYPES_H

