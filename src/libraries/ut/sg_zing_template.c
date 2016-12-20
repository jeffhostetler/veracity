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

#include "sg_zing__private.h"

// TODO temporary hack:
#ifdef WINDOWS
//#pragma warning(disable:4100)
#pragma warning(disable:4706)
#endif

struct SG_zingtemplate
{
    SG_uint32 version;
    SG_vhash* pvh;
    SG_rbtree* prb_field_attributes;
    SG_mutex lock;
};

void SG_zing__is_reserved_field_name(
        SG_context* pCtx,
        const char* psz_name,
        SG_bool* pb
        )
{
  SG_UNUSED(pCtx);

    if (
               (0 == strcmp(psz_name, SG_ZING_FIELD__RECID))
            || (0 == strcmp(psz_name, SG_ZING_FIELD__HIDREC))
            || (0 == strcmp(psz_name, SG_ZING_FIELD__RECTYPE))
            || (0 == strcmp(psz_name, SG_ZING_FIELD__HISTORY))
            || (0 == strcmp(psz_name, SG_ZING_FIELD__LAST_TIMESTAMP))
       )
    {
        *pb = SG_TRUE;
    }
    else
    {
        *pb = SG_FALSE;
    }
}

void SG_zingfieldattributes__free(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa
        )
{
    if (pzfa)
    {
        switch (pzfa->type)
        {
            case SG_ZING_TYPE__INT:
                {
                    SG_VECTOR_I64_NULLFREE(pCtx, pzfa->v._int.allowed);
                    SG_VECTOR_I64_NULLFREE(pCtx, pzfa->v._int.prohibited);
                    break;
                }
            case SG_ZING_TYPE__STRING:
                {
                    SG_VHASH_NULLFREE(pCtx, pzfa->v._string.bad);
                    SG_VHASH_NULLFREE(pCtx, pzfa->v._string.allowed);
                    SG_STRINGARRAY_NULLFREE(pCtx, pzfa->v._string.prohibited);
                    break;
                }
        }
        SG_NULLFREE(pCtx, pzfa);
    }
}

void SG_zingtemplate__free(
        SG_context* pCtx,
        SG_zingtemplate* pThis
        )
{
    if (pThis)
    {
        SG_mutex__destroy(&pThis->lock);
        SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pThis->prb_field_attributes, (SG_free_callback*) SG_zingfieldattributes__free);
        SG_VHASH_NULLFREE(pCtx, pThis->pvh);
        SG_NULLFREE(pCtx, pThis);
    }
}

static void sg_zingtemplate__allowed_members(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        ...
        )
{
    va_list ap;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    // So this n^2-ish loop doesn't look like the fastest way
    // to do this.  But the lists should always be short,
    // right?  The alternative is to build a vhash or
    // rbtree, but then we're involving the memory
    // allocator.  For short lists, the overhead
    // probably isn't worth it.

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
    for (i=0; i<count; i++)
    {
        const SG_variant* pv = NULL;
        const char* psz_candidate = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &psz_candidate, &pv)  );
        va_start(ap, pvh);
        do
        {
            const char* psz_key = va_arg(ap, const char*);

            if (!psz_key)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  '%s' is not allowed here", psz_where, psz_cur, psz_candidate)
                        );
            }

            if (*psz_key) // skip empty string
            {
                if (0 == strcmp(psz_candidate, psz_key))
                {
                    break;
                }
            }

        } while (1);
    }

    return;

fail:
    return;
}

static void sg_zingtemplate__required(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_uint16 type
        )
{
    SG_bool b = SG_FALSE;
    SG_uint16 i2;

    b = SG_FALSE;
    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh, psz_key, &b)  );
    if (!b)
    {
		SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  Required element '%s' is missing", psz_where, psz_cur, psz_key)
                );
    }
    SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh, psz_key, &i2)  );
    if (type != i2)
    {
		SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  Element '%s' must be of type %s", psz_where, psz_cur, psz_key, sg_variant__type_name(type))
                );
    }

    // fall thru
fail:
    return;
}

static void sg_zingtemplate__optional(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_uint16 type,
        SG_bool* pb
        )
{
    SG_bool b = SG_FALSE;
    SG_uint16 i2;

    b = SG_FALSE;
    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh, psz_key, &b)  );
    if (!b)
    {
        *pb = SG_FALSE;
        return;
    }
    SG_ERR_CHECK_RETURN(  SG_vhash__typeof(pCtx, pvh, psz_key, &i2)  );
    if (!(type & i2))
    {
		SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  Element '%s' must be of type %s", psz_where, psz_cur, psz_key, sg_variant__type_name(type))
                );
    }
    *pb = SG_TRUE;

    // fall thru
fail:
    return;
}

static void sg_zingtemplate__required__int64(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_int64* pResult
        )
{
    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__required(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_INT64)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, psz_key, pResult)  );
}

void sg_zingtemplate__required__bool(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_bool* pResult
        )
{
    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__required(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_BOOL)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__bool(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__required__sz(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        const char** pResult
        )
{
    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__required(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_SZ)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__required__vhash(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_vhash** pResult
        )
{
    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__required(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_VHASH)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__optional__vhash(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_vhash** pResult
        )
{
    SG_bool b = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__optional(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_VHASH, &b)  );
    if (!b)
    {
        *pResult = NULL;
        return;
    }
    SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__optional__varray(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_varray** pResult
        )
{
    SG_bool b = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__optional(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_VARRAY, &b)  );
    if (!b)
    {
        *pResult = NULL;
        return;
    }
    SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__optional__bool(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_bool* pb_field_present,
        SG_bool* pResult
        )
{
    SG_bool b = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__optional(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_BOOL, &b)  );
    if (!b)
    {
        *pb_field_present = SG_FALSE;
        return;
    }
    *pb_field_present = SG_TRUE;
    SG_ERR_CHECK_RETURN(  SG_vhash__get__bool(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__optional__int64(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_bool* pb_field_present,
        SG_int64* pResult
        )
{
    SG_bool b = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__optional(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_INT64, &b)  );
    if (!b)
    {
        *pb_field_present = SG_FALSE;
        return;
    }
    *pb_field_present = SG_TRUE;
    SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__optional__int64_or_double(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_bool* pb_field_present,
        SG_int64* pResult
        )
{
    SG_bool b = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__optional(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_INT64 | SG_VARIANT_TYPE_DOUBLE, &b)  );
    if (!b)
    {
        *pb_field_present = SG_FALSE;
        return;
    }
    *pb_field_present = SG_TRUE;
    SG_ERR_CHECK_RETURN(  SG_vhash__get__int64_or_double(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__optional__sz(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        const char** pResult
        )
{
    SG_bool b = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__optional(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_SZ, &b)  );
    if (!b)
    {
        *pResult = NULL;
        return;
    }
    SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh, psz_key, pResult)  );
}

void sg_zingtemplate__required__varray(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        const char* psz_key,
        SG_varray** pResult
        )
{
    SG_NULLARGCHECK_RETURN(pResult);
    SG_ERR_CHECK_RETURN(  sg_zingtemplate__required(pCtx, psz_where, psz_cur, pvh, psz_key, SG_VARIANT_TYPE_VARRAY)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvh, psz_key, pResult)  );
}

static void sg_zingtemplate__vhash__check_type_of_members(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh,
        SG_uint16 type,
        SG_uint32* p_count
        )
{
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
    for (i=0; i<count; i++)
    {
        const SG_variant* pv = NULL;
        const char* psz_key = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &psz_key, &pv)  );
        if (type != pv->type)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  Element '%s' must be of type %s", psz_where, psz_cur, psz_key, sg_variant__type_name(type))
                    );
        }
    }

    *p_count = count;

    return;

fail:
    return;
}

static void sg_zingtemplate__varray__check_type_of_members(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_varray* pva,
        SG_uint16 type,
        SG_uint32* p_count
        )
{
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_varray__get__variant(pCtx, pva, i, &pv)  );
        if (type != pv->type)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  Elements must be of type %s", psz_where, psz_cur, sg_variant__type_name(type))
                    );
        }
    }

    *p_count = count;

    return;

fail:
    return;
}

static void SG_zingtemplate__validate_field_form(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        const char* psz_datatype,
        SG_vhash* pvh_constraints,
        SG_vhash* pvh
        )
{
    SG_string* pstr_where = NULL;
    const char* psz_label = NULL;
    const char* psz_help = NULL;
    const char* psz_tooltip = NULL;
    SG_bool b_has_readonly = SG_FALSE;
    SG_bool b_readonly = SG_FALSE;
    SG_bool b_has_visible = SG_FALSE;
    SG_bool b_visible = SG_FALSE;

    SG_UNUSED(psz_datatype);
    SG_UNUSED(pvh_constraints);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__LABEL, &psz_label)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__HELP, &psz_help)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__TOOLTIP, &psz_tooltip)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__READONLY, &b_has_readonly, &b_readonly)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__VISIBLE, &b_has_visible, &b_visible)  );

    // TODO suggested.  for ints and strings.

    // TODO lots of other things can go here.  width of the text field.  height of the text field.  etc.

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void sg_zingtemplate__validate__journal(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_top,
        SG_vhash* pvh_journal
        )
{
    const char* psz_rectype = NULL;
    SG_vhash* pvh_fields = NULL;
    SG_uint32 count_fields = 0;
    SG_uint32 i_field = 0;
    SG_vhash* pvh_rectypes = NULL;
    SG_bool b_is_a_rectype = SG_FALSE;
    SG_vhash* pvh_journal_rectype = NULL;
    SG_vhash* pvh_journal_rectype_fields = NULL;

    SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh_journal, SG_ZING_TEMPLATE__RECTYPE, &psz_rectype)  );

    // validate that the rectype exists.

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_top, SG_ZING_TEMPLATE__RECTYPES, &pvh_rectypes)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectypes, psz_rectype, &b_is_a_rectype)  );
    if (!b_is_a_rectype)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
            (pCtx, "%s.%s:  %s is not a rectype", psz_where, psz_cur, psz_rectype)
            );
    }

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectypes, psz_rectype, &pvh_journal_rectype)  );
    // TODO we could/should check here to make sure that the journal
    // rectype doesn't have any weird constraints

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_journal_rectype, SG_ZING_TEMPLATE__FIELDS, &pvh_journal_rectype_fields)  );

    SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, psz_where, psz_cur, pvh_journal, SG_ZING_TEMPLATE__FIELDS, &pvh_fields)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count_fields)  );
    for (i_field=0; i_field<count_fields; i_field++)
    {
        const char* psz_field_name = NULL;
        const char* psz_field_value = NULL;
        SG_bool b_is_a_field = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_fields, i_field, &psz_field_name, &psz_field_value)  );

        // make sure psz_field_name exists in the rectype
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_journal_rectype_fields, psz_field_name, &b_is_a_field)  );
        if (!b_is_a_field)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  %s is not a field in %s", psz_where, psz_cur, psz_field_name, psz_rectype)
                );
        }

        // TODO make sure any #MAGIC# inside psz_field_value is valid
    }

    // fall thru

