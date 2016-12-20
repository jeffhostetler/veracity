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
 * This file is based on mongoose.c, which has the following license.
 * See mongoose.c in the "reference" folder for the unmodified version.
 */

/*
 * Copyright (c) 2004-2009 Sergey Lyubka
 * Portions Copyright (c) 2009 Gilbert Wellisch
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
 * $Id: mongoose.c 446 2009-07-08 21:06:56Z valenok $
 */

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS	/* Disable deprecation warning in VS2005 */
#endif /* _WIN32 */

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Ignore static analysis warnings in Windows headers. There are many. */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#if defined(_WIN32)		/* Windows specific #includes and #defines */
#define	_WIN32_WINNT	0x0400	/* To make it link in VS2005 */

#include <windows.h>

#include <process.h>
#include <direct.h>
#include <io.h>

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Stop ignoring static analysis warnings. */
#pragma warning(pop)
#endif

#define EPOCH_DIFF	0x019DB1DED53E8000 /* 116444736000000000 nsecs */
#define RATE_DIFF	10000000 /* 100 nsecs */
#define MAKEUQUAD(lo, hi)	((SG_uint64)(((SG_uint32)(lo)) | \
				((SG_uint64)((SG_uint32)(hi))) << 32))
#define	SYS2UNIX_TIME(lo, hi) \
	(time_t) ((MAKEUQUAD((lo), (hi)) - EPOCH_DIFF) / RATE_DIFF)

/*
 * Visual Studio 6 does not know __func__ or __FUNCTION__
 * The rest of MS compilers use __FUNCTION__, not C99 __func__
 */
#if defined(_MSC_VER) && _MSC_VER < 1300
#define	STRX(x)			#x
#define	STR(x)			STRX(x)
#define	__func__		"line " STR(__LINE__)
#else
#define	__func__		__FUNCTION__
#endif /* _MSC_VER */

#define	ERRNO			GetLastError()
#define	NO_SOCKLEN_T
#define	DIRSEP			'\\'
#define	IS_DIRSEP_CHAR(c)	((c) == '/' || (c) == '\\')
#define	O_NONBLOCK		0

#ifndef EWOULDBLOCK
#  define	EWOULDBLOCK		WSAEWOULDBLOCK
#endif

#define	_POSIX_
#define	UINT64_FMT		"I64"

#define	SHUT_WR			1
#define	snprintf		_snprintf
#define	vsnprintf		_vsnprintf
#define	sleep(x)		Sleep((x) * 1000)

#define	popen(x, y)		_popen(x, y)
#define	pclose(x)		_pclose(x)
#define	close(x)		_close(x)
#define	dlsym(x,y)		GetProcAddress((HINSTANCE) (x), (y))
#define	RTLD_LAZY		0
#define	fseeko(x, y, z)		fseek((x), (y), (z))
#define	write(x, y, z)		_write((x), (y), (unsigned) z)
#define	read(x, y, z)		_read((x), (y), (unsigned) z)
#define	flockfile(x)		(void) 0
#define	funlockfile(x)		(void) 0

#ifdef HAVE_STRTOUI64
#define	strtoull(x, y, z)	_strtoui64(x, y, z)
#else
#define	strtoull(x, y, z)	strtoul(x, y, z)
#endif /* HAVE_STRTOUI64 */

#if !defined(fileno)
#define	fileno(x)		_fileno(x)
#endif /* !fileno MINGW #defines fileno */

typedef HANDLE pthread_mutex_t;
typedef HANDLE pthread_cond_t;
typedef DWORD pthread_t;
typedef HANDLE pid_t;

struct timespec {
	long tv_nsec;
	long tv_sec;
};

static int pthread_mutex_lock(pthread_mutex_t *);
static int pthread_mutex_unlock(pthread_mutex_t *);

#else				/* UNIX  specific	*/
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <pwd.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#define	DIRSEP			'/'
#define	IS_DIRSEP_CHAR(c)	((c) == '/')
#define	O_BINARY		0
#define	closesocket(a)		close(a)
#define	mg_fopen(x, y)		fopen(x, y)
#define	ERRNO			errno
#define	INVALID_SOCKET		(-1)
#define UINT64_FMT		"ll"
typedef int SOCKET;

#endif /* End of Windows and UNIX specific includes */

#include "sg_mongoose.h"

#define	MAX_REQUEST_HEADERS_SIZE    8192
#define MAX_MG_PRINTF_SIZE          8192
#define	MAX_LISTENING_SOCKETS       10
#define	ARRAY_SIZE(array)           (sizeof(array) / sizeof(array[0]))
#define	DEBUG_MGS_PREFIX            "*** Mongoose debug *** "

#ifdef WINDOWS
#pragma warning( disable : 4127 ) // conditional expression is constant
#endif

#if defined(_MGDEBUG)
#define	DEBUG_TRACE(x) do {printf x; putchar('\n'); fflush(stdout);} while (0)
#else
#define DEBUG_TRACE(x)
#endif /* DEBUG */

/*
 * Darwin prior to 7.0 and Win32 do not have socklen_t
 */
#ifdef NO_SOCKLEN_T
typedef int socklen_t;
#endif /* NO_SOCKLEN_T */

#if !defined(FALSE)
enum {FALSE, TRUE};
#endif /* !FALSE */

typedef int bool_t;
typedef void * (*mg_thread_func_t)(void *);

static const char *http_500_error = "Internal Server Error";

/*
 * Month names
 */
