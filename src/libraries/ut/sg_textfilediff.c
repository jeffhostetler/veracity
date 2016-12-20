/**
 * @copyright
 * 
 * Copyright (C) 2003, 2010-2013 SourceGear LLC. All rights reserved.
 * This file contains file-diffing-related source from subversion's
 * libsvn_delta, forked at version 0.20.0, which contained the
 * following copyright notice:
 *
 *      Copyright (c) 2000-2003 CollabNet.  All rights reserved.
 *      
 *      This software is licensed as described in the file COPYING, which
 *      you should have received as part of this distribution.  The terms
 *      are also available at http://subversion.tigris.org/license-1.html.
 *      If newer versions of this license are posted there, you may use a
 *      newer version instead, at your option.
 *      
 *      This software consists of voluntary contributions made by many
 *      individuals.  For exact contribution history, see the revision
 *      history and logs, available at http://subversion.tigris.org/.
 * 
 * @endcopyright
 * 
 * @file sg_textfilediff.c
 * 
 * @details
 * 
 */

#include <sg.h>

///////////////////////////////////////////////////////////////////////////////


#define SG_HAS_SET(bits_buffer, which_bits) ((bits_buffer&which_bits)==which_bits)

static SG_bool my_isspace(char b)
{
    // Full list of unicode whitespace characters is at the top of this page:
    // http://unicode.org/Public/UNIDATA/PropList.txt

    // Our ignore whitespace setting is really just ignoring regular space
    // and regular tab. (Note, line breaks are n/a for us since we're only
    // looking at one line--not including the linebreaks--at a time.)

    // This is easier because we can just look at one byte at a time.

    return (b==' ' || b=='\t');
}


static SG_int32 my_memcmp(const char * p1, const char * p2, SG_uint32 numBytes)
{
    return memcmp(p1, p2, numBytes);
}
static SG_int32 my_memicmp(const char * p1, const char * p2, SG_uint32 numBytes)
{
    return SG_memicmp((const SG_byte *)p1, (const SG_byte *)p2, numBytes);
}
static const char * my_memchr2_byte(const char * pBuf, const char * pattern, SG_uint32 bufSize)
{
    // do a 1- or 2- byte memchr() (like strstr() but with an upperbound)
    const char * pEnd = pBuf+bufSize;
    while(bufSize>0)
    {
        char * p = memchr(pBuf,pattern[0],bufSize);
        if(p==NULL)
            return p;
        if(pattern[1]=='\0') // 1 byte pattern, just use memchr()
            return p;
        if(p[1]==pattern[1]) // 2 byte pattern, and a match
            return p;

        pBuf = p+1;
        bufSize = (SG_uint32)(pEnd - pBuf);
    }
    return NULL;
}
static const char * my_anyeol_byte(const char * pBuf, SG_uint32 bufSize)
{
    // find the first EOL marker using either CR, LF, or CRLF
    while(*pBuf!=SG_CR && *pBuf!=SG_LF && bufSize>0)
    {
        ++pBuf;
        --bufSize;
    }
    if(bufSize==0)
        return NULL;
    else
        return pBuf;
}


///////////////////////////////////////////////////////////////////////////////


// A line of a text in a text file. This is a "token" wrt the diff/diff3 algorithm.
typedef struct __sg_textfile_line _sg_textfile_line;
struct __sg_textfile_line
{
    SG_int32 length;
    const char * pBuf;

    _sg_textfile_line * pNext;
};

void _sg_textfile_line__nullfree(_sg_textfile_line ** ppLine)
{
    _sg_textfile_line * pLine = NULL;

    if(ppLine==NULL||*ppLine==NULL)
        return;

    pLine = *ppLine;
    while(pLine!=NULL)
    {
        _sg_textfile_line * pNext = pLine->pNext;
        SG_free__no_ctx(pLine);
        pLine = pNext;
    }
    *ppLine = NULL;
}

enum __sg_eol_t
{
	_SG_EOL__NONE = 0,
	_SG_EOL__LF   = 1,
	_SG_EOL__CRLF = 2,
	_SG_EOL__CR   = 3,
	
	_SG_EOL_T__COUNT = 4
};
typedef enum __sg_eol_t _sg_eol_t;

struct __sg_textfile_info
{
	const SG_pathname * pPath;      //< Path to the file on disk. We do NOT own this pointer.
	char * pContent;                //< Contents of the file imported to UTF-8. We DO own this pointer.
	SG_encoding encoding;           //< Information about original encoding the file was in, including a SG_utf8_converter that WE OWN.
	_sg_textfile_line * pFirstLine; //< First in a linked list of lines in the file. We DO own this pointer.
	SG_uint32 eolCount[_SG_EOL_T__COUNT]; //< Keep track of eols. If mixed EOLs, highest count wins.
};
typedef struct __sg_textfile_info _sg_textfile_info;

static _sg_eol_t _sg_textfile_info__eol_style(const _sg_textfile_info * pFileinfo)
{
	_sg_eol_t max = 0;
	_sg_eol_t i = 1;
	for(i=1; i<_SG_EOL_T__COUNT; ++i)
		if(pFileinfo->eolCount[i]>pFileinfo->eolCount[max])
			max = i;
	return max;
}


///////////////////////////////////////////////////////////////////////////////


struct __sg_textfilediff_baton_fileinfo
{
	_sg_textfile_info * pBase;
	_sg_textfile_line * pLastLine;
	const char * pCur;
	const char * pEnd;
};
typedef struct __sg_textfilediff_baton_fileinfo _sg_textfilediff_baton_fileinfo;

struct __sg_textfilediff_baton
{
	_sg_textfilediff_baton_fileinfo fileinfo[3];
	SG_textfilediff_options options;
	const char * szEol;
	SG_int32 eolLen;
	SG_int32 (*m_fncmp)(const char * buf1, const char * buf2, SG_uint32 nrChars);
};
typedef struct __sg_textfilediff_baton _sg_textfilediff_baton;

