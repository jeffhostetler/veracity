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

// sg_misc_utils.c
// misc utils.
//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

void SG_strdup(SG_context * pCtx, const char * szSrc, char ** pszCopy)
{
	SG_uint32 len;
	char * szCopy;

	SG_NULLARGCHECK_RETURN(szSrc);
	SG_NULLARGCHECK_RETURN(pszCopy);

	len = SG_STRLEN(szSrc);
	SG_ERR_CHECK_RETURN(  SG_malloc(pCtx, len+1, &szCopy)  );

	memcpy(szCopy,szSrc,len+1);
	*pszCopy = szCopy;
}

void SG_strcpy(SG_context * pCtx, char * szDest, SG_uint32 lenDest, const char * szSrc)
{
	// strcpy() without the _CRT_SECURITY_NO_WARNINGS stuff on Windows.
	// also hides differences between Windows' strcpy_s() and Linux/Mac's
	// strcpy().
	//
	// copy upto the limit -- BUT ALWAYS NULL TERMINATE.
	// then null-out the rest of the buffer.
	//
	// WARNING: input buffers must not overlap.

	SG_uint32 lenSrc;
	SG_uint32 lenToCopy;

	SG_NULLARGCHECK_RETURN(szDest);
	SG_ARGCHECK_RETURN( lenDest>=1 , lenDest );
	SG_NULLARGCHECK_RETURN(szSrc);

	lenSrc = SG_STRLEN(szSrc);

	if (lenSrc < lenDest)
	{
		lenToCopy = lenSrc;
	}
	else
	{
		lenToCopy = lenDest - 1;

		// TODO shouldn't this THROW an error?
		(void)SG_context__err__generic(pCtx, SG_ERR_BUFFERTOOSMALL, __FILE__, __LINE__);
	}

    /* the buffers must not overlap, but they can be adjacent to each other */
	SG_ASSERT( (szDest+lenDest <= szSrc) || (szSrc+lenSrc <= szDest) );

#if defined(WINDOWS)
	memcpy_s(szDest,lenDest,szSrc,lenToCopy);
#else
	memcpy(szDest,szSrc,lenToCopy);
#endif
#if 1
    /* TODO why null out the rest of the buffer?  why not just add a single trailing zero?
	 * Jeff says: I did this because one (I don't remember which) of the MSFT _s routines
	 *            was documented as doing this and I thought we should have the same
	 *            behavior on all platforms.  We can change it.
	 */
	memset(szDest+lenToCopy,0,(lenDest-lenToCopy));
#else
    szDest[lenToCopy] = 0;
#endif
}

void SG_strncpy(SG_context * pCtx, char * szDest, SG_uint32 lenDest, const char * szSrc, SG_uint32 uCount)
{
	SG_uint32 lenSrc;
	SG_uint32 lenToCopy;

	SG_NULLARGCHECK_RETURN(szDest);
	SG_ARGCHECK_RETURN(lenDest >= 1, lenDest);
	SG_NULLARGCHECK_RETURN(szSrc);

	// check the length of the source buffer
	lenSrc = SG_STRLEN(szSrc);

	// make sure the buffers don't overlap
	SG_ASSERT( (szDest+lenDest <= szSrc) || (szSrc+lenSrc <= szDest) );

	// figure out how much we actually want to copy
	lenToCopy = SG_MIN(lenSrc, uCount);
	if (lenToCopy >= lenDest)
	{
		// set a buffer too small error, but go ahead and copy as much as we can fit
		lenToCopy = lenDest - 1;
		(void)SG_context__err__generic(pCtx, SG_ERR_BUFFERTOOSMALL, __FILE__, __LINE__);
	}

	// copy the data
#if defined(WINDOWS)
	memcpy_s(szDest, lenDest, szSrc, lenToCopy);
#else
	memcpy(szDest, szSrc, lenToCopy);
#endif

	// set the rest of the destination buffer to a NULL terminator
	memset(szDest+lenToCopy, 0, lenDest-lenToCopy);
}

