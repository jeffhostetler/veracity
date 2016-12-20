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

// note that it's "uniqify", not "eunuchify".  :-)

#define MY_TIMESTAMP__LAST_MODIFIED 1
#define MY_TIMESTAMP__LAST_CREATED  2

static void sg_uniqify__get_most_recent_timestamp(
        SG_context* pCtx,
        SG_vhash* pvh_entry,
        SG_int64* pi
        )
{
    SG_varray* pva_audits = NULL;
    SG_vhash* pvh_one = NULL;
    SG_int64 i = 0;

    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_entry, "audits", &pva_audits)  );
    SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_audits, 0, &pvh_one)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_one, SG_AUDIT__TIMESTAMP, &i)  );
    *pi = i;

fail:
    ;
}

static void x_append_random_string(
    SG_context* pCtx,
    SG_string* pstr,
    SG_uint32 len,
    const char* psz_genchars,
    SG_uint32 len_genchars
    )
{
    SG_uint32 i = 0;

    SG_NULLARGCHECK_RETURN(pstr);
    SG_NULLARGCHECK_RETURN(psz_genchars);
    SG_ARGCHECK_RETURN(len > 0, len);
    SG_ARGCHECK_RETURN(len_genchars > 0, len_genchars);

    for (i=0; i<len; i++)
    {
        char c = psz_genchars[SG_random_uint32__2(0, len_genchars - 1)];

        SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pstr, (SG_byte*) &c, 1)  );
    }

fail:
    ;
}

