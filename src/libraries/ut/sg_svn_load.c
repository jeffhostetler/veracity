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
#include <sghash.h>

/*
 * This file is broken up into the following sections, in order of their placement
 * in the file as well as in reverse-order of which might depend on which.
 * All of the types/functions in each section are prefixed with specific names
 * to make everything easier to navigate and understand.
 *
 * Error Helpers (ERR_*, error__*)
 * This section contains error handling helpers.  These could be factored out into
 * SG_error_*.
 *
 * Blob Streams (readstream__*, writestream__*)
 * This section contains functions for using an SG_readstream to read from a blob
 * and an SG_writestream to write to a blob.  These could be factored out into
 * sg_stream.c.
 *
 * Buffered Read Stream (buffered_readstream__*)
 * This is a wrapper around SG_readstream that buffers the data being read.
 * This could be factored out into an independent SG_buffered_readstream type
 * in sg_stream.c.
 *
 * Import Helpers (import__*)
 * This is a collection of free functions that should be generally helpful to
 * importers.  They could be factored out into a shared import lib.
 *
 * Pending Node (pending_node__*)
 * This is a scaled down version of pendingtree that only contains enough data and
 * functionality to be useful to importers.  This could be factored out into an
 * independent type that would probably want to live in a shared import lib.
 *
 * SVN Diff (svndiff__*)
 * This is an svndiff reader implementation.  It reads svndiff format deltas and
 * applies them to base input, resulting in modified output.  With some minor
 * updates, this could be factored out into an independent system (it currently uses
 * an import__ type that wouldn't make sense if it were stand-alone).
 *
 * SVN Dump (parse__*, dump__*, revision__*)
 * This is the actual SVN dump importer.
 */


/*
**
** Error Helpers
**
*/

/**
 * As SG_ERR_IGNORE, but if an error occurs it's logged as a warning.
 */
#define ERR_LOG_IGNORE(expr) \
	SG_STATEMENT( \
		SG_context__push_level(pCtx);\
		expr; \
		if (SG_context__has_err(pCtx) != SG_FALSE) { error__log(pCtx, SG_LOG__MESSAGE__WARNING); } \
		SG_context__pop_level(pCtx); \
	)

/**
 * Helper function for ERR_LOG_IGNORE.
 */
static void error__log(
	SG_context*          pCtx,
	SG_log__message_type eType
	)
{
	SG_error   eError;
	SG_string* sMessage = NULL;

	SG_context__get_err(pCtx, &eError);
	if (eError != SG_ERR_OK)
	{
		char        szMessage[SG_ERROR_BUFFER_SIZE + 1u];
		const char* szOutput = NULL;

		SG_context__err_to_string(pCtx, SG_TRUE, &sMessage);
		if (sMessage == NULL)
		{
			SG_error__get_message(eError, SG_TRUE, szMessage, SG_ERROR_BUFFER_SIZE);
			szOutput = szMessage;
		}
		else
		{
			szOutput = SG_string__sz(sMessage);
		}
		SG_ERR_IGNORE(  SG_log__report_message(pCtx, eType, "Error logged and ignored:" SG_PLATFORM_NATIVE_EOL_STR "%s" SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR, szOutput)  );
		SG_STRING_NULLFREE(pCtx, sMessage);
	}
}


/*
**
** Blob Streams
**
*/

/**
 * Underlying data for a readstream that reads from a blob.
 */
typedef struct readstream__blob__data
{
	SG_repo*                   pRepo;  //< Repo to fetch a blob from.
	SG_repo_fetch_blob_handle* pBlob;  //< Handle for the fetch operation.
}
readstream__blob__data;

/**
 * Read implementation for a stream that reads from a blob.
 */
static void readstream__blob__read(
	SG_context * pCtx,
	void* pUnderlying,
	SG_uint32 iNumBytesWanted,
	SG_byte* pBytes,
	SG_uint32* piNumBytesRetrieved /*optional*/
	)
{
	readstream__blob__data* pData = (readstream__blob__data*)pUnderlying;
	SG_bool                 bDone = SG_FALSE;

	*piNumBytesRetrieved = 0u;

	while (pData->pBlob != NULL && iNumBytesWanted > 0u)
	{
		SG_uint32 uGot = 0u;

		SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx, pData->pRepo, pData->pBlob, iNumBytesWanted, pBytes, &uGot, &bDone)  );
		iNumBytesWanted      -= uGot;
		pBytes               += uGot;
		*piNumBytesRetrieved += uGot;

		if (bDone != SG_FALSE)
		{
			SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pData->pRepo, &pData->pBlob)  );
		}
	}

fail:
	return;
}

/**
 * Close implementation for a stream that reads from a blob.
 */
static void readstream__blob__close(
	SG_context * pCtx,
	void* pUnderlying
	)
{
	readstream__blob__data* pData = (readstream__blob__data*)pUnderlying;

	if (pData != NULL)
	{
		if (pData->pBlob != NULL)
		{
			SG_ERR_CHECK(  SG_repo__fetch_blob__abort(pCtx, pData->pRepo, &pData->pBlob)  );
		}
		SG_NULLFREE(pCtx, pData);
	}

fail:
	return;
}

/**
 * Allocates an SG_readstream that reads from a blob.
 * Most arguments are passed straight to SG_repo__fetch_blob__begin.
 */
void readstream__alloc__for_blob(
	SG_context * pCtx,
	SG_repo* pRepo,
	const char* psz_hid_blob,
	SG_bool b_convert_to_full,
	SG_blob_encoding* pBlobFormat,
	char** ppsz_hid_vcdiff_reference,
	SG_uint64* pLenRawData,
	SG_uint64* pLenFull,
	SG_readstream** ppstrm
	)
{
	readstream__blob__data* pData = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pData)  );
	pData->pRepo  = pRepo;

	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pRepo, psz_hid_blob, b_convert_to_full, pBlobFormat, ppsz_hid_vcdiff_reference, pLenRawData, pLenFull, &pData->pBlob)  );
	SG_ERR_CHECK(  SG_readstream__alloc(pCtx, (void*)pData, readstream__blob__read, readstream__blob__close, 0u, ppstrm)  );
	pData = NULL;

fail:
	SG_NULLFREE(pCtx, pData);
}

/**
 * Underlying data for a writestream that writes to a blob.
 */
typedef struct writestream__blob__data
{
	const SG_bool*     pAbort;       //< Whether or not we should abort the store on close.
	                                 //< Owned by the caller/user of the stream.
	                                 //< NULL is treated like false.
	SG_repo*           pRepo;        //< Repo to store the blob to.
	SG_repo_tx_handle* pTransaction; //< Transaction to use when storing the blob.
	                                 //< If NULL, we'll start/commit our own transaction.
	char**             ppHid;        //< Return value for the HID of the stored blob.
	SG_uint64*         pSize;        //< Return value for the size of the stored blob.
	SG_tempfile*       pTempFile;    //< Temp file where the blob is being written.
}
writestream__blob__data;

/**
 * Write implementation for a stream that writes to a blob.
 */
static void writestream__blob__write(
	SG_context * pCtx,
	void* pUnderlying,
	SG_uint32 iNumBytes,
	const SG_byte* pBytes,
	SG_uint32* piNumBytesWritten /*optional*/
	)
{
	writestream__blob__data* pData = (writestream__blob__data*)pUnderlying;

	if (pData->pTempFile == NULL)
	{
		*piNumBytesWritten = 0u;
	}
	else
	{
		SG_ERR_CHECK(  SG_file__write(pCtx, pData->pTempFile->pFile, iNumBytes, pBytes, piNumBytesWritten)  );
	}

fail:
	return;
}

/**
 * Helper that actually stores the blob once it's written to a temp file.
 */
static void writestream__blob__store(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	SG_file*           pFile,        //< [in] Open file containing the blob to store.
	                                 //<      Must already be positioned appropriately.
	SG_repo*           pRepo,        //< [in] Repo to store the blob into.
	SG_repo_tx_handle* pTransaction, //< [in] Transaction to use to store the blob.
	                                 //<      If NULL, we'll start/commit our own.
	char**             ppHid,        //< [out] HID of the stored blob.
	SG_uint64*         pSize         //< [out] Size of the stored blob.
	)
{
	SG_bool  bTransaction = SG_FALSE;

	if (pTransaction == NULL)
	{
		SG_repo__begin_tx(pCtx, pRepo, &pTransaction);
		bTransaction = SG_TRUE;
	}

	SG_ERR_CHECK(  SG_repo__store_blob_from_file(pCtx, pRepo, pTransaction, SG_FALSE, pFile, ppHid, pSize)  );

	if (bTransaction != SG_FALSE)
	{
		SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &pTransaction)  );
	}

fail:
	if (bTransaction != SG_FALSE && pTransaction != NULL)
	{
		SG_repo__abort_tx(pCtx, pRepo, &pTransaction);
	}
	return;
}

/**
 * Close implementation for a stream that writes to a blob.
 */
static void writestream__blob__close(
	SG_context * pCtx,
	void* pUnderlying
	)
{
	writestream__blob__data* pData = (writestream__blob__data*)pUnderlying;

	if (pData != NULL)
	{
		if (pData->pTempFile != NULL)
		{
			if (pData->pAbort == NULL || *pData->pAbort == SG_FALSE)
			{
				SG_ERR_CHECK(  SG_file__seek(pCtx, pData->pTempFile->pFile, 0u)  );
				SG_ERR_CHECK(  writestream__blob__store(pCtx, pData->pTempFile->pFile, pData->pRepo, pData->pTransaction, pData->ppHid, pData->pSize)  );
			}
			SG_ERR_CHECK(  SG_tempfile__close_and_delete(pCtx, &pData->pTempFile)  );
		}
		SG_NULLFREE(pCtx, pData);
	}

fail:
	return;
}

/**
 * Allocates an SG_writestream that writes to a blob.
 * The return values will not be available until the stream is closed (and not aborted).
 */
void writestream__alloc__for_blob(
	SG_context * pCtx,
	const SG_bool* pAbort, //< User can set indicated bool to specify whether the fetch should be aborted on close.
	                       //< If NULL, it will be impossible to abort the fetch.
	SG_repo* pRepo,
	SG_repo_tx_handle* pTx, //< If NULL, a transaction is started/committed internally.
	char** ppszidHidReturned,
	SG_uint64* iBlobFullLength,
	SG_writestream** ppstrm
	)
{
	writestream__blob__data* pData = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pData)  );
	pData->pAbort = pAbort;
	pData->pRepo = pRepo;
	pData->pTransaction = pTx;
	pData->ppHid = ppszidHidReturned;
	pData->pSize = iBlobFullLength;

	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pData->pTempFile)  );
	SG_ERR_CHECK(  SG_writestream__alloc(pCtx, (void*)pData, writestream__blob__write, writestream__blob__close, ppstrm)  );
	pData = NULL;

fail:
	SG_NULLFREE(pCtx, pData);
}


/*
**
** Buffered Read Stream
**
*/

/**
 * An SG_readstream that is buffered.
 */
typedef struct buffered_readstream
{
	SG_readstream* pStream;   //< Stream to read from.
	SG_byte*       pBuffer;   //< Buffer to read into and process in.
	SG_uint32      uSize;     //< Size of pBuffer.
	SG_uint32      uCount;    //< Number of valid bytes in aBuffer.
	SG_uint32      uPosition; //< Stream's current position, index into aBuffer.
	SG_bool        bEOF;      //< True if we've reached the end of the stream.
	                          //< False otherwise.
}
buffered_readstream;

/**
 * Frees a stream.
 */
static void buffered_readstream__free(
	SG_context*          pCtx, //< [in] [out] Error and context info.
	buffered_readstream* pThis //< [in] Stream to free, becomes invalid after this.
	)
{
	if (pThis != NULL)
	{
		SG_NULLFREE(pCtx, pThis->pBuffer);
		SG_NULLFREE(pCtx, pThis);
	}
}

/**
 * NULLFREE wrapper around buffered_readstream__free.
 */
#define BUFFERED_READSTREAM_NULLFREE(pCtx, pStream) _sg_generic_nullfree(pCtx, pStream, buffered_readstream__free);

/**
 * Allocates a new buffered_readstream.
 */
static void buffered_readstream__alloc(
	SG_context*           pCtx,       //< [in] [out] Error and context info.
	buffered_readstream** ppThis,     //< [out] Allocated stream.
	SG_readstream*        pStream,    //< [in] The readstream to wrap with a buffer.
	SG_uint32             uBufferSize //< [in] Size of buffer to use.
	)
{
	buffered_readstream* pThis = NULL;

	SG_NULLARGCHECK(ppThis);
	SG_NULLARGCHECK(pStream);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );

	pThis->pStream   = pStream;
	pThis->uSize     = uBufferSize;
	pThis->uCount    = 0u;
	pThis->uPosition = 0u;
	pThis->bEOF      = SG_FALSE;

	SG_ERR_CHECK(  SG_allocN(pCtx, uBufferSize, pThis->pBuffer)  );

	*ppThis = pThis;
	pThis = NULL;

fail:
	BUFFERED_READSTREAM_NULLFREE(pCtx, pThis);
	return;
}

/**
 * Fills a stream's buffer with data.
 *
 * Any unread data already in the buffer is saved, new data is filled in after it.
 * If the entire buffer is still unread, then this function has no effect.
 * If the EOF is reached, then this function will buffer as much data as is
 * available.  Any additional attempts to fill the buffer after the EOF is reached
 * will have no effect.  Check for EOF using the __eof function.
 */
static void buffered_readstream__fill_buffer(
	SG_context*          pCtx, //< [in] [out] Error and context info.
	buffered_readstream* pThis //< [in] Stream to fill the buffer in.
	)
{
	SG_NULLARGCHECK(pThis);
	
	// make sure we haven't already reached the end of the stream
	// and that there's space in the buffer to read into
	if (   pThis->bEOF == SG_FALSE
		&& (pThis->uPosition > 0u || pThis->uCount < pThis->uSize)
		)
	{
		SG_uint32 uRead = 0u;

		// if there are unread bytes, move them to the beginning of the buffer
		if (pThis->uPosition < pThis->uCount)
		{
			memmove(pThis->pBuffer, pThis->pBuffer + pThis->uPosition, pThis->uCount - pThis->uPosition);
		}

		// roll our indices back to match
		pThis->uCount    -= pThis->uPosition;
		pThis->uPosition -= pThis->uPosition;

		// fill the rest of the buffer with new data
		SG_ERR_TRY(  SG_readstream__read(pCtx, pThis->pStream, pThis->uSize - pThis->uCount, pThis->pBuffer + pThis->uCount, &uRead)  );
		SG_ERR_CATCH_SET(SG_ERR_EOF, pThis->bEOF, SG_TRUE);
		SG_ERR_CATCH_END;
		pThis->uCount += uRead;
	}

fail:
	return;
}

/**
 * Checks if the stream has reached the end of its data.
 */
static void buffered_readstream__eof(
	SG_context*          pCtx,  //< [in] [out] Error and context info.
	buffered_readstream* pThis, //< [in] Stream to check.
	SG_bool*             pEOF   //< [out] Whether or not the stream is at its end.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pEOF);

	if (pThis->bEOF != SG_FALSE && pThis->uPosition == pThis->uCount)
	{
		*pEOF = SG_TRUE;
	}
	else
	{
		*pEOF = SG_FALSE;
	}

fail:
	return;
}

/**
 * Gets the number of bytes that the stream has read so far.
 *
 * Note that this number includes any data read by the underlying SG_readstream
 * before it was wrapped by the buffered_readstream.  The count essentially starts
 * from when the SG_readstream was allocated.
 */
static void buffered_readstream__get_count(
	SG_context*          pCtx,  //< [in] [out] Error and context info.
	buffered_readstream* pThis, //< [in] Stream to get the count from.
	SG_uint64*           pCount //< [out] Number of bytes read by the stream.
	)
{
	SG_uint64 uCount = 0u;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pCount);

	SG_ERR_CHECK(  SG_readstream__get_count(pCtx, pThis->pStream, &uCount)  );
	uCount -= pThis->uCount - pThis->uPosition;

	*pCount = uCount;

fail:
	return;
}

/**
 * Read bytes from the stream and advance its current position.
 */
static void buffered_readstream__read(
	SG_context*          pCtx,     //< [in] [out] Error and context info.
	buffered_readstream* pThis,    //< [in] Stream to read from.
	SG_uint32            uWant,    //< [in] Number of bytes wanted.
	SG_byte*             pBuffer,  //< [in] Buffer to fill bytes into.
	                               //<      Size must be >= uWant.
	                               //<      May be NULL to just skip ahead in the stream.
	SG_uint32*           pReceived //< [out] Number of bytes actually read.
	)
{
	SG_uint32 uPosition = 0u;

	SG_NULLARGCHECK(pThis);

	// keep copying until we've given them all they wanted
	// or until we hit the end of the stream
	while (uWant > 0u && pThis->bEOF == SG_FALSE)
	{
		// check how much we can copy from our current buffer
		SG_uint32 uAvailable = pThis->uCount - pThis->uPosition;
		SG_uint32 uCopy      = SG_MIN(uWant, uAvailable);

		// if they want the data, copy the appropriate amount
		if (pBuffer != NULL)
		{
			memcpy((void*)(pBuffer + uPosition), (void*)(pThis->pBuffer + pThis->uPosition), uCopy);
		}

		// advance our positions
		uPosition        += uCopy;
		pThis->uPosition += uCopy;
		uWant            -= uCopy;

		// if we hit the end of our buffer, read a new one
		if (pThis->uPosition == pThis->uCount)
		{
			SG_ERR_CHECK(  buffered_readstream__fill_buffer(pCtx, pThis)  );
		}
	}

	// return how much we copied
	if (pReceived != NULL)
	{
		*pReceived = uPosition;
	}

fail:
	return;
}

/**
 * Check if the next bytes to be read from the stream equal the specified bytes.
 */
static void buffered_readstream__peek_compare(
	SG_context*          pCtx,   //< [in] [out] Error and context info.
	buffered_readstream* pThis,  //< [in] Stream to peek in.
	SG_byte*             pBytes, //< [in] The bytes to check for in the stream.
	SG_uint32            uBytes, //< [in] Size of pBytes.
	                             //<      Must be > 0 and <= stream's buffer size.
	SG_bool*             pMatch  //< [out] Whether or not the given bytes match the next
	                             //<       bytes to be read from the stream.
	)
{
	SG_bool bMatch = SG_FALSE;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pBytes);
	SG_ARGCHECK(uBytes > 0, uBytes);
	SG_ARGCHECK(uBytes <= pThis->uSize, uBytes);
	SG_NULLARGCHECK(pMatch);

	// try to make sure we have enough data buffered to match against
	if (uBytes > pThis->uCount - pThis->uPosition)
	{
		SG_ERR_CHECK(  buffered_readstream__fill_buffer(pCtx, pThis)  );
	}

	// it can only match if the stream has enough data
	if (uBytes <= pThis->uCount - pThis->uPosition)
	{
		if (memcmp((void*)pBytes, (void*)(pThis->pBuffer + pThis->uPosition), uBytes) == 0)
		{
			bMatch = SG_TRUE;
		}
	}

	// return the result
	*pMatch = bMatch;

fail:
	return;
}

/**
 * Reads a specific expected UTF8 character from the stream.
 * Throws an error if a different character is found instead.
 */
static void buffered_readstream__read_utf8_char(
	SG_context*          pCtx,  //< [in] [out] Error and context info.
	buffered_readstream* pThis, //< [in] Stream to read from.
	SG_int32             iChar  //< [in] The UTF8 character that is expected.
	)
{
	SG_int32  iFound = 0;
	SG_uint32 uNext  = 0u;

	SG_ERR_CHECK(  SG_utf8__next_char(pCtx, pThis->pBuffer, pThis->uCount, pThis->uPosition, &iFound, &uNext)  );
	if (iChar == iFound)
	{
		pThis->uPosition = uNext;
	}
	else
	{
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected char not found: %d", iChar));
	}

fail:
	return;
}

/**
 * Same as SG_file__read_newline, see there for more info.
 */
static void buffered_readstream__read_newline(
	SG_context*          pCtx,
	buffered_readstream* pThis
	)
{
	SG_bool bMatch = SG_FALSE;

	SG_NULLARGCHECK(pThis);

	SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pThis, (SG_byte*)"\r\n", 2u, &bMatch)  );
	if (bMatch != SG_FALSE)
	{
		pThis->uPosition += 2u;
	}
	else
	{
		SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pThis, (SG_byte*)"\n", 1u, &bMatch)  );
		if (bMatch != SG_FALSE)
		{
			pThis->uPosition += 1u;
		}
		else
		{
			SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pThis, (SG_byte*)"\r", 1u, &bMatch)  );
			if (bMatch != SG_FALSE)
			{
				pThis->uPosition += 1u;
			}
			else
			{
				SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected newline not found."));
			}
		}
	}

fail:
	return;
}

/**
 * Same as SG_file__read_while, see there for more info.
 */
static void buffered_readstream__read_while(
	SG_context*          pCtx,
	buffered_readstream* pThis,
	const char*          szChars,
	SG_bool              bWhile,
	SG_string**          ppValue,
	SG_int32*            pChar
	)
{
	SG_string* sValue = NULL;
	SG_bool    bFound = SG_FALSE;
	SG_int32   iChar  = 0;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szChars);

	// if they want the string we read, allocate an empty one to start
	if (ppValue != NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sValue)  );
	}

	// make sure we have data buffered
	if (pThis->uCount - pThis->uPosition == 0u)
	{
		SG_ERR_CHECK(  buffered_readstream__fill_buffer(pCtx, pThis)  );
	}

	// keep going until we find the EOF or something [not] in szChars
	while (pThis->bEOF == SG_FALSE && bFound == SG_FALSE)
	{
		SG_uint32 uStart = pThis->uPosition;
		SG_uint32 uNext  = 0u;

		// loop over each UTF8 character in the buffer
		SG_ERR_CHECK(  SG_utf8__next_char(pCtx, pThis->pBuffer, pThis->uCount, pThis->uPosition, &iChar, &uNext)  );
		while (bFound == SG_FALSE && iChar >= 0 && pThis->uPosition < pThis->uCount)
		{
			SG_bool bContains = SG_FALSE;

			// check if this character is in the set of interesting characters
			SG_ERR_CHECK(  SG_utf8__contains_char(pCtx, szChars, iChar, &bContains)  );
			if (bWhile != bContains)
			{
				bFound = SG_TRUE;
			}
			else
			{
				pThis->uPosition = uNext;
				SG_ERR_CHECK(  SG_utf8__next_char(pCtx, pThis->pBuffer, pThis->uCount, pThis->uPosition, &iChar, &uNext)  );
			}
		}
		SG_ASSERT(pThis->uPosition <= pThis->uCount);

		// append everything we've read so far onto the return value, if they care
		if (sValue != NULL && pThis->uPosition - uStart > 0u)
		{
			SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, sValue, pThis->pBuffer + uStart, pThis->uPosition - uStart)  );
		}

		// if we haven't found what we're looking for yet, read another buffer
		if (bFound == SG_FALSE)
		{
			SG_ASSERT(pThis->uPosition == pThis->uCount || iChar < 0);
			SG_ERR_CHECK(  buffered_readstream__fill_buffer(pCtx, pThis)  );
		}
	}
	SG_ASSERT(pThis->bEOF != bFound);

	// return whatever values the caller wanted
	if (ppValue != NULL)
	{
		*ppValue = sValue;
		sValue = NULL;
	}
	if (pChar != NULL && bFound != SG_FALSE)
	{
		*pChar = iChar;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sValue);
	return;
}


/*
**
** Import Helpers
**
*/

/**
 * Possible relationships between two paths.
 */
typedef enum import__path_relation
{
	IMPORT__PATH_RELATION__EQUAL,   //< The checked path is equal to the base path.
	IMPORT__PATH_RELATION__INSIDE,  //< The checked path is inside (a child of) the base path.
	IMPORT__PATH_RELATION__OUTSIDE, //< The checked path is outside the base path.
	IMPORT__PATH_RELATION__PARENT,  //< The checked path is a parent of the base path.
	IMPORT__PATH_RELATION__COUNT    //< Number of values in the enum, for iteration and invalid value.
}
import__path_relation;

/**
 * Callback for performing custom actions on a blob as it is read.
 */
typedef void (import__blob__callback)(
	SG_context* pCtx,          //< [in] [out] Error and context info.
	void*       pCallbackData, //< [in] Data provided by the caller.
	SG_byte*    pBuffer,       //< [in] Buffer containing the current chunk of the blob being read.
	                           //<      If NULL, then the call is a notification.
	                           //<      Check uBuffer to determine which.
	SG_uint32   uBuffer        //< [in] Size of pBuffer.
	                           //<      When pBuffer is NULL, this is an import__blob__notification value.
	);

/**
 * Notifications sent to an import__blob__callback.
 */
typedef enum import__blob__notification
{
	IMPORT__BLOB__BEGIN, //< Sent when a new blob is being started.
	IMPORT__BLOB__END,   //< Sent when a blob has been finished.
	IMPORT__BLOB__FAIL,  //< Sent to cleanup when an error occurs during reading.
	                     //< Make sure you don't throw any errors responding to this.
}
import__blob__notification;

/**
 * Name of file root nodes.
 */
static const char* gszImport__FileRootName = "@";

/**
 * Collection of characters used as path separators.
 */
static const char* gszImport__PathSeparators = "/\\";

/**
 * Checks if a given character is a path separator.
 */
static
SG_bool                    //< True if the given character is a path separator, false otherwise.
import__is_path_separator(
	char cChar             //< [in] Character to check.
	)
{
	return (strchr(gszImport__PathSeparators, cChar) == NULL ? SG_FALSE : SG_TRUE);
}

/**
 * Looks backward through a path to find the separator between the path's
 * last component and the rest of it.  If the path ends with a separator, that
 * separator is ignored and the one before it will be found instead.  If the path
 * contains no separators, zero is returned.
 *
 * This function is used to separate the path of the parent directory from the
 * name of the entry in that directory.
 */
static void import__find_parent_path_separator(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	const char*  szPath,    //< [in] Path to find the last separator in.
	SG_uint32    uLength,   //< [in] Length of szPath.
	SG_uint32*   pSeparator //< [out] Index in szPath of the last separator.
	)
{
	SG_NULLARGCHECK(szPath);

	// move to the index of the last character
	--uLength;

	// if the path ends with a separator, ignore it
	if (uLength > 0u && import__is_path_separator(szPath[uLength]) == SG_TRUE)
	{
		--uLength;
	}

	// move backwards until we find the last separator
	while (uLength > 0u && import__is_path_separator(szPath[uLength]) == SG_FALSE)
	{
		--uLength;
	}

	// return the results
	*pSeparator = uLength;

fail:
	return;
}

/**
 * Determines the relationship between two paths.
 *
 * When modifying this function, see below for some test cases to run on it.
 * When this function gets factored out to an import library, those tests should become automated.
 * If you find a new case that breaks this function, add it to those tests.
 */
static void import__relate_path(
	SG_context*            pCtx,      //< [in] [out] Error and context info.
	const char*            szBase,    //< [in] Base path to compare against.
	                                  //<      If NULL, the specified default is returned.
	const char*            szCheck,   //< [in] Path to check against the base.
	                                  //<      If NULL, the specified default is returned.
	import__path_relation* pRelation, //< [out] Relationship of the checked path to the base path.
	import__path_relation  eDefault   //< [in] Relationship to return if either path is NULL.
	)
{
	SG_NULLARGCHECK(pRelation);

	if (szBase == NULL || szCheck == NULL)
	{
		*pRelation = eDefault;
	}
	else if (strcmp(szBase, szCheck) == 0)
	{
		*pRelation = IMPORT__PATH_RELATION__EQUAL;
	}
	else
	{
		SG_uint32 uBase  = SG_STRLEN(szBase);
		SG_uint32 uCheck = SG_STRLEN(szCheck);

		if (
			   uBase < uCheck
			&& strncmp(szBase, szCheck, uBase) == 0
			&& (
				   import__is_path_separator(szBase[uBase - 1u]) == SG_TRUE
				|| import__is_path_separator(szCheck[uBase]) == SG_TRUE
				)
			)
		{
			if (strspn(szCheck + uBase, gszImport__PathSeparators) == uCheck - uBase)
			{
				// remainder of szCheck is separators
				*pRelation = IMPORT__PATH_RELATION__EQUAL;
			}
			else
			{
				// remainder of szCheck is inside szBase
				*pRelation = IMPORT__PATH_RELATION__INSIDE;
			}
		}
		else if (
			   uBase > uCheck
			&& strncmp(szBase, szCheck, uCheck) == 0
			&& (
				   import__is_path_separator(szCheck[uCheck - 1u]) == SG_TRUE
				|| import__is_path_separator(szBase[uCheck]) == SG_TRUE
				)
			)
		{
			if (strspn(szBase + uCheck, gszImport__PathSeparators) == uBase - uCheck)
			{
				// remainder of szBase is separators
				*pRelation = IMPORT__PATH_RELATION__EQUAL;
			}
			else
			{
				// remainder of szBase is inside szCheck
				*pRelation = IMPORT__PATH_RELATION__PARENT;
			}
		}
		else
		{
			*pRelation = IMPORT__PATH_RELATION__OUTSIDE;
		}
	}

fail:
	return;
}

/*
 * This functionality is for testing import__relate_path.
 * Just stick a call to import__relate_path__test somewhere.
 * Once import__* is factored out somewhere, this should become an automated test.
 */
#if 0
const import__path_relation gaImport__PathRelationOpposites[] =
{
	IMPORT__PATH_RELATION__EQUAL,
	IMPORT__PATH_RELATION__PARENT,
	IMPORT__PATH_RELATION__OUTSIDE,
	IMPORT__PATH_RELATION__INSIDE,
	IMPORT__PATH_RELATION__COUNT
};

typedef struct import__path_relation__test
{
	const char*           szBase;
	const char*           szCheck;
	import__path_relation eRelation;
}
import__path_relation__test;

// Don't include trailing slashes on any cases, the test code will do those tests automatically.
const import__path_relation__test gaImport__PathRelationTests[] =
{
	{ NULL,            NULL,                   IMPORT__PATH_RELATION__COUNT   },
	{ NULL,            "filename",             IMPORT__PATH_RELATION__COUNT   },
	{ "filename",      NULL,                   IMPORT__PATH_RELATION__COUNT   },

	{ "TestApp/res",   "TestApp/resource.h",   IMPORT__PATH_RELATION__OUTSIDE },
	{ "TestApp/res",   "TestApp/res/filename", IMPORT__PATH_RELATION__INSIDE  },

	{ "a/b/c",         "a/b/d",                IMPORT__PATH_RELATION__OUTSIDE },
	{ "a/b/c",         "a/d/e",                IMPORT__PATH_RELATION__OUTSIDE },
	{ "a/b/c",         "a/b/cd",               IMPORT__PATH_RELATION__OUTSIDE },
	{ "a/b/c",         "a/b/c/d",              IMPORT__PATH_RELATION__INSIDE  },
	{ "a/b/c",         "a/b",                  IMPORT__PATH_RELATION__PARENT  },
	{ "a/b/c",         "a/b/c",                IMPORT__PATH_RELATION__EQUAL   },
};

