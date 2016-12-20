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

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define TRACE_SPEC 0
#else
#define TRACE_SPEC 0
#endif

//////////////////////////////////////////////////////////////////

/*
**
** Types
**
*/

/**
 * A single pattern in the main file_spec structure.
 */
typedef struct
{
	const char* szPattern; //< The pattern string.
	SG_uint32   uFlags;    //< Flags associated with the pattern.
}
_SG_file_spec_pattern;

/**
 * Main file_spec structure containing various collections of patterns.
 */
struct _SG_file_spec
{
	SG_vector*      pIncludes;    //< Paths must match one of these patterns (if there are any) to match.
	SG_vector*      pExcludes;    //< Paths must NOT match any of these patterns to match.
	SG_vector*      pIgnores;     //< Paths must NOT match any of these patterns to match, unless overridden.
	SG_vector*      pReserved;    //< Paths must NOT match any of these patterns to match.
	SG_uint32       uFlags;       //< Flags associated with the whole filespec.
};


/*
**
** Globals
**
*/

/**
 * This global instance represents an empty filespec.
 * The pointer to it is available externally, and can be used by callers to specify an empty filespec.
 * It turns out that there are a lot of places where a caller wants to provide an empty filespec to a function.
 * This global pointer can be used as such without callers having to allocate and free their own empty instance.
 * Note that the data WITHIN this instance is never allocated, and this is therefore NOT a valid filespec instance.
 * Instead, all filespec functions that take a const filespec include a special case check for this pointer.
 */
static SG_file_spec goBlankFilespec;
const SG_file_spec* gpBlankFilespec = &goBlankFilespec;


/*
**
** Internal Functions
**
*/

/**
 * Simple helper to simplify boolean checks to see if a bitfield contains all of a given set of flags.
 * Returns SG_TRUE if all of the given flags are set in the bitfield, or SG_FALSE otherwise.
 */
static SG_bool _flags_has_all(
	SG_uint32 uBitfield, //< [in] Bitfield to check if flags are set in.
	SG_uint32 uFlags     //< [in] Flags to check for in the bitfield.
	)
{
	if ((uBitfield & uFlags) == uFlags)
	{
		return SG_TRUE;
	}
	else
	{
		return SG_FALSE;
	}
}

/**
 * Simple helper to simplify boolean checks to see if a bitfield contains none of a given set of flags.
 * Returns SG_TRUE if none of the given flags are set in the bitfield, or SG_FALSE otherwise.
 */
static SG_bool _flags_has_none(
	SG_uint32 uBitfield,
	SG_uint32 uFlags
	)
{
	if ((uBitfield & uFlags) == 0u)
	{
		return SG_TRUE;
	}
	else
	{
		return SG_FALSE;
	}
}

//////////////////////////////////////////////////////////////////

/**
 * Checks if a given pattern type is valid.
 * If it's not, throws SG_ERR_INVALIDARG.
 * If it is, does nothing.
 */
static void _validate_pattern_type(
	SG_context*                pCtx, //< [in] [out] Error and context info.
	SG_file_spec__pattern_type eType //< [in] The type of pattern to validate.
	)
{
	switch (eType)
	{
	case SG_FILE_SPEC__PATTERN__INCLUDE:
	case SG_FILE_SPEC__PATTERN__EXCLUDE:
	case SG_FILE_SPEC__PATTERN__IGNORE:
	case SG_FILE_SPEC__PATTERN__RESERVE:
		break;
	default:
		SG_ERR_THROW(SG_ERR_INVALIDARG);
	}

fail:
	return;
}

/**
 * Gets the pattern vector from a filespec corresponding to a given pattern type.
 * Accepts a mutable filespec and returns a mutable vector.
 */
static void _get_vector__mutable(
	SG_context*                pCtx,    //< [in] [out] Error and context info.
	SG_file_spec*              pThis,   //< [in] The filespec to retrieve a vector from.
	SG_file_spec__pattern_type eType,   //< [in] The type of the vector to retrieve.
	SG_vector**                ppVector //< [out] The retrieved vector.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(ppVector);

	switch (eType)
	{
	case SG_FILE_SPEC__PATTERN__INCLUDE: *ppVector = pThis->pIncludes; break;
	case SG_FILE_SPEC__PATTERN__EXCLUDE: *ppVector = pThis->pExcludes; break;
	case SG_FILE_SPEC__PATTERN__IGNORE:  *ppVector = pThis->pIgnores;  break;
	case SG_FILE_SPEC__PATTERN__RESERVE: *ppVector = pThis->pReserved; break;
	default: SG_ERR_THROW(SG_ERR_INVALIDARG);
	}

fail:
	return;
}

/**
 * Gets the pattern vector from a filespec corresponding to a given pattern type.
 * Accepts a const filespec and returns a const vector.
 */
static void _get_vector__const(
	SG_context*                pCtx,    //< [in] [out] Error and context info.
	const SG_file_spec*        pThis,   //< [in] The filespec to retrieve a vector from.
	SG_file_spec__pattern_type eType,   //< [in] The type of the vector to retrieve.
	const SG_vector**          ppVector //< [out] The retrieved vector.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(ppVector);
	SG_ARGCHECK(pThis != gpBlankFilespec, pThis); // should never be called on the blank filespec

	switch (eType)
	{
	case SG_FILE_SPEC__PATTERN__INCLUDE: *ppVector = pThis->pIncludes; break;
	case SG_FILE_SPEC__PATTERN__EXCLUDE: *ppVector = pThis->pExcludes; break;
	case SG_FILE_SPEC__PATTERN__IGNORE:  *ppVector = pThis->pIgnores;  break;
	case SG_FILE_SPEC__PATTERN__RESERVE: *ppVector = pThis->pReserved; break;
	default: SG_ERR_THROW(SG_ERR_INVALIDARG);
	}

fail:
	return;
}

