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

/*
 *
 * @file sg_client_vtable__http__curl.c
 *
 */

#include <sg.h>
#include "sg_sync_client__api.h"

#include "sg_sync_client_vtable__http.h"

//////////////////////////////////////////////////////////////////////////

#define SYNC_URL_SUFFIX     "/sync"
#define CLONE_URL_SUFFIX	"/clone"
#define JSON_URL_SUFFIX     ".json"
#define FRAGBALL_URL_SUFFIX ".fragball"

#define PUSH_ID_KEY				"id"

#define DOWNLOAD_PROGRESS_MIN_BYTES	1000

//////////////////////////////////////////////////////////////////////////

struct _sg_sync_client_http_push_handle
{
	char* pszPushId;
};
typedef struct _sg_sync_client_http_push_handle sg_sync_client_http_push_handle;
static void _handle_free(SG_context * pCtx, sg_sync_client_http_push_handle* pPush)
{
	if (pPush)
	{
		if ( (pPush)->pszPushId )
			SG_NULLFREE(pCtx, pPush->pszPushId);

		SG_NULLFREE(pCtx, pPush);
	}
}
#define _NULLFREE_PUSH_HANDLE(pCtx,p) SG_STATEMENT(  _handle_free(pCtx, p); p=NULL;  )

struct _sg_client_http_instance_data
{
	SG_curl* pCurl;
};
typedef struct _sg_client_http_instance_data sg_client_http_instance_data;

//////////////////////////////////////////////////////////////////////////

static void _get_sync_url(SG_context* pCtx, const char* pszBaseUrl, const char* pszUrlSuffix, const char* pszPushId, const char* pszQueryString, char** ppszUrl)
{
	SG_string* pstrUrl = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrUrl, 1024)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszBaseUrl)  );

	if (pszUrlSuffix)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszUrlSuffix)  );

	if (pszPushId)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, "/")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszPushId)  );
	}
	if (pszQueryString)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, "?")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszQueryString)  );
	}

	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrUrl, (SG_byte**)ppszUrl, NULL)  );
	
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstrUrl);
}

static void _report_transfer_progress(SG_context* pCtx, const char* szLabel, double done, double total)
{
	static const uint32 kB = 1024;
	static const uint32 MB = 1048576;
	const char* szUnit = NULL;

	SG_uint32 reportDone;
	SG_uint32 reportTotal;

	// Show MB, unless the total to download is less than 1 MB.
	if (total < kB)
	{
		reportDone = (SG_uint32)done;
		reportTotal = (SG_uint32)total;
		szUnit = "bytes";
	}
	else if (total < MB)
	{
		reportDone = (SG_uint32)(done / kB);
		reportTotal = (SG_uint32)(total / kB);
		szUnit = "kB";
	}
	else 
	{
		reportDone = (SG_uint32)(done / MB);
		reportTotal = (SG_uint32)(total / MB);
		szUnit = "MB";
	}

	if (szLabel)
		SG_ERR_IGNORE(  SG_log__set_operation(pCtx, szLabel)  );

	SG_ERR_IGNORE(  SG_log__set_steps(pCtx, reportTotal, szUnit)  );
	SG_ERR_IGNORE(  SG_log__set_finished(pCtx, reportDone)  );
}

static void _pull_clone_progress_callback(
	SG_context* pCtx,
	double dltotal,
	double dlnow,
	double ultotal,
	double ulnow,
	void *pVoidState)
{
	SG_UNUSED(ultotal);
	SG_UNUSED(ulnow);
	SG_UNUSED(pVoidState);

	if (dlnow > 0 && dltotal > DOWNLOAD_PROGRESS_MIN_BYTES)
		SG_ERR_CHECK_RETURN(  _report_transfer_progress(pCtx, "Downloading repository", dlnow, dltotal)  );
}

