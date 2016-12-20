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

#include <signal.h>
#include <sg.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_wc__public_typedefs.h>

#include <sg_vv2__public_prototypes.h>
#include <sg_wc__public_prototypes.h>

#include <wc0util/sg_wc_diff_utils__private_prototypes.h>	// TODO 2012/05/04 remove this with sg.repo.diff_file().
#include <vv6history/sg_vv6history__private_prototypes.h>	// TODO 2012/07/13 decide on history calls

#include <sghash.h>
#include "sg_js_safeptr__private.h"

// Functions in uridispatch that are not a part of its regular public interface.
void sg_uridispatch__query_repo_availability(SG_context * pCtx, const char * szRepoName, SG_bool * pbIsAvailable);
void sg_uridispatch__repo_addremove_allowed(SG_context* pCtx, SG_bool* pbAllowed);

/* TODO make sure we tell JS that strings are utf8 */

/*
 * Any object that requires a pointer (like an SG_repo)
 * is handled as a special class with private data
 * for storing the pointer (as an SG_safeptr).  See
 * sg_jsglue__get_object_private().
 *
 * Any object that can be serialized to a vhash is
 * simply handled as a plain JS object.
 */

/* Whenever we put a 64 bit integer into JSON, or into
 * a JSObject, if it won't fit into a 32 bit signed
 * int, we actually store it as a string instead of
 * as an int. */

//////////////////////////////////////////////////////////////////


SG_context * SG_jsglue__get_clean_sg_context(JSContext * cx)
{
	// TODO Do we want to use a SG_safeptr to wrap this?
	// TODO We have a bit of a chicken-n-egg problem.
	// TODO
	// TODO I don't think it is as much of an issue because
	// TODO we only have one JSContext.  Which is not like
	// TODO a JSObject where we have many.
	//
	// I'm going to say no for now.
	SG_context * ptr = NULL;
	ptr = (SG_context *)JS_GetContextPrivate(cx);
	SG_context__err_reset(ptr);
	return ptr;
}

SG_context * SG_jsglue__get_sg_context(JSContext * cx)
{
	// TODO Do we want to use a SG_safeptr to wrap this?
	// TODO We have a bit of a chicken-n-egg problem.
	// TODO
	// TODO I don't think it is as much of an issue because
	// TODO we only have one JSContext.  Which is not like
	// TODO a JSObject where we have many.
	//
	// I'm going to say no for now.
	SG_context * ptr = NULL;
	ptr = (SG_context *)JS_GetContextPrivate(cx);
	return ptr;
}
void SG_jsglue__set_sg_context(SG_context * pCtx, JSContext * cx)
{
	// remember pCtx in the cx so that it can be referenced
	// by methods that we export to JavaScript.
	JS_SetContextPrivate(cx, pCtx);
}

//////////////////////////////////////////////////////////////////

/**
 * This is a JSContextCallback.
 *
 * This gets called whenever a CX is created or destroyed.
 *
 * https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference/JS_SetContextCallback
 */
JSBool SG_jsglue__context_callback(JSContext * cx, uintN contextOp)
{
	switch (contextOp)
	{
	case JSCONTEXT_NEW:
		JS_SetErrorReporter(cx,SG_jsglue__error_reporter);
		JS_SetVersion(cx, JSVERSION_LATEST);
		// TODO see SetContextOptions() in vscript.c for other goodies to set.
		return JS_TRUE;

	default:
	//case JSCONTEXT_DESTROY:
		return JS_TRUE;
	}
}

/**
 * This is a JSErrorReporter.
 *
 * This gets called whenever an error happens on this CX (either inside JavaScript proper
 * or within our glue code when SG_jsglue__report_sg_error() is called).
 */
void SG_jsglue__error_reporter(JSContext * cx, const char * pszMessage, JSErrorReport * pJSErrorReport)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);

	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		// we had an error within the jsglue code in sglib (which has pushed
		// one or more sglib-stackframes) before calling SG_jsglue__report_sg_error()
		// at the end of the METHOD/PROPERY.
		//
		// So, the information in the JSErrorReport may or may not be of that
		// much value.
		//
		// We ignore the supplied pszMessage because it is just a copy of
		// the description that was provided when the original sglib-error
		// was thrown.
		//
		// TODO experiment with this and see if the JSErrorReport gives us any
		// TODO new information (like the location in the script) that we'll want.
		// TODO
		// TODO for now, just append a stackframe with the info and see if we
		// TODO get a duplicate entry....

		SG_context__err_stackframe_add(pCtx,
									   ((pJSErrorReport->filename) ? pJSErrorReport->filename : "<no_filename>"),
									   pJSErrorReport->lineno ? pJSErrorReport->lineno : 1);
	}
	else
	{
		// something within JavaScript threw an error (and sglib was not involved).
		// start a new error in pCtx using all of the information in JSErrorReport.

		SG_context__err(pCtx, SG_ERR_JS,
						((pJSErrorReport->filename) ? pJSErrorReport->filename : "<no_filename>"),
						pJSErrorReport->lineno ? pJSErrorReport->lineno : 1,
						pszMessage);
	}
}

/**
 * This should be called as the last step in the fail-block of ***all***
 * METHOD/PROPERTY functions.
 *
 * This is necessary so that JavaScript try/exception handling works
 * as expected.
 *
 * IT IS IMPORTANT THAT ALL CALLS TO SG_jsglue__report_sg_error() ***NOT*** BE
 * WRAPPED IN SG_ERR_IGNORE() (because it would hide the error from us).
 */
void SG_jsglue__report_sg_error(/*const*/ SG_context * pCtx, JSContext * cx)
{
	SG_error err;
	char szErr[SG_ERROR_BUFFER_SIZE];
	SG_string * pStrContextDump = NULL;

	SG_ASSERT(  SG_CONTEXT__HAS_ERR(pCtx)  );

	SG_context__err_to_string(pCtx, SG_TRUE, &pStrContextDump);
	if (!pStrContextDump)
	{
		// We're in an error-on-error state.
		// SG_context's error routines won't give us the description because it doesn't want to allocate memory.
		SG_context__get_err(pCtx, &err);
		SG_error__get_message(err, SG_TRUE, szErr, sizeof(szErr));
		JS_ReportError(cx, "%s", szErr);
	}
	else
	{
		JS_ReportError(cx, "%s", SG_string__sz(pStrContextDump));
		SG_STRING_NULLFREE(pCtx, pStrContextDump);
	}

	SG_context__err_reset(pCtx);
}

//////////////////////////////////////////////////////////////////

void sg_jsglue__set_variant_as_property(SG_context * pCtx, JSContext* cx, JSObject* obj, const char* psz_name, const SG_variant* pv);
void sg_jsglue__set_variant_as_element(SG_context * pCtx, JSContext* cx, JSObject* obj, SG_uint32 i_ndx, const SG_variant* pv);

/* ---------------------------------------------------------------- */
/* JSClass structures.  The InitClass calls are at the bottom of
 * the file */
/* ---------------------------------------------------------------- */

static JSClass sg_audit_class = {
    "sg_audit",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_cbuffer_class = {
    "sg_cbuffer",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_blobset_class = {
    "sg_blobset",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_repo_class = {
    "sg_repo",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_fetchblobhandle_class = {
    "sg_fetchblobhandle",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_fetchfilehandle_class = {
    "sg_fetchfilehandle",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_class = {
    "sg",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass fs_class = {
    "sg_fs",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass server_class = {
    "sg_server",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass file_class = {
    "sg_file",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass fsobj_properties_class = {
    "fsobj_properties",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

/* TODO need a class for sg_file */

/* ---------------------------------------------------------------- */
/* Utility functions */
/* ---------------------------------------------------------------- */


SG_safeptr* sg_jsglue__get_object_private(JSContext *cx, JSObject *obj)
{
    SG_safeptr* psp = (SG_safeptr*) JS_GetPrivate(cx, obj);
    return psp;
}

SG_js_safeptr * SG_jsglue__get_object_private(JSContext * cx, JSObject * obj)
{
	return ((SG_js_safeptr *)sg_jsglue__get_object_private(cx, obj));
}

void sg_jsglue__copy_stringarray_into_jsobject(SG_context * pCtx, JSContext* cx, SG_stringarray* psa, JSObject* obj)
{
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa, &count)  );

    for (i=0; i<count; i++)
    {
        const char * pszv = NULL;
        jsval jv;

        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa, i, &pszv)  );
        JSVAL_FROM_SZ(jv, pszv);

        SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i, &jv)  );
    }

	return;

fail:
    return;
}

// TODO 2011/09/29 Can we make pva const ?
void sg_jsglue__copy_varray_into_jsobject(SG_context * pCtx, JSContext* cx, SG_varray* pva, JSObject* obj)
{
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ARGCHECK(JS_IsArrayObject(cx, obj), obj);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );

    for (i=0; i<count; i++)
    {
        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_varray__get__variant(pCtx, pva, i, &pv)  );
        SG_ERR_CHECK(  sg_jsglue__set_variant_as_element(pCtx, cx, obj, i, pv)  );
    }

	return;

fail:
    /* TODO free obj */
    return;
}

void sg_jsglue__copy_rbtree_keys_into_jsobject(SG_context * pCtx, JSContext* cx, SG_rbtree* prb, JSObject* obj)
{
	SG_rbtree_iterator* pit = NULL;
	const char* szKey;
	SG_bool bFound;
	SG_uint32 i;

	SG_ARGCHECK(JS_IsArrayObject(cx, obj), obj);

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb, &bFound, &szKey, NULL)  );
	for (i = 0; bFound; i++)
	{
		jsval jv;

		JSVAL_FROM_SZ(jv, szKey);

		SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i, &jv)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &bFound, &szKey, NULL)  );
	}

	/* fall through */

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	
}

void sg_jsglue__int64_to_jsval(SG_context * pCtx, JSContext* cx, SG_int64 i, jsval* pjv)
{
    jsval jv = JSVAL_VOID;

    if (SG_int64__fits_in_int32(i))
    {
        jv = INT_TO_JSVAL((SG_int32) i);
    }
    else if (SG_int64__fits_in_double(i))
    {
        double d = (double) i;

        SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, d, &jv)  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_INTEGER_OVERFLOW  );
    }

    *pjv = jv;

    return;

fail:
    return;
}

void sg_jsglue__set_variant_as_element(SG_context * pCtx, JSContext* cx, JSObject* obj, SG_uint32 i_ndx, const SG_variant* pv)
{
    jsval jv = JSVAL_VOID;
    JSString* pjs = NULL;
    JSObject* jso = NULL;

    SG_ARGCHECK(JS_IsArrayObject(cx, obj), obj);

	switch (pv->type)
	{
	case SG_VARIANT_TYPE_DOUBLE:
        {
            double d;

            SG_ERR_CHECK(  SG_variant__get__double(pCtx, pv, &d)  );
            SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, d, &jv)  );
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );
            break;
        }

	case SG_VARIANT_TYPE_INT64:
        {
            SG_int64 i = 0;

            SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pv, &i)  );

            SG_ERR_CHECK(  sg_jsglue__int64_to_jsval(pCtx, cx, i, &jv)  );
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );

            break;
        }

	case SG_VARIANT_TYPE_BOOL:
        {
            SG_bool b;
            int v;

            SG_ERR_CHECK(  SG_variant__get__bool(pCtx, pv, &b)  );

            /* BOOLEAN_TO_JSVAL is documented as accepting only 0 or 1, so
             * we make sure */
            v = b ? 1 : 0;
            jv = BOOLEAN_TO_JSVAL(v);
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );
            break;
        }

	case SG_VARIANT_TYPE_NULL:
        {
            jv = JSVAL_NULL;
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );
            break;
        }

	case SG_VARIANT_TYPE_SZ:
        {
            const char* psz = NULL;

            SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pv, &psz)  );
            SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, psz))  );
            jv = STRING_TO_JSVAL(pjs);
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );
            pjs = NULL;
            break;
        }

	case SG_VARIANT_TYPE_VHASH:
        {
            SG_vhash* pvh = NULL;

            SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh)  );
            SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
            jv = OBJECT_TO_JSVAL(jso);
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );

            SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
            jso = NULL;
            break;
        }

	case SG_VARIANT_TYPE_VARRAY:
        {
            SG_varray* pva = NULL;

            SG_ERR_CHECK(  SG_variant__get__varray(pCtx, pv, &pva)  );
            SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
            jv = OBJECT_TO_JSVAL(jso);
            SG_JS_BOOL_CHECK(  JS_SetElement(cx, obj, i_ndx, &jv)  );

            SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva, jso)  );
            jso = NULL;
            break;
        }
	}

    return;

fail:
    /* TODO free pjs */
    /* TODO free jso */
    return;
}

void sg_jsglue__set_variant_as_property(SG_context * pCtx, JSContext* cx, JSObject* obj, const char* psz_name, const SG_variant* pv)
{
    jsval jv = JSVAL_VOID;
    JSString* pjs = NULL;
    JSObject* jso = NULL;

	switch (pv->type)
	{
	case SG_VARIANT_TYPE_DOUBLE:
        {
            double d;

            SG_ERR_CHECK(  SG_variant__get__double(pCtx, pv, &d)  );
            SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, d, &jv)  );
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );
            break;
        }

	case SG_VARIANT_TYPE_INT64:
        {
            SG_int64 i = 0;

            SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pv, &i)  );

            SG_ERR_CHECK(  sg_jsglue__int64_to_jsval(pCtx, cx, i, &jv)  );
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );

            break;
        }

	case SG_VARIANT_TYPE_BOOL:
        {
            SG_bool b;
            int v;

            SG_ERR_CHECK(  SG_variant__get__bool(pCtx, pv, &b)  );

            /* BOOLEAN_TO_JSVAL is documented as accepting only 0 or 1, so
             * we make sure */
            v = b ? 1 : 0;
            jv = BOOLEAN_TO_JSVAL(v);
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );
            break;
        }

	case SG_VARIANT_TYPE_NULL:
        {
            jv = JSVAL_NULL;
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );
            break;
        }

	case SG_VARIANT_TYPE_SZ:
        {
            const char* psz = NULL;

            SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pv, &psz)  );
            SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, psz))  );
            jv = STRING_TO_JSVAL(pjs);
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );
            pjs = NULL;
            break;
        }

	case SG_VARIANT_TYPE_VHASH:
        {
            SG_vhash* pvh = NULL;

            SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh)  );
            SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
            jv = OBJECT_TO_JSVAL(jso);
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );

            SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
            jso = NULL;
            break;
        }

	case SG_VARIANT_TYPE_VARRAY:
        {
            SG_varray* pva = NULL;

            SG_ERR_CHECK(  SG_variant__get__varray(pCtx, pv, &pva)  );
            SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
            jv = OBJECT_TO_JSVAL(jso);
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );
            SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva, jso)  );
            jso = NULL;
            break;
        }
	}

    return;

fail:
    /* TODO free pjs */
    /* TODO free jso */
    return;
}

void sg_jsglue__copy_ihash_into_jsobject(SG_context * pCtx, JSContext* cx, const SG_ihash* pih, JSObject* obj)
{
    SG_uint32 count = 0;
    SG_uint32 i;
    jsval jv = JSVAL_VOID;

    SG_ERR_CHECK(  SG_ihash__count(pCtx, pih, &count)  );

    for (i=0; i<count; i++)
    {
        const char* psz_name = NULL;
        SG_int64 val = 0;

        SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pih, i, &psz_name, &val)  );
        SG_ERR_CHECK(  sg_jsglue__int64_to_jsval(pCtx, cx, val, &jv)  );
        SG_JS_BOOL_CHECK(  JS_SetProperty(cx, obj, psz_name, &jv)  );
    }

fail:
    /* TODO free obj */
    return;
}

void sg_jsglue__copy_vhash_into_jsobject(SG_context * pCtx, JSContext* cx, const SG_vhash* pvh, JSObject* obj)
{
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );

    for (i=0; i<count; i++)
    {
        const char* psz_name = NULL;
        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &psz_name, &pv)  );
        SG_ERR_CHECK(  sg_jsglue__set_variant_as_property(pCtx, cx, obj, psz_name, pv)  );
    }

fail:
    /* TODO free obj */
    return;
}

void sg_jsglue__jsobject_to_varray(SG_context * pCtx, JSContext* cx, JSObject* obj, SG_varray** ppva)
{
    jsuint len = 0;
    SG_uint32 i = 0;
    SG_varray* pva = NULL;
    SG_varray* pva_sub = NULL;
    SG_vhash* pvh_sub = NULL;

    SG_JS_BOOL_CHECK(  JS_GetArrayLength(cx, obj, &len)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

    for (i=0; i<len; i++)
    {
        jsval jv_value;

        SG_JS_BOOL_CHECK(  JS_GetElement(cx, obj, i, &jv_value)  );

        if (JSVAL_IS_STRING(jv_value))
        {
            SG_ERR_CHECK(  SG_varray__append__jsstring(pCtx, pva, cx, JSVAL_TO_STRING(jv_value))  );
        }
        else if (JSVAL_IS_INT(jv_value))
        {
            SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pva, (SG_int64) JSVAL_TO_INT(jv_value))  );
        }
        else if (
                JSVAL_IS_NUMBER(jv_value)
                || JSVAL_IS_DOUBLE(jv_value)
                )
        {
            jsdouble d = 0;
            SG_JS_BOOL_CHECK(  JS_ValueToNumber(cx, jv_value, &d)  );
            SG_ERR_CHECK(  SG_varray__append__double(pCtx, pva, (double) d)  );
        }
        else if (JSVAL_IS_BOOLEAN(jv_value))
        {
            SG_ERR_CHECK(  SG_varray__append__bool(pCtx, pva, JSVAL_TO_BOOLEAN(jv_value))  );
        }
        else if (JSVAL_IS_NULL(jv_value))
        {
            SG_ERR_CHECK(  SG_varray__append__null(pCtx, pva)  );
        }
        else if (JSVAL_IS_OBJECT(jv_value))
        {
            JSObject* po = JSVAL_TO_OBJECT(jv_value);

            if (JS_IsArrayObject(cx, po))
            {
                SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, po, &pva_sub)  );

                SG_ERR_CHECK(  SG_varray__append__varray(pCtx, pva, &pva_sub)  );
            }
            else
            {
                SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, po, &pvh_sub)  );

                SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva, &pvh_sub)  );
            }
        }
        else if (JSVAL_IS_VOID(jv_value))
        {
			SG_ERR_THROW( SG_ERR_JS );
        }
        else
        {
            SG_ERR_THROW( SG_ERR_JS );
        }
    }

    *ppva = pva;
    pva = NULL;

	return;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_sub);
    SG_VARRAY_NULLFREE(pCtx, pva_sub);
	SG_VARRAY_NULLFREE(pCtx, pva);
}

void sg_jsglue__jsval_to_json(SG_context * pCtx, JSContext * cx, jsval obj, SG_bool bPretty, SG_string ** ppstr)
{
	SG_vhash* pvh = NULL;
    SG_varray* pva = NULL;
	SG_string * pstr = NULL;
	char * psz = NULL;
	JSObject * jso = NULL;
	JSString * pjs = NULL;
	
	
	char buf[256];
	
	if (JSVAL_IS_NULL(obj))
	{
		SG_ERR_CHECK( SG_STRING__ALLOC__RESERVE(pCtx, &pstr, 4) );
		SG_ERR_CHECK( SG_string__set__sz(pCtx, pstr, "null") );
	}
	else if (JSVAL_IS_VOID(obj))
	{
		SG_ERR_CHECK( SG_STRING__ALLOC__RESERVE(pCtx, &pstr, 9) );
		SG_ERR_CHECK( SG_string__set__sz(pCtx, pstr, "undefined") );
	}
	else if (JSVAL_IS_BOOLEAN(obj))
	{
		SG_bool bValue = JSVAL_TO_BOOLEAN(obj);

		if (bValue)
		{
			SG_ERR_CHECK( SG_STRING__ALLOC__RESERVE(pCtx, &pstr, 4));
			SG_ERR_CHECK( SG_string__set__sz(pCtx, pstr, "true") );
		}
		else
		{
			SG_ERR_CHECK( SG_STRING__ALLOC__RESERVE(pCtx, &pstr, 5) );
			SG_ERR_CHECK( SG_string__set__sz(pCtx, pstr, "false") );
		}
	}
	else if(JSVAL_IS_OBJECT(obj))
	{
		jso = JSVAL_TO_OBJECT(obj);

		if (JS_IsArrayObject(cx, jso))
		{
			SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso, &pva)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
			if (bPretty)
				SG_ERR_CHECK(  SG_varray__to_json__pretty_print_NOT_for_storage(pCtx, pva, pstr)  );
			else
				SG_ERR_CHECK(  SG_varray__to_json(pCtx, pva, pstr)  );
				
			SG_VARRAY_NULLFREE(pCtx, pva);
		}
		else
		{
			SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, jso, &pvh)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
			if (bPretty)
				SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh, pstr)  );
			else
				SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );
				
			SG_VHASH_NULLFREE(pCtx, pvh);
		}
	}
	else if(JSVAL_IS_STRING(obj))
	{
		pjs = JSVAL_TO_STRING(obj);
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, pjs, &psz)  );
		SG_ERR_CHECK(  SG_sz__to_json(pCtx, psz, &pstr)  );
	}
	else if(JSVAL_IS_INT(obj))
	{
		SG_int_to_string_buffer tmp;
		SG_int64 i = JSVAL_TO_INT(obj);
		
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pstr, SG_int64_to_sz(i, tmp)) );
	}
	else if(JSVAL_IS_NUMBER(obj) || JSVAL_IS_DOUBLE(obj))
	{
		jsdouble d = 0;
		SG_JS_BOOL_CHECK(  JS_ValueToNumber(cx, obj, &d)  );
		SG_ERR_CHECK(  SG_sprintf(pCtx, buf, 255, "%f", (double) d)  );
		{
			char* p = buf;
			while (*p)
		    {
				if (',' == *p)
				{
					*p = '.';
					break;
				}
				p++;
		    }
		}
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, buf) );
	}
	else
	{
		SG_ERR_THROW(SG_ERR_VALUE_CANNOT_BE_SERIALIZED_TO_JSON);
	}
	
	SG_NULLFREE(pCtx, psz);

	*ppstr = pstr;
	pstr = NULL;
	return;
	
fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_NULLFREE(pCtx, psz);
}

void sg_jsglue__jsstring_to_sz(SG_context *pCtx, JSContext *cx, JSString *str, char **ppOut)
{
	char *pOut = NULL;
	size_t len = 0;

	SG_ASSERT(pCtx);	
	SG_NULLARGCHECK_RETURN(cx);
	SG_NULLARGCHECK_RETURN(str);
	SG_NULLARGCHECK_RETURN(ppOut);
	
	len = JS_GetStringEncodingLength(cx, str);
	SG_ARGCHECK_RETURN(len+1<=SG_UINT32_MAX, str);
	SG_ERR_CHECK_RETURN(  SG_alloc(pCtx, (SG_uint32)(len+1), 1, &pOut)  );
#if !defined(DEBUG) // RELEASE
	(void)JS_EncodeStringToBuffer(str, pOut, len);
#else // DEBUG
	// Same as RELEASE, but asserts length returned by JS_EncodeStringToBuffer()
	// was the same as length returned by JS_GetStringEncodingLength(). I think
	// this is supposed to be true, but the documentation is a bit unclear. I'm
	// interpreting the first sentence to mean "the length of the specified
	// string in bytes *when UTF-8 encoded*, regardless of its *current*
	// encoding." in this exerpt from the documentation:
	//
	// "JS_GetStringEncodingLength() returns the length of the specified string
	// in bytes, regardless of its encoding. You can use this value to create a
	// buffer to encode the string into using the JS_EncodeStringToBuffer()
	// function, which fills the specified buffer with up to length bytes of the
	// string in UTF-8 format. It returns the length of the whole string
	// encoding or -1 if the string can't be encoded as bytes. If the returned
	// value is greater than the length you specified, the string was
	// truncated."
	// -- https://developer.mozilla.org/en/JS_GetStringBytes (2012-07-13)
	{
		size_t len2 = 0;
		len2 = JS_EncodeStringToBuffer(str, pOut, len);
		SG_ASSERT(len2==len);
	}
#endif
	SG_ASSERT(pOut[len]=='\0');
	*ppOut = pOut;
}

void sg_jsglue__jsobject_to_vhash(SG_context * pCtx, JSContext* cx, JSObject* obj, SG_vhash** ppvh)
{
    JSIdArray* properties = NULL;
    SG_vhash* pvh = NULL;
    SG_int32 i;
    SG_vhash* pvh_sub = NULL;
    SG_varray* pva_sub = NULL;
    char name_buff[260];
    char *pTmpBuf = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

    SG_JS_NULL_CHECK(  properties = JS_Enumerate(cx, obj)  );

    for (i=0; i<properties->length; i++)
    {
        jsval jv_name;
        jsval jv_value;
        char* str_name;

        SG_JS_BOOL_CHECK(  JS_IdToValue(cx, properties->vector[i], &jv_name)  );
        if(JSVAL_IS_INT(jv_name))
        {
            // Keys may have been converted to type integer if the string was an integer
            // (Even with objects made by sg_jsglue__copy_vhash_into_jsobject(), which
            // inserts values using JS_SetProperty(), which takes a char * for the key!).
            SG_int64_to_sz(JSVAL_TO_INT(jv_name), name_buff);
            str_name = name_buff;
        }
        else
        {
        	size_t len;
            SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(jv_name)  );
            len = JS_EncodeStringToBuffer(JSVAL_TO_STRING(jv_name), name_buff, sizeof(name_buff));
            if(len<sizeof(name_buff))
            {
            	name_buff[len] = '\0';
            	str_name = name_buff;
            }
            else
            {
                SG_ARGCHECK(len+1<=SG_UINT32_MAX, obj);
            	SG_ERR_CHECK(  SG_alloc(pCtx,(SG_uint32)(len+1),1,&pTmpBuf)  );
            	(void)JS_EncodeStringToBuffer(JSVAL_TO_STRING(jv_name), pTmpBuf, len);
            	SG_ASSERT(pTmpBuf[len]=='\0');
            	str_name = pTmpBuf;
            }
        }

        SG_JS_BOOL_CHECK(  JS_GetProperty(cx, obj, str_name, &jv_value)  );

        if (JSVAL_IS_STRING(jv_value))
        {
            SG_ERR_CHECK(  SG_vhash__add__string__jsstring(pCtx, pvh, str_name, cx, JSVAL_TO_STRING(jv_value))  );
        }
        else if (JSVAL_IS_INT(jv_value))
        {
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, str_name, (SG_int64) JSVAL_TO_INT(jv_value))  );
        }
        else if (
                JSVAL_IS_NUMBER(jv_value)
                || JSVAL_IS_DOUBLE(jv_value)
                )
        {
            jsdouble d = 0;
            SG_JS_BOOL_CHECK(  JS_ValueToNumber(cx, jv_value, &d)  );
            SG_ERR_CHECK(  SG_vhash__add__double(pCtx, pvh, str_name, (double) d)  );
        }
        else if (JSVAL_IS_BOOLEAN(jv_value))
        {
            SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh, str_name, JSVAL_TO_BOOLEAN(jv_value))  );
        }
        else if (JSVAL_IS_NULL(jv_value))
        {
            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh, str_name)  );
        }
        else if (JSVAL_IS_OBJECT(jv_value))
        {
            JSObject* po = JSVAL_TO_OBJECT(jv_value);

            if (JS_IsArrayObject(cx, po))
            {
                SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, po, &pva_sub)  );
                SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh, str_name, &pva_sub)  );

            }
            else
            {
                SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, po, &pvh_sub)  );

                SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, str_name, &pvh_sub)  );
            }
        }
        else if (JSVAL_IS_VOID(jv_value))
        {
            SG_ERR_THROW2(  SG_ERR_JS,
                           (pCtx, "%s cannot be undefined/void", str_name )  );
        }
        else
        {
            SG_ERR_THROW( SG_ERR_JS );
        }
	    SG_NULLFREE(pCtx, pTmpBuf);
    }

    JS_DestroyIdArray(cx, properties);

    *ppvh = pvh;
    pvh = NULL;

	return;

fail:
    JS_DestroyIdArray(cx, properties);
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VHASH_NULLFREE(pCtx, pvh_sub);
    SG_VARRAY_NULLFREE(pCtx, pva_sub);
    SG_NULLFREE(pCtx, pTmpBuf);
}

SG_bool sg_jsglue_get_equals_flag(const char* psz_flag, const char* psz_match, const char** psz_value)
{
    int len = SG_STRLEN(psz_match);
    if (
            (0 == memcmp(psz_flag, psz_match, len))
            && ('=' == psz_flag[len])
        )
    {
        *psz_value = psz_flag + len + 1;
        return SG_TRUE;
    }

    return SG_FALSE;
}


void sg_jsglue_get_pattern_array(SG_context * pCtx, const char* psz_patterns, SG_stringarray** psaPatterns)
{
	char** pppResults = NULL;
	SG_uint32 i;
	SG_uint32 ncount = 0;


	// TODO 2010/06/16 From what I can tell by skimming, this code is intended to
	// TODO            allow the caller to receive "exclude=a,b,c" and we cut these
	// TODO            apart (breaking on the comma) into individual strings and
	// TODO            append them to the array (which we allocate if necessary).
	// TODO
	// TODO            Do we REALLY want to do this?
	// TODO
	// TODO            The callers are set up to allow an arg array containing
	// TODO            something like [ "exclude=a,b,c", "exclude=*.h", ...],
	// TODO            so they could get the same effect with the multiple n/v pairs.
	// TODO
	// TODO            Commas are valid characters in filenames and patterns, so
	// TODO            this prohibits us from, for example, excluding **,v" files.

	SG_ERR_CHECK (  SG_string__split__sz_asciichar(pCtx, psz_patterns, ',', SG_STRLEN(psz_patterns) + 1, &pppResults, &ncount) );
	if (!(*psaPatterns))
	{
		SG_ERR_CHECK (  SG_STRINGARRAY__ALLOC(pCtx, psaPatterns, ncount) );
	}
	for(i = 0; i < ncount; i++)
	{
		SG_ERR_CHECK (  SG_stringarray__add(pCtx, *psaPatterns, pppResults[i])  );
	}


fail:
	if (pppResults)
	{
		for(i = 0; i < ncount; i++)
			SG_NULLFREE(pCtx, pppResults[i]);
		SG_NULLFREE(pCtx, pppResults);
	}
}

static void sg_jsglue__get_repo_from_jsval(SG_context* pCtx, JSContext* cx, jsval arg, SG_repo** ppRepo)
{
	SG_repo* pRepo = NULL;
	char * szRepoName = NULL;
	SG_bool isAvailable = SG_TRUE;

	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(arg));
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(arg), &szRepoName)  );
	
	SG_ERR_CHECK(  sg_uridispatch__query_repo_availability(pCtx, szRepoName, &isAvailable)  );
	if(!isAvailable)
		SG_ERR_THROW(SG_ERR_NOTAREPOSITORY);

	SG_ERR_CHECK(  SG_jscore__check_module_dags(pCtx, cx, JS_GetGlobalObject(cx), szRepoName)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, szRepoName, &pRepo)  );

	SG_RETURN_AND_NULL(pRepo, ppRepo);

	/* fall through */
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, szRepoName);
}

static void sg_jsglue__get_repo_from_cwd(SG_context* pCtx, SG_repo** ppRepo, SG_pathname** ppPathCwd, SG_pathname** ppPathWorkingDirectoryTop)
{
	SG_pathname * pPathCwd = NULL;
	SG_repo * pRepo = NULL;
	SG_pathname * pPathWorkingDirectoryTop = NULL;
	SG_string * pstrRepoDescriptorName = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathCwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathCwd)  );
	SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPathCwd, &pPathWorkingDirectoryTop, &pstrRepoDescriptorName, NULL)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstrRepoDescriptorName), &pRepo)  );

	SG_RETURN_AND_NULL(pPathCwd, ppPathCwd);
	SG_RETURN_AND_NULL(pPathWorkingDirectoryTop, ppPathWorkingDirectoryTop);
	SG_RETURN_AND_NULL(pRepo, ppRepo);

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDirectoryTop);
	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
}

static void _sg_jsglue__get_password(SG_context * pCtx, const char* psz_username, const char* psz_password_param, SG_string** pp_password)
{
	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(psz_username!=NULL);
	SG_ASSERT(psz_password_param!=NULL);
	SG_ASSERT(pp_password!=NULL);
	if(strcmp(psz_password_param, "PROMPT")==0)
	{
		SG_ERR_CHECK_RETURN(  SG_console(pCtx, SG_CS_STDERR, "Enter password for %s: ", psz_username)  );
		SG_ERR_CHECK_RETURN(  SG_console__get_password(pCtx, pp_password)  );
	}
	else if(SG_sz__starts_with(psz_password_param, "PROMPT:"))
	{
		SG_ERR_CHECK_RETURN(  SG_console(pCtx, SG_CS_STDERR, "%s", psz_password_param+7)  );
		SG_ERR_CHECK_RETURN(  SG_console__get_password(pCtx, pp_password)  );
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_STRING__ALLOC__SZ(pCtx, pp_password, psz_password_param)  );
	}
}