/**
 * Frees an _SG_file_spec_pattern structure.
 * If the structure owns the pattern memory, that memory is freed as well.
 * Intended for use as an SG_free_callback.
 */
static void _free_pattern(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pData //< [in] The pattern structure to free.
	)
{
	_SG_file_spec_pattern* pPatternData = NULL;


	if (pData == NULL)
	{
		return;
	}

	pPatternData = (_SG_file_spec_pattern*)pData;

	if ((pPatternData->uFlags & SG_FILE_SPEC__OWN_MEMORY) == SG_FILE_SPEC__OWN_MEMORY)
	{
		SG_free(pCtx, pPatternData->szPattern);
	}

	SG_free(pCtx, pData);
}

/**
 * Creates a new copy of an _SG_file_spec_pattern structure.
 * If the existing pattern owns its memory, then the new pattern will create its own copy.
 * Otherwise, the new pattern will point to the same data as the existing one.
 * Intended for use as an SG_copy_callback.
 */
static void _copy_pattern(
	SG_context* pCtx,  //< [in] [out] Error and context info.
	void*       pData, //< [in] The pattern structure to copy.
	void**      pCopy  //< [out] A new copy of the given pattern structure.
	)
{
	_SG_file_spec_pattern* pInputPattern  = NULL;
	_SG_file_spec_pattern* pOutputPattern = NULL;

	SG_NULLARGCHECK(pData);
	SG_NULLARGCHECK(pCopy);

	pInputPattern = (_SG_file_spec_pattern*)pData;
	SG_ERR_CHECK(  SG_alloc1(pCtx, pOutputPattern)  );

	pOutputPattern->uFlags = pInputPattern->uFlags;
	if ((pInputPattern->uFlags & SG_FILE_SPEC__OWN_MEMORY) == SG_FILE_SPEC__OWN_MEMORY)
	{
		SG_uint32 uLength  = 0u;
		char*     szBuffer = NULL;

		uLength = SG_STRLEN(pInputPattern->szPattern) + 1;
		SG_ERR_CHECK(  SG_alloc(pCtx, uLength, sizeof(char), &szBuffer)  );
		pOutputPattern->szPattern = szBuffer;
		SG_ERR_CHECK(  SG_strcpy(pCtx, szBuffer, uLength, pInputPattern->szPattern)  );
	}
	else
	{
		pOutputPattern->szPattern = pInputPattern->szPattern;
	}

	*pCopy = pOutputPattern;
	return;

fail:
	_free_pattern(pCtx, pOutputPattern);
	return;
}

/**
 * Adds an _SG_file_spec_pattern to an SG_file_spec.
 */
static void _add_pattern(
	SG_context*                pCtx,        //< [in] [out] Error and context info.
	SG_file_spec*              pThis,       //< [in] The file_spec to add a pattern to.
	SG_file_spec__pattern_type eType,       //< [in] The type of pattern being added.
	_SG_file_spec_pattern*     pPatternData //< [in] The pattern to add.
	)
{
	SG_vector* pVector = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(pPatternData);

	SG_ERR_CHECK(  _get_vector__mutable(pCtx, pThis, eType, &pVector)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pVector, pPatternData, NULL)  );

fail:
	return;
}

/**
 * Matches a path against all of the patterns of a single type.
 */
void _match_type(
	SG_context*                pCtx,    //< [in] [out] Error and context info.
	const SG_file_spec*        pThis,   //< [in] The file spec to match against.
	SG_file_spec__pattern_type eType,   //< [in] The type of patterns to match the path against.
	const char*                szPath,  //< [in] The path to match against the patterns.
	SG_uint32                  uFlags,  //< [in] Flags to use to affect the matching.
	SG_bool*                   pMatches //< [out] Set to SG_TRUE if at least one pattern of the given type matched, or SG_FALSE if none of them did.
	)
{
	const SG_vector* pVector  = NULL;
	SG_uint32        uCount   = 0u;
	SG_uint32        uIndex   = 0u;
	SG_bool          bMatches = SG_FALSE;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(pMatches);

#if TRACE_SPEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_file_spec::_match_type: [type %x][path %s][flags %x]\n",
							   eType, szPath, uFlags)  );
#endif

	// the blank filespec is unallocated, but implemented as if it has no patterns and no flags
	if (pThis == gpBlankFilespec)
		goto done;

	SG_ERR_CHECK(  _get_vector__const(pCtx, pThis, eType, &pVector)  );
	SG_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uCount)  );

	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		void*                  pValue      = NULL;
		_SG_file_spec_pattern* pPattern    = NULL;
		SG_uint32              uTotalFlags = 0u;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pVector, uIndex, &pValue)  );
		pPattern = (_SG_file_spec_pattern*)pValue;
		uTotalFlags = pPattern->uFlags | uFlags;

		SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, pPattern->szPattern, szPath, uTotalFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			break;
		}
	}

done:
#if TRACE_SPEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_file_spec::_match_type: [type %x][path %s][flags %x] result %d\n",
							   eType, szPath, uFlags, bMatches)  );
#endif

	*pMatches = bMatches;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_file_spec__alloc(
	SG_context*    pCtx,
	SG_file_spec** pThis
	)
{
	SG_file_spec* pNew = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(*pThis == NULL, pThis);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pNew)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &(pNew->pIncludes), 5)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &(pNew->pExcludes), 5)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &(pNew->pIgnores), 10)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &(pNew->pReserved), 2)  );
	pNew->uFlags = 0u;

	*pThis = pNew;
	return;

