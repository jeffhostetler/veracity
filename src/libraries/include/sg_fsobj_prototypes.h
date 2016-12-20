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

// sg_fsobj.h
// manipulate file system objects.
//////////////////////////////////////////////////////////////////

#ifndef H_SG_FSOBJ_H
#define H_SG_FSOBJ_H

BEGIN_EXTERN_C

//////////////////////////////////////////////////////////////////

struct _sg_fsobj_stat
{
	SG_fsobj_perms			perms;		// only SG_FSOBJ_PERMS__MASK bits are defined
	SG_fsobj_type			type;		// file, symlink, dir, other
	SG_uint64				size;		// only set for files and maybe symlinks
	SG_int64				mtime_ms;	// mod time since epoch in milliseconds
#if defined(SG_BUILD_FLAG_FEATURE_NS)
	SG_int64				mtime_ns;	// mod time since epoch in nanoseconds (with a big handwave on how accurate this might be)
#endif
};

//////////////////////////////////////////////////////////////////

void SG_fsobj_stat__free(SG_context * pCtx, SG_fsobj_stat * pfs);

void SG_fsobj_stat__alloc__copy(SG_context * pCtx,
								SG_fsobj_stat ** ppfsStatNew,
								const SG_fsobj_stat * pfsStatSrc);

#if defined(DEBUG)
#define SG_FSOBJ_STAT__ALLOC__COPY(pCtx,ppNew,pIn)	SG_STATEMENT(	SG_fsobj_stat * _pNew = NULL;										\
																	SG_fsobj_stat__alloc__copy(pCtx,&_pNew,pIn);						\
																	_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_fsobj_stat");	\
																	*(ppNew) = _pNew;													)
#else
#define SG_FSOBJ_STAT__ALLOC__COPY(pCtx,ppNew,pIn)	SG_fsobj_stat__alloc__copy(pCtx,ppNew,pIn)
#endif

//////////////////////////////////////////////////////////////////

void SG_fsobj__stat__pathname_sz(SG_context * pCtx, const SG_pathname * pPathnameDirectory, const char * szFileName, SG_fsobj_stat * pFsStat);
void SG_fsobj__stat__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_fsobj_stat * pFsStat);

void SG_fsobj__length__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_uint64 * piResult, SG_fsobj_type * pFsObjType);
//This is a very paranoid version of exists.  It will read the directory contents and return true
//if there is a child in the directory whose name exactly matches the provided name.
//This is useful because there are some platforms where there are multiple ways to address a
//file system object (README versus readme on windows).
void SG_fsobj__exists__pathname__exact(SG_context * pCtx,
									   const SG_pathname * pPathParentDirectory,
									   const SG_pathname * pPathToCheck,
									   SG_bool * pResult);

void SG_fsobj__exists__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_bool * pResult, SG_fsobj_type * pFsObjType, SG_fsobj_perms * pFsObjPerms);
void SG_fsobj__exists_stat__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_bool * pResult, SG_fsobj_stat * pStat);
void SG_fsobj__remove__pathname(SG_context * pCtx, const SG_pathname * pPathname);

/**
 * mkdir(pathname).  Creates a directory.  Requires that the parent directory exist.
 *
 * Returns SG_ERR_DIR_ALREADY_EXISTS is the directory already exists.
 *
 * Returns SG_ERR_DIR_PATH_NOT_FOUND if a parent directory in the path does not exist.
 *
 * Returns platform OS error for all other errors.
 */
void SG_fsobj__mkdir__pathname(SG_context * pCtx, const SG_pathname * pPathname);

/**
 * mkdir-recursive(pathame).  Like "mkdir -p".  Creates a directory; also creates
 * any necessary parent directories in the pathname.
 *
 * Returns SG_ERR_DIR_ALREADY_EXISTS is the directory already exists.
 *
 * Returns platform OS error for all other errors.
 */
void SG_fsobj__mkdir_recursive__pathname(SG_context * pCtx, const SG_pathname * pPathname);


void SG_fsobj__rmdir__pathname(SG_context * pCtx, const SG_pathname * pPathname);

/**
 * Recursive rmdir.  Like "rmdir -r".
 *
 * Deletes as much as it can.  Stops on the first error (which may be from
 * __rmdir__ or from __remove__).
 *
 * TODO Currently we don't have any notion of "-f" in any of the rmdir or rm
 * routines.  Perhaps we should.
 *
 * TODO Currently, this bails at the first error.  Should it keep going and
 * try to delete as much as possible?  Should this be an option?
 */
void SG_fsobj__rmdir_recursive__pathname(SG_context * pCtx, const SG_pathname * pPathname);

/**
 * The -f or not -f version.
 *
 * TODO think about combining this function with the previous by updating
 * TODO all of our callers to pass a value for bForce.  I only did it this
 * TODO way because I didn't want to touch all of the calls while on an
 * TODO unstable branch.
 */
