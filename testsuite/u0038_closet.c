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
 * @file u0038_closet.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"

void u0038_test_ridesc(SG_context * pCtx)
{
	SG_closet_descriptor_handle* ph = NULL;
	SG_vhash* pvh = NULL;
	SG_vhash* pvh_all = NULL;
	SG_uint32 count = 0;

	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "1")  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_begin(pCtx, "1", NULL, NULL, NULL, NULL, &pvh, &ph)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_commit(pCtx, &ph, pvh, SG_REPO_STATUS__NORMAL));
	SG_VHASH_NULLFREE(pCtx, pvh);
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__remove(pCtx, "1")  );

	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "2")  );		/* This may or may not be an error */

	/* delete one that is not there should be an error */
	VERIFY_ERR_CHECK_HAS_ERR_DISCARD(  SG_closet__descriptors__remove(pCtx, "2")  );

	/* fetch one that is not there should be an error */
	VERIFY_ERR_CHECK_HAS_ERR_DISCARD(  SG_closet__descriptors__get(pCtx, "2", NULL, &pvh)  );
	VERIFY_COND("", pvh==NULL);

	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "3")  );
	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "4")  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_begin(pCtx, "3", NULL, NULL, NULL, NULL, &pvh, &ph)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_commit(pCtx, &ph, pvh, SG_REPO_STATUS__NORMAL)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_begin(pCtx, "4", NULL, NULL, NULL, NULL, &pvh, &ph)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_commit(pCtx, &ph, pvh, SG_REPO_STATUS__NORMAL)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
	
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__get(pCtx, "3", NULL, &pvh)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__list(pCtx, &pvh_all)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_vhash__count(pCtx, pvh_all, &count)  );

	/* adding a duplicate name should be an error */
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(
		SG_closet__descriptors__add_begin(pCtx, "3", NULL, NULL, NULL, NULL, NULL, &ph), 
		SG_ERR_REPO_ALREADY_EXISTS);

	VERIFY_COND("count", (count >= 2));
	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "3")  );
	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "4")  );

	SG_VHASH_NULLFREE(pCtx, pvh_all);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void u0038_test_wdmapping(SG_context * pCtx)
{
	SG_vhash* pvh = NULL;
	SG_closet_descriptor_handle* ph = NULL;
	SG_pathname* pPath = NULL;
	SG_pathname* pMappedPath = NULL;
	SG_string* pstrRepoDescriptorName = NULL;
	char* pszidGid = NULL;
	char buf_tid[SG_TID_MAX_BUFFER_LENGTH];

	VERIFY_ERR_CHECK_DISCARD(  SG_tid__generate2__suffix(pCtx, buf_tid, sizeof(buf_tid), 32, "u0038")  );
	VERIFY_ERR_CHECK_DISCARD(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, buf_tid)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );

	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "r1")  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_begin(pCtx, "r1", NULL, NULL, NULL, NULL, &pvh, &ph)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_closet__descriptors__add_commit(pCtx, &ph, pvh, SG_REPO_STATUS__NORMAL)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
	VERIFY_ERR_CHECK_DISCARD(  SG_workingdir__set_mapping(pCtx, pPath, "r1", NULL)  );
	VERIFY_ERR_CHECK_DISCARD(  SG_workingdir__find_mapping(pCtx, pPath, &pMappedPath, &pstrRepoDescriptorName, &pszidGid)  );
	VERIFY_COND("ridesc match", (0 == strcmp("r1", SG_string__sz(pstrRepoDescriptorName))));
	VERIFY_ERR_CHECK_DISCARD(  SG_pathname__append__from_sz(pCtx, pPath, "foo")  );
	VERIFY_ERR_CHECK_DISCARD(  SG_pathname__append__from_sz(pCtx, pPath, "bar")  );
	VERIFY_ERR_CHECK_DISCARD(  SG_pathname__append__from_sz(pCtx, pPath, "plok")  );

	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
	SG_PATHNAME_NULLFREE(pCtx, pMappedPath);

	VERIFY_ERR_CHECK_DISCARD(  SG_workingdir__find_mapping(pCtx, pPath, &pMappedPath, &pstrRepoDescriptorName, &pszidGid)  );

	SG_STRING_NULLFREE(pCtx, pstrRepoDescriptorName);
	SG_PATHNAME_NULLFREE(pCtx, pMappedPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void u0038_test_version(SG_context * pCtx)
{
	/* This test pokes around in closet internals in ways normal closet callers shouldn't. */

	SG_string* pstrEnv = NULL;
	SG_uint32 len;
	SG_pathname* pPathCloset = NULL;
	SG_pathname* pPathClosetVersion = NULL;
	SG_pathname* pPathClosetVersionBackup = NULL;
	SG_file* pFile = NULL;
	SG_vhash* pvh = NULL;
	
	/* Deliberately making this break for closet version 3 -- current is version 2. */
	SG_byte buf[3]; 
	
	VERIFY_ERR_CHECK(  SG_environment__get__str(pCtx, "SGCLOSET", &pstrEnv, &len)  );
	if (len)
	{
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathCloset, SG_string__sz(pstrEnv))  );
	}
	else
	{
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_APPDATA_DIRECTORY(pCtx, &pPathCloset)  );
		VERIFY_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathCloset, ".sgcloset")  );
	}

	VERIFY_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathClosetVersion, pPathCloset, "version")  );
	VERIFY_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathClosetVersionBackup, pPathCloset, "version.bak")  );

	VERIFY_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPathClosetVersion, pPathClosetVersionBackup)  );
	
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathClosetVersion, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY|SG_FILE_TRUNC, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, sizeof(buf), buf, NULL)  ); 
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_closet__descriptors__list(pCtx, &pvh);
	VERIFY_COND("", SG_context__err_equals(pCtx, SG_ERR_UNSUPPORTED_CLOSET_VERSION));
	SG_ERR_DISCARD;

	VERIFY_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPathClosetVersion)  );
	VERIFY_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPathClosetVersionBackup, pPathClosetVersion)  );

	/* Common cleanup */