static void import__relate_path__test(
	SG_context* pCtx
	)
{
	SG_uint32 uTest      = 0u;
	char*     szBaseSep  = NULL;
	char*     szCheckSep = NULL;

	for (uTest = 0u; uTest < SG_NrElements(gaImport__PathRelationTests); ++uTest)
	{
		const import__path_relation__test* pTest     = gaImport__PathRelationTests + uTest;
		import__path_relation              eRelation = IMPORT__PATH_RELATION__COUNT;
		SG_uint32                          uLength   = 0u;

		// base test
		SG_ERR_CHECK(  import__relate_path(pCtx, pTest->szBase, pTest->szCheck, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
		SG_ASSERT(eRelation ==  pTest->eRelation);

		// passing the paths in the opposite order should have the opposite result
		SG_ERR_CHECK(  import__relate_path(pCtx, pTest->szCheck, pTest->szBase, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
		SG_ASSERT(eRelation ==  gaImport__PathRelationOpposites[pTest->eRelation]);

		// trailing slash on base path shouldn't matter
		if (pTest->szBase != NULL)
		{
			uLength = SG_STRLEN(pTest->szBase);
			SG_ERR_CHECK(  SG_allocN(pCtx, uLength + 2u, szBaseSep)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx, szBaseSep, uLength + 2u, pTest->szBase)  );
			szBaseSep[uLength] = '/';
			szBaseSep[uLength + 1u] = '\0';
			SG_ERR_CHECK(  import__relate_path(pCtx, szBaseSep, pTest->szCheck, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			SG_ASSERT(eRelation ==  pTest->eRelation);
			SG_ERR_CHECK(  import__relate_path(pCtx, pTest->szCheck, szBaseSep, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			SG_ASSERT(eRelation ==  gaImport__PathRelationOpposites[pTest->eRelation]);
		}

		// trailing slash on check path shouldn't matter
		if (pTest->szCheck != NULL)
		{
			uLength = SG_STRLEN(pTest->szCheck);
			SG_ERR_CHECK(  SG_allocN(pCtx, uLength + 2u, szCheckSep)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx, szCheckSep, uLength + 2u, pTest->szCheck)  );
			szCheckSep[uLength] = '/';
			szCheckSep[uLength + 1u] = '\0';
			SG_ERR_CHECK(  import__relate_path(pCtx, pTest->szBase, szCheckSep, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			SG_ASSERT(eRelation ==  pTest->eRelation);
			SG_ERR_CHECK(  import__relate_path(pCtx, szCheckSep, pTest->szBase, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			SG_ASSERT(eRelation ==  gaImport__PathRelationOpposites[pTest->eRelation]);
		}

		// trailing slash on both paths shouldn't matter
		if (pTest->szBase != NULL && pTest->szCheck != NULL)
		{
			SG_ERR_CHECK(  import__relate_path(pCtx, szBaseSep, szCheckSep, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			SG_ASSERT(eRelation ==  pTest->eRelation);
			SG_ERR_CHECK(  import__relate_path(pCtx, szCheckSep, szBaseSep, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			SG_ASSERT(eRelation ==  gaImport__PathRelationOpposites[pTest->eRelation]);
		}

		// free data for next test
		SG_NULLFREE(pCtx, szBaseSep);
		SG_NULLFREE(pCtx, szCheckSep);
	}

fail:
	SG_NULLFREE(pCtx, szBaseSep);
	SG_NULLFREE(pCtx, szCheckSep);
	return;
}
#endif

/**
 * Closes and deletes a repo from disk.
 *
 * Adapted from the code for "vv repo delete".
 */
static void import__delete_repo(
	SG_context* pCtx,  //< [in] [out] Error and context info.
	SG_repo**   ppRepo //< [in] [out] Repo to delete.
	                   //<            The caller's pointer is blanked if successful.
	)
{
	const char* szNameRef = NULL;
	char*       szName    = NULL;

	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, *ppRepo, &szNameRef)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, szNameRef, &szName)  );

	SG_REPO_NULLFREE(pCtx, *ppRepo);
	szNameRef = NULL;

	SG_ERR_CHECK(  SG_repo__delete_repo_instance(pCtx, szName)  );
	SG_ERR_CHECK(  SG_closet__descriptors__remove(pCtx, szName)  );

fail:
	SG_NULLFREE(pCtx, szName);
	return;
}

/**
 * Creates a new repo that mirrors an existing repo.
 *
 * The new repo will share the admin ID of the existing repo.
 * The new repo will contain the usual initial empty changeset.
 * The new repo's file root treenode ("@" node) will share the GID of the existing repo's.
 *
 * The code for this function was adapted from SG_vv_verbs__init_new_repo and
 * several functions that it calls to do its work.  A separate function is necessary
 * because we need to specify the GID of the "@" node in the initial changeset
 * created in the new repo.  That GID needs to match the GID of the incoming repo's
 * for our pending_node code to be able to correctly split/combine a tree across
 * the source repo and the created mirror repo.  That changeset is created within
 * SG_repo__setup_basic_stuff, so I've adapted that function and everything in
 * between it and the verbs-level function into this one.  This function is also
 * hard-coded to have the created repo share the admin DAGs and other basic details
 * of the source repo.
 */
static void import__create_mirror_repo(
	SG_context* pCtx,         //< [in] [out] Error and context info.
	SG_repo*    pRepo,        //< [in] Repo to create a mirror of.
	const char* szCsid,       //< [in] Changeset to base the mirror's initial changeset on.
	const char* szNameFormat, //< [in] Format string to use to generate the mirror repo's name.
	                          //<      Source repo's name is passed as the only argument.
	SG_repo**   ppMirror,     //< [out] The created mirror repo.
	char**      ppFirstCsid   //< [out] ID of the initial changeset in the mirror repo.
	                          //<       May be NULL if not needed.
	)
{
	char*              szAdmin     = NULL;
	const char*        szName      = NULL;
	char*              szHash      = NULL;
	char*              szSuperRoot = NULL;
	SG_treenode*       pSuperRoot  = NULL;
	char*              szGid       = NULL;
	SG_string*         sName       = NULL;
	SG_audit           oAudit;
	SG_committing*     pCommitting = NULL;
	SG_treenode*       pFileRoot   = NULL;
	char*              szFileRoot  = NULL;
	SG_treenode_entry* pEntry      = NULL;
	SG_dagnode*        pDagNode    = NULL;
	SG_repo*           pMirror     = NULL;
	char*              szFirstCsid = NULL;

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(szNameFormat);
	SG_NULLARGCHECK(ppMirror);

	// get basic info about the repo we're mirroring
	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &szAdmin)  );
	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &szName)  );
	SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pRepo, &szHash)  );

	// find the GID of the file root treenode from the given changeset
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, szCsid, &szSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, szSuperRoot, &pSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pSuperRoot, gszImport__FileRootName, &szGid, &pEntry)  );
	SG_NULLFREE(pCtx, szSuperRoot);
	SG_TREENODE_NULLFREE(pCtx, pSuperRoot);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pEntry);

	// build a name for the new repo
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sName, szNameFormat, szName)  );

	// create the mirror repo and open it
	SG_ERR_CHECK(  SG_repo__create__completely_new__empty__closet(pCtx, szAdmin, NULL, szHash, SG_string__sz(sName))  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(sName), &pMirror)  );

	// install built-in Zing templates
	SG_ERR_CHECK(  sg_zing__init_new_repo(pCtx, pMirror)  );

	// initialize an audit for use in modifying DAGs
	SG_ERR_CHECK(  SG_audit__init(pCtx, &oAudit, pMirror, SG_AUDIT__WHEN__NOW, SG_NOBODY__USERID)  );

	// create an empty first changeset, just a super-root and empty file root
	SG_ERR_CHECK(  SG_committing__alloc(pCtx, &pCommitting, pMirror, SG_DAGNUM__VERSION_CONTROL, &oAudit, SG_CSET_VERSION__CURRENT)  );
	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pFileRoot)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pFileRoot, SG_TN_VERSION__CURRENT)  );
	SG_ERR_CHECK(  SG_committing__tree__add_treenode(pCtx, pCommitting, &pFileRoot, &szFileRoot)  );
	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pSuperRoot, SG_TN_VERSION__CURRENT)  );
	SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC(pCtx, &pEntry)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_entry_name(pCtx, pEntry, "@")  );
	SG_ERR_CHECK(  SG_treenode_entry__set_entry_type(pCtx, pEntry, SG_TREENODEENTRY_TYPE_DIRECTORY)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_attribute_bits(pCtx, pEntry, 0)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_hid_blob(pCtx, pEntry, szFileRoot)  );
	SG_ERR_CHECK(  SG_treenode__add_entry(pCtx, pSuperRoot, szGid, &pEntry)  );
	SG_ERR_CHECK(  SG_committing__tree__add_treenode(pCtx, pCommitting, &pSuperRoot, &szSuperRoot)  );
	SG_ERR_CHECK(  SG_committing__tree__set_root(pCtx, pCommitting, szSuperRoot)  );
	SG_ERR_CHECK(  SG_committing__end(pCtx, pCommitting, NULL, &pDagNode)  );
	pCommitting = NULL;
	SG_ERR_CHECK(  SG_dagnode__get_id(pCtx, pDagNode, &szFirstCsid)  );

	// create a master branch that points to the first changeset
	SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pMirror, SG_VC_BRANCHES__DEFAULT, szFirstCsid, NULL, SG_FALSE, &oAudit)  );

	// add the built-in areas
	SG_ERR_CHECK(  SG_area__add(pCtx, pMirror, SG_AREA_NAME__CORE, SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__USERS), &oAudit)  );
	SG_ERR_CHECK(  SG_area__add(pCtx, pMirror, SG_AREA_NAME__VERSION_CONTROL, SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__VERSION_CONTROL), &oAudit)  );

	// pull admin DAGs into the mirror repo
	SG_ERR_CHECK(  SG_pull__admin__local(pCtx, pMirror, pRepo, NULL)  );

	// return data
	*ppMirror = pMirror;
	pMirror = NULL;
	if (ppFirstCsid != NULL)
	{
		*ppFirstCsid = szFirstCsid;
		szFirstCsid = NULL;
	}

fail:
	if (pMirror != NULL)
	{
		ERR_LOG_IGNORE(  import__delete_repo(pCtx, &pMirror)  );
	}
	SG_NULLFREE(pCtx, szAdmin);
	SG_NULLFREE(pCtx, szHash);
	SG_TREENODE_NULLFREE(pCtx, pSuperRoot);
	SG_NULLFREE(pCtx, szSuperRoot);
	SG_NULLFREE(pCtx, szGid);
	SG_STRING_NULLFREE(pCtx, sName);
	if (pCommitting != NULL)
	{
		ERR_LOG_IGNORE(  SG_committing__abort(pCtx, pCommitting)  );
	}
	SG_TREENODE_NULLFREE(pCtx, pFileRoot);
	SG_NULLFREE(pCtx, szFileRoot);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pEntry);
	SG_DAGNODE_NULLFREE(pCtx, pDagNode);
	SG_NULLFREE(pCtx, szFirstCsid);
	return;
}

/**
 * Fills a hash with users from a given repo, mapping name to ID.
 */
static void import__fill_user_cache(
	SG_context* pCtx,   //< [in] [out] Error and context info.
	SG_vhash*   pCache, //< [in] Hash to fill with users.
	SG_repo*    pRepo   //< [in] Repo to get users from.
	)
{
	SG_varray* pUsers = NULL;
	SG_uint32  uUsers = 0u;
	SG_uint32  uUser  = 0u;

	SG_NULLARGCHECK(pCache);
	SG_NULLARGCHECK(pRepo);

	SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pUsers)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pUsers, &uUsers)  );
	for (uUser = 0u; uUser < uUsers; ++uUser)
	{
		SG_vhash*   pUser      = NULL;
		const char* szUsername = NULL;
		const char* szRecId    = NULL;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pUsers, uUser, &pUser)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, "name", &szUsername)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, "recid", &szRecId)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pCache, szUsername, szRecId)  );
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pUsers);
	return;
}

/**
 * Reads a blob from a stream and potentially stores it to a repo.
 * Can also perform caller-specified actions on the blob data as it's read.
 */
static void import__read_blob(
	SG_context*             pCtx,          //< [in] [out] Error and context info.
	buffered_readstream*    pStream,       //< [in] Stream to read the blob from.
	SG_uint64               uLength,       //< [in] Length of the blob to read.
	SG_byte*                pBuffer,       //< [in] Buffer to use to store blob chunks as they are read.
	SG_uint32               uBuffer,       //< [in] Size of pBuffer.
	SG_repo*                pRepo,         //< [in] Repo to store the blob in.
	                                       //<      NULL to not store the blob in a repo.
	SG_repo_tx_handle*      pTransaction,  //< [in] Transaction to make the blob storage a part of.
	                                       //<      Ignored if pRepo is NULL.
	                                       //<      If NULL, a one-time transaction is used.
	import__blob__callback* fCallback,     //< [in] Callback to pass each blob chunk to.
	                                       //<      May be NULL if no actions are needed.
	void*                   pCallbackData, //< [in] Data to pass to fCallback.
	char**                  ppHid          //< [out] HID of the read blob.
	                                       //<       Ignored if pRepo is NULL.
	                                       //<       May be NULL if not needed.
	)
{
	SG_bool                    bTransaction = SG_FALSE;
	SG_repo_store_blob_handle* pBlob        = NULL;
	char*                      szHid        = NULL;

	SG_NULLARGCHECK(pStream);
	SG_NULLARGCHECK(pBuffer);
	SG_ARGCHECK(uBuffer > 0u, uBuffer);

	// if we're storing the blob, start that process
	if (pRepo != NULL)
	{
		// if no transaction was supplied, start one
		if (pTransaction == NULL)
		{
			SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pRepo, &pTransaction)  );
			bTransaction = SG_TRUE;
		}

		// start storing a blob
		SG_ERR_CHECK(  SG_repo__store_blob__begin(pCtx, pRepo, pTransaction, SG_BLOBENCODING__FULL, NULL, uLength, 0u, NULL, &pBlob)  );
	}

	// if we have a callback, notify it that we're starting
	if (fCallback != NULL)
	{
		SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__BEGIN)  );
	}

	// keep reading while there's any length left
	while (uLength > 0u)
	{
		SG_uint32 uWant  = 0u;
		SG_uint32 uGot   = 0u;
		SG_uint32 uWrote = 0u;

		// decide how much to read
		if (uLength > SG_UINT32_MAX)
		{
			// uLength is larger than we can read in one call...
			if (uBuffer == 0u)
			{
				// ...and we're not constrained by buffer size
				// read as much as we can in one call
				uWant = SG_UINT32_MAX;
			}
			else
			{
				// ...but we're constrained by buffer size
				// read as much as can fit in the buffer
				uWant = uBuffer;
			}
		}
		else
		{
			// uLength is small enough to read in one call...
			if (uBuffer == 0u)
			{
				// ...and we're not constrained by buffer size
				// read as much as we need
				uWant = (SG_uint32)uLength;
			}
			else
			{
				// ...and we're constrained by buffer size
				// read the smaller of what we need or what we can fit in the buffer
				uWant = SG_MIN((SG_uint32)uLength, uBuffer);
			}
		}

		// read however much we want
		SG_ERR_CHECK(  buffered_readstream__read(pCtx, pStream, uWant, pBuffer, &uGot)  );
		if (uGot != uWant)
		{
			SG_int_to_string_buffer szLength;
			SG_ERR_THROW2(SG_ERR_INCOMPLETEREAD, (pCtx, "Couldn't read enough bytes from the stream to fill a blob of length: %s (wanted %u, got %u)", SG_uint64_to_sz(uLength, szLength), uWant, uGot));
		}

		// if we're storing the blob, add this chunk
		if (pBlob != NULL)
		{
			SG_ERR_CHECK(  SG_repo__store_blob__chunk(pCtx, pRepo, pBlob, uGot, pBuffer, &uWrote)  );
			if (uWrote != uGot)
			{
				SG_int_to_string_buffer szLength;
				SG_ERR_THROW2(SG_ERR_INCOMPLETEWRITE, (pCtx, "Couldn't write enough bytes to the repository to populate a blob of length: %s (had %u, wrote %u)", SG_uint64_to_sz(uLength, szLength), uGot, uWrote));
			}
		}

		// if we have a callback, give it the chunk as well
		if (fCallback != NULL)
		{
			SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, pBuffer, uGot)  );
		}

		// update how much is left
		uLength -= uGot;
	}

	// if we're storing the blob, finish it off
	if (pBlob != NULL)
	{
		SG_ERR_CHECK(  SG_repo__store_blob__end(pCtx, pRepo, pTransaction, &pBlob, &szHid)  );

		// if we started this transaction, commit it
		if (bTransaction != SG_FALSE)
		{
			SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &pTransaction)  );
			bTransaction = SG_FALSE;
		}
	}

	// if we have a callback, notify it that we're done
	if (fCallback != NULL)
	{
		SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__END)  );

		// blank the callback so we don't send it a fail notification during cleanup
		fCallback = NULL;
	}

	// if we stored the blob and they want the HID, return it
	if (ppHid != NULL && szHid != NULL)
	{
		*ppHid = szHid;
		szHid = NULL;
	}

fail:
	if (fCallback != NULL)
	{
		fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__FAIL);
	}
	if (pBlob != NULL)
	{
		SG_repo__store_blob__abort(pCtx, pRepo, pTransaction, &pBlob);
	}
	if (bTransaction != SG_FALSE)
	{
		SG_repo__abort_tx(pCtx, pRepo, &pTransaction);
	}
	SG_NULLFREE(pCtx, szHid);
	return;
}

/**
 * Fetches an already stored blob and passes it to a callback for processing.
 */
static void import__fetch_blob(
	SG_context*             pCtx,         //< [in] [out] Error and context info.
	SG_repo*                pRepo,        //< [in] Repo containing the blob to hash.
	const char*             szHid,        //< [in] HID of the blob to hash.
	SG_byte*                pBuffer,      //< [in] Buffer to use to store blob chunks as they are read.
	SG_uint32               uBuffer,      //< [in] Size of pBuffer.
	import__blob__callback* fCallback,    //< [in] Callback to pass each blob chunk to.
	void*                   pCallbackData //< [in] Data to pass to fCallback.
	)
{
	SG_repo_fetch_blob_handle* pBlob   = NULL;
	SG_bool                    bDone   = SG_FALSE;

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(szHid);
	SG_NULLARGCHECK(pBuffer);
	SG_ARGCHECK(uBuffer > 0u, uBuffer);
	SG_NULLARGCHECK(fCallback);

	// notify the callback that we're starting
	SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__BEGIN)  );

	// start fetching the blob
	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pRepo, szHid, SG_TRUE, NULL, NULL, NULL, NULL, &pBlob)  );

	// keep fetching until we're done
	while (bDone == SG_FALSE)
	{
		SG_uint32 uGot = 0u;

		// read the next chunk and give it to the callback
		SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx, pRepo, pBlob, uBuffer, pBuffer, &uGot, &bDone)  );
		SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, pBuffer, uGot)  );
	}

	// end the fetch
	SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pRepo, &pBlob)  );

	// notify the callback that we're done
	SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__END)  );

	// blank the callback so we don't send it a fail notification during cleanup
	fCallback = NULL;

fail:
	if (fCallback != NULL)
	{
		SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__FAIL)  );
	}
	return;
}

/**
 * Copies a blob from one repo to another.
 */
static void import__copy_blob(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const char*        szHid,        //< [in] HID of the blob to copy from the source repo.
	SG_repo*           pSource,      //< [in] Repo to copy the blob from.
	SG_repo*           pTarget,      //< [in] Repo to copy the blob to.
	SG_repo_tx_handle* pTransaction, //< [in] Active transaction on pTarget.
	SG_byte*           pBuffer,      //< [in] Buffer to use to store the blob temporarily.
	                                 //<      If NULL, one is allocated internally.
	SG_uint32          uBuffer,      //< [in] Size of pBuffer.
	                                 //<      Ignored if pBuffer is NULL.
	char**             ppHid         //< [out] HID of the copied blob in the target repo.
	)
{
	SG_bool                    bBufferOwned   = SG_FALSE;
	SG_blob_encoding           eEncoding      = 0;
	SG_uint64                  uLengthEncoded = 0u;
	SG_uint64                  uLengthFull    = 0u;
	SG_repo_fetch_blob_handle* pFetch         = NULL;
	SG_repo_store_blob_handle* pStore         = NULL;
	SG_bool                    bDone          = SG_FALSE;

	// if they didn't supply a buffer, allocate one
	if (pBuffer == NULL)
	{
		uBuffer = SG_STREAMING_BUFFER_SIZE;
		SG_ERR_CHECK(  SG_allocN(pCtx, uBuffer, pBuffer)  );
		bBufferOwned = SG_TRUE;
	}

	// start the fetch and store
	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pSource, szHid, SG_TRUE, &eEncoding, NULL, &uLengthEncoded, &uLengthFull, &pFetch)  );
	SG_ERR_CHECK(  SG_repo__store_blob__begin(pCtx, pTarget, pTransaction, eEncoding, NULL, uLengthFull, uLengthEncoded, NULL, &pStore)  );

	// keep copying chunks until we're done
	while (bDone == SG_FALSE)
	{
		SG_uint32 uGot = 0u;

		SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx, pSource, pFetch, uBuffer, pBuffer, &uGot, &bDone)  );
		if (uGot > 0u)
		{
			SG_uint32 uWrote = 0u;

			SG_ERR_CHECK(  SG_repo__store_blob__chunk(pCtx, pTarget, pStore, uGot, pBuffer, &uWrote)  );
			SG_ASSERT(uWrote == uGot);
		}
	}

	// end the fetch and store, and return the HID
	SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pSource, &pFetch)  );
	SG_ERR_CHECK(  SG_repo__store_blob__end(pCtx, pTarget, pTransaction, &pStore, ppHid)  );

fail:
	if (pFetch != NULL)
	{
		ERR_LOG_IGNORE(  SG_repo__fetch_blob__abort(pCtx, pSource, &pFetch)  );
	}
	if (pStore != NULL)
	{
		ERR_LOG_IGNORE(  SG_repo__store_blob__abort(pCtx, pTarget, pTransaction, &pStore)  );
	}
	if (bBufferOwned != SG_FALSE)
	{
		SG_NULLFREE(pCtx, pBuffer);
	}
	return;
}

/**
 * Recursively copies a treenode blob and its child blobs from one repo to another.
 */
static void import__copy_treenode_blob(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	const char*        szHid,        //< [in] HID of the treenode blob to copy from the source repo.
	SG_repo*           pSource,      //< [in] Repo to copy the blob from.
	SG_repo*           pTarget,      //< [in] Repo to copy the blob to.
	SG_repo_tx_handle* pTransaction, //< [in] Active transaction on pTarget.
	SG_changeset*      pChangeset,   //< [in] Changeset to include the copied treenodes in on pTarget.
	SG_byte*           pBuffer,      //< [in] Buffer to use to store the blob temporarily.
	                                 //<      If NULL, one is allocated internally.
	SG_uint32          uBuffer,      //< [in] Size of pBuffer.
	                                 //<      Ignored if pBuffer is NULL.
	char**             ppHid         //< [out] HID of the copied blob in the target repo.
	)
{
	SG_bool            bBufferOwned        = SG_FALSE;
	SG_treenode*       pTargetNode         = NULL;
	SG_treenode*       pSourceNode         = NULL;
	SG_uint32          uEntries            = 0u;
	SG_uint32          uEntry              = 0u;
	char*              szTargetContents    = NULL;
	SG_treenode_entry* pTargetEntry        = NULL;
	SG_uint64          uTargetSize         = 0u;

	// if they didn't supply a buffer, allocate one
	if (pBuffer == NULL)
	{
		uBuffer = SG_STREAMING_BUFFER_SIZE;
		SG_ERR_CHECK(  SG_allocN(pCtx, uBuffer, pBuffer)  );
		bBufferOwned = SG_TRUE;
	}

	// start building the target node
	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pTargetNode)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pTargetNode, SG_TN_VERSION__CURRENT)  );

	// load the node from the source repo and run through its entries
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pSource, szHid, &pSourceNode)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pSourceNode, &uEntries)  );
	for (uEntry = 0u; uEntry < uEntries; ++uEntry)
	{
		const SG_treenode_entry* pSourceEntry        = NULL;
		const char*              szGid               = NULL;
		SG_treenode_entry_type   eType               = SG_TREENODEENTRY_TYPE__INVALID;
		const char*              szName              = NULL;
		SG_int64                 iAttributes         = 0;
		const char*              szSourceContents    = NULL;

		// get the current source entry and its details
		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pSourceNode, uEntry, &szGid, &pSourceEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pSourceEntry, &eType)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pSourceEntry, &szName)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pSourceEntry, &iAttributes)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pSourceEntry, &szSourceContents)  );

		// copy the content blob
		if (eType == SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			SG_ERR_CHECK(  import__copy_treenode_blob(pCtx, szSourceContents, pSource, pTarget, pTransaction, pChangeset, pBuffer, uBuffer, &szTargetContents)  );
		}
		else
		{
			SG_ERR_CHECK(  import__copy_blob(pCtx, szSourceContents, pSource, pTarget, pTransaction, pBuffer, uBuffer, &szTargetContents)  );
		}

		// generate the target entry and add it to the target node
		SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC(pCtx, &pTargetEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__set_entry_name(pCtx, pTargetEntry, szName)  );
		SG_ERR_CHECK(  SG_treenode_entry__set_entry_type(pCtx, pTargetEntry, eType)  );
		SG_ERR_CHECK(  SG_treenode_entry__set_attribute_bits(pCtx, pTargetEntry, iAttributes)  );
		SG_ERR_CHECK(  SG_treenode_entry__set_hid_blob(pCtx, pTargetEntry, szTargetContents)  );
		SG_ERR_CHECK(  SG_treenode__add_entry(pCtx, pTargetNode, szGid, &pTargetEntry)  );

		// free memory for next loop
		SG_NULLFREE(pCtx, szTargetContents);
	}

	// save the target node
	SG_ERR_CHECK(  SG_treenode__save_to_repo(pCtx, pTargetNode, pTarget, pTransaction, &uTargetSize)  );
	SG_ERR_CHECK(  SG_treenode__get_id(pCtx, pTargetNode, ppHid)  );
	SG_ERR_CHECK(  SG_changeset__tree__add_treenode(pCtx, pChangeset, *ppHid, &pTargetNode)  );

fail:
	if (bBufferOwned != SG_FALSE)
	{
		SG_NULLFREE(pCtx, pBuffer);
	}
	SG_TREENODE_NULLFREE(pCtx, pSourceNode);
	SG_TREENODE_NULLFREE(pCtx, pTargetNode);
	SG_NULLFREE(pCtx, szTargetContents);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTargetEntry);
	return;
}


/*
**
** Pending Node
**
*/

/**
 * Maximum number of repos supported by pending_node.
 */
#define PENDING_NODE__MAX_REPOS 2u

/**
 * Flags used with various pending_node functionality.
 */
typedef enum pending_node__flags
{
	PENDING_NODE__NONE            = 0,

	// get_child
	PENDING_NODE__INACTIVE        = 1 << 0, //< Include deleted/moved nodes, which are usually ignored.

	// walk_path
	PENDING_NODE__RESTORE_DELETED = 1 << 1, //< Undelete nodes as necessary.
	PENDING_NODE__ADD_MISSING     = 1 << 2, //< Add new nodes as necessary.

	PENDING_NODE__LAST            = 1 << 3
}
pending_node__flags;

/**
 * An in-memory treenode that is part of a pending changeset.
 */
typedef struct pending_node
{
	// data that doesn't change between revisions
	char                   szGid[SG_GID_BUFFER_LENGTH];            //< Unique GID of the node.
	                                                               //< Empty for the super-root node.
	SG_treenode_entry_type eType;                                  //< Type of the node.

	// data that was loaded from the baseline revision
	char*                  szBaselineName;                         //< Name the node had in the baseline.
	                                                               //< NULL for the super-root.
	SG_int64               iBaselineAttributes;                    //< Attributes the node had in the baseline.
	                                                               //< 0 for the super-root.
	char*                  aBaselineHids[PENDING_NODE__MAX_REPOS]; //< HIDs of the node's content in the baseline, indexed by repo.

	// data that will be committed to a new revision
	SG_bool                bDeleted;                               //< True if this node has been deleted.
	char*                  szName;                                 //< The node's changed name.
	                                                               //< NULL if the node's name hasn't changed from its baseline, or it's the super-root.
	SG_int64               iAttributes;                            //< The node's current attributes.
	char*                  aHids[PENDING_NODE__MAX_REPOS];         //< HIDs of the node's current content, indexed by repo.
	                                                               //< NULL if the node's content hasn't changed from its baseline.
	                                                               //< If non-NULL, then it MUST be different then its baseline.
	                                                               //< Note that directories never count as changed until they're committed.

	// relationships to other nodes
	struct pending_node*   pParent;                                //< The node's current parent.
	                                                               //< NULL for the super-root node.
	SG_rbtree*             pChildren;                              //< The node's current children, indexed by GID.
	                                                               //< NULL for non-directory nodes.
	                                                               //< NULL for directory nodes whose children aren't loaded yet.
	struct pending_node*   pMoveSource;                            //< The node at this node's old location.
	                                                               //< That node's pMoveTarget will point to this one.
	                                                               //< NULL if the node hasn't been moved.
	struct pending_node*   pMoveTarget;                            //< The node where this node moved to.
	                                                               //< That node's pMoveSource will point to this one.
	                                                               //< NULL if the node hasn't been moved.
}
pending_node;

/**
 * Callback function that gets an actual repo for a given repo index.
 */
typedef void (pending_node__get_repo_data)(
	SG_context*         pCtx,          //< [in] [out] Error and context info.
	SG_uint32           uRepo,         //< [in] Index of the repo to get.
	void*               pData,         //< [in] Data supplied by the caller.
	SG_repo**           ppRepo,        //< [out] Repo for the given index.
	                                   //<       May be NULL if not needed.
	SG_repo_tx_handle** ppTransaction, //< [out] Active transaction on the given repo.
	                                   //<       May be NULL if not needed.
	SG_changeset**      ppChangeset    //< [out] Changeset to include changes to the repo in.
	                                   //<       May be NULL if not needed.
	);

/**
 * Callback function to determine if a node should be committed to a repo.
 */
typedef void (pending_node__commit_to_repo)(
	SG_context*   pCtx,   //< [in] [out] Error and context info.
	pending_node* pThis,  //< [in] Node to check.
	const char*   szPath, //< [in] Full path of the node.
	                      //<      NULL for the super-root node.
	SG_uint32     uRepo,  //< [in] Repo to check.
	void*         pData,  //< [in] Data supplied by the caller.
	SG_bool*      pCommit //< [out] True if the node should be committed to the repo.
	                      //<       False if the node should not be committed to the repo.
	);

/*
 * Simple flag checking macros.
 */
#define HAS_NO_FLAGS(uFlags, uMask) (((uFlags) & (uMask)) == 0)
#define HAS_ANY_FLAG(uFlags, uMask) (((uFlags) & (uMask)) != 0)
#define HAS_ALL_FLAGS(uFlags, uMask) (((uFlags) & (uMask)) == (uMask))

/*
 * Forward declarations of various pending_node functions.
 */
static void pending_node__free(SG_context*, pending_node*);
static void pending_node__get_name(SG_context*, const pending_node*, const char**);
static void pending_node__get_hid(SG_context*, const pending_node*, SG_uint32, const char**);
static void pending_node__get_child(SG_context*, pending_node*, const char*, SG_uint32, SG_uint32, pending_node__get_repo_data*, void*, pending_node**);
static void pending_node__add_child(SG_context*, pending_node*, pending_node*);

/**
 * Map of SG_treenode_entry_type values to strings.
 */
static const char* gaPendingNodeType_String[] =
{
	"invalid",   //< SG_TREENODEENTRY_TYPE__INVALID
	"file",      //< SG_TREENODEENTRY_TYPE_REGULAR_FILE
	"directory", //< SG_TREENODEENTRY_TYPE_DIRECTORY
	"symlink",   //< SG_TREENODEENTRY_TYPE_SYMLINK
	"submodule"  //< SG_TREENODEENTRY_TYPE_SUBMODULE
};

/**
 * Wrapper around pending_node__free that fits the SG_free_callback prototype.
 */
static void pending_node__void_free(
	SG_context*   pCtx, //< [in] [out] Error and context info.
	void*         pThis //< [in] pending_node to free.
	)
{
	pending_node__free(pCtx, (pending_node*)pThis);
}

/**
 * Frees a pending_node.
 */
static void pending_node__free(
	SG_context*   pCtx, //< [in] [out] Error and context info.
	pending_node* pThis //< [in] Node to free.
	)
{
	if (pThis != NULL)
	{
		SG_uint32 uRepo = PENDING_NODE__MAX_REPOS;

		SG_NULLFREE(pCtx, pThis->szBaselineName);
		SG_NULLFREE(pCtx, pThis->szName);
		for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
		{
			SG_NULLFREE(pCtx, pThis->aBaselineHids[uRepo]);
			SG_NULLFREE(pCtx, pThis->aHids[uRepo]);
		}
		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pThis->pChildren, pending_node__void_free);
		SG_NULLFREE(pCtx, pThis);
	}
}

/**
 * NULLFREE wrapper around pending_node__free.
 */
