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

#include "sg_js_safeptr__private.h"

#include "sg_zing__private.h"

void sg_zingdb__get_leaf(
    SG_context* pCtx,
	sg_zingdb* pz,
    char** pp
    );

void sg_zingdb__free(
        SG_context* pCtx,
        sg_zingdb* pzs
        );

void sg_zingdb__alloc(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	sg_zingdb** pp
    );

struct sg_zingdb
{
    SG_repo* pRepo;
    SG_uint64 iDagNum;
};

struct sg_statetxcombo
{
    sg_zingdb* pzstate;
    SG_zingtx* pztx;
    SG_vector* pvec_records;
};

void sg_zingdb__free(
        SG_context* pCtx,
        sg_zingdb* pzs
        )
{
    if (pzs)
    {
        SG_NULLFREE(pCtx, pzs);
    }
}

void sg_statetxcombo__alloc(
    SG_context* pCtx,
    sg_zingdb* pzstate,
    SG_zingtx* pztx,
	sg_statetxcombo** pp
    )
{
    sg_statetxcombo* pThis = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );

    pThis->pzstate = pzstate;
    pThis->pztx = pztx;

    SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pThis->pvec_records, 10)  );

    *pp = pThis;
    pThis = NULL;

    return;

fail:
    SG_NULLFREE(pCtx, pThis);
}

void sg_zingdb__alloc(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	sg_zingdb** pp
    )
{
    sg_zingdb* pThis = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(pp);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );

    pThis->pRepo = pRepo;
    pThis->iDagNum = iDagNum;

    *pp = pThis;
    pThis = NULL;

    return;

fail:
    sg_zingdb__free(pCtx, pThis);
}

/* TODO make sure we tell JS that strings are utf8 */

/*
 * Any object that requires a pointer (like an SG_zingtx)
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

/**
 * We cannot control the prototypes of methods and properties;
 * these are defined by SpiderMonkey.
 *
 * Note that they do not take a SG_Context.  Methods and properties
 * will have to extract it from the js-context-private data.
 */
#define SG_ZING_JSGLUE_METHOD_NAME(class, name) sg_zing_jsglue__method__##class##__##name
#define SG_ZING_JSGLUE_METHOD_PROTOTYPE(class, name) static JSBool SG_ZING_JSGLUE_METHOD_NAME(class, name)(JSContext *cx, uintN argc, jsval *vp)

#define SG_ZING_JSGLUE_PROPERTY_NAME(class, name) sg_zing_jsglue__property__##class##__##name
#define SG_ZING_JSGLUE_PROPERTY_PROTOTYPE(class, name) static JSBool SG_ZING_JSGLUE_PROPERTY_NAME(class, name)(JSContext *cx, JSObject *obj, jsid id, jsval *vp)

/* ---------------------------------------------------------------- */
/* Utility functions */
/* ---------------------------------------------------------------- */

SG_safeptr* sg_zing_jsglue__get_object_private(JSContext *cx, JSObject *obj)
{
    SG_safeptr* psp = (SG_safeptr*) JS_GetPrivate(cx, obj);
    return psp;
}

JSBool zingrec_getter(JSContext *cx, JSObject *obj, jsid id, jsval *vp);
JSBool zingrec_setter(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);
void zingrec_finalize(JSContext *cx, JSObject *obj);
void zingdb_finalize(JSContext *cx, JSObject *obj);

/* ---------------------------------------------------------------- */
/* JSClass structures.  The InitClass calls are at the bottom of
 * the file */
/* ---------------------------------------------------------------- */
static JSClass sg_zingdb_class = {
    "zingdb",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, zingdb_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_zingtx_class = {
    "sg_zingtx",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass sg_zingrecord_class = {
    "sg_zingrecord",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, zingrec_getter, zingrec_setter,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, zingrec_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

/* ---------------------------------------------------------------- */
/* JSNative method definitions */
/* ---------------------------------------------------------------- */
JSBool zingrec_getter(JSContext *cx, JSObject *obj, jsid id,
                                 jsval *vp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval idval = JSVAL_VOID;
	SG_zingrecord* pZingRec = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, obj);
	const char* psz_recid = NULL;
	char* psz_name = NULL;
	JSString* pjstr = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    JSString* pjs = NULL;
    JSObject* jso = NULL;

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__zingrecord(pCtx, psp, &pZingRec)  );
    SG_NULLARGCHECK(pZingRec);

	SG_JS_BOOL_CHECK(  JS_IdToValue(cx, id, &idval)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(idval)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(idval), &psz_name)  );

    if (strcmp(psz_name, SG_ZING_FIELD__RECID) == 0)
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pZingRec, &psz_recid) );
        SG_JS_NULL_CHECK(  pjstr = JS_NewStringCopyZ(cx, psz_recid)  );
        *vp = STRING_TO_JSVAL(pjstr);
    }
    else if (strcmp(psz_name, SG_ZING_FIELD__RECTYPE) == 0)
    {
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pZingRec, &psz_rectype) );
        SG_JS_NULL_CHECK(  pjstr = JS_NewStringCopyZ(cx, psz_rectype)  );
        *vp = STRING_TO_JSVAL(pjstr);
    }
    else if (strcmp(psz_name, SG_ZING_FIELD__HISTORY) == 0)
    {
        SG_varray* pva = NULL;
        SG_ERR_CHECK(  SG_zingrecord__get_history(pCtx, pZingRec, &pva) );

        SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
        *vp = OBJECT_TO_JSVAL(jso);
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva, jso)   );
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__lookup_name(pCtx, pZingRec, psz_name, &pzfa)  );

        if (pzfa)
        {
            switch (pzfa->type)
            {
                case SG_ZING_TYPE__BOOL:
                    {
                        SG_bool b = SG_FALSE;

                        SG_zingrecord__get_field__bool(pCtx, pZingRec, pzfa, &b);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							*vp = BOOLEAN_TO_JSVAL(b);
						}
                        break;
                    }

                case SG_ZING_TYPE__DATETIME:
                    {
                        SG_int64 t;

                        // TODO should we perhaps convert to a Date object?

                        SG_zingrecord__get_field__datetime(pCtx, pZingRec, pzfa, &t);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							if (SG_int64__fits_in_double(t))
							{
								double d = (double) t;
								jsval jv;

								SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, d, &jv)  );
								*vp = jv;
							}
							else
							{
								SG_ERR_THROW(  SG_ERR_INTEGER_OVERFLOW  );
							}
						}
                        break;
                    }

                case SG_ZING_TYPE__INT:
                    {
                        SG_int64 intVal;

                        SG_zingrecord__get_field__int(pCtx, pZingRec, pzfa, &intVal);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							if (SG_int64__fits_in_double(intVal))
							{
								double d = (double) intVal;
								jsval jv;

								SG_JS_BOOL_CHECK(  JS_NewNumberValue(cx, d, &jv)  );
								*vp = jv;
							}
							else
							{
								SG_ERR_THROW(  SG_ERR_INTEGER_OVERFLOW  );
							}
						}
                        break;
                    }

                case SG_ZING_TYPE__USERID:
                    {
                        const char* psz_val = NULL;

                        SG_zingrecord__get_field__userid(pCtx, pZingRec, pzfa, &psz_val);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							// TODO do we really want to just return the user id?
							SG_JS_NULL_CHECK(  pjs = JS_NewStringCopyZ(cx, psz_val)  );
							*vp = STRING_TO_JSVAL(pjs);
						}
                        break;
                    }

                case SG_ZING_TYPE__REFERENCE:
                    {
                        const char* psz_val = NULL;

                        SG_zingrecord__get_field__reference(pCtx, pZingRec, pzfa, &psz_val);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							SG_JS_NULL_CHECK(  pjs = JS_NewStringCopyZ(cx, psz_val)  );
							*vp = STRING_TO_JSVAL(pjs);
						}
                        break;
                    }

                case SG_ZING_TYPE__STRING:
                    {
                        const char* psz_val = NULL;

                        SG_zingrecord__get_field__string(pCtx, pZingRec, pzfa, &psz_val);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							SG_JS_NULL_CHECK(  pjs = JS_NewStringCopyZ(cx, psz_val)  );
							*vp = STRING_TO_JSVAL(pjs);
						}
                        break;
                    }

                case SG_ZING_TYPE__ATTACHMENT:
                    {
                        const char* psz_val = NULL;

                        // JS get field on an attachment returns the HID of the blob
                        SG_zingrecord__get_field__attachment(pCtx, pZingRec, pzfa, &psz_val);
						if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						{
							SG_ERR_DISCARD;
							*vp = JSVAL_VOID;
						}
						else
						{
							SG_ERR_CHECK_CURRENT;
							SG_JS_NULL_CHECK(  pjs = JS_NewStringCopyZ(cx, psz_val)  );
							*vp = STRING_TO_JSVAL(pjs);
						}
                        break;
                    }

                default:
                    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
                    break;
            }
        }
        else
        {
            SG_NULLFREE(pCtx, psz_name);
            return JS_PropertyStub(cx, obj, id, vp);
        }
    }

	SG_NULLFREE(pCtx, psz_name);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_name);
	return JS_FALSE;
}

