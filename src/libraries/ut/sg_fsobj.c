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

// sg_fsobj.c
// manipulate file system objects.
//////////////////////////////////////////////////////////////////

#include <sg.h>

#if defined(WINDOWS)
#include <tchar.h>
#include <shellapi.h>
#endif

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
#define DO_UNIX_BASED_STAT
#endif

#if defined(DO_UNIX_BASED_STAT)
/**
 * Do a unix-based stat() or lstat() and convert the results into
 * our platform-neutral SG_fsobj_stat structure.
 *
 * all unix-based platforms seem to define multiple versions of 'struct stat'
 * for 32 vs 64 bits.  sometimes you have to explicitly name the
 * version you want and sometimes you have to define things on the
 * compiler command line (like LFS stuff).
 *
 * we want a version of struct stat that defines 64bit file sizes.
 *
 * we want a version that gives us (or lets us compute) microseconds
 * since EPOCH. It's not critical that this be high-resolution.
 */

#ifdef MAC
/**
 * TODO it turns out that the 'stat64' stuff first appears in Leopard (10.5).
 * TODO Tiger (10.4) only defines 'stat'.  i have SG_ASSERT(sizeof(st.st_size)==8)
 * TODO in various places to try to catch problems here.
 * TODO
 * TODO we should investigate this further later.
 *
 * typedef struct stat64		T_stat;
 * #define FN_LSTAT				lstat64
 */
typedef struct stat				T_stat;
#	define FN_LSTAT				lstat
#endif//MAC

#ifdef LINUX
/**
 * TODO likewise, we want to make sure that we the 64-bit LFS version.
 */
typedef struct stat				T_stat;
#	define FN_LSTAT				lstat
#endif//LINUX

/**
 * call unix stat() and normalize the error code.
 */
static void _my_call_unix_stat(SG_context * pCtx, const SG_pathname * pPathname, T_stat * pST)
{
	int result;
	char * pBufOS = NULL;

	// ensure that the version of "stat" that we're using supports 64-bit files.
	SG_ASSERT(sizeof(pST->st_size) == 8);

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );
	SG_NULLARGCHECK_RETURN(pST);

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname),&pBufOS)  );

	result = FN_LSTAT(pBufOS,pST);
	if (result == -1)
		SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
						(pCtx,"Calling stat() on '%s'",SG_pathname__sz(pPathname))  );	// use utf8 version for error message

	SG_NULLFREE(pCtx, pBufOS);

	return;
fail:
	SG_NULLFREE(pCtx, pBufOS);
}

/**
 * S_ISREG() and friends are not consitently defined on all platforms.
 */
#if defined(MAC) || defined(LINUX)
#define MY_ST_IS_REG(m)		(((m) & S_IFMT) == S_IFREG)
#define MY_ST_IS_DIR(m)		(((m) & S_IFMT) == S_IFDIR)
#define MY_ST_IS_SLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#endif

/**
 * extract object type from OS stat buffer and convert to normalized type.
 */
static void _my_unix_map_type(
	SG_context * pCtx,
	const T_stat * pST,
	SG_fsobj_stat * pFsObjStat
	)
{
	SG_NULLARGCHECK_RETURN(pST);
	SG_NULLARGCHECK_RETURN(pFsObjStat);

	if (MY_ST_IS_DIR(pST->st_mode))
	{
		pFsObjStat->type = SG_FSOBJ_TYPE__DIRECTORY;
		return;
	}

	if (MY_ST_IS_REG(pST->st_mode))
	{
		pFsObjStat->type = SG_FSOBJ_TYPE__REGULAR;
		return;
	}

	if (MY_ST_IS_SLNK(pST->st_mode))
	{
		pFsObjStat->type = SG_FSOBJ_TYPE__SYMLINK;
		return;
	}

	// S_IFIFO, S_FCHR, S_FBLK, S_FSOCK, S_FWHT
	pFsObjStat->type = SG_FSOBJ_TYPE__DEVICE;
}

/**
 * extract mode bits from OS stat buffer and convert to normalized value.
 */
static void _my_unix_map_mode(
	SG_context * pCtx,
	const T_stat * pST,
	SG_fsobj_stat * pFsObjStat
	)
{
	SG_NULLARGCHECK_RETURN(pST);
	SG_NULLARGCHECK_RETURN(pFsObjStat);

	// we only want the mode bits in 0777.  we DO NOT want
	// any of the SUID/SGID and friends.

	pFsObjStat->perms = (pST->st_mode & SG_FSOBJ_PERMS__MASK);
}

/**
 * extract the length from the OS stat buffer and convert to normalized value.
 */
static void _my_unix_map_length(
	SG_context * pCtx,
	const T_stat * pST,
	SG_fsobj_stat * pFsObjStat
	)
{
	SG_ASSERT(sizeof(pST->st_size) == sizeof(pFsObjStat->size));

	SG_NULLARGCHECK_RETURN(pST);
	SG_NULLARGCHECK_RETURN(pFsObjStat);

	if (MY_ST_IS_REG(pST->st_mode))		// a regular file is easy
	{
		pFsObjStat->size = pST->st_size;
		return;
	}

	if (MY_ST_IS_SLNK(pST->st_mode))	// for symlink, get the length of the link not the referenced object
	{
		pFsObjStat->size = pST->st_size;
		return;
	}

	// lie about directories and anything else.

	pFsObjStat->size = 0;
}

/**
 * extract modtime from the OS stat buffer and convert to normalized value.
 * this is a microsecond-since-epoch value.
 *
 * all platforms seem to report sime somewhat differently, so we limit the
 * places where we actually use the modtime.
 */
static void _my_unix_map_mtime(
	SG_context * pCtx,
	const T_stat * pST,
	SG_fsobj_stat * pFsObjStat
	)
{
	SG_NULLARGCHECK_RETURN(pST);
	SG_NULLARGCHECK_RETURN(pFsObjStat);

#if defined(MAC)
	/**
	 * MAC has stat buffer with:
	 *     struct timespec st_mtimespec;
	 * which has:
	 *     __darwin_time_t tv_sec;
	 *     long tv_nsec;
	 * They have an ifdef'd define of
	 *     st_mtime as st_mtimespec.tv_sec
	 *
	 * tv_nsec may or may not be zero.  this depends upon whether the underlying
	 * filesystem supports sub-second resolution.  Currently (2010/05/27 with OS
	 * X 10.6.3 with HFS+) tv_nsec is always zero.
	 */

	pFsObjStat->mtime_ms =
		(SG_int64)(  ((SG_uint64)pST->st_mtimespec.tv_sec * 1000)
				   + ((SG_uint64)pST->st_mtimespec.tv_nsec / 1000000) );

#if defined(SG_BUILD_FLAG_FEATURE_NS)
	pFsObjStat->mtime_ns =
		(SG_int64)(  ((SG_uint64)pST->st_mtimespec.tv_sec * 1000000000)
				   + ((SG_uint64)pST->st_mtimespec.tv_nsec) );
#endif

#endif
#if defined(LINUX)
	/**
	 * Linux stat buffer has either:
	 *     __time_t st_mtime;				// seconds
	 *     unsigned long int st_mtimensec;	// nano-seconds
	 * or (when __USE_MISC is defined):
	 *     struct timespec st_mtim;
	 * which has an equivalent set of fields.
	 *
	 * Likewise, tv_nsec may or may not be zero depending upon the filesystem.
	 * For example, currently (2010/05/27) Ext4 supports sub-second resolution
	 * but Ext3 does not.
	 */

	pFsObjStat->mtime_ms =
		(SG_int64)(  ((SG_uint64)pST->st_mtim.tv_sec * 1000)
				   + ((SG_uint64)pST->st_mtim.tv_nsec / 1000000) );

#if defined(SG_BUILD_FLAG_FEATURE_NS)
	pFsObjStat->mtime_ns =
		(SG_int64)(  ((SG_uint64)pST->st_mtim.tv_sec * 1000000000)
				   + ((SG_uint64)pST->st_mtim.tv_nsec) );
#endif

#endif
}

/**
 * normalized stat() for unix-based systems.
 */
