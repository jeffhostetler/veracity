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
#include <curl/easy.h>

//////////////////////////////////////////////////////////////////////////

struct _sg_curl__file_read_state
{
	SG_file* pFile; // We don't own this and shouldn't attempt to free it.
	SG_uint64 pos; // Current position in file.
	SG_uint64 len; // Length of file.
	SG_bool finished; // Bool indicating we already returned 0 to curl once.
	                  // Theoretically, we shouldn't need this since returning
	                  // 0 ends the transfer... But see work item H5833.
};

/**
 * The structure for which SG_curl is an opaque wrapper.
 */
typedef struct
{
	SG_context* pCtx; // We don't own this and shouldn't attempt to free it. It's the thread's main SG_context.
	CURL* pCurl;
	SG_string* pstrErr; // An error message relayed to us by the server.
	SG_string* pstrRawHeaders; // The raw http headers
	
	SG_curl__callback* pFnReadRequest;
	struct _sg_curl__file_read_state readState;

	SG_curl__callback* pFnWriteResponse;
	void* pWriteState; // We don't own this and shouldn't attempt to free it.

	SG_curl_progress_callback* pFnProgress;
	void* pProgressState; // We don't own this and shouldn't attempt to free it.
} _sg_curl;

//////////////////////////////////////////////////////////////////////////

#define _SETOPT(val) 	_sg_curl* pMe = (_sg_curl*)pCurl;	\
	CURLcode rc = CURLE_OK;									\
	SG_NULLARGCHECK_RETURN(pCurl);							\
	rc = curl_easy_setopt(pMe->pCurl, option, val);			\
	if (rc)													\
		SG_ERR_THROW_RETURN(SG_ERR_LIBCURL(rc))

static void _setopt__pv(SG_context* pCtx, SG_curl* pCurl, CURLoption option, void* pv)
{
	_SETOPT(pv);
}
static void _setopt__read_cb(SG_context* pCtx, SG_curl* pCurl, CURLoption option, curl_read_callback cb)
{
	_SETOPT(cb);
}
static void _setopt__write_cb(SG_context* pCtx, SG_curl* pCurl, CURLoption option, curl_write_callback cb)
{
	_SETOPT(cb);
}
static void _setopt__progress_cb(SG_context* pCtx, SG_curl* pCurl, CURLoption option, curl_progress_callback cb)
{
	_SETOPT(cb);
}
static void _setopt__ioctl_cb(SG_context* pCtx, SG_curl* pCurl, CURLoption option, curl_ioctl_callback cb)
{
	_SETOPT(cb);
}
static void _setopt__seek_cb(SG_context* pCtx, SG_curl* pCurl, CURLoption option, curl_seek_callback cb)
{
	_SETOPT(cb);
}

void SG_curl__setopt__sz(SG_context* pCtx, SG_curl* pCurl, CURLoption option, const char* pszVal)
{
	_SETOPT(pszVal);
}

void SG_curl__setopt__int32(SG_context* pCtx, SG_curl* pCurl, CURLoption option, SG_int32 iVal)
{
	_SETOPT(iVal);
}
void SG_curl__setopt__int64(SG_context* pCtx, SG_curl* pCurl, CURLoption option, SG_int64 iVal)
{
	_SETOPT(iVal);
}

void SG_curl__getinfo__int32(SG_context* pCtx, SG_curl* pCurl, CURLINFO info, SG_int32* piVal)
{
	_sg_curl* p = (_sg_curl*)pCurl;
	CURLcode rc = CURLE_OK;
	SG_int64 val;

	SG_NULLARGCHECK_RETURN(pCurl);

	rc = curl_easy_getinfo(p->pCurl, info, &val);
	if (rc)
		SG_ERR_THROW_RETURN(SG_ERR_LIBCURL(rc));

	*piVal = (SG_int32)val;
}

//////////////////////////////////////////////////////////////////////////

/* libcurl will call these shims for chunked I/O. 
 * They delegate to SG_curl__callback routines, which have sglib-friendly signatures. */

