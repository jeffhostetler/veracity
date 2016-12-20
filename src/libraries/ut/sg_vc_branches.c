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

#define ESCAPEDSTR(esc,raw)   ((const char *)(((esc != NULL) ? (esc) : (raw))))

static void sg_vc_branches__add_head__pre_check(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	const char* psz_branch_name_trimmed, 
	const char* psz_hid_cs_target, 
	SG_varray* pva_parents,
	const char* psz_hid_cs_exempt, // < A node that's exempt from the ancestor check, probably because the head is being moved from it.
	SG_bool b_strict,
	SG_bool* pb_closed,
	SG_vhash** ppvh_del);

static void sg_vc_branches__find_ancestors(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash* pvh_nodes,
        SG_vhash* pvh_del
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_nodes, &count)  );

    for (i=0; i<count; i++)
    {
        const SG_variant* pv = NULL;
        const char* psz_csid = NULL;
        const char* psz_hidrec = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_nodes, i, &psz_csid, &pv)  );
        SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pv, &psz_hidrec)  );

        // if psz_csid is an ancestor of any other node, add it to del
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_del, psz_hidrec, &b_already)  );
        if (!b_already)
        {
            SG_uint32 j = 0;

            for (j=i+1; j<count; j++)
            {
                const SG_variant* pv_other = NULL;
                SG_dagquery_relationship dqRel = 0;
                const char* psz_csid_other = NULL;
                const char* psz_hidrec_other = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_nodes, j, &psz_csid_other, &pv_other)  );
                SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pv_other, &psz_hidrec_other)  );
                SG_dagquery__how_are_dagnodes_related(
                            pCtx, 
                            pRepo, 
                            SG_DAGNUM__VERSION_CONTROL,
                            psz_csid, 
                            psz_csid_other,
                            SG_FALSE,
                            SG_FALSE,
                            &dqRel);

                if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
                {
                    SG_context__err_reset(pCtx);
                    continue;
                }
                SG_ERR_CHECK_CURRENT;

                if (dqRel == SG_DAGQUERY_RELATIONSHIP__ANCESTOR)
                {
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_del, psz_hidrec)  );
                    break;
                }
                else if (dqRel == SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
                {
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_del, psz_hidrec_other)  );
                }
            }
        }
    }


fail:
    ;
}

static void sg_vc_branches__find_dead_stuff(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash* pvh_pile,
        SG_vhash** ppvh_del
        )
{
    SG_vhash* pvh_branches = NULL;
    SG_uint32 count_branches = 0;
    SG_uint32 i = 0;
    SG_vhash* pvh_del = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_del)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branches, &count_branches)  );

    for (i=0; i<count_branches; i++)
    {
        const SG_variant* pv = NULL;
        const char* psz_name = NULL;
        SG_vhash* pvh_branch_info = NULL;
        SG_vhash* pvh_branch_values = NULL;
        SG_uint32 count_branch_values = 0;
        SG_vhash* pvh_branch_records = NULL;
        SG_uint32 count_branch_records = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branches, i, &psz_name, &pv)  );
        SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_branch_info)  );

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "records", &pvh_branch_records)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_branch_values)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_records, &count_branch_records)  );
        SG_ASSERT(count_branch_values == count_branch_records);
        
        if (count_branch_values > 1)
        {
            SG_ERR_CHECK(  sg_vc_branches__find_ancestors(pCtx, pRepo, pvh_branch_values, pvh_del)  );
        }
    }

    {
        SG_uint32 count_del = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_del, &count_del)  );

        if (0 == count_del)
        {
            SG_VHASH_NULLFREE(pCtx, pvh_del);
        }
    }

    *ppvh_del = pvh_del;
    pvh_del = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_del);
}

static void sg_vc_branches__del__one_head(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_vhash* pvh_del,
        const char* psz_del_value,
		SG_bool* pbDeleted
        )
{
    SG_uint32 i = 0;
    SG_uint32 count_del = 0;
	SG_bool bDeleted = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_del, &count_del)  );

    for (i=0; i<count_del; i++)
    {
        const char* psz_hidrec = NULL;
        const char* psz_value = NULL;
        SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_del, i, &psz_hidrec, &psz_value)  );
        if ( 0 == strcmp(psz_del_value, psz_value) )
        {
            SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pztx, "branch", psz_hidrec)  );
			bDeleted = SG_TRUE;
			break;
        }
    }

	*pbDeleted = bDeleted;