static char* sg_jsglue__getObjectType(SG_treenode_entry_type type)
{
	if (type == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		return "File";
	else if (type == SG_TREENODEENTRY_TYPE_DIRECTORY)
			return "Directory";
	else if (type == SG_TREENODEENTRY_TYPE_SYMLINK)
			return "Symlink";
	else
		return "Invalid";
}

static void sg_jsglue__treenode_entry_hash( SG_context * pCtx,
	 SG_repo* pRepo,
	 const char* pszChangeSetID,
	 SG_treenode_entry* pTreenodeEntry,
	 const char* pszPath,
	 const char* pszGid,
	 SG_vhash** ppvhResults )
{
	SG_string* content = NULL;
	SG_uint32 count;
	SG_uint32 k;
	const char* pszHid = NULL;
	const char* pszName = NULL;
	SG_treenode_entry_type ptneType;
	SG_vhash* pvhResults = NULL;
	SG_vhash* pvhChild = NULL;
	SG_varray* pvaChildren = NULL;
	SG_treenode* pTreenode = NULL;
	SG_string* pStrPath = NULL;

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTreenodeEntry, &ptneType)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &content)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResults)  );
	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaChildren)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTreenodeEntry, &pszHid)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTreenodeEntry, &pszName)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, "name", pszName) );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, "changeset_id", pszChangeSetID) );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, "path", pszPath) );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, "gid", pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, "type", sg_jsglue__getObjectType(ptneType))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, "hid", pszHid)  );


	if (ptneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHid, &pTreenode)  );

		SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );

		for (k=0; k < count; k++)
		{
			const char* szGidObject = NULL;
			const char* pszCurHid = NULL;
			const char* pszName = NULL;
			SG_treenode_entry_type ptneCurType;
			const SG_treenode_entry* ptneChild;

			SG_VHASH__ALLOC(pCtx, &pvhChild);
			SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, k, &szGidObject, &ptneChild)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChild, "gid", szGidObject)  );

			SG_treenode_entry__get_hid_blob(pCtx, ptneChild, &pszCurHid);
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChild, "hid", pszCurHid)  );

			SG_treenode_entry__get_entry_type(pCtx, ptneChild, &ptneCurType);
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChild, "type", sg_jsglue__getObjectType(ptneCurType))  );

			SG_treenode_entry__get_entry_name(pCtx, ptneChild, &pszName);
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChild, "name", pszName)  );

			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStrPath, pszPath)  );
			SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pStrPath, pszName, SG_FALSE) );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChild, "path", SG_string__sz(pStrPath))  );

			SG_varray__append__vhash(pCtx, pvaChildren, &pvhChild);

			SG_VHASH_NULLFREE(pCtx, pvhChild);
			SG_STRING_NULLFREE(pCtx, pStrPath);
		}
	}
	SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResults, "contents", &pvaChildren) );
	pvaChildren = NULL;
    *ppvhResults = pvhResults;
	pvhResults = NULL;
fail:
	SG_VHASH_NULLFREE(pCtx, pvhResults);
	SG_STRING_NULLFREE(pCtx, content);
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
    SG_VARRAY_NULLFREE(pCtx, pvaChildren);
	SG_VHASH_NULLFREE(pCtx, pvhChild);
	SG_STRING_NULLFREE(pCtx, pStrPath);

}


static void sg_jsglue__generic_file_response__context__alloc(
	SG_context* pCtx,
	SG_file* pFile,
	SG_pathname* pPathFile,
	SG_bool bDeleteWhenDone,
	SG_generic_file_response_context** ppState)
{
	SG_generic_file_response_context* pState = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pState)  );
	pState->pFile = pFile;
	pState->pPathFile = pPathFile;
	pState->bDeleteWhenDone = bDeleteWhenDone;

	SG_RETURN_AND_NULL(pState, ppState);
fail:
	SG_NULLFREE(pCtx, pState);
}

static void sg_jsglue__generic_file_response__context__free(SG_context* pCtx, SG_generic_file_response_context* pState)
{
	if (pState)
	{
		if (pState->bDeleteWhenDone && pState->pPathFile)
		{
			SG_ERR_IGNORE(  SG_file__close(pCtx, &pState->pFile)  );
			SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pState->pPathFile)  );
		}

		SG_FILE_NULLCLOSE(pCtx, pState->pFile);
		SG_PATHNAME_NULLFREE(pCtx, pState->pPathFile);
		SG_NULLFREE(pCtx, pState);
	}
}
#define _GENERIC_FILE_RESPONSE__CONTEXT__NULLFREE(pCtx,p) SG_STATEMENT(SG_context__push_level(pCtx); sg_jsglue__generic_file_response__context__free(pCtx,p); SG_context__pop_level(pCtx); p=NULL;)
/* ---------------------------------------------------------------- */
/* JSNative method definitions */
/* ---------------------------------------------------------------- */

SG_JSGLUE_METHOD_PROTOTYPE(fs, length)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_uint64 i;
    SG_fsobj_type t;
    jsval jv;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &i, &t)  );

    SG_ERR_CHECK(  sg_jsglue__int64_to_jsval(pCtx, cx, i, &jv)  );

    /* TODO verify t */

    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, jv);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, mkdir)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_bool b = SG_FALSE;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

	//TODO:  This currently throws an error (stopping javascript execution)
    //if there's something there.  Should it just return false?

    SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );
	b = SG_TRUE;
    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(b));

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, rmdir)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_bool b = SG_FALSE;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

	//TODO:  This currently throws an error (stopping javascript execution)
    //if there is not something there.  Should it just return false?

    SG_ERR_CHECK(  SG_fsobj__rmdir__pathname(pCtx, pPath)  );

	b = SG_TRUE;
    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(b));

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, mkdir_recursive)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_bool b = SG_FALSE;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

    //TODO:  This currently throws an error (stopping javascript execution)
    //if there's something there.  Should it just return false?

    SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPath)  );

    b = SG_TRUE;
    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(b));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, rmdir_recursive)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_bool b = SG_FALSE;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );
    //TODO:  This currently throws an error (stopping javascript execution)
    //if there is not something there.  Should it just return false?
    SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath)  );
    b = SG_TRUE;
    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(b));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, exists)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    char* psz_arg = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b = SG_FALSE;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_arg)  );
    if (strlen(psz_arg) != 0)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, psz_arg)  );

    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b, NULL, NULL)  );

    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_NULLFREE(pCtx, psz_arg);

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(b));

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_NULLFREE(pCtx, psz_arg);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, remove)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

    SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );

    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, move)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_pathname* pNewPath = NULL;

    SG_JS_BOOL_CHECK(  2 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pNewPath, cx, JSVAL_TO_STRING(argv[1]))  );

    SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath, pNewPath)  );

    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_PATHNAME_NULLFREE(pCtx, pNewPath);

    JS_SET_RVAL(cx, vp, JSVAL_TRUE);

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_PATHNAME_NULLFREE(pCtx, pNewPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, getcwd)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_string* pPath = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  0 == argc  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__getcwd(pCtx, pPath)  );

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pPath)))  );
    SG_STRING_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_STRING_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, getcwd_top)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_pathname* pPathCwd = NULL;
    SG_pathname* pPathRepoTop = NULL;
    SG_string* pstrRepo = NULL;
    char* pszgid = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  0 == argc  );
   
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathCwd)  );    
    SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathCwd)  );  	
    SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPathCwd, &pPathRepoTop, &pstrRepo, &pszgid));
	
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);    
    SG_NULLFREE(pCtx, pszgid);  
    SG_STRING_NULLFREE(pCtx, pstrRepo);

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(pPathRepoTop)))  );
  
    SG_PATHNAME_NULLFREE(pCtx, pPathRepoTop);
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_STRING_NULLFREE(pCtx, pstrRepo);
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_PATHNAME_NULLFREE(pCtx, pPathRepoTop);
    SG_NULLFREE(pCtx, pszgid); 
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, tmpdir)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    JSString* pjs = NULL;
    SG_pathname *tmppath = NULL;

    SG_JS_BOOL_CHECK(  0 == argc  );

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &tmppath)  );

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(tmppath)))  );
    SG_PATHNAME_NULLFREE(pCtx, tmppath);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, tmppath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, cd)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );

	pjs = JSVAL_TO_STRING(argv[0]);

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, pjs)  );
    SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath)   );
    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, chmod)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_bool b = SG_FALSE;
    SG_fsobj_perms perms = 0;

    SG_JS_BOOL_CHECK(  2 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[1])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );
    perms = JSVAL_TO_INT(argv[1]);

    //TODO:  This currently throws an error (stopping javascript execution)
    //if there's something there.  Should it just return false?

    SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPath, perms )  );

    b = SG_TRUE;
    SG_PATHNAME_NULLFREE(pCtx, pPath);

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(b));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, stat)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
    SG_fsobj_stat FsObjStat;
    int iIsFile;
    int iIsDir;
    int iIsSymlink;
    jsval perms;
    jsval size;
    jsval dateval;
    jsval isFile;
    jsval isDirectory;
    jsval isSymlink;
    JSObject* object = NULL;
    JSObject* dateObject = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

    SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &FsObjStat)  );

    SG_JS_NULL_CHECK(  object = JS_NewObject(cx, &fsobj_properties_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(object));

    //Permissions are in an int.
    perms = INT_TO_JSVAL(FsObjStat.perms);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "permissions", &perms)  );

    //Create a Javascript Date object for the modtime.
    SG_JS_NULL_CHECK(  dateObject = JS_NewDateObjectMsec(cx, (jsdouble)FsObjStat.mtime_ms)  );
    dateval = OBJECT_TO_JSVAL(dateObject);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "modtime", &dateval )  );

	SG_ERR_CHECK(  sg_jsglue__int64_to_jsval(pCtx, cx, FsObjStat.size, &size)   );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "size", &size)  );

	iIsFile = FsObjStat.type == SG_FSOBJ_TYPE__REGULAR;
	iIsDir = FsObjStat.type == SG_FSOBJ_TYPE__DIRECTORY;
	iIsSymlink = FsObjStat.type == SG_FSOBJ_TYPE__SYMLINK;
	isFile = BOOLEAN_TO_JSVAL(iIsFile);
	isDirectory = BOOLEAN_TO_JSVAL(iIsDir);
	isSymlink = BOOLEAN_TO_JSVAL(iIsSymlink);

	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "isFile", &isFile)  );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "isDirectory", &isDirectory)  );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "isSymlink", &isSymlink)  );

    SG_PATHNAME_NULLFREE(pCtx, pPath);

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

typedef struct
{
	JSObject *listObject;
	JSContext *cx;
} _readdirEachContext;

static void _readdirEach(
	SG_context *      pCtx,             //< [in] [out] Error and context info.
	const SG_string * pStringEntryName, //< [in] Name of the current entry.
	SG_fsobj_stat *   pfsStat,          //< [in] Stat data for the current entry (only if SG_DIR__FOREACH__STAT is used).
	void *            pVoidData)        //< [in] Data provided by the caller of SG_dir__foreach.
{
	_readdirEachContext *ctx = (_readdirEachContext *)pVoidData;
	JSObject *listObject = ctx->listObject;
	JSContext *cx = ctx->cx;
	JSObject *object = NULL;
    int iIsFile;
    int iIsDir;
    int iIsSymlink;
    jsval perms;
    jsval size;
    jsval dateval;
    jsval isFile;
    jsval isDirectory;
	jsval isSymlink;
	JSObject* dateObject = NULL;
	jsuint length;
	jsval v;
	jsval name;
	JSString *pjstr;

    SG_JS_NULL_CHECK(  object = JS_NewObject(cx, &fsobj_properties_class, NULL, NULL)  );
	v = OBJECT_TO_JSVAL(object);

	// Root new object now to avoid GC.
	SG_JS_BOOL_CHECK(  JS_GetArrayLength(cx, listObject, &length)  );
	SG_JS_BOOL_CHECK(  JS_SetElement(cx, listObject, length, &v)  );

    //Permissions are in an int.
    perms = INT_TO_JSVAL(pfsStat->perms);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "permissions", &perms)  );

    //Create a Javascript Date object for the modtime.
    SG_JS_NULL_CHECK(  dateObject = JS_NewDateObjectMsec(cx, (jsdouble)pfsStat->mtime_ms)  );
    dateval = OBJECT_TO_JSVAL(dateObject);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "modtime", &dateval )  );

	SG_ERR_CHECK(  sg_jsglue__int64_to_jsval(pCtx, cx, pfsStat->size, &size)   );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "size", &size)  );

	iIsFile = pfsStat->type == SG_FSOBJ_TYPE__REGULAR;
	iIsDir = pfsStat->type == SG_FSOBJ_TYPE__DIRECTORY;
	iIsSymlink = pfsStat->type == SG_FSOBJ_TYPE__SYMLINK;
	isFile = BOOLEAN_TO_JSVAL(iIsFile);
	isDirectory = BOOLEAN_TO_JSVAL(iIsDir);
	isSymlink = BOOLEAN_TO_JSVAL(iIsSymlink);

    SG_JS_NULL_CHECK(  (pjstr = JS_NewStringCopyZ(cx, SG_string__sz(pStringEntryName)))  );

	name = STRING_TO_JSVAL(pjstr);

	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "isFile", &isFile)  );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "isDirectory", &isDirectory)  );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "isSymlink", &isSymlink)  );
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, object, "name", &name)  );

 fail:
	;
}


SG_JSGLUE_METHOD_PROTOTYPE(fs, readdir)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_pathname* pPath = NULL;
	JSObject* listObject = NULL;
	_readdirEachContext ctx;

    SG_JS_BOOL_CHECK(  argc == 1 );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

	SG_JS_NULL_CHECK(  listObject = JS_NewArrayObject(cx, 0, NULL)  );
	// immediately cause the new object to be referenced
	// so that the GC doesn't kill it while we populate
	// the fields within it.  (yes, this is subtle.)
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(listObject));

	ctx.listObject = listObject;
	ctx.cx = cx;

	SG_ERR_CHECK(  SG_dir__foreach(pCtx,
								   pPath,
								   SG_DIR__FOREACH__STAT
								   | SG_DIR__FOREACH__SKIP_SELF
								   | SG_DIR__FOREACH__SKIP_PARENT
								   | SG_DIR__FOREACH__SKIP_SGDRAWER,
								   _readdirEach,
								   &ctx)  );

    SG_PATHNAME_NULLFREE(pCtx, pPath);

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fs, ls)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* psz_match_begin = NULL;
	char* psz_match_anywhere = NULL;
	char* psz_match_end = NULL;
	SG_pathname* pPath = NULL;
	JSObject* listObject = NULL;
	SG_rbtree* prbResults = NULL;
	SG_rbtree_iterator* pit = NULL;

	SG_JS_BOOL_CHECK(  argc > 0 && argc < 5);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

	if (argc >= 2 && !JSVAL_IS_NULL(argv[1]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_match_begin)  );
	}

	if (argc >= 3 && !JSVAL_IS_NULL(argv[2]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_match_anywhere)  );
	}

	if (argc == 4 && !JSVAL_IS_NULL(argv[3]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &psz_match_end)  );
	}

	SG_JS_NULL_CHECK(  listObject = JS_NewArrayObject(cx, 0, NULL)  );
	// immediately cause the new object to be referenced
	// so that the GC doesn't kill it while we populate
	// the fields within it.  (yes, this is subtle.)
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(listObject));

	SG_ERR_CHECK(  SG_dir__list(pCtx, pPath, psz_match_begin, psz_match_anywhere, psz_match_end, &prbResults)  );
	if (prbResults)
		SG_ERR_CHECK(  sg_jsglue__copy_rbtree_keys_into_jsobject(pCtx, cx, prbResults, listObject)  );

	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, psz_match_begin);
	SG_NULLFREE(pCtx, psz_match_anywhere);
	SG_NULLFREE(pCtx, psz_match_end);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	SG_RBTREE_NULLFREE(pCtx, prbResults);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, psz_match_begin);
	SG_NULLFREE(pCtx, psz_match_anywhere);
	SG_NULLFREE(pCtx, psz_match_end);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	SG_RBTREE_NULLFREE(pCtx, prbResults);
	return JS_FALSE;
}

///sg.fs.fetch_file(path, chunksize, deletewhendone) 
SG_JSGLUE_METHOD_PROTOTYPE(fs, fetch_file)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_safeptr* psp_ffh = NULL;
    JSObject* jso = NULL;

    SG_int_to_string_buffer buf_uint64;
    SG_generic_file_response_context* pResponseCtx = NULL;
    SG_uint64 length = 0;
    SG_file* pFile = NULL;
    SG_pathname* pPathFile = NULL;
    SG_bool bDeleteOnSuccessfulFinish;
    jsval jv;
        
    SG_JS_BOOL_CHECK(  argc == 3 );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[1])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[2])  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPathFile, cx, JSVAL_TO_STRING(argv[0]))  );
    bDeleteOnSuccessfulFinish = JSVAL_TO_BOOLEAN(argv[2]);

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, &length)  );

    SG_ERR_CHECK(  sg_jsglue__generic_file_response__context__alloc(pCtx, pFile, pPathFile, bDeleteOnSuccessfulFinish, &pResponseCtx)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, &sg_fetchfilehandle_class, NULL, NULL))  );  
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  SG_safeptr__wrap__fetchfilehandle(pCtx, pResponseCtx, &psp_ffh)  );
    JS_SetPrivate(cx, jso, psp_ffh);
        
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "chunk_length", &argv[1])  );

    JSVAL_FROM_SZ(jv, "0");
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "processed_length", &jv)  );

    JSVAL_FROM_SZ(jv, SG_uint64_to_sz(length, buf_uint64));
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "total_length", &jv)  );

    pFile = NULL;
    pPathFile = NULL;
    pResponseCtx = NULL;
    
    return JS_TRUE;
 fail:
    SG_jsglue__report_sg_error(pCtx, cx);     
    SG_FILE_NULLCLOSE(pCtx, pFile);
    _GENERIC_FILE_RESPONSE__CONTEXT__NULLFREE(pCtx, pResponseCtx);
    SG_SAFEPTR_NULLFREE(pCtx, psp_ffh); 
    SG_PATHNAME_NULLFREE(pCtx, pPathFile);
    return JS_FALSE;
}



/**
 * fetchfilehandle.next_chunk() returns a cbuffer.
 * A cbuffer is essentially an object that is meant to be returned back down to
 * (or passed back up into) C code, without having to be converted back and
 * forth between a JavaScript native type/object. See sg_cbuffer_typedefs.h.
 * 
 * If *don't* pass the returned cbuffer back to C code, you can free it
 * directly from javascript by calling its ".fin()" member function.
 */
SG_JSGLUE_METHOD_PROTOTYPE(fetchfilehandle, next_chunk)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_safeptr* psp_ffh = NULL;
    SG_generic_file_response_context* pContext = NULL;
	
	SG_safeptr* psp_cbuffer = NULL;
	SG_cbuffer * pCbuffer = NULL;
	
	jsval val;   
    jsval jv;
	SG_uint32 chunk_length = 0;
	JSObject * jso = NULL;
    SG_uint64 processed_length = 0;
    SG_uint64 total_length = 0;
    SG_int_to_string_buffer int_as_string_buffer;
    size_t len = 0; // Only used for asserts.
    
    SG_JS_BOOL_CHECK(argc==0);

	psp_ffh = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_safeptr__unwrap__fetchfilehandle(pCtx, psp_ffh, &pContext);
	if(SG_context__err_equals(pCtx, SG_ERR_SAFEPTR_NULL))
	{
		SG_context__err_reset(pCtx);
		JS_SET_RVAL(cx, vp, JSVAL_NULL); // Return null after the last chunk has been sent.
		return JS_TRUE;
	}
	else
		SG_ERR_CHECK_CURRENT;    	

	SG_JS_BOOL_CHECK(  JS_GetProperty(cx, JS_THIS_OBJECT(cx, vp), "chunk_length", &val)  );
	chunk_length = JSVAL_TO_INT(val);

    SG_JS_BOOL_CHECK(  JS_GetProperty(cx, JS_THIS_OBJECT(cx, vp), "total_length", &val)  );
    SG_ASSERT(JSVAL_IS_STRING(val));
    len = JS_EncodeStringToBuffer(JSVAL_TO_STRING(val), int_as_string_buffer, sizeof(int_as_string_buffer));
    SG_ASSERT(len<sizeof(int_as_string_buffer));
    int_as_string_buffer[len]='\0';
    SG_ERR_CHECK(  SG_uint64__parse__strict(pCtx, &total_length, int_as_string_buffer)  );

    SG_JS_BOOL_CHECK(  JS_GetProperty(cx, JS_THIS_OBJECT(cx, vp), "processed_length", &val)  );
    SG_ASSERT(JSVAL_IS_STRING(val));
    len = JS_EncodeStringToBuffer(JSVAL_TO_STRING(val), int_as_string_buffer, sizeof(int_as_string_buffer));
    SG_ASSERT(len<sizeof(int_as_string_buffer));
    int_as_string_buffer[len]='\0';
    SG_ERR_CHECK(  SG_uint64__parse__strict(pCtx, &processed_length, int_as_string_buffer)  );

	SG_UNUSED(len);

	if(total_length-processed_length<(SG_uint64)chunk_length)
		chunk_length = (SG_uint32)(total_length-processed_length);
	SG_ERR_CHECK(  SG_cbuffer__alloc__new(pCtx, &pCbuffer, chunk_length)  );

    SG_ERR_CHECK(  SG_file__seek(pCtx, pContext->pFile, processed_length)  );

	SG_ERR_CHECK(  SG_file__read(pCtx, pContext->pFile, pCbuffer->len, pCbuffer->pBuf, NULL)  );
	processed_length+=chunk_length;
	if (processed_length >= total_length)
	{
		SG_SAFEPTR_NULLFREE(pCtx, psp_ffh);
		_GENERIC_FILE_RESPONSE__CONTEXT__NULLFREE(pCtx, pContext);
		JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);
	}
	JSVAL_FROM_SZ(jv, SG_uint64_to_sz(processed_length, int_as_string_buffer));
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, JS_THIS_OBJECT(cx, vp), "processed_length", &jv)  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_cbuffer_class, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  SG_safeptr__wrap__cbuffer(pCtx, pCbuffer, &psp_cbuffer)  );
	JS_SetPrivate(cx, jso, psp_cbuffer);       

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_SAFEPTR_NULLFREE(pCtx, psp_cbuffer);
	SG_cbuffer__nullfree(&pCbuffer);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fetchfilehandle, abort)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_safeptr* psp_ffh = NULL;
	SG_generic_file_response_context * pHandle = NULL;

    SG_JS_BOOL_CHECK(argc==0);
	
	psp_ffh = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_safeptr__unwrap__fetchfilehandle(pCtx, psp_ffh, &pHandle);
	if(SG_context__err_equals(pCtx, SG_ERR_SAFEPTR_NULL))
	{
		SG_context__err_reset(pCtx);
		JS_SET_RVAL(cx, vp, JSVAL_NULL); // We seem to have already aborted or finished.
		return JS_TRUE;
	}
	else
		SG_ERR_CHECK_CURRENT;

	_GENERIC_FILE_RESPONSE__CONTEXT__NULLFREE(pCtx, pHandle);

	SG_SAFEPTR_NULLFREE(pCtx, psp_ffh);
	JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
    _GENERIC_FILE_RESPONSE__CONTEXT__NULLFREE(pCtx, pHandle);
    SG_SAFEPTR_NULLFREE(pCtx, psp_ffh);
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}


extern SG_bool _sg_uridispatch__debug_remote_shutdown;

SG_JSGLUE_METHOD_PROTOTYPE(server, debug_shutdown)
{
	SG_UNUSED(cx);
	SG_UNUSED(argc);
	_sg_uridispatch__debug_remote_shutdown = SG_TRUE;
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

SG_JSGLUE_METHOD_PROTOTYPE(server, request_profiler_start)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	
	SG_JS_BOOL_CHECK(argc==1);
	SG_JS_BOOL_CHECK(JSVAL_IS_INT(argv[0]) && 0 <= JSVAL_TO_INT(argv[0]) && JSVAL_TO_INT(argv[0]) < SG_HTTPREQUESTPROFILER_CATEGORY__COUNT);

	SG_httprequestprofiler__start(JSVAL_TO_INT(argv[0]));

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(server, request_profiler_stop)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	
	SG_JS_BOOL_CHECK(argc==0);

	SG_httprequestprofiler__stop();

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, from_json)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    char* psz_arg = NULL;
    SG_vhash* pvh = NULL;
    SG_varray* pva = NULL;
    JSObject* jso = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_arg)  );

    SG_ERR_CHECK(  SG_veither__parse_json__buflen(pCtx, psz_arg, SG_STRLEN(psz_arg), &pvh, &pva)  );

    SG_ASSERT(pva || pvh);

    if (pva)
    {
        SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva, jso)  );
        SG_VARRAY_NULLFREE(pCtx, pva);
    }
    else if (pvh)
    {
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
        SG_VHASH_NULLFREE(pCtx, pvh);
    }

    SG_NULLFREE(pCtx, psz_arg);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_NULLFREE(pCtx, psz_arg);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, hash)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_varray *strlist = NULL;
    char leafhash[SG_HID_MAX_BUFFER_LENGTH];
    SGHASH_handle *hashHandle = NULL;
    SG_uint32 i, count;
    JSString* pjs = NULL;
    JSObject* jso = NULL;
	const char *hashalgo = "SHA1/160";
	char * hashalgo_param = NULL;

    SG_JS_BOOL_CHECK(  (argc >= 1) && (argc <= 2)  );
	
	if (JSVAL_IS_NULL(argv[0]))
	{
		// TODO how should null be hashed
		 SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);
	}
	else if(JSVAL_IS_OBJECT(argv[0]))
    {
        jso = JSVAL_TO_OBJECT(argv[0]);

        if (JS_IsArrayObject(cx, jso))
        {
            SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso, &strlist)  );
        }
        else
        {
            SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);//todo: numbers, bool, null
        }
    }
    else if(JSVAL_IS_STRING(argv[0]))
    {
        pjs = JSVAL_TO_STRING(argv[0]);
		SG_ERR_CHECK(  SG_varray__alloc(pCtx, &strlist)  );
		SG_ERR_CHECK(  SG_varray__append__jsstring(pCtx, strlist, cx, pjs)  );
    }
    else
    {
        SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);//todo: numbers, bool, null
    }

	if (argc == 2)
	{
		if (JSVAL_IS_STRING(argv[1]))
		{
			pjs = JSVAL_TO_STRING(argv[1]);
			SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, pjs, &hashalgo_param)  );
			hashalgo = hashalgo_param;
		}
		else
		{
			SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);//todo: numbers, bool, null
		}
	}

    SGHASH_init(hashalgo, &hashHandle);

    SG_ERR_CHECK(  SG_varray__count(pCtx, strlist, &count)  );

    for ( i = 0; i < count; ++i )
    {
	const char *st;
	
	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, strlist, i, &st)  );
	SGHASH_update(hashHandle, (const SG_byte *)st, (SG_uint32)(strlen(st) * sizeof(char)));
    }

    SGHASH_final(&hashHandle, leafhash, sizeof(leafhash));

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, leafhash))  );
    SG_VARRAY_NULLFREE(pCtx, strlist);

	SG_NULLFREE(pCtx, hashalgo_param);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VARRAY_NULLFREE(pCtx, strlist);
	SG_NULLFREE(pCtx, hashalgo_param);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, getenv)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    JSString* pjs = NULL;
	char *vari = NULL;
	SG_string *value = NULL;
	SG_uint32 len = 0;
	const char *valstr = "";

    SG_JS_BOOL_CHECK( argc == 1 );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) );
	
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &vari)  );

	SG_ERR_CHECK( SG_environment__get__str(pCtx, vari, &value, &len)  );

	if (len > 0)
		valstr = SG_string__sz(value);

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, valstr))  );
	SG_STRING_NULLFREE(pCtx, value);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	SG_NULLFREE(pCtx, vari);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_STRING_NULLFREE(pCtx, value);
	SG_NULLFREE(pCtx, vari);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, setenv)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *vari = NULL;
	char *valstr = NULL;

    SG_JS_BOOL_CHECK( argc == 2 );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[1]) );
	
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &vari)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &valstr)  );
	
	SG_ERR_CHECK( SG_environment__set__sz(pCtx, vari, valstr)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, vari);
	SG_NULLFREE(pCtx, valstr);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, vari);
	SG_NULLFREE(pCtx, valstr);
    return JS_FALSE;
}

void sg_closet__refresh_closet_location_from_environment_for_testing_purposes(SG_context * pCtx);
SG_JSGLUE_METHOD_PROTOTYPE(sg, refresh_closet_location_from_environment_for_testing_purposes)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_JS_BOOL_CHECK(argc==0);
	SG_ERR_CHECK(sg_closet__refresh_closet_location_from_environment_for_testing_purposes(pCtx)  );
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, to_json)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_string* pstr = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
	
	SG_ERR_CHECK(  sg_jsglue__jsval_to_json(pCtx, cx, argv[0], SG_FALSE, &pstr)  );

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pstr)))  );
    SG_STRING_NULLFREE(pCtx, pstr);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_STRING_NULLFREE(pCtx, pstr);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, to_json__pretty_print)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_string* pstr = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );

	SG_ERR_CHECK(  sg_jsglue__jsval_to_json(pCtx, cx, argv[0], SG_TRUE, &pstr)  );

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pstr)))  );
    SG_STRING_NULLFREE(pCtx, pstr);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_STRING_NULLFREE(pCtx, pstr);
    return JS_FALSE;
}

