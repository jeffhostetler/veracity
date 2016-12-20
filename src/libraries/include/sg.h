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

// sg.h
// Main header file for SourceGear core library.
//////////////////////////////////////////////////////////////////

#ifndef H_SG_H
#define H_SG_H

//////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#include <sg_defines.h>

#ifdef __cplusplus
}
#endif
//////////////////////////////////////////////////////////////////
// system headers

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Ignore static analysis warnings in Windows headers. There are many. */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include<codeanalysis\sourceannotations.h>
#endif

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define _CRTDBG_MAP_ALLOC // Get source file details from CRT leak check dumps

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#if defined(MAC) || defined(LINUX)
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/file.h>
#else

#include <direct.h>
#include <wchar.h>
#include <windows.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include <TlHelp32.h>

#if defined(DEBUG)
#include <crtdbg.h>
#endif

#endif

#if !defined(SG_IOS)

#if defined(DEBUG)
#define JS_NO_JSVAL_JSID_STRUCT_TYPES
#endif
#include <js/jsapi.h>

#include <curl/curl.h>

#endif // !defined(SG_IOS)

#include <sqlite3.h>

#if defined(SG_CRYPTO)
#include <openssl/err.h>
#endif

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Stop ignoring static analysis warnings. */
#pragma warning(pop)
#endif

