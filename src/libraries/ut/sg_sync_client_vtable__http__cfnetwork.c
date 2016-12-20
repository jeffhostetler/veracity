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

/*
 *
 * @file sg_client_vtable__http__cfnetwork.c
 *
 */

#include <sg.h>
#include "sg_sync_client__api.h"

#include "sg_sync_client_vtable__http.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

//////////////////////////////////////////////////////////////////////////

#define SYNC_URL_SUFFIX     "/sync"
#define JSON_URL_SUFFIX     ".json"
#define FRAGBALL_URL_SUFFIX ".fragball"

#define PUSH_ID_KEY				"id"

//////////////////////////////////////////////////////////////////////////

struct _sg_sync_client_http_push_handle
{
	char* pszPushId;
};
typedef struct _sg_sync_client_http_push_handle sg_sync_client_http_push_handle;
static void _handle_free(SG_context * pCtx, sg_sync_client_http_push_handle* pPush)
{
	if (pPush)
	{
		if ( (pPush)->pszPushId )
			SG_NULLFREE(pCtx, pPush->pszPushId);

		SG_NULLFREE(pCtx, pPush);
	}
}
#define _NULLFREE_PUSH_HANDLE(pCtx,p) SG_STATEMENT(  _handle_free(pCtx, p); p=NULL;  )

struct _sg_client_http_instance_data
{
    int dummy;
};
typedef struct _sg_client_http_instance_data sg_client_http_instance_data;

//////////////////////////////////////////////////////////////////////////

static void _get_sync_url(SG_context* pCtx, const char* pszBaseUrl, const char* pszUrlSuffix, const char* pszPushId, const char* pszQueryString, char** ppszUrl)
{
	SG_string* pstrUrl = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrUrl, 1024)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszBaseUrl)  );

	if (pszUrlSuffix)
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszUrlSuffix)  );

	if (pszPushId)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, "/")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszPushId)  );
	}
	if (pszQueryString)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, "?")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrUrl, pszQueryString)  );
	}

	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pstrUrl, (SG_byte**)ppszUrl, NULL)  );
	
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstrUrl);
}

static void make_request(
                               SG_context* pCtx,
                               const char* pszUrl,
                               const char* psz_method,
                               const char* psz_data,
                               CFHTTPMessageRef* pp
                               )
{
    CFStringRef url = NULL;
    CFURLRef myURL = NULL;
    CFStringRef requestMethod = NULL;
    CFHTTPMessageRef myRequest = NULL;
    
    url = CFStringCreateWithCString(kCFAllocatorDefault, pszUrl,kCFStringEncodingUTF8);
    SG_NULLARGCHECK(url);
    
    myURL = CFURLCreateWithString(kCFAllocatorDefault, url, NULL);
    SG_NULLARGCHECK(myURL);
    
    requestMethod = CFStringCreateWithCString(kCFAllocatorDefault, psz_method,kCFStringEncodingUTF8);
    SG_NULLARGCHECK(requestMethod);
    
    myRequest = CFHTTPMessageCreateRequest(
                                           kCFAllocatorDefault, 
                                           requestMethod, 
                                           myURL, 
                                           kCFHTTPVersion1_1
                                           );
    SG_NULLARGCHECK(myRequest);
    
    if (psz_data)
    {
        CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, psz_data, kCFStringEncodingUTF8);
        CFDataRef bodyData = CFStringCreateExternalRepresentation(kCFAllocatorDefault, s, kCFStringEncodingUTF8, 0);
        CFRelease(s);
        CFHTTPMessageSetBody(myRequest, bodyData);
        CFRelease(bodyData);
    }
    
    *pp = myRequest;
    myRequest = NULL;
    
fail:
    if (myRequest)
    {
        CFRelease(myRequest);
        myRequest = NULL;
    }
    
    if (myURL)
    {
        CFRelease(myURL);
        myURL = NULL;
    }
    
    if (requestMethod)
    {
        CFRelease(requestMethod);
        requestMethod = NULL;
    }
    
    if (url)
    {
        CFRelease(url);
        url = NULL;
    }
}

static void send_upload_request(
        SG_context* pCtx,
        CFHTTPMessageRef myRequest,
        CFReadStreamRef upload,
        CFReadStreamRef* pp
        )
{
    CFReadStreamRef myReadStream = NULL;

    myReadStream = CFReadStreamCreateForStreamedHTTPRequest(kCFAllocatorDefault, myRequest, upload);
     
    if (!CFReadStreamOpen(myReadStream)) 
    {
        CFStreamError myErr = CFReadStreamGetError(myReadStream);

        if (myErr.domain == kCFStreamErrorDomainPOSIX) 
        {
            // Interpret myErr.error as a UNIX errno.
            SG_ERR_THROW(  SG_ERR_ERRNO(myErr.error)  );
        } 
        else if (myErr.domain == kCFStreamErrorDomainMacOSStatus) 
        {
            // Interpret myErr.error as a MacOS error code.
            // TODO SG_ERR_THROW(  SG_ERR_MAC((OSStatus) myErr.error)  );
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }
    }

    *pp = myReadStream;
    myReadStream = NULL;

fail:
    if (myReadStream)
    {
        CFReadStreamClose(myReadStream);
        CFRelease(myReadStream);
        myReadStream = NULL;
    }
}