static size_t _callback_shim(char* buffer, size_t size, size_t nmemb, _sg_curl* pMe, SG_curl__callback* pFn, void* pState)
{
	SG_context* pCtx = pMe->pCtx; // so SG_ERR_CHECK works
	SG_uint32 len_handled = 0;

	SG_ERR_CHECK(  pFn(pCtx, (SG_curl*)pMe, buffer, (SG_uint32)(size*nmemb), pState, &len_handled)  );
	return len_handled;

fail:
	return CURL_READFUNC_ABORT;
}

static size_t _read_callback_shim(char* buffer, size_t size, size_t nmemb, void* pVoidState)
{
	_sg_curl* pMe = (_sg_curl*)pVoidState;
	return _callback_shim(buffer, size, nmemb, pMe, pMe->pFnReadRequest, &pMe->readState);
}

static size_t _write_callback_shim(char* buffer, size_t size, size_t nmemb, void* pVoidState)
{
	_sg_curl* pMe = (_sg_curl*)pVoidState;
	return _callback_shim(buffer, size, nmemb, pMe, pMe->pFnWriteResponse, pMe->pWriteState);
}

static int _progress_callback_shim(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	_sg_curl* pMe = (_sg_curl*)clientp;
	SG_context* pCtx = pMe->pCtx; // so SG_ERR_CHECK works
	
	SG_ERR_CHECK(  pMe->pFnProgress(pCtx, dltotal, dlnow, ultotal, ulnow, pMe->pProgressState)  );
	return 0;

fail:
	return CURL_READFUNC_ABORT;
}

//////////////////////////////////////////////////////////////////////////

/* Built-in SG_curl__callbacks for reading/writing SG_strings and SG_files */

// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTREADFUNCTION

static void _read_file_chunk(SG_context* pCtx, SG_curl* pCurl, char* buffer, SG_uint32 bufLen, void* pVoidState, SG_uint32* pLenHandled)
{
	struct _sg_curl__file_read_state* pReadState = (struct _sg_curl__file_read_state*)pVoidState;

	SG_UNUSED(pCurl);

	if (pReadState->finished)
	{
		SG_ERR_THROW2_RETURN(SG_ERR_UNSPECIFIED, (pCtx, "An unknown error occurred interfacing with libcurl. Please try again."));
	}
	else if (pReadState->pos == pReadState->len)
	{
		pReadState->finished = SG_TRUE;
		*pLenHandled = 0;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_file__read(pCtx, pReadState->pFile, bufLen, (SG_byte*)buffer, pLenHandled)  );
		pReadState->pos += *pLenHandled;
	}
}

