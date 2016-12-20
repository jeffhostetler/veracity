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
 * @file sg_zing_prototypes.h
 *
 */

#ifndef H_SG_ZING_PROTOTYPES_H
#define H_SG_ZING_PROTOTYPES_H

BEGIN_EXTERN_C

void SG_zing__collect_fields_one_rectype_one_template(
	SG_context* pCtx,
    const char* psz_rectype,
    SG_zingtemplate* pzt,
    SG_vhash* pvh_result
    );

void SG_zingtemplate__alloc(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_vhash** ppvh,
        SG_zingtemplate** pptemplate
        );

void SG_zingtemplate__list_fields__vhash(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_vhash** ppvh
        );

void SG_zingtemplate__list_rectypes__update_into_vhash(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        SG_vhash* pvh
        );

void SG_zingtx__get_record__if_exists(
        SG_context* pCtx,
        SG_zingtx* pztx,
	const char* psz_rectype,
        const char* psz_recid,			// This is a GID.
        SG_bool* pb_exists,
        SG_zingrecord** ppzrec
        );

void SG_zing__get_record__vhash(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char* psz_rectype,
        const char* psz_recid,
        SG_vhash** ppvh
        );

void SG_zing__get_record__vhash__fields(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char* psz_rectype,
        const char* psz_recid,
	    SG_varray* psa_fields,
        SG_vhash** ppvh
        );


void SG_zing__begin_tx(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* who,
    const char* psz_hid_baseline,
	SG_zingtx** ppTx
	);

void SG_zingtx__add_parent(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_hid_cs_parent
        );

void SG_zing__commit_tx(
	SG_context* pCtx,
    SG_int64 when,
	SG_zingtx** ppTx,
    SG_changeset** ppcs,
    SG_dagnode** ppdn,
    SG_varray** ppva_constraint_violations
    );

void SG_zing__commit_tx__hidrecs(
	SG_context* pCtx,
    SG_int64 when,
	SG_zingtx** ppTx,
    SG_changeset** ppcs,
    SG_dagnode** ppdn,
    SG_varray** ppva_constraint_violations,
    SG_vhash** ppvh_hidrecs_adds,
    SG_vhash** ppvh_hidrecs_mods
    );

void SG_zing__abort_tx(
	SG_context* pCtx,
	SG_zingtx** ppTx);

void SG_zingtx__create_new_record__force_recid(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid,
        SG_zingrecord** pprec
        );

void SG_zingtx__create_new_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        SG_zingrecord** pprec
        );

void SG_zingrecord__set_field__attachment__pathname(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_pathname** ppPath
        );

void SG_zingrecord__set_field__attachment__buflen(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const SG_byte* pBytes,
        SG_uint32 len
        );

void SG_zingrecord__set_field__string(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        );

void SG_zingrecord__set_field__string_checked(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val,
		SG_varray *pva_violations
        );

void SG_zingrecord__set_field__userid(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        );

void SG_zingrecord__set_field__blobid(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        );

void SG_zingrecord__set_field__reference(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        );

void SG_zingrecord__set_field__datetime(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64 val
        );

void SG_zingrecord__set_field__int(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64 val
        );

void SG_zingrecord__set_field__bool(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_bool val
        );

void SG_zingrecord__get_field__int(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64 *val
        );

void SG_zingrecord__get_field__datetime(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64 *val
        );

void SG_zingrecord__get_field__bool(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_bool *val
        );

void SG_zingrecord__get_field__userid(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** val
        );

void SG_zingrecord__get_field__blobid(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** val
        );

void SG_zingrecord__get_field__reference(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** val
        );

void SG_zingrecord__get_field__string(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** val
        );

void SG_zingrecord__get_field__attachment(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** val
        );

void SG_zingrecord__get_history(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_varray** ppva
        );

void SG_zingrecord__get_rectype(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        const char** ppsz_rectype
        );

void SG_zingrecord__get_recid(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        const char** ppsz_recid
        );

void SG_zingtemplate__get_rectype_info(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_zingrectypeinfo** pp
        );

void SG_zingtemplate__get_field_attributes(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        const char* psz_field_name,
        SG_zingfieldattributes** ppzfa
        );

void SG_zingrecord__remove_field(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa
        );

void SG_zingtx__get_recid_from_uniquified_id_field(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_id,
        char** ppsz_recid ///< You own the result.
        );

void SG_zingtx__get_userid(
    SG_context *pCtx,
    SG_zingtx *pztx,
    const char **ppUserid);

void SG_zingtx__get_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid,
        SG_zingrecord** ppzrec
        );

void SG_zingtx__delete_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid
        );

void SG_zingtx__delete_record__hid(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_hid
        );

void SG_zingtx__set_template__existing(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_hid_template
        );