fail:
	if (pNew != NULL)
	{
		SG_VECTOR_NULLFREE(pCtx, pNew->pIncludes);
		SG_VECTOR_NULLFREE(pCtx, pNew->pExcludes);
		SG_VECTOR_NULLFREE(pCtx, pNew->pIgnores);
		SG_VECTOR_NULLFREE(pCtx, pNew->pReserved);
		SG_NULLFREE(pCtx, pNew);
	}
}

void SG_file_spec__alloc__copy(
	SG_context*         pCtx,
	const SG_file_spec* pThat,
	SG_file_spec**      pThis
	)
{
	SG_file_spec* pNew = NULL;

	SG_NULLARGCHECK(pThat);
	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(*pThis == NULL, pThis);

	if (pThat == gpBlankFilespec)
	{
		SG_ERR_CHECK(  SG_file_spec__alloc(pCtx, pThis)  );
		return;
	}

	SG_ERR_CHECK(  SG_alloc1(pCtx, pNew)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC__COPY(pCtx, pThat->pIncludes, _copy_pattern, _free_pattern, &(pNew->pIncludes))  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC__COPY(pCtx, pThat->pExcludes, _copy_pattern, _free_pattern, &(pNew->pExcludes))  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC__COPY(pCtx, pThat->pIgnores,  _copy_pattern, _free_pattern, &(pNew->pIgnores))  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC__COPY(pCtx, pThat->pReserved, _copy_pattern, _free_pattern, &(pNew->pReserved))  );
	pNew->uFlags = pThat->uFlags;

	*pThis = pNew;
	return;

fail:
	if (pNew != NULL)
	{
		SG_VECTOR_NULLFREE(pCtx, pNew->pIncludes);
		SG_VECTOR_NULLFREE(pCtx, pNew->pExcludes);
		SG_VECTOR_NULLFREE(pCtx, pNew->pIgnores);
		SG_VECTOR_NULLFREE(pCtx, pNew->pReserved);
		SG_NULLFREE(pCtx, pNew);
	}
}

void SG_file_spec__free(
	SG_context*   pCtx,
	const SG_file_spec* pThis
	)
{

	if (pThis == NULL || pThis == gpBlankFilespec)
	{
		return;
	}

	SG_vector__free__with_assoc(pCtx, pThis->pIncludes, _free_pattern);
	SG_vector__free__with_assoc(pCtx, pThis->pExcludes, _free_pattern);
	SG_vector__free__with_assoc(pCtx, pThis->pIgnores,  _free_pattern);
	SG_vector__free__with_assoc(pCtx, pThis->pReserved, _free_pattern);
	SG_NULLFREE(pCtx, pThis);
}

void SG_file_spec__reset(
	SG_context*   pCtx,
	SG_file_spec* pThis
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pThis);

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1)
	{
		SG_vector* pVector = NULL;

		SG_ERR_CHECK(  _get_vector__mutable(pCtx, pThis, (SG_file_spec__pattern_type)uIndex, &pVector)  );
		SG_ERR_CHECK(  SG_vector__clear__with_assoc(pCtx, pVector, _free_pattern)  );
	}

	pThis->uFlags = 0u;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

#include "sg_file_spec__parse_file.h"
#include "sg_file_spec__accumulate_collapsed.h"

//////////////////////////////////////////////////////////////////

void SG_file_spec__add_pattern__sz(
	SG_context*                pCtx,
	SG_file_spec*              pThis,
	SG_file_spec__pattern_type eType,
	const char*                szPattern,
	SG_uint32                  uFlags
	)
{
	_SG_file_spec_pattern* pPatternData = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(szPattern);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPatternData)  );

	pPatternData->uFlags = uFlags;
	if (_flags_has_all(uFlags, SG_FILE_SPEC__SHALLOW_COPY))
	{
		pPatternData->szPattern = szPattern;
	}
	else
	{
		SG_uint32 uSize         = 0u;
		char*     szPatternCopy = NULL;

		pPatternData->uFlags |= SG_FILE_SPEC__OWN_MEMORY;
		uSize = SG_STRLEN(szPattern) + 1;
		SG_ERR_CHECK(  SG_alloc(pCtx, uSize, sizeof(char), &szPatternCopy)  );
		pPatternData->szPattern = szPatternCopy;
		SG_ERR_CHECK(  SG_strcpy(pCtx, szPatternCopy, uSize, szPattern)  );
	}

	SG_ERR_CHECK(  _add_pattern(pCtx, pThis, eType, pPatternData)  );

	return;

fail:
	_free_pattern(pCtx, pPatternData);
}

void SG_file_spec__add_patterns__varray(
	SG_context * pCtx,
	SG_file_spec * pThis,
	SG_file_spec__pattern_type eType,
	const SG_varray * pva,
	SG_uint32 uFlags
	)
{
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
	for (k=0; k<count; k++)
	{
		const char * psz_k;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, k, &psz_k)  );
		SG_ERR_CHECK(  SG_file_spec__add_pattern__sz(pCtx, pThis, eType, psz_k, uFlags)  );
	}

fail:
	return;
}

