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

#define ESCAPEDSTR(esc,raw)   ((const char *)(((esc != NULL) ? (esc) : (raw))))

// TODO temporary hack:
#ifdef WINDOWS
//#pragma warning(disable:4100)
#pragma warning(disable:4706)
#endif

struct SG_zingrecord
{
    SG_zingtx* pztx;
    SG_rbtree* prb_field_values;
    SG_rbtree* prb_attachments;
    SG_bool b_dirty_fields;
    SG_bool bDeleteMe;
    SG_varray* pva_history;
    char* psz_original_hid;
};

struct sg_attachment
{
    SG_pathname* pPath;
    SG_byte* pBytes;
    SG_uint32 len;
};

static void x_free_sg_attachment(SG_context* pCtx, struct sg_attachment* p)
{
    if (!p)
    {
        return;
    }
    SG_PATHNAME_NULLFREE(pCtx, p->pPath);
    SG_NULLFREE(pCtx, p->pBytes);
    SG_NULLFREE(pCtx, p);
}

static void sg_zingtx__change_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingrecord* pzrec,
        const char* psz_name,
        const char* psz_val
        );

void SG_zingtx__get_template(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingtemplate** ppResult
        )
{
  SG_UNUSED(pCtx);

    *ppResult = pztx->ptemplate;
}

static void sg_zingtx__free_template_if_mine(
        SG_context* pCtx,
        SG_zingtx* pztx
        )
{
    if (pztx->b_template_is_mine)
    {
        SG_ZINGTEMPLATE_NULLFREE(pCtx, pztx->ptemplate);
    }
    else
    {
        pztx->ptemplate = NULL;
    }

    SG_NULLFREE(pCtx, pztx->psz_hid_template);
}

void _sg_zingtx__free(
        SG_context* pCtx,
        SG_zingtx* pztx
        )
{
    if (!pztx)
    {
        return;
    }

    SG_ERR_IGNORE(  sg_zingtx__free_template_if_mine(pCtx, pztx)  );
    SG_RBTREE_NULLFREE(pCtx, pztx->prb_rechecks);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pztx->prb_records, (SG_free_callback*) SG_zingrecord__free);
    SG_RBTREE_NULLFREE(pCtx, pztx->prb_parents);
    SG_RBTREE_NULLFREE(pCtx, pztx->prb_delete_hidrec);
    SG_RBTREE_NULLFREE(pCtx, pztx->prb_deltas);
    SG_VHASH_NULLFREE(pCtx, pztx->pvh_user);
    SG_NULLFREE(pCtx, pztx->psz_csid);
    SG_NULLFREE(pCtx, pztx->psz_hid_template);
    SG_NULLFREE(pCtx, pztx);
}

void SG_zingtx__get_repo(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_repo** ppRepo
        )
{
  SG_UNUSED(pCtx);

    *ppRepo = pztx->pRepo;
}

void SG_zingtx__get_dagnum(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_uint64* pdagnum
        )
{
  SG_UNUSED(pCtx);

    *pdagnum = pztx->iDagNum;
}

void SG_zingtx__set_template__existing(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_hid_template
        )
{
	SG_NULLARGCHECK_RETURN(psz_hid_template);

    SG_ASSERT(!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pztx->iDagNum));

    SG_ERR_CHECK(  sg_zingtx__free_template_if_mine(pCtx, pztx)  );

    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_template, &pztx->psz_hid_template)  );
    SG_ERR_CHECK(  SG_zing__get_cached_template__hid_template(pCtx, pztx->pRepo, pztx->iDagNum, psz_hid_template, &pztx->ptemplate)  );

    pztx->b_template_dirty = SG_FALSE;
    pztx->b_template_is_mine = SG_FALSE;

fail:
    ;
}

void SG_zingtx__set_template__new(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_vhash** ppvh_template
        )
{
	SG_NULL_PP_CHECK_RETURN(ppvh_template);

    SG_ASSERT(!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pztx->iDagNum));

    SG_ERR_CHECK(  sg_zingtx__free_template_if_mine(pCtx, pztx)  );
    SG_ERR_CHECK(  SG_zingtemplate__alloc(pCtx, pztx->iDagNum, ppvh_template, &pztx->ptemplate)  );
    pztx->b_template_dirty = SG_TRUE;
    pztx->b_template_is_mine = SG_TRUE;

    // fall thru

fail:
    return;
}

static void sg_zing__load_template__hid_template(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_hid_template,
        SG_zingtemplate** ppzt
        )
{
    SG_vhash* pvh_template = NULL;
    SG_zingtemplate* pzt = NULL;

    if (psz_hid_template)
    {
        SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_hid_template, &pvh_template)  );
        SG_ERR_CHECK(  SG_zingtemplate__alloc(pCtx, dagnum, &pvh_template, &pzt)  );
    }

    *ppzt = pzt;
    pzt = NULL;

    // fall thru

fail:
    SG_ZINGTEMPLATE_NULLFREE(pCtx, pzt);
    SG_VHASH_NULLFREE(pCtx, pvh_template);
}

void SG_zing__begin_tx(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* who,
    const char* psz_hid_baseline,
	SG_zingtx** ppTx
	)
{
    SG_zingtx* pTx = NULL;
    SG_changeset* pcs = NULL;
    SG_vhash* pvh_delta = NULL;
    SG_vhash* pvh_template = NULL;
    SG_string* pstr_json = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(ppTx);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTx)  );

    pTx->pRepo = pRepo;
    pTx->iDagNum = iDagNum;
    pTx->b_in_commit = SG_FALSE;

    SG_NULLARGCHECK_RETURN(who);
    SG_ERR_CHECK(  SG_strcpy(pCtx, pTx->buf_who, sizeof(pTx->buf_who), who)  );

    if (psz_hid_baseline)
    {
        const char* psz_hid_template = NULL;

        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_baseline, &pTx->psz_csid)  );

        SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, pTx->psz_csid, &pcs)  );

        if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pTx->iDagNum))
        {
            SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_hid_template)  );
            SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_template, &pTx->psz_hid_template)  );
            SG_ERR_CHECK_RETURN(  sg_zing__load_template__hid_template(pCtx, pTx->pRepo, pTx->iDagNum, pTx->psz_hid_template, &pTx->ptemplate)  );
            pTx->b_template_is_mine = SG_TRUE;
        }

        SG_CHANGESET_NULLFREE(pCtx, pcs);
    }

    if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pTx->iDagNum))
    {
        SG_ERR_CHECK(  SG_zing__fetch_static_template__json(pCtx, pTx->iDagNum, &pstr_json)  );
        SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh_template, SG_string__sz(pstr_json)));
        SG_STRING_NULLFREE(pCtx, pstr_json);
        SG_ERR_CHECK(  SG_zingtemplate__alloc(pCtx, pTx->iDagNum, &pvh_template, &pTx->ptemplate)  );
        pTx->b_template_is_mine = SG_TRUE;
    }

    if (SG_DAGNUM__HAS_NO_RECID(pTx->iDagNum))
    {
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pTx->prb_delete_hidrec)  );
    }

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pTx->prb_records)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pTx->prb_parents)  );

    *ppTx = pTx;
    pTx = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_delta);
    SG_VHASH_NULLFREE(pCtx, pvh_template);
    SG_STRING_NULLFREE(pCtx, pstr_json);
    SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pTx)  );
}

void SG_zingrecord__alloc(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_original_hid,
        const char* psz_recid,			// This is a GID
        const char* psz_rectype,
        SG_zingrecord** pprec
        )
{
    SG_zingrecord* pzrec = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pzrec)  );
    pzrec->pztx = pztx;
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pzrec->prb_field_values)  );

    if (!SG_DAGNUM__HAS_SINGLE_RECTYPE(pztx->iDagNum))
    {
        SG_ERR_CHECK(  SG_rbtree__add__with_pooled_sz(pCtx, pzrec->prb_field_values, SG_ZING_FIELD__RECTYPE, psz_rectype)  );
    }

    if (psz_recid)
    {
        SG_ERR_CHECK(  SG_rbtree__add__with_pooled_sz(pCtx, pzrec->prb_field_values, SG_ZING_FIELD__RECID, psz_recid)  );
    }

    if (psz_original_hid)
    {
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_original_hid, &pzrec->psz_original_hid)  );
    }

    *pprec = pzrec;
    pzrec = NULL;

    return;

fail:
    SG_ZINGRECORD_NULLFREE(pCtx, pzrec);
}

void SG_zingrecord__alloc__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_dbrecord* pdbrec,
        SG_bool b_use_hid,
        SG_zingrecord** ppzrec
        )
{
    SG_zingrecord* pzrec = NULL;
    const char* psz_recid = NULL;		// This is a GID
    const char* psz_rectype = NULL;
    const char* psz_hid = NULL;
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec, SG_ZING_FIELD__RECID, &psz_recid)  );
    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pztx->iDagNum))
    {
        SG_ERR_CHECK_RETURN(  sg_zingtemplate__get_only_rectype(pCtx, pztx->ptemplate, &psz_rectype, NULL)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
    }

    if (b_use_hid)
    {
        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pdbrec, &psz_hid)  );
    }

    SG_ERR_CHECK(  SG_zingrecord__alloc(pCtx, pztx, psz_hid, psz_recid, psz_rectype, &pzrec)  );

    SG_ERR_CHECK(  SG_dbrecord__count_pairs(pCtx, pdbrec, &count)  );
    for (i=0; i<count; i++)
    {
		const char* psz_Name;
		const char* psz_Value;

		SG_ERR_CHECK(  SG_dbrecord__get_nth_pair(pCtx, pdbrec, i, &psz_Name, &psz_Value)  );

        if (0 == strcmp(psz_Name, SG_ZING_FIELD__RECID))
        {
        }
        else if (0 == strcmp(psz_Name, SG_ZING_FIELD__RECTYPE))
        {
        }
        else
        {
            SG_ERR_CHECK(  SG_rbtree__add__with_pooled_sz(pCtx, pzrec->prb_field_values, psz_Name, psz_Value)  );
        }
    }

    *ppzrec = pzrec;
    pzrec = NULL;

    return;

fail:
    return;
}

void SG_zingtx__add_parent(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_hid_cs_parent
        )
{
    SG_NULLARGCHECK_RETURN(psz_hid_cs_parent);
    SG_ASSERT(psz_hid_cs_parent);

    SG_ERR_CHECK_RETURN(  SG_rbtree__add(pCtx, pztx->prb_parents, psz_hid_cs_parent)  );
}

void sg_zingtx__add__new_from_vhash(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        SG_vhash* pvh
        )
{
    SG_zingrecord* pzrec = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, psz_rectype, &pzrec)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
    for (i=0; i<count; i++)
    {
		const char* psz_Name;
		const char* psz_Value;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh, i, &psz_Name, &psz_Value)  );

        if (0 == strcmp(psz_Name, SG_ZING_FIELD__RECID))
        {
        }
        else if (0 == strcmp(psz_Name, SG_ZING_FIELD__RECTYPE))
        {
        }
        else
        {
            SG_ERR_CHECK(  SG_rbtree__add__with_pooled_sz(pCtx, pzrec->prb_field_values, psz_Name, psz_Value)  );
        }
    }

    pzrec->b_dirty_fields = SG_TRUE;
    pzrec = NULL;

    // fall thru