void SG_strcat(SG_context * pCtx, char * bufDest, SG_uint32 lenBufDest, const char * szSrc)
{
	// strcat() without the _CRT_SECURITY_NO_WARNINGS stuff on Windows.
	//
	// lenBufDest is the defined length of the target buffer to help
	// with buffer-overrun issues and make Windows happy.
	//

	SG_uint32 offsetDest;

	SG_NULLARGCHECK_RETURN(bufDest);
	SG_NULLARGCHECK_RETURN(szSrc);

	offsetDest = SG_STRLEN(bufDest);

	if (lenBufDest > offsetDest)
		SG_ERR_CHECK_RETURN(  SG_strcpy(pCtx, bufDest + offsetDest, lenBufDest - offsetDest, szSrc)  );
}

void SG_sprintf(SG_context * pCtx, char * pBuf, SG_uint32 lenBuf, const char * szFormat, ...)
{
	// a portable version of sprintf, sprintf_s, snprintf that won't overrun
	// the buffer and which produces cross-platform consistent results.
	//
	// we return an error if we had to truncate the buffer or if we have a
	// formatting error.
	//
	// we guarantee that the buffer will be null terminated.

	va_list ap;

	SG_NULLARGCHECK_RETURN(pBuf);
	SG_ARGCHECK_RETURN( lenBuf>0 , lenBuf );
	SG_NULLARGCHECK_RETURN(szFormat);

	va_start(ap,szFormat);
#if defined(MAC) || defined(LINUX)
	{
		// for vsnprintf() return value is size of string excluding final NULL.
		int lenResult = vsnprintf(pBuf,(size_t)lenBuf,szFormat,ap);
		if (lenResult < 0)
			(void)SG_context__err__generic(pCtx, SG_ERR_ERRNO(errno), __FILE__, __LINE__);
		else if (lenResult >= (int)lenBuf)
			(void)SG_context__err__generic(pCtx, SG_ERR_BUFFERTOOSMALL, __FILE__, __LINE__);
	}
#endif
#if defined(WINDOWS)
	{
		// for vsnprintf_s() we get additional parameter checking and (when 'count'
		// is not _TRUNCATE) the return value is -1 for any error or overflow.  i
		// think they also clear the buffer.  the do not set errno on overflow (when
		// _TRUNCATE).
		int lenResult = vsnprintf_s(pBuf,(size_t)lenBuf,(size_t)lenBuf-1,szFormat,ap);
		if (lenResult < 0)
		{
			if (errno == 0)
				(void)SG_context__err__generic(pCtx, SG_ERR_BUFFERTOOSMALL, __FILE__, __LINE__);
			else
				(void)SG_context__err__generic(pCtx, SG_ERR_ERRNO(errno), __FILE__, __LINE__);
		}
		else
		{
			SG_ASSERT(lenResult+1 <= (int)lenBuf);	// formatted result+null <= lenbuf
		}
	}
#endif
	va_end(ap);

	// clear the buffer on error.
	if (SG_CONTEXT__HAS_ERR(pCtx))
		pBuf[0] = 0;
}

void SG_sprintf_truncate(
	SG_context * pCtx,
	char * pBuf,
	SG_uint32 lenBuf,
	const char * szFormat,
	...
	)
{
	va_list ap;

	va_start(ap,szFormat);
	SG_ERR_CHECK(  SG_vsprintf_truncate(pCtx, pBuf, lenBuf, szFormat, ap)  );
	// Fall through
fail:
	va_end(ap);
}

void SG_vsprintf_truncate(
	SG_context * pCtx,
	char * pBuf,
	SG_uint32 lenBuf,
	const char * szFormat,
	va_list ap
	)
{
	int lenResult;

	SG_NULLARGCHECK_RETURN(pBuf);
	SG_ARGCHECK_RETURN( lenBuf>0 , lenBuf );
	SG_NULLARGCHECK_RETURN(szFormat);

	// expand arguments into buffer, truncate if necessary.

#if defined(MAC) || defined(LINUX)
	// for vsnprintf() return value is size of string excluding final NULL.
	lenResult = vsnprintf(pBuf,(size_t)lenBuf,szFormat,ap);
	if (lenResult < 0)
		SG_ERR_THROW(SG_ERR_ERRNO(errno));
#endif
#if defined(WINDOWS)
	lenResult = vsnprintf_s(pBuf,(size_t)lenBuf,_TRUNCATE,szFormat,ap);

	if ((lenResult < 0) && (errno != 0))  // formatter error??
		SG_ERR_THROW(SG_ERR_ERRNO(errno));
#endif

	return;
fail:
	return;
}