/**
 * sg.to_json__sz() returns a cbuffer.
 * A cbuffer is essentially an object that is meant to be returned back down to
 * (or passed back up into) C code, without having to be converted back and
 * forth between a JavaScript native type/object. See sg_cbuffer_typedefs.h.
 * 
 * If *don't* pass the returned cbuffer back to C code, you can free it
 * directly from javascript by calling its ".fin()" member function.
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg, to_json__sz)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_string* pstr = NULL;
    SG_cbuffer* pCbuffer = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
	
	SG_ERR_CHECK(  sg_jsglue__jsval_to_json(pCtx, cx, argv[0], SG_FALSE, &pstr) );

    SG_ERR_CHECK(  SG_cbuffer__alloc__string(pCtx, &pCbuffer, &pstr)  );
	pstr = NULL;	// cbuffer owns it now

    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_cbuffer_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  SG_safeptr__wrap__cbuffer(pCtx, pCbuffer, &psp)  );
    JS_SetPrivate(cx, jso, psp);

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_cbuffer__nullfree(&pCbuffer);
    SG_SAFEPTR_NULLFREE(pCtx, psp);
    return JS_FALSE;
}

/**
 * "to_sz" is somewhat of a misnomer because the string is not actually
 * zero-terminated. It probably should have been called "to_utf8".
 * 
 * This function returns a cbuffer:
 * A cbuffer is essentially an object that is meant to be returned back down to
 * (or passed back up into) C code, without having to be converted back and
 * forth between a JavaScript native type/object. See sg_cbuffer_typedefs.h.
 * 
 * If you *don't* pass the returned cbuffer back to C code, you can free it
 * directly from javascript by calling its ".fin()" member function.
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg, to_sz)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    JSString *str = NULL;
    size_t len;
    SG_cbuffer* pCbuffer = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;

    SG_JS_BOOL_CHECK(argc==1);
    SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
    str = JSVAL_TO_STRING(argv[0]);

	len = JS_GetStringEncodingLength(cx, str);
	SG_ARGCHECK(len<=SG_UINT32_MAX, argv[0]);
	SG_ERR_CHECK(  SG_cbuffer__alloc__new(pCtx, &pCbuffer, (SG_uint32)len)  );
#if !defined(DEBUG) // RELEASE
	(void)JS_EncodeStringToBuffer(str, (char*)pCbuffer->pBuf, len);
#else // DEBUG
	// Same as RELEASE, but asserts length returned by JS_EncodeStringToBuffer()
	// was the same as length returned by JS_GetStringEncodingLength(). I think
	// this is supposed to be true, but the documentation is a bit unclear. I'm
	// interpreting the first sentence to mean "the length of the specified
	// string in bytes *when UTF-8 encoded*, regardless of its *current*
	// encoding." in this exerpt from the documentation:
	//
	// "JS_GetStringEncodingLength() returns the length of the specified string
	// in bytes, regardless of its encoding. You can use this value to create a
	// buffer to encode the string into using the JS_EncodeStringToBuffer()
	// function, which fills the specified buffer with up to length bytes of the
	// string in UTF-8 format. It returns the length of the whole string
	// encoding or -1 if the string can't be encoded as bytes. If the returned
	// value is greater than the length you specified, the string was
	// truncated."
	// -- https://developer.mozilla.org/en/JS_GetStringBytes (2012-07-13)
	{
		size_t len2 = 0;
		len2 = JS_EncodeStringToBuffer(str, (char*)pCbuffer->pBuf, len);
		SG_ASSERT(len2==len);
	}
#endif

    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_cbuffer_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  SG_safeptr__wrap__cbuffer(pCtx, pCbuffer, &psp)  );
    JS_SetPrivate(cx, jso, psp);

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
    SG_cbuffer__nullfree(&pCbuffer);
    SG_SAFEPTR_NULLFREE(pCtx, psp);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, repoid_to_descriptors)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    char * pszRepoId = NULL;
	JSObject* jso = NULL;
	SG_vhash * pvhList = NULL;

    SG_JS_BOOL_CHECK(argc==1);
    SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));

    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszRepoId)  );

	// TODO 2011/01/27 allow pvec_use_submodules hints to be passed in.
	SG_ERR_CHECK(  SG_closet__reverse_map_repo_id_to_descriptor_names(pCtx, pszRepoId, NULL, &pvhList)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhList, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhList);
	SG_NULLFREE(pCtx, pszRepoId);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvhList);
	SG_NULLFREE(pCtx, pszRepoId);
    return JS_FALSE;
}

/**
 * Returns a JSObject containing all available descriptors, of any status, with status included.
 * This will also return a key to let you know if any repositories have been excluded 
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg, list_all_available_descriptors)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	JSObject* jso = NULL;
	SG_vhash* pvhList = NULL;	
    SG_vhash* pvhResult = NULL;
	
	SG_uint32 i = 0;
    SG_uint32 total = 0;
	SG_uint32 numDescriptors = 0;

	SG_JS_BOOL_CHECK(argc==0);

	SG_ERR_CHECK(  SG_closet__descriptors__list__all(pCtx, &pvhList)  );    	

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhList, &numDescriptors)  );
    total = numDescriptors;
	while(i<numDescriptors)
	{
		const char * szRepoName = NULL;
		SG_bool isAvailable = SG_TRUE;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhList, i, &szRepoName, NULL)  );
		SG_ERR_CHECK(  sg_uridispatch__query_repo_availability(pCtx, szRepoName, &isAvailable)  );
		if(!isAvailable)
		{
			SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvhList, szRepoName)  );
			--numDescriptors;
		}
		else
			++i;
	}
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
    SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "descriptors", &pvhList)  );
    SG_VHASH_NULLFREE(pCtx, pvhList);
    SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhResult, "has_excludes", (SG_bool)(total != numDescriptors))  );
     
    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvhResult);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvhList);
	return JS_FALSE;
}

/**
 * Returns a JSObject containing all descriptors, of any status, with status included.
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg, list_all_descriptors)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	JSObject* jso = NULL;
	SG_vhash * pvhList = NULL;

	SG_JS_BOOL_CHECK(argc==0);

	SG_ERR_CHECK(  SG_closet__descriptors__list__all(pCtx, &pvhList)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhList, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhList);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvhList);
	return JS_FALSE;
}

/**
 * Returns a JSObject containing descriptors, having the provided status.
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg, list_descriptors__status)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	JSObject* jso = NULL;
	SG_vhash * pvhList = NULL;

	SG_JS_BOOL_CHECK(argc==1);
	SG_JS_BOOL_CHECK(JSVAL_IS_INT(argv[0]));
	
	SG_ERR_CHECK(  SG_closet__descriptors__list__status(pCtx, 
		(SG_closet__repo_status)JSVAL_TO_INT(argv[0]), &pvhList)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhList, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhList);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvhList);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, set_descriptor_status)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* pszName = NULL;

	SG_JS_BOOL_CHECK(argc==2);
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
	SG_JS_BOOL_CHECK(JSVAL_IS_INT(argv[1]));
	
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );

	SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, pszName,
		(SG_closet__repo_status)JSVAL_TO_INT(argv[1]))  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszName);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, add_closet_property)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* pszName = NULL;
	char* pszValue = NULL;

	SG_JS_BOOL_CHECK(argc==2);
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[1]));

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszValue)  );

	SG_ERR_CHECK(  SG_closet__property__add(pCtx, pszName, pszValue)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszName);
	SG_NULLFREE(pCtx, pszValue);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszName);
	SG_NULLFREE(pCtx, pszValue);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, set_closet_property)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* pszName = NULL;
	char* pszValue = NULL;

	SG_JS_BOOL_CHECK(argc==2);
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[1]));

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszValue)  );

	SG_ERR_CHECK(  SG_closet__property__set(pCtx, pszName, pszValue)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszName);
	SG_NULLFREE(pCtx, pszValue);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszName);
	SG_NULLFREE(pCtx, pszValue);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, create_user_master_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);

	SG_vhash* pvh_args = NULL;
	SG_vhash* pvh_got = NULL;

	const char* psz_adminid = NULL;
	const char* psz_storage = NULL;
	const char* psz_hashmethod = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
	SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "adminid", &psz_adminid)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "storage", NULL, &psz_storage)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "hashmethod", NULL, &psz_hashmethod)  );

	SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "create_user_master_repo", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_repo__user_master__create(
		pCtx,
		psz_adminid,
		psz_storage,
		psz_hashmethod)  );

	SG_VHASH_NULLFREE(pCtx, pvh_args);
	SG_VHASH_NULLFREE(pCtx, pvh_got);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_args);
	SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, open_user_master_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	JSObject* jso = NULL;
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = NULL;

	SG_JS_BOOL_CHECK( argc == 0 );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_repo_class, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

	SG_ERR_CHECK(  SG_REPO__USER_MASTER__OPEN(pCtx, &pRepo)  );

	SG_ERR_CHECK(  SG_safeptr__wrap__repo(pCtx, pRepo, &psp)  );
	JS_SetPrivate(cx, jso, psp);
	psp = NULL;

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_user_master_adminid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	char* pszAdminId = NULL;
	jsval tmp = JSVAL_VOID;

	SG_JS_BOOL_CHECK( argc == 0 );

	SG_ERR_CHECK(  SG_repo__user_master__get_adminid(pCtx, &pszAdminId)  );
	JSVAL_FROM_SZ(tmp, pszAdminId);
	JS_SET_RVAL(cx, vp, tmp);
	SG_NULLFREE(pCtx, pszAdminId);	

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszAdminId);	
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_closet_property)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* pszName = NULL;
	char* pszValue = NULL;
	JSString* pjs = NULL;

	SG_JS_BOOL_CHECK(argc==1);
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );

	SG_ERR_CHECK(  SG_closet__property__get(pCtx, pszName, &pszValue)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszValue))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszName);
	SG_NULLFREE(pCtx, pszValue);

	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszName);
	SG_NULLFREE(pCtx, pszValue);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_clone_staging_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* pszCloneId = NULL;
	SG_vhash* pvhInfo = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK(argc==1);
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszCloneId)  );

	SG_ERR_CHECK(  SG_staging__clone__get_info(pCtx, pszCloneId, &pvhInfo)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhInfo, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhInfo);
	SG_NULLFREE(pCtx, pszCloneId);

	return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhInfo);
	SG_NULLFREE(pCtx, pszCloneId);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, exec)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * pszPathToExec = NULL;
	char * pszArg = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_exit_status childExitStatus = 0;
	SG_tempfile * pTempStdOut;
	SG_tempfile * pTempStdErr;
	SG_string * pStringErr = NULL;
	SG_string * pStringOut = NULL;
	uint i = 0;
	jsval jsChildExitStatus = JSVAL_VOID;
	jsval jsExecResult = JSVAL_VOID;
	jsval jsstderr = JSVAL_VOID;
	jsval jsstdout = JSVAL_VOID;
	JSObject* object = NULL;
	SG_exec_result execResult = SG_EXEC_RESULT__UNKNOWN;
	SG_process_id pid;
	SG_int64 start_time = 0;
	SG_int64 stop_time = 0;
	jsval jsstarttime = JSVAL_VOID;
	jsval jsduration = JSVAL_VOID;

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszPathToExec)  );

	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgVec)   );
	//allocate the new string array that we will send down.
	for (i = 1; i < argc; i++)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[i])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[i]), &pszArg)  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, pszArg) );
		SG_NULLFREE(pCtx, pszArg);
	}

	//We need to create temp files to handle the stdout and stderr output.
	SG_ERR_CHECK( SG_tempfile__open_new(pCtx, &pTempStdOut)  );
	SG_ERR_CHECK( SG_tempfile__open_new(pCtx, &pTempStdErr)  );

	// Run the requested command.  If we run into a setup-type error,
	// this will throw as usual.  But if the child has problems, it
	// returns normally (with both EXEC_RESULT and EXIT_STATUS).
	
	(void)SG_time__get_milliseconds_since_1970_utc__err(&start_time);
	SG_ERR_CHECK(  SG_exec__exec_sync__files__details(pCtx,
													  pszPathToExec, pArgVec,
													  NULL, pTempStdOut->pFile, pTempStdErr->pFile,
													  &childExitStatus,
													  &execResult,
													  &pid)  );
	(void)SG_time__get_milliseconds_since_1970_utc__err(&stop_time);

	SG_ERR_CHECK( SG_file__fsync(pCtx, pTempStdOut->pFile)   );
	SG_ERR_CHECK( SG_file__fsync(pCtx, pTempStdErr->pFile)   );

	SG_ERR_CHECK( SG_file__seek(pCtx, pTempStdOut->pFile, 0)   );
	SG_ERR_CHECK( SG_file__seek(pCtx, pTempStdErr->pFile, 0)   );

	SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pStringErr) );
	SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pStringOut) );
	SG_ERR_CHECK(  SG_file__read_utf8_file_into_string(pCtx, pTempStdErr->pFile, pStringErr)  );
	SG_ERR_CHECK(  SG_file__read_utf8_file_into_string(pCtx, pTempStdOut->pFile, pStringOut)  );

	SG_ERR_CHECK( SG_tempfile__close_and_delete(pCtx, &pTempStdErr) );
	SG_ERR_CHECK( SG_tempfile__close_and_delete(pCtx, &pTempStdOut) );

	//Bundle a Javascript object to return.

	SG_JS_NULL_CHECK(  (object = JS_NewObject(cx, &fsobj_properties_class, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(object));

	jsExecResult = INT_TO_JSVAL(execResult);
	SG_JS_BOOL_CHECK( JS_SetProperty(cx, object, "exec_result", &jsExecResult)  );

	jsChildExitStatus = INT_TO_JSVAL(childExitStatus);
	SG_JS_BOOL_CHECK( JS_SetProperty(cx, object, "exit_status", &jsChildExitStatus) );

	JSVAL_FROM_SZ(jsstderr, SG_string__sz(pStringErr));
	SG_JS_BOOL_CHECK( JS_SetProperty(cx, object, "stderr", &jsstderr) );

	JSVAL_FROM_SZ(jsstdout, SG_string__sz(pStringOut));
	SG_JS_BOOL_CHECK( JS_SetProperty(cx, object, "stdout", &jsstdout) );

	SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, (jsdouble)(start_time), &jsstarttime)  );
	SG_JS_BOOL_CHECK( JS_SetProperty(cx, object, "start_time", &jsstarttime) );

	SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, (jsdouble)(stop_time - start_time), &jsduration)  );
	SG_JS_BOOL_CHECK( JS_SetProperty(cx, object, "duration", &jsduration) );

#if defined(HAVE_EXEC_DEBUG_STACKTRACE)
	if (execResult & SG_EXEC_RESULT__ABNORMAL)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("Child process crashed with stderr:\n"
									"==================================\n"
									"%s\n"
									"==================================\n"),
								   SG_string__sz(pStringErr))  );

		SG_ERR_IGNORE(  SG_exec_debug__get_stacktrace(pCtx, pszPathToExec, pid)  );
	}
#endif

	SG_STRING_NULLFREE(pCtx, pStringOut);
	SG_STRING_NULLFREE(pCtx, pStringErr);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_NULLFREE(pCtx, pszPathToExec);
	return JS_TRUE;
fail:
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempStdErr) );
	SG_ERR_IGNORE( SG_tempfile__close_and_delete(pCtx, &pTempStdOut) );
	SG_STRING_NULLFREE(pCtx, pStringOut);
	SG_STRING_NULLFREE(pCtx, pStringErr);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_NULLFREE(pCtx, pszPathToExec);
	SG_NULLFREE(pCtx, pszArg);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, exec_nowait)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * pszPathToExec = NULL;
	char * pszArg = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_process_id processID = 0;
	SG_pathname * pPathNameForOutput = NULL;
	SG_pathname * pPathNameForError = NULL;
	SG_file * pFileForOutput = NULL;
	SG_file * pFileForError = NULL;
	uint i = 0;
	SG_uint32 firstArg = 0;
	JSObject* jso = NULL;
	SG_varray* pva_options = NULL;
	const char * pszVal = NULL;

	if (argc > 1)
	{
		jsval arg0 = argv[0];
		SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(arg0)  );
		jso = JSVAL_TO_OBJECT(arg0);

		if (JS_IsArrayObject(cx, jso))
		{
			 SG_uint32 count = 0;

			 SG_uint32 j = 0;
			 const char* pszOption;

			 SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso, &pva_options)  );

			 SG_ERR_CHECK(  SG_varray__count(pCtx, pva_options, &count)  );

			 for (j = 0; j < count; j++)
			 {

				 SG_ERR_CHECK( SG_varray__get__sz(pCtx, pva_options, j, &pszOption) );

				 if ( sg_jsglue_get_equals_flag (pszOption, "output", &pszVal) )
				 {
					SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathNameForOutput, pszVal)  );
				 }
				 else if ( sg_jsglue_get_equals_flag (pszOption, "error", &pszVal) )
				 {
					 SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathNameForError, pszVal)  );
				 }
			 }
			 firstArg++;
		}
	}

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[firstArg])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[firstArg]), &pszPathToExec)  );

	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgVec)   );
	//allocate the new string array that we will send down.
	for (i = firstArg + 1; i < argc; i++)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[i])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[i]), &pszArg)  );
		SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, pszArg) );
		SG_NULLFREE(pCtx, pszArg);
	}

	//We need to create temp files to handle the stdout and stderr output.
	if (pPathNameForOutput != NULL)
	{
		SG_ERR_CHECK( SG_file__open__pathname(pCtx, pPathNameForOutput, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY,  0644, &pFileForOutput)   );
	}
	if (pPathNameForError != NULL)
	{
		SG_ERR_CHECK( SG_file__open__pathname(pCtx, pPathNameForError, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY,  0644, &pFileForError)   );
	}
	SG_ERR_CHECK(   SG_exec__exec_async__files(pCtx, pszPathToExec, pArgVec, NULL, pFileForOutput, pFileForError, &processID)  );
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_VARRAY_NULLFREE(pCtx, pva_options);
	SG_PATHNAME_NULLFREE(pCtx, pPathNameForOutput);
	SG_PATHNAME_NULLFREE(pCtx, pPathNameForError);
	//It's ok to close these, because the child process has its own open file descriptor
	SG_FILE_NULLCLOSE(pCtx, pFileForError);
	SG_FILE_NULLCLOSE(pCtx, pFileForOutput);
	SG_NULLFREE(pCtx, pszPathToExec);
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL((SG_int32)processID));
	return JS_TRUE;
fail:
	//It's ok to close these, because the child process has its own open file descriptor
	SG_FILE_NULLCLOSE(pCtx, pFileForError);
	SG_FILE_NULLCLOSE(pCtx, pFileForOutput);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_VARRAY_NULLFREE(pCtx, pva_options);
	SG_PATHNAME_NULLFREE(pCtx, pPathNameForOutput);
	SG_PATHNAME_NULLFREE(pCtx, pPathNameForError);
	SG_NULLFREE(pCtx, pszPathToExec);
	SG_NULLFREE(pCtx, pszArg);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

// sg.exec_result(process_id)
//
// This non-blocking function queries the result of a process spawned by sg.exec_nowait().
//
// On Mac and Linux:
//     It returns undefined if the process is still running, an integer exit code if the
//     process exited normally, or null if the process was terminated abnormally (ie by a
//     signal).
//
// On Windows:
//     It returns undefined if the process is still running, and 0 otherwise (regardless
//     of how the process terminated or what its exit status was).
//
// Note that in any of these cases the value can potentially be interpreted
// as false... so use your === operator when checking the return value!
SG_JSGLUE_METHOD_PROTOTYPE(sg, exec_result)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_exec_result execResult = SG_FALSE;
	SG_exit_status exitStatus = -1;

	SG_JS_BOOL_CHECK(argc==1);
	SG_JS_BOOL_CHECK(JSVAL_IS_INT(argv[0]));
	
	SG_ERR_CHECK(  SG_exec__async_process_result(pCtx, (SG_process_id)JSVAL_TO_INT(argv[0]), &execResult, &exitStatus)  );
	
	if(execResult==SG_EXEC_RESULT__UNKNOWN)
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	else if(execResult==SG_EXEC_RESULT__NORMAL)
	{
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(exitStatus));
	}
	else
	{
		JS_SET_RVAL(cx, vp, JSVAL_NULL);
	}

	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, time)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_int64 t;

    SG_JS_BOOL_CHECK(argc==0);

    // This function is here partly because JS new Date().getTime() is
    // supposed to do the exact same thing, but on Windows, it kind
    // of doesn't.  See
    //
    // https://bugzilla.mozilla.org/show_bug.cgi?id=363258

    SG_time__get_milliseconds_since_1970_utc(pCtx, &t);

    if (SG_int64__fits_in_double(t))
    {
        double d = (double) t;
        jsval jv;

        SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, d, &jv)  );
        JS_SET_RVAL(cx, vp, jv);
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_INTEGER_OVERFLOW  );
    }

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, sleep_ms)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *sz = NULL;
	SG_JS_BOOL_CHECK(argc==1);
	if(JSVAL_IS_INT(argv[0]))
		SG_sleep_ms((SG_int64)JSVAL_TO_INT(argv[0]));
	else if(JSVAL_IS_STRING(argv[0]))
	{
		SG_uint32 i;
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &sz)  );
		SG_ERR_CHECK(SG_uint32__parse(pCtx, &i, sz));
		SG_NULLFREE(pCtx, sz);
		SG_sleep_ms(i);
	}
	else
		SG_ERR_THROW(SG_ERR_INVALIDARG);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx, cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, sz);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, platform)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval tmp = JSVAL_VOID;

	SG_JS_BOOL_CHECK(argc==0);

	JSVAL_FROM_SZ(tmp, SG_PLATFORM);
	JS_SET_RVAL(cx, vp, tmp);
	return JS_TRUE;
fail:
	return JS_FALSE;
}

static SG_bool _sg_jsglue__ctrl_c_pressed = SG_FALSE;

static void _sg_jsglue__set_ctrl_c_flag(int sig)
{
	SG_UNUSED(sig);
	_sg_jsglue__ctrl_c_pressed = SG_TRUE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, trap_ctrl_c)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_JS_BOOL_CHECK(argc==0 || (argc==1 && JSVAL_IS_BOOLEAN(argv[1]) && JSVAL_TO_BOOLEAN(argv[1])==JS_FALSE));
	_sg_jsglue__ctrl_c_pressed = SG_FALSE;
	if(argc==1 && JSVAL_IS_BOOLEAN(argv[1]) && JSVAL_TO_BOOLEAN(argv[1])==JS_FALSE)
		(void)signal(SIGINT,SIG_DFL);
	else
		(void)signal(SIGINT,_sg_jsglue__set_ctrl_c_flag);
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, ctrl_c_pressed)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	SG_JS_BOOL_CHECK(argc==0);
	if(!_sg_jsglue__ctrl_c_pressed)
		JS_SET_RVAL(cx, vp, JSVAL_FALSE);
	else
	{
		(void)signal(SIGINT,SIG_DFL);
		JS_SET_RVAL(cx, vp, JSVAL_TRUE);
	}
	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, rand)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_uint32 v = 0;

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[1])  );

    v = SG_random_uint32__2(JSVAL_TO_INT(argv[0]), JSVAL_TO_INT(argv[1]));

    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(v));

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, gid)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
    char buf[SG_GID_BUFFER_LENGTH];
    jsval tmp=JSVAL_VOID;

    SG_JS_BOOL_CHECK(argc==0);

    SG_ERR_CHECK(  SG_gid__generate(pCtx, buf, sizeof(buf))  );

    JSVAL_FROM_SZ(tmp, buf);
    JS_SET_RVAL(cx, vp, tmp);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, set_local_setting)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *arg1 = NULL;
	char *arg2 = NULL;

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &arg1)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &arg2)  );

    SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, arg1, arg2) );

    // TODO do we want to return something here?
	JS_SET_RVAL(cx, vp, JSVAL_NULL);

	SG_NULLFREE(pCtx, arg1);
	SG_NULLFREE(pCtx, arg2);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, arg1);
	SG_NULLFREE(pCtx, arg2);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_local_setting)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *arg = NULL;
	char* pszVal = NULL;
	JSString* pjs = NULL;

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__CONFIG_READ);

	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &arg)  );

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, arg, NULL, &pszVal, NULL) );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszVal))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszVal);

	SG_httprequestprofiler__stop();

	SG_NULLFREE(pCtx, arg);
	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszVal);
	SG_NULLFREE(pCtx, arg);

	SG_httprequestprofiler__stop();

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, set_local_setting__system_wide)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *arg1 = NULL;
	char *arg2 = NULL;

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &arg1)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &arg2)  );

	SG_ERR_CHECK(  SG_localsettings__system_wide__update_sz(pCtx, arg1, arg2) );

	// TODO do we want to return something here?
	JS_SET_RVAL(cx, vp, JSVAL_NULL);

	SG_NULLFREE(pCtx, arg1);
	SG_NULLFREE(pCtx, arg2);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, arg1);
	SG_NULLFREE(pCtx, arg2);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, local_settings)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_vhash * pvh = NULL;
	JSObject* jso = NULL;
	char * argOne = NULL;
	char * argTwo = NULL;
	SG_uint32 firstArg = 0;
	SG_bool bReset = SG_FALSE;
	SG_varray* pva_options = NULL;

	SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__CONFIG_READ);

    // TODO rework this.  it's a very un-javascript-like API
    // see set_local_setting above for a partial replacement

	if (argc > 0)
	{
		if (JSVAL_IS_OBJECT(argv[0]) && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[0])))
		{
			 SG_uint32 count = 0;
			 SG_uint32 j = 0;
			 const char* pszOption;

			 SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pva_options)  );

			 SG_ERR_CHECK(  SG_varray__count(pCtx, pva_options, &count)  );

			 for (j = 0; j < count; j++)
			 {

				 SG_ERR_CHECK( SG_varray__get__sz(pCtx, pva_options, j, &pszOption) );

				 if ( strcmp(pszOption, "reset")==0)
				 {
					bReset = SG_TRUE;
				 }
			 }
			 firstArg++;
		}
		if (argc >= 2)
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[firstArg])  );
			SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[firstArg]), &argOne)  );
			if (argc > (firstArg + 1))
			{
				if (JSVAL_IS_STRING(argv[firstArg + 1]))
				{
					SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[firstArg+1])  );
					SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[firstArg+1]), &argTwo)  );
					SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, argOne, argTwo) );
				}
				else if (JSVAL_IS_INT(argv[firstArg + 1]))
                {
                    SG_int_to_string_buffer sz_i;

                    SG_int64_to_sz(JSVAL_TO_INT(argv[firstArg + 1]), sz_i);
					SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, argOne, sz_i)  );
                }
			}
			else if (bReset)
			{
				SG_ERR_CHECK(  SG_localsettings__reset(pCtx, argOne)  );
			}
		}
	}

    SG_ERR_CHECK(  SG_localsettings__list__vhash(pCtx, &pvh)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VARRAY_NULLFREE(pCtx, pva_options);

	SG_httprequestprofiler__stop();

	SG_NULLFREE(pCtx, argOne);
	SG_NULLFREE(pCtx, argTwo);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VARRAY_NULLFREE(pCtx, pva_options);

	SG_httprequestprofiler__stop();

	SG_NULLFREE(pCtx, argOne);
	SG_NULLFREE(pCtx, argTwo);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, jsmin)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_arg = NULL;
	SG_string *outJs = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_arg)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &outJs)  );
	SG_ERR_CHECK(  SG_jsmin(pCtx, psz_arg, outJs)  );
    SG_NULLFREE(pCtx, psz_arg);

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(outJs)))  );
    SG_STRING_NULLFREE(pCtx, outJs);
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
 
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS

    SG_NULLFREE(pCtx, psz_arg);
	SG_STRING_NULLFREE(pCtx, outJs);

    return JS_FALSE;

}

//same as list_descriptors but this will also return a key to let you know if any
//repositories have been excluded for any reason
SG_JSGLUE_METHOD_PROTOTYPE(sg, list_descriptors2)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_vhash* pvh = NULL;
    SG_vhash* pvhResult = NULL;
	JSObject* jso = NULL;
	SG_uint32 i = 0;
    SG_uint32 total = 0;
	SG_uint32 numDescriptors = 0;

	SG_JS_BOOL_CHECK(argc==0);

	SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvh)  );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &numDescriptors)  );
    total = numDescriptors;
	while(i<numDescriptors)
	{
		const char * szRepoName = NULL;
		SG_bool isAvailable = SG_TRUE;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &szRepoName, NULL)  );
		SG_ERR_CHECK(  sg_uridispatch__query_repo_availability(pCtx, szRepoName, &isAvailable)  );
		if(!isAvailable)
		{
			SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh, szRepoName)  );
			--numDescriptors;
		}
		else
			++i;
	}
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
    SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "descriptors", &pvh)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhResult, "has_excludes", (SG_bool)(total != numDescriptors))  );
     
    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvhResult);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvh);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, list_descriptors)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_vhash* pvh = NULL;
	JSObject* jso = NULL;
	SG_uint32 i = 0;
	SG_uint32 numDescriptors = 0;

	SG_JS_BOOL_CHECK(argc==0);

	SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvh)  );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &numDescriptors)  );
	while(i<numDescriptors)
	{
		const char * szRepoName = NULL;
		SG_bool isAvailable = SG_TRUE;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &szRepoName, NULL)  );
		SG_ERR_CHECK(  sg_uridispatch__query_repo_availability(pCtx, szRepoName, &isAvailable)  );
		if(!isAvailable)
		{
			SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh, szRepoName)  );
			--numDescriptors;
		}
		else
			++i;
	}

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvh);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_descriptor)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* pszName = NULL;
	SG_bool bIncludeUnavailable = SG_FALSE;
	SG_vhash* pvh = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK(argc==1 || argc==2);
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );
	if (argc == 2)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[1])  );
		bIncludeUnavailable = JSVAL_TO_BOOLEAN(argv[1]);
	}

	if (bIncludeUnavailable)
		SG_ERR_CHECK(  SG_closet__descriptors__get__unavailable(pCtx, pszName, NULL, NULL, NULL, NULL, &pvh)  );
	else
		SG_ERR_CHECK(  SG_closet__descriptors__get(pCtx, pszName, NULL, &pvh)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_NULLFREE(pCtx, pszName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, pszName);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, zip)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* psz_csid = NULL;
	char* psz_path = NULL;
	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;

    SG_JS_BOOL_CHECK(argc==3);
    
	SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_csid)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_path)  );

    SG_ERR_CHECK(  SG_repo__zip(pCtx, pRepo, psz_csid, psz_path)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, psz_csid);
	SG_NULLFREE(pCtx, psz_path);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_csid);
	SG_NULLFREE(pCtx, psz_path);
    return JS_FALSE;
}


SG_JSGLUE_METHOD_PROTOTYPE(sg, server_state_dir)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    JSString* pjs = NULL;
    SG_pathname *tmppath = NULL;

    SG_JS_BOOL_CHECK(  0 == argc  );

	SG_ERR_CHECK(  SG_closet__get_server_state_path(pCtx, &tmppath)  );

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(tmppath)))  );
    SG_PATHNAME_NULLFREE(pCtx, tmppath);

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
    SG_PATHNAME_NULLFREE(pCtx, tmppath);
    return JS_FALSE;
}

#if (defined(MAC) || defined(LINUX)) && !defined(SG_IOS)
SG_JSGLUE_METHOD_PROTOTYPE(sg, user_home_dir)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	JSString* pjs = NULL;
	SG_pathname *tmppath = NULL;

	SG_JS_BOOL_CHECK(  0 == argc  );

	SG_ERR_CHECK(  SG_pathname__alloc__user_home(pCtx, &tmppath)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(tmppath)))  );
	SG_PATHNAME_NULLFREE(pCtx, tmppath);

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx);
	SG_PATHNAME_NULLFREE(pCtx, tmppath);
	return JS_FALSE;
}
#endif

void SG_jsglue__np__required__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint32* pi
    )
{
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__uint32(pCtx, pvh_args, psz_key, pi)  );
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
    }
    else
    {
        SG_ERR_THROW2_RETURN(SG_ERR_JS_NAMED_PARAM_REQUIRED, (pCtx, "%s", psz_key));
    }
}

void SG_jsglue__np__required__varray(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_varray** ppva
    )
{
    SG_varray* pva = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__varray(pCtx, pvh_args, psz_key, &pva)  );
    if (pva)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
        *ppva = pva;
    }
    else
    {
        SG_ERR_THROW2_RETURN(SG_ERR_JS_NAMED_PARAM_REQUIRED, (pCtx, "%s", psz_key));
    }
}

void SG_jsglue__np__optional__varray(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_varray** ppva
    )
{
    SG_varray* pva = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__varray(pCtx, pvh_args, psz_key, &pva)  );
    if (pva)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
        *ppva = pva;
    }
    else
    {
        *ppva = NULL;
    }
}

void SG_jsglue__np__required__vhash(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_vhash** ppvh
    )
{
    SG_vhash* pvh = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_args, psz_key, &pvh)  );
    if (pvh)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
        *ppvh = pvh;
    }
    else
    {
        SG_ERR_THROW2_RETURN(SG_ERR_JS_NAMED_PARAM_REQUIRED, (pCtx, "%s", psz_key));
    }
}

void SG_jsglue__np__required__sz(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    const char** ppsz
    )
{
    const char* p = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvh_args, psz_key, &p)  );
    if (p)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
        *ppsz = p;
    }
    else
    {
        SG_ERR_THROW2_RETURN(SG_ERR_JS_NAMED_PARAM_REQUIRED, (pCtx, "%s", psz_key));
    }
}

//////////////////////////////////////////////////////////////////

/**
 * Parse the value of the key (if present) as a date/time.
 * We return a time-in-ms-since-the-epoch value.
 * 
 * We allow the following formats:
 * 
 * "<key>" : <<value>>
 *
 * <<value>> ::=    <int64__time_in_ms>
 *               |  "<sz__time_in_ms>"
 *               |  "<sz__YYYY-MM-DD>"
 *               |  "<sz__YYYY-MM-DD hh:mm:ss>"
 *
 */
static void sg_jsglue__np__when(SG_context * pCtx,
								SG_vhash * pvh_args,
								SG_vhash * pvh_got,
								const char * psz_key,
								SG_int64 * piWhen)
{
	const char * pszValue = NULL;
	SG_uint16 type;

	SG_ERR_CHECK(  SG_vhash__typeof(pCtx, pvh_args, psz_key, &type)  );
	switch (type)
	{
	case SG_VARIANT_TYPE_SZ:
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_args, psz_key, &pszValue)  );
		SG_ERR_CHECK(  SG_time__parse__friendly(pCtx, pszValue, piWhen)  );
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
		break;

	case SG_VARIANT_TYPE_INT64:
	case SG_VARIANT_TYPE_DOUBLE:	// hack because of how we store int64's internally
		SG_ERR_CHECK(  SG_vhash__get__int64_or_double(pCtx, pvh_args, psz_key, piWhen)  );
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
		break;

	default:
		SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION,
						(pCtx, "The value of '%s' must be a string or integer.", psz_key));
	}

fail:
	return;
}

void SG_jsglue__np__required__when(SG_context * pCtx,
								   SG_vhash * pvh_args,
								   SG_vhash * pvh_got,
								   const char * psz_key,
								   SG_int64 * piWhen)
{
	SG_bool bHasKey;

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &bHasKey)  );
	if (!bHasKey)
		SG_ERR_THROW2_RETURN(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
							   (pCtx, "%s", psz_key)  );

	SG_ERR_CHECK_RETURN(  sg_jsglue__np__when(pCtx, pvh_args, pvh_got, psz_key, piWhen)  );
}

void SG_jsglue__np__optional__when(SG_context * pCtx,
								   SG_vhash * pvh_args,
								   SG_vhash * pvh_got,
								   const char * psz_key,
								   SG_int64 def,
								   SG_int64 * piWhen)
{
	SG_bool bHasKey;

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &bHasKey)  );
	if (!bHasKey)
	{
		*piWhen = def;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  sg_jsglue__np__when(pCtx, pvh_args, pvh_got, psz_key, piWhen)  );
	}
}
								   
//////////////////////////////////////////////////////////////////

/**
 * Look for { ...., "<key>" : "<sz>", .... }
 * or   for { ...., "<key>" : [ "<sz1>", "<sz2>", ... ], .... }
 *
 * Return one pointer depending on which form we found.
 *
 */
static void sg_jsglue__np__sz_or_varray_of_sz(SG_context * pCtx,
											  SG_vhash * pvh_args,
											  SG_vhash * pvh_got,
											  const char * psz_key,
											  const char ** ppsz,
											  SG_varray ** ppva)
{
	const char * psz = NULL;
	SG_varray * pva = NULL;
	SG_uint16 type;

	SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh_args, psz_key, &type)  );
	switch (type)
	{
	case SG_VARIANT_TYPE_SZ:
		SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_args, psz_key, &psz)  );
		SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
		*ppsz = psz;
		*ppva = NULL;
		return;

	case SG_VARIANT_TYPE_VARRAY:
		SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvh_args, psz_key, &pva)  );
		// TODO 2011/09/29 Verify that each element in the varray is a string.
		SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
		*ppva = pva;
		*ppsz = NULL;
		return;

	default:
		SG_ERR_THROW2_RETURN(  SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION,
							   (pCtx, "The value of '%s' must be a string or array of string.", psz_key));
	}
}

