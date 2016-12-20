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

// sg_file.c
// implementation of file stuff -- raw/unbuffered file io on a handle or fd.
//////////////////////////////////////////////////////////////////

#include <sg.h>

#if defined(MAC) || defined(LINUX)
#include <sys/mman.h>
#endif

//////////////////////////////////////////////////////////////////

struct _SG_file
{
#if defined(MAC) || defined(LINUX)
	int				m_fd;
#endif
#if defined(WINDOWS)
	HANDLE			m_hFile;
#endif
};

#if defined(MAC) || defined(LINUX)
#define MY_IS_CLOSED(pThis)		((pThis)->m_fd == -1)
#define MY_SET_CLOSED(pThis)	SG_STATEMENT( (pThis)->m_fd = -1; )
#endif
#if defined(WINDOWS)
#define MY_IS_CLOSED(pThis)		((pThis)->m_hFile == INVALID_HANDLE_VALUE)
#define MY_SET_CLOSED(pThis)	SG_STATEMENT( (pThis)->m_hFile = INVALID_HANDLE_VALUE; )
#endif

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
/**
 * map flags_sg and perms_sg into generic posix-platform values.
 *
 */
void _sg_compute_posix_open_args(
	SG_context* pCtx,
	SG_file_flags flags_sg,
	SG_fsobj_perms perms_sg,
	int * pOFlag,
	mode_t * pMode
	)
{
	int oflag;
	mode_t mode;

	oflag = 0;
	if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_RDONLY)
	{
		SG_ARGCHECK_RETURN( !(flags_sg & SG_FILE_TRUNC) , flags_sg );

		oflag |= O_RDONLY;
	}
	else if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_WRONLY)
	{
		oflag |= O_WRONLY;
	}
	else if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_RDWR)
	{
		oflag |= O_RDWR;
	}
	else
	{
		SG_ARGCHECK_RETURN( SG_FALSE , flags_sg );
	}

	if ((flags_sg & SG_FILE__OPEN_MASK) == SG_FILE_OPEN_EXISTING)
	{
		if (flags_sg & SG_FILE_TRUNC)
			oflag |= O_TRUNC;
	}
	else if ((flags_sg & SG_FILE__OPEN_MASK) == SG_FILE_CREATE_NEW)
	{
		SG_ARGCHECK_RETURN( !(flags_sg & SG_FILE_TRUNC) , flags_sg );

		oflag |= (O_CREAT|O_EXCL);
	}
	else if ((flags_sg & SG_FILE__OPEN_MASK) == SG_FILE_OPEN_OR_CREATE)
	{
		if (flags_sg & SG_FILE_TRUNC)
			oflag |= O_TRUNC;

		oflag |= O_CREAT;
	}
	else
	{
		SG_ARGCHECK_RETURN( SG_FALSE , flags_sg );
	}

	// TODO Linux offers a O_LARGEFILE flag.
	// TODO is it needed if we defined the
	// TODO LFS args on the compile line?

	// we defined perms to match Linux/Mac mode_t mode bits.

	mode = (mode_t)(perms_sg & SG_FSOBJ_PERMS__MASK);

	*pOFlag = oflag;
	*pMode = mode;
}

/**
 * my wrapper for open(2) for posix-based systems.
 *
 */
void _sg_file_posix_open(SG_context* pCtx, const SG_pathname * pPathname, SG_file_flags flags_sg, SG_fsobj_perms perms_sg, SG_file** ppResult)
{
	char * pBufOS = NULL;
	SG_file * pf = NULL;
	int oflag = 0;
	mode_t mode = 0;

	// convert generic flags/perms into something platform-specific.

	SG_ERR_CHECK(  _sg_compute_posix_open_args(pCtx, flags_sg, perms_sg, &oflag, &mode)  );

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pf)  );

	MY_SET_CLOSED(pf);

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname), &pBufOS)  );

	pf->m_fd = open(pBufOS,oflag,mode);
	if (MY_IS_CLOSED(pf))
		SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
						(pCtx, "Attempting open() on '%s'", SG_pathname__sz(pPathname))  );

	SG_NULLFREE(pCtx, pBufOS);

	if (flags_sg & SG_FILE_LOCK)
	{
		/* TODO for now, I'm using flock here.
		 * fcntl might be better.
		 * there are tradeoffs both ways.
		 * flock doesn't work over NFS, but
		 * fcntl has really goofy semantics
		 * around the closing of a file. */

		int reslock;
		int locktype;

		locktype = (flags_sg & SG_FILE_RDONLY) ? LOCK_SH : LOCK_EX;
		locktype |= LOCK_NB;

		reslock = flock(pf->m_fd, locktype);
		if (0 != reslock)
		{
			/* TODO if the file was created, delete it.
			 * unfortunately, there seems to be a case
			 * where we don't know if we created the file
			 * or not:  SG_FILE_OPEN_OR_CREATE
			 *
			 * Alternatively, we may just want to be careless
			 * and let them have the file without the lock
			 * since locks are advisory only... (sigh)
			 */

			/* TODO should we get the err from errno instead?
			 * SG_ERR_THROW(SG_ERR_ERRNO(errno));
			 */
			SG_ERR_THROW2(  SG_ERR_FILE_LOCK_FAILED,
							(pCtx, "Could not lock '%s'.", SG_pathname__sz(pPathname))  );
		}
	}

	*ppResult = pf;

	return;