fail:
    SG_ZINGRECORD_NULLFREE(pCtx, pzrec);
}

void sg_zingtx__add__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_dbrecord* pdbrec
        )
{
    SG_zingrecord* pzrec = NULL;
    const char* psz_recid = NULL;		// This is a GID

    SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec, SG_ZING_FIELD__RECID, &psz_recid)  );
    SG_ERR_CHECK(  SG_zingrecord__alloc__from_dbrecord(pCtx, pztx, pdbrec, SG_FALSE, &pzrec)  );
    pzrec->b_dirty_fields = SG_TRUE;
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_records, psz_recid, pzrec)  );
    pzrec = NULL;

    // fall thru

fail:
    SG_ZINGRECORD_NULLFREE(pCtx, pzrec);
}

void sg_zingtx__delete__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_dbrecord* pdbrec
        )
{
    SG_zingrecord* pzrec = NULL;
    const char* psz_recid = NULL;		// This is a GID

    SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec, SG_ZING_FIELD__RECID, &psz_recid)  );
    SG_ERR_CHECK(  SG_zingrecord__alloc__from_dbrecord(pCtx, pztx, pdbrec, SG_TRUE, &pzrec)  );
    pzrec->bDeleteMe = SG_TRUE;
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_records, psz_recid, pzrec)  );
    pzrec = NULL;

    // fall thru

fail:
    SG_ZINGRECORD_NULLFREE(pCtx, pzrec);
}

void sg_zingtx__mod__from_dbrecord(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_original_hid,
        SG_dbrecord* pdbrec_add
        )
{
    SG_zingrecord* pzrec = NULL;
    const char* psz_recid = NULL;		// This is a GID

    // TODO disallow mod if no_recid

    SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec_add, SG_ZING_FIELD__RECID, &psz_recid)  );
    SG_ERR_CHECK(  SG_zingrecord__alloc__from_dbrecord(pCtx, pztx, pdbrec_add, SG_FALSE, &pzrec)  );
    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_original_hid, &pzrec->psz_original_hid)  );
    pzrec->b_dirty_fields = SG_TRUE;
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_records, psz_recid, pzrec)  );
    pzrec = NULL;

    // fall thru

fail:
    SG_ZINGRECORD_NULLFREE(pCtx, pzrec);
}

void SG_zingtx__get_record__if_exists(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid,			// This is a GID.
        SG_bool* pb_exists,
        SG_zingrecord** ppzrec
        )
{
    SG_bool b;
    SG_zingrecord* pzrec = NULL;
    SG_dbrecord* pdbrec = NULL;
    const char* psz_hid_rec = NULL;
    SG_varray* pva_crit = NULL;
    SG_varray* pva_fields = NULL;
    SG_varray* pva = NULL;
    SG_uint32 count_results = 0;

    if (SG_DAGNUM__HAS_NO_RECID(pztx->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pztx->prb_records, psz_recid, &b, (void**) &pzrec)  );
    if (b)
    {
        if (pzrec->bDeleteMe)
        {
            *pb_exists = SG_FALSE;
            return;
        }

        *ppzrec = pzrec;
        *pb_exists = SG_TRUE;
        return;
    }

    // look up the recid (a gid) to find the current hid
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_recid)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HIDREC)  );
    // TODO shouldn't this use query__one?
    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pztx->pRepo, pztx->iDagNum, pztx->psz_csid, psz_rectype, pva_crit, NULL, 0, 0, pva_fields, &pva)  );
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    if (!pva)
    {
        *pb_exists = SG_FALSE;
        goto fail;
    }

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_results)  );
    if(count_results==0)
    {
        *pb_exists = SG_FALSE;
        goto fail;
    }
    SG_ASSERT(count_results==1);

    // load the dbrecord from the repo
    SG_ERR_CHECK(  SG_vaofvh__get__sz(pCtx, pva, 0, SG_ZING_FIELD__HIDREC, &psz_hid_rec)  );
    SG_ERR_CHECK(  SG_dbrecord__load_from_repo(pCtx, pztx->pRepo, psz_hid_rec, &pdbrec)  );
    SG_VARRAY_NULLFREE(pCtx, pva);

    SG_ERR_CHECK(  SG_zingrecord__alloc__from_dbrecord(pCtx, pztx, pdbrec, SG_TRUE, &pzrec)  );
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_records, psz_recid, pzrec)  );

    *ppzrec = pzrec;
    *pb_exists = SG_TRUE;
    pzrec = NULL;

fail:
    SG_DBRECORD_NULLFREE(pCtx, pdbrec);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void SG_zingtx__get_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid,			// This is a GID.
        SG_zingrecord** ppzrec
        )
{
    SG_bool b_exists = SG_FALSE;
    SG_zingrecord* pzrec = NULL;

	SG_NULLARGCHECK_RETURN(psz_recid);

    SG_ERR_CHECK_RETURN(  SG_zingtx__get_record__if_exists(pCtx, pztx, psz_rectype, psz_recid, &b_exists, &pzrec)  );
    if (b_exists)
    {
        *ppzrec = pzrec;
    }
    else
    {
        SG_ERR_THROW2_RETURN(SG_ERR_ZING_RECORD_NOT_FOUND,
                (pCtx, "%s = %s", SG_ZING_FIELD__RECID, psz_recid)
                );
    }
}

void SG_zingrecord__get_rectype(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        const char** ppsz_rectype
        )
{
	SG_NULLARGCHECK_RETURN(pzrec);
	SG_NULLARGCHECK_RETURN(ppsz_rectype);

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pzrec->pztx->iDagNum))
    {
        SG_ERR_CHECK_RETURN(  sg_zingtemplate__get_only_rectype(pCtx, pzrec->pztx->ptemplate, ppsz_rectype, NULL)  );
    }
    else
    {
        SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pzrec->prb_field_values, SG_ZING_FIELD__RECTYPE, NULL, (void**) ppsz_rectype)  );
    }
}

void SG_zingrecord__get_history(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_varray** ppva
        )
{
	SG_NULLARGCHECK_RETURN(pzrec);
	SG_NULLARGCHECK_RETURN(ppva);

    if (SG_DAGNUM__HAS_NO_RECID(pzrec->pztx->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    if (!pzrec->pva_history)
    {
        const char* psz_recid = NULL;
        const char* psz_rectype = NULL;
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzrec, &psz_recid)  );
        SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzrec, &psz_rectype)  );
        SG_ERR_CHECK(  SG_repo__dbndx__query_record_history(pCtx, pzrec->pztx->pRepo, pzrec->pztx->iDagNum, psz_recid, psz_rectype, &pzrec->pva_history)  );
    }

    *ppva = pzrec->pva_history;

fail:
    ;
}

void SG_zingrecord__get_recid(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        const char** ppsz_recid			// This is a GID.
        )
{
	SG_NULLARGCHECK_RETURN(pzrec);

    if (SG_DAGNUM__HAS_NO_RECID(pzrec->pztx->iDagNum))
    {
        SG_ERR_THROW_RETURN(  SG_ERR_ZING_NO_RECID  );
    }

    SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pzrec->prb_field_values, SG_ZING_FIELD__RECID, NULL, (void**) ppsz_recid)  );
}

void SG_zingrecord__get_tx(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_zingtx** pptx
        )
{
  SG_UNUSED(pCtx);

    *pptx = pzrec->pztx;
}

void SG_zingrecord__to_dbrecord(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_dbrecord** ppResult
        )
{
    SG_dbrecord* pdbrec = NULL;
	SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_field_name = NULL;
    const char* psz_field_val = NULL;

    SG_ERR_CHECK(  SG_DBRECORD__ALLOC(pCtx, &pdbrec)  );

    // copy all the fields into the dbrecord
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pzrec->prb_field_values, &b, &psz_field_name, (void**) &psz_field_val)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_field_val)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_field_name, (void**) &psz_field_val)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    *ppResult = pdbrec;
    pdbrec = NULL;

    return;

fail:
    SG_DBRECORD_NULLFREE(pCtx, pdbrec);
}

void sg_zingfieldattributes__get_type(
	SG_context *pCtx,
	SG_zingfieldattributes *pzfa,
	SG_uint16 *pType)
{
	SG_NULLARGCHECK_RETURN(pzfa);
	SG_NULLARGCHECK_RETURN(pType);

	*pType = pzfa->type;
}

void SG_zingrecord__remove_field(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa
        )
{
    SG_zingtx* pztx = NULL;

    if (pzfa->required)
    {
        SG_ERR_THROW2(  SG_ERR_ZING_CONSTRAINT,
                (pCtx, "Field '%s' is required", pzfa->name)
                );
    }

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, NULL)  );

    // fall thru

fail:
    return;
}

static void sg_zingtx__change_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingrecord* pzrec,
        const char* psz_name,
        const char* psz_val
        )
{
    SG_UNUSED(pztx);

    if (psz_val)
    {
        SG_ERR_CHECK(  SG_rbtree__update__with_pooled_sz(pCtx, pzrec->prb_field_values, psz_name, psz_val)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pzrec->prb_field_values, psz_name)  );
    }

    pzrec->b_dirty_fields = SG_TRUE;

fail:
    return;
}

void SG_zingrecord__get_field__int(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64* pval
        )
{
    const char* psz_val = NULL;
    SG_int64 i = 0;

    if (SG_ZING_TYPE__INT != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i, psz_val)  );

    *pval = i;

    return;

fail:
    return;
}

void SG_zingrecord__get_field__datetime(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64* pval
        )
{
    const char* psz_val = NULL;
    SG_int64 i = 0;

    if (SG_ZING_TYPE__DATETIME != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i, psz_val)  );

    *pval = i;

    return;

fail:
    return;
}

void SG_zingrecord__get_field__bool(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_bool* pval
        )
{
    const char* psz_val = NULL;
    SG_int64 i = 0;

    if (SG_ZING_TYPE__BOOL != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i, psz_val)  );
    *pval = (i != 0);

    return;

fail:
    return;
}

void SG_zingrecord__get_field__attachment(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** pval
        )
{
    const char* psz_val = NULL;

    if (SG_ZING_TYPE__ATTACHMENT != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    *pval = psz_val;

    return;

fail:
    return;
}

void SG_zingrecord__get_field__string(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** pval
        )
{
    const char* psz_val = NULL;

    if (SG_ZING_TYPE__STRING != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    *pval = psz_val;

    return;

fail:
    return;
}

void SG_zingrecord__get_field__userid(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** pval
        )
{
    const char* psz_val = NULL;

    if (SG_ZING_TYPE__USERID != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    *pval = psz_val;

    return;

fail:
    return;
}

void SG_zingrecord__get_field__reference(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char** pval
        )
{
    const char* psz_val = NULL;

    if (SG_ZING_TYPE__REFERENCE != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzr->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );
    *pval = psz_val;

    return;

fail:
    return;
}

void sg_zing__add_error_object__idarray(
        SG_context* pCtx,
        SG_varray* pva_violations,
        const char* psz_name_array,
        SG_rbtree* prb_ids,
        ...
        )
{
    va_list ap;
    SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;
    SG_varray* pva = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

	SG_ERR_CHECK(  SG_rbtree__to_varray__keys_only(pCtx, prb_ids, &pva)  );
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh, psz_name_array, &pva)  );

    va_start(ap, prb_ids);
    do
    {
        const char* psz_key = va_arg(ap, const char*);
        const char* psz_val = NULL;

        if (!psz_key)
        {
            break;
        }

        psz_val = va_arg(ap, const char*);
        SG_ASSERT(psz_val);

        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, psz_key, psz_val)  );
    } while (1);

    if (pva_violations)
    {
        SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva_violations, &pvh)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
        SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );
        SG_ERR_THROW2(  SG_ERR_ZING_CONSTRAINT,
                (pCtx, "%s", SG_string__sz(pstr))
                );
    }

