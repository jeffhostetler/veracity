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
#define TRACE_DIFFMERGE_LOOKUP	0
#else
#define TRACE_DIFFMERGE_LOOKUP	0
#endif

//////////////////////////////////////////////////////////////////

/*
**
** Types
**
*/

/**
 * Specification of a single file tool.
 */
struct _SG_filetool
{
	SG_string*            pName;       //< The unique name of the tool.
	SG_filetool__invoker* fInvoker;    //< A function that runs the tool's logic.
	SG_string*            pExecutable; //< The tool's executable, or NULL if it's internal.
	                                   //< Might be an absolute path, relative path, or just filename (passed straight to SG_exec).
	SG_varray*            pArguments;  //< List of command line arguments to pass to the tool, or NULL if it's built-in.
	                                   //< These arguments contain tokens to be replaced with actual values when the tool is invoked.
	SG_string*            pStdIn;      //< Token that will resolve to the filename to bind to the tool's STDIN stream.
	                                   //< NULL if the tool's internal, or doesn't use STDIN.
	SG_string*            pStdOut;     //< Token that will resolve to the filename to bind to the tool's STDOUT stream.
	                                   //< NULL if the tool's internal, or doesn't use STDOUT.
	SG_string*            pStdErr;     //< Token that will resolve to the filename to bind to the tool's STDERR stream.
	                                   //< NULL if the tool's internal, or doesn't use STDERR.
	SG_stringarray*       pFlags;      //< List of flags associated with the tool.
	                                   //< NULL if the tool has no associated flags.
	SG_vhash*             pResultMap;  //< Configured mapping of exit codes to result codes.
	                                   //< See _translate_exit_code for more info.
	                                   //< Never NULL, always contains at least standard translations.
};


/*
**
** Globals
**
*/

/**
 * List of built-in tools.
 */
static const struct
{
	const char*           szName;   //< Name of the built-in tool.
	SG_filetool__invoker* fInvoker; //< Function that implements the tool.
}
gpBuiltinFiletools[] =
{
	{ SG_FILETOOL__INTERNAL__NULL, SG_filetool__invoke__null },
};

/**
 * "Magic" numbers used as the first bytes in well-known file formats.
 */
static const SG_byte aMagic_UTF32BE[] = { 0x00, 0x00, 0xFE, 0xFF }; // Unicode UTF-32 Big-Endian
static const SG_byte aMagic_UTF32LE[] = { 0xFF, 0xFE, 0x00, 0x00 }; // Unicode UTF-32 Little-Endian
static const SG_byte aMagic_UTF16BE[] = { 0xFE, 0xFF };             // Unicode UTF-16 Big Endian
static const SG_byte aMagic_UTF16LE[] = { 0xFF, 0xFE };             // Unicode UTF-16 Little Endian
static const SG_byte aMagic_UTF8[]    = { 0xEF, 0xBB, 0xBF };       // Unicode UTF-8

/**
 * List of known magic numbers.
 */
static const struct
{
	const SG_byte* pBytes;  //< The bytes that make up the actual magic number.
	SG_uint32      uSize;   //< The number of bytes pointed to by pBytes.
	const char*    szClass; //< The default built-in file class of files with this magic number.
}
gpFileClassMagicNumbers[] =
{
	{ aMagic_UTF32BE, SG_NrElements(aMagic_UTF32BE), SG_FILETOOL__CLASS__TEXT },
	{ aMagic_UTF32LE, SG_NrElements(aMagic_UTF32LE), SG_FILETOOL__CLASS__TEXT },
	{ aMagic_UTF16BE, SG_NrElements(aMagic_UTF16BE), SG_FILETOOL__CLASS__TEXT },
	{ aMagic_UTF16LE, SG_NrElements(aMagic_UTF16LE), SG_FILETOOL__CLASS__TEXT },
	{ aMagic_UTF8,    SG_NrElements(aMagic_UTF8),    SG_FILETOOL__CLASS__TEXT },
	// TODO: others?
};


/*
**
** Helpers
**
*/

/**
 * Allocates a stringarray that contains a copy
 * of all of the string members of a varray.
 */
static void _stringarray__alloc__varray(
	SG_context*      pCtx,          //< [in] [out] Error and context info.
	SG_stringarray** ppStringArray, //< [out] The new stringarray.
	const SG_varray* pArray         //< [in] The varray to copy all the strings from.
	)
{
	SG_stringarray* pStringArray = NULL;
	SG_uint32       uCount       = 0u;
	SG_uint32       uIndex       = 0u;

	SG_NULLARGCHECK(ppStringArray);
	SG_NULLARGCHECK(pArray);

	SG_ERR_CHECK(  SG_varray__count(pCtx, pArray, &uCount)  );
	SG_STRINGARRAY__ALLOC(pCtx, &pStringArray, uCount);

	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const SG_variant* pCurrent = NULL;

		SG_ERR_CHECK(  SG_varray__get__variant(pCtx, pArray, uIndex, &pCurrent)  );
		if (pCurrent->type == SG_VARIANT_TYPE_SZ)
		{
			SG_ERR_CHECK(  SG_stringarray__add(pCtx, pStringArray, pCurrent->v.val_sz)  );
		}
	}

	*ppStringArray = pStringArray;
	pStringArray = NULL;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArray);
	return;
}


/*
**
** Internal Functions
**
*/

/**
 * Allocates a filetool using the vhash data loaded from localsettings.
 */