fail:
	SG_NULLFREE(pCtx, pBufOS);
	if (pf && !MY_IS_CLOSED(pf))
		(void)close(pf->m_fd);
	SG_NULLFREE(pCtx, pf);
}
#endif



#if defined(WINDOWS)
/**
 * map flags and perms into Windows-specific values for CreateFileW.
 *
 */
void _sg_compute_createfile_args(
	SG_context* pCtx,
	SG_file_flags flags_sg,
	SG_fsobj_perms perms_sg,
	DWORD * pdwDesiredAccess,
	DWORD * pdwShareMode,
	DWORD * pdwCreationDisposition,
	DWORD * pdwFlagsAndAttributes
	)
{
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	DWORD dwCreationDisposition;
	DWORD dwFlagsAndAttributes;
	SG_bool bLock;

	// Windows FILE_SHARE_{READ,WRITE} seem to be the exact opposite
	// of a LOCK.  It seems like the file will be locked by default
	// ***unless*** you allow sharing.

	bLock = ((flags_sg & SG_FILE_LOCK) == SG_FILE_LOCK);

	if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_RDONLY)
	{
		SG_ARGCHECK_RETURN( !(flags_sg & SG_FILE_TRUNC) , flags_sg );

		dwDesiredAccess = GENERIC_READ;
		dwShareMode = ((bLock) ? (FILE_SHARE_READ) : (FILE_SHARE_READ | FILE_SHARE_WRITE));
	}
	else if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_WRONLY)
	{
		dwDesiredAccess = GENERIC_WRITE;
		dwShareMode = ((bLock) ? 0 : (FILE_SHARE_READ | FILE_SHARE_WRITE));
	}
	else if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_RDWR)
	{
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		dwShareMode = ((bLock) ? 0 : (FILE_SHARE_READ | FILE_SHARE_WRITE));
	}
	else
	{
		SG_ARGCHECK_RETURN( SG_FALSE , flags_sg );
	}

	dwCreationDisposition = 0;
	if ((flags_sg & SG_FILE__OPEN_MASK) == SG_FILE_OPEN_EXISTING)
	{
		if (flags_sg & SG_FILE_TRUNC)
			dwCreationDisposition = TRUNCATE_EXISTING;
		else
			dwCreationDisposition = OPEN_EXISTING;
	}
	else if ((flags_sg & SG_FILE__OPEN_MASK) == SG_FILE_CREATE_NEW)
	{
		SG_ARGCHECK_RETURN( !(flags_sg & SG_FILE_TRUNC) , flags_sg );

		dwCreationDisposition = CREATE_NEW;
	}
	else if ((flags_sg & SG_FILE__OPEN_MASK) == SG_FILE_OPEN_OR_CREATE)
	{
		if (flags_sg & SG_FILE_TRUNC)
			dwCreationDisposition = CREATE_ALWAYS;
		else
			dwCreationDisposition = OPEN_ALWAYS;
	}
	else
	{
		SG_ARGCHECK_RETURN( SG_FALSE , flags_sg );
	}

	if (SG_FSOBJ_PERMS__WINDOWS_IS_READONLY(perms_sg))
		dwFlagsAndAttributes = FILE_ATTRIBUTE_READONLY;
	else
		dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

	*pdwDesiredAccess = dwDesiredAccess;
	*pdwShareMode = dwShareMode;
	*pdwCreationDisposition = dwCreationDisposition;
	*pdwFlagsAndAttributes = dwFlagsAndAttributes;
}

/**
 * my wrapper for CreateFileW()
 *
 */
void _sg_file_createfilew(SG_context* pCtx, const SG_pathname * pPathname, SG_file_flags flags_sg, SG_fsobj_perms perms_sg, SG_file** ppResult)
{
	SG_file * pf = NULL;
	wchar_t * pBuf = NULL;
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	DWORD dwCreationDisposition;
	DWORD dwFlagsAndAttributes;

	// convert generic flags/perms into something windows-specific.

	SG_ERR_CHECK(  _sg_compute_createfile_args(pCtx, flags_sg,perms_sg,
													 &dwDesiredAccess,&dwShareMode,
													 &dwCreationDisposition,&dwFlagsAndAttributes)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pf)  );

	MY_SET_CLOSED(pf);

	// convert UTF-8 pathname into wchar_t so that we can use the W version of the windows API.

	SG_ERR_CHECK(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,NULL)  );

	pf->m_hFile = CreateFileW(pBuf,
							  dwDesiredAccess,dwShareMode,
							  NULL, // lpSecurityAtributes
							  dwCreationDisposition,dwFlagsAndAttributes,
							  NULL);
	if (MY_IS_CLOSED(pf))
		SG_ERR_THROW2(SG_ERR_GETLASTERROR(GetLastError()), (pCtx, SG_pathname__sz(pPathname)));

	SG_NULLFREE(pCtx, pBuf);

	*ppResult = pf;

	return;
