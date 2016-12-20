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
 * @file sg_jsglue__private_sync_remote
 *
 * @details Routines exposing SG_sync_remote to JS.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_JSGLUE__PRIVATE__SYNC_REMOTE_H
#define H_SG_JSGLUE__PRIVATE__SYNC_REMOTE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * sg.sync_remote.get_repo_info(repo)
 *
 * Returns repository information useful for push and pull.
 * If include_branches_and_locks is specified and true, branch and lock information is included.
 */
#define GET_REPO_INFO_USAGE "Usage: sg.sync_remote.get_repo_info(repo, [include_branches_and_locks], [include_areas])"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, get_repo_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvh = NULL;
	JSObject* jso = NULL;
	SG_bool bIncludeBranchesAndLocks = SG_FALSE;
    SG_bool b_include_areas = SG_FALSE;

	if ((argc != 1) && (argc != 2) && (argc != 3))
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 1 or 2 or 3 arguments.  " GET_REPO_INFO_USAGE)  );
	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " GET_REPO_INFO_USAGE)  );
	if ( argc > 1 )
	{
		if ( !JSVAL_IS_BOOLEAN(argv[1]) )
			SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "include_branches_and_locks must be a bool.  " GET_REPO_INFO_USAGE)  );
		bIncludeBranchesAndLocks = JSVAL_TO_BOOLEAN(argv[1]);
	}
	if ( argc > 2 )
	{
		if ( !JSVAL_IS_BOOLEAN(argv[2]) )
			SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "include_areas must be a bool.  " GET_REPO_INFO_USAGE)  );
		b_include_areas = JSVAL_TO_BOOLEAN(argv[2]);
	}

	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );
	SG_ERR_CHECK(  SG_sync_remote__get_repo_info(pCtx, pRepo, bIncludeBranchesAndLocks, b_include_areas, &pvh)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}
#undef GET_REPO_INFO_USAGE

/**
 * sg.sync_remote.push_begin()
 *
 * Returns the (string) push id.
 */
#define PUSH_BEGIN_USAGE "Usage: sg.sync_remote.push_begin()"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_begin)
{

	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	const char* pszPushId = NULL;
	JSString* pjs = NULL;

	if (argc != 0)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected no arguments.  " PUSH_BEGIN_USAGE)  );
	
	SG_ERR_CHECK(  SG_sync_remote__push_begin(pCtx, &pszPushId)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszPushId))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	SG_NULLFREE(pCtx, pszPushId);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // Don't SG_ERR_IGNORE.
	SG_NULLFREE(pCtx, pszPushId);
	return JS_FALSE;
}
#undef PUSH_BEGIN_USAGE

/**
 * sg.sync_remote.request_fragball(repo, create_in_dir, [fragball_spec_obj])
 *
 * Returns the (string) name of the fragball file created.
 */
#define REQUEST_FRAGBALL_USAGE "Usage: sg.sync_remote.request_fragball(repo, create_in_dir, [fragball_spec_obj])"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, request_fragball)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;
	SG_pathname* pPathname = NULL;
	SG_vhash* pvhFragballSpec = NULL;
	char* pszFileName = NULL;	
	JSString* pjs = NULL;

	if (argc < 2 || argc > 3)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 2 or 3 arguments.  " REQUEST_FRAGBALL_USAGE)  );
	
	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " REQUEST_FRAGBALL_USAGE)  );
	else
	{
		pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
		SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );
	}

	if ( !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "create_in_dir must be a string.  " REQUEST_FRAGBALL_USAGE)  );
	else
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__JSSTRING(pCtx, &pPathname, cx, JSVAL_TO_STRING(argv[1]))  );

	if ( argc == 3 && !JSVAL_IS_VOID(argv[2]) && !JSVAL_IS_NULL(argv[2]) && !JSVAL_IS_OBJECT(argv[2]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "fragball_spec_obj must be an object.  " REQUEST_FRAGBALL_USAGE)  );
	else if ( !JSVAL_IS_NULL(argv[2]) && !JSVAL_IS_VOID(argv[2]))
	{
		SG_JS_BOOL_CHECK(  JSVAL_IS_OBJECT(argv[2])  );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[2]), &pvhFragballSpec)  );
	}

	SUSPEND_REQUEST_ERR_CHECK(  SG_sync_remote__request_fragball(pCtx, pRepo, pPathname, pvhFragballSpec, &pszFileName)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszFileName))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	
	SG_NULLFREE(pCtx, pszFileName);
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_VHASH_NULLFREE(pCtx, pvhFragballSpec);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // Don't SG_ERR_IGNORE.
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_VHASH_NULLFREE(pCtx, pvhFragballSpec);
	SG_NULLFREE(pCtx, pszFileName);
	return JS_FALSE;
}
#undef REQUEST_FRAGBALL_USAGE

