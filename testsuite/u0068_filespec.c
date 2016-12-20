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

#include "unittests.h"
#include "unittests_pendingtree.h"


/**
 * Unit tests for SG_file_spec.
 */


/*
**
** alloc
**
*/

static void u0068__alloc__basic(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;

	VERIFY_ERR_CHECK(  SG_file_spec__alloc(pCtx, &pFilespec)  );
	VERIFY_COND("alloc__basic: successful allocation returned NULL", pFilespec != NULL);
	VERIFY_ERR_CHECK(  SG_file_spec__free(pCtx, pFilespec)  );

fail:
	return;
}

static void u0068__alloc__macro(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_COND("alloc__macro: successful allocation returned NULL", pFilespec != NULL);
	VERIFY_ERR_CHECK(  SG_file_spec__free(pCtx, pFilespec)  );

fail:
	return;
}

static void u0068__alloc__badargs(SG_context* pCtx)
{
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__alloc(pCtx, NULL), SG_ERR_INVALIDARG  );
}


/*
**
** alloc__copy
**
*/

static void u0068__alloc__copy__basic(SG_context* pCtx)
{
	static const SG_uint32 uTestFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_file_spec* pCopy     = NULL;
	SG_bool       bOutput   = SG_FALSE;
	SG_uint32     uOutput   = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, "include_pattern", 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__EXCLUDE, "exclude_pattern", 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__IGNORE,  "ignore_pattern",  0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, uTestFlags, NULL)  );

	VERIFY_ERR_CHECK(  SG_file_spec__alloc__copy(pCtx, pFilespec, &pCopy)  );
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pCopy, SG_FILE_SPEC__PATTERN__INCLUDE, &bOutput)  );
	VERIFY_COND("alloc__copy__basic: includes not copied", bOutput == SG_TRUE);
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pCopy, SG_FILE_SPEC__PATTERN__EXCLUDE, &bOutput)  );
	VERIFY_COND("alloc__copy__basic: excludes not copied", bOutput == SG_TRUE);
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pCopy, SG_FILE_SPEC__PATTERN__IGNORE, &bOutput)  );
	VERIFY_COND("alloc__copy__basic: ignores not copied", bOutput == SG_TRUE);
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pCopy, &uOutput)  );
	VERIFY_COND("alloc__copy__basic: flags not copied", uOutput == uTestFlags);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pCopy);
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__alloc__copy__macro(SG_context* pCtx)
{
	static const SG_uint32 uTestFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_file_spec* pCopy     = NULL;
	SG_bool       bOutput   = SG_FALSE;
	SG_uint32     uOutput   = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, "include_pattern", 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__EXCLUDE, "exclude_pattern", 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__IGNORE,  "ignore_pattern",  0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, uTestFlags, NULL)  );

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC__COPY(pCtx, pFilespec, &pCopy)  );
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pCopy, SG_FILE_SPEC__PATTERN__INCLUDE, &bOutput)  );
	VERIFY_COND("alloc__copy__macro: includes not copied", bOutput == SG_TRUE);
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pCopy, SG_FILE_SPEC__PATTERN__EXCLUDE, &bOutput)  );
	VERIFY_COND("alloc__copy__macro: excludes not copied", bOutput == SG_TRUE);
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pCopy, SG_FILE_SPEC__PATTERN__IGNORE, &bOutput)  );
	VERIFY_COND("alloc__copy__macro: ignores not copied", bOutput == SG_TRUE);
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pCopy, &uOutput)  );
	VERIFY_COND("alloc__copy__macro: flags not copied", uOutput == uTestFlags);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pCopy);
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__alloc__copy__badargs(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__alloc__copy(pCtx, NULL, &pFilespec), SG_ERR_INVALIDARG  );
	VERIFY_COND_FAIL("alloc__copy: allocation error returned non-NULL filespec", pFilespec == NULL);

	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__alloc__copy(pCtx, pFilespec, NULL), SG_ERR_INVALIDARG  );

fail:
	return;
}

static void u0068__alloc__copy__blank(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_TRUE;
	SG_uint32     uOutput   = 1u;

	VERIFY_ERR_CHECK(  SG_file_spec__alloc__copy(pCtx, gpBlankFilespec, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE | SG_FILE_SPEC__PATTERN__EXCLUDE | SG_FILE_SPEC__PATTERN__IGNORE, &bOutput)  );
	VERIFY_COND("alloc__copy__blank: patterns found in copy of blank", bOutput == SG_FALSE);
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("alloc__copy__blank: flags set in copy of blank", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}


/*
**
** free
**
*/

// basic case is covered by u0068__alloc__basic

static void u0068__free__blank(SG_context* pCtx)
{
	// should just do nothing
	VERIFY_ERR_CHECK(  SG_file_spec__free(pCtx, gpBlankFilespec)  );

fail:
	return;
}

// cases where the filespec contains data are covered by other tests such as alloc__copy and add_pattern__*


/*
**
** reset
**
*/

static void u0068__reset__basic(SG_context* pCtx)
{
	static const char*     szPattern  = "test pattern";
	static const SG_uint32 uTestFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_TRUE;
	SG_uint32     uOutput   = 1u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, szPattern, 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__EXCLUDE, szPattern, 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__IGNORE,  szPattern, 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, uTestFlags, NULL)  );

	VERIFY_ERR_CHECK(  SG_file_spec__reset(pCtx, pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE | SG_FILE_SPEC__PATTERN__EXCLUDE | SG_FILE_SPEC__PATTERN__IGNORE, &bOutput)  );
	VERIFY_COND("reset__basic: patterns found after reset", bOutput == SG_FALSE);
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("reset__basic: flags set after reset", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__reset__badargs(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__reset(pCtx, NULL), SG_ERR_INVALIDARG  );
}


/*
**
** add_pattern__sz
**
*/

static void u0068__add_pattern__sz__basic(SG_context* pCtx)
{
	static const char* szPattern = "test pattern";

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uIndex    = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1u)
	{
		SG_file_spec__pattern_type eType   = (SG_file_spec__pattern_type)uIndex;
		SG_bool                    bOutput = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, eType, szPattern, 0u)  );
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("add_pattern__sz__basic: pattern doesn't exist after being added", bOutput == SG_TRUE);
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__add_pattern__sz__badargs(SG_context* pCtx)
{
	static const char*                      szPattern = "test pattern";
	static const SG_file_spec__pattern_type eType     = SG_FILE_SPEC__PATTERN__INCLUDE;
	
	SG_file_spec* pFilespec = NULL;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__sz(pCtx, NULL, eType, szPattern, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__LAST, szPattern, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, eType, NULL, 0u), SG_ERR_INVALIDARG  );
}

static void u0068__add_pattern__sz__deep_copy(SG_context* pCtx)
{
	static const char*     szPattern1 = "test pattern";
	static const char*     szPattern2 = "another pattern";
	static const SG_uint32 uSize      = 123u;
	
	SG_file_spec* pFilespec = NULL;
	char*         szBuffer  = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_alloc(pCtx, sizeof(char), uSize, &szBuffer)  );
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern1)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, szBuffer, 0u)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__sz__deep_copy: filespec didn't match when expected", bOutput == SG_TRUE);

	// modify the memory containing the added pattern to something else
	// if the filespec correctly made a deep copy, its pattern will not change
	// therefore running the exact same match check should continue to succeed
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern2)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__sz__deep_copy: filespec didn't match when expected", bOutput == SG_TRUE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_NULLFREE(pCtx, szBuffer);
}

static void u0068__add_pattern__sz__shallow_copy(SG_context* pCtx)
{
	static const char*     szPattern1 = "test pattern";
	static const char*     szPattern2 = "another pattern";
	static const SG_uint32 uSize      = 123u;
	
	SG_file_spec* pFilespec = NULL;
	char*         szBuffer  = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_alloc(pCtx, sizeof(char), uSize, &szBuffer)  );
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern1)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, szBuffer, SG_FILE_SPEC__SHALLOW_COPY)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__sz__shallow_copy: filespec didn't match when expected", bOutput == SG_TRUE);

	// modify the memory containing the added pattern to something else
	// if the filespec correctly made a shallow copy, its pattern will change as well
	// that should cause the exact same match check to fail this time
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern2)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__sz__shallow_copy: filespec matched unexpectedly", bOutput == SG_FALSE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_NULLFREE(pCtx, szBuffer);
}

static void u0068__add_pattern__sz__shallow_copy_own_memory(SG_context* pCtx)
{
	static const char*     szPattern = "test pattern";
	static const SG_uint32 uSize     = 123u;
	
	SG_file_spec* pFilespec = NULL;
	char*         szBuffer  = NULL;

	VERIFY_ERR_CHECK(  SG_alloc(pCtx, sizeof(char), uSize, &szBuffer)  );
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	// this gives our memory to the filespec, which should free it for us
	// test fails if we get a memory leak
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, szBuffer, SG_FILE_SPEC__SHALLOW_COPY | SG_FILE_SPEC__OWN_MEMORY)  );

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

// cases using various matching flags are covered by match_path tests


/*
**
** add_pattern__buflen
**
*/

static void u0068__add_pattern__buflen__basic(SG_context* pCtx)
{
	static const char*     szPattern = "test pattern";
	static const SG_uint32 uLength   = 6u;

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uIndex    = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1u)
	{
		SG_file_spec__pattern_type eType   = (SG_file_spec__pattern_type)uIndex;
		SG_bool                    bOutput = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, eType, szPattern, uLength, 0u)  );
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("add_pattern__buflen__basic: pattern doesn't exist after being added", bOutput == SG_TRUE);
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__add_pattern__buflen__badargs(SG_context* pCtx)
{
	static const char*                      szPattern = "test pattern";
	static const SG_uint32                  uLength   = 6u;
	static const SG_file_spec__pattern_type eType     = SG_FILE_SPEC__PATTERN__INCLUDE;

	SG_file_spec* pFilespec = NULL;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__buflen(pCtx, NULL, eType, szPattern, uLength, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__LAST, szPattern, uLength, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, eType, NULL, uLength, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, eType, szPattern, 0u, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, eType, szPattern, uLength, SG_FILE_SPEC__SHALLOW_COPY), SG_ERR_INVALIDARG  );
}

static void u0068__add_pattern__buflen__deep_copy(SG_context* pCtx)
{
	static const char*     szPattern1 = "test pattern";
	static const char*     szPattern2 = "another pattern";
	static const SG_uint32 uLength    = 12u; // == strlen(szPattern1)
	static const SG_uint32 uSize      = 123u;

	SG_file_spec* pFilespec = NULL;
	char*         szBuffer  = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_alloc(pCtx, sizeof(char), uSize, &szBuffer)  );
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern1)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, szBuffer, uLength, 0u)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__buflen__deep_copy: filespec didn't match when expected", bOutput == SG_TRUE);

	// modify the memory containing the added pattern to something else
	// if the filespec correctly made a deep copy, its pattern will not change
	// therefore running the exact same match check should continue to succeed
	VERIFY_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uSize, szPattern2)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__buflen__deep_copy: filespec didn't match when expected", bOutput == SG_TRUE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_NULLFREE(pCtx, szBuffer);
}

static void u0068__add_pattern__buflen__short_copy(SG_context* pCtx)
{
	static const char*     szPattern = "test pattern";
	static const SG_uint32 uLength   = 6u; // < strlen(szPattern)

	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__buflen(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, szPattern, uLength, 0u)  );

	// this shouldn't match, even though we're matching against the same pattern we added
	// because we specified a length shorter than the full pattern when we added it
	// so the pattern in the filespec is actually shorter than the one we're matching against now
	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__buflen__short_copy: filespec matched unexpectedly", bOutput == SG_FALSE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

// cases using various matching flags are covered by match_path tests


/*
**
** add_pattern__string
**
*/

static void u0068__add_pattern__string__basic(SG_context* pCtx)
{
	static const char* szPattern = "test pattern";

	SG_string*    pString   = NULL;
	SG_file_spec* pFilespec = NULL;
	SG_uint32     uIndex    = 0u;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pString, szPattern)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1u)
	{
		SG_file_spec__pattern_type eType   = (SG_file_spec__pattern_type)uIndex;
		SG_bool                    bOutput = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__string(pCtx, pFilespec, eType, pString, 0u)  );
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("add_pattern__string__basic: pattern doesn't exist after being added", bOutput == SG_TRUE);
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_STRING_NULLFREE(pCtx, pString);
}

