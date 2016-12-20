/*
Copyright 2011-2013 SourceGear, LLC

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
 * This file is based on mongoose.h, which has the following license.
 * See mongoose.h in the "reference" folder for the unmodified version.
 */

/*
 * Copyright (c) 2004-2009 Sergey Lyubka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * $Id: mongoose.h 404 2009-05-28 10:05:48Z valenok $
 */

#ifndef MONGOOSE_HEADER_INCLUDED
#define	MONGOOSE_HEADER_INCLUDED

#include "sg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mg_context;	/* Handle for the HTTP service itself	*/
struct mg_connection;	/* Handle for the individual connection	*/


#if defined(WINDOWS)
typedef unsigned __int64       uint64_t;
#define UINT64_T uint64_t
#else
#include <stdint.h>
#define UINT64_T long long unsigned int
#endif


/*
 * This structure contains full information about the HTTP request.
 * It is passed to the user-specified callback function as a parameter.
 */
struct mg_request_info {
	char	*request_method;	/* "GET", "POST", etc	*/
	char	*uri;			/* Normalized URI	*/
	char	*query_string;		/* \0 - terminated	*/
	char	*post_data;		/* Bytes read in with the headers */
	long	remote_ip;		/* Client's IP address	*/
	int	remote_port;		/* Client's port	*/
	SG_uint64 post_data_len;	/* Content-Length	*/
	SG_bool	expects_100_continue;
	SG_uint64 num_bytes_received;	/* Length read so far	*/
	int	http_version_major;
	int	http_version_minor;
	int	status_code;		/* HTTP status code	*/
	SG_vhash *headers;		/* HTTP headers		*/
};


/*
 * Start the web server.
 * This must be the first function called by the application.
 * It creates a serving thread, and returns a context structure that
 * can be used to alter the configuration, and stop the server.
 */
void mg_start(
	SG_context * pCtx,
	SG_bool bPublic,
	int port,
	struct mg_context **);


/*
 * Stop the web server.
 * Must be called last, when an application wants to stop the web server and
 * release all associated resources. This function blocks until all Mongoose
 * threads are stopped. Context pointer becomes invalid.
 */
void mg_stop(SG_context * pCtx, struct mg_context *);


/*
 * Send data to the browser.
 * Return number of bytes sent. If the number of bytes sent is less then
 * requested or equals to -1, network error occured, usually meaning the
 * remote side has closed the connection.
 */
int mg_write(struct mg_connection *, const void *buf, int len);


/*
 * Send data to the browser using printf() semantics.
 * Works exactly like mg_write(), but allows to do message formatting.
 * Note that mg_printf() uses internal buffer of size MAX_MG_PRINTF_SIZE
 * (8 Kb by default) as temporary message storage for formatting. Do not
 * print data that is bigger than that, otherwise it will be truncated.
 * Return number of bytes sent.
 */
int mg_printf(struct mg_connection *, const char *fmt, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MONGOOSE_HEADER_INCLUDED */