static void send_request(
        SG_context* pCtx,
        CFHTTPMessageRef myRequest,
        CFReadStreamRef* pp
        )
{
    CFReadStreamRef myReadStream = NULL;

    myReadStream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, myRequest);
     
    if (!CFReadStreamOpen(myReadStream)) 
    {
        CFStreamError myErr = CFReadStreamGetError(myReadStream);

        if (myErr.domain == kCFStreamErrorDomainPOSIX) 
        {
            // Interpret myErr.error as a UNIX errno.
            SG_ERR_THROW(  SG_ERR_ERRNO(myErr.error)  );
        } 
        else if (myErr.domain == kCFStreamErrorDomainMacOSStatus) 
        {
            // Interpret myErr.error as a MacOS error code.
            // TODO SG_ERR_THROW(  SG_ERR_MAC((OSStatus) myErr.error)  );
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }
    }

    *pp = myReadStream;
    myReadStream = NULL;

fail:
    if (myReadStream)
    {
        CFReadStreamClose(myReadStream);
        CFRelease(myReadStream);
        myReadStream = NULL;
    }
}

static void read_entire_stream__file(
        SG_context* pCtx,
        CFReadStreamRef myReadStream,
        SG_pathname* pPath,
        CFHTTPMessageRef* pp,
        SG_bool b_progress
        )
{
    CFHTTPMessageRef myResponse = NULL;
    CFIndex numBytesRead = 0;
    SG_file* pFile = NULL;
    SG_int64 content_length = 0;
    SG_int64 so_far = 0;

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_CREATE_NEW | SG_FILE_WRONLY, 0644, &pFile)  );
    do 
    {
        UInt8 buf[8192]; // TODO
        numBytesRead = CFReadStreamRead(myReadStream, buf, sizeof(buf));
        if( numBytesRead > 0 ) 
        {
            SG_ERR_CHECK(  SG_file__write(pCtx, pFile, numBytesRead, buf, NULL)  );
            if (!myResponse)
            {
                myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(myReadStream, kCFStreamPropertyHTTPResponseHeader);
                CFStringRef contentLengthString = CFHTTPMessageCopyHeaderFieldValue(myResponse, CFSTR("Content-Length"));
                if (contentLengthString) 
                {
                    // TODO 32 bit limit problem here
                    content_length = CFStringGetIntValue(contentLengthString);
                    CFRelease(contentLengthString);
                }
                if (b_progress)
                {
                    SG_ERR_CHECK(  SG_log__set_steps(pCtx, content_length / 1024, "KB")  );
                }
            }
            so_far += (SG_uint32) numBytesRead;
            if (b_progress)
            {
                SG_ERR_CHECK(  SG_log__set_finished(pCtx, so_far / 1024)  );
            }
        } 
        else if( numBytesRead < 0 ) 
        {
            CFStreamError myErr = CFReadStreamGetError(myReadStream);
            // TODO clean this up
            if (myErr.domain == kCFStreamErrorDomainPOSIX) 
            {
                if (ETIMEDOUT == myErr.error)
                {
                    usleep(5000);
                    numBytesRead = 0;
                }
                else
                {
                    // Interpret myErr.error as a UNIX errno.
                    SG_ERR_THROW(  SG_ERR_ERRNO(myErr.error)  );
                }
            } 
            else if (myErr.domain == kCFStreamErrorDomainMacOSStatus) 
            {
                // Interpret myErr.error as a MacOS error code.
                // TODO SG_ERR_THROW(  SG_ERR_MAC((OSStatus) myErr.error)  );
                SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
            }
        }
    } while( numBytesRead > 0 );

    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

    if (!myResponse)
    {
        myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(myReadStream, kCFStreamPropertyHTTPResponseHeader);
    }
    
    *pp = myResponse;
    myResponse = NULL;

fail:
    if (pFile)
    {
        SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
        // TODO delete it too
    }
    if (myResponse)
    {
        CFRelease(myResponse);
    }
}