static void _push_clone_progress_callback(
	SG_context* pCtx,
	double dltotal,
	double dlnow,
	double ultotal,
	double ulnow,
	void *pVoidState)
{
	SG_bool bCommitting = *(SG_bool*)pVoidState;
	SG_UNUSED(bCommitting);
	
	SG_UNUSED(dltotal);
	SG_UNUSED(dlnow);

	if (!bCommitting && ulnow > 0)
	{
		SG_ERR_CHECK_RETURN(  _report_transfer_progress(pCtx, "Uploading repository", ulnow, ultotal)  );

		if (ulnow >= ultotal)
		{
			/* This is a little bit of a hack, but separating these more cleanly would require
			 * adding another HTTP roundtrip. It doesn't seem worth it. */
			*(SG_bool*)pVoidState = SG_TRUE;
			SG_ERR_CHECK_RETURN(  SG_log__pop_operation(pCtx)  );
			SG_ERR_CHECK_RETURN(  SG_log__push_operation(pCtx, "Committing remote repository", SG_LOG__FLAG__NONE)  );
		}
	}
}

static void _pull_progress_callback(
	SG_context* pCtx,
	double dltotal,
	double dlnow,
	double ultotal,
	double ulnow,
	void *pVoidState)
{
	SG_UNUSED(ultotal);
	SG_UNUSED(ulnow);
	SG_UNUSED(pVoidState);

	if (dlnow > 0&& dltotal > DOWNLOAD_PROGRESS_MIN_BYTES)
		SG_ERR_CHECK_RETURN(  _report_transfer_progress(pCtx, NULL, dlnow, dltotal)  );
}

static void _push_progress_callback(
	SG_context* pCtx,
	double dltotal,
	double dlnow,
	double ultotal,
	double ulnow,
	void *pVoidState)
{
	SG_UNUSED(dltotal);
	SG_UNUSED(dlnow);
	SG_UNUSED(pVoidState);

	if (ulnow > 0)
		SG_ERR_CHECK_RETURN(  _report_transfer_progress(pCtx, NULL, ulnow, ultotal)  );
}

static void _curl_reset(SG_context* pCtx, SG_curl* pCurl)
{
	SG_ERR_CHECK_RETURN(  SG_curl__reset(pCtx, pCurl)  );
	SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_FOLLOWLOCATION, 1)  );
	SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_MAXREDIRS, 5)  );
	SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL)  );
}

//////////////////////////////////////////////////////////////////////////

void sg_sync_client__http__open(
	SG_context* pCtx,
	SG_sync_client * pSyncClient)
{
	sg_client_http_instance_data* pMe = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pMe)  );
	pSyncClient->p_vtable_instance_data = (sg_sync_client__vtable__instance_data *)pMe;

	SG_ERR_CHECK_RETURN(  SG_curl__alloc(pCtx, &pMe->pCurl)  );
}

void sg_sync_client__http__close(SG_context * pCtx, SG_sync_client * pSyncClient)
{
	sg_client_http_instance_data* pMe = NULL;

	if (!pSyncClient)
		return;

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_CURL_NULLFREE(pCtx, pMe->pCurl);
	SG_NULLFREE(pCtx, pMe);
}

//////////////////////////////////////////////////////////////////////////

void sg_sync_client__http__push_begin(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_pathname** ppFragballDirPathname,
	SG_sync_client_push_handle** ppPush)
{
	sg_client_http_instance_data* pMe = NULL;
	char* pszUrl = NULL;
	SG_string* pstrResponse = NULL;
	sg_sync_client_http_push_handle* pPush = NULL;
	SG_vhash* pvhResponse = NULL;
	SG_pathname* pPathUserTemp = NULL;
	SG_pathname* pPathFragballDir = NULL;
	char bufTid[SG_TID_MAX_BUFFER_LENGTH];

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(ppFragballDirPathname);
	SG_NULLARGCHECK_RETURN(ppPush);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	// Get the URL we're going to post to
	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX JSON_URL_SUFFIX, NULL, NULL, &pszUrl)  );

	// Set HTTP options
	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POST, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, 0)  );

	// Set up to write response into an SG_string.
	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	// Do it
	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	// Alloc a push handle.  Stuff the push ID we received into it.
	{
		const char* pszRef = NULL;
		SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(sg_sync_client_http_push_handle), &pPush)  );
		SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvhResponse, SG_string__sz(pstrResponse))  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResponse, PUSH_ID_KEY, &pszRef)  );
		SG_ERR_CHECK(  SG_strdup(pCtx, pszRef, &pPush->pszPushId)  );
	}

	// Create a temporary local directory for stashing fragballs before shipping them over the network.
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathUserTemp)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, bufTid, SG_TID_MAX_BUFFER_LENGTH)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFragballDir, pPathUserTemp, bufTid)  );
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathFragballDir)  );

	// Tell caller where to stash fragballs for this push.
	SG_RETURN_AND_NULL(pPathFragballDir, ppFragballDirPathname);

	// Return the new push handle.
	*ppPush = (SG_sync_client_push_handle*)pPush;
	pPush = NULL;

	/* fall through */