void _sg_textfilediff__init_baton(
	_sg_textfilediff_baton * pBaton,
	SG_textfilediff_options options,
	_sg_textfile_info * pFileinfo0,
	_sg_textfile_info * pFileinfo1,
	_sg_textfile_info * pFileinfo2
	)
{
	SG_zero(*pBaton);
	pBaton->fileinfo[0].pBase = pFileinfo0;
	pBaton->fileinfo[1].pBase = pFileinfo1;
	pBaton->fileinfo[2].pBase = pFileinfo2;
	pBaton->options = options;

	if(SG_HAS_SET(options, SG_TEXTFILEDIFF_OPTION__NATIVE_EOL))
	{
		pBaton->szEol = SG_PLATFORM_NATIVE_EOL_STR;
		pBaton->eolLen = SG_STRLEN(SG_PLATFORM_NATIVE_EOL_STR);
	}
	else if(SG_HAS_SET(options, SG_TEXTFILEDIFF_OPTION__LF_EOL))
	{
		pBaton->szEol = SG_LF_STR;
		pBaton->eolLen = 1;
	}
	else if(SG_HAS_SET(options, SG_TEXTFILEDIFF_OPTION__CRLF_EOL))
	{
		pBaton->szEol = SG_CRLF_STR;
		pBaton->eolLen = 2;
	}
	else if(SG_HAS_SET(options, SG_TEXTFILEDIFF_OPTION__CR_EOL))
	{
		pBaton->szEol = SG_CR_STR;
		pBaton->eolLen = 1;
	}

	if(SG_HAS_SET(options, SG_TEXTFILEDIFF_OPTION__IGNORE_CASE))
		pBaton->m_fncmp = my_memicmp;
	else
		pBaton->m_fncmp = my_memcmp;
}


///////////////////////////////////////////////////////////////////////////////


static void _sg_textfilediff__file_datasource_open(SG_context * pCtx, void * pDiffBaton, SG_filediff_datasource datasource)
{
	// Read raw file and convert from whatever encoding into utf-8.
	_sg_textfilediff_baton * pFileDiffBaton = (_sg_textfilediff_baton *)pDiffBaton;
	_sg_textfilediff_baton_fileinfo * pFileinfo = &pFileDiffBaton->fileinfo[datasource];
	SG_fsobj_stat finfo;
	SG_file * pFile = NULL;
	SG_byte * pFileContents = NULL;

	SG_ASSERT(pCtx!=NULL);

	if(pFileinfo->pBase->pPath==NULL)
		return;

	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pFileinfo->pBase->pPath, &finfo)  );
	if(finfo.size!=0)
	{
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pFileinfo->pBase->pPath, SG_FILE_OPEN_EXISTING|SG_FILE_RDONLY, SG_FSOBJ_PERMS__UNUSED, &pFile)  );

		if(finfo.size>SG_UINT32_MAX)
			SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "File '%s' is too large to diff.", SG_pathname__sz(pFileinfo->pBase->pPath)));
		SG_ERR_CHECK(  SG_allocN(pCtx, (SG_uint32)finfo.size, pFileContents)  );
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile,(SG_uint32)finfo.size, pFileContents, NULL)  );
		SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

#if 0
		{
			SG_uint32 k;
			SG_uint32 len = (SG_uint32)finfo.size;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "TextFileDiff:Open: %s [len %d]\n",
									   SG_pathname__sz(pFileinfo->pBase->pPath), len)  );
			for (k=0; k<len; k++)
			{
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, " %02x", pFileContents[k])  );
				if (k % 32 == 31)
					SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\n")  );
			}
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\n")  );
		}
#endif

		SG_ERR_CHECK(  SG_utf8__import_buffer(
			pCtx,
			pFileContents,
			(SG_uint32)finfo.size,
			&pFileinfo->pBase->pContent,
			&pFileinfo->pBase->encoding)  );
		pFileinfo->pCur = pFileinfo->pBase->pContent;
		pFileinfo->pEnd = pFileinfo->pBase->pContent + SG_STRLEN(pFileinfo->pBase->pContent);

#if 0
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TextFileDiff:Open: Encoding computed %s\n",
									   ((pFileinfo->pBase->encoding.szName
										 && *pFileinfo->pBase->encoding.szName)
										? pFileinfo->pBase->encoding.szName
										: "(*null*)"))  );
		}
#endif

		/* SG_utf8__import_buffer should always strip the signature. */
		SG_ASSERT(pFileinfo->pCur[0]!='\xEF' && pFileinfo->pCur[1]!='\xBB' && pFileinfo->pCur[2]!='\xBF');

		SG_NULLFREE(pCtx, pFileContents);
	}

	return;
fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, pFileContents);
}
static void _sg_textfilediff__file_datasource_close(SG_context * pCtx, void *pDiffBaton, SG_filediff_datasource datasource)
{
    SG_UNUSED(pCtx);
    SG_UNUSED(pDiffBaton);
    SG_UNUSED(datasource);
}
static void _sg_textfilediff__file_datasource_get_next_token(SG_context * pCtx, void *pDiffBaton, SG_filediff_datasource datasource, void **ppToken)
{
	_sg_textfilediff_baton * pFileDiffBaton = (_sg_textfilediff_baton *)pDiffBaton;
	_sg_textfilediff_baton_fileinfo * pFileinfo = &pFileDiffBaton->fileinfo[datasource];
	_sg_textfile_line * pLine = NULL;
	const char * pEnd = NULL;
	const char *pCur = NULL;
	const char * pEol = NULL;
	SG_uint32 eolLen;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppToken);

	pCur = pFileinfo->pCur;
	pEnd = pFileinfo->pEnd;

	if(pCur==pEnd)
	{
		*ppToken = NULL;
		return;
	}

	SG_ERR_CHECK(  SG_alloc1(pCtx, pLine)  );

	if(pFileinfo->pLastLine!=NULL)
	{
		pFileinfo->pLastLine->pNext = pLine;
	}
	else
	{
		SG_ASSERT(pFileinfo->pBase->pFirstLine==NULL);
		pFileinfo->pBase->pFirstLine = pLine;
	}
	pFileinfo->pLastLine = pLine;

	pLine->length = 0;

	if(pFileDiffBaton->szEol==NULL)
		pEol = my_anyeol_byte(pCur,(SG_uint32)(pEnd-pCur));
	else
		pEol = my_memchr2_byte(pCur, pFileDiffBaton->szEol, (SG_uint32)(pEnd-pCur));

	if(pEol==NULL)
	{
		++pFileinfo->pBase->eolCount[_SG_EOL__NONE];
		pEol = pEnd;
		eolLen = 0;
	}
	else if(pFileDiffBaton->szEol==NULL)
	{
		if(pEol[0]==SG_LF)
		{
			++pFileinfo->pBase->eolCount[_SG_EOL__LF];
			eolLen = 1;
		}
		else if(pEol[1]==SG_LF)
		{
			++pFileinfo->pBase->eolCount[_SG_EOL__CRLF];
			eolLen = 2;
		}
		else
		{
			++pFileinfo->pBase->eolCount[_SG_EOL__CR];
			eolLen = 1;
		}
	}
	else
	{
		eolLen = pFileDiffBaton->eolLen;
	}

	pLine->pBuf = pCur;
	pLine->length = (SG_uint32)(pEol - pCur);
	if(SG_HAS_SET(pFileDiffBaton->options, SG_TEXTFILEDIFF_OPTION__STRICT_EOL))
		pLine->length += eolLen;// include the EOL in the line

	pFileinfo->pCur = pEol + eolLen;
	*ppToken = pLine;

	return;
