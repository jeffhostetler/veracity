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

#include <sg.h>

#define CURRENT_CLOSET_VERSION		2
#define CLOSET_VERSION_FILE	"version"
/**
 * Closet version history:
 * 
 * Version 1: Both the settings and the descriptors were jsondb files.
 * 
 * Version 2: The descriptors file changed to a normal sqlite3 database and added repository states.
 *            No change to settings.
 */ 

#define DESCRIPTORS_FILE	"descriptors.sqlite3"

#define MY_SLEEP_MS			(100)
#define MY_BUSY_TIMEOUT_MS	(10000)
#define CONVERSION_LOCK_FILE "CONVERSION_LOCK"

#define DESCRIPTOR_STAGING_KEY "staging-id"


//////////////////////////////////////////////////////////////////////////

typedef struct __descriptor_handle
{
	sqlite3* psql;
	SG_bool bInTx;
	char* pszValidatedName;
	char* pszOldDescriptorForReplace; /* When we're replacing an existing descriptor for 
									     usermap/import, this holds the old descriptor (the actual
										 descriptor, NOT the descriptor NAME) so we can delete the 
										 old repo instance on commit */
} _descriptor_handle;

//////////////////////////////////////////////////////////////////////////

static void _get_closet_version(SG_context* pCtx, const SG_pathname* pPathCloset, SG_uint32* piVersion);
void _open_descriptors_db(SG_context* pCtx, sqlite3** ppsql);
void _sg_closet__add_repo_entry(
								SG_context *pCtx,
								sqlite3 *psql,
								const char *reponame,
								const char *name,
								const SG_vhash *pvhFullDescriptor,
								SG_closet__repo_status status);


static void _descriptor_handle__free(SG_context* pCtx, _descriptor_handle* pHandle)
{
	if (pHandle)
	{
		SG_NULLFREE(pCtx, pHandle->pszValidatedName);
		SG_NULLFREE(pCtx, pHandle->pszOldDescriptorForReplace);
		if (pHandle->bInTx && pHandle->psql)
			SG_ERR_CHECK_RETURN(sg_sqlite__exec(pCtx, pHandle->psql, "ROLLBACK TRANSACTION"));
		SG_ERR_CHECK_RETURN(  sg_sqlite__close(pCtx, pHandle->psql)  );
		SG_NULLFREE(pCtx, pHandle);
	}
}

static void _write_current_closet_version(SG_context* pCtx, const SG_pathname* pPathCloset)
{
	SG_pathname* pPathVersionFile = NULL;
	SG_file* pFile = NULL;

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathCloset, CLOSET_VERSION_FILE)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathVersionFile, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY|SG_FILE_TRUNC, 0444, &pFile)  );

	/* The version is represented by the length of the file. We write null bytes to discourage edits. */
	{
		SG_uint32 i;
		SG_byte buf[CURRENT_CLOSET_VERSION];

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

static void _create_property_table(SG_context* pCtx, sqlite3* psql)
{
	SG_ERR_CHECK_RETURN(  sg_sqlite__exec(pCtx, psql,
		"CREATE TABLE properties"
		"   ("
		"     name VARCHAR NOT NULL,"
		"     value VARCHAR NULL"
		"   )")  );
	SG_ERR_CHECK_RETURN(  sg_sqlite__exec(pCtx, psql,
		"CREATE UNIQUE INDEX properties_name on properties ( name )")  );
}

static void _create_empty_descriptors_db(
	SG_context* pCtx, 
	const SG_pathname* pPathCloset, 
	SG_bool bCommitTx, /* If ppsql != NULL, caller can choose whether the creation tx is committed.
					      This conversionss to happen in the same tx as the creation. */
	sqlite3** ppsql, 
	SG_pathname** ppPathFile)
{
	SG_bool bCreated = SG_FALSE;
	SG_bool bInTx = SG_FALSE;

	SG_pathname* pPathDbFile = NULL;
	sqlite3* psql = NULL;
	
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDbFile, pPathCloset, DESCRIPTORS_FILE)  );
	SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPathDbFile, SG_SQLITE__SYNC__FULL, &psql)  );
	bCreated = SG_TRUE;
	sqlite3_busy_timeout(psql, MY_BUSY_TIMEOUT_MS);

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN EXCLUSIVE TRANSACTION;")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
		"CREATE TABLE repos"
		"   ("
		"     id VARCHAR NOT NULL,"
		"     name VARCHAR NOT NULL,"
		"     status INTEGER NOT NULL,"
		"     descriptor VARCHAR NOT NULL"
		"   )")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
		"CREATE INDEX repos_id on repos ( id )")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
		"CREATE UNIQUE INDEX repos_name on repos ( name )")  );
	
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql,
		"CREATE TABLE statuses"
		"   ("
		"     id INTEGER PRIMARY KEY,"
		"     status VARCHAR NOT NULL,"
		"     available INTEGER NOT NULL"
		"   )")  );

	SG_ERR_CHECK(  _create_property_table(pCtx, psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql,
		"INSERT INTO statuses (id, status, available) VALUES (%d, 'Normal', 1);"
		"INSERT INTO statuses (id, status, available) VALUES (%d, 'Cloning', 0);"
		"INSERT INTO statuses (id, status, available) VALUES (%d, 'Needs User Map', 0);"
		"INSERT INTO statuses (id, status, available) VALUES (%d, 'Importing', 0);",
		SG_REPO_STATUS__NORMAL,
		SG_REPO_STATUS__CLONING,
		SG_REPO_STATUS__NEED_USER_MAP,
		SG_REPO_STATUS__IMPORTING
		)  );

	if (ppsql)
	{
		if (bCommitTx)
		{
			SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION;")  );
			bInTx = SG_FALSE;
		}
		*ppsql = psql;
		psql = NULL;
	}
	else
	{
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION;")  );
		bInTx = SG_FALSE;
		SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql));
	}

	if (ppPathFile)
	{
		*ppPathFile = pPathDbFile;
		pPathDbFile = NULL;
	}
	else
		SG_PATHNAME_NULLFREE(pCtx, pPathDbFile);

	return;

fail:
	if (bInTx && psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION;")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql));
	if (bCreated) // If we created the database but we're failing, clean it up.
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathDbFile)  );
	SG_PATHNAME_NULLFREE(pCtx, pPathDbFile);
}


/* pPathCloset should exist on disk before you call this. */
static void _acquire_conversion_lock(SG_context* pCtx, const SG_pathname* pPathCloset, SG_bool* pbLocked)
{
	SG_pathname* pPathLockFile = NULL;
	SG_file* pFileLock = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathLockFile, pPathCloset, CONVERSION_LOCK_FILE)  );

    SG_file__open__pathname(pCtx, pPathLockFile, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFileLock);
    if (SG_CONTEXT__HAS_ERR(pCtx))
    {
        *pbLocked = SG_FALSE;
		SG_context__err_reset(pCtx);
    }
    else
    {
        *pbLocked = SG_TRUE;
    }

	/* fall through */
fail:
	SG_FILE_NULLCLOSE(pCtx, pFileLock);
	SG_PATHNAME_NULLFREE(pCtx, pPathLockFile);
}

static void _release_conversion_lock(SG_context* pCtx, const SG_pathname* pPathCloset)
{
	SG_pathname* pPathLockFile = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathLockFile, pPathCloset, CONVERSION_LOCK_FILE)  );
	SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPathLockFile)  );

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathLockFile);
}

typedef struct _convert_state
{
	sqlite3_stmt* pStmt;
	SG_string* pstrJson;
} convert_state;

static void _convert_cb(SG_context* pCtx, void* ctx, const SG_vhash* pvh, const char* putf8Key, const SG_variant* pv)
{
	convert_state* pState = (convert_state*)ctx;
	sqlite3_stmt* pStmt = pState->pStmt;
	SG_string* pstrJsonDescriptor = pState->pstrJson;
	const SG_vhash* pvhRefDescriptor = pv->v.val_vhash;
	SG_bool b = SG_FALSE;
	const char* pszRefRepoId = NULL;

	SG_UNUSED(pvh);

	SG_ERR_CHECK(  SG_string__clear(pCtx, pstrJsonDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRefDescriptor, pstrJsonDescriptor)  );

	/* Ensuring repo_id exists should be unnecessary in the real world, but the test suite 
	 * creates some nonsense descriptors. */
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefDescriptor, SG_RIDESC_KEY__REPO_ID, &b)  );
	if (b)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhRefDescriptor, SG_RIDESC_KEY__REPO_ID, &pszRefRepoId)  );
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszRefRepoId)  );
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, putf8Key)  );
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 3, SG_string__sz(pstrJsonDescriptor))  );
		SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
	}

	/* fall through */