void SG_fsobj__stat__pathname(
	SG_context * pCtx,
	const SG_pathname * pPathname,
	SG_fsobj_stat * pFsObjStat
	)
{
	// TODO should we have SG_fsobj__stat__pathname() return
	// TODO a normalized error for file/path not found?
	// TODO see discussion in SG_fsobj__exists__pathname().

	T_stat st;

	SG_NULLARGCHECK_RETURN(pPathname);
	SG_NULLARGCHECK_RETURN(pFsObjStat);

	SG_ERR_CHECK_RETURN(  _my_call_unix_stat(pCtx,pPathname,&st)  );

	// TODO the following violates the guidelines in that we modify
	// *pFsObjStat before we know that every one of these steps will
	// succeed.  ideally, we should compute all this in a local
	// SG_fsobj_stat and if everything is ok, copy it to the caller's
	// buffer.

	SG_ERR_CHECK_RETURN(  _my_unix_map_type(pCtx, &st,pFsObjStat)  );
	SG_ERR_CHECK_RETURN(  _my_unix_map_mode(pCtx, &st,pFsObjStat)  );
	SG_ERR_CHECK_RETURN(  _my_unix_map_length(pCtx, &st,pFsObjStat)  );
	SG_ERR_CHECK_RETURN(  _my_unix_map_mtime(pCtx, &st,pFsObjStat)  );
}
#endif//DO_UNIX_BASED_STAT

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
static void _my_call_windows_stat(
	SG_context * pCtx,
	const SG_pathname * pPathname,
	SG_fsobj_stat * pFsObjStat
	)
{
	wchar_t * pBuf = NULL;
	SG_uint32 lenBuf;
	WIN32_FILE_ATTRIBUTE_DATA data;
	BOOL bResult;
	
	SG_uint32 k;
	SG_error err = 0;

	SG_ERR_CHECK_RETURN(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,&lenBuf)  );	// we must free pBuf

	/* Hacky wait loop to cope when the file is in use (e.g. B2596) */
#define NR_ATTEMPTS                      10
#define DELAY_BETWEEN_ATTEMPTS_IN_MS     10

	for (k=0; k<NR_ATTEMPTS; k++)
	{
		bResult = GetFileAttributesExW(pBuf,GetFileExInfoStandard,&data);
		if (bResult)
			goto done;

		err = SG_ERR_GETLASTERROR(GetLastError());
		if ((err == SG_ERR_GETLASTERROR(ERROR_ACCESS_DENIED)) && (k+1<NR_ATTEMPTS))
			SG_sleep_ms(DELAY_BETWEEN_ATTEMPTS_IN_MS);
		else
			break;
	}

	SG_ERR_THROW2(  err,
					(pCtx,"Calling stat() on '%s'", SG_pathname__sz(pPathname))  );	// use utf8 version for error message

done:
	SG_ERR_CHECK(  SG_fsobj__file_attribute_data_to_stat(pCtx, &data,pFsObjStat)  );
	SG_NULLFREE(pCtx, pBuf);

	return;
fail:
	SG_NULLFREE(pCtx, pBuf);
}

/**
 * Do something equivalent to stat() or lstat() on a Windows-based
 * system and map the results into our platform-neutral SG_fsobj_stat
 * structure.
 *
 * WE CANNOT USE THE POSIX-COMPATIBILITY stat() ROUTINES because
 * they do not handle pathnames with "\\?\" prefixes.
 *
 * So we have to use the FindFirstFile and friends.
 *
 */
void SG_fsobj__stat__pathname(
	SG_context * pCtx,
	const SG_pathname * pPathname,
	SG_fsobj_stat * pFsObjStat
	)
{
	// TODO should we have SG_fsobj__stat__pathname() return
	// TODO a normalized error for file/path not found?
	// TODO see discussion in SG_fsobj__exists__pathname().

	SG_NULLARGCHECK_RETURN(pPathname);
	SG_NULLARGCHECK_RETURN(pFsObjStat);

	SG_ERR_CHECK_RETURN(  _my_call_windows_stat(pCtx, pPathname,pFsObjStat)  );
}
#endif//WINDOWS

//////////////////////////////////////////////////////////////////

void SG_fsobj__stat__pathname_sz(
	SG_context * pCtx,
	const SG_pathname * pPathnameDirectory,
	const char * szFileName,
	SG_fsobj_stat * pFsStat
	)
{
	// given a pathname to <directory> and a file or subdir name,
	// create combined pathname "<directory>/<filename>" and stat() it.

	SG_pathname * pPathnameTemp = NULL;

	SG_NULLARGCHECK_RETURN(pFsStat); // other args are validated by SG_pathname__alloc__pathname_sz()

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathnameTemp,pPathnameDirectory,szFileName)  );
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPathnameTemp,pFsStat)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathnameTemp);

	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameTemp);
}