fail:
	;
}


///////////////////////////////////////////////////////////////////////////////


SG_int32 _sg_textfilediff__line_compare(void * pDiffBaton, void * pToken1, void * pToken2)
{
    _sg_textfilediff_baton * pFileDiffBaton = (_sg_textfilediff_baton *)pDiffBaton;
    _sg_textfile_line * pLine1 = (_sg_textfile_line *)pToken1;
    _sg_textfile_line * pLine2 = (_sg_textfile_line *)pToken2;
 
    if (pLine1->length < pLine2->length)
        return -1;
    else if (pLine1->length > pLine2->length)
        return 1;
	else
    	return (*pFileDiffBaton->m_fncmp)(pLine1->pBuf, pLine2->pBuf, pLine1->length);
}
int _sg_textfilediff__line_ignorewhitespace_compare(void * pDiffBaton, void * pToken1, void * pToken2)
{
    _sg_textfilediff_baton * pFileDiffBaton = (_sg_textfilediff_baton *)pDiffBaton;
    _sg_textfile_line * pLine1 = (_sg_textfile_line *)pToken1;
    _sg_textfile_line * pLine2 = (_sg_textfile_line *)pToken2;

    const char * p1;
    const char * p2;
    const char * p1end;
    const char * p2end;

    // quick test if exactly the same.
    // See "WARNING C4244: __int64 to uint conversion" in libsgdcore_private.h
    if((pLine1->length==pLine2->length) && ((*pFileDiffBaton->m_fncmp)(pLine1->pBuf, pLine2->pBuf, pLine1->length) == 0))
        return 0;

    p1      = pLine1->pBuf;
    p2      = pLine2->pBuf;
    p1end   = pLine1->pBuf + pLine1->length;
    p2end   = pLine2->pBuf + pLine2->length;

    // trim trailing whitespace, since we probably don't get the LF
    // in the buffer (fgets semantics) and we want trailing spaces,
    // TABs, and CR to sync up with the LF.
    while( (p1end>p1) && my_isspace(p1end[-1]) )
        p1end--;
    while( (p2end>p2) && my_isspace(p2end[-1]) )
        p2end--;

    while ( (p1<p1end) && (p2<p2end) )
    {
        if (my_isspace(*p1) && my_isspace(*p2))
        {
            // both are whitespace.  collapse multiple to one, effectively.
            do{ p1++; } while( (p1<p1end) && my_isspace(*p1) );
            do{ p2++; } while( (p2<p2end) && my_isspace(*p2) );
        }
        else
        {
            // otherwise, one or both not whitespace, compare.
            SG_int32 r;
            r = (*pFileDiffBaton->m_fncmp)(p1,p2,1); // compare this character
            if(r!=0)
                return r; // if different, we're done.

            // otherwise, we had a match.  keep looking.
            p1++;
            p2++;
        }
    }

    // we reached the end of one or both.
    if(p1==p1end)
        if(p2==p2end)
            return 0; // end of both, so effectively equal.
        else
            return -1; // buf1 shorter
    else
        return 1; // buf2 shorter
}
static const SG_filediff_vtable _sg_textfilediff__file_vtable = {
    _sg_textfilediff__file_datasource_open,
    _sg_textfilediff__file_datasource_close,
    _sg_textfilediff__file_datasource_get_next_token,
    _sg_textfilediff__line_compare,
    NULL,
    NULL
};
static const SG_filediff_vtable _sg_textfilediff__file_ignorewhitespace_vtable = {
    _sg_textfilediff__file_datasource_open,
    _sg_textfilediff__file_datasource_close,
    _sg_textfilediff__file_datasource_get_next_token,
    _sg_textfilediff__line_ignorewhitespace_compare,
    NULL,
    NULL
};
const SG_filediff_vtable * _sg_textfilediff__selectDiffFnsVTable(SG_textfilediff_options eOptions)
{
	if (SG_HAS_SET(eOptions,SG_TEXTFILEDIFF_OPTION__IGNORE_WHITESPACE))
		return &_sg_textfilediff__file_ignorewhitespace_vtable;
	else
		return &_sg_textfilediff__file_vtable;
}


///////////////////////////////////////////////////////////////////////////////


struct __sg_textfilediff_output_baton
{
	struct __sg_textfilediff_output_baton_fileinfo
	{
		_sg_textfile_line * pNextLine;
		SG_int32 nextLineNumber;
		SG_int32 hunkStart;
	} fileinfo[2];
	SG_string * pOutput;
	SG_string * pHunk;
	SG_textfilediff_options options;
	SG_int32 context; // number of lines of context, when appropriate
};
typedef struct __sg_textfilediff_output_baton _sg_textfilediff_output_baton_t;


enum __sg_textfilediff_unified_output_block_type
{
    SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__SKIP,
    SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__CONTEXT,
    SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__DELETE,
    SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__INSERT
};
typedef enum __sg_textfilediff_unified_output_block_type _sg_textfilediff_unified_output_block_type;