fail:
	;
}

/**
 * On conversion from version 1 to 2:
 * 
 * Only the descriptors change, so the conversion occurs on the first read/write access to descriptors,
 * but NOT on settings queries.
 * 
 * ------------------------------------
 * 
 * On locks during conversion:
 * 
 * Some things need to be prevented during conversion so there aren't race conditions leading
 * to lost writes or incomplete reads. Those things are:
 * 
 * (a) No reads from the new descriptors file during conversion.
 * (b) No writes to the old descriptors file during conversion.
 * (c) No parallel conversions.
 * 
 * In order to prevent those 3 things, the conversion has these steps in this order. (The 
 * thing they prevent follows in parentheses:
 * 
 * 1) Acquire the conversion lock. (a,c)
 * 2) Create the new descriptors file.
 * 3) Begin an exlusive sqlite tx in the new file. (a)
 * 4) Update the version file. (b) (This will prevent reads of the old data too, a necessary evil.)
 * 5) Read from the old descriptors file. (b)
 * 6) Write to the new file.
 * 7) Commit the sqlite tx in the new file. (a)
 * 8) Release the conversion lock. (a,c) 
 *
 * Additionally, a failure to acquire the conversion lock results in these steps:
 * 
 * 1) Spin/wait for the conversion lock.
 * 2) Acquire it.
 * 3) Verify version, convert only if necessary.
 * 3) Release it.
 * 4) Continue with the requested operation in the new file. No conversion is necessary.
 */
static void _convert_version_1_to_2(SG_context* pCtx, const SG_pathname* pPathCloset, sqlite3** ppsql)
{
	SG_bool bLockOwned = SG_FALSE;
	SG_pathname* pPathOldDescriptors = NULL;
	SG_pathname* pPathOldDescriptorsBackup = NULL;
	SG_pathname* pPathNewDescriptors = NULL;
	SG_jsondb* pJsondb = NULL;
	SG_vhash* pvhOld = NULL;
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	SG_bool bOldExists = SG_FALSE;
	
	convert_state state;
	state.pStmt = NULL;
	state.pstrJson = NULL;

	while (!bLockOwned)
	{
		SG_ERR_CHECK(  _acquire_conversion_lock(pCtx, pPathCloset, &bLockOwned)  );
		if (!bLockOwned)
			SG_ERR_CHECK(  SG_sleep_ms(MY_SLEEP_MS)  );
	}

	{
		/* If another thread/process beat us to the conversion, do nothing. */
		SG_uint32 version = 0;
		SG_ERR_CHECK(  _get_closet_version(pCtx, pPathCloset, &version)  );
		if (version == 2)
		{
			SG_ERR_CHECK(  _release_conversion_lock(pCtx, pPathCloset)  );
			return;
		}
	}

	_create_empty_descriptors_db(pCtx, pPathCloset, SG_FALSE, &psql, &pPathNewDescriptors);
	if (SG_context__err_equals(pCtx, SG_ERR_FILE_ALREADY_EXISTS))
	{
		SG_context__err_reset(pCtx);
		return;
	}
	SG_ERR_CHECK_CURRENT;
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathOldDescriptors, pPathCloset, "descriptors.jsondb")  );
	{
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathOldDescriptors, &bOldExists, NULL, NULL)  );
		if (bOldExists)
		{
			SG_bool bHasRoot = SG_FALSE;
			SG_ERR_CHECK(  SG_jsondb__open(pCtx, pPathOldDescriptors, "descriptors", &pJsondb)  );
			SG_ERR_CHECK(  SG_jsondb__has(pCtx, pJsondb, "/", &bHasRoot)  );
			if (bHasRoot)
			{
				SG_ERR_CHECK(  _write_current_closet_version(pCtx, pPathCloset)  );

				SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &state.pStmt,
					"INSERT INTO repos (id, name, status, descriptor) "
					"VALUES (?, ?, %d, ?)", SG_REPO_STATUS__NORMAL)  );
				SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &state.pstrJson)  );

				SG_ERR_CHECK(  SG_jsondb__get__vhash(pCtx, pJsondb, "/", &pvhOld)  );
				SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pvhOld, _convert_cb, &state)  );

				SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
				bInTx = SG_FALSE;
			}
		}
		else
		{
			SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION;")  );
			bInTx = SG_FALSE;

			SG_ERR_CHECK(  _write_current_closet_version(pCtx, pPathCloset)  );
		}
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &state.pStmt)  );

	if (ppsql)
	{
		*ppsql = psql;
	}
	else
	{
		SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
	}
	psql = NULL;

	SG_ERR_CHECK(  _release_conversion_lock(pCtx, pPathCloset)  );

	SG_STRING_NULLFREE(pCtx, state.pstrJson);
	SG_JSONDB_NULLFREE(pCtx, pJsondb);
	SG_VHASH_NULLFREE(pCtx, pvhOld);

	if (bOldExists)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathOldDescriptorsBackup, pPathCloset, "descriptors-v1.old")  );
		/* It's not going to hurt anything if this fails. The file could be in use. */
		SG_ERR_IGNORE(  SG_fsobj__move__pathname_pathname(pCtx, pPathOldDescriptors, pPathOldDescriptorsBackup)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathOldDescriptorsBackup);
	}
	SG_PATHNAME_NULLFREE(pCtx, pPathOldDescriptors);

	SG_PATHNAME_NULLFREE(pCtx, pPathNewDescriptors);

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, state.pStmt)  );
	if (bInTx && psql)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	SG_ERR_IGNORE(  _release_conversion_lock(pCtx, pPathCloset)  );

	SG_STRING_NULLFREE(pCtx, state.pstrJson);
	SG_PATHNAME_NULLFREE(pCtx, pPathOldDescriptors);
	SG_PATHNAME_NULLFREE(pCtx, pPathOldDescriptorsBackup);

	SG_JSONDB_NULLFREE(pCtx, pJsondb);
	SG_VHASH_NULLFREE(pCtx, pvhOld);
	if (pPathNewDescriptors)
	{
		/* The conversion failed. Delete the half-baked new descriptors file. */
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathNewDescriptors)  );
	}
	SG_PATHNAME_NULLFREE(pCtx, pPathNewDescriptors);
}

/**
* Returns the current closet version or throws if it can't be determined.
*/
static void _get_closet_version(SG_context* pCtx, const SG_pathname* pPathCloset, SG_uint32* piVersion)
{
	SG_pathname* pPathVersionFile = NULL;
	SG_fsobj_type fsobjType;
	SG_uint64 len = 0;
	SG_bool bFound = SG_FALSE;
	SG_int64 iStart, iNow;

	SG_ASSERT(piVersion);

	SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathVersionFile, pPathCloset, CLOSET_VERSION_FILE)  );
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &iStart)  );
	iNow = iStart;

	/* Handle creation race: the closet dir could exist without the version file existing yet. */
	while ( (iNow - iStart) < 2000 )
	{
		SG_fsobj__length__pathname(pCtx, pPathVersionFile, &len, &fsobjType);
		if (SG_CONTEXT__HAS_ERR(pCtx))
		{
			SG_error err = SG_ERR_UNSPECIFIED;
			SG_context__get_err(pCtx, &err);

			switch (err)
			{
			default:
				SG_ERR_RETHROW;

#if defined(MAC) || defined(LINUX)
			case SG_ERR_ERRNO(ENOENT):
#endif
#if defined(WINDOWS)
			case SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND):
			case SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND):
#endif
				SG_context__err_reset(pCtx);
				SG_sleep_ms(MY_SLEEP_MS);
				SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &iNow)  );
			}
		}
		else
		{
			bFound = SG_TRUE;

			if (fsobjType != SG_FSOBJ_TYPE__REGULAR)
			{
				SG_ERR_THROW2(SG_ERR_UNSUPPORTED_CLOSET_VERSION, (pCtx, "the version file is invalid"));
			}

			if (len > SG_UINT32_MAX)
				SG_ERR_THROW(SG_ERR_LIMIT_EXCEEDED);
			*piVersion = (SG_uint32)len;

			break;
		}
	}

	if (!bFound)
		SG_ERR_THROW2(SG_ERR_UNSUPPORTED_CLOSET_VERSION, (pCtx, "no closet version file found"));

	/* Common cleanup */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathVersionFile);
}

static SG_pathname * gpEnvironmentSGCLOSET = NULL;

#ifdef SG_IOS
static const char *_closet_prefix = NULL;