static const char *month_names[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/*
 * Unified socket address. For IPv6 support, add IPv6 address structure
 * in the union u.
 */
struct usa {
	socklen_t len;
	union {
		struct sockaddr	sa;
		struct sockaddr_in sin;
	} u;
};

/*
 * Specifies a string (chunk of memory).
 * Used to traverse comma separated lists of options.
 */
struct vec {
	const char	*ptr;
	size_t		len;
};

/*
 * Structure used by mg_stat() function. Uses 64 bit file length.
 */
struct mgstat {
	bool_t		is_directory;	/* Directory marker		*/
	SG_uint64	size;		/* File size			*/
	time_t		mtime;		/* Modification time		*/
};

/*
 * Structure used to describe listening socket, or socket which was
 * accept()-ed by the master thread and queued for future handling
 * by the worker thread.
 */
struct socket {
	SOCKET		sock;		/* Listening socket		*/
	struct usa	lsa;		/* Local socket address		*/
	struct usa	rsa;		/* Remote socket address	*/
};

/*
 * Mongoose context
 */
struct mg_context {
	int		stop_flag;	/* Should we stop event loop	*/

	FILE		*access_log;	/* Opened access log		*/

	struct socket	listener;
	int		num_threads;	/* Number of threads		*/
	int		num_idle;	/* Number of idle threads	*/
	pthread_mutex_t	thr_mutex;	/* Protects (max|num)_threads	*/
	pthread_cond_t	thr_cond;
	pthread_mutex_t	bind_mutex;	/* Protects bind operations	*/

	struct socket	queue[20];	/* Accepted sockets		*/
	int		sq_head;	/* Head of the socket queue	*/
	int		sq_tail;	/* Tail of the socket queue	*/
	pthread_cond_t	empty_cond;	/* Socket queue empty condvar	*/
	pthread_cond_t	full_cond;	/* Socket queue full condvar	*/

	SG_pathname	*static_root;	/* Location of /ui on disk. 	*/
};
#define MAX_THREADS 100
#define IDLE_TIME    10

/*
 * Client connection.
 */
struct mg_connection {
	struct mg_request_info	request_info;
	struct mg_context *ctx;		/* Mongoose context we belong to*/
	struct socket	client;		/* Connected client		*/
	time_t		birth_time;	/* Time connection was accepted	*/

	UINT64_T	num_bytes_sent;	/* Total bytes sent to client	*/

	SG_context* pCtx;
};

/*
 * Print error message to the opened error log stream.
 */
static void
cry(SG_context *pCtx, const char *fmt, ...)
{
	va_list	ap;
	if (pCtx!=NULL) {
		SG_context__push_level(pCtx);
		
		va_start(ap, fmt);
		SG_log__report_message__v(pCtx, SG_LOG__MESSAGE__ERROR, fmt, ap);
		va_end(ap);
		
		SG_context__pop_level(pCtx);
	}
}

#ifdef _WIN32
static void
mg_strlcpy(register char *dst, register const char *src, size_t n)
{
	for (; *src != '\0' && n > 1; n--)
		*dst++ = *src++;
	*dst = '\0';
}
#endif

static int
lowercase(const char *s)
{
	return (tolower(* (unsigned char *) s));
}

static int
mg_strcasecmp(const char *s1, const char *s2)
{
	int	diff;

	do {
		diff = lowercase(s1++) - lowercase(s2++);
	} while (diff == 0 && s1[-1] != '\0');

	return (diff);
}

/*
 * Like snprintf(), but never returns negative value, or the value
 * that is larger than a supplied buffer.
 * Thanks to Adam Zeldis to pointing snprintf()-caused vulnerability
 * in his audit report.
 */
static int
mg_vsnprintf(struct mg_connection *conn,
		char *buf, size_t buflen, const char *fmt, va_list ap)
{
	int	n;

	if (buflen == 0)
		return (0);

	n = vsnprintf(buf, buflen, fmt, ap);

	if (n < 0) {
		cry(conn->pCtx, "vsnprintf error");
		n = 0;
	} else if (n >= (int) buflen) {
		cry(conn->pCtx, "truncating vsnprintf buffer: [%.*s]",
		    n > 200 ? 200 : n, buf);
		n = (int) buflen - 1;
	}
	buf[n] = '\0';

	return (n);
}

static int
mg_snprintf(struct mg_connection *conn,
		char *buf, size_t buflen, const char *fmt, ...)
{
	va_list	ap;
	int	n;

	va_start(ap, fmt);
	n = mg_vsnprintf(conn, buf, buflen, fmt, ap);
	va_end(ap);

	return (n);
}

/*
 * Skip the characters until one of the delimiters characters found.
 * 0-terminate resulting word. Skip the rest of the delimiters if any.
 * Advance pointer to buffer to the next word. Return found 0-terminated word.
 */
static char *
skip(char **buf, const char *delimiters)
{
	char	*p, *begin_word, *end_word, *end_delimiters;

	begin_word = *buf;
	end_word = begin_word + strcspn(begin_word, delimiters);
	end_delimiters = end_word + strspn(end_word, delimiters);

	for (p = end_word; p < end_delimiters; p++)
		*p = '\0';

	*buf = end_delimiters;

	return (begin_word);
}

/*
 * Return HTTP header value, or NULL if not found.
 */
static const char *
get_header(SG_context *pCtx, const struct mg_request_info *ri, const char *name)
{
	const char * val = NULL;
	SG_ERR_IGNORE(  SG_vhash__get__sz(pCtx, ri->headers, name, &val)  );
	return val;
}

const char *
mg_get_header(const struct mg_connection *conn, const char *name)
{
	return (get_header(conn->pCtx, &conn->request_info, name));
}

/*
 * Send error message back to the client.
 */
static void
send_error(struct mg_connection *conn, int status, const char *reason,
		const char *fmt, ...)
{
	char		buf[BUFSIZ];
	va_list		ap;
	int		len;

	conn->request_info.status_code = status;

	{
		buf[0] = '\0';
		len = 0;

		/* Errors 1xx, 204 and 304 MUST NOT send a body */
		if (status > 199 && status != 204 && status != 304) {
			len = mg_snprintf(conn, buf, sizeof(buf),
			    "Error %d: %s\n", status, reason);
			cry(conn->pCtx, "%s", buf);

			va_start(ap, fmt);
			len += mg_vsnprintf(conn, buf + len, sizeof(buf) - len,
			    fmt, ap);
			va_end(ap);
			conn->num_bytes_sent = len;
		}

		(void) mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n%s", status, reason, len, buf);
	}
}

#ifdef _WIN32
static int
pthread_mutex_init(pthread_mutex_t *mutex, void *unused)
{
	unused = NULL;
	*mutex = CreateMutex(NULL, FALSE, NULL);
	return (*mutex == NULL ? -1 : 0);
}

static int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	return (CloseHandle(*mutex) == 0 ? -1 : 0);
}

static int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return (WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0? 0 : -1);
}

static int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return (ReleaseMutex(*mutex) == 0 ? -1 : 0);
}

static int
pthread_cond_init(pthread_cond_t *cv, const void *unused)
{
	unused = NULL;
	*cv = CreateEvent(NULL, FALSE, FALSE, NULL);
	return (*cv == NULL ? -1 : 0);
}

static int
pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mutex,
	const struct timespec *ts)
{
	DWORD	status;
	DWORD	msec = INFINITE;
	time_t	now;
	
	if (ts != NULL) {
		now = time(NULL);
		msec = (DWORD)(1000 * (now > ts->tv_sec ? 0 : ts->tv_sec - now));
	}

	(void) ReleaseMutex(*mutex);
	status = WaitForSingleObject(*cv, msec);
	(void) WaitForSingleObject(*mutex, INFINITE);
	
	return (status == WAIT_OBJECT_0 ? 0 : -1);
}

static int
pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mutex)
{
	return (pthread_cond_timedwait(cv, mutex, NULL));
}

static int
pthread_cond_signal(pthread_cond_t *cv)
{
	return (SetEvent(*cv) == 0 ? -1 : 0);
}

static int
pthread_cond_destroy(pthread_cond_t *cv)
{
	return (CloseHandle(*cv) == 0 ? -1 : 0);
}

static pthread_t
pthread_self(void)
{
	return (GetCurrentThreadId());
}

/*
 * Change all slashes to backslashes. It is Windows.
 */
static void
fix_directory_separators(char *path)
{
	int	i;

	for (i = 0; path[i] != '\0'; i++) {
		if (path[i] == '/')
			path[i] = '\\';
		/* i > 0 check is to preserve UNC paths, \\server\file.txt */
		if (path[i] == '\\' && i > 0)
			while (path[i + 1] == '\\' || path[i + 1] == '/')
				(void) memmove(path + i + 1,
				    path + i + 2, strlen(path + i + 1));
	}
}

/*
 * Encode 'path' which is assumed UTF-8 string, into UNICODE string.
 * wbuf and wbuf_len is a target buffer and its length.
 */
static void
to_unicode(const char *path, wchar_t *wbuf, size_t wbuf_len)
{
	char	buf[FILENAME_MAX], *p;

	mg_strlcpy(buf, path, sizeof(buf));
	fix_directory_separators(buf);

	/* Point p to the end of the file name */
	p = buf + strlen(buf) - 1;

	/* Trim trailing backslash character */
	while (p > buf && *p == '\\' && p[-1] != ':')
		*p-- = '\0';

	/*
	 * Protect from CGI code disclosure.
	 * This is very nasty hole. Windows happily opens files with
	 * some garbage in the end of file name. So fopen("a.cgi    ", "r")
	 * actually opens "a.cgi", and does not return an error!
	 */
	if (*p == 0x20 || *p == 0x2e || *p == 0x2b || (*p & ~0x7f)) {
		(void) fprintf(stderr, "Rejecting suspicious path: [%s]", buf);
		buf[0] = '\0';
	}

	(void) MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, (int) wbuf_len);
}


static FILE *
mg_fopen(const char *path, const char *mode)
{
	wchar_t	wbuf[FILENAME_MAX], wmode[20];

	to_unicode(path, wbuf, ARRAY_SIZE(wbuf));
	MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, ARRAY_SIZE(wmode));

	return (_wfopen(wbuf, wmode));
}

static int
mg_stat(const char *path, struct mgstat *stp)
{
	int				ok = -1; /* Error */
	wchar_t				wbuf[FILENAME_MAX];
	WIN32_FILE_ATTRIBUTE_DATA	info;

	to_unicode(path, wbuf, ARRAY_SIZE(wbuf));

	if (GetFileAttributesExW(wbuf, GetFileExInfoStandard, &info) != 0) {
		stp->size = MAKEUQUAD(info.nFileSizeLow, info.nFileSizeHigh);
		stp->mtime = SYS2UNIX_TIME(info.ftLastWriteTime.dwLowDateTime,
		    info.ftLastWriteTime.dwHighDateTime);
		stp->is_directory =
		    info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		ok = 0;  /* Success */
	}

	return (ok);
}

