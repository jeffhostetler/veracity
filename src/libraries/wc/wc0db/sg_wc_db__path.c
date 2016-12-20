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
 * @file sg_wc_db__path.c
 *
 * @details Routines to deal with the pathname to the WC DB.
 * 
 * Routines to deal with the various ways to name an item.  This
 * includes absolute-paths, relative-paths, and repo-paths and
 * conversions between them using the working-directory-top stored
 * in the pDb.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _check_if_relative_paths_allowed(SG_context* pCtx, const sg_wc_db * pDb)
{
	if (SG_FALSE == pDb->bWorkingDirPathFromCwd)
	{
		SG_ERR_THROW2_RETURN( SG_ERR_INVALIDARG, (pCtx, 
			"Relative paths are allowed only in working copy transactions initiated in the current working directory.") );
	}
}

/**
 * Construct a pathname for a DB in the drawer.
 * This is something of the form:
 * 
 *        <root>/.sgdrawer/wc.db
 *
 */
void sg_wc_db__path__compute_pathname_in_drawer(SG_context * pCtx, sg_wc_db * pDb)
{
	SG_ERR_CHECK_RETURN(  SG_workingdir__get_drawer_path(pCtx,
														 pDb->pPathWorkingDirectoryTop,
														 &pDb->pPathDB)  );

	SG_ERR_CHECK_RETURN(  SG_pathname__append__from_sz(pCtx, pDb->pPathDB, "wc.db")  );
}

//////////////////////////////////////////////////////////////////

/**
 * Return a pathname set to the location of the "tmp"
 * directory for this working directory.
 *
 * We create something like:
 *
 *         <root>/.sgdrawer/tmp/
 *
 * Note that the main "tmp" directory is long-lived and
 * may be used by multiple tasks at the same time.  So
 * for significant tasks, create a private sub-directory
 * within it.
 *
 */
void sg_wc_db__path__get_temp_dir(SG_context * pCtx,
								  const sg_wc_db * pDb,
								  SG_pathname ** ppPathTempDir)
{
	SG_pathname * pPath = NULL;
	
	SG_ERR_CHECK(  SG_workingdir__get_drawer_path(pCtx,
												  pDb->pPathWorkingDirectoryTop,
												  &pPath)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, "tmp")  );
	SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPath)  );

	// the "tmp" dir could get deleted by routine housekeeping,
	// so re-create it if someone even thinks about using temp-based
	// pathnames.  i'm going to skip the usual "if (!exists) mkdir"
	// stuff and just try to create it and ignore the error.

	SG_fsobj__mkdir__pathname(pCtx, pPath);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (!SG_context__err_equals(pCtx, SG_ERR_DIR_ALREADY_EXISTS))
			SG_ERR_RETHROW;
		SG_context__err_reset(pCtx);
	}

	*ppPathTempDir = pPath;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Return a temporary pathname (in the "tmp" dir) for any purpose.
 * We DO NOT create an actual file system object; we just generate
 * the pathname.
 * 
 * We create something like:
 *
 *         <root>/.sgdrawer/tmp/t123456789.tmp
 *
 */
void sg_wc_db__path__get_unique_temp_path(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  SG_pathname ** ppPathTemp)
{
	SG_pathname * pPathTempDir = NULL;
	SG_pathname * pPath = NULL;
	char bufTid[SG_TID_MAX_BUFFER_LENGTH + 20];
	SG_bool bExists;

	SG_ERR_CHECK(  sg_wc_db__path__get_temp_dir(pCtx, pDb, &pPathTempDir)  );
	while (1)
	{
		SG_ERR_CHECK(  SG_tid__generate2__suffix(pCtx, bufTid, sizeof(bufTid), 9, "tmp")  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPathTempDir, bufTid)  );

		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
		if (!bExists)
			break;

		SG_PATHNAME_NULLFREE(pCtx, pPath);
	}

	*ppPathTemp = pPath;
	pPath = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTempDir);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Return a "backup" pathname for backing up a file
 * (such as when we get a 'vv remove --force <modified_directory>'
 * request (where we cannot use the file's current directory because
 * it too is being deleted)).
 * 
 * We DO NOT create an actual file system object; we just generate
 * the pathname.
 * 
 * We create something like:
 *
 *         <root>/<gid7>~<entryname>~sg00~
 *
 * If you change the format of this this, see vscript_test_wc__backup_name_of_file().
 * 
 * Warning: I put the GID in the name for uniqueness because we do not
 * use the parent directory pathname in the result.  And because I want
 * to create the candidate backup pathnames in the QUEUE phase rather
 * than the APPLY phase and the result needs to be independent of other
 * items that may also need to be backed up.
 * 
 */

