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
 * @file sg_validate.c
 *
 * @details Implementation of functionality for validating string contents.
 *
 */

#include <sg.h>


/*
**
** Internal Functions
**
*/

/**
 * Checks a validation result and throws an error if it failed.
 */
static void _check_result(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uResult,   //< [in] The validation result to check.
	SG_error    uError,    //< [in] The type of error to throw if the result indicates failure.
	const char* szName,    //< [in] A name for the string being validated.
	SG_uint32   uMin,      //< [in] Minimum length used in the validation.
	SG_uint32   uMax,      //< [in] Maximum length used in the validation.
	const char* szInvalids //< [in] Set of invalid characters used in the validation.
	)
{
	if (uResult & SG_VALIDATE__RESULT__TOO_SHORT)
	{
		SG_ERR_THROW2(uError, (pCtx, "%s must contain at least %u character%s.", szName, uMin, uMin == 1u ? "" : "s"));
	}
	else if (uResult & SG_VALIDATE__RESULT__TOO_LONG)
	{
		SG_ERR_THROW2(uError, (pCtx, "%s cannot contain more than %u character%s.", szName, uMax, uMax == 1u ? "" : "s"));
	}
	else if (uResult & SG_VALIDATE__RESULT__INVALID_CHARACTER)
	{
		SG_ERR_THROW2(uError, (pCtx, "%s cannot contain any of the following: %s", szName, szInvalids));
	}
	else if (uResult & SG_VALIDATE__RESULT__CONTROL_CHARACTER)
	{
		SG_ERR_THROW2(uError, (pCtx, "%s cannot contain any control characters.", szName));
	}
	else
	{
		SG_ASSERT(uResult == SG_VALIDATE__RESULT__VALID);
	}

fail:
	return;
}

/**
 * Wrapper around SG_utf8__to_utf32 that allows empty strings.
 * I'm not sure why that function doesn't allow them, but I didn't want to change
 * something so low-level right now, so I'm wrapping it instead.  Will log something
 * to have it looked into later.
 */
static void _utf8_to_utf32(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	const char* szUtf8,  //< [in] UTF8 string to convert, may be empty but not NULL.
	SG_int32**  ppUtf32, //< [out] Converted UTF32 string.
	SG_uint32*  pLength  //< [out] Length of converted UTF32 string, in characters.
	)
{
	SG_int32* pUtf32  = NULL;
	SG_uint32 uLength = 0u;

	SG_NULLARGCHECK(szUtf8);
	SG_NULLARGCHECK(ppUtf32);

	if (szUtf8[0] == 0)
	{
		SG_alloc1(pCtx, pUtf32);
		pUtf32[0] = 0;
		uLength = 0u;
	}
	else
	{
		SG_ERR_CHECK(  SG_utf8__to_utf32__sz(pCtx, szUtf8, &pUtf32, &uLength)  );
	}

	*ppUtf32 = pUtf32;
	pUtf32 = NULL;
	if (pLength != NULL)
	{
		*pLength = uLength;
	}

fail:
	SG_NULLFREE(pCtx, pUtf32);
	return;
}

/**
 * Wrapper around SG_utf8__from_utf32 that allows empty strings.
 * I'm not sure why that function doesn't allow them, but I didn't want to change
 * something so low-level right now, so I'm wrapping it instead.  Will log something
 * to have it looked into later.
 */
static void _utf32_to_utf8(
	SG_context* pCtx,   //< [in] [out] Error and context info.
	SG_int32*   pUtf32, //< [in] UTF32 string to convert, may be empty but not NULL.
	char**      ppUtf8, //< [in] Converted UTF8 string.
	SG_uint32*  pLength //< [out] Length of converted UTF32 string, in bytes.
	)
{
	char*     szUtf8  = NULL;
	SG_uint32 uLength = 0u;

	SG_NULLARGCHECK(pUtf32);
	SG_NULLARGCHECK(ppUtf8);

	if (pUtf32[0] == 0)
	{
		SG_alloc1(pCtx, szUtf8);
		szUtf8[0] = 0;
		uLength = 1u;
	}
	else
	{
		SG_ERR_CHECK(  SG_utf8__from_utf32(pCtx, pUtf32, &szUtf8, &uLength)  );
	}

	*ppUtf8 = szUtf8;
	szUtf8 = NULL;
	if (pLength != NULL)
	{
		*pLength = uLength;
	}

fail:
	SG_NULLFREE(pCtx, szUtf8);
	return;
}

/**
 * Finds any character from a given set within a string and replaces them with a
 * specified replacement string.
 */