static void read_entire_stream__string(
        SG_context* pCtx,
        CFReadStreamRef myReadStream,
        SG_string** ppstr,
        CFHTTPMessageRef* pp
        )
{
    SG_string* pstr = NULL;
    CFHTTPMessageRef myResponse = NULL;
    CFIndex numBytesRead = 0;

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr, "")  );
    do 
    {
        UInt8 buf[8192]; // TODO
        numBytesRead = CFReadStreamRead(myReadStream, buf, sizeof(buf));
        if( numBytesRead > 0 ) 
        {
            SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pstr, buf, numBytesRead)  );

            if (!myResponse)
            {
                myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(myReadStream, kCFStreamPropertyHTTPResponseHeader);
            }
        } 
        else if( numBytesRead < 0 ) 
        {
            CFStreamError myErr = CFReadStreamGetError(myReadStream);
            // TODO clean this up
            if (myErr.domain == kCFStreamErrorDomainPOSIX) 
            {
                if (ETIMEDOUT == myErr.error)
                {
                    usleep(5000);
                    numBytesRead = 0;
                }
                else
                {
                    // Interpret myErr.error as a UNIX errno.
                    SG_ERR_THROW(  SG_ERR_ERRNO(myErr.error)  );
                }
            } 
            else if (myErr.domain == kCFStreamErrorDomainMacOSStatus) 
            {
                // Interpret myErr.error as a MacOS error code.
                // TODO SG_ERR_THROW(  SG_ERR_MAC((OSStatus) myErr.error)  );
                SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
            }
        }
    } while( numBytesRead > 0 );

    if (!myResponse)
    {
        myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(myReadStream, kCFStreamPropertyHTTPResponseHeader);
    }
    
    *pp = myResponse;
    myResponse = NULL;

    *ppstr = pstr;
    pstr = NULL;

fail:
    if (myResponse)
    {
        CFRelease(myResponse);
    }
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_sync_client__http__open(
	SG_context* pCtx,
	SG_sync_client * pSyncClient)
{
	sg_client_http_instance_data* pMe = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pMe)  );
	pSyncClient->p_vtable_instance_data = (sg_sync_client__vtable__instance_data *)pMe;
}

void sg_sync_client__http__close(SG_context * pCtx, SG_sync_client * pSyncClient)
{
	sg_client_http_instance_data* pMe = NULL;

	if (!pSyncClient)
		return;

	pMe = (sg_client_http_instance_data*)pSyncClient->p_vtable_instance_data;

	SG_NULLFREE(pCtx, pMe);
}

//////////////////////////////////////////////////////////////////////////

static void perform_upload_request__string(
                                    SG_context* pCtx,
                                    CFHTTPMessageRef myRequest,
                                    SG_pathname* pPath,
                                    CFHTTPMessageRef* pmyResponse,
                                    SG_string** ppstr
                                    )
{
    CFReadStreamRef myReadStream = NULL;
    CFHTTPMessageRef myResponse = NULL;
    SG_string* pstr = NULL;
    CFReadStreamRef upload = NULL;
    CFURLRef upload_file_url = NULL;
    
    // set the content-length header
    {
        SG_uint64 len = 0;
        SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len, NULL)  );

        CFStringRef headerFieldName = CFSTR("Content-Length");
        CFStringRef headerFieldValue = CFStringCreateWithFormat (kCFAllocatorDefault, NULL,  CFSTR("%d"), len);
        CFHTTPMessageSetHeaderFieldValue(myRequest, headerFieldName, headerFieldValue);
        CFRelease(headerFieldValue);
    }

    upload_file_url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*) SG_pathname__sz(pPath), SG_STRLEN(SG_pathname__sz(pPath)), SG_FALSE);
    upload = CFReadStreamCreateWithFile(kCFAllocatorDefault, upload_file_url);
    CFRelease(upload_file_url);
    if (!CFReadStreamOpen(upload))
    {
        CFStreamError myErr = CFReadStreamGetError(upload);

        if (myErr.domain == kCFStreamErrorDomainPOSIX) 
        {
            // Interpret myErr.error as a UNIX errno.
            SG_ERR_THROW(  SG_ERR_ERRNO(myErr.error)  );
        } 
        else if (myErr.domain == kCFStreamErrorDomainMacOSStatus) 
        {
            // Interpret myErr.error as a MacOS error code.
            // TODO SG_ERR_THROW(  SG_ERR_MAC((OSStatus) myErr.error)  );
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }
    }

    SG_ERR_CHECK(  send_upload_request(pCtx, myRequest, upload, &myReadStream)  );
    SG_ERR_CHECK(  read_entire_stream__string(pCtx, myReadStream, &pstr, &myResponse)  );

    *ppstr = pstr;
    pstr = NULL;
    
    *pmyResponse = myResponse;
    myResponse = NULL;
    