fail:
    ;
}

static void sg_zingtemplate__validate_fallback_merge(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_top,
        SG_vhash* pvh
        )
{
    SG_varray* pva_auto = NULL;
    SG_string* pstr_where = NULL;
    const char* psz_merge_type = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh,
                SG_ZING_TEMPLATE__MERGE_TYPE,
                SG_ZING_TEMPLATE__AUTO,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__MERGE_TYPE, &psz_merge_type)  );

    if (0 == strcmp("record", psz_merge_type))
    {
        // TODO there are basically three choices here:
        // 1.  auto-choose one record based on who did it
        // 2.  auto-choose one record based on when it was done
        // 3.  call a JS hook to resolve the merge
    }
    else if (0 == strcmp("field", psz_merge_type))
    {
        SG_ERR_CHECK(  sg_zingtemplate__required__varray(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__AUTO, &pva_auto)  );

        if (pva_auto)
        {
            SG_uint32 count_auto = 0;
            SG_uint32 i = 0;

            SG_ERR_CHECK(  sg_zingtemplate__varray__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__AUTO, pva_auto, SG_VARIANT_TYPE_VHASH, &count_auto)  );

            if (0 == count_auto)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  auto is empty", psz_where, psz_cur)
                        );
            }

            for (i=0; i<count_auto; i++)
            {
                SG_vhash* pvh_automerge = NULL;
                SG_vhash* pvh_journal = NULL;
                const char* psz_op = NULL;
                SG_bool b_ok = SG_FALSE;

                SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_auto, i, &pvh_automerge)  );

                SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__AUTO, pvh_automerge, SG_ZING_TEMPLATE__OP, &psz_op)  );
                SG_ERR_CHECK(  sg_zingtemplate__optional__vhash(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__AUTO, pvh_automerge, SG_ZING_TEMPLATE__JOURNAL, &pvh_journal)  );
                if (pvh_journal)
                {
                    SG_ERR_CHECK(  sg_zingtemplate__validate__journal(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__JOURNAL, pvh_top, pvh_journal)  );
                }

                // TODO put other ops here.  such as ops which choose a side
                // based on the authority of the users.

                if (
                        (0 == strcmp(SG_ZING_MERGE__most_recent, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__least_recent, psz_op))
                   )
                {
                    b_ok = SG_TRUE;
                }

                if (!b_ok)
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s:  invalid automerge op: %s", psz_where, psz_cur, psz_op)
                            );
                }
            }
        }
    }
    else
    {
        // TODO error
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void SG_zingtemplate__validate_field_merge(
        SG_context* pCtx,
        SG_uint64 dagnum,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_top,
        const char* psz_datatype,
        SG_vhash* pvh_constraints,
        SG_vhash* pvh
        )
{
    SG_varray* pva_auto = NULL;
    SG_string* pstr_where = NULL;
    SG_bool b_has_allowed = SG_FALSE;
    SG_bool b_has_unique = SG_FALSE;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    if (pvh_constraints)
    {
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &b_has_allowed)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__UNIQUE, &b_has_unique)  );
    }

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh,
                SG_ZING_TEMPLATE__AUTO,
                b_has_unique ? SG_ZING_TEMPLATE__UNIQIFY : "",
                NULL)  );

    // AUTO is optional for each field.  automerge is required at the record
    // level as a fallback.
    SG_ERR_CHECK(  sg_zingtemplate__optional__varray(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__AUTO, &pva_auto)  );

    if (pva_auto)
    {
        SG_uint32 count_auto = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  sg_zingtemplate__varray__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__AUTO, pva_auto, SG_VARIANT_TYPE_VHASH, &count_auto)  );

        if (0 == count_auto)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  auto is empty", psz_where, psz_cur)
                    );
        }

        for (i=0; i<count_auto; i++)
        {
            SG_vhash* pvh_automerge = NULL;
            SG_vhash* pvh_journal = NULL;
            const char* psz_op = NULL;
            SG_bool b_ok = SG_FALSE;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_auto, i, &pvh_automerge)  );

            SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__AUTO, pvh_automerge, SG_ZING_TEMPLATE__OP, &psz_op)  );
            SG_ERR_CHECK(  sg_zingtemplate__optional__vhash(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__AUTO, pvh_automerge, SG_ZING_TEMPLATE__JOURNAL, &pvh_journal)  );
            if (pvh_journal)
            {
                SG_ERR_CHECK(  sg_zingtemplate__validate__journal(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__JOURNAL, pvh_top, pvh_journal)  );
            }

            // TODO put other ops here.  such as ops which choose a side
            // based on the authority of the users.

            if (
                    (0 == strcmp(SG_ZING_MERGE__most_recent, psz_op))
                    || (0 == strcmp(SG_ZING_MERGE__least_recent, psz_op))
               )
            {
                b_ok = SG_TRUE;
            }

            if (!b_ok && (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__INT)))
            {
                if (
                        (0 == strcmp(SG_ZING_MERGE__max, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__min, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__average, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__sum, psz_op))
                   )
                {
                    b_ok = SG_TRUE;
                }
            }

            if (!b_ok && (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__STRING)))
            {
                if (
                        (0 == strcmp(SG_ZING_MERGE__longest, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__shortest, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__concat, psz_op))
                   )
                {
                    b_ok = SG_TRUE;
                }
            }

            if (
                    !b_ok
                    && b_has_allowed
                    && (
                        (0 == strcmp(SG_ZING_MERGE__allowed_last, psz_op))
                        || (0 == strcmp(SG_ZING_MERGE__allowed_first, psz_op))
                    )
               )
            {
                b_ok = SG_TRUE;
            }

            if (!b_ok)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  invalid automerge op: %s", psz_where, psz_cur, psz_op)
                        );
            }
        }
    }

    // now look at uniqify for strings
    if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__STRING))
    {
        SG_vhash* pvh_uniqify = NULL;

        if (b_has_unique)
        {
            SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__UNIQIFY, &pvh_uniqify)  );
            if (pvh_uniqify)
            {
                const char* psz_op = NULL;
                const char* psz_which = NULL;
                SG_vhash* pvh_journal = NULL;
                SG_bool b_userprefix = SG_FALSE;
                SG_bool b_random = SG_FALSE;

                SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh_uniqify, SG_ZING_TEMPLATE__OP, &psz_op)  );

                b_userprefix = (0 == strcmp(psz_op, "append_userprefix_unique"));
                b_random = (0 == strcmp(psz_op, "append_random_unique"));

                SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_uniqify,
                            SG_ZING_TEMPLATE__OP,
                            SG_ZING_TEMPLATE__WHICH,
                            SG_ZING_TEMPLATE__JOURNAL,
                            (b_userprefix || b_random) ? SG_ZING_TEMPLATE__NUM_DIGITS : "",
                            (b_random) ? SG_ZING_TEMPLATE__GENCHARS : "",
                            NULL)  );

                SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh_uniqify, SG_ZING_TEMPLATE__WHICH, &psz_which)  );

                if (0 == strcmp(psz_op, "redo_defaultfunc"))
                {
                    // TODO make sure there was a defaultfunc
                }
                else if (
                        (SG_DAGNUM__USERS != dagnum)
                        && (0 == strcmp(psz_op, "append_userprefix_unique"))
                        )
                {
                    SG_int64 i_num_digits = -1;
                    SG_bool b_field_present = SG_FALSE;

                    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__NUM_DIGITS, &b_field_present, &i_num_digits)  );
                    if (b_field_present)
                    {
                        // TODO validate num_digits
                    }
                }
                else if (0 == strcmp(psz_op, "append_random_unique"))
                {
                    SG_int64 i_num_digits = -1;
                    SG_bool b_field_present = SG_FALSE;
                    const char* psz_genchars = NULL;

                    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__NUM_DIGITS, &b_field_present, &i_num_digits)  );
                    if (b_field_present)
                    {
                        // TODO validate num_digits
                    }
                    SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh_uniqify, SG_ZING_TEMPLATE__GENCHARS, &psz_genchars)  );
                    // TODO validate genchars?
                }
                else
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s:  invalid uniqify op: %s", psz_where, psz_cur, psz_op)
                            );
                }

                SG_ERR_CHECK(  sg_zingtemplate__optional__vhash(pCtx, psz_where, psz_cur, pvh_uniqify, SG_ZING_TEMPLATE__JOURNAL, &pvh_journal)  );
                if (pvh_journal)
                {
                    SG_ERR_CHECK(  sg_zingtemplate__validate__journal(pCtx, psz_where, SG_ZING_TEMPLATE__JOURNAL, pvh_top, pvh_journal)  );
                }
            }
        }
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void SG_zingtemplate__validate__constraints__bool(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_constraints
        )
{
    SG_bool b_field_present = SG_FALSE;
    SG_bool b_required = SG_FALSE;

    SG_bool b_defaultvalue;

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__DEFAULTVALUE,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_field_present, &b_required)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b_field_present, &b_defaultvalue)  );

    // fall thru

