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
#include <io.h>
#endif

static void my_file__close(SG_context * pCtx, void * pUnderlying)
{
    SG_file * pFile = (SG_file*)pUnderlying;
    SG_FILE_NULLCLOSE(pCtx, pFile);
}

static void my_set_mode(
	SG_context* pCtx,
	FILE* pFile,
	SG_bool binary
	)
{
#if defined(WINDOWS)
	if (_setmode( _fileno(pFile), binary == SG_FALSE ? _O_TEXT : _O_BINARY) == -1)
	{
		SG_ERR_THROW(SG_ERR_ERRNO(errno));
	}

fail:
	return;
#else
	SG_UNUSED(pCtx);
	SG_UNUSED(pFile);
	SG_UNUSED(binary);
#endif
}

static void my_stdclose__reset_binary(
	SG_context * pCtx,
	void* pUnderlying
	)
{
	SG_ERR_CHECK_RETURN(  my_set_mode(pCtx, (FILE*)pUnderlying, SG_FALSE)  );
}

static void my_fread(
	SG_context * pCtx,
	void* pUnderlying,
	SG_uint32 iNumBytesWanted,
	SG_byte* pBytes,
	SG_uint32* piNumBytesRetrieved /*optional*/
	)
{
	SG_uint32 uBytes = 0u;
	FILE*     pFile  = (FILE*)pUnderlying;

	if (iNumBytesWanted > 0u)
	{
		uBytes = (SG_uint32)fread(pBytes, sizeof(SG_byte), iNumBytesWanted, pFile);
		if (uBytes == 0u)
		{
			if (ferror(pFile) != 0)
			{
				SG_ERR_THROW(SG_ERR_ERRNO(errno));
			}
			else
			{
				SG_ASSERT(feof(pFile) != 0);
				SG_ERR_THROW(SG_ERR_EOF);
			}
		}
	}

	if (piNumBytesRetrieved != NULL)
	{
		*piNumBytesRetrieved = uBytes;
	}

fail:
	return;
}

static void my_fwrite(
	SG_context * pCtx,
	void* pUnderlying,
	SG_uint32 iNumBytes,
	const SG_byte* pBytes,
	SG_uint32* piNumBytesWritten /*optional*/
	)
{
	SG_uint32 uBytes = (SG_uint32)fwrite(pBytes, 1u, iNumBytes, (FILE*)pUnderlying);
	SG_UNUSED(pCtx);
	if (piNumBytesWritten != NULL)
	{
		*piNumBytesWritten = uBytes;
	}
}

struct _sg_seekreader
{
    void* pUnderlying;
    SG_uint64 iHeaderOffset;
    SG_uint64 iLength;
    SG_stream__func__read* pfn_read;
    SG_stream__func__seek* pfn_seek;
    SG_stream__func__close* pfn_close;
};

struct _sg_readstream
{
    void* pUnderlying;
    SG_stream__func__read* pfn_read;
    SG_stream__func__close* pfn_close;
    SG_uint64 count;
    SG_uint64 limit;
};

struct _sg_writestream
{
    void* pUnderlying;
    SG_stream__func__write* pfn_write;
    SG_stream__func__close* pfn_close;
	SG_uint64 count;
};

void SG_readstream__get_count(
	SG_context * pCtx,
	SG_readstream* pstrm,
	SG_uint64* pi
	)
{
	SG_NULLARGCHECK_RETURN(pstrm);
	SG_NULLARGCHECK_RETURN(pi);

    *pi = pstrm->count;
}

void SG_readstream__alloc(
	SG_context * pCtx,
	void* pUnderlying,
	SG_stream__func__read* pfn_read,
	SG_stream__func__close* pfn_close,
    SG_uint64 limit,
	SG_readstream** ppstrm
	)
{
    SG_readstream* pstrm = NULL;

    SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(SG_readstream), &pstrm)  );
    pstrm->pUnderlying = pUnderlying;
    pstrm->pfn_read = pfn_read;
    pstrm->pfn_close = pfn_close;
    pstrm->limit = limit;

    *ppstrm = pstrm;

    return;

fail:
    SG_NULLFREE(pCtx, pstrm);
}

void SG_readstream__close(
	SG_context * pCtx,
	SG_readstream* pstrm
	)
{
	if (pstrm != NULL)
	{
		if (pstrm->pfn_close)
		{
			SG_ERR_CHECK(  pstrm->pfn_close(pCtx, pstrm->pUnderlying)  );
		}

		SG_NULLFREE(pCtx, pstrm);
	}

    return;

fail:
    return;
}