static curlioerr _my_ioctl__file(CURL *handle, int cmd, void *clientp)
{
	_sg_curl* pMe = (_sg_curl*)clientp;

	SG_UNUSED(handle);

	if (cmd == CURLIOCMD_NOP)
	{
		SG_context__push_level(pMe->pCtx);
		SG_log__report_verbose(pMe->pCtx, "SG_curl handling CURLIOCMD_NOP.");
		SG_context__pop_level(pMe->pCtx);

		return CURLIOE_OK;
	}
	else if (cmd == CURLIOCMD_RESTARTREAD)
	{
		SG_context__push_level(pMe->pCtx);
		SG_log__report_verbose(pMe->pCtx, "SG_curl handling CURLIOCMD_RESTARTREAD.");
		SG_context__pop_level(pMe->pCtx);

		SG_file__seek(pMe->pCtx, pMe->readState.pFile, 0);
		if(SG_context__has_err(pMe->pCtx))
			return CURLIOE_FAILRESTART;

		pMe->readState.pos = 0;
		pMe->readState.finished = SG_FALSE;
		return CURLIOE_OK;
	}
	else
	{
		return CURLIOE_UNKNOWNCMD;
	}
}
static int _my_seek__file(void *instream, curl_off_t offset, int origin)
{
	_sg_curl* pMe = (_sg_curl*)instream;

	if (origin == SEEK_SET)
	{
		SG_context__push_level(pMe->pCtx);
		SG_log__report_verbose(pMe->pCtx, "SG_curl handling SEEK_SET.");
		SG_context__pop_level(pMe->pCtx);

		SG_file__seek(pMe->pCtx, pMe->readState.pFile, (SG_uint64)offset);
		if(SG_context__has_err(pMe->pCtx))
			return CURL_SEEKFUNC_FAIL;

		pMe->readState.pos = (SG_uint64)offset;
		pMe->readState.finished = SG_FALSE;
		return CURL_SEEKFUNC_OK;
	}
	else if (origin == SEEK_CUR)
	{
		SG_uint64 new_pos = (SG_uint64)((SG_int64)pMe->readState.pos+(SG_int64)offset);

		SG_context__push_level(pMe->pCtx);
		SG_log__report_verbose(pMe->pCtx, "SG_curl handling SEEK_CUR.");
		SG_context__pop_level(pMe->pCtx);

		SG_file__seek(pMe->pCtx, pMe->readState.pFile, new_pos);
		if(SG_context__has_err(pMe->pCtx))
			return CURL_SEEKFUNC_FAIL;

		pMe->readState.pos = new_pos;
		pMe->readState.finished = SG_FALSE;
		return CURL_SEEKFUNC_OK;
	}
	else if (origin == SEEK_END)
	{
		SG_uint64 new_pos = (SG_uint64)((SG_int64)pMe->readState.len+(SG_int64)offset);

		SG_context__push_level(pMe->pCtx);
		SG_log__report_verbose(pMe->pCtx, "SG_curl handling SEEK_END.");
		SG_context__pop_level(pMe->pCtx);

		SG_file__seek(pMe->pCtx, pMe->readState.pFile, new_pos);
		if(SG_context__has_err(pMe->pCtx))
			return CURL_SEEKFUNC_FAIL;

		pMe->readState.pos = new_pos;
		pMe->readState.finished = SG_FALSE;
		return CURL_SEEKFUNC_OK;
	}
	else
	{
		return CURL_SEEKFUNC_CANTSEEK;
	}
}

// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTWRITEFUNCTION

static void _check_for_and_handle_json_500(SG_context* pCtx, SG_curl* pCurl, char* buffer, SG_uint32 bufLen)
{
	SG_int32 responseCode = 0;
	_sg_curl* pMe = (_sg_curl*)pCurl;

	SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_RESPONSE_CODE, &responseCode)  );
	if (responseCode == 500 || responseCode == 410)
	{
		if (pMe->pstrErr == NULL)
		{
			SG_int32 lenResponse = 0;

			/* We assume an HTTP 500 response will be small enough to fit into an SG_string. */
			SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lenResponse)  );
			if (lenResponse > 0)
				SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pMe->pstrErr, (SG_uint32)lenResponse + 1)  );
			else
				SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pMe->pstrErr)  );
		}

		SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pMe->pstrErr, (const SG_byte*)buffer, bufLen)  );
	}

fail:
	;
}

// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTHEADERFUNCTION
static size_t _read_header( void *ptr, size_t size, size_t nmemb, void *userdata)
{
	_sg_curl * pMe = (_sg_curl*)userdata;
	SG_context * pCtx = pMe->pCtx;
		
	SG_ERR_IGNORE( SG_string__append__buf_len(pCtx, pMe->pstrRawHeaders, (SG_byte*)ptr, (SG_uint32) (size * nmemb)) );
	
	return size * nmemb;
}

static void _write_file_chunk(SG_context* pCtx, SG_curl* pCurl, char* buffer, SG_uint32 bufLen, void* pVoidState, SG_uint32* pLenHandled)
{
	SG_file* pFile = (SG_file*)pVoidState;

	SG_ERR_CHECK_RETURN(  _check_for_and_handle_json_500(pCtx, pCurl, buffer, bufLen)  );

	if (bufLen) // libcurl docs say we may be called with zero length if file is empty
		SG_ERR_CHECK_RETURN(  SG_file__write(pCtx, pFile, bufLen, (SG_byte*)buffer, pLenHandled)  );
}