#ifdef __cplusplus
extern "C" {
#endif
//////////////////////////////////////////////////////////////////
// put the headers that define basic types first.
// we only include public header files in src/libraries/include.

typedef struct _sg_context SG_context;

//////////////////////////////////////////////////////////////////
// all free-callbacks (both rbtree and vector and ...) now use the same prototype

typedef void (SG_free_callback)(SG_context* pCtx, void * pVoidData);
typedef void (SG_copy_callback)(SG_context* pCtx, void * pVoidData, void ** pVoidCopy);

//////////////////////////////////////////////////////////////////

#include <sg_stdint.h>
#include <sg_bool.h>
#include <sg_error_typedefs.h>

// forward declarations for opaque and non-opaque types.

typedef struct _SG_dir			SG_dir;			// for reading a directory
typedef struct _sg_fsobj_stat	SG_fsobj_stat;	// not opaque
typedef struct _SG_string		SG_string;
typedef struct _SG_list         SG_list;
typedef struct _SG_pathname		SG_pathname;

typedef struct _SG_strpool      SG_strpool;
typedef struct _SG_varpool      SG_varpool;
typedef struct _SG_vhash        SG_vhash;
typedef struct _SG_ihash        SG_ihash;
typedef struct _SG_varray       SG_varray;

typedef struct _SG_jsonwriter   SG_jsonwriter;
typedef struct _SG_jsonparser   SG_jsonparser;

#include <sg_file_typedefs.h>
#include <sg_misc_utils.h>

#include <sg_str_utils.h>

#include <sg_nullfree.h>

#include <sg_gid_typedefs.h>
#include <sg_hid_typedefs.h>
#include <sg_tid_typedefs.h>
#include <sg_blob_typedefs.h>
#include <sg_changeset_typedefs.h>
#include <sg_closet_typedefs.h>
#include <sg_committing_typedefs.h>
#include <sg_console_typedefs.h>
#include <sg_exec_typedefs.h>
#include <sg_exec_argvec_typedefs.h>
#include <sg_thread_typedefs.h>
#include <sg_validate_typedefs.h>
#include <sg_context_typedefs.h>
#include <sg_environment_typedefs.h>
#include <sg_dagnode_typedefs.h>
#include <sg_dagfrag_typedefs.h>
#include <sg_zing_typedefs.h>
#include <sg_dbrecord_typedefs.h>
#include <sg_repo_typedefs.h>
#include <sg_treenode_entry_typedefs.h>
#include <sg_treenode_typedefs.h>
#include <sg_rbtree_typedefs.h>
#include <sg_fragball_typedefs.h>
#include <sg_daglca_typedefs.h>
#include <sg_stringarray_typedefs.h>
#include <sg_repopath_typedefs.h>
#include <sg_fsobj_typedefs.h>
#include <sg_getopt.h>
#include <sg_revision_specifier_typedefs.h>
#include <sg_file_spec_typedefs.h>
#include <sg_variant.h>
#include <sg_vfile_typedefs.h>
#include <sg_lib_typedefs.h>
#include <sg_utf8_typedefs.h>
#include <sg_localsettings_typedefs.h>
#include <sg_pendingdb_typedefs.h>
#include <sg_zip_typedefs.h>
#include <sg_unzip_typedefs.h>
#include <sg_dagnum_typedefs.h>
#include <sg_dagquery_typedefs.h>
#include <sg_clone_typedefs.h>
#include <sg_staging_typedefs.h>
#include <sg_sync_typedefs.h>
#include <sg_audit_typedefs.h>
#include <sg_stream_typedefs.h>
#include <sg_vector_typedefs.h>
#include <sg_vector_i64_typedefs.h>
#include <sg_bitvector_typedefs.h>
#include <sg_filetool_typedefs.h>
#include <sg_mergetool_typedefs.h>
#include <sg_difftool_typedefs.h>
#include <sg_filediff_typedefs.h>
#include <sg_textfilediff_typedefs.h>
#include <sg_jsondb_typedefs.h>
#include <sg_sync_client_typedefs.h>
#include <sg_push_typedefs.h>
#include <sg_pull_typedefs.h>
#include <sg_xmlwriter_typedefs.h>
#include <sg_mutex_typedefs.h>
#include <sg_history.h>
#include <sg_httprequestprofiler_typedefs.h>
#include <sg_rbtreedb.h>
#include <sg_timestamp_cache.h>
#include <sg_version.h>
#include <sg_vc_branches_typedefs.h>
#include <sg_vc_locks_typedefs.h>
#include <sg_vc_hooks_typedefs.h>
#include <sg_hdb_typedefs.h>
#include <sg_tncache_typedefs.h>
#include <sg_blobset_typedefs.h>
#include <sg_verify_typedefs.h>

#ifndef SG_IOS
#include <sg_libcurl_typedefs.h>
#include <sg_cbuffer_typedefs.h>
#include <sg_jscontextpool_typedefs.h>
#include <sg_uridispatch_typedefs.h>
#endif

#include <sg_log_typedefs.h>
#include <sg_log_text_typedefs.h>
#include <sg_log_console_typedefs.h>

#include <sg_limits.h>
#include <sg_ridesc_defines.h>
#include <sg_user_typedefs.h>
#include <sg_usermap_typedefs.h>
#include <sg_password_typedefs.h>

//////////////////////////////////////////////////////////////////

typedef void (SG_ut__prompt_for_comment__callback)(SG_context * pCtx,
												   SG_bool bForCommit,
												   const char * pszUserName,
												   const char * pszBranchName,
												   const SG_varray * pvaStatusCanonical,
												   const SG_varray * pvaDescs,
												   char ** ppszMessage);

//////////////////////////////////////////////////////////////////

// headers that build upon basic types and/or reference opaque types.

#include <sg_validate_prototypes.h>
#include <sg_revision_specifier_prototypes.h>
#include <sg_area_prototypes.h>
#include <sg_blobset_prototypes.h>
#include <sg_tncache_prototypes.h>
#include <sg_hdb_prototypes.h>
#include <sg_httprequestprofiler_prototypes.h>
#include <sg_cert_prototypes.h>
#include <sg_mutex_prototypes.h>
#include <sg_error_prototypes.h>
#include <sg_exec_prototypes.h>
#include <sg_exec_argvec_prototypes.h>
#include <sg_thread_prototypes.h>
#include <sg_context_prototypes.h>
#include <sg_environment_prototypes.h>
#include <sg_sync_remote_prototypes.h>
#include <sg_push_prototypes.h>
#include <sg_sync_client_prototypes.h>
#include <sg_clone_prototypes.h>
#include <sg_staging_prototypes.h>
#include <sg_pull_prototypes.h>
#include <sg_fragball_prototypes.h>
#include <sg_stream_prototypes.h>
#include <sg_sync_prototypes.h>
#include <sg_unzip_prototypes.h>
#include <sg_zip_prototypes.h>
#include <sg_dagnum_prototypes.h>
#include <sg_verify_prototypes.h>
#include <sg_vc_stamps_prototypes.h>
#include <sg_vc_comments_prototypes.h>
#include <sg_pendingdb_prototypes.h>
#include <sg_audit_prototypes.h>
#include <sg_db_prototypes.h>
#include <sg_vc_hooks_prototypes.h>
#include <sg_vc_branches_prototypes.h>
#include <sg_vc_locks_prototypes.h>
#include <sg_vc_tags_prototypes.h>
#include <sg_attributes_prototypes.h>
#include <sg_vfile_prototypes.h>
#include <sg_repopath_prototypes.h>
#include <sg_stringarray_prototypes.h>
#include <sg_rbtree_prototypes.h>
#include <sg_closet_prototypes.h>
#include <sg_sqlite_prototypes.h>
#include <sg_dagfrag_prototypes.h>
#include <sg_daglca_prototypes.h>
#include <sg_dagnode_prototypes.h>
#include <sg_dagquery_prototypes.h>
#include <sg_file_prototypes.h>
#include <sg_fsobj_prototypes.h>
#include <sg_vector_prototypes.h>
#include <sg_vector_i64_prototypes.h>
#include <sg_bitvector_prototypes.h>
#include <sg_jsondb_prototypes.h>
#include <sg_mergereview_prototypes.h>
#include <sg_mergetool_prototypes.h>
#include <sg_difftool_prototypes.h>
#include <sg_user_prototypes.h>
#include <sg_usermap_prototypes.h>
#include <sg_xmlwriter_prototypes.h>
#include <sg_filetool_prototypes.h>
#include <sg_fast_import_prototypes.h>
#include <sg_fast_export_prototypes.h>
#include <sg_svn_load_prototypes.h>

#ifndef SG_IOS
#include <sg_workingdir_prototypes.h>
#include <sg_vv_utils_prototypes.h>
#include <sg_vv_verbs_prototypes.h>
#include <sg_cbuffer_prototypes.h>
#include <sg_jscore_prototypes.h>
#include <sg_jscontextpool_prototypes.h>
#include <sg_jsmin_prototypes.h>
#include <sg_js_safeptr_prototypes.h>
#include <sg_jsglue_prototypes.h>
#include <sg_zing_jsglue_prototypes.h>
#include <sg_uridispatch_prototypes.h>
#include <sg_libcurl_prototypes.h>
#endif

#include <sg_log_prototypes.h>
#include <sg_log_text_prototypes.h>
#include <sg_log_console_prototypes.h>


#include <sg_base64.h>
#include <sg_hex.h>
#include <sg_dir.h>
#include <sg_dagwalker.h>
#include <sg_string.h>
#include <sg_malloc.h>
#include <sg_dbcriteria.h>
#include <sg_varray.h>
#include <sg_vhash.h>
#include <sg_ihash.h>
#include <sg_varpool.h>
#include <sg_strpool.h>
#include <sg_jsonwriter.h>
#include <sg_jsonparser.h>
#include <sg_vcdiff.h>
#include <sg_utf8_prototypes.h>
#include <sg_localsettings_prototypes.h>
#include <sg_pathname.h>
#include <sg_tempfile.h>
#include <sg_time.h>
#include <sg_repo_api.h>
#include <sg_repo_prototypes.h>
#include <sg_repo_utils.h>
#include <sg_filediff_prototypes.h>
#include <sg_textfilediff_prototypes.h>
#include <sg_linediff.h>

#include <sg_gid_prototypes.h>
#include <sg_tid_prototypes.h>
#include <sg_changeset_prototypes.h>
#include <sg_committing_prototypes.h>
#include <sg_console_prototypes.h>
#include <sg_zing_prototypes.h>
#include <sg_dbrecord_prototypes.h>
#include <sg_treenode_entry_prototypes.h>
#include <sg_treenode_prototypes.h>
#include <sg_apple_unicode_prototypes.h>
#include <sg_lib_prototypes.h>
#include <sg_random_prototypes.h>
#include <sg_file_spec_prototypes.h>
#include <sg_web_utils_prototypes.h>
#include <sg_password_prototypes.h>
#include <sg_wc__public_typedefs.h>
#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////

#endif//H_SG_H
