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
#include "stdafx.h"
#include <Wininet.h>
#include "vvWinSync.h"

#define HEARTBEAT_EVERY_N_SECONDS 60

static void ReportError(SG_context*, HWND);
static void LogHearbeatFailure(SG_context*);
static void GetConfiguration(SG_context*, volatile WorkerThreadParms*, char**, char**, SG_int64*, BOOL*);
static void DoHeartbeat(SG_context*, volatile WorkerThreadParms*, const char*, const char*, BOOL*, BOOL*);
static void DoSimpleHeartbeat(SG_context*, HWND, const char*, BOOL*, BOOL*);
static void DoSync(SG_context*, volatile WorkerThreadParms*, const char*, const char*, BOOL*);
static void SavePasswordWhenSet(SG_context*, SG_repo*, volatile WorkerThreadParms*);

void SetTooltipWithLastSyncMessage(SG_context* pCtx, DWORD lSinceLastSync, const char* szSrc, const char* prefix) 
{
	SG_string* pstrTemp = NULL;
	LPWSTR pwszTemp = NULL;

	SG_STRING__ALLOC__RESERVE(pCtx, &pstrTemp, MAX_TOOLTIP);
	
	if (prefix)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrTemp, prefix)  );

	if (lSinceLastSync < 60000)
	{
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrTemp, "Synced less than a minute ago with %s", szSrc)  );
	}
	else
	{
		SG_uint32 niceMinutes = lSinceLastSync / 60000;
		if (niceMinutes == 1)
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrTemp, "Synced 1 minute ago with %s", szSrc)  );
		else
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrTemp, "Synced %u minutes ago with %s", niceMinutes, szSrc)  );
	}

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrTemp), &pwszTemp, NULL)  );
	SetToolTip(pwszTemp);

	/* common cleanup */
fail:
	SG_NULLFREE(pCtx, pwszTemp);
	SG_STRING_NULLFREE(pCtx, pstrTemp);
}