#define PENDING_NODE_NULLFREE(pCtx, pPendingNode) _sg_generic_nullfree(pCtx, pPendingNode, pending_node__free);

/**
 * Allocates a blank pending_node, you probably want one of the other overloads.
 */
static void pending_node__alloc(
	SG_context*    pCtx,  //< [in] [out] Error and context info.
	pending_node** ppThis //< [out] Newly allocated node.
	)
{
	pending_node* pThis = NULL;
	SG_uint32     uRepo = PENDING_NODE__MAX_REPOS;

	SG_NULLARGCHECK(ppThis);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );

	pThis->szGid[0u]   = '\0';
	pThis->eType       = SG_TREENODEENTRY_TYPE__INVALID;
	pThis->bDeleted    = SG_FALSE;
	pThis->szName      = NULL;
	pThis->iAttributes = 0;
	pThis->pParent     = NULL;
	pThis->pChildren   = NULL;
	pThis->pMoveSource = NULL;
	pThis->pMoveTarget = NULL;

	for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
	{
		pThis->aBaselineHids[uRepo] = NULL;
		pThis->aHids[uRepo] = NULL;
	}

	*ppThis = pThis;
	pThis = NULL;

fail:
	PENDING_NODE_NULLFREE(pCtx, pThis);
	return;
}

/**
 * Allocates a pending_node whose data is a copy of another one.
 *
 * The new node will generate its own GID, rather than copying the source's.
 * The source node's relationships (like its parent or move source/target) are also NOT copied.
 */
static void pending_node__alloc__copy(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	pending_node**      ppThis,    //< [out] Newly allocated node.
	const pending_node* pThat,     //< [in] Node to copy.
	const char*         szName,    //< [in] Name of the new node, if different than the name of pThat.
	                               //<      If NULL, the name of pThat is copied.
	SG_uint32           uSize,     //< [in] Size of szName.
	                               //<      Ignored if szName is NULL.
	SG_bool             bRecursive //< [in] Whether or not to recursively copy the source's children.
	)
{
	pending_node*       pThis     = NULL;
	SG_rbtree_iterator* pIterator = NULL;

	SG_NULLARGCHECK(ppThis);
	SG_NULLARGCHECK(pThat);

	SG_ERR_CHECK(  pending_node__alloc(pCtx, &pThis)  );

	// generate a GID
	SG_ERR_CHECK(  SG_gid__generate(pCtx, pThis->szGid, SG_GID_BUFFER_LENGTH)  );

	// copy basic info
	pThis->eType       = pThat->eType;
	pThis->iAttributes = pThat->iAttributes;

	// if they didn't specify a new name, use the source's
	if (szName == NULL)
	{
		SG_ERR_CHECK(  pending_node__get_name(pCtx, pThat, &szName)  );
		uSize = SG_STRLEN(szName);
	}

	// copy the name
	if (szName != NULL)
	{
		SG_ERR_CHECK(  SG_allocN(pCtx, uSize + 1u, pThis->szName)  );
		SG_ERR_CHECK(  SG_strcpy(pCtx, pThis->szName, uSize + 1u, szName)  );
	}

	// copy the contents
	if (pThat->eType != SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_uint32 uRepo = PENDING_NODE__MAX_REPOS;

		// non-directories can just copy the HIDs
		for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
		{
			const char* szHid = NULL;

			SG_ERR_CHECK(  pending_node__get_hid(pCtx, pThat, uRepo, &szHid)  );
			if (szHid != NULL)
			{
				uSize = SG_STRLEN(szHid) + 1u;
				SG_ERR_CHECK(  SG_allocN(pCtx, uSize, pThis->aHids[uRepo])  );
				SG_ERR_CHECK(  SG_strcpy(pCtx, pThis->aHids[uRepo], uSize, szHid)  );
			}
		}
	}
	else if (bRecursive != SG_FALSE && pThat->pChildren != NULL)
	{
		pending_node* pThatChild = NULL;
		SG_bool       bIterator  = SG_FALSE;

		// allocate the tree to store our children in
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->pChildren)  );

		// run through each child of the source
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThat->pChildren, &bIterator, NULL, (void**)&pThatChild)  );
		while (bIterator != SG_FALSE)
		{
			pending_node* pThisChild = NULL;

			// copy the source's child and add the copy as our own child
			SG_ERR_CHECK(  pending_node__alloc__copy(pCtx, &pThisChild, pThatChild, NULL, 0u, bRecursive)  );
			SG_ERR_CHECK(  pending_node__add_child(pCtx, pThis, pThisChild)  );

			// move on to the next
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, NULL, (void**)&pThatChild)  );
		}
	}

	*ppThis = pThis;
	pThis = NULL;

fail:
	PENDING_NODE_NULLFREE(pCtx, pThis);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	return;
}

/**
 * Allocates a pending_node from a baseline SG_treenode_entry.
 */
static void pending_node__alloc__entry(
	SG_context*              pCtx,     //< [in] [out] Error and context info.
	pending_node**           ppThis,   //< [out] Newly allocated node.
	const char*              szGid,    //< [in] GID for the new node.
	SG_uint32                uRepo,    //< [in] Index of the repo that the baseline is from.
	const SG_treenode_entry* pBaseline //< [in] Entry that will be the new node's baseline/unmodified state.
	)
{
	pending_node* pThis  = NULL;
	const char*   szTemp = NULL;

	SG_NULLARGCHECK(ppThis);
	SG_NULLARGCHECK(szGid);
	SG_ARGCHECK(uRepo < PENDING_NODE__MAX_REPOS, uRepo);
	SG_NULLARGCHECK(pBaseline);

	SG_ERR_CHECK(  pending_node__alloc(pCtx, &pThis)  );

	SG_ERR_CHECK(  SG_strcpy(pCtx, pThis->szGid, SG_GID_BUFFER_LENGTH, szGid)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pBaseline, &pThis->eType)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pBaseline, &szTemp)  );
	SG_STRDUP(pCtx, szTemp, &pThis->szBaselineName);

	SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pBaseline, &pThis->iBaselineAttributes)  );
	pThis->iAttributes = pThis->iBaselineAttributes;

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pBaseline, &szTemp)  );
	SG_STRDUP(pCtx, szTemp, &pThis->aBaselineHids[uRepo]);

	*ppThis = pThis;
	pThis = NULL;

fail:
	PENDING_NODE_NULLFREE(pCtx, pThis);
	return;
}

/**
 * Allocates a pending_node that is a brand new add and has no baseline entry.
 */
static void pending_node__alloc__new(
	SG_context*            pCtx,    //< [in] [out] Error and context info.
	pending_node**         ppThis,  //< [out] Newly allocated node.
	const char*            szName,  //< [in] Name of the new node.
	SG_uint32              uLength, //< [in] Size of szName.
	SG_treenode_entry_type eType    //< [in] Type of the new node.
	)
{
	pending_node* pThis = NULL;

	SG_NULLARGCHECK(ppThis);
	SG_NULLARGCHECK(szName);
	SG_ARGCHECK(uLength > 0u, uLength);
	SG_ARGCHECK(eType != SG_TREENODEENTRY_TYPE__INVALID, eType);

	SG_ERR_CHECK(  pending_node__alloc(pCtx, &pThis)  );

	pThis->eType = eType;

	SG_ERR_CHECK(  SG_gid__generate(pCtx, pThis->szGid, SG_GID_BUFFER_LENGTH)  );

	SG_ERR_CHECK(  SG_allocN(pCtx, uLength + 1u, pThis->szName)  );
	SG_ERR_CHECK(  SG_strncpy(pCtx, pThis->szName, uLength + 1u, szName, uLength)  );

	*ppThis = pThis;
	pThis = NULL;

fail:
	PENDING_NODE_NULLFREE(pCtx, pThis);
	return;
}

/**
 * Allocates the pending_node for the super-root of a tree.
 */
static void pending_node__alloc__super_root(
	SG_context*    pCtx,   //< [in] [out] Error and context info.
	pending_node** ppThis, //< [out] Newly allocated super-root node.
	SG_uint32      uRepo,  //< [in] Index of the repo that the HID is from.
	char**         ppHid   //< [in] HID of the blob that contains the tree's super-root node.
	)
{
	pending_node* pThis = NULL;

	SG_NULLARGCHECK(ppThis);
	SG_ARGCHECK(uRepo < PENDING_NODE__MAX_REPOS, uRepo);
	SG_NULLARGCHECK(ppHid);
	SG_NULLARGCHECK(*ppHid);

	SG_ERR_CHECK(  pending_node__alloc(pCtx, &pThis)  );

	pThis->eType = SG_TREENODEENTRY_TYPE_DIRECTORY;
	pThis->aBaselineHids[uRepo] = *ppHid;
	*ppHid = NULL;

	*ppThis = pThis;
	pThis = NULL;

fail:
	PENDING_NODE_NULLFREE(pCtx, pThis);
	return;
}

/**
 * Adds a new baseline to a pending_node from a treenode entry.
 * The node must not already have a baseline for the given repo.
 * The new baseline must match the node's existing baseline data except for its content HID.
 */
static void pending_node__add_baseline__entry(
	SG_context*              pCtx,     //< [in] [out] Error and context info.
	pending_node*            pThis,    //< [in] Node to add a baseline to.
	const char*              szGid,    //< [in] GID of the node from the new baseline.
	SG_uint32                uRepo,    //< [in] Index of the repo that the baseline is from.
	const SG_treenode_entry* pBaseline //< [in] Entry to add as a baseline.
	)
{
	SG_treenode_entry_type eType  = SG_TREENODEENTRY_TYPE__INVALID;
	const char*            szTemp = NULL;
	SG_int64               iTemp  = 0;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szGid);
	SG_ARGCHECK(strcmp(szGid, pThis->szGid) == 0, szGid);
	SG_ARGCHECK(uRepo < PENDING_NODE__MAX_REPOS, uRepo);
	SG_ARGCHECK(pThis->aBaselineHids[uRepo] == NULL, pThis);
	SG_ARGCHECK(pThis->aHids[uRepo] == NULL, pThis);
	SG_NULLARGCHECK(pBaseline);

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pBaseline, &eType)  );
	SG_ARGCHECK(eType == pThis->eType, pBaseline);

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pBaseline, &szTemp)  );
	SG_ARGCHECK(strcmp(szTemp, pThis->szBaselineName) == 0, pBaseline);

	SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pBaseline, &iTemp)  );
	SG_ARGCHECK(iTemp == pThis->iBaselineAttributes, pBaseline);

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pBaseline, &szTemp)  );
	SG_STRDUP(pCtx, szTemp, &pThis->aBaselineHids[uRepo]);

fail:
	return;
}

/**
 * Gets the GID of a pending_node.
 */
static void pending_node__get_gid(
	SG_context*         pCtx,  //< [in] [out] Error and context info.
	const pending_node* pThis, //< [in] Node to get the GID of.
	const char**        ppGid  //< [out] GID of the node.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(ppGid);

	*ppGid = pThis->szGid;

fail:
	return;
}

/**
 * Gets the type of a pending_node.
 */
static void pending_node__get_type(
	SG_context*             pCtx,  //< [in] [out] Error and context info.
	const pending_node*     pThis, //< [in] Node to get the type of.
	SG_treenode_entry_type* pType  //< [out] Type of the node.
	)
{
	SG_NULLARGCHECK(pThis);

	*pType = pThis->eType;

fail:
	return;
}

/**
 * Gets the parent of a pending_node.
 */
static void pending_node__get_parent(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	const pending_node* pThis,   //< [in] Node to get the parent of.
	pending_node**      ppParent //< [out] Parent of the node.
	)
{
	SG_NULLARGCHECK(pThis);

	*ppParent = pThis->pParent;

fail:
	return;
}

/**
 * Gets the current name of a pending_node.
 */
static void pending_node__get_name(
	SG_context*         pCtx,  //< [in] [out] Error and context info.
	const pending_node* pThis, //< [in] Node to get the name of.
	const char**        ppName //< [out] Current name of the node.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(ppName);

	if (pThis->szName != NULL)
	{
		*ppName = pThis->szName;
	}
	else if (pThis->szBaselineName != NULL)
	{
		*ppName = pThis->szBaselineName;
	}
	else
	{
		*ppName = NULL;
	}

fail:
	return;
}

/**
 * Gets the current HID of a pending_node's contents.
 */
static void pending_node__get_hid(
	SG_context*         pCtx,  //< [in] [out] Error and context info.
	const pending_node* pThis, //< [in] Node to get the contents of.
	SG_uint32           uRepo, //< [in] Index of the repo to get the contents for.
	const char**        ppHid  //< [out] HID of the node's current contents from the given repo.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(uRepo < PENDING_NODE__MAX_REPOS, uRepo);
	SG_NULLARGCHECK(ppHid);

	if (pThis->aHids[uRepo] != NULL)
	{
		*ppHid = pThis->aHids[uRepo];
	}
	else if (pThis->aBaselineHids[uRepo] != NULL)
	{
		*ppHid = pThis->aBaselineHids[uRepo];
	}
	else
	{
		*ppHid = NULL;
	}

fail:
	return;
}

/**
 * Ensures that a node has the same contents HID for all repos, and returns it.
 */
static void pending_node__get_unambiguous_hid(
	SG_context*         pCtx,  //< [in] [out] Error and context info.
	const pending_node* pThis, //< [in] Node to get the contents of.
	const char**        ppHid, //< [out] The node's single HID.
	                           //<       NULL if the node has several different HIDs.
	                           //<       May be NULL if not needed.
	SG_uint32*          pRepo  //< [out] A repo where the node has the given HID.
	                           //<       PENDING_NODE__MAX_REPOS if ppHid is NULL.
	                           //<       May be NULL if not needed.
	)
{
	SG_uint32   uCurrentRepo = 0u;
	SG_bool     bFound       = SG_FALSE;
	const char* szHid        = NULL;
	SG_uint32   uRepo        = PENDING_NODE__MAX_REPOS;

	SG_NULLARGCHECK(pThis);

	for (uCurrentRepo = 0u; uCurrentRepo < PENDING_NODE__MAX_REPOS; ++uCurrentRepo)
	{
		const char* szCurrentHid = NULL;

		SG_ERR_CHECK(  pending_node__get_hid(pCtx, pThis, uCurrentRepo, &szCurrentHid)  );
		if (szCurrentHid != NULL)
		{
			bFound = SG_TRUE;
			if (szHid == NULL)
			{
				szHid = szCurrentHid;
				uRepo = uCurrentRepo;
			}
			else if (strcmp(szHid, szCurrentHid) != 0)
			{
				szHid = NULL;
				uRepo = PENDING_NODE__MAX_REPOS;
				break;
			}
		}
	}

	SG_ASSERT(bFound != SG_FALSE);
	SG_UNUSED(bFound);

	if (ppHid != NULL)
	{
		*ppHid = szHid;
	}
	if (pRepo != NULL)
	{
		*pRepo = uRepo;
	}

fail:
	return;
}

/**
 * Sets the content HID of a pending_node, adopting a user-supplied buffer.
 */
static void pending_node__adopt_hid(
	SG_context*   pCtx,  //< [in] [out] Error and context info.
	pending_node* pThis, //< [in] Node to set the content HID of.
	                     //<      Must not be a directory node.
	SG_uint32     uRepo, //< [in] Index of the repo to adopt the HID for.
	char**        ppHid  //< [in] HID to set in the node.
	                     //<      We steal this pointer and NULL the caller's.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(pThis->eType != SG_TREENODEENTRY_TYPE_DIRECTORY, pThis);
	SG_ARGCHECK(uRepo < PENDING_NODE__MAX_REPOS, uRepo);
	SG_NULLARGCHECK(ppHid);
	SG_NULLARGCHECK(*ppHid);

	SG_NULLFREE(pCtx, pThis->aHids[uRepo]);
	pThis->aHids[uRepo] = *ppHid;
	*ppHid = NULL;

	// if we're adopting the same HID as our corresponding baseline HID
	// then we'd really rather just have a NULL pointer for our current HID
	if (pThis->aBaselineHids[uRepo] != NULL && strcmp(pThis->aBaselineHids[uRepo], pThis->aHids[uRepo]) == 0)
	{
		SG_NULLFREE(pCtx, pThis->aHids[uRepo]);
	}

fail:
	return;
}

/**
 * Adds a child node to a pending_node.
 */
static void pending_node__add_child(
	SG_context*   pCtx,  //< [in] [out] Error and context info.
	pending_node* pThis, //< [in] Node to add a new child to.
	pending_node* pChild //< [in] Child to add to the node.
	)
{
	SG_ARGCHECK(pThis->eType == SG_TREENODEENTRY_TYPE_DIRECTORY, pThis);
	SG_ARGCHECK(pThis->pChildren != NULL, pThis);

	// add the new child to our tree of children
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pThis->pChildren, pChild->szGid, (void*)pChild)  );

	// set the child's parent
	pChild->pParent = pThis;

fail:
	return;
}

/**
 * Delete a node.
 * This doesn't actually delete the node from memory, just sets a deleted flag on it.
 */
static void pending_node__delete(
	SG_context*   pCtx, //< [in] [out] Error and context info.
	pending_node* pThis //< [in] Node to mark deleted.
	)
{
	SG_NULLARGCHECK(pThis);

	pThis->bDeleted = SG_TRUE;

fail:
	return;
}

/**
 * Moves a node to a new parent, optionally changing its identity in the process.
 *
 * This is intended for use in moving a node between two unrelated trees, which is
 * why it also makes it possible to change the node's identity.  This allows one
 * node to have two different identities in two different trees, it just can't live
 * in them both simultaneously.  The caller needs to remember what it's parent and
 * identity are in each tree, and then use this function to move it back and forth
 * as needed.
 *
 * That being said, there shouldn't be any reason that this function can't be used
 * to move a node within a single tree.
 *
 * This function can also be used to remove the node from its tree by specifying
 * NULL for its new parent.  If this is done, the caller becomes the owner of the
 * node's pointer, since its old parent will no longer own it.
 */
static void pending_node__move(
	SG_context*    pCtx,       //< [in] [out] Error and context info.
	pending_node*  pThis,      //< [in] Node to move to a new parent.
	pending_node*  pParent,    //< [in] Parent to move the node to.
	                           //<      Its children must already be loaded.
	                           //<      NULL to not add the node to a new parent.
	const char*    szGid,      //< [in] GID to set on the node while being moved.
	                           //<      May be NULL to not change the GID.
	const char*    szName,     //< [in] Name to set on the node while being moved.
	                           //<      May be NULL to not change the name.
	SG_uint32      uName,      //< [in] Length of szName.
	                           //<      Ignored if szName is NULL.
	SG_bool        bPushDelete //< [in] If true, the node will be marked undeleted
	                           //<      and its children will be marked deleted instead.
	                           //<      If true, pThis must already be marked deleted and
	                           //<      its children must already be loaded.
	)
{
	SG_bool             bOwned    = SG_FALSE;
	SG_rbtree_iterator* pIterator = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(pParent == NULL || pParent->pChildren != NULL, pParent);
	SG_ARGCHECK(szGid == NULL || *szGid != '\0', szGid);
	SG_ARGCHECK(bPushDelete == SG_FALSE || pThis->bDeleted != SG_FALSE, bPushDelete);

	// remove the node from its current parent
	SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pThis->pParent->pChildren, pThis->szGid)  );
	bOwned = SG_TRUE;
	pThis->pParent = NULL;

	// if a new parent was specified, add the node to it
	// either way, we don't own the node anymore (either the caller or its new parent does)
	if (pParent != NULL)
	{
		SG_ERR_CHECK(  pending_node__add_child(pCtx, pParent, pThis)  );
	}
	bOwned = SG_FALSE;

	// if a new GID was provided, update it
	if (szGid != NULL)
	{
		// our children are indexed by GID
		// so since we're changing our GID, we'll have to update our parent's map of children as well
		SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pThis->pParent->pChildren, pThis->szGid)  );
		bOwned = SG_TRUE;
		SG_ERR_CHECK(  SG_strcpy(pCtx, pThis->szGid, SG_GID_BUFFER_LENGTH, szGid)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pThis->pParent->pChildren, pThis->szGid, (void*)pThis)  );
		bOwned = SG_FALSE;
	}

	// if a new name was provided, update it
	if (szName != NULL)
	{
		SG_ASSERT(pThis->szBaselineName != NULL || pThis->szName != NULL);
		SG_NULLFREE(pCtx, pThis->szName);
		if (pThis->szBaselineName != NULL)
		{
			SG_NULLFREE(pCtx, pThis->szBaselineName);
			SG_ERR_CHECK(  SG_allocN(pCtx, uName + 1u, pThis->szBaselineName)  );
			SG_ERR_CHECK(  SG_strncpy(pCtx, pThis->szBaselineName, uName + 1u, szName, uName)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_allocN(pCtx, uName + 1u, pThis->szName)  );
			SG_ERR_CHECK(  SG_strncpy(pCtx, pThis->szName, uName + 1u, szName, uName)  );
		}
	}

	// if we need to push a marked delete down, do that
	if (bPushDelete != SG_FALSE)
	{
		SG_bool       bIterator = SG_FALSE;
		pending_node* pChild    = NULL;

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pChildren, &bIterator, NULL, (void**)&pChild)  );
		while (bIterator != SG_FALSE)
		{
			SG_ERR_CHECK(  pending_node__delete(pCtx, pChild)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, NULL, (void**)&pChild)  );
		}
		pThis->bDeleted = SG_FALSE;
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	if (bOwned != SG_FALSE)
	{
		PENDING_NODE_NULLFREE(pCtx, pThis);
	}
	return;
}

/**
 * Builds the node's full path.
 */
static void pending_node__build_path(
	SG_context*   pCtx,  //< [in] [out] Error and context info.
	pending_node* pThis, //< [in] Node to build the path of.
	char**        ppPath //< [out] Full path of the node.
	                     //<       NULL for super-root nodes.
	)
{
	char* szPath = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(ppPath);

	if (pThis->pParent != NULL)
	{
		pending_node* pCurrent  = NULL;
		SG_uint32     uTotal    = 0u;
		char*         szCurrent = NULL;

		// determine the total length we need for the path
		pCurrent = pThis;
		while (pCurrent->pParent != NULL)
		{
			const char* szName = NULL;

			// add in the length of this ancestor's name
			SG_ERR_CHECK(  pending_node__get_name(pCtx, pCurrent, &szName)  );
			SG_ASSERT(szName != NULL);
			uTotal += SG_STRLEN(szName) + 1u; // extra 1 for trailing separator/terminator

			// move up to the next parent
			pCurrent = pCurrent->pParent;
		}
		SG_ASSERT(pCurrent->szBaselineName == NULL);
		SG_ASSERT(pCurrent->szName == NULL);

		// allocate a string large enough
		SG_ERR_CHECK(  SG_allocN(pCtx, uTotal, szPath)  );

		// start at the end of the string and copy each component into place backward
		szCurrent = szPath + uTotal;
		pCurrent = pThis;
		while (pCurrent->pParent != NULL)
		{
			const char* szName   = NULL;
			SG_uint32   uCurrent = 0u;

			// we should never end up falling off the beginning of our total path
			SG_ASSERT(szCurrent > szPath);

			// place the terminator/separator after this component
			--szCurrent;
			if (pCurrent == pThis)
			{
				*szCurrent = '\0';
			}
			else
			{
				*szCurrent = '/';
			}

			// back up to where the component goes and copy it there
			SG_ERR_CHECK(  pending_node__get_name(pCtx, pCurrent, &szName)  );
			uCurrent = SG_STRLEN(szName);
			szCurrent -= uCurrent;
			memcpy((void*)szCurrent, (const void*)szName, uCurrent);

			// move up to the next parent
			pCurrent = pCurrent->pParent;
		}

		// we should end up back at the beginning of the path
		SG_ASSERT(szCurrent == szPath);
	}

	// return the built path
	*ppPath = szPath;
	szPath = NULL;

fail:
	SG_NULLFREE(pCtx, szPath);
	return;
}

/**
 * Creates pending_nodes for all the files and folders within a directory pending_node.
 * The list of children is found in the blob specified by the node's current HID.
 * The created nodes are added as children of the specified node, as you'd expect.
 * This function will silently do nothing on nodes which aren't directories.
 * This function is safe to call multiple times, it will only do any work the first time.
 */
static void pending_node__load_children(
	SG_context*                  pCtx,         //< [in] [out] Error and context info.
	pending_node*                pThis,        //< [in] Node to load children for.
	SG_bool                      bRecursive,   //< [in] Whether or not to recursively load the childrens' children.
	pending_node__get_repo_data* fGetRepoData, //< [in] Function to translate repo indices into repos.
	void*                        pGetRepoData  //< [in] Caller data to pass to fGetRepo.
	)
{
	SG_treenode*        pTreeNode   = NULL;
	pending_node*       pChild      = NULL;
	SG_bool             bChildOwned = SG_FALSE;
	SG_rbtree_iterator* pIterator   = NULL;

	SG_NULLARGCHECK(pThis);

	// only directories have children to load
	if (pThis->eType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		// only bother loading our children if we haven't yet done so
		if (pThis->pChildren == NULL)
		{
			SG_uint32 uRepo                           = PENDING_NODE__MAX_REPOS;
			SG_repo*  aRepos[PENDING_NODE__MAX_REPOS] = { NULL, };

			// allocate the tree to store our children in
			SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->pChildren)  );

			// run through each repo
			for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
			{
				const char* szHid = NULL;

				// find the HID to load from this repo
				SG_ERR_CHECK(  pending_node__get_hid(pCtx, pThis, uRepo, &szHid)  );
				if (szHid != NULL)
				{
					SG_uint32 uChildren = 0u;
					SG_uint32 uChild    = 0u;

					// get the actual repo pointer if we don't have it yet
					if (aRepos[uRepo] == NULL)
					{
						SG_ERR_CHECK(  fGetRepoData(pCtx, uRepo, pGetRepoData, &aRepos[uRepo], NULL, NULL)  );
					}

					// load our treenode from the repo and run through each of its entries
					SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, aRepos[uRepo], szHid, &pTreeNode)  );
					SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreeNode, &uChildren)  );
					for (uChild = 0u; uChild < uChildren; ++uChild)
					{
						const char*              szGid  = NULL;
						const SG_treenode_entry* pEntry = NULL;
						const char*              szName = NULL;

						// get the current entry
						SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreeNode, uChild, &szGid, &pEntry)  );
						SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &szName)  );

						// check if we already have a child with this entry's name
						SG_ERR_CHECK(  pending_node__get_child(pCtx, pThis, szName, SG_STRLEN(szName), PENDING_NODE__INACTIVE, NULL, NULL, &pChild)  );
						if (pChild == NULL)
						{
							// no child with this name yet, create it
							SG_ERR_CHECK(  pending_node__alloc__entry(pCtx, &pChild, szGid, uRepo, pEntry)  );
							bChildOwned = SG_TRUE;
							SG_ERR_CHECK(  pending_node__add_child(pCtx, pThis, pChild)  );
							bChildOwned = SG_FALSE;
						}
						else
						{
							// there's already a child with this name, add a baseline to it
							SG_ERR_CHECK(  pending_node__add_baseline__entry(pCtx, pChild, szGid, uRepo, pEntry)  );
						}
					}

					// free memory for the next loop
					SG_TREENODE_NULLFREE(pCtx, pTreeNode);
				}
			}
		}

		// if we're loading recursively, load our children too
		if (bRecursive != SG_FALSE)
		{
			SG_bool bIterator = SG_FALSE;

			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pChildren, &bIterator, NULL, (void**)&pChild)  );
			while (bIterator != SG_FALSE)
			{
				SG_ERR_CHECK(  pending_node__load_children(pCtx, pChild, bRecursive, fGetRepoData, pGetRepoData)  );
				SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, NULL, (void**)&pChild)  );
			}
		}
	}


fail:
	SG_TREENODE_NULLFREE(pCtx, pTreeNode);
	if (bChildOwned != SG_FALSE)
	{
		PENDING_NODE_NULLFREE(pCtx, pChild);
	}
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	return;
}

/**
 * Finds a child of a pending_node by its name.
 * Optionally calls load_children if necessary, to ensure all children are available.
 */
static void pending_node__get_child(
	SG_context*                  pCtx,         //< [in] [out] Error and context info.
	pending_node*                pThis,        //< [in] Node to get a child of.
	const char*                  szName,       //< [in] Name of the child to get.
	SG_uint32                    uLength,      //< [in] Size of szName.
	SG_uint32                    uFlags,       //< [in] Flags that customize behavior.
	                                           //<      INACTIVE: Find children that are deleted or moved away.
	pending_node__get_repo_data* fGetRepoData, //< [in] Function to translate repo indices into repos.
	                                           //<      If NULL, no new children will be loaded.
	void*                        pGetRepoData, //< [in] Caller data to pass to fGetRepo.
	                                           //<      Ignored if fGetRepo is NULL.
	pending_node**               ppChild       //< [out] The found child.
	                                           //<       NULL if none was found.
	)
{
	pending_node*       pChild    = NULL;
	SG_rbtree_iterator* pIterator = NULL;

	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(pThis->eType == SG_TREENODEENTRY_TYPE_DIRECTORY, pThis);
	SG_NULLARGCHECK(szName);
	SG_ARGCHECK(uLength > 0u, uLength);
	SG_NULLARGCHECK(ppChild);

	// if we don't have children and could try loading them, do it
	if (pThis->pChildren == NULL && fGetRepoData != NULL)
	{
		SG_ERR_CHECK(  pending_node__load_children(pCtx, pThis, SG_FALSE, fGetRepoData, pGetRepoData)  );
	}

	// if we have children, search through them
	if (pThis->pChildren != NULL)
	{
		SG_bool       bIterator = SG_FALSE;
		pending_node* pCurrent  = NULL;

		// run through each child
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pChildren, &bIterator, NULL, (void**)&pCurrent)  );
		while (bIterator != SG_FALSE)
		{
			// if the child is deleted or moved away,
			// ignore it unless the caller specified INACTIVE
			if (
				   HAS_ANY_FLAG(uFlags, PENDING_NODE__INACTIVE)
				|| (
					   pCurrent->bDeleted == SG_FALSE
					&& pCurrent->pMoveTarget == NULL
					)
				)
			{
				const char* szCurrentName = NULL;

				// check if the child's name matches the one we're looking for
				SG_ERR_CHECK(  pending_node__get_name(pCtx, pCurrent, &szCurrentName)  );
				if (szCurrentName[uLength] == '\0' && strncmp(szName, szCurrentName, uLength) == 0)
				{
					pChild = pCurrent;
					break;
				}
			}

			// move on to the next child
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, NULL, (void**)&pCurrent)  );
		}
	}

	// return whatever we found
	*ppChild = pChild;
	pChild = NULL;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	return;
}

/**
 * Finds a node in a tree using a file path.
 * Optionally loads necessary nodes from repos as it walks.
 */