fail:
    if (upload)
    {
        CFRelease(upload);
    }
    if (myReadStream)
    {
        CFReadStreamClose(myReadStream);
        CFRelease(myReadStream);
        myReadStream = NULL;
    }
    
    if (myResponse)
    {
        CFRelease(myResponse);
        myResponse = NULL;
    }
    
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void perform_request__string(
                                    SG_context* pCtx,
                                    CFHTTPMessageRef myRequest,
                                    CFHTTPMessageRef* pmyResponse,
                                    SG_string** ppstr
                                    )
{
    CFReadStreamRef myReadStream = NULL;
    CFHTTPMessageRef myResponse = NULL;
    SG_string* pstr = NULL;
    
    SG_ERR_CHECK(  send_request(pCtx, myRequest, &myReadStream)  );
    SG_ERR_CHECK(  read_entire_stream__string(pCtx, myReadStream, &pstr, &myResponse)  );

    *ppstr = pstr;
    pstr = NULL;
    
    *pmyResponse = myResponse;
    myResponse = NULL;
    
fail:
    if (myReadStream)
    {
        CFReadStreamClose(myReadStream);
        CFRelease(myReadStream);
        myReadStream = NULL;
    }
    
    if (myResponse)
    {
        CFRelease(myResponse);
        myResponse = NULL;
    }
    
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void perform_request__file(
                                    SG_context* pCtx,
                                    CFHTTPMessageRef myRequest,
                                    CFHTTPMessageRef* pmyResponse,
                                    SG_pathname* pPath,
                                    SG_bool b_progress
                                    )
{
    CFReadStreamRef myReadStream = NULL;
    CFHTTPMessageRef myResponse = NULL;
    
    SG_ERR_CHECK(  send_request(pCtx, myRequest, &myReadStream)  );
    SG_ERR_CHECK(  read_entire_stream__file(pCtx, myReadStream, pPath, &myResponse, b_progress)  );

    *pmyResponse = myResponse;
    myResponse = NULL;
    
fail:
    if (myReadStream)
    {
        CFReadStreamClose(myReadStream);
        CFRelease(myReadStream);
        myReadStream = NULL;
    }
    
    if (myResponse)
    {
        CFRelease(myResponse);
        myResponse = NULL;
    }
}

// TODO give this a better name.  It gets used outside this file now.
void do_url(
                              SG_context* pCtx,
                              const char* pszUrl,
                              const char* psz_method,
                              const char* psz_data,
                              const char* psz_username,
                              const char* psz_password,
                              SG_string** ppstr,
                              SG_pathname* pPath,
                              SG_bool b_progress
                              )
{
    SG_string* pstr = NULL;
    CFHTTPMessageRef myRequest = NULL;
    CFHTTPMessageRef myResponse = NULL;
    CFStringRef s_username = NULL;
    CFStringRef s_password = NULL;
    
    if (pPath && ppstr)
    {
		SG_ERR_RESET_THROW2(SG_ERR_INVALIDARG, (pCtx, "do_url() returns into a string or a file, but not both"));
    }

    if (!pPath && !ppstr)
    {
		SG_ERR_RESET_THROW2(SG_ERR_INVALIDARG, (pCtx, "do_url() returns into a string or a file, one or the other"));
    }

    SG_ERR_CHECK(  make_request(pCtx, pszUrl, psz_method, psz_data, &myRequest)  );

#if 0
    {
        CFDataRef d = CFHTTPMessageCopySerializedMessage(myRequest);
        fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
        CFRelease(d);
    }
#endif

    if (pPath)
    {
        SG_ERR_CHECK(  perform_request__file(pCtx, myRequest, &myResponse, pPath, b_progress)  );
    }
    else
    {
        SG_ERR_CHECK(  perform_request__string(pCtx, myRequest, &myResponse, &pstr)  );
    }
    
    if (!myResponse)
    {
        SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "No response from server"));        
    }
    
    //fprintf(stderr, "\n%s\n", SG_string__sz(pstr));
    UInt32 statusCode = CFHTTPMessageGetResponseStatusCode(myResponse);
#if 0
    {
        CFDataRef d = CFHTTPMessageCopySerializedMessage(myResponse);
        fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
        CFRelease(d);
    }
#endif
    
    if (
            psz_username 
            && psz_password
            && (statusCode == 401 || statusCode == 407)
            )
    {
        s_username = CFStringCreateWithCString(kCFAllocatorDefault, psz_username, kCFStringEncodingUTF8);
        s_password = CFStringCreateWithCString(kCFAllocatorDefault, psz_password, kCFStringEncodingUTF8);
        if (!CFHTTPMessageAddAuthentication(myRequest, myResponse, s_username, s_password, kCFHTTPAuthenticationSchemeDigest, FALSE))
        {
            SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "CFHTTPMessageAddAuthentication failed"));
        }

#if 0
        {
            CFDataRef d = CFHTTPMessageCopySerializedMessage(myRequest);
            fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
            CFRelease(d);
        }