fail:
    ;
}

static void sg_vc_branches__del__all_heads(
        SG_context* pCtx,
        SG_zingtx* pztx,
        SG_vhash* pvh_del
        )
{
    SG_uint32 i = 0;
    SG_uint32 count_del = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_del, &count_del)  );

    for (i=0; i<count_del; i++)
    {
        const char* psz_hidrec = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_del, i, &psz_hidrec, NULL)  );
        SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pztx, "branch", psz_hidrec)  );
    }

fail:
    ;
}

void SG_vc_branches__cleanup(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash** ppvh_pile
        )
{
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_del = NULL;
    SG_zingtx* pztx = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;

    SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );
    //SG_VHASH_STDOUT(pvh_pile);
    SG_ERR_CHECK(  sg_vc_branches__find_dead_stuff(pCtx, pRepo, pvh_pile, &pvh_del)  );

    if (pvh_del)
    {
        const char* psz_csid_leaf = NULL;
        SG_audit q;

        SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_pile, "leaf", &psz_csid_leaf)  );

        SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, q.who_szUserId, psz_csid_leaf, &pztx)  );
        SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );

        SG_ERR_CHECK(  sg_vc_branches__del__all_heads(pCtx, pztx, pvh_del)  );

        /* commit the changes */
        SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, q.when_int64, &pztx, &pcs, &pdn, NULL)  );
        SG_DAGNODE_NULLFREE(pCtx, pdn);
        SG_CHANGESET_NULLFREE(pCtx, pcs);

        SG_VHASH_NULLFREE(pCtx, pvh_pile);
        SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );
    }

    if (ppvh_pile)
    {
        *ppvh_pile = pvh_pile;
        pvh_pile = NULL;
    }

fail:
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
    SG_VHASH_NULLFREE(pCtx, pvh_del);
}

void SG_vc_branches__exists(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_branch_name,
	SG_bool* pExists
	)
{
	SG_vhash* pvh_pile = NULL;
	SG_vhash* pvh_pile_branches = NULL;

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_branches, psz_branch_name, pExists)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__ensure_valid_name(
	SG_context* pCtx,
	const char* psz_branch_name
	)
{
	SG_ERR_CHECK_RETURN(  SG_validate__ensure(pCtx, psz_branch_name, 1u, 256u, SG_VALIDATE__BANNED_CHARS, SG_TRUE, SG_ERR_INVALIDARG, "branch name")  );
}

static void sg_vc_branches__add_to_pile(
        SG_context* pCtx,
        SG_vhash* pvh,
        const char* psz_name,
        const char* psz_value,
        const char* psz_hidrec
        )
{
    SG_bool b_already = SG_FALSE;
    SG_vhash* pvh_cur_branch = NULL;
    SG_vhash* pvh_cur_value = NULL;
    SG_vhash* pvh_cur_branch_records = NULL;
    SG_vhash* pvh_cur_branch_values = NULL;
    SG_vhash* pvh_value_hidrecs = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_values = NULL;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "values", &pvh_values)  );
    
    // values.(psz_value)={}
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_values, psz_value, &b_already)  );
    if (!b_already)
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_values, psz_value, &pvh_cur_value)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_values, psz_value, &pvh_cur_value)  );
    }

    // values.(psz_value).(psz_name)={}
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_cur_value, psz_name, &b_already)  );
    if (!b_already)
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_cur_value, psz_name, &pvh_value_hidrecs)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur_value, psz_name, &pvh_value_hidrecs)  );
    }

    // values.(psz_value).(psz_name).(psz_hidrec)=null
    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_value_hidrecs, psz_hidrec)  );

    // branches.(psz_name)={}
    // branches.(psz_name).values={}
    // branches.(psz_name).values.(psz_value)=(psz_hidrec)
    // branches.(psz_name).records={}
    // branches.(psz_name).records.(psz_hidrec)=(psz_value)
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branches, psz_name, &b_already)  );
    if (!b_already)
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_branches, psz_name, &pvh_cur_branch)  );
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_cur_branch, "records", &pvh_cur_branch_records)  );
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_cur_branch, "values", &pvh_cur_branch_values)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branches, psz_name, &pvh_cur_branch)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur_branch, "records", &pvh_cur_branch_records)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur_branch, "values", &pvh_cur_branch_values)  );
    }
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur_branch_records, psz_hidrec, psz_value)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur_branch_values, psz_value, psz_hidrec)  );