//////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
void SG_fsobj__find_data_to_stat(SG_context * pCtx, WIN32_FIND_DATAW * pData, SG_fsobj_stat * pFsStat)
{
	//////////////////////////////////////////////////////////////////
	// WARNING: SG_fsobj__file_attribute_data_to_stat() and SG_fsobj__find_data_to_stat()
	// WARNING: are identical in effect, but are different functions because Windows uses
	// WARNING: 2 different structures - even though the fields they have in common have
	// WARNING: the same names.  if you make a change to one of them, consider changing
	// WARNING: the other too.
	//////////////////////////////////////////////////////////////////

	// manually convert WIN32_FIND_DATAW info (from Find{First,Next}File()) into
	// our platform-neutral SG_fsobj_stat structure.

	SG_NULLARGCHECK_RETURN(pData);
	SG_NULLARGCHECK_RETURN(pFsStat);

	// for permissions, Windows only has a READONLY bit.  everything in the 0777 mask
	// is something of a lie.  (and yes, we are completely ignoring ACLs.)
	// the value we compute here may differ from what the Window CRT stat() reports
	// because it lies (especially WRT the 'x' bit).
	//
	// you should probably use the __equivalent_perms() routine rather than testing
	// for exact matches.

	if (pData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		pFsStat->perms = 0444;
	else
		pFsStat->perms = 0666;

	// for types, we understand files and directories.
	// we only set the size for files.
	//
	// TODO 2012/11/16 We currently DO NOT support symlinks on Windows.
	// TODO            They are new and a type of "reparse point".
	// TODO            When we do choose to support them, we will have
	// TODO            to distinguish between them and other/generic
	// TODO            reparse points.  And we will need to treat them
	// TODO            as a different type than a Linux/Mac symlink
	// TODO            because they have a bIsDirectory bit that makes
	// TODO            them behave differently.

	if (pData->dwFileAttributes & (FILE_ATTRIBUTE_DEVICE
								   |FILE_ATTRIBUTE_REPARSE_POINT))
	{
		// According to the "mklink" command, there are:
		//    [] file symlinks      : mklink link target
		//    [] directory symlinks : mklink /D link target
		//    [] directory junction : mklink /J link target
		// which appear as reparse-points.  We do not currently
		// support them (2.5).
		//
		// There are also hard-links ("mklink /H link target")
		// which are something else and not handled here.

		pFsStat->type = SG_FSOBJ_TYPE__DEVICE;
		pFsStat->size = 0;
	}
	else if (pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		pFsStat->type = SG_FSOBJ_TYPE__DIRECTORY;
		pFsStat->size = 0;
	}
	else
	{
		pFsStat->type = SG_FSOBJ_TYPE__REGULAR;
		pFsStat->size = (((SG_uint64)pData->nFileSizeHigh) << 32) + ((SG_uint64)pData->nFileSizeLow);
	}

	// compute mod time.  on Windows, the "last write time" may be
	// the mod-time, the creation-time, or zero depending upon the
	// type of the underlying filesystem.
	//
	// WARNING: The resolution of ftLastWriteTime is reported to us as the
	// WARNING: number of 100-nanosecond intervals since Jan 1, 1601.
	// WARNING: ***BUT*** this accuracy of this value is dependent upon the
	// WARNING: the underlying filesystem.  So on an NTFS partition, this
	// WARNING: may actually be in 100-ns precision, but on a FAT partition
	// WARNING: this may be more like 2-sec precision.

	SG_ERR_CHECK_RETURN(  SG_time__get_milliseconds_since_1970_utc__ft(pCtx, &pFsStat->mtime_ms,
																	   &pData->ftLastWriteTime)  );

#if defined(SG_BUILD_FLAG_FEATURE_NS)
	SG_ERR_CHECK_RETURN(  SG_time__get_nanoseconds_since_1970_utc__ft(pCtx, &pFsStat->mtime_ns,
																	  &pData->ftLastWriteTime)  );
#endif
}

void SG_fsobj__file_attribute_data_to_stat(SG_context * pCtx, WIN32_FILE_ATTRIBUTE_DATA * pData, SG_fsobj_stat * pFsStat)
{
	//////////////////////////////////////////////////////////////////
	// WARNING: SG_fsobj__file_attribute_data_to_stat() and SG_fsobj__find_data_to_stat()
	// WARNING: are identical in effect, but are different functions because Windows uses
	// WARNING: 2 different structures - even though the fields they have in common have
	// WARNING: the same names.  if you make a change to one of them, consider changing
	// WARNING: the other too.
	//////////////////////////////////////////////////////////////////

	// manually convert WIN32_FILE_ATTRIBUTE_DATA info (from GetFileAttributesEx()) into
	// our platform-neutral SG_fsobj_stat structure.

	SG_NULLARGCHECK_RETURN(pData);
	SG_NULLARGCHECK_RETURN(pFsStat);

	// for permissions, Windows only has a READONLY bit.  everything in the 0777 mask
	// is something of a lie.  (and yes, we are completely ignoring ACLs.)
	// the value we compute here may differ from what the Window CRT stat() reports
	// because it lies (especially WRT the 'x' bit).
	//
	// you should probably use the __equivalent_perms() routine rather than testing
	// for exact matches.

	if (pData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		pFsStat->perms = 0444;
	else
		pFsStat->perms = 0666;

	// for types, we understand files and directories.
	// we only set the size for files.
	//
	// TODO 2012/11/16 We currently DO NOT support symlinks on Windows.
	// TODO            They are new and a type of "reparse point".
	// TODO            When we do choose to support them, we will have
	// TODO            to distinguish between them and other/generic
	// TODO            reparse points.  And we will need to treat them
	// TODO            as a different type than a Linux/Mac symlink
	// TODO            because they have a bIsDirectory bit that makes
	// TODO            them behave differently.

	if (pData->dwFileAttributes & (FILE_ATTRIBUTE_DEVICE
								   |FILE_ATTRIBUTE_REPARSE_POINT))
	{
		pFsStat->type = SG_FSOBJ_TYPE__DEVICE;
		pFsStat->size = 0;
	}
	else if (pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		pFsStat->type = SG_FSOBJ_TYPE__DIRECTORY;
		pFsStat->size = 0;
	}
	else
	{
		pFsStat->type = SG_FSOBJ_TYPE__REGULAR;
		pFsStat->size = (((SG_uint64)pData->nFileSizeHigh) << 32) + ((SG_uint64)pData->nFileSizeLow);
	}

	// compute mod time.  on Windows, the "last write time" may be
	// the mod-time, the creation-time, or zero depending upon the
	// type of the underlying filesystem.
	//
	// WARNING: The resolution of ftLastWriteTime is reported to us as the
	// WARNING: number of 100-nanosecond intervals since Jan 1, 1601.
	// WARNING: ***BUT*** this accuracy of this value is dependent upon the
	// WARNING: the underlying filesystem.  So on an NTFS partition, this
	// WARNING: may actually be in 100-ns precision, but on a FAT partition
	// WARNING: this may be more like 2-sec precision.

	SG_ERR_CHECK_RETURN(  SG_time__get_milliseconds_since_1970_utc__ft(pCtx, &pFsStat->mtime_ms,
																	   &pData->ftLastWriteTime)  );

#if defined(SG_BUILD_FLAG_FEATURE_NS)
	SG_ERR_CHECK_RETURN(  SG_time__get_nanoseconds_since_1970_utc__ft(pCtx, &pFsStat->mtime_ns,
																	  &pData->ftLastWriteTime)  );
#endif
}
#endif

//////////////////////////////////////////////////////////////////

#if defined(LINUX) || defined(MAC)
void SG_fsobj__symlink(SG_context * pCtx, const SG_string* pstrLinkTo, const SG_pathname* pPathOfLink)
{
	int res;
	char * pBufOS_LinkTo = NULL;
	char * pBufOS_PathOfLink = NULL;

	SG_NULLARGCHECK_RETURN(pstrLinkTo);
	SG_NULLARGCHECK_RETURN(pPathOfLink);

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathOfLink),&pBufOS_PathOfLink)  );
	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_string__sz(pstrLinkTo),   &pBufOS_LinkTo)  );

	res = symlink(pBufOS_LinkTo, pBufOS_PathOfLink);
	if (res < 0)
	{
		SG_ERR_THROW2(   SG_ERR_ERRNO(errno),
						 (pCtx,"Calling symlink() on '%s' --> '%s'",	// use utf8 version of path/string
						  SG_pathname__sz(pPathOfLink),SG_string__sz(pstrLinkTo))  );
	}

	SG_NULLFREE(pCtx, pBufOS_LinkTo);
	SG_NULLFREE(pCtx, pBufOS_PathOfLink);

	return;
fail:
	SG_NULLFREE(pCtx, pBufOS_LinkTo);
	SG_NULLFREE(pCtx, pBufOS_PathOfLink);
}

void SG_fsobj__readlink(SG_context * pCtx, const SG_pathname* pPath, SG_string** ppResult)
{
	char * pBufOS_Path = NULL;
	char * pBufOS_Link = NULL;
	ssize_t res;
	SG_string* pstr = NULL;
	SG_uint64 lenLink64 = 0;
	SG_uint32 lenLink32;
	SG_fsobj_type type = SG_FSOBJ_TYPE__UNSPECIFIED;

	SG_NULLARGCHECK_RETURN(pPath);
	SG_NULLARGCHECK_RETURN(ppResult);

	// fetch the length of the contents of the symlink (the path fragment itself)
	// and allocate a buffer to hold the contents of the link.  (i don't think we
	// can use PATH_MAX any more.)

	SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath,&lenLink64,&type)  );
	if (type != SG_FSOBJ_TYPE__SYMLINK)
		SG_ERR_THROW2(  SG_ERR_ERRNO(EINVAL),
						(pCtx,"Calling readlink() on '%s'",SG_pathname__sz(pPath))  );

	lenLink32 = (SG_uint32)lenLink64;
	SG_ERR_CHECK(  SG_alloc(pCtx, 1,lenLink32+2,&pBufOS_Link)  );

	// convert the pathname from UTF-8 to user's locale if appropriate and use
	// it to read the contents of the link.

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPath), &pBufOS_Path)  );
	res = readlink(pBufOS_Path, pBufOS_Link, lenLink32+1);
	if (res < 0)
	{
		SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
						(pCtx,"calling readlink() on '%s'",SG_pathname__sz(pPath))  );
	}

	// convert the contents of the link from the user's locale into UTF-8.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
	SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__sz(pCtx, pstr,pBufOS_Link)  );

	SG_NULLFREE(pCtx, pBufOS_Link);
	SG_NULLFREE(pCtx, pBufOS_Path);

	*ppResult = pstr;

	return;
fail:
	SG_NULLFREE(pCtx, pBufOS_Link);
	SG_NULLFREE(pCtx, pBufOS_Path);
	SG_STRING_NULLFREE(pCtx, pstr);
}
#endif

#if defined(WINDOWS)
/* TODO explore whether we can/should support symlinks on
 * Windows Vista, which apparently adds this feature.
 * Problem:  apparently on vista, only administrators
 * can create symlinks. */