void WorkerThread(void* pVoid)
{
	volatile WorkerThreadParms* pParms = (WorkerThreadParms*)pVoid;
	SG_context* pCtx = NULL;

	char* pszDest = NULL;
	char* pszSrc = NULL;

	SG_context__alloc(&pCtx);
	if (!pCtx)
	{
		PostQuitMessage(-2);
		return;
	}

	BOOL bHeartbeatSuccess = FALSE;
	BOOL bSyncSucceededOrDidNotRun = FALSE;
	char* pszLastSyncSrc = NULL;
	
	while (TRUE)
	{
		BOOL bGetConfigSuccess = FALSE;
		BOOL bHeartbeatOwnsTooltip = FALSE;

		DWORD lNow = GetTickCount();
		DWORD lSinceLastHeartbeat = (lNow - pParms->lLastHearbeat);
		DWORD lSinceLastSyncAttempt = (lNow - pParms->lLastSyncAttempt);
		DWORD lSinceLastSyncSuccess = (lNow - pParms->lLastSyncSuccess);

		SG_int64 lSyncIntervalMinutes;

		SG_ERR_CHECK(  GetConfiguration(pCtx, pParms, &pszDest, &pszSrc, &lSyncIntervalMinutes, &bGetConfigSuccess)  );
		if (!bGetConfigSuccess)
			goto wait;

		BOOL bTimeForHeartbeat = lSinceLastHeartbeat >= (HEARTBEAT_EVERY_N_SECONDS * 1000);
		BOOL bTimeForSync = (pParms->lLastSyncAttempt == 0) || (lSinceLastSyncAttempt >= (lSyncIntervalMinutes * 60000));

		/* Do heartbeat if it's time for heartbeat or for sync. */
		if ( bTimeForHeartbeat || bTimeForSync )
		{
			SG_ERR_CHECK(  DoSimpleHeartbeat(pCtx, pParms->hWnd, pszSrc, &bHeartbeatSuccess, &bHeartbeatOwnsTooltip)  );
			
			/* Update "last heartbeat" timestamps whether it succeeded or not. */
			pParms->lLastHearbeat = lNow;
			lSinceLastHeartbeat = 0;
		}

		/* Do sync if it's time and we're online */
		if ( bTimeForSync && bHeartbeatSuccess )
		{
			//Do the more complicated heartbeat before we sync, to make sure that things are really ok.
			SG_ERR_CHECK(  DoHeartbeat(pCtx, pParms, pszSrc, pszDest, &bHeartbeatSuccess, &bHeartbeatOwnsTooltip)  );

			if (bHeartbeatSuccess)
			{
				SG_ERR_CHECK(  DoSync(pCtx, pParms, pszSrc, pszDest, &bSyncSucceededOrDidNotRun)  );
				if (bSyncSucceededOrDidNotRun)
				{
					pParms->lLastSyncSuccess = lNow;
					lSinceLastSyncSuccess = 0;

					/* If the source repo gets changed somewhere other than our
					   config dialog, pszSrc has a repo we haven't synced with, and 
					   the "Last synced..." tooltip would be wrong. So we keep a copy
					   of the source repo that we actually synced with. */
					SG_NULLFREE(pCtx, pszLastSyncSrc);
					SG_ERR_CHECK(  SG_STRDUP(pCtx, pszSrc, &pszLastSyncSrc)  );
				}
				pParms->lLastSyncAttempt = lNow;
				lSinceLastSyncAttempt = 0;
			}
		}
		else
		{
			bSyncSucceededOrDidNotRun = TRUE; // Didn't run because it wasn't time or heartbeat failed.
		}

		/* If the sync failed, or the heartbeat said so, it set the tooltip with an error message. Don't overwrite it. */
		if (bSyncSucceededOrDidNotRun && !bHeartbeatOwnsTooltip)
		{
			/* The tooltip shows time since the last successful sync, so we intentionally update it 
			   in every loop iteration where we have a known last sync time. */
			if (pParms->lLastSyncSuccess)
			{
				if (bHeartbeatSuccess)
				{
					SG_ERR_CHECK(  SetTooltipWithLastSyncMessage(pCtx, lSinceLastSyncSuccess, pszLastSyncSrc, NULL)  );
					if ( SendMessage(pParms->hWnd, WM_APP_READY, 0, 0) )
						SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
				}
				else
				{
					SG_ERR_CHECK(  SetTooltipWithLastSyncMessage(pCtx, lSinceLastSyncSuccess, pszLastSyncSrc, "Offline\n")  );
					if ( SendMessage(pParms->hWnd, WM_APP_OFFLINE, 0, 0) )
						SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
				}
			}
			else
			{
				/* We don't know when we last synced, but we should set status/tooltip info anyway. */
				if (bHeartbeatSuccess)
				{
					SetToolTip(L"Online\nFirst sync pending.");
					if ( SendMessage(pParms->hWnd, WM_APP_READY, 0, 0) )
						SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
				}
				else
				{
					SetToolTip(L"Offline");
					if ( SendMessage(pParms->hWnd, WM_APP_OFFLINE, 0, 0) )
						SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
				}
			}
		}


		SG_NULLFREE(pCtx, pszDest);
		SG_NULLFREE(pCtx, pszSrc);

wait:
		DWORD waitResult = WaitForSingleObject(pParms->hEvent, INFINITE);
		if (waitResult == WAIT_FAILED)
			SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

		if (pParms->bShutdown)
			break;
	}

	SG_ERR_IGNORE(  SG_log__report_message(pCtx, SG_LOG__MESSAGE__VERBOSE, "Worker thread shut down cleanly.")  );

	/* This will quit the application (it shouldn't be in the fail block). */
	if ( SendMessage(pParms->hWnd, WM_APP_WORKER_DONE, 0, 0) )
		SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));

fail:
	if (SG_CONTEXT__HAS_ERR(pCtx) && !pParms->bShutdown)
		ReportError(pCtx, pParms->hWnd);

	SG_NULLFREE(pCtx, pParms->pszUser);
	SG_NULLFREE(pCtx, pParms->pszPassword);
	SG_NULLFREE(pCtx, pParms->pszSrcUlrForAuth);
	
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszSrc);
	SG_NULLFREE(pCtx, pszLastSyncSrc);
	
	SG_CONTEXT_NULLFREE(pCtx);

	/* If we did terminate abnormally by jumping to fail, let the app
	know we're not running and it can shut down whenever it's ready. */
	pParms->bShutdown = TRUE;
}

static void _discardSimpleHeartbeatBody(SG_context* pCtx,
	SG_curl* pCurl,
	char *buffer,
	SG_uint32 bufLen,
	void *pVoidState,
	SG_uint32* pLenHandled)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pCurl);
	SG_UNUSED(buffer);
	SG_UNUSED(pVoidState);

	if (pLenHandled)
		*pLenHandled = bufLen;
}