static void _write_string_chunk(SG_context* pCtx, SG_curl* pCurl, char* buffer, SG_uint32 bufLen, void* pVoidState, SG_uint32* pLenHandled)
{
	SG_string* pstrResponse = (SG_string*)pVoidState;
	//_sg_curl* pMe = (_sg_curl*)pCurl;
	*pLenHandled = 0;

	SG_ERR_CHECK_RETURN(  _check_for_and_handle_json_500(pCtx, pCurl, buffer, bufLen)  );

	if (bufLen) // libcurl docs say we may be called with zero length if file is empty
	{
		if (pstrResponse)
			SG_ERR_CHECK_RETURN(  SG_string__append__buf_len(pCtx, pstrResponse, (const SG_byte*)buffer, bufLen)  );
		*pLenHandled = bufLen;
	}
}

//////////////////////////////////////////////////////////////////////////

/* Set up to use the built-in read/write callbacks. */

void SG_curl__set__read_string(SG_context* pCtx, SG_curl* pCurl, SG_string* pString)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pCurl);
	SG_UNUSED(pString);

	SG_ERR_THROW_RETURN(SG_ERR_NOTIMPLEMENTED);
}

/**
 * Append the HTTP response to the end of the provided SG_string.
 * The caller retains ownership of the string and should free it.
 */
void SG_curl__set__write_string(SG_context* pCtx, SG_curl* pCurl, SG_string* pString)
{
	_sg_curl* pMe = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);

	pMe->pWriteState = pString;
	pMe->pFnWriteResponse = _write_string_chunk;
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_WRITEDATA, pCurl)  );
	SG_ERR_CHECK_RETURN(  _setopt__write_cb(pCtx, pCurl, CURLOPT_WRITEFUNCTION, _write_callback_shim)  );
}

/**
 * Read the HTTP request from the provided SG_file, which should be open with a readable handle.
 * The caller retains ownership of the file and should free/close/delete it after SG_curl__perform.
 */
void SG_curl__set__read_file(SG_context* pCtx, SG_curl* pCurl, SG_file* pFile, SG_uint64 length)
{
	_sg_curl* pMe = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);
	SG_NULLARGCHECK_RETURN(pFile);

	pMe->readState.pFile = pFile;
	pMe->readState.pos = 0;
	pMe->readState.len = length;
	pMe->readState.finished = SG_FALSE;
	pMe->pFnReadRequest = _read_file_chunk;
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_READDATA, pCurl)  );
	SG_ERR_CHECK_RETURN(  _setopt__read_cb(pCtx, pCurl, CURLOPT_READFUNCTION, _read_callback_shim)  );
	SG_ERR_CHECK_RETURN(  _setopt__ioctl_cb(pCtx, pCurl, CURLOPT_IOCTLFUNCTION, _my_ioctl__file)  );
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_IOCTLDATA, pCurl)  );
	SG_ERR_CHECK_RETURN(  _setopt__seek_cb(pCtx, pCurl, CURLOPT_SEEKFUNCTION, _my_seek__file)  );
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_SEEKDATA, pCurl)  );
}

void SG_curl__set__write_cb(SG_context* pCtx, SG_curl* pCurl, SG_curl__callback* pFn, void* pState)
{
	_sg_curl* pMe = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);
	SG_NULLARGCHECK_RETURN(pFn);

	pMe->pWriteState = pState;
	pMe->pFnWriteResponse = pFn;
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_WRITEDATA, pCurl)  );
	SG_ERR_CHECK_RETURN(  _setopt__write_cb(pCtx, pCurl, CURLOPT_WRITEFUNCTION, _write_callback_shim)  );
}

/**
 * Write the HTTP response to the provided SG_file, which should be open with a writeable handle.
 * The caller retains ownership of the file and should free/close/delete it after SG_curl__perform.
 */
void SG_curl__set__write_file(SG_context* pCtx, SG_curl* pCurl, SG_file* pFile)
{
	_sg_curl* pMe = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);
	SG_NULLARGCHECK_RETURN(pFile);

	pMe->pWriteState = pFile;
	pMe->pFnWriteResponse = _write_file_chunk;
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_WRITEDATA, pCurl)  );
	SG_ERR_CHECK_RETURN(  _setopt__write_cb(pCtx, pCurl, CURLOPT_WRITEFUNCTION, _write_callback_shim)  );
}