fail:
	SG_NULLFREE(pCtx, pBuf);
	if (pf && !MY_IS_CLOSED(pf))
		(void)CloseHandle(pf->m_hFile);
	SG_NULLFREE(pCtx, pf);
}
#endif

//////////////////////////////////////////////////////////////////

void SG_file__open__pathname(SG_context* pCtx, const SG_pathname * pPathname, SG_file_flags flags_sg, SG_fsobj_perms perms_sg, SG_file** ppResult)
{
	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );
	SG_NULLARGCHECK_RETURN(ppResult);

#if defined(MAC) || defined(LINUX)
	SG_ERR_CHECK_RETURN(  _sg_file_posix_open(pCtx, pPathname,flags_sg,perms_sg,ppResult)  );
#endif
#if defined(WINDOWS)
	SG_ERR_CHECK_RETURN(  _sg_file_createfilew(pCtx, pPathname,flags_sg,perms_sg,ppResult)  );
#endif
}

// Couldn't get this to work on Windows.
#if defined(MAC) || defined(LINUX)
void SG_file__open__null_device(SG_context* pCtx, SG_file_flags flags_sg, SG_file** ppResult)
{
	SG_pathname * pPath = NULL;

#if defined(MAC) || defined(LINUX)
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, "/dev/null")  );
#endif
#if defined(WINDOWS)
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, "NUL:/")  );
#endif
	
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_EXISTING|flags_sg, 0666, ppResult)  );
	
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	
	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}
#endif

//////////////////////////////////////////////////////////////////

void SG_file__truncate(SG_context* pCtx, SG_file* pFile)
{
	// seek absolute from beginning

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

#if defined(MAC) || defined(LINUX)
	{
		off_t pos;
		int res;

		// verify that we have LFS support.
		// off_t lseek(..., off_t offset, ...)
		// note that off_t is signed.

		SG_ASSERT(sizeof(off_t) >= sizeof(SG_uint64));

		pos = lseek(pFile->m_fd, (off_t)0, SEEK_CUR);
		if (pos == (off_t)-1)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));


		SG_ASSERT(sizeof(off_t) >= sizeof(SG_uint64));

		res = ftruncate(pFile->m_fd, pos);
		if (res < 0)
		{
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));
		}
	}
#endif

#if defined(WINDOWS)
	{
		if (!SetEndOfFile(pFile->m_hFile))
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));
	}
#endif
}

void SG_file__seek(SG_context* pCtx, SG_file* pFile, SG_uint64 iPos)
{
	// seek absolute from beginning

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

#if defined(MAC) || defined(LINUX)
	{
		off_t res;

		// verify that we have LFS support.
		// off_t lseek(..., off_t offset, ...)
		// note that off_t is signed.

		SG_ASSERT(sizeof(off_t) >= sizeof(SG_uint64));
		SG_ASSERT(iPos <= 0x7fffffffffffffffULL);

		res = lseek(pFile->m_fd, (off_t)iPos, SEEK_SET);
		if (res == (off_t)-1)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));
	}
#endif
#if defined(WINDOWS)
	{
		LARGE_INTEGER li;

		// LARGE_INTEGER is a union/struct...
		// it contains a (signed) LONGLONG called QuadPart.
		SG_ASSERT(sizeof(LARGE_INTEGER) >= sizeof(SG_int64));
		SG_ASSERT(iPos <= 0x7fffffffffffffffULL);

		li.QuadPart = iPos;

		if (!SetFilePointerEx(pFile->m_hFile,li,NULL,FILE_BEGIN))
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));
	}
#endif
}

void SG_file__seek_end(SG_context* pCtx, SG_file* pFile, SG_uint64 * piPos)
{
	// seek to end of file

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

#if defined(MAC) || defined(LINUX)
	{
		off_t res;

		// verify that we have LFS support.
		// off_t lseek(..., off_t offset, ...)
		// note that off_t is signed.

		SG_ASSERT(sizeof(off_t) >= sizeof(SG_uint64));

		res = lseek(pFile->m_fd, (off_t)0, SEEK_END);
		if (res == (off_t)-1)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));

		if (piPos)
			*piPos = (SG_uint64)res;
	}
#endif
#if defined(WINDOWS)
	{
		LARGE_INTEGER zero;
		LARGE_INTEGER res;

		zero.QuadPart = 0;

		// LARGE_INTEGER is a union/struct...
		// it contains a (signed) LONGLONG called QuadPart.
		SG_ASSERT(sizeof(LARGE_INTEGER) >= sizeof(SG_int64));

		if (!SetFilePointerEx(pFile->m_hFile,zero,&res,FILE_END))
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));

		if (piPos)
			*piPos = (SG_uint64)res.QuadPart;
	}
#endif
}