static void DoSimpleHeartbeat(
	SG_context* pCtx, 
	HWND hWnd,
	const char* pszSrc, 
	BOOL* pbSuccess,
	BOOL* pbLeaveTooltip) 
{
	SG_bool bRemote = SG_FALSE;
	LPWSTR pwszSrc = NULL;
	LPWSTR pwszErr = NULL;
	SG_string* pstrScheme = NULL;
	SG_string* pstrHost = NULL;

	SG_string * pstrHeartbeatUrl = NULL;
	SG_curl * pCurl = NULL;
	SG_context * pCtx_tmp = NULL;

	*pbSuccess = FALSE;
	*pbLeaveTooltip = FALSE;

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, pszSrc, &bRemote)  );

	if (bRemote)
	{
		URL_COMPONENTSW UrlComponents;
		WCHAR bufScheme[32];
		WCHAR bufHostName[260];
		SG_uint32 len = 0;

		SG_zero(UrlComponents);
		UrlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
		UrlComponents.dwSchemeLength = sizeof(bufScheme);
		UrlComponents.lpszScheme = bufScheme;
		UrlComponents.dwHostNameLength = sizeof(bufHostName);
		UrlComponents.lpszHostName = bufHostName;

		SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszSrc, &pwszSrc, &len)  );
		if ( !InternetCrackUrlW(pwszSrc, len, ICU_DECODE, &UrlComponents) )
		{
			SendMessage(hWnd, WM_APP_CONFIG_ERR, 0, 0);

			/* More icky string copies for no good reason that could be cleaned up. */
			static const LPWSTR ERR_BAD_URL = L"Invalid source repository URL";
			size_t len_in_chars = 0;
			if (FAILED(  StringCchLength(ERR_BAD_URL, STRSAFE_MAX_CCH, &len_in_chars)  ))
				SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
			pwszErr = (LPWSTR)malloc(sizeof(wchar_t) * (len_in_chars + 1));
			if (!pwszErr)
				SG_ERR_THROW(SG_ERR_MALLOCFAILED);
			if (FAILED(  StringCchCopy(pwszErr, len_in_chars + 1, ERR_BAD_URL)  ))
				SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

			SendMessage(hWnd, WM_APP_SHOW_BALLOON, (WPARAM)L"Veracity Sync Offline", (LPARAM)pwszErr);

			pwszErr = NULL;
			*pbLeaveTooltip = TRUE;
			*pbSuccess = FALSE;
			goto fail;
		}

		SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pstrScheme)  );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrScheme, UrlComponents.lpszScheme)  );
		SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &pstrHost)  );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrHost, UrlComponents.lpszHostName)  );

		SG_ERR_CHECK( SG_STRING__ALLOC__SZ(pCtx, &pstrHeartbeatUrl, SG_string__sz(pstrScheme))  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrHeartbeatUrl, "://")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrHeartbeatUrl, SG_string__sz(pstrHost))  );
		if (UrlComponents.nPort && UrlComponents.nPort != 80 && UrlComponents.nPort != 443)
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrHeartbeatUrl, ":%d", UrlComponents.nPort)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrHeartbeatUrl, "/ui/img/body-bg.gif?winsync")  );
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "hearbeat url: %s", SG_string__sz(pstrHeartbeatUrl))  );
		
		SG_ERR_CHECK(  SG_curl__alloc(pCtx, &pCurl)  );
		SG_ERR_CHECK(  SG_curl__setopt__sz(pCtx, pCurl, CURLOPT_URL, SG_string__sz(pstrHeartbeatUrl))  );
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_FOLLOWLOCATION, 1)  );
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_MAXREDIRS, 2)  );
		SG_ERR_CHECK(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_HTTPGET, 1)  );
		SG_ERR_CHECK(  SG_curl__set__write_cb(pCtx, pCurl, _discardSimpleHeartbeatBody, NULL)  );

		SG_curl__perform(pCtx, pCurl);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			LogHearbeatFailure(pCtx);
			SG_ERR_DISCARD;
			/* No need to check for auth here: static content won't request it. */
			SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Hearbeat: offline.")  );
			*pbSuccess = FALSE;
		}
		else
		{
			SG_int32 httpResponseCode = 0;
			SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_RESPONSE_CODE, &httpResponseCode)  );

			if (200 == httpResponseCode)
				*pbSuccess = TRUE;
			else
			{
				SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Hearbeat returned HTTP %d; setting offline.", httpResponseCode)  );
				*pbSuccess = FALSE;
			}
		}
	}
	else
	{
		/* source repo appears to be a descriptor name */
		SG_bool bExists = SG_FALSE;
		SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;

		SG_ERR_CHECK(  SG_closet__descriptors__exists__status(pCtx, pszSrc, &bExists, &status)  );
		if ( bExists && (SG_REPO_STATUS__NORMAL == status) )
			*pbSuccess = TRUE;
		else
			*pbSuccess = FALSE;
	}
	
	