SG_int32 SG_strnicmp(const char * sz1, const char * sz2, SG_uint32 len)
{
#if defined(WINDOWS)
	return _strnicmp(sz1, sz2, len);
#else
	return strncasecmp(sz1, sz2, len);
#endif
}

SG_int32 SG_stricmp(const char * sz1, const char * sz2)
{
#if defined(WINDOWS)
	return _stricmp(sz1, sz2);
#else
	return strcasecmp(sz1, sz2);
#endif
}
SG_int32 SG_memicmp(const SG_byte * sz1, const SG_byte * sz2, SG_uint32 numBytes)
{
#if defined(WINDOWS)
	return _memicmp(sz1, sz2, numBytes);
#else
	return strncasecmp((const char *)sz1, (const char *)sz2, numBytes);
#endif
}

SG_int32 SG_strcmp__null(const char* szLeft, const char* szRight)
{
	if (szLeft == NULL && szRight == NULL)
	{
		return 0;
	}
	else if (szLeft == NULL)
	{
		return -1;
	}
	else if (szRight == NULL)
	{
		return 1;
	}
	else
	{
		return strcmp(szLeft, szRight);
	}
}

SG_int32 SG_stricmp__null(const char* szLeft, const char* szRight)
{
	if (szLeft == NULL && szRight == NULL)
	{
		return 0;
	}
	else if (szLeft == NULL)
	{
		return -1;
	}
	else if (szRight == NULL)
	{
		return 1;
	}
	else
	{
		return SG_stricmp(szLeft, szRight);
	}
}

SG_bool SG_ascii__is_valid(const char * p)
{
	if( p == NULL )
		return SG_FALSE;
	while( *p != '\0' )
	{
		if( *p < 0 )
			return SG_FALSE;
		++p;
	}
	return SG_TRUE;
}

void SG_ascii__find__char(SG_context * pCtx, const char * sz, char c, SG_uint32 * pResult)
{
	const char * p = sz;

	SG_NULLARGCHECK_RETURN(sz);
	SG_ARGCHECK_RETURN( c>=0 , c );

	while( *p != '\0' )
	{
		if( *p < 0 )
		{
			(void)SG_context__err(pCtx, SG_ERR_INVALIDARG, __FILE__, __LINE__, "sz is invalid: it contains non-ASCII characters.");
			return;
		}
		if( *p==c )
		{
			*pResult = (SG_uint32)(p-sz);
			return;
		}
		++p;
	}

	*pResult = SG_UINT32_MAX;
}

void SG_ascii__substring(SG_context * pCtx, const char *szSrc, SG_uint32 start, SG_uint32 len, char ** pszCopy)
{
	char * szCopy;

	SG_NULLARGCHECK_RETURN(szSrc);
	SG_NULLARGCHECK_RETURN(pszCopy);

	SG_ERR_CHECK_RETURN(  SG_malloc(pCtx, len+1, &szCopy)  );

	memcpy(szCopy,szSrc+start,(size_t)len);
	szCopy[len] = '\0';
	*pszCopy = szCopy;
}

void SG_ascii__substring__to_end(SG_context * pCtx, const char *szSrc, SG_uint32 start, char ** pszCopy)
{
	SG_uint32 len;

	SG_NULLARGCHECK_RETURN(szSrc);
	SG_NULLARGCHECK_RETURN(pszCopy);

	len = SG_STRLEN(szSrc);

	if( start >= len )
		SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, "", pszCopy)  );
	else
		SG_ERR_CHECK_RETURN(  SG_ascii__substring(pCtx, szSrc, start, len-start, pszCopy)  );
}

