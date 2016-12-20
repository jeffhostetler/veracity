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
 * @file sg_uridispatch_prototypes.h
 *
 * SG_uridispatch is quite simply Veracity's api for getting an http response based on an http request.
 *
 */

#ifndef H_SG_URIDISPATCH_PROTOTYPES_H
#define H_SG_URIDISPATCH_PROTOTYPES_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


// SG_uridispatch__init() should be called one time, before any other calls into SG_uridispatch code are made.
void SG_uridispatch__init(
	SG_context * pCtx,

	const char ** ppApplicationRoot //< Output: where the veracity app exists on the server (For use with
	                                //  outside server integration. Pass in NULL for the embedded server.)
	);


///////////////////////////////////////////////////////////////////////////////


// Call this to begin an http request.
// Returns a "dispatch context", used for subsequent calls into SG_uridispatch.
// Will never return NULL.
SG_uridispatchcontext * SG_uridispatch__begin_request(
	SG_context * pCtx, //< Input parameter ONLY! Do not SG_ERR_CHECK.
	                   //  
	                   //  If NULL is passed in, we will basically do nothing, and the http response
	                   //  will be a 500 Internal Server Error (regardless of the rest of the
	                   //  parameters) that basically just says "An unknown error has occurred".
	                   //  
	                   //  If pCtx is in an error state when passed in, we'll log the error and
	                   //  create a 500 Internal Server Error response for it (regardless of the
	                   //  rest of the parameters).
	                   //  
	                   //  After SG_uridispatch__begin_request() returns, any error in pCtx will
	                   //  have been cleared out. In fact pCtx will be in a clean state after any
	                   //  call into ANY SG_uridispatch function.

	const char * szRequestMethod,
	const char * szUri,
	const char * szQueryString,

	const char * szScheme, // "http:" or "https:". Optional. Used to construct links, if provided.

	SG_vhash ** ppHeaders //< vhash of request headers. We take ownership and null the caller's copy.
	);


// Call this to get the response's headers.
// Returns SG_TRUE if the http response is ready and the output parameters have been populated.
// Returns SG_FALSE if more of the request's message body needs to be chunked in before a response can be generated.
SG_bool SG_uridispatch__get_response_headers(
	SG_uridispatchcontext * pDispatchContext,

	const char ** ppStatusCode, //< The http status code. Caller does not own the result.

	SG_uint64 * pContentLength, //< The Content-Length header.

	SG_vhash ** ppResponseHeaders //< All other headers. Caller owns it. Can be NULL.
	                              //  
	                              //  If it's NULL and *pContentLength>0, assume a
								  //  Content-Type header of "text/plain;charset=UTF-8".
								  //  No other headers beyond that.
	);


// Call this if more of the request's message body needs to be chunked in.
void SG_uridispatch__chunk_request_body(
	SG_uridispatchcontext * pDispatchContext,

	const SG_byte * pBuffer,
	SG_uint32 bufferLength
	);


// Call this to get the response's message body, chunk by chunk.
//
// Always call this at least once, even if the content length is 0. Keep calling it until the
// dispatch context has been set to NULL. There may or may not be a call at the end (after all
// content has been fetched) which fetches nothing.
void SG_uridispatch__chunk_response_body(
	SG_uridispatchcontext ** ppDispatchContext,

	const SG_byte ** ppBuffer,
	SG_uint32 * pBufferLength
	);


// For the indecisive. A way to say "nevermind".
void SG_uridispatch__abort(
	SG_uridispatchcontext ** ppDispatchContext
	);

/**
* Global cleanup routine for uridispatch.  This is needed after
* shutting down the mongoose web server, if you want to start it again.
* This will automatically be called from SG_lib__global_cleanup.
*/
void SG_uridispatch__global_cleanup(SG_context * pCtx);

///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
