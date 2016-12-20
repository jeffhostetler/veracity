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

#ifdef WINDOWS
//#pragma warning(disable:4100)
#pragma warning(disable:4706)
#endif

/** This struct represents what one changeset has done to the record(s) for a
 * given recid.
 *
 * If it simply deleted the record, then only prec_deleted will
 * be set.
 *
 * If it added the record when it was previously not present, then
 * only prec_added will be set.
 *
 * If it modified the record, then both will be set.
 */
struct record_changes_from_one_changeset
{
    SG_dbrecord* prec_deleted;
    SG_dbrecord* prec_added;
};

struct changes_for_one_record
{
    struct record_changes_from_one_changeset leaves[2];
};

struct sg_zing_merge_situation
{
    SG_uint64 dagnum;
    const char* psz_hid_template_ancestor;
    const char* psz_hid_template[2];

    SG_int32 i_which_template;
    SG_zingtemplate* pztemplate;

    char* psz_hid_cs_ancestor;
    const SG_audit* pq;
    const char* parents[2];
    struct sg_zing_merge_delta deltas[2];
    SG_varray* audits[2];
    SG_rbtree* prb_keys;

    SG_rbtree* prb_pending_adds;
    SG_rbtree* prb_pending_mods;
    SG_rbtree* prb_pending_deletes;

    SG_rbtree* prb_conflicts__delete_mod;
    SG_rbtree* prb_conflicts__mod_mod;
    SG_rbtree* prb_conflicts__add_add;

    SG_vector* pvec_pcfor;
    SG_vector* pvec_dbrecords;
};

struct sg_zing_merge_situation__trivial
{
    char* psz_hid_cs_ancestor;
    const SG_audit* pq;
    const char* parents[2];
    struct sg_zing_merge_delta deltas[2];

    SG_rbtree* prb_pending_adds;
    SG_rbtree* prb_pending_deletes;
};

static void my_alloc_cfor(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    struct changes_for_one_record** ppcfor
    );

static void sg_zingmerge__most_recent(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_int16* pi_which
    );

static void sg_zingmerge__least_recent(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_int16* pi_which
    );

static void SG_zingmerge__load_records__rbtree(
    SG_context* pCtx,
    SG_rbtree* prb,
    SG_repo* pRepo,
    SG_vector* pvec
    )
{
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* pszHid_record = NULL;
    SG_dbrecord* prec = NULL;

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb, &b, &pszHid_record, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_dbrecord__load_from_repo(pCtx, pRepo, pszHid_record, &prec)  );
        SG_ERR_CHECK(  SG_vector__append(pCtx, pvec, prec, NULL)  );
        SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, prb, pszHid_record, (void*) prec, NULL)  );
        prec = NULL;

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &pszHid_record, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    return;
fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_DBRECORD_NULLFREE(pCtx, prec);
}

/** Given a list of records deleted and added in a single changeset, this
 * function matches up the records according by recid.
 */
static void sg_zing__connect_the_edits(
    SG_context* pCtx,
    SG_rbtree* prb_records_deleted,
    SG_rbtree* prb_records_added,
    struct sg_zing_merge_situation* pzms,
    SG_uint32 ileaf
    )
{
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    SG_dbrecord* prec = NULL;
    const char* psz_hid_record = NULL;
    struct changes_for_one_record* pcfor = NULL;

    /* First, review all the deletes */
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_records_deleted, &b, &psz_hid_record, (void**) &prec)  );
    while (b)
    {
        const char* psz_key = NULL;

        SG_ERR_CHECK(  SG_dbrecord__check_value(pCtx, prec, SG_ZING_FIELD__RECID, &psz_key)  );

        // TODO can't we assume this, since if this was NO_RECID then
        // we are in the wrong merge?
        if (psz_key)
        {
            SG_bool b_has = SG_FALSE;
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzms->prb_keys, psz_key, &b_has, (void**) &pcfor)  );
            if (!b_has)
            {
                SG_ERR_CHECK(  my_alloc_cfor(pCtx, pzms, &pcfor)  );
                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pzms->prb_keys, psz_key, pcfor)  );
            }

            pcfor->leaves[ileaf].prec_deleted = prec;
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, (void**) &prec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    /* Now all the adds */
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_records_added, &b, &psz_hid_record, (void**) &prec)  );
    while (b)
    {
        const char* psz_key = NULL;

        SG_ERR_CHECK(  SG_dbrecord__check_value(pCtx, prec, SG_ZING_FIELD__RECID, &psz_key)  );

        if (psz_key)
        {
            SG_bool b_has = SG_FALSE;
            SG_ERR_CHECK(  SG_rbtree__find(pCtx, pzms->prb_keys, psz_key, &b_has, (void**) &pcfor)  );
            if (!b_has)
            {
                SG_ERR_CHECK(  my_alloc_cfor(pCtx, pzms, &pcfor)  );
                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pzms->prb_keys, psz_key, pcfor)  );
            }

            pcfor->leaves[ileaf].prec_added = prec;
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_record, (void**) &prec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

fail:
    ;
}

static void my_free_cfor(SG_UNUSED_PARAM(SG_context* pCtx), void* p)
{
	SG_UNUSED(pCtx);

    SG_NULLFREE(pCtx, p);
}

static void my_alloc_cfor(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    struct changes_for_one_record** ppcfor
    )
{
    struct changes_for_one_record* p = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, p)  );
    SG_ERR_CHECK(  SG_vector__append(pCtx, pzms->pvec_pcfor, p, NULL)  );
    *ppcfor = p;
    p = NULL;

    // fall thru
fail:
    SG_NULLFREE(pCtx, p);
}

void SG_db__diff(
    SG_context* pCtx,
    SG_repo* pRepo,
	SG_uint64 iDagNum,
    const char* psz_csid_ancestor,
    const char* psz_csid,
    struct sg_zing_merge_delta* pzmd
    )
{
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;

    SG_rbtree* prb_add = NULL;
    SG_rbtree* prb_remove = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(psz_csid_ancestor);
    SG_NULLARGCHECK_RETURN(psz_csid);
    SG_NULLARGCHECK_RETURN(pzmd);

    SG_ERR_CHECK(  SG_repo__db__calc_delta(pCtx, pRepo, iDagNum, psz_csid_ancestor, psz_csid, 0, &pvh_add, &pvh_remove)  );

    if (pvh_add)
    {
        SG_ERR_CHECK(  SG_vhash__get_keys_as_rbtree(pCtx, pvh_add, &prb_add)  );
    }
    if (pvh_remove)
    {
        SG_ERR_CHECK(  SG_vhash__get_keys_as_rbtree(pCtx, pvh_remove, &prb_remove)  );
    }

    pzmd->prb_add = prb_add;
    pzmd->prb_remove = prb_remove;

    prb_add = NULL;
    prb_remove = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_add);
    SG_VHASH_NULLFREE(pCtx, pvh_remove);
    SG_RBTREE_NULLFREE(pCtx, prb_add);
    SG_RBTREE_NULLFREE(pCtx, prb_remove);
}

/** This function builds "prb_keys", an rbtree containing one entry for each
 * recid.  The assoc data for each entry is another rbtree
 * which contains a list of all the changes that were made which involve that
 * recid.  Our goal here is to collect all the record changes
 * from all of the leafs and group them by recid. */
static void sg_zing__collect_and_group_all_changes(
    SG_context* pCtx,
    SG_repo* pRepo,
	SG_uint64 iDagNum,
    struct sg_zing_merge_situation* pzms
    )
{
    SG_uint32 ileaf = 0;

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pzms->prb_keys)  );

    for (ileaf=0; ileaf<2; ileaf++)
    {
        /* Diff it */
        SG_ERR_CHECK(  SG_db__diff(
                    pCtx,
                    pRepo,
                    iDagNum,
                    pzms->psz_hid_cs_ancestor,
                    pzms->parents[ileaf],
                    &pzms->deltas[ileaf]
                    )  );

        /* Fetch all the records that were deleted/added */
        SG_ERR_CHECK(  SG_zingmerge__load_records__rbtree(pCtx, pzms->deltas[ileaf].prb_remove, pRepo, pzms->pvec_dbrecords)  );
        SG_ERR_CHECK(  SG_zingmerge__load_records__rbtree(pCtx, pzms->deltas[ileaf].prb_add, pRepo, pzms->pvec_dbrecords)  );

        /* Match things up by recid */
        SG_ERR_CHECK(  sg_zing__connect_the_edits(pCtx, pzms->deltas[ileaf].prb_remove, pzms->deltas[ileaf].prb_add, pzms, ileaf)  );
    }

    // fall thru