fail:
    return;
}

static void SG_zingtemplate__validate__constraints__attachment(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_constraints
        )
{
    SG_bool b_field_present = SG_FALSE;
    SG_bool b_required = SG_FALSE;
    SG_bool b_index = SG_FALSE;
    SG_int64 i_maxlength = 0;

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__MAXLENGTH,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__INDEX,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_field_present, &b_required)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b_field_present, &b_index)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MAXLENGTH, &b_field_present, &i_maxlength)  );

    // fall thru

fail:
    return;
}

static void SG_zingtemplate__validate__constraints__reference(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_top,
        SG_vhash* pvh_constraints
        )
{
    SG_bool b_has_required = SG_FALSE;
    SG_bool b_required = SG_FALSE;
    SG_bool b_has_index = SG_FALSE;
    SG_bool b_index = SG_FALSE;
    SG_vhash* pvh_rectypes = NULL;
    SG_string* pstr_where = NULL;
    const char* psz_rectype = NULL;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_top, SG_ZING_TEMPLATE__RECTYPES, &pvh_rectypes)  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__INDEX,
                SG_ZING_TEMPLATE__RECTYPE,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_has_required, &b_required)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b_has_index, &b_index)  );

    SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__RECTYPE, &psz_rectype)  );
    {
        SG_bool b = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectypes, psz_rectype, &b)  );
        if (!b)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  %s is not a rectype", SG_string__sz(pstr_where), psz_cur, psz_rectype)
                );
        }
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void SG_zingtemplate__validate__constraints__userid(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_constraints
        )
{
    SG_bool b_has_required = SG_FALSE;
    SG_bool b_required = SG_FALSE;
    SG_bool b_has_index = SG_FALSE;
    SG_bool b_index = SG_FALSE;

    const char* psz_defaultvalue = NULL;

    SG_string* pstr_where = NULL;

    SG_rbtree* prb = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    // TODO allow constraints for group membership and role

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__INDEX,
                SG_ZING_TEMPLATE__DEFAULTVALUE,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_has_required, &b_required)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b_has_index, &b_index)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &psz_defaultvalue)  );

    if (psz_defaultvalue)
    {
        // TODO this should be a defaultfunc
        if (0 == strcmp(psz_defaultvalue, "whoami"))
        {
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  invalid defaultvalue for userid field: %s", psz_where, psz_cur, psz_defaultvalue)
                    );
        }
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
    SG_RBTREE_NULLFREE(pCtx, prb);
}

static void SG_zingtemplate__validate__constraints__string(
        SG_context* pCtx,
        SG_uint64 dagnum,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_constraints
        )
{
    SG_uint32 i = 0;

    SG_bool b_has_full_text_search = SG_FALSE;
    SG_bool b_full_text_search = SG_FALSE;
    SG_bool b_has_required = SG_FALSE;
    SG_bool b_required = SG_FALSE;
    SG_bool b_has_index = SG_FALSE;
    SG_bool b_index = SG_FALSE;
    SG_bool b_has_unique = SG_FALSE;
    SG_bool b_unique = SG_FALSE;
    SG_bool b_has_sort_by_allowed = SG_FALSE;
    SG_bool b_sort_by_allowed = SG_FALSE;

    const char* psz_defaultvalue = NULL;
    const char* psz_defaultfunc = NULL;
    const char* psz_genchars = NULL;

    SG_bool b_has_minlength = SG_FALSE;
    SG_int64 i_minlength = 0;

    SG_bool b_has_num_digits = SG_FALSE;
    SG_int64 i_num_digits = 0;

    SG_bool b_has_maxlength = SG_FALSE;
    SG_int64 i_maxlength = 0;

    SG_varray* pva_bad = NULL;

    SG_varray* pva_allowed = NULL;
    SG_uint32 count_allowed = 0;
    SG_varray* pva_prohibited = NULL;
    SG_uint32 count_prohibited = 0;
    SG_string* pstr_where = NULL;

    SG_rbtree* prb = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__varray(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &pva_allowed)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__varray(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__BAD, &pva_bad)  );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__INDEX,
                SG_ZING_TEMPLATE__FULL_TEXT_SEARCH,
                SG_ZING_TEMPLATE__MINLENGTH,
                SG_ZING_TEMPLATE__NUM_DIGITS,
                SG_ZING_TEMPLATE__MAXLENGTH,
                SG_ZING_TEMPLATE__GENCHARS,
                SG_ZING_TEMPLATE__DEFAULTFUNC,
                SG_ZING_TEMPLATE__DEFAULTVALUE,
                SG_ZING_TEMPLATE__BAD,
                SG_ZING_TEMPLATE__ALLOWED,
                pva_allowed ? SG_ZING_TEMPLATE__SORT_BY_ALLOWED : "",
                SG_ZING_TEMPLATE__PROHIBITED,
                SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(dagnum) ? SG_ZING_TEMPLATE__UNIQUE : "",
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__SORT_BY_ALLOWED, &b_has_sort_by_allowed, &b_sort_by_allowed)  );
    if (SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(dagnum))
    {
        SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__UNIQUE, &b_has_unique, &b_unique)  );
    }
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_has_required, &b_required)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b_has_index, &b_index)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__FULL_TEXT_SEARCH, &b_has_full_text_search, &b_full_text_search)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &psz_defaultvalue)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTFUNC, &psz_defaultfunc)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__GENCHARS, &psz_genchars)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__NUM_DIGITS, &b_has_num_digits, &i_num_digits)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MINLENGTH, &b_has_minlength, &i_minlength)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MAXLENGTH, &b_has_maxlength, &i_maxlength)  );

    if (b_unique && pva_allowed)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  can't have both unique and allowed", psz_where, psz_cur)
                );
    }

    if (b_unique && b_has_index && !b_index)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  can't have unique with index=false", psz_where, psz_cur)
                );
    }

    if (psz_defaultvalue && psz_defaultfunc)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  can't have both defaultvalue and defaultfunc", psz_where, psz_cur)
                );
    }

    if (psz_defaultfunc)
    {
        if (0 == strcmp(psz_defaultfunc, "gen_random_unique"))
        {
            if (!psz_genchars)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  gen_random_unique requires genchars", psz_where, psz_cur)
                        );
            }
            if (!b_has_minlength)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  gen_random_unique requires minlength", psz_where, psz_cur)
                        );
            }
            if (!b_has_maxlength)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  gen_random_unique requires maxlength", psz_where, psz_cur)
                        );
            }
            if (b_has_num_digits)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  gen_userprefix_unique cannot use num_digits", psz_where, psz_cur)
                        );
            }
            // TODO legalchars
        }
        else if (0 == strcmp(psz_defaultfunc, "gen_userprefix_unique"))
        {
            if (b_has_minlength)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  gen_userprefix_unique cannot use minlength", psz_where, psz_cur)
                        );
            }
            if (b_has_maxlength)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  gen_userprefix_unique cannot use maxlength", psz_where, psz_cur)
                        );
            }
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  illegal defaultfunc", psz_where, psz_cur)
                    );
        }
    }
    else
    {
        if (psz_genchars)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  genchars requires gen_random_unique", psz_where, psz_cur)
                    );
        }
    }

    if (b_has_maxlength && b_has_minlength && (i_maxlength < i_minlength))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  maxlength < minlength", psz_where, psz_cur)
                );
    }

    if (b_has_maxlength && psz_defaultvalue && ((SG_int64)strlen(psz_defaultvalue) > i_maxlength))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  defaultvalue longer than maxlength", psz_where, psz_cur)
                );
    }

    if (b_has_minlength && psz_defaultvalue && ((SG_int64)strlen(psz_defaultvalue) < i_minlength))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  defaultvalue shorter than minlength", psz_where, psz_cur)
                );
    }

    if (pva_allowed)
    {
        SG_ERR_CHECK(  sg_zingtemplate__varray__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__ALLOWED, pva_allowed, SG_VARIANT_TYPE_SZ, &count_allowed)  );

        if (0 == count_allowed)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  allowed values is empty", psz_where, psz_cur)
                    );
        }

        if (psz_defaultvalue)
        {
            SG_bool b_defaultvalue_is_allowed = SG_FALSE;
            for (i=0; i<count_allowed; i++)
            {
                const char* psz_va = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_allowed, i, &psz_va)  );
                if (0 == strcmp(psz_defaultvalue, psz_va))
                {
                    b_defaultvalue_is_allowed = SG_TRUE;
                    break;
                }
            }
            if (!b_defaultvalue_is_allowed)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  defaultvalue which is not in list of values allowed", psz_where, psz_cur)
                        );
            }
        }

        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

        for (i=0; i<count_allowed; i++)
        {
            const char* psz_va = NULL;
            SG_bool b = SG_FALSE;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_allowed, i, &psz_va)  );
            if (b_has_minlength)
            {
                if ((SG_int64)strlen(psz_va) < i_minlength)
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s: allowed value is shorter than minlength", psz_where, psz_cur)
                            );
                }
            }
            if (b_has_maxlength)
            {
                if ((SG_int64)strlen(psz_va) > i_maxlength)
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s:  allowed value is longer than maxlength", psz_where, psz_cur)
                            );
                }
            }

            b = SG_FALSE;
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb, psz_va, &b, NULL)  );
            if (b)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  duplicate value in allowed[]", psz_where, psz_cur)
                        );
            }

            SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psz_va)  );
        }
        SG_RBTREE_NULLFREE(pCtx, prb);
    }

    SG_ERR_CHECK(  sg_zingtemplate__optional__varray(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__PROHIBITED, &pva_prohibited)  );
    if (pva_prohibited)
    {
        SG_ERR_CHECK(  sg_zingtemplate__varray__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__PROHIBITED, pva_prohibited, SG_VARIANT_TYPE_SZ, &count_prohibited)  );

        if (count_allowed && count_prohibited)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  has both prohibited and allowed", psz_where, psz_cur)
                    );
        }

        if (psz_defaultvalue)
        {
            for (i=0; i<count_prohibited; i++)
            {
                const char* psz_va = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_prohibited, i, &psz_va)  );
                if (0 == strcmp(psz_defaultvalue, psz_va))
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s:  has defaultvalue which is in list of values prohibited", psz_where, psz_cur)
                            );
                }
            }
        }

        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

        for (i=0; i<count_prohibited; i++)
        {
            const char* psz_va = NULL;
            SG_bool b = SG_FALSE;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_prohibited, i, &psz_va)  );

            b = SG_FALSE;
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb, psz_va, &b, NULL)  );
            if (b)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  duplicate value in prohibited[]", psz_where, psz_cur)
                        );
            }

            SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psz_va)  );
        }
        SG_RBTREE_NULLFREE(pCtx, prb);
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
    SG_RBTREE_NULLFREE(pCtx, prb);
}

