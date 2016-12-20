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

#ifndef H_SG_ZING__PRIVATE_H
#define H_SG_ZING__PRIVATE_H

BEGIN_EXTERN_C;

#define SG_JOURNAL_MAGIC__UNIQIFY__RECID        "#RECID#"
#define SG_JOURNAL_MAGIC__UNIQIFY__OP           "#OP#"
#define SG_JOURNAL_MAGIC__UNIQIFY__FIELD_NAME   "#FIELD_NAME#"
#define SG_JOURNAL_MAGIC__UNIQIFY__OLD_VALUE          "#OLD_VALUE#"
#define SG_JOURNAL_MAGIC__UNIQIFY__NEW_VALUE          "#NEW_VALUE#"

#define SG_JOURNAL_MAGIC__MERGE__RECID        "#RECID#"
#define SG_JOURNAL_MAGIC__MERGE__OP           "#OP#"
#define SG_JOURNAL_MAGIC__MERGE__VAL0          "#VAL0#"
#define SG_JOURNAL_MAGIC__MERGE__VAL1          "#VAL1#"
#define SG_JOURNAL_MAGIC__MERGE__MERGED_VALUE          "#MERGED_VALUE#"
#define SG_JOURNAL_MAGIC__MERGE__FIELD_NAME   "#FIELD_NAME#"

#define SG_ZING_CONSTRAINT_VIOLATION__TYPE         "type"
#define SG_ZING_CONSTRAINT_VIOLATION__RECID        "recid"
#define SG_ZING_CONSTRAINT_VIOLATION__OTHER_RECID  "other_recid"
#define SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME   "field_name"
#define SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE  "field_value"
#define SG_ZING_CONSTRAINT_VIOLATION__IDS               "ids"
#define SG_ZING_CONSTRAINT_VIOLATION__RECTYPE               "rectype"

// TODO use these #defines everywhere, not just in the validation code
#define SG_ZING_TEMPLATE__VERSION "version"
#define SG_ZING_TEMPLATE__RECTYPE "rectype"
#define SG_ZING_TEMPLATE__RECTYPES "rectypes"
#define SG_ZING_TEMPLATE__FIELDS "fields"
#define SG_ZING_TEMPLATE__DATATYPE "datatype"
#define SG_ZING_TEMPLATE__JOURNAL "journal"
#define SG_ZING_TEMPLATE__REQUIRED "required"
#define SG_ZING_TEMPLATE__INDEX "index"
#define SG_ZING_TEMPLATE__FULL_TEXT_SEARCH "full_text_search"
#define SG_ZING_TEMPLATE__DEFAULTVALUE "defaultvalue"
#define SG_ZING_TEMPLATE__DEFAULTFUNC "defaultfunc"
#define SG_ZING_TEMPLATE__GENCHARS "genchars"
#define SG_ZING_TEMPLATE__MIN "min"
#define SG_ZING_TEMPLATE__MAX "max"
#define SG_ZING_TEMPLATE__MINLENGTH "minlength"
#define SG_ZING_TEMPLATE__MAXLENGTH "maxlength"
#define SG_ZING_TEMPLATE__ALLOWED "allowed"
#define SG_ZING_TEMPLATE__SORT_BY_ALLOWED "sort_by_allowed"
#define SG_ZING_TEMPLATE__PROHIBITED "prohibited"
#define SG_ZING_TEMPLATE__READONLY "readonly"
#define SG_ZING_TEMPLATE__VISIBLE "visible"
#define SG_ZING_TEMPLATE__CONSTRAINTS "constraints"
#define SG_ZING_TEMPLATE__FORM "form"
#define SG_ZING_TEMPLATE__NAME "name"
#define SG_ZING_TEMPLATE__FROM "from"
#define SG_ZING_TEMPLATE__TO "to"
#define SG_ZING_TEMPLATE__TYPE "type"
#define SG_ZING_TEMPLATE__FUNCTION "function"
#define SG_ZING_TEMPLATE__FIELD_FROM "field_from"
#define SG_ZING_TEMPLATE__MERGE_TYPE "merge_type"
#define SG_ZING_TEMPLATE__MERGE "merge"
#define SG_ZING_TEMPLATE__UNIQIFY "uniqify"
#define SG_ZING_TEMPLATE__WHICH "which"
#define SG_ZING_TEMPLATE__AUTO "auto"
#define SG_ZING_TEMPLATE__OP "op"
#define SG_ZING_TEMPLATE__BAD "bad"
#define SG_ZING_TEMPLATE__NUM_DIGITS "num_digits"
#define SG_ZING_TEMPLATE__UNIQUE "unique"
#define SG_ZING_TEMPLATE__LABEL "label"
#define SG_ZING_TEMPLATE__HELP "help"
#define SG_ZING_TEMPLATE__TOOLTIP "tooltip"
#define SG_ZING_TEMPLATE__READONLY "readonly"
#define SG_ZING_TEMPLATE__VISIBLE "visible"
#define SG_ZING_TEMPLATE__SUGGESTED "suggested"
#define SG_ZING_TEMPLATE__DEPENDS_ON "depends_on"