#define SG_BACKUP_FORMAT "%7.7s~%s~sg%02d~"

void sg_wc_db__path__generate_backup_path(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const char * pszGid,
										  const char * pszEntryname,
										  SG_pathname ** ppPathBackup)
{
	SG_pathname * pPath = NULL;
	SG_string * pString = NULL;
	SG_uint32 k = 0;
	SG_bool bExists;

	// TODO 2011/10/25 assert no slashes in entryname.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	for (k=0; k<100; k++)
	{
		SG_ERR_CHECK(  SG_string__clear(pCtx, pString)  );
		
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, SG_BACKUP_FORMAT, pszGid, pszEntryname, k)  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pDb->pPathWorkingDirectoryTop, SG_string__sz(pString))  );

		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
		if (!bExists)
			break;

		SG_PATHNAME_NULLFREE(pCtx, pPath);
	}

	if (k == 100)
	{
        SG_ERR_THROW2(  SG_ERR_TOO_MANY_BACKUP_FILES, (pCtx, "%s", pszEntryname)  );
	}

	*ppPathBackup = pPath;
	pPath = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pString);
}

//////////////////////////////////////////////////////////////////

/**
 * Convert absolute path to a wd-top-relative string.
 * 
 * This is just pathname/string parsing; we DO NOT confirm
 * that the path exists.  We only confirm that the result
 * is properly contained within the working directory.
 *
 */
void sg_wc_db__path__absolute_to_repopath(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const SG_pathname * pPathItem,
										  SG_string ** ppStringRepoPath)
{
	SG_string * pString = NULL;
	const char * psz;

	SG_workingdir__wdpath_to_repopath(pCtx,
									  pDb->pPathWorkingDirectoryTop,
									  pPathItem,
									  SG_FALSE,
									  &pString);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_CANNOT_MAKE_RELATIVE_PATH))
			goto throw_not_in_working_copy;
		if (SG_context__err_equals(pCtx, SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL))
			goto throw_not_in_working_copy;
		if (SG_context__err_equals(pCtx, SG_ERR_PATH_NOT_IN_WORKING_COPY))
			goto throw_not_in_working_copy;
		SG_ERR_RETHROW;
	}
	
	psz = SG_string__sz(pString);
	if ((psz[0] == '.') && (psz[1] == '.') && ((psz[2] == '/') || (psz[2] == 0)))
		goto throw_not_in_working_copy;

	*ppStringRepoPath = pString;
	return;

