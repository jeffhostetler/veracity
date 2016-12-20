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

// src/libraries/ut/sg_error.c
// Error message-related functions.
//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

static void _sg_error__trim_external_message(char * pBuf)
{
	SG_uint32 len;
	SG_ASSERT(pBuf!=NULL);
	len=SG_STRLEN(pBuf);
	while(len>0 && (pBuf[len-1]=='.' || pBuf[len-1]==' ' || pBuf[len-1]==SG_CR || pBuf[len-1]==SG_LF))
		--len;
	pBuf[len]='\0';
}

////////////////////////////////////////////////////////////////

#if defined(WINDOWS)
/**
 * Wrapper for FormatMessageW() to get text in UTF8 into a FIXED BUFFER.
 * I'm going to avoid using the SG_utf8__intern_from_os_buffer__wchar()
 * so that we don't have to do any mallocs (in case the original error
 * was a malloc failure).
 * 
 */
static void _sg_FormatMessage__utf8(DWORD dwFlags, DWORD errCode,
									char * pBuf, SG_uint32 lenBuf)
{
	wchar_t wideBuf[SG_ERROR_BUFFER_SIZE];
	DWORD lenWideBuf = SG_ERROR_BUFFER_SIZE;
	DWORD lenNeeded;

	memset(pBuf, 0, lenBuf);

	FormatMessageW(dwFlags, NULL, errCode, 0, wideBuf, lenWideBuf, NULL);
	lenNeeded = WideCharToMultiByte(CP_UTF8, 0, wideBuf, -1, NULL, 0, NULL, NULL);
	if (lenNeeded <= lenBuf)
	{
		WideCharToMultiByte(CP_UTF8, 0, wideBuf, -1, pBuf, lenBuf, NULL, NULL);
	}
	else
	{
		_snprintf_s(pBuf, (size_t)lenBuf-1, _TRUNCATE, "GLE:%d", errCode);
	}
}

static void _sg_strerror__utf8(char * pBuf, SG_uint32 lenBuf, DWORD errCode)
{
	wchar_t wideBuf[SG_ERROR_BUFFER_SIZE];
	DWORD lenWideBuf = SG_ERROR_BUFFER_SIZE;
	DWORD lenNeeded;

	memset(pBuf, 0, lenBuf);

	_wcserror_s(wideBuf, lenWideBuf, errCode);
	lenNeeded = WideCharToMultiByte(CP_UTF8, 0, wideBuf, -1, NULL, 0, NULL, NULL);
	if (lenNeeded <= lenBuf)
	{
		WideCharToMultiByte(CP_UTF8, 0, wideBuf, -1, pBuf, lenBuf, NULL, NULL);
	}
	else
	{
		_snprintf_s(pBuf, (size_t)lenBuf-1, _TRUNCATE, "Errno:%d", errCode);
	}
}
#endif

////////////////////////////////////////////////////////////////