fail:
	if(SG_context__err_equals(pCtx, SG_ERR_SERVER_HTTP_ERROR))
	{
		const char * szInfo = NULL;
		if(SG_IS_OK(SG_context__err_get_description(pCtx, &szInfo)) && strcmp(szInfo, "405")==0)
			SG_ERR_RESET_THROW(SG_ERR_SERVER_DOESNT_ACCEPT_PUSHES);
	}
	_NULLFREE_PUSH_HANDLE(pCtx, pPush);
	SG_NULLFREE(pCtx, pszUrl);
	SG_PATHNAME_NULLFREE(pCtx, pPathUserTemp);
	SG_PATHNAME_NULLFREE(pCtx, pPathFragballDir);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_VHASH_NULLFREE(pCtx, pvhResponse);
}

//////////////////////////////////////////////////////////////////////////

void sg_sync_client__http__push_add(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle* pPush,
	SG_bool bProgressIfPossible, 
	SG_pathname** ppPath_fragball,
	SG_vhash** ppResult)
{
	sg_client_http_instance_data* pMe = NULL;
	sg_sync_client_http_push_handle* pMyPush = (sg_sync_client_http_push_handle*)pPush;
	SG_file* pFragballFile = NULL;
	SG_string* pstrResponse = NULL;
	char* pszUrl = NULL;
	SG_uint64 lenFragball;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pPush);
	SG_NULLARGCHECK_RETURN(ppResult);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );
	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	if (!ppPath_fragball || !*ppPath_fragball)
	{
		/* get the push's current status */
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPGET, 1)  );

		SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
		SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

		SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
		SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

		SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, ppResult, SG_string__sz(pstrResponse))  );
	}
	else
	{
		SG_int_to_string_buffer buf;

		/* add the fragball to the push */
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_UPLOAD, 1)  );

		SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, *ppPath_fragball, &lenFragball, NULL)  );
		SG_ERR_CHECK(  SG_curl__setopt__int64(pCtx, pMe->pCurl, CURLOPT_INFILESIZE_LARGE, lenFragball)  );

		SG_ERR_CHECK(  SG_uint64_to_sz(lenFragball, buf)  );
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Adding %s byte fragball to push", buf)  );

		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, *ppPath_fragball, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, SG_FSOBJ_PERMS__UNUSED, &pFragballFile)  );
		SG_ERR_CHECK(  SG_curl__set__read_file(pCtx, pMe->pCurl, pFragballFile, lenFragball)  );

		if (bProgressIfPossible)
			SG_ERR_CHECK(  SG_curl__set__progress_cb(pCtx, pMe->pCurl, _push_progress_callback, NULL)  );

		SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
		SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

		SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
		SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

		SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, ppResult, SG_string__sz(pstrResponse))  );

		SG_PATHNAME_NULLFREE(pCtx, *ppPath_fragball);
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_FILE_NULLCLOSE(pCtx, pFragballFile);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
}

//////////////////////////////////////////////////////////////////////////