static void _sg_textfilediff__output_unified_line(SG_context * pCtx, _sg_textfilediff_output_baton_t *pBaton, _sg_textfilediff_unified_output_block_type type)
{
	// Initializing this to LF means that we will NOT print the "\ No newline at end of file"
	// message at the end, UNLESS one of the switch cases sets it to something else.
	// Now note that when the strict-eol bit is set, ONLY the last line of the file can EVER
	// end with something other than CR or LF. So that's all we need to check.
	char last_char = SG_LF; 
	
	switch (type)
	{
		case SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__CONTEXT:
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, " ")  );
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pHunk, (const SG_byte*)pBaton->fileinfo[0].pNextLine->pBuf, pBaton->fileinfo[0].pNextLine->length)  );
			if(!SG_HAS_SET(pBaton->options, SG_TEXTFILEDIFF_OPTION__STRICT_EOL))
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, SG_PLATFORM_NATIVE_EOL_STR)  );
			else
			{
				SG_ASSERT(pBaton->fileinfo[0].pNextLine->length>=1);
				last_char = pBaton->fileinfo[0].pNextLine->pBuf[pBaton->fileinfo[0].pNextLine->length-1];
			}
			pBaton->fileinfo[0].pNextLine = pBaton->fileinfo[0].pNextLine->pNext;
			pBaton->fileinfo[0].nextLineNumber++;
			pBaton->fileinfo[1].pNextLine = pBaton->fileinfo[1].pNextLine->pNext;
			pBaton->fileinfo[1].nextLineNumber++;
			break;
		case SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__DELETE:
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, "-")  );
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pHunk, (const SG_byte*)pBaton->fileinfo[0].pNextLine->pBuf, pBaton->fileinfo[0].pNextLine->length)  );
			if(!SG_HAS_SET(pBaton->options, SG_TEXTFILEDIFF_OPTION__STRICT_EOL))
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, SG_PLATFORM_NATIVE_EOL_STR)  );
			else
			{
				SG_ASSERT(pBaton->fileinfo[0].pNextLine->length>=1);
				last_char = pBaton->fileinfo[0].pNextLine->pBuf[pBaton->fileinfo[0].pNextLine->length-1];
			}
			pBaton->fileinfo[0].pNextLine = pBaton->fileinfo[0].pNextLine->pNext;
			pBaton->fileinfo[0].nextLineNumber++;
			break;
		case SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__INSERT:
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, "+")  );
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pHunk, (const SG_byte*)pBaton->fileinfo[1].pNextLine->pBuf, pBaton->fileinfo[1].pNextLine->length)  );
			if(!SG_HAS_SET(pBaton->options, SG_TEXTFILEDIFF_OPTION__STRICT_EOL))
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, SG_PLATFORM_NATIVE_EOL_STR)  );
			else
			{
				SG_ASSERT(pBaton->fileinfo[1].pNextLine->length>=1);
				last_char = pBaton->fileinfo[1].pNextLine->pBuf[pBaton->fileinfo[1].pNextLine->length-1];
			}
			pBaton->fileinfo[1].pNextLine = pBaton->fileinfo[1].pNextLine->pNext;
			pBaton->fileinfo[1].nextLineNumber++;
			break;
		case SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__SKIP:
			pBaton->fileinfo[0].pNextLine = pBaton->fileinfo[0].pNextLine->pNext;
			pBaton->fileinfo[0].nextLineNumber++;
			pBaton->fileinfo[1].pNextLine = pBaton->fileinfo[1].pNextLine->pNext;
			pBaton->fileinfo[1].nextLineNumber++;
			break;
		default:
			break;
	}

	if(!(last_char==SG_LF || last_char==SG_CR))
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pHunk, SG_PLATFORM_NATIVE_EOL_STR "\\ No newline at end of file" SG_PLATFORM_NATIVE_EOL_STR)  );

    return;
fail:
    ;
}

static void _sg_textfilediff__output_unified_flush_hunk(SG_context * pCtx, _sg_textfilediff_output_baton_t *pBaton)
{
	SG_int32 i;
	SG_int32 hunkLength[2];
	
	if (SG_string__length_in_bytes(pBaton->pHunk)==0)
		return; // Nothing to flush

	// Add trailing context to the hunk
	for(i=0; i<pBaton->context; ++i)
		if(pBaton->fileinfo[0].pNextLine!=NULL)
			SG_ERR_CHECK(  _sg_textfilediff__output_unified_line(pCtx, pBaton, SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__CONTEXT));

	// If the file is non-empty, convert the line indexes from zero based to one based
	for (i = 0; i < 2; i++)
	{
		hunkLength[i] = pBaton->fileinfo[i].nextLineNumber - pBaton->fileinfo[i].hunkStart;
		if (hunkLength[i] > 0)
			pBaton->fileinfo[i].hunkStart++;
	}

	// Output the hunk header.  If the hunk length is 1, the file is a one line
	// file.  In this case, surpress the number of lines in the hunk (it is
	// 1 implicitly)
	SG_ERR_CHECK(  SG_string__append__format(pCtx, pBaton->pOutput, "@@ -%d", pBaton->fileinfo[0].hunkStart)  );
	if (hunkLength[0] != 1)
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pBaton->pOutput, ",%d", hunkLength[0])  );

	SG_ERR_CHECK(  SG_string__append__format(pCtx, pBaton->pOutput, " +%d", pBaton->fileinfo[1].hunkStart)  );
	if (hunkLength[1] != 1)
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pBaton->pOutput, ",%d", hunkLength[1])  );

	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, " @@")  );

	if(SG_HAS_SET(pBaton->options, SG_TEXTFILEDIFF_OPTION__LF_EOL))
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, SG_LF_STR)  );
	else
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, SG_PLATFORM_NATIVE_EOL_STR)  );

	// Output the hunk content
	SG_ERR_CHECK(  SG_string__append__string(pCtx, pBaton->pOutput, pBaton->pHunk)  );

	// Prepare for the next hunk
	SG_ERR_CHECK(  SG_string__clear(pCtx, pBaton->pHunk)  );

	return;
fail:
	;
}

static void _sg_textfilediff__output_unified_default_hdr(SG_context * pCtx, const SG_pathname * pPath, SG_string ** ppHeader)
{
    SG_fsobj_stat file_info;
    SG_time time;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(ppHeader);

    if(pPath!=NULL)
    {
        SG_ERR_CHECK_RETURN(  SG_fsobj__stat__pathname(pCtx, pPath, &file_info)  );
        SG_ERR_CHECK_RETURN(  SG_time__decode__local(pCtx, file_info.mtime_ms, &time)  );

        SG_ERR_CHECK_RETURN(  SG_string__alloc__format(pCtx, ppHeader, "%s\t%d-%02d-%02d %02d:%02d:%02d %+03d%02d",
            SG_pathname__sz(pPath),
            time.year, time.month, time.mday,
            time.hour, time.min, time.sec,
            time.offsetUTC/3600,abs(time.offsetUTC)/60%60)  );
    }
    else
    {
        SG_ERR_CHECK_RETURN(  SG_string__alloc__sz(pCtx, ppHeader, "")  );
    }
}