#define	set_close_on_exec(fd)	/* No FD_CLOEXEC on Windows */

static int
start_thread(SG_context * pCtx, mg_thread_func_t func, void *param)
{
	HANDLE	hThread;

	SG_UNUSED(pCtx);
	
	hThread = CreateThread(NULL, 0,
	    (LPTHREAD_START_ROUTINE) func, param, 0, NULL);

	if (hThread != NULL)
		(void) CloseHandle(hThread);

	return (hThread == NULL ? -1 : 0);
}

static int
set_non_blocking_mode(struct mg_connection *conn, SOCKET sock)
{
	unsigned long	on = 1;

	conn = NULL; /* unused */
	return (ioctlsocket(sock, FIONBIO, &on));
}

#else

static int
mg_stat(const char *path, struct mgstat *stp)
{
	struct stat	st;
	int		ok;

	if (stat(path, &st) == 0) {
		ok = 0;
		stp->size = st.st_size;
		stp->mtime = st.st_mtime;
		stp->is_directory = S_ISDIR(st.st_mode);
	} else {
		ok = -1;
	}

	return (ok);
}

static void
set_close_on_exec(int fd)
{
	(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
}


static int
start_thread(SG_context * pCtx, mg_thread_func_t func, void *param)
// pCtx: input parameter only (used for logging)
{
	pthread_t	thread_id;
	pthread_attr_t	attr;
	int		retval;

	(void) pthread_attr_init(&attr);
	(void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if ((retval = pthread_create(&thread_id, &attr, func, param)) != 0)
		cry(pCtx, "%s: %s", __func__, strerror(retval));

	return (retval);
}

static int
set_non_blocking_mode(struct mg_connection *conn, SOCKET sock)
{
	int	flags, ok = -1;

	if ((flags = fcntl(sock, F_GETFL, 0)) == -1) {
		cry(conn->pCtx, "%s: fcntl(F_GETFL): %d", __func__, ERRNO);
	} else if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != 0) {
		cry(conn->pCtx, "%s: fcntl(F_SETFL): %d", __func__, ERRNO);
	} else {
		ok = 0;	/* Success */
	}

	return (ok);
}
#endif /* _WIN32 */

/*
 * Write data to the IO channel - opened file descriptor or socket.
 * Return number of bytes written.
 */
static SG_uint64
push(FILE *fp, SOCKET sock, const char *buf, SG_uint64 len)
{
	SG_uint64	sent;
	int		n, k;

	sent = 0;
	while (sent < len) {

		/* How many bytes we send in this iteration */
		k = len - sent > INT_MAX ? INT_MAX : (int) (len - sent);

		if (fp != NULL) {
			n = (int)fwrite(buf + sent, 1, k, fp);
			if (ferror(fp))
				n = -1;
		} else {
			n = send(sock, buf + sent, k, 0);
		}

		if (n < 0)
			break;

		sent += n;
	}

	return (sent);
}

/*
 * Read from IO channel - opened file descriptor or socket.
 * Return number of bytes read.
 */
static int
pull(FILE *fp, SOCKET sock, char *buf, int len)
{
	int	nread;

	if (fp != NULL) {
		nread = (int)fread(buf, 1, (size_t) len, fp);
		if (ferror(fp))
			nread = -1;
	} else {
		nread = recv(sock, buf, (size_t) len, 0);
	}

	return (nread);
}

int
mg_write(struct mg_connection *conn, const void *buf, int len)
{
	assert(len >= 0);
	return ((int) push(NULL, conn->client.sock,
				(const char *) buf, (SG_uint64) len));
}

int
mg_printf(struct mg_connection *conn, const char *fmt, ...)
{
	char	buf[MAX_MG_PRINTF_SIZE];
	int	len;
	va_list	ap;

	va_start(ap, fmt);
	len = mg_vsnprintf(conn, buf, sizeof(buf), fmt, ap);
	va_end(ap);

	return (mg_write(conn, buf, len));
}

/*
 * Return content length of the request, or UNKNOWN_CONTENT_LENGTH constant if
 * Content-Length header is not set.
 */
static void
get_content_length(const struct mg_connection *conn, SG_uint64 * p_content_length, SG_bool * p_content_length_specified)
{
	const char *cl = mg_get_header(conn, "Content-Length");
	if (cl == NULL)
		*p_content_length_specified = SG_FALSE;
	else
	{
		*p_content_length_specified = SG_TRUE;
		*p_content_length = strtoull(cl, NULL, 10);
	}
}

/*
 * URL-decode input buffer into destination buffer.
 * 0-terminate the destination buffer. Return the length of decoded data.
 * form-url-encoded data differs from URI encoding in a way that it
 * uses '+' as character for space, see RFC 1866 section 8.2.1
 * http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt
 */
static size_t
url_decode(const char *src, size_t src_len, char *dst, size_t dst_len,
		bool_t is_form_url_encoded)
{
	size_t	i, j;
	int	a, b;
#define	HEXTOI(x)	(isdigit(x) ? x - '0' : x - 'W')

	for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
		if (src[i] == '%' &&
		    isxdigit(* (unsigned char *) (src + i + 1)) &&
		    isxdigit(* (unsigned char *) (src + i + 2))) {
			a = tolower(* (unsigned char *) (src + i + 1));
			b = tolower(* (unsigned char *) (src + i + 2));
			dst[j] = ((HEXTOI(a) << 4) | HEXTOI(b)) & 0xff;
			i += 2;
		} else if (is_form_url_encoded && src[i] == '+') {
			dst[j] = ' ';
		} else {
			dst[j] = src[i];
		}
	}

	dst[j] = '\0';	/* Null-terminate the destination */

	return (j);
}

/*
 * Transform URI to the file name.
 */
static SG_bool
convert_uri_to_file_name(struct mg_connection *conn, const char *uri,
		char *buf, size_t buf_len)
{
	if (!(uri[0]=='/' && uri[1]=='u' && uri[2]=='i' && uri[3]=='/'))
		return SG_FALSE;

	mg_snprintf(conn, buf, buf_len, "%s%s", SG_pathname__sz(conn->ctx->static_root), uri+3);

#ifdef _WIN32
	fix_directory_separators(buf);
#endif /* _WIN32 */

	return SG_TRUE;
}

/*
 * Setup listening socket on given port, return socket.
 */