void SG_zingtx__set_template__new(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_vhash** ppvh_template // We take ownership of the vhash and null your pointer for you.
        );

void SG_zing__get_cached_template__csid(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        SG_zingtemplate** ppzt
        );

void SG_zing__get_cached_hid_for_template__csid(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char** ppsz_hid_template
        );

void SG_zingtx__get_template(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingtemplate** ppResult
        );

void SG_zing__get_cached_template__hid_template(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_hid_template,
        SG_zingtemplate** ppzt
        );

void SG_zing__get_cached_template__static_dagnum(
        SG_context* pCtx,
        SG_uint64 iDagNum,
        SG_zingtemplate** ppzt
        );

void SG_zingrecord__lookup_name(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        const char* psz_name,
        SG_zingfieldattributes** ppzfa
        );

void SG_zing__automerge(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const SG_audit* pq,
    const char* psz_csid_node_0,
    const char* psz_csid_node_1,
    SG_dagnode** ppdn_merged
    );

void sg_zing__init_new_repo(
    SG_context* pCtx,
	SG_repo* pRepo
    );

void sg_zingfieldattributes__get_type(
	SG_context *pCtx,
	SG_zingfieldattributes *pzfa,
	SG_uint16 *pType);

void SG_zing__lookup_recid(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_state,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_field_value,
        char** psz_recid
        );

void SG_zingtx__lookup_recid(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_field_value,
        char** psz_recid
        );

void SG_zing__extract_one_from_slice__string(
        SG_context* pCtx,
        SG_varray* pva,
        const char* psz_name,
        SG_varray** ppva2
        );

void SG_zing__query(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_state,
        const char* psz_rectype,
        const char* psz_where,
        const char* psz_sort,
        SG_uint32 limit,
        SG_uint32 skip,
        SG_varray* pva_fields,
        SG_varray** ppva_sliced
        );

void SG_zing__query__recent(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_rectype,
        const char* psz_where,
        SG_uint32 limit,
        SG_uint32 skip,
        SG_varray* pva_fields,
        SG_varray** ppva_sliced
        );

#if 0
void SG_zing__get_any_leaf(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        char** pp
        );
#endif

void SG_zing__get_leaf(
        SG_context* pCtx,
        SG_repo* pRepo,
        const SG_audit* pq,
        SG_uint64 iDagNum,
        char** pp
        );

void SG_zing__get_record(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_state,
        const char* psz_rectype,
        const char* psz_recid,
        SG_dbrecord** ppdbrec
        );

void SG_zingtemplate__validate(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_vhash* pvh,
        SG_uint32* pi_version
        );

void sg_zing__validate_templates(
    SG_context* pCtx
    );

void SG_zingrectypeinfo__free(SG_context* pCtx, SG_zingrectypeinfo* pztx);
void SG_zingfieldattributes__free(SG_context* pCtx, SG_zingfieldattributes* pztx);
void SG_zingtemplate__free(SG_context* pCtx, SG_zingtemplate* pztx);
void SG_zingtemplate__get_vhash(SG_context* pCtx, SG_zingtemplate* pThis, SG_vhash ** ppResult);
void SG_zingrecord__free(SG_context* pCtx, SG_zingrecord* pThis);

void SG_zingrecord__to_dbrecord(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_dbrecord** ppResult
        );

void SG_zing__init_template_caches(
        SG_context* pCtx
        );

void SG_zing__now_free_all_cached_templates(
        SG_context* pCtx
        );

void SG_zing__auto_merge_all_dags(
	SG_context* pCtx,
	SG_repo* pRepo
    );

void SG_zing__fetch_static_template__json(
        SG_context* pCtx,
        SG_uint64 iDagNum,
        SG_string** ppstr
        );

void SG_zing__create_root_node(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        char** pp
        );

void SG_zing__field_type__sz_to_uint16(
        SG_context* pCtx,
        const char* psz_field_type,
        SG_uint16* pi
        );

void SG_zing__field_type_is_integerish(
        SG_context* pCtx,
        SG_uint16 type,
        SG_bool* pb
        );

void SG_zing__field_type_is_integerish__sz(
        SG_context* pCtx,
        const char* psz_field_type,
        SG_bool* pb
        );

/**
 * *pbMissing is true if the provided DAG should be skipped for a push, pull, or clone.
 * 
 * More specifically, this returns true if:
 *  The provided dagnum has a hardwired template that the current binary doesn't have,
 *   OR
 *  the provided dagnum is the audit DAG for a hardwired template DAG that the current binary
 *  doesn't have.
 */
void SG_zing__missing_hardwired_template(SG_context* pCtx, SG_uint64 dagnum, SG_bool* pbMissing);

END_EXTERN_C

#endif

