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
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

#if defined(WINDOWS)
#define DEFAULT_EDITOR "notepad"
#else
#define DEFAULT_EDITOR "vi"
#endif

#define EDITOR_CONFIG_PATH "/machine/editor"

#define COMMENT_FILE__LINE_ENDING SG_PLATFORM_NATIVE_EOL_STR
#define COMMENT_FILE__LINE_PREFIX "#"

#define COMMENT_FILE__BOILERPLATE	COMMENT_FILE__LINE_ENDING \
									COMMENT_FILE__LINE_ENDING \
									COMMENT_FILE__LINE_PREFIX " Enter comment.  Lines beginning with '" COMMENT_FILE__LINE_PREFIX "' are removed." COMMENT_FILE__LINE_ENDING \
									COMMENT_FILE__LINE_PREFIX " Leave message empty to cancel." COMMENT_FILE__LINE_ENDING \
									COMMENT_FILE__LINE_PREFIX " --" COMMENT_FILE__LINE_ENDING

#define COMMENT_FILE__HDR_USER		COMMENT_FILE__LINE_PREFIX "       User: %s" COMMENT_FILE__LINE_ENDING
#define COMMENT_FILE__HDR_BRANCH	COMMENT_FILE__LINE_PREFIX "     Branch: %s" COMMENT_FILE__LINE_ENDING
#define COMMENT_FILE__HDR_WIT_ITEM	COMMENT_FILE__LINE_PREFIX " Associated: " // buf[0..len] + "..." + line-ending
#define COMMENT_FILE__HDR_CHG_ITEM	COMMENT_FILE__LINE_PREFIX "    Changed: %s" COMMENT_FILE__LINE_ENDING


static void _get_editor(
	SG_context* pCtx,
	char** ppszEditor);

static void _get_comment_from_file(SG_context* pCtx, const SG_pathname* pFilePathname, char** ppszComment)
{
	SG_string* pstrCommentFile = NULL;
	SG_string* pstrReturn = NULL;

	SG_file__read_into_string(pCtx, pFilePathname, &pstrCommentFile);

	if (SG_context__has_err(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_EOF)) // empty file
		{
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrCommentFile)  );
		}
		else
			SG_ERR_CHECK_CURRENT;
	}

	if (SG_string__length_in_bytes(pstrCommentFile) == 0)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrReturn)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrReturn, SG_string__length_in_bytes(pstrCommentFile))  );
	}

	{
		const SG_uint32 lenPrefix = (const SG_uint32)strlen(COMMENT_FILE__LINE_PREFIX);
		const SG_byte* pos = (SG_byte*)SG_string__sz(pstrCommentFile);
		const SG_uint32 len = SG_string__length_in_bytes(pstrCommentFile);
		const SG_byte* lastByte = pos + len - 1; /* There's no contract that SG_string's buffer is null-terminated. Use this instead. */

		/* Copy lines that don't start with COMMENT_FILE__LINE_PREFIX */
		while (pos <= lastByte)
		{
			SG_bool bSkipLine = SG_FALSE;
			const SG_byte* szLineStart;

			if ( pos + lenPrefix <= lastByte && !strncmp((const char*)pos, COMMENT_FILE__LINE_PREFIX, lenPrefix) )
				bSkipLine = SG_TRUE;

			/* Find the end of the line or the end of the buffer */
			szLineStart = pos;
			while ( (pos <= lastByte) && (*pos != SG_CR) && (*pos != SG_LF) )
				pos++;

			if (pos < lastByte)
			{
				/* Include the line ending character(s), handle different types. */
				if (*pos == SG_CR && *(pos+1) == SG_LF)
					pos += 2;
				else
					pos++;
			}

			if (!bSkipLine && pos != szLineStart)
				SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pstrReturn, szLineStart, (SG_uint32)(pos-szLineStart))  );

			if (pos == lastByte)
				break;
		}
	}

	/* Trim trailing line breaks */
	{
		SG_byte myByte;
		while (SG_string__length_in_bytes(pstrReturn))
		{
			SG_ERR_CHECK(  SG_string__get_byte_r(pCtx, pstrReturn, 0, &myByte)  );
			if (myByte == SG_CR || myByte == SG_LF)
				SG_ERR_CHECK(  SG_string__remove(pCtx, pstrReturn, SG_string__length_in_bytes(pstrReturn) - 1, 1)  );
			else
				break;
		}
	}
	
	if (SG_string__length_in_bytes(pstrReturn))
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrReturn, (SG_byte**)ppszComment, NULL)  );

	/* Fall through to common cleanup */
