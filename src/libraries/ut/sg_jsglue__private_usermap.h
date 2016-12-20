/*
Copyright 2012-2013 SourceGear, LLC

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
 * @file sg_jsglue__usermap
 *
 * @details Routines exposing SG_usermap to JS.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_JSGLUE__PRIVATE__USERMAP_H
#define H_SG_JSGLUE__PRIVATE__USERMAP_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * sg.usermap.get_all(descriptor_name)
 *
 * Returns all existing user map records for the repository matching the provided descriptor_name.
 */
#define GET_ALL_USAGE "Usage: sg.usermap.get_all(descriptor_name)"
SG_JSGLUE_METHOD_PROTOTYPE(usermap, get_all)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszDescriptorName = NULL;
	SG_vhash* pvhReturn = NULL;
	JSObject* jso = NULL;

	if (argc != 1)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " GET_ALL_USAGE)  );
	
	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "descriptor_name must be a string.  " GET_ALL_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszDescriptorName)  );

	SG_ERR_CHECK(  SG_usermap__users__get_all(pCtx, pszDescriptorName, &pvhReturn)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhReturn, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvhReturn);
	SG_NULLFREE(pCtx, pszDescriptorName);
	return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhReturn);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}
#undef GET_ALL_USAGE

#define GET_ONE_USAGE "Usage: sg.usermap.get_one(descriptor_name, src_recid)"
SG_JSGLUE_METHOD_PROTOTYPE(usermap, get_one)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszDescriptorName = NULL;
	char *pszSrcRecid = NULL;
	char* pszDestRecid = NULL;
	JSString* pjs = NULL;

	if (argc != 2)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " GET_ONE_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "descriptor_name must be a string.  " GET_ONE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszDescriptorName)  );

	if ( JSVAL_IS_NULL(argv[1]) || !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "src_recid must be a string.  " GET_ONE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszSrcRecid)  );

	SG_ERR_CHECK(  SG_usermap__users__get_one(pCtx, pszDescriptorName, pszSrcRecid, &pszDestRecid)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszDestRecid))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	SG_NULLFREE(pCtx, pszDestRecid);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRecid);
	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszDestRecid);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRecid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}
#undef GET_ONE_USAGE

#define ADD_UPDATE_USAGE "Usage: sg.usermap.add_or_update(descriptor_name, src_recid, dest_recid)"
SG_JSGLUE_METHOD_PROTOTYPE(usermap, add_or_update)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszDescriptorName = NULL;
	char *pszSrcRecid = NULL;
	char *pszDestRecid = NULL;

	if (argc != 3)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 3 arguments.  " ADD_UPDATE_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "descriptor_name must be a string.  " ADD_UPDATE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszDescriptorName)  );

	if ( JSVAL_IS_NULL(argv[1]) || !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "src_recid must be a string.  " ADD_UPDATE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszSrcRecid)  );

	if ( JSVAL_IS_NULL(argv[2]) || !JSVAL_IS_STRING(argv[2]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "dest_recid must be a string.  " ADD_UPDATE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &pszDestRecid)  );

    if (0 == strcmp(pszDestRecid, "NEW"))
    {
        // fine
    }
    else
    {
        SG_bool bValid = SG_FALSE;

        SG_ERR_CHECK(  SG_gid__verify_format(pCtx, pszDestRecid, &bValid)  );

        if (!bValid)
        {
            SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "dest_recid must be a userid or NEW.  " ADD_UPDATE_USAGE)  );
        }
    }

	SG_ERR_CHECK(  SG_usermap__users__add_or_update(pCtx, pszDescriptorName, pszSrcRecid, pszDestRecid)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRecid);
	SG_NULLFREE(pCtx, pszDestRecid);
	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRecid);
	SG_NULLFREE(pCtx, pszDestRecid);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}
#undef ADD_UPDATE_USAGE

#define REMOVE_ONE_USAGE "Usage: sg.usermap.remove_one(descriptor_name, src_recid)"
SG_JSGLUE_METHOD_PROTOTYPE(usermap, remove_one)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszDescriptorName = NULL;
	char *pszSrcRecid = NULL;

	if (argc != 2)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " REMOVE_ONE_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "descriptor_name must be a string.  " REMOVE_ONE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszDescriptorName)  );

	if ( JSVAL_IS_NULL(argv[1]) || !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "src_recid must be a string.  " REMOVE_ONE_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &pszSrcRecid)  );

	SG_ERR_CHECK(  SG_usermap__users__remove_one(pCtx, pszDescriptorName, pszSrcRecid)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRecid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszSrcRecid);
	return JS_FALSE;
}
#undef REMOVE_ONE_USAGE

#define REMOVE_ALL_USAGE "Usage: sg.usermap.remove_all(descriptor_name)"
SG_JSGLUE_METHOD_PROTOTYPE(usermap, remove_all)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszDescriptorName = NULL;

	if (argc != 1)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " REMOVE_ALL_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "descriptor_name must be a string.  " REMOVE_ALL_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszDescriptorName)  );

	SG_ERR_CHECK(  SG_usermap__users__remove_all(pCtx, pszDescriptorName)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszDescriptorName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszDescriptorName);
	return JS_FALSE;
}
#undef REMOVE_ALL_USAGE

//////////////////////////////////////////////////////////////////

static JSClass sg_usermap__class = {
    "sg_sync_remote",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec sg_usermap__methods[] = {
	{ "get_all",			SG_JSGLUE_METHOD_NAME(usermap, get_all),			1,0 },
	{ "get_one",			SG_JSGLUE_METHOD_NAME(usermap, get_one),			2,0 },
	{ "add_or_update",		SG_JSGLUE_METHOD_NAME(usermap, add_or_update),		3,0 },
	{ "remove_one",			SG_JSGLUE_METHOD_NAME(usermap, remove_one),			2,0 },
	{ "remove_all",			SG_JSGLUE_METHOD_NAME(usermap, remove_all),			1,0 },
	JS_FS_END
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif // H_SG_JSGLUE__PRIVATE__USERMAP_H