#endif

        CFRelease(s_username);
        s_username = NULL;

        CFRelease(s_password);
        s_password = NULL;

        CFRelease(myResponse);
        myResponse = NULL;

        if (pPath)
        {
            SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath)  );
            SG_ERR_CHECK(  perform_request__file(pCtx, myRequest, &myResponse, pPath, b_progress)  );
        }
        else
        {
            SG_STRING_NULLFREE(pCtx, pstr);
            SG_ERR_CHECK(  perform_request__string(pCtx, myRequest, &myResponse, &pstr)  );
        }
#if 0
        {
            CFDataRef d = CFHTTPMessageCopySerializedMessage(myResponse);
            fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
            CFRelease(d);
        }
#endif
        statusCode = CFHTTPMessageGetResponseStatusCode(myResponse);
    }
    
    if (statusCode != 200)
    {
        if (401 == statusCode)
        {
            SG_ERR_THROW(SG_ERR_HTTP_401);
        }
        else if (404 == statusCode)
        {
            SG_ERR_THROW(SG_ERR_HTTP_404);
        }
        else if (502 == statusCode)
        {
            SG_ERR_THROW(SG_ERR_HTTP_502);
        }
        else
        {
            SG_ERR_THROW2(SG_ERR_SERVER_HTTP_ERROR, (pCtx, "%d", (int) statusCode));
        }
    }
   
    if (ppstr)
    {
        *ppstr = pstr;
        pstr = NULL;
    }
    
	/* fall through */
    
fail:
    if (s_username)
    {
        CFRelease(s_username);
        s_username = NULL;
    }

    if (s_password)
    {
        CFRelease(s_password);
        s_password = NULL;
    }

    if (myRequest)
    {
        CFRelease(myRequest);
        myRequest = NULL;
    }
    
    if (myResponse)
    {
        CFRelease(myResponse);
        myResponse = NULL;
    }
    
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_sync_client__http__push_begin(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_pathname** pFragballDirPathname,
	SG_sync_client_push_handle** ppPush)
{
	char* pszUrl = NULL;
	sg_sync_client_http_push_handle* pPush = NULL;
	SG_vhash* pvhResponse = NULL;
	SG_pathname* pPathUserTemp = NULL;
	SG_pathname* pPathFragballDir = NULL;
	char bufTid[SG_TID_MAX_BUFFER_LENGTH];
    SG_string* pstr = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pFragballDirPathname);
	SG_NULLARGCHECK_RETURN(ppPush);

	// Get the URL we're going to post to
	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX JSON_URL_SUFFIX, NULL, NULL, &pszUrl)  );

    SG_ERR_CHECK(  do_url(pCtx, pszUrl, "POST", NULL, pSyncClient->psz_username, pSyncClient->psz_password, &pstr, NULL, SG_TRUE)  );
    SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvhResponse, SG_string__sz(pstr))  );
    SG_STRING_NULLFREE(pCtx, pstr);

	// Alloc a push handle.  Stuff the push ID we received into it.
	{
		const char* pszRef = NULL;
		SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(sg_sync_client_http_push_handle), &pPush)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResponse, PUSH_ID_KEY, &pszRef)  );
		SG_ERR_CHECK(  SG_strdup(pCtx, pszRef, &pPush->pszPushId)  );
	}

	// Create a temporary local directory for stashing fragballs before shipping them over the network.
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathUserTemp)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, bufTid, SG_TID_MAX_BUFFER_LENGTH)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFragballDir, pPathUserTemp, bufTid)  );
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathFragballDir)  );

	// Tell caller where to stash fragballs for this push.
	SG_RETURN_AND_NULL(pPathFragballDir, pFragballDirPathname);

	// Return the new push handle.
	*ppPush = (SG_sync_client_push_handle*)pPush;
	pPush = NULL;

	/* fall through */
fail:
    SG_STRING_NULLFREE(pCtx, pstr);
	if(SG_context__err_equals(pCtx, SG_ERR_SERVER_HTTP_ERROR))
	{
		const char * szInfo = NULL;
		if(SG_IS_OK(SG_context__err_get_description(pCtx, &szInfo)) && strcmp(szInfo, "405")==0)
			SG_ERR_RESET_THROW(SG_ERR_SERVER_DOESNT_ACCEPT_PUSHES);
	}
	_NULLFREE_PUSH_HANDLE(pCtx, pPush);
	SG_NULLFREE(pCtx, pszUrl);
	SG_PATHNAME_NULLFREE(pCtx, pPathUserTemp);
	SG_PATHNAME_NULLFREE(pCtx, pPathFragballDir);
	SG_VHASH_NULLFREE(pCtx, pvhResponse);
}