fail:
	SG_STRING_NULLFREE(pCtx, pstrCommentFile);
	SG_STRING_NULLFREE(pCtx, pstrReturn);
}

//////////////////////////////////////////////////////////////////

static void _sg_cmd_util__append_associations(
	SG_context *pCtx,
	SG_string * pString,
	const SG_varray *descs)
{
	SG_uint32 i, count;

	if (!descs)
		return;

	SG_ERR_CHECK(  SG_varray__count(pCtx, descs, &count)  );
	if (count == 0)
		return;
	
	for ( i = 0; i < count; ++i )
	{
		const char *asz, *t;
		SG_uint32 len;
		const char *rest = COMMENT_FILE__LINE_ENDING;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, descs, i, &asz)  );
		len = (SG_uint32)strlen(asz);

		for ( t = asz; *t; ++t )
			if ((*t == '\r') || (*t == '\n'))
			{
				len = (SG_uint32)(t - asz);
				rest = "..." COMMENT_FILE__LINE_ENDING;
				break;
			}

		SG_ERR_CHECK(  SG_string__append__sz(     pCtx, pString, COMMENT_FILE__HDR_WIT_ITEM)  );
		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pString, (const SG_byte *)asz, len)  );
		SG_ERR_CHECK(  SG_string__append__sz(     pCtx, pString, rest)  );
	}

fail:
	return;
}

static void _sg_cmd_util__append_status(
	SG_context * pCtx,
	SG_string * pStringResult,
	const SG_varray * pvaStatusCanonical)
{
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatusCanonical, &count)  );
	if (count == 0)
		return;

	for (k=0; k<count; k++)
	{
		SG_int64 i64;
		SG_vhash * pvhItem;
		SG_vhash * pvhStatus;
		SG_wc_status_flags statusFlags;
		const char * pszPath;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaStatusCanonical, k, &pvhItem)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhStatus)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus, "flags", &i64)  );
		statusFlags = ((SG_wc_status_flags)i64);

		if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND | SG_WC_STATUS_FLAGS__U__IGNORED))
			continue;

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszPath)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringResult, COMMENT_FILE__HDR_CHG_ITEM, pszPath)  );
	}

fail:
	return;
}

