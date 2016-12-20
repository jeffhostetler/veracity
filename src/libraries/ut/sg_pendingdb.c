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

struct _SG_pendingdb
{
    SG_uint64 iDagNum;
    SG_repo* pRepo;
    SG_rbtree* prbParents;
    SG_rbtree* prb_records_add;
    SG_rbtree* prb_records_remove;
    SG_vhash* pvh_attach_add;
    SG_rbtree* prbAttachments_buffers;
    SG_rbtree* prbAttachments_files;
    SG_vhash* pvh_template;
    char buf_hid_template[SG_HID_MAX_BUFFER_LENGTH];
    char buf_hid_baseline[SG_HID_MAX_BUFFER_LENGTH];

    SG_rbtree* prb_baseline_deltas_from_merge;
};

void SG_pendingdb__alloc(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_hid_baseline,
    const char* psz_hid_template,
	SG_pendingdb** ppThis
	)
{
    SG_pendingdb* pThis = NULL;

    if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(iDagNum))
    {
        SG_ASSERT(!psz_hid_template);
    }

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );

    if (psz_hid_baseline)
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, pThis->buf_hid_baseline, sizeof(pThis->buf_hid_baseline), psz_hid_baseline)  );
    }

    pThis->iDagNum = iDagNum;
    if (psz_hid_template)
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, pThis->buf_hid_template, sizeof(pThis->buf_hid_template), psz_hid_template)  );
    }

    pThis->pRepo = pRepo;

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->prbParents)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->prb_records_add)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->prb_records_remove)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pThis->pvh_attach_add)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->prbAttachments_buffers)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pThis->prbAttachments_files)  );

    *ppThis = pThis;
    pThis = NULL;

fail:
    SG_PENDINGDB_NULLFREE(pCtx, pThis);
}

void SG_pendingdb__free(SG_context* pCtx, SG_pendingdb* pThis)
{
	if (!pThis)
		return;

    SG_RBTREE_NULLFREE(pCtx, pThis->prbAttachments_buffers);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pThis->prbAttachments_files, (SG_free_callback*) SG_pathname__free);
    SG_RBTREE_NULLFREE(pCtx, pThis->prb_baseline_deltas_from_merge);
    SG_RBTREE_NULLFREE(pCtx, pThis->prb_records_add);
    SG_RBTREE_NULLFREE(pCtx, pThis->prb_records_remove);
    SG_VHASH_NULLFREE(pCtx, pThis->pvh_attach_add);
    SG_RBTREE_NULLFREE(pCtx, pThis->prbParents);
    SG_NULLFREE(pCtx, pThis);
}

void SG_pendingdb__add(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    SG_dbrecord** ppRecord,
    const char** ppsz_hid
    )
{
    const char* psz_hid_rec = NULL;

    SG_ERR_CHECK(  SG_dbrecord__freeze(pCtx, *ppRecord, pPendingDb->pRepo, &psz_hid_rec)  );
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pPendingDb->prb_records_add, psz_hid_rec, *ppRecord)  );
    *ppRecord = NULL;

    if (ppsz_hid)
    {
        *ppsz_hid = psz_hid_rec;
    }

fail:
    return;
}

void SG_pendingdb__add__hidrec(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    const char* psz_hid_rec
    )
{
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pPendingDb->prb_records_add, psz_hid_rec, NULL)  );

fail:
    return;
}

void SG_pendingdb__add_attachment__from_pathname(
    SG_context* pCtx,
	SG_pendingdb * pPendingDb,
    SG_pathname* pPath,
    char** ppsz_hid
    )
{
    char* psz_hid = NULL;
    SG_pathname* pOldPath = NULL;
    SG_file* pFile = NULL;
    SG_pathname* pMyPath = NULL;

	SG_NULLARGCHECK_RETURN(pPath);
	SG_NULLARGCHECK_RETURN(pPendingDb);
	SG_NULLARGCHECK_RETURN(ppsz_hid);

    // calc the hid
    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx, pPendingDb->pRepo, pFile, &psz_hid)  );
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pPendingDb->pvh_attach_add, psz_hid)  );

    SG_ERR_CHECK(  SG_pathname__alloc__copy(pCtx, &pMyPath, pPath)  );
    SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pPendingDb->prbAttachments_files, psz_hid, pMyPath, (void**) &pOldPath)  );
    pMyPath = NULL;

    SG_PATHNAME_NULLFREE(pCtx, pOldPath);

    *ppsz_hid = psz_hid;
    psz_hid = NULL;

    // fall thru