void SG_file__tell(SG_context* pCtx, SG_file* pFile, SG_uint64 * piPos)
{
	// get current file position

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

#if defined(MAC) || defined(LINUX)
	{
		off_t res;

		// verify that we have LFS support.
		// off_t lseek(..., off_t offset, ...)
		// note that off_t is signed.

		SG_ASSERT(sizeof(off_t) >= sizeof(SG_uint64));

		res = lseek(pFile->m_fd, (off_t)0, SEEK_CUR);
		if (res == (off_t)-1)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));

		if (piPos)
			*piPos = (SG_uint64)res;
	}
#endif
#if defined(WINDOWS)
	{
		LARGE_INTEGER zero;
		LARGE_INTEGER res;

		zero.QuadPart = 0;

		// LARGE_INTEGER is a union/struct...
		// it contains a (signed) LONGLONG called QuadPart.
		SG_ASSERT(sizeof(LARGE_INTEGER) >= sizeof(SG_int64));

		if (!SetFilePointerEx(pFile->m_hFile,zero,&res,FILE_CURRENT))
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));

		if (piPos)
			*piPos = (SG_uint64)res.QuadPart;
	}
#endif
}

//////////////////////////////////////////////////////////////////

void SG_file__read(SG_context* pCtx, SG_file* pFile, SG_uint32 iNumBytesWanted, SG_byte* pBytes, SG_uint32* piNumBytesRetrieved)
{
	SG_uint32 iNumBytesRetrieved;

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

	// ssize_t read(..., size_t count);
	// on 64bit systems ssize_t and size_t may be 64bit.
	// on 32bit systems with LFS support, they are still 32bit.
	//
	// read() takes an unsigned arg.  windows ReadFile()
	// takes an unsigned arg.  so we have an unsigned arg.
	// read() returns signed result, so it can't do full
	// 32bit value, but ReadFile() can.
	//
	// so, to be consistent between platforms, we clamp buffer
	// length to max signed int.

	SG_ASSERT(sizeof(size_t) >= sizeof(SG_uint32));
	SG_ARGCHECK_RETURN(iNumBytesWanted <= 0x7fffffff, "iNumBytesWanted");

	SG_ASSERT(pBytes);

#if defined(MAC) || defined(LINUX)
	{
		ssize_t res;

		res = read(pFile->m_fd, pBytes, (size_t)iNumBytesWanted);
		if (res < 0)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));
		if (res == 0)
			SG_ERR_THROW_RETURN(SG_ERR_EOF);

		iNumBytesRetrieved = (SG_uint32)res;
	}
#endif
#if defined(WINDOWS)
	{
		DWORD nbr;

		if (!ReadFile(pFile->m_hFile, pBytes, (DWORD)iNumBytesWanted, &nbr, NULL))
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));
		if (nbr == 0)
			SG_ERR_THROW_RETURN(SG_ERR_EOF);

		iNumBytesRetrieved = (SG_uint32)nbr;
	}
#endif

	// if they gave us a place to return the number of bytes read,
	// then return it and no error.  (even for incomplete reads).

	if (piNumBytesRetrieved)
	{
		*piNumBytesRetrieved = iNumBytesRetrieved;
		return;
	}

	// when we don't have a place to return the number of bytes read.
	// we return OK only for a complete read.  we return an error for
	// an incomplete read. (there shouldn't be any complaints, because
	// we gave them a chance to get the paritial read info.)

	if (iNumBytesRetrieved != iNumBytesWanted)
		SG_ERR_THROW_RETURN(SG_ERR_INCOMPLETEREAD);
}

void SG_file__eof(SG_context* pCtx, SG_file* pFile, SG_bool* pEof)
{
    SG_byte b;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pFile);
    SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );
    SG_NULLARGCHECK_RETURN(pEof);

    SG_file__read(pCtx, pFile, 1, &b, NULL);
    if(SG_context__err_equals(pCtx, SG_ERR_EOF))
    {
        SG_context__err_reset(pCtx);
        *pEof = SG_TRUE;
    }
    else
    {
        SG_uint64 pos = 0;
        SG_ERR_CHECK_CURRENT;
        SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &pos)  );
        SG_ASSERT(pos>0);
        SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, pos-1)  );
        *pEof = SG_FALSE;
    }

    return;
fail:
    ;
}

void SG_file__write(SG_context* pCtx, SG_file* pFile, SG_uint32 iNumBytes, const SG_byte* pBytes, SG_uint32 * piNumBytesWritten)
{
	SG_uint32 nbw;

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

	// see notes in SG_file__read() about 64 vs 32 bits.

	SG_ASSERT(sizeof(size_t) >= sizeof(SG_uint32));
	SG_ARGCHECK_RETURN(iNumBytes <= 0x7fffffff, "iNumBytes");

	SG_ASSERT(pBytes);

#if defined(MAC) || defined(LINUX)
	{
		ssize_t res;

		res = write(pFile->m_fd, pBytes, (size_t)iNumBytes);
		if (res < 0)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));

		nbw = (SG_uint32)res;
	}
#endif
#if defined(WINDOWS)
	{
		if (!WriteFile(pFile->m_hFile, pBytes, (DWORD)iNumBytes, (LPDWORD)&nbw, NULL))
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));
	}