static void do_upload__string(
	SG_context* pCtx,
    const char* pszUrl,
    const char* psz_method,
    SG_pathname* pPath,
    const char* psz_username,
    const char* psz_password,
    SG_string** ppstr
    )
{
    SG_string* pstr = NULL;
    CFHTTPMessageRef myRequest = NULL;
    CFHTTPMessageRef myResponse = NULL;
    CFStringRef s_username = NULL;
    CFStringRef s_password = NULL;
    
    SG_ERR_CHECK(  make_request(pCtx, pszUrl, psz_method, NULL, &myRequest)  );
    SG_ERR_CHECK(  perform_upload_request__string(pCtx, myRequest, pPath, &myResponse, &pstr)  );
#if 0
    {
        CFDataRef d = CFHTTPMessageCopySerializedMessage(myResponse);
        fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
        CFRelease(d);
    }
#endif
    UInt32 statusCode = CFHTTPMessageGetResponseStatusCode(myResponse);
    
    if (
        psz_username 
        && psz_password
        && (statusCode == 401 || statusCode == 407)
        )
    {
        s_username = CFStringCreateWithCString(kCFAllocatorDefault, psz_username, kCFStringEncodingUTF8);
        s_password = CFStringCreateWithCString(kCFAllocatorDefault, psz_password, kCFStringEncodingUTF8);
        if (!CFHTTPMessageAddAuthentication(myRequest, myResponse, s_username, s_password, kCFHTTPAuthenticationSchemeDigest, FALSE))
        {
            SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "CFHTTPMessageAddAuthentication failed"));
        }
        
#if 0
        {
            CFDataRef d = CFHTTPMessageCopySerializedMessage(myRequest);
            fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
            CFRelease(d);
        }
#endif
        
        CFRelease(s_username);
        s_username = NULL;
        
        CFRelease(s_password);
        s_password = NULL;
        
        CFRelease(myResponse);
        myResponse = NULL;
        
        SG_STRING_NULLFREE(pCtx, pstr);
        SG_ERR_CHECK(  perform_upload_request__string(pCtx, myRequest, pPath, &myResponse, &pstr)  );
        
#if 0
        {
            CFDataRef d = CFHTTPMessageCopySerializedMessage(myResponse);
            fprintf(stderr, "%s\n", CFDataGetBytePtr(d));
            CFRelease(d);
        }
#endif
        statusCode = CFHTTPMessageGetResponseStatusCode(myResponse);
    }
    
    if (statusCode != 200)
    {
        SG_ERR_THROW2_RETURN(SG_ERR_SERVER_HTTP_ERROR, (pCtx, "%d", (int) statusCode));
    }

	*ppstr = pstr;
	pstr = NULL;
    
	/* fall through */
    
fail:
    if (s_username)
    {
        CFRelease(s_username);
        s_username = NULL;
    }
    if (s_password)
    {
        CFRelease(s_password);
        s_password = NULL;
    }
    if (myRequest)
    {
        CFRelease(myRequest);
        myRequest = NULL;
    }
    
    if (myResponse)
    {
        CFRelease(myResponse);
        myResponse = NULL;
    }
    
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_sync_client__http__push_add(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle* pPush,
	SG_bool bProgressIfPossible, 
	SG_pathname** ppPath_fragball,
	SG_vhash** ppResult)
{
	sg_sync_client_http_push_handle* pMyPush = (sg_sync_client_http_push_handle*)pPush;
	char* pszUrl = NULL;
    SG_string* pstr = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pPush);
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );

	if (!ppPath_fragball || !*ppPath_fragball)
	{
		/* get the push's current status */
		SG_ERR_CHECK(  do_url(pCtx, pszUrl, "GET", NULL, pSyncClient->psz_username, pSyncClient->psz_password, &pstr, NULL, bProgressIfPossible)  );
        SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, ppResult, SG_string__sz(pstr))  );
        SG_STRING_NULLFREE(pCtx, pstr);
	}
	else
	{
		/* add the fragball to the push */
		SG_ERR_CHECK(  do_upload__string(
                    pCtx, 
                    pszUrl, 
                    "PUT",
                    *ppPath_fragball, 
                    pSyncClient->psz_username,
                    pSyncClient->psz_password,
                    &pstr)  );

        SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, ppResult, SG_string__sz(pstr))  );
		SG_PATHNAME_NULLFREE(pCtx, *ppPath_fragball);
	}

	/* fall through */
fail:
    SG_STRING_NULLFREE(pCtx, pstr);
	SG_NULLFREE(pCtx, pszUrl);
}