void SG_fsobj__rmdir_recursive__pathname2(SG_context * pCtx, const SG_pathname * pPathname, SG_bool bForce);

/**
 * remove the contents of a directory but not the directory itself.
 */
void SG_fsobj__rmdir_contents_recursive(SG_context * pCtx, const SG_pathname * pPathname, SG_bool bForce);

void SG_fsobj__chmod__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_fsobj_perms perms_sg);
void SG_fsobj__move__pathname_pathname(SG_context * pCtx, const SG_pathname * pPathnameOld, const SG_pathname * pPathnameNew);

SG_bool		SG_fsobj__equivalent_perms(SG_fsobj_perms p1, SG_fsobj_perms p2);

// TODO SG_fsobj__symlink() fails if the link already exists.  Should we have
// TODO a form/function that allows you to change the target of a symlink without
// TODO having to delete the symlink first?  (See u0043_pendingtree.c)

void SG_fsobj__symlink(SG_context * pCtx, const SG_string* pstrLinkTo, const SG_pathname* pPathOfLink);
void SG_fsobj__readlink(SG_context * pCtx, const SG_pathname* pPath, SG_string** ppResult);

//////////////////////////////////////////////////////////////////

/**
 * Get CWD into the given string.
 *
 * We clear the string first.
 * We ensure a trailing slash on the returned path.
 *
 * You probably don't want to use this directly, see SG_pathname__set__from_cwd().
 */
void SG_fsobj__getcwd(SG_context * pCtx, SG_string * pString);

/**
 * Get CWD into the given string.
 *
 * We clear the pathname first.
 * We ensure a trailing slash on the returned path.
 *
 * You probably don't want to use this directly, see SG_pathname__set__from_cwd().
 */
void SG_fsobj__getcwd__pathname(SG_context * pCtx, SG_pathname * pPathname);

/**
 * Change the working directory to the one specified.
 */
void SG_fsobj__cd__pathname(SG_context * pCtx, const SG_pathname * pPathname);

#if defined(WINDOWS)
void SG_fsobj__find_data_to_stat(SG_context * pCtx, WIN32_FIND_DATAW * pData, SG_fsobj_stat * pFsStat);
void SG_fsobj__file_attribute_data_to_stat(SG_context * pCtx, WIN32_FILE_ATTRIBUTE_DATA * pData, SG_fsobj_stat * pFsStat);
void SG_fsobj__delete_to_recycle_bin(SG_context * pCtx, SG_bool bOkToShowGUI, const SG_pathname * pPathname);
#endif

//////////////////////////////////////////////////////////////////

/**
 * See if the pathname refers to a directory on disk.
 *
 * Returns various filesystem errors if __exists__ fails
 * or a bogus pointer is given.
 * Returns SG_ERR_NOT_FOUND if directory does not exist.
 * Returns SG_ERR_NOT_A_DIRECTORY if pathname does not refer to a directory.
 */
void SG_fsobj__verify_directory_exists_on_disk__pathname(SG_context * pCtx, const SG_pathname * pPathname);

//////////////////////////////////////////////////////////////////


/**
 * Searches the local disk for the arguments that are given.  If none of them exist, pbFound will be set to SG_FALSE.
 */
void SG_fsobj__verify_that_at_least_one_exists(SG_context* pCtx, SG_uint32 count_args, const char ** paszArgs, SG_bool * pbFound);

/**
 * Searches the local disk for the arguments that are given.  If one of them doesn't exist, SG_ERR_NOT_FOUND will be thrown.
 */
void SG_fsobj__verify_that_all_exist(SG_context* pCtx, SG_uint32 count_args, const char ** paszArgs);

//////////////////////////////////////////////////////////////////

/**
 * Copy a file.  We overwrite the destination.
 */
void SG_fsobj__copy_file(SG_context * pCtx, const SG_pathname * pPathSrc, const SG_pathname * pPathDest, SG_fsobj_perms perms);

/**
 * Copy a directory recursively.
 *
 * Target directories are added to if they already exist, or created if they don't.
 * If a target directory exists as a file, it is overwritten with a directory.
 * Target files are overwritten if they exist (uses SG_fsobj__copy_file for that).
 */
void SG_fsobj__cpdir_recursive__pathname_pathname(
	SG_context*        pCtx,        //< [in] [out] Error and context info.
	const SG_pathname* pSource,     //< [in] Pathname of the directory to copy.
	const SG_pathname* pDestination //< [in] Pathname of the target directory.
	);

void SG_fsobj__du__pathname(SG_context * pCtx, const SG_pathname * pPathname, SG_uint64* piResult);

//////////////////////////////////////////////////////////////////

END_EXTERN_C

#endif//H_SG_FSOBJ_H