static void SG_zingtemplate__validate__constraints__datetime(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_constraints
        )
{
    SG_bool b_has_required = SG_FALSE;
    SG_bool b_required = SG_FALSE;
    SG_bool b_has_index = SG_FALSE;
    SG_bool b_index = SG_FALSE;

    const char* psz_defaultvalue = NULL;

    SG_bool b_has_min = SG_FALSE;
    SG_int64 i_min = 0;

    SG_bool b_has_max = SG_FALSE;
    SG_int64 i_max = 0;

    SG_string* pstr_where = NULL;
    SG_rbtree* prb = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__INDEX,
                SG_ZING_TEMPLATE__MIN,
                SG_ZING_TEMPLATE__MAX,
                SG_ZING_TEMPLATE__DEFAULTVALUE,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_has_required, &b_required)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b_has_index, &b_index)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__sz(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &psz_defaultvalue)  );

    if (psz_defaultvalue)
    {
        if (0 == strcmp(psz_defaultvalue, "now"))
        {
            // TODO
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  invalid defaultvalue for datetime field: %s", psz_where, psz_cur, psz_defaultvalue)
                    );
        }
    }

    SG_ERR_CHECK(  sg_zingtemplate__optional__int64_or_double(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MIN, &b_has_min, &i_min)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__int64_or_double(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MAX, &b_has_max, &i_max)  );

    if (b_has_max && b_has_min && (i_max < i_min))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  max < min", psz_where, psz_cur)
                );
    }

    // fall thru

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void SG_zingtemplate__validate__constraints__int(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_constraints
        )
{
    SG_uint32 i = 0;

    SG_bool b_has_required = SG_FALSE;
    SG_bool b_required = SG_FALSE;
    SG_bool b_has_index = SG_FALSE;
    SG_bool b_index = SG_FALSE;

    SG_bool b_has_defaultvalue = SG_FALSE;
    SG_int64 i_defaultvalue = 0;

    SG_bool b_has_min = SG_FALSE;
    SG_int64 i_min = 0;

    SG_bool b_has_max = SG_FALSE;
    SG_int64 i_max = 0;

    SG_varray* pva_allowed = NULL;
    SG_uint32 count_allowed = 0;
    SG_varray* pva_prohibited = NULL;
    SG_uint32 count_prohibited = 0;
    SG_string* pstr_where = NULL;
    SG_rbtree* prb = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh_constraints,
                SG_ZING_TEMPLATE__REQUIRED,
                SG_ZING_TEMPLATE__INDEX,
                SG_ZING_TEMPLATE__MIN,
                SG_ZING_TEMPLATE__MAX,
                SG_ZING_TEMPLATE__DEFAULTVALUE,
                SG_ZING_TEMPLATE__ALLOWED,
                SG_ZING_TEMPLATE__PROHIBITED,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b_has_required, &b_required)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__bool(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b_has_index, &b_index)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b_has_defaultvalue, &i_defaultvalue)  );

    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MIN, &b_has_min, &i_min)  );
    SG_ERR_CHECK(  sg_zingtemplate__optional__int64(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__MAX, &b_has_max, &i_max)  );

    if (b_has_max && b_has_min && (i_max < i_min))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  max < min", psz_where, psz_cur)
                );
    }

    if (b_has_max && b_has_defaultvalue && (i_defaultvalue > i_max))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  defaultvalue > max", psz_where, psz_cur)
                );
    }

    if (b_has_min && b_has_defaultvalue && (i_defaultvalue < i_min))
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  defaultvalue < min", psz_where, psz_cur)
                );
    }

    SG_ERR_CHECK(  sg_zingtemplate__optional__varray(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &pva_allowed)  );
    if (pva_allowed)
    {
        SG_ERR_CHECK(  sg_zingtemplate__varray__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__ALLOWED, pva_allowed, SG_VARIANT_TYPE_INT64, &count_allowed)  );

        if (0 == count_allowed)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  allowed values is empty", psz_where, psz_cur)
                    );
        }

        if (b_has_defaultvalue)
        {
            SG_bool b_defaultvalue_is_allowed = SG_FALSE;
            for (i=0; i<count_allowed; i++)
            {
                SG_int64 va = 0;

                SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pva_allowed, i, &va)  );
                if (i_defaultvalue == va)
                {
                    b_defaultvalue_is_allowed = SG_TRUE;
                    break;
                }
            }
            if (!b_defaultvalue_is_allowed)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  defaultvalue which is not in list of values allowed", psz_where, psz_cur)
                        );
            }
        }

        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

        for (i=0; i<count_allowed; i++)
        {
            SG_int64 va = 0;
            SG_bool b = SG_FALSE;
            SG_int_to_string_buffer sz_i;

            SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pva_allowed, i, &va)  );
            if (b_has_min)
            {
                if (va < i_min)
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s: allowed value is less than the specified min", psz_where, psz_cur)
                            );
                }
            }
            if (b_has_max)
            {
                if (va > i_max)
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s:  allowed value is greater than the specified max", psz_where, psz_cur)
                            );
                }
            }

            b = SG_FALSE;

            SG_int64_to_sz(va, sz_i);
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb, sz_i, &b, NULL)  );
            if (b)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  duplicate value in allowed[]", psz_where, psz_cur)
                        );
            }

            SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, sz_i)  );
        }

        SG_RBTREE_NULLFREE(pCtx, prb);
    }

    SG_ERR_CHECK(  sg_zingtemplate__optional__varray(pCtx, psz_where, psz_cur, pvh_constraints, SG_ZING_TEMPLATE__PROHIBITED, &pva_prohibited)  );
    if (pva_prohibited)
    {
        SG_ERR_CHECK(  sg_zingtemplate__varray__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__PROHIBITED, pva_prohibited, SG_VARIANT_TYPE_INT64, &count_prohibited)  );

        if (count_allowed && count_prohibited)
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  has both prohibited and allowed", psz_where, psz_cur)
                    );
        }

        if (b_has_defaultvalue)
        {
            for (i=0; i<count_prohibited; i++)
            {
                SG_int64 va = 0;

                SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pva_prohibited, i, &va)  );
                if (i_defaultvalue == va)
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                            (pCtx, "%s.%s:  has defaultvalue which is in list of values prohibited", psz_where, psz_cur)
                            );
                }
            }
        }

        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

        for (i=0; i<count_prohibited; i++)
        {
            SG_int64 va = 0;
            SG_bool b = SG_FALSE;
            SG_int_to_string_buffer sz_i;

            SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pva_prohibited, i, &va)  );
            SG_int64_to_sz(va, sz_i);

            b = SG_FALSE;
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb, sz_i, &b, NULL)  );
            if (b)
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "%s.%s:  duplicate value in prohibited[]", psz_where, psz_cur)
                        );
            }

            SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, sz_i)  );
        }
        SG_RBTREE_NULLFREE(pCtx, prb);
    }

    // fall thru

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void sg_zingtemplate__validate_field_name(
        SG_context* pCtx,
        const char* psz_where,
        const char* psz_cur
        )
{
    SG_uint32 i = 0;
    SG_bool b = SG_FALSE;

    if ((SG_int64)strlen(psz_cur) > SG_ZING_MAX_FIELD_NAME_LENGTH)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  name too long (max %d characters)", psz_where, psz_cur, SG_ZING_MAX_FIELD_NAME_LENGTH)
                );
    }

    SG_ERR_CHECK(  SG_zing__is_reserved_field_name(pCtx, psz_cur, &b)  );
    if (b)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "%s.%s:  cannot use reseved field name", psz_where, psz_cur)
                );
    }

    while (1)
    {
        char c = psz_cur[i];

        if (!c)
        {
            break;
        }

        if (
                (0 == i)
                && (c >= '0')
                && (c <= '9')
           )
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  name cannot start with a digit", psz_where, psz_cur)
                    );
        }

        if (
                ('_' == c)
                || (  (c >= '0') && (c <= '9')  )
                || (  (c >= 'a') && (c <= 'z')  )
                || (  (c >= 'A') && (c <= 'Z')  )
           )
        {
            // no problem
        }
        else
        {
            SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                    (pCtx, "%s.%s:  invalid character in name", psz_where, psz_cur)
                    );
        }

        i++;
    }

    // fall thru

fail:
    return;
}