static void _sg_textfilediff__output_unified_diff_modified(
	SG_context * pCtx,
	void *pOutputBaton,
	SG_int32 originalStart, SG_int32 originalLength,
	SG_int32 modifiedStart, SG_int32 modifiedLength,
	SG_int32 latestStart, SG_int32 latestLength)
{
	_sg_textfilediff_output_baton_t * pBaton = pOutputBaton;
	SG_int32 leadingContext = pBaton->context;
	SG_int32 i;

	SG_UNUSED(latestStart);
	SG_UNUSED(latestLength);

	if (SG_string__length_in_bytes(pBaton->pHunk)!=0)
		leadingContext *= 2;

	// If the changed ranges are far enough apart (no overlapping or connecting
	// context), flush the current hunk, initialize the next hunk and skip the
	// lines not in context.  Also do this when this is the first hunk.
	if (pBaton->fileinfo[0].nextLineNumber+leadingContext < originalStart)
	{
		SG_ERR_CHECK(  _sg_textfilediff__output_unified_flush_hunk(pCtx, pBaton));

		if(originalStart > pBaton->context)
			pBaton->fileinfo[0].hunkStart = originalStart - pBaton->context;
		else
			pBaton->fileinfo[0].hunkStart = 0;
		
		if(modifiedStart > pBaton->context)
			pBaton->fileinfo[1].hunkStart = modifiedStart - pBaton->context;
		else
			pBaton->fileinfo[1].hunkStart = 0;

		// Skip lines until we are at the beginning of the context we want to display
		while (pBaton->fileinfo[0].nextLineNumber < pBaton->fileinfo[0].hunkStart)
			SG_ERR_CHECK(  _sg_textfilediff__output_unified_line(pCtx, pBaton, SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__SKIP));
	}

	// Output the context preceding the changed range
	while (pBaton->fileinfo[0].nextLineNumber < originalStart)
		SG_ERR_CHECK(  _sg_textfilediff__output_unified_line(pCtx, pBaton, SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__CONTEXT));

	for(i=0; i<originalLength; ++i)
		SG_ERR_CHECK(  _sg_textfilediff__output_unified_line(pCtx, pBaton, SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__DELETE));
	for(i=0; i<modifiedLength; ++i)
		SG_ERR_CHECK(  _sg_textfilediff__output_unified_line(pCtx, pBaton, SG_TEXTFILEDIFF_UNIFIED_OUTPUT_BOCK_TYPE__INSERT));

	return;
fail:
	;
}


struct __sg_textfilediff3_output_baton_t
{
	_sg_textfile_line * nextLine[3];
	SG_string * pOutput;

	//const char * szLabelOriginal;
	const char * szLabelModified;
	const char * szLabelLatest;

	//SG_bool displayOriginalInConflict;
	//SG_bool displayResolvedConflicts;

	SG_string * encoding[3];

	SG_bool firstLine;
	const char * szNewline;
};
typedef struct __sg_textfilediff3_output_baton_t _sg_textfilediff3_output_baton_t;

static void _sg_textfilediff3__output_diff_modified(SG_context * pCtx, void * pOutputBaton, SG_int32 originalStart, SG_int32 originalLength, SG_int32 modifiedStart, SG_int32 modifiedLength, SG_int32 latestStart, SG_int32 latestLength)
{
	_sg_textfilediff3_output_baton_t * pBaton = (_sg_textfilediff3_output_baton_t*)pOutputBaton;
	SG_int32 i;
	
	SG_UNUSED(originalStart);
	SG_UNUSED(modifiedStart);
	SG_UNUSED(latestStart);

	for(i=0; i<originalLength; ++i)
		pBaton->nextLine[0] = pBaton->nextLine[0]->pNext;

	for(i=0; i<modifiedLength; ++i)
	{
		if(pBaton->szNewline!=NULL)
		{
			if(!pBaton->firstLine)
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
			else
				pBaton->firstLine = SG_FALSE;
		}

		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pOutput, (const SG_byte*)pBaton->nextLine[1]->pBuf, pBaton->nextLine[1]->length)  );
		pBaton->nextLine[1] = pBaton->nextLine[1]->pNext;
	}

	for(i=0; i<latestLength; ++i)
		pBaton->nextLine[2] = pBaton->nextLine[2]->pNext;

	return;
fail:
	;
}

static void _sg_textfilediff3__output_diff_latest(SG_context * pCtx, void * pOutputBaton, SG_int32 originalStart, SG_int32 originalLength, SG_int32 modifiedStart, SG_int32 modifiedLength, SG_int32 latestStart, SG_int32 latestLength)
{
	_sg_textfilediff3_output_baton_t * pBaton = (_sg_textfilediff3_output_baton_t*)pOutputBaton;
	SG_int32 i;
	
	SG_UNUSED(originalStart);
	SG_UNUSED(modifiedStart);
	SG_UNUSED(latestStart);

	SG_UNUSED(modifiedLength);

	for(i=0; i<originalLength; ++i)
	{
		pBaton->nextLine[0] = pBaton->nextLine[0]->pNext;
	}

	for(i=0; i<modifiedLength; ++i)
		pBaton->nextLine[1] = pBaton->nextLine[1]->pNext;

	for(i=0; i<latestLength; ++i)
	{
		if(pBaton->szNewline!=NULL)
		{
			if(!pBaton->firstLine)
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
			else
				pBaton->firstLine = SG_FALSE;
		}

		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pOutput, (const SG_byte*)pBaton->nextLine[2]->pBuf, pBaton->nextLine[2]->length)  );
		pBaton->nextLine[2] = pBaton->nextLine[2]->pNext;
	}

	return;
fail:
	;
}

static void _sg_textfilediff3__output_conflict(
	SG_context * pCtx,
	void * pOutputBaton,
	SG_int32 originalStart, SG_int32 originalLength,
	SG_int32 modifiedStart, SG_int32 modifiedLength,
	SG_int32 latestStart, SG_int32 latestLength);

const SG_filediff_output_functions _sg_textfilediff3__output_vtable = {
	_sg_textfilediff3__output_diff_modified, // output_common
	_sg_textfilediff3__output_diff_modified,
	_sg_textfilediff3__output_diff_latest,
	_sg_textfilediff3__output_diff_modified, // output_diff_common
	_sg_textfilediff3__output_conflict};

static void _sg_textfilediff3__output_conflict(
	SG_context * pCtx,
	void * pOutputBaton,
	SG_int32 originalStart, SG_int32 originalLength,
	SG_int32 modifiedStart, SG_int32 modifiedLength,
	SG_int32 latestStart, SG_int32 latestLength)
{
	_sg_textfilediff3_output_baton_t * pBaton = pOutputBaton;
	SG_int32 i;

	SG_UNUSED(originalStart);
	SG_UNUSED(modifiedStart);
	SG_UNUSED(latestStart);

	//if(pResolvedDiff!=NULL && pBaton->displayResolvedConflicts)
	//{
	//	SG_ERR_CHECK_RETURN(  SG_filediff__output(pCtx, &_sg_textfilediff3__output_vtable, pOutputBaton, pResolvedDiff)  );
	//	return;
	//}

	if(pBaton->szNewline!=NULL)
	{
		if(!pBaton->firstLine)
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
		else
			pBaton->firstLine = SG_FALSE;
		pBaton->firstLine = SG_FALSE;
	}
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, "<<<<<<< ")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szLabelModified)  );
	if(pBaton->szNewline==NULL)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, SG_PLATFORM_NATIVE_EOL_STR)  );

	for(i=0; i<modifiedLength; ++i)
	{
		if(pBaton->szNewline!=NULL)
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pOutput, (const SG_byte*)pBaton->nextLine[1]->pBuf, pBaton->nextLine[1]->length)  );
		pBaton->nextLine[1] = pBaton->nextLine[1]->pNext;
	}

	//if(pBaton->displayOriginalInConflict)
	//{
	//	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
	//	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pBaton->pOutput, "||||||| ")  );
	//	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pBaton->pOutputFile, pBaton->szLabelOriginal)  );
	//
		for(i=0; i<originalLength; ++i)
		{
	//		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
	//		SG_ERR_CHECK(  SG_file__write(pCtx, pBaton->pOutputFile, pBaton->nextLine[0]->length, (const SG_byte*)pBaton->nextLine[0]->pBuf, NULL)  );
			pBaton->nextLine[0] = pBaton->nextLine[0]->pNext;
		}
	//}

	if(pBaton->szNewline!=NULL)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, "=======")  );
	if(pBaton->szNewline==NULL)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, SG_PLATFORM_NATIVE_EOL_STR)  );

	for(i=0; i<latestLength; ++i)
	{
		if(pBaton->szNewline!=NULL)
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pBaton->pOutput, (const SG_byte*)pBaton->nextLine[2]->pBuf, pBaton->nextLine[2]->length)  );
		pBaton->nextLine[2] = pBaton->nextLine[2]->pNext;
	}

	if(pBaton->szNewline!=NULL)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szNewline)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, ">>>>>>> ")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, pBaton->szLabelLatest)  );
	if(pBaton->szNewline==NULL)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pBaton->pOutput, SG_PLATFORM_NATIVE_EOL_STR)  );

	return;