fail:
    SG_PATHNAME_NULLFREE(pCtx, pMyPath);
    SG_NULLFREE(pCtx, psz_hid);
    SG_FILE_NULLCLOSE(pCtx, pFile);
}

void SG_pendingdb__add_attachment__from_memory(
    SG_context* pCtx,
	SG_pendingdb * pPendingDb,
    void* pBytes,
    SG_uint32 len,
    char** ppsz_hid
    )
{
    const char* psz_hid = NULL;

    SG_ERR_CHECK_RETURN(  SG_rbtree__memoryblob__add__hid(pCtx, pPendingDb->prbAttachments_buffers, pPendingDb->pRepo, pBytes, len, &psz_hid)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__update__null(pCtx, pPendingDb->pvh_attach_add, psz_hid)  );
    SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, psz_hid, ppsz_hid)  );
}

void SG_pendingdb__remove(
    SG_context* pCtx,
	SG_pendingdb * pPendingDb,
    const char* psz_hid_record
    )
{
    SG_ERR_CHECK_RETURN(  SG_rbtree__update(pCtx, pPendingDb->prb_records_remove, psz_hid_record)  );
}

void SG_pendingdb__add_parent(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    const char* psz_hid_cs_parent
    )
{
    if (psz_hid_cs_parent)
    {
        SG_ERR_CHECK_RETURN(  SG_rbtree__add(pCtx, pPendingDb->prbParents, psz_hid_cs_parent)  );
    }
}

void SG_pendingdb__set_template(
	SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    SG_vhash* pvh
    )
{
	SG_NULLARGCHECK_RETURN(pPendingDb);
	SG_NULLARGCHECK_RETURN(pvh);

    SG_ASSERT(!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pPendingDb->iDagNum));

    pPendingDb->pvh_template = pvh;
}

void SG_pendingdb__set_baseline_delta(
	SG_context* pCtx,
	SG_pendingdb* pdb,
    const char* psz_csid_parent,
    struct sg_zing_merge_delta* pd
    )
{
	SG_NULLARGCHECK_RETURN(pdb);
	SG_NULLARGCHECK_RETURN(psz_csid_parent);
	SG_NULLARGCHECK_RETURN(pd);

    if (!pdb->prb_baseline_deltas_from_merge)
    {
        SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC(pCtx, &pdb->prb_baseline_deltas_from_merge)  );
    }

    SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx, pdb->prb_baseline_deltas_from_merge, psz_csid_parent, pd)  );
}