void SG_jsglue__np__required__sz_or_varray_of_sz(SG_context * pCtx,
												 SG_vhash * pvh_args,
												 SG_vhash * pvh_got,
												 const char * psz_key,
												 const char ** ppsz,
												 SG_varray ** ppva)
{
	SG_bool bHasKey;

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &bHasKey)  );
	if (!bHasKey)
		SG_ERR_THROW2_RETURN(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
							   (pCtx, "%s", psz_key)  );

	SG_ERR_CHECK_RETURN(  sg_jsglue__np__sz_or_varray_of_sz(pCtx, pvh_args, pvh_got, psz_key, ppsz, ppva)  );
}

void SG_jsglue__np__optional__sz_or_varray_of_sz(SG_context * pCtx,
												 SG_vhash * pvh_args,
												 SG_vhash * pvh_got,
												 const char * psz_key,
												 const char ** ppsz,
												 SG_varray ** ppva)
{
	SG_bool bHasKey;

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &bHasKey)  );
	if (!bHasKey)
	{
		*ppsz = NULL;
		*ppva = NULL;
		return;
	}
	
	SG_ERR_CHECK_RETURN(  sg_jsglue__np__sz_or_varray_of_sz(pCtx, pvh_args, pvh_got, psz_key, ppsz, ppva)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Convert simple string or string array value into a SG_stringarray.
 * Eat any empty strings.
 *
 * Return NULL if no actual string values.
 *
 */
static void sg_jsglue__np__sz_or_varray_of_sz__stringarray(SG_context * pCtx,
														   SG_vhash * pvh_args,
														   SG_vhash * pvh_got,
														   const char * psz_key,
														   SG_stringarray ** ppsa)
{
	const char * psz = NULL;	// we do not own this
	SG_varray * pva  = NULL;	// we do not own this
	SG_stringarray * psa = NULL;
	SG_uint32 count = 0;

	SG_ERR_CHECK(  sg_jsglue__np__sz_or_varray_of_sz(pCtx, pvh_args, pvh_got, psz_key,
													 &psz, &pva)  );
	if (psz)		// "<name>" : <string>,
	{
		if (*psz)	// "<name>" : "<value>",
		{
			count = 1;
			SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count)  );
			SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, psz)  );
		}
	}
	else			// "<name>" : <string_array>,
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
		if (count > 0)
		{
			SG_uint32 nrKept = 0;
			SG_uint32 k;

			SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count)  );
			for (k=0; k<count; k++)
			{
				const char * psz_k;

				SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, k, &psz_k)  );
				if (*psz_k)
				{
					SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, psz_k)  );
					nrKept++;
				}
			}

			if (nrKept == 0)
				SG_STRINGARRAY_NULLFREE(pCtx, psa);
		}
	}

	*ppsa = psa;
	return;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

void SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(SG_context * pCtx,
															  SG_vhash * pvh_args,
															  SG_vhash * pvh_got,
															  const char * psz_key,
															  SG_stringarray ** ppsa)
{
	SG_bool bHasKey;

	*ppsa = NULL;
	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &bHasKey)  );
	if (bHasKey)
		SG_ERR_CHECK_RETURN(  sg_jsglue__np__sz_or_varray_of_sz__stringarray(pCtx,
																			 pvh_args,
																			 pvh_got,
																			 psz_key,
																			 ppsa)  );
}

void SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(SG_context * pCtx,
															  SG_vhash * pvh_args,
															  SG_vhash * pvh_got,
															  const char * psz_key,
															  SG_stringarray ** ppsa)
{
	SG_bool bHasKey;

	SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &bHasKey)  );
	if (!bHasKey)
		SG_ERR_THROW2_RETURN(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
							   (pCtx, "%s", psz_key)  );

	*ppsa = NULL;
	SG_ERR_CHECK_RETURN(  sg_jsglue__np__sz_or_varray_of_sz__stringarray(pCtx,
																		 pvh_args,
																		 pvh_got,
																		 psz_key,
																		 ppsa)  );
	if (*ppsa == NULL)	// no values associated with key
		SG_ERR_THROW2_RETURN(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
							   (pCtx, "%s", psz_key)  );
}

//////////////////////////////////////////////////////////////////

static void SG_jsglue__convert_arg__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    const char* psz_key,
    SG_bool* p
    )
{
    SG_uint16 t = 0; 

    SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh_args, psz_key, &t)  ); 
    if (SG_VARIANT_TYPE_BOOL == t) 
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__bool(pCtx, pvh_args, psz_key, p)  );
    }
    else if (SG_VARIANT_TYPE_INT64 == t) 
    {
        SG_int64 i64 = 0;

        SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh_args, psz_key, &i64)  );

        if (i64)
        {
            *p = SG_TRUE;
        }
        else
        {
            *p = SG_FALSE;
        }
    }
    else if (SG_VARIANT_TYPE_SZ == t)
    {
        const char* psz = NULL;

        SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_args, psz_key, &psz)  );

        if (*psz)
        {
            *p = SG_TRUE;
        }
        else
        {
            *p = SG_FALSE;
        }
    }
    else if (SG_VARIANT_TYPE_SZ == t)
    {
        *p = SG_FALSE;
    }
    else if (SG_VARIANT_TYPE_VHASH == t)
    {
        *p = SG_TRUE;
    }
    else if (SG_VARIANT_TYPE_VARRAY == t)
    {
        *p = SG_TRUE;
    }
    else if (SG_VARIANT_TYPE_DOUBLE == t)
    {
        double d = 0;

        SG_ERR_CHECK_RETURN(  SG_vhash__get__double(pCtx, pvh_args, psz_key, &d)  );

        if (0 == ( (int) d ))
        {
            *p = SG_FALSE;
        }
        else
        {
            *p = SG_TRUE;
        }
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
    }
}

void SG_jsglue__np__required__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_bool* p
    )
{
    SG_bool b_has = SG_FALSE; 

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &b_has)  ); 
    if (!b_has) 
    { 
        SG_ERR_THROW2_RETURN(SG_ERR_JS_NAMED_PARAM_REQUIRED, (pCtx, "%s", psz_key));
    } 
    else 
    { 
        SG_ERR_CHECK_RETURN(  SG_jsglue__convert_arg__bool(pCtx, pvh_args, psz_key, p)  );
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
    }
}

void SG_jsglue__np__optional__sz(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    const char* psz_def,
    const char** ppsz
    )
{
    const char* p = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvh_args, psz_key, &p)  );
    if (p)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
        *ppsz = p;
    }
    else
    {
        *ppsz = psz_def;
    }
}

void SG_jsglue__np__optional__bool(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_bool def,
    SG_bool* p
    )
{
    SG_bool b_has = SG_FALSE; 

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &b_has)  ); 
    if (!b_has) 
    { 
        *p = def; 
    } 
    else 
    { 
        SG_ERR_CHECK_RETURN(  SG_jsglue__convert_arg__bool(pCtx, pvh_args, psz_key, p)  );
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
    }
}

static void SG_jsglue__convert_arg__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    const char* psz_key,
    SG_uint32* p
    )
{
    SG_uint16 t = 0; 

    SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh_args, psz_key, &t)  ); 

    if (SG_VARIANT_TYPE_INT64 == t) 
    { 
        SG_int64 i64 = 0; 

        SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh_args, psz_key, &i64)  ); 
        if (SG_int64__fits_in_uint32(i64)) 
        { 
            *p = (SG_uint32) i64; 
        } 
        else 
        { 
            SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  ); 
        } 
    } 
    else if (SG_VARIANT_TYPE_SZ == t) 
    { 
        const char* psz = NULL; 
        SG_uint32 ui32 = 0; 

        SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_args, psz_key, &psz)  ); 
        SG_ERR_CHECK_RETURN(  SG_uint32__parse__strict(pCtx, &ui32, psz)  ); 
        *p = ui32; 
    } 
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION  ); 
    }
}

static void SG_jsglue__convert_arg__int32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    const char* psz_key,
    SG_int32* p
    )
{
    SG_uint16 t = 0; 

    SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh_args, psz_key, &t)  ); 

    if (SG_VARIANT_TYPE_INT64 == t) 
    { 
        SG_int64 i64 = 0; 

        SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh_args, psz_key, &i64)  ); 
        if (SG_int64__fits_in_int32(i64)) 
        { 
            *p = (SG_int32) i64; 
        } 
        else 
        { 
            SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  ); 
        } 
    } 
    else if (SG_VARIANT_TYPE_SZ == t) 
    { 
        const char* psz = NULL; 
        SG_int32 i32 = 0; 

        SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_args, psz_key, &psz)  ); 
        SG_ERR_CHECK_RETURN(  SG_int32__parse__strict(pCtx, &i32, psz)  ); 
        *p = i32; 
    } 
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION  ); 
    }
}

static void SG_jsglue__convert_arg__uint64(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    const char* psz_key,
    SG_uint64* p
    )
{
    SG_uint16 t = 0; 

    SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh_args, psz_key, &t)  ); 

    if (SG_VARIANT_TYPE_INT64 == t) 
    { 
        SG_int64 i64 = 0; 

        SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh_args, psz_key, &i64)  ); 
        if (SG_int64__fits_in_uint64(i64)) 
        { 
            *p = i64; 
        } 
        else 
        { 
            SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  ); 
        } 
    } 
    else if (SG_VARIANT_TYPE_SZ == t) 
    { 
        const char* psz = NULL; 
        SG_uint64 ui64 = 0; 

        SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_args, psz_key, &psz)  ); 
        SG_ERR_CHECK_RETURN(  SG_uint64__parse__strict(pCtx, &ui64, psz)  ); 
        *p = ui64; 
    } 
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION  ); 
    }
}

void SG_jsglue__np__optional__uint64(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint64 def,
    SG_uint64* p
    )
{
    SG_bool b_has = SG_FALSE; 

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &b_has)  ); 
    if (!b_has) 
    { 
        *p = def; 
    } 
    else 
    { 
        SG_ERR_CHECK_RETURN(  SG_jsglue__convert_arg__uint64(pCtx, pvh_args, psz_key, p)  );
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
    }
}

void SG_jsglue__np__optional__uint32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_uint32 def,
    SG_uint32* p
    )
{
    SG_bool b_has = SG_FALSE; 

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &b_has)  ); 
    if (!b_has) 
    { 
        *p = def; 
    } 
    else 
    { 
        SG_ERR_CHECK_RETURN(  SG_jsglue__convert_arg__uint32(pCtx, pvh_args, psz_key, p)  );
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
    }
}

void SG_jsglue__np__optional__int32(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
    SG_int32 def,
    SG_int32* p
    )
{
    SG_bool b_has = SG_FALSE; 

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_args, psz_key, &b_has)  ); 
    if (!b_has) 
    { 
        *p = def; 
    } 
    else 
    { 
        SG_ERR_CHECK_RETURN(  SG_jsglue__convert_arg__int32(pCtx, pvh_args, psz_key, p)  );
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
    }
}

void SG_jsglue__np__optional__vhash(
    SG_context* pCtx,
    SG_vhash* pvh_args,
    SG_vhash* pvh_got,
    const char* psz_key,
	SG_vhash * pvh_default,		// we do not own this
    SG_vhash** ppvh				// you do not own this
    )
{
    SG_vhash* pvh = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__check__vhash(pCtx, pvh_args, psz_key, &pvh)  );
    if (pvh)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh_got, psz_key)  );
        *ppvh = pvh;
    }
    else
    {
		*ppvh = pvh_default;
    }
}

void SG_jsglue__np__anything_not_got_is_invalid(
        SG_context* pCtx,
        const char* psz_func,
        SG_vhash* pvh_args,
        SG_vhash* pvh_got
        )
{
    SG_string* pstr = NULL;
    SG_uint32 count = 0;
    SG_uint32 errs = 0;

    if (pvh_args)
    {
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_args, &count)  );
        if (count > 0)
        {
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            for (i=0; i<count; i++)
            {
                const char* psz_param_name = NULL;
                SG_bool b_got = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_args, i, &psz_param_name, NULL)  );
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_got, psz_param_name, &b_got)  );

                if (!b_got)
                {
                    if (errs)
                    {
                        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ", ")  );
                    }
                    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, psz_param_name)  );
                    errs++;
                }
            }
            if (errs)
            {
                SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_NOT_ALLOWED,
                        (pCtx, "%s: %s", psz_func, SG_string__sz(pstr))
                        );
            }
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static JSBool do_clone_all_to_something(
    JSContext *cx, 
    uintN argc, 
    jsval *vp, 
    const char* psz_func,
    SG_blob_encoding encoding
    )
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    const char* psz_existing = NULL;
    const char* psz_new = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_bool b_override_alwaysfull = SG_FALSE;
    struct SG_clone__params__all_to_something ats;
    struct SG_clone__demands demands;

    memset(&ats, 0, sizeof(ats));
    ats.new_encoding = encoding;

    SG_ERR_CHECK(  SG_clone__init_demands(pCtx, &demands)  );

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "existing_repo", &psz_existing)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "new_repo", &psz_new)  );

    SG_ERR_CHECK(  SG_jsglue__np__optional__uint64(pCtx, pvh_args, pvh_got, "low_pass", 0, &ats.low_pass)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__uint64(pCtx, pvh_args, pvh_got, "high_pass", 0, &ats.high_pass)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "override_alwaysfull", SG_FALSE, &b_override_alwaysfull)  );
    if (SG_IS_BLOBENCODING_ZLIB(encoding))
    {
        SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, SG_CLONE_CHANGES__DEMAND_ZLIB_SAVINGS_OVER_FULL, -1, &demands.zlib_savings_over_full)  );
    }

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, psz_func, pvh_args, pvh_got)  );

    if (b_override_alwaysfull)
    {
        ats.flags |= SG_CLONE_FLAG__OVERRIDE_ALWAYSFULL;
    }

    SG_ERR_CHECK(  SG_clone__to_local(
                pCtx, 
                psz_existing, 
                NULL,
                NULL,
                psz_new, 
                &ats,
                NULL,
                &demands
                )  );

	// Save default push/pull location for the new repository instance.
	SG_ERR_CHECK(  SG_localsettings__descriptor__update__sz(pCtx, psz_new, SG_LOCALSETTING__PATHS_DEFAULT, psz_existing)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, clone__all_full)
{
    return do_clone_all_to_something(cx, argc, vp, "clone__all_full", SG_BLOBENCODING__KEEPFULLFORNOW);
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, clone__all_zlib)
{
    return do_clone_all_to_something(cx, argc, vp, "clone__all_zlib", SG_BLOBENCODING__ZLIB);
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, pull_branch_stuff_only)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	const char* psz_from = NULL;
	const char* psz_to = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    char* psz_repo_upstream = NULL;
    const char* psz_username = NULL;
    const char* psz_password = NULL;
    SG_string* p_password = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "from", NULL, &psz_from)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "to", &psz_to)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "username", NULL, &psz_username)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "password", NULL, &psz_password)  );
    if((psz_username!=NULL)&&(psz_password==NULL))
        SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Provided username without password."));
    if((psz_username==NULL)&&(psz_password!=NULL))
        SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Provided password without username."));
    if(psz_password)
    {
        SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, psz_username, psz_password, &p_password)  );
        psz_password = SG_string__sz(p_password);
    }

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "pull_branch_stuff_only", pvh_args, pvh_got)  );

    if (!psz_from)
    {
        // we need the repo spec for the other side
        SG_ERR_CHECK(  SG_localsettings__descriptor__get__sz(pCtx, psz_to, "paths/default", &psz_repo_upstream)  );
        psz_from = psz_repo_upstream;
    }

    SG_ERR_CHECK(  SG_vc_locks__pull_locks_and_branches(pCtx, psz_from, psz_username, psz_password, psz_to)  );
    SG_STRING_NULLFREE(pCtx, p_password);

    SG_NULLFREE(pCtx, psz_repo_upstream);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_NULLFREE(pCtx, psz_repo_upstream);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    SG_STRING_NULLFREE(pCtx, p_password);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, clone__pack)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	const char* psz_existing = NULL;
	const char* psz_new = NULL;
	SG_int32 keyframe_density;
	SG_int32 revisions_to_leave_undeltified;
	SG_int32 min_revisions;
    SG_uint64 low_pass = 0;
    SG_uint64 high_pass = 0;
    struct SG_clone__demands demands;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    struct SG_clone__params__pack params_pack;

    SG_ERR_CHECK(  SG_clone__init_demands(pCtx, &demands)  );

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "existing_repo", &psz_existing)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "new_repo", &psz_new)  );

    SG_ERR_CHECK(  SG_jsglue__np__optional__uint64(pCtx, pvh_args, pvh_got, "low_pass", 0, &low_pass)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__uint64(pCtx, pvh_args, pvh_got, "high_pass", 0, &high_pass)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, SG_CLONE_CHANGES__DEMAND_ZLIB_SAVINGS_OVER_FULL, -1, &demands.zlib_savings_over_full)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, SG_CLONE_CHANGES__DEMAND_VCDIFF_SAVINGS_OVER_FULL, -1, &demands.vcdiff_savings_over_full)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, SG_CLONE_CHANGES__DEMAND_VCDIFF_SAVINGS_OVER_ZLIB, -1, &demands.vcdiff_savings_over_zlib)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, "keyframe_density", SG_PACK_DEFAULT__KEYFRAMEDENSITY, &keyframe_density)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, "leave_full", SG_PACK_DEFAULT__REVISIONSTOLEAVEFULL, &revisions_to_leave_undeltified)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__int32(pCtx, pvh_args, pvh_got, "min_revisions", SG_PACK_DEFAULT__MINREVISIONS, &min_revisions)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "clone__pack", pvh_args, pvh_got)  );

    params_pack.nKeyframeDensity = keyframe_density; 
    params_pack.nRevisionsToLeaveFull = revisions_to_leave_undeltified; 
    params_pack.nMinRevisions = min_revisions;
    params_pack.low_pass = low_pass;
    params_pack.high_pass = high_pass;

    SG_ERR_CHECK(  SG_clone__to_local(
                pCtx, 
                psz_existing, 
                NULL,
                NULL,
                psz_new, 
                NULL,
                &params_pack,
                &demands
                )  );

	// Save default push/pull location for the new repository instance.
	SG_ERR_CHECK(  SG_localsettings__descriptor__update__sz(pCtx, psz_new, SG_LOCALSETTING__PATHS_DEFAULT, psz_existing)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, clone__exact)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_existing = NULL;
	char *psz_new = NULL;
	char *psz_username = NULL;
	char *psz_password_arg = NULL;
	SG_string *pPassword = NULL;
	const char *psz_password = NULL;
	SG_bool bNewIsRemote = SG_FALSE;
	SG_repo* p_repo = NULL;

	SG_JS_BOOL_CHECK(2==argc || 4==argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_existing)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_new)  );
	
	if(argc==4)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_username)  );

		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &psz_password_arg)  );

		SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, psz_username, psz_password_arg, &pPassword)  );
		psz_password = SG_string__sz(pPassword);
	}

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, psz_new, &bNewIsRemote)  );

    if (bNewIsRemote)
    {
        SUSPEND_REQUEST_ERR_CHECK(  SG_clone__to_remote(
                pCtx, 
                psz_existing, 
                psz_username, 
                psz_password, 
                psz_new,
				NULL
                )  );
    }
    else
    {
        SUSPEND_REQUEST_ERR_CHECK(  SG_clone__to_local(
                pCtx, 
                psz_existing, 
                psz_username, 
                psz_password, 
                psz_new, 
                NULL,
                NULL,
                NULL
                )  );

		// Save default push/pull location for the new repository instance.
		SG_ERR_CHECK(  SG_localsettings__descriptor__update__sz(pCtx, psz_new, SG_LOCALSETTING__PATHS_DEFAULT, psz_existing)  );
		if(psz_username)
		{
			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_new, &p_repo)  );
			SG_ERR_CHECK(  SG_user__set_user__repo(pCtx, p_repo, psz_username)  );
			SG_REPO_NULLFREE(pCtx, p_repo);
		}
    }
	
	SG_STRING_NULLFREE(pCtx, pPassword);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, psz_existing);
	SG_NULLFREE(pCtx, psz_new);
	SG_NULLFREE(pCtx, psz_username);
	SG_NULLFREE(pCtx, psz_password_arg);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_REPO_NULLFREE(pCtx, p_repo);
	SG_NULLFREE(pCtx, psz_existing);
	SG_NULLFREE(pCtx, psz_new);
	SG_NULLFREE(pCtx, psz_username);
	SG_NULLFREE(pCtx, psz_password_arg);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, clone__finish_usermap)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	const char* psz_new = NULL;

    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "repo_being_usermapped", &psz_new)  );  

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "clone__finish_usermap", pvh_args, pvh_got)  );

	SUSPEND_REQUEST_ERR_CHECK(  SG_clone__finish_usermap(
		pCtx, 
		psz_new
		)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, clone__prep_usermap)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	const char* psz_existing = NULL;
	const char* psz_username = NULL;
	const char* psz_password = NULL;
	SG_string* p_password = NULL;
	const char* psz_new = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "existing_repo", &psz_existing)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "new_repo", &psz_new)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "username", NULL, &psz_username)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "password", NULL, &psz_password)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "clone__prep_usermap", pvh_args, pvh_got)  );

    if (psz_username && psz_password)
    {
        SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, psz_username, psz_password, &p_password)  );
        psz_password = SG_string__sz(p_password);
    }

	SUSPEND_REQUEST_ERR_CHECK(  SG_clone__prep_usermap(
		pCtx, 
		psz_existing, 
		psz_username, 
		psz_password, 
		psz_new)  );
	SG_STRING_NULLFREE(pCtx, p_password);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_STRING_NULLFREE(pCtx, p_password);
	return JS_FALSE;
}

/**
* void sg.push_all(string src_repo_descriptor, string dest_repo_spec, [bool force=false, [username, password]])
*/	
SG_JSGLUE_METHOD_PROTOTYPE(sg, push_all)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszSrc = NULL;
	char *pszDest = NULL;
	SG_bool bForce = SG_FALSE;
	char *pszUsername = NULL;
	char *pszPasswordArg = NULL;
	const char* pszPassword = NULL;
	SG_string* pPassword = NULL;

	SG_JS_BOOL_CHECK(2 == argc || 3 == argc || 5 == argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszSrc)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszDest)  );

	if (3 <= argc)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[2])  );
		bForce = JSVAL_TO_BOOLEAN(argv[2]);
	}

	if(argc==5)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &pszUsername)  );

		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[4])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[4]), &pszPasswordArg)  );

		SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, pszUsername, pszPasswordArg, &pPassword)  );
		pszPassword = SG_string__sz(pPassword);
	}

	SG_ERR_CHECK(  SG_vv_verbs__push(pCtx, pszDest, pszUsername, pszPassword, pszSrc, NULL, NULL, bForce, NULL, NULL)  );
	SG_STRING_NULLFREE(pCtx, pPassword);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_FALSE;
}

/**
* void sg.push_branch(string src_repo_descriptor, string dest_repo_spec, string branch_name, [bool force=false, [username, password]])
*/	
SG_JSGLUE_METHOD_PROTOTYPE(sg, push_branch)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszSrc = NULL;
	char *pszDest = NULL;
	char *pszUsername = NULL;
	char *pszPasswordArg = NULL;
	const char* pszPassword = NULL;
	SG_string* pPassword = NULL;
	char* pszBranch = NULL;
	SG_rev_spec* pRevSpec = NULL;
	SG_bool bForce = SG_FALSE;

	SG_JS_BOOL_CHECK(3 == argc || 4 == argc || 6 == argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszSrc)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszDest)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszBranch)  );

	if (4 <= argc)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[3])  );
		bForce = JSVAL_TO_BOOLEAN(argv[3]);
	}

	if(argc==6)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[4])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[4]), &pszUsername)  );

		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[5])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[5]), &pszPasswordArg)  );

		SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, pszUsername, pszPasswordArg, &pPassword)  );
		pszPassword = SG_string__sz(pPassword);
	}

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );
	SG_ERR_CHECK(  SG_vv_verbs__push(pCtx, pszDest, pszUsername, pszPassword, pszSrc, NULL, pRevSpec, bForce, NULL, NULL)  );
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszBranch);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_TRUE;

fail:
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszBranch);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/**
* void sg.pull_all(string src_repo_spec, string dest_repo_descriptor [username, password])
*/	
SG_JSGLUE_METHOD_PROTOTYPE(sg, pull_all)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszSrc = NULL;
	char *pszUsername = NULL;
	char *pszPasswordArg = NULL;
	const char* pszPassword = NULL;
	SG_string* pPassword = NULL;
	char* pszDest = NULL;

	SG_JS_BOOL_CHECK(2==argc || 4==argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszSrc)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszDest)  );

	if(argc==4)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszUsername)  );

		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &pszPasswordArg)  );

		SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, pszUsername, pszPasswordArg, &pPassword)  );
		pszPassword = SG_string__sz(pPassword);
	}

	SG_ERR_CHECK(  SG_vv_verbs__pull(pCtx, pszSrc, pszUsername, pszPassword, pszDest, NULL, NULL, NULL, NULL)  );
	SG_STRING_NULLFREE(pCtx, pPassword);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_FALSE;
}

/**
* void sg.incoming(string src_repo_spec, string dest_repo_descriptor[, username, password])
*/
SG_JSGLUE_METHOD_PROTOTYPE(sg, incoming)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszSrc = NULL;
	char *pszUsername = NULL;
	char *pszPasswordArg = NULL;
	const char* pszPassword = NULL;
	SG_string* pPassword = NULL;
	char* pszDest = NULL;
	SG_history_result * p_incomingChangesets = NULL;
	SG_varray * pvaRef = NULL;
	JSObject* jsoReturn = NULL;

	SG_JS_BOOL_CHECK(2==argc || 4==argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszSrc)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszDest)  );

	if(argc==4)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszUsername)  );

		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &pszPasswordArg)  );

		SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, pszUsername, pszPasswordArg, &pPassword)  );
		pszPassword = SG_string__sz(pPassword);
	}

	SG_ERR_CHECK(  SG_vv_verbs__incoming(pCtx, pszSrc, pszUsername, pszPassword, pszDest, &p_incomingChangesets, NULL)  );
	if(p_incomingChangesets)
	{
		SG_ERR_CHECK(  SG_history_result__get_root(pCtx, p_incomingChangesets, &pvaRef)  );

		SG_JS_NULL_CHECK(  (jsoReturn = JS_NewArrayObject(cx, 0, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jsoReturn));
		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaRef, jsoReturn)  );

		SG_HISTORY_RESULT_NULLFREE(pCtx, p_incomingChangesets);
	}
	else
	{
		JS_SET_RVAL(cx, vp, JSVAL_NULL);
	}
	SG_STRING_NULLFREE(pCtx, pPassword);

	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_HISTORY_RESULT_NULLFREE(pCtx, p_incomingChangesets);
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_FALSE;
}

/**
* void sg.outgoing(string src_repo_spec, string dest_repo_descriptor[, username, password])
*/
SG_JSGLUE_METHOD_PROTOTYPE(sg, outgoing)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszSrc = NULL;
	char *pszUsername = NULL;
	char *pszPasswordArg = NULL;
	const char* pszPassword = NULL;
	SG_string* pPassword = NULL;
	char *pszDest = NULL;
	SG_history_result * p_outgoingChangesets = NULL;
	SG_varray * pvaRef = NULL;
	JSObject* jsoReturn = NULL;

	SG_JS_BOOL_CHECK(2==argc || 4==argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszSrc)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszDest)  );

	if(argc==4)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszUsername)  );

		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &pszPasswordArg)  );

		SG_ERR_CHECK(  _sg_jsglue__get_password(pCtx, pszUsername, pszPasswordArg, &pPassword)  );
		pszPassword = SG_string__sz(pPassword);
	}

	SG_ERR_CHECK(  SG_vv_verbs__outgoing(pCtx, pszDest, pszUsername, pszPassword, pszSrc, &p_outgoingChangesets, NULL)  );
	if(p_outgoingChangesets)
	{
		SG_ERR_CHECK(  SG_history_result__get_root(pCtx, p_outgoingChangesets, &pvaRef)  );

		SG_JS_NULL_CHECK(  (jsoReturn = JS_NewArrayObject(cx, 0, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jsoReturn));
		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaRef, jsoReturn)  );

		SG_HISTORY_RESULT_NULLFREE(pCtx, p_outgoingChangesets);
	}
	else
	{
		JS_SET_RVAL(cx, vp, JSVAL_NULL);
	}
	SG_STRING_NULLFREE(pCtx, pPassword);

	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_HISTORY_RESULT_NULLFREE(pCtx, p_outgoingChangesets);
	SG_STRING_NULLFREE(pCtx, pPassword);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszUsername);
	SG_NULLFREE(pCtx, pszPasswordArg);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, open_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;

	SG_JS_BOOL_CHECK( argc == 1 );

    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_repo_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

	SG_ERR_CHECK(  sg_jsglue__get_repo_from_jsval(pCtx, cx, argv[0], &pRepo)  );

    SG_ERR_CHECK(  SG_safeptr__wrap__repo(pCtx, pRepo, &psp)  );
    JS_SetPrivate(cx, jso, psp);
    psp = NULL;

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, open_local_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;

    SG_JS_BOOL_CHECK(argc==0);

    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_repo_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

	SG_ERR_CHECK(  sg_jsglue__get_repo_from_cwd(pCtx, &pRepo, NULL, NULL)  );

    SG_ERR_CHECK(  SG_safeptr__wrap__repo(pCtx, pRepo, &psp)  );
    JS_SetPrivate(cx, jso, psp);
    psp = NULL;

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, rename_repo)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);  
  
    char *pszRepoName = NULL;
    char *pszNewRepoName = NULL;

	SG_JS_BOOL_CHECK( argc == 2 );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );	
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );

    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszRepoName)  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszNewRepoName)  );

	SG_ERR_CHECK(  SG_closet__descriptors__rename(pCtx, pszRepoName, pszNewRepoName)  );
			
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszRepoName);
	SG_NULLFREE(pCtx, pszNewRepoName);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS

	SG_NULLFREE(pCtx, pszRepoName);
	SG_NULLFREE(pCtx, pszNewRepoName);
    return JS_FALSE;
}
SG_JSGLUE_METHOD_PROTOTYPE(sg, delete_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);  
	char *pszName = NULL;

	SG_JS_BOOL_CHECK( argc == 1 );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );	
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );

    SG_ERR_CHECK(  SG_repo__delete_repo_instance(pCtx, pszName)  );
    SG_ERR_CHECK(  SG_closet__descriptors__remove(pCtx, pszName)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszName);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS

	SG_NULLFREE(pCtx, pszName);
    return JS_FALSE;
}


SG_JSGLUE_METHOD_PROTOTYPE(sg, console)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    char * msg = NULL;

	SG_JS_BOOL_CHECK(0 < argc);

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &msg)  );
    
	SG_console(pCtx, SG_CS_STDOUT, "%s\n", msg );     
	
	SG_NULLFREE(pCtx, msg);
	return JS_TRUE;
fail:
	SG_NULLFREE(pCtx, msg);
	return JS_FALSE;
}
SG_JSGLUE_METHOD_PROTOTYPE(sg, log)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * level = NULL;
	char * msg = NULL;

	SG_JS_BOOL_CHECK(0 < argc);

	if (argc == 1)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &msg)  );

		SG_log__report_info(pCtx, "%s", msg);
	}
	else
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &level)  );
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &msg)  );

		if (0 == strcmp(level, "debug"))
		{
			SG_log__report_verbose(pCtx, "%s", msg);
		}
		else if (0 == strcmp(level, "urgent"))
		{
			SG_log__report_error(pCtx, "%s", msg);
		}
		else if (0 == strcmp(level, "normal"))
		{
			SG_log__report_info(pCtx, "%s", msg);
		}
		else
		{
			SG_log__report_verbose(pCtx, "%s", msg);
		}
	}
	SG_NULLFREE(pCtx, level);
	SG_NULLFREE(pCtx, msg);
	return JS_TRUE;
fail:
	SG_NULLFREE(pCtx, level);
	SG_NULLFREE(pCtx, msg);
	return JS_FALSE;
}