fail:
    va_end(ap);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void sg_zing__add_error_object(
        SG_context* pCtx,
        SG_varray* pva_violations,
        ...
        )
{
    va_list ap;
    SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

    va_start(ap, pva_violations);
    do
    {
        const char* psz_key = va_arg(ap, const char*);
        const char* psz_val = NULL;

        if (!psz_key)
        {
            break;
        }

        psz_val = va_arg(ap, const char*);

		/* This is called with a null value in at least one circumstance:
		 * a maxlength violation on a comment. The comment DAG has no recid, 
		 * so we get a null recid argument here. */
		if (psz_val)
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, psz_key, psz_val)  );
		else
			SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh, psz_key)  );

    } while (1);

    if (pva_violations)
    {
        SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva_violations, &pvh)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
        SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );
        SG_ERR_THROW2(  SG_ERR_ZING_CONSTRAINT,
                (pCtx, "%s", SG_string__sz(pstr))
                );
    }

fail:
    va_end(ap);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

static void sg_zing__check_field__bool(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        SG_bool val,
        SG_varray* pva_violations
        )
{
  SG_UNUSED(pCtx);
  SG_UNUSED(pzfa);
  SG_UNUSED(psz_recid);
  SG_UNUSED(val);
  SG_UNUSED(pva_violations);
}

static void sg_zing__check_field__attachment(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        const SG_pathname* pPath,
        SG_varray* pva_violations
        )
{
    SG_uint64 len = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len, NULL)  );

    // check maxlength
    if (pzfa->v._attachment.has_maxlength && (len > (SG_uint32) pzfa->v._attachment.val_maxlength))
    {
        SG_int_to_string_buffer sz_i;
        SG_int64_to_sz(pzfa->v._attachment.val_maxlength, sz_i);

        // TODO this check_field function isn't going to get used in the
        // same way as the others.
        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "maxlength",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                    NULL) );
    }

fail:
    return;
}

static void sg_zing__check_field__attachment__buflen(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        const SG_byte* pBytes,
        SG_uint32 len,
        SG_varray* pva_violations
        )
{
    SG_UNUSED(pBytes);

    // check maxlength
    if (pzfa->v._attachment.has_maxlength && (len > (SG_uint32) pzfa->v._attachment.val_maxlength))
    {
        SG_int_to_string_buffer sz_i;
        SG_int64_to_sz(pzfa->v._attachment.val_maxlength, sz_i);

        // TODO this check_field function isn't going to get used in the
        // same way as the others.
        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "maxlength",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                    NULL) );
    }

fail:
    return;
}

static void sg_zing__check_field__string(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        const char* psz_val,
        SG_varray* pva_violations
        )
{
    SG_ERR_CHECK(  SG_utf8__validate__sz(pCtx, (const unsigned char*) psz_val)  );

    // check minlength
    if (pzfa->v._string.has_minlength && ((SG_int64)strlen(psz_val) < pzfa->v._string.val_minlength))
    {
        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "minlength",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, psz_val,
                    NULL) );
    }

    // check maxlength
    if (pzfa->v._string.has_maxlength && ((SG_int64)strlen(psz_val) > pzfa->v._string.val_maxlength))
    {
        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "maxlength",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, psz_val,
                    NULL) );
    }

    if (pzfa->v._string.prohibited)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_stringarray__count(pCtx, pzfa->v._string.prohibited, &count )  );
        for (i=0; i<count; i++)
        {
            const char* psz_thisval = NULL;

            SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pzfa->v._string.prohibited, i, &psz_thisval)  );
            if (0 == strcmp(psz_thisval, psz_val))
            {
                SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                            SG_ZING_CONSTRAINT_VIOLATION__TYPE, "prohibited",
                            SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                            SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                            SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, psz_val,
                            NULL) );
            }
        }
    }

    if (pzfa->v._string.allowed)
    {
        SG_bool b_found = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pzfa->v._string.allowed, psz_val, &b_found)  );
        if (!b_found)
        {
            SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                        SG_ZING_CONSTRAINT_VIOLATION__TYPE, "allowed",
                        SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                        SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                        SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, psz_val,
                        NULL) );
        }
    }

fail:
    return;
}

static void sg_zing__check_field__reference(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        const char* psz_id,
        SG_varray* pva_violations
        )
{
  SG_UNUSED(pCtx);
  SG_UNUSED(pzfa);
  SG_UNUSED(psz_recid);
  SG_UNUSED(psz_id);
  SG_UNUSED(pva_violations);

    // TODO verify that the given recid looks right
    //
    // TODO verify that it actually exists
    //
    // TODO verify that it is a valid rectype for this field
}

static void sg_zing__check_field__userid(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        const char* psz_id,
        SG_varray* pva_violations
        )
{
  SG_UNUSED(pCtx);
  SG_UNUSED(pzfa);
  SG_UNUSED(psz_recid);
  SG_UNUSED(psz_id);
  SG_UNUSED(pva_violations);

    // TODO verify that the user id looks right
    //
    // TODO verify that it actually exists?
}

static void sg_zing__check_field__datetime(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        SG_int64 val,
        SG_varray* pva_violations
        )
{
    // check min
    if (pzfa->v._datetime.has_min && (val < pzfa->v._datetime.val_min))
    {
        SG_int_to_string_buffer sz_i;
        SG_int64_to_sz(pzfa->v._datetime.val_min, sz_i);

        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "min",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                    NULL) );
    }

    // check max
    if (pzfa->v._datetime.has_max && (val > pzfa->v._datetime.val_max))
    {
        SG_int_to_string_buffer sz_i;
        SG_int64_to_sz(pzfa->v._datetime.val_max, sz_i);

        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "max",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                    NULL) );
    }

fail:
    return;
}

static void sg_zing__check_field__int(
        SG_context* pCtx,
        SG_zingfieldattributes* pzfa,
        const char* psz_recid,
        SG_int64 val,
        SG_varray* pva_violations
        )
{
    // check min
    if (pzfa->v._int.has_min && (val < pzfa->v._int.val_min))
    {
        SG_int_to_string_buffer sz_i;
        SG_int64_to_sz(pzfa->v._int.val_min, sz_i);

        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "min",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                    NULL) );
    }

    // check max
    if (pzfa->v._int.has_max && (val > pzfa->v._int.val_max))
    {
        SG_int_to_string_buffer sz_i;
        SG_int64_to_sz(pzfa->v._int.val_max, sz_i);

        SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "max",
                    SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                    NULL) );
    }

    if (pzfa->v._int.prohibited)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vector_i64__length(pCtx, pzfa->v._int.prohibited, &count)  );
        for (i=0; i<count; i++)
        {
            SG_int64 thisval = 0;

            SG_ERR_CHECK(  SG_vector_i64__get(pCtx, pzfa->v._int.prohibited, i, &thisval)  );
            if (thisval == val)
            {
                SG_int_to_string_buffer sz_i;
                SG_int64_to_sz(val, sz_i);

                SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                            SG_ZING_CONSTRAINT_VIOLATION__TYPE, "prohibited",
                            SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                            SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                            SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                            NULL) );
            }
        }
    }

    if (pzfa->v._int.allowed)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;
        SG_bool b_found_one = SG_FALSE;


        SG_ERR_CHECK(  SG_vector_i64__length(pCtx, pzfa->v._int.allowed, &count)  );
        for (i=0; i<count; i++)
        {
            SG_int64 thisval = 0;

            SG_ERR_CHECK(  SG_vector_i64__get(pCtx, pzfa->v._int.allowed, i, &thisval)  );
            if (thisval == val)
            {
                b_found_one = SG_TRUE;
                break;
            }
        }
        if (!b_found_one)
        {
            SG_int_to_string_buffer sz_i;
            SG_int64_to_sz(val, sz_i);

            SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                        SG_ZING_CONSTRAINT_VIOLATION__TYPE, "allowed",
                        SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                        SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                        SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, sz_i,
                        NULL) );
        }
    }

    // call validation hook?  used to make sure only prime numbers?
fail:
    return;
}

void SG_zingrecord__set_field__bool(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_bool val
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__BOOL != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__bool(pCtx, pzfa, psz_recid, val, NULL)  );

    // set the value
    if (val)
    {
        SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, "1")  );
    }
    else
    {
        SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, "0")  );
    }

fail:
    return;
}

void SG_zingrecord__set_field__attachment__buflen(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const SG_byte* pBytes,
        SG_uint32 len
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;
    struct sg_attachment* patt = NULL;
    struct sg_attachment* p_old_att = NULL;

    SG_NULLARGCHECK_RETURN(pBytes);

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__ATTACHMENT != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__attachment__buflen(pCtx, pzfa, psz_recid, pBytes, len, NULL)  );

    // store the data, and this recid, and the field name, in the tx.
    // on commit, store the attachment in the repo, thus getting its HID,
    // and then put that HID into the value of this field.
    if (!pzr->prb_attachments)
    {
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pzr->prb_attachments)  );
    }
	SG_ERR_CHECK(  SG_alloc1(pCtx, patt)  );
	SG_ERR_CHECK(  SG_allocN(pCtx,len,patt->pBytes)  );
    memcpy(patt->pBytes, pBytes, len);
    patt->len = len;

    SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pzr->prb_attachments, pzfa->name, (void**) patt, (void**) &p_old_att)  );

    patt = NULL;
    if (p_old_att)
    {
        x_free_sg_attachment(pCtx, p_old_att);
        p_old_att = NULL;
    }

    SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, "to be replaced at commit time with attachment hid")  );

    // fall thru

fail:
    if (patt)
    {
        x_free_sg_attachment(pCtx, patt);
    }
}