//////////////////////////////////////////////////////////////////

char * SG_int64_to_sz(SG_int64 x, SG_int_to_string_buffer buffer)
{
	if(buffer==NULL) // Eh?!
		return NULL;

	if(x<0)
	{
		buffer[0]='-';
		SG_uint64_to_sz((SG_uint64)-x,buffer+1);
		return buffer;
	}
	else
	{
		return SG_uint64_to_sz((SG_uint64)x,buffer);
	}
}

char * SG_uint64_to_sz(SG_uint64 x, SG_int_to_string_buffer buffer)
{
	SG_uint32 len = 0;

	if(buffer==NULL) // Eh?!
		return NULL;

	// First construct the string backwords, since that's easier.
	do
	{
		buffer[len] = '0' + x%10;
		len+=1;

		x/=10;
	} while(x>0);
	buffer[len]='\0';

	// Second, reverse the string.
	{
		SG_uint32 i;
		for(i=0;i<len/2;++i)
		{
			buffer[i]^=buffer[len-1-i];
			buffer[len-1-i]^=buffer[i];
			buffer[i]^=buffer[len-1-i];
		}
	}

	return buffer;
}

char * SG_uint64_to_sz__hex(SG_uint64 x, SG_int_to_string_buffer buffer)
{
	const char * digits = "0123456789abcdef";
	SG_uint32 i;

	if(buffer==NULL) // Eh?!
		return NULL;

	for(i=0;i<16;++i)
		buffer[16-1-i] = digits[ (x>>(i*4)) & 0xf ];
	buffer[16] = '\0';

	return buffer;
}


void SG_uint64__parse__strict(SG_context * pCtx, SG_uint64* piResult, const char* psz)
{
	SG_uint64 result = 0;
	const char* cur = psz;
	if (psz == NULL || *psz == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );

	while (*cur)
	{
		if ('0' <= *cur && *cur <='9' )
		{
			result = result * 10 + (*cur) - '0';
			cur++;
		}
		else
		{
            SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
		}
	}

	*piResult = result;
}