static void pending_node__walk_path(
	SG_context*                  pCtx,         //< [in] [out] Error and context info.
	pending_node*                pThis,        //< [in] Node to start from.
	const char*                  szPath,       //< [in] Path of the node to find.
	SG_uint32                    uLength,      //< [in] Size of szPath.
	SG_uint32                    uFlags,       //< [in] Flags to customize behavior.
	                                           //<      RESTORE_DELETED: Clear the deleted flag of any node in the path.
	                                           //<      ADD_MISSING: Create new directory nodes as needed to get to the end of the path.
	pending_node__get_repo_data* fGetRepoData, //< [in] Function to translate repo indices into repos.
	                                           //<      If NULL, no new children will be loaded.
	void*                        pGetRepoData, //< [in] Caller data to pass to fGetRepo.
	                                           //<      Ignored if fGetRepo is NULL.
	pending_node**               ppPendingNode //< [out] The found node.
	                                           //<       NULL if none was found.
	)
{
	pending_node* pPendingNode = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(ppPendingNode);

	if (uLength == 0u)
	{
		// we've reached the end of the path and are done walking
		pPendingNode = pThis;
	}
	else
	{
		SG_uint32 uCurrentLength = 0u;

		SG_NULLARGCHECK(*szPath);

		// make sure the node is a directory before we try walking into it
		if (pThis->eType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			SG_ERR_THROW2(SG_ERR_NOT_A_DIRECTORY, (pCtx, "Non-directory (%s) node found in the middle of a [partial] path: %s", gaPendingNodeType_String[pThis->eType], szPath));
		}

		// get the length of the first component of the path
		uCurrentLength = (SG_uint32)strcspn(szPath, gszImport__PathSeparators);
		if (uCurrentLength == 0u)
		{
			// this only happens if there are adjacent separators
			// just recurse into this node again using the next path component
			pPendingNode = pThis;
		}
		else
		{
			// find an active child named after the first path component
			SG_ERR_CHECK(  pending_node__get_child(pCtx, pThis, szPath, uCurrentLength, uFlags, fGetRepoData, pGetRepoData, &pPendingNode)  );
			if (pPendingNode == NULL && HAS_ANY_FLAG(uFlags, PENDING_NODE__RESTORE_DELETED))
			{
				// no active nodes, but we're allowed to restore deleted ones
				// check for inactive nodes that are deleted
				SG_ERR_CHECK(  pending_node__get_child(pCtx, pThis, szPath, uCurrentLength, PENDING_NODE__INACTIVE, fGetRepoData, pGetRepoData, &pPendingNode)  );
				if (pPendingNode != NULL)
				{
					SG_ASSERT((pPendingNode->bDeleted != SG_FALSE) != (pPendingNode->pMoveTarget != NULL));
					if (pPendingNode->bDeleted != SG_FALSE)
					{
						// restore the deleted node and use it
						pPendingNode->bDeleted = SG_FALSE;
					}
					else
					{
						// it's inactive for another reason, don't use it
						pPendingNode = NULL;
					}
				}
			}
			if (pPendingNode == NULL && HAS_ANY_FLAG(uFlags, PENDING_NODE__ADD_MISSING))
			{
				// no node found, but we're allowed to add missing ones
				SG_ERR_CHECK(  pending_node__alloc__new(pCtx, &pPendingNode, szPath, uCurrentLength, SG_TREENODEENTRY_TYPE_DIRECTORY)  );
				SG_ERR_CHECK(  pending_node__add_child(pCtx, pThis, pPendingNode)  );
			}
		}

		// if we have a node and more path, walk into it recursively
		if (pPendingNode != NULL && uCurrentLength < uLength)
		{
			SG_ERR_CHECK(  pending_node__walk_path(pCtx, pPendingNode, szPath + uCurrentLength + 1u, uLength - uCurrentLength - 1u, uFlags, fGetRepoData, pGetRepoData, &pPendingNode)  );
		}
	}

	// return what we found
	*ppPendingNode = pPendingNode;
	pPendingNode = NULL;

fail:
	return;
}

/**
 * Commit a node and its children to a repo, if dirty.
 * Internal recursive helper for the main commit function.
 */
static void pending_node__commit__internal(
	SG_context*                   pCtx,           //< [in] [out] Error and context info.
	pending_node*                 pThis,          //< [in] Node to commit.
	pending_node__commit_to_repo* fCommitToRepo,  //< [in] Function to determine which nodes to commit to which repos.
	void*                         pCommitToRepo,  //< [in] Data to pass to fCommitToRepo.
	SG_repo**                     ppRepos,        //< [in] Array mapping repo index to actual repo.
	SG_repo_tx_handle**           ppTransactions, //< [in] Array mapping repo index to active transaction.
	SG_changeset**                ppChangesets,   //< [in] Array mapping repo index to current changeset.
	SG_byte*                      pCopyBuffer,    //< [in] A buffer to use when copying a blob from one repo to another.
	SG_uint32                     uCopyBuffer,    //< [in] Size of pCopyBuffer.
	SG_bool*                      pCommit,        //< [out] Array mapping repo index to whether or not the node was committed to that repo.
	SG_bool*                      pDirty          //< [out] Array mapping repo index to whether or not the node was dirty on that repo.
	)
{
	SG_uint32           uRepo                               = PENDING_NODE__MAX_REPOS;
	char*               szPath                              = NULL;
	SG_bool             aCommit[PENDING_NODE__MAX_REPOS]    = { SG_FALSE, };
	SG_bool             aDirty[PENDING_NODE__MAX_REPOS]     = { SG_FALSE, };
	SG_treenode*        aTreeNodes[PENDING_NODE__MAX_REPOS] = { NULL, };
	SG_rbtree_iterator* pIterator                           = NULL;
	SG_treenode_entry*  pEntry                              = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(fCommitToRepo);
	SG_NULLARGCHECK(ppRepos);
	SG_NULLARGCHECK(ppTransactions);
	SG_NULLARGCHECK(ppChangesets);
	SG_NULLARGCHECK(pCommit);
	SG_NULLARGCHECK(pDirty);

	// build the path to this node
	SG_ERR_CHECK(  pending_node__build_path(pCtx, pThis, &szPath)  );

	// check how this node relates to each repo
	for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
	{
		// check if the node should be committed to this repo
		SG_ERR_CHECK(  fCommitToRepo(pCtx, pThis, szPath, uRepo, pCommitToRepo, &aCommit[uRepo])  );
		if (aCommit[uRepo] != SG_FALSE)
		{
			// check if the node is dirty in this repo
			if (
				   (pThis->bDeleted != SG_FALSE)                             // deleted
				|| (pThis->pMoveSource != NULL)                              // moved here
				|| (pThis->pMoveTarget != NULL)                              // moved away
				|| (pThis->szBaselineName == NULL && pThis->pParent != NULL) // added and non-super-root
				|| (pThis->szName != NULL)                                   // name changed
				|| (pThis->iAttributes != pThis->iBaselineAttributes)        // attributes changed
				|| (pThis->aHids[uRepo] != NULL)                             // content changed
				)
			{
				aDirty[uRepo] = SG_TRUE;
			}
		}
	}

	// if this is a directory, build and commit treenodes to appropriate repos
	if (pThis->eType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		// start a treenode for each repo this folder needs to be committed to
		for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
		{
			// no directories should have a current HID before being committed
			SG_ASSERT(pThis->aHids[uRepo] == NULL);

			// if we're committing to this repo, start a treenode
			if (aCommit[uRepo] != SG_FALSE)
			{
				SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &aTreeNodes[uRepo])  );
				SG_ERR_CHECK(  SG_treenode__set_version(pCtx, aTreeNodes[uRepo], SG_TN_VERSION__CURRENT)  );
			}
		}

		// if we have children, process them
		if (pThis->pChildren != NULL)
		{
			SG_bool       bIterator = SG_FALSE;
			const char*   szGid     = NULL;
			pending_node* pChild    = NULL;

			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pChildren, &bIterator, &szGid, (void**)&pChild)  );
			while (bIterator != SG_FALSE)
			{
				SG_bool aChildCommit[PENDING_NODE__MAX_REPOS];
				SG_bool aChildDirty[PENDING_NODE__MAX_REPOS];

				// recursively commit the child
				SG_ERR_CHECK(  pending_node__commit__internal(pCtx, pChild, fCommitToRepo, pCommitToRepo, ppRepos, ppTransactions, ppChangesets, pCopyBuffer, uCopyBuffer, aChildCommit, aChildDirty)  );

				// process this child across each repo
				for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
				{
					// if the child got committed to this repo
					// we also have to be committed to this repo
					SG_ASSERT(aChildCommit[uRepo] == SG_FALSE || aCommit[uRepo] != SG_FALSE);
					// If this trips, then fCommitToRepo said that the child should
					// be committed to this repo, but we (the parent) shouldn't be.
					// That's bad behavior for fCommitToRepo, because it would
					// result in a disconnected tree for this repo.

					// if the child was dirty in this repo, then we are too
					if (aChildDirty[uRepo] != SG_FALSE)
					{
						aDirty[uRepo] = SG_TRUE;
					}

					// if this child needs a tree node entry in this repo, create one
					if (
						   aChildCommit[uRepo] != SG_FALSE // the child is committed in this repo
						&& pChild->bDeleted == SG_FALSE    // the child hasn't been deleted
						&& pChild->pMoveTarget == NULL     // the child hasn't been moved away
						)
					{
						const char* szName = NULL;
						const char* szHid  = NULL;

						// aChildCommit[uRepo] is true
						// which means aCommit[uRepo] must also be true (as asserted earlier)
						// which means we must have started a treenode for it
						SG_ASSERT(aTreeNodes[uRepo] != NULL);

						// get the child's data
						SG_ERR_CHECK(  pending_node__get_name(pCtx, pChild, &szName)  );
						SG_ERR_CHECK(  pending_node__get_hid(pCtx, pChild, uRepo, &szHid)  );
						if (szHid == NULL)
						{
							// the child doesn't have a HID in this repo
							SG_uint32   uSourceRepo  = PENDING_NODE__MAX_REPOS;
							const char* szSourceHid  = NULL;
							SG_uint32   uCurrentRepo = PENDING_NODE__MAX_REPOS;

							// the child must currently exist only in another repo
							// figure out which one(s)
							for (uCurrentRepo = 0u; uCurrentRepo < PENDING_NODE__MAX_REPOS; ++uCurrentRepo)
							{
								if (uCurrentRepo != uRepo)
								{
									const char* szCurrentHid = NULL;

									SG_ERR_CHECK(  pending_node__get_hid(pCtx, pChild, uCurrentRepo, &szCurrentHid)  );
									if (szCurrentHid != NULL)
									{
										if (szSourceHid == NULL)
										{
											// found it
											uSourceRepo = uCurrentRepo;
											szSourceHid = szCurrentHid;
										}
										else
										{
											// child has a HID in multiple other repos
											// This should never happen when PENDING_NODE__MAX_REPOS is 2u.
											// Since the liklihood of that changing is very slim, I'm not
											// going to try and support it right now.  I think that
											// supporting it would require the following:
											// 1) Check if all the HIDs reference identical content.
											//    Since different repos might use different algorithms, any
											//    HIDs that don't already match will have to be re-hashed
											//    with a common algorithm to check if they truly reference
											//    identical content.  Trim the set down to only those HIDs
											//    that actually have unique content.
											// 2) For files, that you must be left with only one.
											//    It shouldn't be possible for one file to have different
											//    content in different repos for the same revision.
											//    Copy that one blob into this repo (from an arbitrary
											//    one of the sources) and you're done.
											// 3) For directories, you might still have several HIDs left.
											//    Ensure that each of those HIDs references a treenode
											//    whose contents are identical EXCEPT for children.
											//    Finally, use a new/custom multi-source copy below to
											//    generate a treenode in the repo that is basically a
											//    union of each of the source treenodes.  One possible
											//    gotcha with the union is if some of the source nodes have
											//    children with the same GID and/or name.  Children with
											//    the same GID should probably have identical data, but
											//    children with the same name and different GIDs I'm not
											//    sure about.
											SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);
										}
									}
								}
							}
							SG_ASSERT(szSourceHid != NULL);

							// copy the child's content blob from the repo where it
							// currently is into this one so that we can reference
							// it in the entry we're creating for the child in
							// this repo
							if (pChild->eType == SG_TREENODEENTRY_TYPE_DIRECTORY)
							{
								SG_ERR_CHECK(  import__copy_treenode_blob(pCtx, szSourceHid, ppRepos[uSourceRepo], ppRepos[uRepo], ppTransactions[uRepo], ppChangesets[uRepo], pCopyBuffer, uCopyBuffer, &pChild->aHids[uRepo])  );
							}
							else
							{
								SG_ERR_CHECK(  import__copy_blob(pCtx, szSourceHid, ppRepos[uSourceRepo], ppRepos[uRepo], ppTransactions[uRepo], pCopyBuffer, uCopyBuffer, &pChild->aHids[uRepo])  );
							}

							// now the child has a HID in this repo
							SG_ERR_CHECK(  pending_node__get_hid(pCtx, pChild, uRepo, &szHid)  );
						}
						SG_ASSERT(szHid != NULL);

						// create the child's entry and add it to the treenode for this repo
						SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC(pCtx, &pEntry)  );
						SG_ERR_CHECK(  SG_treenode_entry__set_entry_name(pCtx, pEntry, szName)  );
						SG_ERR_CHECK(  SG_treenode_entry__set_entry_type(pCtx, pEntry, pChild->eType)  );
						SG_ERR_CHECK(  SG_treenode_entry__set_hid_blob(pCtx, pEntry, szHid)  );
						SG_ERR_CHECK(  SG_treenode_entry__set_attribute_bits(pCtx, pEntry, pChild->iAttributes)  );
						SG_ERR_CHECK(  SG_treenode__add_entry(pCtx, aTreeNodes[uRepo], pChild->szGid, &pEntry)  );
					}
				}

				// move on to the next child
				SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szGid, (void**)&pChild)  );
			}
		}

		// save the treenodes for any repos in which we're dirty
		for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
		{
			if (aTreeNodes[uRepo] != NULL && aDirty[uRepo] != SG_FALSE)
			{
				SG_uint64 uBlobLength = 0u;

				// save the node to the repo and add it to the changeset
				SG_ERR_CHECK(  SG_treenode__save_to_repo(pCtx, aTreeNodes[uRepo], ppRepos[uRepo], ppTransactions[uRepo], &uBlobLength)  );
				SG_ERR_CHECK(  SG_treenode__get_id(pCtx, aTreeNodes[uRepo], &pThis->aHids[uRepo])  );
				SG_ERR_CHECK(  SG_changeset__tree__add_treenode(pCtx, ppChangesets[uRepo], pThis->aHids[uRepo], &aTreeNodes[uRepo])  );

				// if we ended up with the same HID we had in the baseline
				// then we'd rather just have a NULL pointer
				if (pThis->aBaselineHids[uRepo] != NULL && strcmp(pThis->aHids[uRepo], pThis->aBaselineHids[uRepo]) == 0)
				{
					SG_NULLFREE(pCtx, pThis->aHids[uRepo]);
				}
			}
		}
	}

	// return output
	for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
	{
		pCommit[uRepo] = aCommit[uRepo];
		pDirty[uRepo]  = aDirty[uRepo];
	}

fail:
	SG_NULLFREE(pCtx, szPath);
	for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
	{
		SG_TREENODE_NULLFREE(pCtx, aTreeNodes[uRepo]);
	}
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pEntry);
	return;
}

/**
 * Commit a tree to one or more repos.
 */
static void pending_node__commit(
	SG_context*                   pCtx,           //< [in] [out] Error and context info.
	pending_node*                 pThis,          //< [in] Super-root node to commit.
	pending_node__get_repo_data*  fGetRepoData,   //< [in] Function to translate repo indices into actual repo data.
	void*                         pGetRepoData,   //< [in] Data to pass to fGetRepoData.
	pending_node__commit_to_repo* fCommitToRepo,  //< [in] Function to determine which nodes to commit to which repos.
	void*                         pCommitToRepo,  //< [in] Data to pass to fCommitToRepo.
	SG_byte*                      pCopyBuffer,    //< [in] A buffer to use when copying a blob from one repo to another.
	                                              //<      If NULL, a buffer is allocated internally if needed.
	SG_uint32                     uCopyBuffer,    //< [in] Size of pCopyBuffer.
	                                              //<      Ignored if pCopyBuffer is NULL.
	SG_bool*                      pDirty          //< [out] Whether or not the node was dirty in each repo.
	                                              //<       Must be an array at least PENDING_NODE__MAX_REPOS in length.
	                                              //<       Indices into the array match repo indices used with the callbacks.
	                                              //<       May be NULL if not needed.
	)
{
	SG_uint32          uRepo                                  = PENDING_NODE__MAX_REPOS;
	SG_repo*           aRepos[PENDING_NODE__MAX_REPOS]        = { NULL, };
	SG_repo_tx_handle* aTransactions[PENDING_NODE__MAX_REPOS] = { NULL, };
	SG_changeset*      aChangesets[PENDING_NODE__MAX_REPOS]   = { NULL, };
	SG_uint32          uRepos                                 = 0u;
	SG_bool            bCopyBufferOwned                       = SG_FALSE;
	SG_bool            aCommit[PENDING_NODE__MAX_REPOS]       = { SG_FALSE, };
	SG_bool            aDirty[PENDING_NODE__MAX_REPOS]        = { SG_FALSE, };

	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(pThis->szGid[0u] == '\0', pThis);
	SG_ARGCHECK(pThis->eType == SG_TREENODEENTRY_TYPE_DIRECTORY, pThis);
	SG_ARGCHECK(pThis->szBaselineName == NULL, pThis);
	SG_ARGCHECK(pThis->iBaselineAttributes == 0, pThis);
	SG_ARGCHECK(pThis->szName == NULL, pThis);
	SG_ARGCHECK(pThis->iAttributes == 0, pThis);
	SG_ARGCHECK(pThis->pParent == NULL, pThis);
	SG_NULLARGCHECK(fGetRepoData);
	SG_NULLARGCHECK(fCommitToRepo);

	// get all the info we need for each repo
	for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
	{
		SG_ERR_CHECK(  fGetRepoData(pCtx, uRepo, pGetRepoData, &aRepos[uRepo], &aTransactions[uRepo], &aChangesets[uRepo])  );
		if (aRepos[uRepo] != NULL)
		{
			++uRepos;
		}
	}
	SG_ARGCHECK(uRepos > 0u, fGetRepoData);

	// if there are multiple repos, we might need a copy buffer
	if (uRepos > 1u && pCopyBuffer == NULL)
	{
		uCopyBuffer = SG_STREAMING_BUFFER_SIZE;
		SG_ERR_CHECK(  SG_allocN(pCtx, uCopyBuffer, pCopyBuffer)  );
		bCopyBufferOwned = SG_TRUE;
	}

	// do the recursive commit
	SG_ERR_CHECK(  pending_node__commit__internal(pCtx, pThis, fCommitToRepo, pCommitToRepo, aRepos, aTransactions, aChangesets, pCopyBuffer, uCopyBuffer, aCommit, aDirty)  );

	// return dirty data, if they want it
	if (pDirty != NULL)
	{
		for (uRepo = 0u; uRepo < PENDING_NODE__MAX_REPOS; ++uRepo)
		{
			pDirty[uRepo] = aDirty[uRepo];
		}
	}

fail:
	if (bCopyBufferOwned != SG_FALSE)
	{
		SG_NULLFREE(pCtx, pCopyBuffer);
	}
	return;
}


/*
**
** SVN Diff
**
*/

/**
 * A self-managing buffer used while processing an svndiff.
 */
typedef struct svndiff__buffer
{
	SG_byte*  pBytes;     //< The actual bytes of the buffer.
	SG_uint32 uBytesSize; //< Size of pBytes (highest value uSize has ever had).
	SG_bool   bOwned;     //< Whether or not pBytes is owned by the instance.
	SG_uint32 uSize;      //< Current size of the buffer.
	SG_uint32 uPosition;  //< Current index into pBytes.
}
svndiff__buffer;

/**
 * Initializer for an svndiff__buffer.
 */
#define SVNDIFF__BUFFER__INIT { NULL, 0u, SG_FALSE, 0u, 0u }

/**
 * Wrapper around various possible data sources, allowing svndiff processing
 * to read from any of them in the same manner.  Only one of the possible sources
 * is ever non-NULL.
 */
typedef struct svndiff__stream
{
	svndiff__buffer*     pBuffer;             //< Buffer to read data from.
	                                          //< NULL if this is not the stream's source.
	SG_readstream*       pReadStream;         //< SG_readstream to read data from.
	                                          //< NULL if this is not the stream's source.
	buffered_readstream* pBufferedReadStream; //< buffered_readstream to read data from.
	                                          //< NULL if this is not the stream's source.
	SG_uint64            uSize;               //< Size of the data source, or zero if unknown.
	SG_uint64            uProcessed;          //< Bytes read from the source so far.
}
svndiff__stream;

/**
 * Initializer for an svndiff__stream.
 */
#define SVNDIFF__STREAM__INIT { NULL, NULL, NULL, 0u, 0u }

/**
 * Enumeration of possible delta operations.
 * Note: The values for this enumeration are from the svndiff format.
 */
typedef enum svndiff__instruction
{
	SVNDIFF__INSTRUCTION__SOURCE   = 0, //< Copy bytes from the source.
	SVNDIFF__INSTRUCTION__TARGET   = 1, //< Copy bytes from the target.
	SVNDIFF__INSTRUCTION__NEW_DATA = 2, //< Copy bytes from the window's new data.
	SVNDIFF__INSTRUCTION__COUNT         //< Number of values in the enum, for iteration.
	                                         //< Also used as an invalid value.
}
svndiff__instruction;

/**
 * Frees a buffer.
 */
static void svndiff__buffer__free(
	SG_context*      pCtx, //< [in] [out] Error and context info.
	svndiff__buffer* pThis //< [in] Buffer to free.
	)
{
	if (pThis != NULL)
	{
		if (pThis->bOwned != SG_FALSE)
		{
			SG_NULLFREE(pCtx, pThis->pBytes);
		}
	}
}

/**
 * Checks if a buffer's current position is at the end.
 */
static void svndiff__buffer__end(
	SG_context*      pCtx,  //< [in] [out] Error and context info.
	svndiff__buffer* pThis, //< [in] Buffer to check.
	SG_bool*         pEnd   //< [out] Whether or not the buffer's current position is at its end.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pEnd);

	SG_ASSERT(pThis->uPosition <= pThis->uSize);
	*pEnd = (pThis->uPosition == pThis->uSize ? SG_TRUE : SG_FALSE);

fail:
	return;
}

/**
 * Resets a buffer, basically like a constructor.
 *
 * Resets the buffer's current position to the beginning.
 *
 * If a byte range is specified, the buffer wraps it but does not take ownership.
 * It will use those bytes as long as they're large enough.
 *
 * If no byte range is specified, we ensure that the buffer is large enough for
 * the given size, re-allocating if necessary.
 */
static void svndiff__buffer__reset(
	SG_context*      pCtx,   //< [in] [out] Error and context info.
	svndiff__buffer* pThis,  //< [in] Buffer to reset.
	SG_byte*         pBytes, //< [in] Bytes to wrap and use internally as long as possible.
	                         //<      If NULL, the buffer will maintain its own bytes.
	SG_uint32        uSize   //< [in] If pBytes is non-NULL, the size of pBytes.
	                         //<      Otherwise, ensure the internal bytes are at least this large.
	)
{
	SG_NULLARGCHECK(pThis);

	// check if we need a new buffer
	if (pBytes != NULL || uSize > pThis->uBytesSize)
	{
		// we do, free the one we have now
		if (pThis->bOwned != SG_FALSE)
		{
			SG_NULLFREE(pCtx, pThis->pBytes);
		}

		if (pBytes != NULL)
		{
			// we're adopting someone else's buffer
			pThis->pBytes     = pBytes;
			pThis->uBytesSize = uSize;
			pThis->bOwned     = SG_FALSE;
		}
		else
		{
			// we're allocating our own buffer
			pThis->uBytesSize = uSize;
			SG_ERR_CHECK(  SG_allocN(pCtx, pThis->uBytesSize, pThis->pBytes)  );
			pThis->bOwned = SG_TRUE;
		}
	}

	// reset our current data
	pThis->uSize     = uSize;
	pThis->uPosition = 0u;

fail:
	return;
}

/**
 * Reads data from the buffer's current position and advances it.
 */
static void svndiff__buffer__read(
	SG_context*      pCtx,   //< [in] [out] Error and context info.
	svndiff__buffer* pThis,  //< [in] Buffer to read from.
	SG_byte*         pBytes, //< [in] Bytes to read into.
	SG_uint32        uBytes, //< [in] Size of pBytes.
	SG_uint32*       pRead   //< [out] Number of bytes actually read.
	                         //<       May be NULL if not needed.
	)
{
	SG_uint32 uGot = 0u;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pBytes);

	if (uBytes > 0u)
	{
		uGot = SG_MIN(uBytes, pThis->uSize - pThis->uPosition);
		memcpy((void*)pBytes, (const void*)(pThis->pBytes + pThis->uPosition), uGot);
		pThis->uPosition += uGot;
	}

	if (pRead != NULL)
	{
		*pRead = uGot;
	}

fail:
	return;
}

/**
 * Gets the entire range of bytes underlying the buffer.
 * Has no effect on the buffer's current position.
 */
static void svndiff__buffer__get_bytes__all(
	SG_context*      pCtx,    //< [in] [out] Error and context info.
	svndiff__buffer* pThis,   //< [in] Buffer to get bytes from.
	SG_byte**        ppBytes, //< [in] Bytes underlying the buffer.
	SG_uint32*       pSize    //< [in] Size of the returned bytes.
	)
{
	SG_NULLARGCHECK(pThis);

	if (ppBytes != NULL)
	{
		*ppBytes = pThis->pBytes;
	}
	if (pSize != NULL)
	{
		*pSize = pThis->uSize;
	}

fail:
	return;
}

/**
 * Gets a subset of the bytes underlying the buffer.
 * Has no effect on the buffer's current position.
 */
static void svndiff__buffer__get_bytes__random(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	svndiff__buffer* pThis,     //< [in] Buffer to get bytes from.
	SG_uint32        uPosition, //< [in] Position of the first byte to get.
	SG_uint32        uSize,     //< [in] Number of bytes to get.
	SG_byte**        ppBytes    //< [out] The requested bytes.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(uPosition + uSize <= pThis->uSize, uPosition|uSize);
	SG_NULLARGCHECK(ppBytes);

	*ppBytes = pThis->pBytes + uPosition;

fail:
	return;
}

/**
 * Gets a subset of the bytes underlying the buffer, starting at its current position.
 * May optionally advance the current position past the returned bytes.
 */
static void svndiff__buffer__get_bytes__current(
	SG_context*      pCtx,     //< [in] [out] Error and context info.
	svndiff__buffer* pThis,    //< [in] Buffer to get bytes from.
	SG_uint32        uSize,    //< [in] Number of bytes to get.
	SG_bool          bAdvance, //< [in] Whether or not the current position should advance past the returned bytes.
	SG_byte**        ppBytes   //< [out] The requested bytes.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(pThis->uPosition + uSize <= pThis->uSize, uSize);
	SG_NULLARGCHECK(ppBytes);

	*ppBytes = pThis->pBytes + pThis->uPosition;

	if (bAdvance != SG_FALSE)
	{
		pThis->uPosition += uSize;
	}

fail:
	return;
}

/**
 * Resets a stream to use a buffer as its underlying data source.
 */
static void svndiff__stream__reset__buffer(
	SG_context*      pCtx,   //< [in] [out] Error and context info.
	svndiff__stream* pThis,  //< [in] Stream to reset.
	svndiff__buffer* pBuffer //< [in] Buffer to use as the stream's underlying data source.
	                         //<      Buffer must not be reset or freed as long as the stream is using it.
	)
{
	SG_uint32 uSize = 0u;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pBuffer);

	SG_ERR_CHECK(  svndiff__buffer__get_bytes__all(pCtx, pBuffer, NULL, &uSize)  );

	pThis->pBuffer             = pBuffer;
	pThis->pReadStream         = NULL;
	pThis->pBufferedReadStream = NULL;
	pThis->uSize               = uSize;
	pThis->uProcessed          = 0u;

fail:
	return;
}

/**
 * Resets a stream to use an SG_readstream as its underlying data source.
 */
static void svndiff__stream__reset__readstream(
	SG_context*      pCtx,    //< [in] [out] Error and context info.
	svndiff__stream* pThis,   //< [in] Stream to reset.
	SG_readstream*   pStream, //< [in] SG_readstream to use as the stream's underlying data source.
	SG_uint64        uSize    //< [in] Size of pStream, or zero if unknown.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pStream);

	pThis->pBuffer             = NULL;
	pThis->pReadStream         = pStream;
	pThis->pBufferedReadStream = NULL;
	pThis->uSize               = uSize;
	pThis->uProcessed          = 0u;

fail:
	return;
}

/**
 * Resets a stream to use a buffered_readstream as its underlying data source.
 */
static void svndiff__stream__reset__buffered_readstream(
	SG_context*          pCtx,    //< [in] [out] Error and context info.
	svndiff__stream*     pThis,   //< [in] Stream to reset.
	buffered_readstream* pStream, //< [in] buffered_readstream to use as the stream's underlying data source.
	SG_uint64            uSize    //< [in] Size of pStream, or zero if unknown.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pStream);

	pThis->pBuffer             = NULL;
	pThis->pReadStream         = NULL;
	pThis->pBufferedReadStream = pStream;
	pThis->uSize               = uSize;
	pThis->uProcessed          = 0u;

fail:
	return;
}

/**
 * Checks if the stream has reached its end.
 */
static void svndiff__stream__end(
	SG_context*      pCtx,  //< [in] [out] Error and context info.
	svndiff__stream* pThis, //< [in] Stream to check.
	SG_bool*         pEnd   //< [out] Whether or not the stream has reached its end.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pEnd);

	if (pThis->uSize == 0u)
	{
		if (pThis->pBuffer == NULL)
		{
			*pEnd = SG_FALSE;
		}
		else
		{
			SG_ERR_CHECK(  svndiff__buffer__end(pCtx, pThis->pBuffer, pEnd)  );
		}
	}
	else
	{
		SG_ASSERT(pThis->uProcessed <= pThis->uSize);
		*pEnd = (pThis->uProcessed == pThis->uSize ? SG_TRUE : SG_FALSE);
	}

fail:
	return;
}

/**
 * Reads data from the stream.
 * Throws an error if the given number of bytes could not be read.
 */
static void svndiff__stream__read(
	SG_context*      pCtx,   //< [in] [out] Error and context info.
	svndiff__stream* pThis,  //< [in] Stream to read from.
	SG_byte*         pBytes, //< [in] Bytes to read into.
	SG_uint32        uBytes  //< [in] Size of pBytes.
	)
{
	SG_uint32 uGot = 0u;

	SG_NULLARGCHECK(pThis);

	if (pThis->pBuffer != NULL)
	{
		SG_ERR_CHECK(  svndiff__buffer__read(pCtx, pThis->pBuffer, pBytes, uBytes, &uGot)  );
	}
	else if (pThis->pReadStream != NULL)
	{
		SG_ERR_CHECK(  SG_readstream__read(pCtx, pThis->pReadStream, uBytes, pBytes, &uGot)  );
	}
	else if (pThis->pBufferedReadStream != NULL)
	{
		SG_ERR_CHECK(  buffered_readstream__read(pCtx, pThis->pBufferedReadStream, uBytes, pBytes, &uGot)  );
	}
	else
	{
		SG_ASSERT(SG_FALSE);
	}

	if (uGot != uBytes)
	{
		SG_ERR_THROW2(SG_ERR_INCOMPLETEREAD, (pCtx, "Unable to read expected amount from svndiff stream: wanted %u, got %u", uBytes, uGot));
	}

	pThis->uProcessed += uGot;
	SG_ASSERT(pThis->uSize == 0u || pThis->uProcessed <= pThis->uSize);

fail:
	return;
}

/**
 * Reads from the stream until its current position is a given value.
 */
static void svndiff__stream__skip_to(
	SG_context*      pCtx,     //< [in] [out] Error and context info.
	svndiff__stream* pThis,    //< [in] Stream to read through.
	SG_uint64        uPosition //< [in] Position to read until.
	                           //<      Must be >= the stream's current position.
	)
{
	SG_byte aBuffer[1024u];

	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(uPosition >= pThis->uProcessed, uPosition);

	while (uPosition - pThis->uProcessed >= sizeof(aBuffer))
	{
		SG_ERR_CHECK(  svndiff__stream__read(pCtx, pThis, aBuffer, sizeof(aBuffer))  );
	}
	if (uPosition > pThis->uProcessed)
	{
		SG_ERR_CHECK(  svndiff__stream__read(pCtx, pThis, aBuffer, (SG_uint32)(uPosition - pThis->uProcessed))  );
	}

fail:
	return;
}

/**
 * Reads a uint64 value from the stream.
 */
static void svndiff__stream__read_uint64(
	SG_context*      pCtx,  //< [in] [out] Error and context info.
	svndiff__stream* pThis, //< [in] Stream to read from.
	SG_uint64*       pValue //< [out] Read value.
	                        //<       May be NULL if not needed.
	)
{
	SG_uint64 uValue = 0u;
	SG_byte   oByte  = 0;
	SG_bool   bEnd   = SG_FALSE;

	SG_NULLARGCHECK(pThis);

	while (bEnd == SG_FALSE)
	{
		// read the next byte
		SG_ERR_CHECK(  svndiff__stream__read(pCtx, pThis, &oByte, sizeof(oByte))  );

		// if the high bit is 1, we need to read more
		// if the high bit is 0, this is the last byte
		bEnd = (oByte & 0x80) ? SG_FALSE : SG_TRUE;

		// append the other 7 bits to the end of the value
		uValue <<= 7u;
		uValue |= (SG_uint64)(oByte & 0x7F);
	}

	if (pValue != NULL)
	{
		*pValue = uValue;
	}

fail:
	return;
}