throw_not_in_working_copy:
	SG_context__err_reset(pCtx);
	SG_ERR_THROW2(  SG_ERR_PATH_NOT_IN_WORKING_COPY,
					(pCtx, "The path '%s' is not inside the working copy rooted at '%s'.",
					 SG_pathname__sz(pPathItem),
					 SG_pathname__sz(pDb->pPathWorkingDirectoryTop))  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

/**
 * Convert relative path to absolute, but make
 * sure that normalization doesn't take it outside
 * of the working directory.
 *
 * This is just pathname/string parsing; we DO NOT
 * confirm that the path exists or could exist.
 *
 */
void sg_wc_db__path__relative_to_absolute(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const SG_string * pStringRelativePath,
										  SG_pathname ** ppPathItem)
{
	SG_pathname * pPathWDT = NULL;
	SG_pathname * pPath = NULL;
	SG_uint32 lenWDT;

	// choke if they give us a repo-path.
	SG_ARGCHECK_RETURN(  (SG_string__sz(pStringRelativePath)[0] != '@'), pStringRelativePath  );

	// choke if this WC TX isn't cwd-based.
	SG_ERR_CHECK(  _check_if_relative_paths_allowed(pCtx, pDb)  );

	// clone WDT so that we can force a trailing slash.
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx,
											&pPathWDT,
											pDb->pPathWorkingDirectoryTop)  );
	SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathWDT)  );
	lenWDT = SG_pathname__length_in_bytes(pPathWDT);

	// allocate and normalize a new path with the net
	// result rather than writing on the clone (so that
	// we can do the following prefix test safely).
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath,
												   pPathWDT,
												   SG_string__sz(pStringRelativePath))  );

	if (strncmp(SG_pathname__sz(pPath),
				SG_pathname__sz(pPathWDT),
				lenWDT) != 0)
	{
		SG_ERR_THROW2(  SG_ERR_PATH_NOT_IN_WORKING_COPY,
						(pCtx, "The path '%s' is not inside the working copy rooted at '%s'.",
						 SG_string__sz(pStringRelativePath),
						 SG_pathname__sz(pDb->pPathWorkingDirectoryTop))  );
	}

	SG_PATHNAME_NULLFREE(pCtx, pPathWDT);
	*ppPathItem = pPath;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathWDT);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__path__repopath_to_absolute(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const SG_string * pStringRepoPath,
										  SG_pathname ** ppPathItem)
{
	SG_ERR_CHECK_RETURN(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pDb,
																  SG_string__sz(pStringRepoPath),
																  ppPathItem)  );
}

void sg_wc_db__path__sz_repopath_to_absolute(SG_context * pCtx,
											 const sg_wc_db * pDb,
											 const char * pszRepoPath,
											 SG_pathname ** ppPathItem)
{
	SG_pathname * pPath = NULL;

	// TODO 2012/01/23 This code was written before the extended-prefix repo-path
	// TODO            stuff, but it is still valid to require a live/wd-relative
	// TODO            repo-path when building an absolute pathname (because it
	// TODO            probably doesn't make sense to convert an arbitrary-context
	// TODO            repo-path (without someone doing the implied context conversion)).
	// TODO
	// TODO            So I'm going to leave this as an assert for now.
	SG_ASSERT_RELEASE_FAIL(  ((pszRepoPath[0]=='@') && (pszRepoPath[1]=='/'))  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx,
											&pPath,
											pDb->pPathWorkingDirectoryTop)  );
	SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPath)  );

	if (pszRepoPath[2])
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, &pszRepoPath[2])  );

	*ppPathItem = pPath;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////