static void u0068__add_pattern__string__badargs(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType = SG_FILE_SPEC__PATTERN__INCLUDE;

	SG_string*    pString   = NULL;
	SG_file_spec* pFilespec = NULL;
	pString   = (SG_string*)   &pString;   // just needs to be some non-NULL pointer
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__string(pCtx, NULL, eType, pString, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__string(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__LAST, pString, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_pattern__string(pCtx, pFilespec, eType, NULL, 0u), SG_ERR_INVALIDARG  );
}

static void u0068__add_pattern__string__deep_copy(SG_context* pCtx)
{
	static const char* szPattern1 = "test pattern";
	static const char* szPattern2 = "another pattern";

	SG_string*    pString   = NULL;
	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pString, szPattern1)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__string(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, pString, 0u)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__string__deep_copy: filespec didn't match when expected", bOutput == SG_TRUE);

	// modify the string containing the added pattern to something else
	// if the filespec correctly made a deep copy, its pattern will not change
	// therefore running the exact same match check should continue to succeed
	VERIFY_ERR_CHECK(  SG_string__set__sz(pCtx, pString, szPattern2)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__string__deep_copy: filespec didn't match when expected", bOutput == SG_TRUE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_STRING_NULLFREE(pCtx, pString);
}

static void u0068__add_pattern__string__shallow_copy(SG_context* pCtx)
{
	static const char* szPattern1 = "test pattern";
	static const char* szPattern2 = "a pattern"; // shorter than szPattern1, to avoid SG_string reallocating

	SG_string*    pString   = NULL;
	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pString, szPattern1)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__string(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, pString, SG_FILE_SPEC__SHALLOW_COPY)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__string__shallow_copy: filespec didn't match when expected", bOutput == SG_TRUE);

	// modify the string containing the added pattern to something else
	// if the filespec correctly made a shallow copy, its pattern will change as well
	// that should cause the exact same match check to fail this time
	VERIFY_ERR_CHECK(  SG_string__set__sz(pCtx, pString, szPattern2)  );

	VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, szPattern1, 0u, &bOutput)  );
	VERIFY_COND("add_pattern__string__shallow_copy: filespec matched unexpectedly", bOutput == SG_FALSE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_STRING_NULLFREE(pCtx, pString);
}

static void u0068__add_pattern__string__shallow_copy_own_memory(SG_context* pCtx)
{
	static const char* szPattern = "test pattern";

	SG_string*    pString   = NULL;
	SG_file_spec* pFilespec = NULL;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pString, szPattern)  );
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	// this gives our memory to the filespec, which should free it for us
	// test fails if we get a memory leak
	VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__string(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE, pString, SG_FILE_SPEC__SHALLOW_COPY | SG_FILE_SPEC__OWN_MEMORY)  );

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

// cases using various matching flags are covered by match_path tests


/*
**
** add_patterns__array
**
*/

static void u0068__add_patterns__array__basic(SG_context* pCtx)
{
	static const char* ppPatterns[] = {
		"test pattern",
		"a pattern",
		"something else",
		"another one",
	};
	static const SG_uint32 uCount = SG_NrElements(ppPatterns);

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uIndex    = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1u)
	{
		SG_file_spec__pattern_type eType   = (SG_file_spec__pattern_type)uIndex;
		SG_bool                    bOutput = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_file_spec__add_patterns__array(pCtx, pFilespec, eType, ppPatterns, uCount, 0u)  );
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("add_patterns__array__basic: patterns don't exist after being added", bOutput == SG_TRUE);
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__add_patterns__array__badargs(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType  = SG_FILE_SPEC__PATTERN__INCLUDE;
	static const SG_uint32                  uCount = 1u;

	const char* const* ppPatterns = NULL;
	SG_file_spec*      pFilespec  = NULL;
	ppPatterns = (const char* const*)&ppPatterns; // just needs to be some non-NULL pointer
	pFilespec  = (SG_file_spec*)     &pFilespec;  // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__array(pCtx, NULL, eType, ppPatterns, uCount, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__array(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__LAST, ppPatterns, uCount, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__array(pCtx, pFilespec, eType, NULL, uCount, 0u), SG_ERR_INVALIDARG  );
}

// other cases should be covered by add_pattern__sz, since add_pattern__array is implemented in terms of it


/*
**
** add_patterns__stringarray
**
*/

static void u0068__add_patterns__stringarray__basic(SG_context* pCtx)
{
	static const char* ppPatterns[] = {
		"test pattern",
		"a pattern",
		"something else",
		"another one",
	};
	static const SG_uint32 uCount = SG_NrElements(ppPatterns);

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uIndex    = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1u)
	{
		SG_file_spec__pattern_type eType   = (SG_file_spec__pattern_type)uIndex;
		SG_bool                    bOutput = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_file_spec__add_patterns__array(pCtx, pFilespec, eType, ppPatterns, uCount, 0u)  );
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("add_patterns__stringarray__basic: patterns don't exist after being added", bOutput == SG_TRUE);
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__add_patterns__stringarray__badargs(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType  = SG_FILE_SPEC__PATTERN__INCLUDE;

	SG_stringarray* pStringarray = NULL;
	SG_file_spec*   pFilespec    = NULL;
	pStringarray = (SG_stringarray*)&pStringarray; // just needs to be some non-NULL pointer
	pFilespec    = (SG_file_spec*)  &pFilespec;    // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__stringarray(pCtx, NULL, eType, pStringarray, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__stringarray(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__LAST, pStringarray, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__stringarray(pCtx, pFilespec, eType, NULL, 0u), SG_ERR_INVALIDARG  );
}

// other cases should be covered by add_patterns__array, since add_patterns__stringarray is implemented in terms of it


/*
**
** add_patterns__file
**
*/

/**
 * A single test case for reading patterns from a file.
 */
typedef struct
{
	const char*                ppFileLines[10];  //< An array of lines to place in the test file to be read.
	SG_bool                    bTrailingNewline; //< Whether or not to write a newline at the end of the last line.
	SG_file_spec__pattern_type eType;            //< The type to use for the patterns read from the file.
	SG_bool                    bHasPattern;      //< The expected result of has_pattern after reading patterns from the file.
	const char*                szPath;           //< A path to match against the filespec after reading the patterns.
	SG_uint32                  uFlags;           //< Flags to use when matching the test path against the filespec.
	SG_bool                    bMatch;           //< Expected result of match_path.
	SG_file_spec__match_result eResult;          //< Expected result of should_include.
}
u0068__add_patterns__file__case;

/**
 * A list of line endings to test.
 * Each different line ending will be used with each test case.
 */
static const char* u0068__add_patterns__file__endings[] = {
	"\r",
	"\n",
	"\r\n",
};

/**
 * List of test cases to run for reading patterns from a file.
 */