static void _alloc_filetool(
	SG_context*           pCtx,       //< [in] [out] Error and context info.
	const char*           szName,     //< [in] Name of the filetool to allocate.
	SG_filetool__invoker* fInvoker,   //< [in] Function to use to invoke the tool.
	const SG_vhash*       pResultMap, //< [in] Additional/override exit code translations.
	                                  //<      NULL if none are necessary.
	                                  //<      See _translate_exit_codes for more info.
	const SG_vhash*       pData,      //< [in] Localsettings data to initialize the filetool with.
	                                  //<      If NULL, then a built-in tool is assumed, and Filename/Arguments are left blank.
	SG_filetool**         ppTool      //< [out] The allocated and initialized tool.
	)
{
	SG_filetool* pTool = NULL;
	SG_pathname* pPath = NULL;

	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(fInvoker);
	SG_NULLARGCHECK(ppTool);

	SG_alloc1(pCtx, pTool);
	pTool->pName       = NULL;
	pTool->fInvoker    = fInvoker;
	pTool->pExecutable = NULL;
	pTool->pArguments  = NULL;
	pTool->pStdIn      = NULL;
	pTool->pStdOut     = NULL;
	pTool->pStdErr     = NULL;
	pTool->pFlags      = NULL;
	pTool->pResultMap  = NULL;

	// copy the given name
	SG_STRING__ALLOC__SZ(pCtx, &(pTool->pName), szName);

	// create a default exit code translation map
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pTool->pResultMap)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTool->pResultMap, SG_FILETOOL__EXIT_CODE__ZERO, SG_FILETOOL__RESULT__SUCCESS__SZ)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTool->pResultMap, SG_FILETOOL__EXIT_CODE__DEFAULT, SG_FILETOOL__RESULT__FAILURE__SZ)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTool->pResultMap, SG_FILETOOL__RESULT__SUCCESS__SZ, SG_FILETOOL__RESULT__SUCCESS)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pTool->pResultMap, SG_FILETOOL__RESULT__FAILURE__SZ, SG_FILETOOL__RESULT__FAILURE)  );

	// if the caller provided additional translations, add those
	if (pResultMap != NULL)
	{
		SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pResultMap, pTool->pResultMap)  );
	}

	// if the tool has config data, load it
	if (pData != NULL)
	{
		const char*       szString  = NULL;
		SG_varray*        pArray    = NULL;
		SG_variant*       pVariant  = NULL;
		const SG_variant* pcVariant = NULL;
		SG_vhash*         pHash     = NULL;

		// read the path to the tool
		SG_ERR_CHECK(  SG_vhash__get__variant(pCtx, pData, SG_FILETOOLCONFIG__TOOLPATH, &pcVariant)  );
		if (pcVariant->type == SG_VARIANT_TYPE_VARRAY)
		{
			SG_uint32 uIndex = 0u;
			SG_uint32 uCount = 0u;

			// run through each string
			SG_ERR_CHECK(  SG_variant__get__varray(pCtx, pcVariant, &pArray)  );
			SG_ERR_CHECK(  SG_varray__count(pCtx, pArray, &uCount)  );
			for (uIndex = 0u; uIndex < uCount; ++uIndex)
			{
				SG_bool bExists = SG_FALSE;

				// check if this string is a path to an existing file
				SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pArray, uIndex, &szString)  );
				SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, szString)  );
				SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
				SG_PATHNAME_NULLFREE(pCtx, pPath);
				if (bExists == SG_TRUE)
				{
					// this string is an existing file, use it
					break;
				}
			}

			// If we fell out of the loop without finding an existing file
			// then just let it use the last szString we tried (the last one in the list).
			// It might be something that wouldn't appear to exist but might
			// work anyway, like an executable name that's in the user's PATH.
			// If it's not, then they'll get an appropriate "not found" error later.
		}
		else
		{
			// use the only string given
			SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pcVariant, &szString)  );
		}
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &(pTool->pExecutable), szString)  );

		// read the array of arguments
		{
			SG_bool bHasArgs = SG_FALSE;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pData, SG_FILETOOLCONFIG__TOOLARGS, &bHasArgs)  );
			if (bHasArgs)
			{
				SG_uint16 t = SG_VARIANT_TYPE_NULL;
				SG_ERR_CHECK(  SG_vhash__typeof(pCtx, pData, SG_FILETOOLCONFIG__TOOLARGS, &t)  );
				if (t == SG_VARIANT_TYPE_VARRAY)
				{
					SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pData, SG_FILETOOLCONFIG__TOOLARGS, &pArray)  );
					SG_ERR_CHECK(  SG_VARRAY__ALLOC__COPY(pCtx, &(pTool->pArguments), pArray)  );
				}
				else
				{
					SG_ERR_THROW2(  SG_ERR_INVALIDARG,
									(pCtx, ("The '%s' setting for the tool '%s' must be an array"
											" rather than a string; use 'vv config add-to'"
											" rather than 'vv config set' to set this value."),
									 SG_FILETOOLCONFIG__TOOLARGS, szName)  );
				}
			}
			else
			{
				SG_ERR_THROW2(  SG_ERR_INVALIDARG,
								(pCtx, ("The '%s' setting for the tool '%s' has not been set."),
										SG_FILETOOLCONFIG__TOOLARGS, szName)  );
			}
		}

		// read the STDIN string
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pData, SG_FILETOOLCONFIG__TOOLSTDIN, &szString)  );
		if (szString != NULL)
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &(pTool->pStdIn), szString)  );
		}

		// read the STDOUT string
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pData, SG_FILETOOLCONFIG__TOOLSTDOUT, &szString)  );
		if (szString != NULL)
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &(pTool->pStdOut), szString)  );
		}

		// read the STDERR string
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pData, SG_FILETOOLCONFIG__TOOLSTDERR, &szString)  );
		if (szString != NULL)
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &(pTool->pStdErr), szString)  );
		}

		// read the array of flags
		SG_ERR_CHECK(  SG_vhash__check__variant(pCtx, pData, SG_FILETOOLCONFIG__TOOLFLAGS, &pVariant)  );
		if (pVariant != NULL)
		{
			if (pVariant->type == SG_VARIANT_TYPE_VARRAY)
			{
				SG_ERR_CHECK(  _stringarray__alloc__varray(pCtx, &pTool->pFlags, pVariant->v.val_varray)  );
			}
			else if (pVariant->type == SG_VARIANT_TYPE_SZ)
			{
				SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pTool->pFlags, 1u)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, pTool->pFlags, pVariant->v.val_sz)  );
			}
		}

		// read any configured exit code mappings
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pData, SG_FILETOOLCONFIG__TOOLEXITCODES, &pHash)  );
		if (pHash != NULL)
		{
			SG_uint32 uCount = 0u;
			SG_uint32 uIndex = 0u;

			// run through each specified exit code mapping
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pHash, &uCount)  );
			for (uIndex = 0u; uIndex < uCount; ++uIndex)
			{
				const char* szKey     = NULL;
				const char* szValue   = NULL;
				SG_int32    iExitCode = 0;

				// get the current mapping
				SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pHash, uIndex, &szKey, &szValue)  );

				// make sure the key parses into an exit code
				// it's supposed to be a string representation of an integer exit code
				SG_ERR_TRY(  SG_int32__parse__strict(pCtx, &iExitCode, szKey)  );
				SG_ERR_CATCH_CONTINUE(SG_ERR_INT_PARSE);
				SG_ERR_CATCH_END;

				// add this mapping to the tool's exit code map
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pTool->pResultMap, szKey, szValue)  );
			}
		}
	}

	*ppTool = pTool;
	pTool = NULL;

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;
}

/**
 * A default plugin for built-in tools.
 */
static void _plugin__default(
	SG_context*            pCtx,
	const char*            szType,
	const char*            szContext,
	const char*            szName,
	SG_filetool__invoker** ppInvoker
	)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(szType);
	SG_UNUSED(szContext);

	// check what the caller wants
	if (szName == NULL)
	{
		// return our external tool invoker
		*ppInvoker = SG_filetool__invoke__external;
	}
	else
	{
		SG_uint32 uIndex = 0u;

		// run through the list of built-in files looking for the named internal tool
		for (uIndex = 0u; uIndex < SG_NrElements(gpBuiltinFiletools); ++uIndex)
		{
			// check if this is the named tool
			if (strcmp(gpBuiltinFiletools[uIndex].szName, szName) == 0)
			{
				// this is it, return the invoker
				*ppInvoker = gpBuiltinFiletools[uIndex].fInvoker;
				break;
			}
		}
	}
}

/**
 * Replaces all occurrances of a token with its boolean value.
 */
static void _replace_token__bool(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_string*  sString, //< [in] The string to make replacements in.
	const char* szName,  //< [in] The name of the token to replace.
	SG_bool     bValue   //< [in] The value of the token.
	)
{
	SG_string* sToken   = NULL;
	SG_uint32  uCurrent = 0u;

	SG_NULLARGCHECK(sString);
	SG_NULLARGCHECK(szName);

	// build the token string we'll need to search for
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sToken, "@%s|", szName)  );

	// find the first occurrance of the token
	SG_ERR_CHECK(  SG_string__find__string(pCtx, sString, uCurrent, SG_FALSE, sToken, &uCurrent)  );

	// keep going as long as we find a token
	while (uCurrent != SG_UINT32_MAX)
	{
		// the token we're looking at is of the form: @TOKEN|true|false@
		// the following cases are all valid:
		// @TOKEN||false@    (empty true value)
		// @TOKEN|true|@     (empty false value)
		// @TOKEN|true@      (simplified empty false value)
		// our goal is to remove everything except the true or false value, depending on bValue
		//        uCurrent     // the first '@'
		SG_uint32 uTrue  = 0u; // the '|' between the token and true value
		SG_uint32 uFalse = 0u; // the '|' between the true and false values OR the last '@'
		SG_uint32 uEnd   = 0u; // the last '@'
		char      scChar = 0;  // the char that uFalse specifies, either '|' or '@'
		SG_uint32 uValue = 0u; // the length of the value NOT removed, added to uCurrent at the end

		// the true separator is the last character in sToken
		uTrue = uCurrent + SG_string__length_in_bytes(sToken) - 1u;

		// look for the false string
		SG_ERR_CHECK(  SG_string__find_any__char(pCtx, sString, uTrue + 1u, SG_FALSE, "|@", &uFalse, &scChar)  );
		if (uFalse == SG_UINT32_MAX)
		{
			// this isn't a real match because there's no closing '@'
			// additionally, there can't be any more matches
			// because they would all start with a '@', which there aren't any more of
			break;
		}
		else if (scChar == '|')
		{
			// found the false separator
			// the end is at the next @ delimiter
			SG_ERR_CHECK(  SG_string__find__char(pCtx, sString, uFalse + 1u, SG_FALSE, '@', &uEnd)  );
			if (uEnd == SG_UINT32_MAX)
			{
				// this isn't a real match because there's no closing '@'
				// additionally, there can't be any more matches
				// because they would all start with a '@', which there aren't any more of
				break;
			}
		}
		else
		{
			SG_ASSERT(scChar == '@');

			// no false separator, it and end will both point to the found '@'
			uEnd = uFalse;
		}

		SG_ASSERT(uEnd >= uFalse);
		SG_ASSERT(uFalse > uTrue);
		SG_ASSERT(uTrue > uCurrent);

		// remove the appropriate parts of the token, based on its value
		// Note: When we remove something from the string, any indices AFTER
		//       the start of what we removed become invalid.  For that reason we're
		//       removing LATER things FIRST.  Removal of later things won't
		//       invalidate the earlier indices, but removal of earlier things WILL
		//       invalidate the later indices.
		if (bValue == SG_FALSE)
		{
			// remove everything after the false value ends
			// If uFalse and uEnd both point to the end, then there's only one
			// delimiter to remove.  Otherwise uFalse and uEnd would indicate two
			// different delimiters that both need to be removed.  This first remove
			// takes care of the "end" one, but if uFalse and uEnd are equivalent,
			// then it's easiest to not remove it here, but let the next call take
			// care of it.
			if (uEnd != uFalse)
			{
				SG_ERR_CHECK(  SG_string__remove(pCtx, sString, uEnd, 1u)  );
			}

			// remove everything before the false value starts
			SG_ERR_CHECK(  SG_string__remove(pCtx, sString, uCurrent, uFalse - uCurrent + 1u)  );

			// figure out how much we left there
			if (uEnd == uFalse)
			{
				uValue = 0u;
			}
			else
			{
				uValue = uEnd - uFalse - 1u;
			}
		}
		else
		{
			// remove everything after the true value ends
			SG_ERR_CHECK(  SG_string__remove(pCtx, sString, uFalse, uEnd - uFalse + 1u)  );

			// remove everything before the true value starts
			SG_ERR_CHECK(  SG_string__remove(pCtx, sString, uCurrent, uTrue - uCurrent + 1u)  );

			// figure out how much we left there
			uValue = uFalse - uTrue - 1u;
		}

		// advance past the remaining value and find the next occurrance of the token
		uCurrent += uValue;
		SG_ERR_CHECK(  SG_string__find__string(pCtx, sString, uCurrent, SG_FALSE, sToken, &uCurrent)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, sToken);
	return;
}

