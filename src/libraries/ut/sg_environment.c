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

#if defined(WINDOWS)
void SG_environment__get__str(SG_context* pCtx, const char* pszUtf8Name, SG_string** ppstrValue, SG_uint32* piLenStr)
{
	size_t requiredSize;
	wchar_t* pwszName = NULL;
	wchar_t* pwszTempVal = NULL;
	errno_t err;
	SG_string* pString = NULL;

	SG_NULLARGCHECK_RETURN(ppstrValue);

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszUtf8Name, &pwszName, NULL)  );
	err = _wgetenv_s(&requiredSize, NULL, 0, pwszName);
	if (err)
		SG_ERR_THROW(SG_ERR_ERRNO(err));

	if (requiredSize)
	{
		SG_ERR_CHECK_RETURN(  SG_alloc(pCtx, 1, (SG_uint32)(requiredSize * sizeof(wchar_t)), &pwszTempVal)  );
		err = _wgetenv_s(&requiredSize, pwszTempVal, requiredSize, pwszName);
		if (err)
			SG_ERR_THROW(SG_ERR_ERRNO(err));

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pString)  );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pString, pwszTempVal)  );
	}

	if (piLenStr)
	{
		if (pString)
			*piLenStr = SG_string__length_in_bytes(pString);
		else
			*piLenStr = 0;
	}

	*ppstrValue = pString;
	pString = NULL;

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_NULLFREE(pCtx, pwszName);
	SG_NULLFREE(pCtx, pwszTempVal);
}
#endif /* defined (WINDOWS) */

#if defined(MAC) || defined(LINUX)
void SG_environment__get__str(SG_context* pCtx, const char* pszUtf8Name, SG_string** ppstrValue, SG_uint32* piLenStr)
{
	char * szEnv;
	SG_string * pStringUtf8 = NULL;

	SG_NULLARGCHECK_RETURN(ppstrValue);

	szEnv = getenv(pszUtf8Name);
	if (szEnv)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pStringUtf8)  );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__sz(pCtx,pStringUtf8,szEnv)  );
	}

	if (piLenStr)
	{
		if (pStringUtf8)
			*piLenStr = SG_string__length_in_bytes(pStringUtf8);
		else
			*piLenStr = 0;
	}

	*ppstrValue = pStringUtf8;

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pStringUtf8);
}
#endif /* defined(MAC) || defined(LINUX) */

#if defined(WINDOWS)
void SG_environment__set__sz(SG_context *pCtx, const char *pszUtf8Name, const char* pszUtf8Value)
{
	wchar_t* pwszName = NULL;
	wchar_t* pwszVal = NULL;
	errno_t err;

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszUtf8Name, &pwszName, NULL)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszUtf8Value, &pwszVal, NULL)  );

	err = _wputenv_s(pwszName, pwszVal);
	if (err)
		SG_ERR_THROW(SG_ERR_ERRNO(err));

fail:
	SG_NULLFREE(pCtx, pwszName);
	SG_NULLFREE(pCtx, pwszVal);
}
#endif /* defined(WINDOWS) */

#if defined(MAC) || defined(LINUX)
void SG_environment__set__sz(SG_context* pCtx, const char* pszUtf8Name, const char* pszUtf8Value)
{
	SG_NULLARGCHECK_RETURN(pszUtf8Name);
	SG_NULLARGCHECK_RETURN(pszUtf8Value);

	if (setenv(pszUtf8Name, pszUtf8Value, 1) != 0)
		SG_ERR_THROW_RETURN( SG_ERR_ERRNO(errno)  );

	return;
}
#endif /* defined(MAC) || defined(LINUX) */