/**
 * sg.sync_remote.push_add(repo, push_id, fragball_path)
 *
 * Returns a status object, the same as if push_status had been called immediately after push_add.
 */
#define PUSH_ADD_USAGE "Usage: sg.sync_remote.push_add(repo, push_id, fragball_path)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_add)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_pushid = NULL;
	char *psz_fragball_path = NULL;
	SG_pathname* pFragballPath = NULL;

	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;

	SG_pathname * pStagingPath = NULL;
	SG_pathname* pFragballPath2 = NULL;
	char fragballFilename2[SG_TID_MAX_BUFFER_LENGTH];

	SG_vhash* pvhAddResult = NULL;
	JSObject* jso = NULL;

	if (argc != 3)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 3 arguments.  " PUSH_ADD_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " PUSH_ADD_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "push_id must be a string.  " PUSH_ADD_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[2]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "fragball_path must be a string.  " PUSH_ADD_USAGE)  );

	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_pushid)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_fragball_path)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pFragballPath, psz_fragball_path)  );

	SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, psz_pushid, &pStagingPath)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, fragballFilename2, sizeof(fragballFilename2))  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pFragballPath2, pStagingPath, fragballFilename2)  );

	SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pFragballPath, pFragballPath2)  );

	SG_PATHNAME_NULLFREE(pCtx, pStagingPath);
	SG_PATHNAME_NULLFREE(pCtx, pFragballPath);
	SG_PATHNAME_NULLFREE(pCtx, pFragballPath2);

	SUSPEND_REQUEST_ERR_CHECK(  SG_sync_remote__push_add(pCtx, psz_pushid, pRepo, fragballFilename2, &pvhAddResult)  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, NULL, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhAddResult, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhAddResult);
	SG_NULLFREE(pCtx, psz_pushid);
	SG_NULLFREE(pCtx, psz_fragball_path);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_PATHNAME_NULLFREE(pCtx, pStagingPath);
	SG_PATHNAME_NULLFREE(pCtx, pFragballPath);
	SG_PATHNAME_NULLFREE(pCtx, pFragballPath2);
	SG_VHASH_NULLFREE(pCtx, pvhAddResult);
	SG_NULLFREE(pCtx, psz_pushid);
	SG_NULLFREE(pCtx, psz_fragball_path);
	return JS_FALSE;
}
#undef PUSH_ADD_USAGE

/**
 * sg.sync_remote.push_commit(repo, push_id)
 *
 * Returns a status object, the same as if push_status had been called immediately after push_commit.
 */
#define PUSH_COMMIT_USAGE "Usage: sg.sync_remote.push_commit(repo, push_id)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_commit)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_pushid = NULL;

	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;

	SG_vhash* pvhCommitResult = NULL;
	JSObject* jso = NULL;

	if (argc != 2)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " PUSH_COMMIT_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " PUSH_COMMIT_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "push_id must be a string.  " PUSH_COMMIT_USAGE)  );

	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_pushid)  );

	SUSPEND_REQUEST_ERR_CHECK(  SG_sync_remote__push_commit(pCtx, pRepo, psz_pushid, &pvhCommitResult)  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, NULL, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhCommitResult, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhCommitResult);
	SG_NULLFREE(pCtx, psz_pushid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_VHASH_NULLFREE(pCtx, pvhCommitResult);
	SG_NULLFREE(pCtx, psz_pushid);
	return JS_FALSE;
}
#undef PUSH_COMMIT_USAGE

/**
 * sg.sync_remote.push_end(push_id)
 *
 * No return value.
 */
#define PUSH_END_USAGE "Usage: sg.sync_remote.push_end(push_id)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_end)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_pushid = NULL;

	if (argc != 1)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " PUSH_END_USAGE)  );

	if ( !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "push_id must be a string.  " PUSH_END_USAGE)  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_pushid)  );

	SG_ERR_CHECK(  SG_sync_remote__push_end(pCtx, psz_pushid) );

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	SG_NULLFREE(pCtx, psz_pushid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_NULLFREE(pCtx, psz_pushid);
	return JS_FALSE;
}
#undef PUSH_END_USAGE

/**
 * sg.sync_remote.get_dagnode_info(repo, request_obj)
 *
 * Returns a history result object (see SG_history_result__to_json for format).
 */