void SG_int64__parse__strict(SG_context * pCtx, SG_int64* piResult, const char* psz)
{
	SG_bool bNeg = SG_FALSE;
	SG_int64 result = 0;
	const char* cur = psz;
	if (psz == NULL || *psz == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
	if (*cur == '-')
	{
		bNeg = SG_TRUE;
		cur++;
	}
	if (cur == NULL || *cur == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
	while (*cur)
	{
		if ('0' <= *cur && *cur <='9' )
		{
			result = result * 10 + (*cur) - '0';
			cur++;
		}
		else
		{
            SG_ERR_THROW2_RETURN(  SG_ERR_INT_PARSE, (pCtx, "%s", psz)  );
		}
	}

	if (bNeg)
    {
		result = -result;
    }

	*piResult = result;
}

void SG_int32__parse__strict(SG_context * pCtx, SG_int32* piResult, const char* psz)
{
	SG_bool bNeg = SG_FALSE;
	SG_int32 result = 0;
	const char* cur = psz;
	if (psz == NULL || *psz == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
	if (*cur == '-')
	{
		bNeg = SG_TRUE;
		cur++;
	}
	if (cur == NULL || *cur == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
	while (*cur)
	{
		if ('0' <= *cur && *cur <='9' )
		{
			result = result * 10 + (*cur) - '0';
			cur++;
		}
		else
		{
            SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
		}
	}

	if (bNeg)
    {
		result = -result;
    }

	*piResult = result;
}

void SG_int32__parse__strict__buf_len(SG_context * pCtx, SG_int32* pResult, const char * p, SG_uint32 len)
{
	SG_int32 sign = 1;
	SG_int32 result = 0;
	const char* cur = p;
	const char* end = p+len;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pResult);
	SG_NULLARGCHECK_RETURN(p);

	if(len==0)
		SG_ERR_THROW_RETURN(SG_ERR_INT_PARSE);

	if (*cur == '-')
	{
		sign = -1;
		cur++;
	}
	if (cur == NULL || *cur == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
	if(cur==end)
		SG_ERR_THROW_RETURN(SG_ERR_INT_PARSE);

	while (cur<end)
	{
		if (!('0' <= *cur && *cur <='9' ))
			SG_ERR_THROW_RETURN(SG_ERR_INT_PARSE);
		result = result * 10 + (*cur) - '0';
		if(result<0)
			SG_ERR_THROW_RETURN(SG_ERR_INTEGER_OVERFLOW);
		cur++;
	}

	*pResult = result * sign;
}

void SG_int64__parse__stop_on_nondigit(SG_UNUSED_PARAM(SG_context * pCtx), SG_int64* piResult, const char* psz)
{
	SG_bool bNeg = SG_FALSE;
	SG_int64 result = 0;
	const char* cur = psz;

	SG_UNUSED(pCtx);

	while (*cur == ' ')
    {
		cur++;
    }

	if (*cur == '+')
	{
		cur++;
	}
	else if (*cur == '-')
	{
		bNeg = SG_TRUE;
		cur++;
	}

	while (*cur)
	{
        char c = *cur;
		if (('0' <= c) && (c <='9'))
		{
            // (x << 3) + (x << 1)
			result = result * 10 + c - '0';
			cur++;
		}
		else
		{
			break;
		}
	}

	if (bNeg)
    {
		result = -result;
    }

	*piResult = result;
}

void SG_int64__parse(SG_context * pCtx, SG_int64* piResult, const char* psz)
{
    SG_ERR_CHECK_RETURN(  SG_int64__parse__stop_on_nondigit(pCtx, piResult, psz)  );
}

void SG_uint32__parse__strict(SG_context * pCtx, SG_uint32* piResult, const char* psz)
{
    SG_uint64 i = 0;
	if (psz == NULL || *psz == '\0')
		SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );

    SG_ERR_CHECK_RETURN(  SG_uint64__parse__strict(pCtx, &i, psz)  );
    if (SG_uint64__fits_in_uint32(i))
    {
        *piResult = (SG_uint32) i;
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
    }
}

void SG_uint32__parse(SG_context * pCtx, SG_uint32* piResult, const char* psz)
{
    SG_int64 i = 0;

    SG_ERR_CHECK_RETURN(  SG_int64__parse(pCtx, &i, psz)  );
    if (SG_int64__fits_in_uint32(i))
    {
        *piResult = (SG_uint32) i;
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_INT_PARSE  );
    }
}

void SG_double__parse(SG_context * pCtx, double* pResult, const char* sz)
{
	int result = 1;

#if defined(WINDOWS)
	result = sscanf_s(sz, "%lf", pResult);
#else
	result = sscanf(sz, "%lf", pResult);
#endif

	// TODO change this to an SG_ERR_CHECK/THROW...

	if( result != 1 )
		(void)SG_context__err(pCtx, SG_ERR_INVALIDARG,__FILE__, __LINE__, "sz is invalid: Unable to parse it as a double.");
}


void SG_stringlist__free(
	SG_context* pCtx,
	char**      ppStringList,
	SG_uint32   uCount
	)
{
	if (!ppStringList)
		return;

	if (uCount > 0u)
	{
		SG_uint32 uIndex;

		for (uIndex = 0u; uIndex < uCount; ++uIndex)
		{
			SG_NULLFREE(pCtx, ppStringList[uIndex]);
		}
	}
	
	SG_NULLFREE(pCtx, ppStringList);
}


/* Boyer-Moore-Gosper-Horspool search for a substring in a (potentially) large buffer
 * based on example implementation at http://en.wikipedia.org/wiki/Boyer%E2%80%93Moore%E2%80%93Horspool_algorithm
 */
void SG_findInBuf(
	SG_context *pCtx,
	const SG_byte * haystack, 
	SG_uint32 hlen,
	const SG_byte* needle, 
	SG_uint32 nlen,
	const SG_byte **foundAt)
{
	SG_uint32 scan = 0; 
	SG_uint32 skipTable[UCHAR_MAX + 1];
	SG_uint32 last = 0;

	*foundAt = NULL;

	SG_NULLARGCHECK(haystack);
	SG_NULLARGCHECK(needle);
	SG_NULLARGCHECK(foundAt);

	if (nlen == 0)
		return;

	/* ---- Preprocess ---- */ 
	/* Initialize the table to default value */ 
	/* When a character is encountered that does not occur 
	 * in the needle, we can safely skip ahead for the whole 
	 * length of the needle. */ 
	
	for (scan = 0; scan <= UCHAR_MAX; scan = scan + 1) 
		skipTable[scan] = nlen; 
	
	last = nlen - 1; 
	
	/* Then populate it with the analysis of the needle */ 
	
	for (scan = 0; scan < last; scan = scan + 1) 
		skipTable[needle[scan]] = last - scan; 
	
	/* ---- Do the matching ---- */ 
	
	/* Search the haystack, while the needle can still be within it. */ 
	
	while (hlen >= nlen) 
	{ 
		/* scan from the end of the needle */ 
		for (scan = last; haystack[scan] == needle[scan]; scan = scan - 1) 
			if (scan == 0) /* If the first byte matches, we've found it. */ 
			{
				*foundAt = haystack;
				return;
			}
		
		/* otherwise, we need to skip some bytes and start again. Note that here we are getting the skip value based on the last byte of needle, no matter where we didn't match. So if needle is: "abcd" then we are skipping based on 'd' and that value will be 4, and for "abcdd" we again skip on 'd' but the value will be only 1. The alternative of pretending that the mismatched character was the last character is slower in the normal case (Eg. finding "abcd" in "...azcd..." gives 4 by using 'd' but only 4-2==2 using 'z'. */ 
			
		hlen -= skipTable[haystack[last]]; 
		haystack += skipTable[haystack[last]]; 
	} 
	
	return;

fail:
	*foundAt = NULL;
}

void SG_sz__to_json(SG_context * pCtx, const char * sz, SG_string ** ppJson)
{
    SG_string * pJson = NULL;
    SG_jsonwriter * pJsonwriter = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(sz);
    SG_NULLARGCHECK_RETURN(ppJson);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pJson)  );
    SG_ERR_CHECK(  SG_jsonwriter__alloc(pCtx, &pJsonwriter, pJson)  );
    SG_ERR_CHECK(  SG_jsonwriter__write_string__sz(pCtx, pJsonwriter, sz)  );
    SG_JSONWRITER_NULLFREE(pCtx, pJsonwriter);

    *ppJson = pJson;

    return;
fail:
    SG_STRING_NULLFREE(pCtx, pJson);
    SG_JSONWRITER_NULLFREE(pCtx, pJsonwriter);
}