#endif

	// if they gave us a place to return the number of bytes written,
	// then return it and no error.  (even for incomplete writes).

	if (piNumBytesWritten)
	{
		*piNumBytesWritten = (SG_uint32)nbw;
		return;
	}

	// when we don't have a place to return the number of bytes written.
	// we return OK only for a complete write.  we return an error for
	// an incomplete write. (there shouldn't be any complaints, because
	// we gave them a chance to get the paritial write info.)

	if ((SG_uint32)nbw != iNumBytes)
		SG_ERR_THROW_RETURN(SG_ERR_INCOMPLETEWRITE);
}

void SG_file__write__sz(SG_context* pCtx, SG_file * pThis, const char * sz )
{
	SG_file__write(pCtx, pThis, SG_STRLEN(sz), (const SG_byte*)sz, NULL);
}
void SG_file__write__format(SG_context* pCtx, SG_file* pFile, const char * szFormat, ...)
{
    va_list ap;
    va_start(ap,szFormat);
    SG_file__write__vformat(pCtx, pFile, szFormat, ap);
    va_end(ap);
}
void SG_file__write__vformat(SG_context* pCtx, SG_file* pFile, const char * szFormat, va_list ap)
{
    SG_string * pStr = NULL;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(pFile);
    SG_NULLARGCHECK_RETURN(szFormat);

    SG_ERR_CHECK(  SG_STRING__ALLOC__VFORMAT(pCtx, &pStr, szFormat, ap)  );
    SG_ERR_CHECK(  SG_file__write__string(pCtx, pFile, pStr)  );
    SG_STRING_NULLFREE(pCtx, pStr);

    return;
fail:
    SG_STRING_NULLFREE(pCtx, pStr);
}
void SG_file__write__string(SG_context* pCtx, SG_file * pThis, const SG_string * pString )
{
	SG_file__write(pCtx, pThis, SG_string__length_in_bytes(pString), (const SG_byte*)SG_string__sz(pString), NULL);
}

void SG_file__close(SG_context* pCtx, SG_file** ppFile)
{
	if(ppFile==NULL || *ppFile==NULL)
		return;

	if (!MY_IS_CLOSED((*ppFile)))
	{
#if defined(MAC) || defined(LINUX)
		if (close((*ppFile)->m_fd) == -1)
			SG_ERR_THROW(SG_ERR_ERRNO(errno));
#endif
#if defined(WINDOWS)
		if (!CloseHandle((*ppFile)->m_hFile))
			SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
#endif

		MY_SET_CLOSED((*ppFile));
	}

	// we only free pFile if we successfully closed the file.

	SG_free(pCtx, (*ppFile));

	*ppFile	= NULL;

	return;
fail:
	// WARNING: we ***DO NOT*** free pFile if close() fails.
	// WARNING: close() can fail in weird cases, see "Man close(2)"
	// WARNING: on Linux.
	// WARNING:
	// WARNING: I'm not sure what the caller can do about it, but
	// WARNING: if we free their pointer and file handle, they
	// WARNING: can't do anything...

	return;
}

//////////////////////////////////////////////////////////////////

void SG_file__fsync(SG_context* pCtx, SG_file * pFile)
{
	// try to call fsync on the underlying file.

	SG_NULLARGCHECK_RETURN(pFile);
	SG_ARGCHECK_RETURN( !MY_IS_CLOSED(pFile) , pFile );

#if defined(MAC) || defined(LINUX)
	{
		if (fsync(pFile->m_fd) == -1)
			SG_ERR_THROW_RETURN(SG_ERR_ERRNO(errno));
		return;
	}
#endif
#if defined(WINDOWS)
	{
		// NOTE: Windows also has a FILE_FLAG_NO_BUFFERING flag that can be used in CreateFile().

		if (FlushFileBuffers(pFile->m_hFile) == 0)
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));
		return;
	}
#endif
}

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
void SG_file__get_fd(SG_context* pCtx, SG_file * pFile, int * pfd)
{
	SG_NULLARGCHECK_RETURN(pFile);
	SG_NULLARGCHECK_RETURN(pfd);
	*pfd = pFile->m_fd;
}
#endif

#if defined(WINDOWS)
void SG_file__get_handle(SG_context* pCtx, SG_file * pFile, HANDLE * phFile)
{
	SG_NULLARGCHECK_RETURN(pFile);
	SG_NULLARGCHECK_RETURN(phFile);
	*phFile = pFile->m_hFile;
}
#endif

struct _sg_mmap
{
    SG_byte*   mem;

    SG_byte*   _private__mapped_ptr;
    SG_uint64  _private__mapped_size;
};

void SG_file__mmap(
        SG_context* pCtx, 
        SG_file* pFile,
        SG_uint64 offset,
        SG_uint64 requested_len,
        SG_file_flags flags_sg,
        SG_mmap** ppmap
        )
{
    SG_mmap* pmap = NULL;
#if defined(WINDOWS)
    HANDLE hMapping;
#endif

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pmap)  );