static void SG_zingtemplate__validate__one_field(
        SG_context* pCtx,
        SG_uint64 dagnum,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_top,
        SG_vhash* pvh,
        const char* psz_merge_type
        )
{
    const char* psz_datatype = NULL;
    SG_vhash* pvh_constraints = NULL;
    SG_string* pstr_where = NULL;
    SG_bool b_field_merge = SG_FALSE;
    SG_vhash* pvh_merge = NULL;
    SG_vhash* pvh_form = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__validate_field_name(pCtx, psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__DATATYPE, &psz_datatype)  );

    b_field_merge = (
            psz_merge_type
            && (0 == strcmp("field", psz_merge_type))
       );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh,
                SG_ZING_TEMPLATE__DATATYPE,
                SG_ZING_TEMPLATE__CONSTRAINTS,
                SG_ZING_TEMPLATE__FORM,
                (b_field_merge && !SG_DAGNUM__USE_TRIVIAL_MERGE(dagnum)) ? SG_ZING_TEMPLATE__MERGE : "",
                NULL)  );

    if (0 == strcmp(psz_datatype, "reference"))
    {
        SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__CONSTRAINTS, &pvh_constraints)  );
        SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__reference(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_top, pvh_constraints)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_zingtemplate__optional__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__CONSTRAINTS, &pvh_constraints)  );

        if (pvh_constraints)
        {
            if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__INT))
            {
                SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__int(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_constraints)  );
            }
            else if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__DATETIME))
            {
                SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__datetime(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_constraints)  );
            }
            else if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__USERID))
            {
                SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__userid(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_constraints)  );
            }
            else if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__STRING))
            {
                SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__string(pCtx, dagnum, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_constraints)  );
            }
            else if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__BOOL))
            {
                SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__bool(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_constraints)  );
            }
            else if (0 == strcmp(psz_datatype, SG_ZING_TYPE_NAME__ATTACHMENT))
            {
                SG_ERR_CHECK(  SG_zingtemplate__validate__constraints__attachment(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__CONSTRAINTS, pvh_constraints)  );
            }
            else
            {
                // TODO handle other field types here
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                        (pCtx, "Invalid datatype: '%s'", psz_datatype)
                        );
            }
        }
    }

    SG_ERR_CHECK(  sg_zingtemplate__optional__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__FORM, &pvh_form)  );
    if (pvh_form)
    {
        SG_ERR_CHECK(  SG_zingtemplate__validate_field_form(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__FORM, psz_datatype, pvh_constraints, pvh_form)  );
    }

    if (b_field_merge)
    {
        SG_bool b_unique = SG_FALSE;

        // if unique is present, MERGE is required
        // (so it can contain UNIQIFY, which is required)

        if (pvh_constraints)
        {
            SG_bool b_has_unique = SG_FALSE;
            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__UNIQUE, &b_has_unique)  );
            if (b_has_unique)
            {
                SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__UNIQUE, &b_unique)  );
            }
        }

        if (b_unique)
        {
            SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__MERGE, &pvh_merge)  );
        }
        else
        {
            SG_ERR_CHECK(  sg_zingtemplate__optional__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__MERGE, &pvh_merge)  );
        }

        if (pvh_merge)
        {
            SG_ERR_CHECK(  SG_zingtemplate__validate_field_merge(pCtx, dagnum, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__MERGE, pvh_top, psz_datatype, pvh_constraints, pvh_merge)  );
        }
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void SG_zingtemplate__validate__one_rectype(
        SG_context* pCtx,
        SG_uint64 dagnum,
        const char* psz_where,
        const char* psz_cur,
        SG_vhash* pvh_top,
        SG_vhash* pvh
        )
{
    SG_vhash* pvh_fields = NULL;
    SG_uint32 count_fields = 0;
    SG_uint32 i = 0;
    SG_string* pstr_where = NULL;
    const char* psz_merge_type = NULL;
    SG_vhash* pvh_merge = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s", psz_where, psz_cur)  );

    // the rules for the name of a rectype are the same as for a field name
    SG_ERR_CHECK(  sg_zingtemplate__validate_field_name(pCtx, psz_where, psz_cur)  );

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, psz_where, psz_cur, pvh,
                SG_ZING_TEMPLATE__FIELDS,
                SG_DAGNUM__USE_TRIVIAL_MERGE(dagnum) ? "" : SG_ZING_TEMPLATE__MERGE,
                NULL)  );


    if (
            !SG_DAGNUM__USE_TRIVIAL_MERGE(dagnum)
            )
    {
        SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__MERGE, &pvh_merge)  );

        SG_ERR_CHECK(  sg_zingtemplate__validate_fallback_merge(pCtx, psz_where, psz_cur, pvh_top, pvh_merge)  );

        SG_ERR_CHECK(  sg_zingtemplate__required__sz(pCtx, psz_where, psz_cur, pvh_merge, SG_ZING_TEMPLATE__MERGE_TYPE, &psz_merge_type)  );
    }

    SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, psz_where, psz_cur, pvh, SG_ZING_TEMPLATE__FIELDS, &pvh_fields)  );
    SG_ERR_CHECK(  sg_zingtemplate__vhash__check_type_of_members(pCtx, SG_string__sz(pstr_where), SG_ZING_TEMPLATE__FIELDS, pvh_fields, SG_VARIANT_TYPE_VHASH, &count_fields)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s.%s.%s", psz_where, psz_cur, SG_ZING_TEMPLATE__FIELDS)  );
    for (i=0; i<count_fields; i++)
    {
        const SG_variant* pv = NULL;
        const char* psz_field = NULL;
        SG_vhash* pvh_one_field = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_fields, i, &psz_field, &pv)  );
        SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_one_field)  );
        SG_ERR_CHECK(  SG_zingtemplate__validate__one_field(pCtx, dagnum, SG_string__sz(pstr_where), psz_field, pvh_top, pvh_one_field, psz_merge_type)  );
    }

    // fall thru

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void x_zingtemplate__validate(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_vhash* pvh
        )
{
    SG_vhash* pvh_rectypes = NULL;
    SG_uint32 count_rectypes = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  sg_zingtemplate__allowed_members(pCtx, "", "", pvh,
                SG_ZING_TEMPLATE__VERSION,
                SG_ZING_TEMPLATE__RECTYPES,
                NULL)  );

    SG_ERR_CHECK(  sg_zingtemplate__required__vhash(pCtx, "", "", pvh, SG_ZING_TEMPLATE__RECTYPES, &pvh_rectypes)  );

    SG_ERR_CHECK(  sg_zingtemplate__vhash__check_type_of_members(pCtx, "", SG_ZING_TEMPLATE__RECTYPES, pvh_rectypes, SG_VARIANT_TYPE_VHASH, &count_rectypes)  );

    if (
            (count_rectypes > 1)
            && SG_DAGNUM__HAS_SINGLE_RECTYPE(dagnum)
       )
    {
        SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
                (pCtx, "This dag only allows one rectype")
                );
    }

    for (i=0; i<count_rectypes; i++)
    {
        const SG_variant* pv = NULL;
        const char* psz_rectype = NULL;
        SG_vhash* pvh_one_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_rectypes, i, &psz_rectype, &pv)  );
        SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_one_rectype)  );
        SG_ERR_CHECK(  SG_zingtemplate__validate__one_rectype(pCtx, dagnum, SG_ZING_TEMPLATE__RECTYPES, psz_rectype, pvh, pvh_one_rectype)  );
    }

    return;

fail:
    return;
}

void SG_zingtemplate__validate(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_vhash* pvh,
        SG_uint32* pi_version
        )
{
    SG_int64 version = 0;

    SG_ERR_CHECK(  sg_zingtemplate__required__int64(pCtx, "", "", pvh, SG_ZING_TEMPLATE__VERSION, &version)  );
    switch (version)
    {
        case 1:
            {
                SG_ERR_CHECK(  x_zingtemplate__validate(pCtx, dagnum, pvh)  );
                *pi_version = (SG_uint32) version;
            }
            break;
        default:
            {
                SG_ERR_THROW2(  SG_ERR_ZING_INVALID_TEMPLATE,
								(pCtx, "Template version '%d' is unsupported", (int)version)
                        );
            }
            break;
    }

    return;

fail:
    return;
}

void SG_zingtemplate__alloc(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_vhash** ppvh,
        SG_zingtemplate** pptemplate
        )
{
    SG_zingtemplate* pThis = NULL;
    SG_uint32 version = 0;

    SG_ERR_CHECK(  SG_zingtemplate__validate(pCtx, dagnum, *ppvh, &version)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );
    SG_ERR_CHECK(  SG_mutex__init(pCtx, &pThis->lock)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->prb_field_attributes)  );
    pThis->version = version;
    pThis->pvh = *ppvh;
    *ppvh = NULL;

    *pptemplate = pThis;
    pThis = NULL;

    return;

fail:
    SG_NULLFREE(pCtx, pThis);
    return;
}

void sg_zingtemplate__get_only_rectype(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char** ppsz_name,
        SG_vhash** ppvh_rectype
        )
{
    SG_uint32 count = 0;
    SG_vhash* pvh_types = NULL;
    const char* psz_name = NULL;
    SG_vhash* pvh_rectype = NULL;

    /*
     * Trivial db dags are allowed only one rectype.  The syntax
     * of the template is the same.  The rectype has a name in the
     * template, but it is never stored in the record.
     */

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pztemplate->pvh, SG_ZING_TEMPLATE__RECTYPES, &pvh_types)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_types, &count)  );
    if (1 != count)
    {
        SG_ERR_THROW(  SG_ERR_ZING_ONLY_ONE_RECTYPE_ALLOWED  );
    }
    else
    {
        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_types, 0, &psz_name, &pv)  );
		SG_ERR_CHECK(  SG_variant__get__vhash(pCtx,pv,&pvh_rectype)  );
    }

    if (ppsz_name)
    {
        *ppsz_name = psz_name;
    }

    if (ppvh_rectype)
    {
        *ppvh_rectype = pvh_rectype;
    }

    // fall thru
fail:
    ;
}

void SG_zingtemplate__list_fields__vhash(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_vhash* pvh_fields = NULL;
    SG_vhash* pvh_rectype = NULL;
    SG_vhash* pvh_types = NULL;

    // TODO change this to use sg_zingtemplate__get_only_rectype

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pztemplate->pvh, SG_ZING_TEMPLATE__RECTYPES, &pvh_types)  );
    if (psz_rectype)
    {
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_types, psz_rectype, &pvh_rectype)  );
    }
    else
    {
        // there should be exactly one rectype.  whatever its name is,
        // just grab it

        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_types, 0, NULL, &pv)  );
		SG_ERR_CHECK(  SG_variant__get__vhash(pCtx,pv,&pvh_rectype)  );
    }

    if (pvh_rectype)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype, SG_ZING_TEMPLATE__FIELDS, &pvh_fields)  );
        // TODO this could just return pvh_fields if the caller knew that
        // he does not own it and would only look at the keys.
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count)  );
        if (count > 0)
        {
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
            for (i=0; i<count; i++)
            {
                const char* psz_name = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_fields, i, &psz_name, NULL)  );
                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh, psz_name)  );
            }
        }
    }

    *ppvh = pvh;
    pvh = NULL;

    // fall thru
fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_zingtemplate__list_fields(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_stringarray** pp
        )
{
    SG_stringarray* psa = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_vhash* pvh_fields = NULL;
    SG_vhash* pvh_rectype = NULL;
    SG_vhash* pvh_types = NULL;

    // TODO change this to use sg_zingtemplate__get_only_rectype

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pztemplate->pvh, SG_ZING_TEMPLATE__RECTYPES, &pvh_types)  );
    if (psz_rectype)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_types, psz_rectype, &pvh_rectype)  );
    }
    else
    {
        // there should be exactly one rectype.  whatever its name is,
        // just grab it

        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_types, 0, NULL, &pv)  );
		SG_ERR_CHECK(  SG_variant__get__vhash(pCtx,pv,&pvh_rectype)  );
    }
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype, SG_ZING_TEMPLATE__FIELDS, &pvh_fields)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count)  );
    if (count > 0)
    {
        SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_name = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_fields, i, &psz_name, NULL)  );
            SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, psz_name)  );
        }
    }

    *pp = psa;
    psa = NULL;

    // fall thru
fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

void SG_zingtemplate__is_a_rectype(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_bool* pb,
        SG_uint32* pi_count_rectypes
        )
{
    SG_vhash* pvh_rectypes = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__path__get__vhash(pCtx, pztemplate->pvh, &pvh_rectypes, NULL, SG_ZING_TEMPLATE__RECTYPES, NULL)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_rectypes, pi_count_rectypes)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh_rectypes, psz_rectype, pb)  );
}

void SG_zingtemplate__list_rectypes__update_into_vhash(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        SG_vhash* pvh
        )
{
    SG_vhash* pvh_rectypes = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__path__get__vhash(pCtx, pztemplate->pvh, &pvh_rectypes, NULL, SG_ZING_TEMPLATE__RECTYPES, NULL)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rectypes, &count)  );
    for (i=0; i<count; i++)
    {
		const char* psz_rectype = NULL;
		const SG_variant* pv = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_rectypes, i, &psz_rectype, &pv)  );
        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh, psz_rectype)  );
    }

fail:
    ;
}

void SG_zingtemplate__list_rectypes(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        SG_rbtree** pprb
        )
{
    SG_vhash* pvh_rectypes = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_rbtree* prb = NULL;

    SG_ERR_CHECK(  SG_vhash__path__get__vhash(pCtx, pztemplate->pvh, &pvh_rectypes, NULL, SG_ZING_TEMPLATE__RECTYPES, NULL)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rectypes, &count)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, &prb, count, NULL)  );
    for (i=0; i<count; i++)
    {
		const char* psz_rectype = NULL;
		const SG_variant* pv = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_rectypes, i, &psz_rectype, &pv)  );
        SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psz_rectype)  );
    }

    *pprb = prb;
    prb = NULL;

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
}

void SG_zingtemplate__get_field_attributes(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        const char* psz_field_name,
        SG_zingfieldattributes** ppzfa
        )
{
    SG_zingfieldattributes* pzfa = NULL;
    SG_vhash* pvh_fa = NULL;
    SG_vhash* pvh_constraints = NULL;
    const char* psz_temp = NULL;
    const char* psz_owned_field_name = NULL;
    const char* psz_owned_rectype = NULL;
    SG_bool b = SG_FALSE;
    SG_vhash* pvh_fields = NULL;
    SG_vhash* pvh_rectypes = NULL;
    char buf_key[256];

    SG_NULLARGCHECK_RETURN(psz_field_name);

    SG_ERR_CHECK(  SG_mutex__lock(pCtx, &pztemplate->lock)  );

    SG_ERR_CHECK(  SG_sprintf(pCtx, buf_key, sizeof(buf_key), "%s.%s", psz_rectype, psz_field_name)  );
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pztemplate->prb_field_attributes, buf_key, &b, (void**) &pzfa)  );
    if (b && pzfa)
    {
        *ppzfa = pzfa;
        SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &pztemplate->lock)  );
        return;
    }

    SG_ERR_CHECK(  SG_vhash__path__get__vhash(pCtx, pztemplate->pvh, &pvh_rectypes, NULL, SG_ZING_TEMPLATE__RECTYPES, NULL)  );
    SG_ERR_CHECK(  SG_vhash__key(pCtx, pvh_rectypes, psz_rectype, &psz_owned_rectype)  );
    if (!psz_owned_rectype)
    {
        *ppzfa = NULL;
        SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &pztemplate->lock)  );
        return;
    }

    SG_ERR_CHECK(  SG_vhash__path__get__vhash(pCtx, pztemplate->pvh, &pvh_fields, NULL, SG_ZING_TEMPLATE__RECTYPES, psz_rectype, SG_ZING_TEMPLATE__FIELDS, NULL)  );

    SG_ERR_CHECK(  SG_vhash__key(pCtx, pvh_fields, psz_field_name, &psz_owned_field_name)  );
    if (!psz_owned_field_name)
    {
        *ppzfa = NULL;
        SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &pztemplate->lock)  );
        return;
    }

	SG_ERR_CHECK(  SG_alloc1(pCtx, pzfa)  );
    pzfa->name = psz_owned_field_name;
    pzfa->rectype = psz_owned_rectype;
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_fields, psz_field_name, &pvh_fa)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_fa, SG_ZING_TEMPLATE__DATATYPE, &psz_temp)  );
    SG_ERR_CHECK(  SG_strcpy(pCtx, pzfa->buf_type, sizeof(pzfa->buf_type), psz_temp)  );
    SG_ERR_CHECK(  SG_zing__field_type__sz_to_uint16(pCtx, psz_temp, &pzfa->type)  );

    // TODO init all the constraints to nothing, for each type
    pzfa->required = SG_FALSE;
    pzfa->index = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_fa, SG_ZING_TEMPLATE__CONSTRAINTS, &b)  );
    if (b)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_fa, SG_ZING_TEMPLATE__CONSTRAINTS, &pvh_constraints)  );

        // First do the type-independent stuff
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &b)  );
        if (b)
        {
            SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__REQUIRED, &pzfa->required)  );
        }

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &b)  );
        if (b)
        {
            SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__INDEX, &pzfa->index)  );
        }

        switch (pzfa->type)
        {
            case SG_ZING_TYPE__BOOL:
                {
                    pzfa->v._bool.has_defaultvalue = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b)  );
                    if (b)
                    {
                        pzfa->v._bool.has_defaultvalue = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &pzfa->v._bool.defaultvalue)  );
                    }
                    break;
                }

            case SG_ZING_TYPE__ATTACHMENT:
                {
                    pzfa->v._attachment.has_maxlength = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAXLENGTH, &b)  );
                    if (b)
                    {
                        pzfa->v._attachment.has_maxlength = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAXLENGTH, &pzfa->v._attachment.val_maxlength)  );
                    }
                    break;
                }

            case SG_ZING_TYPE__USERID:
                {
                    pzfa->v._userid.has_defaultvalue = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b)  );
                    if (b)
                    {
                        pzfa->v._userid.has_defaultvalue = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &pzfa->v._userid.defaultvalue)  );
                    }
                    break;
                }

            case SG_ZING_TYPE__REFERENCE:
                {
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_constraints, SG_ZING_TEMPLATE__RECTYPE, &pzfa->v._reference.rectype)  );
                    break;
                }

            case SG_ZING_TYPE__STRING:
                {
                    pzfa->v._string.num_digits = 0;
                    pzfa->v._string.has_defaultfunc = SG_FALSE;
                    pzfa->v._string.has_defaultvalue = SG_FALSE;
                    pzfa->v._string.has_minlength = SG_FALSE;
                    pzfa->v._string.has_maxlength = SG_FALSE;
                    pzfa->v._string.prohibited = NULL;
                    pzfa->v._string.allowed = NULL;
                    pzfa->v._string.unique = SG_FALSE;
                    pzfa->v._string.sort_by_allowed = SG_FALSE;
                    pzfa->v._string.full_text_search = SG_FALSE;

                    // -------- unique
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__UNIQUE, &b)  );
                    if (b)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__UNIQUE, &pzfa->v._string.unique)  );
                        if (pzfa->v._string.unique)
                        {
                            pzfa->index = SG_TRUE;
                        }
                    }

                    // -------- genchars
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__GENCHARS, &b)  );
                    if (b)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_constraints, SG_ZING_TEMPLATE__GENCHARS, &pzfa->v._string.genchars)  );
                    }

                    // -------- defaultfunc
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTFUNC, &b)  );
                    if (b)
                    {
                        pzfa->v._string.has_defaultfunc = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTFUNC, &pzfa->v._string.defaultfunc)  );
                    }

                    // -------- num_digits
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__NUM_DIGITS, &b)  );
                    if (b)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__NUM_DIGITS, &pzfa->v._string.num_digits)  );
                    }

                    // -------- defaultvalue
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b)  );
                    if (b)
                    {
                        pzfa->v._string.has_defaultvalue = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &pzfa->v._string.defaultvalue)  );
                    }

                    // -------- minlength
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MINLENGTH, &b)  );
                    if (b)
                    {
                        pzfa->v._string.has_minlength = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MINLENGTH, &pzfa->v._string.val_minlength)  );
                    }

                    // -------- full_text_search
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__FULL_TEXT_SEARCH, &b)  );
                    if (b)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__FULL_TEXT_SEARCH, &pzfa->v._string.full_text_search)  );
                    }

                    // -------- maxlength
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAXLENGTH, &b)  );
                    if (b)
                    {
                        pzfa->v._string.has_maxlength = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAXLENGTH, &pzfa->v._string.val_maxlength)  );
                    }
                    
                    // -------- prohibited
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__PROHIBITED, &b)  );
                    if (b)
                    {
                        SG_varray* pva = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_constraints, SG_ZING_TEMPLATE__PROHIBITED, &pva)  );
                        SG_ERR_CHECK(  SG_varray__to_stringarray(pCtx, pva, &pzfa->v._string.prohibited)  );
                    }

                    // -------- allowed
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &b)  );
                    if (b)
                    {
                        SG_varray* pva = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &pva)  );
                        SG_ERR_CHECK(  SG_varray__to_vhash_with_indexes(pCtx, pva, &pzfa->v._string.allowed)  );
                        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__SORT_BY_ALLOWED, &b)  );
                        if (b)
                        {
                            SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_constraints, SG_ZING_TEMPLATE__SORT_BY_ALLOWED, &pzfa->v._string.sort_by_allowed)  );
                        }
                    }

                    // -------- bad
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__BAD, &b)  );
                    if (b)
                    {
                        SG_varray* pva = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_constraints, SG_ZING_TEMPLATE__BAD, &pva)  );
                        SG_ERR_CHECK(  SG_varray__to_vhash_with_indexes__update(pCtx, pva, &pzfa->v._string.bad)  );
                    }
                    break;
                }

            case SG_ZING_TYPE__INT:
                {
                    pzfa->v._int.has_defaultvalue = SG_FALSE;
                    pzfa->v._int.has_min = SG_FALSE;
                    pzfa->v._int.has_max = SG_FALSE;
                    pzfa->v._int.prohibited = NULL;
                    pzfa->v._int.allowed = NULL;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b)  );
                    if (b)
                    {
                        pzfa->v._int.has_defaultvalue = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &pzfa->v._int.defaultvalue)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MIN, &b)  );
                    if (b)
                    {
                        pzfa->v._int.has_min = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MIN, &pzfa->v._int.val_min)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAX, &b)  );
                    if (b)
                    {
                        pzfa->v._int.has_max = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAX, &pzfa->v._int.val_max)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__PROHIBITED, &b)  );
                    if (b)
                    {
                        SG_varray* pva = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_constraints, SG_ZING_TEMPLATE__PROHIBITED, &pva)  );
                        SG_ERR_CHECK(  SG_vector_i64__alloc__from_varray(pCtx, pva, &pzfa->v._int.prohibited)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &b)  );
                    if (b)
                    {
                        SG_varray* pva = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_constraints, SG_ZING_TEMPLATE__ALLOWED, &pva)  );
                        SG_ERR_CHECK(  SG_vector_i64__alloc__from_varray(pCtx, pva, &pzfa->v._int.allowed)  );
                    }
                    break;
                }

            case SG_ZING_TYPE__DATETIME:
                {
                    pzfa->v._datetime.has_defaultvalue = SG_FALSE;
                    pzfa->v._datetime.defaultvalue = NULL;
                    pzfa->v._datetime.has_min = SG_FALSE;
                    pzfa->v._datetime.has_max = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &b)  );
                    if (b)
                    {
                        pzfa->v._datetime.has_defaultvalue = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_constraints, SG_ZING_TEMPLATE__DEFAULTVALUE, &pzfa->v._datetime.defaultvalue)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MIN, &b)  );
                    if (b)
                    {
                        pzfa->v._datetime.has_min = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64_or_double(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MIN, &pzfa->v._datetime.val_min)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAX, &b)  );
                    if (b)
                    {
                        pzfa->v._datetime.has_max = SG_TRUE;
                        SG_ERR_CHECK(  SG_vhash__get__int64_or_double(pCtx, pvh_constraints, SG_ZING_TEMPLATE__MAX, &pzfa->v._datetime.val_max)  );
                    }
                    break;
                }
        }
    }

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_fa, SG_ZING_TEMPLATE__MERGE, &b)  );
    if (b)
    {
        SG_vhash* pvh_merge = NULL;

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_fa, SG_ZING_TEMPLATE__MERGE, &pvh_merge)  );

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_merge, SG_ZING_TEMPLATE__AUTO, &b)  );
        if (b)
        {
            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_merge, SG_ZING_TEMPLATE__AUTO, &pzfa->pva_automerge)  );
        }

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_merge, SG_ZING_TEMPLATE__UNIQIFY, &b)  );
        if (b)
        {
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_merge, SG_ZING_TEMPLATE__UNIQIFY, &pzfa->v._string.uniqify)  );
        }
    }

    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztemplate->prb_field_attributes, buf_key, pzfa)  );
    SG_ERR_CHECK(  SG_mutex__unlock(pCtx, &pztemplate->lock)  );

    *ppzfa = pzfa;
    pzfa = NULL;