/**
 * Checks if a character is whitespace.
 */
static SG_bool _is_whitespace(char cCharacter //< [in] The character to check.
	)
{
	
	if (
		cCharacter == ' ' ||
		cCharacter == '\t' ||
		cCharacter == '\r' ||
		cCharacter == '\n'
		)
	{
		return SG_TRUE;
	}
	else
	{
		return SG_FALSE;
	}
}

void SG_sz__is_all_whitespace(SG_context* pCtx, const char* psz, SG_bool* pbAllWhitespace)
{
	const char* p = psz;
	
	SG_NULLARGCHECK_RETURN(psz);
	SG_NULLARGCHECK_RETURN(pbAllWhitespace);

	while( _is_whitespace(*p) )
		++p;
	
	*pbAllWhitespace = (*p == '\0');
}

void SG_sz__trim(SG_context * pCtx, const char* pszStr, SG_uint32* out_size, char **ppszOut)
{  
  SG_uint32 len = 0u;
  SG_uint32 outLen = 0u;
  const char *end;
  
  if (out_size)
	  *out_size = 0;
  *ppszOut = NULL;

  if (!pszStr)
      return;

  len = SG_STRLEN(pszStr);
  if(len == 0u)
    return;  
  
  // set end to the character prior to the null terminator
  end = pszStr + len - 1u;

  // Trim leading space
  while(_is_whitespace(*pszStr))
	  pszStr++;

  if(*pszStr == '\0')  
    return;

  // Trim trailing space
  while(end > pszStr && _is_whitespace(*end))
      end--;
  end++;

  // Set output size to minimum of trimmed string length and buffer size 
  // outLen = ((SG_uint32)(end - pszStr) < len) ? (SG_uint32)(end - pszStr) : len;
  outLen = (SG_uint32)(end - pszStr);
    
  SG_ERR_CHECK(  SG_alloc(pCtx, outLen + 1u, sizeof(char*), &(*ppszOut))  );
 
  // Copy trimmed string and add null terminator
  SG_ERR_CHECK(  SG_strncpy(pCtx, *ppszOut,  outLen + 1u, pszStr, outLen)  );  
  
  if(out_size!=NULL)
    *out_size = outLen;
fail:
  return;
}