static SOCKET
mg_open_listening_port(SG_context *pCtx, struct mg_context *ctx, int port, SG_bool public)
// pCtx: input parameter only (used for logging)
{
	struct usa *usa = &ctx->listener.lsa;
	SOCKET		sock;

	/* MacOS needs that. If we do not zero it, bind() will fail. */
	(void) memset(usa, 0, sizeof(*usa));

	if (!public) {
		/* IP address to bind to is 127.0.0.1 */
		usa->u.sin.sin_addr.s_addr =
		    htonl(0x7F000001);
	} else {
		/* Only port number is specified. Bind to all addresses */
		usa->u.sin.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	usa->len			= sizeof(usa->u.sin);
	usa->u.sin.sin_family		= AF_INET;
	usa->u.sin.sin_port		= htons((SG_uint16) port);

	if ((sock = socket(PF_INET, SOCK_STREAM, 6)) != INVALID_SOCKET &&
	    bind(sock, &usa->u.sa, usa->len) == 0 &&
	    listen(sock, 128) == 0) {
		/* Success */
		set_close_on_exec(sock);
	} else {
		/* Error */
		cry(pCtx, "%s(%d): %s", __func__, port, strerror(ERRNO));
		if (sock != INVALID_SOCKET)
			(void) closesocket(sock);
		sock = INVALID_SOCKET;
	}

	return (sock);
}

/*
 * Check whether full request is buffered. Return:
 *   -1         if request is malformed
 *    0         if request is not yet fully buffered
 *   >0         actual request length, including last \r\n\r\n
 */
static int
get_request_len(const char *buf, size_t buflen)
{
	const char	*s, *e;
	int		len = 0;

	for (s = buf, e = s + buflen - 1; len <= 0 && s < e; s++)
		/* Control characters are not allowed but >=128 is. */
		if (!isprint(* (unsigned char *) s) && *s != '\r' &&
		    *s != '\n' && * (unsigned char *) s < 128)
			len = -1;
		else if (s[0] == '\n' && s[1] == '\n')
			len = (int) (s - buf) + 2;
		else if (s[0] == '\n' && &s[1] < e &&
		    s[1] == '\r' && s[2] == '\n')
			len = (int) (s - buf) + 3;

	return (len);
}

/*
 * Convert month to the month number. Return -1 on error, or month number
 */
static int
montoi(const char *s)
{
	size_t	i;

	for (i = 0; i < sizeof(month_names) / sizeof(month_names[0]); i++)
		if (!strcmp(s, month_names[i]))
			return ((int) i);

	return (-1);
}

/*
 * Parse date-time string, and return the corresponding time_t value
 */
static time_t
date_to_epoch(const char *s)
{
	//time_t		current_time;
	struct tm	tm; //, *tmp;
	char		mon[32];
	int		sec, min, hour, mday, month, year;

	(void) memset(&tm, 0, sizeof(tm));
	sec = min = hour = mday = month = year = 0;

	if (((sscanf(s, "%d/%3s/%d %d:%d:%d",
	    &mday, mon, &year, &hour, &min, &sec) == 6) ||
	    (sscanf(s, "%d %3s %d %d:%d:%d",
	    &mday, mon, &year, &hour, &min, &sec) == 6) ||
	    (sscanf(s, "%*3s, %d %3s %d %d:%d:%d",
	    &mday, mon, &year, &hour, &min, &sec) == 6) ||
	    (sscanf(s, "%d-%3s-%d %d:%d:%d",
	    &mday, mon, &year, &hour, &min, &sec) == 6)) &&
	    (month = montoi(mon)) != -1) {
		tm.tm_mday	= mday;
		tm.tm_mon	= month;
		tm.tm_year	= year;
		tm.tm_hour	= hour;
		tm.tm_min	= min;
		tm.tm_sec	= sec;
	}

	if (tm.tm_year > 1900)
		tm.tm_year -= 1900;
	else if (tm.tm_year < 70)
		tm.tm_year += 100;

	tm.tm_isdst = -1;

	/* Set Daylight Saving Time field */
	//current_time = time(NULL);
	//tmp = localtime(&current_time);
	//tm.tm_isdst = tmp->tm_isdst;

	return (mktime(&tm));
}

/*
 * Protect against directory disclosure attack by removing '..',
 * excessive '/' and '\' characters
 */
static void
remove_double_dots_and_double_slashes(char *s)
{
	char	*p = s;

	while (*s != '\0') {
		*p++ = *s++;
		if (s[-1] == '/' || s[-1] == '\\') {
			/* Skip all following slashes and backslashes */
			while (*s == '/' || *s == '\\')
				s++;

			/* Skip all double-dots */
			while (*s == '.' && s[1] == '.')
				s += 2;
		}
	}
	*p = '\0';
}

/*
 * Built-in mime types
 */
static const struct {
	const char	*extension;
	size_t		ext_len;
	const char	*mime_type;
	size_t		mime_type_len;
} mime_types[] = {
	{".html",	5,	"text/html",			9},
	{".htm",	4,	"text/html",			9},
	{".shtm",	5,	"text/html",			9},
	{".shtml",	6,	"text/html",			9},
	{".css",	4,	"text/css;charset=UTF-8",	22},
	{".js",		3,	"application/javascript;charset=UTF-8",	36},
	{".ico",	4,	"image/x-icon",			12},
	{".gif",	4,	"image/gif",			9},
	{".jpg",	4,	"image/jpeg",			10},
	{".jpeg",	5,	"image/jpeg",			10},
	{".png",	4,	"image/png",			9},
	{".svg",	4,	"image/svg+xml",		13},
	{".torrent",	8,	"application/x-bittorrent",	24},
	{".wav",	4,	"audio/x-wav",			11},
	{".mp3",	4,	"audio/x-mp3",			11},
	{".mid",	4,	"audio/mid",			9},
	{".m3u",	4,	"audio/x-mpegurl",		15},
	{".ram",	4,	"audio/x-pn-realaudio",		20},
	{".ra",		3,	"audio/x-pn-realaudio",		20},
	{".doc",	4,	"application/msword",		19},
	{".exe",	4,	"application/octet-stream",	24},
	{".zip",	4,	"application/x-zip-compressed",	28},
	{".xls",	4,	"application/excel",		17},
	{".tgz",	4,	"application/x-tar-gz",		20},
	{".tar",	4,	"application/x-tar",		17},
	{".gz",		3,	"application/x-gunzip",		20},
	{".arj",	4,	"application/x-arj-compressed",	28},
	{".rar",	4,	"application/x-arj-compressed",	28},
	{".rtf",	4,	"application/rtf",		15},
	{".pdf",	4,	"application/pdf",		15},
	{".swf",	4,	"application/x-shockwave-flash",29},
	{".mpg",	4,	"video/mpeg",			10},
	{".mpeg",	5,	"video/mpeg",			10},
	{".asf",	4,	"video/x-ms-asf",		14},
	{".avi",	4,	"video/x-msvideo",		15},
	{".bmp",	4,	"image/bmp",			9},
	{NULL,		0,	NULL,				0}
};

/*
 * Look at the "path" extension and figure what mime type it has.
 * Store mime type in the vector.
 */
static void
get_mime_type(const char *path, struct vec *vec)
{
	const char	*ext;
	size_t		i, path_len;

	path_len = strlen(path);

	/* Now scan built-in mime types */
	for (i = 0; mime_types[i].extension != NULL; i++) {
		ext = path + (path_len - mime_types[i].ext_len);
		if (path_len > mime_types[i].ext_len &&
		    mg_strcasecmp(ext, mime_types[i].extension) == 0) {
			vec->ptr = mime_types[i].mime_type;
			vec->len = mime_types[i].mime_type_len;
			return;
		}
	}

	/* Nothing found. Fall back to text/plain */
	vec->ptr = "text/plain";
	vec->len = 10;
}

/*
 * Send len bytes from the opened file to the client.
 */
static void
send_opened_file_stream(struct mg_connection *conn, FILE *fp, SG_uint64 len)
{
	char	buf[BUFSIZ];
	int	to_read, num_read, num_written;

	while (len > 0) {
		/* Calculate how much to read from the file in the buffer */
		to_read = sizeof(buf);
		if ((SG_uint64) to_read > len)
			to_read = (int) len;

		/* Read from file, exit the loop on error */
		if ((num_read = (int)fread(buf, 1, to_read, fp)) == 0)
			break;

		/* Send read bytes to the client, exit the loop on error */
		if ((num_written = mg_write(conn, buf, num_read)) != num_read)
			break;

		/* Both read and were successful, adjust counters */
		conn->num_bytes_sent += num_written;
		len -= num_written;
	}
}

/*
 * Send regular file contents.
 */

static void
send_file(struct mg_connection *conn, const char *path, struct mgstat *stp)
{
	char		date[64], lm[64], etag[64], range[64];
	const char	*fmt = "%a, %d %b %Y %H:%M:%S %Z", *msg = "OK", *hdr;
	time_t		curtime = time(NULL);
	UINT64_T	cl, r1, r2;
	struct vec	mime_vec;
	FILE		*fp;
	int		n;

	get_mime_type(path, &mime_vec);
	cl = stp->size;
	conn->request_info.status_code = 200;
	range[0] = '\0';

	if ((fp = mg_fopen(path, "rb")) == NULL) {
		send_error(conn, 500, http_500_error,
		    "fopen(%s): %s", path, strerror(ERRNO));
		return;
	}
	set_close_on_exec(fileno(fp));

	/* If Range: header specified, act accordingly */
	r1 = r2 = 0;
	hdr = mg_get_header(conn, "Range");
	if (hdr != NULL && (n = sscanf(hdr,
	    "bytes=%" UINT64_FMT "u-%" UINT64_FMT "u", &r1, &r2)) > 0) {
		conn->request_info.status_code = 206;
		(void) fseeko(fp, (off_t) r1, SEEK_SET);
		cl = n == 2 ? r2 - r1 + 1: cl - r1;
		(void) mg_snprintf(conn, range, sizeof(range),
		    "Content-Range: bytes "
		    "%" UINT64_FMT "u-%"
		    UINT64_FMT "u/%" UINT64_FMT "u\r\n",
		    r1, r1 + cl - 1, stp->size);
		msg = "Partial Content";
	}

	/* Prepare Etag, Date, Last-Modified headers */
	(void) strftime(date, sizeof(date), fmt, localtime(&curtime));
	(void) strftime(lm, sizeof(lm), fmt, localtime(&stp->mtime));
	(void) mg_snprintf(conn, etag, sizeof(etag), "%lx.%lx",
	    (unsigned long) stp->mtime, (unsigned long) stp->size);

	(void) mg_printf(conn,
	    "HTTP/1.1 %d %s\r\n"
	    "Date: %s\r\n"
	    "Last-Modified: %s\r\n"
	    "Etag: \"%s\"\r\n"
	    "Content-Type: %.*s\r\n"
	    "Content-Length: %" UINT64_FMT "u\r\n"
	    "Connection: close\r\n"
	    "Accept-Ranges: bytes\r\n"
	    "%s\r\n",
	    conn->request_info.status_code, msg, date, lm, etag,
	    mime_vec.len, mime_vec.ptr, cl, range);

	if (strcmp(conn->request_info.request_method, "HEAD") != 0)
		send_opened_file_stream(conn, fp, cl);
	(void) fclose(fp);
}

/*
 * Parse HTTP headers from the given buffer, advance buffer to the point
 * where parsing stopped.
 */
static void
parse_http_headers(SG_context *pCtx, char **buf, struct mg_request_info *ri)
{
	const char *name = NULL, *value = NULL;

	SG_ERR_IGNORE(  SG_VHASH__ALLOC(pCtx, &ri->headers)  );
	while (1) {
		name = skip(buf, ": ");
		value = skip(buf, "\r\n");
		if (name[0] == '\0')
			break;
		SG_ERR_IGNORE(  SG_vhash__add__string__sz(pCtx, ri->headers, name, value)  );
	}
}

static bool_t
is_known_http_method(const char *method)
{
	return (!strcmp(method, "GET") ||
	    !strcmp(method, "POST") ||
	    !strcmp(method, "HEAD") ||
	    !strcmp(method, "PUT") ||
	    !strcmp(method, "DELETE"));
}

/*
 * Parse HTTP request, fill in mg_request_info structure.
 */
static bool_t
parse_http_request(SG_context *pCtx, char *buf, struct mg_request_info *ri, const struct usa *usa)
{
	char	*http_version;
	int	n, success_code = FALSE;

	ri->request_method = skip(&buf, " ");
	ri->uri = skip(&buf, " ");
	http_version = skip(&buf, "\r\n");

	if (is_known_http_method(ri->request_method) &&
	    ri->uri[0] == '/' &&
	    sscanf(http_version, "HTTP/%d.%d%n",
	    &ri->http_version_major, &ri->http_version_minor, &n) == 2 &&
	    http_version[n] == '\0') {
		parse_http_headers(pCtx, &buf, ri);
		ri->remote_port = ntohs(usa->u.sin.sin_port);
		(void) memcpy(&ri->remote_ip, &usa->u.sin.sin_addr.s_addr, 4);
		ri->remote_ip = ntohl(ri->remote_ip);
		success_code = TRUE;
	}

	return (success_code);
}

/*
 * Keep reading the input (either opened file descriptor fd or socket sock)
 * into buffer buf, until \r\n\r\n appears in the
 * buffer (which marks the end of HTTP request). Buffer buf may already
 * have some data. The length of the data is stored in nread.
 * Upon every read operation, increase nread by the number of bytes read.
 */
static int
read_request(FILE *fp, SOCKET sock, char *buf, int bufsiz, int *nread)
{
	int	n, request_len;

	request_len = 0;
	while (*nread < bufsiz && request_len == 0) {
		n = pull(fp, sock, buf + *nread, bufsiz - *nread);
		if (n <= 0) {
			break;
		} else {
			*nread += n;
			request_len = get_request_len(buf, (size_t) *nread);
		}
	}

	return (request_len);
}

/*
 * Return True if we should reply 304 Not Modified.
 */
static bool_t
is_not_modified(const struct mg_connection *conn, const struct mgstat *stp)
{
	const char *ims = mg_get_header(conn, "If-Modified-Since");
	time_t calcTime = 0;

	if (ims != NULL)
		calcTime = date_to_epoch(ims);

	return (ims != NULL && stp->mtime <= calcTime);
}

static void
mg_dispatch(struct mg_connection *conn)
{
	SG_context *pCtx = conn->pCtx;
	struct mg_request_info *ri = &conn->request_info;
	SG_uridispatchcontext * pDispatchContext = NULL;

	const char * szResponseStatusCode = "500 Internal Server Error";
	SG_uint64 responseContentLength = 0;
	SG_vhash * pResponseHeaders = NULL;

	SG_httprequestprofiler__start_request();

	pDispatchContext = SG_uridispatch__begin_request(conn->pCtx,
		ri->request_method,
		ri->uri,
		ri->query_string,
		"http:",
		&ri->headers);

	// Receive the request's message body (if applicable).
	while (!SG_uridispatch__get_response_headers(pDispatchContext, &szResponseStatusCode, &responseContentLength, &pResponseHeaders))
	{
		if (ri->expects_100_continue) {
			(void)mg_printf(conn, "HTTP/1.1 100 Continue\r\n\r\n");
			ri->expects_100_continue = SG_FALSE;
		}

		if (ri->post_data) {
			SG_uridispatch__chunk_request_body(pDispatchContext, (SG_byte*)ri->post_data, (SG_uint32)ri->num_bytes_received);
			ri->post_data = NULL;
		} else {
			SG_byte buf[4*1024];
			int nread;

			SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__TRANSFER);
			nread = pull(NULL, conn->client.sock, (char*)buf, SG_MIN((int)(ri->post_data_len - ri->num_bytes_received), (int)sizeof(buf)));
			SG_httprequestprofiler__stop();

			if (nread<=0) {
				send_error(conn, 577, http_500_error, "%s", "Error handling body data");
				SG_uridispatch__abort(&pDispatchContext);
				SG_ERR_IGNORE(  SG_log__report_warning(pCtx, "Failed to fetch data from the client. Perhaps they closed the connection.")  );
				SG_httprequestprofiler__stop_request();
				return;
			}

			ri->num_bytes_received += nread;
			SG_uridispatch__chunk_request_body(pDispatchContext, buf, nread);
		}
	}

	// Send the response's headers.
	{
		SG_int_to_string_buffer tmp;

		SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__TRANSFER);

		mg_printf(conn, "HTTP/1.1 %s\r\n", szResponseStatusCode);
		mg_printf(conn, "Content-Length: %s\r\n", SG_uint64_to_sz(responseContentLength, tmp));

		if (pResponseHeaders!=NULL)
		{
			SG_uint32 numHeaders = 0;
			SG_uint32 j;

			SG_vhash__count(pCtx, pResponseHeaders, &numHeaders);
			SG_ASSERT(!SG_context__has_err(pCtx)); // The only errors SG_vhash__count throws are INVALIDARG errors on NULL arguments.

			for(j = 0; j<numHeaders; ++j)
			{
				const char * szName = NULL;
				const SG_variant * pValue = NULL;

				SG_vhash__get_nth_pair(pCtx, pResponseHeaders, j, &szName, &pValue);
				SG_ASSERT(!SG_context__has_err(pCtx)); // pResponseHeaders is non-NULL and j is in the valid range so we should be fine

				// Some headers are allowed to be set multiple times (namely, Set-Cookie)
				if (pValue->type==SG_VARIANT_TYPE_VARRAY)
				{
					SG_varray * pValues = pValue->v.val_varray;
					SG_uint32 numValues = 0;
					SG_uint32 k;

					SG_varray__count(pCtx, pValues, &numValues);
					SG_ASSERT(!SG_context__has_err(pCtx)); // The only errors SG_vhash__count throws are INVALIDARG errors on NULL arguments.

					for (k = 0; k < numValues; k++)
					{
						const SG_variant * pNestedValue = NULL;
						SG_varray__get__variant(pCtx, pValues, k, &pNestedValue);
						SG_ASSERT(!SG_context__has_err(pCtx)); // k is in the valid range so we should be fine

						// Only strings are supported in the header hash for varrays
						if (pNestedValue->type==SG_VARIANT_TYPE_SZ)
							mg_printf(conn, "%s: %s\r\n", szName, pNestedValue->v.val_sz);
						else
							SG_ERR_IGNORE(  SG_log__report_warning(pCtx, "Skipped a value for HTTP header \"%s\". Unsupported variant type.", szName)  );
					}
				}
				else if (pValue->type == SG_VARIANT_TYPE_SZ)
					mg_printf(conn, "%s: %s\r\n", szName, pValue->v.val_sz);
				else if (pValue->type == SG_VARIANT_TYPE_INT64)
					mg_printf(conn, "%s: %s\r\n", szName, SG_uint64_to_sz(pValue->v.val_int64, tmp));
				else
					SG_ERR_IGNORE(  SG_log__report_warning(pCtx, "Skipped HTTP header \"%s\". Unsupported variant type.", szName)  );
			}
			SG_VHASH_NULLFREE(pCtx, pResponseHeaders);

		}
		else
		{
			if (responseContentLength>0)
				mg_printf(conn, "Content-Type: text/plain;charset=UTF-8\r\n");
		}

		mg_printf(conn, "\r\n");
		
		SG_httprequestprofiler__stop();
	}

	// Send the response's message body (if applicable).
	{
		const SG_byte * pResponseBuffer = NULL;
		SG_uint32 responseBufferLength = 0;

		while(pDispatchContext!=NULL)
		{
			SG_uridispatch__chunk_response_body(
				&pDispatchContext,
				&pResponseBuffer,
				&responseBufferLength);
			if (responseBufferLength>0) {
				SG_httprequestprofiler__start(SG_HTTPREQUESTPROFILER_CATEGORY__TRANSFER);
				mg_write(conn, pResponseBuffer, responseBufferLength);
				SG_httprequestprofiler__stop();
			}
		}
	}
	
	SG_httprequestprofiler__stop_request();
}