static void _complain_if_unnormalized(SG_context * pCtx, const char * pszInput)
{
	const char * psz;

	SG_ASSERT(  (pszInput[0] == '/')  );

	for (psz=pszInput; *psz; psz++)
	{
		if (psz[0]=='/')
		{
			if ((psz[1]=='.') && ((psz[2]=='/') || (psz[2]==0)))
				goto my_throw;		// disallow "/./" or a trailing "/."
		
			if ((psz[1]=='.') && (psz[2]=='.') && ((psz[3]=='/') || (psz[3]==0)))
				goto my_throw;		// disallow "/../" or a trailing "/.."

			if ((psz[1]=='/'))
				goto my_throw;		// disallow "//"
		}

		if (psz[0]=='\\')
			goto my_throw;			// disallow backslashes anywhere in repo-paths
	}

	return;

my_throw:
	SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
						   (pCtx, "The input '%s' is not a valid repo-path (unnormalized).", pszInput)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Import a raw string (probably as the user typed it) and
 * transform it into a repo-path.  And scrub it as appropriate.
 *
 * This routine is inside WC and takes a "pDb" because we need
 * to be able to interpret a null or relative path and convert
 * them into a wd-root-based current/live repo-path.
 *
 * But we also need to be able to call it for arbitrary-cset-based
 * repo-paths that when we don't have a WD.
 *
 * We return both the observed/computed repo-path and the
 * observed/computed repo-path-domain.  The returned repo-path
 * will contain the repo-path-domain so that they are always
 * in normal form.
 *
 * It is up to the caller to decide whether the domain is
 * valid for their purposes and/or throw.
 *
 */
void sg_wc_db__path__anything_to_repopath(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const char * pszInput,
										  sg_wc_db__path__import_flags flags,
										  SG_string ** ppStringRepoPath,
										  char * pcDomain)
{
	SG_pathname * pPathTemp = NULL;
	SG_string * pStringTemp = NULL;
	SG_bool bValid;

	if (!pszInput || !*pszInput)	// when null, assume either CWD or ROOT.
	{
		switch (flags)
		{
		default:
		case SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR:
			SG_ERR_THROW_RETURN(  SG_ERR_INVALIDARG  );

		case SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ROOT:
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPathTemp, pDb->pPathWorkingDirectoryTop)  );
			SG_ERR_CHECK(  sg_wc_db__path__absolute_to_repopath(pCtx, pDb, pPathTemp, ppStringRepoPath)  );
			SG_PATHNAME_NULLFREE(pCtx, pPathTemp);
			*pcDomain = SG_WC__REPO_PATH_DOMAIN__LIVE;
			return;

		case SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_CWD:
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPathTemp)  );
			SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPathTemp)  );
			SG_ERR_CHECK(  sg_wc_db__path__absolute_to_repopath(pCtx, pDb, pPathTemp, ppStringRepoPath)  );
			SG_PATHNAME_NULLFREE(pCtx, pPathTemp);
			*pcDomain = SG_WC__REPO_PATH_DOMAIN__LIVE;
			return;
		}
	}

	if (pszInput[0] != '@')			// a non-repo pathname
	{
		SG_bool bPathIsRelative = SG_FALSE;
		SG_ERR_CHECK(  SG_pathname__is_relative(pCtx, pszInput, &bPathIsRelative)  );
		if (bPathIsRelative)
			SG_ERR_CHECK(  _check_if_relative_paths_allowed(pCtx, pDb)  );

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTemp, pszInput)  );
		SG_ERR_CHECK(  sg_wc_db__path__absolute_to_repopath(pCtx, pDb, pPathTemp, ppStringRepoPath)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathTemp);
		*pcDomain = SG_WC__REPO_PATH_DOMAIN__LIVE;
		return;
	}

	// if they gave us something that started with a "@" we
	// either return it as is or throw if we can't parse it.
	// this ensures that any error messages don't leave the
	// domain out of the repo-path.

	switch (pszInput[1])
	{
	case 0:
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "The input '%s' is not a valid repo-path input.", pszInput)  );

	case SG_WC__REPO_PATH_DOMAIN__G:		// domain "@g..." is a GID and not a path.
		// A '@g' domain input should look like
		// "@gae9ce8e7eaf04b58aef9470b439329f32249aae2358511e1b77b002500da2b78"
		// That is, the 'g' domain specifier is shared with the 'g' in the
		// full GID.  For now a full GID is required; we don't try to do the
		// unique prefix trick like we do for CSET HIDs in a REV-SPEC.  We
		// may think about adding that later.

		SG_ERR_CHECK(  SG_gid__verify_format(pCtx, &pszInput[1], &bValid)  );
		if (!bValid)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "The input '%s' is not a valid gid-domain input.", pszInput)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppStringRepoPath, pszInput)  );
		*pcDomain = SG_WC__REPO_PATH_DOMAIN__G;
		return;

	case SG_WC__REPO_PATH_DOMAIN__T:		// domain "@t..." is a temporary GID and not a path.
		// A '@t' domain input should look like
		// "@tae9ce8e7eaf04b58aef9470b439329f32249aae2358511e1b77b002500da2b78"
		// That is, the 't' domain specifier replaces the normal 'g' in the
		// full GID.  For now a full GID is required; we don't try to do the
		// unique prefix trick like we do for CSET HIDs in a REV-SPEC.  We
		// may think about adding that later.  We use a 't' when reporting
		// STATUS rather than a 'g' as a reminder that the ID is only valid
		// until the end of the transaction.
		//
		// make a g-version of the string so we can validate it.
		// but still return the t-version.

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringTemp)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringTemp, "%c%s",
										  SG_WC__REPO_PATH_DOMAIN__G,
										  &pszInput[2])  );
		SG_ERR_CHECK(  SG_gid__verify_format(pCtx, SG_string__sz(pStringTemp), &bValid)  );
		SG_STRING_NULLFREE(pCtx, pStringTemp);
		if (!bValid)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "The input '%s' is not a valid temp-gid-domain input.", pszInput)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppStringRepoPath, pszInput)  );
		*pcDomain = SG_WC__REPO_PATH_DOMAIN__T;
		return;

	case SG_WC__REPO_PATH_DOMAIN__LIVE:		// domain "@/foo..." is a normal current/live repo-path
		SG_ERR_CHECK(  _complain_if_unnormalized(pCtx, &pszInput[1])  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppStringRepoPath, pszInput)  );
		*pcDomain = SG_WC__REPO_PATH_DOMAIN__LIVE;
		return;

	default:		// we allow any other SINGLE character to be a domain.
		if (pszInput[2] != '/')
			SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
								   (pCtx, "The input '%s' is not a valid repo-path input.", pszInput)  );

		// we have "@x/foo...." for some character x.
		SG_ERR_CHECK(  _complain_if_unnormalized(pCtx, &pszInput[2])  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppStringRepoPath, pszInput)  );
		*pcDomain = pszInput[1];
		return;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTemp);
	SG_STRING_NULLFREE(pCtx, pStringTemp);
}