#if defined(WINDOWS)
    {
        DWORD flProtect = 0;
        DWORD dwDesiredAccess = 0;
        SG_uint32 extra = 0;
        SG_uint64 actual_offset = offset;

        pmap->_private__mapped_size = requested_len;

        if (offset)
        {
            SYSTEM_INFO si;

            GetSystemInfo(&si);

            actual_offset = (offset / si.dwAllocationGranularity) * si.dwAllocationGranularity;
            SG_ASSERT(offset >= actual_offset);
            extra = (SG_uint32) (offset - actual_offset);
            pmap->_private__mapped_size += extra;
        }


        if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_RDONLY)
        {
            flProtect = PAGE_READONLY;
            dwDesiredAccess = FILE_MAP_READ;
        }
        else
        {
            flProtect = PAGE_READWRITE;
            dwDesiredAccess = FILE_MAP_WRITE;
        }

        hMapping = CreateFileMapping(pFile->m_hFile, NULL, flProtect, (DWORD) ((pmap->_private__mapped_size >> 32) & 0xffffffff), (DWORD) (pmap->_private__mapped_size & 0xffffffff), NULL);
        if (!hMapping)
        {
            SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
        }
        // TODO make sure pmap->_private__mapped_size fits in SIZE_T
        pmap->_private__mapped_ptr = MapViewOfFile(hMapping, dwDesiredAccess, (DWORD) ((actual_offset >> 32) & 0xffffffff), (DWORD) (actual_offset & 0xffffffff), (SIZE_T) pmap->_private__mapped_size);
        if (!pmap->_private__mapped_ptr)
        {
            SG_ERR_THROW(SG_ERR_GETLASTERROR(GetLastError()));
        }
        CloseHandle(hMapping);
        hMapping = NULL;

        pmap->mem = pmap->_private__mapped_ptr + extra;
    }
#endif

#if defined(MAC) || defined(LINUX)
    {
        int prot = 0;
        SG_uint32 extra = 0;
        SG_uint64 actual_offset = offset;

        pmap->_private__mapped_size = requested_len;

        if (offset)
        {
            int ps = getpagesize();
            actual_offset = (offset / ps) * ps;
            SG_ASSERT(offset >= actual_offset);
            extra = (SG_uint32) (offset - actual_offset);
            pmap->_private__mapped_size += extra;
        }

        if ((flags_sg & SG_FILE__ACCESS_MASK) == SG_FILE_RDONLY)
        {
            prot = PROT_READ;
        }
        else
        {
            prot = PROT_READ | PROT_WRITE;
        }

        // TODO make sure pmap->_private__mapped_size fits in size_t
        // TODO make sure offset fits in off_t
        pmap->_private__mapped_ptr = mmap(NULL, (size_t) pmap->_private__mapped_size, prot, MAP_SHARED, pFile->m_fd, (off_t) actual_offset);
        if (MAP_FAILED == pmap->_private__mapped_ptr)
        {
            SG_ERR_THROW(SG_ERR_ERRNO(errno));
        }
        pmap->mem = pmap->_private__mapped_ptr + extra;
    }
#endif

    SG_ASSERT(pmap->_private__mapped_ptr);
    SG_ASSERT(pmap->_private__mapped_size);
    SG_ASSERT(pmap->mem);

    *ppmap = pmap;
    pmap = NULL;

fail:
    // TODO free pmap
#if defined(WINDOWS)
	if (hMapping)
		CloseHandle(hMapping);
#endif
    ;
}

void SG_file__munmap(
        SG_context* pCtx, 
        SG_mmap** ppmap
        )
{
	SG_NULLARGCHECK_RETURN(ppmap);

#if defined(WINDOWS)
    UnmapViewOfFile((void*) (*ppmap)->_private__mapped_ptr);
#endif

#if defined(MAC) || defined(LINUX)
    munmap((void*) (*ppmap)->_private__mapped_ptr, (size_t) (*ppmap)->_private__mapped_size);
#endif

    SG_NULLFREE(pCtx, *ppmap);
}

void SG_mmap__get_ptr(
        SG_context* pCtx, 
        SG_mmap* pmap,
        SG_byte** pp
        )
{
    SG_UNUSED(pCtx);

    *pp = pmap->mem;
}

//////////////////////////////////////////////////////////////////

#define BUFFER_SIZE					4096

void SG_file__read_utf8_file_into_string(
	SG_context * pCtx,
	SG_file * pFile,
	SG_string * pString)
{
	SG_uint32 numRead;
	SG_byte buf[BUFFER_SIZE];

	// TODO 2011/02/16 we should just stat the file and get
	// TODO            the file length and allocate a large
	// TODO            enough buffer so that we can read it
	// TODO            all at once.
	// TODO
	// TODO            This routine is implicitly appending
	// TODO            the file content to the existing string
	// TODO            so that might cause some buffer juggling
	// TODO            issues, but I don't think any of the
	// TODO            current callers are using this 'append'
	// TODO            feature since they are passing a freshly
	// TODO            allocated string.
	// TODO
	// TODO            We might want to let this routine
	// TODO            allocate the string so that we can
	// TODO            control this better.

	while (1)
	{
		SG_file__read(pCtx, pFile, BUFFER_SIZE, buf, &numRead);
		if (SG_context__err_equals(pCtx, SG_ERR_EOF))
		{
			SG_context__err_reset(pCtx);
			break;
		}

		SG_ERR_CHECK_CURRENT;
		SG_ERR_CHECK( SG_string__append__buf_len(pCtx, pString, buf, numRead) );
	}
	return;

fail:
	SG_ERR_IGNORE(  SG_string__clear(pCtx, pString) );
}