fail:
    return;
}

static void sg_zingmerge__resolve_commit_errors(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_zingtx* pztx,
    SG_varray* pva_errors,
    SG_vhash** ppvh_journal_entries
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_zingtemplate* pzt = NULL;
    SG_vhash* pvh_journal_entries = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_journal_entries)  );
    SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_errors, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_err_type = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_errors, i, &pvh)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_CONSTRAINT_VIOLATION__TYPE, &psz_err_type)  );

        if (0 == strcmp(psz_err_type, "unique"))
        {
            const char* psz_field_name = NULL;
            const char* psz_rectype = NULL;
            SG_zingfieldattributes* pzfa = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_CONSTRAINT_VIOLATION__RECTYPE, &psz_rectype)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_CONSTRAINT_VIOLATION__FIELD_NAME, &psz_field_name)  );
            SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, psz_rectype, psz_field_name, &pzfa)  );
            SG_ASSERT(SG_ZING_TYPE__STRING == pzfa->type);

            SG_ERR_CHECK(  sg_zing_uniqify_string__do(
                        pCtx,
                        pvh,
                        pzfa->v._string.uniqify,
                        pztx,
                        pzfa,
                        pzms->pq,
                        pzms->parents,
                        pzms->psz_hid_cs_ancestor,
                        pvh_journal_entries
                        )  );
        }
        else
        {
            // TODO when can this happen?  a commit error on a merge, but
            // not related to a unique constraint?  FELIX  Perhaps an
            // automerge sum exceeded a max?

            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }
    }

    *ppvh_journal_entries = pvh_journal_entries;
    pvh_journal_entries = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_journal_entries);
}

/** This function takes the prb_keys data structure and figures out if there
 * are any conflicts.
 * */
static void sg_zingmerge__check_for_conflicts(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms
    )
{
    SG_rbtree* prb_pending_adds = NULL;
    SG_rbtree* prb_pending_mods = NULL;
    SG_rbtree* prb_pending_deletes = NULL;
    SG_rbtree* prb_conflicts__delete_mod = NULL;
    SG_rbtree* prb_conflicts__mod_mod = NULL;
    SG_rbtree* prb_conflicts__add_add = NULL;
    SG_rbtree_iterator* pit_keys = NULL;
    SG_bool b_keys;
    const char* psz_key = NULL;
    struct changes_for_one_record* pcfor = NULL;

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_pending_adds)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_pending_mods)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_pending_deletes)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_conflicts__delete_mod)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_conflicts__mod_mod)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_conflicts__add_add)  );

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit_keys, pzms->prb_keys, &b_keys, &psz_key, (void**) &pcfor)  );
    while (b_keys)
    {
        if (
            (pcfor->leaves[0].prec_added || pcfor->leaves[0].prec_deleted)
            && (pcfor->leaves[1].prec_added || pcfor->leaves[1].prec_deleted)
           )
        {
            /* Both leaves were involved, so we MIGHT have a conflict.  We
             * don't know yet.  We need to review all the changes and see. */

            SG_uint32 count_delete = 0;
            SG_uint32 count_modify = 0;
            SG_uint32 count_add = 0;
            SG_uint32 ileaf = 0;
            SG_bool b_different = SG_FALSE;

            for (ileaf=0; ileaf<2; ileaf++)
            {
                if (pcfor->leaves[ileaf].prec_added)
                {
                    /* if prec_added is set, then this changeset either
                     * modified the record or added it anew. */
                    if (pcfor->leaves[ileaf].prec_deleted)
                    {
                        count_modify++;
                    }
                    else
                    {
                        count_add++;
                    }
                }
                else
                {
                    /* if this changeset did not contribute a new version of
                     * the record, it must have simply deleted it. */
                    SG_ASSERT(pcfor->leaves[ileaf].prec_deleted);
                    count_delete++;
                }
            }

            /* either the recid was present in the ancestor or it was
             * not.  It is not possible for one leaf to have deleted the record
             * and another leaf to have added it anew. */
            SG_ASSERT(!(count_delete && count_add));

            /* it is therefore also not possible for one leaf to have modified
             * the record and another leaf to have added it anew. */
            SG_ASSERT(!(count_modify && count_add));

            if (
                    (count_add > 1)
                    || (count_modify > 1)
               )
            {
                const char* psz_hid_0 = NULL;
                const char* psz_hid_1 = NULL;

                SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[0].prec_added, &psz_hid_0)  );
                SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[1].prec_added, &psz_hid_1)  );
                if (0 != strcmp(psz_hid_0, psz_hid_1))
                {
                    b_different = SG_TRUE;
                }
            }

            if (count_delete && count_modify)
            {
                /* if one changeset modified the record and another one deletes
                 * it, we just report this conflict up and let the caller decide
                 * what to do with it.
                 * */

                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_conflicts__delete_mod, psz_key, pcfor)  );
            }
            else if (count_delete)
            {
                /* More than one leaf has deleted this record, but nobody tried
                 * to modify it. So we're all in agreement that the record
                 * should go away. */

                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_pending_deletes, psz_key, pcfor->leaves[0].prec_deleted)  );
            }
            else if (count_add)
            {
                SG_ASSERT(!count_modify);
                SG_ASSERT(2 == count_add);

                // how is this possible?  Multiple changesets
                // added the same record?  But the recid is a gid!
                // Can it happen in a merge case?  OK, but why didn't
                // the LCA code find a node before it?

                if (b_different)
                {
                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_conflicts__add_add, psz_key, pcfor)  );
                }
                else
                {
                    /* whew!  that was close.  it turns out that both who
                     * added this record did it in exactly the same way. */

                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_pending_adds, psz_key, pcfor->leaves[0].prec_added)  );
                }
            }
            else
            {
                SG_ASSERT(2 == count_modify);

                if (b_different)
                {
                    /* yep, we need a record-level merge */

                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_conflicts__mod_mod, psz_key, pcfor)  );
                }
                else
                {
                    /* whew!  that was close.  it turns out that both who
                     * modified this record did it in exactly the same way. */

                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_pending_mods, psz_key, &pcfor->leaves[0])  );
                }
            }
        }
        else
        {
            /* The trivial case.  Only one changeset involved with this key value. */

            SG_uint32 ileaf = 0;

            if (pcfor->leaves[0].prec_added || pcfor->leaves[0].prec_deleted)
            {
                ileaf = 0;
            }
            else
            {
                ileaf = 1;
            }

            if (pcfor->leaves[ileaf].prec_added
                    && !pcfor->leaves[ileaf].prec_deleted)
            {
                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_pending_adds, psz_key, pcfor->leaves[ileaf].prec_added)  );
            }
            if (!pcfor->leaves[ileaf].prec_added
                    && pcfor->leaves[ileaf].prec_deleted)
            {
                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_pending_deletes, psz_key, pcfor->leaves[ileaf].prec_deleted)  );
            }
            if (pcfor->leaves[ileaf].prec_added
                    && pcfor->leaves[ileaf].prec_deleted)
            {
                SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_pending_mods, psz_key, &pcfor->leaves[ileaf])  );
            }
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit_keys, &b_keys, &psz_key, (void**) &pcfor)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit_keys);

    pzms->prb_pending_adds = prb_pending_adds;
    pzms->prb_pending_mods = prb_pending_mods;
    pzms->prb_pending_deletes = prb_pending_deletes;
    pzms->prb_conflicts__delete_mod = prb_conflicts__delete_mod;
    pzms->prb_conflicts__mod_mod = prb_conflicts__mod_mod;
    pzms->prb_conflicts__add_add = prb_conflicts__add_add;

    prb_pending_adds = NULL;
    prb_pending_mods = NULL;
    prb_pending_deletes = NULL;
    prb_conflicts__delete_mod = NULL;
    prb_conflicts__mod_mod = NULL;
    prb_conflicts__add_add = NULL;

    // fall thru

fail:
    SG_RBTREE_NULLFREE(pCtx, prb_pending_adds);
    SG_RBTREE_NULLFREE(pCtx, prb_pending_deletes);
    SG_RBTREE_NULLFREE(pCtx, prb_pending_mods);
    SG_RBTREE_NULLFREE(pCtx, prb_conflicts__delete_mod);
    SG_RBTREE_NULLFREE(pCtx, prb_conflicts__mod_mod);
    SG_RBTREE_NULLFREE(pCtx, prb_conflicts__add_add);
}