SG_JSGLUE_METHOD_PROTOTYPE(file, write)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_pathname* pPath = NULL;
	SG_file* pFile = NULL;
	char *psz_contents = NULL;
	SG_fsobj_perms perms = 0644;
	
	SG_JS_BOOL_CHECK(  2 <= argc && argc <= 3 );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_contents)  );
	if (3 == argc)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[2]));
		perms = JSVAL_TO_INT(argv[2]);
	}

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY, perms, &pFile)   );

	SG_ERR_CHECK(  SG_file__truncate(pCtx, pFile)   );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, psz_contents)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)   );

	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, psz_contents);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, psz_contents);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(file, append)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_pathname* pPath = NULL;
	SG_file* pFile = NULL;
	char *psz_arg2 = NULL;

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_arg2)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY,  0644, &pFile)   );
	SG_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, NULL)   );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, psz_arg2)   );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)   );

	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, psz_arg2);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, psz_arg2);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	/* TODO */
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(file, read)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_pathname* pPath = NULL;
	SG_file* pFile = NULL;
	SG_string * pString = NULL;
	jsval tmp=JSVAL_VOID;

	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)   );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)   );
	SG_ERR_CHECK(  SG_file__read_utf8_file_into_string(pCtx, pFile, pString)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)   );

	SG_PATHNAME_NULLFREE(pCtx, pPath);

	JSVAL_FROM_SZ(tmp, SG_string__sz(pString));
	JS_SET_RVAL(cx, vp, tmp);
	SG_STRING_NULLFREE(pCtx, pString);
	return JS_TRUE;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pString);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	/* TODO */
	return JS_FALSE;
}

// sg.file.hash(FILE_PATH[, HASH_METHOD])
// Returns a string with the hash of the file's contents.
// Default HASH_METHOD is "SHA1/160".
SG_JSGLUE_METHOD_PROTOTYPE(file, hash)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *szHashMethod = NULL;
	SG_pathname *pPath = NULL;
	SG_file *pFile = NULL;
	SG_error err = SG_ERR_OK;
	SGHASH_handle *pHashHandle = NULL;
	SG_byte buf[1024*4];
	SG_uint32 len = 0;
	jsval tmp=JSVAL_VOID;

	SG_JS_BOOL_CHECK(argc==1 || argc==2);
	SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
	SG_JS_BOOL_CHECK(argc<2 || JSVAL_IS_STRING(argv[1]));
	
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(argv[0]))  );

	if(argc>=2)
	{
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &szHashMethod)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_strdup(pCtx, "SHA1/160", &szHashMethod)  );
	}
	err = SGHASH_init(szHashMethod, &pHashHandle);
	if(!SG_IS_OK(err))
		SG_ERR_THROW(err);

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)   );
	SG_file__read(pCtx, pFile, sizeof(buf), buf, &len);
	while(!SG_CONTEXT__HAS_ERR(pCtx))
	{
		err = SGHASH_update(pHashHandle, buf, len);
		if(!SG_IS_OK(err))
			SG_ERR_THROW(err);
		SG_file__read(pCtx, pFile, sizeof(buf), buf, &len);
	}
	SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_EOF);

	err = SGHASH_final(&pHashHandle, (char *)buf, sizeof(buf));
	if(!SG_IS_OK(err))
		SG_ERR_THROW(err);

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)   );
	SG_PATHNAME_NULLFREE(pCtx, pPath);

	JSVAL_FROM_SZ(tmp, (char *)buf);
	JS_SET_RVAL(cx, vp, tmp);
	SG_NULLFREE(pCtx, szHashMethod);
	return JS_TRUE;
fail:
	SGHASH_abort(&pHashHandle);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, szHashMethod);
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(cbuffer, length)
{
    SG_context* pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    SG_cbuffer* pCbuffer = NULL;

	SG_UNUSED(obj);
    SG_UNUSED(id);

    psp = sg_jsglue__get_object_private(cx, obj);
    SG_ERR_CHECK(  SG_safeptr__unwrap__cbuffer(pCtx, psp, &pCbuffer)  );

    *vp = INT_TO_JSVAL(pCbuffer->len);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(cbuffer, fin)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_cbuffer* pCbuffer = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

	SG_JS_BOOL_CHECK(argc==0);

	SG_ERR_CHECK(  SG_safeptr__unwrap__cbuffer(pCtx, psp, &pCbuffer)  );
	SG_cbuffer__nullfree(&pCbuffer);
	SG_SAFEPTR_NULLFREE(pCtx, psp);
	JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, find_new_dagnodes_since)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    const char* psz_dagnum = NULL;
    SG_varray* pva_leafset = NULL;
    SG_ihash* pih_result = NULL;
    SG_uint64 dagnum = 0;
	JSObject *jso = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "dagnum", &psz_dagnum)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__varray(pCtx, pvh_args, pvh_got, "leafset", &pva_leafset)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "find_new_dagnodes_since", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
    SG_ERR_CHECK(  SG_repo__find_new_dagnodes_since(pCtx, pRepo, dagnum, pva_leafset, &pih_result)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_ihash_into_jsobject(pCtx, cx, pih_result, jso)  );
    SG_IHASH_NULLFREE(pCtx, pih_result);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, install_vc_hook)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    const char* psz_interface = NULL;
    const char* psz_js = NULL;
	const char *module = NULL;
	SG_uint32 version = 0;
	SG_bool replace = SG_FALSE;
    SG_audit q;
	const char *uid = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "interface", &psz_interface)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "js", &psz_js)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "replace", SG_FALSE, &replace)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "module", NULL, &module)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "userid", NULL, &uid)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "version", 0, &version)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "install_vc_hook", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	if (! uid)
		uid = SG_AUDIT__WHO__FROM_SETTINGS;

    SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,uid)  );
    SG_ERR_CHECK(  SG_vc_hooks__install(
                pCtx, 
                pRepo, 
                psz_interface,
                psz_js,
				module,
				version,
				replace,
                &q
                )  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, vc_hooks_installed)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char *psz_interface = NULL;
	SG_varray *recs = NULL;
	JSObject *jso = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_interface)  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
	SG_ERR_CHECK(  SG_vc_hooks__lookup_by_interface(pCtx, pRepo, psz_interface, &recs)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, recs, jso)  );

	SG_VARRAY_NULLFREE(pCtx, recs);
	SG_NULLFREE(pCtx, psz_interface);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS

	SG_VARRAY_NULLFREE(pCtx, recs);
	SG_NULLFREE(pCtx, psz_interface);

    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, get_status_of_version_control_locks)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_vhash* pvh_result = NULL;
    JSObject* jso = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_locks_by_branch = NULL;
    SG_vhash* pvh_branches_needing_merge = NULL;
    SG_vhash* pvh_duplicate_locks = NULL;
    SG_vhash* pvh_violated_locks = NULL;

	// TODO 2012/03/05 forcing argc to be 1 when we don't
	// TODO            define any named parameters seems a
	// TODO            bit harsh.
	// TODO
	// TODO            This requires them to say:
	// TODO            var locks = repo.get_status_of_version_control_locks({});
	// TODO
	// TODO            Allow 0 or 1 so they can also say:
	// TODO            var locks = repo.get_status_of_version_control_locks();
	// 
	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "get_status_of_version_control_locks", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );

    SG_ERR_CHECK(  SG_vc_locks__check__okay_to_push(
                pCtx, 
                pRepo, 
                NULL,
                &pvh_locks_by_branch,
                &pvh_duplicate_locks,
                &pvh_violated_locks
                )  );

    if (pvh_locks_by_branch)
    {
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "locks", &pvh_locks_by_branch)  );
    }

    if (pvh_duplicate_locks)
    {
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "duplicate", &pvh_duplicate_locks)  );
    }

    if (pvh_violated_locks)
    {
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "violated", &pvh_violated_locks)  );
    }

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_result, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
    SG_VHASH_NULLFREE(pCtx, pvh_locks_by_branch);
    SG_VHASH_NULLFREE(pCtx, pvh_branches_needing_merge);
    SG_VHASH_NULLFREE(pCtx, pvh_violated_locks);
    SG_VHASH_NULLFREE(pCtx, pvh_duplicate_locks);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, close)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

    SG_JS_BOOL_CHECK(argc==0);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
    SG_REPO_NULLFREE(pCtx, pRepo);
    SG_SAFEPTR_NULLFREE(pCtx, psp);
    JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, set_user)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char *psz_username = NULL;

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_username)  );

    SG_ERR_CHECK(  SG_user__set_user__repo(pCtx, pRepo, psz_username)  );
    SG_NULLFREE(pCtx, psz_username);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    SG_NULLFREE(pCtx, psz_username);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_group)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    const char* psz_name = NULL;
    const char* psz_recid = NULL;
    SG_vhash* pvh = NULL;
    JSObject* jso = NULL;
    SG_uint32 count_given = 0;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "name", NULL, &psz_name)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "recid", NULL, &psz_recid)  );
    if (psz_name)
    {
        count_given++;
    }
    if (psz_recid)
    {
        count_given++;
    }
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "fetch_group", pvh_args, pvh_got)  );

    if (1 != count_given)
    {
        SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
                (pCtx, "Must specific exactly one of name and recid")
                );
    }

    if (psz_name)
    {
        SG_ERR_CHECK(  SG_group__get_by_name(pCtx, pRepo, psz_name, &pvh)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_group__get_by_recid(pCtx, pRepo, psz_recid, &pvh)  );
    }


    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, create_group)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    const char* psz_name = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "name", &psz_name)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "create_group", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_group__create(pCtx, pRepo, psz_name)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_FALSE;
}


/*  usage: sg.create_user_force_recid({name:username, userid:id, inactive:bool})
 */ 
SG_JSGLUE_METHOD_PROTOTYPE(repo, create_user_force_recid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    const char* psz_username = NULL;
    const char* psz_userid = NULL;  
    SG_bool b_inactive = SG_FALSE;
    
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;   

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
    SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "name", &psz_username)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "userid", &psz_userid)  );   
    SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "inactive", SG_FALSE, &b_inactive)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "create_user_force_recid", pvh_args, pvh_got)  );
    
    SG_ERR_CHECK(  SG_user__create_force_recid(pCtx, psz_username, psz_userid, b_inactive, pRepo)  );
        
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

     JS_SET_RVAL(cx, vp, JSVAL_VOID);
  
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);;

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, create_user)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char *psz_username = NULL;
    char* psz_userid = NULL;
    jsval tmp=JSVAL_VOID;

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_username)  );

    SG_ERR_CHECK(  SG_user__create(pCtx, pRepo, psz_username, &psz_userid)  );

    JSVAL_FROM_SZ(tmp, psz_userid);
    JS_SET_RVAL(cx, vp, tmp);
    SG_NULLFREE(pCtx, psz_userid);
    SG_NULLFREE(pCtx, psz_username);
    return JS_TRUE;

fail:
    SG_NULLFREE(pCtx, psz_userid);
    SG_NULLFREE(pCtx, psz_username);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    return JS_FALSE;
}

// usage:  history(max_returned, user, stamp, date_from, date_to, branch, file_folder_path, hidemerges);
SG_JSGLUE_METHOD_PROTOTYPE(repo, history)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;
	SG_uint32 max = 25; // default
	
	SG_history_result* pHistResult = NULL;
    SG_vhash* pvh_result = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_varray *pvaRef = NULL;

	SG_history_token * pToken = NULL;

	char *pszUser = NULL;
	char *pszStamp = NULL;
	char *pszTemp = NULL;
	SG_stringarray * psaPaths = NULL;

    SG_bool bHideMerges = SG_FALSE;
	SG_bool bReassembleDag = SG_FALSE;
    SG_bool bResult = SG_FALSE;
	SG_bool bListAll = SG_FALSE;		// TODO 2012/07/05 consider supporting this.
	SG_int64 nDateFrom = 0;
	SG_int64 nDateTo = SG_INT64_MAX;

	SG_rev_spec* pRevSpec = NULL;
   
	SG_JS_BOOL_CHECK( argc <= 9 );

    //max
	if ((argc >= 1) && ! JSVAL_IS_NULL(argv[0]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[0])  );
		max = (SG_uint32)JSVAL_TO_INT(argv[0]);

		if (max == 0)
			max = SG_INT32_MAX;
	}

    //user
	if ((argc >= 2) && ! JSVAL_IS_NULL(argv[1]))
	{
        SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
        SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszUser)  );
    }    	

    //stamp
	if ((argc >= 3) && ! JSVAL_IS_NULL(argv[2]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszStamp)  );
	}

    //date from
	if ((argc >= 4) && ! JSVAL_IS_NULL(argv[3]))
	{
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &pszTemp)  );
        SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &nDateFrom, pszTemp)  );
        SG_NULLFREE(pCtx, pszTemp);
	}

    //date to
	if ((argc >= 5) && ! JSVAL_IS_NULL(argv[4]))
	{
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[4]), &pszTemp)  );
        SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &nDateTo, pszTemp)  );
        SG_NULLFREE(pCtx, pszTemp);
	}


    //branch
    if ((argc >= 6) && ! JSVAL_IS_NULL(argv[5]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[5])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[5]), &pszTemp)  );
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszTemp)  );
        SG_NULLFREE(pCtx, pszTemp);
	}

    //file or folder path
    if ((argc >= 7) && ! JSVAL_IS_NULL(argv[6]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[6])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[6]), &pszTemp)  );
		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaPaths, 1)  );
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaPaths, pszTemp)  );
        SG_NULLFREE(pCtx, pszTemp);
	}

    //hide-merges
	if ((argc >= 8) && ! JSVAL_IS_NULL(argv[7]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[7])  );
		bHideMerges = (SG_uint32)JSVAL_TO_BOOLEAN(argv[7]);	
	}
    
	//reassemble-dag
	if ((argc >= 9) && ! JSVAL_IS_NULL(argv[8]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[8])  );
		bReassembleDag = (SG_uint32)JSVAL_TO_BOOLEAN(argv[8]);	
	}
    
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_ERR_CHECK(  sg_vv2__history__repo2(pCtx, pRepo, psaPaths, pRevSpec, NULL, pszUser, pszStamp, max, bHideMerges, nDateFrom, nDateTo, bListAll, bReassembleDag, &bResult, &pHistResult, &pToken ) );

    SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psaPaths);

    if (bResult)
    {           
        SG_varray* pvaRefNew = NULL;
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        
        SG_ERR_CHECK(  SG_history_result__get_root(pCtx, pHistResult, &pvaRef)  );
        SG_ERR_CHECK(  SG_varray__alloc__copy(pCtx, &pvaRefNew, pvaRef)  );
        SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, "items", &pvaRefNew)  ); 
        SG_VARRAY_NULLFREE(pCtx, pvaRefNew);
        if (pToken)
        {         
            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "next_token", (SG_vhash**)&pToken)  );  
          
        }
        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_result, jso)  );   
            
        SG_VHASH_NULLFREE(pCtx, pvh_result);   
        SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult); 
        SG_HISTORY_TOKEN_NULLFREE(pCtx, pToken);
     
    }
    else
    {
        SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
    }
    
    SG_NULLFREE(pCtx, pszUser);
    SG_NULLFREE(pCtx, pszStamp);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_result); 
    SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psaPaths);

 	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);
    SG_HISTORY_TOKEN_NULLFREE(pCtx, pToken);
    
    SG_NULLFREE(pCtx, pszUser);
    SG_NULLFREE(pCtx, pszStamp);
    SG_NULLFREE(pCtx, pszTemp);
    return JS_FALSE;
}

//usage:  history_fetch_more(reponame, max_returned);
SG_JSGLUE_METHOD_PROTOTYPE(repo, history_fetch_more)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;

	SG_uint32 max = 25; // default
	
	SG_history_result* pHistResult = NULL;
    SG_vhash* pvh_result = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
  

    SG_varray *pvaRef = NULL;

	SG_vhash* pToken = NULL;
    SG_history_token * pNewToken = NULL;

   
	SG_JS_BOOL_CHECK( argc >= 1 );
    SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pToken)  );

	if ((argc >= 2) && ! JSVAL_IS_NULL(argv[1]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[1])  );
		max = (SG_uint32)JSVAL_TO_INT(argv[1]);

		if (max == 0)
			max = SG_INT32_MAX;
	}
   
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
    SG_ERR_CHECK(  sg_vv2__history__fetch_more__repo2(pCtx, pRepo, (SG_history_token*)pToken, max, &pHistResult, &pNewToken)  ); 
 	
    if (pHistResult)
    {           
        SG_varray* pvaRefNew = NULL;

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        
        SG_ERR_CHECK(  SG_history_result__get_root(pCtx, pHistResult, &pvaRef)  );
        SG_ERR_CHECK(  SG_varray__alloc__copy(pCtx, &pvaRefNew, pvaRef)  );
        SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh_result, "items", &pvaRefNew)  ); 
        SG_VARRAY_NULLFREE(pCtx, pvaRefNew);
        if (pNewToken)
        {         
            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "next_token", (SG_vhash**)&pNewToken)  );             
        }
        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_result, jso)  );   
        
        
        SG_VHASH_NULLFREE(pCtx, pvh_result);    
        SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);
        SG_VHASH_NULLFREE(pCtx, pToken);
        SG_HISTORY_TOKEN_NULLFREE(pCtx, pNewToken);

    }
    else
    {
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
    }
    
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
 	
   
    SG_VHASH_NULLFREE(pCtx, pvh_result); 
    SG_VHASH_NULLFREE(pCtx, pToken);
    SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);
    SG_HISTORY_TOKEN_NULLFREE(pCtx, pNewToken);
    return JS_FALSE;
}



/*SG_JSGLUE_METHOD_PROTOTYPE(repo, history_old)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;
	SG_uint32 max = 25; // default
	SG_varray *pvaRef = NULL;
	SG_history_result* pHistResult = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	SG_uint32 startWith = 0;
	const char *historyUser = NULL;
	const char *stamp = NULL;
	SG_int64 nDateFrom = 0;
	SG_int64 nDateTo = SG_INT64_MAX;

    SG_UNUSED(obj);
    SG_UNUSED(argc);

	SG_JS_BOOL_CHECK( argc <= 6 );

	if ((argc >= 1) && ! JSVAL_IS_NULL(argv[0]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[0])  );
		max = (SG_uint32)JSVAL_TO_INT(argv[0]);

		if (max == 0)
			max = SG_INT32_MAX;
	}

	if ((argc >= 2) && ! JSVAL_IS_NULL(argv[1]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[1])  );
		startWith = (SG_uint32)JSVAL_TO_INT(argv[1]);
    }

	if ((argc >= 3) && ! JSVAL_IS_NULL(argv[2]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
		historyUser = JS_GetStringBytes(JSVAL_TO_STRING(argv[2])); 
	}

	if ((argc >= 4) && ! JSVAL_IS_NULL(argv[3]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		stamp = JS_GetStringBytes(JSVAL_TO_STRING(argv[3])); 
	}

	if ((argc >= 5) && ! JSVAL_IS_NULL(argv[4]))
	{
		const char *timestr;
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[4])  );
		timestr = JS_GetStringBytes( JSVAL_TO_STRING(argv[4]) );

		SG_ERR_CHECK(  SG_time__parse__informal__local(pCtx, timestr, &nDateFrom, SG_FALSE)  );
	}

	if ((argc >= 6) && ! JSVAL_IS_NULL(argv[5]))
	{
		const char *timestr;
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[5])  );
		timestr = JS_GetStringBytes( JSVAL_TO_STRING(argv[5]) );

		SG_ERR_CHECK(  SG_time__parse__informal__local(pCtx, timestr, &nDateTo, SG_TRUE)  );
	}

	

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_ERR_CHECK(  SG_history__list(pCtx, pRepo, max, startWith, historyUser, stamp, nDateFrom, nDateTo, NULL, &pHistResult, NULL)  );

	SG_ERR_CHECK(  SG_history_result__get_root(pCtx, pHistResult, &pvaRef)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    *rval = OBJECT_TO_JSVAL(jso);
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaRef, jso)  );

 	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
 	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);

    return JS_FALSE;
}*/

// repo r.changeset_history_contains(baseline, other) takes two CSIDs and returns true if
// "other" is an ancestor of or equal to "baseline", and false otherwise.
SG_JSGLUE_METHOD_PROTOTYPE(repo, changeset_history_contains)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_repo * pRepo = NULL;
	char * pszBaseline = NULL;
	char * pszOther = NULL;
	SG_dagquery_relationship rel = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
	SG_safeptr * psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK( 2 == argc );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[1]) );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBaseline)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszOther)  );

	SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszOther, pszBaseline, SG_TRUE, SG_FALSE, &rel)  );

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(rel==SG_DAGQUERY_RELATIONSHIP__ANCESTOR || rel==SG_DAGQUERY_RELATIONSHIP__SAME));
	SG_NULLFREE(pCtx, pszBaseline);
	SG_NULLFREE(pCtx, pszOther);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszBaseline);
	SG_NULLFREE(pCtx, pszOther);
	return JS_FALSE;
}

// repo r.count_new_since_common(baseline, other) returns the length of the list
// you would get by calling r.merge_preview(baseline, other). It is the number
// of changesets that are in 'baseline's history but not in 'other's history.
SG_JSGLUE_METHOD_PROTOTYPE(repo, count_new_since_common)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_repo * pRepo = NULL;
	char * pszBaseline = NULL;
	char * pszOther = NULL;
	SG_stringarray * psaResults = NULL;
	SG_uint32 countResults = 0;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK( 2 == argc );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[1]) );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBaseline)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszOther)  );

	SUSPEND_REQUEST_ERR_CHECK(  SG_dagquery__find_new_since_common(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszBaseline, pszOther, &psaResults)  );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaResults, &countResults)  );

	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(countResults));
	SG_STRINGARRAY_NULLFREE(pCtx, psaResults);
	SG_NULLFREE(pCtx, pszBaseline);
	SG_NULLFREE(pCtx, pszOther);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_STRINGARRAY_NULLFREE(pCtx, psaResults);
	SG_NULLFREE(pCtx, pszBaseline);
	SG_NULLFREE(pCtx, pszOther);
	return JS_FALSE;
}

// repo r.merge_preview(baseline, other, detailed=true) returning an array of history results
// ... or pass in detailed= false to get a raw list of changeset ids.
SG_JSGLUE_METHOD_PROTOTYPE(repo, merge_preview)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	JSObject * jso = NULL;
	SG_stringarray * psaResults = NULL;
	SG_uint32 countResults = 0;
	const char * const * ppszResults = 0;
	SG_varray * pvaResults = NULL;
	SG_repo * pRepo = NULL;
	SG_repo_fetch_dagnodes_handle * pDagnodeFetcher = NULL;
	SG_dagnode * pDagnode = NULL;
	char * pszBaseline = NULL;
	char * pszOther = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_uint32 i = 0;

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK( 2 == argc || 3==argc);
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[1]) );
	SG_JS_BOOL_CHECK( argc<3 || JSVAL_IS_BOOLEAN(argv[2]) );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBaseline)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszOther)  );
	
	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

	if(argc>=3 && argv[2]==JSVAL_FALSE)
	{
		SUSPEND_REQUEST_ERR_CHECK(  SG_dagquery__find_new_since_common(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszBaseline, pszOther, &psaResults)  );
		SG_ERR_CHECK(  sg_jsglue__copy_stringarray_into_jsobject(pCtx, cx, psaResults, jso)  );
	}
	else
	{
		SG_JS_SUSPENDREQUEST();

		REQUEST_SUSPENDED_ERR_CHECK(  SG_dagquery__find_new_since_common(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszBaseline, pszOther, &psaResults)  );

		REQUEST_SUSPENDED_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, psaResults, &ppszResults, &countResults)  );
		REQUEST_SUSPENDED_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaResults)  );
		REQUEST_SUSPENDED_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &pDagnodeFetcher)  );
		for(i=0; i<countResults; ++i)
		{
			SG_vhash * pvhResult = NULL;
			SG_uint32 revno = 0;
			REQUEST_SUSPENDED_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaResults, &pvhResult)  );
			REQUEST_SUSPENDED_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResult, "changeset_id", ppszResults[i])  );
			
			REQUEST_SUSPENDED_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pRepo, pDagnodeFetcher, ppszResults[i], &pDagnode)  );
			REQUEST_SUSPENDED_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pDagnode, &revno)  );
			SG_DAGNODE_NULLFREE(pCtx, pDagnode);
			REQUEST_SUSPENDED_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhResult, "revno", revno)  );
		}
		REQUEST_SUSPENDED_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pDagnodeFetcher)  );
		REQUEST_SUSPENDED_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pvaResults)  );
		
		SG_JS_RESUMEREQUEST();

		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaResults, jso)  );
		SG_VARRAY_NULLFREE(pCtx, pvaResults);
	}

	SG_STRINGARRAY_NULLFREE(pCtx, psaResults);
	SG_NULLFREE(pCtx, pszBaseline);
	SG_NULLFREE(pCtx, pszOther);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_STRINGARRAY_NULLFREE(pCtx, psaResults);
	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
	if(pDagnodeFetcher!=NULL)
	{
		SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pDagnodeFetcher)  );
	}
	SG_NULLFREE(pCtx, pszBaseline);
	SG_NULLFREE(pCtx, pszOther);
	return JS_FALSE;
}

// <repo>.merge_review(start, singleMergeReview, resultLimit, historyDetails, mergeBaselines);
// Returns an object containing vhashes in the same format returned by SG_mergereview(),
// optionally filled in with history details from SG_history__fill_in_details() as well.
// The last object in the list is always a "continuation token", which could be null if the root of the VC dag was reached.
SG_JSGLUE_METHOD_PROTOTYPE(repo, merge_review)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * szHead = NULL;
	SG_varray * pStartToken = NULL;
	SG_varray * pNextNodeToken = NULL;
	SG_bool singleMergeReview = SG_FALSE;
	SG_int32 resultLimit = 0;
	SG_bool historyDetails = SG_FALSE;
	SG_vhash * pMergeBaselines = NULL;
	JSObject * jso = NULL;
	SG_varray * pvaResults = NULL;
	SG_repo * pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
	
	SG_JS_BOOL_CHECK( argc==4 || argc==5 );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) || JSVAL_IS_NONNULL_OBJECT(argv[0]) );
	SG_JS_BOOL_CHECK( JSVAL_IS_BOOLEAN(argv[1]) );
	SG_JS_BOOL_CHECK( JSVAL_IS_INT(argv[2]));
	SG_JS_BOOL_CHECK( JSVAL_IS_BOOLEAN(argv[3]) );
	SG_JS_BOOL_CHECK( argc<5 || JSVAL_IS_OBJECT(argv[4]) );
	
	if(JSVAL_IS_STRING(argv[0]))
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szHead)  );
	else
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pStartToken)  );
	
	singleMergeReview = JSVAL_TO_BOOLEAN(argv[1]);
	resultLimit = JSVAL_TO_INT(argv[2]);
	historyDetails = JSVAL_TO_BOOLEAN(argv[3]);
	
	if(argc>=5 && !JSVAL_IS_NULL(argv[4]))
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[4]), &pMergeBaselines)  );
	
	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

	SG_JS_SUSPENDREQUEST();

	if(pStartToken!=NULL)
	{
		REQUEST_SUSPENDED_ERR_CHECK(  SG_mergereview__continue(pCtx, pRepo, pStartToken, singleMergeReview, pMergeBaselines, resultLimit, &pvaResults, NULL, &pNextNodeToken)  );
		SG_VARRAY_NULLFREE(pCtx, pStartToken);
	}
	else
	{
		REQUEST_SUSPENDED_ERR_CHECK(  SG_mergereview(pCtx, pRepo, szHead, singleMergeReview, pMergeBaselines, resultLimit, &pvaResults, NULL, &pNextNodeToken)  );
	}

	if(historyDetails)
	{
		REQUEST_SUSPENDED_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pvaResults)  );
	}

	SG_VHASH_NULLFREE(pCtx, pMergeBaselines);

	if(pNextNodeToken!=NULL)
	{
		REQUEST_SUSPENDED_ERR_CHECK(  SG_varray__append__varray(pCtx, pvaResults, &pNextNodeToken)  );
	}
	else
	{
		REQUEST_SUSPENDED_ERR_CHECK(  SG_varray__append__null(pCtx, pvaResults)  );
	}

	SG_JS_RESUMEREQUEST();

	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaResults, jso)  );
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_NULLFREE(pCtx, szHead);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VARRAY_NULLFREE(pCtx, pStartToken);
	SG_VHASH_NULLFREE(pCtx, pMergeBaselines);
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_NULLFREE(pCtx, szHead);
	return JS_FALSE;
}

// <repo>.fill_in_history_details(list);
// Returns a copy of the list with SG_history__fill_in_details() called on it.
SG_JSGLUE_METHOD_PROTOTYPE(repo, fill_in_history_details)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	JSObject * jso = NULL;
	SG_varray * pvaResults = NULL;
	SG_repo * pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
	
	SG_JS_BOOL_CHECK( argc==1 );
	SG_JS_BOOL_CHECK( JSVAL_IS_NONNULL_OBJECT(argv[0]) );
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvaResults)  );

	SUSPEND_REQUEST_ERR_CHECK(  SG_history__fill_in_details(pCtx, pRepo, pvaResults)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaResults, jso)  );

	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	return JS_FALSE;
}

// repo r.csid_to_revno(csid) takes a csid and returns the corresponding revno.
SG_JSGLUE_METHOD_PROTOTYPE(repo, csid_to_revno)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_repo * pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	char * pszHid = NULL;
	SG_dagnode * pDagnode = NULL;
	SG_uint32 revno = 0;

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(1==argc);
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING(argv[0]) );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszHid)  );

	SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszHid, &pDagnode);
	if(SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
	{
		SG_context__err_reset(pCtx);
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	else
	{
		SG_ERR_CHECK_CURRENT;
		SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pDagnode, &revno)  );
		SG_DAGNODE_NULLFREE(pCtx, pDagnode);
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(revno));
	}

	SG_NULLFREE(pCtx, pszHid);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
	SG_NULLFREE(pCtx, pszHid);
	return JS_FALSE;
}

// repo r.revno_to_csid(csid) takes a revno and returns the corresponding csid.
SG_JSGLUE_METHOD_PROTOTYPE(repo, revno_to_csid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_repo * pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_uint32 revno = 0;
	char * psz_hid = NULL;

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(1==argc);
	SG_JS_BOOL_CHECK( JSVAL_IS_INT(argv[0]) );
	revno = (SG_uint32)JSVAL_TO_INT(argv[0]);

	SG_repo__find_dagnode_by_rev_id(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, revno, &psz_hid);
	if(SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
	{
		SG_context__err_reset(pCtx);
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	else
	{
		jsval v;
		SG_ERR_CHECK_CURRENT;
		JSVAL_FROM_SZ(v, psz_hid);
		JS_SET_RVAL(cx, vp, v);
		SG_NULLFREE(pCtx, psz_hid);
	}

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_hid);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_blob_into_tempfile)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    SG_tempfile* ptf = NULL;
    SG_blob_encoding encoding = 0;
    char* psz_vcdiff_reference = NULL;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    jsval jv;
    char *psz_blobid = NULL;
    SG_bool b_convert_to_full = SG_FALSE;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(  2 == argc  );

    // blobid
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_blobid)  );

    // convert_to_full
    SG_JS_BOOL_CHECK(  JSVAL_IS_BOOLEAN(argv[1])  );
    b_convert_to_full = JSVAL_TO_BOOLEAN(argv[1]);

    if (b_convert_to_full)
    {
        SG_ERR_CHECK( SG_tempfile__open_new(pCtx, &ptf)  );
        SG_ERR_CHECK(  
                SG_repo__fetch_blob_into_file(
                    pCtx, 
                    pRepo, 
                    psz_blobid,
                    ptf->pFile,
                    &len_full
                    )  );
        SG_ERR_CHECK( SG_tempfile__close(pCtx, ptf) );
        SG_ASSERT(ptf->pPathname);
        SG_ASSERT(!ptf->pFile);
    }
    else
    {
        SG_ERR_CHECK( SG_tempfile__open_new(pCtx, &ptf)  );
        SG_ERR_CHECK(  
                SG_repo__fetch_blob_into_file__encoded(
                    pCtx, 
                    pRepo, 
                    psz_blobid,
                    ptf->pFile,
                    &encoding,
                    &psz_vcdiff_reference,
                    &len_encoded,
                    &len_full
                    )  );
        SG_ERR_CHECK( SG_tempfile__close(pCtx, ptf) );
        SG_ASSERT(ptf->pPathname);
        SG_ASSERT(!ptf->pFile);
    }

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

    JSVAL_FROM_SZ(jv, SG_pathname__sz(ptf->pPathname));
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "path", &jv)  );

    JSVAL_FROM_SZ(jv, psz_blobid);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "blobid", &jv)  );

    {
        SG_int_to_string_buffer buf_uint64;

        JSVAL_FROM_SZ(jv, SG_uint64_to_sz(len_full, buf_uint64));
        SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "len_full", &jv)  );
    }

    if (!b_convert_to_full)
    {
        if (psz_vcdiff_reference)
        {
            JSVAL_FROM_SZ(jv, psz_vcdiff_reference);
        }
        else
        {
            jv = JSVAL_NULL;
        }
        SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "vcdiff_reference", &jv)  );

        {
            char buf_encoding[2];

            SG_ASSERT(  (((unsigned char) encoding) >= 'a')  );
            SG_ASSERT(  (((unsigned char) encoding) <= 'z')  );

            buf_encoding[0] = encoding;
            buf_encoding[1] = 0;
            JSVAL_FROM_SZ(jv, buf_encoding);
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "encoding", &jv)  );
        }

        {
            SG_int_to_string_buffer buf_uint64;

            JSVAL_FROM_SZ(jv, SG_uint64_to_sz(len_encoded, buf_uint64));
            SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "len_encoded", &jv)  );
        }
    }

    SG_PATHNAME_NULLFREE(pCtx, ptf->pPathname);
    SG_NULLFREE(pCtx, ptf);

    SG_NULLFREE(pCtx, psz_vcdiff_reference);

	SG_NULLFREE(pCtx, psz_blobid);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
	SG_NULLFREE(pCtx, psz_blobid);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_blob)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp_repo = NULL;
	SG_safeptr* psp_fbh = NULL;
	JSObject* jso = NULL;
	SG_uint64 len_full = 0;
	char *psz_blobid = NULL;
	SG_repo_fetch_blob_handle * pHandle = NULL;

	psp_repo = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

	SG_JS_BOOL_CHECK(argc==1 || argc==2);

	// blobid
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_blobid)  );

	// chunk length
	SG_JS_BOOL_CHECK(argc<2 || JSVAL_IS_INT(argv[1]));

	if(argc<2 || JSVAL_TO_INT(argv[1])<0)
	{
		SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "todo: return the entire blob as a cbuffer."));
	}
	else
	{
		jsval jv;
		SG_int_to_string_buffer buf_uint64;

		SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx,  pRepo, psz_blobid, SG_TRUE, NULL, NULL, NULL, &len_full, &pHandle)  );

		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, &sg_fetchblobhandle_class, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  SG_safeptr__wrap__fetchblobhandle(pCtx, pHandle, &psp_fbh)  );
		JS_SetPrivate(cx, jso, psp_fbh);

		SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "chunk_length", &argv[1])  );

		JSVAL_FROM_SZ(jv, SG_uint64_to_sz(len_full, buf_uint64));
		SG_JS_BOOL_CHECK(  JS_SetProperty(cx, jso, "length", &jv)  );
	}

	SG_NULLFREE(pCtx, psz_blobid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_SAFEPTR_NULLFREE(pCtx, psp_fbh);
	SG_ERR_IGNORE(  SG_repo__fetch_blob__abort(pCtx, pRepo, &pHandle)  );
	SG_NULLFREE(pCtx, psz_blobid);
	return JS_FALSE;
}

