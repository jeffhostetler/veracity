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
#include "vvWinSync.h"

void GetProfileSettings(
	SG_context* pCtx, 
	const char* szProfile,
	char** ppszDest, 
	char** ppszSrc, 
	SG_int64* plSyncIntervalMinutes,
	LPWSTR* ppwszErr)
{
	static const LPWSTR ERR_NO_DEST = L"No destination repository is configured.";
	static const LPWSTR ERR_NO_SRC = L"No source repository is configured.";

	char* pszDest = NULL;
	char* pszSrc = NULL;
	SG_variant* pvMinutes = NULL;
	SG_string* pstrConfigPath = NULL;
	SG_uint64 lMinutes;
	LPWSTR pwszErr = NULL;

	/* It's kind of icky, but we copy these error strings so the balloon code that displays them doesn't have to
	   be very smart and can always free the message string.  (Generic unhandled errors aren't constant, so it has
	   to free those.) I'm not sure making the balloon code more complexso this can be simpler is a net win, so I'm 
	   leaving this be. But it is icky. */

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pstrConfigPath, "%s/%s/%s", CONFIG_ROOT, szProfile, CONFIG_DEST)  );
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_string__sz(pstrConfigPath), NULL, &pszDest, NULL)  );
	SG_ERR_CHECK(  SG_string__clear(pCtx, pstrConfigPath)  );
	if (!pszDest || !pszDest[0])
	{
		size_t len_in_chars = 0;
		if (FAILED(  StringCchLength(ERR_NO_DEST, STRSAFE_MAX_CCH, &len_in_chars)  ))
			SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
		pwszErr = (LPWSTR)malloc(sizeof(wchar_t) * (len_in_chars + 1));
		if (FAILED(  StringCchCopy(pwszErr, len_in_chars + 1, ERR_NO_DEST)  ))
			SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
	}

	SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrConfigPath, "%s/%s/%s", CONFIG_ROOT, szProfile, CONFIG_SRC)  );
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_string__sz(pstrConfigPath), NULL, &pszSrc, NULL)  );
	SG_ERR_CHECK(  SG_string__clear(pCtx, pstrConfigPath)  );
	if (!pwszErr)
	{
		if (!pszSrc || !pszSrc[0])
		{
			size_t len_in_chars = 0;
			if (FAILED(  StringCchLength(ERR_NO_SRC, STRSAFE_MAX_CCH, &len_in_chars)  ))
				SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
			pwszErr = (LPWSTR)malloc(sizeof(wchar_t) * (len_in_chars + 1));
			if (FAILED(  StringCchCopy(pwszErr, len_in_chars + 1, ERR_NO_SRC)  ))
				SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
		}
	}

	SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrConfigPath, "%s/%s/%s", CONFIG_ROOT, szProfile, CONFIG_INTERVAL)  );
	SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, SG_string__sz(pstrConfigPath), NULL, &pvMinutes, NULL)  );

	if (!pvMinutes)
		lMinutes = DEFAULT_SYNC_EVERY_N_MINUTES;
	else
		SG_ERR_CHECK(  SG_variant__get__uint64(pCtx, pvMinutes, &lMinutes)  );

	if (plSyncIntervalMinutes)
		*plSyncIntervalMinutes = lMinutes;
	SG_RETURN_AND_NULL(pszDest, ppszDest);
	SG_RETURN_AND_NULL(pszSrc, ppszSrc);

	SG_RETURN_AND_NULL(pwszErr, ppwszErr);

	/* common cleanup */
fail:
	SG_NULLFREE(pCtx, pszDest);
	SG_NULLFREE(pCtx, pszSrc);
	SG_VARIANT_NULLFREE(pCtx, pvMinutes);
	SG_STRING_NULLFREE(pCtx, pstrConfigPath);

	if (pwszErr)
		free(pwszErr);
}

void SetProfileSettings(
	SG_context* pCtx, 
	const char* szProfile,
	const char* pszDest, 
	const char* pszSrc, 
	SG_int64 lSyncIntervalMinutes)
{
	SG_string* pstrConfigPath = NULL;

	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pstrConfigPath, "%s/%s/%s", CONFIG_ROOT, szProfile, CONFIG_DEST)  );
	SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, SG_string__sz(pstrConfigPath), pszDest)  );
	SG_ERR_CHECK(  SG_string__clear(pCtx, pstrConfigPath)  );

	SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrConfigPath, "%s/%s/%s", CONFIG_ROOT, szProfile, CONFIG_SRC)  );
	SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, SG_string__sz(pstrConfigPath), pszSrc)  );
	SG_ERR_CHECK(  SG_string__clear(pCtx, pstrConfigPath)  );

	SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrConfigPath, "%s/%s/%s", CONFIG_ROOT, szProfile, CONFIG_INTERVAL)  );
	SG_ERR_CHECK(  SG_localsettings__update__int64(pCtx, SG_string__sz(pstrConfigPath), lSyncIntervalMinutes)  );

	/* common cleanup */
fail:
	SG_STRING_NULLFREE(pCtx, pstrConfigPath);
}