void SG_fsobj__symlink(SG_context * pCtx, const SG_string* pstrLinkTo, const SG_pathname* pPathOfLink)
{
	SG_ERR_THROW2_RETURN(  SG_ERR_SYMLINK_NOT_SUPPORTED,
						   (pCtx,"Calling symlink() on '%s' --> '%s'",	// use utf8 version of path/string
							SG_pathname__sz(pPathOfLink),SG_string__sz(pstrLinkTo))  );
}

void SG_fsobj__readlink(SG_context * pCtx, const SG_pathname* pPath, SG_string** ppResult)
{
	SG_UNUSED(ppResult);

	SG_ERR_THROW2_RETURN(  SG_ERR_SYMLINK_NOT_SUPPORTED,
						   (pCtx,"Calling readlink() on '%s'",SG_pathname__sz(pPath))  );
}
#endif

//////////////////////////////////////////////////////////////////

void SG_fsobj__length__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_uint64* piResult, SG_fsobj_type * pFsObjType)
{
	SG_fsobj_stat fsStat;

	SG_NULLARGCHECK_RETURN(piResult);

	SG_ERR_CHECK_RETURN(  SG_fsobj__stat__pathname(pCtx, pPathname, &fsStat)  );

	if (fsStat.type == SG_FSOBJ_TYPE__REGULAR)
	{
		if (pFsObjType)
			*pFsObjType = fsStat.type;
		*piResult = fsStat.size;

		return;
	}

	if (fsStat.type == SG_FSOBJ_TYPE__SYMLINK)
	{
		// for symlinks we return the length of the link (the pathname
		// fragment (the link pointer)) not the length of the final
		// object being referred to (the pointee).

		if (pFsObjType)
			*pFsObjType = fsStat.type;
		*piResult = fsStat.size;

		return;
	}

	SG_ERR_THROW2_RETURN(  SG_ERR_NOTAFILE,
						   (pCtx,"Calling length() on '%s'",SG_pathname__sz(pPathname))  );
}

//////////////////////////////////////////////////////////////////

struct _exists_exact__callback_data
{
	SG_string * pstrName; //the name to search for.
	SG_bool bResult;   //If a match was found.
};
static SG_dir_foreach_callback _exists_exact__dir_callback;

static void _exists_exact__dir_callback(SG_UNUSED_PARAM( SG_context * pCtx),
									  const SG_string * pStringEntryName,		// we do not own this
									  SG_UNUSED_PARAM( SG_fsobj_stat * pfsStat),
									  void * pVoidData)
{
	const char * pszEntryName = SG_string__sz(pStringEntryName);
	struct _exists_exact__callback_data * pData = (struct _exists_exact__callback_data *)pVoidData;
	if (pData->bResult == SG_TRUE)
		return; //If we already found it, exit early.


	SG_UNUSED(pCtx);
	SG_UNUSED(pfsStat);

	if (strcmp(SG_string__sz(pData->pstrName), pszEntryName) == 0)
		pData->bResult = SG_TRUE;
}

void SG_fsobj__exists__pathname__exact(SG_context * pCtx,
									   const SG_pathname * pPathParentDirectory,
									   const SG_pathname * pPathToCheck,
									   SG_bool * pResult)
{
	struct _exists_exact__callback_data data;
	SG_string * pstrCurrentName = NULL;
	SG_uint32 parentLen = 0;
	SG_pathname * pPathToCheck_Clone = NULL;
	data.bResult = 0;

	SG_ERR_CHECK(  SG_pathname__equal_ignoring_final_slashes(pCtx,
															 pPathParentDirectory,
															 pPathToCheck,
															 &data.bResult)  );
	if (data.bResult)
		goto done;

	// check the parts of the pathname *below* the given parent directory
	// that they exist on disk *and* with the case specified.

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPathToCheck_Clone, pPathToCheck)  );

	parentLen = SG_pathname__length_in_bytes(pPathParentDirectory);
	

	//We do this extra loop, so that we can handle the case where "dir/subdir/file" is passed in as the name.
	while (SG_pathname__length_in_bytes(pPathToCheck_Clone) > parentLen)
	{
		SG_ERR_CHECK(  SG_pathname__get_last(pCtx, pPathToCheck_Clone, &pstrCurrentName )  );
		SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPathToCheck_Clone )  );

		data.pstrName = pstrCurrentName;
		data.bResult = 0;

		// We DO NOT include .sgdrawer in the skip list (__SKIP_SG)
		// because that causes weird "file not found" messages.

		SG_ERR_CHECK(  SG_dir__foreach(pCtx, pPathToCheck_Clone, SG_DIR__FOREACH__SKIP_OS, _exists_exact__dir_callback, &data)  );
		SG_STRING_NULLFREE(pCtx, pstrCurrentName);
		if (data.bResult == 0)
			break;
	}

done:
fail:
#if 0
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("SG_fsobj__exists__pathname__exact: [%s]\n"
								"                                   [%s]\n"
								"                         yields %d\n"),
							   SG_pathname__sz(pPathParentDirectory),
							   SG_pathname__sz(pPathToCheck),
							   data.bResult)  );
#endif
	*pResult = data.bResult;
	SG_STRING_NULLFREE(pCtx, pstrCurrentName);
	SG_PATHNAME_NULLFREE(pCtx, pPathToCheck_Clone);

}

void SG_fsobj__exists__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_bool* pResult, SG_fsobj_type * pFsObjType, SG_fsobj_perms * pFsObjPerms)
{
	SG_error err;
	SG_fsobj_stat fsStat;

	SG_NULLARGCHECK_RETURN(pResult);

	SG_fsobj__stat__pathname(pCtx, pPathname, &fsStat);
	(void)SG_context__get_err(pCtx, &err);
	switch (err)
	{
	default:
		SG_ERR_CHECK_RETURN_CURRENT;

		// TODO should we have SG_fsobj__stat__pathname() return
		// TODO a normalized error for file/path not found?
#if defined(MAC) || defined(LINUX)
	case SG_ERR_ERRNO(ENOENT):
	case SG_ERR_ERRNO(ENOTDIR):
#endif
#if defined(WINDOWS)
	case SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND):
	case SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND):
#endif
		SG_context__err_reset(pCtx);
		*pResult = SG_FALSE;
		return;

	case SG_ERR_OK:
		if (pFsObjType)
			*pFsObjType = fsStat.type;
		if (pFsObjPerms)
			*pFsObjPerms = fsStat.perms;
		*pResult = SG_TRUE;
		return;
	}
}
void SG_fsobj__exists_stat__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_bool * pResult, SG_fsobj_stat * pStat)
{
	SG_error err;
	SG_fsobj_stat fsStat;

	SG_NULLARGCHECK_RETURN(pResult);

	SG_fsobj__stat__pathname(pCtx, pPathname, &fsStat);
	(void)SG_context__get_err(pCtx, &err);
	switch (err)
	{
	default:
		SG_ERR_CHECK_RETURN_CURRENT;

		// TODO should we have SG_fsobj__stat__pathname() return
		// TODO a normalized error for file/path not found?
#if defined(MAC) || defined(LINUX)
	case SG_ERR_ERRNO(ENOENT):
	case SG_ERR_ERRNO(ENOTDIR):
#endif
#if defined(WINDOWS)
	case SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND):
	case SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND):
#endif
		SG_context__err_reset(pCtx);
		*pResult = SG_FALSE;
		return;

	case SG_ERR_OK:
		if (pStat)
			*pStat = fsStat;
		*pResult = SG_TRUE;
		return;
	}
}

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
void SG_fsobj__remove__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	int result;
	char * pBufOS = NULL;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname),&pBufOS)  );

	result = unlink(pBufOS);
	if (result == -1)
		SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
						(pCtx,"Calling unlink() on '%s'",SG_pathname__sz(pPathname))  );

fail:
	SG_NULLFREE(pCtx, pBufOS);
}
#endif

#if defined(WINDOWS)
void SG_fsobj__remove__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	wchar_t * pBuf = NULL;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK_RETURN(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,NULL)  );

	// we must free pBuf

	if (!DeleteFileW(pBuf))
		SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()),
						(pCtx,"Calling DeleteFile() on '%s'",SG_pathname__sz(pPathname))  );