static void sg_zing_merge__try_auto__record(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_vhash* pvh_auto,
    SG_int16* pi_which
    )
{
    SG_int16 which = -1;
    const char* psz_op = NULL;
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_auto, SG_ZING_TEMPLATE__OP, &psz_op)  );

    if (0 == strcmp(psz_op, SG_ZING_MERGE__most_recent))
    {
        SG_ERR_CHECK(  sg_zingmerge__most_recent(pCtx, pzms, &which)  );
        if (
                (0 == which)
                || (1 == which)
           )
        {
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__least_recent))
    {
        SG_ERR_CHECK(  sg_zingmerge__least_recent(pCtx, pzms, &which)  );
        if (
                (0 == which)
                || (1 == which)
           )
        {
        }
        else
        {
            goto unresolved;
        }
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
                (pCtx, "unimplemented automerge op: %s", psz_op)
                );
    }

    *pi_which = which;

    goto done;

unresolved:
    *pi_which = -1;

done:
fail:
    ;
}

static void sg_zing_merge__do_auto_array__record(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_varray* pva_autos,
    SG_int16* pi_which
    )
{
    SG_uint32 count_autos = 0;
    SG_uint32 i_auto = 0;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_autos, &count_autos)  );
    for (i_auto=0; i_auto<count_autos; i_auto++)
    {
        SG_vhash* pvh_auto = NULL;
        SG_int16 which = -1;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_autos, i_auto, &pvh_auto)  );
        SG_ERR_CHECK(  sg_zing_merge__try_auto__record(pCtx, pzms, pvh_auto, &which)  );

        if (which >= 0)
        {
            *pi_which = which;
            return;
        }
    }

    *pi_which = -1;

fail:
    ;
}

static void sg_zingmerge__add_add(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_vhash* pvh_journal_entries
    )
{
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_recid = NULL;
    SG_zingrectypeinfo* pzmi = NULL;

    SG_UNUSED(pvh_journal_entries); // TODO

    if (pzms->prb_conflicts__add_add)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_rbtree__count(pCtx, pzms->prb_conflicts__add_add, &count)  );
        if (count)
        {
            struct changes_for_one_record* pcfor = NULL;

            SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pzms->prb_conflicts__add_add, &b, &psz_recid, (void**) &pcfor)  );
            while (b)
            {
                SG_varray* pva_automerge = NULL;
                SG_int16 which = -1;
                const char* psz_rectype = NULL;

                if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pzms->dagnum))
                {
                    SG_ERR_CHECK_RETURN(  sg_zingtemplate__get_only_rectype(pCtx, pzms->pztemplate, &psz_rectype, NULL)  );
                }
                else
                {
                    SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pcfor->leaves[0].prec_added, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
                }
                SG_ERR_CHECK(  SG_zingtemplate__get_rectype_info(pCtx, pzms->pztemplate, psz_rectype, &pzmi)  );
                SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pzmi->pvh_merge, SG_ZING_TEMPLATE__AUTO, &pva_automerge)  );
                SG_ERR_CHECK(  sg_zing_merge__do_auto_array__record(pCtx, pzms, pva_automerge, &which)  );
                SG_ZINGRECTYPEINFO_NULLFREE(pCtx, pzmi);
                if (which >= 0)
                {
                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pzms->prb_pending_adds, psz_recid, pcfor->leaves[which].prec_added)  );

#if 0
                    SG_ERR_CHECK(  sg_zingmerge__add_log_entry(pCtx, pzms->pva_log,
                                "recid", psz_recid,
                                "reason", "took most recent in add/add",
                                NULL) );
#endif
                }
                else
                {
                    // TODO throw?
                }

                SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**) &pcfor)  );
            }
            SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
        }
    }

fail:
    SG_ZINGRECTYPEINFO_NULLFREE(pCtx, pzmi);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

static void sg_zingmerge__delete_mod(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_vhash* pvh_journal_entries
    )
{
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_recid = NULL;
    SG_zingrectypeinfo* pzmi = NULL;

    SG_UNUSED(pvh_journal_entries); // TODO

    if (pzms->prb_conflicts__delete_mod)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_rbtree__count(pCtx, pzms->prb_conflicts__delete_mod, &count)  );
        if (count)
        {
            struct changes_for_one_record* pcfor = NULL;

            SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pzms->prb_conflicts__delete_mod, &b, &psz_recid, (void**) &pcfor)  );
            while (b)
            {
                SG_varray* pva_automerge = NULL;
                SG_int16 which = -1;
                const char* psz_rectype = NULL;

                if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pzms->dagnum))
                {
                    SG_ERR_CHECK_RETURN(  sg_zingtemplate__get_only_rectype(pCtx, pzms->pztemplate, &psz_rectype, NULL)  );
                }
                else
                {
                    if (pcfor->leaves[0].prec_added)
                    {
                        SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pcfor->leaves[0].prec_added, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pcfor->leaves[1].prec_added, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
                    }
                }
                SG_ERR_CHECK(  SG_zingtemplate__get_rectype_info(pCtx, pzms->pztemplate, psz_rectype, &pzmi)  );
                SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pzmi->pvh_merge, SG_ZING_TEMPLATE__AUTO, &pva_automerge)  );
                SG_ERR_CHECK(  sg_zing_merge__do_auto_array__record(pCtx, pzms, pva_automerge, &which)  );
                SG_ZINGRECTYPEINFO_NULLFREE(pCtx, pzmi);
                if (which >= 0)
                {
                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pzms->prb_pending_deletes, psz_recid, pcfor->leaves[0].prec_deleted)  );

                    if (pcfor->leaves[which].prec_added)
                    {
                        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pzms->prb_pending_adds, psz_recid, pcfor->leaves[which].prec_added)  );
                    }
                    // TODO log
#if 0
                    SG_ERR_CHECK(  sg_zingmerge__add_log_entry(pCtx, pzms->pva_log,
                                "recid", psz_recid,
                                "reason", "took most recent in add/add",
                                NULL) );
#endif
                }
                else
                {
                    // TODO throw?
                }
                SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**) &pcfor)  );
            }
            SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
        }
    }

fail:
    SG_ZINGRECTYPEINFO_NULLFREE(pCtx, pzmi);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    ;
}

static void sg_zingmerge__get_timestamps(
    SG_context* pCtx,
    SG_varray* pva,
    SG_int64* pimin,
    SG_int64* pimax
    )
{
    SG_uint32 i;
    SG_int64 imin = 0;
    SG_int64 imax = 0;
    SG_uint32 count = 0;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        SG_int64 itime = -1;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, SG_AUDIT__TIMESTAMP, &itime)  );

        if (0 == i)
        {
            imin = itime;
            imax = itime;
        }
        else
        {
            imin = SG_MIN(imin, itime);
            imax = SG_MAX(imax, itime);
        }

        i++;
    }

    if (pimin)
    {
        *pimin = imin;
    }
    if (pimax)
    {
        *pimax = imax;
    }

    // fall thru

fail:
    return;
}

static void sg_zingmerge__least_recent(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_int16* pi_which
    )
{
    SG_int64 times[2];

    times[0] = 0;
    times[1] = 0;

    SG_ERR_CHECK(  sg_zingmerge__get_timestamps(pCtx, pzms->audits[0], &times[0], NULL)  );
    SG_ERR_CHECK(  sg_zingmerge__get_timestamps(pCtx, pzms->audits[1], &times[1], NULL)  );

    if (times[0] < times[1])
    {
        *pi_which = 0;
    }
    else if (times[0] > times[1])
    {
        *pi_which = 1;
    }
    else
    {
        // this should never happen.  coding for it is pathological.
        *pi_which = -1;
    }

    // fall thru

fail:
    return;
}

static void sg_zingmerge__most_recent(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_int16* pi_which
    )
{
    SG_int64 times[2];

    times[0] = 0;
    times[1] = 0;

    SG_ERR_CHECK(  sg_zingmerge__get_timestamps(pCtx, pzms->audits[0], NULL, &times[0])  );
    SG_ERR_CHECK(  sg_zingmerge__get_timestamps(pCtx, pzms->audits[1], NULL, &times[1])  );

    if (times[0] > times[1])
    {
        *pi_which = 0;
    }
    else if (times[0] < times[1])
    {
        *pi_which = 1;
    }
    else
    {
        // this should never happen.  coding for it is pathological.
        *pi_which = -1;
    }

    // fall thru

fail:
    return;
}