static const u0068__add_patterns__file__case u0068__add_patterns__file__cases[] = {
	// basics
	{ // only line in a file
		{ "foobar", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // last line in a multi-line file
		{ "foobar", "something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // last line in a multi-line file with no trailing newline
		{ "foobar", "something", NULL, }, SG_FALSE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // middle line in a multi-line file
		{ "foobar", "something", "whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},

	// same as basics, but with leading whitespace on each line
	{
		{ " \t \tfoobar", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ " \t \tfoobar", " \t \tsomething", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ " \t \tfoobar", " \t \tsomething", NULL, }, SG_FALSE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ " \t \tfoobar", " \t \tsomething", " \t \twhatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},

	// same as basics, but with trailing whitespace on each line
	{
		{ "foobar \t \t", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "foobar \t \t", "something \t \t", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "foobar \t \t", "something \t \t", NULL, }, SG_FALSE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "foobar \t \t", "something \t \t", "whatever \t \t", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},

	// same as basics, but with leading and trailing whitespace on each line
	{
		{ " \t \tfoobar \t \t", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ " \t \tfoobar \t \t", " \t \tsomething \t \t", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ " \t \tfoobar \t \t", " \t \tsomething \t \t", NULL, }, SG_FALSE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ " \t \tfoobar \t \t", " \t \tsomething \t \t", " \t \twhatever \t \t", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},

	// comments
	{ // only line is a comment
		{ "# foobar", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
	{ // comment marker has whitespace before it
		{ " \t \t# foobar", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
	{ // all lines in a multi-line file are comments
		{ "# foobar", "# something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
	{ // only first line is a comment
		{ "# foobar", "something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},
	{ // only first line is a comment
		{ "# foobar", "something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // only last line is a comment
		{ "foobar", "# something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // only last line is a comment
		{ "foobar", "# something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},
	{ // only middle line is a comment
		{ "foobar", "# something", "whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},
	{ // only middle line is a comment
		{ "foobar", "# something", "whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "whatever", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // only middle line is NOT a comment
		{ "# foobar", "something", "# whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{ // only middle line is NOT a comment
		{ "# foobar", "something", "# whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "whatever", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},

	// same as comments, but using ; marker insead of #
	{
		{ "; foobar", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
	{
		{ " \t \t; foobar", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
	{
		{ "; foobar", "; something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
	{
		{ "; foobar", "something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},
	{
		{ "; foobar", "something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "foobar", "; something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "foobar", "; something", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},
	{
		{ "foobar", "; something", "whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},
	{
		{ "foobar", "; something", "whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "whatever", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "; foobar", "something", "; whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "something", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__INCLUDED,
	},
	{
		{ "; foobar", "something", "; whatever", NULL, }, SG_TRUE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_TRUE, "whatever", 0u, SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,
	},

	// edge cases
	{ // empty file
		{ NULL, }, SG_FALSE,
		SG_FILE_SPEC__PATTERN__INCLUDE, SG_FALSE, "foobar", 0u, SG_TRUE, SG_FILE_SPEC__RESULT__IMPLIED,
	},
};

static void u0068__add_patterns__file__run_test_cases(SG_context* pCtx)
{
	SG_file_spec* pFilespec   = NULL;
	SG_pathname*  pPathname   = NULL;
	SG_file*      pFile       = NULL;
	SG_bool       bFileExists = SG_FALSE;
	SG_uint32     uCaseIndex  = 0u;

	// allocate a filespec to test with
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	// find a temp filename to use
	VERIFY_ERR_CHECK(  unittest__get_nonexistent_pathname(pCtx, &pPathname)  );

	// iterate through all of our test cases
	for (uCaseIndex = 0u; uCaseIndex < SG_NrElements(u0068__add_patterns__file__cases); ++uCaseIndex)
	{
		const u0068__add_patterns__file__case* pCase        = u0068__add_patterns__file__cases + uCaseIndex;
		SG_uint32                              uEndingIndex = 0u;

		// iterate through all of our line endings
		for (uEndingIndex = 0u; uEndingIndex < SG_NrElements(u0068__add_patterns__file__endings); ++uEndingIndex)
		{
			const char*  szEnding   = u0068__add_patterns__file__endings[uEndingIndex];
			SG_uint32    uLineIndex = 0u;
			SG_uint32    uTypeIndex = 0u;

			// write the test file
			VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0777, &pFile)  );
			bFileExists = SG_TRUE;
			while (pCase->ppFileLines[uLineIndex] != NULL)
			{
				if (uLineIndex > 0u)
				{
					VERIFY_ERR_CHECK(  SG_file__write__format(pCtx, pFile, "%s", szEnding)  );
				}
				VERIFY_ERR_CHECK(  SG_file__write__format(pCtx, pFile, "%s", pCase->ppFileLines[uLineIndex])  );
				++uLineIndex;
			}
			if (pCase->bTrailingNewline == SG_TRUE)
			{
				VERIFY_ERR_CHECK(  SG_file__write__format(pCtx, pFile, "%s", szEnding)  );
			}
			VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

			// iterate through each pattern type
			for (uTypeIndex = 1u; uTypeIndex != SG_FILE_SPEC__PATTERN__LAST; uTypeIndex <<= 1u)
			{
				SG_file_spec__pattern_type eType   = (SG_file_spec__pattern_type)uTypeIndex;
				SG_bool                    bOutput = SG_FALSE;

				// if you need to debug through a specific case
				// then update this if statement to check for the index you care about and stick a breakpoint inside
				// failure messages include the index of the failing case for easy reference
				if (uCaseIndex == SG_UINT32_MAX && uEndingIndex == SG_UINT32_MAX && uTypeIndex == SG_UINT32_MAX)
				{
					bOutput = SG_FALSE;
				}

				// add the patterns from the file to the filespec as the current type
				VERIFY_ERR_CHECK(  SG_file_spec__add_patterns__file(pCtx, pFilespec, eType, pPathname, 0u)  );

				// make sure that we get the expected result from has_pattern
				VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
				VERIFYP_COND("add_patterns__file__run_test_cases: has_pattern result didn't match expectation", bOutput == pCase->bHasPattern, ("case_index(%d) ending_index(%d) type_index(%d)", uCaseIndex, uEndingIndex, uTypeIndex));

				// if the current type is the type that this test case has a path for, then run matches
				if (eType == pCase->eType)
				{
					SG_file_spec__match_result eOutput = SG_FILE_SPEC__RESULT__MAYBE;

					// make sure that we get the expected result from match_path
					VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, pCase->szPath, pCase->uFlags, &bOutput)  );
					VERIFYP_COND("add_patterns__file__run_test_cases: match_path result didn't match expectation", bOutput == pCase->bMatch, ("case_index(%d)", uCaseIndex));

					// make sure that we get the expected result from should_include
					VERIFY_ERR_CHECK(  SG_file_spec__should_include(pCtx, pFilespec, pCase->szPath, pCase->uFlags, &eOutput)  );
					VERIFYP_COND("add_patterns__file__run_test_cases: should_include result didn't match expectation", eOutput == pCase->eResult, ("case_index(%d)", uCaseIndex));
				}

				// clear those patterns out so they don't influence the next type
				VERIFY_ERR_CHECK(  SG_file_spec__clear_patterns(pCtx, pFilespec, eType)  );
				VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
				VERIFYP_COND_FAIL("add_patterns__file__basic: patterns still exist after being cleared", bOutput == SG_FALSE, ("case_index(%d)", uCaseIndex));
			}

			// delete the test file
			VERIFY_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPathname)  );
			bFileExists = SG_FALSE;
		}
	}

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	if (bFileExists == SG_TRUE)
	{
		SG_context__push_level(pCtx);
		SG_fsobj__remove__pathname(pCtx, pPathname);
		SG_context__pop_level(pCtx);
	}
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__add_patterns__file__badargs(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType      = SG_FILE_SPEC__PATTERN__INCLUDE;
	static const char*                      szFilename = "test_file.txt";

	SG_file_spec* pFilespec = NULL;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__file__sz(pCtx, NULL, eType, szFilename, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__file__sz(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__LAST, szFilename, 0u), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__add_patterns__file__sz(pCtx, pFilespec, eType, NULL, 0u), SG_ERR_INVALIDARG  );
}

static void u0068__add_patterns__file__badfiles(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType               = SG_FILE_SPEC__PATTERN__INCLUDE;
	static const char*                      szFilenameNotExists = "not_exists.txt";

	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	// non-existent file
	// TODO: run this on all platforms, figure out the error codes they throw, and implement platform-specific checks for those codes
	//       rather than just checking for any error code at all
	VERIFY_ERR_CHECK_HAS_ERR_DISCARD(  SG_file_spec__add_patterns__file__sz(pCtx, pFilespec, eType, szFilenameNotExists, 0u)  );
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
	VERIFY_COND_FAIL("add_patterns__file__badfiles: patterns added from file that doesn't exist", bOutput == SG_FALSE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}


/*
**
** load_ignores
**
*/

/**
 * A single test case for load_ignores.
 */
typedef struct
{
	const char* szIgnorePattern;  //< The ignore pattern to load.
	SG_uint32   uIgnoreFlags;     //< Flags to associate with the ignore pattern.
	const char* szIgnoredPath;    //< A path that should be ignored due to the ignore pattern.
	SG_uint32   uIgnoredFlags;    //< Flags to associate with the ignored path.
	const char* szNonIgnoredPath; //< A path that should not be ignored due to the ignore pattern.
	SG_uint32   uNonIgnoredFlags; //< Flags to associate with the non-ignored path.
}
u0068__load_ignores__case;

/**
 * A list of test cases to run.
 *
 * Note: The commented out cases are because MATCH_ANYWHERE is currently implied with all ignore patterns.
 */
static const u0068__load_ignores__case u0068__load_ignores__cases[] = {
	{ "**/Debug/**", 0u, "foo/Debug/bar.txt", 0u, "foo/bar.txt", 0u },
	{ "Debug/**", SG_FILE_SPEC__MATCH_ANYWHERE, "foo/Debug/bar.txt", 0u, "foo/bar.txt", 0u },
	{ "**/Debug", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY, "foo/Debug/bar.txt", 0u, "foo/bar.txt", 0u },
	{ "Debug", SG_FILE_SPEC__MATCH_ANYWHERE | SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY, "foo/Debug/bar.txt", 0u, "foo/bar.txt", 0u },
};

static void u0068__load_ignores__run_test_cases(SG_context* pCtx)
{
	char          szRepoName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname*  pWorkingDir       = NULL;
	SG_bool       bWorkingDirExists = SG_FALSE;
	SG_repo*      pRepo             = NULL;
	SG_uint32     uIndex            = 0u;
	SG_string*    pSetting          = NULL;
	SG_file_spec* pFilespec         = NULL;

	// create and open a test repo
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, szRepoName, sizeof(szRepoName), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pWorkingDir, szRepoName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pWorkingDir)  );
	bWorkingDirExists = SG_TRUE;
	VERIFY_ERR_CHECK(  _ut_pt__new_repo(pCtx, szRepoName, pWorkingDir)  );
	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, szRepoName, &pRepo)  );

	// build the localsetting string for ignores in this repo
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pSetting)  );
	VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, pSetting, "%s/%s/%s", SG_LOCALSETTING__SCOPE__INSTANCE, szRepoName, SG_LOCALSETTING__IGNORES)  );

	// allocate a filespec to test with
	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uIndex = 0u; uIndex < SG_NrElements(u0068__load_ignores__cases); ++uIndex)
	{
		const u0068__load_ignores__case* pCase   = u0068__load_ignores__cases + uIndex;
		SG_bool                          bOutput = SG_TRUE;

		// set the ignore pattern in the repo
		VERIFY_ERR_CHECK(  SG_localsettings__varray__append(pCtx, SG_string__sz(pSetting), pCase->szIgnorePattern)  );

		// verify that the filespec has no patterns
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__ALL, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: patterns already exist in filespec", bOutput == SG_FALSE, ("index(%d)", uIndex));

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			bOutput = SG_FALSE;
		}

		// load the ignores into the filespec
		VERIFY_ERR_CHECK(  SG_file_spec__load_ignores(pCtx, pFilespec, pRepo, NULL, pCase->uIgnoreFlags)  );

		// verify that the filespec now has ignores
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__IGNORE, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: ignores don't exist after being loaded", bOutput == SG_TRUE, ("index(%d)", uIndex));

		// verify that the filespec has no other pattern types
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__INCLUDE | SG_FILE_SPEC__PATTERN__EXCLUDE, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: non-ignore patterns exist after loading ignores", bOutput == SG_FALSE, ("index(%d)", uIndex));

		// verify that the paths match correctly
		VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, pCase->szIgnoredPath, pCase->uIgnoredFlags, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: path wasn't correctly ignored", bOutput == SG_FALSE, ("index(%d) ignores(%s) pattern(%s) flags(0x%04X) expected(ignored/non-match)", uIndex, pCase->szIgnorePattern, pCase->szIgnoredPath, pCase->uIgnoredFlags));
		VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, pCase->szNonIgnoredPath, pCase->uNonIgnoredFlags, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: path was incorrectly ignored", bOutput == SG_TRUE, ("index(%d) ignores(%s) pattern(%s) flags(0x%04X) expected(not-ignored/match)", uIndex, pCase->szIgnorePattern, pCase->szNonIgnoredPath, pCase->uNonIgnoredFlags));
		VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, pCase->szIgnoredPath, pCase->uIgnoredFlags | SG_FILE_SPEC__NO_IGNORES, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: path was incorrectly ignored with NO_IGNORES flag", bOutput == SG_TRUE, ("index(%d) ignores(%s) pattern(%s) flags(0x%04X) expected(not-ignored/match)", uIndex, pCase->szIgnorePattern, pCase->szIgnoredPath, pCase->uIgnoredFlags | SG_FILE_SPEC__NO_IGNORES));

		// clear the ignores from the filespec for the next case
		VERIFY_ERR_CHECK(  SG_file_spec__clear_patterns(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__IGNORE)  );
		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__IGNORE, &bOutput)  );
		VERIFYP_COND("load_ignores__run_test_cases: ignores still exist in filespec after being cleared", bOutput == SG_FALSE, ("index(%d)", uIndex));
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_STRING_NULLFREE(pCtx, pSetting);
	SG_REPO_NULLFREE(pCtx, pRepo);
	if (bWorkingDirExists == SG_TRUE)
	{
		SG_context__push_level(pCtx);
		SG_fsobj__rmdir_recursive__pathname(pCtx, pWorkingDir);
		SG_context__pop_level(pCtx);
		bWorkingDirExists = SG_FALSE;
	}
	SG_PATHNAME_NULLFREE(pCtx, pWorkingDir);
	return;
}


/*
**
** clear_patterns
**
*/

static void u0068__clear_patterns__per_type(SG_context* pCtx)
{
	static const char* szPatterns[] = {
		"test pattern",
		"another pattern",
		"whatever pattern",
		"foo pattern",
		"bar pattern",
	};

	SG_file_spec* pFilespec  = NULL;
	SG_uint32     uTypeIndex = 0u;
	SG_bool       bOutput    = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uTypeIndex = 1u; uTypeIndex != SG_FILE_SPEC__PATTERN__LAST; uTypeIndex <<= 1)
	{
		SG_uint32                  uPatternIndex = 0u;
		SG_file_spec__pattern_type eType         = (SG_file_spec__pattern_type)uTypeIndex;

		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("clear_patterns__per_type: patterns already exist before being added", bOutput == SG_FALSE);

		for (uPatternIndex = 0u; uPatternIndex < SG_NrElements(szPatterns); ++uPatternIndex)
		{
			VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, eType, szPatterns[uPatternIndex], 0u)  );
		}

		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("clear_patterns__per_type: patterns don't exist after being added", bOutput == SG_TRUE);

		VERIFY_ERR_CHECK(  SG_file_spec__clear_patterns(pCtx, pFilespec, eType)  );

		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("clear_patterns__per_type: patterns still exist after being cleared", bOutput == SG_FALSE);
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__clear_patterns__all_types(SG_context* pCtx)
{
	static const char* szPatterns[] = {
		"test pattern",
		"another pattern",
		"whatever pattern",
		"foo pattern",
		"bar pattern",
	};

	SG_file_spec* pFilespec  = NULL;
	SG_uint32     uTypeIndex = 0u;
	SG_bool       bOutput    = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uTypeIndex = 1u; uTypeIndex != SG_FILE_SPEC__PATTERN__LAST; uTypeIndex <<= 1)
	{
		SG_uint32                  uPatternIndex = 0u;
		SG_file_spec__pattern_type eType         = (SG_file_spec__pattern_type)uTypeIndex;

		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("clear_patterns__all_types: patterns already exist before being added", bOutput == SG_FALSE);

		for (uPatternIndex = 0u; uPatternIndex < SG_NrElements(szPatterns); ++uPatternIndex)
		{
			VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, eType, szPatterns[uPatternIndex], 0u)  );
		}

		VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, &bOutput)  );
		VERIFY_COND("clear_patterns__all_types: patterns don't exist after being added", bOutput == SG_TRUE);
	}

	VERIFY_ERR_CHECK(  SG_file_spec__clear_patterns(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__ALL)  );

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__ALL, &bOutput)  );
	VERIFY_COND("clear_patterns__all_types: patterns still exist after being cleared", bOutput == SG_FALSE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__clear_patterns__cross_types(SG_context* pCtx)
{
	static const char* szPatterns[] = {
		"test pattern",
		"another pattern",
		"whatever pattern",
		"foo pattern",
		"bar pattern",
	};
	static const SG_file_spec__pattern_type eType1 = SG_FILE_SPEC__PATTERN__INCLUDE;
	static const SG_file_spec__pattern_type eType2 = SG_FILE_SPEC__PATTERN__EXCLUDE;

	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_FALSE;
	SG_uint32     uIndex    = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType1, &bOutput)  );
	VERIFY_COND("clear_patterns__cross_types: patterns of type1 already exist before being added", bOutput == SG_FALSE);

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType2, &bOutput)  );
	VERIFY_COND("clear_patterns__cross_types: patterns of type2 already exist before being added", bOutput == SG_FALSE);

	for (uIndex = 0u; uIndex < SG_NrElements(szPatterns); ++uIndex)
	{
		VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, eType1, szPatterns[uIndex], 0u)  );
		VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, eType2, szPatterns[uIndex], 0u)  );
	}

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType1, &bOutput)  );
	VERIFY_COND("clear_patterns__cross_types: patterns of type1 don't exist after being added", bOutput == SG_TRUE);

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType2, &bOutput)  );
	VERIFY_COND("clear_patterns__cross_types: patterns of type2 don't exist after being added", bOutput == SG_TRUE);

	VERIFY_ERR_CHECK(  SG_file_spec__clear_patterns(pCtx, pFilespec, eType1)  );

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType1, &bOutput)  );
	VERIFY_COND("clear_patterns__cross_types: patterns of type1 still exist after being cleared", bOutput == SG_FALSE);

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, eType2, &bOutput)  );
	VERIFY_COND("clear_patterns__cross_types: patterns of type1 don't exist after only patterns of type2 were cleared", bOutput == SG_TRUE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__clear_patterns__badargs(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType = SG_FILE_SPEC__PATTERN__INCLUDE;

	SG_file_spec* pFilespec = NULL;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__clear_patterns(pCtx, NULL, eType), SG_ERR_INVALIDARG  );
}