static void _replace_chars_with_string(
	SG_context* pCtx,         //< [in] [out] Error and context info.
	SG_string*  sValue,       //< [in] [out] String to perform replacements in.
	const char* szChars,      //< [in] Set of characters to replace, as a string.
	                          //<      NULL is treated as an empty string.
	const char* szReplacement //< [in] String to use as a replacement for the characters.
	                          //<      This whole string is a replacement for each found character.
	                          //<      NULL is treated as an empty string.
	)
{
	SG_int32* pValue32       = NULL;
	SG_uint32 uValue32       = 0u;
	SG_int32* pChars32       = NULL;
	SG_uint32 uChars32       = 0u;
	SG_int32* pReplacement32 = NULL;
	SG_uint32 uReplacement32 = 0u;
	SG_int32* pResult32      = NULL;
	SG_uint32 uResult32      = 0u;
	char*     szResult       = NULL;
	SG_uint32 uResult        = 0u;
	SG_uint32 uValueIndex    = 0u;

	SG_NULLARGCHECK(sValue);

	// treat NULLs as empty strings
	if (szChars == NULL)
	{
		szChars = "";
	}
	if (szReplacement == NULL)
	{
		szReplacement = "";
	}

	// convert everything to UTF32
	// I couldn't come up with a way to do this directly in UTF8 using the APIs
	// available in sg_utf8.
	SG_ERR_CHECK(  _utf8_to_utf32(pCtx, SG_string__sz(sValue), &pValue32, &uValue32)  );
	SG_ERR_CHECK(  _utf8_to_utf32(pCtx, szChars, &pChars32, &uChars32)  );
	SG_ERR_CHECK(  _utf8_to_utf32(pCtx, szReplacement, &pReplacement32, &uReplacement32)  );

	// allocate a result buffer
	if (uReplacement32 > 1u)
	{
		// largest possible size we could end up with is if we replace every single
		// character in the value with the replacement string
		SG_ERR_CHECK(  SG_allocN(pCtx, (uReplacement32 * uValue32) + 1u, pResult32)  );
	}
	else
	{
		// largest possible size we could end up with is if we do no replacements
		// at all and are left with exactly the input value
		SG_ERR_CHECK(  SG_allocN(pCtx, uValue32 + 1u, pResult32)  );
	}

	// run through each character in the value
	for (uValueIndex = 0u; uValueIndex < uValue32; ++uValueIndex)
	{
		SG_int32  iValueChar  = pValue32[uValueIndex];
		SG_bool   bReplace    = SG_FALSE;
		SG_uint32 uCharsIndex = 0u;

		// check if this character should be replaced
		for (uCharsIndex = 0u; uCharsIndex < uChars32; ++uCharsIndex)
		{
			if (iValueChar == pChars32[uCharsIndex])
			{
				bReplace = SG_TRUE;
				break;
			}
		}
		if (bReplace == SG_FALSE)
		{
			// append the character to the output
			pResult32[uResult32] = iValueChar;
			++uResult32;
		}
		else
		{
			// append the replacement string to the output
			memcpy((void*)(pResult32 + uResult32), (void*)pReplacement32, uReplacement32 * sizeof(SG_int32));
			uResult32 += uReplacement32;
		}
	}

	// NULL-terminate the result and convert it back to UTF8
	pResult32[uResult32] = 0;
	SG_ERR_CHECK(  _utf32_to_utf8(pCtx, pResult32, &szResult, &uResult)  );

	// return the result by replacing the original value's contents
	SG_ERR_CHECK(  SG_string__adopt_buffer(pCtx, sValue, szResult, uResult)  );
	szResult = NULL;

fail:
	SG_NULLFREE(pCtx, pValue32);
	SG_NULLFREE(pCtx, pChars32);
	SG_NULLFREE(pCtx, pReplacement32);
	SG_NULLFREE(pCtx, pResult32);
	SG_NULLFREE(pCtx, szResult);
	return;
}

/**
 * Truncates a string to be at most the given number of characters in length.
 */