void SG_readstream__read(
	SG_context * pCtx,
	SG_readstream* pstrm,
	SG_uint32 iNumBytesWanted,
	SG_byte* pBytes,
	SG_uint32* piNumBytesRetrieved /*optional*/
	)
{
    SG_uint32 count = 0;
    SG_uint32 want = iNumBytesWanted;

    if (pstrm->limit)
    {
        if (pstrm->count == pstrm->limit)
        {
			SG_ERR_THROW(SG_ERR_EOF);
        }

        if ((pstrm->count + want) > pstrm->limit)
        {
            want = (SG_uint32) (pstrm->limit - pstrm->count);
        }
    }

    if (want)
    {
        SG_ERR_CHECK(  pstrm->pfn_read(pCtx, pstrm->pUnderlying, want, pBytes, &count)  );
    }

    pstrm->count += count;

    if (piNumBytesRetrieved)
    {
        *piNumBytesRetrieved = count;
    }

    return;

fail:
    return;
}

void SG_readstream__alloc__for_file(
	SG_context * pCtx,
	SG_pathname* pPath,
	SG_readstream** ppstrm
	)
{
    SG_file* pFile = NULL;
    SG_readstream* pstrm = NULL;

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  SG_readstream__alloc(
					   pCtx,
					   pFile,
					   (SG_stream__func__read*) SG_file__read,
					   (SG_stream__func__close*) my_file__close,
                       0,
					   &pstrm)  );

    *ppstrm = pstrm;

    return;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
    SG_NULLFREE(pCtx, pstrm);
}

void SG_readstream__alloc__for_stdin(
	SG_context * pCtx,
	SG_readstream** ppstrm,
	SG_bool binary
	)
{
	SG_stream__func__close* close_func = NULL;
	if (binary != SG_FALSE)
	{
		SG_ERR_CHECK_RETURN(  my_set_mode(pCtx, stdin, SG_TRUE)  );
		close_func = my_stdclose__reset_binary;
	}
	SG_ERR_CHECK(  SG_readstream__alloc(pCtx, (void*)stdin, my_fread, close_func, 0, ppstrm)  );

fail:
	return;
}

void SG_writestream__alloc(
	SG_context * pCtx,
	void* pUnderlying,
	SG_stream__func__write* pfn_write,
	SG_stream__func__close* pfn_close,
	SG_writestream** ppstrm
	)
{
    SG_writestream* pstrm = NULL;

    SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(SG_writestream), &pstrm)  );
    pstrm->pUnderlying = pUnderlying;
    pstrm->pfn_write = pfn_write;
    pstrm->pfn_close = pfn_close;

    *ppstrm = pstrm;

    return;

fail:
    SG_NULLFREE(pCtx, pstrm);
}

void SG_writestream__close(
	SG_context * pCtx,
	SG_writestream* pstrm
	)
{
	if (pstrm != NULL)
	{
		if (pstrm->pfn_close)
		{
			SG_ERR_CHECK(  pstrm->pfn_close(pCtx,pstrm->pUnderlying)  );
		}

		SG_NULLFREE(pCtx, pstrm);
	}

    return;

fail:
	// TODO should this do a SG_NULLFREE(pstrm) ??
    return;
}

void SG_writestream__write(
	SG_context * pCtx,
	SG_writestream* pstrm,
	SG_uint32 iNumBytesWanted,
	SG_byte* pBytes,
	SG_uint32* piNumBytesRetrieved /*optional*/
	)
{
	SG_uint32 iWritten;

    SG_ERR_CHECK_RETURN(  pstrm->pfn_write(pCtx, pstrm->pUnderlying, iNumBytesWanted, pBytes, &iWritten)  );

	pstrm->count += iWritten;

	if (piNumBytesRetrieved)
		*piNumBytesRetrieved = iWritten;
}

void SG_writestream__get_count(
	SG_context * pCtx,
	SG_writestream* pstrm,
	SG_uint64* pi
	)
{
	SG_NULLARGCHECK_RETURN(pstrm);
	SG_NULLARGCHECK_RETURN(pi);

	*pi = pstrm->count;
}