/**
 * fetchblobhandle.next_chunk() returns a cbuffer.
 * A cbuffer is essentially an object that is meant to be returned back down to
 * (or passed back up into) C code, without having to be converted back and
 * forth between a JavaScript native type/object. See sg_cbuffer_typedefs.h.
 * 
 * If *don't* pass the returned cbuffer back to C code, you can free it
 * directly from javascript by calling its ".fin()" member function.
 */
SG_JSGLUE_METHOD_PROTOTYPE(fetchblobhandle, next_chunk)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp_fbh = NULL;
	SG_repo_fetch_blob_handle * pHandle = NULL;
	SG_safeptr* psp_repo = NULL;
	SG_repo* pRepo = NULL;
	SG_safeptr* psp_cbuffer = NULL;
	SG_cbuffer * pCbuffer = NULL;
	SG_bool done = SG_FALSE;
	jsval val;
	SG_uint32 chunk_length = 0;
	JSObject * jso = NULL;

	psp_fbh = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_safeptr__unwrap__fetchblobhandle(pCtx, psp_fbh, &pHandle);
	if(SG_context__err_equals(pCtx, SG_ERR_SAFEPTR_NULL))
	{
		SG_context__err_reset(pCtx);
		JS_SET_RVAL(cx, vp, JSVAL_NULL); // Return null after the last chunk has been sent.
		return JS_TRUE;
	}
	else
		SG_ERR_CHECK_CURRENT;

	SG_JS_BOOL_CHECK(argc==1);

	SG_JS_BOOL_CHECK(JSVAL_IS_OBJECT(argv[0]));
	psp_repo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

	SG_JS_BOOL_CHECK(  JS_GetProperty(cx, JS_THIS_OBJECT(cx, vp), "chunk_length", &val)  );
	chunk_length = JSVAL_TO_INT(val);

	SG_ERR_CHECK(  SG_cbuffer__alloc__new(pCtx, &pCbuffer, chunk_length)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx,  pRepo, pHandle, pCbuffer->len, pCbuffer->pBuf, &pCbuffer->len, &done)  );

	if(done)
	{
		SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pRepo, &pHandle)  );
		SG_SAFEPTR_NULLFREE(pCtx, psp_fbh);
		JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);
	}

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_cbuffer_class, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  SG_safeptr__wrap__cbuffer(pCtx, pCbuffer, &psp_cbuffer)  );
	JS_SetPrivate(cx, jso, psp_cbuffer);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	SG_SAFEPTR_NULLFREE(pCtx, psp_cbuffer);
	SG_cbuffer__nullfree(&pCbuffer);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(fetchblobhandle, abort)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp_fbh = NULL;
	SG_repo_fetch_blob_handle * pHandle = NULL;
	SG_safeptr* psp_repo = NULL;
	SG_repo* pRepo = NULL;

	psp_fbh = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_safeptr__unwrap__fetchblobhandle(pCtx, psp_fbh, &pHandle);
	if(SG_context__err_equals(pCtx, SG_ERR_SAFEPTR_NULL))
	{
		SG_context__err_reset(pCtx);
		JS_SET_RVAL(cx, vp, JSVAL_NULL); // We seem to have already aborted or finished.
		return JS_TRUE;
	}
	else
		SG_ERR_CHECK_CURRENT;

	SG_JS_BOOL_CHECK(argc==1);

	SG_JS_BOOL_CHECK(JSVAL_IS_OBJECT(argv[0]));
	psp_repo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__fetch_blob__abort(pCtx,  pRepo, &pHandle)  );
	SG_SAFEPTR_NULLFREE(pCtx, psp_fbh);
	JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, file_classes)
{
	SG_context * pCtx = SG_jsglue__get_sg_context(cx);
    SG_vhash* pvhClasses = NULL;
    SG_repo* pRepo = NULL;    
    JSObject* jso = NULL;
     SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

    SG_JS_BOOL_CHECK(argc==0);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    // try the file classes defined in localsettings
    SG_ERR_CHECK(  SG_localsettings__get__collapsed_vhash(pCtx, SG_FILETOOLCONFIG__CLASSES, pRepo, &pvhClasses)  );
    
	JS_SET_RVAL(cx, vp, JSVAL_NULL);
  	if (pvhClasses)
    {
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhClasses, jso)  );
        SG_VHASH_NULLFREE(pCtx, pvhClasses);	 
    }
    
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhClasses);	
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, rebuild_indexes)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(0==argc);

    SG_ERR_CHECK(  SG_repo__rebuild_indexes(pCtx, pRepo)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, list_blobs)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;
    SG_safeptr* psp_repo = NULL;
    SG_safeptr* psp_blobset = NULL;
    SG_blobset* pbs = NULL;

    psp_repo = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==0);

    // TODO allow args here for encoding, limit, offset
#if 0
    if (1 == argc)
    {
        int32 jsi = 0;
        SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[0])  );
        JS_ValueToInt32(cx, argv[0], &jsi);
        limit = (SG_uint32) jsi;
    }
#endif

    SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepo, 0, 0, 0, &pbs)  );

    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_blobset_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  SG_safeptr__wrap__blobset(pCtx, pbs, &psp_blobset)  );
    JS_SetPrivate(cx, jso, psp_blobset);
    psp_blobset = NULL;

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, list_areas)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp_repo = NULL;
	JSObject *jso = NULL;
	SG_vhash *vhAreas = NULL;

	SG_JS_BOOL_CHECK(argc == 0);

    psp_repo = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

	SG_ERR_CHECK(  SG_area__list(pCtx, pRepo, &vhAreas)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, vhAreas, jso)  );
	SG_VHASH_NULLFREE(pCtx, vhAreas);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS

	SG_VHASH_NULLFREE(pCtx, vhAreas);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, area_add)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp_repo = NULL;
	SG_audit q;
	SG_uint64 area_id = 0;
	char *area_name = NULL;
	char *uid_param = NULL;
	const char *uid = SG_AUDIT__WHO__FROM_SETTINGS;

	SG_JS_BOOL_CHECK( (argc == 2) || (argc == 3) );
	SG_JS_BOOL_CHECK( JSVAL_IS_STRING( argv[0] ) );
	SG_JS_BOOL_CHECK( JSVAL_IS_INT( argv[1] ) );

	if ((argc == 3) && ! JSVAL_IS_NULL(argv[2]))
	{
		SG_JS_BOOL_CHECK( JSVAL_IS_STRING( argv[2] ) );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &uid_param)  );
		uid = uid_param;
	}

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &area_name)  );
	area_id = (SG_uint64)JSVAL_TO_INT(argv[1]);

    psp_repo = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

	SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, uid)  );

	SG_ERR_CHECK(  SG_area__add(pCtx, pRepo, area_name, area_id, &q)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);

	SG_NULLFREE(pCtx, area_name);
	SG_NULLFREE(pCtx, uid_param);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS

	SG_NULLFREE(pCtx, area_name);
	SG_NULLFREE(pCtx, uid_param);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, list_dags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    JSObject* jso = NULL;
    SG_repo* pRepo = NULL;
    SG_safeptr* psp_repo = NULL;
	SG_uint32 nDags = 0;
	SG_uint64 *paDagnums = NULL;
	SG_varray *results = NULL;
	SG_uint32 i = 0;

    psp_repo = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp_repo, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==0);

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &nDags, &paDagnums)  );
	SG_ERR_CHECK(  SG_varray__alloc(pCtx, &results)  );

	for ( i = 0; i < nDags; ++i )
	{
		SG_int64 dagnum = paDagnums[i];
		char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

		SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );
		
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, results, buf_dagnum)  );
	}

	SG_NULLFREE(pCtx, paDagnums);

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, results, jso)  );
	SG_VARRAY_NULLFREE(pCtx, results);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_VARRAY_NULLFREE(pCtx, results);
	SG_NULLFREE(pCtx, paDagnums);									  

    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(blobset, cleanup)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_blobset* pbs = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

    SG_JS_BOOL_CHECK(argc==0);

    SG_ERR_CHECK(  SG_safeptr__unwrap__blobset(pCtx, psp, &pbs)  );
    SG_BLOBSET_NULLFREE(pCtx, pbs);
    SG_SAFEPTR_NULLFREE(pCtx, psp);
    JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(blobset, get_stats)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_blobset* pbs = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_vhash* pvh_result = NULL;
    JSObject* jso = NULL;
    SG_uint32 count = 0;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    SG_uint64 max_len_encoded = 0;
    SG_uint64 max_len_full = 0;
    SG_blob_encoding encoding;
    SG_uint32 i32 = 0;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "encoding", 0, &i32)  );
    encoding = (SG_blob_encoding) i32;

    SG_ERR_CHECK(  SG_safeptr__unwrap__blobset(pCtx, psp, &pbs)  );

    SG_ERR_CHECK(  SG_blobset__get_stats(
                pCtx,
                pbs,
                encoding,
                &count,
                &len_encoded,
                &len_full,
                &max_len_encoded,
                &max_len_full
                )  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_result)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "count", count)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "len_encoded", len_encoded)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "len_full", len_full)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "max_len_encoded", max_len_encoded)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "max_len_full", max_len_full)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_result, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh_result);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(blobset, get)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_blobset* pbs = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_vhash* pvh = NULL;
    JSObject* jso = NULL;
    SG_uint32 limit = 0;
    SG_uint32 skip = 0;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__uint32(pCtx, pvh_args, pvh_got, "limit", &limit)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__uint32(pCtx, pvh_args, pvh_got, "skip", &skip)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "get", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__blobset(pCtx, psp, &pbs)  );

    SG_ERR_CHECK(  SG_blobset__get(
                pCtx, 
                pbs, 
                limit,
                skip,
                &pvh
                )  );
    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(blobset, lookup)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_blobset* pbs = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_vhash* pvh_found = NULL;
    JSObject* jso = NULL;
    const char* psz_hid = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "hid", &psz_hid)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "lookup", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__blobset(pCtx, psp, &pbs)  );

    SG_ERR_CHECK(  SG_blobset__lookup__vhash(
                pCtx, 
                pbs, 
                psz_hid, 
                &pvh_found
                )  );
    if (pvh_found)
    {
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_found, jso)  );
        SG_VHASH_NULLFREE(pCtx, pvh_found);
    }
    else
    {
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
    }

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_found);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, make_dagnum)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_bool b_admin = SG_FALSE;
    SG_bool b_trivial = SG_FALSE;
    SG_bool b_hardwired_template = SG_FALSE;
    SG_bool b_single_rectype = SG_FALSE;
    SG_uint64 dagnum = 0;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
    const char* psz_type = NULL;
    SG_uint32 id = 0;
    SG_uint32 vendor = 0;
    SG_uint32 grouping = 0;
    jsval tmp=JSVAL_VOID;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__uint32(pCtx, pvh_args, pvh_got, "id", &id)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__uint32(pCtx, pvh_args, pvh_got, "vendor", &vendor)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__uint32(pCtx, pvh_args, pvh_got, "grouping", &grouping)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "type", &psz_type)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "trivial", SG_FALSE, &b_trivial)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "hardwired_template", SG_FALSE, &b_hardwired_template)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "admin", SG_FALSE, &b_admin)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "single_rectype", SG_FALSE, &b_single_rectype)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "make_dagnum", pvh_args, pvh_got)  );

    dagnum = SG_DAGNUM__SET_DAGID(id, 0);
    dagnum = SG_DAGNUM__SET_VENDOR(vendor, dagnum);
    dagnum = SG_DAGNUM__SET_GROUPING(grouping, dagnum);

    if (0 == strcmp(psz_type, "db"))
    {
        dagnum = SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, dagnum);
    }
    else if (0 == strcmp(psz_type, "tree"))
    {
        dagnum = SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__TREE, dagnum);
    }
    else
    {
        SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);//todo: numbers, bool, null
    }

    if (b_trivial)
    {
        dagnum |= SG_DAGNUM__FLAG__TRIVIAL_MERGE;
    }
    if (b_hardwired_template)
    {
        dagnum |= SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE;
    }
    if (b_admin)
    {
        dagnum |= SG_DAGNUM__FLAG__ADMIN;
    }
    if (b_single_rectype)
    {
        dagnum |= SG_DAGNUM__FLAG__SINGLE_RECTYPE;
    }

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JSVAL_FROM_SZ(tmp, buf_dagnum);
    JS_SET_RVAL(cx, vp, tmp);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_area_id)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_uint64 dagnum = 0;
	SG_uint64 area = 0;
	char *dagstr = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &dagstr)  );

	SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, dagstr, &dagnum)  );

	area = SG_DAGNUM__GET_AREA_ID(dagnum);

	JS_SET_RVAL(cx, vp, INT_TO_JSVAL((SG_uint32)area));

	SG_NULLFREE(pCtx, dagstr);
	return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx, cx);
	SG_NULLFREE(pCtx, dagstr);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(sg, get_users_from_needs_usermap_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszDescriptorName = NULL;
	SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
	char* pszStatus = NULL;
	SG_repo* pRepo = NULL;
	char* pszHidLeaf = NULL;
	SG_varray* pvaFields = NULL;
	SG_varray* pvaResult = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszDescriptorName)  );

	SG_ERR_CHECK(  SG_repo__open_unavailable_repo_instance(pCtx, pszDescriptorName, NULL, &status, &pszStatus, &pRepo)  );
	if (SG_REPO_STATUS__NEED_USER_MAP != status)
		SG_ERR_THROW2(SG_ERR_REPO_UNAVAILABLE, (pCtx, "%s", pszStatus));

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &pszHidLeaf)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaFields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvaFields, "*")  );

	SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__USERS, pszHidLeaf, "user", NULL, "name", 0, 0, pvaFields, &pvaResult)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaResult, jso)  );

	SG_NULLFREE(pCtx, pszHidLeaf);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, pszStatus);
	SG_VARRAY_NULLFREE(pCtx, pvaFields);
	SG_VARRAY_NULLFREE(pCtx, pvaResult);
	SG_NULLFREE(pCtx, pszDescriptorName);

	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszHidLeaf);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, pszStatus);
	SG_VARRAY_NULLFREE(pCtx, pvaFields);
	SG_VARRAY_NULLFREE(pCtx, pvaResult);
	SG_NULLFREE(pCtx, pszDescriptorName);
	
	SG_jsglue__report_sg_error(pCtx, cx);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, low_level_delete_record)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_uint64 dagnum = 0;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    char *psz_hid_rec = NULL;
    char *psz_csid_parent = NULL;
    const char* psz_hid_template = NULL;
    SG_pendingdb* ptx = NULL;
    SG_changeset* pcs = NULL;
    SG_dagnode* pdn = NULL;
    SG_vhash* pvh = NULL;
    char *psz_dagnum = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(3==argc);

    // dagnum
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_dagnum)  );
    SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

    // csid
    SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[1]));
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_csid_parent)  );

    // hidrec
    SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[2]));
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_hid_rec)  );

    // fetch the template
    if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
    {
        SG_ERR_CHECK(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, dagnum, psz_csid_parent, &psz_hid_template)  );
    }

    // do the tx
    SG_ERR_CHECK(  SG_pendingdb__alloc(pCtx, pRepo, dagnum, psz_csid_parent, psz_hid_template, &ptx)  );
    SG_ERR_CHECK(  SG_pendingdb__add_parent(pCtx, ptx, psz_csid_parent)  );

    SG_ERR_CHECK(  SG_pendingdb__remove(pCtx, ptx, psz_hid_rec)  );

    // and COMMIT
    {
        SG_audit q;

        SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );
        SG_ERR_CHECK(  SG_pendingdb__commit(pCtx, ptx, &q, &pcs, &pdn)  );
    }

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

    if (pdn)
	{
        SG_int32 generation = -1;
        const char* psz_hid_cs0 = NULL;

		SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pdn, &psz_hid_cs0)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "csid", psz_hid_cs0)  );

		SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &generation)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "generation", (SG_int64) generation)  );
	}

    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_PENDINGDB_NULLFREE(pCtx, ptx);

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);

	SG_NULLFREE(pCtx, psz_dagnum);
	SG_NULLFREE(pCtx, psz_csid_parent);
	SG_NULLFREE(pCtx, psz_hid_rec);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, psz_dagnum);
	SG_NULLFREE(pCtx, psz_csid_parent);
	SG_NULLFREE(pCtx, psz_hid_rec);
    SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, create_audit)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    JSObject* jso = NULL;
    SG_audit * pq = NULL;
    SG_safeptr* psp = NULL;
    char * szArg = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(1==argc);
    SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szArg)  );

    SG_ERR_CHECK(  SG_alloc1(pCtx, pq)  );
    SG_ERR_CHECK(  SG_audit__init(pCtx, pq, pRepo, SG_AUDIT__WHEN__NOW, szArg)  );

    SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_audit_class, NULL, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  SG_safeptr__wrap__audit(pCtx, pq, &psp)  );
    JS_SetPrivate(cx, jso, psp);
    psp = NULL;

	SG_NULLFREE(pCtx, szArg);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
    
    SG_NULLFREE(pCtx, pq);

	SG_NULLFREE(pCtx, szArg);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(audit, fin)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_audit* pq = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));

    SG_JS_BOOL_CHECK(argc==0);

    SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );
    SG_NULLFREE(pCtx, pq);
    SG_SAFEPTR_NULLFREE(pCtx, psp);
    JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx); // DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, add_vc_comment)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_audit * pq;
    SG_safeptr* psp = NULL;
    char *psz_csid = NULL;
    char *psz_text = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==3);

    // csid
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_csid)  );

    // text
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_text)  );

    // audit
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
    psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
    SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

    SG_ERR_CHECK(  SG_vc_comments__add(pCtx, pRepo, psz_csid, psz_text, pq)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_vc_comments)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;
    char *pChangesetHid = NULL;
    SG_varray* pvaComments = NULL;
    JSObject* jso = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==1);
    SG_JS_BOOL_CHECK(JSVAL_IS_STRING(argv[0]));
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pChangesetHid)  );

    SG_ERR_CHECK(   SG_history__get_changeset_comments(pCtx, pRepo, pChangesetHid, &pvaComments)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaComments, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pvaComments);
	SG_NULLFREE(pCtx, pChangesetHid);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
    SG_VARRAY_NULLFREE(pCtx, pvaComments);
	SG_NULLFREE(pCtx, pChangesetHid);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, lookup_branch_names)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_varray* pva_changesets = NULL;
    SG_varray* pva_copy = NULL;
    SG_vhash* pvh_result = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    JSObject* jso = NULL;

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__varray(pCtx, pvh_args, pvh_got, "changesets", &pva_changesets)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "lookup_branch_names", pvh_args, pvh_got)  );

    SG_ERR_CHECK(  SG_varray__alloc__copy(pCtx, &pva_copy, pva_changesets)  );
    SG_ERR_CHECK(  SG_vc_branches__get_branch_names_for_given_changesets(pCtx, pRepo, &pva_copy, &pvh_result)  );

    if (pvh_result)
    {
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_result, jso)  );
        jso = NULL;

        SG_VHASH_NULLFREE(pCtx, pvh_result);
    }
    else
    {
		JS_SET_RVAL(cx, vp, JSVAL_NULL);
    }

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, named_branches)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = NULL;
	SG_vhash* pvhReturn = NULL;
	SG_vhash* pvhRefTemp = NULL;
	SG_vhash* pvhBranchPile = NULL;
	SG_bool bHas = SG_FALSE;
	JSObject* jso = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==0);

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhReturn)  );

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhBranchPile)  );

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhBranchPile, "branches", &pvhRefTemp)  );
	SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhReturn, "branches", pvhRefTemp)  );   

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhBranchPile, "values", &pvhRefTemp)  );
	SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhReturn, "values", pvhRefTemp)  );  
	
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhBranchPile, "closed", &bHas)  );
	if (bHas)
	{
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhBranchPile, "closed", &pvhRefTemp)  );
		SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhReturn, "closed", pvhRefTemp)  );  
	}

	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhReturn, jso)  );
	
	jso = NULL;
	SG_VHASH_NULLFREE(pCtx, pvhReturn);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);

    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
    SG_VHASH_NULLFREE(pCtx, pvhReturn);  
    SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
    
    return JS_FALSE;
}

// repo.add_head(branchName, csid, audit)
SG_JSGLUE_METHOD_PROTOTYPE(repo, add_head)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_audit * pq;
	SG_repo* pRepo = NULL;
	char * pszBranchName = NULL;
	char * pszCsid = NULL;
	SG_safeptr* psp = NULL;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(argc==3);

	// branchName
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBranchName)  );

	// csid
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszCsid)  );

	// audit
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
	psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

	SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pRepo, pszBranchName, pszCsid, NULL, SG_TRUE, pq)  );

	SG_NULLFREE(pCtx, pszBranchName);
	SG_NULLFREE(pCtx, pszCsid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszBranchName);
	SG_NULLFREE(pCtx, pszCsid);
	return JS_FALSE;
}

// repo.move_head(branchName, csidFrom, csidTo, audit)
SG_JSGLUE_METHOD_PROTOTYPE(repo, move_head)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_audit * pq;
	SG_repo* pRepo = NULL;
	char * pszBranchName = NULL;
	char * pszCsidFrom = NULL;
	char * pszCsidTo = NULL;
	SG_safeptr* psp = NULL;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(argc==4);

	// branchName
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBranchName)  );

	// csidFrom
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszCsidFrom)  );

	// csidTo
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszCsidTo)  );

	// audit
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[3])  );
	psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[3]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

	SG_ERR_CHECK(  SG_vc_branches__move_head(pCtx, pRepo, pszBranchName, pszCsidFrom, pszCsidTo, pq)  );

	SG_NULLFREE(pCtx, pszBranchName);
	SG_NULLFREE(pCtx, pszCsidFrom);
	SG_NULLFREE(pCtx, pszCsidTo);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszBranchName);
	SG_NULLFREE(pCtx, pszCsidFrom);
	SG_NULLFREE(pCtx, pszCsidTo);
	return JS_FALSE;
}

// repo.remove_head(branchName, csid, audit)
SG_JSGLUE_METHOD_PROTOTYPE(repo, remove_head)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_audit * pq;
	SG_repo* pRepo = NULL;
	char * pszBranchName = NULL;
	char * pszCsid = NULL;
	SG_safeptr* psp = NULL;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(argc==3);

	// branchName
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBranchName)  );

	// csid
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszCsid)  );

	// audit
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
	psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

	SG_ERR_CHECK(  SG_vc_branches__remove_head(pCtx, pRepo, pszBranchName, pszCsid, pq)  );

	SG_NULLFREE(pCtx, pszBranchName);
	SG_NULLFREE(pCtx, pszCsid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszBranchName);
	SG_NULLFREE(pCtx, pszCsid);
	return JS_FALSE;
}

// repo.close_branch(branchName, audit)
SG_JSGLUE_METHOD_PROTOTYPE(repo, close_branch)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_audit * pq;
	SG_repo* pRepo = NULL;
	char * pszBranchName = NULL;
	SG_safeptr* psp = NULL;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(argc==2);

	// branchName
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBranchName)  );

	// audit
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[1])  );
	psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[1]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

	SG_ERR_CHECK(  SG_vc_branches__close(pCtx, pRepo, pszBranchName, pq)  );

	SG_NULLFREE(pCtx, pszBranchName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszBranchName);
	return JS_FALSE;
}

// repo.reopen_branch(branchName, audit)
SG_JSGLUE_METHOD_PROTOTYPE(repo, reopen_branch)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_audit * pq;
	SG_repo* pRepo = NULL;
	char * pszBranchName = NULL;
	SG_safeptr* psp = NULL;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(argc==2);

	// branchName
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszBranchName)  );

	// audit
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[1])  );
	psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[1]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

	SG_ERR_CHECK(  SG_vc_branches__reopen(pCtx, pRepo, pszBranchName, pq)  );

	SG_NULLFREE(pCtx, pszBranchName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszBranchName);
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_json)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    char* psz_hid = NULL;

	SG_byte* buf = NULL;
	SG_uint64 len = 0;
	SG_vhash* pvh = NULL;
	SG_varray* pva = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_hid)  );

	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pRepo, psz_hid, &buf, &len)  );
	if (len > SG_UINT32_MAX)
		SG_ERR_THROW(SG_ERR_LIMIT_EXCEEDED);
	SG_ERR_CHECK(  SG_veither__parse_json__buflen(pCtx, (const char*)buf, (SG_uint32)len, &pvh, &pva)  );
	SG_ASSERT (pvh != NULL || pva != NULL);

	if (pva == NULL)
	{
		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
		SG_VHASH_NULLFREE(pCtx, pvh);
	}
	else
	{
		SG_uint32 count = 0;
		SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
		SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, count, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva, jso)  );
		SG_VARRAY_NULLFREE(pCtx, pva);
	}

    jso = NULL;
	SG_NULLFREE(pCtx, buf);
	SG_NULLFREE(pCtx, psz_hid);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, buf);
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, psz_hid);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_string)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;
	SG_byte * pbuf = NULL;
    char* psz_hid = NULL;
    JSString* pjs = NULL;
    SG_uint64 len = 0;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(  1 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_hid)  );

	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx,pRepo,psz_hid,&pbuf,&len)  );

    pjs = JS_NewStringCopyN(cx, (char*) pbuf, (size_t) len);
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	SG_NULLFREE(pCtx, pbuf);
	SG_NULLFREE(pCtx, psz_hid);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
	SG_NULLFREE(pCtx, psz_hid);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_dagnode)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_uint64 dagnum = 0;
    char* psz_dagnum = NULL;
    char* pszidHidChangeset = NULL;
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = NULL;
    SG_dagnode* pdn = NULL;
    SG_vhash* pvh = NULL;
    JSObject* jso = NULL;

    SG_JS_BOOL_CHECK(argc==2);

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_dagnum)  );
    SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszidHidChangeset)  );

    SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, dagnum, pszidHidChangeset, &pdn)  );

    SG_ERR_CHECK(  SG_dagnode__to_vhash__shared(pCtx, pdn, NULL, &pvh)  );
    SG_DAGNODE_NULLFREE(pCtx, pdn);

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, psz_dagnum);
	SG_NULLFREE(pCtx, pszidHidChangeset);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, psz_dagnum);
	SG_NULLFREE(pCtx, pszidHidChangeset);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_dag_leaves)
{
   SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_rbtree* prb_leaves = NULL;
    SG_uint64 dagnum;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_id = NULL;
    SG_uint32 i;
    JSObject* jso;
    char *psz_dagnum = NULL;

    SG_JS_BOOL_CHECK(argc==1);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_dagnum)  );
    SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

    SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, dagnum, &prb_leaves)  );

    SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_leaves, &b, &psz_id, NULL)  );
    i = 0;
    while (b)
    {
        jsval jv;

        JSVAL_FROM_SZ(jv, psz_id);
        SG_JS_BOOL_CHECK(  JS_SetElement(cx, jso, i++, &jv)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_id, NULL)  );
    }
    SG_rbtree__iterator__free(pCtx, pit);
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);

	SG_NULLFREE(pCtx, psz_dagnum);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
	SG_NULLFREE(pCtx, psz_dagnum);
    return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, compare)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = NULL;

	SG_repo* pRepo = NULL;
	char * szArg = NULL;
	SG_repo* pOtherRepo = NULL;
	SG_bool bIdentical;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szArg)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, szArg, &pOtherRepo)  );

	SG_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pRepo, pOtherRepo, &bIdentical)  );

	if (bIdentical)
    {
		SG_ERR_CHECK(  SG_sync__compare_repo_blobs(pCtx, pRepo, pOtherRepo, &bIdentical)  );
    }

	SG_REPO_NULLFREE(pCtx, pOtherRepo);

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(bIdentical));
	SG_NULLFREE(pCtx, szArg);
	return JS_TRUE;

fail:
	SG_REPO_NULLFREE(pCtx, pOtherRepo);
	SG_NULLFREE(pCtx, szArg);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}


SG_JSGLUE_METHOD_PROTOTYPE(repo, vc_lookup_hids)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = NULL;
	SG_repo* pRepo = NULL;
    JSObject* pjo = NULL;
	char *szArg = NULL;
	SG_rbtree* prbHids = NULL;
	
	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
   
	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szArg)  );

	SG_ERR_CHECK(  SG_repo__find_dagnodes_by_prefix(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, 
					szArg, &prbHids)  );

    SG_JS_NULL_CHECK(  (pjo = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(pjo));
	SG_ERR_CHECK(  sg_jsglue__copy_rbtree_keys_into_jsobject(pCtx, cx, prbHids, pjo)  ); 

    SG_RBTREE_NULLFREE(pCtx, prbHids);
    SG_NULLFREE(pCtx, szArg);
	return JS_TRUE;

fail:
	SG_RBTREE_NULLFREE(pCtx, prbHids);
    SG_NULLFREE(pCtx, szArg);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}