void SG_closet__set_prefix(SG_context *pCtx, const char *prefix)
{
	if (_closet_prefix)
		SG_NULLFREE(pCtx, _closet_prefix);
	
	if (prefix)
		SG_ERR_CHECK_RETURN(  SG_strdup(pCtx, prefix, (char **)&_closet_prefix)  );
}


void SG_closet__get_prefix(SG_context *pCtx, const char **pprefix)
{
	char *prefix = NULL;
	
	SG_NULLARGCHECK(pprefix);
	
	if (_closet_prefix)
		SG_ERR_CHECK(  SG_strdup(pCtx, _closet_prefix, &prefix)  );
	
	*pprefix = prefix;
	prefix = NULL;
	
fail:
	SG_NULLFREE(pCtx, prefix);
}


#endif

static void _get_closet_path(SG_context * pCtx, SG_pathname** ppPath)
{
	SG_pathname* pPath = NULL;
	if (gpEnvironmentSGCLOSET!=NULL)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath, gpEnvironmentSGCLOSET)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_APPDATA_DIRECTORY(pCtx, &pPath)  );
		
#ifdef SG_IOS
		if (_closet_prefix)
			SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, _closet_prefix)  );
#endif
		SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, ".sgcloset")  );
	}

    *ppPath = pPath;
    pPath = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Public API for the above.
 * Callers shouldn't actually do anything
 * significant to the closet or cache this
 * pointer too long.
 *
 * Currently the closet is a directory of
 * files and stuff (probably in your home
 * directory or wherever you have $SGCLOSET
 * set).  (If this changes and we have an
 * option to store the entire closet in a
 * database somewhere, we should return NULL
 * here.)
 *
 * You own the returned pathname. 
 */
void SG_closet__get_closet_path(SG_context * pCtx,
								SG_pathname ** ppPath)
{
	SG_ERR_CHECK_RETURN(  _get_closet_path(pCtx, ppPath)  );
}

/**
 * Return the path to the current closet, making sure it exists.
 * piClosetVersion is optional, ppPath is not.
 */
static void _open_closet(SG_context * pCtx, SG_pathname** ppPath, SG_uint32* piClosetVersion)
{
	SG_pathname* pPath = NULL;

	SG_ASSERT(ppPath);

    SG_ERR_CHECK(  _get_closet_path(pCtx, &pPath)  );

	/* race-tolerant closet creation */
	{
		SG_bool bExists = SG_FALSE;
		SG_bool bCreatedByMe = SG_FALSE;
		SG_fsobj_type fsobj_type = SG_FSOBJ_TYPE__UNSPECIFIED;
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, &fsobj_type, NULL)  );
		while (!bExists)
		{
			SG_fsobj__mkdir_recursive__pathname(pCtx, pPath);
			if (!SG_CONTEXT__HAS_ERR(pCtx))
			{
				bCreatedByMe = SG_TRUE;
				fsobj_type = SG_FSOBJ_TYPE__DIRECTORY;
				bExists = SG_TRUE;
			}
			else
			{
				if (SG_context__err_equals(pCtx, SG_ERR_DIR_ALREADY_EXISTS))
				{
					SG_context__err_reset(pCtx);
					/* Because that's a recursive mkdir above, we can't be sure the dir that 
					 * exists is the one we want. So we've got to check (at least) once more. */
					SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, &fsobj_type, NULL)  );
					if (!bExists)
						SG_sleep_ms(MY_SLEEP_MS);
				}
				else
				{
					/* SG_fsobj__mkdir_recursive__pathname threw some other error. Handle it. */
					SG_ERR_CHECK_CURRENT;
				}
			}
		}
		if (fsobj_type != SG_FSOBJ_TYPE__DIRECTORY)
		{
			SG_ERR_THROW2(  SG_ERR_NOT_A_DIRECTORY,
				(pCtx,"the closet path (%s) exists but is not a directory", SG_pathname__sz(pPath))  );
		}
		if (bCreatedByMe)
			SG_ERR_CHECK(  _write_current_closet_version(pCtx, pPath)  );
	}


	if (piClosetVersion)
		SG_ERR_CHECK(  _get_closet_version(pCtx, pPath, piClosetVersion)  );
	SG_RETURN_AND_NULL(pPath, ppPath);

	/* Fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Opens a version 2 desciptors database. Will convert from version 1.
 */
void _open_descriptors_db(SG_context* pCtx, sqlite3** ppsql)
{
	SG_uint32 version;
	SG_pathname* pPathCloset = NULL;
	SG_pathname* pPathDescriptorsFile = NULL;
	SG_bool bExists = SG_FALSE;
	sqlite3* psql = NULL;

	SG_ERR_CHECK(  _open_closet(pCtx, &pPathCloset, &version)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDescriptorsFile, pPathCloset, DESCRIPTORS_FILE)  );
	if (CURRENT_CLOSET_VERSION == version)
	{
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathDescriptorsFile, &bExists, NULL, NULL)  );
		if (!bExists)
		{
			SG_ERR_CHECK(  _create_empty_descriptors_db(pCtx, pPathCloset, SG_TRUE, &psql, NULL)  );
			SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, psql, "PRAGMA journal_mode=WAL", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
		}
		else
		{
			SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPathDescriptorsFile, SG_SQLITE__SYNC__NORMAL, &psql)  );
		}
	}
	else if (1 == version)
	{
		SG_ERR_CHECK(  _convert_version_1_to_2(pCtx, pPathCloset, &psql)  );
		SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, psql, "PRAGMA journal_mode=WAL", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	}
	else
	{
		SG_ERR_THROW2(SG_ERR_UNSUPPORTED_CLOSET_VERSION, (pCtx, "%u", version));
	}

	*ppsql = psql;
	psql = NULL;

	SG_PATHNAME_NULLFREE(pCtx, pPathCloset);
	SG_PATHNAME_NULLFREE(pCtx, pPathDescriptorsFile);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCloset);
	SG_PATHNAME_NULLFREE(pCtx, pPathDescriptorsFile);
	if (psql)
		SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__get_localsettings(
	SG_context * pCtx,
	SG_jsondb** ppJsondb
    )
{
	SG_uint32 closetVersion = SG_UINT32_MAX;
	SG_pathname* pPath = NULL;
	SG_jsondb* pJsondb = NULL;
	SG_bool bHasRoot;

	SG_ERR_CHECK(  _open_closet(pCtx, &pPath, &closetVersion)  );
	/* Version 1 and 2 of the closet have identical settings, so we support both. */
	if (closetVersion != 1 && closetVersion != 2)
		SG_ERR_THROW2(SG_ERR_UNSUPPORTED_CLOSET_VERSION, (pCtx, "%u", closetVersion));

	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, "settings.jsondb")  );
	SG_ERR_CHECK(  SG_jsondb__create_or_open(pCtx, pPath, "settings", &pJsondb)  );

	SG_ERR_CHECK(  SG_jsondb__has(pCtx, pJsondb, "/", &bHasRoot)  );
	if (!bHasRoot)
		SG_ERR_CHECK(  SG_jsondb__add__vhash(pCtx, pJsondb, "/", SG_FALSE, NULL)  );

	SG_PATHNAME_NULLFREE(pCtx, pPath);

	*ppJsondb = pJsondb;

	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_JSONDB_NULLFREE(pCtx, pJsondb);
}

static void _get_and_create_path_under_closet(SG_context* pCtx, const char* pszName, SG_pathname** ppPath)
{
	SG_pathname* pPath = NULL;

	/* All we're using is the path, so the closet's version is irrelevant. */
	SG_ERR_CHECK(  _open_closet(pCtx, &pPath, NULL)  );

	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, pszName)  );

	SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pPath);
	if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
	{
		SG_context__err_reset(pCtx);
		/* Deliberately not a recursive mkdir, because the closet itself
		 * is created above, and an empty closet is invalid. */
		SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );
	}
	else
	{
		SG_ERR_CHECK_CURRENT;
	}

	SG_RETURN_AND_NULL(pPath, ppPath);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_closet__get_server_state_path(
	SG_context * pCtx,
	SG_pathname ** ppPath
    )
{
	SG_ERR_CHECK_RETURN(  _get_and_create_path_under_closet(pCtx, "server-state", ppPath)  );
}

void SG_closet__get_usermap_path(
	SG_context * pCtx,
	SG_pathname ** ppPath
	)
{
	SG_ERR_CHECK_RETURN(  _get_and_create_path_under_closet(pCtx, "usermaps", ppPath)  );
}