void sg_sync_client__http__push_commit(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle* pPush,
	SG_vhash** ppResult)
{
	sg_client_http_instance_data* pMe = NULL;
	SG_string* pstrResponse = NULL;
	char* pszUrl = NULL;
	sg_sync_client_http_push_handle* pMyPush = (sg_sync_client_http_push_handle*)pPush;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pPush);
	SG_NULLARGCHECK_RETURN(ppResult);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );

	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POST, 1)  );

	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, 0)  );

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, ppResult, SG_string__sz(pstrResponse))  );

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
}

void sg_sync_client__http__push_end(
	SG_context * pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle** ppPush)
{
	sg_client_http_instance_data* pMe = NULL;
	sg_sync_client_http_push_handle* pMyPush = NULL;
	char* pszUrl = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULL_PP_CHECK_RETURN(ppPush);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;
	pMyPush = (sg_sync_client_http_push_handle*)*ppPush;

	// Get the remote URL
	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );

	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_CUSTOMREQUEST, "DELETE")  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	// fall through
fail:
	// We free the push handle even on failure, because SG_push_handle is opaque outside this file:
	// this is the only way to free it.
	_NULLFREE_PUSH_HANDLE(pCtx, pMyPush);
	*ppPush = NULL;
	SG_NULLFREE(pCtx, pszUrl);
}

void sg_sync_client__http__request_fragball(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_vhash* pvhRequest,
	SG_bool bProgressIfPossible,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName)
{
	sg_client_http_instance_data* pMe = NULL;
	char* pszFragballName = NULL;
	SG_pathname* pPathFragball = NULL;
	SG_file* pFragballFile = NULL;
	SG_string* pstrRequest = NULL;
	char* pszUrl = NULL;
	struct curl_slist* pHeaderList = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  SG_allocN(pCtx, SG_TID_MAX_BUFFER_LENGTH, pszFragballName)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, pszFragballName, SG_TID_MAX_BUFFER_LENGTH)  );

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathFragball, pStagingPathname, (const char*)pszFragballName)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFragball, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0644, &pFragballFile)  );

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX FRAGBALL_URL_SUFFIX, NULL, NULL, &pszUrl)  );
	
	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POST, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	if (pvhRequest)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrRequest)  );
		SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRequest, pstrRequest)  );

 		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_POSTFIELDS, SG_string__sz(pstrRequest))  );
 		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, SG_string__length_in_bytes(pstrRequest))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, 0)  );
	}

	if (bProgressIfPossible)
		SG_ERR_CHECK(  SG_curl__set__progress_cb(pCtx, pMe->pCurl, _pull_progress_callback, NULL)  );

	SG_ERR_CHECK(  SG_curl__set__write_file(pCtx, pMe->pCurl, pFragballFile)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	SG_RETURN_AND_NULL(pszFragballName, ppszFragballName);

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszFragballName);
	SG_PATHNAME_NULLFREE(pCtx, pPathFragball);
	SG_FILE_NULLCLOSE(pCtx, pFragballFile);
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
	SG_CURL_HEADERS_NULLFREE(pCtx, pHeaderList);
}

void sg_sync_client__http__pull_clone(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
    SG_vhash* pvh_clone_request,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName)
{
	sg_client_http_instance_data* pMe = NULL;
	SG_vhash* pvhRequest = NULL;
	char* pszFragballName = NULL;
	SG_pathname* pPathFragball = NULL;
	SG_file* pFragballFile = NULL;
	SG_string* pstrRequest = NULL;
	char* pszUrl = NULL;
	struct curl_slist* pHeaderList = NULL;
	SG_int32 httpResponseCode = 0;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Waiting for server to start transfer", SG_LOG__FLAG__NONE)  );

	SG_NULLARGCHECK_RETURN(pSyncClient);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  SG_allocN(pCtx, SG_TID_MAX_BUFFER_LENGTH, pszFragballName)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, pszFragballName, SG_TID_MAX_BUFFER_LENGTH)  );

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathFragball, pStagingPathname, (const char*)pszFragballName)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFragball, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0644, &pFragballFile)  );

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX FRAGBALL_URL_SUFFIX, NULL, NULL, &pszUrl)  );

	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POST, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhRequest)  );
	SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__CLONE)  );
    if (pvh_clone_request)
    {
        SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__CLONE_REQUEST, pvh_clone_request)  );
    }
	SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrRequest, 50)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRequest, pstrRequest)  );

	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_POSTFIELDS, SG_string__sz(pstrRequest))  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, SG_string__length_in_bytes(pstrRequest))  );

	SG_ERR_CHECK(  SG_curl__set__progress_cb(pCtx, pMe->pCurl, _pull_clone_progress_callback, NULL)  );
	SG_ERR_CHECK(  SG_curl__set__write_file(pCtx, pMe->pCurl, pFragballFile)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );

	SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pMe->pCurl, CURLINFO_RESPONSE_CODE, &httpResponseCode)  );
	if (httpResponseCode == 404)
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));

	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	SG_RETURN_AND_NULL(pszFragballName, ppszFragballName);

	/* fall through */