/**
 * Replaces all occurrances of a token with its string value.
 *
 * HACK: This function also currently replaces "\-" with "-".
 *       See the longer comments within the function for more info.
 */
static void _replace_token__sz(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_string*  sString, //< [in] The string to make replacements in.
	const char* szName,  //< [in] The name of the token to replace.
	const char* szValue  //< [in] The value to replace the token with.
	)
{
	SG_string* sToken  = NULL;
	SG_byte*   szToken = NULL;
	SG_uint32  uLength = 0u;

	SG_NULLARGCHECK(sString);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(szValue);

	// add leading/trailing @ to form the actual token to replace
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sToken, "@%s@", szName)  );
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &sToken, &szToken, &uLength)  );

	// do the replacement
	SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, sString, szToken, uLength, (SG_byte*)szValue, SG_STRLEN(szValue), SG_UINT32_MAX, SG_TRUE, NULL)  );

	// HACK: also replace "\-" with "-"
	// this is currently necessary because without being able to escape a '-'
	// it isn't possible to set arguments from the command line using 'vv config'
	//
	// for example:
	//   vv config add-to filetools/type/tool/args --something=@VALUE@
	//
	// When vv parses that command line, it thinks that "--something"
	// is a flag for vv, rather than a plain value.
	// So to make it work, we escape it like so:
	//   vv config add-to filetools/type/tool/args \--something=@VALUE@
	// So what we're doing here is removing that escape character.
	//
	// A better fix in the future might be to update the command line parser
	// so that flags in quotes are treated as plain values.
	// Then you could do this instead:
	//   vv config add-to filetools/type/tool/args "--something=@VALUE@"
	// And we wouldn't have a hacky escape character to remove here.
	SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, sString, (SG_byte*)"\\-", 2u, (SG_byte*)"-", 1u, SG_UINT32_MAX, SG_TRUE, NULL)  );

fail:
	SG_STRING_NULLFREE(pCtx, sToken);
	SG_NULLFREE(pCtx, szToken);
	return;
}

/**
 * Replaces all occurrances of a token with its boolean value.
 */
static void _replace_token__int64(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_string*  sString, //< [in] The string to make replacements in.
	const char* szName,  //< [in] The name of the token to replace.
	SG_int64    iValue   //< [in] The value of the token.
	)
{
	SG_int_to_string_buffer szValue;

	SG_NULLARGCHECK(sString);
	SG_NULLARGCHECK(szName);

	// convert the integer to a string and then treat it as a string value
	SG_ERR_CHECK(  SG_int64_to_sz(iValue, szValue)  );
	SG_ERR_CHECK(  _replace_token__sz(pCtx, sString, szName, szValue)  );

fail:
	return;
}

/**
 * Replaces tokens in a tool's argument list with provided values.
 * Generates an SG_exec_argvec from the resulting arguments.
 *
 * The replacement values are provided in the form of an SG_vhash.
 * The keys in the vhash are the placeholder tokens to be replaced.
 * The values in the vhash are what the corresponding token will be replaced with.
 */
static void _prepare_arguments(
	SG_context*        pCtx,           //< [in] [out] Error and context info.
	const SG_filetool* pTool,          //< [in] The tool whose arguments to replace tokens in.
	const SG_vhash*    pTokenValues,   //< [in] Values for placeholder tokens in the tool's arguments.
	                                   //<      May be NULL if there are no values to substitute.
	const char*        szLogValueName, //< [in] If non-NULL, log each argument to the current operation under this base name.
	SG_exec_argvec**   ppArgVec        //< [out] An argument vector with the resulting command line arguments.
	                                   //<       Caller owns this.
	)
{
	SG_uint32       uArgCount     = 0u;
	SG_uint32       uArgIndex     = 0u;
	SG_exec_argvec* pArgVec       = NULL;
	SG_string*      sReplaced     = NULL;

	SG_NULLARGCHECK(pTool);
	SG_NULLARGCHECK(ppArgVec);

	// allocate an empty argvec
	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx, &pArgVec)  );

	// run through each argument in the tool
	SG_ERR_CHECK(  SG_varray__count(pCtx, pTool->pArguments, &uArgCount)  );
	for (uArgIndex = 0u; uArgIndex < uArgCount; ++uArgIndex)
	{
		const char* szArgument = NULL;

		// get the current argument
		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pTool->pArguments, uArgIndex, &szArgument)  );

		// run the replacement
		SG_ERR_CHECK(  SG_filetool__replace_tokens(pCtx, pTokenValues, szArgument, &sReplaced)  );

		// if the result isn't empty, append the replaced argument to the result argvec
		if (SG_string__length_in_bytes(sReplaced) > 0u)
		{
			SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_string__sz(sReplaced))  );
			if (szLogValueName != NULL)
			{
				SG_ERR_CHECK(  SG_log__set_value_index__sz(pCtx, szLogValueName, uArgIndex, SG_string__sz(sReplaced), SG_LOG__FLAG__INTERMEDIATE)  );
			}
		}

		// free the replaced string, since we'll get a new one next time
		SG_STRING_NULLFREE(pCtx, sReplaced);
	}

	// return the resulting argvec
	*ppArgVec = pArgVec;
	pArgVec = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sReplaced);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	return;
}

/**
 * Prepares/opens a file to be bound to an input/output stream of an external tool.
 */