/*
 * This is the heart of the Mongoose's logic.
 * This function is called when the request is read, parsed and validated,
 * and Mongoose must decide what action to take: serve a file, or
 * a directory, or call embedded function, etcetera.
 */
static void
analyze_request(struct mg_connection *conn)
{
	struct mg_request_info *ri = &conn->request_info;
	char			path[FILENAME_MAX], *uri = ri->uri;
	struct mgstat		st;
	SG_bool			is_static = SG_FALSE;

	if ((conn->request_info.query_string = strchr(uri, '?')) != NULL)
		* conn->request_info.query_string++ = '\0';

	(void) url_decode(uri, (int) strlen(uri), uri, strlen(uri) + 1, FALSE);
	remove_double_dots_and_double_slashes(uri);
	is_static = convert_uri_to_file_name(conn, uri, path, sizeof(path));

	if (!is_static) {
		SG_bool content_length_specified = SG_FALSE;
		const char * expect = NULL;
		bool_t errorSent = FALSE;

		get_content_length(conn, &ri->post_data_len, &content_length_specified);
		expect = mg_get_header(conn, "Expect");

		if (strcmp(ri->request_method,"POST")==0 || strcmp(ri->request_method,"PUT")==0) {
			if (!content_length_specified) {
				send_error(conn, 411, "Length Required", "");
				errorSent = TRUE;
			} else if (expect != NULL && mg_strcasecmp(expect, "100-continue")) {
				send_error(conn, 417, "Expectation Failed", "");
				errorSent = TRUE;
			}
		}

		if (!errorSent) {
			ri->expects_100_continue = (expect != NULL);
			mg_dispatch(conn);
		}

	} else if (!strcmp(ri->request_method, "PUT")) {
		send_error(conn, 405, "Method Not Allowed", "");
	} else if (!strcmp(ri->request_method, "DELETE")) {
		send_error(conn, 405, "Method Not Allowed", "");
	} else if (mg_stat(path, &st) != 0) {
		send_error(conn, 404, "Not Found", "%s", "File not found");
	} else if (st.is_directory && uri[strlen(uri) - 1] != '/') {
		(void) mg_printf(conn,
		    "HTTP/1.1 301 Moved Permanently\r\n"
		    "Location: %s/\r\n\r\n", uri);
	} else if (st.is_directory) {
		send_error(conn, 403, "Forbidden", "Directory listing denied");
	} else if (is_not_modified(conn, &st)) {
		send_error(conn, 304, "Not Modified", "");
	} else {
		send_file(conn, path, &st);
	}
}