void sg_zing_jsglue__set_field(
        JSContext* cx,
        SG_context* pCtx,
        SG_zingrecord* pZingRec,
        SG_zingfieldattributes* pzfa,
        const char* psz_name,
        jsval* vp
        )
{
    SG_pathname* pPath = NULL;
	SG_vhash* pvh = NULL;
	SG_varray* pva = NULL;
	SG_string* pstr = NULL;
	char *sz = NULL;

    // assigning a field to null is equivalent to removing it from the record
    if (JSVAL_IS_NULL(*vp))
    {
        SG_ERR_CHECK(  SG_zingrecord__remove_field(pCtx, pZingRec, pzfa)  );
    }
    else
    {
        switch (pzfa->type)
        {
            case SG_ZING_TYPE__ATTACHMENT:
                {
                    if (JSVAL_IS_STRING(*vp))
                    {
                        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPath, cx, JSVAL_TO_STRING(*vp))  );
                        SG_ERR_CHECK(  SG_zingrecord__set_field__attachment__pathname(pCtx, pZingRec, pzfa, &pPath) );
						SG_PATHNAME_NULLFREE(pCtx, pPath);
                    }
					else if (JSVAL_IS_OBJECT(*vp))
					{
						// Store the object as JSON.
						JSObject* pjso = JSVAL_TO_OBJECT(*vp);

						if (JS_IsArrayObject(cx, pjso))
						{
							SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, pjso, &pva)  );
							SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
							SG_ERR_CHECK(  SG_varray__to_json(pCtx, pva, pstr)  );
							SG_VARRAY_NULLFREE(pCtx, pva);
						}
						else
						{
							SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, pjso, &pvh)  );
							SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
							SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );
							SG_VHASH_NULLFREE(pCtx, pvh);
						}
						SG_ERR_CHECK(  SG_zingrecord__set_field__attachment__buflen(pCtx, pZingRec, pzfa, 
							(const SG_byte*)SG_string__sz(pstr), SG_string__length_in_bytes(pstr))  );
						SG_STRING_NULLFREE(pCtx, pstr);
					}
                    else
                    {
                        // TODO what other ways could an attachment be set?
                        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                    }
                    break;
                }

            case SG_ZING_TYPE__BOOL:
                {
                    if (JSVAL_IS_BOOLEAN(*vp))
                    {
                        SG_ERR_CHECK(  SG_zingrecord__set_field__bool(pCtx, pZingRec, pzfa, JSVAL_TO_BOOLEAN(*vp)) );
                    }
                    else
                    {
                        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                    }
                    break;
                }

            case SG_ZING_TYPE__DATETIME:
                {
                    if (JSVAL_IS_INT(*vp))
                    {
                        SG_ERR_CHECK(  SG_zingrecord__set_field__datetime(pCtx, pZingRec, pzfa, JSVAL_TO_INT(*vp)) );
                    }
                    else if (JSVAL_IS_NUMBER(*vp))
                    {
                        jsdouble d;
                        SG_int64 i;

                        SG_JS_BOOL_CHECK( JS_ValueToNumber(cx, *vp, &d)  );
                        i = (SG_int64)d;

                        SG_ERR_CHECK(  SG_zingrecord__set_field__datetime(pCtx, pZingRec, pzfa, i) );
                    }
                    else if (JSVAL_IS_OBJECT(*vp))
                    {
                        JSObject* jsotmp = NULL;

                        jsotmp = JSVAL_TO_OBJECT(*vp);
                        if (JS_ObjectIsDate(cx, jsotmp))
                        {
                            jsval tmpval;
                            jsdouble d;
                            SG_int64 i;

                            SG_JS_BOOL_CHECK( JS_CallFunctionName(cx, jsotmp, "getTime", 0, NULL, &tmpval) );

                            SG_JS_BOOL_CHECK( JS_ValueToNumber(cx, *vp, &d)  );
                            i = (SG_int64)d;

                            SG_ERR_CHECK(  SG_zingrecord__set_field__datetime(pCtx, pZingRec, pzfa, i) );
                        }
                        else
                        {
                            SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                        }
                    }
                    else
                    {
                        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                    }
                    break;
                }

            case SG_ZING_TYPE__INT:
                {
                    if (JSVAL_IS_INT(*vp))
                    {
                        SG_ERR_CHECK(  SG_zingrecord__set_field__int(pCtx, pZingRec, pzfa, JSVAL_TO_INT(*vp)) );
                    }
                    else
                    {
                        SG_ERR_THROW2(  SG_ERR_ZING_TYPE_MISMATCH,
                            (pCtx, "Field '%s'", psz_name)
                            );
                    }
                    break;
                }

            case SG_ZING_TYPE__STRING:
                {
                    if (JSVAL_IS_STRING(*vp))
                    {
                    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(*vp), &sz)  );
                        SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, pZingRec, pzfa, sz)  );
                        SG_NULLFREE(pCtx, sz);
                    }
                    else
                    {
                        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                    }
                    break;
                }

            case SG_ZING_TYPE__USERID:
                {
                    if (JSVAL_IS_STRING(*vp))
                    {
                    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(*vp), &sz)  );
                        SG_ERR_CHECK(  SG_zingrecord__set_field__userid(pCtx, pZingRec, pzfa, sz)  );
                        SG_NULLFREE(pCtx, sz);
                    }
                    else
                    {
                        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                    }
                    break;
                }

            case SG_ZING_TYPE__REFERENCE:
                {
                    if (JSVAL_IS_STRING(*vp))
                    {
                    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(*vp), &sz)  );
                        SG_ERR_CHECK(  SG_zingrecord__set_field__reference(pCtx, pZingRec, pzfa, sz)  );
                        SG_NULLFREE(pCtx, sz);
                    }
                    else
                    {
                        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
                    }
                    break;
                }

            default:
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
                break;
        }
    }

	return;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_NULLFREE(pCtx, sz);
}