void SG_cmd_util__get_comment_from_editor(
	SG_context * pCtx,
	SG_bool bForCommit,
	const char * pszUserName,
	const char * pszBranchName,
	const SG_varray * pvaStatusCanonical,
	const SG_varray * pvaDescsAssociations,
	char ** ppszComment)
{
	SG_tempfile* pTempFile = NULL;
	char* pszEditor = NULL;
	SG_string* pstrFullCmdLine = NULL;
	SG_string* pstrEscapedTempFile = NULL;
	SG_exit_status editorExitStatus;
	char* pszComment = NULL;
	SG_string * pString = NULL;

	SG_NULLARGCHECK_RETURN( pszUserName );
	// pszBranchName is optional -- when commit, defaults to "*None*".
	// pvaStatusCanonical is optional -- only required when commit.
	// pvaDescsAssociations is optional -- optional when commit.
	SG_NULLARGCHECK_RETURN(ppszComment);

	// build the content of the tmp file that we show the user.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__append__sz(        pCtx, pString, COMMENT_FILE__BOILERPLATE)  );
	SG_ERR_CHECK(  SG_string__append__format(    pCtx, pString, COMMENT_FILE__HDR_USER, pszUserName)  );
	if (bForCommit)
	{
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pString, COMMENT_FILE__HDR_BRANCH,
												 ((pszBranchName) ? pszBranchName : "*None*"))  );

		// TODO 2011/11/02 Think about listing the parent(s) here too.

		SG_ERR_CHECK(  _sg_cmd_util__append_associations(pCtx, pString, pvaDescsAssociations)  );
		SG_ERR_CHECK(  _sg_cmd_util__append_status      (pCtx, pString, pvaStatusCanonical)  );
	}

	/* Create temp file, write boilerplate to it. */
	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pTempFile)  );

	/* It's not clear whether writing a UTF8 signature is a good idea or not. It's mostly a Windows
	   convention. For now, we don't. */
	/* SG_ERR_CHECK(  SG_file__write(pCtx, pTempFile->pFile, sizeof(UTF8_BOM), UTF8_BOM, NULL)  ); */

	SG_ERR_CHECK(  SG_file__write__string(pCtx, pTempFile->pFile, pString)  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, pTempFile)  );

	/* Escape temp file's name for use as shell arg */
	SG_ERR_CHECK(  SG_exec__escape_arg(pCtx, SG_pathname__sz(pTempFile->pPathname), &pstrEscapedTempFile)  );
	/* Note that pszEditor is purposefully NOT escaped, because it might be more than
	   a simple executable path.  Users might have it configured to include flags
	   and other such things.  We're relying on them to have escaped it as needed. */

	/* Open temp file with editor */
	SG_ERR_CHECK(  _get_editor(pCtx, &pszEditor)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrFullCmdLine, pszEditor)  );
	SG_ERR_CHECK(  SG_string__append__format(pCtx, pstrFullCmdLine, " %s", SG_string__sz(pstrEscapedTempFile))  );
	SG_ERR_CHECK(  SG_exec__system(pCtx, SG_string__sz(pstrFullCmdLine), &editorExitStatus)  );
	
	/* Read from file. */
	if (!editorExitStatus)
		SG_ERR_CHECK(  _get_comment_from_file(pCtx, pTempFile->pPathname, &pszComment)  );

	*ppszComment = pszComment;
	pszComment = NULL;

	/* Fall through to common cleanup */
fail:
	SG_NULLFREE(pCtx, pszComment);
	SG_ERR_IGNORE(  SG_tempfile__close_and_delete(pCtx, &pTempFile)  );
	SG_STRING_NULLFREE(pCtx, pstrFullCmdLine);
	SG_STRING_NULLFREE(pCtx, pstrEscapedTempFile);
	SG_NULLFREE(pCtx, pszEditor);
	SG_STRING_NULLFREE(pCtx, pString);
}


static void _get_editor(
	SG_context* pCtx,
	char** ppszEditor)
{
	char* psz = NULL;
	SG_string* pstr = NULL;
	SG_uint32 len = 0;

	/* Check config setting first. */
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, EDITOR_CONFIG_PATH, NULL, &psz, NULL)  );
	if (psz)
	{
		*ppszEditor = psz;
		return;
	}

	/* Check environment variables in order of preference. */
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "VVEDITOR", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "HGEDITOR", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "GIT_EDITOR", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "BZR_EDITOR", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "SVN_EDITOR", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "VISUAL", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "EDITOR", &pstr, &len)  );
	if (len)
	{
		SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstr, (SG_byte**)ppszEditor, NULL)  );
		return;
	}

	/* No relevant settings or environment variables set. Return the platform default. */
	SG_ERR_CHECK(  SG_STRDUP(pCtx, DEFAULT_EDITOR, ppszEditor)  );

	return;

fail:
	SG_NULLFREE(pCtx, psz);
	SG_STRING_NULLFREE(pCtx, pstr);
}