void SG_writestream__alloc__for_file(
	SG_context * pCtx,
	SG_pathname* pPath,
	SG_writestream** ppstrm
	)
{
    SG_file* pFile = NULL;
    SG_writestream* pstrm = NULL;

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY | SG_FILE_CREATE_NEW, 0644, &pFile)  );
    SG_ERR_CHECK(  SG_writestream__alloc(
					   pCtx,
					   pFile,
					   (SG_stream__func__write*) SG_file__write,
					   (SG_stream__func__close*) my_file__close,
					   &pstrm)  );

    *ppstrm = pstrm;

    return;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
    SG_NULLFREE(pCtx, pstrm);
}

void SG_writestream__alloc__for_stdout(
	SG_context * pCtx,
	SG_writestream** ppstrm,
	SG_bool binary
	)
{
	SG_stream__func__close* close_func = NULL;
	if (binary != SG_FALSE)
	{
		SG_ERR_CHECK_RETURN(  my_set_mode(pCtx, stdout, SG_TRUE)  );
		close_func = my_stdclose__reset_binary;
	}
	SG_ERR_CHECK(  SG_writestream__alloc(pCtx, (void*)stdout, my_fwrite, close_func, ppstrm)  );

fail:
	return;
}

void SG_writestream__alloc__for_stderr(
	SG_context * pCtx,
	SG_writestream** ppstrm,
	SG_bool binary
	)
{
	SG_stream__func__close* close_func = NULL;
	if (binary != SG_FALSE)
	{
		SG_ERR_CHECK_RETURN(  my_set_mode(pCtx, stderr, SG_TRUE)  );
		close_func = my_stdclose__reset_binary;
	}
	SG_ERR_CHECK(  SG_writestream__alloc(pCtx, (void*)stderr, my_fwrite, close_func, ppstrm)  );

fail:
	return;
}

void SG_seekreader__alloc(
	SG_context * pCtx,
	void* pUnderlying,
	SG_uint64 iHeaderOffset,
	SG_uint64 iLength,
	SG_stream__func__read* pfn_read,
	SG_stream__func__seek* pfn_seek,
	SG_stream__func__close* pfn_close,
	SG_seekreader** ppsr
	)
{
    SG_seekreader* psr = NULL;

    SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(SG_seekreader), &psr)  );
    psr->pUnderlying = pUnderlying;
    psr->iHeaderOffset = iHeaderOffset;
    psr->iLength = iLength;
    psr->pfn_read = pfn_read;
    psr->pfn_seek = pfn_seek;
    psr->pfn_close = pfn_close;

    *ppsr = psr;

    return;

fail:
    SG_NULLFREE(pCtx, psr);
}

void SG_seekreader__length(
	SG_context * pCtx,
	SG_seekreader* psr,
	SG_uint64* piLength
	)
{
	SG_NULLARGCHECK_RETURN(psr);
	SG_NULLARGCHECK_RETURN(piLength);

    *piLength = psr->iLength;
}

void SG_seekreader__close(
	SG_context * pCtx,
	SG_seekreader* psr
	)
{
	if (!psr)
		return;

    SG_ERR_CHECK(  psr->pfn_close(pCtx, psr->pUnderlying)  );

    SG_NULLFREE(pCtx, psr);

    return;

fail:
	// TODO should this do a SG_NULLFREE(psr) ??
    return;
}

void SG_seekreader__read(
	SG_context * pCtx,
	SG_seekreader* psr,
	SG_uint64 iPos,
	SG_uint32 iNumBytesWanted,
	SG_byte* pBytes,
	SG_uint32* piNumBytesRetrieved /*optional*/
	)
{
    SG_ERR_CHECK(  psr->pfn_seek(pCtx, psr->pUnderlying, iPos + psr->iHeaderOffset)  );

    SG_ERR_CHECK(  psr->pfn_read(pCtx, psr->pUnderlying, iNumBytesWanted, pBytes, piNumBytesRetrieved)  );

    return;

fail:
    return;
}

void SG_seekreader__alloc__for_file(
	SG_context * pCtx,
	SG_pathname* pPath,
	SG_uint64 iHeaderOffset,
	SG_seekreader** ppsr
	)
{
    SG_file* pFile = NULL;
    SG_seekreader* psr = NULL;
    SG_uint64 len = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len, NULL)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  SG_seekreader__alloc(
					   pCtx,
					   pFile,
					   iHeaderOffset,
					   len - iHeaderOffset,
					   (SG_stream__func__read*) SG_file__read,
					   (SG_stream__func__seek*) SG_file__seek,
					   (SG_stream__func__close*) my_file__close,
					   &psr)  );

    *ppsr = psr;

    return;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
    SG_NULLFREE(pCtx, psr);
}