fail:
	SG_log__pop_operation(pCtx);
	SG_NULLFREE(pCtx, pszFragballName);
	SG_VHASH_NULLFREE(pCtx, pvhRequest);
	SG_PATHNAME_NULLFREE(pCtx, pPathFragball);
	SG_FILE_NULLCLOSE(pCtx, pFragballFile);
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
	SG_CURL_HEADERS_NULLFREE(pCtx, pHeaderList);
}

void sg_sync_client__http__get_repo_info(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_bool bIncludeBranchInfo,
    SG_bool b_include_areas,
    SG_vhash** ppvh)
{
	sg_client_http_instance_data* pMe = NULL;
	char* pszUrl = NULL;
	SG_vhash* pvh = NULL;
	SG_string* pstrResponse = NULL;
	SG_uint32 iProtocolVersion = 0;
	SG_int32 httpResponseCode = 0;
    const char* psz_qs = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

    if (bIncludeBranchInfo && b_include_areas)
    {
        psz_qs = "details&areas";
    }
    else if (bIncludeBranchInfo && !b_include_areas)
    {
        psz_qs = "details";
    }
    else if (!bIncludeBranchInfo && b_include_areas)
    {
        psz_qs = "areas";
    }
    else
    {
        psz_qs = NULL;
    }

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, 
		SYNC_URL_SUFFIX JSON_URL_SUFFIX, NULL, psz_qs, &pszUrl)  );

	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );

	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPGET, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	SG_ERR_CHECK(  SG_string__alloc__reserve(pCtx, &pstrResponse, 256)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );

	SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pMe->pCurl, CURLINFO_RESPONSE_CODE, &httpResponseCode)  );
 	if (httpResponseCode == 404)
	{
		if (SG_string__length_in_bytes(pstrResponse) > 0)
			SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", SG_string__sz(pstrResponse)));
		else
 			SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));
	}

	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	SG_vhash__alloc__from_json__sz(pCtx, &pvh, SG_string__sz(pstrResponse));
	if(SG_context__err_equals(pCtx, SG_ERR_JSON_WRONG_TOP_TYPE) || SG_context__err_equals(pCtx, SG_ERR_JSONPARSER_SYNTAX))
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));
	else
		SG_ERR_CHECK_CURRENT;

	SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__PROTOCOL_VERSION, &iProtocolVersion)  );
	if (iProtocolVersion != 1)
		SG_ERR_THROW2(SG_ERR_UNKNOWN_SYNC_PROTOCOL_VERSION, (pCtx, "%u", iProtocolVersion));

	*ppvh = pvh;
	pvh = NULL;

	/* fall through */

fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_sync_client__http__heartbeat(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_vhash** ppvh)
{
	sg_client_http_instance_data* pMe = NULL;
	char* pszUrl = NULL;
	SG_vhash* pvh = NULL;
	SG_string* pstrResponse = NULL;
	SG_int32 httpResponseCode = 0;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, 
		"/heartbeat" JSON_URL_SUFFIX, NULL, NULL, &pszUrl)  );

	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPGET, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	SG_ERR_CHECK(  SG_string__alloc__reserve(pCtx, &pstrResponse, 256)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );

	SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pMe->pCurl, CURLINFO_RESPONSE_CODE, &httpResponseCode)  );
	if (httpResponseCode == 404)
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));

	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	SG_vhash__alloc__from_json__sz(pCtx, ppvh, SG_string__sz(pstrResponse));
	if(SG_context__err_equals(pCtx, SG_ERR_JSON_WRONG_TOP_TYPE) || SG_context__err_equals(pCtx, SG_ERR_JSONPARSER_SYNTAX))
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));
	else
		SG_ERR_CHECK_CURRENT;

	/* fall through */

fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_sync_client__http__get_dagnode_info(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo)
{
	sg_client_http_instance_data* pMe = NULL;
	SG_string* pstrRequest = NULL;
	SG_string* pstrResponse = NULL;
	char* pszUrl = NULL;
	struct curl_slist* pHeaderList = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pvhRequest);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX "/incoming" JSON_URL_SUFFIX, NULL, NULL, &pszUrl)  );

	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POST, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrRequest)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRequest, pstrRequest)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_POSTFIELDS, SG_string__sz(pstrRequest))  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, SG_string__length_in_bytes(pstrRequest))  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	SG_ERR_CHECK(  SG_history_result__from_json(pCtx, SG_string__sz(pstrResponse), ppInfo)  );

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_CURL_HEADERS_NULLFREE(pCtx, pHeaderList);
}

void sg_sync_client__http__push_clone__begin(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	const SG_vhash* pvhExistingRepoInfo,
	SG_sync_client_push_handle** ppPush
	)
{
	sg_client_http_instance_data* pMe = NULL;
	sg_sync_client_http_push_handle* pPush = NULL;
	SG_string* pstrRequest = NULL;
	SG_vhash* pvhResponse = NULL;
	SG_string* pstrResponse = NULL;
	char* pszUrl = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(ppPush);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	// Get the URL we're going to post to
	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX CLONE_URL_SUFFIX, NULL, NULL, &pszUrl)  );

	// Set HTTP options
	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POST, 1)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	// POST repo info in request
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrRequest)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhExistingRepoInfo, pstrRequest)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_POSTFIELDS, SG_string__sz(pstrRequest))  );
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_POSTFIELDSIZE, SG_string__length_in_bytes(pstrRequest))  );

	// Set up to write response into an SG_string.
	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	// Do it
	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	// Alloc a push handle.  Stuff the push ID we received into it.
	{
		const char* pszRef = NULL;
		SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(sg_sync_client_http_push_handle), &pPush)  );
		SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvhResponse, SG_string__sz(pstrResponse))  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResponse, PUSH_ID_KEY, &pszRef)  );
		SG_ERR_CHECK(  SG_strdup(pCtx, pszRef, &pPush->pszPushId)  );
	}

	// Return the new push handle.
	*ppPush = (SG_sync_client_push_handle*)pPush;
	pPush = NULL;

	/* fall through */
fail:
	if(SG_context__err_equals(pCtx, SG_ERR_SERVER_HTTP_ERROR))
	{
		const char * szInfo = NULL;
		if(SG_IS_OK(SG_context__err_get_description(pCtx, &szInfo)) && strcmp(szInfo, "405")==0)
			SG_ERR_RESET_THROW(SG_ERR_SERVER_DOESNT_ACCEPT_PUSHES);
	}
	_NULLFREE_PUSH_HANDLE(pCtx, pPush);
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
	SG_STRING_NULLFREE(pCtx, pstrResponse);
	SG_VHASH_NULLFREE(pCtx, pvhResponse);
}