/**
 * Reads a uint32 value from the stream.
 */
static void svndiff__stream__read_uint32(
	SG_context*      pCtx,  //< [in] [out] Error and context info.
	svndiff__stream* pThis, //< [in] Stream to read from.
	SG_uint32*       pValue //< [out] Read value.
	                        //<       May be NULL if not needed.
	)
{
	SG_uint64 uValue = 0u;

	SG_NULLARGCHECK(pThis);

	SG_ERR_CHECK(  svndiff__stream__read_uint64(pCtx, pThis, &uValue)  );
	if (uValue > SG_UINT32_MAX)
	{
		SG_int_to_string_buffer szValue;
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DIFF, (pCtx, "Expected to read a 32-bit number, but got 64-bit: %s", SG_uint64_to_sz(uValue, szValue)));
	}

	if (pValue != NULL)
	{
		*pValue = (SG_uint32)uValue;
	}

fail:
	return;
}

/**
 * Reads an svndiff header from the stream.
 */
static void svndiff__stream__read_header(
	SG_context*      pCtx,    //< [in] [out] Error and context info.
	svndiff__stream* pThis,   //< [in] Stream to read from.
	SG_uint32*       pVersion //< [out] Version of svndiff indicated in the header.
	                          //<       May be NULL if not needed.
	)
{
	SG_byte aHeader[4u];

	SG_NULLARGCHECK(pThis);

	SG_ERR_CHECK(  svndiff__stream__read(pCtx, pThis, aHeader, sizeof(aHeader))  );
	if (aHeader[0u] != 'S' || aHeader[1u] != 'V' || aHeader[2u] != 'N')
	{
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DIFF, (pCtx, "Doesn't start with 'SVN' header"));
	}

	if (pVersion != NULL)
	{
		*pVersion = aHeader[3u];
	}

fail:
	return;
}

/**
 * Reads an svndiff window instruction from the stream.
 */
static void svndiff__stream__read_instruction(
	SG_context*           pCtx,         //< [in] [out] Error and context info.
	svndiff__stream*      pThis,        //< [in] Stream to read from.
	svndiff__instruction* pInstruction, //< [out] Type of instruction read.
	                                    //<       May be NULL if not needed.
	SG_uint32*            pSize,        //< [out] Size parameter of the instruction.
	                                    //<       May be NULL if not needed.
	SG_uint32*            pPosition     //< [out] Position parameter of the instruction.
	                                    //<       Set to zero if the instruction has no position parameter.
	                                    //<       May be NULL if not needed.
	)
{
	SG_byte              oByte        = 0;
	svndiff__instruction eInstruction = SVNDIFF__INSTRUCTION__COUNT;
	SG_uint32            uSize        = 0u;
	SG_uint32            uPosition    = 0u;

	SG_NULLARGCHECK(pThis);

	// read the first byte
	SG_ERR_CHECK(  svndiff__stream__read(pCtx, pThis, &oByte, sizeof(oByte))  );

	// first two bits of the first byte are the instruction
	eInstruction = (svndiff__instruction)((oByte & 0xC0) >> 6u);

	// remaining six bits of the first byte are the size
	uSize = (oByte & 0x3F);

	// if the size is zero, then it was too big for 6 bits
	// it's in the following bytes instead
	if (uSize == 0u)
	{
		SG_ERR_CHECK(  svndiff__stream__read_uint32(pCtx, pThis, &uSize)  );
	}

	// if the instruction is to read from source or target
	// then the next bytes are the position to start at
	if (
		   (eInstruction == SVNDIFF__INSTRUCTION__SOURCE)
		|| (eInstruction == SVNDIFF__INSTRUCTION__TARGET)
		)
	{
		SG_ERR_CHECK(  svndiff__stream__read_uint32(pCtx, pThis, &uPosition)  );
	}

	// return whatever they wanted
	if (pInstruction != NULL)
	{
		*pInstruction = eInstruction;
	}
	if (pSize != NULL)
	{
		*pSize = uSize;
	}
	if (pPosition != NULL)
	{
		*pPosition = uPosition;
	}

fail:
	return;
}

/**
 * Reads a delta from a stream, applies it to base data from an input stream,
 * and writes the result to an output stream.
 * Can also perform caller-specified actions on the final data as it's generated.
 */
static void svndiff__process_delta(
	SG_context*             pCtx,          //< [in] [out] Error and context info.
	buffered_readstream*    pDeltaStream,  //< [in] Stream to read the delta from.
	SG_uint64               uDeltaSize,    //< [in] Length of the delta to read.
	SG_readstream*          pInputStream,  //< [in] Stream to read the input data from.
	                                       //<      NULL if there is no input data to apply the delta to.
	                                       //<      This works as long as the delta doesn't reference the source view.
	SG_writestream*         pOutputStream, //< [in] Stream to write the output to.
	                                       //<      If NULL, the output is not written anywhere.
	SG_byte*                pBuffer,       //< [in] Buffer to use to store chunks as they are generated.
	                                       //<      If NULL, one will be allocated internally.
	SG_uint32               uBuffer,       //< [in] Size of pBuffer.
	                                       //<      If zero, a buffer will be allocated internally.
	import__blob__callback* fCallback,     //< [in] Callback to pass each final chunk to.
	                                       //<      May be NULL if no actions are needed.
	void*                   pCallbackData  //< [in] Data to pass to fCallback.
	)
{
	svndiff__stream oDeltaStream        = SVNDIFF__STREAM__INIT;
	svndiff__stream oInputStream        = SVNDIFF__STREAM__INIT;
	svndiff__buffer oSourceBuffer       = SVNDIFF__BUFFER__INIT;
	svndiff__buffer oTargetBuffer       = SVNDIFF__BUFFER__INIT;
	svndiff__buffer oInstructionsBuffer = SVNDIFF__BUFFER__INIT;
	svndiff__buffer oNewDataBuffer      = SVNDIFF__BUFFER__INIT;
	SG_uint32       uVersion            = 0u;
	SG_bool         bDeltaEnd           = SG_FALSE;

	SG_NULLARGCHECK(pDeltaStream);
	SG_ARGCHECK(uDeltaSize > 0u, uDeltaSize);

	// wrap the given streams
	SG_ERR_CHECK(  svndiff__stream__reset__buffered_readstream(pCtx, &oDeltaStream, pDeltaStream, uDeltaSize)  );
	if (pInputStream != NULL)
	{
		SG_ERR_CHECK(  svndiff__stream__reset__readstream(pCtx, &oInputStream, pInputStream, 0u)  );
	}

	// if the caller provided a buffer, use it for our target buffer
	if (pBuffer != NULL && uBuffer > 0u)
	{
		SG_ERR_CHECK(  svndiff__buffer__reset(pCtx, &oTargetBuffer, pBuffer, uBuffer)  );
	}

	// if we have a callback, notify it that we're starting
	if (fCallback != NULL)
	{
		SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__BEGIN)  );
	}

	// read the delta's header
	SG_ERR_CHECK(  svndiff__stream__read_header(pCtx, &oDeltaStream, &uVersion)  );
	if (uVersion != 0u)
	{
		SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unsupported version of svndiff: %u", uVersion));
		// TODO: add support for version 1 (instructions and/or new data might be zlib compressed)
	}

	// read windows until there aren't any more
	SG_ERR_CHECK(  svndiff__stream__end(pCtx, &oDeltaStream, &bDeltaEnd)  );
	while (bDeltaEnd == SG_FALSE)
	{
		SG_uint64       uInputStart         = 0u;
		SG_uint32       uSize               = 0u;
		SG_byte*        pBytes              = NULL;
		svndiff__stream oInstructionsStream = SVNDIFF__STREAM__INIT;
		svndiff__stream oNewDataStream      = SVNDIFF__STREAM__INIT;
		SG_bool         bInstructionsEnd    = SG_FALSE;
		SG_bool         bNewDataEnd         = SG_FALSE;
		SG_bool         bTargetEnd          = SG_FALSE;

		// read the dimensions of the source view
		SG_ERR_CHECK(  svndiff__stream__read_uint64(pCtx, &oDeltaStream, &uInputStart)  );
		SG_ERR_CHECK(  svndiff__stream__read_uint32(pCtx, &oDeltaStream, &uSize)  );

		// if there's a source view, read it into our buffer
		if (uSize > 0u)
		{
			// we need to have an input stream if we're referencing data in it
			SG_NULLARGCHECK(pInputStream);

			// skip to the start position in the input stream
			SG_ERR_CHECK(  svndiff__stream__skip_to(pCtx, &oInputStream, uInputStart)  );

			// reset the buffer to the correct size and fill it from the stream
			SG_ERR_CHECK(  svndiff__buffer__reset(pCtx, &oSourceBuffer, NULL, uSize)  );
			SG_ERR_CHECK(  svndiff__buffer__get_bytes__all(pCtx, &oSourceBuffer, &pBytes, &uSize)  );
			SG_ERR_CHECK(  svndiff__stream__read(pCtx, &oInputStream, pBytes, uSize)  );
		}

		// read the size of the target view and reset our buffer to match
		SG_ERR_CHECK(  svndiff__stream__read_uint32(pCtx, &oDeltaStream, &uSize)  );
		SG_ERR_CHECK(  svndiff__buffer__reset(pCtx, &oTargetBuffer, NULL, uSize)  );

		// read the size of the instructions and reset our buffer to match
		SG_ERR_CHECK(  svndiff__stream__read_uint32(pCtx, &oDeltaStream, &uSize)  );
		SG_ERR_CHECK(  svndiff__buffer__reset(pCtx, &oInstructionsBuffer, NULL, uSize)  );

		// read the size of the new data and reset our buffer to match
		SG_ERR_CHECK(  svndiff__stream__read_uint32(pCtx, &oDeltaStream, &uSize)  );
		SG_ERR_CHECK(  svndiff__buffer__reset(pCtx, &oNewDataBuffer, NULL, uSize)  );

		// read the instructions into our buffer
		SG_ERR_CHECK(  svndiff__buffer__get_bytes__all(pCtx, &oInstructionsBuffer, &pBytes, &uSize)  );
		SG_ERR_CHECK(  svndiff__stream__read(pCtx, &oDeltaStream, pBytes, uSize)  );

		// read the new data into our buffer
		SG_ERR_CHECK(  svndiff__buffer__get_bytes__all(pCtx, &oNewDataBuffer, &pBytes, &uSize)  );
		SG_ERR_CHECK(  svndiff__stream__read(pCtx, &oDeltaStream, pBytes, uSize)  );

		// wrap the instructions and new data buffers in streams
		SG_ERR_CHECK(  svndiff__stream__reset__buffer(pCtx, &oInstructionsStream, &oInstructionsBuffer)  );
		SG_ERR_CHECK(  svndiff__stream__reset__buffer(pCtx, &oNewDataStream, &oNewDataBuffer)  );

		// read and process all the instructions
		SG_ERR_CHECK(  svndiff__stream__end(pCtx, &oInstructionsStream, &bInstructionsEnd)  );
		while (bInstructionsEnd == SG_FALSE)
		{
			svndiff__instruction eInstruction = SVNDIFF__INSTRUCTION__COUNT;
			SG_uint32            uPosition    = 0u;
			SG_byte*             pFromBytes   = NULL;
			SG_byte*             pToBytes     = NULL;

			// read and process the current instruction
			SG_ERR_CHECK(  svndiff__stream__read_instruction(pCtx, &oInstructionsStream, &eInstruction, &uSize, &uPosition)  );
			switch (eInstruction)
			{
			case SVNDIFF__INSTRUCTION__SOURCE:
				{
					// copy from the source view to the target view
					SG_ERR_CHECK(  svndiff__buffer__get_bytes__random(pCtx, &oSourceBuffer, uPosition, uSize, &pFromBytes)  );
					SG_ERR_CHECK(  svndiff__buffer__get_bytes__current(pCtx, &oTargetBuffer, uSize, SG_TRUE, &pToBytes)  );
					memcpy((void*)pToBytes, (const void*)pFromBytes, uSize);
				}
				break;
			case SVNDIFF__INSTRUCTION__TARGET:
				{
					SG_uint32 uWant = 0u;
					SG_uint32 uHave = 0u;

					// copy from an already written part of the target view to the target view
					SG_ERR_CHECK(  svndiff__buffer__get_bytes__random(pCtx, &oTargetBuffer, uPosition, uSize, &pFromBytes)  );
					SG_ERR_CHECK(  svndiff__buffer__get_bytes__current(pCtx, &oTargetBuffer, uSize, SG_TRUE, &pToBytes)  );
					if (pFromBytes >= pToBytes)
					{
						SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DIFF, (pCtx, "Cannot copy from unwritten portion of target view."));
					}

					// It's possible for the end of the source byte range to
					// overlap the beginning of the target byte range.  This
					// works okay if you think of it as copying byte-by-byte.
					// By the time we want to copy FROM any part of the target
					// range, we will already have copied data TO that same
					// part.  Therefore, the end result will be that some of
					// the data will be repeated.  To make this work without
					// literally copying byte for byte, repeatedly copy
					// everything from the source range that does NOT overlap
					// into the target until no overlap remains.
					uWant = uSize;
					uHave = (SG_uint32)(pToBytes - pFromBytes);
					while (uWant > uHave)
					{
						memcpy((void*)pToBytes, (const void*)pFromBytes, uHave);
						pToBytes   += uHave;
						pFromBytes += uHave;
						uWant      -= uHave;
					}

					// finally, copy the non-overlapping portion
					memcpy((void*)pToBytes, (const void*)pFromBytes, uWant);
				}
				break;
			case SVNDIFF__INSTRUCTION__NEW_DATA:
				{
					// copy from the new data to the target view
					SG_ERR_CHECK(  svndiff__buffer__get_bytes__current(pCtx, &oTargetBuffer, uSize, SG_TRUE, &pToBytes)  );
					SG_ERR_CHECK(  svndiff__stream__read(pCtx, &oNewDataStream, pToBytes, uSize)  );
				}
				break;
			default:
				SG_ASSERT(SG_FALSE);
				break;
			}

			// check if there are more instructions
			SG_ERR_CHECK(  svndiff__stream__end(pCtx, &oInstructionsStream, &bInstructionsEnd)  );
		}

		// we should be at the end of the new data and the target
		SG_ERR_CHECK(  svndiff__stream__end(pCtx, &oNewDataStream, &bNewDataEnd)  );
		SG_ASSERT(bNewDataEnd != SG_FALSE);
		SG_ERR_CHECK(  svndiff__buffer__end(pCtx, &oTargetBuffer, &bTargetEnd)  );
		SG_ASSERT(bTargetEnd != SG_FALSE);

		// get the bytes that we generated in the target buffer
		// and give them to the specified places
		SG_ERR_CHECK(  svndiff__buffer__get_bytes__all(pCtx, &oTargetBuffer, &pBytes, &uSize)  );
		if (pOutputStream != NULL)
		{
			SG_uint32 uWrote = 0u;

			SG_ERR_CHECK(  SG_writestream__write(pCtx, pOutputStream, uSize, pBytes, &uWrote)  );
			if (uWrote != uSize)
			{
				SG_ERR_THROW2(SG_ERR_INCOMPLETEWRITE, (pCtx, "Couldn't write enough bytes to svndiff target: had %u, wrote %u", uSize, uWrote));
			}
		}
		if (fCallback != NULL)
		{
			SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, pBytes, uSize)  );
		}

		// check if we're at the end of the delta stream yet
		SG_ERR_CHECK(  svndiff__stream__end(pCtx, &oDeltaStream, &bDeltaEnd)  );
	}

	// if we have a callback, notify it that we're done
	if (fCallback != NULL)
	{
		SG_ERR_CHECK(  fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__END)  );

		// blank the callback so we don't send it a fail notification during cleanup
		fCallback = NULL;
	}

fail:
	if (fCallback != NULL)
	{
		fCallback(pCtx, pCallbackData, NULL, IMPORT__BLOB__FAIL);
	}
	svndiff__buffer__free(pCtx, &oSourceBuffer);
	svndiff__buffer__free(pCtx, &oTargetBuffer);
	svndiff__buffer__free(pCtx, &oInstructionsBuffer);
	svndiff__buffer__free(pCtx, &oNewDataBuffer);
	return;
}


/*
**
** SVN Dump
**
*/

/*
**
** Types
**
*/

/*
 * Constants used for buffer sizes.
 */
#define SIZE_MD5 32u
#define SIZE_SHA1 40u
#define SIZE_UUID 32u
#define SIZE_READ_BUFFER 1024u
#define SIZE_BLOB_BUFFER SG_STREAMING_BUFFER_SIZE
#define SIZE_HID_STRPOOL (((SIZE_SHA1) + 1u) * 100u)

/*
 * Other constants.
 */
static const char* gszCacheRepoNameFormat = "%s_svn_load_cache";     // passed the name of the corresponding import/target repo
static const char* gszExtraCommentFormat  = "From SVN revision: %s"; // passed revision ID, converted to string

/**
 * Enumeration of repos used during an import.
 */
typedef enum dump__repo
{
	DUMP__REPO__IMPORT, //< The repo actually being imported into.
	DUMP__REPO__CACHE,  //< Caches data that isn't being imported.
	DUMP__REPO__COUNT   //< Number of items in the enumeration, for iteration and invalid value.
}
dump__repo;

/**
 * Data associated with a single SVN path being imported.
 */
typedef struct dump__path
{
	// initialization data
	dump__repo  eRepo;                           //< Repo that the path is being imported into.
	const char* szNextParent;                    //< The parent for our next commit from this path.
	                                             //< Points into pChangesets.
	const char* szBranch;                        //< Branch that the path is being imported into.
	                                             //< If NULL the changesets imported from this path will not be included in a branch.
	const char* szStamp;                         //< Stamp being applied to all changesets imported from this path.
	                                             //< If NULL no stamp is applied.
	SG_bool     bComment;                        //< Whether or not an extra comment is added to each imported changeset.
	                                             //< The extra comment includes the SVN revision that the changeset came from.
	char        szPathGid[SG_GID_BUFFER_LENGTH]; //< GID of the path's directory node when in its own independent tree.
	char        szSvnGid[SG_GID_BUFFER_LENGTH];  //< GID of the path's directory node when in the SVN tree.

	// internal data
	SG_uint64   uBaseRevision;                   //< First SVN revision committed from this path.
	SG_vector*  pRevisionMap;                    //< Maps SVN revisions to the Veracity changesets that were imported from this path.
	                                             //< The values are Veracity changeset HIDs that point into pChangesets.
	                                             //< Index 0 is the HID for the SVN revision in uBaseRevision.
	                                             //< Subtract uBaseRevision from any other SVN revision to get its index.
	                                             //< Indices with NULL values indicate that the path did not exist in that SVN revision.
	SG_strpool* pChangesets;                     //< Owns all the changeset HID strings pointed to by various other members.
	const char* szBranchHead;                    //< HID that the branch head we're updating currently points to.
	                                             //< NULL if we haven't created a branch head for this path yet.
	                                             //< Points into pChangesets.
	SG_bool     bExistsInLatest;                 //< Whether or not the path exists in the latest imported revision.
	                                             //< If no revisions have been imported, reflects our existence in the first parent changeset.

	// temporary data
	// These only exist to pass some per-path data between
	// revision__commit__path__in_transaction and revision__commit__path__post_transaction.
	// Ideally we wouldn't need these because we wouldn't need any separate
	// post_transaction commit step because we'd be able to do everything in the
	// same transaction, but the branch/comment/etc APIs don't currently work that
	// way.  Think of these as local variables in revision__commit that are per-path.
	char*       szCommitting;                    //< HID of the changeset currently being committed from this path.
	                                             //< Might be NULL if no changeset was needed for this path in the current revision.
	const char* szCommittingPool;                //< Pointer into pChangesets with the same value as szCommitting.
	                                             //< NULL if it hasn't been added to the pool yet.
}
dump__path;

/**
 * Static initializer for dump__path.
 */
#define DUMP__PATH__INIT { DUMP__REPO__COUNT, NULL, NULL, NULL, SG_FALSE, { '\0', }, { '\0' }, 0u, NULL, NULL, NULL, SG_FALSE, NULL, NULL }

/**
 * Data shared across the entire load process.
 */
typedef struct dump
{
	// path-specific data
	dump__path oRootPath;               //< Data for importing whatever's left after other import paths are processed.
	SG_rbtree* pPaths;                  //< Set of specific paths being imported (maps SVN path => dump__path*).

	// repo-specific data
	struct
	{
		SG_repo*  pRepo;                //< The repo itself.
		SG_vhash* pUsers;               //< Set of users that exist in the repo (maps name => recid).
	}
	aRepos[DUMP__REPO__COUNT];          //< Data about each repo that we're using.
	                                    //< Indexed by a dump__repo value.

	// global dumpfile data
	SG_uint32   uFormatVersion;         //< Version of the dump file format in use.
	char        szUuid[SIZE_UUID + 1u]; //< UUID of the dumped repository.

	// dynamic/cache data
	SG_bool     bOperation;             //< Whether or not the dump has an SG_log operation pushed.
	SG_byte*    pBlobBuffer;            //< Buffer used for reading chunks of blobs.
	SG_uint32   uBlobBuffer;            //< Size of pBlobBuffer.
}
dump;

/**
 * Static initializer for dump.
 */
#define DUMP__INIT { DUMP__PATH__INIT, NULL, { { NULL, NULL }, }, 0u, { '\0', }, SG_FALSE, NULL, 0u }

/**
 * Data related to a single revision record.
 */
typedef struct revision
{
	// always used
	SG_uint64          uId;              //< Revision's unique ID.
	SG_uint64          uPropertySize;    //< Size in bytes of the revision record's property section.
	SG_uint64          uContentSize;     //< Size in bytes of the revision record's content (always matches uPropertySize).

	// optional properties, may be NULL/0 if they weren't specified
	SG_vhash*          pProperties;      //< Properties read from the record.
	SG_int64           iTime;            //< Time that the revision was created.
	const char*        szUser;           //< Name of the user that created the revision.
	                                     //< Points into pProperties.
	const char*        szMessage;        //< Commit message associated with the revision.
	                                     //< Points into pProperties.

	// repo-specific data
	struct
	{
		SG_repo_tx_handle* pTransaction; //< Transaction for committing the revision to this repo.
		const char*        szUserId;     //< GID of the user we'll be committing to this repo as.
		                                 //< Points into pDump->aRepos[i]->pUsers or to SG_NOBODY__USERID.
	}
	aRepos[DUMP__REPO__COUNT];           //< Data about each repo that we're using.
	                                     //< Indexed by a dump__repo value.

	// dynamic information associated with the revision
	// but not actually read from the dump
	dump*              pDump;            //< Dump that owns the revision.
	SG_bool            bOperation;       //< Whether or not the revision has an SG_log operation pushed.
	char               szOperation[30u]; //< strlen("Revision ") + sizeof(SG_int_to_string_buffer)
	pending_node*      pSuperRoot;       //< Super-root pending node of the SVN tree (which is the parent of the '@' node).
	pending_node*      pFileRoot;        //< The '@' pending node that is the root of all actual file/dir nodes.
	SG_rbtree*         pPathSuperRoots;  //< Super-root nodes loaded for specific import paths (maps SVN path => pending_node*).
	                                     //< Keys match those in pDump->pPaths.
}
revision;

/**
 * Static initializer for revision.
 */
#define REVISION__INIT { 0u, 0u, 0u, NULL, 0, NULL, NULL, { { NULL, NULL }, }, NULL, SG_FALSE, { '\0', }, NULL, NULL, NULL }

/**
 * Enumeration of possible actions to be taken on a node.
 */
typedef enum node_action
{
	NODE_ACTION__CHANGE,  //< Node is being modified.
	NODE_ACTION__ADD,     //< Node is being created.
	NODE_ACTION__DELETE,  //< Node is being deleted.
	NODE_ACTION__REPLACE, //< Node is being replaced with a different/new node.
	NODE_ACTION__COUNT    //< Number of values in the enum, for iteration.
	                      //< Also used as an invalid value.
}
node_action;

/**
 * Enumeration of possible node types.
 */
typedef enum node_type
{
	NODE_TYPE__FILE,   //< Node is a file.
	NODE_TYPE__FOLDER, //< Node is a folder.
	NODE_TYPE__COUNT   //< Number of values in the enum, for iteration.
	                   //< Also used as an invalid value.
}
node_type;


/*
**
** Global Data
**
*/

/**
 * Map of SG_treenode_entry_type values to equivalent node_type values.
 */
static node_type gaPendingNodeType_NodeType[] =
{
	NODE_TYPE__COUNT,  //< SG_TREENODEENTRY_TYPE__INVALID
	NODE_TYPE__FILE,   //< SG_TREENODEENTRY_TYPE_REGULAR_FILE
	NODE_TYPE__FOLDER, //< SG_TREENODEENTRY_TYPE_DIRECTORY
	NODE_TYPE__FILE,   //< SG_TREENODEENTRY_TYPE_SYMLINK
	NODE_TYPE__COUNT   //< SG_TREENODEENTRY_TYPE_SUBMODULE
};

/**
 * Map of node_type values to equivalent SG_treenode_entry_type values.
 */
static SG_treenode_entry_type gaNodeType_PendingNodeType[] =
{
	SG_TREENODEENTRY_TYPE_REGULAR_FILE, //< NODE_TYPE__FILE
	SG_TREENODEENTRY_TYPE_DIRECTORY,    //< NODE_TYPE__FOLDER
	SG_TREENODEENTRY_TYPE__INVALID      //< NODE_TYPE__COUNT
};

/**
 * Map of node_type values to strings.
 */
static const char* gaNodeType_String[] =
{
	"file",   //< NODE_TYPE__FILE
	"folder", //< NODE_TYPE__FOLDER
	"invalid" //< NODE_TYPE__COUNT
};

// misc
const char* gszHashName_Sha1 = "SHA1/160";
const char* gszHashName_Md5  = "MD5/128";

// dump data headers
const char* gszDump_Version = "SVN-fs-dump-format-version";
const char* gszDump_Uuid    = "UUID";

// revision record headers/properties
const char* gszRevision_Id           = "Revision-number";
const char* gszRevision_PropertySize = "Prop-content-length";
const char* gszRevision_ContentSize  = "Content-length";
const char* gszRevision_Time         = "svn:date";
const char* gszRevision_User         = "svn:author";
const char* gszRevision_Message      = "svn:log";

// node record headers/properties
const char* gszNode_Path            = "Node-path";
const char* gszNode_Action          = "Node-action";
const char* gszNode_Type            = "Node-kind";
const char* gszNode_PropertySize    = "Prop-content-length";
const char* gszNode_ContentSize     = "Content-length";
const char* gszNode_SourceRevision  = "Node-copyfrom-rev";
const char* gszNode_SourcePath      = "Node-copyfrom-path";
const char* gszNode_SourceMd5       = "Text-copy-source-md5";
const char* gszNode_SourceSha1      = "Text-copy-source-sha1";
const char* gszNode_PropertiesDelta = "Prop-delta";
const char* gszNode_TextSize        = "Text-content-length";
const char* gszNode_TextMd5         = "Text-content-md5";
const char* gszNode_TextSha1        = "Text-content-sha1";
const char* gszNode_TextDelta       = "Text-delta";
const char* gszNode_TextBaseMd5     = "Text-delta-base-md5";
const char* gszNode_TextBaseSha1    = "Text-delta-base-sha1";

// property section constants
const char  gcProperty_Key    = 'K';
const char  gcProperty_Value  = 'V';
const char  gcProperty_Delete = 'D';
const char* gszProperty_End   = "PROPS-END";

// node action constants
const char* gszNodeAction_Change  = "change";
const char* gszNodeAction_Add     = "add";
const char* gszNodeAction_Delete  = "delete";
const char* gszNodeAction_Replace = "replace";

// node type constants
const char* gszNodeType_File   = "file";
const char* gszNodeType_Folder = "dir";


/*
**
** Parsing
**
*/

/**
 * Implements function parse__hash_value__NAME that retrieves a string value
 * from a vhash and parses it into a value of type TYPE using function
 * parse__value__NAME, which must already be defined.
 */
#define PARSE_HASH_VALUE(NAME, TYPE)                                         \
static void parse__hash_value__##NAME(                                       \
	SG_context*     pCtx,                                                    \
	const SG_vhash* pHash,                                                   \
	const char*     szKey,                                                   \
	TYPE*           pValue                                                   \
	)                                                                        \
{                                                                            \
	const char* szValue = NULL;                                              \
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHash, szKey, &szValue)  );       \
	SG_ERR_CHECK(  parse__value__##NAME(pCtx, szValue, pValue)  );           \
fail:                                                                        \
	return;                                                                  \
}

/**
 * As PARSE_HASH_VALUE, except it allows for the named string to not be
 * present in the vhash.  It takes a default value to return when that is
 * the case.
 */
#define PARSE_OPTIONAL_HASH_VALUE(NAME, TYPE)                                \
static void parse__hash_value__##NAME##__optional(                           \
	SG_context*     pCtx,                                                    \
	const SG_vhash* pHash,                                                   \
	const char*     szKey,                                                   \
	TYPE*           pValue,                                                  \
	TYPE            oDefault                                                 \
	)                                                                        \
{                                                                            \
	const char* szValue = NULL;                                              \
	if (pHash != NULL)                                                       \
	{                                                                        \
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHash, szKey, &szValue)  ); \
	}                                                                        \
	if (szValue == NULL)                                                     \
	{                                                                        \
		*pValue = oDefault;                                                  \
	}                                                                        \
	else                                                                     \
	{                                                                        \
		SG_ERR_CHECK(  parse__value__##NAME(pCtx, szValue, pValue)  );       \
	}                                                                        \
fail:                                                                        \
	return;                                                                  \
}

/**
 * String value parser for bool.
 */
static void parse__value__bool(
	SG_context* pCtx,
	const char* szValue,
	SG_bool*    pValue
	)
{
	if (strcmp(szValue, "true") == 0)
	{
		*pValue = SG_TRUE;
	}
	else if (strcmp(szValue, "false") == 0)
	{
		*pValue = SG_FALSE;
	}
	else
	{
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Unknown boolean value: %s", szValue));
	}

fail:
	return;
}
PARSE_OPTIONAL_HASH_VALUE(bool, SG_bool)

/**
 * String value parser for uint32.
 */
static void parse__value__uint32(
	SG_context* pCtx,
	const char* szValue,
	SG_uint32*  pValue
	)
{
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, pValue, szValue)  );

fail:
	return;
}
PARSE_HASH_VALUE(uint32, SG_uint32)

/**
 * String value parser for uint64.
 */
static void parse__value__uint64(
	SG_context* pCtx,
	const char* szValue,
	SG_uint64*  pValue
	)
{
	SG_ERR_CHECK(  SG_uint64__parse__strict(pCtx, pValue, szValue)  );

fail:
	return;
}
PARSE_HASH_VALUE(uint64, SG_uint64)
PARSE_OPTIONAL_HASH_VALUE(uint64, SG_uint64)

/**
 * String value parser for timestamps (SG_int64).
 */