static void _get_path_for_remote_leaves(
	SG_context* pCtx,
    const char* psz_remote_repo_spec,
    SG_pathname** pp
    )
{
    SG_pathname* pPath = NULL;
    char buf[512];
    char* p = NULL;

    SG_NULLARGCHECK_RETURN(psz_remote_repo_spec);

	SG_ERR_CHECK(  _get_and_create_path_under_closet(pCtx, "remote-leaves", &pPath)  );
    SG_ERR_CHECK(  SG_strcpy(pCtx, buf, sizeof(buf), psz_remote_repo_spec)  );

    p = buf;
    while (*p)
    {
        switch (*p)
        {
            case '/':
            case ' ':
            case '.':
            case ':':
            case '\\':
            case '*':
            case '?':
            case '(':
            case ')':
                *p = '_';
                break;
            default:
                break;
        }
        p++;
    }

    SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, buf)  );

    *pp = pPath;
    pPath = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_closet__read_remote_leaves(
	SG_context* pCtx,
    const char* psz_remote_repo_spec,
    SG_vhash** ppvh_since
    )
{
    SG_pathname* pPath = NULL;
    SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;
    SG_bool b_exists = SG_FALSE;

    SG_NULLARGCHECK_RETURN(psz_remote_repo_spec);

    if (psz_remote_repo_spec)
    {
        SG_ERR_CHECK(  _get_path_for_remote_leaves(pCtx, psz_remote_repo_spec, &pPath)  );
        
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );

        if (b_exists)
        {
            SG_ERR_CHECK(  SG_file__read_into_string(pCtx, pPath, &pstr)  );
            SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, SG_string__sz(pstr))  );
        }
    }

    *ppvh_since = pvh;
    pvh = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_closet__write_remote_leaves(
	SG_context* pCtx,
    const char* psz_remote_repo_spec,
    SG_vhash* pvh_since
    )
{
    SG_pathname* pPath = NULL;
    SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;
	SG_fsobj_perms perms = 0644;
    SG_file* pFile = NULL;

    SG_NULLARGCHECK_RETURN(psz_remote_repo_spec);

    SG_ERR_CHECK(  _get_path_for_remote_leaves(pCtx, psz_remote_repo_spec, &pPath)  );
    
    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh_since, pstr)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY, perms, &pFile)   );
	SG_ERR_CHECK(  SG_file__truncate(pCtx, pFile)   );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, SG_string__sz(pstr))  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)   );

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_closet__get_descriptor_rowid(SG_context* pCtx, const char* pszDescriptorName, SG_int64* pId)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pstmt = NULL;
	int rc = 0;

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pstmt, "SELECT rowid FROM repos WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pstmt, 1, pszDescriptorName)  );
	rc = sqlite3_step(pstmt);

	if (SQLITE_ROW == rc)
		*pId = sqlite3_column_int64(pstmt, 0);
	else if (SQLITE_DONE == rc)
		SG_ERR_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pszDescriptorName));
	else
		SG_ERR_THROW(SG_ERR_SQLITE(rc));

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pstmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql));
}

//////////////////////////////////////////////////////////////////////////

static void _get_partial_descriptor_for_new_local_repo(
	SG_context * pCtx,
	const char * pszStorage,	// use config: new_repo/driver if null
	const char * pszRepoId,		// optional
	const char * pszAdminId,	// optional
	SG_vhash** ppvhDescriptor)
{
	SG_vhash* pvhPartialDescriptor = NULL;
	SG_pathname* pPath = NULL;
	char* pszRepoImpl = NULL;
	char buf[SG_GID_BUFFER_LENGTH];

	/* All we're using is the path, so the closet's version is irrelevant. */
	SG_ERR_CHECK(  _open_closet(pCtx, &pPath, NULL)  );

	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath, "repo")  );

	SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pPath);
	if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
	{
		SG_context__err_reset(pCtx);
		SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );
	}

	SG_ERR_CHECK_CURRENT;


	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhPartialDescriptor)  );

	if (pszStorage && *pszStorage)
	{
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_KEY__STORAGE, pszStorage)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__NEWREPO_DRIVER, NULL, &pszRepoImpl, NULL)  );
		if (pszRepoImpl)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_KEY__STORAGE, pszRepoImpl)  );
		}
		SG_NULLFREE(pCtx, pszRepoImpl);
	}

	/* TODO: this predates the closet altogether and should probably go away. */
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_FSLOCAL__PATH_PARENT_DIR, SG_pathname__sz(pPath))  );

	// repo and admin id are now required. If they weren't provided, we generate new values here.
	if (pszRepoId && *pszRepoId)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_KEY__REPO_ID, pszRepoId)  );
	else
	{
		SG_ERR_CHECK(  SG_gid__generate(pCtx, buf, sizeof(buf))  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_KEY__REPO_ID, buf)  );
	}

	if (pszAdminId && *pszAdminId)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_KEY__ADMIN_ID, pszAdminId)  );
	else
	{
		SG_ERR_CHECK(  SG_gid__generate(pCtx, buf, sizeof(buf))  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhPartialDescriptor, SG_RIDESC_KEY__ADMIN_ID, buf)  );
	}

	SG_PATHNAME_NULLFREE(pCtx, pPath);

	*ppvhDescriptor = pvhPartialDescriptor;

	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_VHASH_NULLFREE(pCtx, pvhPartialDescriptor);
	SG_NULLFREE(pCtx, pszRepoImpl);
}

static void _descriptor_exists(SG_context* pCtx, sqlite3* psql, const char* pszRefValidatedName, SG_bool* pbExists, SG_closet__repo_status* piStatus)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
									  "SELECT status FROM repos WHERE name=? LIMIT 1")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszRefValidatedName)  );

	rc = sqlite3_step(pStmt);
	if (rc == SQLITE_ROW)
    {
		*pbExists = SG_TRUE;
        if (piStatus)
        {
            *piStatus = (SG_closet__repo_status)sqlite3_column_int(pStmt, 0);
        }
    }
	else if (rc == SQLITE_DONE)
    {
		*pbExists = SG_FALSE;
    }
	else
    {
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void SG_closet__descriptors__list(SG_context * pCtx, SG_vhash** ppvhDescriptors)
{
	SG_ERR_CHECK_RETURN(  SG_closet__descriptors__list__status(pCtx, SG_REPO_STATUS__NORMAL, ppvhDescriptors)  );
}

void SG_closet__descriptors__list__status(
	SG_context * pCtx, 
	SG_closet__repo_status status, 
	SG_vhash** ppResult)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_vhash* pvhResult = NULL;
	SG_vhash* pvhDescriptor = NULL;
	SG_int32 rc;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"SELECT name, descriptor "
		"FROM repos "
		"WHERE status = %d "
		"ORDER BY name", 
		status)  );
	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* pszRefDescriptor = (const char*)sqlite3_column_text(pStmt, 1);
		SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhDescriptor, pszRefDescriptor)  );
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, (const char*)sqlite3_column_text(pStmt, 0), &pvhDescriptor)  );
	}
	if (rc != SQLITE_DONE)
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );

	SG_RETURN_AND_NULL(pvhResult, ppResult);

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
}

void SG_closet__descriptors__list__all(SG_context * pCtx, SG_vhash** ppvhResult)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_vhash* pvh = NULL;
	SG_vhash* pvhResult = NULL;
	SG_vhash* pvhDescriptor = NULL;
	SG_int32 rc;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"SELECT name, r.status, s.status, descriptor "
		"FROM repos r INNER JOIN statuses s ON r.status = s.id "
		"ORDER BY name")  );
	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* pszRefDescriptorName = (const char*)sqlite3_column_text(pStmt, 0);
		SG_closet__repo_status status = (SG_closet__repo_status)sqlite3_column_int(pStmt, 1);
		const char* pszRefStatusName = (const char*)sqlite3_column_text(pStmt, 2);
		const char* pszRefDescriptor = (const char*)sqlite3_column_text(pStmt, 3);

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, SG_CLOSET_DESCRIPTOR_STATUS_KEY, status)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_CLOSET_DESCRIPTOR_STATUS_NAME_KEY, pszRefStatusName)  );

		SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhDescriptor, pszRefDescriptor)  );
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, SG_CLOSET_DESCRIPTOR_KEY, &pvhDescriptor)  );

		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, pszRefDescriptorName, &pvh)  );
	}
	if (rc != SQLITE_DONE)
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );

	SG_RETURN_AND_NULL(pvhResult, ppvhResult);

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
}

