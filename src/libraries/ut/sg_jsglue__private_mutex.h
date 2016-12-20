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
 * @file sg_jsglue__private_mutex
 *
 * @details SG_mutex functionality in JS
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_JSGLUE__PRIVATE__MUTEX_H
#define H_SG_JSGLUE__PRIVATE__MUTEX_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * sg.mutex.lock(name)
 *
 * No return value.
 */
#define LOCK_USAGE "Usage: sg.mutex.lock(name)"
SG_JSGLUE_METHOD_PROTOTYPE(mutex, lock)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszName = NULL;

	if (argc != 1)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " LOCK_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "name must be a string.  " LOCK_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );

	/* It's very important to suspend the JS context here. Without it, there will be blood. Er, deadlocks. */
	SUSPEND_REQUEST_ERR_CHECK2(  SG_jscore__mutex__lock(pCtx, pszName), SG_HTTPREQUESTPROFILER_CATEGORY__JSGLUE_MUTEX_LOCK  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	SG_NULLFREE(pCtx, pszName);
	return JS_FALSE;
}
#undef LOCK_USAGE

/**
 * sg.mutex.unlock(name)
 *
 * No return value.
 */
#define UNLOCK_USAGE "Usage: sg.mutex.unlock(name)"
SG_JSGLUE_METHOD_PROTOTYPE(mutex, unlock)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszName = NULL;

	if (argc != 1)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " UNLOCK_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "name must be a string.  " UNLOCK_USAGE)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszName)  );
	
	SG_ERR_CHECK(  SG_jscore__mutex__unlock(pCtx, pszName)  );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, pszName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // Don't SG_ERR_IGNORE.
	SG_NULLFREE(pCtx, pszName);
	return JS_FALSE;
}
#undef UNLOCK_USAGE


//////////////////////////////////////////////////////////////////

static JSClass sg_mutex__class = {
    "sg_mutex",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec sg_mutex__methods[] = {
	{ "lock",	SG_JSGLUE_METHOD_NAME(mutex,	lock),		1,0 },
	{ "unlock",	SG_JSGLUE_METHOD_NAME(mutex,	unlock),	1,0 },
	JS_FS_END
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif // H_SG_JSGLUE__PRIVATE__MUTEX_H