fail:
	SG_NULLFREE(pCtx, pBuf);
}
#endif

//////////////////////////////////////////////////////////////////

// TODO do we need an access-mode arg here.

#if defined(MAC) || defined(LINUX)
void SG_fsobj__mkdir__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	// create a directory.  this routine does not create
	// intermediate directories.

	int result;
	char * pBufOS = NULL;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname),&pBufOS)  );

	result = mkdir(pBufOS,0755);
	if (result == -1)
	{
		switch (errno)
		{
		case EEXIST:	// this may or may not be an error to the caller, so we unify the return code
			SG_ERR_THROW2(  SG_ERR_DIR_ALREADY_EXISTS,
							(pCtx,"Calling mkdir() on '%s'",SG_pathname__sz(pPathname))  );

		case ENOENT:	// a parent in the path does not exist
			SG_ERR_THROW2(  SG_ERR_DIR_PATH_NOT_FOUND,
							(pCtx,"Calling mkdir() on '%s'",SG_pathname__sz(pPathname))  );

		default:
			SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
							(pCtx,"Calling mkdir() on '%s'",SG_pathname__sz(pPathname))  );
		}
	}

fail:
	SG_NULLFREE(pCtx, pBufOS);
}
#endif

#if defined(WINDOWS)
void SG_fsobj__mkdir__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	// create a directory.  this routine does not create
	// intermediate directories.

	wchar_t * pBuf = NULL;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );
	SG_ERR_CHECK_RETURN(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,NULL)  );

	// we must free pBuf

	if (!CreateDirectoryW(pBuf,NULL))
	{
		DWORD e = GetLastError();

		switch (e)
		{
		case ERROR_ALREADY_EXISTS:	// this may or may not be an error to the caller, so we unify the return code
			SG_ERR_THROW2(  SG_ERR_DIR_ALREADY_EXISTS,
							(pCtx,"Calling CreateDirectory() on '%s'",SG_pathname__sz(pPathname))  );

		case ERROR_PATH_NOT_FOUND:	// a parent in the path does not exist
			SG_ERR_THROW2(  SG_ERR_DIR_PATH_NOT_FOUND,
							(pCtx,"Calling CreateDirectory() on '%s'",SG_pathname__sz(pPathname))  );

		default:
			SG_ERR_THROW2(  SG_ERR_GETLASTERROR(e),
							(pCtx,"Calling CreateDirectory() on '%s'",SG_pathname__sz(pPathname))  );
		}
	}

fail:
	SG_NULLFREE(pCtx, pBuf);
}
#endif

void SG_fsobj__mkdir_recursive__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	SG_error err;

	// create a directory.  this routine *does* create
	// intermediate directories.

	SG_pathname * pPathnameParent = NULL;

	// first try to create directly.

	SG_fsobj__mkdir__pathname(pCtx, pPathname);
	(void)SG_context__get_err(pCtx, &err);

	// if this succeeds or if we get a hard error (like a readonly
	// filesystem), we just quit.
	//
	// if we get an error that indicates that a parent directory
	// doesn't exist, we continue.

	switch (err)
	{
	default:
		SG_ERR_RETHROW_RETURN;

	case SG_ERR_DIR_ALREADY_EXISTS:
		SG_ERR_RETHROW_RETURN;

	case SG_ERR_OK:
		return;

	case SG_ERR_DIR_PATH_NOT_FOUND:
		SG_context__err_reset(pCtx);
		break;
	}

	// try to create parent directory (recursively) and then
	// retry to create this directory.

	SG_ERR_CHECK_RETURN(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPathnameParent, pPathname)  );

	SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPathnameParent)  );
	SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pPathnameParent)  );
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathname)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameParent);
}

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
void SG_fsobj__rmdir__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	// remove a directory.  this directory must be empty.

	int result;
	char * pBufOS = NULL;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname),&pBufOS)  );

	result = rmdir(pBufOS);
	if (result == -1)
		SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
						(pCtx,"Calling rmdir() on '%s'",SG_pathname__sz(pPathname))  );

fail:
	SG_NULLFREE(pCtx, pBufOS);
}
#endif

#if defined(WINDOWS)
void SG_fsobj__rmdir__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	// remove a directory.  this directory must be empty.

	wchar_t * pBuf = NULL;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK_RETURN(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,NULL)  );

	// we must free pBuf

	if (!RemoveDirectoryW(pBuf))
		SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()),
						(pCtx,"Calling RemoveDirectory() on '%s'",SG_pathname__sz(pPathname))  );

fail:
	SG_NULLFREE(pCtx, pBuf);
}
#endif

/**
 * Remove the contents of a directory, but not the directory itself.
 *
 */
void SG_fsobj__rmdir_contents_recursive(SG_context * pCtx, const SG_pathname * pPathname, SG_bool bForce)
{
	SG_error errRead;
	SG_fsobj_stat fsStat;
	SG_dir * pDir = NULL;
	SG_string * pStringEntryName;
	SG_pathname * pPathEntry = NULL;

	SG_NULLARGCHECK_RETURN(pPathname);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringEntryName)  );

	SG_ERR_CHECK(  SG_dir__open(pCtx, pPathname,&pDir,&errRead,pStringEntryName,&fsStat)  );
	while (SG_IS_OK(errRead))
	{
		const char * sz = SG_string__sz(pStringEntryName);

		SG_ASSERT( sz && sz[0] );
		if (   (sz[0] == '.')
			&& (   (sz[1] == 0)
				|| (   (sz[1] == '.')
					&& (sz[2] == 0))))
		{
			// we ignore "." and ".." on all platforms.
		}
		else
		{
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathEntry,
														   pPathname,
														   sz)  );

			switch (fsStat.type)
			{
			default:
//			case SG_FSOBJ_TYPE__REGULAR:
//			case SG_FSOBJ_TYPE__SYMLINK:
				// we only expect plain files and symlinks because that is all we
				// deal with, but who knows what else might be lurking.
				//
				// TODO for example, suppose they call us on "/dev"....
				//
				// i'm going to assume that we can just delete anything that is not
				// a directory.

				// TODO if this is a symlink, does this remove the link or the file it points to ?

				if (bForce)
				{
					// This is mainly for Windows.  It complains if we try to delete a read-only file.
					SG_ERR_IGNORE(  SG_fsobj__chmod__pathname(pCtx, pPathEntry, 0600)  );
				}
				SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPathEntry)  );
				break;

			case SG_FSOBJ_TYPE__DIRECTORY:
				SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPathEntry, bForce)  );
				break;
			}

			SG_PATHNAME_NULLFREE(pCtx, pPathEntry);
		}

		SG_dir__read(pCtx,pDir,pStringEntryName,&fsStat);
		(void)SG_context__get_err(pCtx,&errRead);
	}
	if (errRead == SG_ERR_NOMOREFILES)
		SG_context__err_reset(pCtx);
	else
		SG_ERR_CHECK_CURRENT;

	SG_DIR_NULLCLOSE(pCtx, pDir);

	SG_STRING_NULLFREE(pCtx, pStringEntryName);

	return;

fail:
	SG_DIR_NULLCLOSE(pCtx, pDir);
	SG_STRING_NULLFREE(pCtx, pStringEntryName);
	SG_PATHNAME_NULLFREE(pCtx, pPathEntry);
}

/**
 * rm -rf <path>  or   rm -r <path>
 *
 * Remove the directory and its contents.
 * 
 */
void SG_fsobj__rmdir_recursive__pathname2(SG_context * pCtx, const SG_pathname * pPathname, SG_bool bForce)
{
	SG_ERR_CHECK(  SG_fsobj__rmdir_contents_recursive(pCtx, pPathname, bForce)  );
	SG_ERR_CHECK(  SG_fsobj__rmdir__pathname(pCtx, pPathname)  );

fail:
	return;
}

/**
 * rm -r
 *
 * TODO think about combining this function with the previous by updating
 * TODO all of our callers to pass a value for bForce.  I only did it this
 * TODO way because I didn't want to touch all of the calls while on an
 * TODO unstable branch.
 */