SG_bool SG_sz__starts_with(const char *szFullString, const char *szStart)
{
	if(!szFullString || !szStart)
		return SG_FALSE;

	while(*szFullString && *szStart && *szFullString==*szStart)
	{
		++szFullString;
		++szStart;
	}

	return *szStart=='\0';
}

void SG_utf8__validate__sz(
        SG_context* pCtx,
        const unsigned char* p
        )
{
#define SG_utf8__validate__MUST_BE_CONTINUATION(c) SG_STATEMENT(if (((c) < 0x80) || ((c) > 0xbf)) { SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  ); })

    SG_NULLARGCHECK_RETURN(p);

    while (*p)
    {
        if (*p <= 0x7f)
        {
            p++;
        }
        else if (*p <= 0xbf)
        {
            // a continuation byte, but not valid here
            SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
        }
        else if (*p <= 0xc1)
        {
            SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
        }
        else if (*p <= 0xdf)
        {
            SG_utf8__validate__MUST_BE_CONTINUATION(p[1]);
            p += 2;
        }
        else if (*p <= 0xef)
        {
            SG_utf8__validate__MUST_BE_CONTINUATION(p[1]);
            SG_utf8__validate__MUST_BE_CONTINUATION(p[2]);
            p += 3;
        }
        else if (*p <= 0xf4)
        {
            SG_utf8__validate__MUST_BE_CONTINUATION(p[1]);
            SG_utf8__validate__MUST_BE_CONTINUATION(p[2]);
            SG_utf8__validate__MUST_BE_CONTINUATION(p[3]);
            p += 4;
        }
        else if (*p >= 0xf5)
        {
            SG_ERR_THROW_RETURN(  SG_ERR_UTF8INVALID  );
        }
    }
#undef SG_utf8__validate__MUST_BE_CONTINUATION
}

void SG_utf8__fix__sz(
        SG_context* pCtx,
        unsigned char* p
        )
{
#define SG_utf8__fix__MUST_BE_CONTINUATION(c) SG_STATEMENT(if (((c) < 0x80) || ((c) > 0xbf)) { goto bad; })

    SG_NULLARGCHECK_RETURN(p);

    while (*p)
    {
        if (*p <= 0x7f)
        {
            p++;
        }
        else if (*p <= 0xbf)
        {
            // a continuation byte, but not valid here
            goto bad;
        }
        else if (*p <= 0xc1)
        {
            goto bad;
        }
        else if (*p <= 0xdf)
        {
            SG_utf8__fix__MUST_BE_CONTINUATION(p[1]);
            p += 2;
        }
        else if (*p <= 0xef)
        {
            SG_utf8__fix__MUST_BE_CONTINUATION(p[1]);
            SG_utf8__fix__MUST_BE_CONTINUATION(p[2]);
            p += 3;
        }
        else if (*p <= 0xf4)
        {
            SG_utf8__fix__MUST_BE_CONTINUATION(p[1]);
            SG_utf8__fix__MUST_BE_CONTINUATION(p[2]);
            SG_utf8__fix__MUST_BE_CONTINUATION(p[3]);
            p += 4;
        }
        else if (*p >= 0xf5)
        {
            goto bad;
        }

        continue;

bad:
        *p++ = '?';
    }

#undef SG_utf8__fix__MUST_BE_CONTINUATION
}