JSBool zingrec_setter(JSContext *cx, JSObject *obj, jsid id, JSBool strict,
                                 jsval *vp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval idval = JSVAL_VOID;
	SG_zingrecord* pZingRec = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, obj);
	char* psz_name = NULL;
    SG_zingfieldattributes* pzfa = NULL;

	SG_UNUSED(strict);

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__zingrecord(pCtx, psp, &pZingRec)  );
    SG_NULLARGCHECK(pZingRec);

	SG_JS_BOOL_CHECK(  JS_IdToValue(cx, id, &idval)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(idval)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(idval), &psz_name)  );

    SG_ERR_CHECK(  SG_zingrecord__lookup_name(pCtx, pZingRec, psz_name, &pzfa)  );

    if (pzfa)
    {
        SG_ERR_CHECK(  sg_zing_jsglue__set_field(cx, pCtx, pZingRec, pzfa, psz_name, vp)  );
    }
    else
    {
        // rec.foo is not allowed if foo is not a defined field in the template
        SG_ERR_THROW2(  SG_ERR_ZING_FIELD_NOT_FOUND,
                        (pCtx, "Field '%s'", psz_name)
                );
    }

	SG_NULLFREE(pCtx, psz_name);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_name);
	return JS_FALSE;

}

void zingdb_finalize(JSContext *cx, JSObject *obj)
{
	SG_context * pCtx = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, obj);
	sg_zingdb* pzstate = NULL;

	//err = SG_context__alloc__temporary(&pCtx);
	SG_context__alloc__temporary(&pCtx);
    // TODO check err

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pzstate)  );
	sg_zingdb__free(pCtx, pzstate);
	SG_SAFEPTR_NULLFREE(pCtx, psp);
    JS_SetPrivate(cx, obj, NULL);
	SG_CONTEXT_NULLFREE(pCtx);
	return;

fail:
	sg_zingdb__free(pCtx, pzstate);
	SG_SAFEPTR_NULLFREE(pCtx, psp);
	SG_CONTEXT_NULLFREE(pCtx);
}

void zingrec_finalize(JSContext *cx, JSObject *obj)
{
	SG_context * pCtx = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, obj);

	//err = SG_context__alloc__temporary(&pCtx);
	SG_context__alloc__temporary(&pCtx);
    // TODO check err

	SG_NULLARGCHECK(psp);

	SG_SAFEPTR_NULLFREE(pCtx, psp);
    JS_SetPrivate(cx, obj, NULL);
	SG_CONTEXT_NULLFREE(pCtx);
	return;
fail:
	SG_SAFEPTR_NULLFREE(pCtx, psp);
	SG_CONTEXT_NULLFREE(pCtx);
}

JSBool zingdb_constructor(JSContext *cx, uintN argc, jsval *vp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	JSObject* jso = NULL;
    sg_zingdb* pzstate = NULL;
	SG_safeptr* psp = NULL;
	SG_repo* pRepo1 = NULL;
	SG_safeptr * pspRepo = NULL;
    char* psz_hid_cs0 = NULL;
    SG_uint64 iDagNum = 0;
    char *psz_dagnum = NULL;

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	
	pspRepo = sg_zing_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo1)  );
	if (pRepo1 == NULL)
    {
		goto fail;
    }

    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_dagnum)  );
    SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &iDagNum)  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_zingdb_class, NULL, NULL)  );

	SG_ERR_CHECK(  sg_zingdb__alloc(pCtx, pRepo1, iDagNum, &pzstate)  );
	SG_ERR_CHECK(  SG_safeptr__wrap__zingdb(pCtx, pzstate, &psp)  );
    JS_SetPrivate(cx, jso, psp);

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_NULLFREE(pCtx, psz_hid_cs0);
	SG_NULLFREE(pCtx, psz_dagnum);
	return JS_TRUE;