fail:
	SG_NULLFREE(pCtx, pwszSrc);
	SG_NULLFREE(pCtx, pwszErr);
	SG_CURL_NULLFREE(pCtx, pCurl);
	SG_STRING_NULLFREE(pCtx, pstrScheme);
	SG_STRING_NULLFREE(pCtx, pstrHost);
	SG_STRING_NULLFREE(pCtx, pstrHeartbeatUrl);
	SG_CONTEXT_NULLFREE(pCtx_tmp);
}

static void DoHeartbeat(
	SG_context* pCtx, 
	volatile WorkerThreadParms* pParms,
	const char* pszSrc, 
	const char* pszDest,
	BOOL* pbSuccess,
	BOOL* pbLeaveTooltip) 
{
	SG_sync_client* pSyncClient = NULL;
	SG_vhash* pvhSrcRepoInfo = NULL;
	LPWSTR pwszErr = NULL;
	SG_repo* pRepoDest = NULL;
	char* pszDestRepoId = NULL;

	*pbSuccess = FALSE;
	*pbLeaveTooltip = FALSE;

	SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Checking heartbeat of %s.", pszSrc)  );
	SG_ERR_CHECK(  SG_sync_client__open(pCtx, pszSrc, pParms->pszUser, pParms->pszPassword, &pSyncClient)  );
	SG_sync_client__heartbeat(pCtx, pSyncClient, &pvhSrcRepoInfo);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_ERR_DISCARD;
			SG_NULLFREE(pCtx, pParms->pszSrcUlrForAuth);
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszSrc, (char**)&pParms->pszSrcUlrForAuth)  );
			SendMessage(pParms->hWnd, WM_APP_NEED_LOGIN, 0, 0);
		}
		else
		{
			LogHearbeatFailure(pCtx);
			SG_ERR_DISCARD;
		}
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Hearbeat: offline.")  );
	}
	else
	{
		/* Verify that repo IDs match */
		const char* pszSrcRepoIdRef = NULL;
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSrcRepoInfo, "RepoID", &pszSrcRepoIdRef)  );
		SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDest, &pRepoDest);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			if (SG_context__err_equals(pCtx, SG_ERR_SG_LIBRARY(SG_ERR_NOTAREPOSITORY)))
			{
				SG_ERR_DISCARD;

				if ( FAILED(SetToolTip(L"Veracity Sync Offline: The local destination repository does not exist.", pwszErr)) )
					SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

				SendMessage(pParms->hWnd, WM_APP_CONFIG_ERR, 0, 0);

				/* More icky string copies for no good reason that could be cleaned up. */
				static const LPWSTR ERR_NO_REPO = L"The local destination repository does not exist.";
				size_t len_in_chars = 0;
				if (FAILED(  StringCchLength(ERR_NO_REPO, STRSAFE_MAX_CCH, &len_in_chars)  ))
					SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
				pwszErr = (LPWSTR)malloc(sizeof(wchar_t) * (len_in_chars + 1));
				if (!pwszErr)
					SG_ERR_THROW(SG_ERR_MALLOCFAILED);
				if (FAILED(  StringCchCopy(pwszErr, len_in_chars + 1, ERR_NO_REPO)  ))
					SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

				SendMessage(pParms->hWnd, WM_APP_SHOW_BALLOON, (WPARAM)L"Veracity Sync Offline", (LPARAM)pwszErr);
				pwszErr = NULL;
				*pbLeaveTooltip = TRUE;
			}
			else
				SG_ERR_RETHROW;
		}
		else
		{
			/* Opened local destination repo successfully. */
			SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepoDest, &pszDestRepoId)  );
			int repoIdsMatch = strcmp(pszSrcRepoIdRef, pszDestRepoId);

			if (repoIdsMatch != 0)
			{
				if ( FAILED(SetToolTip(L"Veracity Sync Offline: The repositories must be clones.", pwszErr)) )
					SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

				SendMessage(pParms->hWnd, WM_APP_CONFIG_ERR, 0, 0);

				/* More icky string copies for no good reason that could be cleaned up. */
				static const LPWSTR ERR_NOT_CLONES = L"The repositories must be clones.";
				size_t len_in_chars = 0;
				if (FAILED(  StringCchLength(ERR_NOT_CLONES, STRSAFE_MAX_CCH, &len_in_chars)  ))
					SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
				pwszErr = (LPWSTR)malloc(sizeof(wchar_t) * (len_in_chars + 1));
				if (!pwszErr)
					SG_ERR_THROW(SG_ERR_MALLOCFAILED);
				if (FAILED(  StringCchCopy(pwszErr, len_in_chars + 1, ERR_NOT_CLONES)  ))
					SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

				SendMessage(pParms->hWnd, WM_APP_SHOW_BALLOON, (WPARAM)L"Veracity Sync Offline", (LPARAM)pwszErr);
				pwszErr = NULL;
				*pbLeaveTooltip = TRUE;
			}
			else
			{
				*pbSuccess = TRUE;
				SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Hearbeat: online.")  );
			}
		}
	}

	/* common cleanup */