static void _sg_zing_merge_field_concat(
	SG_context *pCtx,
	const char *psz_vals[2],
	const char *psz_lca_val,
	SG_dbrecord *dbrec,
	const char *psz_field_name)
{
	SG_tempfile *handles[4] = {0,0,0,0};
	SG_uint32 i;
	SG_textfilediff_t *pDiff = NULL;
	SG_bool bHadConflicts = SG_FALSE;
	SG_string *dummy = NULL;

	SG_UNUSED(dbrec);
	SG_UNUSED(psz_field_name);

	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &handles[0])  );
	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &handles[1])  );
	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &handles[2])  );

	SG_ERR_CHECK(  SG_file__write__sz(pCtx, handles[0]->pFile, psz_vals[0])  );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, handles[1]->pFile, psz_vals[1])  );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, handles[2]->pFile, psz_lca_val)  );

	SG_ERR_CHECK(  SG_tempfile__close(pCtx, handles[0])  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, handles[1])  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, handles[2])  );

	SG_ERR_CHECK(  SG_textfilediff3(pCtx, handles[2]->pPathname, handles[0]->pPathname, handles[1]->pPathname,
									SG_TEXTFILEDIFF_OPTION__IGNORE_WHITESPACE, &pDiff, &bHadConflicts)
		);

	if (bHadConflicts)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &dummy, psz_vals[0])  );

		SG_ERR_CHECK(  SG_string__append__sz(pCtx, dummy, "\n== ZING_MERGE_CONCAT ==\n")  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, dummy, psz_vals[1])  );
					  
		SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, dbrec, psz_field_name, SG_string__sz(dummy))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &dummy)  );
		SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &handles[3])  );

		SG_ERR_CHECK(  SG_textfilediff3__output__file(pCtx, pDiff, dummy, dummy, handles[3]->pFile)  );
		SG_ERR_CHECK(  SG_tempfile__close(pCtx, handles[3])  );

		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, handles[3]->pPathname, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &handles[3]->pFile)   );

		SG_ERR_CHECK(  SG_file__read_utf8_file_into_string(pCtx, handles[3]->pFile, dummy)  );
		SG_ERR_CHECK(  SG_tempfile__close(pCtx, handles[3])  );

		SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, dbrec, psz_field_name, SG_string__sz(dummy))  );
	}

fail:
	for ( i = 0; i < 4; ++i )
		SG_tempfile__close_and_delete(pCtx, &handles[i]);
	SG_textfilediff__free(pCtx, pDiff);
	SG_STRING_NULLFREE(pCtx, dummy);
}

static void sg_zing_merge__try_auto__field(
    SG_context* pCtx,
    const char* psz_recid,
    struct sg_zing_merge_situation* pzms,
    const char* psz_field_name,
    SG_dbrecord* pdbrec,
    const char* psz_vals[2],
	const char *psz_lca_val,
    SG_uint16 type,
    SG_vhash* pvh_auto,
    SG_bool* pb_resolved,
    SG_vhash* pvh_journal_entries
    )
{
    const char* psz_op = NULL;
    SG_int16 which = -1;
    SG_bool b_has = SG_FALSE;
    SG_vhash* pvh_entry = NULL;
    SG_string* pstr_expanded = NULL;

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_auto, SG_ZING_TEMPLATE__OP, &psz_op)  );

    if (0 == strcmp(psz_op, SG_ZING_MERGE__most_recent))
    {
        SG_ERR_CHECK(  sg_zingmerge__most_recent(pCtx, pzms, &which)  );
        if (
                (0 == which)
                || (1 == which)
           )
        {
            if (psz_vals[which])
            {
                SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
            }
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__least_recent))
    {
        SG_ERR_CHECK(  sg_zingmerge__least_recent(pCtx, pzms, &which)  );
        if (
                (0 == which)
                || (1 == which)
           )
        {
            if (psz_vals[which])
            {
                SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
            }
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__longest))
    {
        if (SG_ZING_TYPE__STRING == type)
        {
            SG_uint32 len0 = SG_STRLEN(psz_vals[0]);
            SG_uint32 len1 = SG_STRLEN(psz_vals[1]);

            if (len0 > len1)
            {
                which = 0;
            }
            else if (len1 > len0)
            {
                which = 1;
            }
            else
            {
                which = -1;
            }
        }
        if (
                (0 == which)
                || (1 == which)
           )
        {
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__shortest))
    {
        if (SG_ZING_TYPE__STRING == type)
        {
            SG_uint32 len0 = SG_STRLEN(psz_vals[0]);
            SG_uint32 len1 = SG_STRLEN(psz_vals[1]);

            if (len0 < len1)
            {
                which = 0;
            }
            else if (len1 < len0)
            {
                which = 1;
            }
            else
            {
                which = -1;
            }
        }
        if (
                (0 == which)
                || (1 == which)
           )
        {
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__concat))
    {
		if (SG_ZING_TYPE__STRING == type)
			SG_ERR_CHECK(  _sg_zing_merge_field_concat(pCtx, psz_vals, psz_lca_val, pdbrec, psz_field_name)  );
		else
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__allowed_last))
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__allowed_first))
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__max))
    {
        if (
                (SG_ZING_TYPE__INT == type)
                || (SG_ZING_TYPE__DATETIME == type)
           )
        {
            SG_int64 i_vals[2];
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[0], psz_vals[0])  );
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[1], psz_vals[1])  );
            if (i_vals[0] > i_vals[1])
            {
                which = 0;
            }
            else
            {
                which = 1;
            }
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__min))
    {
        if (
                (SG_ZING_TYPE__INT == type)
                || (SG_ZING_TYPE__DATETIME == type)
           )
        {
            SG_int64 i_vals[2];
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[0], psz_vals[0])  );
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[1], psz_vals[1])  );
            if (i_vals[0] < i_vals[1])
            {
                which = 0;
            }
            else
            {
                which = 1;
            }
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__average))
    {
        if (
                (SG_ZING_TYPE__INT == type)
                || (SG_ZING_TYPE__DATETIME == type)
           )
        {
            SG_int64 i_vals[2];
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[0], psz_vals[0])  );
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[1], psz_vals[1])  );
            SG_ERR_CHECK(  SG_dbrecord__add_pair__int64(pCtx, pdbrec, psz_field_name, (i_vals[0] + i_vals[1]) / 2)  );
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__sum))
    {
        if (
                (SG_ZING_TYPE__INT == type)
                || (SG_ZING_TYPE__DATETIME == type)
           )
        {
            SG_int64 i_vals[2];
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[0], psz_vals[0])  );
            SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &i_vals[1], psz_vals[1])  );
            SG_ERR_CHECK(  SG_dbrecord__add_pair__int64(pCtx, pdbrec, psz_field_name, (i_vals[0] + i_vals[1]))  );
        }
        else
        {
            goto unresolved;
        }
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__allowed_last))
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else if (0 == strcmp(psz_op, SG_ZING_MERGE__allowed_first))
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
                (pCtx, "unimplemented automerge op: %s", psz_op)
                );
    }

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_auto, SG_ZING_TEMPLATE__JOURNAL, &b_has)  );
    if (b_has)
    {
        SG_vhash* pvh_j_fields = NULL;
        SG_vhash* pvh_journal = NULL;
        const char* psz_rectype = NULL;
        SG_uint32 count_fields = 0;
        SG_uint32 i_field = 0;

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_auto, SG_ZING_TEMPLATE__JOURNAL, &pvh_journal)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_journal, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_journal, SG_ZING_TEMPLATE__FIELDS, &pvh_j_fields)  );

        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_entry)  );

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_j_fields, &count_fields)  );
        for (i_field=0; i_field<count_fields; i_field++)
        {
            const char* psz_journal_field_name = NULL;
            const char* psz_journal_field_value = NULL;
            const char* psz_merged_value = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_j_fields, i_field, &psz_journal_field_name, &psz_journal_field_value)  );

            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_expanded, psz_journal_field_value)  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__MERGE__RECID,SG_STRLEN(SG_JOURNAL_MAGIC__MERGE__RECID),
                (const SG_byte *)psz_recid,SG_STRLEN(psz_recid),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__MERGE__FIELD_NAME,SG_STRLEN(SG_JOURNAL_MAGIC__MERGE__FIELD_NAME),
                (const SG_byte *)psz_field_name,SG_STRLEN(psz_field_name),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__MERGE__OP,SG_STRLEN(SG_JOURNAL_MAGIC__MERGE__OP),
                (const SG_byte *)psz_op,SG_STRLEN(psz_op),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            if (psz_vals[0])
            {
                SG_ERR_CHECK( SG_string__replace_bytes(
                    pCtx,
                    pstr_expanded,
                    (const SG_byte *)SG_JOURNAL_MAGIC__MERGE__VAL0,SG_STRLEN(SG_JOURNAL_MAGIC__MERGE__VAL0),
                    (const SG_byte *)psz_vals[0],SG_STRLEN(psz_vals[0]),
                    SG_UINT32_MAX,SG_TRUE,
                    NULL
                    )  );
            }
            else
            {
                // TODO replace with something else?  We don't want to just leave
                // %VAL0% in the record.
            }

            if (psz_vals[1])
            {
                SG_ERR_CHECK( SG_string__replace_bytes(
                    pCtx,
                    pstr_expanded,
                    (const SG_byte *)SG_JOURNAL_MAGIC__MERGE__VAL1,SG_STRLEN(SG_JOURNAL_MAGIC__MERGE__VAL1),
                    (const SG_byte *)psz_vals[1],SG_STRLEN(psz_vals[1]),
                    SG_UINT32_MAX,SG_TRUE,
                    NULL
                    )  );
            }
            else
            {
                // TODO replace with something else?
            }

            SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pdbrec, psz_field_name, &psz_merged_value)  );
            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__MERGE__MERGED_VALUE,SG_STRLEN(SG_JOURNAL_MAGIC__MERGE__MERGED_VALUE),
                (const SG_byte *)psz_merged_value,SG_STRLEN(psz_merged_value),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_entry, psz_journal_field_name, SG_string__sz(pstr_expanded))  );
            SG_STRING_NULLFREE(pCtx, pstr_expanded);
        }

        {
            SG_varray* pva_entries = NULL;
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_journal_entries, psz_rectype, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_journal_entries, psz_rectype, &pva_entries)  );
            }
            else
            {
                SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_journal_entries, psz_rectype, &pva_entries)  );
            }

            SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva_entries, &pvh_entry)  );
        }

    }

    *pb_resolved = SG_TRUE;

    goto done;

