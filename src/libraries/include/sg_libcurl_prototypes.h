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
 * @file sg_libcurl_prototypes.h
 *
 * @details sglib's thin crunchy wrapper around libcurl, primarily
 *			to play nice with its error handling.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_LIBCURL_PROTOTYPES_H
#define H_SG_LIBCURL_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_curl__global_init(SG_context* pCtx);
void SG_curl__global_cleanup(void);

void SG_curl__alloc(SG_context* pCtx, SG_curl** ppCurl);
void SG_curl__free(SG_context* pCtx, SG_curl* ppCurl);

void SG_curl__reset(SG_context* pCtx, SG_curl* pCurl);

void SG_curl__setopt__sz(SG_context* pCtx, SG_curl* pCurl, CURLoption option, const char* pszVal);
void SG_curl__setopt__int32(SG_context* pCtx, SG_curl* pCurl, CURLoption option, SG_int32 iVal);
void SG_curl__setopt__int64(SG_context* pCtx, SG_curl* pCurl, CURLoption option, SG_int64 iVal);

/* Set generic read/write callbacks: if you want to use something other than SG_strings or SG_files, implement these. */
//void SG_curl__set__read_cb(SG_context* pCtx, SG_curl* pCurl, SG_curl__callback cb, SG_curl_progress_callback pcb, void* pvProgressState);
//void SG_curl__set__write_cb(SG_context* pCtx, SG_curl* pCurl, SG_curl__callback cb, SG_curl_progress_callback pcb, void* pvProgressState);

/* Not implemented */
// void SG_curl__set__read_string(SG_context* pCtx, SG_curl* pCurl, SG_string* pString);

/**
 * Append the HTTP response to the end of the provided SG_string.
 * The caller retains ownership of the string and should free it.
 */
void SG_curl__set__write_string(SG_context* pCtx, SG_curl* pCurl, SG_string* pString);

/**
 * Append the HTTP response to the end of the provided SG_string.
 * The caller retains ownership of the string and should free it.
 */
void SG_curl__set__read_file(SG_context* pCtx, SG_curl* pCurl, SG_file* pFile, SG_uint64 length);

/**
 * Write the HTTP response to the provided SG_file, which should be open with a writeable handle.
 * The caller retains ownership of the file and should free/close/delete it after SG_curl__perform.
 */
void SG_curl__set__write_file(SG_context* pCtx, SG_curl* pCurl, SG_file* pFile);

/**
 * Use the provided write callback for this request/response.
 * The caller retains ownership of pState and should free it after SG_curl__perform.
 */
void SG_curl__set__write_cb(SG_context* pCtx, SG_curl* pCurl, SG_curl__callback* pFn, void* pState);
	
/**
 * Use the provided progress callback for this request/response.
 * The caller retains ownership of pState and should free it after SG_curl__perform.
 */
void SG_curl__set__progress_cb(SG_context* pCtx, SG_curl* pCurl, SG_curl_progress_callback* pcb, void* pState);

void SG_curl__getinfo__int32(SG_context* pCtx, SG_curl* pCurl, CURLINFO info, SG_int32* piVal);

void SG_curl__set_headers_from_varray(SG_context* pCtx, SG_curl* pCurl, SG_varray* pvaHeaders, struct curl_slist ** ppHeaderList);

void SG_curl__free_headers(SG_context* pCtx, struct curl_slist* pHeaderList);

/**
 * Use the callbacks provided in the library to record the raw headers
 * in the http response
 */ 
void SG_curl__record_headers(SG_context * pCtx, SG_curl* pCurl);

 /** 
 * Get the raw HTTP response headers out of the curl context
 */
void SG_curl__get_response_headers(SG_context * pCtx, SG_curl * pCurl, SG_string ** ppHeaders);

void SG_curl__throw_on_non200(SG_context* pCtx, SG_curl* pCurl);

void SG_curl__perform(SG_context* pCtx, SG_curl* pCurl);

#define SG_CURL_NULLFREE(pCtx,p)				SG_STATEMENT(	SG_context__push_level(pCtx); \
																SG_curl__free(pCtx, p); \
																SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); \
																SG_context__pop_level(pCtx); \
																p=NULL; )

#define SG_CURL_HEADERS_NULLFREE(pCtx,p)		SG_STATEMENT(	SG_context__push_level(pCtx); \
																SG_curl__free_headers(pCtx, p); \
																SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); \
																SG_context__pop_level(pCtx); \
																p=NULL; )

END_EXTERN_C;

#endif//H_SG_LIBCURL_PROTOTYPES_H