void SG_file_spec__add_pattern__buflen(
	SG_context*                pCtx,
	SG_file_spec*              pThis,
	SG_file_spec__pattern_type eType,
	const char*                szPattern,
	SG_uint32                  uLength,
	SG_uint32                  uFlags
	)
{
	_SG_file_spec_pattern* pPatternData  = NULL;
	char*                  szPatternCopy = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(szPattern);
	SG_ARGCHECK(uLength > 0u, uLength);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPatternData)  );

	// we can't use a shallow copy of the caller's memory
	// because we can't be sure that it's NULL-terminated
	if (_flags_has_all(uFlags, SG_FILE_SPEC__SHALLOW_COPY))
	{
		SG_ERR_THROW(SG_ERR_INVALIDARG);
	}

	pPatternData->uFlags = uFlags | SG_FILE_SPEC__OWN_MEMORY;
	SG_ERR_CHECK(  SG_alloc(pCtx, uLength+1, sizeof(char), &szPatternCopy)  );
	pPatternData->szPattern = szPatternCopy;
	SG_ERR_CHECK(  SG_strncpy(pCtx, szPatternCopy, uLength+1, szPattern, uLength)  );

	SG_ERR_CHECK(  _add_pattern(pCtx, pThis, eType, pPatternData)  );

	return;

fail:
	_free_pattern(pCtx, pPatternData);
}

void SG_file_spec__add_pattern__string(
	SG_context*                pCtx,
	SG_file_spec*              pThis,
	SG_file_spec__pattern_type eType,
	SG_string*                 pPattern,
	SG_uint32                  uFlags
	)
{
	_SG_file_spec_pattern* pPatternData = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(pPattern);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPatternData)  );

	pPatternData->uFlags = uFlags;
	if (_flags_has_all(uFlags, SG_FILE_SPEC__SHALLOW_COPY))
	{
		if (_flags_has_all(uFlags, SG_FILE_SPEC__OWN_MEMORY))
		{
			SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pPattern, (SG_byte**)(&(pPatternData->szPattern)), NULL)  );
		}
		else
		{
			pPatternData->szPattern = SG_string__sz(pPattern);
		}
	}
	else
	{
		SG_uint32 uSize         = 0u;
		char*     szPatternCopy = NULL;

		pPatternData->uFlags |= SG_FILE_SPEC__OWN_MEMORY;
		uSize = SG_string__length_in_bytes(pPattern) + 1;
		SG_ERR_CHECK(  SG_alloc(pCtx, uSize, sizeof(char), &szPatternCopy)  );
		pPatternData->szPattern = szPatternCopy;
		SG_ERR_CHECK(  SG_strcpy(pCtx, szPatternCopy, uSize, SG_string__sz(pPattern))  );
	}

	SG_ERR_CHECK(  _add_pattern(pCtx, pThis, eType, pPatternData)  );

	return;

fail:
	_free_pattern(pCtx, pPatternData);
}

void SG_file_spec__add_patterns__array(
	SG_context*                pCtx,
	SG_file_spec*              pThis,
	SG_file_spec__pattern_type eType,
	const char* const*         ppPatterns,
	SG_uint32                  uCount,
	SG_uint32                  uFlags
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_ARGCHECK(ppPatterns != NULL || uCount == 0u, ppPatterns);

	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_ARGCHECK(ppPatterns[uIndex] != NULL, ppPatterns);
	}

	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_file_spec__add_pattern__sz(pCtx, pThis, eType, ppPatterns[uIndex], uFlags);
	}

fail:
	return;
}

void SG_file_spec__add_patterns__stringarray(
	SG_context*                pCtx,
	SG_file_spec*              pThis,
	SG_file_spec__pattern_type eType,
	const SG_stringarray*      pPatterns,
	SG_uint32                  uFlags
	)
{
	const char* const* ppArray = NULL;
	SG_uint32          uCount  = 0u;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(pPatterns);

	SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, pPatterns, &ppArray, &uCount)  );
	SG_ERR_CHECK(  SG_file_spec__add_patterns__array(pCtx, pThis, eType, ppArray, uCount, uFlags)  );

fail:
	return;
}

/**
 * Add the contents of a simple file of patterns to
 * the requested type.
 *
 * We DO NOT handle "-remove" or "<includefile" patterns.
 * 
 * We DO NOT get anything from config/localsettings.
 *
 */
