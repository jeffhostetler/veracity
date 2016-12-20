/*
Copyright 2012-2013 SourceGear, LLC

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

#include <sg.h>

//////////////////////////////////////////////////////////////////////////

#define LOCK_FILE_PREFIX		"USERMAP_LOCK_"
#define LOCK_RETRY_SLEEP_MS		100
#define LOCK_TIMEOUT_MS			30000

#define CURRENT_USERMAP_VERSION	1
#define USERMAP_VERSION_FILE	"version"

#define USERMAP_DB_FILE			"usermap.sqlite3"

//////////////////////////////////////////////////////////////////////////

static void _get_paths(SG_context* pCtx, const char* pszDescriptorName, SG_pathname** ppPathMe, SG_pathname** ppPathUsermap)
{
	SG_pathname* pPathMe = NULL;
	SG_int64 rowid = 0;
	SG_int_to_string_buffer buf;

	SG_ERR_CHECK(  SG_closet__get_usermap_path(pCtx, &pPathMe)  );
	if (pszDescriptorName && ppPathUsermap)
	{
		SG_ERR_CHECK(  SG_closet__get_descriptor_rowid(pCtx, pszDescriptorName, &rowid)  );
		SG_ERR_CHECK(  SG_int64_to_sz(rowid, buf)  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, ppPathUsermap, pPathMe, buf)  );
	}
	
	SG_RETURN_AND_NULL(pPathMe, ppPathMe);

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathMe);
}

//////////////////////////////////////////////////////////////////////////

static void _write_current_usermap_version(SG_context* pCtx, const SG_pathname* pPathUsermap)
{
	SG_pathname* pPathVersionFile = NULL;
	SG_file* pFile = NULL;

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathUsermap, USERMAP_VERSION_FILE)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathVersionFile, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY|SG_FILE_TRUNC, 0444, &pFile)  );

	/* The version is represented by the length of the file. We write null bytes to discourage edits. */
	{
		SG_uint32 i;
		SG_byte buf[CURRENT_USERMAP_VERSION];

		for (i = 0; i < sizeof(buf); i++)
			buf[i] = 0;
		SG_ERR_CHECK(  SG_file__write(pCtx, pFile, sizeof(buf), buf, NULL)  );
	}

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

/**
* Returns the current usermap version or throws if it can't be determined.
*/
static void _get_usermap_version(SG_context* pCtx, const SG_pathname* pPathUsermap, SG_uint32* piVersion)
{
	SG_pathname* pPathVersionFile = NULL;
	SG_fsobj_type fsobjType = SG_FSOBJ_TYPE__UNSPECIFIED;
	SG_uint64 len = 0;

	SG_ASSERT(piVersion);

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathUsermap, USERMAP_VERSION_FILE)  );
	SG_fsobj__length__pathname(pCtx, pPathVersionFile, &len, &fsobjType);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_log__report_verbose__current_error(pCtx);
		SG_ERR_RESET_THROW2(  SG_ERR_NOT_FOUND, (pCtx, "No such usermap: %s", SG_pathname__sz(pPathUsermap))  );
	}

	if (fsobjType != SG_FSOBJ_TYPE__REGULAR)
	{
		SG_ERR_THROW2(SG_ERR_UNSUPPORTED_USERMAP_VERSION, (pCtx, "the version file is invalid"));
	}

	if (len > SG_UINT32_MAX)
		SG_ERR_THROW(SG_ERR_LIMIT_EXCEEDED);
	*piVersion = (SG_uint32)len;

	/* Common cleanup */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);
}

//////////////////////////////////////////////////////////////////////////

static void _getLockFilePath(SG_context* pCtx, const SG_pathname* pPathMe, const char* pszDescriptorName, SG_pathname** ppPath)
{
	SG_string* pstrFilename = NULL;
	SG_pathname* pPathLockFile = NULL;

	SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstrFilename, LOCK_FILE_PREFIX)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstrFilename, pszDescriptorName)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathLockFile, pPathMe, SG_string__sz(pstrFilename))  );

	SG_STRING_NULLFREE(pCtx, pstrFilename);

	*ppPath = pPathLockFile;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstrFilename);
	SG_PATHNAME_NULLFREE(pCtx, pPathLockFile);
}

static void _acquireLock(SG_context* pCtx, const SG_pathname* pPathMe, const char* pszDescriptorName, SG_bool *pbLocked)
{
	SG_pathname* pPathLockFile = NULL;
	SG_file* pFileLock = NULL;
	SG_bool bLocked = SG_FALSE;
	SG_int64 msStart = -1;
	SG_int64 msNow = -1;

	SG_ERR_CHECK(  _getLockFilePath(pCtx, pPathMe, pszDescriptorName, &pPathLockFile)  );

	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &msStart)  );
	msNow = msStart;

	while (!bLocked && ((msNow - msStart) < LOCK_TIMEOUT_MS) )
	{
		SG_file__open__pathname(pCtx, pPathLockFile, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFileLock);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  SG_sleep_ms(LOCK_RETRY_SLEEP_MS)  );
			SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &msNow)  );
		}
		else
		{
			bLocked = SG_TRUE;
		}
	}

	*pbLocked = bLocked;

	/* fall through */