void SG_zingrecord__set_field__attachment__pathname(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_pathname** ppPath
        )
{
    SG_zingtx* pztx = NULL;
	SG_fsobj_type type = SG_FSOBJ_TYPE__UNSPECIFIED;
	SG_fsobj_perms perms;
	SG_bool bExists = SG_FALSE;
    SG_pathname* pPath = NULL;
    const char* psz_recid = NULL;
    struct sg_attachment* patt = NULL;
    struct sg_attachment* p_old_att = NULL;

    SG_NULLARGCHECK_RETURN(ppPath);
    pPath = *ppPath;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__ATTACHMENT != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, &type, &perms)  );

	if (!bExists)
	{
        SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
                (pCtx, "Path %s does not exist", SG_pathname__sz(pPath))
                );
	}

    if (SG_FSOBJ_TYPE__REGULAR != type)
    {
        SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
                (pCtx, "Path %s is not a regular file", SG_pathname__sz(pPath))
                );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__attachment(pCtx, pzfa, psz_recid, pPath, NULL)  );

    // store the pathname, and this recid, and the field name, in the tx.
    // on commit, store the attachment in the repo, thus getting its HID,
    // and then put that HID into the value of this field.
    if (!pzr->prb_attachments)
    {
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pzr->prb_attachments)  );
    }
	SG_ERR_CHECK(  SG_alloc1(pCtx, patt)  );
    patt->pPath = pPath;

    SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pzr->prb_attachments, pzfa->name, (void**) patt, (void**) &p_old_att)  );
    patt = NULL;
    // we own the pPath pointer now
    *ppPath = NULL;
    if (p_old_att)
    {
        x_free_sg_attachment(pCtx, p_old_att);
        p_old_att = NULL;
    }

    SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, "to be replaced at commit time with attachment hid")  );

    // fall thru

fail:
    if (patt)
    {
        x_free_sg_attachment(pCtx, patt);
    }
}

void SG_zingrecord__set_field__string(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        )
{
    // TODO the following line passes NULL for pva_violations, which
    // the called routine cannot handle
	SG_ERR_CHECK_RETURN(  SG_zingrecord__set_field__string_checked(pCtx, pzr, pzfa, psz_val, NULL)  );
}

void SG_zingrecord__set_field__string_checked(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val,
		SG_varray *pva_violations
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__STRING != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__string(pCtx, pzfa, psz_recid, psz_val, pva_violations)  );

    // set the value
    SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, psz_val)  );

    return;

fail:
    return;
}

void SG_zingtx__run_defaultfunc__string(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingfieldattributes* pzfa,
        SG_string** ppstr
        )
{
    SG_string* pstr = NULL;

    if (0 == strcmp(pzfa->v._string.defaultfunc, "gen_random_unique"))
    {
        SG_ERR_CHECK(  sg_zing__gen_random_unique_string(pCtx, pztx, pzfa, &pstr)  );
    }
    else if (0 == strcmp(pzfa->v._string.defaultfunc, "gen_userprefix_unique"))
    {
        SG_ERR_CHECK(  sg_zing__gen_userprefix_unique_string(
                    pCtx, 
                    pztx, 
                    pzfa->rectype, 
                    pzfa->name, 
                    (SG_uint32) (pzfa->v._string.num_digits ? pzfa->v._string.num_digits : 4),
                    NULL, 
                    &pstr)  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_zingrecord__do_defaultfunc__string(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa
        )
{
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_zingtx__run_defaultfunc__string(pCtx, pzr->pztx, pzfa, &pstr)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, pzr, pzfa, SG_string__sz(pstr))  );

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_zingrecord__set_field__reference(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__REFERENCE != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__reference(pCtx, pzfa, psz_recid, psz_val, NULL)  );

    // set the value
    SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, psz_val)  );

    return;

fail:
    return;
}

void SG_zingrecord__set_field__userid(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        const char* psz_val
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__USERID != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__userid(pCtx, pzfa, psz_recid, psz_val, NULL)  );

    // set the value
    SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, psz_val)  );

    return;

fail:
    return;
}

void SG_zingrecord__lookup_name(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        const char* psz_name,
        SG_zingfieldattributes** ppzfa
        )
{
    const char* psz_rectype = NULL;
    SG_zingtx* pztx = NULL;
    SG_zingtemplate* pztemplate = NULL;
    SG_zingfieldattributes* pzfa = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    // ask the tx for its template
    SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pztemplate)  );

    // get the rectype
    SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzr, &psz_rectype)  );

    // now see if it's a field
    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pztemplate, psz_rectype, psz_name, &pzfa)  );

    *ppzfa = pzfa;
    pzfa = NULL;

    return;

fail:
    return;
}

void SG_zingrecord__set_field__int(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64 val
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__INT != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__int(pCtx, pzfa, psz_recid, val, NULL)  );

    // set the value
	{
		SG_int_to_string_buffer buf;

		SG_int64_to_sz(val, buf);

		SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, buf)  );
	}

    return;

fail:
    return;
}

void SG_zingrecord__set_field__datetime(
        SG_context* pCtx,
        SG_zingrecord* pzr,
        SG_zingfieldattributes* pzfa,
        SG_int64 val
        )
{
    SG_zingtx* pztx = NULL;
    const char* psz_recid = NULL;

    // ask the record for its tx
    SG_ERR_CHECK(  SG_zingrecord__get_tx(pCtx, pzr, &pztx)  );

    if (SG_ZING_TYPE__DATETIME != pzfa->type)
    {
        SG_ERR_THROW(  SG_ERR_ZING_TYPE_MISMATCH  );
    }

    if (SG_DAGNUM__HAS_NO_RECID(pzr->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzr, &psz_recid)  );
    }
    SG_ERR_CHECK(  sg_zing__check_field__datetime(pCtx, pzfa, psz_recid, val, NULL)  );

    // set the value
	{
		SG_int_to_string_buffer buf;

		SG_int64_to_sz(val, buf);

		SG_ERR_CHECK(  sg_zingtx__change_record(pCtx, pztx, pzr, pzfa->name, buf)  );
	}

    return;

fail:
    return;
}

void SG_zingrecord__free(SG_context* pCtx, SG_zingrecord* pThis)
{
    if (!pThis)
    {
        return;
    }

    SG_VARRAY_NULLFREE(pCtx, pThis->pva_history);
    SG_RBTREE_NULLFREE(pCtx, pThis->prb_field_values);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pThis->prb_attachments, (SG_free_callback*) x_free_sg_attachment);
    SG_NULLFREE(pCtx, pThis->psz_original_hid);
    SG_NULLFREE(pCtx, pThis);
}

static void x_zingtx__create_new_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid,
        SG_zingrecord** pprec
        )
{
    SG_zingrecord* prec = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_stringarray* psa_fields = NULL;
    SG_zingtemplate* pztemplate = NULL;
    SG_zingfieldattributes* pzfa = NULL;

    // ask the tx for its template
    SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pztemplate)  );

    if (pztx->b_in_commit)
    {
        SG_ERR_THROW(  SG_ERR_ZING_ILLEGAL_DURING_COMMIT  );
    }

	if (! pztemplate)
	{
		SG_ERR_THROW(  SG_ERR_ZING_TEMPLATE_NOT_FOUND  );
	}

    if (SG_DAGNUM__HAS_NO_RECID(pztx->iDagNum))
    {
        SG_ERR_CHECK(  SG_zingrecord__alloc(pCtx, pztx, NULL, NULL, psz_rectype, &prec)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__alloc(pCtx, pztx, NULL, psz_recid, psz_rectype, &prec)  );
    }

    prec->b_dirty_fields = SG_TRUE;

    // init defaults by getting the field list and enumerating

    SG_ERR_CHECK(  SG_zingtemplate__list_fields(pCtx, pztemplate, psz_rectype, &psa_fields)  );
    SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_fields, &count )  );
    for (i=0; i<count; i++)
    {
        const char* psz_field_name = NULL;

        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_fields, i, &psz_field_name)  );
        SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pztemplate, psz_rectype, psz_field_name, &pzfa)  );

        switch (pzfa->type)
        {
            case SG_ZING_TYPE__BOOL:
                {
                    if (pzfa->v._bool.has_defaultvalue)
                    {
                        SG_ERR_CHECK(  SG_zingrecord__set_field__bool(pCtx, prec, pzfa, pzfa->v._bool.defaultvalue)  );
                    }
                    break;
                }
            case SG_ZING_TYPE__DATETIME:
                {
                    if (pzfa->v._datetime.has_defaultvalue)
                    {
                        if (0 == strcmp("now", pzfa->v._datetime.defaultvalue))
                        {
                            SG_int64 t = 0;

                            SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t)  );
                            SG_ERR_CHECK(  SG_zingrecord__set_field__datetime(pCtx, prec, pzfa, t)  );
                        }
                        else
                        {
                            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
                        }
                    }
                    break;
                }
            case SG_ZING_TYPE__INT:
                {
                    if (pzfa->v._int.has_defaultvalue)
                    {
                        SG_ERR_CHECK(  SG_zingrecord__set_field__int(pCtx, prec, pzfa, pzfa->v._int.defaultvalue)  );
                    }
                    break;
                }
            case SG_ZING_TYPE__STRING:
                {
                    if (pzfa->v._string.has_defaultvalue)
                    {
                        SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, pzfa->v._string.defaultvalue)  );
                    }
                    else if (pzfa->v._string.has_defaultfunc)
                    {
                        SG_ERR_CHECK(  SG_zingrecord__do_defaultfunc__string(pCtx, prec, pzfa)  );
                    }
                    break;
                }
            case SG_ZING_TYPE__USERID:
                {
                    if (pzfa->v._userid.has_defaultvalue)
                    {
                        // TODO this should be defaultfunc, not defaultvalue
                        if (0 == strcmp("whoami", pzfa->v._userid.defaultvalue))
                        {
                            SG_ERR_CHECK(  SG_zingrecord__set_field__userid(pCtx, prec, pzfa, pztx->buf_who)  );
                        }
                        else
                        {
                            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
                        }
                    }
                    break;
                }
        }
    }

    SG_STRINGARRAY_NULLFREE(pCtx, psa_fields);

    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_records, psz_recid, prec)  );

    *pprec = prec;
    prec = NULL;

    return;

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa_fields);
    SG_ZINGRECORD_NULLFREE(pCtx, prec);
}

void SG_zingtx__create_new_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        SG_zingrecord** pprec
        )
{
	char buf_recid[SG_GID_BUFFER_LENGTH];
    //
    // we always generate a recid, since it is used as the key to
    // prb_records.  but if the rectype isn't supposed to have
    // a recid, we don't add one.

    SG_ERR_CHECK(  SG_gid__generate(pCtx, buf_recid, sizeof(buf_recid))  );

    SG_ERR_CHECK(  x_zingtx__create_new_record(pCtx, pztx, psz_rectype, buf_recid, pprec)  );

fail:
    ;
}

void SG_zingtx__create_new_record__force_recid(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid,
        SG_zingrecord** pprec
        )
{
    SG_ERR_CHECK_RETURN(  x_zingtx__create_new_record(pCtx, pztx, psz_rectype, psz_recid, pprec)  );
}

void SG_zingtx__delete_record__hid(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_hidrec
        )
{
    SG_vhash* pvh_rec = NULL;
    SG_varray* pva_fields = NULL;

    if (!SG_DAGNUM__HAS_NO_RECID(pztx->iDagNum))
    {
        // this function only works for trivial merge dags
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
    SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pztx->pRepo, pztx->iDagNum, pztx->psz_csid, psz_rectype, "hidrec", psz_hidrec, pva_fields, &pvh_rec)  );
    if (!pvh_rec)
    {
        SG_ERR_THROW(  SG_ERR_ZING_RECORD_NOT_FOUND  );
    }

    SG_ERR_CHECK(  SG_rbtree__update(pCtx, pztx->prb_delete_hidrec, psz_hidrec)  );

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VHASH_NULLFREE(pCtx, pvh_rec);
}