unresolved:
    *pb_resolved = SG_FALSE;

done:
fail:
    SG_VHASH_NULLFREE(pCtx, pvh_entry);
    SG_STRING_NULLFREE(pCtx, pstr_expanded);
}

static void sg_zing_merge__do_auto_array__field(
    SG_context* pCtx,
    const char* psz_recid,
    struct sg_zing_merge_situation* pzms,
    const char* psz_field_name,
    SG_dbrecord* pdbrec,
    const char* psz_vals[2],
	const char* psz_lca_val,
    SG_uint16 type,
    SG_varray* pva_autos,
    SG_bool* pb_resolved,
    SG_vhash* pvh_journal_entries
    )
{
    SG_uint32 count_autos = 0;
    SG_uint32 i_auto = 0;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_autos, &count_autos)  );
    for (i_auto=0; i_auto<count_autos; i_auto++)
    {
        SG_vhash* pvh_auto = NULL;
        SG_bool b_resolved = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_autos, i_auto, &pvh_auto)  );
        SG_ERR_CHECK(  sg_zing_merge__try_auto__field(pCtx, psz_recid, pzms, psz_field_name, pdbrec, psz_vals, psz_lca_val, type, pvh_auto, &b_resolved, pvh_journal_entries)  );

        if (b_resolved)
        {
            *pb_resolved = SG_TRUE;
            return;
        }
    }

    *pb_resolved = SG_FALSE;

fail:
    ;
}

static void sg_zing__merge_record_by_field(
    SG_context* pCtx,
    const char* psz_recid,
    struct sg_zing_merge_situation* pzms,
    SG_zingtemplate* pzt,
    const char* psz_rectype,
    SG_dbrecord* pdbrec,
    SG_zingrectypeinfo* pzmi,
    struct changes_for_one_record* pcfor,
    SG_vhash* pvh_journal_entries
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_zingfieldattributes* pzfa = NULL;
    SG_stringarray* psa_fields = NULL;
    const char* psz_field_name = NULL;

    // load both dagnodes
    // get the rectype
    // the lca version of the record should be in prec_deleted
    // iterate over all the fields in the template
    // if a field has no conflict, just take the change
    // for each one that conflicts, check its merge rules
    // in the end construct a new dbrecord add add it to prb_pending_mods

    SG_ERR_CHECK(  SG_zingtemplate__list_fields(pCtx, pzt, psz_rectype, &psa_fields)  );

    SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_fields, &count )  );
    for (i=0; i<count; i++)
    {
        const char* psz_val_lca = NULL;
        const char* psz_vals[2];
        SG_bool b_changed_val[2];
        const char* psz_hid_lca = NULL;
        const char* psz_hid_leaves[2];
        SG_uint32 count_bools = 0;

        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_fields, i, &psz_field_name)  );

        b_changed_val[0] = SG_FALSE;
        b_changed_val[1] = SG_FALSE;
        psz_vals[0] = NULL;
        psz_vals[1] = NULL;
        psz_hid_leaves[0] = NULL;
        psz_hid_leaves[1] = NULL;

        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[0].prec_deleted, &psz_hid_lca)  );
        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[0].prec_added, &psz_hid_leaves[0])  );
        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[1].prec_added, &psz_hid_leaves[1])  );

        // get all three values of the field
        SG_ERR_CHECK(  SG_dbrecord__check_value(pCtx, pcfor->leaves[0].prec_deleted, psz_field_name, &psz_val_lca)  );
        SG_ERR_CHECK(  SG_dbrecord__check_value(pCtx, pcfor->leaves[0].prec_added, psz_field_name, &psz_vals[0])  );
        SG_ERR_CHECK(  SG_dbrecord__check_value(pCtx, pcfor->leaves[1].prec_added, psz_field_name, &psz_vals[1])  );

        count_bools = 0;
        if (psz_val_lca)
        {
            count_bools++;
        }
        if (psz_vals[0])
        {
            count_bools++;
            if (psz_val_lca)
            {
                b_changed_val[0] = (0 != strcmp(psz_val_lca, psz_vals[0]));
            }
            else
            {
                b_changed_val[0] = SG_TRUE;
            }
        }
        if (psz_vals[1])
        {
            count_bools++;
            if (psz_val_lca)
            {
                b_changed_val[1] = (0 != strcmp(psz_val_lca, psz_vals[1]));
            }
            else
            {
                b_changed_val[1] = SG_TRUE;
            }
        }

        if (
                (3 == count_bools)
                && (0 == strcmp(psz_vals[0], psz_vals[1]))
           )
        {
            // if the two leaves agree, we're fine.  no conflict.
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[0])  );
        }
        else if (
                (2 == count_bools)
                && !psz_val_lca
                && (0 == strcmp(psz_vals[0], psz_vals[1]))
           )
        {
            // if the two leaves agree, we're fine.  no conflict.
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[0])  );
        }
        else if (0 == count_bools)
        {
            // nobody has this field.  no worries.
        }
        else if (1 == count_bools)
        {
            if (psz_val_lca)
            {
                // the LCA had the field, but both leaves removed it.
                // no problem
            }
            else
            {
                // exactly one of the leaves has added the field
                if (psz_vals[0])
                {
                    SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[0])  );
                }
                else
                {
                    SG_ASSERT(psz_vals[1]);
                    SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[1])  );
                }
            }
        }
        else if (
                (2 == count_bools)
                && psz_val_lca
                && psz_vals[0]
                && !b_changed_val[0]
                )
        {
            // leaf 1 removed it
            //
            // but leaf 0 didn't change it.  no conflict.
            // just accept the remove from leaf 1 by not
            // adding the field to our new copy
        }
        else if (
                (2 == count_bools)
                && psz_val_lca
                && psz_vals[1]
                && !b_changed_val[1]
                )
        {
            // leaf 0 removed it
            //
            // but leaf 1 didn't change it.  no conflict.
            // just accept the remove from leaf 0 by not
            // adding the field to our new copy
        }
        else if (
                (3 == count_bools)
                && b_changed_val[0]
                && !b_changed_val[1]
                )
        {
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[0])  );
        }
        else if (
                (3 == count_bools)
                && !b_changed_val[0]
                && b_changed_val[1]
                )
        {
            SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[1])  );
        }
        else
        {
            SG_bool b_resolved = SG_FALSE;

            // EITHER
            //     2 == count_bools, in which case one 
            //     of the leaves changed it and the other one deleted it
            // OR
            //     3 == count_bools, and both leaves changed it
            // OR
            //     2 == count_bools, psz_val_lca is NULL, and both leaves added it differently

            SG_ASSERT(count_bools >= 2);
            SG_ASSERT(
                       ( (2 == count_bools) && psz_val_lca && b_changed_val[0] && !b_changed_val[1])
                    || ( (2 == count_bools) && psz_val_lca && !b_changed_val[0] && b_changed_val[1])
                    || ( (2 == count_bools) && !psz_val_lca )
                    || ( (3 == count_bools) && psz_val_lca && b_changed_val[0] && b_changed_val[1])
                    );

            // see if the template has a way of resolving this conflict
            SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, psz_rectype, psz_field_name, &pzfa)  );

            if (pzfa->pva_automerge)
            {
                SG_ERR_CHECK(  sg_zing_merge__do_auto_array__field(pCtx, psz_recid, pzms, psz_field_name, pdbrec, psz_vals, psz_val_lca, pzfa->type, pzfa->pva_automerge, &b_resolved, pvh_journal_entries)  );
            }

            if (!b_resolved)
            {
                SG_varray* pva_automerge = NULL;

                SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pzmi->pvh_merge, SG_ZING_TEMPLATE__AUTO, &pva_automerge)  );
                SG_ERR_CHECK(  sg_zing_merge__do_auto_array__field(pCtx, psz_recid, pzms, psz_field_name, pdbrec, psz_vals, psz_val_lca, pzfa->type, pva_automerge, &b_resolved, pvh_journal_entries)  );
            }

            if (!b_resolved)
            {
                // force it by keeping most recent.  This should never happen,
                // because the template is supposed to provide fallbacks.
                SG_int16 which = -1;
                SG_ERR_CHECK(  sg_zingmerge__most_recent(pCtx, pzms, &which)  );
                if (which < 0)
                {
                    which = 0;
                }
                if (psz_vals[which])
                {
                    SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec, psz_field_name, psz_vals[which])  );
                }
                // we do not journal this case.
            }
        }
    }

    // fall thru

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa_fields);
}