fail:
    ;
}

void SG_vc_branches__get_partial_pile(
        SG_context* pCtx,
        SG_repo* pRepo,
        const char* psz_branch_name,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh = NULL;
    SG_vhash* pvh_closed = NULL;
	SG_varray* pva_fields = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_closed = NULL;
    char* psz_csid_leaf = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_values = NULL;
    char* psz_root_node = NULL;
	SG_varray* pva_crit = NULL;

    // create the vhash for our result
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "closed", &pvh_closed)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "values", &pvh_values)  );

    SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_BRANCHES, &psz_csid_leaf)  );

    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "leaf", psz_csid_leaf)  );

    // prep a fields array for use by all queries
	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__HIDREC)  );

    // fetch all the closed records
    SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, psz_csid_leaf, "closed", NULL, NULL, 0, 0, pva_fields, &pva_closed)  );
    if (pva_closed)
    {
        SG_uint32 count_records = 0;
        SG_uint32 i = 0;

        // for each record
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_closed, &count_records)  );
        for (i=0; i<count_records; i++)
        {
            SG_vhash* pvh_closed_record = NULL;
            const char* psz_name = NULL;
            const char* psz_hidrec = NULL;

            // grab data for this record
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_closed, i, &pvh_closed_record)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_closed_record, "name", &psz_name)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_closed_record, SG_ZING_FIELD__HIDREC, &psz_hidrec)  );

            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_closed, psz_name, psz_hidrec)  );
        }
    }

    SG_VARRAY_NULLFREE(pCtx, pva_closed);

    if (psz_branch_name)
    {
		SG_VARRAY__ALLOC(pCtx, &pva_crit);
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "name")  );
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_branch_name)  );
    }

    // fetch all the branch records in this leaf
    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, psz_csid_leaf, "branch", pva_crit, NULL, 0, 0, pva_fields, &pva)  );
    if (pva)
    {
        SG_uint32 count_records = 0;
        SG_uint32 i = 0;

        // for each record
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_records)  );
        for (i=0; i<count_records; i++)
        {
            SG_vhash* pvh_branch_record = NULL;
            const char* psz_name = NULL;
            const char* psz_value = NULL;
            const char* psz_hidrec = NULL;

            // grab data for this record
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh_branch_record)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_branch_record, "name", &psz_name)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_branch_record, "csid", &psz_value)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_branch_record, SG_ZING_FIELD__HIDREC, &psz_hidrec)  );

            SG_ERR_CHECK(  sg_vc_branches__add_to_pile(pCtx, pvh, psz_name, psz_value, psz_hidrec)  );
        }
    }

    SG_VARRAY_NULLFREE(pCtx, pva);

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_NULLFREE(pCtx, psz_root_node);
    SG_NULLFREE(pCtx, psz_csid_leaf);
	SG_VARRAY_NULLFREE(pCtx, pva_crit);
}

void SG_vc_branches__get_whole_pile(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash** ppvh
        )
{
    SG_ERR_CHECK_RETURN(  SG_vc_branches__get_partial_pile(pCtx, pRepo, NULL, ppvh)  );
}