fail:
	if (pwszErr)
		free(pwszErr);
	SG_ERR_IGNORE(  SG_sync_client__close_free(pCtx, pSyncClient)  );
	SG_VHASH_NULLFREE(pCtx, pvhSrcRepoInfo);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_NULLFREE(pCtx, pszDestRepoId);
}

static void DoSync(
	SG_context* pCtx, 
	volatile WorkerThreadParms* pParms,
	const char* pszSrc, 
	const char* pszDest,
	BOOL* pbSuccess) 
{
	SG_repo* pRepoDest = NULL;

	*pbSuccess = FALSE;

	if ( FAILED(SetToolTip(L"Syncing now...")) )
		SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));
	if ( SendMessage(pParms->hWnd, WM_APP_BUSY, 0, 0) )
		SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));

	/* Pull everything */
	SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Syncing from %s.", pszSrc)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDest, &pRepoDest)  );
	SG_pull__all(pCtx, pRepoDest, pszSrc, pParms->pszUser, pParms->pszPassword, NULL, NULL);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_AUTHORIZATION_REQUIRED))
		{
			SG_ERR_DISCARD;
			SG_NULLFREE(pCtx, pParms->pszSrcUlrForAuth);
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszSrc, (char**)&pParms->pszSrcUlrForAuth)  );
			SendMessage(pParms->hWnd, WM_APP_NEED_LOGIN, 0, 0);
		}
		else
		{
			ReportError(pCtx, pParms->hWnd);
			SG_ERR_DISCARD;
		}
	}
	else
	{
		SG_ERR_CHECK(  SavePasswordWhenSet(pCtx, pRepoDest, pParms)  );
		*pbSuccess = TRUE;
	}

	/* Fall through */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoDest);
}

/**
 * Does not propagate errors via pCtx, despite what you might guess, looking at the function signature.
 *
 * The "heartbeat" uses this. Heartbeat failures are expected and typical, so they're logged at the verbose level.
 */
static void LogHearbeatFailure(SG_context* pCtx)
{
	SG_string* pstrMessage = NULL;

	SG_error err = SG_context__err_to_string(pCtx, SG_TRUE, &pstrMessage);
	if (SG_IS_OK(err))
	{
		SG_ERR_IGNORE(  SG_string__insert__sz(pCtx, pstrMessage, 0, "Heartbeat failure: ")  );
		SG_ERR_IGNORE(  SG_log__report_verbose(pCtx, SG_string__sz(pstrMessage))  );
	}

	SG_STRING_NULLFREE(pCtx, pstrMessage);
}