static void parse__value__time(
	SG_context* pCtx,
	const char* szValue,
	SG_int64*   pValue
	)
{
	char szYear[5u];
	char szMonth[3u];
	char szDay[3u];
	char szHour[3u];
	char szMinute[3u];
	char szSecond[3u];
	char szMicrosecond[7u];
	SG_uint32 uYear;
	SG_uint32 uMonth;
	SG_uint32 uDay;
	SG_uint32 uHour;
	SG_uint32 uMinute;
	SG_uint32 uSecond;
	SG_uint32 uMicrosecond;
	SG_int64  iTime;

	SG_ARGCHECK(szValue[ 4u] == '-',  szValue);
	SG_ARGCHECK(szValue[ 7u] == '-',  szValue);
	SG_ARGCHECK(szValue[10u] == 'T',  szValue);
	SG_ARGCHECK(szValue[13u] == ':',  szValue);
	SG_ARGCHECK(szValue[16u] == ':',  szValue);
	SG_ARGCHECK(szValue[19u] == '.',  szValue);
	SG_ARGCHECK(szValue[26u] == 'Z',  szValue);
	SG_ARGCHECK(szValue[27u] == '\0', szValue);

#define COPY_CHUNK(DEST, START, SIZE)                    \
	memcpy((void*)DEST, (void*)(szValue + START), SIZE); \
	DEST[SIZE] = '\0';

	// copy each individual substring out
	COPY_CHUNK(szYear,         0u, 4u);
	COPY_CHUNK(szMonth,        5u, 2u);
	COPY_CHUNK(szDay,          8u, 2u);
	COPY_CHUNK(szHour,        11u, 2u);
	COPY_CHUNK(szMinute,      14u, 2u);
	COPY_CHUNK(szSecond,      17u, 2u);
	COPY_CHUNK(szMicrosecond, 20u, 6u);

#undef COPY_CHUNK

	// convert each substring into a number
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uYear,        szYear)  );
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uMonth,       szMonth)  );
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uDay,         szDay)  );
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uHour,        szHour)  );
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uMinute,      szMinute)  );
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uSecond,      szSecond)  );
	SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uMicrosecond, szMicrosecond)  );

	// make a time from the individual numbers
	// The returned time is in millseconds,
	// though it's only accurate to the second because that's all we can pass in.
	SG_ERR_CHECK(  SG_time__mktime__utc(pCtx, uYear, uMonth, uDay, uHour, uMinute, uSecond, &iTime)  );

	// add in the milliseconds
	iTime += (uMicrosecond / 1000u);

	// done
	*pValue = iTime;

fail:
	return;
}
PARSE_OPTIONAL_HASH_VALUE(time, SG_int64)

/**
 * String value parser for node_action.
 */
static void parse__value__node_action(
	SG_context*  pCtx,
	const char*  szValue,
	node_action* pValue
	)
{
	if (strcmp(szValue, gszNodeAction_Change) == 0)
	{
		*pValue = NODE_ACTION__CHANGE;
	}
	else if (strcmp(szValue, gszNodeAction_Add) == 0)
	{
		*pValue = NODE_ACTION__ADD;
	}
	else if (strcmp(szValue, gszNodeAction_Delete) == 0)
	{
		*pValue = NODE_ACTION__DELETE;
	}
	else if (strcmp(szValue, gszNodeAction_Replace) == 0)
	{
		*pValue = NODE_ACTION__REPLACE;
	}
	else
	{
		*pValue = NODE_ACTION__COUNT;
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Unknown node action: %s", szValue));
	}

fail:
	return;
}
PARSE_HASH_VALUE(node_action, node_action)

/**
 * String value parser for node_type.
 */
static void parse__value__node_type(
	SG_context* pCtx,
	const char* szValue,
	node_type*  pValue
	)
{
	if (strcmp(szValue, gszNodeType_File) == 0)
	{
		*pValue = NODE_TYPE__FILE;
	}
	else if (strcmp(szValue, gszNodeType_Folder) == 0)
	{
		*pValue = NODE_TYPE__FOLDER;
	}
	else
	{
		*pValue = NODE_TYPE__COUNT;
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Unknown node type: %s", szValue));
	}

fail:
	return;
}
PARSE_OPTIONAL_HASH_VALUE(node_type, node_type)

/**
 * String value parser that copies a UUID into an existing buffer.
 */
static void parse__value__uuid(
	SG_context* pCtx,
	const char* szValue,
	char*       szBuffer
	)
{
	// UUID has the form: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	// We want to parse it into the same string without hyphens.

	SG_UNUSED(pCtx);

#define COPY_CHUNK(SIZE)                           \
	memcpy((void*)szBuffer, (void*)szValue, SIZE); \
	szBuffer += SIZE;                              \
	szValue += SIZE + 1u;

	COPY_CHUNK(8u);
	COPY_CHUNK(4u);
	COPY_CHUNK(4u);
	COPY_CHUNK(4u);
	COPY_CHUNK(12u);
	*szBuffer = '\0';

#undef COPY_CHUNK
}
PARSE_HASH_VALUE(uuid, char)

/**
 * Parses a set of headers from a record.
 */
static void parse__headers(
	SG_context*          pCtx,     //< [in] [out] Error and context info.
	buffered_readstream* pStream,  //< [in] Stream to read headers from.
	SG_vhash**           ppHeaders //< [out] The headers read, mapping keys to values.
	)
{
	SG_vhash*  pHeaders = NULL;
	SG_int32   iChar    = 0;
	SG_bool    bEOF     = SG_FALSE;
	SG_bool    bEmpty   = SG_FALSE;
	SG_string* sKey     = NULL;
	SG_string* sValue   = NULL;

	SG_NULLARGCHECK(pStream);
	SG_NULLARGCHECK(ppHeaders);

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pHeaders)  );

	// keep reading until an empty line or the end of the file
	SG_ERR_CHECK(  buffered_readstream__eof(pCtx, pStream, &bEOF)  );
	while (bEmpty == SG_FALSE && bEOF == SG_FALSE)
	{
		// read the header key (everything up through a colon)
		SG_ERR_CHECK(  buffered_readstream__read_while(pCtx, pStream, ":\r\n", SG_FALSE, &sKey, &iChar)  );
		if (iChar == ':')
		{
			if (SG_string__length_in_bytes(sKey) > 0u)
			{
				// read the header value (the rest of the line)
				SG_ERR_CHECK(  buffered_readstream__read_utf8_char(pCtx, pStream, ':')  );
				SG_ERR_CHECK(  buffered_readstream__read_while(pCtx, pStream, " \t", SG_TRUE, NULL, NULL)  );
				SG_ERR_CHECK(  buffered_readstream__read_while(pCtx, pStream, "\r\n", SG_FALSE, &sValue, NULL)  );
				SG_ERR_CHECK(  buffered_readstream__read_newline(pCtx, pStream)  );
			}
			else
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Header has empty key/name."));
			}
		}
		else
		{
			if (SG_string__length_in_bytes(sKey) > 0u)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Header line contains no key/value separator colon: %s", SG_string__sz(sKey)));
			}
			else
			{
				// empty line, we're done reading
				SG_ERR_CHECK(  buffered_readstream__read_newline(pCtx, pStream)  );
				bEmpty = SG_TRUE;
			}
		}
		SG_ERR_CHECK(  buffered_readstream__eof(pCtx, pStream, &bEOF)  );

		// if we read header data, store it
		if (sKey != NULL && sValue != NULL)
		{
			SG_ASSERT(bEmpty == SG_FALSE);
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHeaders, SG_string__sz(sKey), SG_string__sz(sValue))  );
		}
		SG_STRING_NULLFREE(pCtx, sKey);
		SG_STRING_NULLFREE(pCtx, sValue);
	}

	// return the headers
	*ppHeaders = pHeaders;
	pHeaders = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pHeaders);
	SG_STRING_NULLFREE(pCtx, sKey);
	SG_STRING_NULLFREE(pCtx, sValue);
	return;
}

/**
 * Parses a single piece of data from a property section.
 */
static void parse__property_data(
	SG_context*          pCtx,    //< [in] [out] Error and context info.
	buffered_readstream* pStream, //< [in] Stream to read property data from.
	SG_int32*            pType,   //< [out] The type of the read data (K, V, or D).
	                              //<       Set to 0 if the section contains no more data.
	SG_string**          ppData   //< [out] The actual data read.
	                              //<       Set to NULL if the section contains no more data.
	)
{
	SG_string*  sMeta  = NULL;
	const char* szMeta = NULL;
	SG_int32    iType  = 0;
	SG_uint32   uSize  = 0u;
	SG_byte*    pData  = NULL;
	SG_string*  sData  = NULL;

	SG_NULLARGCHECK(pStream);
	SG_NULLARGCHECK(pType);
	SG_NULLARGCHECK(ppData);

	// read the first line, which specifies the type and size of the data
	SG_ERR_CHECK(  buffered_readstream__read_while(pCtx, pStream, "\r\n", SG_FALSE, &sMeta, NULL)  );
	SG_ERR_CHECK(  buffered_readstream__read_newline(pCtx, pStream)  );
	szMeta = SG_string__sz(sMeta);

	// check for the end of the property section
	if (strcmp(szMeta, gszProperty_End) != 0)
	{
		SG_uint32 uRead = 0u;

		// get the type of the data
		iType = szMeta[0u];
		++szMeta;
		if (
			   iType != gcProperty_Key
			&& iType != gcProperty_Value
			&& iType != gcProperty_Delete
			)
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Unknown property type: %c", (char)iType));
		}

		// find the start of the data's size
		while (*szMeta == ' ' || *szMeta == '\t')
		{
			++szMeta;
		}

		// parse the data's size
		SG_ERR_CHECK(  SG_uint32__parse__strict(pCtx, &uSize, szMeta)  );

		// read the specified amount of data into a string
		if (uSize > 0u)
		{
			SG_ERR_CHECK(  SG_allocN(pCtx, uSize + 1u, pData)  );
			SG_ERR_CHECK(  buffered_readstream__read(pCtx, pStream, uSize, pData, &uRead)  );
			if (uRead < uSize)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Actual size of property data (%u) doesn't match specified size (%u).", uRead, uSize));
			}
			pData[uSize] = '\0';

			// we're going to start treating the bytes as a string from here
			// This should only cause any weirdness if the data we read actually
			// contained non-terminating null characters.  In that case, the caller
			// might look at the returned string and think that it's shorter than
			// it really is.  However, I'm having a hard time finding any case where
			// you'd want a NULL character in the middle of a property name or value.

			// fix up the data to be UTF8 (replace bad bytes with '?')
			SG_ERR_CHECK(  SG_utf8__fix__sz(pCtx, pData)  );

			// wrap the data in a string
			SG_ERR_CHECK(  SG_string__alloc__adopt_buf(pCtx, &sData, (char**)&pData, uSize)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_string__alloc(pCtx, &sData)  );
		}

		// read the newline that's always present after the data (even if the data was zero-length)
		SG_ERR_CHECK(  buffered_readstream__read_newline(pCtx, pStream)  );
	}

	// return everything
	*pType = iType;
	*ppData = sData;
	sData = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sMeta);
	SG_NULLFREE(pCtx, pData);
	SG_STRING_NULLFREE(pCtx, sData);
	return;
}

/**
 * Parses a record's property section.
 */
static void parse__property_section(
	SG_context*          pCtx,          //< [in] [out] Error and context info.
	buffered_readstream* pStream,       //< [in] Stream to read the property section from.
	SG_uint64            uExpectedSize, //< [in] Expected size in bytes of the property section.
	SG_vhash**           ppProperties   //< [out] The read properties, mapping names to values.
	                                    //<       NULL values indicate a "delete" property.
	)
{
	SG_vhash*  pProperties = NULL;
	SG_uint64  uStart      = 0u;
	SG_bool    bEnd        = SG_FALSE;
	SG_string* sKey        = NULL;
	SG_string* sValue      = NULL;
	SG_uint64  uEnd        = 0u;

	SG_NULLARGCHECK(pStream);
	SG_NULLARGCHECK(ppProperties);

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pProperties)  );

	// remember where the section started
	SG_ERR_CHECK(  buffered_readstream__get_count(pCtx, pStream, &uStart)  );

	// read data until we reach the end of the section
	while (bEnd == SG_FALSE)
	{
		SG_int32 iKey = 0;

		// read the property's key
		SG_ERR_CHECK(  parse__property_data(pCtx, pStream, &iKey, &sKey)  );
		if (iKey == 0 || sKey == NULL)
		{
			// end of the section
			bEnd = SG_TRUE;
		}
		else if (iKey == gcProperty_Delete)
		{
			// a D property has no value, use NULL
			SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pProperties, SG_string__sz(sKey))  );
		}
		else if (iKey == gcProperty_Key)
		{
			SG_int32 iValue = 0;

			// a K property has a value, read it
			SG_ERR_CHECK(  parse__property_data(pCtx, pStream, &iValue, &sValue)  );
			if (iValue != gcProperty_Value)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Unknown or unexpected property data type: %c", (char)iValue));
			}
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pProperties, SG_string__sz(sKey), SG_string__sz(sValue))  );
		}
		else
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Unknown or unexpected property data type: %c", (char)iKey));
		}

		// free the data for reuse next iteration
		SG_STRING_NULLFREE(pCtx, sKey);
		SG_STRING_NULLFREE(pCtx, sValue);
	}

	// make sure the section was of the expected length
	SG_ERR_CHECK(  buffered_readstream__get_count(pCtx, pStream, &uEnd)  );
	if (uEnd - uStart != uExpectedSize)
	{
		SG_int_to_string_buffer szActual;
		SG_int_to_string_buffer szExpected;

		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Actual size of property section (%s) doesn't match specified size (%s).", SG_uint64_to_sz(uEnd - uStart, szActual), SG_uint64_to_sz(uExpectedSize, szExpected)));
	}

	// return the properties
	*ppProperties = pProperties;
	pProperties = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sKey);
	SG_STRING_NULLFREE(pCtx, sValue);
	SG_VHASH_NULLFREE(pCtx, pProperties);
	return;
}

/**
 * Reads a blob from a stream and potentially stores it to a repo.
 * Can also perform caller-specified actions on the blob data as it's read.
 */
static void parse__blob(
	SG_context*             pCtx,          //< [in] [out] Error and context info.
	buffered_readstream*    pStream,       //< [in] Stream to read the blob from.
	SG_uint64               uLength,       //< [in] Length of the blob to read.
	SG_byte*                pBuffer,       //< [in] Buffer to use to store blob chunks as they are read.
	SG_uint32               uBuffer,       //< [in] Size of pBuffer.
	SG_bool                 bDelta,        //< [in] Whether or not the blob is a delta.
	const char*             szBaseHid,     //< [in] If bDelta is true, this is the HID of the blob to apply the delta to.
	                                       //<      May be NULL even if bDelta is true (this happens with nodes being added).
	SG_repo*                pBaseRepo,     //< [in] Repo to read the base blob from.
	                                       //<      Ignored if szBaseHid is NULL.
	SG_repo*                pRepo,         //< [in] Repo to store the blob in.
	                                       //<      NULL to not store the blob in a repo.
	SG_repo_tx_handle*      pTransaction,  //< [in] Transaction to make the blob storage a part of.
	                                       //<      Ignored if pRepo is NULL.
	                                       //<      If NULL, a one-time transaction is used.
	import__blob__callback* fCallback,     //< [in] Callback to pass each blob chunk to.
	                                       //<      May be NULL if no actions are needed.
	void*                   pCallbackData, //< [in] Data to pass to fCallback.
	char**                  ppHid          //< [out] HID of the read blob.
	                                       //<       Ignored if pRepo is NULL.
	                                       //<       May be NULL if not needed.
	)
{
	if (bDelta == SG_FALSE)
	{
		// blob isn't a delta, use the basic blob reader
		SG_ERR_CHECK(  import__read_blob(pCtx, pStream, uLength, pBuffer, uBuffer, pRepo, pTransaction, fCallback, pCallbackData, ppHid)  );
	}
	else
	{
		// blob is a delta, we'll have to run it through svndiff
		SG_readstream*  pInputStream  = NULL;
		SG_writestream* pOutputStream = NULL;

		// if there's a base blob, open a stream to read from it
		if (szBaseHid != NULL)
		{
			SG_ERR_CHECK(  readstream__alloc__for_blob(pCtx, pBaseRepo, szBaseHid, SG_TRUE, NULL, NULL, NULL, NULL, &pInputStream)  );
		}

		// open a stream to write to an output blob
		SG_ERR_CHECK(  writestream__alloc__for_blob(pCtx, NULL, pRepo, pTransaction, ppHid, NULL, &pOutputStream)  );

		// read and process the delta
		SG_ERR_CHECK(  svndiff__process_delta(pCtx, pStream, uLength, pInputStream, pOutputStream, pBuffer, uBuffer, fCallback, pCallbackData)  );

		// close the streams, which will finalize the write
		// Note: We're doing this here rather than using NULLCLOSE in fail, because
		//       the writestream especially does real work while being closed and
		//       we want to more gracefully handle any errors that occur.  The
		//       only drawback should be that we'll leak their memory if an error
		//       occurs during the delta processing.
		SG_ERR_CHECK(  SG_readstream__close(pCtx, pInputStream)  );
		SG_ERR_CHECK(  SG_writestream__close(pCtx, pOutputStream)  );
	}

fail:
	return;
}

/**
 * Data used by blob_callback__verify_hashes.
 */
typedef struct parse__blob__verify_hashes__data
{
	// parameters
	const char* szExpectedMd5;  //< [in] Expected MD5 hash value.
	const char* szExpectedSha1; //< [in] Expected SHA1 hash value.
	SG_bool     bMatchMd5;      //< [out] Whether or not the MD5 hash matched expectation.
	SG_bool     bMatchSha1;     //< [out] Whether or not the SHA1 hash matched expectation.

	// used internally
	SGHASH_handle* pMd5;                         //< Handle for the MD5 hash in progress.
	SGHASH_handle* pSha1;                        //< Handle for the SHA1 hash in progress.
	char           szActualMd5[SIZE_MD5 + 1u];   //< Buffer to store the actual MD5 hash.
	char           szActualSha1[SIZE_SHA1 + 1u]; //< Buffer to store the actual SHA1 hash.
}
parse__blob__verify_hashes__data;

/**
 * An import__blob__callback that verifies that hashing the blob gets an expected result.
 * Can verify MD5 and SHA1.
 */
static void parse__blob__verify_hashes(
	SG_context* pCtx,
	void*       pCallbackData,
	SG_byte*    pBuffer,
	SG_uint32   uBuffer
	)
{
	parse__blob__verify_hashes__data* pThis   = (parse__blob__verify_hashes__data*)pCallbackData;
	SG_error                          eResult = SG_ERR_OK;

#define HASH_ERR_CHECK(EXPRESSION) \
	eResult = EXPRESSION;          \
	if (SG_IS_ERROR(eResult))      \
	{                              \
		SG_ERR_THROW(eResult);     \
	}

	if (pBuffer != NULL)
	{
		// add this chunk to any hashes we have going
		if (pThis->pMd5 != NULL)
		{
			HASH_ERR_CHECK(  SGHASH_update(pThis->pMd5, pBuffer, uBuffer)  );
		}
		if (pThis->pSha1 != NULL)
		{
			HASH_ERR_CHECK(  SGHASH_update(pThis->pSha1, pBuffer, uBuffer)  );
		}
	}
	else if (uBuffer == IMPORT__BLOB__BEGIN)
	{
		// initialize our data
		pThis->bMatchMd5        = SG_TRUE;
		pThis->bMatchSha1       = SG_TRUE;
		pThis->pMd5             = NULL;
		pThis->pSha1            = NULL;
		pThis->szActualMd5[0u]  = '\0';
		pThis->szActualSha1[0u] = '\0';

		// start any hashes that we're going to need
		if (pThis->szExpectedMd5 != NULL)
		{
			HASH_ERR_CHECK(  SGHASH_init(gszHashName_Md5, &pThis->pMd5)  );
		}
		if (pThis->szExpectedSha1 != NULL)
		{
			HASH_ERR_CHECK(  SGHASH_init(gszHashName_Sha1, &pThis->pSha1)  );
		}
	}
	else if (uBuffer == IMPORT__BLOB__END)
	{
		// finalize any hashes we have going and match them against expectations
		if (pThis->pMd5 != NULL)
		{
			HASH_ERR_CHECK(  SGHASH_final(&pThis->pMd5, pThis->szActualMd5, sizeof(pThis->szActualMd5))  );
			pThis->bMatchMd5 = (strcmp(pThis->szExpectedMd5, pThis->szActualMd5) == 0) ? SG_TRUE : SG_FALSE;
		}
		if (pThis->pSha1 != NULL)
		{
			HASH_ERR_CHECK(  SGHASH_final(&pThis->pSha1, pThis->szActualSha1, sizeof(pThis->szActualSha1))  );
			pThis->bMatchSha1 = (strcmp(pThis->szExpectedSha1, pThis->szActualSha1) == 0) ? SG_TRUE : SG_FALSE;
		}
	}
	else if (uBuffer == IMPORT__BLOB__FAIL)
	{
		// abort any handles we have going
		if (pThis->pMd5 != NULL)
		{
			SGHASH_abort(&pThis->pMd5);
		}
		if (pThis->pSha1 != NULL)
		{
			SGHASH_abort(&pThis->pSha1);
		}
	}

#undef HASH_ERR_CHECK

fail:
	return;
}


/*
**
** Dump Management
**
*/

/**
 * Frees a dump's path data but not the path itself.
 */
static void dump__path__free_contents(
	SG_context* pCtx, //< [in] [out] Error and context info.
	dump__path* pThis //< [in] Data to free.
	)
{
	if (pThis != NULL)
	{
		SG_VECTOR_NULLFREE(pCtx, pThis->pRevisionMap);
		SG_STRPOOL_NULLFREE(pCtx, pThis->pChangesets);
		SG_NULLFREE(pCtx, pThis->szCommitting);
	}
}

/**
 * Frees a dump's path data including the path itself.
 */
static void dump__path__void_free(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pThis //< [in] Data to free.
	)
{
	if (pThis != NULL)
	{
		dump__path__free_contents(pCtx, (dump__path*)pThis);
		SG_NULLFREE(pCtx, pThis);
	}
}

/**
 * Initializes a dump path.
 */
static void dump__path__init(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	dump__path* pThis,    //< [in] Dump path to init.
	dump__repo  eRepo,    //< [in] Repo that the path will be committed to.
	const char* szParent, //< [in] HID of the existing parent to start importing this path from.
	const char* szBranch, //< [in] Branch that the path will be committed on.
	                      //<      NULL to not associate changesets imported from this path with a branch.
	const char* szStamp,  //< [in] Stamp to apply to all changesets imported from this path.
	                      //<      NULL to not apply a stamp.
	SG_bool     bComment  //< [in] Whether or not to add an extra comment to changesets imported from this path.
	                      //<      The extra comment contains the SVN revision the changeset came from.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szParent);

	pThis->eRepo            = eRepo;
	pThis->szNextParent     = NULL;
	pThis->szBranch         = szBranch;
	pThis->szStamp          = szStamp;
	pThis->bComment         = bComment;
	pThis->szPathGid[0u]    = '\0';
	pThis->szSvnGid[0u]     = '\0';
	pThis->uBaseRevision    = 0u;
	pThis->pRevisionMap     = NULL;
	pThis->szBranchHead     = NULL;
	pThis->bExistsInLatest  = SG_FALSE;
	pThis->szCommitting     = NULL;
	pThis->szCommittingPool = NULL;

	// copy the given parent HID into our pool to initialize our next parent
	SG_STRPOOL__ALLOC(pCtx, &pThis->pChangesets, SIZE_HID_STRPOOL);
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pThis->pChangesets, szParent, &pThis->szNextParent)  );

fail:
	return;
}

/**
 * Gets the HID of a changeset already imported from this path by its SVN revision number.
 */
static void dump__path__get_revision(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	const dump__path* pThis,     //< [in] Path that the revision was imported from.
	SG_uint64         uRevision, //< [in] SVN revision number to get.
	const char**      ppHid      //< [out] Changeset HID from the given path corresponding to the SVN revision.
	                             //<       NULL if the revision wasn't imported (may have been skipped, may not be done yet).
	)
{
	const char* szHid = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(ppHid);

	// check if any changesets had even been imported from this path, as of now and as of the given revision
	// if not, then this path definitely didn't exist at that revision
	if (pThis->pRevisionMap != NULL && uRevision >= pThis->uBaseRevision)
	{
		SG_uint32   uSize    = 0u;
		SG_uint64   uIndex64 = 0u;
		SG_uint32   uIndex   = 0u;

		// find the index where this revision would be
		uIndex64 = uRevision - pThis->uBaseRevision;
		SG_ARGCHECK(uIndex64 < SG_UINT32_MAX, uRevision);
		uIndex = (SG_uint32)uIndex64;

		// look up the revision at the index
		SG_ERR_CHECK(  SG_vector__length(pCtx, pThis->pRevisionMap, &uSize)  );
		if (uIndex < uSize)
		{
			SG_ERR_CHECK(  SG_vector__get(pCtx, pThis->pRevisionMap, uIndex, (void**)&szHid)  );
		}
	}

	// return the result
	*ppHid = szHid;
	szHid = NULL;

fail:
	return;
}

/**
 * Adds a new imported revision to the path's cache.
 */
static void dump__path__set_revision(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	dump__path* pThis,     //< [in] Path that the revision was imported from.
	SG_uint64   uRevision, //< [in] SVN revision number to add.
	const char* szHid      //< [in] Equivalent Veracity HID to associate with the SVN revision.
	                       //<      NULL if the path didn't exist in the revision.
	                       //<      If non-NULL, must point into pThis->pChangesets!
	)
{
	SG_uint64 uIndex64 = 0u;
	SG_uint32 uIndex   = 0u;

	SG_NULLARGCHECK(pThis);
	SG_ARGCHECK(uRevision >= pThis->uBaseRevision || pThis->uBaseRevision == 0u, uRevision);

	// if this is our first revision, setup initial data
	if (pThis->pRevisionMap == NULL)
	{
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pThis->pRevisionMap, 1024u)  );
		pThis->uBaseRevision = uRevision;
	}

	// find the index where this revision should be stored
	uIndex64 = uRevision - pThis->uBaseRevision;
	if (uIndex64 >= SG_UINT32_MAX - 1u)
	{
		SG_ERR_THROW2(SG_ERR_BUFFERTOOSMALL, (pCtx, "Cannot import more than %u continuous revision numbers.", SG_UINT32_MAX - 1u));
	}
	uIndex = (SG_uint32)uIndex64;

	// add the revision data at the appropriate index
	SG_ERR_CHECK(  SG_vector__reserve(pCtx, pThis->pRevisionMap, uIndex + 1u)  );
	SG_ERR_CHECK(  SG_vector__set(pCtx, pThis->pRevisionMap, uIndex, (void*)szHid)  );

fail:
	return;
}

/**
 * Frees a dump.
 */
static void dump__free(
	SG_context* pCtx, //< [in] [out] Error and context info.
	dump*       pThis //< [in] Data to free from.
	)
{
	if (pThis != NULL)
	{
		SG_uint32 uRepo = 0u;

		ERR_LOG_IGNORE(  dump__path__free_contents(pCtx, &pThis->oRootPath)  );
		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pThis->pPaths, dump__path__void_free);

#if defined(WINDOWS)
		// This is a total hack around some kind of undiscovered race condition
		// on Windows between activity in a repo and deleting it.  If we don't
		// wait here, there's a decent chance that deleting any temporary repos
		// will fail on Windows saying that the directory is not empty.  When that
		// happens, a single zero-length SQLite file remains in the repo's folder,
		// and the repo is not removed from the list of descriptors.  Therefore, it
		// continues to show up in the repo list, but it cannot be opened (and
		// therefore cannot be deleted with `vv repo delete`) because all its data
		// is basically gone.  There must be a race condition between that SQLite
		// database being closed/released/whatever and our trying to delete the
		// folder.  This same problem can occur with fast-import (create a new repo,
		// fast-import into it, and then delete it right away), so it's not
		// specific to something that we're doing in the SVN importer.
		// Waiting 1s cuts down on the errors but doesn't eliminiate them, while
		// waiting 10s seems to be enough to get rid of them.  Might be overkill,
		// but it's better than leaving people with temporary repos that they can't
		// easily delete.  Plus, what's 10 more seconds after an import that's
		// already lasted on the order of minutes or hours?
		{
			SG_uint32 uTempRepos = 0u;

			for (uRepo = 0u; uRepo < DUMP__REPO__COUNT; ++uRepo)
			{
				if (uRepo != DUMP__REPO__IMPORT && pThis->aRepos[uRepo].pRepo != NULL)
				{
					++uTempRepos;
				}
			}

			if (uTempRepos > 0u)
			{
				SG_sleep_ms(10000u);
			}
		}
#endif

		for (uRepo = 0u; uRepo < DUMP__REPO__COUNT; ++uRepo)
		{
			if (uRepo != DUMP__REPO__IMPORT)
			{
				ERR_LOG_IGNORE(  import__delete_repo(pCtx, &pThis->aRepos[uRepo].pRepo)  );
			}
			SG_VHASH_NULLFREE(pCtx, pThis->aRepos[uRepo].pUsers);
		}

		SG_NULLFREE(pCtx, pThis->pBlobBuffer);

		if (pThis->bOperation != SG_FALSE)
		{
			SG_log__pop_operation(pCtx);
		}
	}
}

/**
 * Implementation of pending_node__get_repo_data that retrieves repos from a dump.
 */
static void dump__get_repo_data(
	SG_context*         pCtx,
	SG_uint32           uRepo,
	void*               pDump,
	SG_repo**           ppRepo,
	SG_repo_tx_handle** ppTransaction,
	SG_changeset**      ppChangeset
	)
{
	const dump* pThis = (const dump*)pDump;

	SG_UNUSED(ppTransaction);
	SG_UNUSED(ppChangeset);

	SG_ARGCHECK(uRepo < DUMP__REPO__COUNT, uRepo);
	SG_NULLARGCHECK(pDump);

	if (ppRepo != NULL)
	{
		*ppRepo = pThis->aRepos[uRepo].pRepo;
	}
	SG_ASSERT(ppTransaction == NULL);
	SG_ASSERT(ppChangeset == NULL);

fail:
	return;
}

/**
 * Initializes a dump by reading a version record.
 */
