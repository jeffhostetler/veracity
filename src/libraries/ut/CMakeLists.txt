# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Copyright 2010-2013 SourceGear, LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

set(C_SOURCES
sg_apple_unicode.c
sg_area.c
sg_attributes.c
sg_audit.c
sg_base64.c
sg_bitvector.c
sg_blobset.c
sg_difftool.c
sg_changeset.c
sg_sync_client.c
sg_sync_client__api.c
sg_sync_client_vtable__c.c
sg_clone.c
sg_closet.c
sg_committing.c
sg_console.c
sg_context.c
sg_dagfrag.c
sg_daglca.c
sg_dagnode.c
sg_dagnum.c
sg_dagquery.c
sg_dagwalker.c
sg_dbrecord.c
sg_dir.c
sg_environment.c
sg_error.c
sg_exec.c
sg_exec_argvec.c
sg_fast_export.c
sg_fast_import.c
sg_file.c
sg_filediff.c
sg_filetool.c
sg_fragball.c
sg_fsobj.c
sg_gid.c
sg_hdb.c
sg_hex.c
sg_history.c
sg_httprequestprofiler.c
sg_ihash.c
sg_jsondb.c
sg_jsonparser.c
sg_jsonwriter.c
sg_lib.c
sg_linediff.c
sg_localsettings.c
sg_log.c
sg_log_console.c
sg_log_text.c
sg_malloc.c
sg_mergereview.c
sg_mergetool.c
sg_misc_utils.c
sg_mutex.c
sg_password.c
sg_pathname.c
sg_pendingdb.c
sg_pull.c
sg_push.c
sg_random.c
sg_rbtree.c
sg_rbtreedb.c
sg_repo.c
sg_repo__bind_vtables.c
sg_repo_utils.c
sg_repo_zip.c
sg_repopath.c
sg_repo_misc.c
sg_revision_specifier.c
sg_sync_remote.c
sg_sqlite.c
sg_staging.c
sg_str_utils.c
sg_stream.c
sg_string.c
sg_stringarray.c
sg_strpool.c
sg_svn_load.c
sg_sync.c
sg_tempfile.c
sg_textfilediff.c
sg_thread.c
sg_tid.c
sg_time.c
sg_tncache.c
sg_treenode.c
sg_treenode_entry.c
sg_unzip.c
sg_user.c
sg_usermap.c
sg_utf8.c
sg_validate.c
sg_variant.c
sg_varpool.c
sg_varray.c
sg_vc_branches.c
sg_vc_comments.c
sg_vc_locks.c
sg_vc_stamps.c
sg_vc_tags.c
sg_vcdiff.c
sg_verify.c
sg_vector.c
sg_vector_i64.c
sg_vfile.c
sg_vhash.c
sg_web_utils.c
sg_xmlwriter.c
sg_zing.c
sg_zing_init.c
sg_zing_merge.c
sg_zing_sort.c
sg_zing_template.c
sg_zing_uniqify.c
sg_zing_where.c
sg_zip.c
)

IF (SG_IOS)
    SET(C_SOURCES ${C_SOURCES}
        sg_sync_client_vtable__http__cfnetwork.c
        )
ELSE()
    SET(C_SOURCES ${C_SOURCES}
        sg_sync_client_vtable__http__curl.c
        sg_timestamp_cache.c
        sg_workingdir.c
        sg_file_spec.c
        sg_getopt.c
        sg_vv_utils.c
        sg_vv_verbs.c
        sg_libcurl.c
        sg_cbuffer.c
        sg_jsglue.c
        sg_jsmin.c
        sg_js_safeptr.c
        sg_zing_jsglue.c
        sg_jscontextpool.c
        sg_jscore.c
        sg_uridispatch.c
        sg_vc_hooks.c
       )
ENDIF()


file(GLOB PRIVATE_HEADERS ./*.h)
file(GLOB HEADERS ../include/*.h)

set(ALL_SOURCE ${C_SOURCES} ${HEADERS} ${PRIVATE_HEADERS})

source_group("Private Headers" FILES ${PRIVATE_HEADERS})
source_group("Header Files" FILES ${HEADERS})

add_library(sglib STATIC ${ALL_SOURCE})
target_link_libraries(sglib sg_vv2 sg_wc sghash sgtemplates sqlite3 ${SG_THIRDPARTY_LIBRARIES} ${SG_OS_LIBS})

set_target_properties(sglib PROPERTIES FOLDER "Libraries")