void sg_zing__gen_userprefix_unique_string(
    SG_context* pCtx,
    SG_zingtx* pztx,
    const char* psz_rectype,
    const char* psz_field_name,
    SG_uint32 num_digits,
    const char* psz_base,
    SG_string** ppstr
    )
{
    const char* psz_prefix = NULL;
    SG_string* pstr = NULL;
    SG_uint32 count_fails_this_round = 0;
    SG_string* pstr_extended_prefix = NULL;

    // get the prefix
    if (!pztx->pvh_user)
    {
        // userprefix_unique is not allowed as a uniqify
        // solution in the template for the users dag itself.

        if (SG_DAGNUM__USERS == pztx->iDagNum)
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }

        SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx, pztx->pRepo, NULL, pztx->buf_who, &pztx->pvh_user)  );
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pztx->pvh_user, "prefix", &psz_prefix)  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );

    while (1)
    {
        SG_bool b_unique = SG_FALSE;

        SG_ERR_CHECK(  SG_string__clear(pCtx, pstr)  );
        if (psz_base)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, psz_base)  );
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, "_")  );
        }
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, psz_prefix)  );
        if (pstr_extended_prefix)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, SG_string__sz(pstr_extended_prefix))  );
        }
        SG_ERR_CHECK(  x_append_random_string(pCtx, pstr, num_digits, "0123456789", 10)  );

        SG_ERR_CHECK(  SG_zingtx__generating__would_be_unique(pCtx, pztx, psz_rectype, psz_field_name, SG_string__sz(pstr), &b_unique)  );
        if (b_unique)
        {
            *ppstr = pstr;
            pstr = NULL;
            break;
        }

        // if too many tries, start extended the prefix
        count_fails_this_round++;
        if (
                (count_fails_this_round >= 10) // TODO always 10?
           )
        {
            // extend the prefix
            if (!pstr_extended_prefix)
            {
                SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstr_extended_prefix, "Q")  );
            }
            else
            {
                SG_ERR_CHECK(  x_append_random_string(pCtx, pstr_extended_prefix, 1, "ABCDEFGHJKLMNPRSTUVWXYZ", 23)  );
            }
            count_fails_this_round = 0;
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_extended_prefix);
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_zing__append_random_unique_string(
    SG_context* pCtx,
    SG_zingtx* pztx,
    const char* psz_rectype,
    const char* psz_name,
    const char* psz_base,
    SG_uint32 num_digits,
    const char* psz_genchars,
    SG_vhash* pvh_bad,
    SG_uint32 max_len_suffix,
    SG_string** ppstr
    )
{
    SG_uint32 try_this_len = 0;
    SG_uint32 count_fails_this_len = 0;
    SG_string* pstr = NULL;
    SG_uint32 len_genchars = 0;

    len_genchars = SG_STRLEN(psz_genchars);
    // TODO errcheck on len_genchars

    try_this_len = num_digits;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );

    while (1)
    {
        SG_bool b_unique = SG_FALSE;

        // first generate a random string.  loop until we get one
        // that is "legit".
        while (1)
        {
            SG_ERR_CHECK(  SG_string__clear(pCtx, pstr)  );
            SG_ERR_CHECK(  x_append_random_string(pCtx, pstr, try_this_len, psz_genchars, len_genchars)  );
            if (pvh_bad)
            {
                SG_bool b_has = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_bad, SG_string__sz(pstr), &b_has)  );
                if (!b_has)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        SG_ERR_CHECK(  SG_string__insert__sz(pCtx, pstr, 0, "_")  );
        SG_ERR_CHECK(  SG_string__insert__sz(pCtx, pstr, 0, psz_base)  );

        // now see if it's unique
        SG_ERR_CHECK(  SG_zingtx__generating__would_be_unique(pCtx, pztx, psz_rectype, psz_name, SG_string__sz(pstr), &b_unique)  );
        if (b_unique)
        {
            *ppstr = pstr;
            pstr = NULL;
            break;
        }

        count_fails_this_len++;
        if (
                (count_fails_this_len >= 10) // TODO always 10?
                && (try_this_len < max_len_suffix)
           )
        {
            try_this_len++;
            count_fails_this_len = 0;
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_zing__gen_random_unique_string(
    SG_context* pCtx,
    SG_zingtx* pztx,
    SG_zingfieldattributes* pzfa,
    SG_string** ppstr
    )
{
    SG_uint32 try_this_len = 0;
    SG_uint32 count_fails_this_len = 0;
    SG_string* pstr = NULL;
    SG_uint32 len_genchars = 0;

    SG_ASSERT(pzfa->v._string.has_minlength);
    SG_ASSERT(pzfa->v._string.has_maxlength);
    SG_ASSERT(pzfa->v._string.genchars);

    len_genchars = SG_STRLEN(pzfa->v._string.genchars);
    // TODO errcheck on len_genchars

	SG_ASSERT(pzfa->v._string.val_minlength <= (SG_int64)SG_UINT32_MAX);
    try_this_len = (SG_uint32)pzfa->v._string.val_minlength;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );

    while (1)
    {
        SG_bool b_unique = SG_FALSE;

        // first generate a random string.  loop until we get one
        // that is "legit".
        while (1)
        {
            SG_ERR_CHECK(  SG_string__clear(pCtx, pstr)  );
            SG_ERR_CHECK(  x_append_random_string(pCtx, pstr, try_this_len, pzfa->v._string.genchars, len_genchars)  );
            if (pzfa->v._string.bad)
            {
                SG_bool b_has = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__has(pCtx, pzfa->v._string.bad, SG_string__sz(pstr), &b_has)  );
                if (!b_has)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        // now see if it's unique
        SG_ERR_CHECK(  SG_zingtx__generating__would_be_unique(pCtx, pztx, pzfa->rectype, pzfa->name, SG_string__sz(pstr), &b_unique)  );
        if (b_unique)
        {
            *ppstr = pstr;
            pstr = NULL;
            break;
        }

        count_fails_this_len++;
        if (
                (count_fails_this_len >= 10) // TODO always 10?
                && (try_this_len < pzfa->v._string.val_maxlength)
           )
        {
            try_this_len++;
            count_fails_this_len = 0;
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_zing_uniqify__which__least_impact(
    SG_context* pCtx,
    SG_varray* pva_recids,
    const char* psz_rectype,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const SG_audit* pq,
    const char** leaves,
    const char* psz_hid_cs_ancestor,
    const char** pp
    )
{
    SG_uint32 count_recids = 0;
    SG_uint32 i = 0;
    SG_varray* pva_history = NULL;
    struct impact
    {
        const char* psz_recid;
        SG_bool b_me_created;
        SG_uint32 count_history_entries;
        SG_int64 time_last_modified;
        SG_int64 time_created;
        SG_uint32 count_who_modified;
        SG_int32 gen_leaf;
    } winning;
    SG_rbtree* prb_who = NULL;

    memset(&winning, 0, sizeof(winning));

    // TODO use the leaves and ancestor to figure out which
    // leaf each record is associated with.
    SG_UNUSED(leaves);
    SG_UNUSED(psz_hid_cs_ancestor);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_recids, &count_recids)  );
    for (i=0; i<count_recids; i++)
    {
        struct impact cur;
        SG_uint32 j = 0;

        memset(&cur, 0, sizeof(cur));
        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_recids, i, &cur.psz_recid)  );

        // Grab the record history and write down a few details from it
        SG_ERR_CHECK(  SG_repo__dbndx__query_record_history(pCtx, pRepo, iDagNum, cur.psz_recid, psz_rectype, &pva_history)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_history, &cur.count_history_entries)  );
        SG_ASSERT(cur.count_history_entries >= 1);

        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_who)  );
        for (j=0; j<cur.count_history_entries; j++)
        {
            const char* psz_who = NULL;
            SG_varray* pva_audits = NULL;
            SG_vhash* pvh_one_audit = NULL;
            SG_vhash* pvh_entry = NULL;

            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_history, j, &pvh_entry)  );

            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_entry, "audits", &pva_audits)  );
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_audits, 0, &pvh_one_audit)  );

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_audit, SG_AUDIT__USERID, &psz_who)  );
            if (j == (cur.count_history_entries-1))
            {
                if (0 == strcmp(psz_who, pq->who_szUserId))
                {
                    cur.b_me_created = SG_TRUE;
                }
                else
                {
                    cur.b_me_created = SG_FALSE;
                }
                SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_one_audit, SG_AUDIT__TIMESTAMP, &cur.time_created)  );
            }
            else if (0 == j)
            {
                SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_one_audit, SG_AUDIT__TIMESTAMP, &cur.time_last_modified)  );
            }
            SG_ERR_CHECK(  SG_rbtree__update(pCtx, prb_who, psz_who)  );
        }
        SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_who, &cur.count_who_modified)  );
        SG_RBTREE_NULLFREE(pCtx, prb_who);

        // TODO now look at the dag.  find how many changesets are children on
        // each branch and how many people modified those changesets.

        if (!winning.psz_recid)
        {
            winning = cur;
        }
        else
        {
            // This is the code that decides between two choices
            if (cur.count_who_modified < winning.count_who_modified)
            {
                winning = cur;
            }
            else if (cur.count_who_modified > winning.count_who_modified)
            {
                // still winning
            }
            else
            {
                if (cur.count_history_entries < winning.count_history_entries)
                {
                    winning = cur;
                }
                else if (cur.count_history_entries > winning.count_history_entries)
                {
                    // still winning
                }
                else
                {
                    if (cur.gen_leaf < winning.gen_leaf)
                    {
                        winning = cur;
                    }
                    else if (cur.gen_leaf > winning.gen_leaf)
                    {
                        // still winning
                    }
                    else
                    {
                        if ((!winning.b_me_created) && cur.b_me_created)
                        {
                            winning = cur;
                        }
                        else if (winning.b_me_created && cur.b_me_created)
                        {
                            if (cur.time_created > winning.time_created)
                            {
                                winning = cur;
                            }
                            else
                            {
                            }
                        }
                        else
                        {
                        }
                    }
                }
            }
        }
        SG_VARRAY_NULLFREE(pCtx, pva_history);
    }

    *pp = winning.psz_recid;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_history);
    SG_RBTREE_NULLFREE(pCtx, prb_who);
}

