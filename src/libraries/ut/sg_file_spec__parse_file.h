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

//////////////////////////////////////////////////////////////////

#ifndef H_SG_FILE_SPEC__PARSE_FILE_H
#define H_SG_FILE_SPEC__PARSE_FILE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Reads the entire contents of a text file into a string buffer.
 */
static void _read_file(
	SG_context* pCtx,       //< [in] [out] Error and context info.
	const SG_pathname * pPathname, //< [in] The name of the text file to read.
	char**      ppBuffer,   //< [out] A string buffer containing the contents of the file.  Caller owns this memory.
	SG_uint32*  pBufferSize //< [out] The size of the output buffer (including NULL terminator).
	)
{
	SG_file*     pFile       = NULL;
	SG_uint64    uFileSize64 = 0u;
	SG_uint32    uFileSize32 = 0u;
	char*        szBuffer    = NULL;
	SG_uint32    uBufferSize = 0u;

	SG_NULLARGCHECK( pPathname );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );

	// get the file's length
	SG_ERR_CHECK(  SG_file__seek_end(pCtx, pFile, &uFileSize64)  );
	SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, 0u)  );

	// truncate the length to a 32-bit size
	// if someone manages to give us a >4GB file, we'll just read the first 4GB of it, I guess
	// the -1 is to make sure we'll have space for a NULL terminator
	uFileSize32 = (SG_uint32)SG_MIN(uFileSize64, SG_UINT32_MAX-1);

	// read the file
	if (ppBuffer != NULL)
	{
		// allocate a buffer
		uBufferSize = uFileSize32 + 1;
		SG_ERR_CHECK(  SG_alloc(pCtx, uBufferSize, sizeof(char), &szBuffer)  );

		// read the file into the buffer
		if (uFileSize32 > 0u)
		{
			SG_uint32 uReadSize = 0u;

			SG_ERR_CHECK(  SG_file__read(pCtx, pFile, uFileSize32, (SG_byte*)szBuffer, &uReadSize)  );
			if (uFileSize32 != uReadSize)
			{
				SG_ERR_THROW(SG_ERR_INCOMPLETEREAD);
			}
		}

		szBuffer[uFileSize32] = '\0';
	}

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	if (ppBuffer != NULL)
		*ppBuffer = szBuffer;
	if (pBufferSize != NULL)
		*pBufferSize = uBufferSize;
	return;

fail:
	SG_NULLFREE(pCtx, szBuffer);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

/**
 * Gets the length of a line from a string.
 * In other words, reads a string up until the next newline and returns the number of characters read.
 * Also returns the start of the next line in the string, if there is one.
 */
static void _get_line_length(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	const char*  szStart,   //< [in] String to read a line from.
	SG_uint32*   pLength,   //< [out] The length from szStart to the end of the line.
	const char** ppNextLine //< [out] The place where the next line starts (or set to NULL if no more lines exist).
	                        //<       Pass this as szStart on the next call when reading many consecutive lines.
	)
{
	const char* szCurrent = szStart;
	SG_uint32   uLength   = 0u;

	SG_NULLARGCHECK(szStart);
	SG_NULLARGCHECK(pLength);

	// loop through characters in the string until we find a newline or the end of the string
	while (
		*szCurrent != '\r' &&
		*szCurrent != '\n' &&
		*szCurrent != '\0' &&
		uLength < SG_UINT32_MAX
		)
	{
		++szCurrent;
		++uLength;
	}

	// output the length
	*pLength = uLength;

	// find the next line after the newline
	if (ppNextLine != NULL)
	{
		if (szCurrent[0] == '\0')
		{
			*ppNextLine = NULL;
		}
		else if (szCurrent[0] == '\r' && szCurrent[1] == '\n')
		{
			*ppNextLine = szCurrent + 2;
		}
		else if (uLength == SG_UINT32_MAX)
		{
			*ppNextLine = szCurrent;
			SG_ERR_THROW(SG_ERR_LIMIT_EXCEEDED);
		}
		else
		{
			*ppNextLine = szCurrent + 1;
		}

		// if the next line would be an empty last line
		// just say that we're on the last line now
		// this effectively ignores newlines at the end of files
		if (*ppNextLine != NULL && **ppNextLine == '\0')
		{
			*ppNextLine = NULL;
		}
	}

fail:
	return;
}