void SG_closet__descriptors__replace_begin(
	SG_context * pCtx,
	const char* pszName,
	const char* pszStorage,
	const char* pszRepoId,
	const char* pszAdminId,
	const char** ppszValidatedName,
	SG_vhash** ppvhPartialDescriptor,
	SG_closet_descriptor_handle** ppHandle)
{
	_descriptor_handle* pHandle = NULL;
	SG_vhash* pvhPartial = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = SQLITE_ABORT;

	SG_NULLARGCHECK_RETURN(ppHandle);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pHandle)  );

	SG_ERR_CHECK(  SG_sz__trim(pCtx, pszName, NULL, &pHandle->pszValidatedName)  );
	SG_ERR_CHECK(  _get_partial_descriptor_for_new_local_repo(pCtx, pszStorage, pszRepoId, pszAdminId, &pvhPartial)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &pHandle->psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pHandle->psql, "BEGIN IMMEDIATE TRANSACTION", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	pHandle->bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pHandle->psql, &pStmt,
		"SELECT descriptor FROM repos WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszName)  );
	SG_ERR_CHECK(  sg_sqlite__step__nocheck__retry(pCtx, pStmt, &rc, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	if (SQLITE_DONE == rc)
		SG_ERR_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pszName) );
	else if (SQLITE_ROW != rc)
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char*)sqlite3_column_text(pStmt, 0), &pHandle->pszOldDescriptorForReplace)  );

	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	SG_RETURN_AND_NULL(pvhPartial, ppvhPartialDescriptor);
	if (ppszValidatedName)
		*ppszValidatedName = pHandle->pszValidatedName;
	*ppHandle = (SG_closet_descriptor_handle*)pHandle;

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_VHASH_NULLFREE(pCtx, pvhPartial);
	SG_ERR_IGNORE(  _descriptor_handle__free(pCtx, pHandle)  );
}

static void _validate_descriptor(SG_context* pCtx, const SG_vhash* pvhFullDescriptor, const char** ppszRefRepoId)
{
	const char* pszRefRepoId = NULL;

	SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvhFullDescriptor, SG_RIDESC_KEY__REPO_ID, &pszRefRepoId)  );
	if (!pszRefRepoId)
		SG_ERR_THROW2_RETURN(SG_ERR_INVALID_DESCRIPTOR, (pCtx, "%s is missing", SG_RIDESC_KEY__REPO_ID));

	if (ppszRefRepoId)
		*ppszRefRepoId = pszRefRepoId;
}

void SG_closet__descriptors__replace_commit(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle,
	const SG_vhash* pvhFullDescriptor,
	SG_closet__repo_status status)
{
	_descriptor_handle* pMe = NULL;
	const char* pszRefRepoId = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_string* pstrJsonDescriptor = NULL;
	
	SG_vhash* pvhDescriptorOld = NULL;
	SG_repo* pRepoOld = NULL;

	SG_NULL_PP_CHECK_RETURN(ppHandle);
	pMe = (_descriptor_handle*)*ppHandle;

	SG_ERR_CHECK_RETURN(  _validate_descriptor(pCtx, pvhFullDescriptor, &pszRefRepoId)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrJsonDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhFullDescriptor, pstrJsonDescriptor)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
		"UPDATE repos SET id = ?, status = ?, descriptor = ? WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszRefRepoId)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt, 2, status)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 3, SG_string__sz(pstrJsonDescriptor))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 4, pMe->pszValidatedName)  );
	SG_ERR_CHECK(  sg_sqlite__step__retry(pCtx, pStmt, SQLITE_DONE, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	pStmt = NULL;
	SG_STRING_NULLFREE(pCtx, pstrJsonDescriptor);

	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhDescriptorOld, pMe->pszOldDescriptorForReplace)  );
	SG_ERR_CHECK(  SG_repo__open_from_descriptor(pCtx, &pvhDescriptorOld, &pRepoOld)  );
	SG_ERR_CHECK(  SG_repo__delete_repo_instance__repo(pCtx, &pRepoOld)  );

	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pMe->psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	pMe->bInTx = SG_FALSE;

	SG_ERR_CHECK(  _descriptor_handle__free(pCtx, pMe)  );
	*ppHandle = NULL;

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_STRING_NULLFREE(pCtx, pstrJsonDescriptor);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptorOld);
	SG_REPO_NULLFREE(pCtx, pRepoOld);
	SG_ERR_IGNORE(  _descriptor_handle__free(pCtx, pMe)  );
}

void SG_closet__descriptors__replace_abort(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle)
{
	if (ppHandle && *ppHandle)
	{
		_descriptor_handle* pHandle = (_descriptor_handle*)*ppHandle;
		SG_ERR_CHECK_RETURN(  _descriptor_handle__free(pCtx, pHandle)  );
		*ppHandle = NULL;
	}
}

void SG_closet__descriptors__add_begin(
	SG_context * pCtx,
	const char* pszName,
	const char* pszStorage,
	const char* pszRepoId,
	const char* pszAdminId,
	const char** ppszValidatedName,
	SG_vhash** ppvhPartialDescriptor,
	SG_closet_descriptor_handle** ppHandle)
{
	_descriptor_handle* pHandle = NULL;
	SG_bool bExists = SG_FALSE;
	SG_vhash* pvhPartial = NULL;

	SG_NULLARGCHECK_RETURN(ppHandle);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pHandle)  );

	SG_ERR_CHECK(  SG_repo__validate_repo_name(pCtx, pszName, &pHandle->pszValidatedName)  );
	SG_ERR_CHECK(  _get_partial_descriptor_for_new_local_repo(pCtx, pszStorage, pszRepoId, pszAdminId, &pvhPartial)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &pHandle->psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pHandle->psql, "BEGIN IMMEDIATE TRANSACTION", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	pHandle->bInTx = SG_TRUE;

	SG_ERR_CHECK(  _descriptor_exists(pCtx, pHandle->psql, pHandle->pszValidatedName, &bExists, NULL)  );
	if (bExists)
		SG_ERR_THROW2(SG_ERR_REPO_ALREADY_EXISTS, (pCtx, "%s", pszName));

	SG_RETURN_AND_NULL(pvhPartial, ppvhPartialDescriptor);
	if (ppszValidatedName)
		*ppszValidatedName = pHandle->pszValidatedName;
	*ppHandle = (SG_closet_descriptor_handle*)pHandle;

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhPartial);
	SG_ERR_IGNORE(  _descriptor_handle__free(pCtx, pHandle)  );
}

void _sg_closet__add_repo_entry(
	SG_context *pCtx,
	sqlite3 *psql,
	const char *reponame,
	const char *name,
	const SG_vhash *pvhFullDescriptor,
	SG_closet__repo_status status)
{
	SG_string* pstrJsonDescriptor = NULL;
	sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrJsonDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhFullDescriptor, pstrJsonDescriptor)  );
	
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
									  "INSERT INTO repos (id, name, status, descriptor) "
									  "VALUES (?, ?, ?, ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, reponame)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, name)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt, 3, status)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 4, SG_string__sz(pstrJsonDescriptor))  );
	SG_ERR_CHECK(  sg_sqlite__step__retry(pCtx, pStmt, SQLITE_DONE, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	pStmt = NULL;
	SG_STRING_NULLFREE(pCtx, pstrJsonDescriptor);
	
	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );

fail:
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_STRING_NULLFREE(pCtx, pstrJsonDescriptor);
}

void SG_closet__descriptors__add_commit(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle,
	const SG_vhash* pvhFullDescriptor,
	SG_closet__repo_status status)
{
	_descriptor_handle* pMe = NULL;
	const char* pszRefRepoId = NULL;

	SG_NULL_PP_CHECK_RETURN(ppHandle);
	pMe = (_descriptor_handle*)*ppHandle;

	SG_ERR_CHECK_RETURN(  _validate_descriptor(pCtx, pvhFullDescriptor, &pszRefRepoId)  );

	SG_ERR_CHECK(  _sg_closet__add_repo_entry(pCtx, pMe->psql, pszRefRepoId, pMe->pszValidatedName, pvhFullDescriptor, status)  );
	pMe->bInTx = SG_FALSE;

	SG_ERR_CHECK(  _descriptor_handle__free(pCtx, pMe)  );
	*ppHandle = NULL;

	return;

fail:
	SG_ERR_IGNORE(  _descriptor_handle__free(pCtx, pMe)  );
}

void SG_closet__descriptors__add_abort(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle)
{
	if (ppHandle && *ppHandle)
	{
		_descriptor_handle* pHandle = (_descriptor_handle*)*ppHandle;
		SG_ERR_CHECK_RETURN(  _descriptor_handle__free(pCtx, pHandle)  );
		*ppHandle = NULL;
	}
}