static void _truncate_string(
	SG_context* pCtx,   //< [in] [out] Error and context info.
	SG_string*  sValue, //< [in] [out] The string to truncate.
	SG_uint32   uLength //< [in] The length to truncate the string to, in characters.
	)
{
	SG_uint32 uValue   = 0u;
	SG_int32* pValue32 = NULL;
	char*     szResult = NULL;

	// get the length of the value and check if it's too long
	SG_ERR_CHECK(  SG_utf8__length_in_characters__sz(pCtx, SG_string__sz(sValue), &uValue)  );
	if (uValue > uLength)
	{
		SG_uint32 uResult = 0u;

		// convert the value to UTF32
		// I can't come up with a good way to do this in UTF8 using the APIs available
		// in sg_utf8.  Truncating to bytes means that we might not end up on a character
		// boundary, and there's no good way to ask how many bytes a given character
		// requires, so iterating through characters doesn't help much either.
		SG_ERR_CHECK(  _utf8_to_utf32(pCtx, SG_string__sz(sValue), &pValue32, NULL)  );

		// chop the buffer down to the given length
		pValue32[uLength] = 0;

		// convert the value back to UTF8
		SG_ERR_CHECK(  _utf32_to_utf8(pCtx, pValue32, &szResult, &uResult)  );

		// replace the old value with the new
		SG_ERR_CHECK(  SG_string__adopt_buffer(pCtx, sValue, szResult, uResult)  );
		szResult = NULL;
	}

fail:
	SG_NULLFREE(pCtx, pValue32);
	SG_NULLFREE(pCtx, szResult);
	return;
}


/*
**
** Public Functions
**
*/

void SG_validate__check(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32   uMin,
	SG_uint32   uMax,
	const char* szInvalids,
	SG_bool     bControls,
	SG_uint32*  pResult
	)
{
	SG_uint32 uResult = 0u;
	SG_uint32 uLength = 0u;

	SG_ARGCHECK(uMin <= uMax, uMin|uMax);
	SG_NULLARGCHECK(pResult);

	// treat NULL as an empty string
	if (szValue == NULL)
	{
		szValue = "";
	}

	// validate minimum length
	uLength = SG_STRLEN(szValue);
	if (uLength < uMin)
	{
		uResult |= SG_VALIDATE__RESULT__TOO_SHORT;
	}

	// validate maximum length
	if (uLength > uMax)
	{
		uResult |= SG_VALIDATE__RESULT__TOO_LONG;
	}

	// validate specified characters
	if (szInvalids != NULL)
	{
		SG_bool bShares = SG_FALSE;
		SG_ERR_CHECK(  SG_utf8__shares_characters(pCtx, szValue, szInvalids, &bShares)  );
		if (bShares != SG_FALSE)
		{
			uResult |= SG_VALIDATE__RESULT__INVALID_CHARACTER;
		}
	}

	// validate control characters
	if (bControls != SG_FALSE)
	{
		SG_bool bShares = SG_FALSE;
		SG_ERR_CHECK(  SG_utf8__shares_characters(pCtx, szValue, SG_VALIDATE__CHARS__CONTROL, &bShares)  );
		if (bShares != SG_FALSE)
		{
			uResult |= SG_VALIDATE__RESULT__CONTROL_CHARACTER;
		}
	}

	*pResult = uResult;

fail:
	return;
}

void SG_validate__check__trim(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32   uMin,
	SG_uint32   uMax,
	const char* szInvalids,
	SG_bool     bControls,
	SG_uint32*  pResult,
	char**      ppTrimmed
	)
{
	char* szTrimmed = NULL;

	SG_NULLARGCHECK(pResult);

	// trim the value (will return NULL if the value is NULL or trims to nothing)
	SG_ERR_CHECK(  SG_sz__trim(pCtx, szValue, NULL, &szTrimmed)  );

	// validate the trimmed value
	if (szTrimmed == NULL)
	{
		SG_ERR_CHECK(  SG_validate__check(pCtx, "", uMin, uMax, szInvalids, bControls, pResult)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_validate__check(pCtx, szTrimmed, uMin, uMax, szInvalids, bControls, pResult)  );
	}

	// if it's valid and they want the trimmed value, return it
	if (*pResult == SG_VALIDATE__RESULT__VALID && ppTrimmed != NULL)
	{
		*ppTrimmed = szTrimmed;
		szTrimmed = NULL;
	}

fail:
	SG_NULLFREE(pCtx, szTrimmed);
	return;
}

void SG_validate__ensure(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32   uMin,
	SG_uint32   uMax,
	const char* szInvalids,
	SG_bool     bControls,
	SG_error    uError,
	const char* szName
	)
{
	SG_uint32 uResult = 0u;

	SG_ERR_CHECK(  SG_validate__check(pCtx, szValue, uMin, uMax, szInvalids, bControls, &uResult)  );
	SG_ERR_CHECK(  _check_result(pCtx, uResult, uError, szName, uMin, uMax, szInvalids)  );

fail:
	return;
}