fail:
    sg_zingdb__free(pCtx, pzstate);
	SG_NULLFREE(pCtx, psz_dagnum);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, get_template)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_zingtemplate * pZingTemplate = NULL;
	SG_vhash * pvh = NULL;
	JSObject * jso = NULL;
	SG_NULLARGCHECK(psp);

	SG_UNUSED(argc);


	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pcombo->pztx, &pZingTemplate)  );
	if (pZingTemplate != NULL)
	{
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
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

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, set_template)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_vhash * pvh = NULL;
	SG_NULLARGCHECK(psp);
	SG_JS_BOOL_CHECK(  1 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[0])  );

	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh)  );
	SG_ERR_CHECK(  SG_zingtx__set_template__new(pCtx, pcombo->pztx, &pvh)  );
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, get_history)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_safeptr* psp = NULL;
    char *psz_recid = NULL;
    char *psz_rectype = NULL;
    JSObject* jso = NULL;
    sg_zingdb* pz = NULL;
    SG_varray* pva = NULL;

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

	SG_JS_BOOL_CHECK(  2 == argc  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_recid)  );

    SG_ERR_CHECK(  SG_repo__dbndx__query_record_history(pCtx, pz->pRepo, pz->iDagNum, psz_recid, psz_rectype, &pva)  );

    SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva, jso)   );
    SG_VARRAY_NULLFREE(pCtx, pva);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
    return JS_TRUE;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, get_record)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = NULL;
	SG_vhash* pvh_rec = NULL;
	char *psz_rectype = NULL;
	char *psz_recid = NULL;
	JSObject* jso = NULL;
	sg_zingdb* pz = NULL;
	char* psz_csid = NULL;
	SG_varray *vafields = NULL;
	JSObject *jso_fields = NULL;

	psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

	SG_JS_BOOL_CHECK(  (argc >= 2) && (argc <= 4)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_recid)  );

	if (argc > 2)
	{
		if (!JSVAL_IS_NULL(argv[1]))
		{
		SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
    	jso_fields = JSVAL_TO_OBJECT(argv[2]);
    	SG_JS_BOOL_CHECK(  JS_IsArrayObject(cx, jso_fields)  );
    	SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso_fields, &vafields)  );
	}
	}

	if (argc > 3)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &psz_csid)  );
	}
	else
	{
		SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_csid)  );
	}

	if (vafields)
	{
        SG_ERR_CHECK(  SG_zing__get_record__vhash__fields(pCtx, pz->pRepo, pz->iDagNum, psz_csid, psz_rectype, psz_recid, vafields, &pvh_rec)  );

    	SG_VARRAY_NULLFREE(pCtx, vafields);
	}
	else
	{
    	SG_ERR_CHECK(  SG_zing__get_record__vhash(pCtx, pz->pRepo, pz->iDagNum, psz_csid, psz_rectype, psz_recid, &pvh_rec)  );
    }

    SG_NULLFREE(pCtx, psz_csid);

	if (pvh_rec != NULL)
	{
        SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_rec, jso)  );
        SG_VHASH_NULLFREE(pCtx, pvh_rec);
	}
	else
	    JS_SET_RVAL(cx, vp, JSVAL_NULL);

	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
    return JS_TRUE;

fail:
	SG_VARRAY_NULLFREE(pCtx, vafields);
	SG_NULLFREE(pCtx, psz_csid);
	SG_VHASH_NULLFREE(pCtx, pvh_rec);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}


SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, get_record_all_versions)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* psp = NULL;
	char *psz_rectype = NULL;
	char *psz_recid = NULL;
	JSObject* jso = NULL;
	sg_zingdb* pz = NULL;
	char* psz_csid = NULL;
	SG_varray *fields= NULL;
	SG_varray *pva_crit = NULL;
	SG_varray *recs = NULL;

	psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

	SG_JS_BOOL_CHECK(  argc == 2  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_recid)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_recid)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, fields, SG_ZING_FIELD__GIMME_ALL_FIELDS)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, fields, SG_ZING_FIELD__HIDREC)  );

    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pz->pRepo, pz->iDagNum, NULL, psz_rectype, pva_crit, NULL, 0, 0, fields, &recs)  );

	if (recs != NULL)
	{
        SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, recs, jso)   );
	    SG_VARRAY_NULLFREE(pCtx, recs);
	}
	else
	    JS_SET_RVAL(cx, vp, JSVAL_NULL);

	SG_VARRAY_NULLFREE(pCtx, pva_crit);
	SG_VARRAY_NULLFREE(pCtx, fields);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);

    return JS_TRUE;