#define GET_DAGNODE_INFO_USAGE "Usage: sg.sync_remote.get_dagnode_info(repo, request_obj)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, get_dagnode_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);

	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvhRequest = NULL;

	SG_history_result* pHistResult = NULL;
	JSObject* jso = NULL;

	SG_varray* pvaRef = NULL;
	SG_uint32 len = 0;

	if (argc != 2)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " GET_DAGNODE_INFO_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " GET_DAGNODE_INFO_USAGE)  );
	if ( !JSVAL_IS_OBJECT(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "request_obj must be an object.  " GET_DAGNODE_INFO_USAGE)  );

	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[1]), &pvhRequest)  );

	SG_JS_SUSPENDREQUEST();
	REQUEST_SUSPENDED_ERR_CHECK(  SG_sync_remote__get_dagnode_info(pCtx, pRepo, pvhRequest, &pHistResult)  );
	REQUEST_SUSPENDED_ERR_CHECK(  SG_history_result__get_root(pCtx, pHistResult, &pvaRef)  );
	REQUEST_SUSPENDED_ERR_CHECK(  SG_history_result__count(pCtx, pHistResult, &len)  );
	SG_JS_RESUMEREQUEST();

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, len, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaRef, jso)  );

	SG_VHASH_NULLFREE(pCtx, pvhRequest);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);

	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_VHASH_NULLFREE(pCtx, pvhRequest);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistResult);
	
	return JS_FALSE;
}
#undef GET_DAGNODE_INFO_USAGE

/**
 * sg.sync_remote.check_status(repo, push_id)
 *
 * Returns an object describing the status of the pending push identified by push_id.
 */
#define CHECK_STATUS_USAGE "Usage: sg.sync_remote.check_status(repo, push_id)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, check_status)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_pushid = NULL;
	SG_staging* pStaging = NULL;
	SG_vhash* pvhStatus = NULL;
	JSObject* jso = NULL;
	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;

	if (argc != 2)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " CHECK_STATUS_USAGE)  );

	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " CHECK_STATUS_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "push_id must be a string.  " CHECK_STATUS_USAGE)  );

	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_pushid)  );

	SG_ERR_CHECK(  SG_staging__open(pCtx, psz_pushid, &pStaging)  );
	SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pRepo,
		SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, 
		&pvhStatus)  );

	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, NULL, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhStatus, jso)  );

	SG_STAGING_NULLFREE(pCtx, pStaging);
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
	SG_NULLFREE(pCtx, psz_pushid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_STAGING_NULLFREE(pCtx, pStaging);
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
	SG_NULLFREE(pCtx, psz_pushid);
	return JS_FALSE;
}
#undef CHECK_STATUS_USAGE

/**
 * sg.sync_remote.heartbeat(repo)
 *
 * Returns repository basic information: repo ID, admin ID, and hash method.
 * Intended to be used to indicate a repository's availability
 */
#define HEARTBEAT_USAGE "Usage: sg.sync_remote.heartbeat(repo)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, heartbeat)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_safeptr* pspRepo = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvh = NULL;
	JSObject* jso = NULL;

	if (argc != 1)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " HEARTBEAT_USAGE)  );
	if ( JSVAL_IS_NULL(argv[0]) || !JSVAL_IS_OBJECT(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo must be a repository object.  " HEARTBEAT_USAGE)  );

	pspRepo = sg_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(argv[0]));
	SG_ERR_CHECK(  SG_safeptr__unwrap__repo(pCtx, pspRepo, &pRepo)  );

	SG_ERR_CHECK(  SG_sync_remote__heartbeat(pCtx, pRepo, &pvh)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return JS_FALSE;
}
#undef HEARTBEAT_USAGE

static void _throw_if_clone_disallowed(SG_context* pCtx)
{
	SG_bool bAllowed = SG_FALSE;
	SG_ERR_CHECK_RETURN(  sg_uridispatch__repo_addremove_allowed(pCtx, &bAllowed)  );
	if (!bAllowed)
		SG_ERR_THROW_RETURN(SG_ERR_SERVER_DISALLOWED_REPO_CREATE_OR_DELETE);
}

/**
 * sg.sync_remote.push_clone_begin(repo_name, repo_info_obj)
 *
 * Returns the (string) clone id.
 */
#define PUSH_CLONE_BEGIN_USAGE "Usage: sg.sync_remote.push_clone_begin(repo_name, repo_info_obj)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_clone_begin)
{

	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *pszRepoName = NULL;
	SG_vhash* pvhRepoInfo = NULL;
	const char* pszCloneId = NULL;
	JSString* pjs = NULL;

	if (argc != 2)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " PUSH_CLONE_BEGIN_USAGE)  );
	
	if (! JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo_name must be a string.  " PUSH_CLONE_BEGIN_USAGE)  );
	if (! JSVAL_IS_OBJECT(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "repo_info_obj must be an object.  " PUSH_CLONE_BEGIN_USAGE)  );

	SG_ERR_CHECK(  _throw_if_clone_disallowed(pCtx)  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &pszRepoName)  );

	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[1]), &pvhRepoInfo)  );

	SG_ERR_CHECK(  SG_sync_remote__push_clone__begin(pCtx, pszRepoName, pvhRepoInfo, &pszCloneId)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszCloneId))  );
	
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
	SG_NULLFREE(pCtx, pszCloneId);

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));
	SG_NULLFREE(pCtx, pszRepoName);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx,cx); // Don't SG_ERR_IGNORE.
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
	SG_NULLFREE(pCtx, pszCloneId);
	SG_NULLFREE(pCtx, pszRepoName);
	return JS_FALSE;
}
#undef PUSH_CLONE_BEGIN_USAGE

