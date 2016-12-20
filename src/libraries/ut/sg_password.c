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
 * @file sg_password.c
 *
 * @details Routines for storing/retrieving saved passwords.
 */

#include <sg.h>

#if defined(WINDOWS)

//////////////////////////////////////////////////////////////////////////

#include <WinCred.h>
#include <Wininet.h>

#define KEY_PREFIX "veracity"
#define KEY_SEPARATOR ":"

/* The key for our credentials looks like this:
 * veracity:host:username
 * veracity:sourcegear.onveracity.com:ian@sourcegear.com */

static void _get_host(SG_context* pCtx, const char* szRepoSpec, SG_string** ppstrHost)
{
	LPWSTR pwszRepoSpec = NULL;
	SG_uint32 len = 0;
	URL_COMPONENTSW UrlComponents;
	SG_string* pstrHost = NULL;
	WCHAR buf[260];

	SG_zero(UrlComponents);
	UrlComponents.dwStructSize = sizeof(URL_COMPONENTS);
	UrlComponents.dwHostNameLength = sizeof(buf);
	UrlComponents.lpszHostName = buf;

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, szRepoSpec, &pwszRepoSpec, &len)  );

	if ( !InternetCrackUrlW(pwszRepoSpec, len, ICU_DECODE, &UrlComponents) )
		SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()), (pCtx, "unable to parse host from URL")  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrHost)  );
	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrHost, UrlComponents.lpszHostName)  );

	SG_RETURN_AND_NULL(pstrHost, ppstrHost);

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pwszRepoSpec);
	SG_STRING_NULLFREE(pCtx, pstrHost);
}

static void _get_key(SG_context* pCtx, const char* szRepoSpec, const char* szUserName, SG_string** ppstrKey)
{
	SG_string* pstrKey = NULL;
	SG_string* pstrHostname = NULL;

	SG_ERR_CHECK(  _get_host(pCtx, szRepoSpec, &pstrHostname)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrKey, KEY_PREFIX KEY_SEPARATOR)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrKey, SG_string__sz(pstrHostname))  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrKey, KEY_SEPARATOR)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrKey, szUserName)  );

	*ppstrKey = pstrKey;
	pstrKey = NULL;

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pstrKey);
	SG_STRING_NULLFREE(pCtx, pstrHostname);
}

SG_bool SG_password__supported()
{
	return SG_TRUE;
}

void SG_password__set(
	SG_context* pCtx,
	const char *szRepoSpec,
	SG_string *pstrUserName,
	SG_string *pstrPassword)
{
	SG_string* pstrTarget = NULL;
	LPWSTR pwszTarget = NULL;
	LPWSTR pwszPassword = NULL;
	SG_uint32 lenPassword = 0;
	LPWSTR pwszUserName = NULL;
	CREDENTIAL cred;

	SG_NULLARGCHECK_RETURN(szRepoSpec);
	SG_NULLARGCHECK_RETURN(pstrUserName);
	SG_NULLARGCHECK_RETURN(pstrPassword);
	if (!SG_string__length_in_bytes(pstrUserName))
		SG_ERR_THROW2_RETURN(SG_ERR_INVALIDARG, (pCtx, "%s", "pstrUserName is empty"));

	SG_ERR_CHECK(  _get_key(pCtx, szRepoSpec, SG_string__sz(pstrUserName), &pstrTarget)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrTarget), &pwszTarget, NULL)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrPassword), &pwszPassword, &lenPassword)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrUserName), &pwszUserName, NULL)  );

	SG_zero(cred);
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = pwszTarget;
	cred.CredentialBlob = (LPBYTE)pwszPassword;
	cred.CredentialBlobSize = lenPassword*sizeof(wchar_t);
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE; // unsupported on Windows Vista Home Basic, Windows Vista Home Premium, Windows Vista Starter, and Windows XP Home Edition
	cred.UserName = pwszUserName;

	if ( !CredWriteW(&cred, 0) )
		SG_ERR_THROW2( SG_ERR_GETLASTERROR(GetLastError()), (pCtx, "%s", "unable to save credentials") );

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pstrTarget);
	SG_NULLFREE(pCtx, pwszTarget);
	SG_NULLFREE(pCtx, pwszPassword);
	SG_NULLFREE(pCtx, pwszUserName);
}