#define SG_ZING_MERGE__most_recent "most_recent"
#define SG_ZING_MERGE__least_recent "least_recent"
#define SG_ZING_MERGE__max "max"
#define SG_ZING_MERGE__min "min"
#define SG_ZING_MERGE__average "average"
#define SG_ZING_MERGE__sum "sum"
#define SG_ZING_MERGE__allowed_last "allowed_last"
#define SG_ZING_MERGE__allowed_first "allowed_first"
#define SG_ZING_MERGE__longest "longest"
#define SG_ZING_MERGE__shortest "shortest"
#define SG_ZING_MERGE__concat "concat"

struct sg_zing_merge_delta
{
    SG_rbtree* prb_add;
    SG_rbtree* prb_remove;
};

void SG_zingtx__set_baseline_delta(
	SG_context* pCtx,
    SG_zingtx* pztx,
    const char* psz_csid_parent,
    struct sg_zing_merge_delta* pd
    );

struct SG_zingtx
{
    SG_repo* pRepo;
    SG_uint64 iDagNum;
    SG_zingtemplate* ptemplate;
    SG_bool b_template_dirty;
    SG_bool b_template_is_mine;
    char* psz_csid;
    char* psz_hid_template;

    SG_rbtree* prb_records;
    SG_rbtree* prb_parents;
    SG_rbtree* prb_deltas;

    SG_rbtree* prb_rechecks;

    SG_rbtree*  prb_delete_hidrec;

    SG_bool b_in_commit;

    char buf_who[SG_GID_BUFFER_LENGTH];

    const char* psz_desired_baseline;
    SG_rbtree* prb_desired_baseline__add;
    SG_rbtree* prb_desired_baseline__remove;

    SG_vhash* pvh_user;
};

struct SG_zingfieldattributes
{
    const char* name;
    const char* rectype;
    SG_uint16 type;
    char buf_type[32]; // TODO maybe instead of storing the type here, we should get it on the fly?
    SG_varray* pva_automerge;

    // constraints which apply to all field types
    SG_bool required;
    SG_bool index;

    union
    {
        struct
        {
            SG_bool has_defaultvalue;
            SG_bool defaultvalue;
        } _bool;

        struct
        {
            SG_bool has_defaultvalue;
            SG_bool has_min;
            SG_bool has_max;
            SG_int64 defaultvalue;
            SG_int64 val_min;
            SG_int64 val_max;
            SG_vector_i64* prohibited;
            SG_vector_i64* allowed;
        } _int;

        struct
        {
            SG_bool has_min;
            SG_bool has_max;
            SG_bool has_defaultvalue;
            const char* defaultvalue;
            SG_int64 val_min;
            SG_int64 val_max;
        } _datetime;