static void _prepare_stream_file(
	SG_context*      pCtx,            //< [in] [out] Error and context info.
	const SG_string* pStreamArgument, //< [in] The stream argument for the stream file to bind.
	                                  //<      One of: pTool->StdIn, pTool->StdOut, or pTool->StdErr
	const SG_vhash*  pTokenValues,    //< [in] Values for placeholder tokens in the tool's stream files.
	                                  //<      May be NULL if there are no values to substitute.
	SG_file_flags    uFlags,          //< [in] Flags to use when opening the file.
	const char*      szLogValueName,  //< [in] If non-NULL, log the pathname to the current operation under this name.
	SG_file**        ppFile           //< [out] The file to bind to the tool's stream, or NULL to not bind one.
	)
{
	SG_string*   pReplaced = NULL;
	SG_pathname* pPathname = NULL;
	SG_file*     pFile     = NULL;

	SG_NULLARGCHECK(pTokenValues);
	SG_NULLARGCHECK(ppFile);

	// if the argument is NULL, then we don't need to bind a stream here
	if (pStreamArgument == NULL)
	{
		*ppFile = NULL;
		return;
	}

	// replace token values in the argument
	SG_ERR_CHECK(  SG_filetool__replace_tokens(pCtx, pTokenValues, SG_string__sz(pStreamArgument), &pReplaced)  );

	// allocate a pathname from the replaced value
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname, SG_string__sz(pReplaced))  );
	if (szLogValueName != NULL)
	{
		SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, szLogValueName, SG_string__sz(pReplaced), SG_LOG__FLAG__INTERMEDIATE)  );
	}

	// open the file
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname, uFlags, 0600, &pFile)  );

	// return the opened file
	*ppFile = pFile;
	pFile = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pReplaced);
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	return;
}

/**
 * Translates the exit code from an external tool into a more useful result value.
 *
 * This function should never be used on internal tools, because they return
 * result codes directly; their return values don't need translating.
 *
 * Each external tool assigns different meanings for exit codes.  The only real
 * convention is that zero indicates success, and usually non-zero indicates
 * failure.  Beyond that, it's hard to know what a particular exit code from an
 * external tool means.  Therefore, filetool makes it possible to configure a
 * translation for each that maps its exit codes to well defined result codes.
 *
 * The set of possible results is a combination of those built into filetool
 * (SG_FILETOOL__RESULT__*) and any defined by the tool type implementation
 * (so mergetool, difftool, and any others may each have additional type-specific
 * result codes).  Each possible result has both a string representation and
 * an unsigned integer representation.  The string representation exists to
 * improve the readability of tool configurations.  In a tool's config, we'd rather
 * see/configure mappings of exit codes to strings, rather than mapping them to
 * arbitrary magic numbers.  In the code, however, it's easier to work with the
 * magic numbers (hidden behind constants/#defines, of course).
 *
 * The translation happens in two phases:
 *
 * First, the exit code from the tool is lexically converted to a string (for
 * example, a 0 exit code becomes "0").  This is only done because we don't have a
 * convenient container that maps integer keys to strings.  That exit code string
 * is looked up in the translation map to determine the string representation of
 * the appropriate result (for example, if 0 indicates success, and the string
 * representation of the success result is "success", then the translation map
 * should contain a "0" -> "success" pair).  If the exit code isn't found in the
 * map, then whatever value the empty string maps to is used instead.  This
 * provides a default mechanism, allowing any exit code that's not explicitly
 * mapped to still have a default result code.  This also means that all
 * translation maps should contain a value for an empty string key!
 *
 * Second, the string representation of the result is itself looked up in the same
 * translation map to find an SG_int64 value, which is cast to an SG_uint32 and
 * used as the final result code (for example, if the unsigned integer
 * representation of the "success" result above is 5u, then the translation map
 * should contain a "success" -> 5 pair).
 *
 * Therefore, the given translation map should contain three sets of mappings:
 * 1) A map of exit codes to string results.
 * 2) A map of the empty string to the default string result.
 * 3) A map of all possible string results to their integer representations.
 * This can all work with the same map without conflicting keys because all keys
 * related to (1) will only contain numeric characters, and all keys related to
 * (3) will contain at least one alphabetic character, and no keys related to (1)
 * or (3) will be an empty string.
 */
void _translate_exit_code(
	SG_context*     pCtx,         //< [in] [out] Error and context info.
	const SG_vhash* pTranslation, //< [in] Translation map to use.
	SG_int32        iExitCode,    //< [in] The exit code to translate.
	SG_int32*       pResult,      //< [out] The translated result code.
	                              //<       May be NULL if unneeded.
	const char**    ppResult      //< [out] The string representation of the translated result code.
	                              //<       Points into pTranslation.
	                              //<       May be NULL if unneeded.
	)
{
	SG_int_to_string_buffer szExitCode;
	const char*             szResult;
	SG_int64                iResult;

	// parse the exit code into a string
	SG_ERR_CHECK(  SG_int64_to_sz(iExitCode, szExitCode)  );

	// translate the exit code string into a result string
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pTranslation, szExitCode, &szResult)  );
	if (szResult == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTranslation, SG_FILETOOL__EXIT_CODE__DEFAULT, &szResult)  );
	}

	// translate the result string into the equipvalent result code
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pTranslation, szResult, &iResult)  );

	// return the results
	if (pResult != NULL)
	{
		*pResult = (SG_int32)iResult;
	}
	if (ppResult != NULL)
	{
		*ppResult = szResult;
	}

fail:
	return;
}


/*
**
** Public Functions
**
*/

void SG_filetool__get_class(
	SG_context*        pCtx,
	const char*        szPath,
	const SG_pathname* pDiskPath,
	SG_repo*           pRepo,
	SG_string**        ppClass
	)
{
	SG_vhash*  pClasses    = NULL;
	SG_uint32  uClassCount = 0u;
	SG_uint32  uClassIndex = 0u;
	SG_string* pClass      = NULL;
	SG_bool    bFound      = SG_FALSE;
	SG_bool	   bAllocedClassesArray = SG_FALSE;
	SG_varray* pPatterns     = NULL;

	if (!szPath && !pDiskPath)
		SG_ERR_THROW(  SG_ERR_INVALIDARG  );

	SG_NULLARGCHECK(ppClass);

	// if they didn't supply a path, use the disk path
	if (szPath == NULL)
	{
		szPath = SG_pathname__sz(pDiskPath);
	}

	// allocate our output string
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pClass)  );

	if (bFound != SG_TRUE)
	{
		// try the file classes defined in localsettings
		SG_ERR_CHECK(  SG_localsettings__get__collapsed_vhash(pCtx, SG_FILETOOLCONFIG__CLASSES, pRepo, &pClasses)  );

		// run through each class
		// note: we're relying on vhash iterating in the order that items were added
		//       because we want types in higher priority scopes to override types in lower priority ones
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pClasses, &uClassCount)  );
		for (uClassIndex = 0u; uClassIndex < uClassCount; ++uClassIndex)
		{
			const char*       szClassName   = NULL;
			const SG_variant* pClassValue   = NULL;
			SG_vhash*         pClassData    = NULL;
			SG_uint32         uPatternCount = 0u;
			SG_uint32         uPatternIndex = 0u;
			SG_bool           bHasPatterns  = SG_FALSE;

			// get the data for this class
			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pClasses, uClassIndex, &szClassName, &pClassValue)  );
			SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pClassValue, &pClassData)  );

			// get the list of patterns
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pClassData, SG_FILETOOLCONFIG__CLASSPATTERNS, &bHasPatterns)  );
			if (bHasPatterns == SG_FALSE)
			{
				continue;
			}
		
			SG_vhash__get__varray(pCtx, pClassData, SG_FILETOOLCONFIG__CLASSPATTERNS, &pPatterns);
			bAllocedClassesArray = SG_FALSE;
			if (SG_context__err_equals(pCtx, SG_ERR_VARIANT_INVALIDTYPE) )
			{
				const char * pszValue = NULL;
				SG_context__err_reset(pCtx);
				//Try to load a single string.
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pClassData, SG_FILETOOLCONFIG__CLASSPATTERNS, &pszValue)  );
				bAllocedClassesArray = SG_TRUE;
				SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pPatterns)  );
				SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pPatterns, pszValue)  );
			}
			else
			{
				SG_ERR_CHECK_CURRENT;
			}

			// run through each pattern
			SG_ERR_CHECK(  SG_varray__count(pCtx, pPatterns, &uPatternCount)  );
			for (uPatternIndex = 0u; uPatternIndex < uPatternCount; ++uPatternIndex)
			{
				const char* szPattern = NULL;
				SG_bool     bMatches  = SG_FALSE;

				// get the current pattern
				SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pPatterns, uPatternIndex, &szPattern)  );

				// match this pattern against the filename
				SG_ERR_CHECK(  SG_file_spec__match_pattern(pCtx, szPattern, szPath, SG_FILE_SPEC__MATCH_ANYWHERE, &bMatches)  );

				// if it matched, use the current class name
				if (bMatches == SG_TRUE)
				{
					SG_ERR_CHECK(  SG_string__set__sz(pCtx, pClass, szClassName)  );
					bFound = SG_TRUE;
					break;
				}
			}
			if (bAllocedClassesArray)
				SG_VARRAY_NULLFREE(pCtx, pPatterns);
			
			// if this class matched, stop looking at others
			if (bFound == SG_TRUE)
			{
				break;
			}
		}
	}

	if ((bFound != SG_TRUE) && (pDiskPath))
	{
		// try to detect a class by sniffing the file.
		const char* szDefaultClass = NULL;

		SG_ERR_CHECK(  SG_filetool__get_default_class(pCtx, pDiskPath, &szDefaultClass)  );
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pClass, szDefaultClass)  );
		bFound = SG_TRUE;
	}

	if (bFound != SG_TRUE)
	{
		// just default to text
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pClass, SG_FILETOOL__CLASS__TEXT)  );
	}

	// return whatever class we ended up with
	*ppClass = pClass;
	pClass = NULL;