void SG_password__get(
	SG_context *pCtx,
	const char *szRepoSpec,
	const char *szUsername,
	SG_string **ppstrPassword)
{
	SG_string* pstrTarget = NULL;
	SG_string* pstrPassword = NULL;
	LPWSTR pwszTarget = NULL;
	SG_byte* pbPassword = NULL;
	PCREDENTIAL pCred = NULL;
	BOOL result = FALSE;

	SG_NULLARGCHECK_RETURN(szRepoSpec);
	SG_NULLARGCHECK_RETURN(szUsername);
	SG_NULLARGCHECK_RETURN(ppstrPassword);

	_get_key(pCtx, szRepoSpec, szUsername, &pstrTarget);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_error err, err2;
		err2 = SG_context__get_err(pCtx, &err);
		if (SG_IS_ERROR(err2))
		{
			SG_ERR_DISCARD;
			SG_ERR_THROW(err2);
		}

		if (err & __SG_ERR__GETLASTERROR__)
			SG_ERR_DISCARD;
		else
			SG_ERR_RETHROW;
	}

	if (pstrTarget)
	{
		SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrTarget), &pwszTarget, NULL)  );

		result = CredReadW(pwszTarget, CRED_TYPE_GENERIC, 0, &pCred);
		if (!result)
		{
			DWORD err = GetLastError();
			if (err != ERROR_NOT_FOUND && err != ERROR_NO_SUCH_LOGON_SESSION)
				SG_ERR_THROW2( SG_ERR_GETLASTERROR(GetLastError()), (pCtx, "%s", "unable to retrieve saved credentials") );
		}
		else
		{
			SG_uint32 size = pCred->CredentialBlobSize+sizeof(wchar_t);
			SG_ERR_CHECK(  SG_allocN(pCtx, pCred->CredentialBlobSize+sizeof(wchar_t), pbPassword)  );
			memcpy(pbPassword, pCred->CredentialBlob, size);
			SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstrPassword)  );
			SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pstrPassword, (const LPWSTR)pbPassword)  );

			*ppstrPassword = pstrPassword;
			pstrPassword = NULL;
		}
	}

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pstrTarget);
	SG_STRING_NULLFREE(pCtx, pstrPassword);
	SG_NULLFREE(pCtx, pwszTarget);
	SG_NULLFREE(pCtx, pbPassword);
	if (pCred)
		CredFree(pCred);
}

void SG_password__forget_all(SG_context *pCtx)
{
	SG_string* pstrFilter = NULL;
	WCHAR* pwszFilter = NULL;
	DWORD numCreds = 0;
	PCREDENTIAL* paCreds = NULL;
	SG_uint32 i;

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrFilter, KEY_PREFIX)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrFilter, KEY_SEPARATOR)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrFilter, "*")  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, SG_string__sz(pstrFilter), &pwszFilter, NULL)  );

	if ( !CredEnumerateW( pwszFilter, 0, &numCreds, &paCreds) )
	{
		DWORD err = GetLastError();
		if ( ERROR_NOT_FOUND != err)
			SG_ERR_THROW2( SG_ERR_GETLASTERROR(err), (pCtx, "%s", "unable to enumerate saved credentials") );
	}

	for (i = 0; i < numCreds; i++)
	{
		if ( !CredDeleteW( paCreds[i]->TargetName, CRED_TYPE_GENERIC, 0) )
			SG_ERR_THROW2( SG_ERR_GETLASTERROR(GetLastError()), (pCtx, "%s", "unable to delete credential") );
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pwszFilter);
	SG_STRING_NULLFREE(pCtx, pstrFilter);
}

//////////////////////////////////////////////////////////////////////////

#elif defined( MAC)

//////////////////////////////////////////////////////////////////////////

#define _UINT64
#include "Security/Security.h"

#define _SG_THROW_MAC_SEC_ERROR(code)	\
	SG_STATEMENT( \
		CFStringRef st = SecCopyErrorMessageString(code, NULL);	\
		if (st) \
		{ \
			char msgbuf[1024]; \
			CFStringGetCString(st, msgbuf, sizeof(msgbuf), kCFStringEncodingUTF8); \
			CFRelease(st); \
			SG_ERR_THROW2(SG_ERR_PASSWORD_STORAGE_FAIL, (pCtx, "%s", msgbuf)); \
		} \
		else \
			SG_ERR_THROW2(SG_ERR_PASSWORD_STORAGE_FAIL, (pCtx, "Keychain access error %ld", (long)code)); \
	)