void SG_vc_branches__move_head(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs_from, 
        const char* psz_hid_cs_target, 
        const SG_audit* pq
        )
{
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    const char* psz_csid_leaf = NULL;
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_branch_info = NULL;
    SG_vhash* pvh_branch_records = NULL;

    SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branches, psz_branch_name, &pvh_branch_info)  );
    if (pvh_branch_info)
    {
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branch_info, "records", &pvh_branch_records)  );
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_pile, "leaf", &psz_csid_leaf)  );

    /* start a changeset */

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

    if (pvh_branch_records)
    {
		SG_bool bDeleted = SG_FALSE;
        SG_ERR_CHECK(  sg_vc_branches__del__one_head(pCtx, pztx, pvh_branch_records, psz_hid_cs_from, &bDeleted)  );
		if (!bDeleted)
			SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "%s is not a head of %s", psz_hid_cs_from, psz_branch_name));
    }

	SG_ERR_CHECK(  sg_vc_branches__add_head__pre_check(pCtx, pRepo, psz_branch_name, psz_hid_cs_target, NULL,
		psz_hid_cs_from, SG_TRUE, NULL, NULL)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "branch", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "csid", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_hid_cs_target) );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_branch_name) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__move_head__all(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs_target, 
        const SG_audit* pq
        )
{
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    const char* psz_csid_leaf = NULL;
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_branch_info = NULL;
    SG_vhash* pvh_branch_records = NULL;

    SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branches, psz_branch_name, &pvh_branch_info)  );
    if (pvh_branch_info)
    {
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branch_info, "records", &pvh_branch_records)  );
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_pile, "leaf", &psz_csid_leaf)  );

    /* start a changeset */

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

    if (pvh_branch_records)
    {
        SG_ERR_CHECK(  sg_vc_branches__del__all_heads(pCtx, pztx, pvh_branch_records)  );
    }

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "branch", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "csid", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_hid_cs_target) );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_branch_name) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__reopen(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const SG_audit* pq
        )
{
    SG_zingtx* pztx = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    const char* psz_csid_leaf = NULL;
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_closed = NULL;
    const char* psz_hidrec = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_bool b_exists = SG_FALSE;

    SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branches, psz_branch_name, &b_exists)  );
    if (!b_exists)
    {
        SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
    }
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_closed)  );
    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_closed, psz_branch_name, &psz_hidrec)  );
    if (!psz_hidrec)
    {
        SG_ERR_THROW(  SG_ERR_BRANCH_NOT_CLOSED  );
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_pile, "leaf", &psz_csid_leaf)  );

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );

    SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pztx, "closed", psz_hidrec)  );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__close(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const SG_audit* pq
        )
{
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    const char* psz_csid_leaf = NULL;
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_closed = NULL;
    SG_bool b_already = SG_FALSE;
    SG_bool b_exists = SG_FALSE;
    SG_vhash* pvh_branches = NULL;

    SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branches, psz_branch_name, &b_exists)  );
    if (!b_exists)
    {
        SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
    }
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_closed)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_closed, psz_branch_name, &b_already)  );
    if (b_already)
    {
        SG_ERR_THROW(  SG_ERR_BRANCH_ALREADY_CLOSED  );
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_pile, "leaf", &psz_csid_leaf)  );

    /* start a changeset */

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "closed", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "closed", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_branch_name) );

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__add_head__state(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs_target, 
        SG_varray* pva_parents,
        SG_vhash* pvh_state,
        const SG_audit* pq
        )
{
    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    char* psz_csid_leaf = NULL;
    SG_vhash* pvh_branch = NULL;
    SG_vhash* pvh_new_records = NULL;

    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_state, psz_branch_name, &pvh_branch)  );

    {
        SG_audit q;

        SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
        SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, &q, SG_DAGNUM__VC_BRANCHES, &psz_csid_leaf)  );
    }

    /* start a changeset */

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "branch", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "csid", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_hid_cs_target) );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_branch_name) );

    // delete the parent values
    if (pvh_branch)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_csid = NULL;
            const char* psz_hidrec = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz_csid)  );

            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_branch, psz_csid, &psz_hidrec)  );
            if (psz_hidrec)
            {
                SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_branch, psz_csid)  );
                SG_ERR_CHECK(  SG_zingtx__delete_record__hid(pCtx, pztx, "branch", psz_hidrec)  );
            }
        }
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_state, psz_branch_name, &pvh_branch)  );
    }

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx__hidrecs(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL, &pvh_new_records, NULL)  );

    {
        const char* psz_new_hidrec = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_new_records, 0, &psz_new_hidrec, NULL)  );
        SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_branch, psz_hid_cs_target, psz_new_hidrec)  );
    }

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh_new_records);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_NULLFREE(pCtx, psz_csid_leaf);
}