void SG_file_spec__add_patterns__file(
	SG_context*                pCtx,
	SG_file_spec*              pThis,
	SG_file_spec__pattern_type eType,
	const SG_pathname *        pPathname,
	SG_uint32 uFlags)
{
	SG_varray * pva = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ERR_CHECK(  _validate_pattern_type(pCtx, eType)  );
	SG_NULLARGCHECK(pPathname);

	SG_ERR_CHECK(  _sg_file_spec__parse_file(pCtx, pPathname, &pva)  );
	SG_ERR_CHECK(  SG_file_spec__add_patterns__varray(pCtx, pThis, eType, pva, uFlags)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}

void SG_file_spec__add_patterns__file__sz(
	SG_context * pCtx,
	SG_file_spec * pThis,
	SG_file_spec__pattern_type eType,
	const char * pszFilename,
	SG_uint32 uFlags)
{
	SG_pathname * pPath = NULL;

	SG_NONEMPTYCHECK_RETURN(pszFilename);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, pszFilename)  );
	SG_ERR_CHECK(  SG_file_spec__add_patterns__file(pCtx, pThis, eType, pPath, uFlags)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

#include "sg_file_spec__load_ignores.h"

//////////////////////////////////////////////////////////////////


void SG_file_spec__clear_patterns(
	SG_context*   pCtx,
	SG_file_spec* pThis,
	SG_uint32     uTypes
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pThis);

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1)
	{
		SG_vector* pVector = NULL;

		if ((uIndex & uTypes) == 0)
		{
			continue;
		}

		SG_ERR_CHECK(  _get_vector__mutable(pCtx, pThis, (SG_file_spec__pattern_type)uIndex, &pVector)  );
		SG_ERR_CHECK(  SG_vector__clear__with_assoc(pCtx, pVector, _free_pattern)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_file_spec__has_pattern(
	SG_context*         pCtx,
	const SG_file_spec* pThis,
	SG_uint32           uTypes,
	SG_bool*            pHasPattern
	)
{
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pHasPattern);

	// the blank filespec is unallocated, but implemented as if it has no patterns and no flags
	if (pThis == gpBlankFilespec)
	{
		*pHasPattern = SG_FALSE;
		return;
	}

	for (uIndex = 1u; uIndex != SG_FILE_SPEC__PATTERN__LAST; uIndex <<= 1)
	{
		const SG_vector* pVector = NULL;
		SG_uint32        uCount  = 0u;

		if ((uIndex & uTypes) == 0)
		{
			continue;
		}

		SG_ERR_CHECK(  _get_vector__const(pCtx, pThis, (SG_file_spec__pattern_type)uIndex, &pVector)  );
		SG_ERR_CHECK(  SG_vector__length(pCtx, pVector, &uCount)  );
		if (uCount > 0u)
		{
			*pHasPattern = SG_TRUE;
			return;
		}
	}

	*pHasPattern = SG_FALSE;

fail:
	return;
}

void SG_file_spec__get_flags(
	SG_context*         pCtx,
	const SG_file_spec* pThis,
	SG_uint32*          pFlags
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pFlags);

	// the blank filespec is unallocated, but implemented as if it has no patterns and no flags
	if (pThis == gpBlankFilespec)
	{
		*pFlags = 0u;
		return;
	}

	*pFlags = pThis->uFlags;

fail:
	return;
}

void SG_file_spec__set_flags(
	SG_context*   pCtx,
	SG_file_spec* pThis,
	SG_uint32     uFlags,
	SG_uint32*    pOldFlags
	)
{
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pThis);

	if (pOldFlags != NULL)
	{
		*pOldFlags = pThis->uFlags;
	}
	
	pThis->uFlags = uFlags;

fail:
	return;
}

void SG_file_spec__modify_flags(
	SG_context*   pCtx,
	SG_file_spec* pThis,
	SG_uint32     uAddFlags,
	SG_uint32     uRemoveFlags,
	SG_uint32*    pOldFlags,
	SG_uint32*    pNewFlags
	)
{
	SG_NULLARGCHECK(pThis);

	if (pOldFlags != NULL)
	{
		*pOldFlags = pThis->uFlags;
	}

	pThis->uFlags &= ~uRemoveFlags;
	pThis->uFlags |= uAddFlags;

	if (pNewFlags != NULL)
	{
		*pNewFlags = pThis->uFlags;
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_file_spec__match_path(
	SG_context*         pCtx,
	const SG_file_spec* pThis,
	const char*         szPath,
	SG_uint32           uFlags,
	SG_bool             *pMatches
	)
{
	SG_bool   bMatches    = SG_FALSE;
	SG_uint32 uCount      = 0u;
	SG_uint32 uTotalFlags = 0u;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(pMatches);

	// the blank filespec is unallocated, but implemented as if it has no patterns and no flags
	if (pThis == gpBlankFilespec)
	{
		*pMatches = SG_TRUE;
		return;
	}

	// compute the total set of effective flags
	uTotalFlags = pThis->uFlags | uFlags;

	// match includes
	if ((uTotalFlags & SG_FILE_SPEC__NO_INCLUDES) == 0)
	{
		SG_ERR_CHECK(  SG_vector__length(pCtx, pThis->pIncludes, &uCount)  );
		if (uCount > 0u)
		{
			SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__INCLUDE, szPath, uFlags, &bMatches)  );
			if (bMatches == SG_FALSE)
			{
				*pMatches = SG_FALSE;
				return;
			}
		}
	}

	// match excludes
	if ((uTotalFlags & SG_FILE_SPEC__NO_EXCLUDES) == 0)
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__EXCLUDE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			*pMatches = SG_FALSE;
			return;
		}
	}

	// match ignores
	if ((uTotalFlags & SG_FILE_SPEC__NO_IGNORES) == 0)
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__IGNORE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			*pMatches = SG_FALSE;
			return;
		}
	}

	// match reserved
	if ((uTotalFlags & SG_FILE_SPEC__NO_RESERVED) == 0)
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__RESERVE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
#if TRACE_SPEC
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Matched reserved pattern in %s\n", szPath)  );
#endif
			*pMatches = SG_FALSE;
			return;
		}
	}

	// if we haven't bailed out yet, then we have a match
	*pMatches = SG_TRUE;

fail:
	return;
}

void SG_file_spec__match_pattern(
	SG_context* pCtx,
	const char* szPattern,
	const char* szPath,
	SG_uint32   uFlags,
	SG_bool*    pMatches
	)
{
	const char* szPathStart     = NULL;
	const char* szPatternStart  = NULL;
	SG_string*  pNewPattern     = NULL;

#if TRACE_SPEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_file_spec__match_pattern: [path %s][pattern %s][flags %x]\n",
							   szPath, szPattern, uFlags)  );