static void _sg_password__parse_url(
	SG_context *pCtx,
	const char *url,
	SG_bool *pIsValid,
	SecProtocolType *pProto,
	SG_string **ppServer,
	SG_string **ppPath,
	SG_uint32 *pPort
	)
{
	const char *slash, *colon, *ss;
	SG_string *server = NULL;
	SG_string *path = NULL;
	SG_NULLARGCHECK(url);

	*pIsValid = SG_FALSE;

	if (strncmp("http://", url, 7) == 0)
	{
		*pProto = kSecProtocolTypeHTTP;
		*pPort = 80;
		url += 7;
	}
	else if (strncmp("https://", url, 8) == 0)
	{
		*pProto = kSecProtocolTypeHTTPS;
		*pPort = 443;
		url += 8;
	}
	else
		goto fail;

	slash = strchr(url, '/');

	if (! slash)
		goto fail;

	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &server, (const SG_byte *)url, (SG_uint32)(slash - url))  );

	ss = SG_string__sz(server);
	colon = strchr(ss, ':');
	if (colon)
	{
		SG_uint32 port = atoi(colon + 1);
		SG_uint32 plen = colon - ss;
		if (port == 0)
			goto fail;

		*pPort = port;

		SG_ERR_CHECK(  SG_string__remove(pCtx, server, plen, SG_STRLEN(ss) - plen)  );
	}

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &path, "/")  );

	*ppPath = path;
	path = NULL;
	*ppServer = server;
	server = NULL;

	*pIsValid = SG_TRUE;

fail:
	SG_STRING_NULLFREE(pCtx, server);
	SG_STRING_NULLFREE(pCtx, path);
}

SG_bool SG_password__supported()
{
	return SG_TRUE;
}

void SG_password__set(
	SG_context *pCtx,
	const char *szRepoSpec,
	SG_string *pUserName,
	SG_string *pPassword)
{
	const char *username, *password;
	SG_string *path = NULL;
	SG_string *server = NULL;
	SecProtocolType proto;
	SG_uint32 port;
	SG_bool isValid = SG_FALSE;

	OSStatus saveRes, findRes;
	SecKeychainItemRef item = NULL;

	SG_NULLARGCHECK(pUserName);
	SG_NULLARGCHECK(pPassword);
	SG_NULLARGCHECK(szRepoSpec);

	username = SG_string__sz(pUserName);
	password = SG_string__sz(pPassword);

	SG_ERR_CHECK(  _sg_password__parse_url(pCtx, szRepoSpec,
		&isValid, &proto, &server, &path, &port)  );

	if (! isValid)
		SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);

	findRes = SecKeychainFindInternetPassword(
		NULL,
		SG_STRLEN( SG_string__sz(server) ), SG_string__sz(server),
		0, NULL,
		SG_STRLEN(username), username,
		SG_STRLEN( SG_string__sz(path) ), SG_string__sz(path),
		port, proto, kSecAuthenticationTypeDefault,
		NULL, NULL,
		&item);

	if (findRes == errSecSuccess)
	{
		saveRes = SecKeychainItemModifyAttributesAndData(item, NULL, SG_STRLEN(password), password);
	}
	else
	{
		saveRes = SecKeychainAddInternetPassword(
			NULL,
			SG_STRLEN( SG_string__sz(server) ), SG_string__sz(server),
			0, NULL,
			SG_STRLEN(username), username,
			SG_STRLEN( SG_string__sz(path) ), SG_string__sz(path),
			port, proto, kSecAuthenticationTypeDefault,
			SG_STRLEN(password), password,
			NULL);
	}

	if (saveRes != errSecSuccess)
		_SG_THROW_MAC_SEC_ERROR(saveRes);

fail:
	if (item)
		CFRelease(item);

	SG_STRING_NULLFREE(pCtx, path);
	SG_STRING_NULLFREE(pCtx, server);
}