static void sg_zing_uniqify__which__timestamp(
    SG_context* pCtx,
    SG_uint32 iHow,
    SG_varray* pva_recids,
    const char* psz_rectype,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char** pp
    )
{
    SG_int64 timestamp_winning = 0;
    SG_uint32 count_recids = 0;
    SG_uint32 i = 0;
    SG_varray* pva_history = NULL;
    const char* psz_recid_winning = NULL;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_recids, &count_recids)  );
    for (i=0; i<count_recids; i++)
    {
        const char* psz_recid = NULL;
        SG_vhash* pvh_entry = NULL;
        SG_uint32 count_entries = 0;
        SG_int64 cur_timestamp = 0;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_recids, i, &psz_recid)  );
        SG_ERR_CHECK(  SG_repo__dbndx__query_record_history(pCtx, pRepo, iDagNum, psz_recid, psz_rectype, &pva_history)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_history, &count_entries)  );
        SG_ASSERT(count_entries >= 1);
        // record history is always sorted by generation, with the most recent event
        // at index 0.
        if (MY_TIMESTAMP__LAST_MODIFIED == iHow)
        {
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_history, 0, &pvh_entry)  );
        }
        else if (MY_TIMESTAMP__LAST_CREATED == iHow)
        {
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_history, count_entries-1, &pvh_entry)  );
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }
        SG_ERR_CHECK(  sg_uniqify__get_most_recent_timestamp(pCtx, pvh_entry, &cur_timestamp)  );

        if (
                (!psz_recid_winning)
                || (cur_timestamp > timestamp_winning)
           )
        {
            psz_recid_winning = psz_recid;
            timestamp_winning = cur_timestamp;
        }
        SG_VARRAY_NULLFREE(pCtx, pva_history);
    }

    *pp = psz_recid_winning;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_history);
}