void SG_file__read_into_string(
	SG_context* pCtx,
	const SG_pathname* pPathname,
	SG_string** ppString)
{
	SG_uint64 fileLen;
	SG_uint32 safeLen = 0;

	SG_file* pFile = NULL;
	SG_uint32 bytesRead = 0;
	
	SG_byte* pBuf = NULL;
	char* pszBuf = NULL;

	SG_STATIC_ASSERT(SG_STRING__MAX_LENGTH_IN_BYTES <= SG_UINT32_MAX);

	SG_NULLARGCHECK_RETURN(ppString);

	SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPathname, &fileLen, NULL)  );
	if (fileLen > SG_STRING__MAX_LENGTH_IN_BYTES)
		SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "The file is too big to read into a string"));
	safeLen = (SG_uint32)fileLen;

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathname, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile )  );
	SG_ERR_CHECK(  SG_malloc(pCtx, safeLen + 1, &pBuf)  );
    SG_ERR_CHECK(  SG_file__read(pCtx, pFile, safeLen, pBuf, &bytesRead)  );
    pBuf[safeLen] = 0;
	
#ifdef SG_IOS
	SG_ERR_CHECK(  SG_STRING__ALLOC__ADOPT_BUF(pCtx, ppString, (char**) &pBuf, SG_STRLEN((char*) pBuf))); 
#else
	SG_ERR_CHECK(  SG_utf8__import_buffer(pCtx, pBuf, safeLen, &pszBuf, NULL)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__ADOPT_BUF(pCtx, ppString, &pszBuf, SG_STRLEN(pszBuf))); 

	SG_NULLFREE(pCtx, pBuf);
#endif

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );	

	return;

fail:
	SG_NULLFREE(pCtx, pBuf);
	SG_NULLFREE(pCtx, pszBuf);
	SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );	
}

void SG_file__read__line__LF(
        SG_context* pCtx, 
        SG_file* pFile, 
        SG_string** ppstr
        )
{
    SG_byte buf[88];
    SG_uint32 bytes_read = 0;
    SG_byte* pb = NULL;
    SG_string* pstr = NULL;
    SG_uint64 initial_pos = 0;

    SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &initial_pos)  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );

    while (1)
    {
        SG_file__read(pCtx, pFile, sizeof(buf), buf, &bytes_read);
        if(SG_context__err_equals(pCtx, SG_ERR_EOF))
        {
            SG_context__err_reset(pCtx);
            *ppstr = NULL;
            goto fail;
        }
        else
        {
            SG_ERR_CHECK_CURRENT;
        }

        pb = buf;
        while (pb < (buf + bytes_read))
        {
            if (10 == *pb)
            {
                if ((pb - buf) > 0)
                {
                    // the LF should not be written to the result string
                    SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pstr, buf, (SG_uint32) (pb - buf))  );
                }
                goto done;
            }
            else
            {
                pb++;
            }
        }
        SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pstr, buf, sizeof(buf))  );
    }

done:
    // seek to the byte immediately after the LF
    SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, initial_pos + 1 + SG_string__length_in_bytes(pstr))  );

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_file__read_newline(
	SG_context* pCtx,
	SG_file*    pFile
	)
{
	SG_uint64 uPosition  = 0u;
	SG_byte   aBytes[2u];
	SG_uint32 uBytes     = 0u;
	SG_bool   bEOF       = SG_FALSE;

	SG_NULLARGCHECK(pFile);

	SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &uPosition)  );

	SG_ERR_TRY(  SG_file__read(pCtx, pFile, sizeof(aBytes), aBytes, &uBytes)  );
	SG_ERR_CATCH_SET(SG_ERR_EOF, bEOF, SG_TRUE);
	SG_ERR_CATCH_END;

	if (uBytes == 2u && (aBytes[0u] == '\r' && aBytes[1u] == '\n'))
	{
		uPosition += 2u;
	}
	else if (uBytes >= 1u && (aBytes[0u] == '\r' || aBytes[0u] == '\n'))
	{
		uPosition += 1u;
	}
	else if (bEOF == SG_FALSE)
	{
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Expected newline not found."));
	}

	SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, uPosition)  );

fail:
	return;
}