/* Checks ancestor/descendant relationships and branch name validity. Call before adding or moving a branch head. */
static void sg_vc_branches__add_head__pre_check(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	const char* psz_branch_name_trimmed, 
	const char* psz_hid_cs_target, 
	SG_varray* pva_parents,
	const char* psz_hid_cs_exempt, // < A node that's exempt from the ancestor/descendant checks because the head is being moved from it.
	SG_bool b_strict,
	SG_bool* pb_closed,
	SG_vhash** ppvh_del)
{
	SG_vhash* pvh_del = NULL;
	SG_vhash* pvh_pile = NULL;
	SG_bool b_already = SG_FALSE;
	SG_bool b_closed = SG_FALSE;
	SG_uint32 count_branch_values = 0;
	SG_uint32 i = 0;
	SG_vhash* pvh_parents = NULL;

	if (pva_parents)
	{
		SG_uint32 count = 0;
		SG_uint32 i = 0;

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_parents)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count)  );
		for (i=0; i<count; i++)
		{
			const char* psz_parent = NULL;

			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz_parent)  );
			SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_parents, psz_parent)  );
		}
	}

	SG_ERR_CHECK(  SG_vc_branches__get_partial_pile(pCtx, pRepo, psz_branch_name_trimmed, &pvh_pile)  );
	{
		SG_vhash* pvh_closed = NULL;
		SG_vhash* pvh_branches = NULL;
		SG_vhash* pvh_branch_info = NULL;
		SG_vhash* pvh_branch_values = NULL;

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_closed)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_closed, psz_branch_name_trimmed, &b_closed)  );

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_del)  );

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );

		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branches, psz_branch_name_trimmed, &pvh_branch_info)  );
		if (pvh_branch_info)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
			// check here to see if the branch already exists at this point
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branch_values, psz_hid_cs_target, &b_already)  );
			if (b_already)
			{
				goto done;
			}

			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_branch_values)  );
			for (i=0; i<count_branch_values; i++)
			{
				const char* psz_csid = NULL;
				const char* psz_hidrec = NULL;
				SG_dagquery_relationship dqRel = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
				SG_bool b_in_parents = SG_FALSE;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_branch_values, i, &psz_csid, &psz_hidrec)  );

				if (pvh_parents)
				{
					SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_parents, psz_csid, &b_in_parents)  );
				}

				if (b_in_parents)
				{
					dqRel = SG_DAGQUERY_RELATIONSHIP__ANCESTOR;
				}
				else
				{
					SG_dagquery__how_are_dagnodes_related(
						pCtx, 
						pRepo, 
						SG_DAGNUM__VERSION_CONTROL,
						psz_csid, 
						psz_hid_cs_target,
						SG_FALSE,
						SG_FALSE,
						&dqRel);

					if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
					{
						SG_context__err_reset(pCtx);
						continue;
					}
					SG_ERR_CHECK_CURRENT;
				}

				// if psz_csid is an ancestor of the new node, and not the exempt node, add it to del
				if (dqRel == SG_DAGQUERY_RELATIONSHIP__ANCESTOR && !(psz_hid_cs_exempt && !strcmp(psz_hid_cs_exempt, psz_csid)))
				{
					SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_del, psz_hidrec)  );
				}
				else if (dqRel == SG_DAGQUERY_RELATIONSHIP__DESCENDANT)
				{
					// The new node is an ancestor of an existing node.
					// If we're doing a strict check, and this isn't the exempt node, throw the ancestor error.
					if (b_strict && !(psz_hid_cs_exempt && !strcmp(psz_hid_cs_exempt, psz_csid)))
					{
						SG_ERR_THROW(  SG_ERR_BRANCH_ADD_ANCESTOR  );
					}
				}
			}
		}
		else
		{
			// the branch doesn't exist yet, validate its name
			SG_ERR_CHECK(  SG_vc_branches__ensure_valid_name(pCtx, psz_branch_name_trimmed)  );
		}
	}

	if (b_strict)
	{
		SG_uint32 count_dead = 0;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_del, &count_dead)  );
		if (count_dead)
		{
			SG_ERR_THROW(  SG_ERR_BRANCH_ADD_DESCENDANT  );
		}
	}

done:
	if (pb_closed)
		*pb_closed = b_closed;
	if (ppvh_del)
	{
		*ppvh_del = pvh_del;
		pvh_del = NULL;
	}


fail:
	SG_VHASH_NULLFREE(pCtx, pvh_del);
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_VHASH_NULLFREE(pCtx, pvh_parents);
}