static void GetConfiguration(
	SG_context* pCtx, 
	volatile WorkerThreadParms* pParms,
	char** ppszDest, 
	char** ppszSrc, 
	SG_int64* plSyncIntervalMinutes,
	BOOL* pbSuccess) 
{
	char* pszDest = NULL;
	char* pszSrc = NULL;
	SG_int64 lSyncIntervalMinutes = 0;
	LPWSTR pwszErr = NULL;
	SG_repo* pRepo = NULL;
	SG_string* pstrPassword = NULL;
	char* pszRepoUser = NULL;

	*pbSuccess = FALSE;

	// We deliberately look these up every time through the loop so we pick up changes while we're running.
	SG_ERR_CHECK(  GetProfileSettings(pCtx, pParms->szProfile, &pszDest, &pszSrc, &lSyncIntervalMinutes, &pwszErr)  );
	if (pwszErr)
	{
		if ( FAILED(SetToolTip(L"Veracity Sync Offline: %s", pwszErr)) )
			SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));

		SendMessage(pParms->hWnd, WM_APP_CONFIG_ERR, 0, 0);
		/* This is kind of a hack: WndProc knows more about this than it should. 
		It always frees lParam and never frees wParam, for example. 
		If WM_APP_SHOW_BALLOON gets used again, this should be better generalized. */
		SendMessage(pParms->hWnd, WM_APP_SHOW_BALLOON, (WPARAM)L"Veracity Sync Offline", (LPARAM)pwszErr);
		pwszErr = NULL;
	}
	else
	{
		/* Successfully retrieved settings */
		
		/* If the user has not been prompted for credentials, look up whoami and the saved password, if any. */
		if (!pParms->bCredentialsChanged)
		{
			SG_ERR_IGNORE(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDest, &pRepo)  );
			if (pRepo)
			{
				SG_ERR_IGNORE(  SG_user__get_username_for_repo(pCtx, pRepo, &pszRepoUser)  );
				if (pszRepoUser)
				{
					if (SG_strcmp__null(pszRepoUser, pParms->pszUser) != 0)
					{
						SG_NULLFREE(pCtx, pParms->pszUser);
						pParms->pszUser = pszRepoUser;
						pszRepoUser = NULL;
					}
					else
						SG_NULLFREE(pCtx, pszRepoUser);
				}
			}

			if (pParms->pszUser)
			{
				SG_password__get(pCtx, pszSrc, pParms->pszUser, &pstrPassword);
				SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);
				if (pstrPassword)
				{
					SG_NULLFREE(pCtx, pParms->pszPassword);
					SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrPassword, (SG_byte**)&pParms->pszPassword, NULL)  );
				}
			}
		}

		SG_RETURN_AND_NULL(pszDest, ppszDest);
		SG_RETURN_AND_NULL(pszSrc, ppszSrc);
		if (plSyncIntervalMinutes)
			*plSyncIntervalMinutes = lSyncIntervalMinutes;
		*pbSuccess = TRUE;
	}

	/* common cleanup */
fail:
	if (pwszErr)
		free(pwszErr);
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszSrc);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRING_NULLFREE(pCtx, pstrPassword);
	SG_NULLFREE(pCtx, pszRepoUser);
}

static void ReportError(SG_context* pCtx, HWND hWnd) 
{
	SG_string* pstrErr = NULL;
	LPTSTR szErr = NULL;
	SG_log__report_error__current_error(pCtx);

	/* Show the error message in a balloon notification. */
	SG_error err = SG_context__err_to_string(pCtx, SG_FALSE, &pstrErr);
	if (SG_IS_OK(err))
	{
		SG_ERR_IGNORE(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrErr), &szErr, NULL)  );
		SetToolTip(szErr);
		/* This is kind of a hack: WndProc knows more about this than it should. 
		It always frees lParam and never frees wParam, for example. 
		If WM_APP_SHOW_BALLOON gets used again, this should be better generalized. */
		SendMessage(hWnd, WM_APP_SHOW_BALLOON, (WPARAM)L"Veracity Sync Error", (LPARAM)szErr);
	}

	SendMessage(hWnd, WM_APP_OFFLINE, 0, 0);

	SG_STRING_NULLFREE(pCtx, pstrErr);
}

static void SavePasswordWhenSet(	
	SG_context* pCtx, 
	SG_repo* pRepoDest,
	volatile WorkerThreadParms* pParms)
{
	SG_string* pstrUser = NULL;
	SG_string* pstrPass = NULL;

	if (pParms->bRememberCredentials && pParms->pszUser && pParms->pszPassword)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrUser, pParms->pszUser)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrPass, pParms->pszPassword)  );
		SG_password__set(pCtx, pParms->pszSrcUlrForAuth, pstrUser, pstrPass);
		SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOTIMPLEMENTED);

		SG_ERR_CHECK(  SG_user__set_user__repo(pCtx, pRepoDest, pParms->pszUser)  );
		
		pParms->bRememberCredentials = FALSE;
	}

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pstrUser);
	SG_STRING_NULLFREE(pCtx, pstrPass);
}
