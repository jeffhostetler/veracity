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
#include <zlib.h>

//////////////////////////////////////////////////////////////////

SG_bool SG_parse_bool(const char* psz)
{
	SG_ASSERT(psz);

	if (
		   SG_stricmp(psz, "true") == 0
		|| SG_stricmp(psz, "t")    == 0
		|| SG_stricmp(psz, "yes")  == 0
		|| SG_stricmp(psz, "y")    == 0
		|| SG_stricmp(psz, "on")   == 0
		|| SG_stricmp(psz, "1")    == 0
		)
	{
		return SG_TRUE;
	}
	else
	{
		return SG_FALSE;
	}
}

SG_bool SG_int64__fits_in_double(SG_int64 i64)
{
    double d;
    SG_int64 i64b;

    d = (double) i64;
    i64b = (SG_int64) d;

    return i64 == i64b;
}

SG_bool SG_int64__fits_in_int32(SG_int64 i64)
{
    SG_int32 i32;
    SG_int64 i64b;

    i32 = (SG_int32) i64;
    i64b = (SG_int64) i32;

    return i64 == i64b;
}

SG_bool SG_int64__fits_in_uint32(SG_int64 i64)
{
    SG_uint32 ui32;
    SG_int64 i64b;

    if (i64 < 0)
    {
        return SG_FALSE;
    }

    ui32 = (SG_uint32) i64;
    i64b = (SG_int64) ui32;

    return i64 == i64b;
}

SG_bool SG_int64__fits_in_uint64(SG_int64 i64)
{
    return i64 >= 0;
}

SG_bool SG_uint64__fits_in_int32(SG_uint64 ui64)
{
    SG_int32 i32;
    SG_uint64 ui64b;

    i32 = (SG_int32) ui64;
    if (i32 < 0)
    {
        return SG_FALSE;
    }

    ui64b = (SG_uint64) i32;

    return ui64 == ui64b;
}

SG_bool SG_uint64__fits_in_uint32(SG_uint64 ui64)
{
    SG_uint32 ui32;
    SG_uint64 ui64b;

    ui32 = (SG_uint32) ui64;
    ui64b = (SG_uint64) ui32;

    return ui64 == ui64b;
}

SG_bool SG_double__fits_in_int64(double d)
{
	return ((double)(SG_int64)d) == d;
}

//////////////////////////////////////////////////////////////////

void SG_sleep_ms(SG_uint32 ms)
{
#if defined(WINDOWS)
	Sleep(ms);					// windows Sleep in milliseconds
#else
	usleep( ((useconds_t) ms) * 1000 );		// usleep is microseconds
#endif
}

//////////////////////////////////////////////////////////////////
// QSort is a problem.  The prototype for the compare function for
// the normal qsort() does not have a context pointer of any kind.
//
// On the Mac, we have qsort_r() -- a re-entrant version that allows
// 1 context pointer to be passed thru to the compare function.
//
// On Windows, we have qsort_s() -- a security-enhanced version that
// does the same thing.
//
// We need to pass a pCtx because everything uses it now.
//
// I created this interlude version that allows 2 -- a stock pCtx
// and any *actual* caller-data that the compare function might need.
//
// We use this interlude function with qsort_[rs]() and then re-pack
// the args and call the caller's real compare function.

typedef struct _sg_qsort_context_data
{
	SG_context *					pCtx;
	void *							pVoidCallerData;
	SG_qsort_compare_function *		pfnCompare;

} sg_qsort_context_data;

#if defined(LINUX)
static int _my_qsort_interlude(const void * pArg1,
							   const void * pArg2,
							   void * pVoidQSortContextData)
#else
static int _my_qsort_interlude(void * pVoidQSortContextData,
							   const void * pArg1,
							   const void * pArg2)
#endif
{
	sg_qsort_context_data * p = (sg_qsort_context_data *)pVoidQSortContextData;

	return (*p->pfnCompare)(p->pCtx,
							pArg1,
							pArg2,
							p->pVoidCallerData);
}

void SG_qsort(SG_context * pCtx,
			  void * pArray, size_t nrElements, size_t sizeElements,
			  SG_qsort_compare_function * pfnCompare,
			  void * pVoidCallerData)
{
	sg_qsort_context_data d;

	d.pCtx = pCtx;
	d.pVoidCallerData = pVoidCallerData;
	d.pfnCompare = pfnCompare;

#if defined(WINDOWS)
	qsort_s(pArray,nrElements,sizeElements,_my_qsort_interlude,&d);
#endif

#if defined(MAC)
	qsort_r(pArray,nrElements,sizeElements,&d,_my_qsort_interlude);
#endif

#if defined(LINUX)
	qsort_r(pArray,nrElements,sizeElements,_my_qsort_interlude,&d);
#endif
}