fail:
	;
}


///////////////////////////////////////////////////////////////////////////////


struct _SG_textfilediff_t
{
	SG_textfilediff_options options;
	_sg_textfile_info fileinfo[4];
	SG_filediff_t * pFilediff;
};

struct _SG_textfilediff_iterator
{
	SG_filediff_t * pIterator;
};

///////////////////////////////////////////////////////////////////////////////


void SG_textfilediff(
	SG_context * pCtx,
	const SG_pathname * pPathnameOriginal, const SG_pathname * pPathnameModified,
	SG_textfilediff_options options,
	SG_textfilediff_t ** ppDiff)
{
	_sg_textfilediff_baton baton;
	const SG_filediff_vtable * pVtable;
	SG_textfilediff_t * pTextfilediff = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppDiff);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTextfilediff)  );
	pTextfilediff->options = options;
	pTextfilediff->fileinfo[0].pPath = pPathnameOriginal;
	pTextfilediff->fileinfo[1].pPath = pPathnameModified;

	_sg_textfilediff__init_baton(&baton, options, &pTextfilediff->fileinfo[0], &pTextfilediff->fileinfo[1], NULL);
	pVtable = _sg_textfilediff__selectDiffFnsVTable(options);

	SG_ERR_CHECK(  SG_filediff(pCtx, pVtable, &baton, &pTextfilediff->pFilediff)  );

	*ppDiff = pTextfilediff;

	return;
fail:
	//todo: free stuff from baton
	SG_TEXTFILEDIFF_NULLFREE(pCtx, pTextfilediff);
}

void SG_textfilediff3(
	SG_context * pCtx,
	const SG_pathname * pPathnameOriginal, const SG_pathname * pPathnameModified, const SG_pathname * pPathnameLatest,
	SG_textfilediff_options options,
	SG_textfilediff_t ** ppDiff,
	SG_bool * pHadConflicts)
{
	_sg_textfilediff_baton baton;
	const SG_filediff_vtable * pVtable;
	SG_textfilediff_t * pTextfilediff = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppDiff);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTextfilediff)  );
	pTextfilediff->options = options;
	pTextfilediff->fileinfo[0].pPath = pPathnameOriginal;
	pTextfilediff->fileinfo[1].pPath = pPathnameModified;
	pTextfilediff->fileinfo[2].pPath = pPathnameLatest;

	_sg_textfilediff__init_baton(&baton, options, &pTextfilediff->fileinfo[0], &pTextfilediff->fileinfo[1], &pTextfilediff->fileinfo[2]);
	pVtable  = _sg_textfilediff__selectDiffFnsVTable(options);
	SG_ERR_CHECK(  SG_filediff3(pCtx, pVtable, &baton, &pTextfilediff->pFilediff)  );

	*ppDiff = pTextfilediff;
	if(pHadConflicts!=NULL)
		*pHadConflicts =  SG_filediff__contains_conflicts(pTextfilediff->pFilediff);

	return;
fail:
	//todo: free stuff from baton
	SG_TEXTFILEDIFF_NULLFREE(pCtx, pTextfilediff);
}

void SG_textfilediff__output_unified__string(
	SG_context * pCtx,
	SG_textfilediff_t * pDiff,
	const SG_string * pHeaderOriginal, const SG_string * pHeaderModified,
	int context,
	SG_string ** ppOutput)
{
	SG_filediff_output_functions output_functions = {
		NULL, // output_common
		_sg_textfilediff__output_unified_diff_modified,
		NULL, // output_diff_latest
		NULL, // output_diff_common
		NULL}; // output_conflict
	_sg_textfilediff_output_baton_t baton;

	int i;
	SG_string * pDefaultHeaderOriginal = NULL;
	SG_string * pDefaultHeaderModified = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppOutput);

	if(SG_filediff__contains_diffs(pDiff->pFilediff))
	{
		SG_zero(baton);

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &baton.pOutput)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &baton.pHunk)  );
		baton.options = pDiff->options;

		baton.context = ((context >= 0) ? context : 3); // if negative, use default

		for (i = 0; i < 2; i++)
		{
			//baton.fileinfo[i].pContent = pDiff->fileinfo[i].pContent;
			baton.fileinfo[i].pNextLine = pDiff->fileinfo[i].pFirstLine;
		}

		if (pHeaderOriginal == NULL)
		{
			SG_ERR_CHECK(  _sg_textfilediff__output_unified_default_hdr(pCtx, pDiff->fileinfo[0].pPath, &pDefaultHeaderOriginal)  );
			pHeaderOriginal = pDefaultHeaderOriginal;
		}

		if (pHeaderModified == NULL)
		{
			SG_ERR_CHECK(  _sg_textfilediff__output_unified_default_hdr(pCtx, pDiff->fileinfo[1].pPath, &pDefaultHeaderModified)  );
			pHeaderModified = pDefaultHeaderModified;
		}

		SG_ERR_CHECK(  SG_string__append__format(pCtx, baton.pOutput,
			"--- %s" SG_PLATFORM_NATIVE_EOL_STR
			"+++ %s" SG_PLATFORM_NATIVE_EOL_STR,
			SG_string__sz(pHeaderOriginal), SG_string__sz(pHeaderModified)) );
		SG_ERR_CHECK(  SG_filediff__output(pCtx, &output_functions, &baton, pDiff->pFilediff) );
		SG_ERR_CHECK(  _sg_textfilediff__output_unified_flush_hunk(pCtx, &baton) );

		SG_STRING_NULLFREE(pCtx, pDefaultHeaderOriginal);
		SG_STRING_NULLFREE(pCtx, pDefaultHeaderModified);
		SG_STRING_NULLFREE(pCtx, baton.pHunk);

		*ppOutput = baton.pOutput;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_STRING__ALLOC(pCtx, ppOutput)  );
	}

	return;