SG_JSGLUE_METHOD_PROTOTYPE(repo, lookup_hid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = NULL;
	SG_repo* pRepo = NULL;
	char* pszArg = NULL;
	char* pszTargetChangeset = NULL;
	jsval tmp=JSVAL_VOID;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszArg)  );

	SG_ERR_CHECK(  SG_repo__hidlookup__dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, 
		pszArg, &pszTargetChangeset)  );

	JSVAL_FROM_SZ(tmp, pszTargetChangeset);
	JS_SET_RVAL(cx, vp, tmp);
	SG_NULLFREE(pCtx, pszTargetChangeset);
	SG_NULLFREE(pCtx, pszArg);
	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszTargetChangeset);
	SG_NULLFREE(pCtx, pszArg);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/* string repo.store_blobs_from_files(string filePath, string filePath, ...) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, store_blobs_from_files)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_repo* pRepo = NULL;

	SG_pathname* pPath = NULL;
	SG_file* pFile = NULL;
	SG_repo_tx_handle* pRepoTx = NULL;

	char* pszPath = NULL;
	char* pszNewHid = NULL;

	SG_vhash* pvhResults = NULL;
	JSObject* pjo = NULL;

	SG_uint32 i;

	SG_NULLARGCHECK(psp);

	for (i = 0; i < argc; i++)
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[i])  );

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pRepo, &pRepoTx)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhResults, argc, NULL, NULL)  );

	for (i = 0; i < argc; i++)
	{
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[i]), &pszPath)  );
		SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath, pszPath)  );
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_EXISTING|SG_FILE_RDONLY, SG_FSOBJ_PERMS__UNUSED, &pFile)  );

		SG_ERR_CHECK(  SG_repo__store_blob_from_file(pCtx, pRepo, pRepoTx, SG_FALSE, pFile, &pszNewHid, NULL)  );

		SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
		SG_PATHNAME_NULLFREE(pCtx, pPath);

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResults, pszNewHid, pszPath)  );

		SG_NULLFREE(pCtx, pszNewHid);
		SG_NULLFREE(pCtx, pszPath);
	}

	SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &pRepoTx)  );

    SG_JS_NULL_CHECK(  (pjo = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(pjo));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResults, pjo)  );
    SG_VHASH_NULLFREE(pCtx, pvhResults);

	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_NULLFREE(pCtx, pszNewHid);
	SG_VHASH_NULLFREE(pCtx, pvhResults);
	SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
	if (pRepo && pRepoTx)
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pRepo, &pRepoTx)  );

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/* string repo.new_since(string startChangesetHid, string endChangesetHid) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, new_since)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_repo* pRepo = NULL;
	char *szStartChangesetHid = NULL;
	char *szEndChangesetHid = NULL;
	SG_rbtree* prbNewNodes = NULL;
	JSObject* pjo = NULL;

	SG_NULLARGCHECK(psp);
	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szStartChangesetHid)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &szEndChangesetHid)  );

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_dagquery__new_since(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, 
		szStartChangesetHid, szEndChangesetHid, &prbNewNodes)  );

	SG_JS_NULL_CHECK(  (pjo = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(pjo));
	SG_ERR_CHECK(  sg_jsglue__copy_rbtree_keys_into_jsobject(pCtx, cx, prbNewNodes, pjo)  );

	SG_RBTREE_NULLFREE(pCtx, prbNewNodes);
	SG_NULLFREE(pCtx, szStartChangesetHid);
	SG_NULLFREE(pCtx, szEndChangesetHid);
	return JS_TRUE;

fail:
	SG_RBTREE_NULLFREE(pCtx, prbNewNodes);
	SG_NULLFREE(pCtx, szStartChangesetHid);
	SG_NULLFREE(pCtx, szEndChangesetHid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}


/* object repo.get_changeset_description(string changesetHid, bool includeChildren=false) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, get_changeset_description)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_repo* pRepo = NULL;
	char *szChangesetHid = NULL;
	SG_vhash* pvhCS = NULL;
	JSObject* pjoReturn = NULL;
	SG_bool bChildren = SG_FALSE;

	SG_NULLARGCHECK(psp);
	SG_JS_BOOL_CHECK(  1 == argc  ||  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  argc<2 || JSVAL_IS_BOOLEAN(argv[1])  );

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szChangesetHid)  );
	bChildren = argc>1 && argv[1]==JSVAL_TRUE;

	SG_ERR_CHECK(  SG_history__get_changeset_description(pCtx, pRepo, szChangesetHid, bChildren, &pvhCS)  );

    SG_JS_NULL_CHECK(  (pjoReturn = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(pjoReturn));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhCS, pjoReturn)  );
    SG_VHASH_NULLFREE(pCtx, pvhCS);
	SG_NULLFREE(pCtx, szChangesetHid);
	return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhCS);
	SG_NULLFREE(pCtx, szChangesetHid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/* object repo.get_vc_tags( <csid optional>) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_vc_tags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	JSObject* jso = NULL;   
    SG_repo* pRepo = NULL;   
    SG_varray* pva_tags = NULL;
    char *sz_csid = NULL;

    if (argc > 0)
    {
	    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    }

	SG_NULLARGCHECK(psp);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );		

    if (argc > 0)
    {
    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &sz_csid)  );
        SG_vc_tags__lookup(pCtx, pRepo, sz_csid, &pva_tags);
    }
    else
	    SG_ERR_CHECK(  SG_vc_tags__list_all(pCtx, pRepo, &pva_tags)  );   

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_tags, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pva_tags);
    SG_NULLFREE(pCtx, sz_csid);
	return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_tags);
    SG_NULLFREE(pCtx, sz_csid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/* object repo.add_vc_tag(csid, tag, audit) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, add_vc_tag)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_audit * pq;
    SG_safeptr* psp = NULL;
    char *psz_csid = NULL;
    char *psz_text = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==3);

    // csid
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_csid)  );

    // text
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_text)  );

    // audit
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
    psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
    SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

    SG_ERR_CHECK(  SG_vc_tags__add(pCtx, pRepo, psz_csid, psz_text, pq, SG_FALSE)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_FALSE;
}

/* object repo.delete_vc_tag(tag, audit) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, delete_vc_tag)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_audit * pq;
    SG_safeptr* psp = NULL;
    char *psz_text = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==3);

    // text
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_text)  );

    // audit
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
    psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
    SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

    SG_ERR_CHECK( SG_vc_tags__remove(pCtx, pRepo, pq, 1, (const char**)&psz_text) );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    SG_NULLFREE(pCtx, psz_text);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    SG_NULLFREE(pCtx, psz_text);
    return JS_FALSE;
}

/* object repo.lookup_vc_tag(tag) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, lookup_vc_tag)
{
    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    JSString* pjs = NULL;
    char* psz_csid = NULL;
    char* psz_text = NULL;

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==1);

    // text
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_text)  );

    SG_ERR_CHECK(  SG_vc_tags__lookup__tag(pCtx, pRepo, psz_text, &psz_csid)  );
  
    pjs = JS_NewStringCopyN(cx, psz_csid, (size_t) SG_STRLEN(psz_csid));
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    SG_NULLFREE(pCtx, psz_csid);
	SG_NULLFREE(pCtx, psz_text);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);    // DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_text);
    /* TODO */
    return JS_FALSE;
}

/* object repo.add_vc_stamp(csid, stamp, audit) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, add_vc_stamp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_audit * pq;
    SG_safeptr* psp = NULL;
    char *psz_csid = NULL;
    char *psz_text = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==3);

    // csid
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_csid)  );

    // text
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_text)  );

    // audit
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
    psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
    SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

    SG_ERR_CHECK(  SG_vc_stamps__add(pCtx, pRepo, psz_csid, psz_text, pq, NULL)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_FALSE;
}

/* object repo.delete_vc_stamp(csid, stamp, audit) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, delete_vc_stamp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_repo* pRepo = NULL;
    SG_audit * pq;
    SG_safeptr* psp = NULL;
    char *psz_csid = NULL;
    char *psz_text = NULL;

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_JS_BOOL_CHECK(argc==3);

    // csid
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_csid)  );

    // text
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_text)  );

    // audit
    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
    psp = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[2]));
    SG_ERR_CHECK(  SG_safeptr__unwrap__audit(pCtx, psp, &pq)  );

    SG_ERR_CHECK( SG_vc_stamps__remove(pCtx, pRepo, pq, psz_csid, 1, (const char* const*) &psz_text) );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_TRUE;

fail:
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    /* TODO */
    SG_NULLFREE(pCtx, psz_csid);
    SG_NULLFREE(pCtx, psz_text);
    return JS_FALSE;
}

/* object repo.get_vc_stamps(<csid optional>) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, fetch_vc_stamps)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	JSObject* jso = NULL;   
    SG_repo* pRepo = NULL;   
    char *sz_csid = NULL;
    SG_varray* pva_stamps = NULL;

   
    if (argc > 0)
    {
	    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    }

	SG_NULLARGCHECK(psp);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );		

    if (argc > 0)
    {
    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &sz_csid)  );
        SG_ERR_CHECK(  SG_vc_stamps__lookup(pCtx, pRepo, sz_csid, &pva_stamps)  );	  
    }
    else
        SG_ERR_CHECK(  SG_vc_stamps__list_all_stamps(pCtx, pRepo, &pva_stamps)  );  

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_stamps, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pva_stamps);
    SG_NULLFREE(pCtx, sz_csid);
	return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_stamps);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_NULLFREE(pCtx, sz_csid);
	return JS_FALSE;
}

/* object repo.get_treenode_entry_by_gid(changeset_id, gid) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, get_treenode_info_by_gid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    JSObject* jso = NULL;   
    SG_repo* pRepo = NULL;   

    SG_vhash* pvhResults = NULL;
    SG_treenode_entry* tne = NULL;
    SG_treenode* ptn = NULL;
    char *pChangesetHid = NULL;
    char *pFolderGid = NULL;
    SG_string* pstr_path = NULL;  
    SG_changeset* pChangeset = NULL;

	SG_NULLARGCHECK(psp);

    SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );	

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pChangesetHid)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pFolderGid)  );
	
	SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, pChangesetHid, &pChangeset)  );

	SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pFolderGid, pChangesetHid, &pstr_path, &tne)  );
	
    if (tne)
    {
        SG_ERR_CHECK(  sg_jsglue__treenode_entry_hash(pCtx, pRepo, pChangesetHid, tne, SG_string__sz(pstr_path), pFolderGid, &pvhResults)  );
	    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResults, jso)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvhResults);
    SG_STRING_NULLFREE(pCtx, pstr_path);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, tne);
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
	SG_TREENODE_NULLFREE(pCtx, ptn);
	SG_NULLFREE(pCtx, pChangesetHid);
	SG_NULLFREE(pCtx, pFolderGid);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhResults);
    SG_STRING_NULLFREE(pCtx, pstr_path);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, tne);
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
	SG_TREENODE_NULLFREE(pCtx, ptn);
	SG_NULLFREE(pCtx, pChangesetHid);
	SG_NULLFREE(pCtx, pFolderGid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/* object repo.get_treenode_entry_by_path(changeset_id, path) */
SG_JSGLUE_METHOD_PROTOTYPE(repo, get_treenode_info_by_path)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    JSObject* jso = NULL;   
    SG_repo* pRepo = NULL;   

    SG_vhash* pvhResults = NULL;
    SG_treenode_entry* tne = NULL;
	char* pszHidRoot = NULL;
    char *pszPath = NULL;
    char *pszChangesetID = NULL;
	SG_treenode* ptn = NULL;
	char* pszGid = NULL;		

	SG_NULLARGCHECK(psp);

    SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );	

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszChangesetID)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszPath)  );

    SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszChangesetID, &pszHidRoot)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidRoot, &ptn)  );	
	
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, ptn, pszPath, &pszGid, &tne)  );

    if (tne)
    {
        SG_ERR_CHECK(  sg_jsglue__treenode_entry_hash(pCtx, pRepo, pszChangesetID, tne, pszPath, pszGid, &pvhResults)  );
	    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResults, jso)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvhResults);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, tne);
	SG_NULLFREE(pCtx, pszHidRoot);
	SG_TREENODE_NULLFREE(pCtx, ptn);
	SG_NULLFREE(pCtx, pszGid);
	SG_NULLFREE(pCtx, pszChangesetID);
	SG_NULLFREE(pCtx, pszPath);
	return JS_TRUE;

fail:
    SG_TREENODE_ENTRY_NULLFREE(pCtx, tne);
	SG_NULLFREE(pCtx, pszHidRoot);
    SG_VHASH_NULLFREE(pCtx, pvhResults);
	SG_TREENODE_NULLFREE(pCtx, ptn);
	SG_NULLFREE(pCtx, pszGid);	
	SG_NULLFREE(pCtx, pszChangesetID);
	SG_NULLFREE(pCtx, pszPath);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, sync_all_users_in_closet)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = NULL;
	JSObject* jso = NULL;

	SG_repo* pRepo = NULL;
	char* pszHidLeaf = NULL;
 	SG_varray* pvaUsers = NULL;

	psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  argc <= 1 );

	if (argc == 1)
	{
		if (!JSVAL_IS_NULL(argv[0]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0]) );
			SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszHidLeaf)  );
		}
	}

	SG_ERR_CHECK(  SG_sync__closet_user_dags(pCtx, pRepo, pszHidLeaf, &pvaUsers)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaUsers, jso)  );

	SG_VARRAY_NULLFREE(pCtx, pvaUsers);
	SG_NULLFREE(pCtx, pszHidLeaf);
	return JS_TRUE;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaUsers);
	SG_NULLFREE(pCtx, pszHidLeaf);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, admin_id)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    SG_repo* pRepo = NULL;
    char* psz_id = NULL;

	SG_UNUSED(obj);
    SG_UNUSED(id);

    psp = sg_jsglue__get_object_private(cx, obj);
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_id)  );

    JSVAL_FROM_SZ(*vp, psz_id);

    SG_NULLFREE(pCtx, psz_id);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, name)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    SG_repo* pRepo = NULL;
    const char* psz_name = NULL;

	SG_UNUSED(obj);
    SG_UNUSED(id);

    psp = sg_jsglue__get_object_private(cx, obj);
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &psz_name)  );

    JSVAL_FROM_SZ(*vp, psz_name);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, repo_id)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    SG_repo* pRepo = NULL;
    char* psz_id = NULL;

	SG_UNUSED(obj);
    SG_UNUSED(id);

    psp = sg_jsglue__get_object_private(cx, obj);
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &psz_id)  );

    JSVAL_FROM_SZ(*vp, psz_id);

    SG_NULLFREE(pCtx, psz_id);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(blobset, count)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    SG_blobset* pRepo = NULL;
    SG_uint32 count = 0;

	SG_UNUSED(obj);
    SG_UNUSED(id);

    psp = sg_jsglue__get_object_private(cx, obj);
    SG_ERR_CHECK(  SG_safeptr__unwrap__blobset(pCtx, psp, &pRepo)  );

    SG_ERR_CHECK(  SG_blobset__count(pCtx, pRepo, &count)  );

    *vp = INT_TO_JSVAL(count);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, hash_method)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    SG_repo* pRepo = NULL;
    char* psz_method = NULL;

	SG_UNUSED(obj);
    SG_UNUSED(id);

    psp = sg_jsglue__get_object_private(cx, obj);
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pRepo, &psz_method)  );

    JSVAL_FROM_SZ(*vp, psz_method);

    SG_NULLFREE(pCtx, psz_method);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, supports_live_with_working_copy)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	SG_bool bAnswer = SG_FALSE;

	SG_UNUSED(obj);
	SG_UNUSED(id);

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__query_implementation(pCtx, pRepo, SG_REPO__QUESTION__BOOL__SUPPORTS_LIVE_WITH_WORKING_DIRECTORY, &bAnswer, NULL, NULL, 0, NULL)  );
	*vp = BOOLEAN_TO_JSVAL(bAnswer);

	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
	return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, supports_zlib)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	SG_bool bAnswer = SG_FALSE;

	SG_UNUSED(obj);
	SG_UNUSED(id);

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__query_implementation(pCtx, pRepo, SG_REPO__QUESTION__BOOL__SUPPORTS_ZLIB, &bAnswer, NULL, NULL, 0, NULL)  );
	*vp = BOOLEAN_TO_JSVAL(bAnswer);

	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
	return JS_FALSE;
}

SG_JSGLUE_PROPERTY_PROTOTYPE(repo, supports_vcdiff)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	SG_bool bAnswer = SG_FALSE;

	SG_UNUSED(obj);
	SG_UNUSED(id);

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__query_implementation(pCtx, pRepo, SG_REPO__QUESTION__BOOL__SUPPORTS_VCDIFF, &bAnswer, NULL, NULL, 0, NULL)  );
	*vp = BOOLEAN_TO_JSVAL(bAnswer);

	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
	return JS_FALSE;
}
SG_JSGLUE_PROPERTY_PROTOTYPE(repo, supports_dbndx)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	SG_bool bAnswer = SG_FALSE;

	SG_UNUSED(obj);
	SG_UNUSED(id);

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__query_implementation(pCtx, pRepo, SG_REPO__QUESTION__BOOL__SUPPORTS_DBNDX, &bAnswer, NULL, NULL, 0, NULL)  );
	*vp = BOOLEAN_TO_JSVAL(bAnswer);

	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
	return JS_FALSE;
}
SG_JSGLUE_PROPERTY_PROTOTYPE(repo, supports_treendx)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_repo* pRepo = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	SG_bool bAnswer = SG_FALSE;

	SG_UNUSED(obj);
	SG_UNUSED(id);

	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__query_implementation(pCtx, pRepo, SG_REPO__QUESTION__BOOL__SUPPORTS_TREEDX, &bAnswer, NULL, NULL, 0, NULL)  );
	*vp = BOOLEAN_TO_JSVAL(bAnswer);

	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to free anything?
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, create_nobody)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_repo* pRepo = NULL;

	SG_NULLARGCHECK(psp);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  (0 == argc)  );

    SG_ERR_CHECK(  SG_user__create_nobody(pCtx, pRepo)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_JSGLUE_METHOD_PROTOTYPE(repo, lookup_audits)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	JSObject* jso = NULL;
    char *psz_csid = NULL;
    SG_repo* pRepo = NULL;
    SG_uint64 iDagNum = 0;
    SG_varray* pva_audits = NULL;
    char *psz_dagnum = NULL;

	SG_NULLARGCHECK(psp);

    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );

	SG_JS_BOOL_CHECK(  (2 == argc)  );

    // dagnum
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_dagnum)  );
    SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &iDagNum)  );

    // csid
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_csid)  );

    SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, iDagNum, psz_csid, &pva_audits)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_audits, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pva_audits);
    SG_NULLFREE(pCtx, psz_dagnum);
    SG_NULLFREE(pCtx, psz_csid);
	return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_audits);
    SG_NULLFREE(pCtx, psz_dagnum);
    SG_NULLFREE(pCtx, psz_csid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

//sg.repo.file_diff(hid1, hid2, filename)
SG_JSGLUE_METHOD_PROTOTYPE(repo, diff_file)
{
	// TODO 2012/05/04 REPLACE THIS WHOLE FUNCTION WITH A NEW FUNCTION IN sg.vv2

    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));	 
    SG_repo* pRepo = NULL;
    JSString* pjs = NULL;

	char *szHid1 = NULL;
	char *szHid2 = NULL;
	char *szFilename = NULL;

	SG_tempfile* pTempFileA = NULL;
	SG_tempfile* pTempFileB = NULL;

	SG_string* pDiffText = NULL;
	SG_string* pStringLabel_a = NULL;
	SG_string* pStringLabel_b = NULL;

    
	//TODO get this from a config file of some sort. For now just hardcode it	
    SG_difftool__built_in_tool * pfnCompare = SG_difftool__built_in_tool__textfilediff_ignore_eols;	
	SG_int32 result_code = SG_FILETOOL__RESULT__SUCCESS;
    
    SG_NULLARGCHECK(psp);
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );	
   
	SG_JS_BOOL_CHECK(  3 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szHid1)  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &szHid2)  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &szFilename)  );
	
    SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFileA)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx,pRepo,szHid1,pTempFileA->pFile,NULL)  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFileA)  );
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx,szFilename,szHid1,NULL,&pStringLabel_a)  );
    
    SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFileB)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx,pRepo,szHid2,pTempFileB->pFile,NULL)  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFileB)  );
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx,szFilename,szHid2,NULL,&pStringLabel_b)  );
	

	(pfnCompare)(pCtx, pTempFileA->pPathname, pTempFileB->pPathname,
				 pStringLabel_a,pStringLabel_b,
				 SG_FALSE,
				 &pDiffText, &result_code);
	if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
	{
		SG_context__err_reset(pCtx);
		SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pDiffText, "--- %s\n+++ %s\n\n",
			SG_string__sz(pStringLabel_a),
			SG_string__sz(pStringLabel_b))  );
		if(SG_stricmp(szHid1, szHid2)==0)
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pDiffText, "(Binary files are identical.)")  );
		}
		else
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pDiffText, "(Binary files differ.)")  );
		}
		SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pDiffText)))  );   
		JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	}
	else
	{
		SG_ERR_CHECK_CURRENT;
		if (pDiffText)
		{
			SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pDiffText)))  );    
			JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
		}
		else
		{
			JS_SET_RVAL(cx, vp, JSVAL_VOID);
		}
	}
	
    SG_STRING_NULLFREE(pCtx, pStringLabel_a);
	SG_STRING_NULLFREE(pCtx, pStringLabel_b);
	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFileA)  );
	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFileB)  );
	SG_STRING_NULLFREE(pCtx, pDiffText);
	SG_NULLFREE(pCtx, szHid1);
	SG_NULLFREE(pCtx, szHid2);
	SG_NULLFREE(pCtx, szFilename);
    return JS_TRUE;

fail:
	SG_STRING_NULLFREE(pCtx, pStringLabel_a);
	SG_STRING_NULLFREE(pCtx, pStringLabel_b);
	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFileA)  );
	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFileB)  );
	SG_STRING_NULLFREE(pCtx, pDiffText);
	SG_NULLFREE(pCtx, szHid1);
	SG_NULLFREE(pCtx, szHid2);
	SG_NULLFREE(pCtx, szFilename);
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

void _sg_jsglue_get_difftool(SG_context *pCtx, SG_difftool__built_in_tool **ppfnCompare)
{
	// TODO 2012/05/04 Delete this functionwhen sg.repo.{diff_file,diff_file_local}() are deleted.

	SG_NULLARGCHECK_RETURN(ppfnCompare);

	//TODO get this from a config file of some sort. For now just hardcode it	
	*ppfnCompare = SG_difftool__built_in_tool__textfilediff_ignore_eols;
}

//diff a file at a certain version with the working copy file
//sg.repo.diff_file_local(hid1, repopath)
SG_JSGLUE_METHOD_PROTOTYPE(repo, diff_file_local)
{
	// TODO 2012/05/04 REPLACE THIS WHOLE FUNCTION WITH A NEW FUNCTION IN sg.wc

    SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));	 
    SG_repo* pRepo = NULL;
    JSString* pjs = NULL;

	char *szHid1 = NULL;
	char *szRepopath = NULL;

	SG_tempfile* pTempFileA = NULL;

	SG_string* pDiffText = NULL;  
    SG_string* pstrRepo = NULL;
    char* pszgid = NULL;
	SG_string* pStringLabel_a = NULL;
	SG_string* pStringLabel_b = NULL;
    SG_pathname* pPathRepoTop = NULL;
    SG_pathname* pPathLocal = NULL;
    SG_pathname* pPathCwd = NULL;	
    
    SG_difftool__built_in_tool * pfnCompare = NULL;
	SG_int32 result_code = SG_FILETOOL__RESULT__SUCCESS;

	SG_ERR_CHECK(  _sg_jsglue_get_difftool(pCtx, &pfnCompare)  );

    SG_NULLARGCHECK(psp);
    SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, psp, &pRepo)  );	

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &szHid1)  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &szRepopath)  );
    
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathCwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathCwd)  );
  	
    SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPathCwd, &pPathRepoTop, &pstrRepo, &pszgid));
	SG_ERR_CHECK(  SG_workingdir__construct_absolute_path_from_repo_path(pCtx, pPathRepoTop, szRepopath, &pPathLocal) );
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_PATHNAME_NULLFREE(pCtx, pPathRepoTop);
    SG_NULLFREE(pCtx, pszgid);
    SG_STRING_NULLFREE(pCtx, pstrRepo);
	
	//fetch the blob into a temp file
	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFileA)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx,pRepo,szHid1,pTempFileA->pFile,NULL)  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFileA)  );
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx,szRepopath,szHid1,NULL,&pStringLabel_a)  );
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, SG_pathname__sz(pPathLocal), NULL, NULL, &pStringLabel_b)  );

	SG_ERR_CHECK(  (pfnCompare)(pCtx, pTempFileA->pPathname, pPathLocal,
										   pStringLabel_a,pStringLabel_b,
										   SG_FALSE,
								&pDiffText, &result_code)  );
    if (pDiffText)
    {
        SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pDiffText)))  );   

        JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
    }
    else
    {
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
    }
	
    SG_STRING_NULLFREE(pCtx, pStringLabel_a);
	SG_STRING_NULLFREE(pCtx, pStringLabel_b);
	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFileA)  );
	SG_STRING_NULLFREE(pCtx, pDiffText); 
	SG_PATHNAME_NULLFREE(pCtx, pPathLocal);
	SG_NULLFREE(pCtx, szHid1);
	SG_NULLFREE(pCtx, szRepopath);
    return JS_TRUE;

fail:
    SG_STRING_NULLFREE(pCtx, pStringLabel_a);
	SG_STRING_NULLFREE(pCtx, pStringLabel_b);
    SG_STRING_NULLFREE(pCtx, pstrRepo);
	SG_PATHNAME_NULLFREE(pCtx, pPathLocal);
	SG_STRING_NULLFREE(pCtx, pDiffText);
    SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
    SG_PATHNAME_NULLFREE(pCtx, pPathRepoTop);
    SG_NULLFREE(pCtx, pszgid);
	SG_NULLFREE(pCtx, szHid1);
	SG_NULLFREE(pCtx, szRepopath);

	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFileA)  );	

    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

//diff a file at a certain version with the working copy file
//sg.repo.diff_file_local(hid1, repopath)
SG_JSGLUE_METHOD_PROTOTYPE(sg, diff_files)
{
 	// TODO 2012/05/04 REPLACE THIS WHOLE FUNCTION WITH A NEW FUNCTION IN sg.wc

	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    JSString* pjs = NULL;

	SG_string* pDiffText = NULL;  
	SG_string* pStringLabel_a = NULL;
	SG_string* pStringLabel_b = NULL;
    SG_pathname* pPath1 = NULL;
	SG_pathname *pPath2 = NULL;
	char *path1=NULL;
	char *path2=NULL;
	char *label1=NULL;
	char *label2=NULL;
	char *fn=NULL;
    
    SG_difftool__built_in_tool * pfnCompare = NULL;
	SG_int32 result_code = SG_FILETOOL__RESULT__SUCCESS;

	SG_ERR_CHECK(  _sg_jsglue_get_difftool(pCtx, &pfnCompare)  );

	SG_JS_BOOL_CHECK(  5 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[4])  );
	
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &path1)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &label1)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &path2)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &label2)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[4]), &fn)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath1, path1)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath2, path2)  );
    
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, fn, label1,NULL,&pStringLabel_a)  );
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, fn, label2,NULL,&pStringLabel_b)  );

	SG_ERR_CHECK(  (pfnCompare)(pCtx, pPath1, pPath2,
										   pStringLabel_a,pStringLabel_b,
										   SG_FALSE,
										   &pDiffText, &result_code)  );
    if (pDiffText)
    {
        SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(pDiffText)))  );   

        JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
    }
    else
    {
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
    }
	
    SG_STRING_NULLFREE(pCtx, pStringLabel_a);
	SG_STRING_NULLFREE(pCtx, pStringLabel_b);
	SG_STRING_NULLFREE(pCtx, pDiffText); 
	SG_PATHNAME_NULLFREE(pCtx, pPath1);
	SG_PATHNAME_NULLFREE(pCtx, pPath2);
	SG_NULLFREE(pCtx, path1);
	SG_NULLFREE(pCtx, label1);
	SG_NULLFREE(pCtx, path2);
	SG_NULLFREE(pCtx, label2);
	SG_NULLFREE(pCtx, fn);
    return JS_TRUE;

fail:
    SG_STRING_NULLFREE(pCtx, pStringLabel_a);
	SG_STRING_NULLFREE(pCtx, pStringLabel_b);
	SG_STRING_NULLFREE(pCtx, pDiffText);
	SG_PATHNAME_NULLFREE(pCtx, pPath1);
	SG_PATHNAME_NULLFREE(pCtx, pPath2);
	SG_NULLFREE(pCtx, path1);
	SG_NULLFREE(pCtx, label1);
	SG_NULLFREE(pCtx, path2);
	SG_NULLFREE(pCtx, label2);
	SG_NULLFREE(pCtx, fn);
    SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

/* ---------------------------------------------------------------- */
/* xmlwriter class */
/* ---------------------------------------------------------------- */

static void xmlwriter_finalize(JSContext *cx, JSObject *obj);
static void xmlwriter__free(SG_context *pCtx, sg_xmlwriter *pWriter);

static JSClass xmlwriter_class = {
    "xmlwriter",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, xmlwriter_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};


struct sg_xmlwriter
{
	SG_xmlwriter *writer;
	SG_string *dest;
};


static void xmlwriter__alloc(
	SG_context *pCtx,
	sg_xmlwriter **ppWriter)
{
	sg_xmlwriter *pWriter = NULL;

	SG_NULLARGCHECK(ppWriter);

	*ppWriter = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pWriter)  );

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &(pWriter->dest))  );
	SG_ERR_CHECK(  SG_xmlwriter__alloc(pCtx, &(pWriter->writer), pWriter->dest, SG_TRUE)  );

	*ppWriter = pWriter;
	pWriter = NULL;

fail:
	if (pWriter)
	{
		xmlwriter__free(pCtx, pWriter);
		pWriter = NULL;
	}
}


static void xmlwriter__free(
	SG_context *pCtx,
	sg_xmlwriter *pWriter)
{
	if (pWriter)
	{
		if (pWriter->writer)
			SG_XMLWRITER_NULLFREE(pCtx, pWriter->writer);
		if (pWriter->dest)
			SG_STRING_NULLFREE(pCtx, pWriter->dest);

		SG_NULLFREE(pCtx, pWriter);
	}
}


JSBool xmlwriter_constructor(JSContext *cx, uintN argc, jsval *vp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	JSObject* jso = NULL;
	SG_safeptr* psp = NULL;
	sg_xmlwriter *writer = NULL;

	SG_JS_BOOL_CHECK(  0 == argc  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &xmlwriter_class, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));

	SG_ERR_CHECK(  xmlwriter__alloc(pCtx, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_start_document(pCtx, writer->writer)  );

	SG_ERR_CHECK(  SG_safeptr__wrap__xmlwriter(pCtx, writer, &psp)  );
    JS_SetPrivate(cx, jso, psp);

	return JS_TRUE;

fail:
    xmlwriter__free(pCtx, writer);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

static void xmlwriter_finalize(JSContext *cx, JSObject *obj)
{
	SG_context * pCtx = NULL;
	SG_safeptr* psp = sg_jsglue__get_object_private(cx, obj);
	sg_xmlwriter* writer = NULL;

	SG_context__alloc__temporary(&pCtx);
	//err = SG_context__alloc__temporary(&pCtx);
    // TODO check err

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );
	xmlwriter__free(pCtx, writer);
	SG_SAFEPTR_NULLFREE(pCtx, psp);
	SG_CONTEXT_NULLFREE(pCtx);
	return;

fail:
	xmlwriter__free(pCtx, writer);
	SG_SAFEPTR_NULLFREE(pCtx, psp);
	SG_CONTEXT_NULLFREE(pCtx);

}



/* .start_element(name) */
SG_JSGLUE_METHOD_PROTOTYPE(xmlwriter, start_element)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_safeptr* psp = NULL;
	char *tagname = NULL;
	sg_xmlwriter *writer = NULL;

    SG_JS_BOOL_CHECK(  argc == 1 );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); 
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &tagname)  );

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_start_element__sz(pCtx, writer->writer, tagname)  );

	SG_NULLFREE(pCtx, tagname);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, tagname);
    return JS_FALSE;
}


/* .attribute(name, value) */
SG_JSGLUE_METHOD_PROTOTYPE(xmlwriter, attribute)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_safeptr* psp = NULL;
	char *attname = NULL;
	char *attval = NULL;
	sg_xmlwriter *writer = NULL;

    SG_JS_BOOL_CHECK(  argc == 2 );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); 
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  ); 

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &attname)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &attval)  );

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_attribute__sz(pCtx, writer->writer, attname, attval)  );

	SG_NULLFREE(pCtx, attname);
	SG_NULLFREE(pCtx, attval);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, attname);
	SG_NULLFREE(pCtx, attval);
    return JS_FALSE;
}


/* .content(text) */
SG_JSGLUE_METHOD_PROTOTYPE(xmlwriter, content)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_safeptr* psp = NULL;
	char *text = NULL;
	sg_xmlwriter *writer = NULL;

    SG_JS_BOOL_CHECK(  argc == 1 );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); 
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &text)  );

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_content__sz(pCtx, writer->writer, text)  );

	SG_NULLFREE(pCtx, text);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, text);
    return JS_FALSE;
}


/* .end_element() */
SG_JSGLUE_METHOD_PROTOTYPE(xmlwriter, end_element)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
	sg_xmlwriter *writer = NULL;

    SG_JS_BOOL_CHECK(  argc == 0 );

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_end_element(pCtx, writer->writer)  );

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}


/* .element(name, content) */
SG_JSGLUE_METHOD_PROTOTYPE(xmlwriter, element)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_safeptr* psp = NULL;
	char *tagname = NULL;
	char *text = NULL;
	sg_xmlwriter *writer = NULL;
	
    SG_JS_BOOL_CHECK(  argc == 2 );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); 
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  ); 

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &tagname)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &text)  );

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_element__sz(pCtx, writer->writer, tagname, text)  );

	SG_NULLFREE(pCtx, tagname);
	SG_NULLFREE(pCtx, text);
    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, tagname);
	SG_NULLFREE(pCtx, text);
    return JS_FALSE;
}

/* .finish() */
/* end doc, return text */
SG_JSGLUE_METHOD_PROTOTYPE(xmlwriter, finish)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
	sg_xmlwriter *writer = NULL;
    JSString* pjs = NULL;

    SG_JS_BOOL_CHECK(  argc == 0 );

    psp = sg_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__xmlwriter(pCtx, psp, &writer)  );

	SG_ERR_CHECK(  SG_xmlwriter__write_end_document(pCtx, writer->writer)  );

    SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_string__sz(writer->dest)))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

// todo: how/when do we free?



/* ---------------------------------------------------------------- */
/* Now the method tables */
/* ---------------------------------------------------------------- */

static JSPropertySpec sg_audit_properties[] =
{
    //{"who", ...},
    //{"when", ...},
    {NULL,0,0,NULL,NULL}
};
static JSFunctionSpec sg_audit_methods[] =
{
    {"fin", SG_JSGLUE_METHOD_NAME(audit,fin), 0,0},
    {NULL,NULL,0,0}
};

static JSPropertySpec sg_cbuffer_properties[] =
{
    {"length", 0, JSPROP_READONLY, SG_JSGLUE_PROPERTY_NAME(cbuffer, length), NULL},
    {NULL,0,0,NULL,NULL}
};
static JSFunctionSpec sg_cbuffer_methods[] =
{
    {"fin", SG_JSGLUE_METHOD_NAME(cbuffer,fin), 0,0},
    {NULL,NULL,0,0}
};