fail:
	SG_VARRAY_NULLFREE(pCtx, fields);
	SG_VARRAY_NULLFREE(pCtx, recs);
	SG_VARRAY_NULLFREE(pCtx, pva_crit);
	SG_NULLFREE(pCtx, psz_csid);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, get_template)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_vhash* pvh = NULL;
    char* psz_csid = NULL;

    SG_JS_BOOL_CHECK(  0 == argc  );

    // TODO this func always returns the template from the leaf.
    // it might want to optionally accept a csid parameter

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

	if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pz->iDagNum))
	{
		SG_ERR_CHECK( SG_zing__get_cached_template__static_dagnum(pCtx, pz->iDagNum, &pzt) );
	}
    else
	{
		SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_csid)  );
    	SG_ERR_CHECK(  SG_zing__get_cached_template__csid(pCtx, pz->pRepo, pz->iDagNum, psz_csid, &pzt)  );
	}
    SG_NULLFREE(pCtx, psz_csid);
    SG_ERR_CHECK(  SG_zingtemplate__get_vhash(pCtx, pzt, &pvh)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );

    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, psz_csid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, get_leaves)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    SG_safeptr* psp = NULL;
    JSObject* jsa = NULL;
    SG_uint32 i = 0;
    JSString* pjstr = NULL;
    sg_zingdb* pz = NULL;
    SG_rbtree* prb_leaves = NULL;
    const char* psz_hid = NULL;
	SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;

    SG_JS_BOOL_CHECK(  0 == argc  );

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

    SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pz->pRepo, pz->iDagNum, &prb_leaves)  );

    SG_JS_NULL_CHECK(  jsa = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jsa));

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_leaves, &b, &psz_hid, NULL)  );
    while (b)
    {
        jsval jv;

        SG_JS_NULL_CHECK(  pjstr = JS_NewStringCopyZ(cx, psz_hid)  );
        jv = STRING_TO_JSVAL(pjstr);
        SG_JS_BOOL_CHECK(  JS_SetElement(cx, jsa, i++, &jv)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);

    return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, query__fts)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    char* psz_leaf = NULL;
    const char* psz_csid = NULL;
    char *psz_specified_state = NULL;
    char *psz_rectype = NULL;
    char *psz_field_name = NULL;
    char *psz_keywords = NULL;
    SG_varray* pva_fields = NULL;
    SG_varray* pva_sliced = NULL;
    JSObject* jso_fields = NULL;
    SG_uint32 limit = 0;
    SG_uint32 skip = 0;

    SG_JS_BOOL_CHECK(  argc >= 4  );

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); // rectype
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );

    if (argv[1] && ! JSVAL_IS_NULL(argv[1]))
    {
        SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  ); // field name
        SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_field_name)  );
    }

    SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  ); // keywords
    SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_keywords)  );

    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[3])  ); // fields
    jso_fields = JSVAL_TO_OBJECT(argv[3]);
    SG_JS_BOOL_CHECK(  JS_IsArrayObject(cx, jso_fields)  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso_fields, &pva_fields)  );

    if (argc > 4)
    {
		if (!JSVAL_IS_NULL(argv[4]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[4])  ); // limit
			limit = JSVAL_TO_INT(argv[4]);
		}
    }

    if (argc > 5)
    {
		if (!JSVAL_IS_NULL(argv[5]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[5])  ); // skip
			skip = JSVAL_TO_INT(argv[5]);
		}
    }

    if (argc > 6)
    {
        SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[6])  ); // state
        SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[6]), &psz_specified_state)  );
    }

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

    if (psz_specified_state)
    {
        psz_csid = psz_specified_state;
    }
    else
    {
        SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_leaf)  );
        psz_csid = psz_leaf;
    }

	SUSPEND_REQUEST_ERR_CHECK2(
	    SG_repo__dbndx__query__fts(
                pCtx,
                pz->pRepo,
                pz->iDagNum,
                psz_csid,
                psz_rectype,
                psz_field_name,
                psz_keywords,
                limit,
                skip,
                pva_fields,
                &pva_sliced
                ),
		SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY);

    SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    if (pva_sliced)
    {
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_sliced, jso)  );
        SG_VARRAY_NULLFREE(pCtx, pva_sliced);
    }
    else
    {
    }

    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_NULLFREE(pCtx, psz_leaf);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_field_name);
	SG_NULLFREE(pCtx, psz_keywords);
	SG_NULLFREE(pCtx, psz_specified_state);
    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, psz_leaf);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
	SG_VARRAY_NULLFREE(pCtx, pva_sliced);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_field_name);
	SG_NULLFREE(pCtx, psz_keywords);
	SG_NULLFREE(pCtx, psz_specified_state);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, query__recent)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    char* psz_leaf = NULL;
    char* psz_rectype = NULL;
    char* psz_where = NULL;
    SG_uint32 limit = 0;
    SG_uint32 skip = 0;
    SG_varray* pva_fields = NULL;
    SG_varray* pva_sliced = NULL;
    JSObject* jso_fields = NULL;

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

    SG_JS_BOOL_CHECK(  argc >= 2  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pz->iDagNum) && JSVAL_IS_NULL(argv[0]))
    {
        psz_rectype = NULL;
    }
    else
    {
        SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); // rectype
        SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
    }

    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[1])  ); // fields
    jso_fields = JSVAL_TO_OBJECT(argv[1]);
    SG_JS_BOOL_CHECK(  JS_IsArrayObject(cx, jso_fields)  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso_fields, &pva_fields)  );

    if (argc > 2)
    {
        if (!JSVAL_IS_NULL(argv[2]))
        {
			SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  ); // where
			SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_where)  );
        }
    }

    if (argc > 3)
    {
		if (!JSVAL_IS_NULL(argv[3]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[3])  ); // limit
			limit = JSVAL_TO_INT(argv[3]);
		}
    }

    if (argc > 4)
    {
		if (!JSVAL_IS_NULL(argv[4]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[4])  ); // skip
			skip = JSVAL_TO_INT(argv[4]);
		}
    }

	SUSPEND_REQUEST_ERR_CHECK2(
	    SG_zing__query__recent(
                pCtx,
                pz->pRepo,
                pz->iDagNum,
                psz_rectype,
                psz_where,
                limit,
                skip,
                pva_fields,
                &pva_sliced
                ),
		SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY);

    SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    if (pva_sliced)
    {
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_sliced, jso)  );
        SG_VARRAY_NULLFREE(pCtx, pva_sliced);
    }
    else
    {
    }

    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_NULLFREE(pCtx, psz_leaf);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_where);
    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, psz_leaf);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
	SG_VARRAY_NULLFREE(pCtx, pva_sliced);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_where);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, query_record_history)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    SG_varray* pva_recids = NULL;
    const char* psz_rectype = NULL;
    const char* psz_field = NULL;
    SG_vhash* pvh_raw = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "rectype", &psz_rectype)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "field", &psz_field)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__varray(pCtx, pvh_args, pvh_got, "recids", &pva_recids)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "query_record_history", pvh_args, pvh_got)  );

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

	SUSPEND_REQUEST_ERR_CHECK2(
	    SG_repo__dbndx__query_multiple_record_history(
                pCtx,
                pz->pRepo,
                pz->iDagNum,
                psz_rectype,
                pva_recids,
                psz_field,
                &pvh_raw
                ),
		SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY);

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_raw, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh_raw);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    SG_VHASH_NULLFREE(pCtx, pvh_raw);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, query__raw_history)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    SG_uint32 hours = 0;
    SG_vhash* pvh_raw = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_int64 min_timestamp = -1;
    SG_int64 max_timestamp = -1;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__uint32(pCtx, pvh_args, pvh_got, "hours", &hours)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "query__raw_history", pvh_args, pvh_got)  );

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &max_timestamp)  );
    min_timestamp = max_timestamp - (hours * 60 * 60 * (SG_int64) 1000);

	SUSPEND_REQUEST_ERR_CHECK2(
	    SG_repo__dbndx__query__raw_history(
                pCtx,
                pz->pRepo,
                pz->iDagNum,
                min_timestamp,
                max_timestamp,
                &pvh_raw
                ),
		SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY);

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_raw, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh_raw);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    SG_VHASH_NULLFREE(pCtx, pvh_raw);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, q)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    char* psz_leaf = NULL;
    const char* psz_csid = NULL;
    const char* psz_specified_state = NULL;
    const char* psz_rectype = NULL;
    SG_varray* pva_where = NULL;
    SG_varray* pva_sort = NULL;
    SG_uint32 limit = 0;
    SG_uint32 skip = 0;
    SG_varray* pva_fields = NULL;
    SG_varray* pva_sliced = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_JS_BOOL_CHECK(1 == argc);
    SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[0]) && JSVAL_IS_OBJECT(argv[0])  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "rectype", &psz_rectype)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__varray(pCtx, pvh_args, pvh_got, "fields", &pva_fields)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__varray(pCtx, pvh_args, pvh_got, "where", &pva_where)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__varray(pCtx, pvh_args, pvh_got, "sort", &pva_sort)  );

    SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "limit", 0, &limit)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "skip", 0, &skip)  );
    SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "state", NULL, &psz_specified_state)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "q", pvh_args, pvh_got)  );

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

    if (psz_specified_state)
    {
        psz_csid = psz_specified_state;
    }
    else
    {
        SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_leaf)  );
        psz_csid = psz_leaf;
    }

	SUSPEND_REQUEST_ERR_CHECK2(
	    SG_repo__dbndx__query(
                pCtx,
                pz->pRepo,
                pz->iDagNum,
                psz_csid,
                psz_rectype,
                pva_where,
                pva_sort,
                limit,
                skip,
                pva_fields,
                &pva_sliced
                ),
		SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY);

    SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    if (pva_sliced)
    {
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_sliced, jso)  );
        SG_VARRAY_NULLFREE(pCtx, pva_sliced);
    }
    else
    {
    }

    SG_NULLFREE(pCtx, psz_leaf);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

	SG_NULLFREE(pCtx, psz_leaf);
	SG_VARRAY_NULLFREE(pCtx, pva_sliced);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, query)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    SG_safeptr* psp = NULL;
    JSObject* jso = NULL;
    char* psz_leaf = NULL;
    const char* psz_csid = NULL;
    char *psz_specified_state = NULL;
    char *psz_rectype = NULL;
    char *psz_where = NULL;
    char *psz_sort = NULL;
    SG_uint32 limit = 0;
    SG_uint32 skip = 0;
    SG_varray* pva_fields = NULL;
    SG_varray* pva_sliced = NULL;
    JSObject* jso_fields = NULL;

    psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

    SG_JS_BOOL_CHECK(  argc >= 2  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pz->iDagNum) && JSVAL_IS_NULL(argv[0]))
    {
        psz_rectype = NULL;
    }
    else
    {
        SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  ); // rectype
        SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
    }

    SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[1])  ); // fields
    jso_fields = JSVAL_TO_OBJECT(argv[1]);
    SG_JS_BOOL_CHECK(  JS_IsArrayObject(cx, jso_fields)  );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_varray(pCtx, cx, jso_fields, &pva_fields)  );

    if (argc > 2)
    {
        if (!JSVAL_IS_NULL(argv[2]))
        {
			SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  ); // where
			SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_where)  );
        }
    }

    if (argc > 3)
    {
        if (!JSVAL_IS_NULL(argv[3]))
        {
			SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[3])  ); // sort
			SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[3]), &psz_sort)  );
        }
    }

    if (argc > 4)
    {
		if (!JSVAL_IS_NULL(argv[4]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[4])  ); // limit
			limit = JSVAL_TO_INT(argv[4]);
		}
    }

    if (argc > 5)
    {
		if (!JSVAL_IS_NULL(argv[5]))
		{
			SG_JS_BOOL_CHECK(  JSVAL_IS_INT(argv[5])  ); // skip
			skip = JSVAL_TO_INT(argv[5]);
		}
    }

    if (argc > 6)
    {
        SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[6])  ); // state
        SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[6]), &psz_specified_state)  );
    }

    if (psz_specified_state)
    {
        psz_csid = psz_specified_state;
    }
    else
    {
        SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_leaf)  );
        psz_csid = psz_leaf;
    }

	SUSPEND_REQUEST_ERR_CHECK2(
	    SG_zing__query(
                pCtx,
                pz->pRepo,
                pz->iDagNum,
                psz_csid,
                psz_rectype,
                psz_where,
                psz_sort,
                limit,
                skip,
                pva_fields,
                &pva_sliced
                ),
		SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY);

    SG_JS_NULL_CHECK(  jso = JS_NewArrayObject(cx, 0, NULL)  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    if (pva_sliced)
    {
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_sliced, jso)  );
        SG_VARRAY_NULLFREE(pCtx, pva_sliced);
    }
    else
    {
    }

    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_NULLFREE(pCtx, psz_leaf);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_where);
	SG_NULLFREE(pCtx, psz_sort);
	SG_NULLFREE(pCtx, psz_specified_state);
    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, psz_leaf);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
	SG_VARRAY_NULLFREE(pCtx, pva_sliced);
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_where);
	SG_NULLFREE(pCtx, psz_sort);
	SG_NULLFREE(pCtx, psz_specified_state);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