fail:
	if (bAllocedClassesArray)
		SG_VARRAY_NULLFREE(pCtx, pPatterns);
	SG_VHASH_NULLFREE(pCtx, pClasses);
	SG_STRING_NULLFREE(pCtx, pClass);
	return;
}

void SG_filetool__get_default_class(
	SG_context*        pCtx,
	const SG_pathname* pPathname,
	const char**       ppClass
	)
{
	SG_byte     aBuffer[1024] = { 0, };
	SG_file*    pFile         = NULL;
	SG_uint32   uBytesRead    = 0u;
	SG_uint32   uIndex        = 0u;
	const char* szClass       = NULL;

	// open the file
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, 0u, &pFile)  );

	// fill our buffer by reading from the file
	SG_file__read(pCtx, pFile, sizeof(aBuffer), aBuffer, &uBytesRead);

	if (SG_context__err_equals(pCtx, SG_ERR_EOF))
    {
		SG_context__err_reset(pCtx);
    }

	SG_ERR_CHECK_CURRENT;
	// first we'll check the initial bytes against our known set of "magic" numbers
	for (uIndex = 0u; uIndex < SG_NrElements(gpFileClassMagicNumbers); ++uIndex)
	{
		SG_uint32 uByte  = 0u;
		SG_bool   bMatch = SG_TRUE;

		// if the magic number is longer than the file, it definitely can't match
		if (gpFileClassMagicNumbers[uIndex].uSize > uBytesRead)
		{
			continue;
		}

		// run through and compare the magic number against the file's first bytes
		for (uByte = 0u; uByte < gpFileClassMagicNumbers[uIndex].uSize; ++uByte)
		{
			if (gpFileClassMagicNumbers[uIndex].pBytes[uByte] != aBuffer[uByte])
			{
				bMatch = SG_FALSE;
				break;
			}
		}

		// if we found a match, use the corresponding file class
		if (bMatch == SG_TRUE)
		{
			szClass = gpFileClassMagicNumbers[uIndex].szClass;
		}
	}

	// TODO: consider checking for #! line at the top of scripts
	//       maybe just add "#!" to the magic number list
	//       or maybe include logic to also look for something resembling a path

	// if we haven't figured it out yet, we'll scan the whole buffer for NUL characters
	// if we find one we'll assume binary, since NUL is generally invalid text
	if (szClass == NULL)
	{
		for (uIndex = 0u; uIndex < uBytesRead; ++uIndex)
		{
			if (aBuffer[uIndex] == '\0')
			{
				szClass = SG_FILETOOL__CLASS__BINARY;
				break;
			}
		}
	}

	// if we haven't figured it out yet, we'll just assume text
	if (szClass == NULL)
	{
		szClass = SG_FILETOOL__CLASS__TEXT;
	}

	// output whatever we found
	*ppClass = szClass;
	szClass = NULL;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	return;
}

void SG_filetool__find_tool(
	SG_context* pCtx,
	const char* szType,
	const char* szContext,
	const char* szClass,
	SG_repo*    pRepo,
	SG_string** ppName
	)
{
	SG_string*  pSettingName = NULL;
	SG_variant* pData        = NULL;
	const char* szName       = NULL;
	char * tmpszName         = NULL;

	SG_NULLARGCHECK(szClass);
	SG_NULLARGCHECK(ppName);

	// allocate a string to build setting names in
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pSettingName)  );

	// check for a tool defined for this type and file class
	if (pData == NULL)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pSettingName, "%s/%s/%s", SG_FILETOOLCONFIG__BINDINGS, szType, szClass)  );
		SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, SG_string__sz(pSettingName), pRepo, &pData, NULL)  );
	}

	// if we found some tool data, parse through it
	if (pData != NULL)
	{
		// check if we're interested in a context-specific tool
		if (szContext != NULL && pData->type == SG_VARIANT_TYPE_VHASH)
		{
			SG_vhash* pContexts = NULL;
			SG_bool   bExists   = SG_FALSE;

			// if there's a tool defined for this context in this class, retrieve its name
			SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pData, &pContexts)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pContexts, szContext, &bExists)  );
			if (bExists == SG_TRUE)
			{
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pContexts, szContext, &szName)  );
			}
			else
			{
				//See bug Y3132 for context. The initial lookup returned an empty hash for the filetool binding
				//(or one that doesn't contain the context we need).  Get the default one.  
				SG_VARIANT_NULLFREE(pCtx, pData);
				SG_ERR_CHECK(  SG_string__insert__sz(pCtx, pSettingName, 0, SG_LOCALSETTING__SCOPE__DEFAULT "/" )  );
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pSettingName, "/")  );
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pSettingName, szContext)  );
				SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_string__sz(pSettingName), pRepo, &tmpszName, NULL)  );
				szName = tmpszName;
			}
		}
		else if (pData->type == SG_VARIANT_TYPE_SZ)
		{
			// either we don't care about context, or this class doesn't have context-specific tools
			// we have a cross-context name, though, so use that
			SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pData, &szName)  );
		}
	}

	// if we found a name, return it
	if (szName != NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppName, szName)  );
	}
	else
	{
		*ppName = NULL;
	}

fail:
	SG_NULLFREE(pCtx, tmpszName);
	SG_STRING_NULLFREE(pCtx, pSettingName);
	SG_VARIANT_NULLFREE(pCtx, pData);
	return;
}

void SG_filetool__get_tool(
	SG_context*         pCtx,
	const char*         szType,
	const char*         szContext,
	const char*         szName,
	SG_filetool__plugin fPlugin,
	SG_repo*            pRepo,
	const SG_vhash*     pResultMap,
	SG_filetool**       ppTool
	)
{
	SG_string*   pSettingName = NULL;
	SG_vhash*    pData        = NULL;
	SG_filetool* pTool        = NULL;

	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppTool);

	// look for an external tool in the localsettings
	if (pTool == NULL)
	{
		// build the setting name
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pSettingName)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pSettingName, "%s/%s/%s", SG_FILETOOLCONFIG__TOOLS, szType, szName)  );

		// look up the setting
		SG_ERR_CHECK(  SG_localsettings__get__vhash(pCtx, SG_string__sz(pSettingName), pRepo, &pData, NULL)  );
		if (pData != NULL)
		{
			SG_filetool__invoker* fInvoker = NULL;

			// check if the caller has a custom external tool invoker
			if (fInvoker == NULL && fPlugin != NULL)
			{
				SG_ERR_CHECK(  fPlugin(pCtx, szType, szContext, NULL, &fInvoker)  );
			}

			// check for our own external tool invoker
			if (fInvoker == NULL)
			{
				SG_ERR_CHECK(  _plugin__default(pCtx, szType, szContext, NULL, &fInvoker)  );
			}

			// allocate a tool with the invoker we found
			if (fInvoker != NULL)
			{
				SG_ERR_CHECK(  _alloc_filetool(pCtx, szName, fInvoker, pResultMap, pData, &pTool)  );
			}
		}
	}

	// look for an internal tool
	if (pTool == NULL)
	{
		SG_filetool__invoker* fInvoker = NULL;

		// check if the caller is providing a matching internal tool
		if (fInvoker == NULL && fPlugin != NULL)
		{
			SG_ERR_CHECK(  fPlugin(pCtx, szType, szContext, szName, &fInvoker)  );
		}

		// check for our own matching internal tool
		if (fInvoker == NULL)
		{
			SG_ERR_CHECK(  _plugin__default(pCtx, szType, szContext, szName, &fInvoker)  );
		}

		// allocate a tool with the invoker we found
		if (fInvoker != NULL)
		{
			SG_ERR_CHECK(  _alloc_filetool(pCtx, szName, fInvoker, NULL, NULL, &pTool)  );
		}
	}

	// output whatever we found
	*ppTool = pTool;
	pTool = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pSettingName);
	SG_VHASH_NULLFREE(pCtx, pData);
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return;
}