fail:
	SG_FILE_NULLCLOSE(pCtx, pFileLock);
	SG_PATHNAME_NULLFREE(pCtx, pPathLockFile);
}

static void _releaseLock(SG_context* pCtx, const SG_pathname* pPathMe, const char* pszDescriptorName)
{
	SG_pathname* pPathLockFile = NULL;

	SG_ERR_CHECK(  _getLockFilePath(pCtx, pPathMe, pszDescriptorName, &pPathLockFile)  );
	SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPathLockFile)  );

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathLockFile);
}

//////////////////////////////////////////////////////////////////////////

static void _create_empty_db(SG_context* pCtx, const SG_pathname* pPathUsermap)
{
	SG_pathname* pPathDbFile = NULL;
	sqlite3* psql = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDbFile, pPathUsermap, USERMAP_DB_FILE)  );
	SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPathDbFile, SG_SQLITE__SYNC__FULL, &psql)  );
	SG_PATHNAME_NULLFREE(pCtx, pPathDbFile);

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
		"CREATE TABLE map"
		"   ("
		"     src_recid VARCHAR PRIMARY KEY NOT NULL,"
		"     dest_recid name VARCHAR NOT NULL"
		"   )")  );

	SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql));

	return;

fail:
	/* The entire usermap dir is cleaned up on failure, 
	 * so there's no need to delete the db file here */

	SG_PATHNAME_NULLFREE(pCtx, pPathDbFile);
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql));
}

static void _open_db(SG_context* pCtx, const char* pszDescriptorName, sqlite3** ppsql)
{
	SG_pathname* pPath = NULL;
	SG_uint32 version = 0;

	SG_ERR_CHECK(  _get_paths(pCtx, pszDescriptorName, NULL, &pPath)  );
 	SG_ERR_CHECK(  _get_usermap_version(pCtx, pPath, &version)  );
	if (version > CURRENT_USERMAP_VERSION)
		SG_ERR_THROW2(SG_ERR_UNSUPPORTED_USERMAP_VERSION, (pCtx, "%u", version));

	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, USERMAP_DB_FILE)  );

	SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath, SG_SQLITE__SYNC__FULL, ppsql)  );

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

//////////////////////////////////////////////////////////////////////////

void SG_usermap__create(SG_context* pCtx, const char* pszDescriptorName, const SG_pathname* pPathStaging)
{
	SG_pathname* pPathMe = NULL; // The path the the base SGCLOSET/usermap dir
	SG_pathname* pPathUsermap = NULL; // The path to the usermap itself: SGCLOSET/usermap/descriptor
	SG_pathname* pPathUsermapStaging = NULL;
	SG_string* pstrDir = NULL;
	SG_bool bLocked = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pszDescriptorName);
	SG_NULLARGCHECK_RETURN(pPathStaging);

	SG_ERR_CHECK(  _get_paths(pCtx, pszDescriptorName, &pPathMe, &pPathUsermap)  );

	SG_ERR_CHECK(  _acquireLock(pCtx, pPathMe, pszDescriptorName, &bLocked)  );
	if (!bLocked)
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "timed out waiting for usermap lock '%s'", pszDescriptorName));

	/* Deliberately not a recursive mkdir because the usermap folder is created by _get_paths, above. */
	SG_fsobj__mkdir__pathname(pCtx, pPathUsermap);
	if (SG_context__err_equals(pCtx, SG_ERR_DIR_ALREADY_EXISTS))
	{
		SG_ERR_RETHROW2(  (pCtx, "Cannot create usermap. '%s' exists", SG_pathname__sz(pPathUsermap)) );
	}
	else
		SG_ERR_CHECK_CURRENT;

	SG_ERR_CHECK(  _write_current_usermap_version(pCtx, pPathUsermap)  );
	
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathUsermapStaging, pPathUsermap, "staging")  );

	// TODO: This hack should really implemented down in SG_fsobj (K9299).
	{
		SG_bool bNeedsCopyAndDelete = SG_FALSE;
		SG_fsobj__move__pathname_pathname(pCtx, pPathStaging, pPathUsermapStaging);
#if defined(WINDOWS)
		if (SG_context__err_equals(pCtx, SG_ERR_GETLASTERROR(ERROR_NOT_SAME_DEVICE)))
		{
			bNeedsCopyAndDelete = SG_TRUE;
			SG_ERR_DISCARD;
		}
#else
		if (SG_context__err_equals(pCtx, SG_ERR_ERRNO(EXDEV)))
		{
			bNeedsCopyAndDelete = SG_TRUE;
			SG_ERR_DISCARD;
		}
#endif
		SG_ERR_CHECK_CURRENT;

		if (bNeedsCopyAndDelete)
		{
			SG_ERR_CHECK(  SG_fsobj__cpdir_recursive__pathname_pathname(pCtx, pPathStaging, pPathUsermapStaging)  );
			SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPathStaging, SG_TRUE)  );
		}
	}

	SG_ERR_CHECK(  _create_empty_db(pCtx, pPathUsermap)  );

	/* fall through */