static void
close_all_listening_sockets(struct mg_context *ctx)
{
	if(ctx->listener.sock!=-1)
		(void)closesocket(ctx->listener.sock);
}

static void
log_header(const struct mg_connection *conn, const char *header, FILE *fp)
{
	const char	*header_value;

	if ((header_value = mg_get_header(conn, header)) == NULL) {
		(void) fprintf(fp, "%s", " -");
	} else {
		(void) fprintf(fp, " \"%s\"", header_value);
	}
}

static void
log_access(const struct mg_connection *conn)
{
	const struct mg_request_info *ri;
	char		date[64];

	if (conn->ctx->access_log == NULL)
		return;

	(void) strftime(date, sizeof(date), "%d/%b/%Y:%H:%M:%S %z",
	    localtime(&conn->birth_time));

	ri = &conn->request_info;

	flockfile(conn->ctx->access_log);

	(void) fprintf(conn->ctx->access_log,
	    "%s [%s] \"%s %s HTTP/%d.%d\" %d %" UINT64_FMT "u",
	    inet_ntoa(conn->client.rsa.u.sin.sin_addr),
	    date,
	    ri->request_method ? ri->request_method : "-",
	    ri->uri ? ri->uri : "-",
	    ri->http_version_major, ri->http_version_minor,
	    conn->request_info.status_code, conn->num_bytes_sent);
	log_header(conn, "Referer", conn->ctx->access_log);
	log_header(conn, "User-Agent", conn->ctx->access_log);
	(void) fputc('\n', conn->ctx->access_log);
	(void) fflush(conn->ctx->access_log);

	funlockfile(conn->ctx->access_log);
}

static void
add_to_set(SOCKET fd, fd_set *set, int *max_fd)
{
	FD_SET(fd, set);
	if (*max_fd==-1 || fd > (SOCKET) *max_fd)
		*max_fd = (int) fd;
}

/*
 * Deallocate mongoose context, free up the resources
 */
static void
mg_fini(struct mg_context *ctx)
{
	close_all_listening_sockets(ctx);

	/* Wait until all threads finish */
	(void) pthread_mutex_lock(&ctx->thr_mutex);
#if !defined(WINDOWS)
	ctx->sq_head = -1;
	(void) pthread_cond_broadcast(&ctx->empty_cond); /* Signal server shutdown to idle threads. */
	while (ctx->num_threads > 0)
		(void) pthread_cond_wait(&ctx->thr_cond, &ctx->thr_mutex);
#else
	while (ctx->num_threads > 0) {
		ctx->sq_head = -1;
		(void) pthread_cond_signal(&ctx->empty_cond); /* Signal server shutdown to an idle thread. */
		(void) pthread_cond_wait(&ctx->thr_cond, &ctx->thr_mutex);
	}
#endif
	(void) pthread_mutex_unlock(&ctx->thr_mutex);

	/* Close log files */
	if (ctx->access_log)
		(void) fclose(ctx->access_log);

	(void) pthread_mutex_destroy(&ctx->thr_mutex);
	(void) pthread_mutex_destroy(&ctx->bind_mutex);
	(void) pthread_cond_destroy(&ctx->thr_cond);
	(void) pthread_cond_destroy(&ctx->empty_cond);
	(void) pthread_cond_destroy(&ctx->full_cond);
}

