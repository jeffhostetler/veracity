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
 * @file sg_dbndx_prototypes.h
 *
 */

#ifndef H_SG_DBNDX_H
#define H_SG_DBNDX_H

BEGIN_EXTERN_C

void SG_dbndx_query__open(
	SG_context*,

	SG_repo* pRepo,                         /**< You still own this, but don't
	                                          free it until the dbndx is
	                                          freed.  It will retain this
	                                          pointer. */

	SG_uint64 iDagNum,                      /**< Which dag in that repo are we
	                                          indexing? */

    sqlite3* psql,
	SG_pathname* pPath, // the path for setting up the filters

	SG_dbndx_query** ppNew
	);

void SG_dbndx_update__open(
	SG_context*,

	SG_repo* pRepo,                         /**< You still own this, but don't
	                                          free it until the dbndx is
	                                          freed.  It will retain this
	                                          pointer. */

	SG_uint64 iDagNum,                      /**< Which dag in that repo are we
	                                          indexing? */

    sqlite3* psql,

	SG_dbndx_update** ppNew
	);

void SG_dbndx_query__free(SG_context* pCtx, SG_dbndx_query* pdbc);
void SG_dbndx_update__free(SG_context* pCtx, SG_dbndx_update* pdbc);

void SG_dbndx_query__query_record_history(
        SG_context* pCtx,
        SG_dbndx_query* pdbc,
        const char* psz_recid,
        const char* psz_rectype,
        SG_varray** ppva
        );

void sg_dbndx__update_cur_state(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        const char* psz_csid,
        SG_vhash** ppvh_schema
        );

void SG_dbndx_query__query_multiple_record_history(
        SG_context* pCtx,
        SG_dbndx_query* pdbc,
        const char* psz_rectype,
        SG_varray* pva_recids,
        const char* psz_field,
        SG_vhash** ppvh
        );

/**
 * Tell the dbndx to update itself with new dagnodes.
 *
 * Typical usage here is to call
 * this routine immediately after adding a changeset to the REPO.
 *
 */
void SG_dbndx_update__do(SG_context* pCtx, SG_dbndx_update* pdbc, SG_vhash* pvh_changesets);

void sg_dbndx__load_schema_from_sql(
        SG_context* pCtx, 
        sqlite3* psql,
        const char* psz_table,
        SG_vhash** ppvh_schema
        );

void SG_dbndx__create_composite_schema_for_all_templates(
	SG_context* pCtx,
    SG_uint64 dagnum,
    SG_vector* pvec_templates,
    SG_vhash** ppvh
    );

void SG_dbndx__create_new(
	SG_context* pCtx,
    SG_uint64 dagnum,
    sqlite3* psql,
    const char* psz_schema_token,
    SG_vhash* pvh_schema
	);

void SG_dbndx__get_schema_info(
	SG_context* pCtx,
    sqlite3* psql,
    char** ppsz_schema_token,
    char** ppsz_schema
	);

void SG_dbndx__update_schema_token(
	SG_context* pCtx,
    sqlite3* psql,
    const char* psz_schema_token
	);

// this function is used by the sqlite and fs2 repos to do
// ndx updates.  it needs callbacks to fetch the actual
// indexes.

typedef void (SG_get_dbndx_path_callback)(SG_context* pCtx, void * pVoidData, SG_uint64 iDagNum, SG_pathname** pp);
typedef void (SG_get_sql_callback)(SG_context* pCtx, void * pVoidData, SG_pathname* pPath, sqlite3** pp);
typedef void (SG_get_treendx_callback)(SG_context* pCtx, void * pVoidData, SG_uint64 iDagNum, SG_treendx** ppndx);