fail:
	SG_STRING_NULLFREE(pCtx, pDefaultHeaderOriginal);
	SG_STRING_NULLFREE(pCtx, pDefaultHeaderModified);
	SG_STRING_NULLFREE(pCtx, baton.pHunk);
	SG_STRING_NULLFREE(pCtx, baton.pOutput);
}

void _sg_textfilediff3__output(
	SG_context * pCtx,
	SG_textfilediff_t * pDiff,
	//const SG_string * pLabelOriginal,
	const SG_string * pLabelModified,
	const SG_string * pLabelLatest,
	//SG_bool displayOriginalInConflict, SG_bool displayResolvedConflicts,
	SG_string ** ppOutput,
	SG_encoding ** ppEncoding)
{
	_sg_textfilediff3_output_baton_t baton;
	int idx;
	_sg_eol_t eol_modified, eol_latest;
	_sg_eol_t eol_result = _SG_EOL__NONE;

	SG_encoding * pEncoding = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppOutput);

	SG_zero(baton);
	SG_ERR_CHECK( SG_STRING__ALLOC(pCtx, &baton.pOutput)  );

	// Figure EOL style of the merge result.
	if(!SG_HAS_SET(pDiff->options, SG_TEXTFILEDIFF_OPTION__STRICT_EOL))
	{
		eol_modified = _sg_textfile_info__eol_style(&pDiff->fileinfo[1]);
		eol_latest   = _sg_textfile_info__eol_style(&pDiff->fileinfo[2]);
		if(eol_modified==eol_latest)
			eol_result = eol_modified;
		else
		{
			_sg_eol_t eol_original;
			eol_original = _sg_textfile_info__eol_style(&pDiff->fileinfo[0]);
			if(eol_original==eol_modified)
				eol_result = eol_latest;
			else if(eol_original==eol_latest)
				eol_result = eol_modified;
		}
		switch(eol_result)
		{
			case _SG_EOL__LF:   baton.szNewline = SG_LF_STR;   break;
			case _SG_EOL__CRLF: baton.szNewline = SG_CRLF_STR; break;
			case _SG_EOL__CR:   baton.szNewline = SG_CR_STR;   break;
			default:            baton.szNewline = SG_PLATFORM_NATIVE_EOL_STR; break;
		}
	}

	// Figure encoding of the merge result
	if(ppEncoding!=NULL)
	{
		// Huge set of 'if's to figure out the correct output encoding.
		if(SG_ENCODINGS_EQUAL(pDiff->fileinfo[1].encoding, pDiff->fileinfo[2].encoding))
			pEncoding = &pDiff->fileinfo[1].encoding;
		else if(pDiff->fileinfo[1].encoding.szName==NULL) // Special case for 7-bit ASCII. Always use the other encoding.
			pEncoding = &pDiff->fileinfo[2].encoding;
		else if(pDiff->fileinfo[2].encoding.szName==NULL) // Special case for 7-bit ASCII. Always use the other encoding.
			pEncoding = &pDiff->fileinfo[1].encoding;
		else if(SG_ENCODINGS_EQUAL(pDiff->fileinfo[0].encoding, pDiff->fileinfo[1].encoding))
			pEncoding = &pDiff->fileinfo[2].encoding;
		else if(SG_ENCODINGS_EQUAL(pDiff->fileinfo[0].encoding, pDiff->fileinfo[2].encoding))
			pEncoding = &pDiff->fileinfo[1].encoding;
		else
		{
			// Encoding diverged. Great.
			
			// Favor UTF-8, no signature.
			if(SG_utf8_converter__utf8(pDiff->fileinfo[0].encoding.pConverter) && !pDiff->fileinfo[0].encoding.signature)
				pEncoding = &pDiff->fileinfo[0].encoding;
			else if(SG_utf8_converter__utf8(pDiff->fileinfo[0].encoding.pConverter) && !pDiff->fileinfo[0].encoding.signature)
				pEncoding = &pDiff->fileinfo[1].encoding;
			else if(SG_utf8_converter__utf8(pDiff->fileinfo[0].encoding.pConverter) && !pDiff->fileinfo[0].encoding.signature)
				pEncoding = &pDiff->fileinfo[2].encoding;
			
			// Then favor anything with a signature (since the encoding will be able to handle any unicode character).
			else if(pDiff->fileinfo[1].encoding.signature)
				pEncoding = &pDiff->fileinfo[1].encoding;
			else if(pDiff->fileinfo[2].encoding.signature)
				pEncoding = &pDiff->fileinfo[2].encoding;
			else if(pDiff->fileinfo[0].encoding.signature)
				pEncoding = &pDiff->fileinfo[0].encoding;
			
			//Final tie breaker? We'll just go with "Modified" version rather than "Original" or "Latest"...
			else
				pEncoding = &pDiff->fileinfo[1].encoding;
		}

		if(pEncoding!=NULL && pEncoding->signature)
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, baton.pOutput, "\xEF\xBB\xBF")  );
	}

//	if(pLabelOriginal!=NULL)
//		baton.szLabelOriginal = SG_string__sz(pLabelOriginal);
//	else if(pDiff->fileinfo[0].pPath!=NULL)
//		baton.szLabelOriginal = SG_pathname__sz(pDiff->fileinfo[0].pPath);
//	else
//		baton.szLabelOriginal = "Original";

	if(pLabelModified!=NULL)
		baton.szLabelModified = SG_string__sz(pLabelModified);
	else if(pDiff->fileinfo[1].pPath!=NULL)
		baton.szLabelModified = SG_pathname__sz(pDiff->fileinfo[1].pPath);
	else
		baton.szLabelModified = "Modified";

	if(pLabelLatest!=NULL)
		baton.szLabelLatest = SG_string__sz(pLabelLatest);
	else if(pDiff->fileinfo[2].pPath!=NULL)
		baton.szLabelLatest = SG_pathname__sz(pDiff->fileinfo[2].pPath);
	else
		baton.szLabelLatest = "Latest";

	//baton.displayOriginalInConflict = displayOriginalInConflict;
	//baton.displayResolvedConflicts = displayResolvedConflicts && !displayOriginalInConflict;

	for(idx=0;idx<3;++idx)
	{
		baton.nextLine[idx] = pDiff->fileinfo[idx].pFirstLine;
	}

	baton.firstLine = SG_TRUE;

	SG_ERR_CHECK(  SG_filediff__output(pCtx, &_sg_textfilediff3__output_vtable, &baton, pDiff->pFilediff)  );

	// Write newline at end of file, if appropriate.
	if((pDiff->fileinfo[1].eolCount[_SG_EOL__NONE]==0) && (pDiff->fileinfo[2].eolCount[_SG_EOL__NONE]==0))
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, baton.pOutput, baton.szNewline)  );
	else if(((pDiff->fileinfo[1].eolCount[_SG_EOL__NONE]==0) ^ (pDiff->fileinfo[2].eolCount[_SG_EOL__NONE]==0)) && !(pDiff->fileinfo[0].eolCount[_SG_EOL__NONE]==0))
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, baton.pOutput, baton.szNewline)  );

	*ppOutput = baton.pOutput;
	if(ppEncoding!=NULL)
		*ppEncoding = pEncoding;

	return;