/*
**
** has_pattern
**
*/

static void u0068__has_pattern__fresh(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_TRUE;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__ALL, &bOutput)  );
	VERIFY_COND("has_pattern__fresh: new filespec has patterns", bOutput == SG_FALSE);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__has_pattern__blank(SG_context* pCtx)
{
	SG_bool bOutput = SG_TRUE;

	VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, gpBlankFilespec, SG_FILE_SPEC__PATTERN__ALL, &bOutput)  );
	VERIFY_COND("has_pattern__blank: blank filespec has patterns", bOutput == SG_FALSE);

fail:
	return;
}

static void u0068__has_pattern__badargs(SG_context* pCtx)
{
	static const SG_file_spec__pattern_type eType = SG_FILE_SPEC__PATTERN__ALL;

	SG_file_spec* pFilespec = NULL;
	SG_bool       bOutput   = SG_FALSE;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__has_pattern(pCtx, NULL, eType, &bOutput), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__has_pattern(pCtx, pFilespec, eType, NULL), SG_ERR_INVALIDARG  );
}

// before/after adding/clearing patterns is covered by the clear_patterns tests


/*
**
** get_flags
**
*/

static void u0068__get_flags__fresh(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 1234u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("get_flags__fresh: new filespec has flags set", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__get_flags__blank(SG_context* pCtx)
{
	SG_uint32 uOutput = 1234u;

	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, gpBlankFilespec, &uOutput)  );
	VERIFY_COND("get_flags__blank: blank filespec has flags set", uOutput == 0u);

fail:
	return;
}

static void u0068__get_flags__badargs(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 0u;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__get_flags(pCtx, NULL, &uOutput), SG_ERR_INVALIDARG  );
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__get_flags(pCtx, pFilespec, NULL), SG_ERR_INVALIDARG  );
}

// before/after setting flags is covered by the set_flags and modify_flags tests


/*
**
** set_flags
**
*/

static void u0068__set_flags__basic(SG_context* pCtx)
{
	static const SG_uint32 uFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("set_flags__basic: new filespec has non-zero flags", uOutput == 0u);

	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, uFlags, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("set_flags__basic: filespec has different flags than were just set", uOutput == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, 0u, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("set_flags__basic: filespec has non-zero flags after zero was set", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__set_flags__old(SG_context* pCtx)
{
	static const SG_uint32 uFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, uFlags, &uOutput)  );
	VERIFY_COND("set_flags__old: new filespec returned non-zero old flags", uOutput == 0u);

	VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, 0u, &uOutput)  );
	VERIFY_COND("set_flags__old: filespec returned different old flags than were just set", uOutput == uFlags);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__set_flags__badargs(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	pFilespec = (SG_file_spec*)&pFilespec; // just needs to be some non-NULL pointer

	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_file_spec__set_flags(pCtx, NULL, 0u, NULL), SG_ERR_INVALIDARG  );
}


/*
**
** modify_flags
**
*/

static void u0068__modify_flags__basic(SG_context* pCtx)
{
	static const SG_uint32 uFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__basic: new filespec has non-zero flags", uOutput == 0u);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, uFlags, 0u, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__basic: filespec has different flags than were just set", uOutput == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, uFlags, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__basic: filespec still has flags that were just removed", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__modify_flags__multiple(SG_context* pCtx)
{
	SG_file_spec* pFilespec = NULL;
	SG_uint32     uFlags    = 0u;
	SG_uint32     uOutput   = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: new filespec has non-zero flags", uOutput == 0u);

	uFlags |= SG_FILE_SPEC__NO_INCLUDES;
	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, SG_FILE_SPEC__NO_INCLUDES, 0u, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec flags different than expected (1)", uOutput == uFlags);

	uFlags |= SG_FILE_SPEC__NO_EXCLUDES;
	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, SG_FILE_SPEC__NO_EXCLUDES, 0u, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec flags different than expected (2)", uOutput == uFlags);

	uFlags |= SG_FILE_SPEC__NO_IGNORES;
	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, SG_FILE_SPEC__NO_IGNORES, 0u, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec flags different than expected (3)", uOutput == uFlags);

	uFlags &= ~SG_FILE_SPEC__NO_INCLUDES;
	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, SG_FILE_SPEC__NO_INCLUDES, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec flags different than expected (4)", uOutput == uFlags);

	uFlags &= ~SG_FILE_SPEC__NO_EXCLUDES;
	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, SG_FILE_SPEC__NO_EXCLUDES, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec flags different than expected (5)", uOutput == uFlags);

	uFlags &= ~SG_FILE_SPEC__NO_IGNORES;
	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, SG_FILE_SPEC__NO_IGNORES, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec flags different than expected (6)", uOutput == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__get_flags(pCtx, pFilespec, &uOutput)  );
	VERIFY_COND("modify_flags__multiple: filespec still has flags after all were individually removed", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__modify_flags__old(SG_context* pCtx)
{
	static const SG_uint32 uFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, uFlags, 0u, &uOutput, NULL)  );
	VERIFY_COND("modify_flags__old: new filespec returned non-zero flags", uOutput == 0u);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, uFlags, &uOutput, NULL)  );
	VERIFY_COND("modify_flags__old: filespec returned different old flags than were just set", uOutput == uFlags);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__modify_flags__new(SG_context* pCtx)
{
	static const SG_uint32 uFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec = NULL;
	SG_uint32     uOutput   = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, uFlags, 0u, NULL, &uOutput)  );
	VERIFY_COND("modify_flags__new: filespec returned different new flags than were just set", uOutput == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, uFlags, NULL, &uOutput)  );
	VERIFY_COND("modify_flags__new: filespec returned different new flags than were just set", uOutput == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__modify_flags__old_new(SG_context* pCtx)
{
	static const SG_uint32 uFlags = SG_FILE_SPEC__NO_IGNORES;

	SG_file_spec* pFilespec  = NULL;
	SG_uint32     uOutputOld = 123u;
	SG_uint32     uOutputNew = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, uFlags, 0u, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__old_new: new filespec returned non-zero flags", uOutputOld == 0u);
	VERIFY_COND("modify_flags__old_new: filespec returned different new flags than were just set", uOutputNew == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, uFlags, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__old_new: filespec returned different old flags than were just set", uOutputOld == uFlags);
	VERIFY_COND("modify_flags__old_new: filespec returned different new flags than were just set", uOutputNew == 0u);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__modify_flags__multiple_old_new(SG_context* pCtx)
{
	SG_file_spec* pFilespec  = NULL;
	SG_uint32     uFlags     = 0u;
	SG_uint32     uOutputOld = 123u;
	SG_uint32     uOutputNew = 123u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, SG_FILE_SPEC__NO_INCLUDES, 0u, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__multiple_old_new: new filespec returned non-zero flags (1)", uOutputOld == uFlags);
	uFlags |= SG_FILE_SPEC__NO_INCLUDES;
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different new flags than were just set (1)", uOutputNew == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, SG_FILE_SPEC__NO_EXCLUDES, 0u, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different old flags than were just set (2)", uOutputOld == uFlags);
	uFlags |= SG_FILE_SPEC__NO_EXCLUDES;
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different new flags than were just set (2)", uOutputNew == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, SG_FILE_SPEC__NO_IGNORES, 0u, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different old flags than were just set (3)", uOutputOld == uFlags);
	uFlags |= SG_FILE_SPEC__NO_IGNORES;
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different new flags than were just set (3)", uOutputNew == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, SG_FILE_SPEC__NO_INCLUDES, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different old flags than were just set (4)", uOutputOld == uFlags);
	uFlags &= ~SG_FILE_SPEC__NO_INCLUDES;
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different new flags than were just set (4)", uOutputNew == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, SG_FILE_SPEC__NO_EXCLUDES, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different old flags than were just set (5)", uOutputOld == uFlags);
	uFlags &= ~SG_FILE_SPEC__NO_EXCLUDES;
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different new flags than were just set (5)", uOutputNew == uFlags);

	VERIFY_ERR_CHECK(  SG_file_spec__modify_flags(pCtx, pFilespec, 0u, SG_FILE_SPEC__NO_IGNORES, &uOutputOld, &uOutputNew)  );
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different old flags than were just set (6)", uOutputOld == uFlags);
	uFlags &= ~SG_FILE_SPEC__NO_IGNORES;
	VERIFY_COND("modify_flags__multiple_old_new: filespec returned different new flags than were just set (6)", uOutputNew == uFlags);

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}


/*
**
** match_path
**
*/

/**
 * Specifies how a pattern list should be setup for a test case.
 */
typedef enum
{
	U0068__MATCH_PATH__EMPTY,    //< Pattern list should be empty.
	U0068__MATCH_PATH__MATCH,    //< Pattern list should contain a match.
	U0068__MATCH_PATH__NONMATCH, //< Pattern list should contain a non-match.
	U0068__MATCH_PATH__BOTH      //< Pattern list should contain a match AND a non-match.
}
u0068__match_path__pattern_spec;

/**
 * Strings associated with u0068__match_path__pattern_spec enum values.
 * Used to print out the value of the enum in trace/error messages.
 */
static const char* u0068__match_path__pattern_spec_strings[] = {
	"empty",
	"match",
	"non-match",
	"match AND non-match",
};

/**
 * A single test case for match_path.
 */
typedef struct
{
	SG_bool                         bMatches;       //< The expected output from match_path.
	SG_file_spec__match_result      eResult;        //< The expected output from should_include.
	u0068__match_path__pattern_spec eIncludes;      //< How the includes list should be setup.
	u0068__match_path__pattern_spec eExcludes;      //< How the excludes list should be setup.
	u0068__match_path__pattern_spec eIgnores;       //< How the ignores list should be setup.
	SG_uint32                       uFilespecFlags; //< Flags that the filespec should be setup with.
	SG_uint32                       uFunctionFlags; //< Flags to pass directly to the match function.
}
u0068__match_path__case;

/**
 * List of test cases to run on match_path.
 */