void SG_dbndx_query__query__recent(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    const char* psz_rectype,
	const SG_varray* pcrit,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

void SG_dbndx_query__query(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
	const SG_varray* pSort,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

void SG_dbndx_query__query__one(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_field_value,
    SG_varray* pva_fields,
    SG_vhash** ppvh
	);

void SG_dbndx_query__fts(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_keywords,
    const char* psz_field_name,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

void sg_repo__fs3__fetch_all_dagnodes(
	SG_context * pCtx,
	SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_stringarray** ppsa
    );

void sg_repo__fs3__count_all_dagnodes(
	SG_context * pCtx,
	SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_uint32* pi
    );

void sg_repo__update_ndx__all__tree(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_get_treendx_callback* pfn_get_treendx,
    void* p_arg_get_treendx
    );

void sg_repo__update_ndx__all__db(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_get_dbndx_path_callback* pfn_get_dbndx_path,
    void* p_arg_get_dbndx
    );

void sg_fs3__list_dags(SG_context * pCtx, SG_repo* pRepo, SG_uint32* piCount, SG_uint64** paDagNums);

void sg_repo__update_ndx(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_rbtree* prb_new_dagnodes,
    SG_get_dbndx_path_callback* pfn_get_dbndx_path,
    void* p_arg_get_dbndx,
    SG_get_sql_callback* pfn_get_sql,
    void* p_arg_get_sql,
    SG_get_treendx_callback* pfn_get_treendx,
    void* p_arg_get_treendx,
    SG_vhash** ppvh_rebuild,
    SG_vhash* pvh_fresh
    );

void sg_repo__rebuild_dbndxes(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_rbtree* prb_new_dagnodes,
    SG_get_dbndx_path_callback* pfn_get_dbndx_path,
    void* p_arg_get_dbndx,
    SG_get_sql_callback* pfn_get_sql,
    void* p_arg_get_sql
    );

void SG_dbndx_query__prep(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState
	);

void SG_dbndx_query__count(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
    SG_uint32* pi_result
	);

void sg_dbndx__create_record_tables_from_schema(
	SG_context* pCtx,
    sqlite3* psql,
    SG_vhash* pvh_schema,
    const char* pidState
    );

void SG_dbndx__remove(
	SG_context* pCtx,
	SG_uint64 iDagNum,
	SG_pathname* pPath
	);

void sg_dbndx__verify_all_filters(
        SG_context* pCtx, 
        SG_dbndx_query* pndx
        );

void sg_dbndx__get_filters_dir_path(
	SG_context* pCtx,
    SG_pathname* pPath_dbndx_file,
    SG_pathname** ppPath
    );

void sg_dbndx_query__make_delta_from_path(
    SG_context* pCtx,
    SG_dbndx_query* pndx,
    SG_varray* pva_path,
    SG_uint32 flags,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
    );

void sg_fs3_create_schema_token(
    SG_context * pCtx,
    SG_varray* pva,
    SG_string** ppstr_token
    );

void sg_dbndx_update__one_record(
    SG_context* pCtx, 
    SG_dbndx_update* pndx, 
    const char* psz_hid_rec,
    SG_vhash* pvh
    );

void SG_dbndx_update__begin(SG_context* pCtx, SG_dbndx_update* pdbc);
void SG_dbndx_update__end(SG_context* pCtx, SG_dbndx_update* pdbc);

void sg_fs3_get_list_of_templates(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        SG_varray** ppva
        );

void sg_dbndx__add_deltas(
        SG_context* pCtx,
        SG_dbndx_update* pdbc,
        SG_int64 csidrow,
        SG_vhash* pvh_changes
        );

void SG_dbndx_update__do_audits(
	SG_context* pCtx,
	SG_dbndx_update* pdbc,
    SG_varray* pva_specified_changesets
	);

void sg_dbndx__add_db_history_for_new_records(
        SG_context* pCtx,
        SG_dbndx_update* pdbc,
        SG_int64 csidrow,
        SG_vhash* pvh_changes
        );

void sg_dbndx__add_csid(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        const char* psz_csid,
        SG_int32 gen,
        SG_int64* pi_row
        );

void SG_dbndx_query__raw_history(
        SG_context* pCtx,
        SG_dbndx_query* pndx,
        SG_int64 min_timestamp,
        SG_int64 max_timestamp,
        SG_vhash** ppvh
        );

END_EXTERN_C

#endif