void SG_zingtx__delete_record(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_recid
        )
{
    SG_zingrecord* pzrec = NULL;

    if (SG_DAGNUM__HAS_NO_RECID(pztx->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    if (pztx->b_in_commit)
    {
        SG_ERR_THROW(  SG_ERR_ZING_ILLEGAL_DURING_COMMIT  );
    }

    SG_ERR_CHECK(  SG_zingtx__get_record(pCtx, pztx, psz_rectype, psz_recid, &pzrec)  );
    pzrec->bDeleteMe = SG_TRUE;

    return;

fail:
    ;
}

void SG_zingrecord__is_delete_me(
        SG_context* pCtx,
        SG_zingrecord* pzrec,
        SG_bool* pb
        )
{
  SG_UNUSED(pCtx);

    *pb = pzrec->bDeleteMe;
}

static void sg_zingtx__do_one_unique_check(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_val,
        SG_varray* pva_violations
        )
{
    SG_varray* pva = NULL;
    SG_varray* pva_crit = NULL;
    SG_varray* pva_fields = NULL;
	SG_rbtree_iterator* pit = NULL;
	SG_bool b = SG_FALSE;
    const char* psz_recid_other = NULL;
    const char* psz_rectype_other = NULL;
    const char* psz_val_other = NULL;
    SG_zingrecord* pzrec_other = NULL;
    SG_rbtree* prb_matches = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_matches)  );

    /* construct the crit */
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_field_name)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_val)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pztx->pRepo, pztx->iDagNum, pztx->psz_csid, psz_rectype, pva_crit, NULL, 0, 0, pva_fields, &pva)  );
    SG_VARRAY_NULLFREE(pCtx, pva_crit);

    if (pva)
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
        for (i=0; i<count; i++)
        {
            SG_bool b_found = SG_FALSE;

            SG_ERR_CHECK(  SG_vaofvh__get__sz(pCtx, pva, 0, NULL, &psz_recid_other)  );

            SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pztx->prb_records, psz_recid_other, &b_found, (void**) &pzrec_other)  );
            if (!b_found)
            {
                SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb_matches, psz_recid_other)  );
            }
        }
        SG_VARRAY_NULLFREE(pCtx, pva);
    }

    // now iterate over all dirty records
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_records, &b, &psz_recid_other, (void**) &pzrec_other)  );
    while (b)
    {
        SG_bool bDeleteMe = SG_FALSE;

        SG_ERR_CHECK(  SG_zingrecord__is_delete_me(pCtx, pzrec_other, &bDeleteMe)  );

        if (bDeleteMe)
        {
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_matches, psz_recid_other, &b_has, NULL)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_rbtree__remove(pCtx, prb_matches, psz_recid_other)  );
            }
        }
        else
        {
            if (pzrec_other->b_dirty_fields)
            {
                SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzrec_other, &psz_rectype_other)  );
                if (0 == strcmp(psz_rectype, psz_rectype_other))
                {
                    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzrec_other->prb_field_values, psz_field_name, NULL, (void**) &psz_val_other)  );
                    if (0 == strcmp(psz_val_other, psz_val))
                    {
                        SG_ERR_CHECK(  SG_rbtree__update(pCtx, prb_matches, psz_recid_other)  );
                    }
                }
            }
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid_other, (void**) &pzrec_other)  );
    }

    SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_matches, &count)  );
    SG_ASSERT(count != 0);
    if (count > 1)
    {
        SG_ERR_CHECK(  sg_zing__add_error_object__idarray(pCtx, pva_violations,
                    SG_ZING_CONSTRAINT_VIOLATION__IDS, prb_matches,
                    SG_ZING_CONSTRAINT_VIOLATION__TYPE, "unique",
                    SG_ZING_CONSTRAINT_VIOLATION__RECTYPE, psz_rectype,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, psz_field_name,
                    SG_ZING_CONSTRAINT_VIOLATION__FIELD_VALUE, psz_val,
                    NULL) );
    }

    // fall thru

fail:
    SG_RBTREE_NULLFREE(pCtx, prb_matches);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

static void sg_zingtx__do_unique_checks(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_vhash* pvh_unique_checks,
        SG_varray* pva_violations
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_unique_checks, &count)  );
    for (i=0; i<count; i++)
    {
		const char* psz_key = NULL;
		const SG_variant* pv = NULL;
        SG_vhash* pvh = NULL;
        const char* psz_rectype = NULL;
        const char* psz_field_name = NULL;
        const char* psz_val = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_unique_checks, i, &psz_key, &pv)  );
        SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "field", &psz_field_name)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "val", &psz_val)  );
        SG_ERR_CHECK(  sg_zingtx__do_one_unique_check(pCtx, pztx, psz_rectype, psz_field_name, psz_val, pva_violations)  );
    }

fail:
    ;
}

static void sg_zingrecord__find_unique_checks_to_be_done(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_zingtemplate* pztemplate,
        SG_zingrecord* pzrec,
        SG_vhash* pvh_unique_checks
        )
{
    SG_stringarray* psa_fields = NULL;
    const char* psz_rectype = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_zingfieldattributes* pzfa = NULL;
    SG_vhash* pvh = NULL;
    SG_string* pstr = NULL;

#ifdef DEBUG
    SG_ASSERT(SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(pztx->iDagNum));
#else
    SG_UNUSED(pztx);
#endif

    SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzrec, &psz_rectype)  );

    SG_ERR_CHECK(  SG_zingtemplate__list_fields(pCtx, pztemplate, psz_rectype, &psa_fields)  );
    SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_fields, &count )  );
    for (i=0; i<count; i++)
    {
        const char* psz_field_name = NULL;

        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_fields, i, &psz_field_name)  );
        SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pztemplate, psz_rectype, psz_field_name, &pzfa)  );

        switch (pzfa->type)
        {
            case SG_ZING_TYPE__STRING:
                {
                    if (pzfa->v._string.unique)
                    {
                        const char* psz_val = NULL;
                        SG_bool b_already = SG_FALSE;

                        SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzrec->prb_field_values, pzfa->name, NULL, (void**) &psz_val)  );

                        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "%s.%s.%s", psz_rectype, psz_field_name, psz_val)  );
                        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_unique_checks, SG_string__sz(pstr), &b_already)  );
                        if (!b_already)
                        {
                            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
                            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_ZING_FIELD__RECTYPE, psz_rectype)  );
                            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "field", psz_field_name)  );
                            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "val", psz_val)  );
                            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_unique_checks, SG_string__sz(pstr), &pvh)  );
                        }
                        SG_STRING_NULLFREE(pCtx, pstr);
                    }
                    break;
                }
        }
    }

    // fall thru

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa_fields);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_zingrecord__check_intrarecord(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        SG_zingrecord* pzrec,
        SG_varray* pva_violations
        )
{
	SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_field_name = NULL;
    const char* psz_field_val = NULL;
    const char* psz_rectype = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    const char* psz_recid = NULL;

    SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzrec, &psz_rectype)  );

    if (SG_DAGNUM__HAS_NO_RECID(pzrec->pztx->iDagNum))
    {
        psz_recid = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzrec, &psz_recid)  );
    }

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pzrec->prb_field_values, &b, &psz_field_name, (void**) &psz_field_val)  );
    while (b)
    {
        if (
                (0 == strcmp(psz_field_name, SG_ZING_FIELD__RECID))
                || (0 == strcmp(psz_field_name, SG_ZING_FIELD__RECTYPE))
           )
        {
            goto continue_loop;
        }

        SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pztemplate, psz_rectype, psz_field_name, &pzfa)  );
        if (!pzfa)
        {
            SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                        SG_ZING_CONSTRAINT_VIOLATION__TYPE, "illegal_field",
                        SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                        SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, psz_field_name,
                        NULL) );

            goto continue_loop;
        }

        switch (pzfa->type)
        {
            case SG_ZING_TYPE__BOOL:
            {
                SG_int64 ival = 0;

                SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &ival, psz_field_val)  );
                SG_ERR_CHECK(  sg_zing__check_field__bool(pCtx, pzfa, psz_recid, ival != 0, pva_violations)  );
            }
            break;

            case SG_ZING_TYPE__DATETIME:
            {
                SG_int64 ival = 0;

                SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &ival, psz_field_val)  );
                SG_ERR_CHECK(  sg_zing__check_field__datetime(pCtx, pzfa, psz_recid, ival, pva_violations)  );
            }
            break;

            case SG_ZING_TYPE__INT:
            {
                SG_int64 ival = 0;

                SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &ival, psz_field_val)  );
                SG_ERR_CHECK(  sg_zing__check_field__int(pCtx, pzfa, psz_recid, ival, pva_violations)  );
            }
            break;

            case SG_ZING_TYPE__STRING:
            {
                SG_ERR_CHECK(  sg_zing__check_field__string(pCtx, pzfa, psz_recid, psz_field_val, pva_violations)  );
            }
            break;

            case SG_ZING_TYPE__USERID:
            {
                SG_ERR_CHECK(  sg_zing__check_field__userid(pCtx, pzfa, psz_recid, psz_field_val, pva_violations)  );
            }
            break;

            case SG_ZING_TYPE__REFERENCE:
            {
                SG_ERR_CHECK(  sg_zing__check_field__reference(pCtx, pzfa, psz_recid, psz_field_val, pva_violations)  );
            }
            break;

            case SG_ZING_TYPE__ATTACHMENT:
            {
                // TODO
            }
            break;

            default:
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
                break;
        }

continue_loop:
        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_field_name, (void**) &psz_field_val)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // fall thru

fail:
    ;
}

static void sg_zingrecord__check_required_fields(
        SG_context* pCtx,
        SG_zingtemplate* pztemplate,
        SG_zingrecord* pzrec,
        SG_varray* pva_violations
        )
{
    SG_stringarray* psa_fields = NULL;
    const char* psz_rectype = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_zingfieldattributes* pzfa = NULL;

    SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzrec, &psz_rectype)  );

    SG_ERR_CHECK(  SG_zingtemplate__list_fields(pCtx, pztemplate, psz_rectype, &psa_fields)  );
    SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_fields, &count )  );
    for (i=0; i<count; i++)
    {
        const char* psz_field_name = NULL;

        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_fields, i, &psz_field_name)  );
        SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pztemplate, psz_rectype, psz_field_name, &pzfa)  );

        if (pzfa->required)
        {
            SG_bool b = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzrec->prb_field_values, psz_field_name, &b, NULL)  );
            if (!b)
            {
                const char* psz_recid = NULL;

                if (SG_DAGNUM__HAS_NO_RECID(pzrec->pztx->iDagNum))
                {
                    SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                                SG_ZING_CONSTRAINT_VIOLATION__TYPE, "required_field",
                                SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                                NULL) );
                }
                else
                {
                    SG_ERR_CHECK(  SG_zingrecord__get_recid(pCtx, pzrec, &psz_recid)  );
                    SG_ERR_CHECK(  sg_zing__add_error_object(pCtx, pva_violations,
                                SG_ZING_CONSTRAINT_VIOLATION__TYPE, "required_field",
                                SG_ZING_CONSTRAINT_VIOLATION__RECID, psz_recid,
                                SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, pzfa->name,
                                NULL) );
                }
            }
        }
    }

    // fall thru

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa_fields);
}