void sg_sync_client__http__push_commit(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle* pPush,
	SG_vhash** ppResult)
{
	char* pszUrl = NULL;
	sg_sync_client_http_push_handle* pMyPush = (sg_sync_client_http_push_handle*)pPush;
    SG_string* pstr = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pPush);
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );

    SG_ERR_CHECK(  do_url(
                pCtx, 
                pszUrl, 
                "POST",
                NULL,
                pSyncClient->psz_username,
                pSyncClient->psz_password,
                &pstr,
                NULL,
                SG_TRUE)  );

	SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, ppResult, SG_string__sz(pstr))  );

	/* fall through */
fail:
    SG_STRING_NULLFREE(pCtx, pstr);
	SG_NULLFREE(pCtx, pszUrl);
}

void sg_sync_client__http__push_end(
	SG_context * pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle** ppPush)
{
	sg_sync_client_http_push_handle* pMyPush = NULL;
	char* pszUrl = NULL;
    SG_string* pstr = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULL_PP_CHECK_RETURN(ppPush);

	pMyPush = (sg_sync_client_http_push_handle*)*ppPush;

	// Get the remote URL
	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX, pMyPush->pszPushId, NULL, &pszUrl)  );

    SG_ERR_CHECK(  do_url(
                pCtx,
                pszUrl,
                "DELETE",
                NULL,
                pSyncClient->psz_username,
                pSyncClient->psz_password,
                &pstr,
                NULL,
                SG_TRUE
                )  );


	// fall through
fail:
	// We free the push handle even on failure, because SG_push_handle is opaque outside this file:
	// this is the only way to free it.
	_NULLFREE_PUSH_HANDLE(pCtx, pMyPush);
	*ppPush = NULL;
	SG_NULLFREE(pCtx, pszUrl);
    SG_STRING_NULLFREE(pCtx, pstr);
}


void sg_sync_client__http__request_fragball(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_vhash* pvhRequest,
	SG_bool bProgressIfPossible,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName)
{
	char* pszFragballName = NULL;
	SG_pathname* pPathFragball = NULL;
	SG_string* pstrRequest = NULL;
	char* pszUrl = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

	SG_ERR_CHECK(  SG_allocN(pCtx, SG_TID_MAX_BUFFER_LENGTH, pszFragballName)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, pszFragballName, SG_TID_MAX_BUFFER_LENGTH)  );


	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX FRAGBALL_URL_SUFFIX, NULL, NULL, &pszUrl)  );


	if (pvhRequest)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrRequest)  );
		SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRequest, pstrRequest)  );
	}

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathFragball, pStagingPathname, (const char*)pszFragballName)  );
    SG_ERR_CHECK(  do_url(
                pCtx,
                pszUrl,
                "POST",
                pstrRequest ? SG_string__sz(pstrRequest) : NULL,
                pSyncClient->psz_username,
                pSyncClient->psz_password,
                NULL,
                pPathFragball,
                bProgressIfPossible
                )  );


	SG_RETURN_AND_NULL(pszFragballName, ppszFragballName);

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszFragballName);
	SG_PATHNAME_NULLFREE(pCtx, pPathFragball);
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
}

void sg_sync_client__http__pull_clone(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
    SG_vhash* pvh_clone_request,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName)
{
	SG_vhash* pvhRequest = NULL;
	char* pszFragballName = NULL;
	SG_pathname* pPathFragball = NULL;
	SG_string* pstrRequest = NULL;
	char* pszUrl = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Requesting repository from server", SG_LOG__FLAG__NONE)  );

	SG_NULLARGCHECK_RETURN(pSyncClient);

	SG_ERR_CHECK(  SG_allocN(pCtx, SG_TID_MAX_BUFFER_LENGTH, pszFragballName)  );
	SG_ERR_CHECK(  SG_tid__generate(pCtx, pszFragballName, SG_TID_MAX_BUFFER_LENGTH)  );

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX FRAGBALL_URL_SUFFIX, NULL, NULL, &pszUrl)  );


	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhRequest)  );
	SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__CLONE)  );
    if (pvh_clone_request)
    {
        SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__CLONE_REQUEST, pvh_clone_request)  );
    }
	SG_ERR_CHECK(  SG_STRING__ALLOC__RESERVE(pCtx, &pstrRequest, 50)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRequest, pstrRequest)  );

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathFragball, pStagingPathname, (const char*)pszFragballName)  );
   SG_ERR_CHECK(  do_url(pCtx, pszUrl, "POST", SG_string__sz(pstrRequest), pSyncClient->psz_username, pSyncClient->psz_password, NULL, pPathFragball, SG_TRUE)  );

	SG_RETURN_AND_NULL(pszFragballName, ppszFragballName);

	/* fall through */
fail:
	SG_log__pop_operation(pCtx);
	SG_NULLFREE(pCtx, pszFragballName);
	SG_VHASH_NULLFREE(pCtx, pvhRequest);
	SG_PATHNAME_NULLFREE(pCtx, pPathFragball);
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
}

