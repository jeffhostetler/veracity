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
 * @file sg_dag_sqlite3_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_DAG_SQLITE3_PROTOTYPES_H
#define H_SG_DAG_SQLITE3_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_dag_sqlite3__find_new_dagnodes_since(
	SG_context* pCtx,
    sqlite3* psql,
	SG_uint64 iDagNum,
    SG_varray* pva_starting,
    SG_ihash** ppih
	);

void sg_dag_sqlite3__create_deferred_indexes(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum);

void sg_fs3_get_fragball_stuff(
	SG_context* pCtx,
	sqlite3* psql,
	SG_uint64 dagnum,
    const char* psz_tid
    );

void SG_dag_sqlite3__open(SG_context* pCtx, const SG_pathname* pPath, sqlite3** ppNew);

void SG_dag_sqlite3__create(SG_context* pCtx, const SG_pathname* pPath, sqlite3** ppNew);

void SG_dag_sqlite3__nullclose(SG_context* pCtx, sqlite3** ppsql);

void SG_dag_sqlite3__fetch_dagnode(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, const char* pszidHidChangeset, SG_dagnode ** ppNewDagnode);

void SG_dag_sqlite3__find_dag_path(
	SG_context* pCtx,
    sqlite3* psql,
	SG_uint64 iDagNum,
    const char* psz_id_min,
    const char* psz_id_max,
    SG_varray** ppva
	);

void SG_dag_sqlite3__fetch_dagnodes__begin(SG_context* pCtx, SG_uint64 iDagnum, SG_dag_sqlite3_fetch_dagnodes_handle** ppHandle);
void SG_dag_sqlite3__fetch_dagnodes__one(
	SG_context* pCtx, 
	sqlite3* psql, 
	SG_dag_sqlite3_fetch_dagnodes_handle* pHandle,
	const char* pszidHidChangeset, 
	SG_dagnode** ppNewDagnode);
void SG_dag_sqlite3__fetch_dagnodes__end(SG_context* pCtx, SG_dag_sqlite3_fetch_dagnodes_handle** ppHandle);

void SG_dag_sqlite3__list_dags(SG_context* pCtx, sqlite3* psql, SG_uint32* piCount, SG_uint64** paDagNums);

void SG_dag_sqlite3__fetch_leaves(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned);

void SG_dag_sqlite3__fetch_dagnode_children(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum, const char * pszDagnodeID, SG_rbtree ** ppIdsetReturned);

void SG_dag_sqlite3__fetch_chrono_dagnode_list(
	SG_context* pCtx,
	sqlite3* pSql,
	SG_uint64 iDagNum,
	SG_uint32 iStartRevNo,
	SG_uint32 iNumRevsRequested,
	SG_uint32* piNumRevsReturned,
	char*** ppaszHidsReturned);

void SG_dag_sqlite3__check_consistency(SG_context* pCtx, sqlite3* psql, SG_uint64 iDagNum);

void SG_dag_sqlite3__insert_frag(
	SG_context* pCtx,
    sqlite3* psql,
    SG_dagfrag * pFrag,
    SG_rbtree ** ppIdsetMissing,
    SG_stringarray ** ppsaAdded,
    SG_uint32 flags
    );

void SG_dag_sqlite3__check_frag(
    SG_context* pCtx,
	sqlite3* psql,
    SG_dagfrag * pFrag,
	SG_bool * pbConnected,
    SG_rbtree ** ppIdsetMissing,
    SG_stringarray ** ppsaAdded
    );

/**
 * Compute the LCA for the given set of dagnode leaves.
 *
 * You must free the returned pDagLca.
 */
void SG_dag_sqlite3__get_lca(
	SG_context* pCtx,
	sqlite3* psql,
    SG_uint64 iDagNum,
    const SG_rbtree* prbNodes,
    SG_daglca ** ppDagLca
    );

void SG_dag_sqlite3__find_by_prefix(SG_context* pCtx, sqlite3*, SG_uint64 iDagNum, const char* psz_hid_prefix, SG_rbtree** pprb);

void SG_dag_sqlite3__find_by_rev_id(SG_context* pCtx, sqlite3*, SG_uint64 iDagNum, SG_uint32 iRevisionId, char** ppsz_hid);

void SG_dag_sqlite3__count_all_dagnodes(
	SG_context* pCtx,
	sqlite3* pSql,
	SG_uint64 iDagNum,
    SG_uint32* pi
    );

void SG_dag_sqlite3__fetch_dagnode_ids(
	SG_context* pCtx,
    sqlite3* psql,
	SG_uint64 dagnum,
    SG_int32 gen_min,
    SG_int32 gen_max,
    SG_ihash** ppih
    );

void SG_dag_sqlite3__fetch_all_dagnodes(
	SG_context* pCtx,
	sqlite3* pSql,
	SG_uint64 iDagNum,
    SG_stringarray* psa
    );

END_EXTERN_C;

#endif //H_SG_DAG_SQLITE3_PROTOTYPES_H