static void sg_zingmerge__mod_mod__one_record(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    const char* psz_recid,
    struct changes_for_one_record* pcfor,
    SG_bool* pb,
    SG_vhash* pvh_journal_entries
    )
{
    const char* psz_rectype = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingrectypeinfo* pzmi = NULL;
    SG_dbrecord* pdbrec_freeme = NULL;
    SG_dbrecord* pdbrec_keepme = NULL;
    struct changes_for_one_record* my_pcfor = NULL;

    SG_ASSERT(pcfor);
    SG_ASSERT(pcfor->leaves[0].prec_deleted);
    SG_ASSERT(pcfor->leaves[0].prec_added);
    SG_ASSERT(pcfor->leaves[1].prec_deleted);
    SG_ASSERT(pcfor->leaves[1].prec_added);

#if defined(DEBUG)
    {
        const char* psz_hid_deleted_0 = NULL;
        const char* psz_hid_deleted_1 = NULL;
        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[0].prec_deleted, &psz_hid_deleted_0)  );
        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, pcfor->leaves[1].prec_deleted, &psz_hid_deleted_1)  );
        SG_ASSERT(0 == strcmp(psz_hid_deleted_0, psz_hid_deleted_1));
    }
#endif

    pzt = pzms->pztemplate;

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pzms->dagnum))
    {
        SG_ERR_CHECK_RETURN(  sg_zingtemplate__get_only_rectype(pCtx, pzms->pztemplate, &psz_rectype, NULL)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_dbrecord__get_value(pCtx, pcfor->leaves[1].prec_added, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
    }

    SG_ERR_CHECK(  SG_zingtemplate__get_rectype_info(pCtx, pzt, psz_rectype, &pzmi)  );

    SG_ERR_CHECK(  SG_DBRECORD__ALLOC(pCtx, &pdbrec_freeme)  );
    pdbrec_keepme = pdbrec_freeme;
    SG_ERR_CHECK(  SG_vector__append(pCtx, pzms->pvec_dbrecords, pdbrec_freeme, NULL)  );
    pdbrec_freeme = NULL;
    if (!SG_DAGNUM__HAS_SINGLE_RECTYPE(pzms->dagnum))
    {
        SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec_keepme, SG_ZING_FIELD__RECTYPE, psz_rectype)  );
    }
    SG_ERR_CHECK(  SG_dbrecord__add_pair(pCtx, pdbrec_keepme, SG_ZING_FIELD__RECID, psz_recid)  );

    if (pzmi->b_merge_type_is_record)
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else
    {
        SG_ERR_CHECK(  sg_zing__merge_record_by_field(pCtx, psz_recid, pzms, pzt, psz_rectype, pdbrec_keepme, pzmi, pcfor, pvh_journal_entries)  );
    }

    SG_ERR_CHECK(  my_alloc_cfor(pCtx, pzms, &my_pcfor)  );
    my_pcfor->leaves[0].prec_deleted = pcfor->leaves[0].prec_deleted;
    my_pcfor->leaves[0].prec_added = pdbrec_keepme;
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pzms->prb_pending_mods, psz_recid, &my_pcfor->leaves[0])  );
    my_pcfor = NULL;

    *pb = SG_TRUE;

    // fall thru

fail:
    SG_NULLFREE(pCtx, my_pcfor);
    SG_DBRECORD_NULLFREE(pCtx, pdbrec_freeme);
    SG_ZINGRECTYPEINFO_NULLFREE(pCtx, pzmi);
}

static void sg_zingmerge__mod_mod(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_vhash* pvh_journal_entries
    )
{
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_recid = NULL;
    struct changes_for_one_record* pcfor = NULL;
    SG_vector* pvec_fixed = NULL;
    SG_uint32 i = 0;
    SG_uint32 count = 0;

    SG_ERR_CHECK(  SG_rbtree__count(pCtx, pzms->prb_conflicts__mod_mod, &count)  );

    if (count)
    {
        SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pvec_fixed, count)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pzms->prb_conflicts__mod_mod, &b, &psz_recid, (void**) &pcfor)  );
        while (b)
        {
            SG_bool b_conflict_fixed = SG_FALSE;

            SG_ERR_CHECK(  sg_zingmerge__mod_mod__one_record(pCtx, pzms, psz_recid, pcfor, &b_conflict_fixed, pvh_journal_entries)  );
            if (b_conflict_fixed)
            {
                SG_ERR_CHECK(  SG_vector__append(pCtx, pvec_fixed, (void*) psz_recid, NULL)  );
            }

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**) &pcfor)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

        SG_ERR_CHECK(  SG_vector__length(pCtx, pvec_fixed, &count)  );
        for (i=0; i<count; i++)
        {
            SG_ERR_CHECK(  SG_vector__get(pCtx, pvec_fixed, i, (void**) &psz_recid)  );
            SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pzms->prb_conflicts__mod_mod, psz_recid)  );
        }
        SG_VECTOR_NULLFREE(pCtx, pvec_fixed);
    }

    return;

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}


static void sg_zingmerge__handle_record_conflicts(
    SG_context* pCtx,
    struct sg_zing_merge_situation* pzms,
    SG_vhash* pvh_journal_entries
    )
{
    SG_ERR_CHECK_RETURN(  sg_zingmerge__add_add(pCtx, pzms, pvh_journal_entries)  );
    SG_ERR_CHECK_RETURN(  sg_zingmerge__delete_mod(pCtx, pzms, pvh_journal_entries)  );
    SG_ERR_CHECK_RETURN(  sg_zingmerge__mod_mod(pCtx, pzms, pvh_journal_entries)  );
}

void sg_zingmerge__add_journal_entries(
    SG_context* pCtx,
    SG_zingtx* pztx,
    SG_vhash* pvh_journal_entries
    )
{
    SG_uint32 count_journal_rectypes = 0;
    SG_uint32 i_journal_rectype = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_journal_entries, &count_journal_rectypes)  );
    for (i_journal_rectype=0; i_journal_rectype<count_journal_rectypes; i_journal_rectype++)
    {
        const char* psz_journal_rectype = NULL;
        SG_varray* pva_entries = NULL;
        SG_uint32 count_entries = 0;
        SG_uint32 i_entry = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__varray(pCtx, pvh_journal_entries, i_journal_rectype, &psz_journal_rectype, &pva_entries)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_entries, &count_entries)  );
        for (i_entry=0; i_entry<count_entries; i_entry++)
        {
            SG_vhash* pvh_entry = NULL;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_entries, i_entry, &pvh_entry)  );
            SG_ERR_CHECK(  sg_zingtx__add__new_from_vhash(pCtx, pztx, psz_journal_rectype, pvh_entry)  );
        }
    }

fail:
    ;
}