#if 0
static void sg_zingtx__add_recheck(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_where
        )
{
    const char* psz_cur = NULL;
    SG_bool b = SG_FALSE;

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pztx->prb_rechecks, psz_rectype, &b, (void**) &psz_cur)  );

    // TODO perform an OR operation
    // sprintf(newstring, "(%s) || (%s)", prev_string, psz_where)
    // if either string is "", that means just recheck everything,
    // so set the whole thing to ""

fail:
    ;
}
#endif

#if 1
static void sg_zingtx__figure_out_template_change(
        SG_context* pCtx,
        SG_zingtx* pztx
        )
{
	SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_rectype = NULL;

    // when the template has changed, we need to re-run
    // constraint checks on records currently in the
    // database.

    // The following code is WAY less efficient than it could be.
    //
    // When this code is smarter, it will compare the template to the
    // one we started with and figure out what has changed.  Then it
    // will create queries which describe the records that need to be
    // rechecked.  At the very least, this will mean that when somebody
    // tweaks the template for one rectype, we only need to recheck
    // records of that rectype.  If the code is really smart, it can
    // figure out that when somebody changes the max on a field from
    // 5 to 9, no records need to be rechecked at all.
    //
    // For now, we play dumb.  Anytime the template changes, we
    // trigger a recheck of every record in every rectype.  This
    // means that template changes will be really expensive when the
    // db already has lots of records in it.

    SG_ERR_CHECK(  SG_zingtemplate__list_rectypes(pCtx, pztx->ptemplate, &pztx->prb_rechecks)  );
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_rechecks, &b, &psz_rectype, NULL)  );
    while (b)
    {
        // the empty string "" is a query which matches all records of that rectype
        SG_ERR_CHECK(  SG_rbtree__update__with_pooled_sz(pCtx, pztx->prb_rechecks, psz_rectype, "")  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_rectype, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

fail:
    return;
}

static void sg_zingtx__do_one_recheck(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_where,
        SG_varray* pva_violations
        )
{
    SG_varray* pva = NULL;
    SG_dbrecord* pdbrec = NULL;
    SG_zingrecord* pzrec = NULL;
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HIDREC)  );
    SG_ERR_CHECK(  SG_zing__query(
                pCtx,
                pztx->pRepo,
                pztx->iDagNum,
                pztx->psz_csid,
                psz_rectype,
                psz_where,
                NULL,
                0,
                0,
                pva_fields,
                &pva
                )  );

    if (pva)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count )  );
        for (i=0; i<count; i++)
        {
            const char* psz_hid_rec = NULL;
            const char* psz_recid = NULL;
            SG_bool b_already_loaded = SG_FALSE;

            SG_ERR_CHECK(  SG_vaofvh__get__sz(pCtx, pva, i, NULL, &psz_hid_rec)  );
            // TODO load the dbrecord, stuff it into a zing record, check it,
            // then unload it.
            SG_ERR_CHECK(  SG_dbrecord__load_from_repo(pCtx, pztx->pRepo, psz_hid_rec, &pdbrec)  );
            SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec, SG_ZING_FIELD__RECID, &psz_recid)  );
            b_already_loaded = SG_FALSE;
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, pztx->prb_records, psz_recid, &b_already_loaded, (void**) &pzrec)  );
            if (b_already_loaded)
            {
                // TODO
            }
            else
            {
                SG_ERR_CHECK(  SG_zingrecord__alloc__from_dbrecord(pCtx, pztx, pdbrec, SG_TRUE, &pzrec)  );
                //SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_records, psz_recid, pzrec)  );
                // TODO what checks should be done here?
                SG_ERR_CHECK(  sg_zingrecord__check_required_fields(pCtx, pztx->ptemplate, pzrec, pva_violations)  );
                SG_ERR_CHECK(  sg_zingrecord__check_intrarecord(pCtx, pztx->ptemplate, pzrec, pva_violations)  );

                SG_DBRECORD_NULLFREE(pCtx, pdbrec);

                // now remove the pzrec
                //SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pztx->prb_records, psz_recid)  );
                SG_ZINGRECORD_NULLFREE(pCtx, pzrec);
            }
        }

        SG_VARRAY_NULLFREE(pCtx, pva);
    }

fail:
    SG_DBRECORD_NULLFREE(pCtx, pdbrec);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

static void sg_zingtx__do_rechecks(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_varray* pva_violations
        )
{
	SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_rectype = NULL;
    const char* psz_where = NULL;

    if (pztx->prb_rechecks)
    {
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_rechecks, &b, &psz_rectype, (void**) &psz_where)  );
        while (b)
        {
            SG_ERR_CHECK(  sg_zingtx__do_one_recheck(pCtx, pztx, psz_rectype, psz_where, pva_violations)  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_rectype, (void**) &psz_where)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

fail:
    ;
}
#endif

static void sg_zingtx__check_commit_time_constraints(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_varray* pva_violations
        )
{
	SG_rbtree_iterator* pit = NULL;
	SG_bool b = SG_FALSE;
    const char* psz_recid = NULL;
    SG_zingrecord* pzrec = NULL;
    SG_zingtemplate* pztemplate = NULL;
    SG_vhash* pvh_unique_checks = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_unique_checks)  );

    // ask the tx for its template
    SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pztemplate)  );

    // now iterate over all dirty records
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_records, &b, &psz_recid, (void**) &pzrec)  );
    while (b)
    {
        SG_bool bDeleteMe = SG_FALSE;

        SG_ERR_CHECK(  SG_zingrecord__is_delete_me(pCtx, pzrec, &bDeleteMe)  );

        if (!bDeleteMe)
        {
            if (pzrec->b_dirty_fields)
            {
                SG_ERR_CHECK(  sg_zingrecord__check_required_fields(pCtx, pztemplate, pzrec, pva_violations)  );
                SG_ERR_CHECK(  sg_zingrecord__check_intrarecord(pCtx, pztemplate, pzrec, pva_violations)  );
                if (SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(pztx->iDagNum))
                {
                    SG_ERR_CHECK(  sg_zingrecord__find_unique_checks_to_be_done(pCtx, pztx, pztemplate, pzrec, pvh_unique_checks)  );
                }
            }
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**) &pzrec)  );
    }

    if (SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(pztx->iDagNum))
    {
        SG_ERR_CHECK(  sg_zingtx__do_unique_checks(pCtx, pztx, pvh_unique_checks, pva_violations)  );
    }

    // fall thru

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_unique_checks);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

void SG_zingtx__set_baseline_delta(
	SG_context* pCtx,
    SG_zingtx* pztx,
    const char* psz_csid_parent,
    struct sg_zing_merge_delta* pd
    )
{
	SG_NULLARGCHECK_RETURN(pztx);
	SG_NULLARGCHECK_RETURN(psz_csid_parent);
	SG_NULLARGCHECK_RETURN(pd);

    if (!pztx->prb_deltas)
    {
        SG_RBTREE__ALLOC(pCtx, &pztx->prb_deltas);
    }

    SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx, pztx->prb_deltas, psz_csid_parent, pd)  );
}

void SG_zing__commit_tx(
	SG_context* pCtx,
    SG_int64 when,
	SG_zingtx** ppTx,
    SG_changeset** ppcs,
    SG_dagnode** ppdn,
    SG_varray** ppva_constraint_violations
    )
{
    SG_ERR_CHECK_RETURN(  SG_zing__commit_tx__hidrecs(pCtx, when, ppTx, ppcs, ppdn, ppva_constraint_violations, NULL, NULL)  );
}

void SG_zing__commit_tx__hidrecs(
        SG_context* pCtx,
        SG_int64 when,
        SG_zingtx** ppztx,
        SG_changeset** ppcs,
        SG_dagnode** ppdn,
        SG_varray** ppva_constraint_violations,
        SG_vhash** ppvh_hidrec_adds,
        SG_vhash** ppvh_hidrec_mods
        )
{
    SG_pendingdb* pdb = NULL;
	SG_rbtree_iterator* pit = NULL;
	SG_bool b = SG_FALSE;
    const char* psz_recid = NULL;
	SG_rbtree_iterator* pit_att = NULL;
	SG_bool b_att = SG_FALSE;
    const char* psz_att_fieldname = NULL;
    struct sg_attachment* p_att = NULL;
    SG_zingrecord* pzrec = NULL;
    SG_dbrecord* pdbrec = NULL;
	SG_zingtx* pztx = NULL;
    const char* psz_csid_parent = NULL;
    SG_varray* pva_violations = NULL;
    SG_uint32 count_violations = 0;
    SG_audit q;
    SG_vhash* pvh_hidrec_adds = NULL;
    SG_vhash* pvh_hidrec_mods = NULL;

	if(ppztx==NULL||*ppztx==NULL)
    {
		return;
    }

	pztx = *ppztx;

    if (ppvh_hidrec_adds)
    {
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_hidrec_adds)  );
    }

    if (ppvh_hidrec_mods)
    {
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_hidrec_mods)  );
    }

    SG_ERR_CHECK(  SG_strcpy(pCtx, q.who_szUserId, sizeof(q.who_szUserId), pztx->buf_who)  );
    q.when_int64 = when;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_violations)  );

#if 1
    if (pztx->b_template_dirty)
    {
        SG_ERR_CHECK(  sg_zingtx__figure_out_template_change(pCtx, pztx)  );
    }

    if (pztx->psz_csid)
    {
        SG_ERR_CHECK(  sg_zingtx__do_rechecks(pCtx, pztx, pva_violations)  );
    }
    SG_RBTREE_NULLFREE(pCtx, pztx->prb_rechecks);
