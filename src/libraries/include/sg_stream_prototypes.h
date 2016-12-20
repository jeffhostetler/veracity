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
 * @file sg_stream_prototypes.h
 *
 */

#ifndef H_SG_STREAM_PROTOTYPES_H
#define H_SG_STREAM_PROTOTYPES_H

BEGIN_EXTERN_C

void SG_readstream__alloc(
        SG_context * pCtx,
        void* pUnderlying,
        SG_stream__func__read*,
        SG_stream__func__close*,
        SG_uint64 limit,
        SG_readstream** ppstrm
        );

void SG_readstream__alloc__for_file(
        SG_context * pCtx,
        SG_pathname* pPath,
        SG_readstream** ppstrm
        );

void SG_readstream__alloc__for_buflen(
        SG_context * pCtx,
        SG_byte* p,
        SG_uint32 len,
        SG_readstream** ppstrm
        );

void SG_readstream__alloc__for_stdin(
        SG_context * pCtx,
        SG_readstream** ppstrm,
        SG_bool binary
        );

void SG_readstream__read(
        SG_context * pCtx,
        SG_readstream* pstrm,
        SG_uint32 iNumBytesWanted,
        SG_byte* pBytes,
        SG_uint32* piNumBytesRetrieved /*optional*/
        );

void SG_readstream__get_count(
        SG_context * pCtx,
        SG_readstream* pstrm,
        SG_uint64* pi
        );

void SG_readstream__close(
        SG_context * pCtx,
        SG_readstream* pstrm
        );

#define SG_READSTREAM_NULLCLOSE(pCtx, pstrm) _sg_generic_nullfree(pCtx, pstrm, SG_readstream__close);

void SG_writestream__alloc(
        SG_context * pCtx,
        void* pUnderlying,
        SG_stream__func__write*,
        SG_stream__func__close*,
        SG_writestream** ppstrm
        );

void SG_writestream__alloc__for_file(
        SG_context * pCtx,
        SG_pathname* pPath,
        SG_writestream** ppstrm
        );

void SG_writestream__alloc__for_buflen(
        SG_context * pCtx,
        SG_byte* p,
        SG_uint32 len,
        SG_writestream** ppstrm
        );

void SG_writestream__alloc__for_stdout(
        SG_context * pCtx,
        SG_writestream** ppstrm,
        SG_bool binary
        );

void SG_writestream__alloc__for_stderr(
        SG_context * pCtx,
        SG_writestream** ppstrm,
        SG_bool binary
        );

void SG_writestream__write(
        SG_context * pCtx,
        SG_writestream* pstrm,
        SG_uint32 iNumBytes,
        SG_byte* pBytes,
        SG_uint32* piNumBytesWritten /*optional*/
        );

void SG_writestream__get_count(
        SG_context * pCtx,
        SG_writestream* pstrm,
        SG_uint64* pi
	);

void SG_writestream__close(
        SG_context * pCtx,
        SG_writestream* pstrm
        );

#define SG_WRITESTREAM_NULLCLOSE(pCtx, pstrm) _sg_generic_nullfree(pCtx, pstrm, SG_writestream__close);

void SG_seekreader__alloc(
        SG_context * pCtx,
        void* pUnderlying,
        SG_uint64 iHeaderOffset,
        SG_uint64 iLength,
        SG_stream__func__read*,
        SG_stream__func__seek*,
        SG_stream__func__close*,
        SG_seekreader** ppsr
        );

void SG_seekreader__read(
        SG_context * pCtx,
        SG_seekreader* psr,
        SG_uint64 iPos,
        SG_uint32 iNumBytesWanted,
        SG_byte* pBytes,
        SG_uint32* piNumBytesRetrieved /*optional*/
        );

void SG_seekreader__length(
        SG_context * pCtx,
        SG_seekreader* psr,
        SG_uint64* piLength
        );

void SG_seekreader__close(
        SG_context * pCtx,
        SG_seekreader* psr
        );

#define SG_SEEKREADER_NULLCLOSE(pCtx, pstrm) _sg_generic_nullfree(pCtx, pstrm, SG_seekreader__close);

void SG_seekreader__alloc__for_file(
        SG_context * pCtx,
        SG_pathname* pPath,
        SG_uint64 iHeaderOffset,
        SG_seekreader** ppstrm
        );

void SG_seekreader__alloc__for_buflen(
        SG_context * pCtx,
        SG_byte* p,
        SG_uint32 len,
        SG_seekreader** ppstrm
        );

END_EXTERN_C

#endif