static void  SG_zing__automerge__regular(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const SG_audit* pq,
    const char* psz_csid_node_0,
    const char* psz_csid_node_1,
    SG_dagnode** ppdn_merged
    )
{
    SG_dagnode* pdn_merged = NULL;
	SG_changeset* pcs = NULL;
    SG_zingtx* pztx = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_recid = NULL;
    SG_dbrecord* pdbrec = NULL;
    struct sg_zing_merge_situation zms;
    struct record_changes_from_one_changeset* rcfoc = NULL;
    SG_varray* pva_violations = NULL;
    SG_vhash* pvh_journal_entries = NULL;
    SG_vhash* pvh_more_journal_entries = NULL;
    SG_daglca* plca = NULL;
    SG_rbtree* prb_temp = NULL;

    SG_ASSERT(!SG_DAGNUM__USE_TRIVIAL_MERGE(iDagNum));

	SG_NULLARGCHECK_RETURN(pRepo);
    memset(&zms, 0, sizeof(zms));

    zms.dagnum = iDagNum;

    SG_ERR_CHECK(  SG_vector__alloc(pCtx, &zms.pvec_pcfor, 10)  );
    SG_ERR_CHECK(  SG_vector__alloc(pCtx, &zms.pvec_dbrecords, 10)  );
    zms.pq = pq;

    zms.parents[0] = psz_csid_node_0;
    zms.parents[1] = psz_csid_node_1;

    SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, iDagNum, zms.parents[0], &zms.audits[0])  );
    SG_ERR_CHECK(  SG_repo__lookup_audits(pCtx, pRepo, iDagNum, zms.parents[1], &zms.audits[1])  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_temp)  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb_temp,zms.parents[0])  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb_temp,zms.parents[1])  );
	SG_ERR_CHECK(  SG_repo__get_dag_lca(pCtx,pRepo,iDagNum,prb_temp,&plca)  );
    {
        const char* psz_hid = NULL;
        SG_daglca_node_type node_type = 0;
        SG_int32 gen = -1;

        SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx,
                                                  NULL,
                                                  plca,
                                                  SG_FALSE,
                                                  &psz_hid,
                                                  &node_type,
                                                  &gen,
                                                  NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid, &zms.psz_hid_cs_ancestor)  );
    }

    if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(iDagNum))
    {
        SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, iDagNum, zms.psz_hid_cs_ancestor, &zms.psz_hid_template_ancestor)  );
        SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, iDagNum, zms.parents[0], &zms.psz_hid_template[0])  );
        SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, iDagNum, zms.parents[1], &zms.psz_hid_template[1])  );

        zms.i_which_template = -1;

        if (0 == strcmp(zms.psz_hid_template[0], zms.psz_hid_template[1]))
        {
            // the parents agree
            zms.i_which_template = 0;
        }
        else
        {
            SG_bool b_mod[2];

            b_mod[0] = (0 != strcmp(zms.psz_hid_template[0], zms.psz_hid_template_ancestor));
            b_mod[1] = (0 != strcmp(zms.psz_hid_template[1], zms.psz_hid_template_ancestor));

            if (b_mod[0] && b_mod[1])
            {
                SG_int16 which = -1;

                // we can't really have the template specify how to resolve
                // a template conflict.  So we just take the most recent.

                SG_ERR_CHECK(  sg_zingmerge__most_recent(pCtx, &zms, &which)  );
                zms.i_which_template = which;
#if 0
                SG_ERR_CHECK(  sg_zingmerge__add_log_entry(pCtx, zms.pva_log,
                            "reason", "took most recent template",
                            NULL) );
#endif
            }
            else
            {
                SG_ASSERT(b_mod[0] || b_mod[1]);

                if (b_mod[0])
                {
                    zms.i_which_template = 0;
                }
                else
                {
                    zms.i_which_template = 1;
                }
            }
        }
        SG_ASSERT((zms.i_which_template == 0) || (zms.i_which_template == 1));
        SG_ERR_CHECK(  SG_zing__get_cached_template__hid_template(pCtx, pRepo, iDagNum, zms.psz_hid_template[zms.i_which_template], &zms.pztemplate)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_zing__get_cached_template__static_dagnum(pCtx, iDagNum, &zms.pztemplate)  );
    }
    SG_ASSERT(zms.pztemplate);

    /* Step 1:  collect all the changes together and group them by recid */

    SG_ERR_CHECK(  sg_zing__collect_and_group_all_changes(pCtx, pRepo, iDagNum, &zms)  );

    /* Step 2:  Run through prb_keys and see if any of them have conflicts */

    SG_ERR_CHECK(  sg_zingmerge__check_for_conflicts(
                pCtx,
                &zms
                )  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_journal_entries)  );

    // now see if we can resolve the conflicts using rules
    // from the template.

    SG_ERR_CHECK(  sg_zingmerge__handle_record_conflicts(
                pCtx,
                &zms,
                pvh_journal_entries
                )  );

    // Now commit

    if (pq)
    {
        SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, iDagNum, pq->who_szUserId, zms.psz_hid_cs_ancestor, &pztx)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, iDagNum, NULL, zms.psz_hid_cs_ancestor, &pztx)  );
    }

    // add the leaves as PARENTS
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_node_0)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_node_1)  );

    // TODO the following line assumes that the template for the result is in fact
    // one of the templates from the parents, ie, not a newly constructed template
    // which was made by merging the template of the parents.  Someday this will
    // probably change.
    if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(iDagNum))
    {
        SG_ERR_CHECK(  SG_zingtx__set_template__existing(pCtx, pztx, zms.psz_hid_template[zms.i_which_template])  );
    }

    // the ADDS
    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, zms.prb_pending_adds, &b, &psz_recid, (void**) &pdbrec)  );
    while (b)
    {
        SG_ERR_CHECK(  sg_zingtx__add__from_dbrecord(pCtx, pztx, pdbrec)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**)&pdbrec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // the DELETES
    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, zms.prb_pending_deletes, &b, &psz_recid, (void**) &pdbrec)  );
    while (b)
    {
        SG_ERR_CHECK(  sg_zingtx__delete__from_dbrecord(pCtx, pztx, pdbrec)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**)&pdbrec)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // the MODS
    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, zms.prb_pending_mods, &b, &psz_recid, (void**) &rcfoc)  );
    while (b)
    {
        const char* psz_hid_delete = NULL;

        SG_ERR_CHECK(  SG_dbrecord__get_hid__ref(pCtx, rcfoc->prec_deleted, &psz_hid_delete)  );
        SG_ERR_CHECK(  sg_zingtx__mod__from_dbrecord(pCtx, pztx, psz_hid_delete, rcfoc->prec_added)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_recid, (void**)&rcfoc)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // the pending journal entries
    SG_ERR_CHECK(  sg_zingmerge__add_journal_entries(pCtx, pztx, pvh_journal_entries)  );

    {
        SG_uint32 ileaf = 0;

        for (ileaf=0; ileaf<2; ileaf++)
        {
            SG_ERR_CHECK(  SG_zingtx__set_baseline_delta(
                        pCtx,
                        pztx,
                        zms.parents[ileaf],
                        &zms.deltas[ileaf]
                        )  );
        }
    }

    // and COMMIT
    if (pq)
    {
        SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn_merged, &pva_violations)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, 0, &pztx, &pcs, &pdn_merged, &pva_violations)  );
    }
    SG_CHANGESET_NULLFREE(pCtx, pcs);

    if (!pdn_merged)
    {
        SG_ERR_CHECK(  sg_zingmerge__resolve_commit_errors(
                    pCtx,
                    &zms,
                    pztx,
                    pva_violations,
                    &pvh_more_journal_entries
                    )
                );

        // add all the journal entries
        SG_ERR_CHECK(  sg_zingmerge__add_journal_entries(pCtx, pztx, pvh_more_journal_entries)  );
        SG_VHASH_NULLFREE(pCtx, pvh_more_journal_entries);
        SG_VARRAY_NULLFREE(pCtx, pva_violations);
        if (pq)
        {
            SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn_merged, &pva_violations)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, 0, &pztx, &pcs, &pdn_merged, &pva_violations)  );
        }
        SG_CHANGESET_NULLFREE(pCtx, pcs);
    }

    *ppdn_merged = pdn_merged;
    pdn_merged = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_journal_entries);
    SG_VHASH_NULLFREE(pCtx, pvh_more_journal_entries);
    SG_VARRAY_NULLFREE(pCtx, pva_violations);
	SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    SG_RBTREE_NULLFREE(pCtx, zms.prb_keys);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_pending_adds);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_pending_mods);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_pending_deletes);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_conflicts__add_add);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_conflicts__mod_mod);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_conflicts__delete_mod);

    SG_RBTREE_NULLFREE(pCtx, zms.deltas[0].prb_add);
    SG_RBTREE_NULLFREE(pCtx, zms.deltas[0].prb_remove);
    SG_RBTREE_NULLFREE(pCtx, zms.deltas[1].prb_add);
    SG_RBTREE_NULLFREE(pCtx, zms.deltas[1].prb_remove);

    SG_NULLFREE(pCtx, zms.psz_hid_cs_ancestor);
    SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, zms.pvec_pcfor, my_free_cfor);
    SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, zms.pvec_dbrecords, (SG_free_callback*) SG_dbrecord__free);

    SG_VARRAY_NULLFREE(pCtx, zms.audits[0]);
    SG_VARRAY_NULLFREE(pCtx, zms.audits[1]);

    /* the dbrecord ptrs in prb_pending_adds/deletes were freed just above */

    SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
    SG_RBTREE_NULLFREE(pCtx, prb_temp);
    SG_DAGLCA_NULLFREE(pCtx, plca);
}