/**
 * Use the provided progress callback for this request/response.
 * The caller retains ownership of pState and should free it after SG_curl__perform.
 */
void SG_curl__set__progress_cb(SG_context* pCtx, SG_curl* pCurl, SG_curl_progress_callback* pcb, void* pState)
{
	_sg_curl* pMe = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);
	SG_NULLARGCHECK_RETURN(pcb);

	pMe->pProgressState = pState;
	pMe->pFnProgress = pcb;

	SG_ERR_CHECK_RETURN(  SG_curl__setopt__int32(pCtx, pCurl, CURLOPT_NOPROGRESS, 0)  );
	SG_ERR_CHECK_RETURN(  _setopt__pv(pCtx, pCurl, CURLOPT_PROGRESSDATA, pCurl)  );
	SG_ERR_CHECK_RETURN(  _setopt__progress_cb(pCtx, pCurl, CURLOPT_PROGRESSFUNCTION, _progress_callback_shim)  );
}

//////////////////////////////////////////////////////////////////////////


void SG_curl__global_init(SG_context* pCtx)
{
	CURLcode rc;

	rc = curl_global_init(CURL_GLOBAL_ALL);

	if (rc)
		SG_ERR_THROW_RETURN(SG_ERR_LIBCURL(rc));
}

void SG_curl__global_cleanup()
{
	(void) curl_global_cleanup();
}

void SG_curl__free(SG_context* pCtx, SG_curl* pCurl)
{
	_sg_curl* p = (_sg_curl*)pCurl;

	if (p)
	{
		SG_STRING_NULLFREE(pCtx, p->pstrErr);
		SG_STRING_NULLFREE(pCtx, p->pstrRawHeaders);
		curl_easy_cleanup(p->pCurl);
		SG_NULLFREE(pCtx, p);
	}
}


void _set_curl_options(SG_context* pCtx, CURL* pCurl)
{
	char * szServerFiles = NULL;
	char * szVerifyCerts = NULL;
	SG_pathname *pServerFiles = NULL;
	CURLcode rc = CURLE_OK;
#ifdef WINDOWS
	SG_bool bExists = SG_FALSE;
	SG_bool bVerifyCerts = SG_TRUE;
#endif
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__VERIFY_SSL_CERTS, NULL, &szVerifyCerts, NULL)  );
	if (szVerifyCerts != NULL && (SG_strcmp__null(szVerifyCerts, "false") == 0 || SG_strcmp__null(szVerifyCerts, "FALSE")  == 0))
	{
#ifdef WINDOWS
		bVerifyCerts = SG_FALSE;
#endif
		rc = curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, SG_FALSE);		
	}

	if (rc)
		SG_ERR_THROW(SG_ERR_LIBCURL(rc));


#ifdef WINDOWS
	if (bVerifyCerts)
	{
		SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_FILES, NULL, &szServerFiles, NULL)  );
		if (szServerFiles)
		{
			SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pServerFiles, szServerFiles)  );
			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pServerFiles, "ssl") );
			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pServerFiles, "curl-ca-bundle.crt") );
			SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pServerFiles, &bExists, NULL, NULL)  );
		}

		if (bExists)
		{
			rc = curl_easy_setopt(pCurl, CURLOPT_CAINFO, SG_pathname__sz(pServerFiles));	
			if (rc)
				SG_ERR_THROW(SG_ERR_LIBCURL(rc));
		}
		else
		{
			if (pServerFiles)
				SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Could not find root certificate file. Looked for it at: %s. SSL connections will not work.", SG_pathname__sz(pServerFiles))  );
			else
				SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Could not find root certificate file: no server/files path is configured. SSL connections will not work.")  );
		}
	}
#endif

fail:
	SG_PATHNAME_NULLFREE(pCtx, pServerFiles);
	SG_NULLFREE(pCtx, szServerFiles);
	SG_NULLFREE(pCtx, szVerifyCerts);
}