//////////////////////////////////////////////////////////////////

/**
 * Convert simple GID into a gid-domain repo-path.
 *
 */
void sg_wc_db__path__gid_to_gid_repopath(SG_context * pCtx,
										 const char * pszGid,
										 SG_string ** ppStringGidRepoPath)
{
	SG_string * pString = NULL;

	SG_NONEMPTYCHECK_RETURN( pszGid );
	SG_NULLARGCHECK_RETURN( ppStringGidRepoPath );

	SG_ARGCHECK_RETURN( ((pszGid[0] == 'g') || (pszGid[0] == 't')), pszGid );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "@%s", pszGid)  );

	*ppStringGidRepoPath = pString;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

void sg_wc_db__path__alias_to_gid_repopath(SG_context * pCtx,
										   sg_wc_db * pDb,
										   SG_uint64 uiAliasGid,
										   SG_string ** ppStringGidRepoPath)
{
	char * pszGid = NULL;

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pDb, uiAliasGid, &pszGid)  );
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, ppStringGidRepoPath)  );

fail:
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

/**
 * Return true if the entryname collides (or potentially
 * collides) with the .sgdrawer and/or .sgcloset (or has
 * a prefix that does).
 *
 * The user can set $SGCLOSET but we don't look at it.
 * S9718.
 *
 */
void sg_wc_db__path__is_reserved_entryname(SG_context * pCtx,
										   sg_wc_db * pDb,
										   const char * pszEntryname,
										   SG_bool * pbIsReserved)
{
	const char * pszDrawerName        = ".sgdrawer";
	const char * pszDefaultClosetName = ".sgcloset";

	SG_UNUSED( pCtx );
	SG_UNUSED( pDb );

	// It's OK to use stricmp/strnicmp rather than a
	// portability collider because the ".sgdrawer" and
	// ".sgcloset" prefix we are looking for are 7-bit clean.
	//
	// However I suppose it coulde give the wrong answer
	// if they pass us something that includes some NFD
	// ignorable characters or something.
	//
	// I'm mainly avoiding using a portability collider
	// because I only want a prefix check.

	if (SG_strnicmp(pszEntryname, pszDrawerName, SG_STRLEN(pszDrawerName)) == 0)
	{
		*pbIsReserved = SG_TRUE;
	}
	else if (SG_strnicmp(pszEntryname, pszDefaultClosetName, SG_STRLEN(pszDefaultClosetName)) == 0)
	{
		*pbIsReserved = SG_TRUE;
	}
	else
	{
		*pbIsReserved = SG_FALSE;
	}
}

