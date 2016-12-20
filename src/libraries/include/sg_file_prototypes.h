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

// sg_file.h
// Declarations for the file i/o portability layer.
// This module implements unbuffered IO.
//////////////////////////////////////////////////////////////////

#ifndef H_SG_FILE_PROTOTYPES_H
#define H_SG_FILE_PROTOTYPES_H

BEGIN_EXTERN_C

// NOTE: we do not support O_APPEND because (on Linux/Mac)
// NOTE: it causes a seek to the end before *EACH* write.
// NOTE: windows does not support auto-seeking files (when
// NOTE: we're using the low-level routines).  this may not
// NOTE: matter since we'll probably not be writing to files
// NOTE: shared by others, but still we shouldn't give the
// NOTE: illusion that we support this feature.  the caller
// NOTE: should call SG_file_seek_end() after opening the file.

//////////////////////////////////////////////////////////////////

// in SG_file__open(), perms are only used when we actually create a new file.  (see open(2)).

void SG_file__open__pathname(SG_context*, const SG_pathname * pPathname, SG_file_flags flags, SG_fsobj_perms perms, SG_file** pResult);

// Couldn't get this to work on Windows.
#if defined(MAC) || defined(LINUX)
// Open '/dev/null' on unix or 'NUL:' on windows.
// flags will be ORed with SG_FILE_OPEN_EXISTING, so you only need to specify the SG_FILE__ACCESS_MASK part.
void SG_file__open__null_device(SG_context*, SG_file_flags flags, SG_file** pResult);
#endif

void SG_file__seek(SG_context*, SG_file* pFile, SG_uint64 iPos);  // seek absolute from beginning
void SG_file__seek_end(SG_context*, SG_file* pFile, SG_uint64 * piPos);  // seek to end, return position

void SG_file__truncate(SG_context*, SG_file* pFile);

void SG_file__tell(SG_context*, SG_file* pFile, SG_uint64* piPos);

void SG_file__read(SG_context*, SG_file* pFile, SG_uint32 iNumBytesWanted, SG_byte* pBytes, SG_uint32* piNumBytesRetrieved /*optional*/);
void SG_file__read__line__LF(SG_context*, SG_file* pFile, SG_string** ppstr);
void SG_file__eof(SG_context*, SG_file* pFile, SG_bool * pEof);
void SG_file__write(SG_context*, SG_file* pFile, SG_uint32 iNumBytes, const SG_byte* pBytes, SG_uint32 * piNumBytesWritten /*optional*/);
SG_DECLARE_PRINTF_PROTOTYPE( void SG_file__write__format(SG_context*, SG_file* pFile, const char * szFormat, ...), 3, 4);
void SG_file__write__vformat(SG_context*, SG_file* pFile, const char * szFormat, va_list);
void SG_file__write__sz(SG_context*, SG_file* pFile, const char *);
void SG_file__write__string(SG_context*, SG_file * pFile, const SG_string *);

/**
 * Reads a single newline (of any platform-style) from a file.
 * Throws an error if the file's current position isn't a newline,
 * unless it's at EOF, in which case this function does nothing.
 */
void SG_file__read_newline(
	SG_context* pCtx, //< [in] [out] Error and context info.
	SG_file*    pFile //< [in] File to read a newline from.
	);

/**
 * Reads while/until a specific set of characters is found.
 *
 * The bWhile flag here is a little tricky.  When it's true, the function reads as
 * many characters as it can, as long as they are all in the specified set (szChars).
 * Think of this case as "keep reading as long as we find any of these characters".
 * In this case, the returned string must by definition contain only characters in
 * szChars.  For example, to read a number you could call it like this:
 * SG_file__read_while(pCtx, pFile, "1234567890", SG_TRUE, &sNumber, NULL)
 *
 * When bWhile is false, the function reads as many characters as it can, as long as
 * none of them are in the specified set (szChars).  Think of this case as "keep
 * reading until you find any of these characters".  In this case, the returned
 * string must by definition not contain any characters in szChars.  For example, to
 * read the rest of the current line, you could call it like this:
 * SG_file__read_while(pCtx, pFile, "\r\n", SG_FALSE, &sLine, NULL)
 *
 * This function will always stop at EOF without throwing an error.  If the file is
 * already at EOF when calling this function, it does nothing and returns an empty
 * string.
 */