static bool_t
open_log_file(SG_context * pCtx, FILE **fpp, const char *path)
// pCtx: input parameter only (used for logging)
{
	bool_t	retval = TRUE;

	if (*fpp != NULL)
		(void) fclose(*fpp);

	if (path == NULL) {
		*fpp = NULL;
	} else if ((*fpp = mg_fopen(path, "a")) == NULL) {
		cry(pCtx, "%s(%s): %s", __func__, path, strerror(errno));
		retval = FALSE;
	} else {
		set_close_on_exec(fileno(*fpp));
	}

	return (retval);
}

static bool_t
set_access_log(SG_context * pCtx, struct mg_context *ctx, const char *path)
// pCtx: input parameter only (used for logging)
{
	return (open_log_file(pCtx, &ctx->access_log, path));
}

static void
close_socket_gracefully(struct mg_connection *conn, SOCKET sock)
{
	char	buf[BUFSIZ];
	int	n;

	/* Send FIN to the client */
	(void) shutdown(sock, SHUT_WR);
	set_non_blocking_mode(conn, sock);

	/*
	 * Read and discard pending data. If we do not do that and close the
	 * socket, the data in the send buffer may be discarded. This
	 * behaviour is seen on Windows, when client keeps sending data
	 * when server decide to close the connection; then when client
	 * does recv() it gets no data back.
	 */
	do {
		n = pull(NULL, sock, buf, sizeof(buf));
	} while (n > 0);

	/* Now we know that our FIN is ACK-ed, safe to close */
	(void) closesocket(sock);
}

static void
close_connection(struct mg_connection *conn)
{
	if (conn->client.sock != INVALID_SOCKET)
		close_socket_gracefully(conn, conn->client.sock);
}

static void
reset_connection_attributes(struct mg_connection *conn)
{
	conn->request_info.status_code = -1;
	conn->num_bytes_sent = 0;
	(void) memset(&conn->request_info, 0, sizeof(conn->request_info));
}

static void
shift_to_next(struct mg_connection *conn, char *buf, int req_len, int *nread)
{
	SG_uint64 cl = 0;
	SG_bool  cl_specified = SG_FALSE;
	int		over_len, body_len;

	get_content_length(conn, &cl, &cl_specified);
	over_len = *nread - req_len;
	assert(over_len >= 0);

	if (!cl_specified) {
		body_len = 0;
	} else if (cl < (SG_uint64) over_len) {
		body_len = (int) cl;
	} else {
		body_len = over_len;
	}

	*nread -= req_len + body_len;
	(void) memmove(buf, buf + req_len + body_len, *nread);
}

static void
process_new_connection(struct mg_connection *conn)
{
	struct mg_request_info *ri = &conn->request_info;
	char	buf[MAX_REQUEST_HEADERS_SIZE];
	int	request_len, nread;

	nread = 0;
	reset_connection_attributes(conn);

	/* If next request is not pipelined, read it in */
	if ((request_len = get_request_len(buf, (size_t) nread)) == 0)
		request_len = read_request(NULL, conn->client.sock,
		    buf, sizeof(buf), &nread);
	assert(nread >= request_len);

	if (request_len <= 0)
		return;	/* Remote end closed the connection */

	/* 0-terminate the request: parse_request uses sscanf */
	buf[request_len - 1] = '\0';

	if (parse_http_request(conn->pCtx, buf, ri, &conn->client.rsa)) {
		if (ri->http_version_major != 1 ||
		    (ri->http_version_major == 1 &&
		    (ri->http_version_minor < 0 ||
		    ri->http_version_minor > 1))) {
			send_error(conn, 505,
			    "HTTP version not supported",
			    "%s", "Weird HTTP version");
			log_access(conn);
		} else {
			ri->num_bytes_received = nread - request_len;
			if(ri->num_bytes_received>0)
				ri->post_data = buf + request_len;
			else
				ri->post_data = NULL;
			conn->birth_time = time(NULL);
			analyze_request(conn);
			log_access(conn);
			shift_to_next(conn, buf, request_len, &nread);
		}
	} else {
		/* Do not put garbage in the access log */
		send_error(conn, 400, "Bad Request",
		    "Can not parse request: [%.*s]", nread, buf);
	}
	
	SG_VHASH_NULLFREE(conn->pCtx, ri->headers);
}

/*
 * Worker threads take accepted socket from the queue
 */
static bool_t
get_socket(struct mg_context *ctx, struct socket *sp)
{
	struct timespec	ts;

	(void) pthread_mutex_lock(&ctx->thr_mutex);
	DEBUG_TRACE((DEBUG_MGS_PREFIX "%s: thread %p: going idle",
	    __func__, (void *) pthread_self()));

	/* If the queue is empty, wait. We're idle at this point. */
	ctx->num_idle++;
	while (ctx->sq_head == ctx->sq_tail) {
		ts.tv_nsec = 0;
		ts.tv_sec = (long)(time(NULL) + IDLE_TIME + 1);
		if (pthread_cond_timedwait(&ctx->empty_cond,
		    &ctx->thr_mutex, &ts) != 0) {
			/* Timeout! release the mutex and return */
			(void) pthread_mutex_unlock(&ctx->thr_mutex);
			return (FALSE);
		}
	}
	
	if (ctx->sq_head < 0) {
		/* Server is shutting down. */
		(void) pthread_mutex_unlock(&ctx->thr_mutex);
		return (FALSE);
	}
	
	assert(ctx->sq_head > ctx->sq_tail);

	/* We're going busy now: got a socket to process! */
	ctx->num_idle--;

	/* Copy socket from the queue and increment tail */
	*sp = ctx->queue[ctx->sq_tail % ARRAY_SIZE(ctx->queue)];
	ctx->sq_tail++;
	DEBUG_TRACE((DEBUG_MGS_PREFIX
	    "%s: thread %p grabbed socket %d, going busy",
	    __func__, (void *) pthread_self(), sp->sock));

	/* Wrap pointers if needed */
	while (ctx->sq_tail > (int) ARRAY_SIZE(ctx->queue)) {
		ctx->sq_tail -= ARRAY_SIZE(ctx->queue);
		ctx->sq_head -= ARRAY_SIZE(ctx->queue);
	}

	pthread_cond_signal(&ctx->full_cond);
	(void) pthread_mutex_unlock(&ctx->thr_mutex);

	return (TRUE);
}

static void
worker_thread(struct mg_context *ctx)
{
	struct mg_connection	conn;

	DEBUG_TRACE((DEBUG_MGS_PREFIX "%s: thread %p starting",
	    __func__, (void *) pthread_self()));

	(void) memset(&conn, 0, sizeof(conn));
	conn.ctx = ctx;
	
	if (SG_IS_OK(SG_context__alloc(&conn.pCtx)) && conn.pCtx != NULL ) {
		while (get_socket(ctx, &conn.client) == TRUE) {
			conn.birth_time = time(NULL);
			conn.ctx = ctx;
	
			process_new_connection(&conn);
	
			close_connection(&conn);
		}
	}

	/* Signal master that we're done with connection and exiting */
	pthread_mutex_lock(&conn.ctx->thr_mutex);
	conn.ctx->num_threads--;
	conn.ctx->num_idle--;
	pthread_cond_signal(&conn.ctx->thr_cond);
	assert(conn.ctx->num_threads >= 0);
	pthread_mutex_unlock(&conn.ctx->thr_mutex);

	SG_CONTEXT_NULLFREE(conn.pCtx);

	DEBUG_TRACE((DEBUG_MGS_PREFIX "%s: thread %p exiting",
	    __func__, (void *) pthread_self()));
}

/*
 * Master thread adds accepted socket to a queue
 */
static void
put_socket(SG_context * pCtx, struct mg_context *ctx, const struct socket *sp)
// pCtx: input parameter only (used for logging)
{
	(void) pthread_mutex_lock(&ctx->thr_mutex);

	/* If the queue is full, wait */
	while (ctx->sq_head - ctx->sq_tail >= (int) ARRAY_SIZE(ctx->queue))
		(void) pthread_cond_wait(&ctx->full_cond, &ctx->thr_mutex);
	assert(ctx->sq_head - ctx->sq_tail < (int) ARRAY_SIZE(ctx->queue));

	/* Copy socket to the queue and increment head */
	ctx->queue[ctx->sq_head % ARRAY_SIZE(ctx->queue)] = *sp;
	ctx->sq_head++;
	DEBUG_TRACE((DEBUG_MGS_PREFIX "%s: queued socket %d",
	    __func__, sp->sock));

	/* If there are no idle threads, start one */
	if (ctx->num_idle == 0 && ctx->num_threads < MAX_THREADS) {
		if (start_thread(pCtx,
		    (mg_thread_func_t) worker_thread, ctx) != 0)
			cry(pCtx, "Cannot start thread: %d", ERRNO);
		else
			ctx->num_threads++;
	}

	pthread_cond_signal(&ctx->empty_cond);
	(void) pthread_mutex_unlock(&ctx->thr_mutex);
}