void SG_vc_branches__add_head(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs_target, 
        SG_varray* pva_parents,
        SG_bool b_strict,
        const SG_audit* pq
        )
{
	char* psz_branch_name_trimmed = NULL;
	SG_bool b_closed = SG_FALSE;
	SG_vhash* pvh_del = SG_FALSE;

    SG_zingtx* pztx = NULL;
    SG_zingrecord* prec = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_zingfieldattributes* pzfa = NULL;
    char* psz_csid_leaf = NULL;

	SG_ERR_CHECK(  SG_sz__trim(pCtx, psz_branch_name, NULL, &psz_branch_name_trimmed)  );
	SG_ERR_CHECK(  sg_vc_branches__add_head__pre_check(pCtx, pRepo, psz_branch_name_trimmed, psz_hid_cs_target, 
		pva_parents, NULL, b_strict, &b_closed, &pvh_del)  );
    
	{
        SG_audit q;

        SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
        SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, &q, SG_DAGNUM__VC_BRANCHES, &psz_csid_leaf)  );
    }

    /* start a changeset */

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "branch", &prec)  );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "csid", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_hid_cs_target) );

    SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "branch", "name", &pzfa)  );
    SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_branch_name_trimmed) );

    // delete the stuff obsoleted by this
    if (pvh_del)
    {
        SG_ERR_CHECK(  sg_vc_branches__del__all_heads(pCtx, pztx, pvh_del)  );
    }

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    if (b_closed)
    {
        SG_ERR_CHECK(  SG_vc_branches__reopen(pCtx, pRepo, psz_branch_name_trimmed, pq)  );
    }

    // fall thru
fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_del);
    SG_NULLFREE(pCtx, psz_csid_leaf);
	SG_NULLFREE(pCtx, psz_branch_name_trimmed);
}

void SG_vc_branches__remove_head(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs,
        const SG_audit* pq
        )
{
    SG_zingtx* pztx = NULL;
    SG_dagnode* pdn = NULL;
    SG_changeset* pcs = NULL;
    const char* psz_csid_leaf = NULL;
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_branch_info = NULL;
    SG_vhash* pvh_branch_records = NULL;

    SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pvh_pile)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    
	SG_vhash__get__vhash(pCtx, pvh_branches, psz_branch_name, &pvh_branch_info);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_REPLACE2(SG_ERR_VHASH_KEYNOTFOUND, SG_ERR_BRANCH_NOT_FOUND, (pCtx, "%s", psz_branch_name));
		SG_ERR_CHECK_CURRENT;
	}
    
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "records", &pvh_branch_records)  );

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_pile, "leaf", &psz_csid_leaf)  );

    /* start a changeset */

    SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, pq->who_szUserId, psz_csid_leaf, &pztx)  );
    SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_csid_leaf)  );

    if (psz_hid_cs)
    {
		SG_bool bDeleted = SG_FALSE;
        SG_ERR_CHECK(  sg_vc_branches__del__one_head(pCtx, pztx, pvh_branch_records, psz_hid_cs, &bDeleted)  );
		if (!bDeleted)
			SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "%s is not a head of %s", psz_hid_cs, psz_branch_name));
    }
    else
    {
        SG_ERR_CHECK(  sg_vc_branches__del__all_heads(pCtx, pztx, pvh_branch_records)  );
    }

    /* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

    // fall thru

fail:
    if (pztx)
    {
        SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
    }
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__get_unambiguous(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name,
        SG_bool* pb_exists,
        char** ppsz_csid
        )
{
    SG_vhash* pvh_pile = NULL;
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_branch_info = NULL;
    SG_vhash* pvh_branch_values = NULL;
    SG_uint32 count_values = 0;
    const char* psz_val = NULL;
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branches, psz_branch_name, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branches, psz_branch_name, &pvh_branch_info)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_values)  );
        if (count_values > 1)
        {
            SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE, (pCtx, "%s", psz_branch_name)  );
        }

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, 0, &psz_val, NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_val, ppsz_csid)  );

        if (pb_exists)
        {
            *pb_exists = SG_TRUE;
        }
    }
    else
    {
        if (pb_exists)
        {
            *pb_exists = SG_FALSE;
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
}

void SG_vc_branches__throw_changeset_not_present(
	SG_context* pCtx,
	const char* pszBranchName,
	const char* pszHidCs)
{
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ASSERT(SG_context__err_equals(pCtx, SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT));
		SG_ERR_RETHROW2_RETURN((pCtx, "%s refers to changeset %s. Consider pulling.", pszBranchName, pszHidCs));
	}

	SG_ERR_THROW2_RETURN(SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT, 
		(pCtx, "%s refers to changeset %s. Consider pulling.", pszBranchName, pszHidCs));
}

void SG_vc_branches__get_branch_names_for_given_changesets(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_varray** ppva_csid_list, 
    SG_vhash** ppvh_results_by_csid
    )
{
    SG_varray* pva_fields = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_crit = NULL;
    SG_vhash* pvh_result = NULL;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "csid")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "in")  );
    SG_ERR_CHECK(  SG_varray__append__varray(pCtx, pva_crit, ppva_csid_list)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "name")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "csid")  );

    SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, SG_DAGNUM__VC_BRANCHES, NULL, "branch", pva_crit, NULL, 0, 0, pva_fields, &pva)  );
    if (pva)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
        if (count > 0)
        {
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
            for (i=0; i<count; i++)
            {
                SG_vhash* pvh_rec = NULL;
                const char* psz_csid = NULL;
                const char* psz_branch_name = NULL;
                SG_vhash* pvh_names = NULL;

                SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh_rec)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_rec, "name", &psz_branch_name)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_rec, "csid", &psz_csid)  );
                SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_result, psz_csid, &pvh_names)  );
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_names, psz_branch_name)  );
            }
        }
    }

    *ppvh_results_by_csid = pvh_result;
    pvh_result = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
}

//////////////////////////////////////////////////////////////////

/**
 * Inspect the given candidate branch name and reject if
 * it isn't valid.
 *
 * Return the normalized form.
 *
 */