/**
 * sg.sync_remote.push_clone_commit(clone_id, fragball_path)
 *
 * No return value.
 */
#define PUSH_CLONE_COMMIT_USAGE "Usage: sg.sync_remote.push_clone_commit(clone_id, fragball_path)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_clone_commit)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_cloneid = NULL;
	char *psz_fragball_path = NULL;
	SG_pathname* pFragballPath = NULL;

	JS_SET_RVAL(cx, vp, JSVAL_NULL);

	if (argc != 2)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 2 arguments.  " PUSH_CLONE_COMMIT_USAGE)  );

	if ( !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "clone_id must be a string.  " PUSH_CLONE_COMMIT_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "fragball_path must be a string.  " PUSH_CLONE_COMMIT_USAGE)  );

	SG_ERR_CHECK(  _throw_if_clone_disallowed(pCtx)  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_cloneid)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_fragball_path)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pFragballPath, psz_fragball_path)  );

	SUSPEND_REQUEST_ERR_CHECK(  SG_sync_remote__push_clone__commit(pCtx, psz_cloneid, &pFragballPath) );
	SG_NULLFREE(pCtx, psz_cloneid);
	SG_NULLFREE(pCtx, psz_fragball_path);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_PATHNAME_NULLFREE(pCtx, pFragballPath);
	SG_NULLFREE(pCtx, psz_cloneid);
	SG_NULLFREE(pCtx, psz_fragball_path);
	return JS_FALSE;
}
#undef PUSH_CLONE_COMMIT_USAGE

/**
 * sg.sync_remote.push_clone_commit_maybe_usermap(clone_id, closet_admin_id, fragball_path)
 *
 * No return value.
 */
#define PUSH_CLONE_COMMIT_MAYBE_USERMAP_USAGE "Usage: sg.sync_remote.push_clone_commit_maybe_usermap(clone_id, closet_admin_id fragball_path)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_clone_commit_maybe_usermap)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char *psz_cloneid = NULL;
	char *psz_closet_admin_id = NULL;
	char *psz_fragball_path = NULL;
	SG_pathname* pFragballPath = NULL;
	
	char* pszStatus = NULL;
	SG_bool bAvail = SG_FALSE;
	SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
	char* pszDescriptorName = NULL;
	
	SG_vhash* pvhReturn = NULL;
	JSObject* jso = NULL;

	if (argc != 3)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 3 arguments.  " PUSH_CLONE_COMMIT_MAYBE_USERMAP_USAGE)  );

	if ( !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "clone_id must be a string.  " PUSH_CLONE_COMMIT_MAYBE_USERMAP_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[1]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "closet_admin_id must be a string.  " PUSH_CLONE_COMMIT_MAYBE_USERMAP_USAGE)  );
	if ( !JSVAL_IS_STRING(argv[2]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "fragball_path must be a string.  " PUSH_CLONE_COMMIT_MAYBE_USERMAP_USAGE)  );

	SG_ERR_CHECK(  _throw_if_clone_disallowed(pCtx)  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_cloneid)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[1]), &psz_closet_admin_id)  );
	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[2]), &psz_fragball_path)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pFragballPath, psz_fragball_path)  );

	SUSPEND_REQUEST_ERR_CHECK(  SG_sync_remote__push_clone__commit_maybe_usermap(
		pCtx, psz_cloneid, psz_closet_admin_id, &pFragballPath,
		&pszDescriptorName, &bAvail, &status, &pszStatus) );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhReturn)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhReturn, "available", bAvail)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhReturn, "name", pszDescriptorName)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhReturn, "status_sz", pszStatus)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhReturn, "status", status)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhReturn, jso)  );

	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszStatus);
	SG_VHASH_NULLFREE(pCtx, pvhReturn);
	SG_NULLFREE(pCtx, psz_cloneid);
	SG_NULLFREE(pCtx, psz_closet_admin_id);
	SG_NULLFREE(pCtx, psz_fragball_path);
	return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_NULLFREE(pCtx, pszStatus);
	SG_VHASH_NULLFREE(pCtx, pvhReturn);
	SG_PATHNAME_NULLFREE(pCtx, pFragballPath);
	SG_NULLFREE(pCtx, psz_cloneid);
	SG_NULLFREE(pCtx, psz_closet_admin_id);
	SG_NULLFREE(pCtx, psz_fragball_path);
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	return JS_FALSE;
}
#undef PUSH_CLONE_COMMIT_MAYBE_USERMAP_USAGE