static JSPropertySpec sg_blobset_properties[] =
{
    {"count",           0,      JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(blobset, count), NULL},
    {NULL,0,0,NULL,NULL}
};

static JSFunctionSpec sg_blobset_methods[] = 
{
    {"cleanup", 			  	SG_JSGLUE_METHOD_NAME(blobset, cleanup),0,0},
    {"get_stats", 			  	SG_JSGLUE_METHOD_NAME(blobset, get_stats),1,0},
    {"lookup", 			  	SG_JSGLUE_METHOD_NAME(blobset, lookup),1,0},
    {"get", 			  	SG_JSGLUE_METHOD_NAME(blobset, get),1,0},
	JS_FS_END
};

/*
 * These properties & methods are available on a repo class.  To create a repo
 * use the javascript "var repo = sg.open_repo("repo name")
 */
static JSPropertySpec sg_repo_properties[] = {
    {"hash_method",           0,      JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, hash_method), NULL},
    {"repo_id",           0,      JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, repo_id), NULL},
    {"admin_id",          0,     JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, admin_id), NULL},
	{"name",	0, JSPROP_READONLY, SG_JSGLUE_PROPERTY_NAME(repo, name), NULL},
    {"supports_live_with_working_copy",          0,     JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, supports_live_with_working_copy), NULL},
    {"supports_zlib",          0,     JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, supports_zlib), NULL},
    {"supports_vcdiff",          0,     JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, supports_vcdiff), NULL},
    {"supports_dbndx",          0,     JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, supports_dbndx), NULL},
    {"supports_treendx",          0,     JSPROP_READONLY,       SG_JSGLUE_PROPERTY_NAME(repo, supports_treendx), NULL},
    {NULL,0,0,NULL,NULL}
};
static JSFunctionSpec sg_repo_methods[] = {
    {"lookup_audits", 			SG_JSGLUE_METHOD_NAME(repo,lookup_audits),0,0},
    {"create_nobody", 			SG_JSGLUE_METHOD_NAME(repo,create_nobody),0,0},
    {"fetch_dag_leaves", SG_JSGLUE_METHOD_NAME(repo,fetch_dag_leaves),1,0},
    {"close", SG_JSGLUE_METHOD_NAME(repo,close),0,0},
    {"find_new_dagnodes_since", SG_JSGLUE_METHOD_NAME(repo,find_new_dagnodes_since),1,0},
    {"install_vc_hook", SG_JSGLUE_METHOD_NAME(repo,install_vc_hook),1,0},
    {"vc_hooks_installed", SG_JSGLUE_METHOD_NAME(repo,vc_hooks_installed),1,0},
    {"get_status_of_version_control_locks", SG_JSGLUE_METHOD_NAME(repo,get_status_of_version_control_locks),1,0},
    {"set_user", SG_JSGLUE_METHOD_NAME(repo,set_user),1,0},
    {"create_user", SG_JSGLUE_METHOD_NAME(repo,create_user),1,0},
    {"create_user_force_recid", SG_JSGLUE_METHOD_NAME(repo,create_user_force_recid),1,0},
    {"create_group", SG_JSGLUE_METHOD_NAME(repo,create_group),1,0},
    {"fetch_group", SG_JSGLUE_METHOD_NAME(repo,fetch_group),1,0},
    {"fetch_vc_tags", SG_JSGLUE_METHOD_NAME(repo,fetch_vc_tags),0,0},
    {"delete_vc_tag", SG_JSGLUE_METHOD_NAME(repo,delete_vc_tag),3,0},
    {"add_vc_tag", SG_JSGLUE_METHOD_NAME(repo,add_vc_tag),3,0},
    {"lookup_vc_tag", SG_JSGLUE_METHOD_NAME(repo, lookup_vc_tag),1,0},
    {"fetch_vc_stamps", SG_JSGLUE_METHOD_NAME(repo,fetch_vc_stamps),0,0},
    {"delete_vc_stamp", SG_JSGLUE_METHOD_NAME(repo,delete_vc_stamp),3,0},
    {"add_vc_stamp", SG_JSGLUE_METHOD_NAME(repo,add_vc_stamp),3,0},
    {"fetch_dagnode", SG_JSGLUE_METHOD_NAME(repo,fetch_dagnode),1,0},
    {"fetch_string", SG_JSGLUE_METHOD_NAME(repo,fetch_string),1,0},
    {"fetch_json", SG_JSGLUE_METHOD_NAME(repo,fetch_json),1,0},
    {"fetch_blob_into_tempfile", SG_JSGLUE_METHOD_NAME(repo,fetch_blob_into_tempfile),1,0},
    {"fetch_blob", SG_JSGLUE_METHOD_NAME(repo,fetch_blob), 2,0},
    {"diff_file", SG_JSGLUE_METHOD_NAME(repo,diff_file), 3,0},
    {"diff_file_local", SG_JSGLUE_METHOD_NAME(repo, diff_file_local), 2,0},
    {"get_treenode_info_by_gid", SG_JSGLUE_METHOD_NAME(repo, get_treenode_info_by_gid), 2,0},
    {"get_treenode_info_by_path", SG_JSGLUE_METHOD_NAME(repo, get_treenode_info_by_path), 2,0},
    {"history", SG_JSGLUE_METHOD_NAME(repo,history), 6, 0},
    {"changeset_history_contains", SG_JSGLUE_METHOD_NAME(repo,changeset_history_contains),2,0},
    {"count_new_since_common", SG_JSGLUE_METHOD_NAME(repo,count_new_since_common),2,0},
    {"merge_preview", SG_JSGLUE_METHOD_NAME(repo,merge_preview),2,0},
    {"merge_review", SG_JSGLUE_METHOD_NAME(repo,merge_review), 2, 0 },
    {"fill_in_history_details", SG_JSGLUE_METHOD_NAME(repo, fill_in_history_details), 1, 0 },
    {"csid_to_revno", SG_JSGLUE_METHOD_NAME(repo,csid_to_revno), 1, 0},
    {"revno_to_csid", SG_JSGLUE_METHOD_NAME(repo,revno_to_csid), 1, 0},

	{"area_add", SG_JSGLUE_METHOD_NAME(repo,area_add), 2, 0},
	{"list_areas", SG_JSGLUE_METHOD_NAME(repo,list_areas), 0, 0},

	{"history_fetch_more", SG_JSGLUE_METHOD_NAME(repo,history_fetch_more), 2, 0},

    {"list_blobs", SG_JSGLUE_METHOD_NAME(repo,list_blobs), 3,0},
    {"low_level_delete_record", SG_JSGLUE_METHOD_NAME(repo,low_level_delete_record), 3,0},
    {"rebuild_indexes", SG_JSGLUE_METHOD_NAME(repo,rebuild_indexes), 0,0},
    {"create_audit", SG_JSGLUE_METHOD_NAME(repo,create_audit), 2,0},
    {"add_vc_comment", SG_JSGLUE_METHOD_NAME(repo,add_vc_comment), 3,0},
    {"fetch_vc_comments", SG_JSGLUE_METHOD_NAME(repo,fetch_vc_comments), 1,0},
    {"named_branches", SG_JSGLUE_METHOD_NAME(repo,named_branches), 0,0},
	{"add_head", SG_JSGLUE_METHOD_NAME(repo,add_head),3,0},
	{"move_head", SG_JSGLUE_METHOD_NAME(repo,move_head),4,0},
	{"remove_head", SG_JSGLUE_METHOD_NAME(repo,remove_head),3,0},
	{"close_branch", SG_JSGLUE_METHOD_NAME(repo,close_branch),2,0},
	{"reopen_branch", SG_JSGLUE_METHOD_NAME(repo,reopen_branch),2,0},
    {"lookup_branch_names", SG_JSGLUE_METHOD_NAME(repo,lookup_branch_names), 1,0},
	{"list_dags", SG_JSGLUE_METHOD_NAME(repo,list_dags), 0, 0},
    {"file_classes", SG_JSGLUE_METHOD_NAME(repo,file_classes), 0, 0},


	/**
	 * boolean repo.compare(string descriptorName)
	 * 
	 * Compares repo to the repository corresponding with descriptorName.
	 * Returns true if all dags and blobs are identical, false if not.
	 */	
	{"compare", SG_JSGLUE_METHOD_NAME(repo,compare),1,0},

	/**
	 * string repo.lookup_hid(string prefixOrRevisionID)
	 * 
	 * Returns the full VC changeset HID if the provided prefix 
	 * or revision ID could be found, otherwise throws.
	 */	
	{"lookup_hid", SG_JSGLUE_METHOD_NAME(repo,lookup_hid),1,0},

    /**
	 * object repo.lookup_hid(string prefixOrRevisionID)
	 * 
	 * Returns the list VC changeset HIDs if the provided prefix is found
	 * or if revision IDs has matches, otherwise throws.
	 */	
	{"vc_lookup_hids", SG_JSGLUE_METHOD_NAME(repo,vc_lookup_hids),1,0},

	/**
	 * string repo.store_blobs_from_files(string filePath1, string filePath2, ...)
	 * 
	 * Stores the contents of the listed files as repo blobs.
	 * Returns an object with the files' HIDs as properties 
	 * and the correcsponding file path as their value.
	 */	
	{"store_blobs_from_files", SG_JSGLUE_METHOD_NAME(repo,store_blobs_from_files),1,0},

	/**
	 * string repo.new_since(string startChangesetHid, string endChangesetHid)
	 * 
	 * Returns an array of changeset hids representing nodes that are new between
	 * startChangesetHid and endChangesetHid. In other words, returns the nodes
	 * that are ancestors of endChangesetHid that are NOT ancestors of 
	 * startChangesetHid.
	 */	
	{"new_since", SG_JSGLUE_METHOD_NAME(repo, new_since), 2,0},

	/**
	 * object repo.get_changeset_description(string changesetHid)
	 */	
	{"get_changeset_description", SG_JSGLUE_METHOD_NAME(repo, get_changeset_description), 1,0},

	//////////////////////////////////////////////////////////////////
	// repo.diff_changesets() has been replaced by sg.vv2.{status,mstatus}
	//////////////////////////////////////////////////////////////////

    /**
	 * object repo.sync_all_users_in_closet([string userDagLeafHid])
	 */	
	{"sync_all_users_in_closet", SG_JSGLUE_METHOD_NAME(repo, sync_all_users_in_closet), 0,0},

	JS_FS_END
};

/*
 * properties and methods of a fetchblobhandle
 */
static JSPropertySpec sg_fetchblobhandle_properties[] = {
	{NULL,0,0,NULL,NULL}
};
static JSFunctionSpec sg_fetchblobhandle_methods[] = {
    {"next_chunk", SG_JSGLUE_METHOD_NAME(fetchblobhandle,next_chunk),0,0},
    {"abort", SG_JSGLUE_METHOD_NAME(fetchblobhandle,abort),0,0},
    JS_FS_END
};

/*
 * properties and methods of a fetchfilehandle
 */
static JSPropertySpec sg_fetchfilehandle_properties[] = {
	{NULL,0,0,NULL,NULL}
};
static JSFunctionSpec sg_fetchfilehandle_methods[] = {
    {"next_chunk", SG_JSGLUE_METHOD_NAME(fetchfilehandle, next_chunk),0,0},
    {"abort", SG_JSGLUE_METHOD_NAME(fetchfilehandle, abort),0,0},
    JS_FS_END
};

/*
 * These methods are available on the global static "sg" object.
 * example of usage:
 * sg.time()
 * sg.gid()
 */
static JSFunctionSpec sg_methods[] = {
    {"open_repo",            SG_JSGLUE_METHOD_NAME(sg,open_repo),       1,0},
	{"open_local_repo",	   SG_JSGLUE_METHOD_NAME(sg,open_local_repo), 0, 0},

    {"delete_repo",            SG_JSGLUE_METHOD_NAME(sg,delete_repo),       1,0},
    {"rename_repo",            SG_JSGLUE_METHOD_NAME(sg,rename_repo),       2,0},
    {"clone__exact",             SG_JSGLUE_METHOD_NAME(sg,clone__exact), 2,0},
    {"clone__pack",             SG_JSGLUE_METHOD_NAME(sg,clone__pack), 2,0},
    {"clone__all_full",             SG_JSGLUE_METHOD_NAME(sg,clone__all_full), 1,0},
    {"clone__all_zlib",             SG_JSGLUE_METHOD_NAME(sg,clone__all_zlib), 1,0},
	{"clone__prep_usermap",          SG_JSGLUE_METHOD_NAME(sg,clone__prep_usermap), 1,0},
	{"clone__finish_usermap",          SG_JSGLUE_METHOD_NAME(sg,clone__finish_usermap), 1,0},
    {"pull_branch_stuff_only",             SG_JSGLUE_METHOD_NAME(sg,pull_branch_stuff_only), 1,0},

	/**
	 * void sg.push_all(string src_repo_descriptor, string dest_repo_spec, [bool force=false])
	 */	
	{"push_all",               SG_JSGLUE_METHOD_NAME(sg, push_all), 2,0},

	/**
	* void sg.push_branch(string src_repo_descriptor, string dest_repo_spec, string branch_name, [bool force=false])
	*/	
	{"push_branch",               SG_JSGLUE_METHOD_NAME(sg, push_branch), 3,0},

	/**
	 * void sg.pull_all(string src_repo_spec, string dest_repo_descriptor)
	 */	
	{"pull_all",               SG_JSGLUE_METHOD_NAME(sg, pull_all), 2,0},
	{"incoming",               SG_JSGLUE_METHOD_NAME(sg, incoming), 2,0},
	{"outgoing",               SG_JSGLUE_METHOD_NAME(sg, outgoing), 2,0},

    {"zip",               SG_JSGLUE_METHOD_NAME(sg,zip), 3,0},

	{"server_state_dir", SG_JSGLUE_METHOD_NAME(sg,server_state_dir), 0,0},
#if (defined(MAC) || defined(LINUX)) && !defined(SG_IOS)
	{"user_home_dir",     SG_JSGLUE_METHOD_NAME(sg,user_home_dir), 0,0},
#endif

	{"time",              SG_JSGLUE_METHOD_NAME(sg,time),     0,0},
    {"sleep_ms",          SG_JSGLUE_METHOD_NAME(sg,sleep_ms), 1,0},
    {"platform",          SG_JSGLUE_METHOD_NAME(sg,platform), 0,0},
    {"trap_ctrl_c",       SG_JSGLUE_METHOD_NAME(sg,trap_ctrl_c),    0,0},
    {"ctrl_c_pressed",    SG_JSGLUE_METHOD_NAME(sg,ctrl_c_pressed), 0,0},
    {"rand",              SG_JSGLUE_METHOD_NAME(sg,rand),     0,0},
    {"make_dagnum",               SG_JSGLUE_METHOD_NAME(sg,make_dagnum),      1,0},
    {"gid",               SG_JSGLUE_METHOD_NAME(sg,gid),      0,0},
    {"set_local_setting", SG_JSGLUE_METHOD_NAME(sg,set_local_setting),      2,0},
	{"get_local_setting", SG_JSGLUE_METHOD_NAME(sg,get_local_setting),      1,0},
	{"set_local_setting__system_wide", SG_JSGLUE_METHOD_NAME(sg,set_local_setting__system_wide),      2,0},
    {"local_settings",    SG_JSGLUE_METHOD_NAME(sg,local_settings),      0,0},
	{"get_area_id", SG_JSGLUE_METHOD_NAME(sg,get_area_id), 1, 0},

	{"get_users_from_needs_usermap_repo",	SG_JSGLUE_METHOD_NAME(sg,get_users_from_needs_usermap_repo), 1, 0},

// TODO Now that we allow REPOs to select a Hash Algorithm, HIDs are
// TODO essentially variable length.  We need to have a REPO variable
// TODO on hand before we can request a HID be computed.  I'm commenting
// TODO out this method because I don't think it is being used.  I think
// TODO it was only added for debugging when we first got started.
//    {"hid",             SG_JSGLUE_METHOD_NAME(sg,hid),      1,0},

    {"jsmin",                            SG_JSGLUE_METHOD_NAME(sg,jsmin),                     1,0},

    {"exec",                             SG_JSGLUE_METHOD_NAME(sg,exec),                      1,0},
    {"exec_nowait",                      SG_JSGLUE_METHOD_NAME(sg,exec_nowait),               1,0},
    {"exec_result",                      SG_JSGLUE_METHOD_NAME(sg,exec_result),               1,0},
    {"list_descriptors",                 SG_JSGLUE_METHOD_NAME(sg,list_descriptors),          0,0},
    {"list_descriptors2",                SG_JSGLUE_METHOD_NAME(sg,list_descriptors2),         0,0},
    {"to_json",                          SG_JSGLUE_METHOD_NAME(sg,to_json),                   1,0},
    {"to_json__pretty_print",            SG_JSGLUE_METHOD_NAME(sg,to_json__pretty_print),     1,0},
    {"to_json__sz",                      SG_JSGLUE_METHOD_NAME(sg,to_json__sz),               1,0},
    {"to_sz",                            SG_JSGLUE_METHOD_NAME(sg,to_sz),                     1,0},
    {"from_json",                        SG_JSGLUE_METHOD_NAME(sg,from_json),                 1,0},
    {"log",                              SG_JSGLUE_METHOD_NAME(sg,log),                       1,0},
    {"console",                          SG_JSGLUE_METHOD_NAME(sg,console),                   1,0},
    {"hash",                             SG_JSGLUE_METHOD_NAME(sg,hash),                      2,0},
    {"repoid_to_descriptors",            SG_JSGLUE_METHOD_NAME(sg,repoid_to_descriptors),     1,0},	// should this be debug only

	{"get_descriptor",		             SG_JSGLUE_METHOD_NAME(sg,get_descriptor),            1,0},
    {"list_all_descriptors",             SG_JSGLUE_METHOD_NAME(sg,list_all_descriptors),      0,0},
    //list_all_available_descriptors is the same as list_all_descriptors except it takes into account excluded servers
    {"list_all_available_descriptors",             SG_JSGLUE_METHOD_NAME(sg,list_all_available_descriptors),      0,0},
    {"list_descriptors__status",         SG_JSGLUE_METHOD_NAME(sg,list_descriptors__status),  1,0},
    {"set_descriptor_status",            SG_JSGLUE_METHOD_NAME(sg,set_descriptor_status),     2,0},

	{"add_closet_property",            SG_JSGLUE_METHOD_NAME(sg,add_closet_property), 2,0},
	{"set_closet_property",            SG_JSGLUE_METHOD_NAME(sg,set_closet_property), 2,0},
	{"get_closet_property",            SG_JSGLUE_METHOD_NAME(sg,get_closet_property), 1,0},

	{"create_user_master_repo",			SG_JSGLUE_METHOD_NAME(sg,create_user_master_repo),	1,0},
	{"open_user_master_repo",			SG_JSGLUE_METHOD_NAME(sg,open_user_master_repo),	0,0},
	{"get_user_master_adminid",			SG_JSGLUE_METHOD_NAME(sg,get_user_master_adminid),	0,0},

	{"get_clone_staging_info",			SG_JSGLUE_METHOD_NAME(sg,get_clone_staging_info), 1,0},

	{"setenv",							SG_JSGLUE_METHOD_NAME(sg,setenv), 2, 0},
	{"getenv",							SG_JSGLUE_METHOD_NAME(sg,getenv), 1, 0},
	{"refresh_closet_location_from_environment_for_testing_purposes",SG_JSGLUE_METHOD_NAME(sg,refresh_closet_location_from_environment_for_testing_purposes),0,0},
    {"diff_files", 						SG_JSGLUE_METHOD_NAME(sg, diff_files), 5,0},

    {NULL,NULL,0,0}
};

/*
 * xmlwriter is not a global object; we create them as needed
 */

static JSPropertySpec xmlwriter_properties[] =
{
    {NULL,0,0,NULL,NULL}
};

static JSFunctionSpec xmlwriter_methods[] = {
	/* .start_element(name) */
	{"start_element", SG_JSGLUE_METHOD_NAME(xmlwriter, start_element), 1,0},

	/* .attribute(name, value) */
	{"attribute", SG_JSGLUE_METHOD_NAME(xmlwriter, attribute), 2,0},

	/* .content(text) */
	{"content", SG_JSGLUE_METHOD_NAME(xmlwriter, content), 1,0},

	/* .end_element() */
	{"end_element", SG_JSGLUE_METHOD_NAME(xmlwriter, end_element), 0,0},

	/* .element(name, content) */
	{"element", SG_JSGLUE_METHOD_NAME(xmlwriter, element), 2,0},

	/* .finish() */
	{"finish", SG_JSGLUE_METHOD_NAME(xmlwriter, finish), 0,0},

	{NULL, NULL, 0, 0}
};

/*
 * These methods are available on the global static "sg.fs" object.
 * examples of usage: sg.fs.length("/etc/hosts")
 * sg.fs.exists("/etc/hotss")
 * sg.fs.getcwd()
 */
static JSFunctionSpec fs_methods[] = {
    {"length",            SG_JSGLUE_METHOD_NAME(fs,length),       1,0},
    {"remove",            SG_JSGLUE_METHOD_NAME(fs,remove),       1,0},
    {"exists",            SG_JSGLUE_METHOD_NAME(fs,exists),       1,0},
    {"mkdir",            SG_JSGLUE_METHOD_NAME(fs,mkdir),       1,0},
    {"rmdir",            SG_JSGLUE_METHOD_NAME(fs,rmdir),       1,0},
    {"mkdir_recursive",   SG_JSGLUE_METHOD_NAME(fs,mkdir_recursive),       1,0},
    {"rmdir_recursive",   SG_JSGLUE_METHOD_NAME(fs,rmdir_recursive),       1,0},
    {"move",  	      SG_JSGLUE_METHOD_NAME(fs,move),       2,0},
    //Gets the Current Working Directory
    {"getcwd",  	  	      SG_JSGLUE_METHOD_NAME(fs,getcwd),       0,0},
    {"getcwd_top",  	  	      SG_JSGLUE_METHOD_NAME(fs,getcwd_top),       0,0},
    {"chmod",  	  	      SG_JSGLUE_METHOD_NAME(fs,chmod),       2,0},
	//This returns a javascript object that contains information about the timestamp and permissions, and other stuff
    {"stat",  	  	      SG_JSGLUE_METHOD_NAME(fs,stat),       1,0},
	{"readdir",			SG_JSGLUE_METHOD_NAME(fs,readdir),	1,0},
	{"ls",			SG_JSGLUE_METHOD_NAME(fs,ls),	1,0},
    //Change the Current Working Directory
    {"cd",  	  	      SG_JSGLUE_METHOD_NAME(fs,cd),       1,0},
    {"tmpdir", SG_JSGLUE_METHOD_NAME(fs,tmpdir), 0,0},
    {"fetch_file",            SG_JSGLUE_METHOD_NAME(fs,fetch_file),       3,0},
    // fsobj_type?
    // perms?
    // time?
    {NULL,NULL,0,0}
};

/*
 * These methods are available on the global static "sg.server" object.
 * example usage: sg.server.debug_shutdown()
 */
static JSFunctionSpec server_methods[] = {
	{"debug_shutdown", SG_JSGLUE_METHOD_NAME(server,debug_shutdown), 0,0},
	{"request_profiler_start", SG_JSGLUE_METHOD_NAME(server,request_profiler_start), 1,0},
	{"request_profiler_stop",  SG_JSGLUE_METHOD_NAME(server,request_profiler_stop),  0,0},
	{NULL,NULL,0,0}
};

/*
 * These methods are available on the global static "sg.file" object.
 * examples of usage: sg.file.write("relative/path/to/object", "file contents")
 * sg.file.append("file.txt", "new line to append")
 */
static JSFunctionSpec file_methods[] = {
    {"write",  SG_JSGLUE_METHOD_NAME(file, write),  2,0},
    {"append", SG_JSGLUE_METHOD_NAME(file, append), 2,0},
    {"read",   SG_JSGLUE_METHOD_NAME(file, read),   1,0},
    {"hash",   SG_JSGLUE_METHOD_NAME(file, hash),   1,0},
    {NULL,NULL,0,0}
};

// TODO need class/methods to do SG_pathname operations

//////////////////////////////////////////////////////////////////

#include "sg_jsglue__private_setprop.h"
#include "sg_jsglue__private_sg_config.h"
#include "sg_jsglue__private_sync_remote.h"
#include "sg_jsglue__private_sg_curl.h"
#include "sg_jsglue__private_usermap.h"
#include "sg_jsglue__private_mutex.h"

//////////////////////////////////////////////////////////////////

/* ---------------------------------------------------------------- */
/* This is the function which sets everything up */
/* ---------------------------------------------------------------- */

void SG_jsglue__install_scripting_api(SG_context * pCtx,
									  JSContext *cx, JSObject *glob)
{
	const char * szVersion = NULL;
    JSObject* pjsobj_sg = NULL;
    JSObject* pjsobj_fs = NULL;
	JSObject* pjsobj_sync_remote = NULL;
    JSObject* pjsobj_server = NULL;
    JSObject* pjsobj_file = NULL;
    JSObject* pjsobj_dagnums = NULL;
    JSObject* pjsobj_blobencodings = NULL;
	JSObject* pjsobj_vendors = NULL;
	JSObject* pjsobj_curl = NULL;
	JSObject* pjsobj_usermap = NULL;
	JSObject* pjsobj_repo_statuses = NULL;
	JSObject* pjsobj_mutex = NULL;
    jsval jv;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

	SG_JS_NULL_CHECK(  pjsobj_sg = JS_DefineObject(cx, glob, "sg", &sg_class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_sg, sg_methods)  );

	// TODO 2010/11/30 This defines "sg.version".  But we also have "sg.config.version.*"
	// TODO            (defined in sg_jsglue__private_sg_config.h).  Shouldn't we combine these?

	SG_ERR_CHECK(  SG_lib__version(pCtx, &szVersion)  );
	JSVAL_FROM_SZ(jv, szVersion);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_sg, "version", &jv)  );

	SG_ERR_CHECK(  SG_vv2__jsglue__install_scripting_api(pCtx, cx, glob, pjsobj_sg)  );
	SG_ERR_CHECK(  SG_wc__jsglue__install_scripting_api(pCtx, cx, glob, pjsobj_sg)  );


	SG_JS_NULL_CHECK(  pjsobj_fs = JS_DefineObject(cx, pjsobj_sg, "fs", &fs_class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_fs, fs_methods)  );

	SG_JS_NULL_CHECK(  pjsobj_sync_remote = JS_DefineObject(cx, pjsobj_sg, "sync_remote", &sg_sync_remote__class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_sync_remote, sg_sync_remote__methods)  );

	SG_JS_NULL_CHECK(  pjsobj_server = JS_DefineObject(cx, pjsobj_sg, "server", &server_class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_server, server_methods)  );

	SG_JS_NULL_CHECK(  pjsobj_mutex = JS_DefineObject(cx, pjsobj_sg, "mutex", &sg_mutex__class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_mutex, sg_mutex__methods)  );

	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__COOKIE_AUTH);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "COOKIE_AUTH", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__ONPOSTRECEIVED_CB);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "ONPOSTRECEIVED", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__AUTH);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "AUTH", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__ONDISPATCH_CB);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "ONDISPATCH", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__CACHE_CHECK);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "CACHE_CHECK", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__CACHE_WRITE);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "CACHE_WRITE", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__CACHE_READ);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "CACHE_READ", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_READ);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "SESSION_READ", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_WRITE);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "SESSION_WRITE", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_CLEANUP);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "SESSION_CLEANUP", &jv)  );
	jv = INT_TO_JSVAL(SG_HTTPREQUESTPROFILER_CATEGORY__ACTIVITY_DISPATCH);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_server, "ACTIVITY_DISPATCH", &jv)  );

    SG_JS_NULL_CHECK(  pjsobj_file = JS_DefineObject(cx, pjsobj_sg, "file", &file_class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_file, file_methods)  );

	SG_JS_NULL_CHECK(  pjsobj_usermap = JS_DefineObject(cx, pjsobj_sg, "usermap", &sg_usermap__class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_usermap, sg_usermap__methods)  );

	// Define global sg.config. array with a list of known values.
	SG_ERR_CHECK(  sg_jsglue__install_sg_config(pCtx, cx, pjsobj_sg)  );
	
	// Define the curl_response class
	SG_ERR_CHECK( sg_jsglue__install_sg_curl(pCtx, cx, glob) );
	
	SG_JS_NULL_CHECK(  pjsobj_curl = JS_DefineObject(cx, pjsobj_sg, "curl", &sg_curl__class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_curl, sg_curl__methods)  );

    SG_JS_NULL_CHECK(  pjsobj_blobencodings = JS_DefineObject(cx, pjsobj_sg, "blobencoding", NULL, NULL, 0)  );

    jv = INT_TO_JSVAL(SG_BLOBENCODING__FULL);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_blobencodings, "FULL", &jv)  );

    jv = INT_TO_JSVAL(SG_BLOBENCODING__KEEPFULLFORNOW);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_blobencodings, "KEEPFULLFORNOW", &jv)  );

    jv = INT_TO_JSVAL(SG_BLOBENCODING__ALWAYSFULL);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_blobencodings, "ALWAYSFULL", &jv)  );

    jv = INT_TO_JSVAL(SG_BLOBENCODING__ZLIB);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_blobencodings, "ZLIB", &jv)  );

    jv = INT_TO_JSVAL(SG_BLOBENCODING__VCDIFF);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_blobencodings, "VCDIFF", &jv)  );

	SG_JS_NULL_CHECK(  pjsobj_vendors = JS_DefineObject(cx, pjsobj_sg, "vendor", NULL, NULL, 0)  );

	jv = INT_TO_JSVAL(SG_VENDOR__SOURCEGEAR);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_vendors, "SOURCEGEAR", &jv)  );

    SG_JS_NULL_CHECK(  pjsobj_dagnums = JS_DefineObject(cx, pjsobj_sg, "dagnum", NULL, NULL, 0)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VERSION_CONTROL, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VERSION_CONTROL", &jv)  );

#if 0
	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__WORK_ITEMS, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "WORK_ITEMS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__BUILDS, buf_dagnum, sizeof(buf_dagnum))  );
    jv = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf_dagnum));
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "BUILDS", &jv)  );
#endif

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__USERS, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "USERS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__AREAS, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "AREAS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VC_COMMENTS, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VC_COMMENTS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VC_TAGS, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VC_TAGS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VC_BRANCHES, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VC_BRANCHES", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VC_STAMPS, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VC_STAMPS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VC_HOOKS, buf_dagnum, sizeof(buf_dagnum))  );
    jv = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf_dagnum));
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VC_HOOKS", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VC_LOCKS, buf_dagnum, sizeof(buf_dagnum))  );
    jv = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf_dagnum));
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "VC_LOCKS", &jv)  );

// the following are only used in the test suite
#define SG_DAGNUM__TESTING__DB   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__TESTING, \
        SG_DAGNUM__SET_DAGID(1, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        ((SG_uint64) 0)))))

#define SG_DAGNUM__TESTING2__DB   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__TESTING, \
        SG_DAGNUM__SET_DAGID(2, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        ((SG_uint64) 0)))))

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__TESTING__DB, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "TESTING_DB", &jv)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__TESTING2__DB, buf_dagnum, sizeof(buf_dagnum))  );
    JSVAL_FROM_SZ(jv, buf_dagnum);
    SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_dagnums, "TESTING2_DB", &jv)  );


	SG_JS_NULL_CHECK(  pjsobj_repo_statuses = JS_DefineObject(cx, pjsobj_sg, "repo_status", NULL, NULL, 0)  );
	jv = INT_TO_JSVAL(SG_REPO_STATUS__NORMAL);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_repo_statuses, "NORMAL", &jv)  );
	jv = INT_TO_JSVAL(SG_REPO_STATUS__CLONING);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_repo_statuses, "CLONING", &jv)  );
	jv = INT_TO_JSVAL(SG_REPO_STATUS__NEED_USER_MAP);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_repo_statuses, "NEEDS_USER_MAP", &jv)  );
	jv = INT_TO_JSVAL(SG_REPO_STATUS__IMPORTING);
	SG_JS_BOOL_CHECK(  JS_SetProperty(cx, pjsobj_repo_statuses, "IMPORTING", &jv)  );
	
	SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_audit_class,
            NULL, /* no constructor */
            0, /* nargs */
            sg_audit_properties,
            sg_audit_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_cbuffer_class,
            NULL, /* no constructor */
            0, /* nargs */
            sg_cbuffer_properties,
            sg_cbuffer_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_repo_class,
            NULL, /* no constructor */
            0, /* nargs */
            sg_repo_properties,
            sg_repo_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_blobset_class,
            NULL, /* no constructor */
            0, /* nargs */
            sg_blobset_properties,
            sg_blobset_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_fetchblobhandle_class,
            NULL, /* no constructor */
            0, /* nargs */
            sg_fetchblobhandle_properties,
            sg_fetchblobhandle_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_fetchfilehandle_class,
            NULL, /* no constructor */
            0, /* nargs */
            sg_fetchfilehandle_properties,
            sg_fetchfilehandle_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );
      

	SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &xmlwriter_class,
            xmlwriter_constructor,
            0, /* nargs */
            xmlwriter_properties,
            xmlwriter_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

    return;

fail:
    // TODO we had an error installing stuff.
    // TODO do we need to tear down anything that we just created?
    return;
}