void SG_vc_branches__normalize_name(SG_context * pCtx,
									const char * pszCandidateBranchName,
									char ** ppszNormalizedBranchName)
{
	char * pszNormalized = NULL;
	SG_uint32 lenNormalized = 0;

	SG_NULLARGCHECK_RETURN( pszCandidateBranchName );	// throw SG_ERR_INVALIDARG for NULL
	SG_NULLARGCHECK_RETURN( ppszNormalizedBranchName );

	if (!*pszCandidateBranchName)
		SG_ERR_THROW(  SG_ERR_INVALID_BRANCH_NAME  );

	SG_ERR_CHECK(  SG_sz__trim(pCtx, pszCandidateBranchName,
							   &lenNormalized, &pszNormalized)  );
	if ((lenNormalized==0) || (!pszNormalized))
		SG_ERR_THROW(  SG_ERR_INVALID_BRANCH_NAME  );

	*ppszNormalizedBranchName = pszNormalized;
	pszNormalized = NULL;
	return;

fail:
	SG_NULLFREE(pCtx, pszNormalized);
}

//////////////////////////////////////////////////////////////////

/**
 * Inspect the given "candidate" branch name (probably from either
 * --attach or --attach-new) and optionally validate it.
 * 
 * [] is syntactically valid?
 * [] if must exist, does it?
 * [] if must not exist, does it?
 *
 * Return normalized name or throw.
 * 
 */
void SG_vc_branches__check_attach_name(SG_context * pCtx,
									   SG_repo * pRepo,
									   const char * pszCandidate,
									   SG_vc_branches__check_attach_name__flags flags,
									   SG_bool bValidate,
									   char ** ppszNormalized)
{
	char * pszNormalized = NULL;
	SG_vhash * pvhPile = NULL;
	SG_vhash * pvhPileOfBranches = NULL;	// we do not own this
	SG_bool bHas;

	// Always trim whitespace and put in canonical form.
	// (This has always been a requirement, I think.)
	//
	// Optionally disallow invalid characters in the name.
	// This constraint was recently added in 2.0 so we
	// enforce this when creating a branch, but allow
	// trash so that the user can refer to an existing
	// branch that was created before the restrictions.

	SG_ERR_CHECK(  SG_vc_branches__normalize_name(pCtx, pszCandidate, &pszNormalized)  );
	if (bValidate)
		SG_ERR_CHECK(  SG_vc_branches__ensure_valid_name(pCtx, pszNormalized)  );

	if ((flags == SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_EXIST)
		|| (flags == SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_NOT_EXIST))
	{
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPile)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhPile, "branches", &pvhPileOfBranches)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhPileOfBranches, pszNormalized, &bHas)  );

		if (flags == SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_EXIST)
		{
			if (!bHas)
				SG_ERR_THROW2(  SG_ERR_BRANCH_NOT_FOUND,
								(pCtx, "'%s'", pszNormalized)  );
		}
		else
		{
			if (bHas)
				SG_ERR_THROW2(  SG_ERR_BRANCH_ALREADY_EXISTS,
								(pCtx, "'%s'", pszNormalized)  );
		}
	}

	*ppszNormalized = pszNormalized;
	pszNormalized = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhPile);
	SG_NULLFREE(pCtx, pszNormalized);
}