void SG_closet__descriptors__update(
	SG_context* pCtx,
	const char* psz_existing_name,
	const SG_vhash* pvhNewFullDescriptor,
	SG_closet__repo_status status)
{
	const char* pszRefRepoId = NULL;
	SG_string* pstrJsonDescriptor = NULL;
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	sqlite3_stmt* pStmt = NULL;
	SG_uint32 numChanges = 0;

	SG_ERR_CHECK_RETURN(  _validate_descriptor(pCtx, pvhNewFullDescriptor, &pszRefRepoId)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrJsonDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhNewFullDescriptor, pstrJsonDescriptor)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"UPDATE repos SET id = ?, status = ?, descriptor = ? WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszRefRepoId)  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt, 2, status)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 3, SG_string__sz(pstrJsonDescriptor))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 4, psz_existing_name)  );
	SG_ERR_CHECK(  sg_sqlite__step__retry(pCtx, pStmt, SQLITE_DONE, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	SG_ERR_CHECK(  sg_sqlite__num_changes(pCtx, psql, &numChanges)  );
	if (0 == numChanges)
	{
		SG_ERR_THROW(SG_ERR_NOTAREPOSITORY);
	}
	else if (1 != numChanges)
	{
		SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, 
			"Unexpected failure renaming repository. Expected 1 record change, but %u were reported.", numChanges)  );
	}

	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
	psql = NULL;
	bInTx = SG_FALSE;

	SG_STRING_NULLFREE(pCtx, pstrJsonDescriptor);

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	if (psql)
	{
		if (bInTx)
			SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
		SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	}
	SG_STRING_NULLFREE(pCtx, pstrJsonDescriptor);
}

void SG_closet__descriptors__rename(SG_context* pCtx, const char* pszOld, const char* pszNew)
{
	char* pszOldTrimmed = NULL;
	char* pszValidatedNew = NULL;
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	sqlite3_stmt* pStmt = NULL;
	SG_bool bExists = SG_FALSE;
	SG_uint32 numChanges = 0;

	SG_ERR_CHECK(  SG_sz__trim(pCtx, pszOld, NULL, &pszOldTrimmed)  );
	SG_ERR_CHECK(  SG_repo__validate_repo_name(pCtx, pszNew, &pszValidatedNew)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT EXISTS (SELECT id FROM repos WHERE name = ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszOldTrimmed)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_ROW)  );
	bExists = sqlite3_column_int(pStmt, 0);
	if (!bExists)
		SG_ERR_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pszOldTrimmed)  );

	SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
	SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszValidatedNew)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_ROW)  );
	bExists = sqlite3_column_int(pStmt, 0);
	if(bExists)
		SG_ERR_THROW2(SG_ERR_REPO_ALREADY_EXISTS, (pCtx, "%s", pszValidatedNew));
	
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"UPDATE repos SET name = ? WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszValidatedNew)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszOldTrimmed)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
	
	SG_ERR_CHECK(  sg_sqlite__num_changes(pCtx, psql, &numChanges)  );
	if (1 != numChanges)
	{
		SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, 
			"Unexpected failure renaming repository. Expected 1 record change, but %u were reported.", numChanges)  );
	}

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	bInTx = SG_FALSE;

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszOldTrimmed);
	SG_NULLFREE(pCtx, pszValidatedNew);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	if (bInTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__descriptors__set_status(
	SG_context * pCtx,
	const char* pszName,
	SG_closet__repo_status new_status)
{
	char* szName = NULL;
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_uint32 numChanges = 0;

	SG_ERR_CHECK(  SG_sz__trim(pCtx, pszName, NULL, &szName)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"UPDATE repos "
		"SET status = ? "
		"WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt, 1, new_status)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, szName)  );

	SG_ERR_CHECK(  sg_sqlite__step__retry(pCtx, pStmt, SQLITE_DONE, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__num_changes(pCtx, psql, &numChanges)  );
	if (0==numChanges)
		SG_ERR_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pszName) );

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	SG_NULLFREE(pCtx, szName);
}

void SG_closet__descriptors__exists__status(SG_context* pCtx, const char* pszName, SG_bool* pbExists, SG_closet__repo_status* piStatus)
{
	char* pszTrimmedName = NULL;
	sqlite3* psql = NULL;

	SG_NULLARGCHECK_RETURN(pbExists);

	SG_sz__trim(pCtx, pszName, NULL, &pszTrimmedName);
	if(SG_context__err_equals(pCtx, SG_ERR_INVALID_REPO_NAME))
	{
		SG_context__err_reset(pCtx);
		*pbExists=SG_FALSE;
	}
	else
	{
		SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
		SG_ERR_CHECK(  _descriptor_exists(pCtx, psql, pszTrimmedName, pbExists, piStatus)  );
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszTrimmedName);
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__descriptors__exists(SG_context* pCtx, const char* pszName, SG_bool* pbExists)
{
    SG_ERR_CHECK_RETURN(  SG_closet__descriptors__exists__status(pCtx, pszName, pbExists, NULL)  );
}

const char *strrstr(
	const char *haystack,
	const char *needle)
{
	const char *last = NULL;
	
	while (*haystack)
	{
		const char *p = strstr(haystack, needle);
		
		if (p)
		{
			last = p;
			haystack = last + 1;
		}
		else
			break;
	}
	
	return( last );
}


#ifdef SG_IOS
static void _sg_closet__fix_descriptor_path(
	SG_context *pCtx,
	const char *name,
	SG_vhash *pvhDescriptor,
	SG_closet__repo_status status)
{
	SG_pathname *pPath = NULL;
	const char *oldPath = NULL;
	const char *newPath = NULL;
	SG_string *newStr = NULL;
	SG_uint32 len = 0;
	
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhDescriptor, SG_RIDESC_FSLOCAL__PATH_PARENT_DIR, &oldPath)  );
	
	SG_ERR_CHECK(  _get_closet_path(pCtx, &pPath)  );
	
	newPath = SG_pathname__sz(pPath);
	
	len = SG_STRLEN(newPath);
	
	if (SG_strnicmp(oldPath, newPath, len) != 0)
	{
		const char *lpos = strrstr(oldPath, ".sgcloset");
		
		if (lpos)
		{
			const char *suffix = lpos + 9;   // Parent's trailing slash
			SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &newStr, newPath)  );
			
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, newStr, suffix)  );
			
			SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvhDescriptor, SG_RIDESC_FSLOCAL__PATH_PARENT_DIR, SG_string__sz(newStr) )  );
			
			SG_ERR_CHECK(  SG_closet__descriptors__update(pCtx, name, pvhDescriptor, status)  );
		}
		else 
		{
			SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "repository storage path not found: %s", oldPath));
		}
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, newStr);
}
#endif

static void _get_descriptor(
	SG_context * pCtx, 
	const char* pszName, 
	SG_bool bAvailableOnly,
	char** ppszTrimmedName,
	SG_bool* pbAvailable,
	SG_closet__repo_status* pStatus,
	char** ppszStatus,
	SG_vhash** ppvhDescriptor)
{
	char * pszTrimmedName = NULL;
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	int rc;

	SG_ERR_CHECK(  SG_sz__trim(pCtx, pszName, NULL, &pszTrimmedName)  );
	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT r.descriptor, s.available, s.status, s.id "
		"FROM repos r INNER JOIN statuses s ON r.status = s.id "
		"WHERE name = ? LIMIT 1")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszTrimmedName)  );

	rc = sqlite3_step(pStmt);
	if (rc == SQLITE_DONE)
		SG_ERR_RESET_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pszTrimmedName));
	else if (rc != SQLITE_ROW)
		SG_ERR_THROW(SG_ERR_SQLITE(rc));

	if (bAvailableOnly)
	{
		SG_bool bAvailable = sqlite3_column_int(pStmt, 1);
		if (!bAvailable)
			SG_ERR_THROW2(SG_ERR_REPO_UNAVAILABLE, (pCtx, "%s", (const char*)sqlite3_column_text(pStmt, 2)));
	}

	SG_RETURN_AND_NULL(pszTrimmedName, ppszTrimmedName);
	if (pbAvailable)
		*pbAvailable = sqlite3_column_int(pStmt, 1);
	if (pStatus)
		*pStatus = (SG_closet__repo_status)sqlite3_column_int(pStmt, 3);
	if (ppszStatus)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char*)sqlite3_column_text(pStmt, 2), ppszStatus)  );
	if (ppvhDescriptor)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, ppvhDescriptor, 
			(const char*)sqlite3_column_text(pStmt, 0))  );