void SG_fsobj__rmdir_recursive__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	SG_ERR_CHECK_RETURN(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPathname, SG_FALSE)  );
}

#if defined(WINDOWS)
void SG_fsobj__delete_to_recycle_bin(SG_context * pCtx, SG_bool bOkToShowGUI, const SG_pathname * pPathname)
{
	SHFILEOPSTRUCT fileOptStruct;
	wchar_t * pBuf = NULL;
	wchar_t buf[_MAX_PATH + 1]; // allow one more character
	int returnVal = 0;
	size_t nConvertedChars = 0;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	//Zero the path buffer, to ensure that it ends with the double NULL
	//that SHFileOperation expects.
	memset((wchar_t*)&buf,0,sizeof(wchar_t) * (_MAX_PATH + 1));
	//Zero out the SHFILEOPSTRUCT memory.
	memset((SHFILEOPSTRUCT*)&fileOptStruct,0,sizeof(SHFILEOPSTRUCT));
	
	if (bOkToShowGUI == SG_FALSE)
	{
		fileOptStruct.fFlags |= FOF_SILENT;                // don't report progress
		fileOptStruct.fFlags |= FOF_NOERRORUI;             // don't report errors
		fileOptStruct.fFlags |= FOF_NOCONFIRMATION;        // don't confirm delete
	}
	fileOptStruct.fFlags |= FOF_ALLOWUNDO;  

	//Convert the path to wchar_t.  When I tried the SG_pathname__to_unc_wchar function,
	//but SHFileOperation didn't handle it correctly (I'm assuming that it had to do with the 
	//unc stuff at the beginning.
	mbstowcs_s(&nConvertedChars, buf, SG_pathname__length_in_bytes(pPathname) + 1, SG_pathname__sz(pPathname), _TRUNCATE);
	
	fileOptStruct.wFunc = FO_DELETE;
	fileOptStruct.pFrom = buf;
	fileOptStruct.pTo = NULL;
   
	returnVal = SHFileOperation(&fileOptStruct);

	if (returnVal != 0)
		SG_ERR_THROW(  SG_ERR_GETLASTERROR(GetLastError())  );
fail:
	SG_NULLFREE(pCtx, pBuf);
}
#endif

//////////////////////////////////////////////////////////////////

#if defined(MAC) || defined(LINUX)
void SG_fsobj__chmod__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_fsobj_perms perms_sg)
{
	int result;
	char * pBufOS = NULL;

	// do a chmod() on the pathname.  we silently limit the given permissions to 0777.
	// that is, we do not allow/support S{UG}ID or STICKY BIT as these aren't portable
	// and don't reliably work on network mounted filesystems.

	SG_fsobj_perms p = (perms_sg & SG_FSOBJ_PERMS__MASK);

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname),&pBufOS)  );

	result = chmod(pBufOS,p);
	if (result == -1)
		SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
						(pCtx,"Calling chmod() on '%s'",SG_pathname__sz(pPathname))  );

fail:
	SG_NULLFREE(pCtx, pBufOS);
}
#endif

#if defined(WINDOWS)
void SG_fsobj__chmod__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_fsobj_perms perms_sg)
{
	// the only thing we can change is the READONLY attribute.  and this is for the user
	// only (not group/other).  and the 'x' bit is a lie.

	wchar_t * pBuf = NULL;
	DWORD attr;

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathname) , pPathname );

	SG_ERR_CHECK_RETURN(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,NULL)  );

	// we must SG_free pBuf.

	attr = GetFileAttributesW(pBuf);
	if (attr == INVALID_FILE_ATTRIBUTES)
		SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()),
						(pCtx,"Calling GetFileAttributes() on '%s'",SG_pathname__sz(pPathname))  );

	if (SG_FSOBJ_PERMS__WINDOWS_IS_READONLY(perms_sg))
		attr |= FILE_ATTRIBUTE_READONLY;
	else
		attr &= ~FILE_ATTRIBUTE_READONLY;

	if (!SetFileAttributes(pBuf,attr))
		SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()),
						(pCtx,"Calling SetFileAttributes() on '%s'",SG_pathname__sz(pPathname))  );

fail:
	SG_NULLFREE(pCtx, pBuf);
}
#endif

//////////////////////////////////////////////////////////////////

void SG_fsobj__move__pathname_pathname(SG_context * pCtx, const SG_pathname * pPathnameOld, const SG_pathname * pPathnameNew)
{
	// rename the file (or directory) given in 'old' to the pathname given in 'new'.
	//
	// there are times when we *NEED* this to be an atomic operation (such as renaming
	// TEMPFILES to BLOBFILES during BLOB creation (z Rename Trick) when the REPO is
	// shared by multiple clients).  There are other times when we don't care about
	// it being atomic (such as when updating a local/private working copy).
	//
	// since we cannot control how/what the filesystem does when we ask it to rename
	// something, we have to restrict what we ask for.
	//
	// Linux:
	//   rename() is atomic.
	//   rename() will overwrite/replace an existing destination.
	//   rename() fails if replace-ee is of different type (file/dir) than source type.
	//   rename() will move a file between directories.
	//   rename() gets funky around symlinks.
	//   rename() fails if source and destination on different filesystems.
	//
	// Windows: (using MoveFileEx())
	//   We turn off MOVEFILE_COPY_ALLOWED to prevent it from doing a COPY and DELETE.
	//   (This also prevents cross-device moves for files.)
	//   Cross-device moves are not permitted for directories.
	//   Does not overwrite/replace unless you use MOVEFILE_REPLACE_EXISTING, but this
	//   only works for files not directories.  You cannot overwrite directories with rename.
	//   The manual doesn't say if a file-->file rename in the same directory is atomic
	//   or not.
	//
	// Mac:
	//   TBD
	//

	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathnameOld) , pPathname );
	SG_ARGCHECK_RETURN( SG_pathname__is_set(pPathnameNew) , pPathname );

#if defined(MAC) || defined(LINUX)
	{
		int result;
		char * pBufOS_Old = NULL;
		char * pBufOS_New = NULL;

		SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathnameOld),&pBufOS_Old)  );
		SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathnameNew),&pBufOS_New)  );

		result = rename(pBufOS_Old,pBufOS_New);
		if (result == -1)
			SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
							(pCtx,"Calling rename() on '%s' --> '%s'",
							 SG_pathname__sz(pPathnameOld),SG_pathname__sz(pPathnameNew))  );

	fail:
		SG_NULLFREE(pCtx, pBufOS_Old);
		SG_NULLFREE(pCtx, pBufOS_New);
	}
#endif

#if defined(WINDOWS)
	{
		BOOL bResult;
		wchar_t * pBufOld = NULL;
		wchar_t * pBufNew = NULL;
		SG_error err = SG_ERR_OK;
		SG_uint32 k;

		SG_ERR_CHECK(  SG_pathname__to_unc_wchar(pCtx, pPathnameOld,&pBufOld,NULL)  );
		SG_ERR_CHECK(  SG_pathname__to_unc_wchar(pCtx, pPathnameNew,&pBufNew,NULL)  );

		// HACK 2011/03/09 Sometimes MoveFileEx() will randomly fail with "Access is denied".
		// HACK            This is usually only seen when running the test suite (that I know of).
		// HACK            A Google search indicates that this might be caused by the Windows
		// HACK            Indexer running in the background.  The test suite creates/moves
		// HACK            stuff without all of the usual human delays, so I inclined to believe
		// HACK            this (assuming the indexer is watching creation/modification events).
		// HACK            Several reports indicated that turning off the indexer caused the
		// HACK            problem to go away.
		// HACK
		// HACK            So I'm going to put a little spin loop around this to give the
		// HACK            indexer a chance to get out of the way.

#define NR_ATTEMPTS                      10
#define DELAY_BETWEEN_ATTEMPTS_IN_MS     10

		for (k=0; k<NR_ATTEMPTS; k++)
		{
			bResult = MoveFileEx(pBufOld,pBufNew, MOVEFILE_REPLACE_EXISTING);
			if (bResult)
				goto done;

			err = SG_ERR_GETLASTERROR(GetLastError());
			if ((err == SG_ERR_GETLASTERROR(ERROR_ACCESS_DENIED)) && (k+1<NR_ATTEMPTS))
				SG_sleep_ms(DELAY_BETWEEN_ATTEMPTS_IN_MS);
			else
				break;
		}

		SG_ERR_THROW2(  err,
						(pCtx,"Calling MoveFileEx() on '%s' --> '%s'",
						 SG_pathname__sz(pPathnameOld),SG_pathname__sz(pPathnameNew))  );

	done:
		;

	fail:
		SG_NULLFREE(pCtx, pBufNew);
		SG_NULLFREE(pCtx, pBufOld);
	}