static void sg_zing_uniqify__which(
    SG_context* pCtx,
    SG_vhash* pvh_uniqify,
    SG_varray* pva_recids,
    const char* psz_rectype,
    SG_zingtx* pztx,
    const SG_audit* pq,
    const char** leaves,
    const char* psz_hid_cs_ancestor,
    const char** pp
    )
{
    const char* psz_which = NULL;
    const char* psz_recid_result = NULL;
    SG_repo* pRepo = NULL;
    SG_uint64 iDagNum = 0;

    SG_ERR_CHECK(  SG_zingtx__get_repo(pCtx, pztx, &pRepo)  );
    SG_ERR_CHECK(  SG_zingtx__get_dagnum(pCtx, pztx, &iDagNum)  );

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__WHICH, &psz_which)  );
    if (0 == strcmp("last_modified", psz_which))
    {
        SG_ERR_CHECK(  sg_zing_uniqify__which__timestamp(
                    pCtx,
                    MY_TIMESTAMP__LAST_MODIFIED,
                    pva_recids,
                    psz_rectype,
                    pRepo,
                    iDagNum,
                    &psz_recid_result
                    )
                );
    }
    else if (0 == strcmp("last_created", psz_which))
    {
        SG_ERR_CHECK(  sg_zing_uniqify__which__timestamp(
                    pCtx,
                    MY_TIMESTAMP__LAST_CREATED,
                    pva_recids,
                    psz_rectype,
                    pRepo,
                    iDagNum,
                    &psz_recid_result
                    )
                );
    }
    else if (0 == strcmp("least_impact", psz_which))
    {
        SG_ERR_CHECK(  sg_zing_uniqify__which__least_impact(
                    pCtx,
                    pva_recids,
                    psz_rectype,
                    pRepo,
                    iDagNum,
                    pq,
                    leaves,
                    psz_hid_cs_ancestor,
                    &psz_recid_result
                    )
                );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    *pp = psz_recid_result;

fail:
    ;
}

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
    )
{
    SG_varray* pva_recids = NULL;
    SG_zingrecord* pzr = NULL;
    const char* psz_op = NULL;
    SG_string* pstr_uniqified_value = NULL;
    const char* psz_recid = NULL;
    const char* psz_old_value = NULL;
    SG_vhash* pvh_journal = NULL;
    SG_vhash* pvh_entry = NULL;
    SG_bool b_has = SG_FALSE;
    SG_string* pstr_expanded = NULL;

    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_error, SG_ZING_CONSTRAINT_VIOLATION__IDS, &pva_recids)  );

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__OP, &psz_op)  );

    SG_ERR_CHECK(  sg_zing_uniqify__which(pCtx, pvh_uniqify, pva_recids, pzfa->rectype, pztx, pq, leaves, psz_hid_cs_ancestor, &psz_recid)  );
    SG_ERR_CHECK(  SG_zingtx__get_record(pCtx, pztx, pzfa->rectype, psz_recid, &pzr)  );
    SG_ERR_CHECK(  SG_zingrecord__get_field__string(pCtx, pzr, pzfa, &psz_old_value)  );

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__JOURNAL, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__JOURNAL, &pvh_journal)  );
    }

    if (0 == strcmp(psz_op, "redo_defaultfunc"))
    {
        SG_ERR_CHECK(  SG_zingtx__run_defaultfunc__string(pCtx, pztx, pzfa, &pstr_uniqified_value)  );
        SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, pzr, pzfa, SG_string__sz(pstr_uniqified_value)) );
    }
    else if (0 == strcmp(psz_op, "append_random_unique"))
    {
        SG_uint32 num_digits = 4;
        SG_bool b_has = SG_FALSE;
        const char* psz_genchars = NULL;

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__NUM_DIGITS, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__NUM_DIGITS, &num_digits)  );
        }

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__GENCHARS, &psz_genchars)  );

        SG_ERR_CHECK(  sg_zing__append_random_unique_string(
                    pCtx, 
                    pztx, 
                    pzfa->rectype, 
                    pzfa->name, 
                    psz_old_value, 
                    num_digits, 
                    psz_genchars, 
                    NULL,  // TODO could pass a vhash here for "bad"
                    16, 
                    &pstr_uniqified_value)  );

        SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, pzr, pzfa, SG_string__sz(pstr_uniqified_value)) );
    }
    else if (0 == strcmp(psz_op, "append_userprefix_unique"))
    {
        SG_uint32 num_digits = 4;
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__NUM_DIGITS, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_uniqify, SG_ZING_TEMPLATE__NUM_DIGITS, &num_digits)  );
        }

        SG_ERR_CHECK(  sg_zing__gen_userprefix_unique_string(pCtx, pztx, pzfa->rectype, pzfa->name, num_digits, psz_old_value, &pstr_uniqified_value)  );
        SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, pzr, pzfa, SG_string__sz(pstr_uniqified_value)) );
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
                    (pCtx, "%s", psz_op)
                    );
    }

    if (pvh_journal)
    {
        SG_vhash* pvh_j_fields = NULL;
        const char* psz_journal_rectype = NULL;
        SG_uint32 count_fields = 0;
        SG_uint32 i_field = 0;

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_journal, SG_ZING_FIELD__RECTYPE, &psz_journal_rectype)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_journal, SG_ZING_TEMPLATE__FIELDS, &pvh_j_fields)  );

        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_entry)  );

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_j_fields, &count_fields)  );
        for (i_field=0; i_field<count_fields; i_field++)
        {
            const char* psz_journal_field_name = NULL;
            const char* psz_journal_field_value = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_j_fields, i_field, &psz_journal_field_name, &psz_journal_field_value)  );

            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_expanded, psz_journal_field_value)  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__UNIQIFY__RECID,SG_STRLEN(SG_JOURNAL_MAGIC__UNIQIFY__RECID),
                (const SG_byte *)psz_recid,SG_STRLEN(psz_recid),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__UNIQIFY__FIELD_NAME,SG_STRLEN(SG_JOURNAL_MAGIC__UNIQIFY__FIELD_NAME),
                (const SG_byte *)pzfa->name,SG_STRLEN(pzfa->name),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__UNIQIFY__OP,SG_STRLEN(SG_JOURNAL_MAGIC__UNIQIFY__OP),
                (const SG_byte *)psz_op,SG_STRLEN(psz_op),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__UNIQIFY__OLD_VALUE,SG_STRLEN(SG_JOURNAL_MAGIC__UNIQIFY__OLD_VALUE),
                (const SG_byte *)psz_old_value,SG_STRLEN(psz_old_value),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );

            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_expanded,
                (const SG_byte *)SG_JOURNAL_MAGIC__UNIQIFY__NEW_VALUE,SG_STRLEN(SG_JOURNAL_MAGIC__UNIQIFY__NEW_VALUE),
                (const SG_byte *)SG_string__sz(pstr_uniqified_value),SG_STRLEN(SG_string__sz(pstr_uniqified_value)),
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_entry, psz_journal_field_name, SG_string__sz(pstr_expanded))  );
            SG_STRING_NULLFREE(pCtx, pstr_expanded);
        }

        //SG_VHASH_STDOUT(pvh_entry);

        {
            SG_varray* pva_entries = NULL;
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pending_journal_entries, psz_journal_rectype, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_pending_journal_entries, psz_journal_rectype, &pva_entries)  );
            }
            else
            {
                SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_pending_journal_entries, psz_journal_rectype, &pva_entries)  );
            }

            SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva_entries, &pvh_entry)  );
        }

    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_expanded);
    SG_VHASH_NULLFREE(pCtx, pvh_entry);
    SG_STRING_NULLFREE(pCtx, pstr_uniqified_value);
}