#ifdef SG_IOS
		if (ppszTrimmedName)
		{
			SG_closet__repo_status status = (SG_closet__repo_status)sqlite3_column_int(pStmt, 3);
		
			SG_ERR_CHECK(  _sg_closet__fix_descriptor_path(pCtx, *ppszTrimmedName, *ppvhDescriptor, status)  );
		}
#endif		
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszTrimmedName);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__descriptors__get__unavailable(
	SG_context * pCtx, 
	const char* pszName,
	char** ppszValidatedName,
	SG_bool* pbAvailable,
	SG_closet__repo_status* pStatus,
	char** ppszStatus,
	SG_vhash** ppvhDescriptor)
{
	SG_ERR_CHECK_RETURN(  _get_descriptor(pCtx, pszName, SG_FALSE, ppszValidatedName, pbAvailable, pStatus, ppszStatus, ppvhDescriptor)  );
}

void SG_closet__descriptors__get(
	SG_context * pCtx, 
	const char* pszName, 
	char** ppszValidatedName,
	SG_vhash** ppvhDescriptor)
{
	SG_NULLARGCHECK_RETURN(ppvhDescriptor);
	SG_ERR_CHECK_RETURN(  _get_descriptor(pCtx, pszName, SG_TRUE, ppszValidatedName, NULL, NULL, NULL, ppvhDescriptor)  );
}

void SG_closet__descriptors__remove(SG_context * pCtx, const char* pszName)
{
	char* szName = NULL;
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	sqlite3_stmt* pStmt = NULL;
	SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
	int rc = 0;

	SG_ERR_CHECK(  SG_sz__trim(pCtx, pszName, NULL, &szName)  );

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT status FROM repos WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, szName)  );
	
	SG_ERR_CHECK(  sg_sqlite__step__nocheck__retry(pCtx, pStmt, &rc, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );
	if (SQLITE_DONE == rc)
		SG_ERR_THROW2(SG_ERR_NOTAREPOSITORY, (pCtx, "%s", pszName) );
	else if (SQLITE_ROW == rc)
		status = (SG_closet__repo_status)sqlite3_column_int(pStmt, 0);
	else
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"DELETE FROM repos WHERE name = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, szName)  );
	SG_ERR_CHECK(  sg_sqlite__step__retry(pCtx, pStmt, SQLITE_DONE, MY_SLEEP_MS, MY_BUSY_TIMEOUT_MS)  );

	if (SG_REPO_STATUS__NEED_USER_MAP == status || SG_REPO_STATUS__IMPORTING == status)
		SG_ERR_CHECK(  SG_usermap__delete(pCtx, szName)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	bInTx = SG_FALSE;

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	if (bInTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
	SG_NULLFREE(pCtx, szName);
}

//////////////////////////////////////////////////////////////////

struct _rev_map_data
{
	SG_vhash * pvhResult;
	const char * pszRepoIdWanted;
};

static void _rev_map_cb__eval(SG_context * pCtx,
							  struct _rev_map_data * pData,
							  const char * psz_repo_id,
							  const char * pszKey_DescName)
{
	SG_string * pString = NULL;
	SG_vhash * pvhBindings = NULL;

	if (strcmp(psz_repo_id, pData->pszRepoIdWanted) != 0)
		return;

	// this repo matches the requested repo-id, so we want to add it to the result vhash.
	// 
	// see if it also has any bindings (urls for pulls/pushes) associated with it in the
	// localsettings.  if so, add them too.  normally, we only have the paths/default
	// binding that was (probably) created when repo was cloned, but the user could have
	// deleted it or added other aliases to other servers.  we allow for there to be zero
	// or many bindings by copying the "paths" sub-vhash.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "%s/%s/%s",
									  SG_LOCALSETTING__SCOPE__INSTANCE,
									  pszKey_DescName,
									  SG_LOCALSETTING__PATHS)  );

	SG_ERR_IGNORE(  SG_localsettings__get__vhash(pCtx,
												 SG_string__sz(pString),
												 NULL,		// null repo because we only want /instance/ settings.
												 &pvhBindings,
												 NULL)  );
	// pvhBindings may or may not be null.  it will be null afterwards.
    if (pvhBindings)
    {
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pData->pvhResult, pszKey_DescName, &pvhBindings)  );
    }
    else
    {
        // code elsewhere depends on the old behavior of add__vhash, which
        // added a null if given a null.
        SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pData->pvhResult, pszKey_DescName)  );
    }
	SG_ASSERT(  (pvhBindings == NULL)  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_VHASH_NULLFREE(pCtx, pvhBindings);
}
							  
static SG_vhash_foreach_callback _rev_map_cb;
static void _rev_map_cb(SG_context * pCtx,
						void * pVoidData,
						const SG_vhash * pvhForeach,
						const char * pszKey_DescName,
						const SG_variant * pvValue)
{
	struct _rev_map_data * pData = (struct _rev_map_data *)pVoidData;
	const SG_vhash * pvhValue;
	SG_repo * pRepo = NULL;
	char * psz_repo_id = NULL;
	SG_bool bHasRepoId;

	SG_UNUSED( pvhForeach );

	// pvValue contains a vhash with
	//      {
	//        storage
	//        path_parent_dir
	//        dir_name
	// and optionally
	//        repo_id
	//      }
	// 
	// if we have a repo_id field, just use it and avoid having to open the repo to get it.

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pvValue, (SG_vhash **)&pvhValue)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhValue, SG_RIDESC_KEY__REPO_ID, &bHasRepoId)  );
	if (bHasRepoId)
	{
		const char * psz_repo_id_ref;

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhValue, SG_RIDESC_KEY__REPO_ID,  &psz_repo_id_ref )  );	// we don't own this string
		SG_ERR_CHECK(  _rev_map_cb__eval(pCtx, pData, psz_repo_id_ref, pszKey_DescName)  );
	}
	else
	{
		// use the given descriptor name to try to open the repo.
		// if this fails (because the user deleted the actual repo
		// without deleting the record in the descriptor list),
		// then ignore this entry in the table.

		SG_ERR_IGNORE(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszKey_DescName, &pRepo)  );
		if (pRepo)
		{
			SG_ERR_CHECK(  SG_repo__get_repo_id( pCtx, pRepo, &psz_repo_id )  );

			SG_ERR_CHECK(  _rev_map_cb__eval(pCtx, pData, psz_repo_id, pszKey_DescName)  );

			// TODO 2010/01/07 Consider backfilling the repo-id into
			// TODO            the descriptor list so that a subsequent invocation
			// TODO            won't have to stumble here again.
		}
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, psz_repo_id);
}

/**
 * Use the array of hints (a list of repo-names for 'preferred'
 * submodules).  Lookup the (repo_id) of the hint by
 * name.  See if that matches the (repo_id) that we are
 * searching for.
 *
 * NOTE: The user can gives us a SINGLE/FLAT list of repo-names
 * as suggestions.  We pass this down through whatever nested
 * hierarchy of submodules as a flat list.  We do not force them
 * to try to match the submodule structure and nest the hints
 * nor bother peeling off outer matches as we dive.  This has
 * the odd property that if they reference a library submodule
 * in more than one place in the tree (and it has multiple
 * repo-instances), we'll bind both submodules to the same
 * instance.
 *
 * NOTE: Also note that we use the hints in the order given
 * and stop searching when we find a match.  WE DO NOT check
 * to see that the hints refer to distinct repo_id's.
 * This also means that we do not look to see if one repo-instance
 * has more csets than another.
 */
static void _rev_map__find_using_hints(SG_context * pCtx,
									   struct _rev_map_data * pData,
									   const SG_vector * pvec_use_submodules,
									   const SG_vhash * pvhDescList,
									   SG_bool * pbFoundMatch)
{
	SG_uint32 nrHints, kHint;
	SG_uint32 count_results_before, count_results_after;

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pData->pvhResult, &count_results_before)  );
	SG_ASSERT(  (count_results_before == 0)  );

	SG_ERR_CHECK(  SG_vector__length(pCtx, pvec_use_submodules, &nrHints)  );
	for (kHint=0; kHint < nrHints; kHint++)
	{
		char * pszRepoNameHint_k;
		SG_bool bHas_k;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pvec_use_submodules, kHint, (void **)&pszRepoNameHint_k)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhDescList, pszRepoNameHint_k, &bHas_k)  );
		if (bHas_k)
		{
			const SG_variant * pvDesc_k;

			SG_ERR_CHECK(  SG_vhash__get__variant(pCtx, pvhDescList, pszRepoNameHint_k, &pvDesc_k)  );
			SG_ERR_CHECK(  _rev_map_cb(pCtx, pData, pvhDescList, pszRepoNameHint_k, pvDesc_k)  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pData->pvhResult, &count_results_after)  );
			if (count_results_after > count_results_before)
			{
				*pbFoundMatch = SG_TRUE;
				return;
			}
		}
	}