static void dump__init(
	SG_context*          pCtx,     //< [in] [out] Error and context info.
	dump*                pThis,    //< [in] Dump to init.
	SG_repo*             pRepo,    //< [in] Repo that the dump will be imported into.
	const char*          szParent, //< [in] HID of the revision to start importing from.
	const char*          szBranch, //< [in] Branch to make each imported revision a head of.
	                               //<      NULL to not make imported revisions part of a branch.
	const char*          szPath,   //< [in] SVN path to import from, or NULL to import everything.
	const char*          szStamp,  //< [in] Stamp to apply to each imported revision.
	                               //<      NULL to not stamp imported revisions.
	SG_bool              bComment, //< [in] Whether or not to add an extra comment to each imported revision.
	                               //<      The extra comment contains the source SVN revision.
	buffered_readstream* pStream   //< [in] Stream to read the initial version record from.
	)
{
	char*               szCacheParent  = NULL;
	dump__path*         pPath          = NULL;
	char*               szSuperRoot    = NULL;
	pending_node*       pSuperRoot     = NULL;
	pending_node*       pPathSuperRoot = NULL;
	SG_rbtree_iterator* pIterator      = NULL;
	SG_uint32           uRepo          = DUMP__REPO__COUNT;
	SG_vhash*           pHeaders       = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(szParent);

	// start an operation associated with the entire import
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Importing", SG_LOG__FLAG__NONE)  );
	pThis->bOperation = SG_TRUE;
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 0u, "revisions")  );

	// store the import repo
	pThis->aRepos[DUMP__REPO__IMPORT].pRepo = pRepo;

	// check if we'll be importing a specific path
	if (szPath == NULL)
	{
		// no need for a cache repo
		pThis->aRepos[DUMP__REPO__CACHE].pRepo  = NULL;
		pThis->aRepos[DUMP__REPO__CACHE].pUsers = NULL;

		// initialize the root path to send everything to the import repo
		SG_ERR_CHECK(  dump__path__init(pCtx, &pThis->oRootPath, DUMP__REPO__IMPORT, szParent, szBranch, szStamp, bComment)  );

		// add some data to our operation
		SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Start Revision", pThis->oRootPath.szNextParent, SG_LOG__FLAG__INPUT)  );
		if (pThis->oRootPath.szBranch != NULL)
		{
			SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Target Branch", pThis->oRootPath.szBranch, SG_LOG__FLAG__INPUT)  );
		}
	}
	else
	{
		// we'll import everything outside the path into a cache repo
		// to make that work, we'll setup the root to commit to the cache repo
		// and then setup the path they specified to commit to the import repo
		SG_NULLARGCHECK(*szPath);

		// create the cache repo
		SG_ERR_CHECK(  import__create_mirror_repo(pCtx, pRepo, szParent, gszCacheRepoNameFormat, &pThis->aRepos[DUMP__REPO__CACHE].pRepo, &szCacheParent)  );

		// initialize the root path to send everything to the cache repo
		SG_ERR_CHECK(  dump__path__init(pCtx, &pThis->oRootPath, DUMP__REPO__CACHE, szCacheParent, SG_VC_BRANCHES__DEFAULT, NULL, SG_FALSE)  );

		// create a specific import path for the path they want
		SG_ERR_CHECK(  SG_alloc1(pCtx, pPath)  );
		SG_ERR_CHECK(  dump__path__init(pCtx, pPath, DUMP__REPO__IMPORT, szParent, szBranch, szStamp, bComment)  );
		SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Start Revision", pPath->szNextParent, SG_LOG__FLAG__INPUT)  );

		// create a list of paths and add this one to it
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->pPaths)  );
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pThis->pPaths, szPath, (void*)pPath)  );
		pPath = NULL;
		SG_ERR_CHECK(  SG_rbtree__key(pCtx, pThis->pPaths, szPath, &szPath)  );
		SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Source Path", szPath, SG_LOG__FLAG__INPUT)  );
	}

	// if we're loading specific paths, finish configuring them
	pThis->oRootPath.bExistsInLatest = SG_TRUE;
	if (pThis->pPaths != NULL)
	{
		pending_node* pFileRoot     = NULL;
		SG_bool       bIterator     = SG_FALSE;
		const char*   szCurrentPath = NULL;
		dump__path*   pCurrentPath  = NULL;

		// load up the tree that already exists where we'll be importing the SVN root
		// we'll need to reference this from each individual path
		SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pThis->aRepos[pThis->oRootPath.eRepo].pRepo, pThis->oRootPath.szNextParent, &szSuperRoot)  );
		SG_ERR_CHECK(  pending_node__alloc__super_root(pCtx, &pSuperRoot, pThis->oRootPath.eRepo, &szSuperRoot)  );
		SG_ERR_CHECK(  pending_node__walk_path(pCtx, pSuperRoot, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis, &pFileRoot)  );

		// run through each specific path and configure it
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pPaths, &bIterator, &szCurrentPath, (void**)&pCurrentPath)  );
		while (bIterator != SG_FALSE)
		{
			pending_node* pPendingNode = NULL;
			const char*   szGid        = NULL;

			// find the file root of this path's individual tree
			// We need to know its GID because we'll be replacing that file root
			// with our own node from the SVN tree when committing from this path.
			SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pThis->aRepos[pCurrentPath->eRepo].pRepo, pCurrentPath->szNextParent, &szSuperRoot)  );
			SG_ERR_CHECK(  pending_node__alloc__super_root(pCtx, &pPathSuperRoot, pCurrentPath->eRepo, &szSuperRoot)  );
			SG_ERR_CHECK(  pending_node__walk_path(pCtx, pPathSuperRoot, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis, &pPendingNode)  );
			SG_ERR_CHECK(  pending_node__get_gid(pCtx, pPendingNode, &szGid)  );
			SG_ERR_CHECK(  SG_strcpy(pCtx, pCurrentPath->szPathGid, SG_GID_BUFFER_LENGTH, szGid)  );
			PENDING_NODE_NULLFREE(pCtx, pPathSuperRoot);

			// find this path in the pre-existing SVN tree, if it exists
			SG_ERR_CHECK(  pending_node__walk_path(pCtx, pFileRoot, szCurrentPath, SG_STRLEN(szCurrentPath), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis, &pPendingNode)  );
			if (pPendingNode == NULL)
			{
				// doesn't exist yet
				// we won't know its GID until the import process adds it
				pCurrentPath->bExistsInLatest = SG_FALSE;
			}
			else
			{
				// this path already exists, so we can already know its GID
				pCurrentPath->bExistsInLatest = SG_TRUE;
				SG_ERR_CHECK(  pending_node__get_gid(pCtx, pPendingNode, &szGid)  );
				SG_ERR_CHECK(  SG_strcpy(pCtx, pCurrentPath->szSvnGid, SG_GID_BUFFER_LENGTH, szGid)  );
			}

			// move on to the next path
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szCurrentPath, (void**)&pCurrentPath)  );
		}
	}

	// fill the user cache for each repo we're using
	for (uRepo = 0u; uRepo < DUMP__REPO__COUNT; ++uRepo)
	{
		if (pThis->aRepos[uRepo].pRepo != NULL)
		{
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pThis->aRepos[uRepo].pUsers)  );
			SG_ERR_CHECK(  import__fill_user_cache(pCtx, pThis->aRepos[uRepo].pUsers, pThis->aRepos[uRepo].pRepo)  );
		}
	}

	// allocate a buffer for reading chunks of blobs
	pThis->uBlobBuffer = SIZE_BLOB_BUFFER;
	SG_ERR_CHECK(  SG_allocN(pCtx, pThis->uBlobBuffer, pThis->pBlobBuffer)  );

	// parse the headers
	SG_ERR_CHECK(  parse__headers(pCtx, pStream, &pHeaders)  );
	SG_ERR_CHECK(  parse__hash_value__uint32(pCtx, pHeaders, gszDump_Version, &pThis->uFormatVersion)  );
	SG_ERR_CHECK(  SG_log__set_value__int(pCtx, "Dump Format Version", pThis->uFormatVersion, SG_LOG__FLAG__INTERMEDIATE | SG_LOG__FLAG__VERBOSE)  );

	// blank out other data
	pThis->szUuid[0u] = '\0';

fail:
	SG_NULLFREE(pCtx, szCacheParent);
	dump__path__void_free(pCtx, (void*)pPath);
	SG_NULLFREE(pCtx, szSuperRoot);
	PENDING_NODE_NULLFREE(pCtx, pSuperRoot);
	PENDING_NODE_NULLFREE(pCtx, pPathSuperRoot);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	SG_VHASH_NULLFREE(pCtx, pHeaders);
	return;
}

/**
 * Relates a given path to the dump's import path.
 */
static void dump__get_path_data(
	SG_context*  pCtx,   //< [in] [out] Error and context info.
	dump*        pThis,  //< [in] Dump to get path data from.
	const char*  szPath, //< [in] Path to get import data about.
	dump__path** ppPath  //< [out] Import data related to the given path.
	)
{
	SG_rbtree_iterator* pIterator = NULL;
	dump__path*         pPath     = &pThis->oRootPath;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(ppPath);

	if (pThis->pPaths != NULL)
	{
		SG_bool     bIterator     = SG_FALSE;
		const char* szCurrentPath = NULL;
		dump__path* pCurrentPath  = NULL;

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pPaths, &bIterator, &szCurrentPath, (void**)&pCurrentPath)  );
		while (bIterator != SG_FALSE)
		{
			import__path_relation eRelation = IMPORT__PATH_RELATION__COUNT;

			SG_ERR_CHECK(  import__relate_path(pCtx, szCurrentPath, szPath, &eRelation, IMPORT__PATH_RELATION__COUNT)  );
			if (eRelation == IMPORT__PATH_RELATION__INSIDE || eRelation == IMPORT__PATH_RELATION__EQUAL)
			{
				pPath = pCurrentPath;
				break;
			}

			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szCurrentPath, (void**)&pCurrentPath)  );
		}
	}

	*ppPath = pPath;
	pPath = NULL;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	return;
}

/**
 * Callback used by dump__load_svn_tree to translate a dump__path into the changeset HID to load that path from.
 */
typedef void (dump__load_svn_tree__callback)(
	SG_context*       pCtx,  //< [in] [out] Error and context info.
	const dump__path* pPath, //< [in] Path to get the changeset HID for.
	void*             pData, //< [in] Data provided by the caller.
	const char**      ppHid  //< [out] Changeset HID for the specified path.
	                         //<       NULL if the path doesn't exist.
	);

/**
 * Implementation of dump__load_svn_tree__callback that returns the path's szNextParent member.
 */
static void dump__load_svn_tree__parent(
	SG_context*       pCtx,  //< [in] [out] Error and context info.
	const dump__path* pPath, //< [in] Path to get the changeset HID for.
	void*             pData, //< [in] Ignored.
	const char**      ppHid  //< [out] Changeset HID for the specified path's next parent.
	                         //<       NULL if the path doesn't exist in the next parent.
	)
{
	SG_NULLARGCHECK(pPath);
	SG_UNUSED(pData);
	SG_NULLARGCHECK(ppHid);

	if (pPath->bExistsInLatest == SG_FALSE)
	{
		*ppHid = NULL;
	}
	else
	{
		*ppHid = pPath->szNextParent;
	}

fail:
	return;
}

/**
 * Implementation of dump__load_svn_tree__callback that returns a HID from the path's pRevisionMap.
 */
static void dump__load_svn_tree__past(
	SG_context*       pCtx,  //< [in] [out] Error and context info.
	const dump__path* pPath, //< [in] Path to get the changeset HID for.
	void*             pData, //< [in] SG_uint64* that points to the SVN revision ID to get the HID for.
	const char**      ppHid  //< [out] Changeset HID for the specified path.
	                         //<       NULL if the path doesn't exist in the specified revision.
	)
{
	SG_uint64* pRevision = (SG_uint64*)pData;

	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(pData);
	SG_NULLARGCHECK(ppHid);

	SG_ERR_CHECK(  dump__path__get_revision(pCtx, pPath, *pRevision, ppHid)  );

fail:
	return;
}

/**
 * Loads an SVN pending_node tree.
 */
static void dump__load_svn_tree(
	SG_context*                    pCtx,            //< [in] [out] Error and context info.
	const dump*                    pThis,           //< [in] Dump that the changeset was imported during.
	dump__load_svn_tree__callback* fCallback,       //< [in] Callback that translates dump__path pointers into the changeset HID to load that SVN path from.
	                                                //<      dump__load_svn_tree__parent loads from the HIDs that will be the next revision's parent (in other words, the latest).
	                                                //<      dump__load_svn_tree__past loads from the HIDs imported from a specific SVN revision.
	void*                          fCallbackData,   //< [in] Data to pass to fCallback.
	                                                //<      dump__load_svn_tree__parent ignores this parameter.
	                                                //<      dump__load_svn_tree__past requires that this be an SG_uint64* that points to an already imported SVN revision ID.
	pending_node**                 ppSuperRoot,     //< [out] Super-root node of the specified revision (parent of the "@" node).
	                                                //<       Caller is responsible for freeing this.
	pending_node**                 ppFileRoot,      //< [out] Root "@" node of the files in the specified revision.
	                                                //<       May be NULL if not needed.
	SG_rbtree**                    ppPathSuperRoots //< [out] Super-root nodes of specific subpaths being loaded from different places.
	                                                //<       Maps the subpath to its super-root pending_node*.
	                                                //<       The super-root's child will have been moved into the main tree returned in ppSuperRoot, if it belongs there.
	)
{
	const char*         szChangeset       = NULL;
	char*               szSuperRoot       = NULL;
	pending_node*       pSuperRoot        = NULL;
	pending_node*       pFileRoot         = NULL;
	SG_rbtree_iterator* pIterator         = NULL;
	pending_node*       pCurrentSuperRoot = NULL;
	SG_rbtree*          pPathSuperRoots   = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(ppSuperRoot);

	// if the caller wants the individual super-roots back, start a list
	if (ppPathSuperRoots != NULL)
	{
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pPathSuperRoots)  );
	}

	// get the changeset HID of the root path
	SG_ERR_CHECK(  fCallback(pCtx, &pThis->oRootPath, fCallbackData, &szChangeset)  );
	SG_ASSERT(szChangeset != NULL);

	// find and load its root treenode into a super-root and file root pending_nodes
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pThis->aRepos[pThis->oRootPath.eRepo].pRepo, szChangeset, &szSuperRoot)  );
	SG_ERR_CHECK(  pending_node__alloc__super_root(pCtx, &pSuperRoot, pThis->oRootPath.eRepo, &szSuperRoot)  );
	SG_ERR_CHECK(  pending_node__walk_path(pCtx, pSuperRoot, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis, &pFileRoot)  );

	// if we're importing specific paths, run through each one and add its tree from the given SVN revision into the one we're loading
	if (pThis->pPaths != NULL)
	{
		SG_bool     bIterator     = SG_FALSE;
		const char* szCurrentPath = NULL;
		dump__path* pCurrentPath  = NULL;

		// run through each path
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pPaths, &bIterator, &szCurrentPath, (void**)&pCurrentPath)  );
		while (bIterator != SG_FALSE)
		{
			SG_uint32     uParentLength = 0u;
			pending_node* pParent       = NULL;

			// get the changeset HID of this path
			// if we don't get one, it means this path doesn't exist in the SVN tree
			SG_ERR_CHECK(  fCallback(pCtx, pCurrentPath, fCallbackData, &szChangeset)  );
			if (szChangeset != NULL)
			{
				pending_node* pCurrentFileRoot = NULL;
				const char*   szCurrentName    = NULL;
				SG_uint32     uCurrentName     = 0u;

				// load the root treenode from this changeset into a super-root pending_node
				SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pThis->aRepos[pCurrentPath->eRepo].pRepo, szChangeset, &szSuperRoot)  );
				SG_ERR_CHECK(  pending_node__alloc__super_root(pCtx, &pCurrentSuperRoot, pCurrentPath->eRepo, &szSuperRoot)  );

				// find the parent node in the SVN tree where we want to place this path's tree
				SG_ERR_CHECK(  import__find_parent_path_separator(pCtx, szCurrentPath, SG_STRLEN(szCurrentPath), &uParentLength)  );
				SG_ERR_CHECK(  pending_node__walk_path(pCtx, pFileRoot, szCurrentPath, uParentLength, PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis, &pParent)  );
				SG_ASSERT(pParent != NULL);
				// If there's no parent to attach this path's node to,
				// then this path can't exist in the SVN tree,
				// and the callback shouldn't have returned us a changeset.

				// find the file root that we actually want to move into the SVN tree
				SG_ERR_CHECK(  pending_node__walk_path(pCtx, pCurrentSuperRoot, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis, &pCurrentFileRoot)  );
				SG_ASSERT(pCurrentFileRoot != NULL);

				// find the name portion of the current path
				szCurrentName = szCurrentPath;
				if (uParentLength != 0u)
				{
					szCurrentName += uParentLength + 1u;
				}
				uCurrentName = SG_STRLEN(szCurrentName);
				while (import__is_path_separator(szCurrentName[uCurrentName - 1u]) == SG_TRUE)
				{
					// ignore any trailing path separators
					--uCurrentName;
				}

				// move this path's file root into the SVN tree at the found parent
				// and change its identity to what it should be in the SVN tree
				SG_ERR_CHECK(  pending_node__load_children(pCtx, pParent, SG_FALSE, dump__get_repo_data, (void*)pThis)  );
				SG_ERR_CHECK(  pending_node__move(pCtx, pCurrentFileRoot, pParent, pCurrentPath->szSvnGid, szCurrentName, uCurrentName, SG_FALSE)  );

				// decide what to do with the loaded super-root
				if (pPathSuperRoots != NULL)
				{
					// caller wants it, add it to the list
					SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pPathSuperRoots, szCurrentPath, pCurrentSuperRoot)  );
					pCurrentSuperRoot = NULL;
				}
				else
				{
					// don't need it anymore
					PENDING_NODE_NULLFREE(pCtx, pCurrentSuperRoot);
				}
			}

			// move on to the next path
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szCurrentPath, (void**)&pCurrentPath)  );
		}
	}

	// return results
	*ppSuperRoot = pSuperRoot;
	pSuperRoot = NULL;
	if (ppFileRoot != NULL)
	{
		*ppFileRoot = pFileRoot;
		pFileRoot = NULL;
	}
	if (ppPathSuperRoots != NULL)
	{
		SG_ASSERT(pPathSuperRoots != NULL);
		*ppPathSuperRoots = pPathSuperRoots;
		pPathSuperRoots = NULL;
	}

fail:
	SG_NULLFREE(pCtx, szSuperRoot);
	PENDING_NODE_NULLFREE(pCtx, pSuperRoot);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	PENDING_NODE_NULLFREE(pCtx, pCurrentSuperRoot);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pPathSuperRoots, pending_node__void_free);
	return;
}

/**
 * Reads a UUID record into the dump.
 */
static void dump__uuid(
	SG_context*          pCtx,   //< [in] [out] Error and context info.
	dump*                pThis,  //< [in] Dump to read a UUID into.
	buffered_readstream* pStream //< [in] Stream to read from.
	)
{
	SG_vhash* pHeaders = NULL;

	// parse the headers
	SG_ERR_CHECK(  parse__headers(pCtx, pStream, &pHeaders)  );
	SG_ERR_CHECK(  parse__hash_value__uuid(pCtx, pHeaders, gszDump_Uuid, pThis->szUuid)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Source Repository UUID", pThis->szUuid, SG_LOG__FLAG__INTERMEDIATE | SG_LOG__FLAG__VERBOSE)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pHeaders);
}


/*
**
** Revision Management
**
*/

/**
 * Frees a revision.
 */
static void revision__free(
	SG_context* pCtx, //< [in] [out] Error and context info.
	revision*   pThis //< [in] Revision to free.
	)
{
	if (pThis != NULL)
	{
		SG_uint32 uRepo = 0u;

		SG_VHASH_NULLFREE(pCtx, pThis->pProperties);
		for (uRepo = 0u; uRepo < DUMP__REPO__COUNT; ++uRepo)
		{
			if (pThis->aRepos[uRepo].pTransaction != NULL)
			{
				SG_repo__abort_tx(pCtx, pThis->pDump->aRepos[uRepo].pRepo, &pThis->aRepos[uRepo].pTransaction);
			}
		}
		PENDING_NODE_NULLFREE(pCtx, pThis->pSuperRoot);
		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pThis->pPathSuperRoots, pending_node__void_free);
		if (pThis->bOperation != SG_FALSE)
		{
			SG_log__pop_operation(pCtx);
		}
	}
}

/**
 * Inits a revision by reading a revision record.
 */
static void revision__init(
	SG_context*          pCtx,    //< [in] [out] Error and context info.
	revision*            pThis,   //< [in] Revision to init.
	buffered_readstream* pStream, //< [in] Stream to read the revision record from.
	dump*                pDump    //< [in] Dump that owns the revision.
	)
{
	SG_vhash*               pHeaders   = NULL;
	SG_int_to_string_buffer szId;
	char*                   szUsername = NULL;
	char*                   szUserId   = NULL;
	SG_uint32               uRepo      = 0u;
	char*                   szHid      = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pStream);
	SG_NULLARGCHECK(pDump);

	// start an operation associated with the revision
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, NULL, SG_LOG__FLAG__NONE)  );
	pThis->bOperation = SG_TRUE;
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 0u, "nodes")  );

	// store the dump
	pThis->pDump = pDump;

	// parse the headers
	SG_ERR_CHECK(  parse__headers(pCtx, pStream, &pHeaders)  );
	SG_ERR_CHECK(  parse__hash_value__uint64(pCtx, pHeaders, gszRevision_Id, &pThis->uId)  );
	SG_ERR_CHECK(  parse__hash_value__uint64(pCtx, pHeaders, gszRevision_PropertySize, &pThis->uPropertySize)  );
	SG_ERR_CHECK(  parse__hash_value__uint64(pCtx, pHeaders, gszRevision_ContentSize,  &pThis->uContentSize)  );
	if (pThis->uPropertySize != pThis->uContentSize)
	{
		SG_int_to_string_buffer szContentSize;
		SG_int_to_string_buffer szPropertySize;
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Revision record's content-length (%s) doesn't match its prop-content-length (%s).", SG_uint64_to_sz(pThis->uContentSize, szContentSize), SG_uint64_to_sz(pThis->uPropertySize, szPropertySize)));
	}

	// give our operation a better description
	SG_ERR_CHECK(  SG_sprintf(pCtx, pThis->szOperation, sizeof(pThis->szOperation), "Revision %s", SG_uint64_to_sz(pThis->uId, szId))  );
	SG_ERR_CHECK(  SG_log__set_operation(pCtx, pThis->szOperation)  );

	// parse the properties
	SG_ERR_CHECK(  parse__property_section(pCtx, pStream, pThis->uPropertySize, &pThis->pProperties)  );
	SG_ERR_CHECK(  parse__hash_value__time__optional(pCtx, pThis->pProperties, gszRevision_Time, &pThis->iTime, 0)  );
	if (pThis->iTime != 0)
	{
		SG_ERR_CHECK(  SG_log__set_value__int(pCtx, "Timestamp", pThis->iTime, SG_LOG__FLAG__INTERMEDIATE | SG_LOG__FLAG__VERBOSE)  );
	}
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pThis->pProperties, gszRevision_User, &pThis->szUser)  );
	if (pThis->szUser != NULL)
	{
		SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Author", pThis->szUser, SG_LOG__FLAG__INTERMEDIATE | SG_LOG__FLAG__VERBOSE)  );
	}
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pThis->pProperties, gszRevision_Message, &pThis->szMessage)  );
	if (pThis->szMessage != NULL && strspn(pThis->szMessage, " \t\r\n") == SG_STRLEN(pThis->szMessage))
	{
		pThis->szMessage = NULL;
	}

	// read the terminating empty line
	SG_ERR_CHECK(  buffered_readstream__read_newline(pCtx, pStream)  );

	// setup any repo-specific data that's necessary
	for (uRepo = 0u; uRepo < DUMP__REPO__COUNT; ++uRepo)
	{
		if (pDump->aRepos[uRepo].pRepo == NULL)
		{
			// we're not using this repo
			pThis->aRepos[uRepo].pTransaction = NULL;
			pThis->aRepos[uRepo].szUserId     = NULL;

			// we can't not be using the import repo
			SG_ASSERT(uRepo != DUMP__REPO__IMPORT);
		}
		else
		{
			// find the appropriate user ID
			if (pThis->szUser == NULL)
			{
				pThis->aRepos[uRepo].szUserId = SG_NOBODY__USERID;
			}
			else
			{
				// look up the ID for the specified username in the import repo
				SG_ERR_CHECK(  SG_user__sanitize_username(pCtx, pThis->szUser, &szUsername)  );
				SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pDump->aRepos[uRepo].pUsers, szUsername, &pThis->aRepos[uRepo].szUserId)  );
				if (pThis->aRepos[uRepo].szUserId == NULL)
				{
					// user doesn't exist yet, create them
					SG_ERR_CHECK(  SG_user__create(pCtx, pDump->aRepos[uRepo].pRepo, szUsername, &szUserId)  );
					SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pDump->aRepos[uRepo].pUsers, szUsername, szUserId)  );
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pDump->aRepos[uRepo].pUsers, szUsername, &pThis->aRepos[uRepo].szUserId)  );
				}
			}

			// start a transaction
			SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pDump->aRepos[uRepo].pRepo, &pThis->aRepos[uRepo].pTransaction)  );
		}

		// free memory for the next loop
		SG_NULLFREE(pCtx, szUsername);
		SG_NULLFREE(pCtx, szUserId);
	}

	// start with the SVN tree from the revision that will be our parent
	SG_ERR_CHECK(  dump__load_svn_tree(pCtx, pThis->pDump, dump__load_svn_tree__parent, NULL, &pThis->pSuperRoot, &pThis->pFileRoot, &pThis->pPathSuperRoots)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pHeaders);
	SG_NULLFREE(pCtx, szUsername);
	SG_NULLFREE(pCtx, szUserId);
	SG_NULLFREE(pCtx, szHid);
	return;
}

/**
 * Verifies information about a node that's being used as a reference.
 */
static void revision__node__verify_reference(
	SG_context*   pCtx,           //< [in] [out] Error and context info.
	revision*     pThis,          //< [in] Revision being processed.
	pending_node* pPendingNode,   //< [in] Pending node to verify.
	node_type     eExpectedType,  //< [in] Expected type of the node.
	const char*   szExpectedMd5,  //< [in] Expected MD5 hash of the node's text.
	                              //<      NULL to not verify its MD5.
	                              //<      Must be NULL if eExpectedType is FOLDER.
	const char*   szExpectedSha1, //< [in] Expected SHA1 hash of the node's text.
	                              //<      NULL to not verify its SHA1.
	                              //<      Must be NULL if eExpectedType is FOLDER.
	const char*   szErrorPath,    //< [in] Path to include in error messages.
	const char*   szErrorText,    //< [in] Short/simple name of the node being verified, to include in error messages.
	                              //<      Examples: "base", "source"
	                              //<      Example usage: "The node's %s text doesn't match the expected hash."
	const char**  ppHid,          //< [out] Unambiguous HID of pPendingNode.
	                              //<       Only returned if eExpectedType is FILE.
	dump__repo*   pRepo           //< [out] Repo that the returned HID is from.
	                              //<       Only returned if eExpectedType is FILE.
	)
{
	SG_treenode_entry_type eType = SG_TREENODEENTRY_TYPE__INVALID;
	const char*            szHid = NULL;
	dump__repo             eRepo = DUMP__REPO__COUNT;

	SG_NULLARGCHECK(pPendingNode);

	// verify the node's type
	SG_ERR_CHECK(  pending_node__get_type(pCtx, pPendingNode, &eType)  );
	if (eType != gaNodeType_PendingNodeType[eExpectedType])
	{
		SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Type of node record (%s) doesn't match type of its specified %s (%s): %s", gaNodeType_String[eExpectedType], szErrorText, gaPendingNodeType_String[eType], szErrorPath));
	}

	// if the type is a file, find the relevent HID and repo
	if (eExpectedType == NODE_TYPE__FILE)
	{
		SG_ERR_CHECK(  pending_node__get_unambiguous_hid(pCtx, pPendingNode, &szHid, (SG_uint32*)&eRepo)  );
	}

	// verify any hashes that are available
	if (szExpectedMd5 != NULL || szExpectedSha1 != NULL)
	{
		parse__blob__verify_hashes__data oVerifyData;

		// we can't do verification of hashes on folders
		// We can't know how SVN would even calculate the hash of a folder.
		// This also being the case, we know that we've already found the
		// umambiguous HID and repo to verify against the hashes.
		SG_ASSERT(eExpectedType == NODE_TYPE__FILE);

		// verify the hashes
		oVerifyData.szExpectedMd5  = szExpectedMd5;
		oVerifyData.szExpectedSha1 = szExpectedSha1;
		SG_ERR_CHECK(  import__fetch_blob(pCtx, pThis->pDump->aRepos[eRepo].pRepo, szHid, pThis->pDump->pBlobBuffer, pThis->pDump->uBlobBuffer, parse__blob__verify_hashes, (void*)&oVerifyData)  );
		if (oVerifyData.bMatchMd5 == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "MD5 hash of node's %s text doesn't match: %s (expected: %s, actual: %s)", szErrorText, szErrorPath, oVerifyData.szExpectedMd5, oVerifyData.szActualMd5));
		}
		if (oVerifyData.bMatchSha1 == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "SHA1 hash of node's %s text doesn't match: %s (expected: %s, actual: %s)", szErrorText, szErrorPath, oVerifyData.szExpectedSha1, oVerifyData.szActualSha1));
		}
	}

	// return any data they wanted
	if (ppHid != NULL)
	{
		*ppHid = szHid;
	}
	if (pRepo != NULL)
	{
		*pRepo = eRepo;
	}

fail:
	return;
}

/**
 * Adds a new pending node to the revision's tree, optionally starting with an existing one.
 */
static void revision__node__add(
	SG_context*    pCtx,      //< [in] [out] Error and context info.
	revision*      pThis,     //< [in] Revision to add a new pending node to.
	pending_node*  pParent,   //< [in] Parent to add the new pending node under.
	pending_node*  pSource,   //< [in] Source pending node to base the new one on.
	                          //<      If NULL, the new pending node will not be based on another one.
	const char*    szName,    //< [in] Name of the new pending node.
	SG_uint32      uName,     //< [in] Length of szName.
	node_type      eType,     //< [in] Type of the new pending node.
	                          //<      Ignored if pSource is non-NULL, the new node must inherit the source's type.
	char**         ppHid,     //< [in] Content HID of the new pending node.
	                          //<      Required if pSource is NULL and eType is NODE_TYPE__FILE.
	                          //<      Optional if pSource is non-NULL and its type is not SG_TREENODEENTRY_TYPE_DIRECTORY.
	                          //<      Ignored in other cases.
	                          //<      If used, we steal the pointer and NULL the caller's.
	dump__repo     eRepo,     //< [in] The repo that the content HID is in.
	                          //<      Ignored if ppHid is NULL.
	pending_node** ppNew      //< [out] The new pending node.
	)
{
	pending_node* pNew = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pParent);
	SG_NULLARGCHECK(ppNew);

	// check if we're copying from a source node or not
	if (pSource != NULL)
	{
		// make sure the source's children are loaded before we copy it
		SG_ERR_CHECK(  pending_node__load_children(pCtx, pSource, SG_TRUE, dump__get_repo_data, (void*)pThis->pDump)  );

		// allocate the new node by copying the source and renaming it
		SG_ERR_CHECK(  pending_node__alloc__copy(pCtx, &pNew, pSource, szName, uName, SG_TRUE)  );
	}
	else
	{
		// if the new node is a file, it needs content
		if (eType == NODE_TYPE__FILE)
		{
			SG_NULLARGCHECK(ppHid);
			SG_NULLARGCHECK(*ppHid);
		}

		// allocate the new node with the specified name and type
		SG_ERR_CHECK(  pending_node__alloc__new(pCtx, &pNew, szName, uName, gaNodeType_PendingNodeType[eType])  );
	}

	// if content was specified, and the new node isn't a directory, adopt it
	if (ppHid != NULL && *ppHid != NULL)
	{
		SG_treenode_entry_type eNewType = SG_TREENODEENTRY_TYPE__INVALID;

		SG_ERR_CHECK(  pending_node__get_type(pCtx, pNew, &eNewType)  );
		if (eNewType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			SG_ERR_CHECK(  pending_node__adopt_hid(pCtx, pNew, eRepo, ppHid)  );
		}
	}

	// add the new node to the specified parent
	SG_ERR_CHECK(  pending_node__add_child(pCtx, pParent, pNew)  );

	// return the new node
	*ppNew = pNew;
	pNew = NULL;

fail:
	PENDING_NODE_NULLFREE(pCtx, pNew);
	return;
}

/**
 * Reads a single node record and adds it to the revision.
 */