void sg_zingdb__get_leaf(
    SG_context* pCtx,
	sg_zingdb* pz,
    char** pp
    )
{
    SG_ERR_CHECK_RETURN(  SG_zing__get_leaf(pCtx, pz->pRepo, NULL, pz->iDagNum, pp)  );
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, begin_tx)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pz = NULL;
    sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	JSObject* jso = NULL;
	SG_zingtx* pZingTx = NULL;
    char* psz_hid_cs0 = NULL;
    char* psz_db_state_id = NULL;
    char* psz_specified_userid = NULL;
    const char* psz_userid = NULL;
    SG_audit q;

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pz)  );

	SG_JS_BOOL_CHECK(  (0 == argc) || (1 == argc) || (2 == argc) );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_zingtx_class, NULL, NULL)  );

    if (0 == argc)
    {
        SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_db_state_id)  );
    }
    else
    {
        if (JSVAL_IS_NULL(argv[0]))
        {
            SG_ERR_CHECK(  sg_zingdb__get_leaf(pCtx, pz, &psz_db_state_id)  );
        }
        else
        {
            SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
            SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_db_state_id)  );
        }

        if ((argc > 1) && ! JSVAL_IS_NULL(argv[1]))
        {
            SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
            SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_specified_userid)  );
        }
    }

    if (psz_specified_userid)
    {
        psz_userid = psz_specified_userid;
    }
    else
    {
        SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pz->pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
        psz_userid = q.who_szUserId;
    }

	SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pz->pRepo, pz->iDagNum, psz_userid, psz_db_state_id, &pZingTx)  );
    if (psz_db_state_id)
    {
        SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pZingTx, psz_db_state_id)  );
    }
    SG_NULLFREE(pCtx, psz_db_state_id);
    SG_ERR_CHECK(  sg_statetxcombo__alloc(pCtx, pz, pZingTx, &pcombo)  );
	SG_ERR_CHECK(  SG_safeptr__wrap__statetxcombo(pCtx, pcombo, &psp)  );
    JS_SetPrivate(cx, jso, psp);

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_NULLFREE(pCtx, psz_hid_cs0);
	SG_NULLFREE(pCtx, psz_specified_userid);
	return JS_TRUE;