#endif

    SG_ERR_CHECK(  sg_zingtx__check_commit_time_constraints(pCtx, pztx, pva_violations)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_violations, &count_violations)  );
    if (count_violations)
    {
        if (ppva_constraint_violations)
        {
            *ppva_constraint_violations = pva_violations;
            pva_violations = NULL;
            goto done;
        }
        else
        {
            SG_VARRAY_NULLFREE(pCtx, pva_violations);
            SG_ERR_THROW(  SG_ERR_ZING_CONSTRAINT  );
        }
    }
    SG_VARRAY_NULLFREE(pCtx, pva_violations);

    pztx->b_in_commit = SG_TRUE;

    SG_ERR_CHECK(  SG_pendingdb__alloc(pCtx, pztx->pRepo, pztx->iDagNum, pztx->psz_csid, pztx->psz_hid_template, &pdb)  );

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_parents, &b, &psz_csid_parent, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_pendingdb__add_parent(pCtx, pdb, psz_csid_parent)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid_parent, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    if (pztx->b_template_dirty && pztx->b_template_is_mine)
    {
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_zingtemplate__get_vhash(pCtx, pztx->ptemplate, &pvh)  );
        SG_ERR_CHECK(  SG_pendingdb__set_template(pCtx, pdb, pvh)  );
    }

    // first the records
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_records, &b, &psz_recid, (void**) &pzrec)  );
    while (b)
    {
        SG_bool bDeleteMe = SG_FALSE;

        SG_ERR_CHECK(  SG_zingrecord__is_delete_me(pCtx, pzrec, &bDeleteMe)  );

        if (pzrec->prb_attachments)
        {
            SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit_att, pzrec->prb_attachments, &b_att, &psz_att_fieldname, (void**) &p_att)  );
            while (b_att)
            {
                char* psz_hid_att_blob = NULL;

                if (p_att->pPath)
                {
                    SG_ERR_CHECK(  SG_pendingdb__add_attachment__from_pathname(pCtx, pdb, p_att->pPath, &psz_hid_att_blob)  );
                }
                else
                {
                    SG_ERR_CHECK(  SG_pendingdb__add_attachment__from_memory(pCtx, pdb, p_att->pBytes, p_att->len, &psz_hid_att_blob)  );
                }
                SG_ERR_CHECK(  SG_rbtree__update__with_pooled_sz(pCtx, pzrec->prb_field_values, psz_att_fieldname, psz_hid_att_blob)  );
                SG_NULLFREE(pCtx, psz_hid_att_blob);

                SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit_att, &b_att, &psz_att_fieldname, (void**) &p_att)  );
            }
            SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit_att);
        }

        if (pzrec->psz_original_hid)
        {
            if (bDeleteMe)
            {
                SG_ERR_CHECK(  SG_pendingdb__remove(pCtx, pdb, pzrec->psz_original_hid)  );
            }
            else
            {
                if (pzrec->b_dirty_fields)
                {
                    const char* psz_hid_rec = NULL;

                    SG_ERR_CHECK(  SG_pendingdb__remove(pCtx, pdb, pzrec->psz_original_hid)  );

                    SG_ERR_CHECK(  SG_zingrecord__to_dbrecord(pCtx, pzrec, &pdbrec)  );

                    SG_ERR_CHECK(  SG_pendingdb__add(pCtx, pdb, &pdbrec, &psz_hid_rec)  );

                    if (pvh_hidrec_mods)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_hidrec_mods, psz_hid_rec)  );
                    }
                }
            }
        }
        else
        {
            if (!bDeleteMe)
            {
                const char* psz_hid_rec = NULL;

                SG_ERR_CHECK(  SG_zingrecord__to_dbrecord(pCtx, pzrec, &pdbrec)  );

                SG_ERR_CHECK(  SG_pendingdb__add(pCtx, pdb, &pdbrec, &psz_hid_rec)  );

                if (pvh_hidrec_adds)
                {
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_hidrec_adds, psz_hid_rec)  );
                }
            }
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**) &pzrec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    if (SG_DAGNUM__HAS_NO_RECID(pztx->iDagNum))
    {
        const char* psz_hidrec = NULL;

        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_delete_hidrec, &b, &psz_hidrec, NULL)  );
        while (b)
        {
            SG_ERR_CHECK(  SG_pendingdb__remove(pCtx, pdb, psz_hidrec)  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hidrec,  NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

    if (pztx->prb_deltas)
    {
        const char* psz_csid_parent = NULL;
        struct sg_zing_merge_delta* pd = NULL;

        b = SG_FALSE;
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_deltas, &b, &psz_csid_parent, (void**) &pd)  );
        while (b)
        {
            SG_ERR_CHECK(  SG_pendingdb__set_baseline_delta(
                        pCtx,
                        pdb,
                        psz_csid_parent,
                        pd
                        )  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid_parent, (void**) &pd)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

    SG_ERR_CHECK(  SG_pendingdb__commit(pCtx, pdb, &q, ppcs, ppdn)  );

    SG_PENDINGDB_NULLFREE(pCtx, pdb);

    // TODO what is the state right now of the members of the zingtx struct?
    // Can anything happen to it now except for free() ?

    pztx->b_in_commit = SG_FALSE;
	SG_ERR_CHECK(  _sg_zingtx__free(pCtx, pztx)  );
	*ppztx = NULL;

    if (ppvh_hidrec_adds)
    {
        *ppvh_hidrec_adds = pvh_hidrec_adds;
        pvh_hidrec_adds = NULL;
    }

    if (ppvh_hidrec_mods)
    {
        *ppvh_hidrec_mods = pvh_hidrec_mods;
        pvh_hidrec_mods = NULL;
    }

done:

fail:
    SG_PENDINGDB_NULLFREE(pCtx, pdb);
    SG_VARRAY_NULLFREE(pCtx, pva_violations);
    SG_VHASH_NULLFREE(pCtx, pvh_hidrec_adds);
    SG_VHASH_NULLFREE(pCtx, pvh_hidrec_mods);
}

void SG_zing__abort_tx(
        SG_context* pCtx,
        SG_zingtx** ppztx
        )
{
	if(ppztx==NULL||*ppztx==NULL)
		return;

    // TODO

	SG_ERR_CHECK(  _sg_zingtx__free(pCtx, *ppztx)  );
	*ppztx = NULL;

	return;
fail:
	;
}

void SG_zing__get_record__vhash__fields(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char* psz_rectype,
        const char* psz_recid,
	    SG_varray* pva_fields,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_result = NULL;

    if (SG_DAGNUM__HAS_NO_RECID(iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pRepo, iDagNum, psz_csid, psz_rectype, SG_ZING_FIELD__RECID, psz_recid, pva_fields, &pvh_result)  );

    *ppvh = pvh_result;
    pvh_result = NULL;

    // fall thru

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_result);
}


void SG_zing__get_record__vhash(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char* psz_rectype,
        const char* psz_recid,
        SG_vhash** ppvh
        )
{
    SG_varray* pva_fields = NULL;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HISTORY)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );

	SG_ERR_CHECK(  SG_zing__get_record__vhash__fields(pCtx, pRepo, iDagNum, psz_csid, psz_rectype, psz_recid, pva_fields, ppvh)  );

    // fall thru

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

void SG_zingtx__generating__would_be_unique(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_val,
        SG_bool* pb
        )
{
    SG_varray* pva_crit = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_fields = NULL;
    SG_uint32 count_results = 0;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    SG_zingrecord* pzrec_other = NULL;

    // TODO it sucks that this is called when generating a unique value,
    // and then do_unique_check is ALSO called when committing the tx.
    // we should have a way of flagging this value as generated and already
    // verified to be unique.

    // first iterate over all dirty records
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pztx->prb_records, &b, NULL, (void**) &pzrec_other)  );
    while (b)
    {
        SG_bool bDeleteMe = SG_FALSE;

        SG_ERR_CHECK(  SG_zingrecord__is_delete_me(pCtx, pzrec_other, &bDeleteMe)  );

        if (!bDeleteMe)
        {
            if (pzrec_other->b_dirty_fields)
            {
                const char* psz_rectype_other = NULL;

                SG_ERR_CHECK(  SG_zingrecord__get_rectype(pCtx, pzrec_other, &psz_rectype_other)  );
                if (0 == strcmp(psz_rectype, psz_rectype_other))
                {
                    const char* psz_val_other = NULL;

                    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzrec_other->prb_field_values, psz_field_name, NULL, (void**) &psz_val_other)  );
                    if (0 == strcmp(psz_val_other, psz_val))
                    {
                        *pb = SG_FALSE;
                        goto done;
                    }
                }
            }
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, NULL, (void**) &pzrec_other)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // no matches in the pending records.  let's check the db.
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_field_name)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_val)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HIDREC)  );
    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pztx->pRepo, pztx->iDagNum, pztx->psz_csid, psz_rectype, pva_crit, NULL, 1, 0, pva_fields, &pva)  );
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    if (!pva)
    {
        *pb = SG_TRUE;
        goto done;
    }
    else
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_results)  );
        if(0 == count_results)
        {
            *pb = SG_TRUE;
            goto done;
        }
    }

    *pb = SG_FALSE;

done:
fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
}

/**
 * Use this function to get a record you do not intend to modify.
 */
void SG_zing__get_record(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char* psz_rectype,
        const char* psz_recid,
        SG_dbrecord** ppdbrec
        )
{
    SG_dbrecord* pdbrec = NULL;
    const char* psz_hid_rec = NULL;
    SG_varray* pva_crit = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_fields = NULL;
    SG_uint32 count_results = 0;

    if (SG_DAGNUM__HAS_NO_RECID(iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    // look up the recid (a gid) to find the current hid
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_recid)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HIDREC)  );
    // TODO shouldn't this use query__one?
    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, iDagNum, psz_csid, psz_rectype, pva_crit, NULL, 0, 0, pva_fields, &pva)  );
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    if (!pva)
    {
        SG_ERR_THROW(  SG_ERR_ZING_RECORD_NOT_FOUND  );
    }

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_results)  );
    if(count_results==0)
    {
        SG_ERR_THROW(SG_ERR_ZING_RECORD_NOT_FOUND);
    }
    SG_ASSERT(count_results==1);

    // load the dbrecord from the repo
    SG_ERR_CHECK(  SG_vaofvh__get__sz(pCtx, pva, 0, NULL, &psz_hid_rec)  );

    SG_ERR_CHECK(  SG_dbrecord__load_from_repo(pCtx, pRepo, psz_hid_rec, &pdbrec)  );
    SG_VARRAY_NULLFREE(pCtx, pva);

    *ppdbrec = pdbrec;
    pdbrec = NULL;

    // fall thru

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_DBRECORD_NULLFREE(pCtx, pdbrec);
}

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
        )
{
    SG_varray* pva_where = NULL;
    const char* psz_where_ltrim = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

    psz_where_ltrim = psz_where;
    if (psz_where_ltrim)
    {
        while (
                (*psz_where_ltrim)
                && (
                        (' ' == *psz_where_ltrim)
                        || ('\t' == *psz_where_ltrim)
                        || ('\r' == *psz_where_ltrim)
                        || ('\n' == *psz_where_ltrim)
                   )
              )
        {
            psz_where_ltrim++;
        }

        if (!*psz_where_ltrim)
        {
            psz_where_ltrim = NULL;
        }
    }

    SG_ERR_CHECK(  sg_zing__query__parse_where(pCtx, psz_rectype, psz_where_ltrim, &pva_where)  );

    SG_ERR_CHECK(  SG_repo__dbndx__query__recent(pCtx, pRepo, iDagNum, psz_rectype, pva_where, limit, skip, pva_fields, ppva_sliced)  );

    // fall thru

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_where);
}

void SG_zing__query(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_csid,
        const char* psz_rectype,
        const char* psz_where,
        const char* psz_sort,
        SG_uint32 limit,
        SG_uint32 skip,
        SG_varray* pva_fields,
        SG_varray** ppva_sliced
        )
{
    SG_varray* pva_where = NULL;
    SG_varray* pva_sort = NULL;
    const char* psz_where_ltrim = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

    psz_where_ltrim = psz_where;
    if (psz_where_ltrim)
    {
        while (
                (*psz_where_ltrim)
                && (
                        (' ' == *psz_where_ltrim)
                        || ('\t' == *psz_where_ltrim)
                        || ('\r' == *psz_where_ltrim)
                        || ('\n' == *psz_where_ltrim)
                   )
              )
        {
            psz_where_ltrim++;
        }

        if (!*psz_where_ltrim)
        {
            psz_where_ltrim = NULL;
        }
    }

    SG_ERR_CHECK(  sg_zing__query__parse_where(pCtx, psz_rectype, psz_where_ltrim, &pva_where)  );

    if (psz_sort)
    {
        SG_ERR_CHECK(  sg_zing__query__parse_sort(pCtx, psz_rectype, psz_sort, &pva_sort)  );
    }

    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, iDagNum, psz_csid, psz_rectype, pva_where, pva_sort, limit, skip, pva_fields, ppva_sliced)  );

    // fall thru

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_where);
    SG_VARRAY_NULLFREE(pCtx, pva_sort);
}