fail:
	*pbFoundMatch = SG_FALSE;
}

void SG_closet__reverse_map_repo_id_to_descriptor_names(SG_context * pCtx,
														const char * pszRepoId,
														const SG_vector * pvec_use_submodules,
														SG_vhash ** ppvhList)
{
	SG_vhash * pvhDescList = NULL;
	struct _rev_map_data data;
	SG_bool bFoundMatch = SG_FALSE;

	memset(&data, 0, sizeof(data));

	SG_NONEMPTYCHECK_RETURN( pszRepoId );
	// pvec_use_submodules is optional
	SG_NULLARGCHECK_RETURN( ppvhList );

	data.pszRepoIdWanted = pszRepoId;
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &data.pvhResult)  );

	SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvhDescList)  );
	if (pvec_use_submodules)
	{
		// try to find a match using the hints given.

		SG_ERR_CHECK(  _rev_map__find_using_hints(pCtx, &data, pvec_use_submodules, pvhDescList, &bFoundMatch)  );
	}

	if (!bFoundMatch)
	{
		// no hints or no match.  do full linear search on
		// the list of all named repo-instances.

		SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pvhDescList, _rev_map_cb, (void *)&data)  );
	}

#if 0 && defined(DEBUG)
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx, data.pvhResult, pszRepoId)  );
#endif

	*ppvhList = data.pvhResult;
	data.pvhResult = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, data.pvhResult);
	SG_VHASH_NULLFREE(pCtx, pvhDescList);
}

static void _open_properties_db(SG_context* pCtx, sqlite3** ppsql)
{
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	SG_bool bTableExists = SG_FALSE;

	SG_ERR_CHECK(  _open_descriptors_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  SG_sqlite__table_exists(pCtx, psql, "properties", &bTableExists)  );
	if (!bTableExists)
		SG_ERR_CHECK(  _create_property_table(pCtx, psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	bInTx = SG_FALSE;

	*ppsql = psql;
	psql = NULL;

	/* fall through */
fail:
	if (bInTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__property__add(SG_context* pCtx, const char* pszName, const char* pszValue)
{
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	sqlite3_stmt* pStmt = NULL;
	SG_int32 rc = 0;

	SG_ERR_CHECK(  _open_properties_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"INSERT INTO properties (name, value) VALUES (?, ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszName)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszValue)  );
	rc = sqlite3_step(pStmt);
	switch (rc)
	{
	default:
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
		break;
	case SQLITE_CONSTRAINT:
		SG_ERR_THROW2(SG_ERR_CLOSET_PROP_ALREADY_EXISTS, (pCtx, "%s", pszName));
		break;
	case SQLITE_DONE:
		break;
	}
	
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	bInTx = SG_FALSE;

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	if (bInTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__property__set(SG_context* pCtx, const char* pszName, const char* pszValue)
{
	sqlite3* psql = NULL;
	SG_bool bInTx = SG_FALSE;
	sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  _open_properties_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );
	bInTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, 
		"INSERT OR REPLACE INTO properties (name, value) VALUES (?, ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszName)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszValue)  );
	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	bInTx = SG_FALSE;

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	if (bInTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, psql, "ROLLBACK TRANSACTION")  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__property__get(SG_context* pCtx, const char* pszName, char** ppszValue)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	SG_int32 rc = 0;

	SG_NULLARGCHECK_RETURN(ppszValue);

	SG_ERR_CHECK(  _open_properties_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT value FROM properties WHERE name = ? LIMIT 1")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszName)  );
	
	rc = sqlite3_step(pStmt);
	switch (rc)
	{
	default:
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
		break;
	case SQLITE_ROW:
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char*)sqlite3_column_text(pStmt, 0), ppszValue)  );
		break;
	case SQLITE_DONE:
		SG_ERR_THROW2(SG_ERR_NO_SUCH_CLOSET_PROP, (pCtx, "%s", pszName));
		break;
	}

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void SG_closet__property__exists(SG_context* pCtx, const char* pszName, SG_bool* pbExists)
{
	sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;

	SG_NULLARGCHECK_RETURN(pbExists);

	SG_ERR_CHECK(  _open_properties_db(pCtx, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT EXISTS (SELECT 1 FROM properties WHERE name = ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszName)  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_ROW)  );
	SG_ERR_CHECK(  *pbExists = (SG_bool)sqlite3_column_int(pStmt, 0)  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}
void SG_closet__get_size(
        SG_context* pCtx,
        SG_uint64* pi
        )
{
	SG_pathname* pPath_closet = NULL;

	SG_ERR_CHECK(  _open_closet(pCtx, &pPath_closet, NULL)  );
    SG_ERR_CHECK(  SG_fsobj__du__pathname(pCtx, pPath_closet, pi)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_closet);
}

void SG_closet__backup(
        SG_context* pCtx,
        SG_pathname** pp
        )
{
	SG_pathname* pPath_closet = NULL;
	SG_pathname* pPath_backup = NULL;
    char buf[256];
    SG_int64 itime = -1;
    SG_time tmLocal;

	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &itime)  );
    SG_ERR_CHECK(  SG_time__decode__local(pCtx, itime, &tmLocal)  );
    SG_ERR_CHECK(  SG_sprintf(pCtx, 
                buf, 
                sizeof(buf),
                "sgcloset_backup_%4d_%02d_%02d_%02d_%02d_%02d", 
                tmLocal.year,
                tmLocal.month,
                tmLocal.mday,
                tmLocal.hour,
                tmLocal.min,
                tmLocal.sec
                )  );

	SG_ERR_CHECK(  _open_closet(pCtx, &pPath_closet, NULL)  );

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_backup, pPath_closet)  );
    SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPath_backup)  );
    SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath_backup, buf)  );

    SG_ERR_CHECK(  SG_fsobj__cpdir_recursive__pathname_pathname(pCtx, pPath_closet, pPath_backup)  );

    *pp = pPath_backup;
    pPath_backup = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_closet);
    SG_PATHNAME_NULLFREE(pCtx, pPath_backup);
}

void SG_closet__restore(
        SG_context* pCtx,
        SG_pathname* pPath_backup,
        SG_pathname** ppPath_new_backup
        )
{
	SG_pathname* pPath_closet = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_fsobj_type t = 0;

    SG_NULLARGCHECK(pPath_backup);

    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_backup, &b_exists, &t, NULL)  );
    if (!b_exists)
    {
        SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "%s does not exist", SG_pathname__sz(pPath_backup)));
    }
    if (SG_FSOBJ_TYPE__DIRECTORY != t)
    {
        SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "%s is not a directory", SG_pathname__sz(pPath_backup)));
    }
    // TODO make sure the contents of the directory actually look like a closet

    // get the path to the existing closet
    SG_ERR_CHECK(  _get_closet_path(pCtx, &pPath_closet)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_closet, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        SG_ERR_CHECK(  SG_closet__backup(pCtx, ppPath_new_backup)  );
        SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPath_closet, SG_TRUE)  );
    }

    SG_ERR_CHECK(  SG_fsobj__cpdir_recursive__pathname_pathname(pCtx, pPath_backup, pPath_closet)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_closet);
}

//////////////////////////////////////////////////////////////////////////

void sg_closet__global_initialize(SG_context * pCtx)
{
	SG_string* pstrEnv = NULL;
	SG_uint32 len = 0;

#if !defined(SG_IOS)
	SG_ERR_CHECK(  SG_environment__get__str(pCtx, "SGCLOSET", &pstrEnv, &len)  );
#endif
	if (len)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &gpEnvironmentSGCLOSET, SG_string__sz(pstrEnv))  );
	}
	SG_STRING_NULLFREE(pCtx, pstrEnv);

	return;
fail:
	SG_STRING_NULLFREE(pCtx, pstrEnv);
}
void sg_closet__global_cleanup(SG_context * pCtx)
{
	SG_PATHNAME_NULLFREE(pCtx, gpEnvironmentSGCLOSET);
}

void sg_closet__refresh_closet_location_from_environment_for_testing_purposes(SG_context * pCtx)
{
	SG_ERR_CHECK_RETURN(  sg_closet__global_cleanup(pCtx)  );
	SG_ERR_CHECK_RETURN(  sg_closet__global_initialize(pCtx)  );
}