#endif
}

//////////////////////////////////////////////////////////////////

SG_bool SG_fsobj__equivalent_perms(SG_fsobj_perms p1, SG_fsobj_perms p2)
{
	// are the 2 sets of file permissions equivalent.
	// this is a no-op on Linux/Mac, but is needed for Windows because
	// Windows doesn't support the full set of 0777 bits (and it lies).

#if defined(MAC) || defined(LINUX)
	return (SG_bool)((p1 & SG_FSOBJ_PERMS__MASK) == (p2 & SG_FSOBJ_PERMS__MASK));
#endif
#if defined(WINDOWS)
	// windows only has a single READONLY bit.  mode bits are a bit of a hack.
	// and it only stores user-info.  the group/other are copied from user.
	// the read-bit is always on.  and the 'x' bit is sometimes on (if we
	// stat() a .exe, stat may turn it on.

	return (SG_bool)(SG_FSOBJ_PERMS__WINDOWS_IS_READONLY(p1) == SG_FSOBJ_PERMS__WINDOWS_IS_READONLY(p2));
#endif
}

void SG_fsobj__getcwd(SG_context * pCtx, SG_string * pString)
{
	SG_pathname * pPathname = NULL;
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathname)   );
	SG_ERR_CHECK(  SG_fsobj__getcwd__pathname(pCtx, pPathname)   );
	SG_ERR_CHECK(  SG_string__set__sz(pCtx, pString, SG_pathname__sz( pPathname))   );
	//Fall through
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
}

//////////////////////////////////////////////////////////////////

void SG_fsobj__getcwd__pathname(SG_context * pCtx, SG_pathname * pPathname)
{
	// get cwd into the given string.  we clear it first.
	//
	// RESULT ALWAYS HAS FINAL SLASH.

	SG_NULLARGCHECK_RETURN(pPathname);

#if defined(MAC) || defined(LINUX)
	{
		// we make use of the the extension documented in both Linux and Mac
		// man pages of allowing null arguments to getcwd() so that it will
		// malloc the proper size buffer.  this is an extension to normal
		// posix behavior.

		char * sz = NULL;
		SG_string * pStringTmp = NULL;

		sz = getcwd(NULL,0);							// must free sz
		if (!sz)
			SG_ERR_THROW2_RETURN(  SG_ERR_ERRNO(errno), (pCtx, "getcwd() failed.")  );

		// properly INTERN the buffer that getcwd() gave us.  this takes
		// care of locale charenc/utf-8 and/or nfd/nfc issues.

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringTmp)   );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__sz(pCtx, pStringTmp,sz)  );
		SG_ERR_CHECK(  SG_pathname__set__from_sz(pCtx, pPathname, SG_string__sz(pStringTmp))   );
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathname)   );

		// fallthru to common exit
	fail:
		SG_STRING_NULLFREE(pCtx, pStringTmp);
		free(sz);		// MUST USE free() not SG_free() because getcwd() called malloc().
	}
#endif
#if defined(WINDOWS)
	{
		wchar_t * pBuf = NULL;
		SG_string * pStringTmp = NULL;

		pBuf = _wgetcwd(NULL,0);						// must free pBuf
		if (!pBuf)
			SG_ERR_THROW_RETURN(SG_ERR_GETLASTERROR(GetLastError()));

		// properly INTERN the buffer that _wgetcwd() gave us.  this takes
		// care of locale charenc/utf-8 and/or nfd/nfc issues.

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringTmp)   );
		SG_ERR_CHECK(  SG_utf8__intern_from_os_buffer__wchar(pCtx, pStringTmp,pBuf)  );
		SG_ERR_CHECK(  SG_pathname__set__from_sz(pCtx, pPathname, SG_string__sz(pStringTmp))   );
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathname)   );

		// fallthru to common exit
	fail:
		SG_STRING_NULLFREE(pCtx, pStringTmp);
		free(pBuf);		// MUST USE free() not SG_free() because _wgetcwd() called malloc().
	}
#endif
}

//////////////////////////////////////////////////////////////////

void SG_fsobj__cd__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	// Change the working copy to the one specified in pPathname

	SG_NULLARGCHECK_RETURN(pPathname);

#if defined(MAC) || defined(LINUX)
	{
		int error = 0;
		char * pBufOS = NULL;

		SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__sz(pCtx, SG_pathname__sz(pPathname),&pBufOS)  );

		error = chdir(pBufOS);
		if (error == -1)
			SG_ERR_THROW2(  SG_ERR_ERRNO(errno),
							(pCtx,"Calling chdir() on '%s'",SG_pathname__sz(pPathname))  );

fail:
		SG_NULLFREE(pCtx,pBufOS);
	}
#endif
#if defined(WINDOWS)
	{
		wchar_t * pBuf = NULL;
		int error = 0;

		SG_ERR_CHECK_RETURN(  SG_pathname__to_unc_wchar(pCtx, pPathname,&pBuf,NULL)  );

		error = _wchdir(pBuf);
		if (error != 0)
			SG_ERR_THROW2(  SG_ERR_GETLASTERROR(GetLastError()),
							(pCtx,"Calling chdir() on '%s'",SG_pathname__sz(pPathname))  );

fail:
		SG_NULLFREE(pCtx, pBuf);
	}
#endif
}

//////////////////////////////////////////////////////////////////

void SG_fsobj__verify_directory_exists_on_disk__pathname(SG_context * pCtx, const SG_pathname * pPathname)
{
	SG_bool bExists = SG_FALSE;
	SG_fsobj_type fsObjType = SG_FSOBJ_TYPE__UNSPECIFIED;

	SG_NULLARGCHECK_RETURN(pPathname);

	SG_ERR_CHECK_RETURN(  SG_fsobj__exists__pathname(pCtx, pPathname,&bExists,&fsObjType,NULL)  );

	if (!bExists)
		SG_ERR_THROW2_RETURN(  SG_ERR_NOT_FOUND,
							   (pCtx,"Calling verify_directory() on '%s'",SG_pathname__sz(pPathname))  );

	if (fsObjType != SG_FSOBJ_TYPE__DIRECTORY)
		SG_ERR_THROW2_RETURN(  SG_ERR_NOT_A_DIRECTORY,
							   (pCtx,"Calling verify_directory() on '%s'",SG_pathname__sz(pPathname))  );
}

//////////////////////////////////////////////////////////////////

void SG_fsobj__verify_that_at_least_one_exists(SG_context* pCtx, SG_uint32 count_args, const char ** paszArgs, SG_bool * pbFound)
{
	SG_uint32 currentIndex = 0;
	SG_bool bLocalFound = SG_FALSE;
	SG_bool pb_thisOneExists = SG_FALSE;
	SG_pathname * pPathname = NULL;

	for (currentIndex = 0; currentIndex < count_args; currentIndex++)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname, paszArgs[currentIndex])  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname, &pb_thisOneExists, NULL, NULL));
		if (pb_thisOneExists)
		{
			bLocalFound = SG_TRUE;
			break;
		}
		SG_PATHNAME_NULLFREE(pCtx, pPathname);
	}

//fall through.
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	*pbFound = bLocalFound;
	return;
}