fail:
	SG_STRING_NULLFREE(pCtx, baton.pOutput);
}

void SG_textfilediff3__output__string(
	SG_context * pCtx,
	SG_textfilediff_t * pDiff,
	//const SG_string * pLabelOriginal,
	const SG_string * pLabelModified,
	const SG_string * pLabelLatest,
	//SG_bool displayOriginalInConflict, SG_bool displayResolvedConflicts,
	SG_string ** ppOutput
	)
{
	SG_ERR_CHECK_RETURN(  _sg_textfilediff3__output(pCtx, pDiff, pLabelModified, pLabelLatest, ppOutput, NULL)  );
}

void SG_textfilediff3__output__file(
	SG_context * pCtx,
	SG_textfilediff_t * pDiff,
	//const SG_string * pLabelOriginal,
	const SG_string * pLabelModified,
	const SG_string * pLabelLatest,
	//SG_bool displayOriginalInConflict, SG_bool displayResolvedConflicts,
	SG_file * pOutputFile)
{
	SG_string * pOutput = NULL;
	SG_encoding * pEncoding = NULL;
	
	SG_byte * pBuf = NULL;
	SG_uint32 bufLen = 0;
	
	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pOutputFile);
	
	SG_ERR_CHECK(  _sg_textfilediff3__output(pCtx, pDiff, pLabelModified, pLabelLatest, &pOutput, &pEncoding)  );
	
	if(pEncoding->pConverter==NULL || SG_utf8_converter__utf8(pEncoding->pConverter))
	{
		// No conversion needed. Just use the string that was output.
		SG_ERR_CHECK(  SG_file__write__string(pCtx, pOutputFile, pOutput)  );
	}
	else
	{
		SG_utf8_converter__to_charset__string__buf_len(pCtx, pEncoding->pConverter, pOutput, &pBuf, &bufLen);
		
		if(SG_context__err_equals(pCtx, SG_ERR_ILLEGAL_CHARSET_CHAR))
		{
			// In some obscure cases converting the result could fail with SG_ERR_ILLEGAL_CHARSET_CHAR.
			// If so, we'll just write the output as UTF-8.
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  SG_file__write__string(pCtx, pOutputFile, pOutput)  );
		}
		else
		{
			SG_ERR_CHECK_CURRENT;
			SG_ERR_CHECK(  SG_file__write(pCtx, pOutputFile, bufLen, pBuf, NULL)  );
			SG_NULLFREE(pCtx, pBuf);
		}
	}
	
	SG_STRING_NULLFREE(pCtx, pOutput);
	
	return;
fail:
	SG_STRING_NULLFREE(pCtx, pOutput);
	SG_NULLFREE(pCtx, pBuf);
}

void SG_textfilediff__free(SG_context * pCtx, SG_textfilediff_t * pDiff)
{
	SG_uint32 i;
	if(pDiff==NULL)
		return;
	for(i=0;i<4;++i)
	{
		SG_NULLFREE(pCtx, pDiff->fileinfo[i].pContent);
		SG_NULLFREE(pCtx, pDiff->fileinfo[i].encoding.szName);
		SG_UTF8_CONVERTER_NULLFREE(pCtx, pDiff->fileinfo[i].encoding.pConverter);
		_sg_textfile_line__nullfree(&pDiff->fileinfo[i].pFirstLine);
	}
	SG_FILEDIFF_NULLFREE(pCtx, pDiff->pFilediff);
	SG_NULLFREE(pCtx, pDiff);
}


/**
 * Begin iterating over the diffs.
 */
void SG_textfilediff__iterator__first(
	SG_context * pCtx,							
	SG_textfilediff_t * pDiff,					
	SG_textfilediff_iterator ** ppDiffIterator, 
	SG_bool * pbOk)
{
	SG_textfilediff_iterator * pDiffIterator = NULL;
	SG_ASSERT(pCtx!=NULL);
	

	SG_NULLARGCHECK_RETURN(ppDiffIterator);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pDiffIterator)  );

	pDiffIterator->pIterator = pDiff->pFilediff;

	SG_RETURN_AND_NULL(pDiffIterator, ppDiffIterator);

	if (pbOk != NULL)
		*pbOk = *ppDiffIterator != NULL;
fail:
	SG_TEXTFILEDIFF_ITERATOR_NULLFREE(pCtx, pDiffIterator);
}

void SG_textfilediff__iterator__next(
	SG_context * pCtx,							
	SG_textfilediff_iterator * pDiffIterator, 
	SG_bool * pbOk)
{
	if (pDiffIterator != NULL && pDiffIterator->pIterator)
	{
		SG_filediff_t * pNextDiff = NULL;
		SG_ERR_CHECK(  SG_filediff__get_next(pCtx, pDiffIterator->pIterator, &pNextDiff)  );
		pDiffIterator->pIterator = pNextDiff;
	}
	
	if (pbOk != NULL)
		*pbOk = pDiffIterator->pIterator != NULL;
fail:
	;
}

void SG_textfilediff__iterator__details(
	SG_context * pCtx,							
	SG_textfilediff_iterator * pDiffIterator, 
	SG_diff_type * pDiffType,
	SG_int32 * pStart1,
	SG_int32 * pStart2,
	SG_int32 * pStart3,
	SG_int32 * pLen1,
	SG_int32 * pLen2,
	SG_int32 * pLen3)
{
	SG_ERR_CHECK(  SG_filediff__get_details(pCtx, pDiffIterator->pIterator, pDiffType, pStart1, pStart2, pStart3, pLen1, pLen2, pLen3)  );
fail:
	;
}

void SG_textfilediff__iterator__free(
	SG_context * pCtx,							 
	SG_textfilediff_iterator * pDiffIterator)
{
	SG_NULLFREE(pCtx, pDiffIterator);
}