static void sg_pendingdb__calc_parent_delta(
        SG_context* pCtx,
        SG_rbtree* prb_records_add,
        SG_rbtree* prb_records_remove,
        SG_rbtree* prb_desired_baseline__add,
        SG_rbtree* prb_desired_baseline__remove,
        const char* psz_key_add,
        const char* psz_key_remove,
        SG_vhash* pvh_delta
        )
{
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_hid_record = NULL;

    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_delta, psz_key_add, &pvh_add)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_delta, psz_key_remove, &pvh_remove)  );

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_records_add, &b, &psz_hid_record, NULL)  );
    while (b)
    {
        SG_bool b_doit = SG_TRUE;

        // any record in add which is already in desired_baseline.add
        // can be skipped
        if (prb_desired_baseline__add)
        {
            SG_bool b_is_in_parent_baseline = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_desired_baseline__add, psz_hid_record, &b_is_in_parent_baseline, NULL)  );
            if (b_is_in_parent_baseline)
            {
                b_doit = SG_FALSE;
            }
        }

        if (b_doit)
        {
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_add, psz_hid_record, 0)  );
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // now the removes
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_records_remove, &b, &psz_hid_record, NULL)  );
    while (b)
    {
        SG_bool b_doit = SG_TRUE;

        // any record in remove which is already in desired_baseline.remove
        // can be skipped
        if (prb_desired_baseline__remove)
        {
            SG_bool b_is_in_parent_baseline = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_desired_baseline__remove, psz_hid_record, &b_is_in_parent_baseline, NULL)  );
            if (b_is_in_parent_baseline)
            {
                b_doit = SG_FALSE;
            }
        }

        if (b_doit)
        {
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_remove, psz_hid_record, 0)  );
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // for any rec in desired_parent.add which is NOT in our add,
    // we need to add to our remove
    if (prb_desired_baseline__add)
    {
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_desired_baseline__add, &b, &psz_hid_record, NULL)  );
        while (b)
        {
            SG_bool b_found = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_records_add, psz_hid_record, &b_found, NULL)  );
            if (!b_found)
            {
                SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_remove, psz_hid_record, 0)  );
            }

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

    // for any rec in desired_parent.remove which is NOT in our remove,
    // we need to add to our add
    if (prb_desired_baseline__remove)
    {
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_desired_baseline__remove, &b, &psz_hid_record, NULL)  );
        while (b)
        {
            SG_bool b_found = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_records_remove, psz_hid_record, &b_found, NULL)  );
            if (!b_found)
            {
                SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_add, psz_hid_record, 0)  );
            }

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