static void revision__node(
	SG_context*          pCtx,   //< [in] [out] Error and context info.
	revision*            pThis,  //< [in] Revision to read a node into.
	buffered_readstream* pStream //< [in] Stream to read from.
	)
{
	SG_vhash*     pHeaders         = NULL;
	SG_uint64     uPropertySize    = 0u;
	SG_vhash*     pProperties      = NULL;
	SG_bool       bPropertiesDelta = SG_FALSE;
	const char*   szPath           = NULL;
	node_type     eType            = NODE_TYPE__COUNT;
	pending_node* pPending         = NULL;
	SG_uint64     uSourceRevision  = 0u;
	const char*   szSourcePath     = NULL;
	pending_node* pSourceSuperRoot = NULL;
	pending_node* pSource          = NULL;
	SG_bool       bText            = SG_FALSE;
	dump__repo    eTextRepo        = DUMP__REPO__COUNT;
	char*         szTextHid        = NULL;
	node_action   eAction          = NODE_ACTION__COUNT;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pStream);

	// parse the headers
	SG_ERR_CHECK(  parse__headers(pCtx, pStream, &pHeaders)  );

	// parse the properties, if any
	SG_ERR_CHECK(  parse__hash_value__uint64__optional(pCtx, pHeaders, gszNode_PropertySize, &uPropertySize, 0u)  );
	if (uPropertySize > 0u)
	{
		SG_ERR_CHECK(  parse__hash_value__bool__optional(pCtx, pHeaders, gszNode_PropertiesDelta, &bPropertiesDelta, SG_FALSE)  );
		SG_ERR_CHECK(  parse__property_section(pCtx, pStream, uPropertySize, &pProperties)  );
	}

	// get the path of this node
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHeaders, gszNode_Path, &szPath)  );
	SG_ERR_CHECK(  SG_log__set_step(pCtx, szPath)  );

	// find the corresponding pending node
	SG_ERR_CHECK(  parse__hash_value__node_type__optional(pCtx, pHeaders, gszNode_Type, &eType, NODE_TYPE__COUNT)  );
	SG_ERR_CHECK(  pending_node__walk_path(pCtx, pThis->pFileRoot, szPath, SG_STRLEN(szPath), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis->pDump, &pPending)  );

	// make sure its type matches the record
	if (pPending != NULL)
	{
		SG_treenode_entry_type ePendingType = SG_TREENODEENTRY_TYPE__INVALID;

		SG_ERR_CHECK(  pending_node__get_type(pCtx, pPending, &ePendingType)  );
		if (eType != NODE_TYPE__COUNT && eType != gaPendingNodeType_NodeType[ePendingType])
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Type of node record (%s) doesn't match type of existing node (%s).", gaNodeType_String[eType], gaPendingNodeType_String[ePendingType]));
		}
	}

	// if a source node was specified, find it
	SG_ERR_CHECK(  parse__hash_value__uint64__optional(pCtx, pHeaders, gszNode_SourceRevision, &uSourceRevision, 0u)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_SourcePath, &szSourcePath)  );
	if (uSourceRevision != 0u && szSourcePath != NULL)
	{
		pending_node* pSourceFileRoot = NULL;
		const char*   szMd5           = NULL;
		const char*   szSha1          = NULL;

		// source node must have come from a previous revision
		SG_ASSERT(uSourceRevision < pThis->uId);

		// find the source node
		SG_ERR_CHECK(  dump__load_svn_tree(pCtx, pThis->pDump, dump__load_svn_tree__past, (void*)&uSourceRevision, &pSourceSuperRoot, &pSourceFileRoot, NULL)  );
		SG_ERR_CHECK(  pending_node__walk_path(pCtx, pSourceFileRoot, szSourcePath, SG_STRLEN(szSourcePath), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis->pDump, &pSource)  );
		SG_ASSERT(pSource != NULL);

		// verify its type/hashes
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_SourceMd5, &szMd5)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_SourceSha1, &szSha1)  );
		SG_ERR_CHECK(  revision__node__verify_reference(pCtx, pThis, pSource, eType, szMd5, szSha1, szPath, "source text", NULL, NULL)  );
	}

	// read the text, if any
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pHeaders, gszNode_TextSize, &bText)  );
	if (bText != SG_FALSE)
	{
		SG_uint64                        uTextSize   = 0u;
		SG_bool                          bTextDelta  = SG_FALSE;
		const char*                      szBaseHid   = NULL;
		dump__repo                       eBaseRepo   = DUMP__REPO__COUNT;
		dump__path*                      pPath       = NULL;
		parse__blob__verify_hashes__data oVerifyData;

		// we can only have a text section on a file
		SG_ASSERT(eType == NODE_TYPE__FILE);

		// parse text information out of the headers
		SG_ERR_CHECK(  parse__hash_value__uint64(pCtx, pHeaders, gszNode_TextSize, &uTextSize)  );
		SG_ERR_CHECK(  parse__hash_value__bool__optional(pCtx, pHeaders, gszNode_TextDelta, &bTextDelta, SG_FALSE)  );

		// if the text is a delta and we have base text to apply to, find and verify it
		if (bTextDelta != SG_FALSE && (pPending != NULL || pSource != NULL))
		{
			pending_node* pBase  = (pSource == NULL ? pPending : pSource);
			const char*   szMd5  = NULL;
			const char*   szSha1 = NULL;

			// verify the base node's type/hashes
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_TextBaseMd5, &szMd5)  );
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_TextBaseSha1, &szSha1)  );
			SG_ERR_CHECK(  revision__node__verify_reference(pCtx, pThis, pBase, eType, szMd5, szSha1, szPath, "base text", &szBaseHid, &eBaseRepo)  );

			// since our base node is a file, we must have gotten a HID and repo back
			SG_ASSERT(szBaseHid != NULL);
			SG_ASSERT(eBaseRepo != DUMP__REPO__COUNT);
		}

		// figure out which repo the text needs to be stored in
		SG_ERR_CHECK(  dump__get_path_data(pCtx, pThis->pDump, szPath, &pPath)  );
		eTextRepo = pPath->eRepo;

		// parse the text into a blob, and verify its hash(es)
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_TextMd5, &oVerifyData.szExpectedMd5)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHeaders, gszNode_TextSha1, &oVerifyData.szExpectedSha1)  );
		SG_ERR_CHECK(  parse__blob(pCtx, pStream, uTextSize, pThis->pDump->pBlobBuffer, pThis->pDump->uBlobBuffer, bTextDelta, szBaseHid, pThis->pDump->aRepos[eBaseRepo].pRepo, pThis->pDump->aRepos[eTextRepo].pRepo, pThis->aRepos[eTextRepo].pTransaction, parse__blob__verify_hashes, (void*)&oVerifyData, &szTextHid)  );
		if (oVerifyData.bMatchMd5 == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "MD5 hash of node's text doesn't match: %s (expected: %s, actual: %s)", szPath, oVerifyData.szExpectedMd5, oVerifyData.szActualMd5));
		}
		if (oVerifyData.bMatchSha1 == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "SHA1 hash of node's text doesn't match: %s (expected: %s, actual: %s)", szPath, oVerifyData.szExpectedSha1, oVerifyData.szActualSha1));
		}
	}

	// read through any remaining newlines
	// In theory there should be a known number of newlines, but the conditions under
	// which they exist or don't exist have little or no pattern.  Look back at the
	// past few revisions of this code for previous attempts to codify those conditions.
	SG_ERR_CHECK(  buffered_readstream__read_while(pCtx, pStream, "\r\n", SG_TRUE, NULL, NULL)  );

	// update our tree of pending nodes appropriately
	SG_ERR_CHECK(  parse__hash_value__node_action(pCtx, pHeaders, gszNode_Action, &eAction)  );
	switch (eAction)
	{
	case NODE_ACTION__ADD:
		{
			SG_uint32     uPathLength   = 0u;
			SG_uint32     uParentLength = 0u;
			SG_uint32     uNameStart    = 0u;
			pending_node* pParent       = NULL;

			// make sure we do not have an existing node
			if (pPending != NULL)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Cannot add a node that already exists: %s", szPath));
			}

			// split the path into the parent part and the name part
			uPathLength = SG_STRLEN(szPath);
			SG_ERR_CHECK(  import__find_parent_path_separator(pCtx, szPath, uPathLength, &uParentLength)  );
			uNameStart = (uParentLength == 0u ? uParentLength : uParentLength + 1u);

			// find the parent for the new node and make sure its children are loaded
			SG_ERR_CHECK(  pending_node__walk_path(pCtx, pThis->pFileRoot, szPath, uParentLength, PENDING_NODE__RESTORE_DELETED | PENDING_NODE__ADD_MISSING, dump__get_repo_data, (void*)pThis->pDump, &pParent)  );
			SG_ERR_CHECK(  pending_node__load_children(pCtx, pParent, SG_FALSE, dump__get_repo_data, (void*)pThis->pDump)  );

			// add a new node
			SG_ERR_CHECK(  revision__node__add(pCtx, pThis, pParent, pSource, szPath + uNameStart, uPathLength - uNameStart, eType, &szTextHid, eTextRepo, &pPending)  );
		}
		break;
	case NODE_ACTION__DELETE:
		{
			// make sure we have an existing node
			if (pPending == NULL)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Cannot find node to delete: %s", szPath));
			}

			// delete it
			SG_ERR_CHECK(  pending_node__delete(pCtx, pPending)  );
		}
		break;
	case NODE_ACTION__REPLACE:
		{
			pending_node* pParent = NULL;
			const char*   szName  = NULL;

			// make sure we have an existing node
			if (pPending == NULL)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Cannot find node to replace: %s", szPath));
			}

			// get its parent and name
			SG_ERR_CHECK(  pending_node__get_parent(pCtx, pPending, &pParent)  );
			SG_ERR_CHECK(  pending_node__get_name(pCtx, pPending, &szName)  );

			// delete it and add a replacement
			SG_ERR_CHECK(  pending_node__delete(pCtx, pPending)  );
			SG_ERR_CHECK(  revision__node__add(pCtx, pThis, pParent, pSource, szName, SG_STRLEN(szName), eType, &szTextHid, eTextRepo, &pPending)  );
		}
		break;
	case NODE_ACTION__CHANGE:
		{
			// make sure we have an existing node
			if (pPending == NULL)
			{
				SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Cannot find node to modify: %s", szPath));
			}

			// if we have updated content for the node, update it
			// (we may not if the only change is to the node's properties)
			if (szTextHid != NULL)
			{
				SG_ERR_CHECK(  pending_node__adopt_hid(pCtx, pPending, eTextRepo, &szTextHid)  );
			}
		}
		break;
	default:
		// shouldn't be possible, because we validate this during parsing
		SG_ASSERT(SG_FALSE);
		SG_ERR_THROW(SG_ERR_MALFORMED_SVN_DUMP);
		break;
	}

	// done with another node
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pHeaders);
	SG_VHASH_NULLFREE(pCtx, pProperties);
	PENDING_NODE_NULLFREE(pCtx, pSourceSuperRoot);
	SG_NULLFREE(pCtx, szTextHid);
	return;
}

/**
 * Data used by revision__commit__get_repo_data and revision__commit__commit_to_repo.
 */
typedef struct revision__commit__data
{
	const revision*   pRevision;  //< Revision being committed.
	const dump__path* pPath;      //< Path being committed.
	SG_changeset*     pChangeset; //< Changeset being committed.
}
revision__commit__data;

/**
 * Implementation of get_repo_data used by revision__commit.
 */
static void revision__commit__get_repo_data(
	SG_context*         pCtx,
	SG_uint32           uRepo,
	void*               pVoidData,
	SG_repo**           ppRepo,
	SG_repo_tx_handle** ppTransaction,
	SG_changeset**      ppChangeset
	)
{
	revision__commit__data* pData = (revision__commit__data*)pVoidData;

	SG_NULLARGCHECK(pVoidData);

	if (ppRepo != NULL)
	{
		*ppRepo = pData->pRevision->pDump->aRepos[uRepo].pRepo;
	}
	if (ppTransaction != NULL)
	{
		*ppTransaction = pData->pRevision->aRepos[uRepo].pTransaction;
	}
	if (ppChangeset != NULL)
	{
		*ppChangeset = (uRepo == (SG_uint32)pData->pPath->eRepo ? pData->pChangeset : NULL);
	}

fail:
	return;
}

/**
 * Implementation of commit_to_repo used by commit__repo.
 */
static void revision__commit__commit_to_repo(
	SG_context*   pCtx,
	pending_node* pThis,
	const char*   szPath,
	SG_uint32     uRepo,
	void*         pVoidData,
	SG_bool*      pCommit
	)
{
	revision__commit__data* pData = (revision__commit__data*)pVoidData;

	SG_NULLARGCHECK(pThis);
	SG_UNUSED(szPath);
	SG_NULLARGCHECK(pVoidData);
	SG_NULLARGCHECK(pCommit);

	*pCommit = (uRepo == (SG_uint32)pData->pPath->eRepo ? SG_TRUE : SG_FALSE);

fail:
	return;
}

/**
 * Commits a super-root node for a path and returns the resulting changeset.
 */
static void revision__commit__changeset(
	SG_context*       pCtx,       //< [in] [out] Error and context info.
	revision*         pThis,      //< [in] The revision the changeset will be from.
	pending_node*     pSuperRoot, //< [in] Super-root of the tree to commit.
	const dump__path* pPath,      //< [in] Path data that the super-root corresponds to.
	char**            ppChangeset //< [out] ID of the committed changeset.
	                              //<       NULL if no changeset was committed because the node wasn't dirty.
	)
{
	SG_changeset*          pChangeset  = NULL;
	revision__commit__data oData;
	SG_bool                aDirty[PENDING_NODE__MAX_REPOS] = { SG_FALSE, };
	const char*            szHid       = NULL;
	SG_dagnode*            pDagNode    = NULL;
	char*                  szChangeset = NULL;
	SG_audit               oAudit;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pSuperRoot);
	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(ppChangeset);

	// start a changeset for this path
	SG_ERR_CHECK(  SG_changeset__alloc__committing(pCtx, SG_DAGNUM__VERSION_CONTROL, &pChangeset)  );
	SG_ERR_CHECK(  SG_changeset__set_version(pCtx, pChangeset, SG_CSET_VERSION__CURRENT)  );
	SG_ERR_CHECK(  SG_changeset__add_parent(pCtx, pChangeset, pPath->szNextParent)  );

	// commit this tree on the changeset
	oData.pRevision  = pThis;
	oData.pPath      = pPath;
	oData.pChangeset = pChangeset;
	SG_ERR_CHECK(  pending_node__commit(pCtx, pSuperRoot, revision__commit__get_repo_data, (void*)&oData, revision__commit__commit_to_repo, (void*)&oData, pThis->pDump->pBlobBuffer, pThis->pDump->uBlobBuffer, aDirty)  );

	// only bother committing the changeset if the node was dirty in this repo
	if (aDirty[pPath->eRepo] != SG_FALSE)
	{
		// get the committed HID and set it as the changeset's root
		SG_ERR_CHECK(  pending_node__get_hid(pCtx, pSuperRoot, pPath->eRepo, &szHid)  );
		SG_ERR_CHECK(  SG_changeset__tree__set_root(pCtx, pChangeset, szHid)  );

		// save the changeset to the repo
		SG_ERR_CHECK(  SG_changeset__save_to_repo(pCtx, pChangeset, pThis->pDump->aRepos[pPath->eRepo].pRepo, pThis->aRepos[pPath->eRepo].pTransaction)  );

		// generate a dagnode for the new changeset and save it to the repo
		SG_ERR_CHECK(  SG_changeset__create_dagnode(pCtx, pChangeset, &pDagNode)  );
		SG_ERR_CHECK(  SG_dagnode__get_id(pCtx, pDagNode, &szChangeset)  );
		SG_ERR_TRY(  SG_repo__store_dagnode(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pThis->aRepos[pPath->eRepo].pTransaction, SG_DAGNUM__VERSION_CONTROL, pDagNode)  );
		SG_ERR_CATCH_IGNORE(SG_ERR_DAGNODE_ALREADY_EXISTS);
		SG_ERR_CATCH_END;
		pDagNode = NULL;

		// generate an audit for the new changeset and save it to the repo
		SG_ERR_CHECK(  SG_audit__init(pCtx, &oAudit, pThis->pDump->aRepos[pPath->eRepo].pRepo, pThis->iTime, pThis->aRepos[pPath->eRepo].szUserId)  );
		SG_ERR_CHECK(  SG_repo__store_audit(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pThis->aRepos[pPath->eRepo].pTransaction, szChangeset, SG_DAGNUM__VERSION_CONTROL, oAudit.who_szUserId, oAudit.when_int64)  );
	}

	// return the changeset ID
	*ppChangeset = szChangeset;
	szChangeset = NULL;

fail:
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
	SG_DAGNODE_NULLFREE(pCtx, pDagNode);
	SG_NULLFREE(pCtx, szChangeset);
	return;
}

/**
 * Performs a commit for a path that exists but was not added or deleted by the current revision.
 */
static void revision__commit__path__normal(
	SG_context*   pCtx,        //< [in] [out] Error and context info.
	revision*     pThis,       //< [in] Revision being committed.
	dump__path*   pPath,       //< [in] Data about the path being committed.
	pending_node* pSuperRoot,  //< [in] Super-root node that the path was loaded from,
	                           //<      and that it should be moved back to.
	pending_node* pPendingNode //< [in] Pending node at the root of the path, that was
	                           //<      loaded from pSuperRoot and must be moved back to it.
	                           //<      NULL to just commit pSuperRoot as it is.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(pSuperRoot);

	// move the node back to its own tree, if it's not there already
	if (pPendingNode != NULL)
	{
		SG_ERR_CHECK(  pending_node__move(pCtx, pPendingNode, pSuperRoot, pPath->szPathGid, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), SG_FALSE)  );
	}

	// commit the independent tree
	SG_ERR_CHECK(  revision__commit__changeset(pCtx, pThis, pSuperRoot, pPath, &pPath->szCommitting)  );

	// update the path's data
	if (pPath->szCommitting == NULL)
	{
		// the node wasn't dirty, so we didn't actually commit a new changeset
		SG_ERR_CHECK(  dump__path__set_revision(pCtx, pPath, pThis->uId, pPath->szNextParent)  );
		// don't need to change the next parent
	}
	else
	{
		// the node was dirty so we committed a changeset
		SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pPath->pChangesets, pPath->szCommitting, &pPath->szCommittingPool)  );
		SG_ERR_CHECK(  dump__path__set_revision(pCtx, pPath, pThis->uId, pPath->szCommittingPool)  );
		pPath->szNextParent = pPath->szCommittingPool;
	}
	pPath->bExistsInLatest = SG_TRUE;

fail:
	return;
}

/**
 * Performs a commit for a path that was deleted by the current revision.
 */
static void revision__commit__path__deleted(
	SG_context*   pCtx,      //< [in] [out] Error and context info.
	revision*     pThis,     //< [in] Revision being committed.
	const char*   szPath,    //< [in] Path being committed.
	dump__path*   pPath,     //< [in] Data about the path being committed.
	pending_node* pSuperRoot //< [in] Super-root node that the path was loaded from.
	)
{
	pending_node* pPendingNode = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(pSuperRoot);

	// find the inactive node
	SG_ERR_CHECK(  pending_node__walk_path(pCtx, pThis->pFileRoot, szPath, SG_STRLEN(szPath), PENDING_NODE__INACTIVE, NULL, NULL, &pPendingNode)  );
	SG_ASSERT(pPendingNode != NULL);

	// make sure its children are loaded
	SG_ERR_CHECK(  pending_node__load_children(pCtx, pPendingNode, SG_FALSE, dump__get_repo_data, (void*)pThis->pDump)  );

	// move it back to its own tree, pushing its deleted status down to its children
	SG_ERR_CHECK(  pending_node__move(pCtx, pPendingNode, pSuperRoot, pPath->szPathGid, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), SG_TRUE)  );

	// commit the independent tree
	SG_ERR_CHECK(  revision__commit__changeset(pCtx, pThis, pSuperRoot, pPath, &pPath->szCommitting)  );
	SG_ASSERT(pPath->szCommitting != NULL);
	// Shouldn't be possible to have this changeset remove the path
	// and yet not have the independent tree be dirty.

	// update the path's data
	SG_ERR_CHECK(  dump__path__set_revision(pCtx, pPath, pThis->uId, NULL)  );
	// don't need to change the next parent
	pPath->bExistsInLatest = SG_FALSE;

fail:
	return;
}

/**
 * Performs a commit for a path that was added by the current revision.
 */
static void revision__commit__path__added(
	SG_context*   pCtx,        //< [in] [out] Error and context info.
	revision*     pThis,       //< [in] Revision being committed.
	const char*   szPath,      //< [in] Path being committed.
	dump__path*   pPath,       //< [in] Data about the path being committed.
	pending_node* pPendingNode //< [in] Pending node at the root of the path.
	)
{
	SG_treenode_entry_type eType       = SG_TREENODEENTRY_TYPE__INVALID;
	const char*            szGid       = NULL;
	char*                  szSuperRoot = NULL;
	pending_node*          pSuperRoot  = NULL;
	pending_node*          pFileRoot   = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(pPath);
	SG_NULLARGCHECK(pPendingNode);

	// make sure the path's root is a directory
	// we can't use a non-directory as a file root for an independent tree
	SG_ERR_CHECK(  pending_node__get_type(pCtx, pPendingNode, &eType)  );
	if (eType != SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Cannot import from non-directory: %s", szPath));
	}

	// save the node's GID so we can restore it in the future
	SG_ERR_CHECK(  pending_node__get_gid(pCtx, pPendingNode, &szGid)  );
	SG_ERR_CHECK(  SG_strcpy(pCtx, pPath->szSvnGid, SG_GID_BUFFER_LENGTH, szGid)  );

	// load the super-root from this path's next parent
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pPath->szNextParent, &szSuperRoot)  );
	SG_ERR_CHECK(  pending_node__alloc__super_root(pCtx, &pSuperRoot, pPath->eRepo, &szSuperRoot)  );
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pThis->pPathSuperRoots, szPath, (void*)pSuperRoot)  );

	// this node is going to assume the identity of the super-root's existing file root
	SG_ERR_CHECK(  pending_node__walk_path(pCtx, pSuperRoot, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), PENDING_NODE__NONE, dump__get_repo_data, (void*)pThis->pDump, &pFileRoot)  );
	SG_ERR_CHECK(  pending_node__get_gid(pCtx, pFileRoot, &szGid)  );
	SG_ERR_CHECK(  SG_strcpy(pCtx, pPath->szPathGid, SG_GID_BUFFER_LENGTH, szGid)  );

	// remove the existing file root and free it
	SG_ERR_CHECK(  pending_node__move(pCtx, pFileRoot, NULL, NULL, NULL, 0u, SG_FALSE)  );
	PENDING_NODE_NULLFREE(pCtx, pFileRoot);

	// move this node over to take its place
	SG_ERR_CHECK(  pending_node__move(pCtx, pPendingNode, pSuperRoot, pPath->szPathGid, gszImport__FileRootName, SG_STRLEN(gszImport__FileRootName), SG_FALSE)  );

	// commit the independent tree
	SG_ERR_CHECK(  revision__commit__changeset(pCtx, pThis, pSuperRoot, pPath, &pPath->szCommitting)  );
	SG_ASSERT(pPath->szCommitting != NULL);
	// Shouldn't be possible to have this changeset create the path
	// and yet not have the independent tree be dirty.

	// update the path's data
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pPath->pChangesets, pPath->szCommitting, &pPath->szCommittingPool)  );
	SG_ERR_CHECK(  dump__path__set_revision(pCtx, pPath, pThis->uId, pPath->szCommittingPool)  );
	pPath->szNextParent = pPath->szCommittingPool;
	pPath->bExistsInLatest = SG_TRUE;

fail:
	SG_NULLFREE(pCtx, szSuperRoot);
	return;
}

/**
 * Performs any commit actions for a path that can occur within an existing transaction.
 */
static void revision__commit__path__in_transaction(
	SG_context* pCtx,   //< [in] [out] Error and context info.
	revision*   pThis,  //< [in] Revision being committed.
	const char* szPath, //< [in] Path being committed.
	dump__path* pPath   //< [in] Data about the path being committed.
	)
{
	SG_bool       bSuperRoot   = SG_FALSE;
	pending_node* pSuperRoot   = NULL;
	pending_node* pPendingNode = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(szPath);
	SG_NULLARGCHECK(pPath);

	// the dump should never contain a path for a repo it's not using
	SG_ASSERT(pThis->pDump->aRepos[pPath->eRepo].pRepo != NULL);

	// find the super-root node for this path
	// if there isn't one, then this path didn't exist in the baseline
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pThis->pPathSuperRoots, szPath, &bSuperRoot, (void**)&pSuperRoot)  );

	// find the pending node at this path
	// if there isn't one, then this path doesn't currently exist
	SG_ERR_CHECK(  pending_node__walk_path(pCtx, pThis->pFileRoot, szPath, SG_STRLEN(szPath), PENDING_NODE__NONE, NULL, NULL, &pPendingNode)  );

	// check how to handle the path
	// In general, we need to move the path's node to its own independent super-root
	// and change its identity it so that it becomes the file root of that tree.
	// Then we commit that tree as its own independent changeset and cache that changeset's HID.
	// However, the specifics depend on whether or not the path was added or deleted.
	if (bSuperRoot != SG_FALSE && pPendingNode != NULL)
	{
		// this path existed in the baseline and still does
		SG_ERR_CHECK(  revision__commit__path__normal(pCtx, pThis, pPath, pSuperRoot, pPendingNode)  );
	}
	else if (bSuperRoot != SG_FALSE)
	{
		// this path existed in the baseline but doesn't anymore
		SG_ERR_CHECK(  revision__commit__path__deleted(pCtx, pThis, szPath, pPath, pSuperRoot)  );
	}
	else if (pPendingNode != NULL)
	{
		// this path didn't exist in the baseline but does now
		SG_ERR_CHECK(  revision__commit__path__added(pCtx, pThis, szPath, pPath, pPendingNode)  );
	}
	else
	{
		// this path didn't exist in the baseline and still doesn't
		// no need to do anything
	}

fail:
	return;
}

/**
 * Performs any commit actions for a path that must occur in their own transaction(s).
 */
static void revision__commit__path__post_transaction(
	SG_context* pCtx,  //< [in] [out] Error and context info.
	revision*   pThis, //< [in] Revision being committed.
	dump__path* pPath  //< [in] Path being committed.
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pPath);

	// nothing to do unless we actually committed a changeset
	if (pPath->szCommitting != NULL)
	{
		SG_audit oAudit;

		// initialize an audit reflecting the path's data
		SG_ERR_CHECK(  SG_audit__init(pCtx, &oAudit, pThis->pDump->aRepos[pPath->eRepo].pRepo, pThis->iTime, pThis->aRepos[pPath->eRepo].szUserId)  );

		// if we have a comment, add that to the changeset
		if (pThis->szMessage != NULL)
		{
			SG_ERR_CHECK(  SG_vc_comments__add(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pPath->szCommitting, pThis->szMessage, &oAudit)  );
		}

		// if we're tracking a branch, move the head to the changeset
		if (pPath->szBranch != NULL)
		{
			if (pPath->szBranchHead == NULL)
			{
				// haven't created a head for this branch yet, do that
				SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pPath->szBranch, pPath->szCommitting, NULL, SG_FALSE, &oAudit)  );
			}
			else
			{
				// already created a head for this branch, move that one
				SG_ERR_CHECK(  SG_vc_branches__move_head(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pPath->szBranch, pPath->szBranchHead, pPath->szCommitting, &oAudit)  );
			}

			// regardless, our branch head is now this changeset
			if (pPath->szCommittingPool == NULL)
			{
				SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pPath->pChangesets, pPath->szCommitting, &pPath->szCommittingPool)  );
			}
			pPath->szBranchHead = pPath->szCommittingPool;
		}

		// if we have a stamp, add that to the changeset
		if (pPath->szStamp != NULL)
		{
			SG_ERR_CHECK(  SG_vc_stamps__add(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pPath->szCommitting, pPath->szStamp, &oAudit, NULL)  );
		}

		// if we're adding an additional comment, do that
		if (pPath->bComment != SG_FALSE)
		{
			char                    szComment[19 + 21]; // strlen(gszExtraCommentFormat) + sizeof(SG_int_to_string_buffer)
			SG_int_to_string_buffer szRevision;

			SG_ERR_CHECK(  SG_sprintf(pCtx, szComment, sizeof(szComment), gszExtraCommentFormat, SG_uint64_to_sz(pThis->uId, szRevision))  );
			SG_ERR_CHECK(  SG_vc_comments__add(pCtx, pThis->pDump->aRepos[pPath->eRepo].pRepo, pPath->szCommitting, szComment, &oAudit)  );
		}

		// done with this HID now
		SG_NULLFREE(pCtx, pPath->szCommitting);
	}

fail:
	return;
}

/**
 * Commits all the change nodes added to the revision and then free it.
 */
static void revision__commit(
	SG_context* pCtx, //< [in] [out] Error and context info.
	revision*   pThis //< [in] In progress changeset to commit and free.
	                  //<      We free this data in the process.
	)
{
	SG_rbtree_iterator* pIterator = NULL;
	SG_uint32           uRepo     = DUMP__REPO__COUNT;

	SG_NULLARGCHECK(pThis);

	// run through each specific path and commit it
	if (pThis->pDump->pPaths != NULL)
	{
		SG_bool     bIterator = SG_FALSE;
		const char* szPath    = NULL;
		dump__path* pPath     = NULL;

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pDump->pPaths, &bIterator, &szPath, (void**)&pPath)  );
		while (bIterator != SG_FALSE)
		{
			SG_ERR_CHECK(  revision__commit__path__in_transaction(pCtx, pThis, szPath, pPath)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szPath, (void**)&pPath)  );
		}
		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	}

	// commit whatever's left of the SVN tree using the dump's root path data
	SG_ERR_CHECK(  revision__commit__path__normal(pCtx, pThis, &pThis->pDump->oRootPath, pThis->pSuperRoot, NULL)  );

	// commit each transaction to its repo
	for (uRepo = 0u; uRepo < DUMP__REPO__COUNT; ++uRepo)
	{
		if (pThis->aRepos[uRepo].pTransaction != NULL)
		{
			// we shouldn't have a transaction at all if the repo's not in use
			SG_ASSERT(pThis->pDump->aRepos[uRepo].pRepo != NULL);

			// commit the transaction
			SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pThis->pDump->aRepos[uRepo].pRepo, &pThis->aRepos[uRepo].pTransaction)  );
		}
	}

	// run through each path and perform post-commit steps
	// these are only post-commit because they require their own independent transactions
	if (pThis->pDump->pPaths != NULL)
	{
		SG_bool     bIterator = SG_FALSE;
		const char* szPath    = NULL;
		dump__path* pPath     = NULL;

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pThis->pDump->pPaths, &bIterator, &szPath, (void**)&pPath)  );
		while (bIterator != SG_FALSE)
		{
			SG_ERR_CHECK(  revision__commit__path__post_transaction(pCtx, pThis, pPath)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bIterator, &szPath, (void**)&pPath)  );
		}
		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	}

	// run post-commit steps for the root path
	SG_ERR_CHECK(  revision__commit__path__post_transaction(pCtx, pThis, &pThis->pDump->oRootPath)  );

	// done with our operation on this revision
	if (pThis->bOperation != SG_FALSE)
	{
		SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
		pThis->bOperation = SG_FALSE;
	}

	// done with another revision
	// Note: This finishes a step in the operation owned by the dump, not the revision.
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
	return;
}


/*
**
** Public Functions
**
*/

void SG_svn_load__import(
	SG_context*    pCtx,
	SG_repo*       pRepo,
	SG_readstream* pRawStream,
	const char*    szParent,
	const char*    szBranch,
	const char*    szPath,
	const char*    szStamp,
	SG_bool        bComment
	)
{
	buffered_readstream* pStream   = NULL;
	dump                 oDump     = DUMP__INIT;
	SG_bool              bUuid     = SG_FALSE;
	SG_bool              bEOF      = SG_FALSE;
	revision             oRevision = REVISION__INIT;

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(pRawStream);
	SG_NULLARGCHECK(szParent);

	// buffer the given stream
	SG_ERR_CHECK(  buffered_readstream__alloc(pCtx, &pStream, pRawStream, SIZE_READ_BUFFER)  );

	// initialize the dump
	SG_ERR_CHECK(  dump__init(pCtx, &oDump, pRepo, szParent, szBranch, szPath, szStamp, bComment, pStream)  );

	// if there's a UUID record, read it
	SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pStream, (SG_byte*)gszDump_Uuid, SG_STRLEN(gszDump_Uuid), &bUuid)  );
	if (bUuid != SG_FALSE)
	{
		SG_ERR_CHECK(  dump__uuid(pCtx, &oDump, pStream)  );
	}

	// process revision records until we reach the end of the file
	SG_ERR_CHECK(  buffered_readstream__eof(pCtx, pStream, &bEOF)  );
	while (bEOF == SG_FALSE)
	{
		SG_bool bRevision = SG_FALSE;
		SG_bool bNode     = SG_FALSE;

		// make sure we're looking at a revision record
		SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pStream, (SG_byte*)gszRevision_Id, SG_STRLEN(gszRevision_Id), &bRevision)  );
		if (bRevision == SG_FALSE)
		{
			SG_ERR_THROW2(SG_ERR_MALFORMED_SVN_DUMP, (pCtx, "Expected revision record not found."));
		}

		// init revision data from the record
		SG_ERR_CHECK(  revision__init(pCtx, &oRevision, pStream, &oDump)  );

		// process all the following node records
		SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pStream, (SG_byte*)gszNode_Path, SG_STRLEN(gszNode_Path), &bNode)  );
		while (bNode != SG_FALSE)
		{
			SG_ERR_CHECK(  revision__node(pCtx, &oRevision, pStream)  );
			SG_ERR_CHECK(  buffered_readstream__peek_compare(pCtx, pStream, (SG_byte*)gszNode_Path, SG_STRLEN(gszNode_Path), &bNode)  );
		}

		// commit the revision and free it
		SG_ERR_CHECK(  revision__commit(pCtx, &oRevision)  );
		SG_ERR_CHECK(  revision__free(pCtx, &oRevision)  );

		// check if we're at the end of the stream yet
		SG_ERR_CHECK(  buffered_readstream__eof(pCtx, pStream, &bEOF)  );
	}

fail:
	revision__free(pCtx, &oRevision);
	dump__free(pCtx, &oDump);
	BUFFERED_READSTREAM_NULLFREE(pCtx, pStream);
	return;
}