static void
accept_new_connection(SG_context *pCtx, const struct socket *listener, struct mg_context *ctx)
// pCtx: input parameter only (used for logging)
{
	struct socket		accepted;

	accepted.rsa.len = sizeof(accepted.rsa.u.sin);
	accepted.lsa = listener->lsa;
	if ((accepted.sock = accept(listener->sock,
	    &accepted.rsa.u.sa, &accepted.rsa.len)) == INVALID_SOCKET)
		return;

	/* Put accepted socket structure into the queue */
	DEBUG_TRACE((DEBUG_MGS_PREFIX "%s: accepted socket %d",
	    __func__, accepted.sock));
	put_socket(pCtx, ctx, &accepted);
}

static void
master_thread(struct mg_context *ctx)
{
	SG_context * pCtx;
	fd_set		read_set;
	struct timeval	tv;
	int		max_fd;

	(void)SG_context__alloc(&pCtx);

	while (ctx->stop_flag == 0) {
		FD_ZERO(&read_set);
		max_fd = -1;

		/* Add listening socket to the read set */
		add_to_set(ctx->listener.sock, &read_set, &max_fd);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if (select(max_fd + 1, &read_set, NULL, NULL, &tv) < 0) {
#ifdef _WIN32
			/*
			 * On windows, if read_set and write_set are empty,
			 * select() returns "Invalid parameter" error
			 * (at least on my Windows XP Pro). So in this case,
			 * we sleep here.
			 */
			sleep(1);
#endif /* _WIN32 */
		} else {
			if (FD_ISSET(ctx->listener.sock, &read_set))
				accept_new_connection(pCtx, &ctx->listener, ctx);
		}
	}

	/* Stop signal received: somebody called mg_stop. Quit. */
	mg_fini(ctx);

	/* call uridispatch cleanup. */
	SG_uridispatch__global_cleanup(pCtx);

	/* Signal mg_stop() that we're done */
	ctx->stop_flag = 2;

	SG_CONTEXT_NULLFREE(pCtx);
}

void
mg_stop(SG_context * pCtx, struct mg_context *ctx)
{
	ctx->stop_flag = 1;

	/* Wait until mg_fini() stops */
	while (ctx->stop_flag != 2)
		(void) sleep(1);

	assert(ctx->num_threads == 0);

	SG_PATHNAME_NULLFREE(pCtx, ctx->static_root);

	SG_NULLFREE(pCtx, ctx);

#if defined(_WIN32)
	(void) WSACleanup();
#endif /* _WIN32 */
}

static void _requireDirectory(SG_context *pCtx, SG_pathname *path)
{
	SG_bool ssiExists = SG_FALSE;
	SG_fsobj_type fot;
	SG_fsobj_perms perms;

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, path, &ssiExists, &fot, &perms)  );

	if (! ssiExists)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "The required subfolder %s (inferred from localsetting %s) does not exist.\n\n",
			SG_pathname__sz(path), SG_LOCALSETTING__SERVER_FILES)  );
		SG_ERR_THROW(SG_ERR_NOT_A_DIRECTORY);
	}
	else if (fot != SG_FSOBJ_TYPE__DIRECTORY)
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s (inferred from localsetting %s) is not a directory.\n\n",
			SG_pathname__sz(path), SG_LOCALSETTING__SERVER_FILES)  );
		SG_ERR_THROW(SG_ERR_NOT_A_DIRECTORY);
	}

fail:
	;
}

static void
mg_init(SG_context *pCtx, struct mg_context *ctx, SG_bool public, int port)
{
	char * szServerFiles = NULL;
	SG_pathname *pServerFiles = NULL;
	SG_pathname *pCoreTemplates = NULL;
	SG_fsobj_type fot;
	SG_fsobj_perms perms;
	SG_bool ssiExists = SG_FALSE;
	char * sz_access_log = NULL;

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_FILES, NULL, &szServerFiles, NULL)  );
	if((szServerFiles==NULL) || (strlen(szServerFiles) == 0)) {
		SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "\rThe localsetting %s is missing or empty.\n\n%s must be set to run the server.",
			SG_LOCALSETTING__SERVER_FILES, SG_LOCALSETTING__SERVER_FILES)  );
	}
	SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pServerFiles, szServerFiles)  );
	SG_NULLFREE(pCtx, szServerFiles);

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pServerFiles, &ssiExists, &fot, &perms)  );

	if (!ssiExists) {
		SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "\rThe path %s specified in localsetting %s does not exist.",
			SG_pathname__sz(pServerFiles), SG_LOCALSETTING__SERVER_FILES)  );
	} else if (fot != SG_FSOBJ_TYPE__DIRECTORY) {
		SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "\rThe path %s specified in localsetting %s is not a directory.",
			SG_pathname__sz(pServerFiles), SG_LOCALSETTING__SERVER_FILES)  );
	}

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &ctx->static_root, pServerFiles, "ui")  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pCoreTemplates, pServerFiles, "core/templates")  );
	SG_PATHNAME_NULLFREE(pCtx, pServerFiles);

	SG_ERR_CHECK(  _requireDirectory(pCtx, ctx->static_root)  );
	SG_ERR_CHECK(  _requireDirectory(pCtx, pCoreTemplates)  );
	SG_PATHNAME_NULLFREE(pCtx, pCoreTemplates);

	SG_ERR_CHECK(  SG_pathname__remove_final_slash(pCtx, ctx->static_root, NULL)  );

	ctx->listener.sock = mg_open_listening_port(pCtx, ctx, port, public);
	if (ctx->listener.sock==INVALID_SOCKET)
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED,  (pCtx, "An error occured when binding server to port(s) '%d'.", port));

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, "server/access_log", NULL, &sz_access_log, NULL)  );
	if(sz_access_log!=NULL) {
		set_access_log(pCtx, ctx, sz_access_log);
		SG_NULLFREE(pCtx, sz_access_log);
	}

	SG_ERR_CHECK(  SG_uridispatch__init(pCtx, NULL)  );

	return;
fail:
	SG_NULLFREE(pCtx, szServerFiles);
	SG_PATHNAME_NULLFREE(pCtx, pServerFiles);
	SG_PATHNAME_NULLFREE(pCtx, ctx->static_root);
	SG_PATHNAME_NULLFREE(pCtx, pCoreTemplates);
	SG_NULLFREE(pCtx, sz_access_log);
}

void
mg_start(SG_context * pCtx, SG_bool public, int port, struct mg_context **pp_ctx)
{
	struct mg_context	*ctx;

#if defined(_WIN32)
	WSADATA data;
	WSAStartup(MAKEWORD(2,2), &data);
#endif /* _WIN32 */

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, ctx)  );

#if !defined(_WIN32)
	/*
	 * Ignore SIGPIPE signal, so if browser cancels the request, it
	 * won't kill the whole process.
	 */
	(void) signal(SIGPIPE, SIG_IGN);
#endif /* _WIN32 */

	(void) pthread_mutex_init(&ctx->thr_mutex, NULL);
	(void) pthread_mutex_init(&ctx->bind_mutex, NULL);
	(void) pthread_cond_init(&ctx->thr_cond, NULL);
	(void) pthread_cond_init(&ctx->empty_cond, NULL);
	(void) pthread_cond_init(&ctx->full_cond, NULL);

	SG_ERR_CHECK(  mg_init(pCtx, ctx, public, port)  );

	/* Start master (listening) thread */
	start_thread(pCtx, (mg_thread_func_t) master_thread, ctx);

	*pp_ctx = ctx;

	return;
fail:
	mg_fini(ctx);
	SG_NULLFREE(pCtx, ctx);
}