void SG_password__get(
	SG_context *pCtx,
	const char *szRepoSpec,
	const char *username,
	SG_string **pPassword)
{
	SG_byte *pwdata = NULL;
	SG_uint32 pwlen;
	SG_string *password = NULL;
	SG_string *path = NULL;
	SG_string *server = NULL;
	SecProtocolType proto;
	SG_uint32 port;
	SG_bool isValid = SG_FALSE;

	OSStatus findRes;

	SG_NULLARGCHECK_RETURN(pPassword);
	*pPassword = NULL;

	SG_NULLARGCHECK(username);
	SG_NULLARGCHECK(szRepoSpec);

	SG_ERR_CHECK(  _sg_password__parse_url(pCtx, szRepoSpec,
		&isValid, &proto, &server, &path, &port)  );

	if (! isValid)
		SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);

	findRes = SecKeychainFindInternetPassword(
		NULL,
		SG_STRLEN( SG_string__sz(server) ), SG_string__sz(server),
		0, NULL,
		SG_STRLEN(username), username,
		SG_STRLEN( SG_string__sz(path) ), SG_string__sz(path),
		port, proto, kSecAuthenticationTypeDefault,
		(UInt32 *)&pwlen, (void **)&pwdata,
		NULL);

	if (findRes == errSecItemNotFound ||
		findRes == errSecInteractionNotAllowed)
		goto fail;
	else if (findRes != errSecSuccess)
		_SG_THROW_MAC_SEC_ERROR(findRes);

	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &password, pwdata, pwlen)  );
	*pPassword = password;
	password = NULL;

fail:
	if (pwdata)
		SecKeychainItemFreeContent(NULL, pwdata);
	SG_STRING_NULLFREE(pCtx, path);
	SG_STRING_NULLFREE(pCtx, server);
	SG_STRING_NULLFREE(pCtx, password);
}

//////////////////////////////////////////////////////////////////////////

#elif defined(LINUX) && defined(VVGNOMEKEYRING)


#include "gnome-keyring.h"

#define 		_SG_THROW_LINUX_SEC_ERROR(code)	\
	SG_STATEMENT( \
		const char *st = gnome_keyring_result_to_message(code);    \
		if (st) \
		{ \
			SG_ERR_THROW2(SG_ERR_PASSWORD_STORAGE_FAIL, (pCtx, "%s", st)); \
		} \
		else \
			SG_ERR_THROW2(SG_ERR_PASSWORD_STORAGE_FAIL, (pCtx, "Keychain access error %ld", (long)code)); \
	)

static void _sg_password__parse_url(
	SG_context *pCtx,
	const char *url,
	SG_bool *pIsValid,
	SG_string **ppProto,
	SG_string **ppServer,
	SG_string **ppPath,
	SG_uint32 *pPort
	)
{
	const char *slash, *colon, *ss;
	SG_string *server = NULL;
	SG_string *path = NULL;
	SG_string *proto = NULL;
	SG_NULLARGCHECK(url);

	*pIsValid = SG_FALSE;

	if (strncmp("http://", url, 7) == 0)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &proto, "http")  );
		*pPort = 80;
		url += 7;
	}
	else if (strncmp("https://", url, 8) == 0)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &proto, "https")  );
		*pPort = 443;
		url += 8;
	}
	else
		goto fail;

	slash = strchr(url, '/');

	if (! slash)
		goto fail;

	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &server, (const SG_byte *)url, (SG_uint32)(slash - url))  );

	ss = SG_string__sz(server);
	colon = strchr(ss, ':');
	if (colon)
	{
		SG_uint32 port = atoi(colon + 1);
		SG_uint32 plen = colon - ss;
		if (port == 0)
			goto fail;

		*pPort = port;

		SG_ERR_CHECK(  SG_string__remove(pCtx, server, plen, SG_STRLEN(ss) - plen)  );
	}

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &path, "/")  );

	*ppPath = path;
	path = NULL;
	*ppServer = server;
	server = NULL;
	*ppProto = proto;
	proto = NULL;

	*pIsValid = SG_TRUE;

fail:
	SG_STRING_NULLFREE(pCtx, server);
	SG_STRING_NULLFREE(pCtx, path);
	SG_STRING_NULLFREE(pCtx, proto);
}


SG_bool SG_password__supported()
{
	SG_bool	haveDaemon = (SG_bool)gnome_keyring_is_available();

	if (haveDaemon)
	{
		char *keyring = NULL;

		GnomeKeyringResult res = gnome_keyring_get_default_keyring_sync(&keyring);

		if (keyring)
			g_free(keyring);

		if (res == GNOME_KEYRING_RESULT_OK)
			return( SG_TRUE );
	}

	return SG_FALSE;
}