fail:
	SG_STRING_NULLFREE(pCtx, pstrEnv);
	SG_PATHNAME_NULLFREE(pCtx, pPathCloset);
	SG_PATHNAME_NULLFREE(pCtx, pPathClosetVersion);
	SG_PATHNAME_NULLFREE(pCtx, pPathClosetVersionBackup);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

//////////////////////////////////////////////////////////////////////////

typedef struct _thread_data
{
	SG_context* pCtx;
} thread_data;

#if defined(WINDOWS)
static void _start_thread(SG_context * pCtx, LPTHREAD_START_ROUTINE pFunc, void* pParam, HANDLE* ppHandle)
{
	HANDLE	hThread;

	SG_UNUSED(pCtx);

	hThread = CreateThread(NULL, 0, pFunc, pParam, 0, NULL);
	

	if (hThread != NULL)
		*ppHandle = hThread;
	else
	{
		SG_ERR_THROW2_RETURN(SG_ERR_GETLASTERROR(GetLastError()), 
			(pCtx, "%s", "CreateThread failed"));
	}
}
#else
static void _start_thread(SG_context * pCtx, void* pFunc, void* pParam, pthread_t* pThreadId)
// pCtx: input parameter only (used for logging)
{
	pthread_t	thread_id;
//	pthread_attr_t	attr;
	int		retval;

//	(void) pthread_attr_init(&attr);
//	(void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	retval = pthread_create(&thread_id, NULL, pFunc, pParam);
	if (retval)
		SG_ERR_THROW2_RETURN(SG_ERR_ERRNO(retval), 
			(pCtx, "%s", "pthread_create failed"));

	*pThreadId = thread_id;
}
#endif

#if defined(WINDOWS)
static DWORD WINAPI _add_conflicting_descriptor_in_thread(LPVOID pParam)
#else
static void _add_conflicting_descriptor_in_thread(void* pParam)
#endif
{
	thread_data* pMe = (thread_data*)pParam;
	SG_context* pCtx = pMe->pCtx;
	SG_closet_descriptor_handle* ph = NULL;
	SG_vhash* pvh = NULL;

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_closet__descriptors__add_begin(pCtx, "1", NULL, NULL, NULL, NULL, &pvh, &ph),
		SG_ERR_REPO_ALREADY_EXISTS);
	VERIFY_COND("", !pvh);
	VERIFY_COND("", !ph);

	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_ERR_IGNORE(SG_closet__descriptors__add_abort(pCtx, &ph));