static void sg_zingmerge__trivial__collect_all_changes(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 dagnum,
    struct sg_zing_merge_situation__trivial* pzms
    )
{
    SG_uint32 ileaf = 0;

    for (ileaf=0; ileaf<2; ileaf++)
    {
        /* Diff it */
        SG_ERR_CHECK(  SG_db__diff(
                    pCtx,
                    pRepo,
                    dagnum,
                    pzms->psz_hid_cs_ancestor,
                    pzms->parents[ileaf],
                    &pzms->deltas[ileaf]
                    )  );


        SG_ERR_CHECK(  SG_rbtree__update__from_other_rbtree__keys_only(pCtx, pzms->prb_pending_adds, pzms->deltas[ileaf].prb_add)  );
        SG_ERR_CHECK(  SG_rbtree__update__from_other_rbtree__keys_only(pCtx, pzms->prb_pending_deletes, pzms->deltas[ileaf].prb_remove)  );
    }

    // fall thru

fail:
    return;
}

static void SG_zing__automerge__trivial(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const SG_audit* pq,
    const char* psz_csid_node_0,
    const char* psz_csid_node_1,
    SG_dagnode** ppdn_merged
    )
{
    SG_dagnode* pdn_merged = NULL;
	SG_changeset* pcs = NULL;
    SG_pendingdb* ptx = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    struct sg_zing_merge_situation__trivial zms;
    const char* psz_hid_rec = NULL;
    const char* psz_hid_template_merge = NULL;
    SG_daglca* plca = NULL;
    SG_rbtree* prb_temp = NULL;

    SG_ASSERT(SG_DAGNUM__USE_TRIVIAL_MERGE(iDagNum));

	SG_NULLARGCHECK_RETURN(pRepo);
    memset(&zms, 0, sizeof(zms));

    zms.pq = pq;

    zms.parents[0] = psz_csid_node_0;
    zms.parents[1] = psz_csid_node_1;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_temp)  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb_temp,zms.parents[0])  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb_temp,zms.parents[1])  );
	SG_ERR_CHECK(  SG_repo__get_dag_lca(pCtx,pRepo,iDagNum,prb_temp,&plca)  );
    {
        const char* psz_hid = NULL;
        SG_daglca_node_type node_type = 0;
        SG_int32 gen = -1;

        SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx,
                                                  NULL,
                                                  plca,
                                                  SG_FALSE,
                                                  &psz_hid,
                                                  &node_type,
                                                  &gen,
                                                  NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid, &zms.psz_hid_cs_ancestor)  );
    }

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &zms.prb_pending_adds)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &zms.prb_pending_deletes)  );

    SG_ERR_CHECK(  sg_zingmerge__trivial__collect_all_changes(pCtx, pRepo, iDagNum, &zms)  );

    if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(iDagNum))
    {
        const char* psz_hid_template_ancestor;
        const char* psz_hid_template[2];
        SG_int32 i_which_template;

        SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, iDagNum, zms.psz_hid_cs_ancestor, &psz_hid_template_ancestor)  );
        SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, iDagNum, zms.parents[0], &psz_hid_template[0])  );
        SG_ERR_CHECK_RETURN(  SG_zing__get_cached_hid_for_template__csid(pCtx, pRepo, iDagNum, zms.parents[1], &psz_hid_template[1])  );

        i_which_template = -1;

        if (0 == strcmp(psz_hid_template[0], psz_hid_template[1]))
        {
            // the parents agree
            i_which_template = 0;
        }
        else
        {
            SG_bool b_mod[2];

            b_mod[0] = (0 != strcmp(psz_hid_template[0], psz_hid_template_ancestor));
            b_mod[1] = (0 != strcmp(psz_hid_template[1], psz_hid_template_ancestor));

            if (b_mod[0] && b_mod[1])
            {
                // TODO FELIX just take the most recent and log it
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            else
            {
                SG_ASSERT(b_mod[0] || b_mod[1]);

                if (b_mod[0])
                {
                    i_which_template = 0;
                }
                else
                {
                    i_which_template = 1;
                }
            }
        }
        SG_ASSERT((i_which_template == 0) || (i_which_template == 1));

        psz_hid_template_merge = psz_hid_template[i_which_template];
    }

    // Now commit

    SG_ERR_CHECK(  SG_pendingdb__alloc(pCtx, pRepo, iDagNum, zms.psz_hid_cs_ancestor, psz_hid_template_merge, &ptx)  );
    SG_ERR_CHECK(  SG_pendingdb__add_parent(pCtx, ptx, psz_csid_node_0)  );
    SG_ERR_CHECK(  SG_pendingdb__add_parent(pCtx, ptx, psz_csid_node_1)  );

    // the ADDS
    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, zms.prb_pending_adds, &b, &psz_hid_rec, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_pendingdb__add__hidrec(pCtx, ptx, psz_hid_rec)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_rec, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // the DELETES
    b = SG_FALSE;
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, zms.prb_pending_deletes, &b, &psz_hid_rec, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_pendingdb__remove(pCtx, ptx, psz_hid_rec)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_hid_rec, NULL)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    {
        SG_uint32 ileaf = 0;

        for (ileaf=0; ileaf<2; ileaf++)
        {
            SG_ERR_CHECK(  SG_pendingdb__set_baseline_delta(
                        pCtx,
                        ptx,
                        zms.parents[ileaf],
                        &zms.deltas[ileaf]
                        )  );
        }
    }

    // and COMMIT
    SG_ERR_CHECK(  SG_pendingdb__commit(pCtx, ptx, pq, &pcs, &pdn_merged)  );

    SG_CHANGESET_NULLFREE(pCtx, pcs);

    SG_PENDINGDB_NULLFREE(pCtx, ptx);

    *ppdn_merged = pdn_merged;
    pdn_merged = NULL;

fail:
    // TODO abort the pendingdb?
    SG_RBTREE_NULLFREE(pCtx, zms.prb_pending_adds);
    SG_RBTREE_NULLFREE(pCtx, zms.prb_pending_deletes);

    SG_RBTREE_NULLFREE(pCtx, zms.deltas[0].prb_add);
    SG_RBTREE_NULLFREE(pCtx, zms.deltas[0].prb_remove);
    SG_RBTREE_NULLFREE(pCtx, zms.deltas[1].prb_add);
    SG_RBTREE_NULLFREE(pCtx, zms.deltas[1].prb_remove);

    SG_NULLFREE(pCtx, zms.psz_hid_cs_ancestor);

    SG_DAGNODE_NULLFREE(pCtx, pdn_merged);
    SG_RBTREE_NULLFREE(pCtx, prb_temp);
    SG_DAGLCA_NULLFREE(pCtx, plca);
}

void  SG_zing__automerge(
    SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const SG_audit* pq,
    const char* psz_csid_node_0,
    const char* psz_csid_node_1,
    SG_dagnode** ppdn_merged
    )
{
    if (SG_DAGNUM__USE_TRIVIAL_MERGE(iDagNum))
    {
        SG_ERR_CHECK_RETURN(  SG_zing__automerge__trivial(pCtx,
                    pRepo,
                    iDagNum,
                    pq,
                    psz_csid_node_0,
                    psz_csid_node_1,
                    ppdn_merged
                    )  );
    }
    else
    {
        SG_ERR_CHECK_RETURN(   SG_zing__automerge__regular(pCtx,
                    pRepo,
                    iDagNum,
                    pq,
                    psz_csid_node_0,
                    psz_csid_node_1,
                    ppdn_merged
                    )  );
    }
}