void SG_filetool__list_tools(
	SG_context*      pCtx,
	const char*      szType,
	SG_repo*         pRepo,
	SG_stringarray** ppTools
	)
{
	SG_string*      pSetting = NULL;
	SG_vhash*       pHash    = NULL;
	SG_stringarray* pTools   = NULL;
	SG_uint32       uIndex   = 0u;
	SG_uint32       uCount   = 0u;

	// build the setting name to look up the tool list in
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pSetting)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pSetting, "%s/%s", SG_FILETOOLCONFIG__TOOLS, szType)  );

	// get a collapsed vhash of the setting across all relevent scopes
	SG_ERR_CHECK(  SG_localsettings__get__collapsed_vhash(pCtx, SG_string__sz(pSetting), pRepo, &pHash)  );

	// allocate our output list
	SG_STRINGARRAY__ALLOC(pCtx, &pTools, 15u);

	// run through the returned hash of children
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pHash, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char* szName = NULL;

		// the keys are tool names, the values are each a vhash of the tool's settings
		// just add the tool names to the output list
		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pHash, uIndex, &szName, NULL)  );
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, pTools, szName)  );
	}

	// add our own built-in tools to the list
	uCount = SG_NrElements(gpBuiltinFiletools);
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, pTools, gpBuiltinFiletools[uIndex].szName)  );
	}

	// return the result
	*ppTools = pTools;
	pTools = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pSetting);
	SG_VHASH_NULLFREE(pCtx, pHash);
	SG_STRINGARRAY_NULLFREE(pCtx, pTools);
	return;
}

void SG_filetool__free(
	SG_context*  pCtx,
	SG_filetool* pTool
	)
{
	if (pTool == NULL)
	{
		return;
	}

	SG_STRING_NULLFREE(pCtx, pTool->pName);
	SG_STRING_NULLFREE(pCtx, pTool->pExecutable);
	SG_VARRAY_NULLFREE(pCtx, pTool->pArguments);
	SG_STRING_NULLFREE(pCtx, pTool->pStdIn);
	SG_STRING_NULLFREE(pCtx, pTool->pStdOut);
	SG_STRING_NULLFREE(pCtx, pTool->pStdErr);
	SG_STRINGARRAY_NULLFREE(pCtx, pTool->pFlags);
	SG_VHASH_NULLFREE(pCtx, pTool->pResultMap);

	SG_ERR_CHECK(  SG_free(pCtx, pTool)  );

fail:
	return;
}

void SG_filetool__get_name(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const char**       ppName
	)
{
	SG_NULLARGCHECK(pTool);
	SG_NULLARGCHECK(ppName);

	*ppName = SG_string__sz(pTool->pName);

fail:
	return;
}

void SG_filetool__get_executable(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const char**       ppExecutable
	)
{
	SG_NULLARGCHECK(pTool);
	SG_NULLARGCHECK(ppExecutable);

	if (pTool->pExecutable)
		*ppExecutable = SG_string__sz(pTool->pExecutable);
	else
		*ppExecutable = NULL;

fail:
	return;
}

void SG_filetool__has_flag(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const char*        szFlag,
	SG_bool*           pHasFlag
	)
{
	if (pTool->pFlags == NULL)
	{
		*pHasFlag = SG_FALSE;
	}
	else
	{
		// Note: I am aware that doing a search on a stringarray is done in linear
		//       time and therefore slow, but I don't care in this case because the
		//       list of flags on a tool is never going to be large enough for it
		//       to matter.
		SG_ERR_CHECK(  SG_stringarray__find(pCtx, pTool->pFlags, szFlag, 0u, pHasFlag, NULL)  );
	}

fail:
	return;
}

void SG_filetool__invoke(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pResult
	)
{
	SG_string* sError = NULL;

	SG_NULLARGCHECK(pTool);

	SG_ERR_TRY(  pTool->fInvoker(pCtx, pTool, pTokenValues, pResult)  );
	SG_ERR_CATCH_ANY
	{
		// the tool invocation crashed/threw/errored
		if (pResult != NULL)
		{
			*pResult = SG_FILETOOL__RESULT__ERROR;
		}

		// log the underlying error before we lose it
		SG_context__err_to_string(pCtx, SG_FALSE, &sError);
		SG_ERR_IGNORE(  SG_log__report_error(pCtx, "%s", SG_string__sz(sError))  );
	}
	SG_ERR_CATCH_END;

fail:
	SG_STRING_NULLFREE(pCtx, sError);
	return;
}

void SG_filetool__invoke__null(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pResult
	)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pTool);
	SG_UNUSED(pTokenValues);

	if (pResult != NULL)
	{
		*pResult = 0;
	}
}

void SG_filetool__invoke__external(
	SG_context*        pCtx,
	const SG_filetool* pTool,
	const SG_vhash*    pTokenValues,
	SG_int32*          pResult
	)
{
	SG_exec_argvec* pArgVec     = NULL;
	SG_bool         bSystemCall = SG_FALSE;
	SG_file*        pStdIn      = NULL;
	SG_file*        pStdOut     = NULL;
	SG_file*        pStdErr     = NULL;
	SG_exit_status  iExitStatus = 0;
	SG_string*      sCommand    = NULL;
	SG_int32        iResult     = 0;
	const char*     szResult    = NULL;

	SG_NULLARGCHECK(pTool);

	// log an operation
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Invoking filetool.", SG_LOG__FLAG__VERBOSE)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Tool Name", SG_string__sz(pTool->pName), SG_LOG__FLAG__INPUT)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Executable", SG_string__sz(pTool->pExecutable), SG_LOG__FLAG__INTERMEDIATE)  );

	// substitute our argument values into the tool's arguments
	SG_ERR_CHECK(  _prepare_arguments(pCtx, pTool, pTokenValues, "Argument", &pArgVec)  );

	// check how we should invoke this tool
	SG_ERR_CHECK(  SG_filetool__has_flag(pCtx, pTool, SG_FILETOOL__FLAG__SYSTEM_CALL, &bSystemCall)  );
	if (bSystemCall == SG_FALSE)
	{
		// open any files that we need to bind to streams
		SG_ERR_CHECK(  _prepare_stream_file(pCtx, pTool->pStdIn,  pTokenValues, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING,                  "StdIn",  &pStdIn)  );
		SG_ERR_CHECK(  _prepare_stream_file(pCtx, pTool->pStdOut, pTokenValues, SG_FILE_WRONLY | SG_FILE_OPEN_OR_CREATE | SG_FILE_TRUNC, "StdOut", &pStdOut)  );
		SG_ERR_CHECK(  _prepare_stream_file(pCtx, pTool->pStdErr, pTokenValues, SG_FILE_WRONLY | SG_FILE_OPEN_OR_CREATE | SG_FILE_TRUNC, "StdErr", &pStdErr)  );

		// execute the tool
		SG_ERR_CHECK(  SG_exec__exec_sync__files(pCtx, SG_string__sz(pTool->pExecutable), pArgVec, pStdIn, pStdOut, pStdErr, &iExitStatus)  );
	}
	else
	{
		// execute the tool
		SG_ERR_CHECK(  SG_exec__build_command(pCtx, SG_string__sz(pTool->pExecutable), pArgVec, &sCommand)  );
		SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Command", SG_string__sz(sCommand), SG_LOG__FLAG__INTERMEDIATE)  );
		SG_ERR_CHECK(  SG_exec__system(pCtx, SG_string__sz(sCommand), &iExitStatus)  );
	}

	// translate the exit code into a result
	SG_ERR_CHECK(  _translate_exit_code(pCtx, pTool->pResultMap, iExitStatus, &iResult, &szResult)  );

	// log the results
	SG_ERR_CHECK(  SG_log__set_value__int(pCtx, "Exit Code", iExitStatus, SG_LOG__FLAG__OUTPUT)  );
	SG_ERR_CHECK(  SG_log__set_value__int(pCtx, "Result Code", iResult, SG_LOG__FLAG__OUTPUT)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Result String", szResult, SG_LOG__FLAG__OUTPUT)  );
#if 0
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "FileTool: mapped '%d' --> {'%d','%s'}\n",
							   iExitStatus, iResult, szResult)  );