void sg_sync_client__http__push_clone__upload_and_commit(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_sync_client_push_handle** ppPush,
	SG_bool bProgressIfPossible,
	SG_pathname** ppPathCloneFragball,
	SG_vhash** ppvhResult)
{
	sg_client_http_instance_data* pMe = NULL;
	sg_sync_client_http_push_handle* pMyPush = NULL;
	SG_file* pFragballFile = NULL;
	char* pszUrl = NULL;
	SG_uint64 lenFragball;
	SG_int_to_string_buffer buf;
	SG_string* pstrResponse = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULL_PP_CHECK_RETURN(ppPush);
	SG_NULL_PP_CHECK_RETURN(ppPathCloneFragball);

	pMyPush = (sg_sync_client_http_push_handle*)*ppPush;
	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX CLONE_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );
	SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
	if(pSyncClient->psz_username && pSyncClient->psz_password)
	{
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
	}

	/* add the fragball to the push */
	SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_UPLOAD, 1)  );

	SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, *ppPathCloneFragball, &lenFragball, NULL)  );
	SG_ERR_CHECK(  SG_curl__setopt__int64(pCtx, pMe->pCurl, CURLOPT_INFILESIZE_LARGE, lenFragball)  );

	SG_ERR_CHECK(  SG_uint64_to_sz(lenFragball, buf)  );
	SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Adding %s byte fragball to push", buf)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, *ppPathCloneFragball, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, SG_FSOBJ_PERMS__UNUSED, &pFragballFile)  );
	SG_ERR_CHECK(  SG_curl__set__read_file(pCtx, pMe->pCurl, pFragballFile, lenFragball)  );

	SG_UNUSED(bProgressIfPossible);
	if (bProgressIfPossible)
	{
		SG_bool bCommitting = SG_FALSE;
		SG_ERR_CHECK(  SG_curl__set__progress_cb(pCtx, pMe->pCurl, _push_clone_progress_callback, &bCommitting)  );
	}

	/* Note that there's no response body unless there's an error, which SG_curl__throw_on_non200 handles.
	 * We set a null response string to keep libcurl from printing the response to the console. */
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrResponse)  );
	SG_ERR_CHECK(  SG_curl__set__write_string(pCtx, pMe->pCurl, pstrResponse)  );

	SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
	SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );

	if (ppvhResult)
	{
		if (SG_string__length_in_bytes(pstrResponse))
		{
			SG_vhash__alloc__from_json__sz(pCtx, ppvhResult, SG_string__sz(pstrResponse));
			SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_JSONPARSER_SYNTAX)
		}
	}

	/* fall through */
fail:
	SG_FILE_NULLCLOSE(pCtx, pFragballFile);
	if (ppPathCloneFragball && *ppPathCloneFragball)
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, *ppPathCloneFragball)  );
	SG_PATHNAME_NULLFREE(pCtx, *ppPathCloneFragball);

	SG_NULLFREE(pCtx, pszUrl);

	SG_STRING_NULLFREE(pCtx, pstrResponse);

	// We free the caller's push handle whether we succeeded or not.
	_NULLFREE_PUSH_HANDLE(pCtx, pMyPush);
	*ppPush = NULL;
}

void sg_sync_client__http__push_clone__abort(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle** ppPush)
{
	sg_client_http_instance_data* pMe = NULL;
	sg_sync_client_http_push_handle* pMyPush = NULL;
	char* pszUrl = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;
	if (ppPush)
		pMyPush = (sg_sync_client_http_push_handle*)*ppPush;

	if (pMyPush)
	{
		SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX CLONE_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );

		SG_ERR_CHECK(  _curl_reset(pCtx, pMe->pCurl)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_URL, pszUrl)  );
		if(pSyncClient->psz_username && pSyncClient->psz_password)
		{
			SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pMe->pCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST)  );
			SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_USERNAME, pSyncClient->psz_username)  );
			SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_PASSWORD, pSyncClient->psz_password)  );
		}
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pMe->pCurl, CURLOPT_CUSTOMREQUEST, "DELETE")  );

		SG_ERR_CHECK(  SG_curl__perform(pCtx, pMe->pCurl)  );
		SG_ERR_CHECK(  SG_curl__throw_on_non200(pCtx, pMe->pCurl)  );
	}

	// fall through
fail:
	// We free the push handle even on failure, because SG_push_handle is opaque outside this file:
	// this is the only way to free it.
	_NULLFREE_PUSH_HANDLE(pCtx, pMyPush);
	if (ppPush)
		*ppPush = NULL;
	SG_NULLFREE(pCtx, pszUrl);
}