fail:
    SG_ZINGFIELDATTRIBUTES_NULLFREE(pCtx, pzfa);
}

void SG_zingtemplate__get_json(
        SG_context* pCtx,
        SG_zingtemplate* pThis,
        SG_string** ppstr
        )
{
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pThis->pvh, pstr)  );

    *ppstr = pstr;
    pstr = NULL;

    return;

fail:
    return;
}

void SG_zingtemplate__get_vhash(SG_context* pCtx, SG_zingtemplate* pThis, SG_vhash ** ppResult)
{
  SG_UNUSED(pCtx);
    *ppResult = pThis->pvh;
}

void SG_zingtemplate__get_rectype_info(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        const char* psz_rectype,
        SG_zingrectypeinfo** pp
        )
{
    SG_zingrectypeinfo* pmi = NULL;
    SG_vhash* pvh_rectype = NULL;
    const char* psz_merge_type = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, pmi)  );

    SG_ERR_CHECK(  SG_vhash__path__get__vhash(pCtx, pztemplate->pvh, &pvh_rectype, NULL, SG_ZING_TEMPLATE__RECTYPES, psz_rectype, NULL)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype, SG_ZING_TEMPLATE__MERGE, &pmi->pvh_merge)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pmi->pvh_merge, SG_ZING_TEMPLATE__MERGE_TYPE, &psz_merge_type)  );

    if (0 == strcmp("record", psz_merge_type))
    {
        pmi->b_merge_type_is_record = SG_TRUE;
    }
    else
    {
        pmi->b_merge_type_is_record = SG_FALSE;
    }

    *pp = pmi;
    pmi = NULL;

    // fall thru

fail:
    SG_ZINGRECTYPEINFO_NULLFREE(pCtx, pmi);
}

void SG_zingrectypeinfo__free(SG_context* pCtx, SG_zingrectypeinfo* p)
{
    if (!p)
    {
        return;
    }

    SG_NULLFREE(pCtx, p);
}

static SG_mutex g_mutex_template_cache_map;
static SG_mutex g_mutex_template_cache_store;
static SG_mutex g_mutex_template_cache_dagnum;

static SG_rbtree* g_template_cache_map;
static SG_rbtree* g_template_cache_store;
static SG_rbtree* g_template_cache_dagnum;

void SG_zing__get_cached_template__hid_template(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_hid_template,
        SG_zingtemplate** ppzt
        )
{
    char *psz_repoid = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_bool b = SG_FALSE;
    SG_vhash* pvh_template = NULL;

    SG_ASSERT(!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum));

    SG_ERR_CHECK(  SG_mutex__lock(pCtx, &g_mutex_template_cache_store)  );

    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, g_template_cache_store, psz_hid_template, &b, (void**) &pzt)  );

    if (!b)
    {
        SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_hid_template, &pvh_template)  );
        SG_ERR_CHECK(  SG_zingtemplate__alloc(pCtx, dagnum, &pvh_template, &pzt)  );


        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, g_template_cache_store, psz_hid_template, pzt)  );
    }

    *ppzt = pzt;
    pzt = NULL;

	/* Fall through to common cleanup */
fail:
	SG_NULLFREE(pCtx, psz_repoid);

	if SG_CONTEXT__HAS_ERR(pCtx)
	{
		SG_mutex__unlock__bare(&g_mutex_template_cache_store);
	}
	else
	{
		/* This is a legit SG_ERR_CHECK_RETURN, despite being in a fail block, 
			because we're in common cleanup code and we've verified above that 
			we're not in an error state. */
		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &g_mutex_template_cache_store)  );
	}

}

void SG_zing__get_cached_hid_for_template__csid(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid,
        const char** ppsz_hid_template
        )
{
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
    char buf_cache_key[SG_GID_ACTUAL_LENGTH + SG_DAGNUM__BUF_MAX__HEX + SG_HID_MAX_BUFFER_LENGTH + 10];
    char *psz_repoid = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_hid_template = NULL;
    SG_changeset* pcs = NULL;
    SG_vhash* pvh_delta = NULL;

    SG_ASSERT(!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum));

    SG_ERR_CHECK(  SG_mutex__lock(pCtx, &g_mutex_template_cache_map)  );

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );
	SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &psz_repoid)  );
    SG_ERR_CHECK(  SG_sprintf(pCtx, buf_cache_key, sizeof(buf_cache_key), "%s.%s.%s",
                psz_repoid,
                buf_dagnum,
                psz_csid)  );
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, g_template_cache_map, buf_cache_key, &b, (void**) &psz_hid_template)  );
    if (!b)
    {
        SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, psz_csid, &pcs)  );

        SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_hid_template)  );

        SG_ERR_CHECK(  SG_rbtree__add__with_pooled_sz(pCtx, g_template_cache_map, buf_cache_key, psz_hid_template)  );

        SG_ERR_CHECK(  SG_rbtree__find(pCtx, g_template_cache_map, buf_cache_key, NULL, (void**) &psz_hid_template)  );
    }

    *ppsz_hid_template = psz_hid_template;

	/* Fall through to common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_delta);
	SG_CHANGESET_NULLFREE(pCtx, pcs);
	SG_NULLFREE(pCtx, psz_repoid);

	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_mutex__unlock__bare(&g_mutex_template_cache_map);
	}
	else
	{
		/* This is a legit SG_ERR_CHECK_RETURN, despite being in a fail block, 
			because we're in common cleanup code and we've verified above that 
			we're not in an error state. */
		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &g_mutex_template_cache_map)  );
	}
}