void SG_fsobj__verify_that_all_exist(SG_context* pCtx, SG_uint32 count_args, const char ** paszArgs)
{
	SG_uint32 currentIndex = 0;
	SG_bool bLocalFound = SG_TRUE;
	SG_bool pb_thisOneExists = SG_FALSE;
	SG_pathname * pPathname = NULL;

	for (currentIndex = 0; currentIndex < count_args; currentIndex++)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname, paszArgs[currentIndex])  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname, &pb_thisOneExists, NULL, NULL));
		if (!pb_thisOneExists)
		{
			bLocalFound = SG_FALSE;
			break;
		}
		SG_PATHNAME_NULLFREE(pCtx, pPathname);
	}

	if (bLocalFound == SG_FALSE && count_args > 0)
			SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "Could not find %s", paszArgs[currentIndex])  );
//fall through.
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	return;
}

//////////////////////////////////////////////////////////////////

void SG_fsobj__copy_file(SG_context * pCtx, const SG_pathname * pPathSrc, const SG_pathname * pPathDest, SG_fsobj_perms perms)
{
	SG_file * pFileSrc = NULL;
	SG_file * pFileDest = NULL;
	SG_byte * pBuf = NULL;
	SG_uint32 lenBuf;
	SG_uint32 nbr;

	lenBuf = 32 * 1024;
	SG_ERR_CHECK(  SG_malloc(pCtx, lenBuf, &pBuf)  );

	// TODO 2012/03/23 Should we assert/verify that the 2
	// TODO            pathnames are not equal and/or
	// TODO            refer to the same actual file.

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
										   pPathSrc,
										   SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY,
										   SG_FSOBJ_PERMS__UNUSED,
										   &pFileSrc)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
										   pPathDest,
										   SG_FILE_OPEN_OR_CREATE | SG_FILE_WRONLY | SG_FILE_TRUNC,
										   perms,
										   &pFileDest)  );

	while (1)
	{
		SG_file__read(pCtx, pFileSrc, lenBuf, pBuf, &nbr);
		if (SG_context__err_equals(pCtx, SG_ERR_EOF))
		{
			SG_context__err_reset(pCtx);
			break;
		}
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_ERR_RETHROW;
		}

		SG_ERR_CHECK(  SG_file__write(pCtx, pFileDest, nbr, pBuf, NULL)  );
	}

fail:
	SG_NULLFREE(pCtx, pBuf);
	SG_FILE_NULLCLOSE(pCtx, pFileSrc);
	SG_FILE_NULLCLOSE(pCtx, pFileDest);
}

/**
 * Data/parameters used by _copy_item.
 */
typedef struct _copy_item__data
{
	const SG_pathname* pSource;      //< [in] Directory that is being iterated.
	const SG_pathname* pDestination; //< [in] Directory to copy the item into.
}
_copy_item__data;

/**
 * SG_dir__foreach_callback that copies the specified item to another location.
 */
static void _copy_item(
	SG_context*      pCtx,     //< [in] [out] Error and context info.
	const SG_string* sName,    //< [in] The name of the current item.
	SG_fsobj_stat*   pStat,    //< [in] Stat of the current item.
	void*            pUserData //< [in] A _copy_item__data* with more parameters.
	)
{
	_copy_item__data* pData        = (_copy_item__data*)pUserData;
	SG_pathname*      pSource      = NULL;
	SG_pathname*      pDestination = NULL;
	SG_string*        sTarget      = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pSource, pData->pSource, SG_string__sz(sName))  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pDestination, pData->pDestination, SG_string__sz(sName))  );

	switch (pStat->type)
	{
	case SG_FSOBJ_TYPE__REGULAR:
		SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pSource, pDestination, pStat->perms)  );
		break;
	case SG_FSOBJ_TYPE__DIRECTORY:
		SG_ERR_CHECK(  SG_fsobj__cpdir_recursive__pathname_pathname(pCtx, pSource, pDestination)  );
		break;
	case SG_FSOBJ_TYPE__SYMLINK:
		{
			SG_bool bExists = SG_FALSE;

			SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pDestination, &bExists, NULL, NULL)  );
			SG_ERR_CHECK(  SG_fsobj__readlink(pCtx, pSource, &sTarget)  );
			if (bExists == SG_TRUE)
			{
				SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pDestination)  );
			}
			SG_ERR_CHECK(  SG_fsobj__symlink(pCtx, sTarget, pDestination)  );
			break;
		}
	default:
		SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unsupported file system object type: %d", pStat->type));
		break;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pSource);
	SG_PATHNAME_NULLFREE(pCtx, pDestination);
	SG_STRING_NULLFREE(pCtx, sTarget);
	return;
}

void SG_fsobj__cpdir_recursive__pathname_pathname(
	SG_context*        pCtx,
	const SG_pathname* pSource,
	const SG_pathname* pDestination
	)
{
	SG_bool          bExists = SG_FALSE;
	SG_fsobj_type    eType   = SG_FSOBJ_TYPE__UNSPECIFIED;
	_copy_item__data cData;

	// check the destination
	SG_fsobj__exists__pathname(pCtx, pDestination, &bExists, &eType, NULL);
	if (bExists == SG_TRUE && eType != SG_FSOBJ_TYPE__DIRECTORY)
	{
		// target is something else, delete it
		SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pDestination)  );
		bExists = SG_FALSE;
	}
	if (bExists == SG_FALSE)
	{
		// target doesn't exist, create it
		SG_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, pDestination)  );
	}

	// iterate each item in the source and copy it to the destination
	cData.pSource      = pSource;
	cData.pDestination = pDestination;
	SG_dir__foreach(pCtx, pSource, SG_DIR__FOREACH__STAT | SG_DIR__FOREACH__SKIP_OS, _copy_item, (void*)&cData);

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_fsobj_stat__alloc__copy(SG_context * pCtx,
								SG_fsobj_stat ** ppfsStatNew,
								const SG_fsobj_stat * pfsStatSrc)
{
	SG_fsobj_stat * pfsNew = NULL;

	SG_NULLARGCHECK_RETURN(  ppfsStatNew  );
	SG_NULLARGCHECK_RETURN(  pfsStatSrc  );
	
	SG_ERR_CHECK(  SG_alloc1(pCtx, pfsNew)  );
	*pfsNew = *pfsStatSrc;	// structure copy
	*ppfsStatNew = pfsNew;
	return;

fail:
	SG_FSOBJ_STAT_NULLFREE(pCtx, pfsNew);
}

void SG_fsobj_stat__free(SG_context * pCtx, SG_fsobj_stat * pfs)
{
	if (!pfs)
		return;

	SG_NULLFREE(pCtx, pfs);
}

//////////////////////////////////////////////////////////////////


/**
 * Data/parameters used by _du_item.
 */
typedef struct _du_item__data
{
	const SG_pathname* pPath;
    SG_uint64 total;
}
_du_item__data;

static void _du_item(
	SG_context*      pCtx,     //< [in] [out] Error and context info.
	const SG_string* sName,    //< [in] The name of the current item.
	SG_fsobj_stat*   pStat,    //< [in] Stat of the current item.
	void*            pUserData //< [in] A _du_item__data* with more parameters.
	)
{
	_du_item__data* pData        = (_du_item__data*)pUserData;
	SG_pathname*      pPath      = NULL;
    SG_uint64 len = 0;

    SG_UNUSED(pStat);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pData->pPath, SG_string__sz(sName))  );

    SG_ERR_CHECK(  SG_fsobj__du__pathname(pCtx, pPath, &len)  );
    pData->total += len;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_fsobj__du__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_uint64* piResult)
{
	SG_fsobj_stat fsStat;
	_du_item__data cData;

	SG_NULLARGCHECK_RETURN(piResult);

	SG_ERR_CHECK_RETURN(  SG_fsobj__stat__pathname(pCtx, pPathname, &fsStat)  );

	if (fsStat.type == SG_FSOBJ_TYPE__REGULAR)
	{
        *piResult = fsStat.size;
	}
    else if (fsStat.type == SG_FSOBJ_TYPE__DIRECTORY)
    {
        cData.pPath      = pPathname;
        cData.total = 0;
        SG_dir__foreach(pCtx, pPathname, SG_DIR__FOREACH__STAT | SG_DIR__FOREACH__SKIP_OS, _du_item, (void*)&cData);
        *piResult = cData.total;
    }
    else
    {
        *piResult = 0;
    }
}