void SG_validate__ensure__trim(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32   uMin,
	SG_uint32   uMax,
	const char* szInvalids,
	SG_bool     bControls,
	SG_error    uError,
	const char* szName,
	char**      ppTrimmed
	)
{
	SG_uint32 uResult   = 0u;
	char*     szTrimmed = NULL;

	SG_ERR_CHECK(  SG_validate__check__trim(pCtx, szValue, uMin, uMax, szInvalids, bControls, &uResult, &szTrimmed)  );
	SG_ERR_CHECK(  _check_result(pCtx, uResult, uError, szName, uMin, uMax, szInvalids)  );

	if (ppTrimmed != NULL)
	{
		*ppTrimmed = szTrimmed;
		szTrimmed = NULL;
	}

fail:
	SG_NULLFREE(pCtx, szTrimmed);
	return;
}

void SG_validate__sanitize(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32   uMin,
	SG_uint32   uMax,
	const char* szInvalids,
	SG_uint32   uFixFlags,
	const char* szReplace,
	const char* szAdd,
	SG_string** ppSanitized
	)
{
	SG_string* sSanitized = NULL;

	SG_NULLARGCHECK(ppSanitized);

	// treat NULL replacement string as empty
	if (szReplace == NULL)
	{
		szReplace = "";
	}

	// allocate our result string
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sSanitized, szValue)  );

	// if we need to sanitize bad characters, do that
	// Note: We do this first because sanitizing characters might change the
	//       length of the string and affect the min/max length check.
	if (uFixFlags & SG_VALIDATE__RESULT__INVALID_CHARACTER)
	{
		SG_ERR_CHECK(  _replace_chars_with_string(pCtx, sSanitized, szInvalids, szReplace)  );
	}
	if (uFixFlags & SG_VALIDATE__RESULT__CONTROL_CHARACTER)
	{
		SG_ERR_CHECK(  _replace_chars_with_string(pCtx, sSanitized, SG_VALIDATE__CHARS__CONTROL, szReplace)  );
	}

	// if we need to lengthen the string, do that
	// Note: We do this prior to checking the max length because we have more fine
	//       grained control over reducing length than we do over expanding it.  We
	//       can remove individual characters, but only add characters in blocks of
	//       strlen(szAdd).  If uMin and uMax are close to each other, then adding
	//       a single szAdd might take us over uMax.  If that happens, we want to
	//       be able to trim that back down to uMax afterward.
	if (uFixFlags & SG_VALIDATE__RESULT__TOO_SHORT)
	{
		SG_uint32 uSanitized = 0u;
		SG_uint32 uAdd       = 0u;

		SG_ARGCHECK(szAdd != NULL && SG_STRLEN(szAdd) > 0u, szAdd);

		// get the length of both strings
		SG_ERR_CHECK(  SG_utf8__length_in_characters__sz(pCtx, SG_string__sz(sSanitized), &uSanitized)  );
		SG_ERR_CHECK(  SG_utf8__length_in_characters__sz(pCtx, szAdd, &uAdd)  );

		// keep adding until the sanitized string is long enough
		while (uSanitized < uMin)
		{
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sSanitized, szAdd)  );
			uSanitized += uAdd;
		}
	}

	// if we need to shorten the string, do that
	if (uFixFlags & SG_VALIDATE__RESULT__TOO_LONG)
	{
		SG_ERR_CHECK(  _truncate_string(pCtx, sSanitized, uMax)  );
	}

	// return the sanitized result
	*ppSanitized = sSanitized;
	sSanitized = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sSanitized);
	return;
}

void SG_validate__sanitize__trim(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32   uMin,
	SG_uint32   uMax,
	const char* szInvalids,
	SG_uint32   uFixFlags,
	const char* szReplace,
	const char* szAdd,
	SG_string** ppSanitized
	)
{
	char* szTrimmed = NULL;

	SG_NULLARGCHECK(ppSanitized);

	// trim the value (will return NULL if the value is NULL or trims to nothing)
	SG_ERR_CHECK(  SG_sz__trim(pCtx, szValue, NULL, &szTrimmed)  );

	// sanitize the trimmed value
	if (szTrimmed == NULL)
	{
		SG_ERR_CHECK(  SG_validate__sanitize(pCtx, "", uMin, uMax, szInvalids, uFixFlags, szReplace, szAdd, ppSanitized)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_validate__sanitize(pCtx, szTrimmed, uMin, uMax, szInvalids, uFixFlags, szReplace, szAdd, ppSanitized)  );
	}

fail:
	SG_NULLFREE(pCtx, szTrimmed);
	return;
}