void SG_error__get_message(
	SG_error err,
	SG_bool bDetailed,
	char * pBuf,
	SG_uint32 lenBuf)
{
	// fetch error message for given error code.
	//
	// we try to be absolutely minimalist so that have high probability
	// of success (in a low-free-memory environment, for example).  therefore,
	// we only copy (with truncatation) into the buffer provided and don't
	// do a lot of fancy string tricks.
	//
	// we return a pointer to the buffer supplied (so caller can call us
	// in the arg list of a printf, for example).

	if (!pBuf || (lenBuf==0))  // if no buffer given, or zero length
		return;                // we can't do anything.

	memset(pBuf,0,lenBuf*sizeof(*pBuf));

	if (SG_IS_OK(err))			// TODO should we return "No error."
		return;

	// we have multiple ranges of error codes and each gets its message
	// text from a different source.  see sg_error.h

	switch (SG_ERR_TYPE(err))
	{
	case __SG_ERR__ERRNO__:			// an errno-based system error.
		{
#if defined(WINDOWS)
			_sg_strerror__utf8(pBuf, lenBuf, SG_ERR_ERRNO_VALUE(err));
#else
			char bufErrnoMessage[SG_ERROR_BUFFER_SIZE+1];
			int errCode;

			errCode = SG_ERR_ERRNO_VALUE(err);

#  if defined(MAC)
			strerror_r(errCode,bufErrnoMessage,SG_ERROR_BUFFER_SIZE+1);	// use thread-safe version
			strncpy(pBuf,bufErrnoMessage,lenBuf-1);
#  endif
#  if defined(LINUX)
#    if defined(__USE_GNU)
			// we get the non-standard GNU version
			strncpy(pBuf,
					strerror_r(errCode,bufErrnoMessage,SG_ERROR_BUFFER_SIZE+1),
					lenBuf-1);
#    else
			// otherwise, we get the XSI version
			strerror_r(errCode,bufErrnoMessage,SG_ERROR_BUFFER_SIZE+1);	// use thread-safe version
			strncpy(pBuf,bufErrnoMessage,lenBuf-1);
#    endif
#  endif
#endif
			_sg_error__trim_external_message(pBuf);
			return;
		}

#if defined(WINDOWS)
	case __SG_ERR__GETLASTERROR__:	// a GetLastError() based system error
		{
			_sg_FormatMessage__utf8((FORMAT_MESSAGE_FROM_SYSTEM
									 | FORMAT_MESSAGE_IGNORE_INSERTS
									 | FORMAT_MESSAGE_MAX_WIDTH_MASK),
									SG_ERR_GETLASTERROR_VALUE(err),
									pBuf, lenBuf);
			_sg_error__trim_external_message(pBuf);
			return;
		}

	//case __SG_ERR__COM__:
	// TODO...
#endif

	default:
		SG_ASSERT(0 && err);			// should not happen (quiets compiler)

	case __SG_ERR__ZLIB__:
#if defined(WINDOWS)
		_snprintf_s(pBuf,lenBuf,_TRUNCATE,"Error %d (zlib)",SG_ERR_ZLIB_VALUE(err));
#else
		snprintf(pBuf,(size_t)lenBuf,"Error %d (zlib)",SG_ERR_ZLIB_VALUE(err));
#endif
		_sg_error__trim_external_message(pBuf);
		return;

	case __SG_ERR__SQLITE__:		// an error code from sqlite
#if defined(WINDOWS)
		_snprintf_s(pBuf,lenBuf,_TRUNCATE,"Error %d (sqlite)",SG_ERR_SQLITE_VALUE(err));
#else
		snprintf(pBuf,(size_t)lenBuf,"Error %d (sqlite)",SG_ERR_SQLITE_VALUE(err));
#endif
		_sg_error__trim_external_message(pBuf);
		return;

#ifndef SG_IOS
	case __SG_ERR__ICU__:			// an ICU error code
#if defined(WINDOWS)
		_snprintf_s(pBuf,lenBuf,_TRUNCATE,"Error %d (icu): %s",
				  SG_ERR_ICU_VALUE(err),
				  SG_utf8__get_icu_error_msg(err));
#else
		snprintf(pBuf,(size_t)lenBuf,"Error %d (icu): %s",
				 SG_ERR_ICU_VALUE(err),
				 SG_utf8__get_icu_error_msg(err));
#endif
		_sg_error__trim_external_message(pBuf);
		return;

	case __SG_ERR__LIBCURL__:		// a libcurl error code
#if defined(WINDOWS)
		_snprintf_s(pBuf,lenBuf,_TRUNCATE,"Error %d (curl): %s",
			SG_ERR_LIBCURL_VALUE(err),
			curl_easy_strerror(SG_ERR_LIBCURL_VALUE(err)));
#else
		snprintf(pBuf,(size_t)lenBuf,"Error %d (curl): %s",
			SG_ERR_LIBCURL_VALUE(err),
			curl_easy_strerror(SG_ERR_LIBCURL_VALUE(err)));
#endif
		_sg_error__trim_external_message(pBuf);
		return;
#endif // !SG_IOS

	case __SG_ERR__SG_LIBRARY__:	// a library error
		switch (err)
		{
#if defined(WINDOWS)
#define E(e,m)	case e: \
					if (bDetailed) \
						_snprintf_s(pBuf,lenBuf,_TRUNCATE, "Error %d (sglib): " m,SG_ERR_SG_LIBRARY_VALUE(e)); \
					else \
						_snprintf_s(pBuf,lenBuf,_TRUNCATE, "Error: " m); \
					return;
#else
#define E(e,m)	case e: \
					if (bDetailed) \
						snprintf(pBuf,(size_t)lenBuf,"Error %d (sglib): " m,SG_ERR_SG_LIBRARY_VALUE(e)); \
					else \
						snprintf(pBuf,(size_t)lenBuf, "Error: " m);	\
					return;
#endif

			E(SG_ERR_UNSPECIFIED,                                   "Unspecified error");
			E(SG_ERR_INVALIDARG,                                    "Invalid argument");
			E(SG_ERR_NOTIMPLEMENTED,                                "Not implemented");
			E(SG_ERR_UNINITIALIZED,                                 "Uninitialized");
			E(SG_ERR_MALLOCFAILED,                                  "Memory allocation failed");
			E(SG_ERR_INCOMPLETEWRITE,                               "Incomplete write");
			E(SG_ERR_REPO_ALREADY_OPEN,                             "Repository already open");
			E(SG_ERR_NOTAREPOSITORY,                                "Not a repository");
			E(SG_ERR_NOTAFILE,                                      "Not a file");
			E(SG_ERR_COLLECTIONFULL,                                "Collection full");
			E(SG_ERR_USERNOTFOUND,                                  "User not found");
			E(SG_ERR_INVALIDHOMEDIRECTORY,                          "Invalid home directory");
			E(SG_ERR_CANNOTTRIMROOTDIRECTORY,                       "Cannot get parent directory of root directory");
			E(SG_ERR_LIMIT_EXCEEDED,                                "A fixed limit has been exceeded");
			E(SG_ERR_OVERLAPPINGBUFFERS,                            "Overlapping buffers in memory copy");
			E(SG_ERR_REPO_NOT_OPEN,                                 "Repository not open");
			E(SG_ERR_NOMOREFILES,                                   "No more files in directory");
			E(SG_ERR_BUFFERTOOSMALL,                                "Buffer too small");
			E(SG_ERR_INVALIDBLOBOPERATION,                          "Invalid blob operation");
			E(SG_ERR_BLOBVERIFYFAILED,                              "Blob verification failed");
			E(SG_ERR_DBRECORDFIELDNAMEDUPLICATE,                    "Field name already exists");
			E(SG_ERR_DBRECORDFIELDNAMEINVALID,                      "Field name invalid");
			E(SG_ERR_INDEXOUTOFRANGE,                               "Index out of range");
			E(SG_ERR_DBRECORDFIELDNAMENOTFOUND,                     "Field name not found");
			E(SG_ERR_UTF8INVALID,                                   "UTF-8 string invalid");
			E(SG_ERR_XMLWRITERNOTWELLFORMED,                        "This operation would result in non-well-formed XML");
			E(SG_ERR_EOF,                                           "End of File");
			E(SG_ERR_INCOMPLETEREAD,                                "Incomplete read");
			E(SG_ERR_VCDIFF_NUMBER_ENCODING,                        "Invalid vcdiff number encoding");
			E(SG_ERR_VCDIFF_UNSUPPORTED,                            "Unsupported vcdiff feature");
			E(SG_ERR_VCDIFF_INVALID_FORMAT,                         "Invalid vcdiff file");
			E(SG_ERR_DAGLCA_LEAF_IS_ANCESTOR,                       "Cannot merge a changeset with an ancestor");
			E(SG_ERR_CANNOT_DO_FF_MERGE,                            "Cannot do fast-forward merge");
			E(SG_ERR_ASSERT,                                        "Assert failed");
			E(SG_ERR_CHANGESETDIRALREADYEXISTS,                     "Changeset directory already exists");
			E(SG_ERR_INVALID_BLOB_DIRECTORY,                        "Invalid blob directory");
			E(SG_ERR_COMPRESSION_NOT_HELPFUL,                       "Compression not helpful");
			E(SG_ERR_INVALID_BLOB_HEADER,                           "The blob header is invalid");
			E(SG_ERR_UNSUPPORTED_BLOB_VERSION,                      "Unsupported Blob Version");
			E(SG_ERR_BLOB_NOT_VERIFIED_INCOMPLETE,                  "The blob could not be verified because it is incomplete");
			E(SG_ERR_BLOB_NOT_VERIFIED_MISMATCH,                    "The blob could not be verified: the calculated HID does not match the stored HID");
			E(SG_ERR_JSONWRITERNOTWELLFORMED,                       "The JSON is not well-formed");
			E(SG_ERR_JSONPARSER_SYNTAX,                             "JSON syntax error in parser");
			E(SG_ERR_VHASH_DUPLICATEKEY,                            "The vhash key already exists");
			E(SG_ERR_VHASH_KEYNOTFOUND,                             "vhash key could not be found");
			E(SG_ERR_VARIANT_INVALIDTYPE,                           "Invalid variant type");
			E(SG_ERR_NOT_A_DIRECTORY,                               "A file was found where a directory was expected");
			E(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE,                     "Varray index out of bounds");
			E(SG_ERR_TREENODE_ENTRY_VALIDATION_FAILED,              "Treenode entry validation failed"); // TODO: better error message?
			E(SG_ERR_BLOB_NOT_FOUND,                                "Blob not found");
			E(SG_ERR_INVALID_WHILE_FROZEN,                          "The requested operation cannot be performed because the object is frozen");
			E(SG_ERR_INVALID_UNLESS_FROZEN,                         "The requested operation cannot be performed unless the object is frozen");
			E(SG_ERR_ARGUMENT_OUT_OF_RANGE,                         "An argument is outside the valid range");
			E(SG_ERR_CHANGESET_VALIDATION_FAILED,                   "Changeset validation failed");
			E(SG_ERR_TREENODE_VALIDATION_FAILED,                    "Treenode validation failed");
			E(SG_ERR_TRIENODE_VALIDATION_FAILED,                    "Trienode validation failed");
			E(SG_ERR_INVALID_DBCRITERIA,                            "Invalid database criteria");  // TODO: better error message?
			E(SG_ERR_DBRECORD_VALIDATION_FAILED,                    "Database record validation failed");
			E(SG_ERR_DAGNODE_ALREADY_EXISTS,                        "The DAG node already exists");
			E(SG_ERR_NOT_FOUND,                                     "The requested object could not be found");
			E(SG_ERR_CANNOT_NEST_DB_TX,                             "Cannot nest database transactions");
			E(SG_ERR_NOT_IN_DB_TX,                                  "The operation cannot be performed outside the scope of a database transaction");
			E(SG_ERR_VTABLE_NOT_BOUND,                              "The vtable is not bound");
			E(SG_ERR_REPO_DB_NOT_OPEN,                              "The repository database is not open");
			E(SG_ERR_DAG_NOT_CONSISTENT,                            "The DAG is inconsistent");
			E(SG_ERR_DB_BUSY,                                       "The repository database is busy");
			E(SG_ERR_CANNOT_CREATE_SPARSE_DAG,                      "Cannot create sparse DAG");  // TODO: change this to refer to unknown parents rather than sparse dags?
			E(SG_ERR_RBTREE_DUPLICATEKEY,                           "The rbtree key already exists");
			E(SG_ERR_RESTART_FOREACH,                               "Restarting foreach"); // TODO: Remove this.  Pretty sure it's an error that shouldn't be.
			E(SG_ERR_SPLIT_MOVE,                                    "Split move");
			E(SG_ERR_PENDINGTREE_OBJECT_ALREADY_EXISTS,             "The file/folder already exists");
			E(SG_ERR_FILE_LOCK_FAILED,                              "File lock failed");
			E(SG_ERR_SYMLINK_NOT_SUPPORTED,                         "Symbolic links are unsupported on this platform");
			E(SG_ERR_USAGE,                                         "Usage error");
			E(SG_ERR_UNMAPPABLE_UNICODE_CHAR,                       "Unmappable unicode character"); // TODO: better error message?
			E(SG_ERR_ILLEGAL_CHARSET_CHAR,                          "Illegal charset character"); // TODO: better error message?
			E(SG_ERR_INVALID_PENDINGTREE_OBJECT_TYPE,               "Invalid pending tree object type");
			E(SG_ERR_PORTABILITY_WARNINGS,                          "There are portability warnings");		// TODO deprecate this -- see SG_ERR_WC_PORT_FLAGS

			E(SG_ERR_WC_PORT_FLAGS,                                 "Portability conflict");
			E(SG_ERR_PATH_NOT_IN_WORKING_COPY,                      "Path not in working copy");
			E(SG_ERR_WC_CANNOT_APPLY_CHANGES_IN_READONLY_TX,        "Cannot apply changes to working copy while in a read-only transaction");
			E(SG_ERR_WC_IS_SPARSE,                                  "Cannot make changes in a sparse portion of the tree");
			E(SG_ERR_WC_PATH_NOT_UNIQUE,                            "Path refers to multiple items");
			E(SG_ERR_WC_PARTIAL_COMMIT,                             "Consistency error in partial commit");
			E(SG_ERR_WC_ITEM_BLOCKS_REVERT,                         "An item will interfere with revert; please move it out of the way");
			E(SG_ERR_WC__MOVE_DESTDIR_NOT_FOUND,                    "Destination directory not found");
			E(SG_ERR_WC__MOVE_DESTDIR_UNCONTROLLED,                 "Destination directory not under version control");
			E(SG_ERR_WC__MOVE_DESTDIR_PENDING_DELETE,               "Destination directory has pending delete");
			E(SG_ERR_WC__REVERT_BLOCKED_BY_DESTDIR,                 "Revert cannot move/restore item because of missing/deleted parent directory");

			E(SG_ERR_CANNOT_REMOVE_SAFELY,                          "The object cannot be safely removed");
			E(SG_ERR_TOO_MANY_BACKUP_FILES,                         "The number of supported backup files has been exceeded");
			E(SG_ERR_TAG_ALREADY_EXISTS,                            "The tag already exists");
			E(SG_ERR_TAG_CONFLICT,                                  "The tag conflicts with an existing tag");
			E(SG_ERR_AMBIGUOUS_ID_PREFIX,                           "The HID prefix does not uniquely identify a dagnode or blob");
			E(SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION,                "Unknown repository storage implementation");
			E(SG_ERR_DBNDX_ALLOWS_QUERY_ONLY,                       "The database index is in a query-only state");
			E(SG_ERR_ZIP_BAD_FILE,                                  "Bad zip file");
			E(SG_ERR_ZIP_CRC,                                       "Zip file CRC mismatch");
			E(SG_ERR_DAGFRAG_DESERIALIZATION_VERSION,               "Serialized DAG fragment has an invalid version");
			E(SG_ERR_DAGNODE_DESERIALIZATION_VERSION,               "Serialized DAG node has an invalid version");
			E(SG_ERR_FRAGBALL_INVALID_VERSION,                      "Invalid fragball version");
			E(SG_ERR_FRAGBALL_BLOB_HASH_MISMATCH,                   "Fragball blob hash mismatch");
			E(SG_ERR_MALFORMED_SUPERROOT_TREENODE,                  "The super-root tree node is malformed");
			E(SG_ERR_ABNORMAL_TERMINATION,                          "A spawned process terminated abnormally");
			E(SG_ERR_EXTERNAL_TOOL_ERROR,                           "An external tool reported an error");
			E(SG_ERR_FRAGBALL_INVALID_OP,                           "Invalid op in fragball");
			E(SG_ERR_INCOMPLETE_BLOB_IN_REPO_TX,                    "The pending repository transaction includes an incomplete blob");
			E(SG_ERR_REPO_MISMATCH,                                 "Repository mismatch: the repositories have no common history");
			E(SG_ERR_UNKNOWN_CLIENT_PROTOCOL,                       "Unknown client protocol");
			E(SG_ERR_SAFEPTR_WRONG_TYPE,                            "Safeptr type mismatch");
			E(SG_ERR_SAFEPTR_NULL,                                  "Null safeptr");
			E(SG_ERR_JS,                                            "JavaScript error");
			E(SG_ERR_INTEGER_OVERFLOW,                              "Integer overflow");
			E(SG_ERR_JS_DBRECORD_FIELD_VALUES_MUST_BE_STRINGS,      "Database record fields must be strings"); // TODO: Better error message?
			E(SG_ERR_ALREADY_INITIALIZED,                           "Already initialized");
			E(SG_ERR_NOT_ALLOWED,                                   "Not allowed");
			E(SG_ERR_CTX_HAS_ERR,                                   "The context object already indicates an error state");
			E(SG_ERR_CTX_NEEDS_ERR,                                 "The context object does not indicate an error state");
			E(SG_ERR_LOGGING_MODULE_IN_USE,                         "Logging module already in use");
			E(SG_ERR_EXEC_FAILED,                                   "Failed to spawn external process");
			E(SG_ERR_MERGE_TARGET_EQUALS_BASELINE,                  "The merge target is equal to the baseline");
			E(SG_ERR_MERGE_TARGET_IS_ANCESTOR_OF_BASELINE,          "The merge target is an ancestor of the baseline");
			E(SG_ERR_MERGE_TARGET_IS_DESCENDANT_OF_BASELINE,        "The merge target is a descendant of the baseline");
			E(SG_ERR_MERGE_REQUESTED_CLEAN_WD,                      "This type of merge requires a clean working copy");
			E(SG_ERR_CANNOT_MOVE_INTO_CURRENT_PARENT,                "You cannot move something into the folder it is already in");
			E(SG_ERR_VALUE_CANNOT_BE_SERIALIZED_TO_JSON,            "Value is of a type that cannot be serialized to JSON");
			E(SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT,				"The branch references a changeset which is not present");
			E(SG_ERR_HTTP_400_BAD_REQUEST,                          "400 Bad Request");
			E(SG_ERR_HTTP_413_REQUEST_ENTITY_TOO_LARGE,             "413 Request Entity Too Large");
			E(SG_ERR_CANNOT_MOVE_INTO_SUBFOLDER,                    "You cannot move an object into its own subfolder");
			E(SG_ERR_DIR_ALREADY_EXISTS,                            "Directory already exists");
			E(SG_ERR_DIR_PATH_NOT_FOUND,                            "A parent directory in the pathname does not exist");
			E(SG_ERR_WORKING_DIRECTORY_IN_USE,						"The current directory already belongs to a working copy");

			E(SG_ERR_PENDINGTREE_PARTIAL_CHANGE_COLLISION,          "Entryname conflict in partial change");	// TODO give this a better message.
			E(SG_ERR_REPO_BUSY,                                     "Repository too busy");
			E(SG_ERR_PENDINGTREE_PARENT_DIR_MISSING_OR_INACTIVE,    "Parent directory missing or inactive");
			E(SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,                "Attempt to operate on an item which is not under version control");

			E(SG_ERR_UNKNOWN_BLOB_TYPE,								"Unknown blob type");
			E(SG_ERR_DIFFMERGE_EXE_NOT_FOUND,						"Could not find DiffMerge executable");

			E(SG_ERR_GETOPT_NO_MORE_OPTIONS,						"Reached end of options");
			E(SG_ERR_GETOPT_BAD_ARG,								"Bad argument");

			E(SG_ERR_GID_FORMAT_INVALID,							"Format of GID not valid");		// GID string has invalid format; does not mean it wasn't found
			E(SG_ERR_UNKNOWN_HASH_METHOD,                           "Unknown hash-method");
			E(SG_ERR_TRIVIAL_HASH_DIFFERENT,                        "Trivial hash different");
			E(SG_ERR_DAGNODES_UNRELATED,							"Changesets are unrelated");
			E(SG_ERR_MULTIPLE_HEADS_FROM_DAGNODE,					"There are multiple heads from this changeset");
			E(SG_ERR_DATE_PARSING_ERROR,							"The date could not be parsed.  Please provide either \"YYYY-MM-DD HH:MM:SS\"\nor \"YYYY-MM-DD\"");
			E(SG_ERR_INT_PARSE,							            "Invalid integer string");
			E(SG_ERR_TAG_NOT_FOUND,							        "Tag not found");

			E(SG_ERR_STAGING_AREA_MISSING,							"Couldn't find staging area");
			E(SG_ERR_NOT_A_WORKING_COPY,							"There is no working copy here");
			E(SG_ERR_REPO_ID_MISMATCH,								"The repositories are not clones");
			E(SG_ERR_ADMIN_ID_MISMATCH,								"The repositories don't belong to the same administrative group")
			E(SG_ERR_REPO_HASH_METHOD_MISMATCH,						"The repositories don't use the same hash");
			
			E(SG_ERR_AUTHORIZATION_REQUIRED,                        "Authorization required");
			E(SG_ERR_AUTHORIZATION_TOO_MANY_ATTEMPTS,               "Authorization failed -- too many attempts");
			E(SG_ERR_SERVER_HTTP_ERROR,                             "The server returned an HTTP error");
			E(SG_ERR_EXTENDED_HTTP_500,								"The server returned an error");
			E(SG_ERR_SERVER_DOESNT_ACCEPT_PUSHES,                   "The server does not accept pushes or clones, or the destination URL is invalid");
			E(SG_ERR_SERVER_DISALLOWED_REPO_CREATE_OR_DELETE,		"The server configuration does not allow creating or deleting repositories")

			E(SG_ERR_JSONDB_NO_CURRENT_OBJECT,						"No current jsondb object is set");
			E(SG_ERR_JSONDB_INVALID_PATH,							"The jsondb element path is invalid");
			E(SG_ERR_JSONDB_OBJECT_ALREADY_EXISTS,					"The jsondb object already exists");
			E(SG_ERR_JSONDB_PARENT_DOESNT_EXIST,					"The parent object doesn't exist");
			E(SG_ERR_JSONDB_NON_CONTAINER_ANCESTOR_EXISTS,			"Add not possible because a non-container ancestor exists");

			E(SG_ERR_CANNOT_MERGE_SYMLINK_VALUES,					"UPDATE cannot merge symlink values.  Consider COMMIT followed by MERGE");

			E(SG_ERR_PULL_FAILED,                                   "Pull failed");
			E(SG_ERR_PULL_INVALID_FRAGBALL_REQUEST,                 "Invalid pull request");
			E(SG_ERR_NO_SUCH_DAG,									"The requested DAG wasn't found");
			E(SG_ERR_BASELINE_NOT_HEAD,								"Baseline not a HEAD");
			E(SG_ERR_INVALID_REPO_NAME,								"The repository name provided is invalid");
			E(SG_ERR_CANNOT_COMMIT_LOST_ENTRY,                      "Cannot commit a LOST entry");

			E(SG_ERR_SUBMODULE_NESTED_REVERT_PROBLEM,               "A nested submodule could not be reverted");
			E(SG_ERR_ENTRY_ALREADY_UNDER_VERSION_CONTROL,           "The entry is already under version control");
			E(SG_ERR_SUBMODULE_NESTED_UPDATE_CHECKOUT_PROBLEM,		"A nested submodule checkout or update could not be performed");
			E(SG_ERR_SUBMODULE_UPDATE_CONFLICT,						"Conflicts in submodule prevented nested UPDATE");
			E(SG_ERR_UPDATE_CONFLICT,								"Conflicts or changes in the working copy prevented UPDATE");
			E(SG_ERR_ENTRYNAME_CONFLICT,							"Operation would produce an entryname conflict in result");
			E(SG_ERR_CANNOT_MAKE_RELATIVE_PATH,						"Cannot make a relative path using the given pathnames");
			E(SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,				"Cannot perform operation with an uncommitted MERGE");
			E(SG_ERR_INVALID_REPO_PATH,								"Invalid repository path");
			E(SG_ERR_NOTHING_TO_COMMIT,								"Nothing to COMMIT");
			E(SG_ERR_REPO_FEATURE_NOT_SUPPORTED,					"This storage implementation does not support that feature");
			E(SG_ERR_ISSUE_NOT_FOUND,                               "Issue not found");
			E(SG_ERR_CANNOT_COMMIT_WITH_UNRESOLVED_ISSUES,          "Cannot commit with unresolved issues");
			E(SG_ERR_CANNOT_PARTIAL_COMMIT_AFTER_MERGE,             "Cannot do partial commit after merge");
			E(SG_ERR_CANNOT_PARTIAL_COMMIT_WITH_DIRTY_SUBMODULE,	"Cannot do nested submodule commits for a partial commit");
			E(SG_ERR_CANNOT_PARTIAL_REVERT_AFTER_MERGE,             "Cannot do partial revert after merge");
			E(SG_ERR_DUPLICATE_ISSUE,                               "Issue duplicated")
			E(SG_ERR_NO_MERGE_TOOL_CONFIGURED,						"No external merge tool has been configured to handle files like this");
			
			E(SG_ERR_UNKNOWN_DAG_STORAGE_VERSION,					"Unknown DAG storage version");
			E(SG_ERR_UNSUPPORTED_CLOSET_VERSION,					"The closet is an unsupported version");
			E(SG_ERR_UNSUPPORTED_DRAWER_VERSION,					"The working copy is an unsupported version");

			E(SG_ERR_TOO_MANY_HEADS,								"There are too many heads");
			E(SG_ERR_DIR_NOT_EMPTY,									"The directory is not empty");

			E(SG_ERR_STAMP_NOT_FOUND,								"The stamp was not found");

			E(SG_ERR_USER_NOT_FOUND,                                "The user does not exist. Use 'vv user create' first");
			E(SG_ERR_ZING_NO_RECID,                                 "This operation is not valid because the record has no recid");
			E(SG_ERR_ZING_TYPE_MISMATCH,                            "Type mismatch");
			E(SG_ERR_ZING_INVALID_TEMPLATE,                         "Invalid template");
			E(SG_ERR_ZING_UNKNOWN_ACTION_TYPE,                      "Unknown action type in template");
			E(SG_ERR_ZING_UNKNOWN_BUILTIN_ACTION,		       		"Unknown builtin action");
			E(SG_ERR_ZING_NO_HISTORY_IF_NO_STATE,		       		"You can't ask for history if you didn't provide a db state");
			E(SG_ERR_ZING_TYPE_CHANGE_NOT_ALLOWED,		       		"Illegal to change the type of a field");
			E(SG_ERR_ZING_ILLEGAL_DURING_COMMIT,		       		"This operation is illegal during a commit");
			E(SG_ERR_ZING_TEMPLATE_NOT_FOUND,                       "Template not found");
			E(SG_ERR_ZING_ONE_TEMPLATE,                             "Only one template allowed");
			E(SG_ERR_ZING_RECORD_NOT_FOUND,                         "Record not found");
			E(SG_ERR_ZING_CONSTRAINT,                               "Constraint violation");
			E(SG_ERR_ZING_FIELD_NOT_FOUND,                          "Field not found");
			E(SG_ERR_ZING_MERGE_CONFLICT,                           "Merge conflict");
			E(SG_ERR_ZING_WHERE_PARSE_ERROR,                        "Syntax error in where");
			E(SG_ERR_ZING_SORT_PARSE_ERROR,                         "Syntax error in sort");
			E(SG_ERR_ZING_RECTYPE_NOT_FOUND,                        "Record type not found");
			E(SG_ERR_NO_CURRENT_WHOAMI,                             "No current user is set. Set one with 'vv whoami USERNAME'. Create a new user with 'vv whoami --create USERNAME'");
			E(SG_ERR_ZING_NO_ANCESTOR,                              "No ancestor found");
			E(SG_ERR_INVALID_USERID,                                "Invalid userid");
			E(SG_ERR_WRONG_DAG_TYPE,                                "Wrong DAG type");
			E(SG_ERR_ZING_NO_RECTYPE,                               "No rectype");
			E(SG_ERR_ZING_ONLY_ONE_RECTYPE_ALLOWED,                 "Only one rectype allowed in this dag");
			E(SG_ERR_REPO_ALREADY_EXISTS,                           "A repository with that name already exists");
			E(SG_ERR_BRANCH_NEEDS_MERGE,                            "The branch needs to be merged");
			E(SG_ERR_CHANGESET_NOT_IN_INDEX,                        "The requested changeset has not been indexed");
			E(SG_ERR_NO_RECURSIVE_GROUPS,                           "No recursive groups");
			E(SG_ERR_USER_ALREADY_HAS_PUBLIC_KEY,                   "The user already has a public key");
			E(SG_ERR_PRIVATE_KEY_FILE_ALREADY_EXISTS,               "The private key file already exists");
			E(SG_ERR_USER_HAS_NO_PUBLIC_KEY,                        "The user has no public key");
			E(SG_ERR_PRIVATE_KEY_FILE_NOT_FOUND,                    "Private key file not found");
			E(SG_ERR_SIGNATURE_USES_OLD_KEY,                        "Signature verification failure: public key is not current");
			E(SG_ERR_SIGNATURE_FORMAT_UNKNOWN,                      "Signature format unknown");

			E(SG_ERR_SUBMODULE_TECHNOLOGY_MISMATCH,                 "Submodule technology mismatch");
			E(SG_ERR_PATH_IN_ANOTHER_SUBMODULE,                     "The pathname references another submodule");
			E(SG_ERR_HDB_DUPLICATE_KEY,                             "Duplicate key in hdb file");
			E(SG_ERR_HDB_CANNOT_ROLLBACK,                           "Cannot rollback hdb");
			E(SG_ERR_NOT_TIED,                                      "The working copy is not attached to a branch");
			E(SG_ERR_BRANCH_DOES_NOT_NEED_MERGE,                    "This branch does not need to be merged");
			E(SG_ERR_TIED_BRANCH_NOT_BASELINE,                      "The working copy's parent is not a head of the attached branch");
			E(SG_ERR_JSON_WRONG_TOP_TYPE,                           "Wrong type of JSON object");
			E(SG_ERR_BRANCH_NOT_FOUND,                              "Branch does not exist");

			E(SG_ERR_OLD_AUDITS,                                    "Old repository format.  Use 'vv upgrade' to upgrade it");
			E(SG_ERR_CANCEL,                                        "The operation was cancelled");
			E(SG_ERR_JS_NAMED_PARAM_REQUIRED,                       "Missing required named parameter");
			E(SG_ERR_JS_NAMED_PARAM_NOT_ALLOWED,                    "Invalid parameter name");
			E(SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION,                "Named parameter type conversion error");

			E(SG_ERR_MUTEX_FAILURE,                                 "A mutex operation failed");
			E(SG_ERR_CHANGESET_BLOB_NOT_FOUND,                      "The requested changeset is not present in the repository");
			E(SG_ERR_VC_LOCK_FILES_ONLY,                            "You can only lock files");
			E(SG_ERR_VC_LOCK_BRANCH_MUST_BE_CURRENT,                "To place a lock, your working copy must be updated");
			E(SG_ERR_VC_LOCK_ALREADY_LOCKED,                        "Already locked");
			E(SG_ERR_VC_LOCK_VIOLATION,                             "Lock violation");
			E(SG_ERR_VC_LOCK_DUPLICATE,                             "Lock duplicate");
			E(SG_ERR_VC_LOCK_NOT_FOUND,                             "Lock not found");
			E(SG_ERR_VC_LOCK_NOT_COMMITTED_YET,                     "Cannot lock a file which is not yet committed to the repository");
			E(SG_ERR_VC_LOCK_CANNOT_DELETE_ENDED,                   "Cannot unlock something which is already ended");
			E(SG_ERR_VC_HOOK_MISSING,                               "A version control hook function is required for this operation");
			E(SG_ERR_VC_HOOK_REFUSED,                               "A version control hook function failed");

			E(SG_ERR_VC_LINE_HISTORY_NOT_COMMITTED_YET,             "Cannot perform line history on a file which is not yet committed to the repository");
			E(SG_ERR_VC_LINE_HISTORY_WORKING_COPY_MODIFIED,         "The given lines of the file have been modified in the working copy");
			
			E(SG_ERR_BRANCH_ALREADY_CLOSED,                         "Branch already closed");
			E(SG_ERR_BRANCH_NOT_CLOSED,                             "Branch not closed");
			E(SG_ERR_BRANCH_ALREADY_EXISTS,                         "Branch already exists");
			E(SG_ERR_BRANCH_ADD_ANCESTOR,                           "Cannot add branch head due to ancestor");
			E(SG_ERR_BRANCH_ADD_DESCENDANT,                         "Cannot add branch head due to descendant");

			E(SG_ERR_INVALID_REV_SPEC,                              "The revision specifier is invalid");
			E(SG_ERR_SOURCE_AND_DEST_REPO_SAME,                     "The source and destination repositories are the same");
			E(SG_ERR_EMPTYCOMMITMESSAGE,                            "Commit cancelled: empty commit message");

			E(SG_ERR_UNKNOWN_SYNC_PROTOCOL_VERSION,					"The server is using a version of the sync protocol this client does not support");
			E(SG_ERR_UNKNOWN_STAGING_VERSION,						"The staging area is an unknown version")

			E(SG_ERR_RESOLVE_INVALID_SAVE_DATA,                     "Saved resolve data is invalid");
			E(SG_ERR_RESOLVE_TOOL_NOT_FOUND,                        "A tool wasn't found for resolve to use");
			E(SG_ERR_RESOLVE_INVALID_VALUE,                         "A resolve value was used in an invalid way");
			E(SG_ERR_RESOLVE_INVALID_TARGET,                        "The item being resolved was invalid");
			E(SG_ERR_RESOLVE_ALREADY_RESOLVED,                      "The conflict is already resolved as specified");

			E(SG_ERR_PUSH_WOULD_CREATE_NEW_HEADS,					"The push would create new heads in a named branch");
			E(SG_ERR_GOTTA_BE_SOMEBODY,				            	"Cannot be nobody");
			E(SG_ERR_USER_ALREADY_EXISTS,			            	"A user with that name already exists");
			E(SG_ERR_MALFORMED_GIT_FAST_IMPORT,		            	"Malformed git-fast-import");

			E(SG_ERR_JSMIN_ERROR,									"Error minimizing JavaScript");
			E(SG_ERR_NEED_REINDEX,									"You need to 'vv reindex'");

			E(SG_ERR_INACTIVE_USER,									"The specified user is inactive");
			E(SG_ERR_AREA_NOT_FOUND,								"Invalid area");

			E(SG_ERR_FILE_ALREADY_EXISTS,							"File already exists");
			E(SG_ERR_DBNDX_FILTER_TIMEOUT,							"Can't wait any longer for dbndx prep");

			E(SG_ERR_UNSUPPORTED_USERMAP_VERSION,					"The usermap is an unsupported version");
			E(SG_ERR_INVALID_DESCRIPTOR,							"The repository descriptor is invalid");
			E(SG_ERR_REPO_UNAVAILABLE,								"The repository is unavailable");
			E(SG_ERR_CLOSET_PROP_ALREADY_EXISTS,					"The closet property already exists");
			E(SG_ERR_NO_SUCH_CLOSET_PROP,							"The closet property does not exist");
			E(SG_ERR_PASSWORD_STORAGE_FAIL,							"Error accessing password storage");
			
			E(SG_ERR_SIGINT,										"The process received an interrupt signal");
			
			E(SG_ERR_NO_REPO_TX,										"Not in a repository transaction");
			E(SG_ERR_ONLY_ONE_REPO_TX,										"Only one repository transaction allowed");
			E(SG_ERR_BAD_REPO_TX_HANDLE,										"Bad repository transaction handle");
			
			E(SG_ERR_VC_HOOK_AMBIGUOUS,                               "A version control hook has been defined multiple times");
			E(SG_ERR_DB_DELTA,                                       "db delta problem");
			E(SG_ERR_MALFORMED_SVN_DUMP,                             "Malformed svn dump");
			E(SG_ERR_MALFORMED_SVN_DIFF,                             "Malformed svndiff");

			E(SG_ERR_HTTP_401,                                       "Authentication failed");
			E(SG_ERR_HTTP_404,                                       "HTTP 404");
			E(SG_ERR_HTTP_502,                                       "HTTP 502");
			





			E(SG_ERR_MUST_ATTACH_OR_DETACH,							"The proper branch cannot be determined automatically; please use one of the attach or detached options");
			E(SG_ERR_INVALID_BRANCH_NAME,							"Invalid branch name");
			E(SG_ERR_NOT_IN_A_MERGE,								"Not in a merge");
			E(SG_ERR_NO_SERVER_SPECIFIED,							"No default path and no server specified");
			E(SG_ERR_VC_LOCK_CANNOT_LOCK_DELETED_ITEM,				"Cannot lock a file with pended delete");
			E(SG_ERR_NOT_ALLOWED_WITH_LOST_ITEM,					"Operation not allowed with a lost item");
			E(SG_ERR_NO_EFFECT,	                                    "No effect");
			E(SG_ERR_WC_PORT_DUPLICATE,                             "Duplicate entryname");
			E(SG_ERR_UNRESOLVED_ITEM_WOULD_BE_AFFECTED,             "An unresolved item would be affected");
			E(SG_ERR_RESOLVE_NEEDS_OVERWRITE,                       "The conflict has already been resolved.  Use --overwrite to accept a different value");
			E(SG_ERR_DAGLCA_NO_ANCESTOR,                            "DAGLCA could not find a common ancestor");
			E(SG_ERR_WC_RESERVED_ENTRYNAME,                         "Reserved entryname");
			E(SG_ERR_UPDATE_FROM_NEW_BRANCH,                        "The Working copy is attached to a new branch; you must specify a target changeset for update");
			E(SG_ERR_WC_DB_BUSY,									"The working copy database file is busy");
			E(SG_ERR_NO_CHANGESET_WITH_PREFIX,						"Changeset not found or none found with that prefix");
			E(SG_ERR_UNSUPPORTED_DEVICE_SPECIAL_FILE,	            "Device and special files are not supported");
			E(SG_ERR_WC_HAS_MULTIPLE_PARENTS,						"The working copy has multiple parents");
			E(SG_ERR_ITEM_NOT_PRESENT_IN_PARENTS,	                "Item is not present in either parent");
			E(SG_ERR_DIFFTOOL_CANCELED,                             "The difftool was canceled");

#if defined(DEBUG)
			E(SG_ERR_DEBUG_1,                                       "Debug error 1");
#endif

			// Add new error messages above this comment... Also, note: When
			// SG_context__err_to_string constructs a more specific/complete report,
			// it will append either a period or a colon to the end of the message
			// (depending on whether more details follow). For that reason, we put
			// no punctuation at the end here (test u0000_error_message even checks
			// for this).
#undef E

		default:
#if defined(WINDOWS)
			_snprintf_s(pBuf,lenBuf,_TRUNCATE,"Error %d (sglib)",(int)err);
#else
			snprintf(pBuf,(size_t)lenBuf,"Error %d (sglib)",(int)err);
#endif
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////
// SG_assert__release() is used to generate an SG_ERR_ASSERT and
// let the program continue.  It ***DOES NOT*** call the CRT abort()
// or assert() routines/macros.
//
// These are defined in both DEBUG and RELEASE builds and always do
// the same thing.

#if defined(MAC)

void SG_assert__release(SG_context* pCtx, const char * szExpr, const char * szFile, int line)
{
	SG_context__err(pCtx, SG_ERR_ASSERT, szFile, line, szExpr); // set a breakpoint on this to catch assertions.
}

#elif defined(LINUX)

void SG_assert__release(SG_context* pCtx, const char * szExpr, const char * szFile, int line)
{
	SG_context__err(pCtx, SG_ERR_ASSERT, szFile, line, szExpr); // set a breakpoint on this to catch assertions.
}

#elif defined(WINDOWS)

void SG_assert__release(SG_context* pCtx, const char * szExpr, const char * szFile, int line)
{
	SG_context__err(pCtx, SG_ERR_ASSERT, szFile, line, szExpr); // set a breakpoint on this to catch assertions.
}

#endif // WINDOWS

//////////////////////////////////////////////////////////////////
// SG_assert__debug() is used ***IN DEBUG BUILDS ONLY*** to wrap the
// the call to abort() and/or assert().  We do this so that we don't
// get bogus false-positives on code coverage throughout the code.
//
// The problem is that assert() usually has a test within it and gcov/lcov/zcov
// sees it and reports that we never took the assert and skews our numbers.
// We use this function to hide that so that the code doing the SG_ASSERT()
// doesn't get dinged.
#if defined(DEBUG)
void SG_assert__debug(SG_bool bExpr, const char * pszExpr, const char * pszFile, int line)
{
	if (bExpr)
		return;

	fprintf(stderr,"SG_assrt__debug: [%s] on %s:%d\n",pszExpr,pszFile,line);

	// let platform assert() take care of hooking into the debugger and/or stopping the program.

#if defined(MAC) || defined(LINUX)
	assert(bExpr);
#elif defined(WINDOWS)
	_ASSERTE(bExpr);
#if defined(SG_REAL_BUILD)
	/* On the Windows build machine, assert failures don't raise a dialog.
	 * See the calls to _CrtSetReportMode in SG_lib__global_initialize for
	 * where this behavior is set up.
	 *
	 * We abort here to ensure the build reports failure. */
	abort();
#endif
#endif
}
#endif