void SG_curl__alloc(SG_context* pCtx, SG_curl** ppCurl)
{
	_sg_curl* p = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, p)  );

	p->pCurl = curl_easy_init();
	if (!p->pCurl)
		SG_ERR_THROW_RETURN(SG_ERR_LIBCURL(CURLE_FAILED_INIT));

	SG_ERR_CHECK(  _set_curl_options(pCtx, p->pCurl)  );
	p->pCtx = pCtx;

	*ppCurl = (SG_curl*)p;

	return;

fail:
	SG_ERR_IGNORE(  SG_curl__free(pCtx, (SG_curl*)p)  );
}

void SG_curl__reset(SG_context* pCtx, SG_curl* pCurl)
{
	_sg_curl* p = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);

	SG_STRING_NULLFREE(pCtx, p->pstrErr);
	SG_STRING_NULLFREE(pCtx, p->pstrRawHeaders);
	curl_easy_reset(p->pCurl);

	SG_ERR_CHECK(  _set_curl_options(pCtx, p->pCurl)  );

	p->pFnReadRequest = NULL;
	p->readState.pFile = NULL;
	p->readState.pos = 0;
	p->readState.len = 0;
	p->readState.finished = SG_FALSE;
	p->pFnWriteResponse = NULL;
	p->pWriteState = NULL;
	p->pFnProgress = NULL;
	p->pProgressState = NULL;
fail:
	return;
}

void SG_curl__record_headers(SG_context * pCtx, SG_curl* pCurl)
{
	CURLcode rc = CURLE_OK;
	_sg_curl* pMe = (_sg_curl*)pCurl;
	
	SG_NULLARGCHECK_RETURN(pCurl);
	
	SG_ERR_CHECK( SG_STRING__ALLOC(pMe->pCtx, &pMe->pstrRawHeaders) );
	rc = curl_easy_setopt(pMe->pCurl, CURLOPT_HEADERFUNCTION, _read_header);	
	if (rc)
		SG_ERR_THROW(SG_ERR_LIBCURL(rc));
	
	rc = curl_easy_setopt(pMe->pCurl, CURLOPT_WRITEHEADER, pCurl);	
	if (rc)
		SG_ERR_THROW(SG_ERR_LIBCURL(rc));

	return;
	
fail:
	SG_STRING_NULLFREE(pMe->pCtx, pMe->pstrRawHeaders);
}

void SG_curl__get_response_headers(SG_context * pCtx, SG_curl * pCurl, SG_string ** ppHeaders)
{
	_sg_curl* pMe = (_sg_curl*)pCurl;
	
	SG_ASSERT(pMe->pstrRawHeaders!=NULL);
	
	SG_ERR_CHECK_RETURN( SG_STRING__ALLOC__COPY(pCtx, ppHeaders, pMe->pstrRawHeaders) );
}

//////////////////////////////////////////////////////////////////////////

void SG_curl__set_headers_from_varray(SG_context * pCtx, SG_curl * pCurl, SG_varray * pvaHeaders, struct curl_slist ** ppHeaderList)
{
	CURLcode rc = CURLE_OK;
    _sg_curl* p = (_sg_curl*)pCurl;
	struct curl_slist* pHeaderList = NULL;
	SG_uint32 count = 0;
	SG_uint32 i = 0;
	
	SG_NULLARGCHECK_RETURN(pCurl);
	SG_NULLARGCHECK_RETURN(pvaHeaders);
	
	SG_ERR_CHECK( SG_varray__count(pCtx, pvaHeaders, &count) );
	for (i = 0; i < count; i++)
	{
		const char * psz = NULL;
		SG_ERR_CHECK_RETURN( SG_varray__get__sz(pCtx, pvaHeaders, i, &psz) );
		pHeaderList = curl_slist_append(pHeaderList, psz);
		if (!pHeaderList)
			SG_ERR_THROW2_RETURN(SG_ERR_UNSPECIFIED, (pCtx, "Failed to add HTTP header."));
		
	}
	
	rc = curl_easy_setopt(p->pCurl, CURLOPT_HTTPHEADER, pHeaderList);
	if (rc)
		SG_ERR_THROW2(SG_ERR_LIBCURL(rc), (pCtx, "Problem setting HTTP headers" ));

	SG_RETURN_AND_NULL(pHeaderList, ppHeaderList);
	
fail:
	if (pHeaderList)
		SG_CURL_HEADERS_NULLFREE(pCtx, pHeaderList);

}