#endif

	// return the result
	if (pResult != NULL)
	{
		*pResult = iResult;
	}

fail:
	SG_log__pop_operation(pCtx);
	SG_FILE_NULLCLOSE(pCtx, pStdIn);
	SG_FILE_NULLCLOSE(pCtx, pStdOut);
	SG_FILE_NULLCLOSE(pCtx, pStdErr);
	SG_STRING_NULLFREE(pCtx, sCommand);
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	return;
}

void SG_filetool__replace_tokens(
	SG_context*     pCtx,
	const SG_vhash* pValues,
	const char*     szInput,
	SG_string**     ppOutput
	)
{
	SG_uint32  uTokenCount = 0u;
	SG_uint32  uTokenIndex = 0u;
	SG_string* pResult     = NULL;

	// allocate a result string, starting with the given input
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pResult, szInput)  );

	// run through each value in the hash of token values
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pValues, &uTokenCount)  );
	for (uTokenIndex = 0u; uTokenIndex < uTokenCount; ++uTokenIndex)
	{
		const char*       szName  = NULL;
		const SG_variant* pValue  = NULL;

		// get the current name/value pair
		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pValues, uTokenIndex, &szName, &pValue)  );
		if (pValue->type == SG_VARIANT_TYPE_BOOL)
		{
			SG_bool bValue = SG_FALSE;

			SG_ERR_CHECK(  SG_variant__get__bool(pCtx, pValue, &bValue)  );
			SG_ERR_CHECK(  _replace_token__bool(pCtx, pResult, szName, bValue)  );
		}
		else if (pValue->type == SG_VARIANT_TYPE_INT64)
		{
			SG_int64 iValue = 0;

			SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pValue, &iValue)  );
			SG_ERR_CHECK(  _replace_token__int64(pCtx, pResult, szName, iValue)  );
		}
		else
		{
			const char* szValue = NULL;

			SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pValue, &szValue)  );
			SG_ERR_CHECK(  _replace_token__sz(pCtx, pResult, szName, szValue)  );
		}
	}

	// return the result
	*ppOutput = pResult;
	pResult = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pResult);
	return;
}

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
static void _try__look_for_diffmerge_exe_path_in_registry(SG_context * pCtx, const char * pszTry, SG_pathname ** ppPathExe)
{
	HKEY hKey = NULL;
	char * pszBuf = NULL;
	SG_pathname * pPathExe = NULL;
	DWORD dwType;
	DWORD dwLenBuf;
	SG_bool bExists;
	SG_fsobj_type fsobjType;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, pszTry, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

	if (RegQueryValueExA(hKey, "Location", NULL, &dwType, NULL, &dwLenBuf) != ERROR_SUCCESS)
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

	if (dwType != REG_SZ)
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

	SG_ERR_CHECK(  SG_alloc(pCtx, dwLenBuf+1, sizeof(char), &pszBuf)  );
#if defined(DEBUG)
	_sg_mem__set_caller_data(pszBuf,__FILE__,__LINE__,"RegistryBuffer");
#endif

	if (RegQueryValueExA(hKey, "Location", NULL, &dwType, (LPBYTE)pszBuf, &dwLenBuf) != ERROR_SUCCESS)
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathExe, pszBuf)  );
	
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathExe, &bExists, &fsobjType, NULL)  );
	if (!bExists || (fsobjType != SG_FSOBJ_TYPE__REGULAR))
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

#if TRACE_DIFFMERGE_LOOKUP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Found DiffMerge path in registry key [HKLM] \\ %s \\ Location = %s\n",
							   pszTry, SG_pathname__sz(pPathExe))  );
#endif

	*ppPathExe = pPathExe;
	pPathExe = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathExe);
	SG_NULLFREE(pCtx, pszBuf);
	if (hKey)
		RegCloseKey(hKey);
}

static void _look_for_diffmerge_exe_path_in_registry(SG_context * pCtx, SG_pathname ** ppPathExe)
{
	static const char * aszKeys[] = {
		"SOFTWARE\\SourceGear\\Common\\DiffMerge\\Installer",					// location for MSM-based 3.3.1+
		"SOFTWARE\\SourceGear\\SourceGear DiffMerge\\Installer",				// same-bitness location for 3.3.0 and earlier MSI
		"SOFTWARE\\Wow6432Node\\SourceGear\\SourceGear DiffMerge\\Installer",	// 32-bit 3.3.0 and earlier MSI on 64-bit system 
	};

	SG_uint32 k;

	for (k=0; k<SG_NrElements(aszKeys); k++)
	{
		_try__look_for_diffmerge_exe_path_in_registry(pCtx, aszKeys[k], ppPathExe);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
			return;

		if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			SG_ERR_RETHROW_RETURN;
		SG_context__err_reset(pCtx);
	}

	SG_ERR_THROW_RETURN(  SG_ERR_NOT_FOUND  );
}

static void _try__look_for_diffmerge_exe_path_in_program_files(SG_context * pCtx, const char * pszTry, SG_pathname ** ppPathExe)
{
	SG_pathname * pPathExe = NULL;
	SG_bool bExists;
	SG_fsobj_type fsobjType;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathExe, pszTry)  );

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathExe, &bExists, &fsobjType, NULL)  );
	if (!bExists || (fsobjType != SG_FSOBJ_TYPE__REGULAR))
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

#if TRACE_DIFFMERGE_LOOKUP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Found DiffMerge path in program files: %s\n",
							   SG_pathname__sz(pPathExe))  );
#endif

	*ppPathExe = pPathExe;
	pPathExe = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathExe);
}

static void _look_for_diffmerge_exe_path_in_program_files(SG_context * pCtx, SG_pathname ** ppPathExe)
{
	// 2010/09/30 This is a complete hack.  There, I said it,
	//            but I'm going to do it anyway.
	// 
	//            We want to find SourceGear DiffMerge.  Normally,
	//            I'd just use the entryname and tell the user to
	//            make sure it can be found somewhere in their PATH,
	//            but I just noticed that VS2010 includes a program
	//            called .../Common7/IDE/diffmerge.exe that might
	//            get in the way.  So for now, I'm going to do this.
	// 
	//            This also lets users who have Vault or Fortress
	//            installed (which includes a bundled version of
	//            DiffMerge) to magically work without having to
	//            install the stand-alone version.
	// 
	//            I'm going to list all of the standard install
	//            directories for each product for both 32- and
	//            64-bit machines.

	static const char * aszGuesses[] = {
		"C:\\Program Files\\SourceGear\\Common\\DiffMerge\\sgdm.exe",			// 3.3.1+ MSM-based common
		"C:\\Program Files (x86)\\SourceGear\\Common\\DiffMerge\\sgdm.exe",

		"C:\\Program Files\\SourceGear\\DiffMerge\\DiffMerge.exe",				// 3.3.0 and earlier stand-alone
		"C:\\Program Files (x86)\\SourceGear\\DiffMerge\\DiffMerge.exe",

		"C:\\Program Files\\SourceGear\\Fortress Client\\sgdm.exe",				// bundled versions with other products
		"C:\\Program Files (x86)\\SourceGear\\Fortress Client\\sgdm.exe",
		"C:\\Program Files\\SourceGear\\Vault Client\\sgdm.exe",
		"C:\\Program Files (x86)\\SourceGear\\Vault Client\\sgdm.exe",
		"C:\\Program Files\\SourceGear\\VaultPro Client\\sgdm.exe",
		"C:\\Program Files (x86)\\SourceGear\\VaultPro Client\\sgdm.exe",
	};

	SG_uint32 k;

	for (k=0; k<SG_NrElements(aszGuesses); k++)
	{
		_try__look_for_diffmerge_exe_path_in_program_files(pCtx, aszGuesses[k], ppPathExe);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
			return;

		if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			SG_ERR_RETHROW_RETURN;
		SG_context__err_reset(pCtx);
	}

	SG_ERR_THROW_RETURN(  SG_ERR_NOT_FOUND  );
}
		