#endif

	// if the filename or pattern is NULL
	// then whether or not it matches depends on whether or not they passed the corresponding flag
	if (szPath == NULL)
	{
		*pMatches = (uFlags & SG_FILE_SPEC__MATCH_NULL_FILENAME) == SG_FILE_SPEC__MATCH_NULL_FILENAME;
		goto cleanup;
	}
	if (szPattern == NULL)
	{
		*pMatches = (uFlags & SG_FILE_SPEC__MATCH_NULL_PATTERN) == SG_FILE_SPEC__MATCH_NULL_PATTERN;
		goto cleanup;
	}

	// check for pre-processing flags
	if ((uFlags & SG_FILE_SPEC__MATCH_REPO_ROOT) == SG_FILE_SPEC__MATCH_REPO_ROOT)
	{
		// the pattern only matches at the repo root
		// this effectively means the pattern has an implicit "@/" prefix

		// rather than call ourselves recursively
		// just try to match the implicit prefix on the path
		if (szPath[0] != '@' || (szPath[1] != '/' && szPath[1] != '\\'))
		{
			*pMatches = SG_FALSE;
			goto cleanup;
		}

		// advance past the "@/" in the path
		szPath = szPath + 2;

		// strip the flag so that recursive calls don't try to use it again
		uFlags = uFlags & ~SG_FILE_SPEC__MATCH_REPO_ROOT;
	}
	if ((uFlags & SG_FILE_SPEC__MATCH_ANYWHERE) == SG_FILE_SPEC__MATCH_ANYWHERE)
	{
		// the pattern is allowed to match anywhere in the path, not just the beginning
		// this effectively means the pattern has an implicit "**/" prefix
		// create a new pattern with that prefix and call ourselves with it
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pNewPattern)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pNewPattern, "**/")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pNewPattern, szPattern)  );
		SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, SG_string__sz(pNewPattern), szPath, uFlags & ~SG_FILE_SPEC__MATCH_ANYWHERE, pMatches)  );
		goto cleanup;
	}
	if ((uFlags & SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY) == SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY)
	{
		// if the pattern matches a folder, it also matches everything IN that folder
		// this effectively means the pattern has an implicit "/**/*" suffix
		// create a new pattern with that suffix and call ourselves with it
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pNewPattern)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pNewPattern, szPattern)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pNewPattern, "/**")  );
		SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, SG_string__sz(pNewPattern), szPath, uFlags & ~SG_FILE_SPEC__MATCH_FOLDERS_RECURSIVELY, pMatches)  );
		goto cleanup;
	}

	// check for special cases
	if (strcmp(szPattern, "**") == 0)
	{
		// the "**" pattern actually matches everything
		*pMatches = SG_TRUE;
		goto cleanup;
	}

	// copy off the start of the path and pattern
	// so that we can refer back to them as we're matching
	szPathStart    = szPath;
	szPatternStart = szPattern;

	// loop through both strings, matching characters
	// if we get to the end of one of them, we'll fall out of the loop
	// if we encounter non-matching characters, we'll just return false immediately
	while (*szPath != '\0' && *szPattern != '\0')
	{
		if (*szPattern == '?' && *szPath != '/' && *szPath != '\\')
		{
			// a '?' wildcard matches any single non-slash character in the filename

			// EXCEPTION: '?' won't match a leading dot, since *nix uses that as a hidden file marker
			if (*szPath == '.' && (szPath == szPathStart || *(szPath-1) == '/' || *(szPath-1) == '\\'))
			{
				*pMatches = SG_FALSE;
				goto cleanup;
			}

			// step over the matched character
			++szPath;
			++szPattern;
		}
		else if (
			   (szPattern == szPatternStart && *szPattern == '*' && *(szPattern+1) == '*' && (*(szPattern+2) == '/' || *(szPattern+2) == '\\'))
			|| ((*szPattern == '/' || *szPattern == '\\') && *(szPattern+1) == '*' && *(szPattern+2) == '*' && (*(szPattern+3) == '/' || *(szPattern+3) == '\\'))
			)
		{
			// we end up here in one of these cases:
			// 1) a "**/" wildcard at the beginning of the pattern
			// 2) a "/**/" wildcard in the middle of the pattern
			// either way we need to match zero or more whole directory names

			// check which case we caught so that we can do additional validation
			if (szPattern == szPatternStart)
			{
				// in case (1), the path must also be at its beginning
				if (szPath != szPathStart)
				{
					*pMatches = SG_FALSE;
					goto cleanup;
				}

				// advance past the wildcard in the pattern
				szPattern = szPattern + 3;
			}
			else
			{
				// in case (2) the path must also currently be at a slash character
				if (*szPath != '/' && *szPath != '\\')
				{
					*pMatches = SG_FALSE;
					goto cleanup;
				}

				// advance past the wildcard in the pattern
				szPattern = szPattern + 4;
			}

			// advance through the path, matching the wildcard
			while (*szPath != '\0')
			{
				// interesting bits to process are:
				// 1) a slash character
				// 2) the start of the path
				// 3) the end of the path
				// so advance until we find one of them
				while (*szPath != '\0' && *szPath != '/' && *szPath != '\\' && szPath != szPathStart)
				{
					++szPath;
				}

				// if we found an interesting character, check if we should stop matching
				if (*szPath != '\0')
				{
					SG_bool bMatchesHere = SG_FALSE;

					// advance the path past the interesting character, since the pattern is already past it
					// (if we're at the start of the path, there's no interesting character to advance past)
					if (szPath != szPathStart)
					{
						++szPath;
					}

					// check if the rest of the path matches the rest of the pattern
					SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, szPattern, szPath, uFlags, &bMatchesHere)  );
					if (bMatchesHere == SG_TRUE)
					{
						// we found a match using this slash as the wildcard delimiter
						// we're completely done
						*pMatches = SG_TRUE;
						goto cleanup;
					}

					// if we were checking the start of the path, but didn't match it
					// then move past it now so we don't check it again
					if (szPath == szPathStart)
					{
						++szPath;
					}

					// no match there, try being greedy and matching another directory
				}
			}
		}
		else if ((*szPattern == '/' || *szPattern == '\\') && *(szPattern+1) == '*' && *(szPattern+2) == '*' && *(szPattern+3) == '\0')
		{
			// a "/**" wildcard at the end of the pattern will match anything that's left
			// as long as the path is currently at a slash character or the end
			*pMatches = (*szPath == '/' || *szPath == '\\' || *szPath == '\0') ? SG_TRUE : SG_FALSE;
			goto cleanup;
		}
		else if (*szPattern == '*')
		{
			// a '*' wildcard matches zero or more non-slash characters in the path

			// EXCEPTION: '*' won't match a leading dot, since *nix uses that as a hidden file marker
			if (*szPath == '.' && (szPath == szPathStart || *(szPath-1) == '/' || *(szPath-1) == '\\'))
			{
				*pMatches = SG_FALSE;
				goto cleanup;
			}

			// advance the pattern past the wildcard
			// a bunch of adjacent '*' wildcards is equivalent to a single '*'
			// (assuming it's not the '**/' wildcard, which we already checked for)
			do
			{
				++szPattern;
			}
			while (*szPattern == '*');

			// advance through the path, matching the wildcard
			while (*szPath != '\0')
			{
				// interesting bits to process are:
				// 1) the next character in the pattern
				// 2) a slash character
				// 3) the end of the path
				// 4) every character, if the next character in the pattern is a '?' wildcard
				// so advance until we find one of them
				while (*szPath != '\0' && *szPath != *szPattern && *szPath != '/' && *szPath != '\\' && *szPattern != '?')
				{
					++szPath;
				}

				// check what we found
				if (*szPath == '/' || *szPath == '\\')
				{
					// we found a slash character
					// we're done matching this wildcard
					break;
				}
				else if (*szPath == *szPattern || *szPattern == '?')
				{
					// if we found the next character in the pattern, then we MIGHT be done
					// if the next character in the pattern is '?'
					// then ANY next character might match the pattern
					SG_bool bMatchesHere = SG_FALSE;

					// check if the rest of the path matches the rest of the pattern
					SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, szPattern, szPath, uFlags, &bMatchesHere)  );
					if (bMatchesHere == SG_TRUE)
					{
						// we found a match using this as the wildcard delimiter
						// we're completely done
						*pMatches = SG_TRUE;
						goto cleanup;
					}
					else
					{
						// no match there, try being greedy and matching more
						++szPath;
					}
				}
			}
		}
		else if (*szPath == *szPattern)
		{
			// the characters are literal matches
			++szPath;
			++szPattern;
		}
		else if ( (*szPath == '/' && *szPattern == '\\') || (*szPath == '\\' && *szPattern == '/') )
		{
			// the characters are both directory separators, and thus match
			++szPath;
			++szPattern;
		}
		else
		{
			// we were unable to successfully match the current characters in the filename and pattern
			*pMatches = SG_FALSE;
			goto cleanup;
		}
	}

	// we have a few different exit cases to check for
	if (*szPath == '\0' && *szPattern == '\0')
	{
		// if we got through both strings simultaneously, then we've got a match
		*pMatches = SG_TRUE;
	}
	else if (*szPattern != '\0')
	{
		// if we're not at the end of the pattern yet
		// check to see if the remainder of the pattern is wildcards that can match zero characters
		// consume as many wildcards matching zero characters as possible
		// then see if we made it to the end of the pattern
		while (
			   (*szPattern == '*')
			|| ((*szPattern == '/' || *szPattern == '\\') && *(szPattern+1) == '*' && *(szPattern+2) == '*')
			)
		{
			++szPattern;
		}
		*pMatches = (*szPattern == '\0') ? SG_TRUE : SG_FALSE;
	}
	else if (*szPath != '\0')
	{
		// if MATCH_TRAILING_SLASH is among the flags
		// then we can match a slash even if it's not in the pattern
		if ((uFlags & SG_FILE_SPEC__MATCH_TRAILING_SLASH) == SG_FILE_SPEC__MATCH_TRAILING_SLASH)
		{
			if (*szPath == '/' || *szPath == '\\')
			{
				++szPath;
			}
		}
		*pMatches = (*szPath == '\0') ? SG_TRUE : SG_FALSE;
	}
	else
	{
		// we got to the end of one string without getting to the end of the other
		// so there are unmatched characters in one of them, no match
		*pMatches = SG_FALSE;
	}

