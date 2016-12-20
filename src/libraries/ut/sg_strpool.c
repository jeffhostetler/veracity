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

typedef struct _sg_strsubpool
{
	SG_uint32 count;
	SG_uint32 space;
	char* pBytes;
	struct _sg_strsubpool* pNext;
} sg_strsubpool;

struct _SG_strpool
{
	SG_uint32 subpool_space;
	SG_uint32 count_bytes;
	SG_uint32 count_subpools;
	SG_uint32 count_strings;
	sg_strsubpool *pHead;
};

void sg_strsubpool__free(SG_context * pCtx, sg_strsubpool *psp);

void sg_strsubpool__alloc(
        SG_context* pCtx,
        SG_uint32 space,
        sg_strsubpool* pNext,
        sg_strsubpool** ppNew
        )
{
	sg_strsubpool* pThis = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pThis)  );

	pThis->space = space;

	SG_ERR_CHECK(  SG_allocN(pCtx, space,pThis->pBytes)  );

	pThis->pNext = pNext;
	pThis->count = 0;

	*ppNew = pThis;
	return;

fail:
	SG_ERR_IGNORE(  sg_strsubpool__free(pCtx, pThis)  );
}

void sg_strsubpool__free(SG_context * pCtx, sg_strsubpool *psp)
{
	while (psp)
	{
		sg_strsubpool * pspNext = psp->pNext;

		SG_NULLFREE(pCtx, psp->pBytes);
		SG_NULLFREE(pCtx, psp);

		psp = pspNext;
	}
}

void SG_strpool__alloc(SG_context* pCtx, SG_strpool** ppResult, SG_uint32 subpool_space)
{
	SG_strpool* pThis = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pThis)  );

	SG_ERR_CHECK(  sg_strsubpool__alloc(pCtx, subpool_space,NULL,&pThis->pHead)  );

	pThis->subpool_space = subpool_space;
	pThis->count_subpools = 1;

	*ppResult = pThis;

	return;

fail:
	SG_STRPOOL_NULLFREE(pCtx, pThis);
}

void            SG_strpool__free(SG_context * pCtx, SG_strpool* pPool)
{
	if (!pPool)
	{
		return;
	}

	sg_strsubpool__free(pCtx, pPool->pHead);
	SG_NULLFREE(pCtx, pPool);
}

static void sg_strpool__get_space(
        SG_context* pCtx,
        SG_strpool* pPool,
        SG_uint32 len,
        char** ppOut
        )
{
	char* pResult = NULL;
    SG_uint32 mod = 0;

    // TODO review this constant.  for now, I'm just aiming for 4-byte ptr
    // alignment

#define MY_PTR_ALIGN 4

    mod = len % MY_PTR_ALIGN;
    if (mod)
    {
        len += (MY_PTR_ALIGN - mod);
    }

	if ((pPool->pHead->count + len) > pPool->pHead->space)
	{
		SG_uint32 space = pPool->subpool_space;
		sg_strsubpool* psp = NULL;

		if (len > space)
		{
			space = len;
		}

		SG_ERR_CHECK_RETURN(  sg_strsubpool__alloc(pCtx, space, pPool->pHead, &psp)  );

		pPool->pHead = psp;
		pPool->count_subpools++;
	}

	pResult = pPool->pHead->pBytes + pPool->pHead->count;
	pPool->pHead->count += len;
	pPool->count_bytes += len;
	pPool->count_strings++;

	*ppOut = pResult;
}

void SG_strpool__add__buflen__binary(
        SG_context* pCtx,
        SG_strpool* pPool,
        const char* pIn,
        SG_uint32 len,
        const char** ppOut
        )
{
	char* pResult = NULL;

    SG_ERR_CHECK_RETURN(  sg_strpool__get_space(pCtx, pPool, len, &pResult)  );

	if (pIn)
	{
		memcpy(pResult, pIn, len);
	}

	*ppOut = pResult;
}

void SG_strpool__add__len(
        SG_context* pCtx,
        SG_strpool* pPool,
        SG_uint32 len,
        const char** ppOut
        )
{
	SG_ERR_CHECK_RETURN(  SG_strpool__add__buflen__binary(pCtx, pPool, NULL, len, ppOut)  );
}

void SG_strpool__add__buflen__sz(
        SG_context* pCtx,
        SG_strpool* pPool,
        const char* pszIn,
        SG_uint32 len_spec,
        const char** ppszOut
        )
{
	char* pResult = NULL;
	SG_uint32 len;

	if (len_spec == 0)
	{
		len = SG_STRLEN(pszIn);
	}
	else
	{
		const char* p = pszIn;
		len = 0;
		while (*p && (len < len_spec))
		{
			p++;
			len++;
		}
	}

    SG_ERR_CHECK_RETURN(  sg_strpool__get_space(pCtx, pPool, len+1, &pResult)  );

    memcpy(pResult, pszIn, len);
    pResult[len] = 0;

	*ppszOut = pResult;
}

void SG_strpool__add__sz(
        SG_context* pCtx,
        SG_strpool* pPool,
        const char* pszIn,
        const char** ppszOut
        )
{
	SG_ERR_CHECK_RETURN(  SG_strpool__add__buflen__binary(pCtx, pPool, pszIn, (1 + SG_STRLEN(pszIn)), ppszOut)  );
}

#ifndef SG_IOS

void SG_strpool__add__jsstring(
	SG_context* pCtx,
	SG_strpool* pPool,
	JSContext *cx,
	JSString *str,
	const char **ppOut
	)
{
	char *pOut = NULL;
	size_t len = 0;

	SG_ASSERT(pCtx);	
	SG_NULLARGCHECK_RETURN(pPool);
	SG_NULLARGCHECK_RETURN(cx);
	SG_NULLARGCHECK_RETURN(str);
	SG_NULLARGCHECK_RETURN(ppOut);
	
	len = JS_GetStringEncodingLength(cx, str);
	SG_ARGCHECK_RETURN(len+1<=SG_UINT32_MAX, str);
	SG_ERR_CHECK_RETURN(  SG_strpool__add__buflen__binary(pCtx, pPool, NULL, (SG_uint32)(len+1), (const char**)&pOut)  );
#if !defined(DEBUG) // RELEASE
	(void)JS_EncodeStringToBuffer(str, pOut, len);
#else // DEBUG
	// Same as RELEASE, but asserts length returned by JS_EncodeStringToBuffer()
	// was the same as length returned by JS_GetStringEncodingLength(). I think
	// this is supposed to be true, but the documentation is a bit unclear. I'm
	// interpreting the first sentence to mean "the length of the specified
	// string in bytes *when UTF-8 encoded*, regardless of its *current*
	// encoding." in this exerpt from the documentation:
	//
	// "JS_GetStringEncodingLength() returns the length of the specified string
	// in bytes, regardless of its encoding. You can use this value to create a
	// buffer to encode the string into using the JS_EncodeStringToBuffer()
	// function, which fills the specified buffer with up to length bytes of the
	// string in UTF-8 format. It returns the length of the whole string
	// encoding or -1 if the string can't be encoded as bytes. If the returned
	// value is greater than the length you specified, the string was
	// truncated."
	// -- https://developer.mozilla.org/en/JS_GetStringBytes (2012-07-13)
	{
		size_t len2 = 0;
		len2 = JS_EncodeStringToBuffer(str, pOut, len);
		SG_ASSERT(len2==len);
	}
#endif
	pOut[len]='\0';
	*ppOut = pOut;
}

#endif