fail:
	SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pZingTx)  );
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    SG_NULLFREE(pCtx, psz_db_state_id);
	SG_NULLFREE(pCtx, psz_specified_userid);
	return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingdb, merge)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	sg_zingdb* pzstate = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char* psz_hid_cs0 = NULL;
    SG_vhash* pvh = NULL;
    JSObject* jso = NULL;
    SG_audit q;
    char *psz_user = NULL;
    char *psz_leaf_0 = NULL;
    char *psz_leaf_1 = NULL;
    SG_dagnode* pdn_merged = NULL;

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__zingdb(pCtx, psp, &pzstate)  );

	SG_JS_BOOL_CHECK(  (0 == argc) || // user from settings, auto changesets
					   (1 == argc) || // explicit user, auto changesets
					   (3 == argc)    // explicit user (or null to get from settings), explicit changesets
					);

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
    // TODO the result object no longer needs to be a vhash since it no
    // longer has errors/log in it.  It's just a csid.

	if ((argc == 0) || JSVAL_IS_NULL(argv[0]))
		SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pzstate->pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
	else
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_user)  );
		SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pzstate->pRepo, SG_AUDIT__WHEN__NOW, psz_user)  );
	}

    if (3 == argc)
    {
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_leaf_0)  );
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[2])  );
    	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_leaf_1)  );

        SG_ERR_CHECK(  SG_zing__automerge(pCtx,
                    pzstate->pRepo,
                    pzstate->iDagNum,
                    &q,
                    psz_leaf_0,
                    psz_leaf_1,
                    &pdn_merged
                    )  );

        if (pdn_merged)
        {
            const char* psz_hid = NULL;

            SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pdn_merged, &psz_hid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "csid", psz_hid)  );
            SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
        }
    }
    else
    {
        SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pzstate->pRepo, &q, pzstate->iDagNum, &psz_hid_cs0)  );
        if (psz_hid_cs0)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "csid", psz_hid_cs0)  );
            SG_NULLFREE(pCtx, psz_hid_cs0);
        }
    }

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, psz_user);
	SG_NULLFREE(pCtx, psz_leaf_0);
	SG_NULLFREE(pCtx, psz_leaf_1);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
    SG_NULLFREE(pCtx, psz_hid_cs0);
	SG_NULLFREE(pCtx, psz_user);
	SG_NULLFREE(pCtx, psz_leaf_0);
	SG_NULLFREE(pCtx, psz_leaf_1);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

static void null_pvec_records(
        SG_context* pCtx,
        SG_vector* pvec
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vector__length(pCtx, pvec, &count)  );
    for (i=0; i<count; i++)
    {
        SG_safeptr* psp = NULL;

        SG_ERR_CHECK(  SG_vector__get(pCtx, pvec, i, (void**) &psp)  );

        SG_ERR_CHECK(  SG_safeptr__null(pCtx, psp)  );
    }

fail:
    ;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, abort)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
    sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = NULL;
	
	psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_NULLARGCHECK(psp);

	SG_UNUSED(argc);

	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );
	SG_ERR_CHECK(  SG_zing__abort_tx(pCtx, &pcombo->pztx)  );
    SG_ERR_CHECK(  null_pvec_records(pCtx, pcombo->pvec_records)  );
    SG_VECTOR_NULLFREE(pCtx, pcombo->pvec_records);
    SG_NULLFREE(pCtx, pcombo);
	SG_SAFEPTR_NULLFREE(pCtx, psp);
    JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);

	return JS_TRUE;

fail:
	SG_SAFEPTR_NULLFREE(pCtx, psp);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, commit)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
	SG_changeset* pcs = NULL;
	SG_dagnode* pdn = NULL;
	const char* psz_hid_cs0 = NULL;
    SG_varray* pva_violations = NULL;
    SG_vhash* pvh = NULL;
    JSObject* jso = NULL;
    SG_int64 when = -1;
    char *tmp = NULL;

    SG_UNUSED(argc);

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );

	if (argc == 1)
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
		SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &tmp)  );
		SG_ERR_CHECK(  SG_int64__parse(pCtx, &when, tmp)  );
		SG_NULLFREE(pCtx, tmp);
	}
	else
	{
		// TODO it would probably be nifty to allow this timestamp to be set in JS code
		SG_time__get_milliseconds_since_1970_utc(pCtx, &when);
	}

	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, when, &pcombo->pztx, &pcs, &pdn, &pva_violations)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, SG_AUDIT__TIMESTAMP, when)  );

    if (pva_violations)
    {
        SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh, "errors", &pva_violations)  );
    }

    if (pdn)
	{
        SG_int32 generation = -1;

		SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pdn, &psz_hid_cs0)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "csid", psz_hid_cs0)  );

		SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &generation)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "generation", (SG_int64) generation)  );
	}

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvh);

    if (pdn)
    {
        SG_ERR_CHECK(  null_pvec_records(pCtx, pcombo->pvec_records)  );
        SG_VECTOR_NULLFREE(pCtx, pcombo->pvec_records);
        SG_NULLFREE(pCtx, pcombo);
        SG_SAFEPTR_NULLFREE(pCtx, psp);
        JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);
    }

	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_CHANGESET_NULLFREE(pCtx, pcs);

	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VARRAY_NULLFREE(pCtx, pva_violations);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_CHANGESET_NULLFREE(pCtx, pcs);
	SG_NULLFREE(pCtx, tmp);

	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, new_record)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_rectype = NULL;
	char *psz_name = NULL;
    sg_statetxcombo* pcombo = NULL;
	JSObject* jso = NULL;
	JSObject* jso_init = NULL;
	SG_zingrecord* pRec = NULL;
	SG_safeptr* psrec = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    JSIdArray* properties = NULL;
    SG_zingfieldattributes* pzfa = NULL;

	SG_NULLARGCHECK(psp);

	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );

	SG_JS_BOOL_CHECK(  (1 == argc) || (2 == argc)  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_zingrecord_class, NULL, NULL)  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pcombo->pztx, psz_rectype, &pRec)  );

    if (2 == argc)
    {
        SG_int32 i = 0;

        SG_JS_BOOL_CHECK(  !JSVAL_IS_NULL(argv[1]) && JSVAL_IS_OBJECT(argv[1])  );
        jso_init = JSVAL_TO_OBJECT(argv[1]);

        SG_JS_NULL_CHECK(  properties = JS_Enumerate(cx, jso_init)  );

        for (i=0; i<properties->length; i++)
        {
            jsval jv_name;
            jsval jv_value;
            JSString* str_name = NULL;

            SG_JS_BOOL_CHECK(  JS_IdToValue(cx, properties->vector[i], &jv_name)  );
            SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(jv_name)  );

            str_name = JSVAL_TO_STRING(jv_name);
            SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, str_name, &psz_name)  );

            SG_JS_BOOL_CHECK(  JS_GetProperty(cx, jso_init, psz_name, &jv_value)  );

            SG_ERR_CHECK(  SG_zingrecord__lookup_name(pCtx, pRec, psz_name, &pzfa)  );

            if (pzfa)
            {
                SG_ERR_CHECK(  sg_zing_jsglue__set_field(cx, pCtx, pRec, pzfa, psz_name, &jv_value)  );
            }
            else
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            SG_NULLFREE(pCtx, psz_name);
        }

        JS_DestroyIdArray(cx, properties);
    }

	SG_ERR_CHECK(  SG_safeptr__wrap__zingrecord(pCtx, pRec, &psrec)  );
    SG_ERR_CHECK(  SG_vector__append(pCtx, pcombo->pvec_records, psrec, NULL)  );
	JS_SetPrivate(cx, jso, psrec);

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_NULLFREE(pCtx, psz_rectype);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_name);
	SG_NULLFREE(pCtx, psz_rectype);
	return JS_FALSE;

}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, open_record)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    sg_statetxcombo* pcombo = NULL;
	JSObject* jso = NULL;
	SG_zingrecord* pRec = NULL;
	SG_safeptr* psrec = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char *psz_rectype = NULL;
    char *psz_recid = NULL;

	SG_NULLARGCHECK(psp);
	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );

    if (SG_DAGNUM__HAS_NO_RECID(pcombo->pzstate->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_recid)  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_zingrecord_class, NULL, NULL)  );

	SG_ERR_CHECK(  SG_zingtx__get_record(pCtx, pcombo->pztx, psz_rectype, psz_recid, &pRec)  );
	SG_ERR_CHECK(  SG_safeptr__wrap__zingrecord(pCtx, pRec, &psrec)  );
    SG_ERR_CHECK(  SG_vector__append(pCtx, pcombo->pvec_records, psrec, NULL)  );
	JS_SetPrivate(cx, jso, psrec);
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
	return JS_FALSE;

}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, delete_record)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char *psz_rectype = NULL;
    char *psz_recid = NULL;

	SG_NULLARGCHECK(psp);
	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_recid)  );

    if (SG_DAGNUM__HAS_NO_RECID(pcombo->pzstate->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

	SG_ERR_CHECK(  SG_zingtx__delete_record(pCtx, pcombo->pztx, psz_rectype, psz_recid)  );
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_recid);
	return JS_FALSE;

}