void SG_file__read_while(
	SG_context* pCtx,
	SG_file*    pFile,
	const char* szChars,
	SG_bool     bWhile,
	SG_string** ppWord,
	SG_int32*   pChar
	)
{
	SG_uint64  uPosition = 0u;       //< Position in the file we've read through.
	SG_string* sWord     = NULL;     //< Return value.
	SG_bool    bEOF      = SG_FALSE; //< Whether or not we found the end-of-file.
	SG_bool    bFound    = SG_FALSE; //< Whether or not we found something [not] in szChars.
	SG_byte    aBytes[80];           //< Buffer that we're reading into and processing.
	SG_uint32  uIndex    = 0u;       //< Currently interesting index into aBytes.
	SG_int32   iChar     = 0;        //< Current character we're examining.

	SG_NULLARGCHECK(pFile);
	SG_NULLARGCHECK(szChars);

	// start at the file's current position
	SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &uPosition)  );

	// if they want the string we read, allocate an empty one to start
	if (ppWord != NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sWord)  );
	}

	// keep going until we find the EOF or something [not] in szChars
	while (bEOF == SG_FALSE && bFound == SG_FALSE)
	{
		SG_uint32 uBytes = 0u; //< Effective size of aBytes.

		// fill aBytes from the file, starting at uIndex
		// We might have left ourselves uIndex number of bytes at the beginning
		// of the buffer that still require processing.  That'd happen if a multi-
		// byte character got split across buffer boundaries.
		SG_ERR_TRY(  SG_file__read(pCtx, pFile, sizeof(aBytes) - uIndex, aBytes + uIndex, &uBytes)  );
		SG_ERR_CATCH_SET(SG_ERR_EOF, bEOF, SG_TRUE);
		SG_ERR_CATCH_END;

		// if we didn't find the EOF, process the buffer
		if (bEOF == SG_FALSE)
		{
			SG_uint32 uNextIndex = 0u; //< Index of the character after the current one.

			// start processing at the beginning of the buffer
			uBytes += uIndex;
			uIndex = 0u;

			// loop over each UTF8 character in the buffer
			// leave uIndex indicating the character we stopped at
			SG_ERR_CHECK(  SG_utf8__next_char(pCtx, aBytes, uBytes, uIndex, &iChar, &uNextIndex)  );
			while (bFound == SG_FALSE && iChar >= 0 && uIndex < uBytes)
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
					uIndex = uNextIndex;
					SG_ERR_CHECK(  SG_utf8__next_char(pCtx, aBytes, uBytes, uIndex, &iChar, &uNextIndex)  );
				}
			}
			SG_ASSERT(uIndex <= uBytes);

			// append everything we've read so far onto the return value, if they care about it
			if (sWord != NULL && uIndex > 0u)
			{
				SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, sWord, aBytes, uIndex)  );
			}

			// keep track of where in the file we've processed up to
			uPosition += uIndex;

			// if we haven't found what we're looking for yet, prepare to read another buffer
			if (bFound == SG_FALSE)
			{
				// we should either be at the end of the buffer, or have a negative
				// (invalid) char, which would indicate that we chopped a multi-byte
				// character at the end of the buffer
				SG_ASSERT(uIndex == uBytes || iChar < 0);

				// if there are unprocessed characters at the end of the buffer, copy
				// them to the beginning of the buffer and we'll process them next
				// time; leave uIndex indicating how many such characters there are
				if (uIndex < uBytes)
				{
					memcpy(aBytes, aBytes + uIndex, uBytes - uIndex);
				}
				uIndex = uBytes - uIndex;
			}
		}
	}
	SG_ASSERT(bEOF != bFound);

	// if we didn't hit the end of the file, seek to the end of where we processed
	if (bEOF == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, uPosition)  );
	}

	// return whichever values the caller wanted
	if (ppWord != NULL)
	{
		*ppWord = sWord;
		sWord = NULL;
	}
	if (pChar != NULL && bFound != SG_FALSE)
	{
		*pChar = iChar;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sWord);
	return;
}

void SG_file__text__write(
	SG_context* pCtx,
	const char* pszPath,
	const char* pszText)
{
#if defined(WINDOWS)

	/**
	 * On Windows, console redirection (using >) does line ending translation.
	 * If you want the same effect writing to a file directly (e.g. "vv config export")
	 * you have to open the file as a text mode stream and writing to that.
	 */

	WCHAR* pwszPath = NULL;
	WCHAR* pwszText = NULL;
	FILE* pf = NULL;
	errno_t open_err = 0;
	int written = 0;

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszPath, &pwszPath, NULL)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, pszText, &pwszText, NULL)  );

	open_err = _wfopen_s(&pf, pwszPath, L"wt"); // w= write; t = text mode
	if (open_err)
		SG_ERR_THROW(open_err);

	written = fwprintf_s(pf, pwszText);
	if (written < 0)
		SG_ERR_THROW(errno);

fail:
	SG_NULLFREE(pCtx, pwszPath);
	SG_NULLFREE(pCtx, pwszText);
	if (pf)
		fclose(pf);

#else

	SG_pathname* pPath = NULL;
	SG_file* pFile = NULL;

	SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath, pszPath)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY | SG_FILE_OPEN_OR_CREATE | SG_FILE_TRUNC, 0777, &pFile)  );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, pszText)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_FILE_NULLCLOSE(pCtx, pFile);

#endif
}