fail:
cleanup:

#if TRACE_SPEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_file_spec__match_pattern: [path %s][pattern %s][flags %x] result %d\n",
							   szPath, szPattern, uFlags, *pMatches)  );
#endif

	SG_STRING_NULLFREE(pCtx, pNewPattern);
}

void SG_file_spec__should_include(
	SG_context*                 pCtx,
	const SG_file_spec*         pThis,
	const char*                 szPath,
	SG_uint32                   uFlags,
	SG_file_spec__match_result* pResult
	)
{
	SG_bool      bMatches      = SG_FALSE;
	SG_uint32    uCount        = 0u;
	SG_uint32    uTotalFlags   = 0u;
	SG_pathname* pAbsolutePath = NULL;
	SG_pathname* pCwd          = NULL;
	SG_string*   pRelativePath = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(pResult);

	// the blank filespec is unallocated, but implemented as if it has no patterns and no flags
	if (pThis == gpBlankFilespec)
	{
		*pResult = SG_FILE_SPEC__RESULT__IMPLIED;
		goto cleanup;
	}

	// compute the total set of effective flags
	uTotalFlags = pThis->uFlags | uFlags;

	// if we need to convert a repo path, do that
/*
	//Commented out:  See: J9736
	if (_flags_has_all(uTotalFlags, SG_FILE_SPEC__CONVERT_REPO_PATH))
	{
		SG_ARGCHECK( ((szPath[0]=='@') && (szPath[1]=='/')), szPath );
		szPath = &szPath[2];
	}
	*/

	// match excludes
	if (_flags_has_none(uTotalFlags, SG_FILE_SPEC__NO_EXCLUDES))
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__EXCLUDE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			*pResult = SG_FILE_SPEC__RESULT__EXCLUDED;
			goto cleanup;
		}
	}

	// match includes
	if (_flags_has_none(uTotalFlags, SG_FILE_SPEC__NO_INCLUDES))
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__INCLUDE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			*pResult = SG_FILE_SPEC__RESULT__INCLUDED;
			goto cleanup;
		}
	}

	// match ignores
	if (_flags_has_none(uTotalFlags, SG_FILE_SPEC__NO_IGNORES))
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__IGNORE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			*pResult = SG_FILE_SPEC__RESULT__IGNORED;
			goto cleanup;
		}
	}

	// match reserved
	if (_flags_has_none(uTotalFlags, SG_FILE_SPEC__NO_RESERVED))
	{
		SG_ERR_CHECK(  _match_type(pCtx, pThis, SG_FILE_SPEC__PATTERN__RESERVE, szPath, uFlags, &bMatches)  );
		if (bMatches == SG_TRUE)
		{
			*pResult = SG_FILE_SPEC__RESULT__RESERVED;
			goto cleanup;
		}
	}

	// didn't match any of our patterns..
	SG_ERR_CHECK(  SG_vector__length(pCtx, pThis->pIncludes, &uCount)  );
	if (uCount == 0u || _flags_has_all(uTotalFlags, SG_FILE_SPEC__NO_INCLUDES))
	{
		// if we have no includes, then we'll implicitly match anything
		*pResult = SG_FILE_SPEC__RESULT__IMPLIED;
	}
	else
	{
		// if we have includes that we didn't match, then we can't be sure
		// if the path is a directory, the caller will have to dive in and match its contents
		// if the path is a file, then caller should consider this a non-match
		*pResult = SG_FILE_SPEC__RESULT__MAYBE;
	}