void SG_pendingdb__commit(
	SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    const SG_audit* pq,
    SG_changeset** ppcs,
    SG_dagnode** ppdn
    )
{
    SG_committing* ptx = NULL;
    SG_rbtree* prb_records_both_add_and_remove = NULL;
    SG_rbtree_iterator* pit = NULL;
    const char* psz_hid_cs_parent = NULL;
    const char* psz_hid_att = NULL;
    SG_pathname* pPath_att = NULL;
    SG_bool b = SG_FALSE;
    SG_changeset* pcs = NULL;
    SG_dagnode* pdn = NULL;
    char* psz_hid_delta = NULL;
    SG_dbrecord* prec = NULL;
    const char* psz_hid_record = NULL;
    SG_byte* pPacked = NULL;
    char* pszActualHid = NULL;
    SG_file* pFile_att = NULL;
    SG_vhash* pvh_delta = NULL;
    SG_string* pstr = NULL;
    char* psz_hid_template = NULL;
    SG_uint32 count_parents = 0;
    SG_varray* pva_fields = NULL;
    SG_vhash* pvh_rec = NULL;
    SG_vhash* pvh_dont_add = NULL;

	SG_NULLARGCHECK_RETURN(pPendingDb);

    SG_ERR_CHECK(  SG_committing__alloc(pCtx, &ptx, pPendingDb->pRepo, pPendingDb->iDagNum, pq, SG_CSET_VERSION_1)  );

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pPendingDb->prbParents, &b, &psz_hid_cs_parent, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_committing__add_parent(pCtx, ptx, psz_hid_cs_parent)  );
        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_cs_parent, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &prb_records_both_add_and_remove)  );
	SG_ERR_CHECK(  SG_rbtree__compare__keys_only(
                pCtx,
                pPendingDb->prb_records_add,
                pPendingDb->prb_records_remove,
                NULL,
                NULL,
                NULL,
                prb_records_both_add_and_remove
                )  );

    // eliminate the both_add_and_remove cases
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_records_both_add_and_remove, &b, &psz_hid_record, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pPendingDb->prb_records_remove, psz_hid_record)  );
        SG_ERR_CHECK(  SG_rbtree__remove__with_assoc(pCtx, pPendingDb->prb_records_add, psz_hid_record, (void**) &prec)  );
        SG_DBRECORD_NULLFREE(pCtx, prec);

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_RBTREE_NULLFREE(pCtx, prb_records_both_add_and_remove);

    SG_ERR_CHECK(  SG_rbtree__count(pCtx, pPendingDb->prbParents, &count_parents)  );

    // for trivial merge dags, make sure we are not adding a record that already exists in the single parent
    if (SG_DAGNUM__HAS_NO_RECID(pPendingDb->iDagNum) && (1 == count_parents))
    {
        const char* psz_csid_parent = NULL;

        SG_ERR_CHECK(  SG_rbtree__get_only_entry(pCtx, pPendingDb->prbParents, &psz_csid_parent, NULL)  );


        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );

        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pPendingDb->prb_records_add, &b, &psz_hid_record, (void**) &prec)  );
        while (b)
        {
            if (prec)
            {
                const char* psz_rectype = NULL;
                if (!SG_DAGNUM__HAS_SINGLE_RECTYPE(pPendingDb->iDagNum))
                {
                    SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, prec, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
                }

                SG_ERR_CHECK(  SG_repo__dbndx__query__one(pCtx, pPendingDb->pRepo, pPendingDb->iDagNum, psz_csid_parent, psz_rectype, "hidrec", psz_hid_record, pva_fields, &pvh_rec)  );
                if (pvh_rec)
                {
                    if (!pvh_dont_add)
                    {
                        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_dont_add)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_dont_add, psz_hid_record)  );
                }
                SG_VHASH_NULLFREE(pCtx, pvh_rec);
            }

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, (void**) &prec)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

        SG_VARRAY_NULLFREE(pCtx, pva_fields);

        if (pvh_dont_add)
        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dont_add, &count)  );
            for (i=0; i<count; i++)
            {
                const char* psz_hid = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dont_add, i, &psz_hid, NULL)  );
                SG_ERR_CHECK(  SG_rbtree__remove__with_assoc(pCtx, pPendingDb->prb_records_add, psz_hid, (void**) &prec)  );
                SG_DBRECORD_NULLFREE(pCtx, prec);
            }

            SG_VHASH_NULLFREE(pCtx, pvh_dont_add);
        }
    }


    // store the records
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pPendingDb->prb_records_add, &b, &psz_hid_record, (void**) &prec)  );
    while (b)
    {
        if (prec)
        {
            const char* psz_hid_from_record = NULL;

            SG_ERR_CHECK(  SG_committing__db__add_record(pCtx, ptx, prec)  );
            SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, prec, &psz_hid_from_record)  );
            /* TODO we COULD verify that the new hid matches the rbtree's key */
        }

        SG_DBRECORD_NULLFREE(pCtx, prec);

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, (void**) &prec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // Now calculate the deltas

    if (0 == count_parents)
    {
        SG_ASSERT(!pPendingDb->prb_baseline_deltas_from_merge);
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_delta)  );
        SG_ERR_CHECK(  sg_pendingdb__calc_parent_delta(
                    pCtx, 
                    pPendingDb->prb_records_add,
                    pPendingDb->prb_records_remove,
                    NULL,
                    NULL,
                    "add", "remove",
                    pvh_delta
                    )  );
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_delta, "attach_add", &pPendingDb->pvh_attach_add)  );
        SG_ERR_CHECK(  SG_committing__db__set_parent_delta(pCtx, ptx, "", &pvh_delta)  );
    }
    else if (1 == count_parents)
    {
        const char* psz_csid_parent = NULL;

        SG_ASSERT(!pPendingDb->prb_baseline_deltas_from_merge);
        SG_ERR_CHECK(  SG_rbtree__get_only_entry(pCtx, pPendingDb->prbParents, &psz_csid_parent, NULL)  );
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_delta)  );
        SG_ERR_CHECK(  sg_pendingdb__calc_parent_delta(
                    pCtx, 
                    pPendingDb->prb_records_add,
                    pPendingDb->prb_records_remove,
                    NULL,
                    NULL,
                    "add", "remove",
                    pvh_delta
                    )  );
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_delta, "attach_add", &pPendingDb->pvh_attach_add)  );
        SG_ERR_CHECK(  SG_committing__db__set_parent_delta(pCtx, ptx, psz_csid_parent, &pvh_delta)  );
    }
    else
    {
        const char* psz_csid_parent = NULL;
        struct sg_zing_merge_delta* pd = NULL;

        SG_ASSERT(pPendingDb->prb_baseline_deltas_from_merge);

        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pPendingDb->prb_baseline_deltas_from_merge, &b, &psz_csid_parent, (void**) &pd)  );
        while (b)
        {
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_delta)  );
            SG_ERR_CHECK(  sg_pendingdb__calc_parent_delta(
                        pCtx, 
                        pPendingDb->prb_records_add,
                        pPendingDb->prb_records_remove,
                        pd->prb_add,
                        pd->prb_remove,
                        "add", "remove",
                        pvh_delta
                        )  );
            // we don't add attach_add here because on a merge there usually
            // shouldn't be any.
            SG_ERR_CHECK(  SG_committing__db__set_parent_delta(pCtx, ptx, psz_csid_parent, &pvh_delta)  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid_parent, (void**) &pd)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

    // now store the attachments
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pPendingDb->prbAttachments_files, &b, &psz_hid_att, (void**) &pPath_att)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_att, SG_FILE_LOCK | SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile_att)  );

        /* TODO fix the b_dont_bother flag below according to the filename of this item? */
        SG_ERR_CHECK(  SG_committing__add_file(
                    pCtx,
                    ptx,
                    SG_FALSE,
                    pFile_att,
                    &pszActualHid)  );

        SG_FILE_NULLCLOSE(pCtx, pFile_att);
        /* TODO should we check to make sure actual hid is what it should be ? */
        SG_NULLFREE(pCtx, pszActualHid);

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_att, (void**) &pPath_att)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pPendingDb->prbAttachments_buffers, &b, NULL, (void**) &pPacked)  );
    while (b)
    {
        SG_uint32 len;
        const SG_byte* pData = NULL;

        SG_ERR_CHECK(  SG_memoryblob__unpack(pCtx, pPacked, &pData, &len)  );
        SG_ERR_CHECK(  SG_committing__add_bytes__buflen(
                    pCtx,
                    ptx,
                    pData,
                    len,
                    SG_FALSE,
                    &pszActualHid)  );
        /* TODO should we check to make sure actual hid is what it should be ? */
        SG_NULLFREE(pCtx, pszActualHid);

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, NULL, (void**) &prec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // now the template
    if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pPendingDb->iDagNum))
    {
        if (pPendingDb->pvh_template)
        {
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pPendingDb->pvh_template, pstr)  );
            SG_ERR_CHECK(  SG_committing__add_bytes__buflen(
                        pCtx,
                        ptx,
                        (SG_byte*) SG_string__sz(pstr),
                        SG_string__length_in_bytes(pstr),
                        SG_FALSE, // TODO really?
                        &psz_hid_template)  );
            SG_STRING_NULLFREE(pCtx, pstr);
            SG_ERR_CHECK(  SG_committing__db__set_template(pCtx, ptx, psz_hid_template)  );
            SG_NULLFREE(pCtx, psz_hid_template);
        }
        else
        {
            SG_ASSERT(pPendingDb->buf_hid_template[0]);
            SG_ERR_CHECK(  SG_committing__db__set_template(pCtx, ptx, pPendingDb->buf_hid_template)  );
        }
    }

    SG_NULLFREE(pCtx, psz_hid_delta);
    SG_STRING_NULLFREE(pCtx, pstr);

    SG_ERR_CHECK(  SG_committing__end(pCtx, ptx, &pcs, &pdn)  );
    ptx = NULL;

    SG_RETURN_AND_NULL(pcs, ppcs);
    SG_RETURN_AND_NULL(pdn, ppdn);

fail:
    if (ptx)
    {
        SG_ERR_IGNORE(  SG_committing__abort(pCtx, ptx)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh_dont_add);
    SG_RBTREE_NULLFREE(pCtx, prb_records_both_add_and_remove);
    SG_NULLFREE(pCtx, psz_hid_template);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VHASH_NULLFREE(pCtx, pvh_delta);
    SG_VHASH_NULLFREE(pCtx, pvh_rec);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}