static void _try__look_for_diffmerge_exe_path_using_PATH(SG_context * pCtx, const wchar_t * pwszExeName, SG_pathname ** ppPathExe)
{
	SG_uint32 lenAllocated, lenRequired;
	SG_string * pString = NULL;
	wchar_t * pWideBuf = NULL;
	SG_pathname * pPathExe = NULL;

	// The registry lookup failed and we didn't find it in any of the known
	// install directories, so it probably isn't installed.  But before we
	// give up, see if they have a copy in their PATH.  This takes care of
	// the case where they used the DiffMerge ZIP distribution rather than
	// one of the installer packages.
	//
	// I hesitate to call SearchPath() and look
	// for it, but if we just return the entryname and let CreateProcess()
	// try to find it and it gets the MSFT version, the use will see an
	// odd syntax error dialog (because we take different arguments).

	lenAllocated = 4096;
	pWideBuf = (wchar_t *)SG_calloc(lenAllocated,sizeof(wchar_t));
	if (!pWideBuf)
		SG_ERR_THROW( SG_ERR_MALLOCFAILED );

top:
	lenRequired = SearchPath(NULL, pwszExeName, NULL, lenAllocated, pWideBuf, NULL);
	if (lenRequired == 0)				// We did not find any instances anywhere in our PATH.
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

	if (lenRequired > lenAllocated)		// pathname too long for our buffer, try again.
	{
		SG_NULLFREE(pCtx, pWideBuf);
		lenAllocated += 4096;
		pWideBuf = (wchar_t *)SG_calloc(lenAllocated,sizeof(wchar_t));
		if (!pWideBuf)
			SG_ERR_THROW( SG_ERR_MALLOCFAILED );
		goto top;
	}

	// We found it somewhere in the PATH, who knows what it is.

	if (wcsstr(pWideBuf, L"Visual Studio"))
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

	// If not part of Visual Studio, assume it is ours.
	// We need to convert the wchar pathname into a utf8 psz
	// before we can put it in a SG_pathname.

	SG_ERR_CHECK(  SG_string__alloc(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pString, pWideBuf)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathExe, SG_string__sz(pString))  );

#if TRACE_DIFFMERGE_LOOKUP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Found DiffMerge path in PATH: %s\n",
							   SG_pathname__sz(pPathExe))  );
#endif

	*ppPathExe = pPathExe;
	pPathExe = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathExe);
	SG_STRING_NULLFREE(pCtx, pString);
	SG_NULLFREE(pCtx, pWideBuf);
}

static void _look_for_diffmerge_exe_path_using_PATH(SG_context * pCtx, SG_pathname ** ppPathExe)
{
	static const wchar_t * aszGuesses[] = {
		L"sgdm.exe",
		L"DiffMerge.exe",
	};
	SG_uint32 k;

	for (k=0; k<SG_NrElements(aszGuesses); k++)
	{
		_try__look_for_diffmerge_exe_path_using_PATH(pCtx, aszGuesses[k], ppPathExe);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
			return;

		if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			SG_ERR_RETHROW_RETURN;
		SG_context__err_reset(pCtx);
	}

	SG_ERR_THROW_RETURN(  SG_ERR_NOT_FOUND  );
}

void SG_filetool__get_builtin_diffmerge_exe(
	SG_context * pCtx,          //< [in] [out] Error and context info.
	SG_pathname ** ppPathCmd    //< [out] Path to diffmerge executable. Caller owns it.
	)
{
	// The official DiffMerge MSI/MSM installers recorded the install path in the registry.

	_look_for_diffmerge_exe_path_in_registry(pCtx, ppPathCmd);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
		return;

	if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
		SG_ERR_RETHROW_RETURN;
	SG_context__err_reset(pCtx);

	// Search for the EXE in all of the various other products it might be bundled with.

	_look_for_diffmerge_exe_path_in_program_files(pCtx, ppPathCmd);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
		return;

	if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
		SG_ERR_RETHROW_RETURN;
	SG_context__err_reset(pCtx);

	// See if it is in their PATH (while avoiding the MSFT program with the same name).

	_look_for_diffmerge_exe_path_using_PATH(pCtx, ppPathCmd);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
		return;

	if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
		SG_ERR_RETHROW_RETURN;
	SG_context__err_reset(pCtx);

	// anywhere else we should look ?

	SG_ERR_THROW2_RETURN(  SG_ERR_DIFFMERGE_EXE_NOT_FOUND,
						   (pCtx, "Please install DiffMerge.")  );
}
#endif // end if defined(WINDOWS)

#if defined(LINUX) || defined(MAC)
static void _try__look_for_diffmerge_exe_path_in_known_location(SG_context * pCtx, const char * pszTry, SG_pathname ** ppPathExe)
{
	SG_pathname * pPathExe = NULL;
	SG_bool bExists;
	SG_fsobj_type fsobjType;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathExe, pszTry)  );

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathExe, &bExists, &fsobjType, NULL)  );
	if (!bExists || (fsobjType != SG_FSOBJ_TYPE__REGULAR))
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );

#if TRACE_DIFFMERGE_LOOKUP
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Found DiffMerge path in: %s\n",
							   SG_pathname__sz(pPathExe))  );
#endif

	*ppPathExe = pPathExe;
	pPathExe = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathExe);
}
#endif

#if defined(LINUX)
static void _look_for_diffmerge_exe_path_in_known_location(SG_context * pCtx, SG_pathname ** ppPathExe)
{
	static const char * aszGuesses[] = {
		"/usr/bin/diffmerge",			// official Ubuntu and Fedora installers put it here
		"/usr/local/bin/diffmerge",
	};

	SG_uint32 k;

	for (k=0; k<SG_NrElements(aszGuesses); k++)
	{
		_try__look_for_diffmerge_exe_path_in_known_location(pCtx, aszGuesses[k], ppPathExe);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
			return;

		if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			SG_ERR_RETHROW_RETURN;
		SG_context__err_reset(pCtx);
	}

	SG_ERR_THROW_RETURN(  SG_ERR_NOT_FOUND  );
}
#endif

#if defined(MAC)
static void _look_for_diffmerge_exe_path_in_known_location(SG_context * pCtx, SG_pathname ** ppPathExe)
{
	static const char * aszGuesses[] = {
		"/usr/bin/diffmerge",			// they installed the shell script I put on the .DMG.
		"/usr/local/bin/diffmerge",
		"/Applications/DiffMerge.app/Contents/Resources/diffmerge.sh",	// use the hidden version of the shell script I put in the .APP.
	};

	SG_uint32 k;

	for (k=0; k<SG_NrElements(aszGuesses); k++)
	{
		_try__look_for_diffmerge_exe_path_in_known_location(pCtx, aszGuesses[k], ppPathExe);
		if (!SG_CONTEXT__HAS_ERR(pCtx))
			return;

		if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			SG_ERR_RETHROW_RETURN;
		SG_context__err_reset(pCtx);
	}

	SG_ERR_THROW_RETURN(  SG_ERR_NOT_FOUND  );
}
#endif

#if defined(LINUX) || defined(MAC)
void SG_filetool__get_builtin_diffmerge_exe(
	SG_context * pCtx,          //< [in] [out] Error and context info.
	SG_pathname ** ppPathCmd    //< [out] Path to diffmerge executable. Caller owns it.
	)
{
	_look_for_diffmerge_exe_path_in_known_location(pCtx, ppPathCmd);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
		return;

	if (!SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
		SG_ERR_RETHROW_RETURN;
	SG_context__err_reset(pCtx);

	// anywhere else we should look ?

	SG_ERR_THROW2_RETURN(  SG_ERR_DIFFMERGE_EXE_NOT_FOUND,
						   (pCtx, "Please install DiffMerge.")  );
}
#endif