void sg_sync_client__http__get_repo_info(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_bool bIncludeBranchInfo,
    SG_bool b_include_areas,
    SG_vhash** ppvh)
{
	char* pszUrl = NULL;
	SG_vhash* pvh = NULL;
	SG_uint32 iProtocolVersion = 0;
    SG_string* pstr = NULL;
    const char* psz_qs = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);

    if (bIncludeBranchInfo && b_include_areas)
    {
        psz_qs = "details&areas";
    }
    else if (bIncludeBranchInfo && !b_include_areas)
    {
        psz_qs = "details";
    }
    else if (!bIncludeBranchInfo && b_include_areas)
    {
        psz_qs = "areas";
    }
    else
    {
        psz_qs = NULL;
    }

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, 
		SYNC_URL_SUFFIX JSON_URL_SUFFIX, NULL, psz_qs, &pszUrl)  );

    SG_ERR_CHECK(  do_url(
                pCtx, 
                pszUrl, 
                "GET",
                NULL,
                pSyncClient->psz_username,
                pSyncClient->psz_password,
                &pstr,
                NULL,
                SG_TRUE
                )  );

	SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, SG_string__sz(pstr))  );
	if(SG_context__err_equals(pCtx, SG_ERR_JSON_WRONG_TOP_TYPE) || SG_context__err_equals(pCtx, SG_ERR_JSONPARSER_SYNTAX))
    {
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));
    }
	else
    {
		SG_ERR_CHECK_CURRENT;
    }

	SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__PROTOCOL_VERSION, &iProtocolVersion)  );
	if (iProtocolVersion != 1)
    {
		SG_ERR_THROW2(SG_ERR_UNKNOWN_SYNC_PROTOCOL_VERSION, (pCtx, "%u", iProtocolVersion));
    }

	*ppvh = pvh;
	pvh = NULL;

	/* fall through */

fail:
    SG_NULLFREE(pCtx, pszUrl);
	SG_VHASH_NULLFREE(pCtx, pvh);
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_sync_client__http__heartbeat(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_vhash** ppvh)
{
	char* pszUrl = NULL;
	SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(ppvh);

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, 
		"/heartbeat" JSON_URL_SUFFIX, NULL, NULL, &pszUrl)  );

    SG_ERR_CHECK(  do_url(
                pCtx, 
                pszUrl, 
                "GET",
                NULL,
                pSyncClient->psz_username,
                pSyncClient->psz_password,
                &pstr,
                NULL,
                SG_TRUE
                )  );

	SG_vhash__alloc__from_json__sz(pCtx, ppvh, SG_string__sz(pstr));
	if(SG_context__err_equals(pCtx, SG_ERR_JSON_WRONG_TOP_TYPE) || SG_context__err_equals(pCtx, SG_ERR_JSONPARSER_SYNTAX))
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pSyncClient->psz_remote_repo_spec));
	else
		SG_ERR_CHECK_CURRENT;

	/* fall through */

fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_VHASH_NULLFREE(pCtx, pvh);
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_sync_client__http__get_dagnode_info(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo)
{
	SG_string* pstrRequest = NULL;
	char* pszUrl = NULL;
    SG_string* pstr = NULL;

	SG_NULLARGCHECK_RETURN(pSyncClient);
	SG_NULLARGCHECK_RETURN(pvhRequest);
	SG_NULLARGCHECK_RETURN(ppInfo);

	SG_ERR_CHECK(  _get_sync_url(pCtx, pSyncClient->psz_remote_repo_spec, SYNC_URL_SUFFIX "/incoming" JSON_URL_SUFFIX, NULL, NULL, &pszUrl)  );


	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrRequest)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRequest, pstrRequest)  );

    SG_ERR_CHECK(  do_url(pCtx, pszUrl, "GET", SG_string__sz(pstrRequest), pSyncClient->psz_username, pSyncClient->psz_password, &pstr, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_history_result__from_json(pCtx, SG_string__sz(pstr), ppInfo)  );

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszUrl);
	SG_STRING_NULLFREE(pCtx, pstrRequest);
	SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_sync_client__http__push_clone__begin(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	const SG_vhash* pvhExistingRepoInfo,
	SG_sync_client_push_handle** ppPush
	)
{
    SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

void sg_sync_client__http__push_clone__upload_and_commit(
	SG_context* pCtx,
	SG_sync_client* pSyncClient,
	SG_sync_client_push_handle** ppPush,
	SG_bool bProgressIfPossible,
	SG_pathname** ppPathCloneFragball,
    SG_vhash** ppvhResult
    )
{
    SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

void sg_sync_client__http__push_clone__abort(
	SG_context* pCtx,
	SG_sync_client * pSyncClient,
	SG_sync_client_push_handle** ppPush)
{
    SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