void SG_password__set(
	SG_context* pCtx,
	const char *szRepoSpec,
	SG_string *pUserName,
	SG_string *pPassword)
{
	const char *username;
	const char *password;
	SG_string *path = NULL;
	SG_string *server = NULL;
	SG_string *proto = NULL;
	SG_uint32 port;
	SG_bool isValid = SG_FALSE;
	GnomeKeyringResult saveRes = 0;
	guint32 itemid = 0;

	if (! SG_password__supported())
		goto fail;

	SG_NULLARGCHECK(pUserName);
	SG_NULLARGCHECK(pPassword);
	SG_NULLARGCHECK(szRepoSpec);

	username = SG_string__sz(pUserName);
	password = SG_string__sz(pPassword);

	SG_ERR_CHECK(  _sg_password__parse_url(pCtx, szRepoSpec,
		&isValid, &proto, &server, &path, &port)  );

	if (! isValid)
		SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);

	saveRes = gnome_keyring_set_network_password_sync(
		NULL,
		username, NULL,
		SG_string__sz(server), SG_string__sz(path),
		SG_string__sz(proto), NULL, (guint32)port,
		password,
		&itemid);

	if ((saveRes != GNOME_KEYRING_RESULT_OK) && (saveRes != GNOME_KEYRING_RESULT_NO_MATCH) && (saveRes != GNOME_KEYRING_RESULT_CANCELLED))
		_SG_THROW_LINUX_SEC_ERROR(saveRes);

fail:
	SG_STRING_NULLFREE(pCtx, path);
	SG_STRING_NULLFREE(pCtx, server);
	SG_STRING_NULLFREE(pCtx, proto);
}

void SG_password__get(
	SG_context *pCtx,
	const char *szRepoSpec,
	const char *username,
	SG_string **ppstrPassword)
{
	SG_string *password = NULL;
	SG_string *path = NULL;
	SG_string *server = NULL;
	SG_string *proto = NULL;
	SG_uint32 port;
	SG_bool isValid = SG_FALSE;
	GnomeKeyringResult saveRes = 0;
	GList *results = NULL;
	guint count = 0;

	SG_NULLARGCHECK(username);
	SG_NULLARGCHECK(ppstrPassword);
	SG_NULLARGCHECK(szRepoSpec);

	if (! SG_password__supported())
		goto fail;

	SG_ERR_CHECK(  _sg_password__parse_url(pCtx, szRepoSpec,
		&isValid, &proto, &server, &path, &port)  );

	if (! isValid)
		SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);

	saveRes = gnome_keyring_find_network_password_sync(
		username, NULL,
		SG_string__sz(server), SG_string__sz(path),
		SG_string__sz(proto), NULL, (guint32)port,
		&results);

	if ((saveRes != GNOME_KEYRING_RESULT_OK) && (saveRes != GNOME_KEYRING_RESULT_NO_MATCH) && (saveRes != GNOME_KEYRING_RESULT_CANCELLED))
		_SG_THROW_LINUX_SEC_ERROR(saveRes);

	if (results != NULL)
		count = g_list_length(results);

	if (count > 0)
	{
		const char *pw = "";
		GnomeKeyringNetworkPasswordData *entry = g_list_nth_data(results, 0);

		SG_ASSERT(entry != NULL);

		if (entry->password)
			pw = entry->password;

		SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &password, pw)  );
	}

	*ppstrPassword = password;
	password = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, path);
	SG_STRING_NULLFREE(pCtx, server);
	SG_STRING_NULLFREE(pCtx, proto);
	SG_STRING_NULLFREE(pCtx, password);
	if (results)
		gnome_keyring_network_password_list_free(results);
}

//////////////////////////////////////////////////////////////////////////

#else

//////////////////////////////////////////////////////////////////////////

SG_bool SG_password__supported()
{
	return SG_FALSE;
}

void SG_password__set(
	SG_context* pCtx,
	const char *szRepoSpec,
	SG_string *pstrUserName,
	SG_string *pstrPassword)
{
	SG_UNUSED(szRepoSpec);
	SG_UNUSED(pstrUserName);
	SG_UNUSED(pstrPassword);
	SG_ERR_THROW_RETURN(SG_ERR_NOTIMPLEMENTED);
}

void SG_password__get(
	SG_context *pCtx,
	const char *szRepoSpec,
	const char *szUsername,
	SG_string **ppstrPassword)
{
	SG_UNUSED(szRepoSpec);
	SG_UNUSED(szUsername);
	SG_UNUSED(ppstrPassword);
	SG_ERR_THROW_RETURN(SG_ERR_NOTIMPLEMENTED);
}

//////////////////////////////////////////////////////////////////////////

#endif