/**
 * Checks if a character is whitespace.
 */
static SG_bool _is_whitespace(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	char        cCharacter //< [in] The character to check.
	)
{
	SG_UNUSED(pCtx);

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

/**
 * Finds the substring within a string that doesn't contain leading or trailing whitespace.
 */
static void _trim_whitespace(
	SG_context*  pCtx,           //< [in] [out] Error and context info.
	const char*  szInputString,  //< [in] The string to trim whitespace from.
	SG_uint32    uInputLength,   //< [in] The length of the input string in characters.
	const char** ppOutputString, //< [out] Pointed to the first character in the input string that's not whitespace.
	SG_uint32*   pOutputLength   //< [out] Set to the length of the output string that won't include any trailing whitespace.
	)
{
	SG_NULLARGCHECK(szInputString);
	SG_NULLARGCHECK(ppOutputString);
	SG_NULLARGCHECK(pOutputLength);

	// start with the input string
	*ppOutputString = szInputString;
	*pOutputLength  = uInputLength;

	// nothing to trim off a zero-length string
	if (uInputLength == 0u)
	{
		return;
	}

	// advance past any leading whitespace
	while (*pOutputLength > 0u && _is_whitespace(pCtx, **ppOutputString) == SG_TRUE)
	{
		++*ppOutputString;
		--*pOutputLength;
	}

	// shorten the length past any trailing whitespace
	while (*pOutputLength > 0u && _is_whitespace(pCtx, (*ppOutputString)[(*pOutputLength)-1]) == SG_TRUE)
	{
		--*pOutputLength;
	}

fail:
	return;
}

/**
 * Parse our include-from and/or .sgignore file format.
 * This is the basic "trim whitespace, ignore lines starting
 * with '#' or ';' and blank lines.  We DO NOT interpret
 * the contents of the file, just build a VARRAY of STRINGS
 * of the patterns.  We do not check for duplicates.
 */
static void _sg_file_spec__parse_file(SG_context * pCtx,
									  const SG_pathname * pPathname,
									  SG_varray ** ppvaResult)
{
	static const char* szCommentChars = "#;";

	SG_varray * pvaResult = NULL;
	char*       szFileContents = NULL;
	SG_uint32   uFileSize      = 0u;
	const char* szLine         = NULL;
	SG_uint32   uLineLength    = 0u;

	SG_NULLARGCHECK(pPathname);

#if TRACE_SPEC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "SG_file_spec__parse_file: '%s'\n", SG_pathname__sz(pPathname))  );
#endif

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaResult)  );

	// read the file into a buffer
	SG_ERR_CHECK(  _read_file(pCtx, pPathname, &szFileContents, &uFileSize)  );

	// loop through each line in the buffer
	szLine = szFileContents;
	while (szLine != NULL)
	{
		const char* szTrimmed      = NULL;
		SG_uint32   uTrimmedLength = 0u;
		const char* szNextLine     = NULL;

		// get the length of the current line and the start of the next line
		SG_ERR_CHECK(  _get_line_length(pCtx, szLine, &uLineLength, &szNextLine)  );

		// trim the whitespace off the line
		SG_ERR_CHECK(  _trim_whitespace(pCtx, szLine, uLineLength, &szTrimmed, &uTrimmedLength)  );

		// add this line as a pattern, as long as it's not empty or a comment
		if (uTrimmedLength > 0u && strchr(szCommentChars, szTrimmed[0]) == NULL)
		{
			SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pvaResult, szTrimmed, uTrimmedLength)  );
		}

		// move on to the next line
		szLine = szNextLine;
	}

	*ppvaResult = pvaResult;
	pvaResult = NULL;

fail:
	SG_NULLFREE(pCtx, szFileContents);
	SG_VARRAY_NULLFREE(pCtx, pvaResult);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_FILE_SPEC__PARSE_FILE_H