void SG_zlib__inflate__memory(
        SG_context* pCtx, 
        SG_byte* pbSource, 
        SG_uint32 len_source, 
        SG_byte* pbDest, 
        SG_uint32 len_dest
        )
{
	int zErr;
	z_stream strm;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	zErr = inflateInit(&strm);
	if (zErr != Z_OK)
		SG_ERR_THROW_RETURN(SG_ERR_ZLIB(zErr));

	strm.avail_in = len_source;
	strm.next_in = pbSource;

	/* run inflate() on input until output buffer not full */
	do {
		strm.avail_out = len_dest;
		strm.next_out = pbDest;
		zErr = inflate(&strm, Z_NO_FLUSH);
		SG_ASSERT(zErr != Z_STREAM_ERROR);  /* state not clobbered */
		switch (zErr) {
			case Z_NEED_DICT:
				zErr = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				SG_ERR_THROW_RETURN(SG_ERR_ZLIB(zErr));
		}
	} while (strm.avail_out == 0);

	/* clean up and return */
	(void)inflateEnd(&strm);
	if (zErr != Z_STREAM_END)
		SG_ERR_THROW_RETURN(SG_ERR_ZLIB(zErr));
}

void SG_zlib__deflate__memory(
	SG_context* pCtx,
    SG_byte* pbSource,
	SG_uint32 len_source,
    SG_byte** pp_compressed,
    SG_uint32* pp_len_compressed
	)
{
	z_stream zStream;
    int zError = -1;
    SG_byte* pbDest = NULL;
    SG_uint32 len_dest = 0;
    SG_uint32 len_compressed = 0;

    len_dest = len_source / 2;

try_again:
    // init the compressor
    memset(&zStream,0,sizeof(zStream));
    zError = deflateInit(&zStream,Z_DEFAULT_COMPRESSION);
    if (zError != Z_OK)
    {
        SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
    }

    zStream.next_in = pbSource;
    zStream.avail_in = len_source;

    SG_NULLFREE(pCtx, pbDest);
    SG_ERR_CHECK(  SG_allocN(pCtx, len_dest, pbDest)  );

    zStream.next_out = pbDest;
    zStream.avail_out = len_dest;

    zError = deflate(&zStream,Z_FINISH);
    if (Z_OK == zError)
    {
        len_dest *= 2;
        deflateEnd(&zStream);
        goto try_again;
    }
    if (Z_STREAM_END != zError)
    {
        SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
    }

    len_compressed = (SG_uint32) (zStream.next_out - pbDest);
	deflateEnd(&zStream);

    *pp_compressed = pbDest;
    *pp_len_compressed = len_compressed;

fail:
    ;
}

void SG_zlib__deflate__file(
	SG_context* pCtx,
    SG_pathname* pPath_src,
    SG_pathname* pPath_dest
	)
{
	SG_uint64 sofar = 0;
	SG_uint32 got = 0;
	z_stream zStream;
    int zError = -1;
    SG_file* pFile_dest = NULL;
    SG_file* pFile_src = NULL;
    SG_uint64 len_full = 0;
    struct _sg_twobuffers
    {
        SG_byte buf1[16*1024];
        SG_byte buf2[16*1024];
    }* ptwo;

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptwo)  );

    // init the compressor
    memset(&zStream,0,sizeof(zStream));
    zError = deflateInit(&zStream,Z_DEFAULT_COMPRESSION);
    if (zError != Z_OK)
    {
        SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
    }

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_src, &len_full, NULL)  );
    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_src, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, 0644, &pFile_src)  );
    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_dest, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile_dest)  );

	while (sofar < len_full)
	{
		SG_uint32 want = 0;
		if ((len_full - sofar) > (SG_uint64) sizeof(ptwo->buf1))
		{
			want = (SG_uint32) sizeof(ptwo->buf1);
		}
		else
		{
			want = (SG_uint32) (len_full - sofar);
		}
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile_src, want, ptwo->buf1, &got)  );

        // give this chunk to compressor (it will update next_in and avail_in as
        // it consumes the input).

        zStream.next_in = (SG_byte*) ptwo->buf1;
        zStream.avail_in = got;

        while (1)
        {
            // give the compressor the complete output buffer

            zStream.next_out = ptwo->buf2;
            zStream.avail_out = sizeof(ptwo->buf2);

            // let compressor compress what it can of our input.  it may or
            // may not take all of it.  this will update next_in, avail_in,
            // next_out, and avail_out.

            zError = deflate(&zStream,Z_NO_FLUSH);
            if (zError != Z_OK)
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }

            // if there was enough input to generate a compression block,
            // we can write it to our output file.  the amount generated
            // is the delta of avail_out (or equally of next_out).

            if (zStream.avail_out < sizeof(ptwo->buf2))
            {
                SG_uint32 nOut = sizeof(ptwo->buf2) - zStream.avail_out;
                SG_ASSERT ( (nOut == (SG_uint32)(zStream.next_out - ptwo->buf2)) );

                SG_ERR_CHECK(  SG_file__write(pCtx, pFile_dest, nOut, (SG_byte*) ptwo->buf2, NULL)  );
            }

            // if compressor didn't take all of our input, let it
            // continue with the existing buffer (next_in was advanced).

            if (zStream.avail_in == 0)
                break;
        }

		sofar += got;
	}

    // we reached end of the input file.  now we need to tell the
    // compressor that we have no more input and that it needs to
    // compress and flush any partial buffers that it may have.

    SG_ASSERT( (zStream.avail_in == 0) );
    while (1)
    {
        zStream.next_out = ptwo->buf2;
        zStream.avail_out = sizeof(ptwo->buf2);

        zError = deflate(&zStream,Z_FINISH);
        if ((zError != Z_OK) && (zError != Z_STREAM_END))
        {
            SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
        }

        if (zStream.avail_out < sizeof(ptwo->buf2))
        {
            SG_uint32 nOut = sizeof(ptwo->buf2) - zStream.avail_out;
            SG_ASSERT ( (nOut == (SG_uint32)(zStream.next_out - ptwo->buf2)) );

            SG_ERR_CHECK(  SG_file__write(pCtx, pFile_dest, nOut, (SG_byte*) ptwo->buf2, NULL)  );
        }

        if (zError == Z_STREAM_END)
            break;
    }

	deflateEnd(&zStream);

    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_src)  );
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_dest)  );