cleanup:
fail:
	SG_PATHNAME_NULLFREE(pCtx, pAbsolutePath);
	SG_PATHNAME_NULLFREE(pCtx, pCwd);
	SG_STRING_NULLFREE(pCtx, pRelativePath);
	return;
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
static void _dump_vector(SG_context * pCtx,
						 const char * pszName,
						 const SG_vector * pVector)
{
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\t%s:\n", pszName)  );

	if (pVector)
	{
		SG_uint32 len, k;
		SG_ERR_CHECK(  SG_vector__length(pCtx, pVector, &len)  );
		for (k=0; k<len; k++)
		{
			_SG_file_spec_pattern * p;
			SG_ERR_CHECK(  SG_vector__get(pCtx, pVector, k, (void **)&p)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "\t\t[%2d/%2d] : [flags 0x%08x] %s\n",
									   k, len, p->uFlags, p->szPattern)  );
		}
	}

fail:
	return;
}

void SG_file_spec_debug__dump_to_console(SG_context * pCtx,
										 const SG_file_spec * pFilespec,
										 const char * pszLabel)
{
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_file_spec_debug: %s\n",
							   ((pszLabel) ? pszLabel : ""))  );
	if (pFilespec)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "\t\t[uFlags 0x%08x]\n",
								   pFilespec->uFlags)  );
		SG_ERR_IGNORE(  _dump_vector(pCtx, "Includes", pFilespec->pIncludes)  );
		SG_ERR_IGNORE(  _dump_vector(pCtx, "Excludes", pFilespec->pExcludes)  );
		SG_ERR_IGNORE(  _dump_vector(pCtx, "Ignores",  pFilespec->pIgnores)  );
		SG_ERR_IGNORE(  _dump_vector(pCtx, "Reserved", pFilespec->pReserved)  );
	}
	
}
#endif

//////////////////////////////////////////////////////////////////

void SG_file_spec__count_patterns(SG_context * pCtx,
								  const SG_file_spec * pFilespec,
								  SG_file_spec__pattern_type eType,
								  SG_uint32 * pCount)
{
	const SG_vector * pvec = NULL;

	SG_ERR_CHECK(  _get_vector__const(pCtx, pFilespec, eType, &pvec)  );
	if (pvec)
		SG_ERR_CHECK(  SG_vector__length(pCtx, pvec, pCount)  );
	else
		*pCount = 0;

fail:
	return;
}

void SG_file_spec__get_nth_pattern(SG_context * pCtx,
								   const SG_file_spec * pFilespec,
								   SG_file_spec__pattern_type eType,
								   SG_uint32 uIndex,
								   const char ** ppszPattern,
								   SG_uint32 * puFlags)
{
	const SG_vector * pvec = NULL;
	_SG_file_spec_pattern * pPattern = NULL;

	SG_ERR_CHECK(  _get_vector__const(pCtx, pFilespec, eType, &pvec)  );
	SG_ERR_CHECK(  SG_vector__get(pCtx, pvec, uIndex, (void **)&pPattern)  );

	if (ppszPattern)
		*ppszPattern = pPattern->szPattern;
	if (puFlags)
		*puFlags = pPattern->uFlags;

fail:
	return;
}