        struct
        {
            SG_bool has_defaultvalue;
            SG_bool has_minlength;
            SG_bool has_maxlength;
            const char* defaultvalue;
            SG_int64 val_minlength;
            SG_int64 val_maxlength;
            SG_stringarray* prohibited;
            SG_vhash* allowed;
            SG_bool sort_by_allowed;
            SG_bool unique;
            SG_bool has_defaultfunc;
            const char* defaultfunc;
            const char* genchars;
            SG_bool full_text_search;
            SG_int64 num_digits;
            SG_vhash* bad;
            SG_vhash* uniqify;
        } _string;

        struct
        {
            SG_bool has_defaultvalue;
            const char* defaultvalue;
        } _userid;

        struct
        {
            const char* rectype;
        } _reference;

        struct
        {
            SG_bool has_maxlength;
            SG_int64 val_maxlength;
        } _attachment;
    } v;
};

struct SG_zingrectypeinfo
{
    SG_bool b_merge_type_is_record;
    SG_vhash* pvh_merge;
};

void SG_zingtemplate__alloc(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_vhash** ppvh,
        SG_zingtemplate** pptemplate
        );

void SG_zingtemplate__list_fields(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_stringarray** pp
        );

void SG_zingtemplate__get_field_attributes(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        const char* psz_field_name,
        SG_zingfieldattributes** ppzfa
        );

void SG_zingtemplate__get_json(
        SG_context* pCtx,
        SG_zingtemplate* pThis,
        SG_string** ppstr
        );

void SG_zingrecord__get_tx(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_zingtx** pptx
        );

void sg_zingtx__add__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_dbrecord* pdbrec
        );

void sg_zingtx__add__new_from_vhash(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        SG_vhash* pvh
        );

void sg_zingtx__delete__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_dbrecord* pdbrec
        );

void sg_zingtx__mod__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_original_hid,
        SG_dbrecord* pdbrec_add
        );

void sg_zing__query__parse_where(
        SG_context* pCtx,
        const char* psz_rectype,
        const char* psz_where,
        SG_varray** pp
        );

void sg_zing__query__parse_sort(
        SG_context* pCtx,
        const char* psz_rectype,
        const char* psz_sort,
        SG_varray** pp
        );

void SG_zingtemplate__is_a_rectype(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_bool* pb,
        SG_uint32* pi_count_rectypes
        );

void SG_zingtemplate__list_rectypes(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        SG_rbtree** pprb
        );

void sg_zing_uniqify_string__do(
    SG_context* pCtx,
    SG_vhash* pvh_error,
    SG_vhash* pvh_uniqify,
    SG_zingtx* pztx,
    SG_zingfieldattributes* pzfa,
    const SG_audit* pq,
    const char** leaves,
    const char* psz_hid_cs_ancestor,
    SG_vhash* pvh_pending_journal_entries
    );

void sg_zingtx__query(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_where,
        const char* psz_sort,
        SG_int32 limit,
        SG_int32 skip,
        SG_stringarray** ppResult
        );

void sg_zing__gen_random_unique_string(
    SG_context* pCtx,
    SG_zingtx* pztx,
    SG_zingfieldattributes* pzfa,
    SG_string** ppstr
    );

void sg_zing__gen_userprefix_unique_string(
    SG_context* pCtx,
    SG_zingtx* pztx,
    const char* psz_rectype,
    const char* psz_field_name,
    SG_uint32 num_digits,
    const char* psz_base,
    SG_string** ppstr
    );

void SG_zingtx__run_defaultfunc__string(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingfieldattributes* pzfa,
        SG_string** ppstr
        );

void SG_zingtx__generating__would_be_unique(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_val,
        SG_bool* pb
        );

void SG_zingtx__get_repo(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_repo** ppRepo
        );

void SG_zingtx__get_dagnum(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_uint64* pdagnum
        );

void sg_zingtemplate__get_only_rectype(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char** ppsz_name,
        SG_vhash** ppvh_rectype
        );

void sg_zing__get_all_templates_for_dagnum(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        SG_vector** ppvec
        );

END_EXTERN_C;

#endif//H_SG_ZING__PRIVATE_H