fail:
    SG_NULLFREE(pCtx, ptwo);
    if (pFile_src)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_src)  );
    }
    if (pFile_dest)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_dest)  );
    }
}

void SG_zlib__inflate__file__already_open(
	SG_context* pCtx,
    SG_file* pFile_src,
    SG_uint64 len_encoded,
    SG_pathname* pPath_dest
	)
{
	SG_uint64 sofar_encoded = 0;
	SG_uint64 sofar_full = 0;
    int zError = -1;
	z_stream zStream;
    SG_file* pFile_dest = NULL;
    SG_bool b_done = SG_FALSE;
    struct _sg_twobuffers
    {
        SG_byte buf1[16*1024];
        SG_byte buf2[16*1024];
    }* ptwo;

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptwo)  );

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_dest, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile_dest)  );

    memset(&zStream,0,sizeof(zStream));
    zError = inflateInit(&zStream);
    if (zError != Z_OK)
    {
        SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
    }

    while (!b_done)
    {
        SG_uint32 got_full = 0;

        if (0 == zStream.avail_in)
        {
            SG_uint32 want = 0;
            SG_uint32 got_encoded = 0;

            if ((len_encoded - sofar_encoded) > sizeof(ptwo->buf1))
            {
                want = sizeof(ptwo->buf1);
            }
            else
            {
                want = (SG_uint32) (len_encoded - sofar_encoded);
            }
            SG_ERR_CHECK(  SG_file__read(pCtx, pFile_src, want, ptwo->buf1, &got_encoded)  );

            zStream.next_in = ptwo->buf1;
            zStream.avail_in = got_encoded;

            sofar_encoded += got_encoded;
        }

        zStream.next_out = ptwo->buf2;
        zStream.avail_out = sizeof(ptwo->buf2);

        while (1)
        {
            // let decompressor decompress what it can of our input.  it may or
            // may not take all of it.  this will update next_in, avail_in,
            // next_out, and avail_out.
            //
            // WARNING: we get Z_STREAM_END when the decompressor is finished
            // WARNING: processing all of the input.  it gives this immediately
            // WARNING: (with the last partial buffer) rather than returning OK
            // WARNING: for the last partial buffer and then returning END with
            // WARNING: zero bytes transferred.

            zError = inflate(&zStream,Z_NO_FLUSH);
            if ((zError != Z_OK)  &&  (zError != Z_STREAM_END))
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }

            if (zError == Z_STREAM_END)
            {
                b_done = SG_TRUE;
                break;
            }

            // if decompressor didn't take all of our input, let it continue
            // with the existing buffer (next_in was advanced).

            if (zStream.avail_in == 0)
            {
                break;
            }

            if (zStream.avail_out == 0)
            {
                /* no room left */
                break;
            }
        }

        if (zStream.avail_out < sizeof(ptwo->buf2))
        {
            got_full = sizeof(ptwo->buf2) - zStream.avail_out;
        }
        else
        {
            got_full = 0;
        }

        if (got_full)
        {
            SG_ERR_CHECK(  SG_file__write(pCtx, pFile_dest, got_full, (SG_byte*) ptwo->buf2, NULL)  );
            sofar_full += got_full;
        }
    }

    if (sofar_encoded != len_encoded)
    {
        SG_ERR_THROW(  SG_ERR_INCOMPLETEWRITE  );
    }

    SG_ASSERT( (zStream.avail_in == 0) );

    inflateEnd(&zStream);

    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_dest)  );

fail:
    SG_NULLFREE(pCtx, ptwo);
    if (pFile_dest)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_dest)  );
    }
}

void SG_zlib__inflate__file(
	SG_context* pCtx,
    SG_pathname* pPath_src,
    SG_pathname* pPath_dest
	)
{
    SG_file* pFile_src = NULL;
    SG_uint64 len_encoded = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_src, &len_encoded, NULL)  );
    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_src, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, 0644, &pFile_src)  );
    SG_ERR_CHECK(  SG_zlib__inflate__file__already_open(pCtx, pFile_src, len_encoded, pPath_dest)  );
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_src)  );

fail:
    if (pFile_src)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_src)  );
    }
}