/**
 * sg.sync_remote.push_clone_abort(clone_id)
 *
 * No return value.
 */
#define PUSH_CLONE_ABORT_USAGE "Usage: sg.sync_remote.push_clone_abort(clone_id)"
SG_JSGLUE_METHOD_PROTOTYPE(sync_remote, push_clone_abort)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char* psz_cloneid = NULL;

	JS_SET_RVAL(cx, vp, JSVAL_NULL);

	if (argc != 1)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected 1 argument.  " PUSH_CLONE_ABORT_USAGE)  );

	if ( !JSVAL_IS_STRING(argv[0]) )
		SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "clone_id must be a string.  " PUSH_CLONE_ABORT_USAGE)  );

	SG_ERR_CHECK(  _throw_if_clone_disallowed(pCtx)  );

	SG_ERR_CHECK(  sg_jsglue__jsstring_to_sz(pCtx, cx, JSVAL_TO_STRING(argv[0]), &psz_cloneid)  );

	SG_ERR_CHECK(  SG_staging__clone__abort(pCtx, psz_cloneid) );
	SG_NULLFREE(pCtx, psz_cloneid);
	return JS_TRUE;

fail:
	SG_jsglue__report_sg_error(pCtx, cx); // Don't SG_ERR_IGNORE.
	SG_NULLFREE(pCtx, psz_cloneid);
	return JS_FALSE;
}
#undef PUSH_CLONE_ABORT_USAGE

//////////////////////////////////////////////////////////////////

static JSClass sg_sync_remote__class = {
    "sg_sync_remote",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec sg_sync_remote__methods[] = {
	{ "get_repo_info",					SG_JSGLUE_METHOD_NAME(sync_remote,	get_repo_info),						1,0 },
	{ "request_fragball",				SG_JSGLUE_METHOD_NAME(sync_remote,	request_fragball),					1,0 },
	{ "push_begin",						SG_JSGLUE_METHOD_NAME(sync_remote,	push_begin),						0,0 },
	{ "push_add",						SG_JSGLUE_METHOD_NAME(sync_remote,	push_add),							3,0 },
	{ "push_commit",					SG_JSGLUE_METHOD_NAME(sync_remote,	push_commit),						2,0 },
	{ "push_end",						SG_JSGLUE_METHOD_NAME(sync_remote,	push_end),							1,0 },
	{ "get_dagnode_info",				SG_JSGLUE_METHOD_NAME(sync_remote,	get_dagnode_info),					2,0 },
	{ "check_status",					SG_JSGLUE_METHOD_NAME(sync_remote,	check_status),						2,0 },
	{ "heartbeat",						SG_JSGLUE_METHOD_NAME(sync_remote,	heartbeat),							1,0 },
	{ "push_clone_begin",				SG_JSGLUE_METHOD_NAME(sync_remote,	push_clone_begin),					1,0 },
	{ "push_clone_commit",				SG_JSGLUE_METHOD_NAME(sync_remote,	push_clone_commit),					2,0 },
	{ "push_clone_commit_maybe_usermap",SG_JSGLUE_METHOD_NAME(sync_remote,	push_clone_commit_maybe_usermap),	3,0 },
	{ "push_clone_abort",				SG_JSGLUE_METHOD_NAME(sync_remote,	push_clone_abort),					1,0 },
	JS_FS_END
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif // H_SG_JSGLUE__PRIVATE__SYNC_REMOTE_H