void SG_curl__free_headers(SG_UNUSED_PARAM(SG_context* pCtx), struct curl_slist* pHeaderList)
{
	SG_UNUSED(pCtx);
	if (pHeaderList)
		curl_slist_free_all(pHeaderList);
}

//////////////////////////////////////////////////////////////////////////

void SG_curl__throw_on_non200(SG_context* pCtx, SG_curl* pCurl)
{
	SG_int32 httpResponseCode = 0;
	SG_vhash* pvhErr = NULL;
	_sg_curl* p = (_sg_curl*)pCurl;

	SG_NULLARGCHECK_RETURN(pCurl);

	SG_ERR_CHECK(  SG_curl__getinfo__int32(pCtx, pCurl, CURLINFO_RESPONSE_CODE, &httpResponseCode)  );
	if (httpResponseCode != 200)
	{
		if ((httpResponseCode == 500 || httpResponseCode == 410) && p->pstrErr)
		{
			SG_bool bHas = SG_FALSE;
			const char* szMsg = NULL;
			
			SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhErr, SG_string__sz(p->pstrErr));
			if (SG_context__err_equals(pCtx, SG_ERR_JSONPARSER_SYNTAX))
			{
				// The server didn't return a JSON-formatted response.
				if (httpResponseCode == 500)
					SG_ERR_RESET_THROW2(SG_ERR_EXTENDED_HTTP_500, (pCtx, "%s", SG_string__sz(p->pstrErr)));
				else
					SG_ERR_THROW2(SG_ERR_SERVER_HTTP_ERROR, (pCtx, "%d", httpResponseCode));
			}
			SG_ERR_CHECK_CURRENT;

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhErr, "msg", &bHas)  );
			if (bHas)
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhErr, "msg", &szMsg)  );
			if (szMsg)
				SG_ERR_THROW2(SG_ERR_EXTENDED_HTTP_500, (pCtx, "%s", szMsg));
			else
				SG_ERR_THROW2(SG_ERR_EXTENDED_HTTP_500, (pCtx, "%s", SG_string__sz(p->pstrErr)));
		}
		else if (httpResponseCode == 401)
		{
			SG_ERR_THROW(SG_ERR_AUTHORIZATION_REQUIRED);
		}
		else
			SG_ERR_THROW2(SG_ERR_SERVER_HTTP_ERROR, (pCtx, "%d", httpResponseCode));
	}

	/* common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhErr);
}

void SG_curl__perform(SG_context* pCtx, SG_curl* pCurl)
{
	CURLcode rc = CURLE_OK;
	_sg_curl* pMe = (_sg_curl*)pCurl;
	
	SG_NULLARGCHECK_RETURN(pCurl);
	
	rc = curl_easy_perform(pMe->pCurl);
	
	// Check for errors in the request and response callbacks.
	SG_ERR_CHECK_RETURN_CURRENT;

	// No callback errors. Make sure curl result code is also kosher.
	if (rc)
	{
		switch (rc)
		{
			case CURLE_SEND_ERROR:
			case CURLE_RECV_ERROR:
				SG_ERR_THROW2_RETURN(SG_ERR_LIBCURL(rc), (pCtx, "%s", "please try again."));
				break;
			
#if defined(WINDOWS)
			case CURLE_SSL_CACERT:
				SG_ERR_THROW2_RETURN(SG_ERR_LIBCURL(rc), (pCtx, "Verify that the server/files setting is correct, or use the command 'vv config set network/verify_ssl_certs false' to disable this warning."));
				break;
#endif

			default:
				SG_ERR_THROW_RETURN(SG_ERR_LIBCURL(rc));
		}
	}
}