SG_ZING_JSGLUE_METHOD_PROTOTYPE(zingtx, delete_record__hid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    sg_statetxcombo* pcombo = NULL;
	SG_safeptr* psp = sg_zing_jsglue__get_object_private(cx, JS_THIS_OBJECT(cx, vp));
    char *psz_rectype = NULL;
    char *psz_hid = NULL;

	SG_NULLARGCHECK(psp);
	SG_ERR_CHECK(  SG_safeptr__unwrap__statetxcombo(pCtx, psp, &pcombo)  );

	SG_JS_BOOL_CHECK(  2 == argc  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[0])  );
	SG_JS_BOOL_CHECK(  JSVAL_IS_STRING(argv[1])  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_rectype)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_hid)  );


	SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pcombo->pztx, psz_rectype, psz_hid)  );
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_hid);
	return JS_TRUE;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, psz_rectype);
	SG_NULLFREE(pCtx, psz_hid);
	return JS_FALSE;

}

/* ---------------------------------------------------------------- */
/* Now the method tables */
/* ---------------------------------------------------------------- */

static JSPropertySpec sg_zingtx_properties[] =
{
    {NULL,0,0,NULL,NULL}
};

static JSFunctionSpec sg_zingdb_methods[] =
{
    {"begin_tx", 			SG_ZING_JSGLUE_METHOD_NAME(zingdb,begin_tx),0,0},
    {"get_template", 		SG_ZING_JSGLUE_METHOD_NAME(zingdb,get_template),0,0},
    {"get_leaves", 		    SG_ZING_JSGLUE_METHOD_NAME(zingdb,get_leaves),0,0},
    {"get_record", 			SG_ZING_JSGLUE_METHOD_NAME(zingdb,get_record),3,0},
    {"get_record_all_versions", 			SG_ZING_JSGLUE_METHOD_NAME(zingdb,get_record_all_versions),1,0},
    {"q", 			    SG_ZING_JSGLUE_METHOD_NAME(zingdb,q),1,0},
    {"query", 			    SG_ZING_JSGLUE_METHOD_NAME(zingdb,query),1,0},
    {"query__recent", 			    SG_ZING_JSGLUE_METHOD_NAME(zingdb,query__recent),1,0},
    {"query__raw_history", 		    SG_ZING_JSGLUE_METHOD_NAME(zingdb,query__raw_history),1,0},
    {"query__fts", 			    SG_ZING_JSGLUE_METHOD_NAME(zingdb,query__fts),1,0},
    {"get_history", 	    SG_ZING_JSGLUE_METHOD_NAME(zingdb,get_history),1,0},
    {"query_record_history", 	    SG_ZING_JSGLUE_METHOD_NAME(zingdb,query_record_history),1,0},
    {"merge", 			    SG_ZING_JSGLUE_METHOD_NAME(zingdb,merge),3,0},
    {NULL,NULL,0,0}
};

static JSFunctionSpec sg_zingtx_methods[] =
{
    {"abort", 			  	SG_ZING_JSGLUE_METHOD_NAME(zingtx,abort),0,0},
    {"commit", 			  	SG_ZING_JSGLUE_METHOD_NAME(zingtx,commit),0,0},
    {"new_record", 	        SG_ZING_JSGLUE_METHOD_NAME(zingtx,new_record),1,0},
    {"open_record", 		SG_ZING_JSGLUE_METHOD_NAME(zingtx,open_record),1,0},
    {"delete_record", 		SG_ZING_JSGLUE_METHOD_NAME(zingtx,delete_record),2,0},
    {"delete_record__hid", 		SG_ZING_JSGLUE_METHOD_NAME(zingtx,delete_record__hid),1,0},
    {"get_template", 		SG_ZING_JSGLUE_METHOD_NAME(zingtx,get_template),0,0},
    {"set_template", 		SG_ZING_JSGLUE_METHOD_NAME(zingtx,set_template),1,0},
    {NULL,NULL,0,0}
};

/* ---------------------------------------------------------------- */
/* This is the function which sets everything up */
/* ---------------------------------------------------------------- */

void SG_zing_jsglue__install_scripting_api(SG_context * pCtx,
									  JSContext *cx, JSObject *glob)
{
	SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_zingtx_class,
            NULL,
            0, /* nargs */
            sg_zingtx_properties,
            sg_zingtx_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );
	SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_zingrecord_class,
	    	NULL,
            0, /* nargs */
            NULL,
            NULL,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );
	SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_zingdb_class,
	    	zingdb_constructor,
            0, /* nargs */
            NULL,
            sg_zingdb_methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );
	return;
fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return;
}