void SG_zing__extract_one_from_slice__string(
        SG_context* pCtx,
        SG_varray* pva,
        const char* psz_name,
        SG_varray** ppva2
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_varray* pva2 = NULL;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    SG_ERR_CHECK(  SG_VARRAY__ALLOC__PARAMS(pCtx, &pva2, count, NULL, NULL)  );
    for (i=0; i<count; i++)
    {
        const char* psz_val = NULL;
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, psz_name, &psz_val)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva2, psz_val)  );
    }

    *ppva2 = pva2;
    pva2 = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva2);
}

void SG_zingtx__lookup_recid(
        SG_context* pCtx,
        SG_zingtx* pztx,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_field_value,
        char** ppsz_recid
        )
{
    SG_ERR_CHECK_RETURN(  SG_zing__lookup_recid(pCtx, pztx->pRepo, pztx->iDagNum, pztx->psz_csid, psz_rectype, psz_field_name, psz_field_value, ppsz_recid)  );
}

void SG_zing__lookup_recid(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_state,
        const char* psz_rectype,
        const char* psz_field_name,
        const char* psz_field_value,
        char** ppsz_recid
        )
{
    SG_string* pstr_where = NULL;
    SG_varray* pva_fields = NULL;
    SG_varray* pva = NULL;
    const char* psz_recid = NULL;
    SG_uint32 count_results = 0;
	char *psz_esc_name = NULL;
	char *psz_esc_val = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_where)  );

	SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_field_name, &psz_esc_name)  );
	SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_field_value, &psz_esc_val)  );

    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_where, "%s == '%s'", ESCAPEDSTR(psz_esc_name, psz_field_name), ESCAPEDSTR(psz_esc_val, psz_field_value))  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );
    SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, iDagNum, psz_state, psz_rectype, SG_string__sz(pstr_where), NULL, 0, 0, pva_fields, &pva)  );
    if (!pva)
    {
        SG_ERR_THROW(  SG_ERR_ZING_RECORD_NOT_FOUND  );
    }

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_results)  );

    if(count_results==0)
    {
        SG_ERR_THROW(SG_ERR_ZING_RECORD_NOT_FOUND);
    }

    if(count_results > 1)
    {
        SG_ERR_THROW(SG_ERR_ZING_RECORD_NOT_FOUND);  // TODO better error
    }

    SG_ERR_CHECK(  SG_vaofvh__get__sz(pCtx, pva, 0, NULL, &psz_recid)  );
    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_recid, ppsz_recid)  );

fail:
    SG_STRING_NULLFREE(pCtx, pstr_where);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VARRAY_NULLFREE(pCtx, pva);
	SG_NULLFREE(pCtx, psz_esc_name);
	SG_NULLFREE(pCtx, psz_esc_val);
}

void SG_zing__create_root_node(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        char** pp
        )
{
    SG_audit q;
    SG_pendingdb* pdb = NULL;
    SG_dagnode* pdn = NULL;
    const char* psz_csid = NULL;

	SG_ERR_CHECK(  SG_audit__init__nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW)  );
    SG_ERR_CHECK(  SG_pendingdb__alloc(pCtx, pRepo, iDagNum, NULL, NULL, &pdb)  );
    SG_ERR_CHECK(  SG_pendingdb__commit(pCtx, pdb, &q, NULL, &pdn)  );
    SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pdn, &psz_csid)  );
    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_csid, pp)  );

fail:
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_PENDINGDB_NULLFREE(pCtx, pdb);
    ;
}

#if 0
void SG_zing__get_any_leaf(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        char** pp
        )
{
    SG_rbtree* prb_leaves = NULL;
    const char* psz_hid_cs_leaf = NULL;
    SG_uint32 count_leaves = 0;

    SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, iDagNum, &prb_leaves)  );
    SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_leaves, &count_leaves)  );

    if (0 == count_leaves)
    {
        if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(iDagNum))
        {
            SG_ERR_CHECK(  SG_zing__create_root_node(pCtx, pRepo, iDagNum, pp)  );
        }
    }
    else if (1 == count_leaves)
    {
        SG_ERR_CHECK(  SG_rbtree__get_only_entry(pCtx, prb_leaves, &psz_hid_cs_leaf, NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_cs_leaf, pp)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, NULL, prb_leaves, NULL, &psz_hid_cs_leaf, NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_cs_leaf, pp)  );
    }

    // fall thru

fail:
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);
}
#endif

void SG_zing__get_leaf(
        SG_context* pCtx,
        SG_repo* pRepo,
        const SG_audit* pq,
        SG_uint64 iDagNum,
        char** pp
        )
{
    SG_rbtree* prb_leaves = NULL;
    const char* psz_hid_cs_leaf = NULL;
    SG_dagnode* pdn_merged = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    SG_audit q;

	SG_NULLARGCHECK_RETURN(pp);

    while (1)
    {
        const char* aleaves[2];
        SG_uint32 count_leaves = 0;
        SG_uint32 i = 0;
        const char* psz_csid = NULL;

        aleaves[0] = NULL;
        aleaves[1] = NULL;

        SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, iDagNum, &prb_leaves)  );
        SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_leaves, &count_leaves)  );

        if (0 == count_leaves)
        {
            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(iDagNum))
            {
                SG_ERR_CHECK(  SG_zing__create_root_node(pCtx, pRepo, iDagNum, pp)  );
            }
            else
            {
                *pp = NULL;
            }
            break;
        }

        if (1 == count_leaves)
        {
            SG_ERR_CHECK(  SG_rbtree__get_only_entry(pCtx, prb_leaves, &psz_hid_cs_leaf, NULL)  );
            SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_cs_leaf, pp)  );
            break;
        }

        SG_ASSERT(count_leaves >= 2);

        // get the first two leaves
        i = 0;
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_leaves, &b, &psz_csid, NULL)  );
        while (b)
        {
            aleaves[i++] = psz_csid;
            if (2 == i)
            {
                break;
            }

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid, NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
        SG_ASSERT(2 == i);

        if (!pq)
        {
            SG_ERR_CHECK(  SG_audit__init__nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW)  );
            pq = &q;
        }

        // merge them
        SG_ERR_CHECK(  SG_zing__automerge(pCtx,
                    pRepo,
                    iDagNum,
                    pq,
                    aleaves[0],
                    aleaves[1],
                    &pdn_merged
                    )  );

        if (2 == count_leaves)
        {
            SG_ERR_CHECK(  SG_dagnode__get_id(pCtx, pdn_merged, pp)  );
            SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
            SG_RBTREE_NULLFREE(pCtx, prb_leaves);
            break;
        }

        // if there were more than 2 leaves, we need to go back
        // through the loop to merge again

        SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
        SG_RBTREE_NULLFREE(pCtx, prb_leaves);
    }

    // fall thru

fail:
    SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);
}

static void _auto_merge_one_dag(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 dagnum,
	const SG_audit* pAudit
    )
{
	char* pszLeaf = NULL;
	SG_vhash* pvhNew = NULL;

	if (SG_DAGNUM__IS_DB(dagnum))
	{
#if TRACE_PULL
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Auto-merging zing dag %d\n", dagnum)  );
#endif
		SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, pAudit, dagnum, &pszLeaf)  );
		SG_NULLFREE(pCtx, pszLeaf); // TODO: make zing handle null leaf arg, or write a merge-only routine
	}

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhNew);
	SG_NULLFREE(pCtx, pszLeaf);
}


void SG_zing__auto_merge_all_dags(
	SG_context* pCtx,
	SG_repo* pRepo
    )
{
	SG_uint32 i, count;
	SG_uint64* dagnumArray = NULL;
	SG_audit audit;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Merging databases", SG_LOG__FLAG__NONE)  );

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count, &dagnumArray)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count, NULL)  );

    // and merge the users dag before the others
	SG_ERR_CHECK(  SG_audit__init__nobody(pCtx, &audit, pRepo, SG_AUDIT__WHEN__NOW)  );
    SG_ERR_CHECK(  _auto_merge_one_dag(pCtx, pRepo, SG_DAGNUM__USERS, &audit)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    // now merge all the other dags
	SG_ERR_CHECK(  SG_audit__init__nobody(pCtx, &audit, pRepo, SG_AUDIT__WHEN__NOW)  );

	for (i = 0; i < count; i++)
	{
		SG_uint64 dagnum = dagnumArray[i];
		if (
            (SG_DAGNUM__USERS != dagnum)
			)
		{
			if (SG_DAGNUM__IS_DB(dagnum))
			{
				SG_ERR_CHECK(  _auto_merge_one_dag(pCtx, pRepo, dagnum, &audit)  );
			}
			SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
		}
	}

	/* fall through */
fail:
	SG_NULLFREE(pCtx, dagnumArray);
	SG_log__pop_operation(pCtx);
}

void SG_zing__field_type_is_integerish(
        SG_context* pCtx,
        SG_uint16 type,
        SG_bool* pb
        )
{
    SG_NULLARGCHECK_RETURN(pb);

    switch (type)
    {
        case SG_ZING_TYPE__BOOL:
        case SG_ZING_TYPE__DATETIME:
        case SG_ZING_TYPE__INT:
            *pb = SG_TRUE;
            return;
        default:
            *pb = SG_FALSE;
            return;
    }
}

void SG_zing__field_type_is_integerish__sz(
        SG_context* pCtx,
        const char* psz_field_type,
        SG_bool* pb
        )
{
    SG_uint16 type = 0;

    SG_ERR_CHECK_RETURN(  SG_zing__field_type__sz_to_uint16(pCtx, psz_field_type, &type)  );
    SG_ERR_CHECK_RETURN(  SG_zing__field_type_is_integerish(pCtx, type, pb)  );
}

void SG_zing__field_type__sz_to_uint16(
        SG_context* pCtx,
        const char* psz_field_type,
        SG_uint16* pi
        )
{
    if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__INT))
    {
        *pi = SG_ZING_TYPE__INT;
    }
    else if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__DATETIME))
    {
        *pi = SG_ZING_TYPE__DATETIME;
    }
    else if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__USERID))
    {
        *pi = SG_ZING_TYPE__USERID;
    }
    else if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__STRING))
    {
        *pi = SG_ZING_TYPE__STRING;
    }
    else if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__BOOL))
    {
        *pi = SG_ZING_TYPE__BOOL;
    }
    else if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__ATTACHMENT))
    {
        *pi = SG_ZING_TYPE__ATTACHMENT;
    }
    else if (0 == strcmp(psz_field_type, SG_ZING_TYPE_NAME__REFERENCE))
    {
        *pi = SG_ZING_TYPE__REFERENCE;
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
    }
}