void SG_zing__get_cached_template__csid(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid,
        SG_zingtemplate** ppzt
        )
{
    const char* psz_hid_template = NULL;

    SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, dagnum, psz_csid, &psz_hid_template)  );
    SG_ERR_CHECK_RETURN(  SG_zing__get_cached_template__hid_template(pCtx, pRepo, dagnum, psz_hid_template, ppzt)  );
}

void SG_zing__init_template_caches(
        SG_context* pCtx
        )
{
    SG_ERR_CHECK_RETURN(  SG_mutex__init(pCtx, &g_mutex_template_cache_map)  );
    SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, &g_template_cache_map)  );

    SG_ERR_CHECK_RETURN(  SG_mutex__init(pCtx, &g_mutex_template_cache_store)  );
    SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, &g_template_cache_store)  );

    SG_ERR_CHECK_RETURN(  SG_mutex__init(pCtx, &g_mutex_template_cache_dagnum)  );
    SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, &g_template_cache_dagnum)  );
}

void SG_zing__now_free_all_cached_templates(
        SG_context* pCtx
        )
{
    SG_mutex__destroy(&g_mutex_template_cache_map);
    SG_mutex__destroy(&g_mutex_template_cache_store);
    SG_mutex__destroy(&g_mutex_template_cache_dagnum);

    SG_RBTREE_NULLFREE(pCtx, g_template_cache_map);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, g_template_cache_store, (SG_free_callback*) SG_zingtemplate__free);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, g_template_cache_dagnum, (SG_free_callback*) SG_zingtemplate__free);
}

extern unsigned char sg_ztemplate__comments_json[];
extern unsigned char sg_ztemplate__stamps_json[];
extern unsigned char sg_ztemplate__tags_json[];
extern unsigned char sg_ztemplate__branches_json[];
extern unsigned char sg_ztemplate__oldbranches_json[];
extern unsigned char sg_ztemplate__hooks_json[];
extern unsigned char sg_ztemplate__locks_json[];
extern unsigned char sg_ztemplate__users_json[];
extern unsigned char sg_ztemplate__areas_json[];

extern void my_strip_comments(
    SG_context* pCtx,
    char* pjson,
    SG_string** ppstr
    );

void SG_zing__fetch_static_template__json(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_string** ppstr
        )
{
    if (dagnum == SG_DAGNUM__VC_COMMENTS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__comments_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__VC_STAMPS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__stamps_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__VC_TAGS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__tags_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__VC_BRANCHES) 
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__branches_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, SG_DAGNUM__VC_BRANCHES))
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__oldbranches_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__VC_LOCKS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__locks_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__VC_HOOKS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__hooks_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__USERS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__users_json, ppstr)  );
    }
    else if (dagnum == SG_DAGNUM__AREAS)
    {
        SG_ERR_CHECK_RETURN(  my_strip_comments(pCtx, (char*) sg_ztemplate__areas_json, ppstr)  );
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_ZING_TEMPLATE_NOT_FOUND  );
    }
}

void SG_zing__get_cached_template__static_dagnum(
        SG_context* pCtx,
        SG_uint64 dagnum,
        SG_zingtemplate** ppzt
        )
{
    SG_zingtemplate* pzt = NULL;
    SG_bool b = SG_FALSE;
    SG_vhash* pvh_template = NULL;
	char bufDagnum[SG_DAGNUM__BUF_MAX__HEX];
    SG_string* pstr_json = NULL;

    SG_ASSERT(SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum));

    SG_ERR_CHECK(  SG_mutex__lock(pCtx, &g_mutex_template_cache_dagnum)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, bufDagnum, sizeof(bufDagnum))  );

    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, g_template_cache_dagnum, bufDagnum, &b, (void**) &pzt)  );

    if (!b)
    {
        SG_ERR_CHECK(  SG_zing__fetch_static_template__json(pCtx, dagnum, &pstr_json)  );
        SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh_template, SG_string__sz(pstr_json)));
        SG_STRING_NULLFREE(pCtx, pstr_json);

        SG_ERR_CHECK(  SG_zingtemplate__alloc(pCtx, dagnum, &pvh_template, &pzt)  );

        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, g_template_cache_dagnum, bufDagnum, pzt)  );
    }

    *ppzt = pzt;
    pzt = NULL;

	/* Fall through to common cleanup */
fail:
	SG_STRING_NULLFREE(pCtx, pstr_json);
    
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_mutex__unlock__bare(&g_mutex_template_cache_dagnum);
	}
	else
	{
		/* This is a legit SG_ERR_CHECK_RETURN, despite being in a fail block, 
		   because we're in common cleanup code and we've verified above that 
		   we're not in an error state. */
		SG_ERR_CHECK_RETURN(  SG_mutex__unlock(pCtx, &g_mutex_template_cache_dagnum)  );
	}
}

void SG_zing__collect_fields_one_rectype_one_template(
	SG_context* pCtx,
    const char* psz_rectype,
    SG_zingtemplate* pzt,
    SG_vhash* pvh_result
    )
{
    SG_vhash* pvh_fields_in_this_template = NULL;
    SG_uint32 i_field = 0;
    SG_vhash* pvh_allowed_copy = NULL;
    SG_string* pstr = NULL;
    char* psz_hash = NULL;

    SG_ERR_CHECK(  SG_zingtemplate__list_fields__vhash(pCtx, pzt, psz_rectype, &pvh_fields_in_this_template)  );
    if (pvh_fields_in_this_template)
    {
        SG_uint32 count_fields_in_this_template = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields_in_this_template, &count_fields_in_this_template )  );
        for (i_field=0; i_field<count_fields_in_this_template; i_field++)
        {
            const char* psz_field_name = NULL;
            SG_zingfieldattributes* pzfa = NULL;
            SG_bool b_already = SG_FALSE;
            SG_vhash* pvh_one_field = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_fields_in_this_template, i_field, &psz_field_name, NULL)  );
            SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, psz_rectype, psz_field_name, &pzfa)  );

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_result, psz_field_name, &b_already)  );
            if (b_already)
            {
                const char* psz_prev_type = NULL;

                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_result, psz_field_name, &pvh_one_field)  );

                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_prev_type)  );
                if (0 != strcmp(psz_prev_type, pzfa->buf_type))
                {
                    // can't change type
                    SG_ERR_THROW(  SG_ERR_ZING_TYPE_CHANGE_NOT_ALLOWED  );
                }

                if (pzfa->index)
                {
                    SG_ERR_CHECK(  SG_vhash__update__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__INDEX, SG_TRUE)  );
                    if (
                            (SG_ZING_TYPE__STRING == pzfa->type)
                            && (pzfa->v._string.unique)
                       )
                    {
                        SG_ERR_CHECK(  SG_vhash__update__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__UNIQUE, SG_TRUE)  );
                    }
                }
            }
            else
            {
                SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_result, psz_field_name, &pvh_one_field)  );
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__TYPE, pzfa->buf_type)  );
                SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__INDEX, pzfa->index)  );
                if (pzfa->index)
                {
                    if (
                            (SG_ZING_TYPE__STRING == pzfa->type)
                            && (pzfa->v._string.unique)
                       )
                    {
                        SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__UNIQUE, SG_TRUE)  );
                    }
                }
            }

            if (
                    (SG_ZING_TYPE__STRING == pzfa->type)
                    && (pzfa->v._string.full_text_search)
                    )
            {
                SG_ERR_CHECK(  SG_vhash__update__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, SG_TRUE)  );
            }

            if (
                    (SG_ZING_TYPE__STRING == pzfa->type)
                    && (pzfa->v._string.sort_by_allowed)
                    )
            {
                SG_vhash* pvh_orderings = NULL;

                SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__ORDERINGS, &pvh_orderings)  );

                SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &pvh_allowed_copy, pzfa->v._string.allowed)  );
                SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_allowed_copy, SG_TRUE, SG_vhash_sort_callback__increasing)  );

                SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh_allowed_copy, pstr)  );

                SG_ERR_CHECK(  sg_repo_utils__one_step_hash__from_sghash(pCtx, "SHA1/160", SG_STRLEN(SG_string__sz(pstr)), (SG_byte*) SG_string__sz(pstr), &psz_hash)  );
                SG_ERR_CHECK(  SG_vhash__update__vhash(pCtx, pvh_orderings, psz_hash, &pvh_allowed_copy)  );
                SG_STRING_NULLFREE(pCtx, pstr);
                SG_NULLFREE(pCtx, psz_hash);
            }

            if (SG_ZING_TYPE__REFERENCE == pzfa->type)
            {
                // TODO what if the ref_rectype of this reference field is not
                // always the same?
                SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, pzfa->v._reference.rectype)  );
            }

            switch (pzfa->type)
            {
                case SG_ZING_TYPE__INT:
                case SG_ZING_TYPE__BOOL:
                case SG_ZING_TYPE__DATETIME:
                case SG_ZING_TYPE__STRING:
                case SG_ZING_TYPE__ATTACHMENT:
                case SG_ZING_TYPE__USERID:
                case SG_ZING_TYPE__REFERENCE:
                {
                }
                break;

                default:
                {
                    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
                }
                break;
            }
        }

        SG_VHASH_NULLFREE(pCtx, pvh_fields_in_this_template);
    }

fail:
    SG_NULLFREE(pCtx, psz_hash);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh_allowed_copy);
    SG_VHASH_NULLFREE(pCtx, pvh_fields_in_this_template);
}

void SG_zing__missing_hardwired_template(SG_context* pCtx, SG_uint64 dagnum, SG_bool* pbMissing)
{
	SG_zingtemplate* pzt = NULL;
	SG_bool bMissing = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pbMissing);

	if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
	{
		SG_zing__get_cached_template__static_dagnum(pCtx, dagnum, &pzt);
		if (SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
		{
			SG_context__err_reset(pCtx);
			bMissing= SG_TRUE;
		}
		else
			SG_ERR_CHECK_RETURN_CURRENT;
	}

	*pbMissing = bMissing;
}