fail:
	if (bLocked)
		SG_ERR_IGNORE(  _releaseLock(pCtx, pPathMe, pszDescriptorName)  );

	/* Clean up half-baked usermap */
	if (SG_CONTEXT__HAS_ERR(pCtx) && pszDescriptorName)
		SG_ERR_IGNORE(  SG_usermap__delete(pCtx, pszDescriptorName)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathMe);
	SG_PATHNAME_NULLFREE(pCtx, pPathUsermap);
	SG_PATHNAME_NULLFREE(pCtx, pPathUsermapStaging);
	SG_STRING_NULLFREE(pCtx, pstrDir);
}

void SG_usermap__delete(
	SG_context* pCtx, 
	const char* pszDescriptorName
	)
{
	SG_pathname* pPathMe = NULL;
	SG_bool bLocked = SG_FALSE;
	SG_pathname* pPathUsermap = NULL;
	SG_bool bExists = SG_FALSE;

	SG_ERR_CHECK(  _get_paths(pCtx, pszDescriptorName, &pPathMe, &pPathUsermap)  );
	SG_ERR_CHECK(  _acquireLock(pCtx, pPathMe, pszDescriptorName, &bLocked)  );
	if (!bLocked)
		SG_ERR_THROW2(SG_ERR_UNSPECIFIED, (pCtx, "timed out waiting for usermap lock '%s'", pszDescriptorName));

	/* It would be better if we could just let the delete fail and ignore "does not exist"-type
	 * errors, but the intricacies of handling the right error codes across platforms with the 
	 * current SG_fsobj prevents that, today. */
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathUsermap, &bExists, NULL, NULL)  );
	if (bExists)
		SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPathUsermap, SG_TRUE)  );

	/* fall through */
fail:
	if (bLocked)
		SG_ERR_IGNORE(  _releaseLock(pCtx, pPathMe, pszDescriptorName)  );
	SG_PATHNAME_NULLFREE(pCtx, pPathMe);
	SG_PATHNAME_NULLFREE(pCtx, pPathUsermap);
}

//////////////////////////////////////////////////////////////////////////

void SG_usermap__users__get_all(
	SG_context* pCtx,
	const char* pszDescriptorName,
	SG_vhash** ppvhList)
{
	sqlite3* psql = NULL;
	SG_vhash* pvh = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_int32 rc;

	SG_ERR_CHECK(  _open_db(pCtx, pszDescriptorName, &psql)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"SELECT src_recid, dest_recid "
		"FROM map")  );
	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, 
			(const char*)sqlite3_column_text(pStmt, 0), 
			(const char*)sqlite3_column_text(pStmt, 1))  );
	}
	if (rc != SQLITE_DONE)
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );

	SG_RETURN_AND_NULL(pvh, ppvhList);

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_usermap__users__get_one(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const char* pszSrcRecId,
	char** ppszDestRecId)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_int32 rc;

	SG_NULLARGCHECK_RETURN(ppszDestRecId);

	SG_ERR_CHECK(  _open_db(pCtx, pszDescriptorName, &psql)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"SELECT dest_recid "
		"FROM map "
		"WHERE src_recid = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszSrcRecId)  );
	rc=sqlite3_step(pStmt);
	if (SQLITE_ROW == rc)
	{
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char*)sqlite3_column_text(pStmt, 0), ppszDestRecId)  );
	}
	else if (SQLITE_DONE == rc)
	{
		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "No mapping for recid '%s' exists", pszSrcRecId));
	}
	else
	{
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
	}

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}


void SG_usermap__users__add_or_update(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const char* pszSrcRecId,
	const char* pszDestRecId)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;

	SG_NULLARGCHECK_RETURN(pszSrcRecId);
	SG_NULLARGCHECK_RETURN(pszDestRecId);

	SG_ERR_CHECK(  _open_db(pCtx, pszDescriptorName, &psql)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"INSERT OR REPLACE INTO map VALUES (?, ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszSrcRecId)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszDestRecId)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_usermap__users__remove_one(
	SG_context* pCtx,
	const char* pszDescriptorName,
	const char* pszSrcRecId)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;

	SG_NULLARGCHECK_RETURN(pszSrcRecId);

	SG_ERR_CHECK(  _open_db(pCtx, pszDescriptorName, &psql)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"DELETE FROM map WHERE src_recid = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszSrcRecId)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}


void SG_usermap__users__remove_all(
	SG_context* pCtx,
	const char* pszDescriptorName)
{
	sqlite3* psql = NULL;

	SG_ERR_CHECK(  _open_db(pCtx, pszDescriptorName, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DELETE FROM map")  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_usermap__get_fragball_path(
	SG_context* pCtx,
	const char* pszDescriptorName,
    SG_pathname** pp
    )
{
	SG_pathname* pPath = NULL;

    SG_NULLARGCHECK_RETURN(pszDescriptorName);
    SG_NULLARGCHECK_RETURN(pp);

	SG_ERR_CHECK(  _get_paths(pCtx, pszDescriptorName, NULL, &pPath)  );

	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, "staging")  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, SG_USERMAP_FRAGBALL_NAME)  );

    *pp = pPath;
    pPath = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