static const u0068__match_path__case u0068__match_path__cases[] = {
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, 0u, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     0u, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_INCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__IGNORED,  U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_EXCLUDES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__IMPLIED,  U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__MAYBE,    U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_TRUE,  SG_FILE_SPEC__RESULT__INCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__EMPTY,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__MATCH,    SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__NONMATCH, SG_FILE_SPEC__NO_IGNORES, 0u },
	{ SG_FALSE, SG_FILE_SPEC__RESULT__EXCLUDED, U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     U0068__MATCH_PATH__BOTH,     SG_FILE_SPEC__NO_IGNORES, 0u },
};

/**
 * A path and associated patterns to use with test cases.
 */
typedef struct
{
	const char* szPath;         //< Path to match against filespecs.
	const char* szMatch;        //< A pattern that matches the path.
	SG_uint32   uMatchFlags;    //< Flags to use with the match pattern.
	const char* szNonMatch;     //< A pattern that doesn't match the path.
	SG_uint32   uNonMatchFlags; //< Flags to use with the non-match pattern.
}
u0068__match_path__path;

/**
 * A list of paths/patterns to run against each test case.
 * u0068__match_path__check_paths verifies that these patterns all match or not as expected, using match_pattern.
 */
static const u0068__match_path__path u0068__match_path__paths[] = {
	// basic
	{ "foo/bar/file.txt", "**/file.txt", 0u, "foo/bar.txt", 0u },

	// matching pattern only matches because of flag
	{ "foo/bar/file.txt", "file.txt", SG_FILE_SPEC__MATCH_ANYWHERE, "foo/bar.txt", 0u },

	// non-matching pattern would match except for flag
	{ "foo/bar/file.txt", "**/file.txt", 0u, "foo/bar/file.txt", SG_FILE_SPEC__MATCH_REPO_ROOT },

	// matching and non-matching patterns both only work because of flag
	{ "foo/bar/file.txt", "file.txt", SG_FILE_SPEC__MATCH_ANYWHERE, "foo/bar/file.txt", SG_FILE_SPEC__MATCH_REPO_ROOT },
};

static void u0068__match_path__check_paths(SG_context* pCtx)
{
	SG_uint32 uPathIndex = 0u;

	for (uPathIndex = 0u; uPathIndex < SG_NrElements(u0068__match_path__paths); ++uPathIndex)
	{
		const u0068__match_path__path* pPath   = u0068__match_path__paths + uPathIndex;
		SG_bool                        bOutput = SG_FALSE;

		VERIFY_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, pPath->szMatch, pPath->szPath, pPath->uMatchFlags, &bOutput)  );
		VERIFYP_COND("match_path__check_paths: test path doesn't match as expected", bOutput == SG_TRUE, ("path(%s), pattern(%s), flags(0x%04X), expected(match)", pPath->szPath, pPath->szMatch, pPath->uMatchFlags));

		VERIFY_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, pPath->szNonMatch, pPath->szPath, pPath->uNonMatchFlags, &bOutput)  );
		VERIFYP_COND("match_path__check_paths: test path doesn't match as expected", bOutput == SG_FALSE, ("path(%s), pattern(%s), flags(0x%04X), expected(non-match)", pPath->szPath, pPath->szNonMatch, pPath->uNonMatchFlags));
	}

fail:
	return;
}

static void u0068__match_path__run_test_cases(SG_context* pCtx)
{
	SG_file_spec* pFilespec  = NULL;
	SG_uint32     uPathIndex = 0u;

	VERIFY_ERR_CHECK(  SG_FILE_SPEC__ALLOC(pCtx, &pFilespec)  );

	for (uPathIndex = 0u; uPathIndex < SG_NrElements(u0068__match_path__paths); ++uPathIndex)
	{
		SG_uint32                      uCaseIndex = 0u;
		const u0068__match_path__path* pPath      = u0068__match_path__paths + uPathIndex;

		for (uCaseIndex = 0u; uCaseIndex < SG_NrElements(u0068__match_path__cases); ++uCaseIndex)
		{
			SG_bool                        bOutput       = SG_TRUE;
			SG_file_spec__match_result     eOutput       = SG_FILE_SPEC__RESULT__MAYBE;
			SG_uint32                      uPatternIndex = 0u;
			const u0068__match_path__case* pCase         = u0068__match_path__cases + uCaseIndex;

			// make sure the filespec is clean
			VERIFY_ERR_CHECK(  SG_file_spec__has_pattern(pCtx, pFilespec, SG_FILE_SPEC__PATTERN__ALL, &bOutput)  );
			VERIFYP_COND("match_path__run_test_cases: filespec already has patterns at start of test case", bOutput == SG_FALSE, ("path_index(%d) case_index(%d)", uPathIndex, uCaseIndex));

			// configure the filespec patterns one at a time
			for (uPatternIndex = 1u; uPatternIndex != SG_FILE_SPEC__PATTERN__LAST; uPatternIndex <<= 1)
			{
				u0068__match_path__pattern_spec ePatternSpec = U0068__MATCH_PATH__EMPTY;
				SG_file_spec__pattern_type      ePatternType = (SG_file_spec__pattern_type)uPatternIndex;

				// find the pattern_spec in the case that matches the current pattern type
				switch (uPatternIndex)
				{
				case SG_FILE_SPEC__PATTERN__INCLUDE: ePatternSpec = pCase->eIncludes; break;
				case SG_FILE_SPEC__PATTERN__EXCLUDE: ePatternSpec = pCase->eExcludes; break;
				case SG_FILE_SPEC__PATTERN__IGNORE:  ePatternSpec = pCase->eIgnores;  break;
				case SG_FILE_SPEC__PATTERN__RESERVE: ePatternSpec = U0068__MATCH_PATH__EMPTY; break;
				default: VERIFYP_COND("match_path__run_test_cases: unknown pattern type", SG_FALSE, ("path_index(%d) case_index(%d), pattern_index(%d)", uPathIndex, uCaseIndex, uPatternIndex)); break;
				}

				// check the pattern_spec to see which pattern(s) to add to the filespec for the current pattern type
				switch (ePatternSpec)
				{
				case U0068__MATCH_PATH__EMPTY:
					break;
				case U0068__MATCH_PATH__MATCH:
					VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, ePatternType, pPath->szMatch, pPath->uMatchFlags)  );
					break;
				case U0068__MATCH_PATH__NONMATCH:
					VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, ePatternType, pPath->szNonMatch, pPath->uNonMatchFlags)  );
					break;
				case U0068__MATCH_PATH__BOTH:
					VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, ePatternType, pPath->szMatch, pPath->uMatchFlags)  );
					VERIFY_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pFilespec, ePatternType, pPath->szNonMatch, pPath->uNonMatchFlags)  );
					break;
				}
			}

			// configure the filespec flags
			VERIFY_ERR_CHECK(  SG_file_spec__set_flags(pCtx, pFilespec, pCase->uFilespecFlags, NULL)  );

			// if you need to debug through a specific case
			// then update this if statement to check for the index you care about and stick a breakpoint inside
			// failure messages include the index of the failing case for easy reference
			if (uCaseIndex == SG_UINT32_MAX && uPathIndex == SG_UINT32_MAX)
			{
				bOutput = SG_FALSE;
			}

			// run the match_path test case
			VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, pFilespec, pPath->szPath, pCase->uFunctionFlags, &bOutput)  );
			VERIFYP_COND("match_path__run_test_cases: match_path test case failed", bOutput == pCase->bMatches, ("path_index(%d) case_index(%d) includes(%s) excludes(%s) ignores(%s) filespec_flags(0x%04X) function_flags(0x%04X) expected(%s)", uPathIndex, uCaseIndex, u0068__match_path__pattern_spec_strings[pCase->eIncludes], u0068__match_path__pattern_spec_strings[pCase->eExcludes], u0068__match_path__pattern_spec_strings[pCase->eIgnores], pCase->uFilespecFlags, pCase->uFunctionFlags, pCase->bMatches == SG_TRUE ? "match" : "non-match"));

			// run the should_include test case
			VERIFY_ERR_CHECK(  SG_file_spec__should_include(pCtx, pFilespec, pPath->szPath, pCase->uFunctionFlags, &eOutput)  );
			VERIFYP_COND("match_path__run_test_cases: should_include test case failed", eOutput == pCase->eResult, ("path_index(%d) case_index(%d) includes(%s) excludes(%s) ignores(%s) filespec_flags(0x%04X) function_flags(0x%04X) expected(SG_file_spec__match_result: 0x%02X)", uPathIndex, uCaseIndex, u0068__match_path__pattern_spec_strings[pCase->eIncludes], u0068__match_path__pattern_spec_strings[pCase->eExcludes], u0068__match_path__pattern_spec_strings[pCase->eIgnores], pCase->uFilespecFlags, pCase->uFunctionFlags, pCase->eResult));

			// reset the filespec for the next test
			VERIFY_ERR_CHECK(  SG_file_spec__reset(pCtx, pFilespec)  );
		}
	}

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
}

static void u0068__match_path__blank(SG_context* pCtx)
{
	SG_uint32 uPathIndex = 0u;

	for (uPathIndex = 0u; uPathIndex < SG_NrElements(u0068__match_path__paths); ++uPathIndex)
	{
		const u0068__match_path__path* pPath   = u0068__match_path__paths + uPathIndex;
		SG_bool                        bOutput = SG_FALSE;
		SG_file_spec__match_result     eOutput = SG_FILE_SPEC__RESULT__MAYBE;

		// run the match_path test case
		VERIFY_ERR_CHECK(  SG_file_spec__match_path(pCtx, gpBlankFilespec, pPath->szPath, 0u, &bOutput)  );
		VERIFY_COND("match_path__blank: blank filespec got match_path result other than TRUE", bOutput == SG_TRUE);

		// run the should_include test cases
		VERIFY_ERR_CHECK(  SG_file_spec__should_include(pCtx, gpBlankFilespec, pPath->szPath, 0u, &eOutput)  );
		VERIFY_COND("match_path__blank: blank filespec got should_include result other than implied", eOutput == SG_FILE_SPEC__RESULT__IMPLIED);
	}

fail:
	return;
}


/*
**
** should_include
**
*/

// all cases covered along with parallel cases of match_path


/*
**
** match_pattern
**
*/

/**
 * A single test case for match_pattern.
 */
typedef struct
{
	SG_bool     bMatches;   //< The result we expect the function to return.
	const char* szPattern;  //< The pattern to pass in as a test.
	const char* szFilename; //< The path to pass in as a test.
	SG_uint32   uFlags;     //< The flags to pass in as a test.
}
u0068__match_pattern__case;

/**
 * The set of test cases to run on match_pattern.
 *
 * NOTE: The filespec_bash_ref.sh script parses this source file looking for these test cases.
 *       It runs them in the Bash shell to verify that it produces the same behavior.
 *       This makes sure that our pattern matcher works like Bash's as much as possible.
 *       Lines including NOBASH (case-sensitive) are skipped from Bash testing.
 */