#if defined(WINDOWS)
	return (SG_context__has_err(pCtx));
#endif
}

void u0038_test_name_conflict(SG_context * pCtx)
{
	thread_data* pThreadData = NULL;
	SG_closet_descriptor_handle* ph = NULL;
	SG_vhash* pvh = NULL;
	SG_closet_descriptor_handle* ph2 = NULL;
	SG_vhash* pvh2 = NULL;
#if defined(WINDOWS)
	HANDLE hThread = NULL;
#else
	pthread_t thread_id = 0;
#endif

	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "1")  );
	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "2")  );

	VERIFY_ERR_CHECK(  SG_closet__descriptors__add_begin(pCtx, "1", NULL, NULL, NULL, NULL, &pvh, &ph)  );

	VERIFY_ERR_CHECK(  SG_alloc1(pCtx, pThreadData)  );
	VERIFY_ERR_CHECK(  SG_context__alloc__temporary(&pThreadData->pCtx)  );
#if defined(WINDOWS)
	VERIFY_ERR_CHECK(  _start_thread(pCtx, _add_conflicting_descriptor_in_thread, pThreadData, &hThread)  );
#else
	VERIFY_ERR_CHECK(  _start_thread(pCtx, _add_conflicting_descriptor_in_thread, pThreadData, &thread_id)  );
#endif

	VERIFY_ERR_CHECK(  SG_sleep_ms(1000)  );
	VERIFY_ERR_CHECK(  SG_closet__descriptors__add_commit(pCtx, &ph, pvh, SG_REPO_STATUS__NORMAL)  );
	VERIFY_COND("", !ph);
	SG_VHASH_NULLFREE(pCtx, pvh);

#if defined(WINDOWS)
	{
		DWORD dwRet = WaitForSingleObject(hThread, 1000);
		VERIFY_COND("failure waiting for thread termination", dwRet == WAIT_OBJECT_0);
		VERIFY_CTX_IS_OK("thread context has error", pThreadData->pCtx);
		(void) CloseHandle(hThread);
	}
#else
	{
		int rc = pthread_join(thread_id, NULL);
		if (rc)
			SG_ERR_THROW2(SG_ERR_ERRNO(rc), (pCtx, "%s", "pthread_join failed"));
		VERIFY_CTX_IS_OK("thread context has error", pThreadData->pCtx);
	}
#endif

	VERIFY_ERR_CHECK(  SG_closet__descriptors__add_begin(pCtx, "2", NULL, NULL, NULL, NULL, &pvh2, &ph2)  );
	VERIFY_ERR_CHECK(  SG_closet__descriptors__add_commit(pCtx, &ph2, pvh2, SG_REPO_STATUS__NORMAL)  );
	
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(SG_closet__descriptors__add_begin(pCtx, "1", NULL, NULL, NULL, NULL, &pvh, &ph),
		SG_ERR_REPO_ALREADY_EXISTS);
	VERIFY_COND("", !pvh);
	VERIFY_COND("", !ph);

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VHASH_NULLFREE(pCtx, pvh2);
	SG_ERR_IGNORE(SG_closet__descriptors__add_abort(pCtx, &ph));
	SG_ERR_IGNORE(SG_closet__descriptors__add_abort(pCtx, &ph2));
	if (pThreadData)
	{
		SG_CONTEXT_NULLFREE(pThreadData->pCtx);
		SG_NULLFREE(pCtx, pThreadData);
	}
	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "1")  );
	SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, "2")  );
}

//////////////////////////////////////////////////////////////////////////

TEST_MAIN(u0038_closet)
{
	TEMPLATE_MAIN_START;

// 	BEGIN_TEST(  u0038_test_ridesc(pCtx)  );
// 	BEGIN_TEST(  u0038_test_wdmapping(pCtx)  );
// 	BEGIN_TEST(  u0038_test_version(pCtx)  );
	BEGIN_TEST(  u0038_test_name_conflict(pCtx)  );

	TEMPLATE_MAIN_END;
}