void SG_file__read_while(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_file*    pFile,   //< [in] File to read from.
	const char* szChars, //< [in] The set of characters to compare against.
	SG_bool     bWhile,  //< [in] If true, read WHILE current char is in szChars.
	                     //<      If false, read UNTIL current char is in szChars.
	SG_string** ppWord,  //< [out] Data that was read.
	                     //<       May be NULL if not needed.
	SG_int32*   pChar    //< [out] The first character that didn't meet criteria,
	                     //<       and so stopped the reading.
	                     //<       Unchanged/ignored if EOF was found first.
	                     //<       May be NULL if not needed.
	);

/**
 * Close the OS file associated with the SG_file handle and free
 * the SG_file handle.
 *
 * THIS MAY RETURN AN ERROR CODE IF THE OS CLOSE FAILS.  THIS IS
 * UNLIKELY, BUT IT DOES HAPPEN.  See "man close(2)" for details.
 *
 * Frees the file handle and nulls the caller's copy.
 */
void SG_file__close(SG_context*, SG_file** ppFile);

void SG_file__fsync(SG_context*, SG_file* pFile);

//////////////////////////////////////////////////////////////////

/**
 * Read the entire contents of the given file into the given string.
 * We assume the file is opened for reading.  We seek(0) and read
 * and leave the file position undefined (er, at the bottom if all
 * goes well).
 *
 * No attempt is made to determine the file's encoding, so it must
 * be UTF-8.
 *
 * TODO 2011/02/16 This was extracted/refactored from sg_jsglue.c:
 * TODO            _read_file_into_string().  It was written for
 * TODO            collecting stdout/stderr from a sub-process and
 * TODO            returning the output to the JS script.  If the
 * TODO            output is huge, this will maybe cause us to crash.
 * TODO            Think about maybe putting a limit on the amount of
 * TODO            output the caller should receive. (the first or
 * TODO            last n bytes.)
 */
void SG_file__read_utf8_file_into_string(SG_context * pCtx,
									SG_file * pFile,
									SG_string * pString);

/**
 * Read a file into a new SG_string.
 * We attempt to determine the file's encoding and return only a valid UTF-8 string.
 */
void SG_file__read_into_string(
	SG_context* pCtx,
	const SG_pathname* pPathname,
	SG_string** ppString);

/**
 * On Windows, console redirection (using >) does line ending translation.
 * If you want the same effect writing to a file directly (e.g. "vv config export")
 * you should call this routine rather than SG_file__open__pathname and SG_file__write__sz.
 * (Note that on Mac and Linux, this routine simply calls SG_file__open__pathname and SG_file__write__sz.)
 */
void SG_file__text__write(
	SG_context* pCtx,
	const char* pszPath,
	const char* pszText);

//////////////////////////////////////////////////////////////////

/**
 * Close the given file and null the pointer.
 *
 * This ignores any error codes from the close
 * WHICH MAY INTRODUCE A LEAK/ERROR.
 */
#define SG_FILE_NULLCLOSE(_pCtx, _pFile) SG_STATEMENT(\
	if(_pFile!=NULL)\
	{\
		SG_context__push_level(_pCtx);\
		SG_file__close(_pCtx, &_pFile);\
		SG_context__pop_level(_pCtx);\
		_pFile = NULL;\
	})

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
/**
 * Platform-dependent routine to get the FD inside the SG_file.
 *
 * Don't use this unless you ***REALLY*** need to.  This was
 * added to allow for stupid FD dup/dup2 tricks in the child
 * process in a fork/exec.
 *
 * You DO NOT own the FD file handle, so don't close() it directly
 * or try to do any fcntl() operations on it.
 */
void SG_file__get_fd(SG_context*, SG_file * pFile, int * pfd);
#endif

#if defined(WINDOWS)
/**
 * Platform-dependent routine to get the Windows File Handle
 * inside the SG_file.
 *
 * Don't use this unless you ***REALLY*** need to.  This was
 * added to allow for stupid DuplicateHandle/Inherit-Handle
 * tricks when calling CreateProcess().
 *
 * You DO NOT own the File Handle returned.
 */
void SG_file__get_handle(SG_context*, SG_file * pFile, HANDLE * phFile);
#endif

//////////////////////////////////////////////////////////////////

void SG_file__mmap(
        SG_context* pCtx, 
        SG_file* pFile,
        SG_uint64 offset,
        SG_uint64 len,
        SG_file_flags flags_sg,
        SG_mmap** ppmap
        );

void SG_file__munmap(
        SG_context* pCtx, 
        SG_mmap** ppmap
        );

void SG_mmap__get_ptr(
        SG_context* pCtx, 
        SG_mmap* pmap,
        SG_byte** pp
        );

END_EXTERN_C

#endif//H_SG_FILE_PROTOTYPES_H