typedef struct buflen
{
    SG_byte* p;
    SG_uint32 len;
    SG_uint32 cur;
} sg_buflen;

static void buflen__seek(
	SG_context * pCtx,
    sg_buflen* pbl,
    SG_uint64 iPos
    )
{
    SG_NULLARGCHECK_RETURN(pbl);

    pbl->cur = (SG_uint32) iPos;
}

static void buflen__close(
	SG_context * pCtx,
    sg_buflen* pbl
    )
{
    SG_NULLFREE(pCtx, pbl);
}

static void buflen__stream__read(
	SG_context * pCtx,
    sg_buflen* pbl,
    SG_uint32 iNumBytesWanted,
    SG_byte* pBytes,
    SG_uint32* piNumBytesRetrieved /*optional*/
    )
{
    SG_uint32 getting = iNumBytesWanted;

    SG_NULLARGCHECK_RETURN(pbl);

    if (pbl->cur >= pbl->len)
    {
        SG_ERR_THROW_RETURN(  SG_ERR_EOF  );
    }

    if (getting > (pbl->len - pbl->cur))
    {
        getting = (pbl->len - pbl->cur);
    }

    if (getting)
    {
        memcpy(pBytes, pbl->p + pbl->cur, getting);
        pbl->cur += getting;
    }

    if (piNumBytesRetrieved)
    {
        *piNumBytesRetrieved = getting;
    }
}

static void buflen__write(
	SG_context * pCtx,
    sg_buflen* pbl,
    SG_uint32 iNumBytes,
    SG_byte* pBytes,
    SG_uint32* piNumBytesWritten /*optional*/
    )
{
    SG_NULLARGCHECK_RETURN(pbl);

    // TODO check pbl->len

    memcpy(pbl->p + pbl->cur, pBytes, iNumBytes);
    pbl->cur += iNumBytes;

    if (piNumBytesWritten)
    {
        *piNumBytesWritten = iNumBytes;
    }
}

void SG_seekreader__alloc__for_buflen(
        SG_context * pCtx,
        SG_byte* p,
        SG_uint32 len,
        SG_seekreader** ppsr
        )
{
    SG_seekreader* psr = NULL;
    sg_buflen* pbuflen = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, pbuflen)  );
    pbuflen->p = p;
    pbuflen->len = len;
    pbuflen->cur = 0;

    SG_ERR_CHECK(  SG_seekreader__alloc(
					   pCtx,
                       pbuflen,
                       0,
					   len,
					   (SG_stream__func__read*) buflen__stream__read,
					   (SG_stream__func__seek*) buflen__seek,
					   (SG_stream__func__close*) buflen__close,
					   &psr)  );

    *ppsr = psr;

    return;

fail:
    SG_NULLFREE(pCtx, psr);
}

void SG_readstream__alloc__for_buflen(
        SG_context * pCtx,
        SG_byte* p,
        SG_uint32 len,
        SG_readstream** ppstrm
        )
{
    SG_readstream* pstrm = NULL;
    sg_buflen* pbuflen = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, pbuflen)  );
    pbuflen->p = p;
    pbuflen->len = len;
    pbuflen->cur = 0;

    SG_ERR_CHECK(  SG_readstream__alloc(
					   pCtx,
					   pbuflen,
					   (SG_stream__func__read*) buflen__stream__read,
					   (SG_stream__func__close*) buflen__close,
                       0,
					   &pstrm)  );

    *ppstrm = pstrm;

    return;

fail:
    SG_NULLFREE(pCtx, pbuflen);
    SG_NULLFREE(pCtx, pstrm);
}

void SG_writestream__alloc__for_buflen(
        SG_context * pCtx,
        SG_byte* p,
        SG_uint32 len,
        SG_writestream** ppstrm
        )
{
    SG_writestream* pstrm = NULL;
    sg_buflen* pbuflen = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, pbuflen)  );
    pbuflen->p = p;
    pbuflen->len = len;
    pbuflen->cur = 0;

    SG_ERR_CHECK(  SG_writestream__alloc(
					   pCtx,
					   pbuflen,
					   (SG_stream__func__write*) buflen__write,
					   (SG_stream__func__close*) buflen__close,
					   &pstrm)  );

    *ppstrm = pstrm;

    return;

fail:
    SG_NULLFREE(pCtx, pbuflen);
    SG_NULLFREE(pCtx, pstrm);
}