static const u0068__match_pattern__case u0068__match_pattern__cases[] = {
	// NULL arguments
	{ SG_FALSE, NULL, "whatever", 0u },
	{ SG_TRUE,  NULL, "whatever", SG_FILE_SPEC__MATCH_NULL_PATTERN },
	{ SG_FALSE, "whatever", NULL, 0u },
	{ SG_TRUE,  "whatever", NULL, SG_FILE_SPEC__MATCH_NULL_FILENAME },

	// no wildcards
	{ SG_TRUE,  "foo", "foo", 0u },
	{ SG_FALSE, "foo", "bar", 0u },
	{ SG_FALSE, "fo",  "foo", 0u },
	{ SG_FALSE, "foo", "fo",  0u },

	// equivalence of slashes ('/' matches '\')
	// keep in mind we have to escape backslashes for the C language
	{ SG_TRUE,  "/", "\\", 0u },
	{ SG_TRUE,  "///", "\\\\\\", 0u },
	{ SG_TRUE,  "/\\/\\", "\\/\\/", 0u },
	{ SG_FALSE, "/", "x", 0u },
	{ SG_FALSE, "\\", "x", 0u },
	{ SG_TRUE,  "a/b/c/d/e", "a\\b\\c\\d\\e", 0u },
	{ SG_FALSE, "a/b/c/d/e/", "a\\b\\c\\d\\e", 0u },
	{ SG_TRUE,  "/a/b/c/d/e", "\\a\\b\\c\\d\\e", 0u },
	{ SG_FALSE, "/a/b/c/d/e/", "\\a\\b\\c\\d\\e", 0u },
	{ SG_TRUE,  "a\\b\\c\\d\\e", "a/b/c/d/e", 0u },
	{ SG_FALSE, "a\\b\\c\\d\\e\\", "a/b/c/d/e", 0u },
	{ SG_TRUE,  "\\a\\b\\c\\d\\e", "/a/b/c/d/e", 0u },
	{ SG_FALSE, "\\a\\b\\c\\d\\e\\", "/a/b/c/d/e", 0u },

	// trailing slashes
	{ SG_TRUE,  "foo", "foo", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo", "foo/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo", "foo\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "foo", "foo//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "foo", "foo\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "foo", "foo/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "foo", "foo\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },

	// simple ? wildcards (beginning, middle, end, all)
	{ SG_TRUE,  "?oo", "foo", 0u },
	{ SG_FALSE, "?foo", "foo", 0u },
	{ SG_TRUE,  "f?o", "foo", 0u },
	{ SG_TRUE,  "fo?", "foo", 0u },
	{ SG_FALSE, "foo?", "foo", 0u },
	{ SG_TRUE,  "foo.?", "foo.c", 0u },
	{ SG_TRUE,  "foo.?", "foo.h", 0u },
	{ SG_FALSE, "foo.?", "foo.cpp", 0u },
	{ SG_TRUE,  "?", "f", 0u },
	{ SG_FALSE, "?", "foo", 0u },
	{ SG_FALSE, "?", "", 0u },
	{ SG_FALSE, "?oo", "ffoo", 0u },
	{ SG_FALSE, "f?o", "fo", 0u },
	{ SG_FALSE, "f?o", "fooo", 0u },
	{ SG_TRUE,  "foo?c", "foo.c", 0u },
	{ SG_FALSE, "foo?c", "foo/c", 0u },
	{ SG_FALSE, "foo?c", "foo\\c", 0u },
	{ SG_FALSE, "fo?", "fooo", 0u },
	{ SG_FALSE, "?foo", ".foo", 0u }, // * and ? don't match leading dots
	{ SG_TRUE,  ".?oo", ".foo", 0u },
	{ SG_FALSE, "stuff/?foo", "stuff/.foo", 0u },
	{ SG_TRUE,  "stuff/.?oo", "stuff/.foo", 0u },
	{ SG_TRUE,  "stuff/.foo?", "stuff/.foo.", 0u },
	{ SG_FALSE, "?", ".", 0u },
	{ SG_FALSE, "foo/?", "foo/.", 0u },
	{ SG_TRUE,  "foo?", "foo.", 0u },
	{ SG_FALSE, "foo.?", "foo.", 0u },
	{ SG_FALSE, "foo", "foo.", 0u },
	{ SG_TRUE,  "roo?file.c", "rootfile.c", 0u },

	// multiple ? wildcards
	{ SG_TRUE,  "???", "foo", 0u },
	{ SG_TRUE,  "???", "bar", 0u },
	{ SG_FALSE, "???", "foobar", 0u },
	{ SG_TRUE,  "?o?", "foo", 0u },
	{ SG_FALSE, "?a?", "foo", 0u },
	{ SG_FALSE, "?o?", "bar", 0u },
	{ SG_TRUE,  "?a?", "bar", 0u },
	{ SG_TRUE,  "??o", "foo", 0u },
	{ SG_FALSE, "??o", "bar", 0u },
	{ SG_TRUE,  "f??", "foo", 0u },
	{ SG_FALSE, "f??", "bar", 0u },
	{ SG_TRUE,  "f??", "f.o", 0u },
	{ SG_FALSE, "f??", "f/o", 0u },
	{ SG_FALSE, "f??", "f\\o", 0u },

	// ending ? wildcards with trailing slashes
	{ SG_TRUE,  "fo?", "foo", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "fo?", "foo/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "fo?", "foo\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "foo//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "foo\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "foo/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "foo\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "fo?", "for", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "fo?", "for/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "fo?", "for\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "for//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "for\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "for/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "fo?", "for\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },

	// simple * wildcards (beginning, middle, end, all)
	{ SG_TRUE,  "*.c", "foo.c", 0u },
	{ SG_FALSE, "*.c", "foo.h", 0u },
	{ SG_TRUE,  "*bar/whee", "foobar/whee", 0u },
	{ SG_FALSE, "*foo/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "foo*/whee", "foobar/whee", 0u },
	{ SG_FALSE, "bar*/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "foo*/*ee", "foobar/whee", 0u },
	{ SG_TRUE,  "foobar/*", "foobar/whee", 0u },
	{ SG_TRUE,  "foobar/wh*", "foobar/whee", 0u },
	{ SG_TRUE,  "*", "foobar", 0u },
	{ SG_FALSE, "*", "foobar/whee", 0u },
	{ SG_TRUE,  "*.c", "foo.bar.c", 0u },
	{ SG_FALSE,  "*.c", "foo/bar.c", 0u },
	{ SG_FALSE,  "*.c", "foo\\bar.c", 0u },
	{ SG_FALSE, "*.c", "bar.c/foo.h", 0u },
	{ SG_FALSE, "*.c", "bar.c\\foo.h", 0u },
	{ SG_TRUE,  "*bar", "foobar", 0u },
	{ SG_TRUE,  "*bar", "foo.bar", 0u },
	{ SG_TRUE,  "*bar", "foot.ibar", 0u },
	{ SG_TRUE,  "foo*bar", "foo.bar", 0u },
	{ SG_TRUE,  "foo*bar", "foo.zag.bar", 0u },
	{ SG_FALSE, "foo*bar", "foo/bar", 0u },
	{ SG_FALSE, "foo*bar", "food/bar", 0u },
	{ SG_FALSE, "foo*bar", "foo/abar", 0u },
	{ SG_TRUE,  "foo/*/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/*/whee", "foo/bar/har/whee", 0u },
	{ SG_FALSE, "foo*/*ee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo*", "football", 0u },
	{ SG_TRUE,  "foo*", "food.bar", 0u },
	{ SG_FALSE, "foo*", "foo/bar", 0u },
	{ SG_FALSE, "*foo", ".foo", 0u }, // * and ? don't match leading dots
	{ SG_TRUE,  ".*oo", ".foo", 0u },
	{ SG_FALSE, "stuff/*foo", "stuff/.foo", 0u },
	{ SG_TRUE,  "stuff/.*oo", "stuff/.foo", 0u },
	{ SG_TRUE,  "stuff/.foo*", "stuff/.foo.", 0u },
	{ SG_FALSE, "*", ".", 0u },
	{ SG_FALSE, "foo/*", "foo/.", 0u },
	{ SG_FALSE, "*", ".foo", 0u },
	{ SG_FALSE, "*of*fo", "xofxofxofox", 0u },
	{ SG_TRUE,  "foo/b*/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/*r/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/b*r/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/*bar/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/b*ar/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/bar*/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/*a*/whee", "foo/bar/har/whee", 0u },
	{ SG_FALSE, "foo/*a/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/b*a/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/*aar/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/ba*ar/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/baa*/whee", "foo/bar/whee", 0u },
	{ SG_FALSE,  "X*Y", "XiYiZ", 0u },
	{ SG_FALSE,  "X/*/Y", "X/i/Y/i/Y", 0u },
	{ SG_FALSE,  "*.c", "rootfile", 0u },
	{ SG_TRUE,   "*.c", "rootfile.c", 0u },
	{ SG_TRUE,   "include/*.h", "include/level2.h", 0u },

	// reduction of * to match zero characters
	{ SG_TRUE,  "*foobar/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "****foobar/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "foobar/whee*", "foobar/whee", 0u },
	{ SG_TRUE,  "foobar/whee****", "foobar/whee", 0u },
	{ SG_TRUE,  "foo*bar/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "foo****bar/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "*f*o*o*b*a*r*/*w*h*e*e*", "foobar/whee", 0u },
	{ SG_TRUE,  "****f****o****o****b****a****r****/****w****h****e****e****", "foobar/whee", 0u },
	{ SG_TRUE,  "*", "", 0u },
	{ SG_TRUE,  "****", "", 0u },
	{ SG_FALSE, "foo/*/whee", "foo/whee", 0u },

	// greediness of * wildcards
	{ SG_TRUE,  "*.c", "foo.c.c", 0u },
	{ SG_FALSE, "*.c", "foo.c.h", 0u },
	{ SG_TRUE,  "*oo", "foo", 0u },
	{ SG_TRUE,  "*oo", "foooo", 0u },
	{ SG_TRUE,  "*oo", "foooooo", 0u },
	{ SG_FALSE, "*oo", "foooooof", 0u },
	{ SG_TRUE,  "*of", "fof", 0u },
	{ SG_TRUE,  "*of", "fofof", 0u },
	{ SG_TRUE,  "*of", "fofofof", 0u },
	{ SG_FALSE, "*of", "fofofofo", 0u },
	{ SG_FALSE, "*ofof", "fof", 0u },
	{ SG_TRUE,  "*ofof", "fofof", 0u },
	{ SG_TRUE,  "*ofof", "fofofof", 0u },
	{ SG_FALSE, "*ofof", "fofofofo", 0u },
	{ SG_TRUE,  "*X*Y*", "oooXoooYoooXooo", 0u },

	// multiple * wildcards
	{ SG_TRUE,  "*a*", "bar", 0u },
	{ SG_TRUE,  "*ob*", "foobar", 0u },
	{ SG_FALSE, "*bo*", "foobar", 0u },
	{ SG_TRUE,  "*o*r", "foobar", 0u },
	{ SG_TRUE,  "*/*a*", "whee/bar", 0u },
	{ SG_TRUE,  "*/*ob*", "whee/foobar", 0u },
	{ SG_FALSE, "*/*bo*", "whee/foobar", 0u },
	{ SG_TRUE,  "*/*o*r", "whee/foobar", 0u },
	{ SG_TRUE,  "*/foobar/*", "whee/foobar/stuff", 0u },
	{ SG_FALSE, "*/foobar/*", "whee/foobar/stuff/something", 0u },
	{ SG_FALSE, "*/foobar/*", "something/whee/foobar/stuff", 0u },
	{ SG_FALSE, "*/foo*/*", "whee/foobar", 0u },
	{ SG_TRUE,  "*/foo*/*", "whee/foobar/stuff", 0u },
	{ SG_TRUE,  "*/foo*/*", "whee/foobeep/stuff", 0u },
	{ SG_FALSE, "*/*bar/*", "whee/foobar", 0u },
	{ SG_TRUE,  "*/*bar/*", "whee/foobar/stuff", 0u },
	{ SG_FALSE, "*/*bar/*", "whee/foobeep/stuff", 0u },
	{ SG_TRUE,  "foo*/*bar", "foo/bar", 0u },
	{ SG_FALSE, "foo*/*ee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "*az*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaz", 0u },
	{ SG_TRUE,  "*az*", "azaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0u },
	{ SG_TRUE,  "*aaaaaz*", "aaaaaaaaaaaaaaaaaaaaaaaaaaazaaaaaa", 0u },
	{ SG_FALSE, "*aaaaaz*", "aaaazaaaazaaaazaaaazaaaazaaaazaaaa", 0u },
	{ SG_TRUE,  "*aaaaaz*", "aaaazaaaazaaaazaaaazaaaaazaaazaaaa", 0u },
	{ SG_FALSE, "*aaaaax*", "aaaaaaaaaaaaaaaaaaaaaaaaaaazaaaaaa", 0u },

	// ending * wildcards with trailing slashes
	{ SG_TRUE,  "f*", "fo", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "f*", "fo/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "f*", "fo\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "fo//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "fo\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "fo/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "fo\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "f*", "foobar", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "f*", "foobar/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "f*", "foobar\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "foobar//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "foobar\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "foobar/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_FALSE, "f*", "foobar\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },

	// simple ** wildcards (beginning, middle, end, all)
	{ SG_TRUE,  "**/whee", "foobar/whee", 0u },
	{ SG_TRUE,  "**/whee", "foobar/stuff/whee", 0u },
	{ SG_FALSE, "**/whee", "foobar/whee/stuff", 0u },
	{ SG_TRUE,  "whee/**/a.c", "whee/foobar/a.c", 0u },
	{ SG_TRUE,  "whee/**/a.c", "whee/foobar/something/a.c", 0u },
	{ SG_TRUE,  "whee/**/a.c", "whee/foobar/something/stuff/a.c", 0u },
	{ SG_FALSE, "whee/**/a.c", "whee/foobar/something/stuff/a.h", 0u },
	{ SG_FALSE, "whee/**/something/a.c", "whee/foobar/a.c", 0u },
	{ SG_TRUE,  "whee/**/something/a.c", "whee/foobar/something/a.c", 0u },
	{ SG_FALSE, "whee/**/something/a.c", "whee/foobar/something/a.h", 0u },
	{ SG_TRUE,  "whee/foobar/**", "whee/foobar", 0u }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE,  "whee/foobar/**", "whee/foobar/something", 0u },
	{ SG_TRUE,  "whee/foobar/**", "whee/foobar/something/stuff", 0u },
	{ SG_TRUE,  "whee/foobar/**", "whee/foobar/a.c", 0u },
	{ SG_TRUE,  "whee/foobar/**", "whee/foobar/something/a.c", 0u },
	{ SG_TRUE,  "whee/foobar/**", "whee/foobar/something/stuff/a.c", 0u },
	{ SG_TRUE,  "**", "foobar/whee", 0u },
	{ SG_TRUE,  "**", "a/b/c/d/e/f/g/foo.c", 0u },
	{ SG_TRUE,  "foo/**/whee", "foo/whee", 0u },
	{ SG_TRUE,  "foo/**/whee", "foo/stuff/whee", 0u },
	{ SG_TRUE,  "foo/**/whee", "foo/a/b/c/d/e/f/g/whee", 0u },
	{ SG_FALSE, "foo/**/whee", "foo/whee/end", 0u },
	{ SG_FALSE, "foo/**/whee", "foo/stuff/whee/end", 0u },
	{ SG_FALSE, "foo/**/whee", "foo/a/b/c/d/e/f/g/whee/end", 0u },
	{ SG_FALSE, "foo/**/whee", "beg/foo/whee", 0u },
	{ SG_FALSE, "foo/**/whee", "beg/foo/stuff/whee", 0u },
	{ SG_FALSE, "foo/**/whee", "beg/foo/a/b/c/d/e/f/g/whee", 0u },
	{ SG_TRUE,  "foo/**/whee", "foo/foo/whee/whee", 0u },
	{ SG_TRUE,  "foo/**/whee", "foo/stuff/foo/whee/gruff/whee", 0u },
	{ SG_TRUE,  "foo/**/whee", "foo/a/b/foo/c/d/e/whee/f/g/whee", 0u },
	{ SG_TRUE,  "X/**/Y", "X/i/Y/i/Y", 0u },
	{ SG_TRUE,  "whee/**", "whee/xy/whatever/foo.c", 0u },

	// reduction of ** wildcards to match zero directories
	{ SG_TRUE, "**/whee", "whee", 0u },
	{ SG_TRUE, "whee/**", "whee", 0u }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE, "foo/**/whee", "foo/whee", 0u },

	// reduction of ** wildcards to * if they're not whole directories
	{ SG_TRUE,  "**", "", 0u },
	{ SG_TRUE,  "**ee/foobar/a.c", "whee/foobar/a.c", 0u },
	{ SG_FALSE, "**ee/foobar/a.c", "stuff/whee/foobar/a.c", 0u },
	{ SG_TRUE,  "wh**/foobar/a.c", "whee/foobar/a.c", 0u },
	{ SG_FALSE, "wh**/foobar/a.c", "whee/stuff/foobar/a.c", 0u },
	{ SG_TRUE,  "whee/foo**/a.c", "whee/foobar/a.c", 0u },
	{ SG_FALSE, "whee/foo**/a.c", "whee/foobar/something/a.c", 0u },
	{ SG_FALSE, "whee/foo**/a.c", "whee/foobar/something/stuff/a.c", 0u },
	{ SG_TRUE,  "whee/**bar/a.c", "whee/foobar/a.c", 0u },
	{ SG_FALSE, "whee/**bar/a.c", "whee/foobar/something/a.c", 0u },
	{ SG_FALSE, "whee/**bar/a.c", "whee/foobar/something/stuff/a.c", 0u },
	{ SG_TRUE,  "whee/foobar/**.c", "whee/foobar/a.c", 0u },
	{ SG_FALSE, "whee/foobar/**.c", "whee/foobar/something/a.c", 0u },
	{ SG_FALSE, "whee/foobar/some**", "whee/foobar/a.c", 0u },
	{ SG_TRUE,  "whee/foobar/some**", "whee/foobar/something", 0u },
	{ SG_TRUE,  "whee/foobar/some**", "whee/foobar/some.c", 0u },
	{ SG_FALSE, "whee/foobar/some**", "whee/foobar/something/a.c", 0u },
	{ SG_TRUE,  "foo/b**/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/**r/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/b**r/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/**bar/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/b**ar/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/bar**/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/a**/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/**a/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/b**a/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/**aar/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/ba**ar/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/baa**/whee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/b**/whee", "foo/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/**r/whee", "foo/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/b**r/whee", "foo/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/**bar/whee", "foo/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/b**ar/whee", "foo/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/bar**/whee", "foo/bar/bar/whee", 0u },
	{ SG_TRUE,  "foo/bar/**ee", "foo/bar/whee", 0u },
	{ SG_FALSE, "foo/bar/**ee", "foo/bar/whee/whee", 0u },
	{ SG_FALSE, "**.c", "foo/rootfile.c", 0u },

	// greediness of ** wildcards
	{ SG_TRUE,  "foo/**/bar/whee", "foo/bar/whee", 0u },
	{ SG_TRUE,  "foo/**/bar/whee", "foo/bar/bar/whee", 0u },
	{ SG_TRUE,  "foo/**/bar/whee", "foo/bar/bar/bar/whee", 0u },
	{ SG_TRUE,  "foo/**/bar/whee", "foo/bar/bar/bar/bar/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/**/bar/whee", "foo/bar", 0u },
	{ SG_FALSE, "foo/**/bar/whee", "foo/bar/whee/whatever", 0u },
	{ SG_TRUE,  "foo/**/bar/whee", "foo/bar/bar/bar/whatever/bar/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/**/bar/whee", "foo/bar/bar/bar/whee/bar/bar/bar", 0u },
	{ SG_TRUE,  "foo/**/bar/whee", "foo/bar/bar/bar/whee/bar/bar/bar/whee", 0u },
	{ SG_FALSE, "foo/**/bar/whee", "foo/bar/bar/bar/whee/bar/bar/bar/whatever", 0u },

	// multiple ** wildcards
	{ SG_TRUE,  "**/whee/**/a.c", "whee/a.c", 0u },
	{ SG_TRUE,  "**/whee/**/a.c", "foobar/whee/a.c", 0u },
	{ SG_TRUE,  "**/whee/**/a.c", "foobar/whee/something/a.c", 0u },
	{ SG_TRUE,  "**/whee/**/a.c", "foobar/whatever/whee/something/stuff/a.c", 0u },
	{ SG_TRUE,  "**/**/foobar", "foobar", 0u },
	{ SG_TRUE,  "**/**/foobar", "whee/foobar", 0u },
	{ SG_TRUE,  "**/**/foobar", "whee/something/foobar", 0u },
	{ SG_TRUE,  "**/**/foobar", "whee/something/stuff/whatever/foobar", 0u },
	{ SG_TRUE,  "foobar/**/**", "foobar", 0u }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE,  "foobar/**/**", "foobar/whee", 0u },
	{ SG_TRUE,  "foobar/**/**", "foobar/whee/something", 0u },
	{ SG_TRUE,  "foobar/**/**", "foobar/whee/something/stuff/whatever", 0u },

	// ending ** wildcards with trailing slashes
	// note: the ending ** will match all the trailing slashes, regardless of the flag
	{ SG_TRUE,  "foo/**", "foo", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee//", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee\\\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee/\\", SG_FILE_SPEC__MATCH_TRAILING_SLASH },
	{ SG_TRUE,  "foo/**", "foo/bar/whee\\/", SG_FILE_SPEC__MATCH_TRAILING_SLASH },

	// combinations of ? and * (including multiples of each)
	{ SG_TRUE,  "*?*bar", "fooooooobar", 0u },
	{ SG_FALSE, "*?*bar", "something/foobar", 0u },
	{ SG_FALSE, "*?*bar", "bar", 0u },
	{ SG_TRUE,  "foo*?*", "foobaaaaaar", 0u },
	{ SG_TRUE,  "foo*?*", "fooooobaaar", 0u },
	{ SG_FALSE, "foo*?*", "foo", 0u },
	{ SG_TRUE,  "*?bar", "foobar", 0u },
	{ SG_TRUE,  "*?bar", "fbar", 0u },
	{ SG_FALSE, "*?bar", "bar", 0u },
	{ SG_TRUE,  "?*bar", "foobar", 0u },
	{ SG_TRUE,  "?*bar", "fbar", 0u },
	{ SG_FALSE, "?*bar", "bar", 0u },
	{ SG_TRUE,  "?*?bar", "foobar", 0u },
	{ SG_TRUE,  "?*?bar", "fobar", 0u },
	{ SG_FALSE, "?*?bar", "fbar", 0u },
	{ SG_FALSE, "?*?bar", "bar", 0u },
	{ SG_TRUE,  "fo*?ar", "fooharbar", 0u },
	{ SG_TRUE,  "fo*?ar", "foobar", 0u },
	{ SG_FALSE, "fo*?ar", "foar", 0u },
	{ SG_TRUE,  "fo?*ar", "fooharbar", 0u },
	{ SG_TRUE,  "fo?*ar", "foobar", 0u },
	{ SG_FALSE, "fo?*ar", "foar", 0u },
	{ SG_TRUE,  "fo?*?ar", "fooharbar", 0u },
	{ SG_TRUE,  "fo?*?ar", "foobar", 0u },
	{ SG_FALSE, "fo?*?ar", "fobar", 0u },
	{ SG_FALSE, "fo?*?ar", "foar", 0u },
	{ SG_TRUE,  "*a?cd?fg*", "fooa0cd0f0aba1cd1f1ababcdefgbar", 0u },
	{ SG_FALSE, "*a?cd?fg*", "fooa0cd0f0aba1cd1f1ababcdefzbar", 0u },

	// combinations of ? and ** (including multiples of each)
	{ SG_TRUE,  "**?**/foo.c", "something/foo.c", 0u },
	{ SG_FALSE, "**?**/foo.c", "whee/foobar/something/foo.c", 0u },
	{ SG_TRUE,  "**/?oo.c", "something/foo.c", 0u },
	{ SG_TRUE,  "**/??o.c", "something/foo.c", 0u },
	{ SG_TRUE,  "**/?oo.c", "whee/foobar/something/foo.c", 0u },
	{ SG_TRUE,  "**/??o.c", "whee/foobar/something/foo.c", 0u },
	{ SG_TRUE,  "**/?/**/foo.c", "whee/x/whatever/foo.c", 0u },
	{ SG_TRUE,  "**/?/**/foo.c", "whee/y/whatever/foo.c", 0u },
	{ SG_FALSE, "**/?/**/foo.c", "whee/xy/whatever/foo.c", 0u }, // NOBASH: with bash, this is TRUE.  Not sure why.
	{ SG_TRUE,  "**/?""?/**/foo.c", "whee/xy/whatever/foo.c", 0u }, // pattern separated so that ??/ isn't treated as a trigraph
	{ SG_TRUE,  "**/?", "foo/x", 0u },
	{ SG_TRUE,  "**/?", "foo/y", 0u },
	{ SG_FALSE, "**/?", "foo/xy", 0u },
	{ SG_TRUE,  "**/?", "foo/stuff/x", 0u },
	{ SG_TRUE,  "**/?", "foo/stuff/y", 0u },
	{ SG_FALSE, "**/?", "foo/stuff/xy", 0u },
	{ SG_TRUE,  "??**/foo.c", "stuff/foo.c", 0u },
	{ SG_FALSE, "??**/foo.c", "x/foo.c", 0u },
	{ SG_FALSE, "??**/foo.c", "stuff/whatever/foo.c", 0u },

	// combinations of ** and * (including multiples of each)
	{ SG_TRUE,  "**/*", "foobar/whee", 0u },
	{ SG_TRUE,  "**/*ee", "foobar/whee", 0u },
	{ SG_TRUE,  "**/whee/*", "foobar/whee/stuff", 0u },
	{ SG_FALSE, "**/whee/*", "foobar/stuff/whee", 0u },
	{ SG_TRUE,  "**/whee/*", "foobar/stuff/whee/whatever.c", 0u },
	{ SG_TRUE,  "**/*/*", "foobar/whee/stuff", 0u },
	{ SG_TRUE,  "**/*/*", "foobar/whee/stuff/whatever.c", 0u },
	{ SG_FALSE, "**/include/*.h", "include.h", 0u },
	{ SG_TRUE,  "**/include/*.h", "include/level1.h", 0u },
	{ SG_TRUE,  "**/include/*.h", "further/down/include/level3.h", 0u },

	// MATCH_ANYWHERE flag
	// (this is basically every test from above that started with "**/")
	{ SG_TRUE,  "whee", "foobar/whee", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee", "foobar/stuff/whee", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_FALSE, "whee", "foobar/whee/stuff", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee", "whee", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee/**/a.c", "whee/a.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee/**/a.c", "foobar/whee/a.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee/**/a.c", "foobar/whee/something/a.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee/**/a.c", "foobar/whatever/whee/something/stuff/a.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "**/foobar", "foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "**/foobar", "whee/foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "**/foobar", "whee/something/foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "**/foobar", "whee/something/stuff/whatever/foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "foobar", "foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "foobar", "whee/foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "foobar", "whee/something/foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "foobar", "whee/something/stuff/whatever/foobar", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?oo.c", "something/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "??o.c", "something/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?oo.c", "whee/foobar/something/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "??o.c", "whee/foobar/something/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?/**/foo.c", "whee/x/whatever/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?/**/foo.c", "whee/y/whatever/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_FALSE, "?/**/foo.c", "whee/xy/whatever/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE }, // NOBASH: with bash, this is TRUE.  Not sure why.
	{ SG_TRUE,  "?""?/**/foo.c", "whee/xy/whatever/foo.c", SG_FILE_SPEC__MATCH_ANYWHERE }, // pattern separated so that ??/ isn't treated as a trigraph
	{ SG_TRUE,  "?", "foo/x", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?", "foo/y", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_FALSE, "?", "foo/xy", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?", "foo/stuff/x", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "?", "foo/stuff/y", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_FALSE, "?", "foo/stuff/xy", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "*", "foobar/whee", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "*ee", "foobar/whee", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee/*", "foobar/whee/stuff", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_FALSE, "whee/*", "foobar/stuff/whee", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "whee/*", "foobar/stuff/whee/whatever.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "*/*", "foobar/whee/stuff", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "*/*", "foobar/whee/stuff/whatever.c", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_FALSE, "include/*.h", "include.h", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "include/*.h", "include/level1.h", SG_FILE_SPEC__MATCH_ANYWHERE },
	{ SG_TRUE,  "include/*.h", "further/down/include/level3.h", SG_FILE_SPEC__MATCH_ANYWHERE },

	// MATCH_FOLDERS_RECURSIVELY flag
	// (this is basically every test from above that ended with "/**")
	{ SG_TRUE,  "whee/foobar", "whee/foobar", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE,  "whee/foobar", "whee/foobar/something", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "whee/foobar", "whee/foobar/something/stuff", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "whee/foobar", "whee/foobar/a.c", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "whee/foobar", "whee/foobar/something/a.c", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "whee/foobar", "whee/foobar/something/stuff/a.c", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "whee", "whee/xy/whatever/foo.c", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "whee", "whee", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE,  "foobar/**", "foobar", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE,  "foobar/**", "foobar/whee", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "foobar/**", "foobar/whee/something", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "foobar/**", "foobar/whee/something/stuff/whatever", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "foobar", "foobar", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY }, // NOBASH: with bash, this is TRUE for dirs and FALSE for files
	{ SG_TRUE,  "foobar", "foobar/whee", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "foobar", "foobar/whee/something", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },
	{ SG_TRUE,  "foobar", "foobar/whee/something/stuff/whatever", SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY },

	// Note: There are no cases with explicitly repo-rooted paths (that start with "@/").
	//       We expect the matcher to treat the '@' like any other non-special character.
	//       There are also no cases that use the MATCH_REPO_ROOT flag.
	//       The __rooted version of the test uses variations on the above defined cases to cover many of those situations.
};

static void u0068__match_pattern__run_test_cases(SG_context* pCtx)
{
	SG_uint32 uIndex = 0u;

	for (uIndex = 0u; uIndex < SG_NrElements(u0068__match_pattern__cases); ++uIndex)
	{
		const u0068__match_pattern__case* pCase    = u0068__match_pattern__cases + uIndex;
		SG_bool                           bMatches = SG_FALSE;

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			bMatches = SG_FALSE;
		}

		VERIFY_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, pCase->szPattern, pCase->szFilename, pCase->uFlags, &bMatches)  );
		VERIFYP_COND("match_pattern__run_test_case: test case failed", bMatches == pCase->bMatches, ("index(%d) pattern(%s) filename(%s) flags(0x%04X) expected(%s)", uIndex, pCase->szPattern, pCase->szFilename, pCase->uFlags, pCase->bMatches == SG_TRUE ? "match" : "non-match"));
	}

fail:
	return;
}

static void u0068__match_pattern__run_test_cases__rooted(SG_context* pCtx)
{
	SG_uint32  uIndex   = 0u;
	SG_string* pPath    = NULL;
	SG_string* pPattern = NULL;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pPath)  );
	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pPattern)  );

	for (uIndex = 0u; uIndex < SG_NrElements(u0068__match_pattern__cases); ++uIndex)
	{
		const u0068__match_pattern__case* pCase    = u0068__match_pattern__cases + uIndex;
		SG_bool                           bMatches = SG_FALSE;


		// First test: prepend @/ to the test path and match it to the pattern using MATCH_REPO_ROOT

		// prepending @/ to NULL paths doesn't really make sense
		if (pCase->szFilename == NULL)
		{
			continue;
		}

		// if the filename already starts with @/, then this isn't a good case for this test
		if (pCase->szFilename[0] == '@' && (pCase->szFilename[1] == '/' || pCase->szFilename[1] == '\\'))
		{
			continue;
		}

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			bMatches = SG_FALSE;
		}

		// construct the path with @/ prepended
		VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, pPath, "@/%s", pCase->szFilename)  );

		// run the match using MATCH_REPO_ROOT
		VERIFY_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, pCase->szPattern, SG_string__sz(pPath), pCase->uFlags | SG_FILE_SPEC__MATCH_REPO_ROOT, &bMatches)  );
		VERIFYP_COND("match_pattern__run_test_case__rooted: explicit test case failed", bMatches == pCase->bMatches, ("index(%d) pattern(%s) filename(%s) flags(0x%04X) expected(%s)", uIndex, pCase->szPattern, SG_string__sz(pPath), pCase->uFlags | SG_FILE_SPEC__MATCH_REPO_ROOT, pCase->bMatches == SG_TRUE ? "match" : "non-match"));


		// Second test: prepend @/ to the test path AND pattern and match them against each other

		// prepending @/ to NULL patterns doesn't really make sense
		if (pCase->szPattern == NULL)
		{
			continue;
		}

		// if the pattern already starts with @/, then this isn't a good case for this test
		if (pCase->szPattern[0] == '@' && (pCase->szPattern[1] == '/' || pCase->szPattern[1] == '\\'))
		{
			continue;
		}

		// if this case uses the MATCH_ANYWHERE flag,
		// then prepending @/ to the pattern may not have the same expected result
		// so this won't be a good test
		if ((pCase->uFlags & SG_FILE_SPEC__MATCH_ANYWHERE) == SG_FILE_SPEC__MATCH_ANYWHERE)
		{
			continue;
		}

		// construct the pattern with @/ prepended
		VERIFY_ERR_CHECK(  SG_string__sprintf(pCtx, pPattern, "@/%s", pCase->szPattern)  );

		// run the match with both path and pattern having @/ prepended
		VERIFY_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, SG_string__sz(pPattern), SG_string__sz(pPath), pCase->uFlags, &bMatches)  );
		VERIFYP_COND("match_pattern__run_test_case__rooted: explicit test case failed", bMatches == pCase->bMatches, ("index(%d) pattern(%s) filename(%s) flags(0x%04X) expected(%s)", uIndex, SG_string__sz(pPattern), SG_string__sz(pPath), pCase->uFlags, pCase->bMatches == SG_TRUE ? "match" : "non-match"));
	}

fail:
	SG_STRING_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pPattern);
}


/*
**
** MAIN
**
*/

TEST_MAIN(u0068_main)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0068__alloc__basic(pCtx)  );
	BEGIN_TEST(  u0068__alloc__macro(pCtx)  );
	BEGIN_TEST(  u0068__alloc__badargs(pCtx)  );
	BEGIN_TEST(  u0068__alloc__copy__basic(pCtx)  );
	BEGIN_TEST(  u0068__alloc__copy__macro(pCtx)  );
	BEGIN_TEST(  u0068__alloc__copy__badargs(pCtx)  );
	BEGIN_TEST(  u0068__alloc__copy__blank(pCtx)  );
	BEGIN_TEST(  u0068__free__blank(pCtx)  );
	BEGIN_TEST(  u0068__reset__basic(pCtx)  );
	BEGIN_TEST(  u0068__reset__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__sz__basic(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__sz__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__sz__deep_copy(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__sz__shallow_copy(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__sz__shallow_copy_own_memory(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__buflen__basic(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__buflen__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__buflen__deep_copy(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__buflen__short_copy(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__string__basic(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__string__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__string__deep_copy(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__string__shallow_copy(pCtx)  );
	BEGIN_TEST(  u0068__add_pattern__string__shallow_copy_own_memory(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__array__basic(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__array__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__stringarray__basic(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__stringarray__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__file__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__file__badargs(pCtx)  );
	BEGIN_TEST(  u0068__add_patterns__file__badfiles(pCtx)  );
	BEGIN_TEST(  u0068__load_ignores__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0068__clear_patterns__per_type(pCtx)  );
	BEGIN_TEST(  u0068__clear_patterns__all_types(pCtx)  );
	BEGIN_TEST(  u0068__clear_patterns__cross_types(pCtx)  );
	BEGIN_TEST(  u0068__clear_patterns__badargs(pCtx)  );
	BEGIN_TEST(  u0068__has_pattern__fresh(pCtx)  );
	BEGIN_TEST(  u0068__has_pattern__blank(pCtx)  );
	BEGIN_TEST(  u0068__has_pattern__badargs(pCtx)  );
	BEGIN_TEST(  u0068__get_flags__fresh(pCtx)  );
	BEGIN_TEST(  u0068__get_flags__blank(pCtx)  );
	BEGIN_TEST(  u0068__get_flags__badargs(pCtx)  );
	BEGIN_TEST(  u0068__set_flags__basic(pCtx)  );
	BEGIN_TEST(  u0068__set_flags__old(pCtx)  );
	BEGIN_TEST(  u0068__set_flags__badargs(pCtx)  );
	BEGIN_TEST(  u0068__modify_flags__basic(pCtx)  );
	BEGIN_TEST(  u0068__modify_flags__multiple(pCtx)  );
	BEGIN_TEST(  u0068__modify_flags__old(pCtx)  );
	BEGIN_TEST(  u0068__modify_flags__new(pCtx)  );
	BEGIN_TEST(  u0068__modify_flags__old_new(pCtx)  );
	BEGIN_TEST(  u0068__modify_flags__multiple_old_new(pCtx)  );
	BEGIN_TEST(  u0068__match_path__check_paths(pCtx)  );
	BEGIN_TEST(  u0068__match_path__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0068__match_path__blank(pCtx)  );
	BEGIN_TEST(  u0068__match_pattern__run_test_cases(pCtx)  );
	BEGIN_TEST(  u0068__match_pattern__run_test_cases__rooted(pCtx)  );

	TEMPLATE_MAIN_END;
}
