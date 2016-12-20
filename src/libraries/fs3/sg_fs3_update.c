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

#include "sg_fs3__private.h"

static void call_end_update_multiple_and_free(
    SG_context * pCtx,
    SG_rbtree* prb_treendx
    )
{
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    SG_treendx* pTreeNdx = NULL;

    // and the treendxes
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_treendx, &b, NULL, (void**) &pTreeNdx)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_treendx__end_update_multiple(pCtx, pTreeNdx)  );
        SG_TREENDX_NULLFREE(pCtx, pTreeNdx);

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, NULL, (void**) &pTreeNdx)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

static void call_abort_update_multiple(
    SG_context * pCtx,
    SG_rbtree* prb_treendx
    )
{
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    SG_treendx* pTreeNdx = NULL;

    // and the treendxes
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_treendx, &b, NULL, (void**) &pTreeNdx)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_treendx__abort_update_multiple(pCtx, pTreeNdx)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, NULL, (void**) &pTreeNdx)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

static void call_update_multiple(
    SG_context * pCtx,
    SG_rbtree* prb_treendx
    )
{
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    const char* psz_dagnum = NULL;
    SG_treendx* pTreeNdx = NULL;

    // and the treendxes
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_treendx, &b, &psz_dagnum, (void**) &pTreeNdx)  );
    while (b)
    {
        SG_ERR_CHECK(  SG_treendx__begin_update_multiple(pCtx, pTreeNdx)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_dagnum, (void**) &pTreeNdx)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    return;

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_ERR_IGNORE(  call_abort_update_multiple(pCtx,
                prb_treendx
                )  );
}

void sg_fs3_create_schema_token(
    SG_context * pCtx,
    SG_varray* pva,
    SG_string** ppstr_token
    )
{
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_varray__to_json(pCtx, pva, pstr)  );

    *ppstr_token = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void sg_fs3_get_list_of_templates(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        SG_varray** ppva
        )
{
    SG_changeset* pcs = NULL;
    SG_rbtree* prb_leaves = NULL;
    SG_rbtree_iterator* pit = NULL;
    SG_vhash* pvh_templates = NULL;

    SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, dagnum, &prb_leaves)  );
    if (prb_leaves)
    {
        SG_bool b = SG_FALSE;
        const char* psz_csid = NULL;

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_templates)  );
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_leaves, &b, &psz_csid, NULL)  );
        while (b)
        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;
            SG_varray* pva_templates = NULL;

            SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, psz_csid, &pcs)  );
            SG_ERR_CHECK(  SG_changeset__db__get_all_templates(pCtx, pcs, &pva_templates)  );
            if (pva_templates)
            {
                SG_ERR_CHECK(  SG_varray__count(pCtx, pva_templates, &count)  );
                for (i=0; i<count; i++)
                {
                    const char* psz_hid_template = NULL;

                    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_templates, i, &psz_hid_template)  );
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_templates, psz_hid_template)  );
                }
            }
            SG_CHANGESET_NULLFREE(pCtx, pcs);

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_csid, NULL)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

    if (pvh_templates)
    {
        SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_templates, SG_FALSE, SG_vhash_sort_callback__increasing)  );
        SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvh_templates, ppva)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, ppva)  );
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_templates);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_RBTREE_NULLFREE(pCtx, prb_leaves);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
}

static void my_fetch_all_templates(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        SG_varray* pva,
        SG_vector** ppvec
        )
{
    SG_vector* pvec = NULL;
    SG_zingtemplate* pzt = NULL;

    SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pvec, 1)  );

    if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
    {
        // TODO hardwired templates need versioning
        SG_zing__get_cached_template__static_dagnum(pCtx, dagnum, &pzt);
        if (
                SG_CONTEXT__HAS_ERR(pCtx)
                && (SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
                )
        {
            SG_context__err_reset(pCtx);
            SG_VECTOR_NULLFREE(pCtx, pvec);
        }
        else
        {
            SG_ERR_CHECK(  SG_vector__append(pCtx, pvec, pzt, NULL)  );
        }
    }
    else
    {
        SG_uint32 count_templates = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_templates)  );
        for (i=0; i<count_templates; i++)
        {
            const char* psz_hid = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz_hid)  );
            SG_ERR_CHECK(  SG_zing__get_cached_template__hid_template(pCtx, pRepo, dagnum, psz_hid, &pzt)  );
            SG_ERR_CHECK(  SG_vector__append(pCtx, pvec, pzt, NULL)  );
        }
    }

    if (pvec)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_vector__length(pCtx, pvec, &count)  );
        if (0 == count)
        {
            SG_VECTOR_NULLFREE(pCtx, pvec);
        }
    }

    *ppvec = pvec;
    pvec = NULL;

fail:
    SG_VECTOR_NULLFREE(pCtx, pvec);
}

void sg_repo__rebuild_dbndxes(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_rbtree* prb_new_dagnodes,
    SG_get_dbndx_path_callback* pfn_get_dbndx_path,
    void* p_arg_get_dbndx,
    SG_get_sql_callback* pfn_get_sql,
    void* p_arg_get_sql
    )
{
	SG_stringarray* psa_new = NULL;
    SG_uint32 count = 0;
    const char* psz_dagnum = NULL;
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    SG_dbndx_update* pndx = NULL;
    SG_uint32 i = 0;
    SG_pathname* pPath_dbndx = NULL;

    SG_string* pstr_new_schema = NULL;
    SG_string* pstr_new_schema_token = NULL;

    SG_varray* pva_templates = NULL;
    SG_vector* pvec_templates = NULL;
    SG_vhash* pvh_schema = NULL;

    SG_vhash* pvh_dbndx_waiting = NULL;

    SG_changeset* pcs = NULL;
    sqlite3* psql_new = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Rebuilding repository indexes", SG_LOG__FLAG__NONE)  );

    // TODO set_steps
   
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_dbndx_waiting)  );

    // now iterate over all the dags, and all changesets in each dag
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_new_dagnodes, &b, &psz_dagnum, (void**) &psa_new)  );
    while (b)
    {
        SG_uint64 iDagNum = 0;

        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &iDagNum)  );

        if (SG_DAGNUM__IS_NOTHING(iDagNum))
        {
            goto skip_this;
        }

        // now iterate over every changeset in this dag

        SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_new, &count )  );
        for (i=0; i<count; i++)
        {
            const char* psz_csid = NULL;

            SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_new, i, &psz_csid)  );

            if (SG_DAGNUM__IS_DB(iDagNum))
            {
                SG_vhash* pvh_this_dagnum = NULL;
                SG_vhash* pvh_this_dagnum_cs = NULL;
                SG_bool b_already = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_dbndx_waiting, psz_dagnum, &b_already)  );
                if (b_already)
                {
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_dbndx_waiting, psz_dagnum, &pvh_this_dagnum)  );
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_this_dagnum, "changesets", &b_already)  );
                    if (b_already)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_this_dagnum_cs)  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_this_dagnum_cs)  );
                    }
                }
                else
                {
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_dbndx_waiting, psz_dagnum, &pvh_this_dagnum)  );
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_this_dagnum_cs)  );
                }

                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_this_dagnum_cs, psz_csid)  );
            }
        }

skip_this:
        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_dagnum, (void**) &psa_new)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    //SG_VHASH_STDOUT(pvh_dbndx_waiting);

    {
        SG_uint32 count_dags = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dbndx_waiting, &count_dags)  );

        // first make sure each dbndx is ready
        for (i=0; i<count_dags; i++)
        {
            SG_uint64 dagnum = 0;
            const char* psz_dagnum = NULL;
            SG_bool b_exists = SG_FALSE;
            SG_vhash* pvh_this_dagnum = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_dbndx_waiting, i, &psz_dagnum, &pvh_this_dagnum)  );
            SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
            {
                SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_templates)  );
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_templates, "1")  );
            }
            else
            {
                SG_ERR_CHECK(  sg_fs3_get_list_of_templates(pCtx, pRepo, dagnum, &pva_templates)  );
            }

            SG_ERR_CHECK(  pfn_get_dbndx_path(pCtx, p_arg_get_dbndx, dagnum, &pPath_dbndx)  );
            SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_dbndx, &b_exists, NULL, NULL)  );
            if (b_exists)
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            else
            {
                SG_ERR_CHECK(  my_fetch_all_templates(pCtx, pRepo, dagnum, pva_templates, &pvec_templates)  );
                if (pvec_templates)
                {
                    SG_ERR_CHECK(  SG_dbndx__create_composite_schema_for_all_templates(pCtx, dagnum, pvec_templates, &pvh_schema)  );
                    SG_VECTOR_NULLFREE(pCtx, pvec_templates);
                    //SG_VHASH_STDOUT(pvh_schema);
                    SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath_dbndx, SG_SQLITE__SYNC__OFF, &psql_new)  );
                    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql_new, "PRAGMA temp_store=2")  );

                    SG_ERR_CHECK(  sg_fs3_create_schema_token(pCtx, pva_templates, &pstr_new_schema_token)  );
                    SG_ERR_CHECK(  SG_dbndx__create_new(pCtx, dagnum, psql_new, SG_string__sz(pstr_new_schema_token), pvh_schema)  );
                    SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql_new)  );
                    psql_new = NULL;
                    SG_VHASH_NULLFREE(pCtx, pvh_schema);
                }
                else
                {
                    //SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Cannot index dag: %s", psz_dagnum)  );
                }
            }

            SG_VARRAY_NULLFREE(pCtx, pva_templates);
            SG_CHANGESET_NULLFREE(pCtx, pcs);
            SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
            SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
        }

        // now update each one
        for (i=0; i<count_dags; i++)
        {
            SG_uint64 dagnum = 0;
            SG_vhash* pvh_this_dagnum = NULL;
            const char* psz_dagnum = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_dbndx_waiting, i, &psz_dagnum, &pvh_this_dagnum)  );

            {
                SG_vhash* pvh_changesets = NULL;
                SG_bool b_has = SG_FALSE;
                sqlite3* psql = NULL;
                SG_bool b_exists = SG_FALSE;

                SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

                // changesets
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_this_dagnum, "changesets", &b_has)  );
                if (b_has)
                {
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_changesets)  );
                }

                // update the dbndx
                SG_ERR_CHECK(  pfn_get_dbndx_path(pCtx, p_arg_get_dbndx, dagnum, &pPath_dbndx)  );
                SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_dbndx, &b_exists, NULL, NULL)  );
                if (b_exists)
                {
                    SG_ERR_CHECK(  pfn_get_sql(pCtx, p_arg_get_sql, pPath_dbndx, &psql)  );
                    SG_ERR_CHECK(  SG_dbndx_update__open(pCtx, pRepo, dagnum, psql, &pndx)  );
                    SG_ERR_CHECK(  SG_dbndx_update__do(pCtx, pndx, pvh_changesets)  );
                    SG_DBNDX_UPDATE_NULLFREE(pCtx, pndx);
                }
                SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
            }
        }
    }

fail:
    if (psql_new)
    {
        SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql_new)  );
    }
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
    SG_STRING_NULLFREE(pCtx, pstr_new_schema);
    SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
    SG_VHASH_NULLFREE(pCtx, pvh_dbndx_waiting);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VECTOR_NULLFREE(pCtx, pvec_templates);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
}

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
    )
{
    const char* psz_dagnum = NULL;
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
	SG_stringarray* psa_new = NULL;
    SG_uint32 count = 0;
    SG_dbndx_update* pndx = NULL;
    SG_treendx* pTreeNdx = NULL;
    SG_uint32 i = 0;
    SG_pathname* pPath_dbndx = NULL;

    SG_string* pstr_new_schema = NULL;
    SG_string* pstr_new_schema_token = NULL;
    char* psz_existing_schema_token = NULL;
    char* psz_existing_schema = NULL;

    SG_vector* pvec_templates = NULL;
    SG_vhash* pvh_schema = NULL;

    SG_rbtree* prb_treendx = NULL;

    SG_varray* pva_templates = NULL;

    SG_vhash* pvh_dbndx_waiting = NULL;
    SG_vhash* pvh_dbndx_needs_rebuild = NULL;

    sqlite3* psql_new = NULL;

    SG_vhash* pvh_pcs = NULL;
    SG_uint32 count_all_changesets = 0;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Indexing", SG_LOG__FLAG__NONE)  );

    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &prb_treendx)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_dbndx_waiting)  );

    // -------- count all the changesets needing to be indexed
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_new_dagnodes, &b, &psz_dagnum, (void**) &psa_new)  );
    while (b)
    {
        SG_uint32 count_changesets = 0;

        SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_new, &count_changesets)  );
        count_all_changesets += count_changesets;

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_dagnum, (void**) &psa_new)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_all_changesets, "changesets")  );

    // first we get all the indexes we are going to need

    // the following loop gets most of the indexes we need.  see the next loop.
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_new_dagnodes, &b, &psz_dagnum, (void**) &psa_new)  );
    while (b)
    {
        SG_uint64 iDagNum = 0;

        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &iDagNum)  );

        // -------- treendx
        if (SG_DAGNUM__VERSION_CONTROL == (iDagNum))
        {
            SG_ERR_CHECK(  pfn_get_treendx(pCtx, p_arg_get_treendx, iDagNum, &pTreeNdx)  );
            SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_treendx, psz_dagnum, pTreeNdx)  );
            pTreeNdx = NULL;
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_dagnum, (void**) &psa_new)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    // now grab locks for every ndx
    SG_ERR_CHECK(  call_update_multiple(pCtx,
                prb_treendx
                )  );

    // now iterate over all the dags, and all changesets in each dag
    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_new_dagnodes, &b, &psz_dagnum, (void**) &psa_new)  );
    while (b)
    {
        SG_uint64 iDagNum = 0;

        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &iDagNum)  );

        if (SG_DAGNUM__IS_NOTHING(iDagNum))
        {
            goto skip_this;
        }

        // now iterate over every changeset in this dag

        SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_new, &count )  );
        for (i=0; i<count; i++)
        {
            const char* psz_csid = NULL;

            SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_new, i, &psz_csid)  );

            if (SG_DAGNUM__IS_DB(iDagNum))
            {
                SG_vhash* pvh_this_dagnum = NULL;
                SG_vhash* pvh_this_dagnum_cs = NULL;
                SG_bool b_already = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_dbndx_waiting, psz_dagnum, &b_already)  );
                if (b_already)
                {
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_dbndx_waiting, psz_dagnum, &pvh_this_dagnum)  );
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_this_dagnum, "changesets", &b_already)  );
                    if (b_already)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_this_dagnum_cs)  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_this_dagnum_cs)  );
                    }
                }
                else
                {
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_dbndx_waiting, psz_dagnum, &pvh_this_dagnum)  );
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_this_dagnum_cs)  );
                }

                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_this_dagnum_cs, psz_csid)  );
            }
            else if (SG_DAGNUM__VERSION_CONTROL == (iDagNum))
            {
                SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_treendx, psz_dagnum, NULL, (void**) &pTreeNdx)  );
                SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid, &pvh_pcs)  );
                SG_ERR_CHECK(  SG_treendx__update_one_changeset(pCtx, pTreeNdx, psz_csid, pvh_pcs)  );
                SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
                // done with this changeset
                SG_VHASH_NULLFREE(pCtx, pvh_pcs);
            }
            else
            {
                // TODO should never happen
            }
        }

skip_this:
        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_dagnum, (void**) &psa_new)  );
    }
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    SG_ERR_CHECK(  call_end_update_multiple_and_free(pCtx, 
                prb_treendx
                )  );

    // TODO the prb changesets thingie could maybe be freed now

    {
        SG_uint32 count_dags = 0;

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_dbndx_needs_rebuild)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dbndx_waiting, &count_dags)  );

        // first make sure each dbndx is ready
        for (i=0; i<count_dags; i++)
        {
            SG_uint64 dagnum = 0;
            const char* psz_dagnum = NULL;
            SG_bool b_exists = SG_FALSE;
            SG_vhash* pvh_this_dagnum = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_dbndx_waiting, i, &psz_dagnum, &pvh_this_dagnum)  );
            SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
            {
                SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_templates)  );
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_templates, "1")  );
            }
            else
            {
                SG_ERR_CHECK(  sg_fs3_get_list_of_templates(pCtx, pRepo, dagnum, &pva_templates)  );
            }

            SG_ERR_CHECK(  sg_fs3_create_schema_token(pCtx, pva_templates, &pstr_new_schema_token)  );
            SG_ERR_CHECK(  pfn_get_dbndx_path(pCtx, p_arg_get_dbndx, dagnum, &pPath_dbndx)  );
            SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_dbndx, &b_exists, NULL, NULL)  );
            if (b_exists)
            {
                sqlite3* psql = NULL;

                // now check to see if the templates have changed in such a way
                // as to require a dbndx rebuild

                SG_ERR_CHECK(  pfn_get_sql(pCtx, p_arg_get_sql, pPath_dbndx, &psql)  );
                SG_ERR_CHECK(  SG_dbndx__get_schema_info(pCtx, psql, &psz_existing_schema_token, &psz_existing_schema)  );

                if (0 != strcmp(SG_string__sz(pstr_new_schema_token), psz_existing_schema_token))
                {
                    // the token is different.
                    // now construct the schema itself and compare that.
                    SG_ERR_CHECK(  my_fetch_all_templates(pCtx, pRepo, dagnum, pva_templates, &pvec_templates)  );
                    SG_ERR_CHECK(  SG_dbndx__create_composite_schema_for_all_templates(pCtx, dagnum, pvec_templates, &pvh_schema)  );
                    SG_VECTOR_NULLFREE(pCtx, pvec_templates);

                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_new_schema)  );
                    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh_schema, pstr_new_schema)  );
                    if (0 == strcmp(psz_existing_schema, SG_string__sz(pstr_new_schema)))
                    {
                        // the schema token changed but the schema did not.
                        // this means the templates changed but not in a way
                        // that the schema cares about.
                       
                        SG_ERR_CHECK(  SG_dbndx__update_schema_token(pCtx, psql, SG_string__sz(pstr_new_schema_token))  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_dbndx_needs_rebuild, psz_dagnum)  );
                    }
                    SG_STRING_NULLFREE(pCtx, pstr_new_schema);
                    SG_VHASH_NULLFREE(pCtx, pvh_schema);
                }

                SG_NULLFREE(pCtx, psz_existing_schema_token);
                SG_NULLFREE(pCtx, psz_existing_schema);
            }
            else
            {
                SG_ERR_CHECK(  my_fetch_all_templates(pCtx, pRepo, dagnum, pva_templates, &pvec_templates)  );
                if (pvec_templates)
                {
                    SG_ERR_CHECK(  SG_dbndx__create_composite_schema_for_all_templates(pCtx, dagnum, pvec_templates, &pvh_schema)  );
                    SG_VECTOR_NULLFREE(pCtx, pvec_templates);

                    SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath_dbndx, SG_SQLITE__SYNC__OFF, &psql_new)  );
                    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql_new, "PRAGMA temp_store=2")  );

                    SG_ERR_CHECK(  SG_dbndx__create_new(pCtx, dagnum, psql_new, SG_string__sz(pstr_new_schema_token), pvh_schema)  );
                    SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql_new)  );
                    psql_new = NULL;
                    if (pvh_fresh)
                    {
                        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_fresh, psz_dagnum,  SG_pathname__sz(pPath_dbndx))  );
                    }
                    SG_VHASH_NULLFREE(pCtx, pvh_schema);
                }
                else
                {
                    //SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Cannot index dag: %s", psz_dagnum)  );
                }
            }

            SG_VARRAY_NULLFREE(pCtx, pva_templates);
            SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
            SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
        }

        // now update each one
        for (i=0; i<count_dags; i++)
        {
            SG_uint64 dagnum = 0;
            SG_vhash* pvh_this_dagnum = NULL;
            const char* psz_dagnum = NULL;
            SG_bool b_needs_rebuild = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_dbndx_waiting, i, &psz_dagnum, &pvh_this_dagnum)  );
            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_dbndx_needs_rebuild, psz_dagnum, &b_needs_rebuild)  );

            if (!b_needs_rebuild)
            {
                SG_vhash* pvh_changesets = NULL;
                SG_bool b_has = SG_FALSE;
                sqlite3* psql = NULL;
                SG_bool b_exists = SG_FALSE;

                SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

                // changesets
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_this_dagnum, "changesets", &b_has)  );
                if (b_has)
                {
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_this_dagnum, "changesets", &pvh_changesets)  );
                }

                // update the dbndx
                SG_ERR_CHECK(  pfn_get_dbndx_path(pCtx, p_arg_get_dbndx, dagnum, &pPath_dbndx)  );
                SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_dbndx, &b_exists, NULL, NULL)  );
                if (b_exists)
                {
                    SG_ERR_CHECK(  pfn_get_sql(pCtx, p_arg_get_sql, pPath_dbndx, &psql)  );
                    SG_ERR_CHECK(  SG_dbndx_update__open(pCtx, pRepo, dagnum, psql, &pndx)  );
                    SG_ERR_CHECK(  SG_dbndx_update__do(pCtx, pndx, pvh_changesets)  );
                    SG_DBNDX_UPDATE_NULLFREE(pCtx, pndx);
                }
                SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
            }
        }
    }

    {
        SG_uint32 count_needing_rebuild = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dbndx_needs_rebuild, &count_needing_rebuild)  );
        if (0 == count_needing_rebuild)
        {
            SG_VHASH_NULLFREE(pCtx, pvh_dbndx_needs_rebuild);
        }
    }

    *ppvh_rebuild = pvh_dbndx_needs_rebuild;
    pvh_dbndx_needs_rebuild = NULL;

fail:
    if (psql_new)
    {
        SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql_new)  );
    }
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
    SG_STRING_NULLFREE(pCtx, pstr_new_schema);
    SG_NULLFREE(pCtx, psz_existing_schema_token);
    SG_NULLFREE(pCtx, psz_existing_schema);
    SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
    SG_RBTREE_NULLFREE(pCtx, prb_treendx);
    SG_VHASH_NULLFREE(pCtx, pvh_dbndx_waiting);
    SG_VHASH_NULLFREE(pCtx, pvh_dbndx_needs_rebuild);
    SG_VHASH_NULLFREE(pCtx, pvh_pcs);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VECTOR_NULLFREE(pCtx, pvec_templates);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_log__pop_operation(pCtx);
}

void sg_repo__update_ndx__all__db(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_get_dbndx_path_callback* pfn_get_dbndx_path,
    void* p_arg_get_dbndx
    )
{
    SG_dbndx_update* pndx = NULL;
    SG_uint32 i = 0;
    SG_pathname* pPath_dbndx = NULL;

    SG_string* pstr_new_schema_token = NULL;

    SG_vector* pvec_templates = NULL;
    SG_vhash* pvh_schema = NULL;

    SG_varray* pva_templates = NULL;

    sqlite3* psql_new = NULL;

    SG_changeset* pcs = NULL;
    SG_uint32 count_dags = 0;
    SG_uint64* pa_dagnums = NULL;
    SG_stringarray* psa_changesets = NULL;
    SG_uint32 count_all_changesets = 0;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Indexing database", SG_LOG__FLAG__NONE)  );

    SG_ERR_CHECK(  sg_fs3__list_dags(pCtx, pRepo, &count_dags, &pa_dagnums)  );

    for (i=0; i<count_dags; i++)
    {
        SG_uint64 dagnum = pa_dagnums[i];
        SG_bool b_index = SG_FALSE;

        if (SG_DAGNUM__VERSION_CONTROL == dagnum)
        {
        }
        else if (SG_DAGNUM__IS_DB(dagnum))
        {
            b_index = SG_TRUE;
            // unless...
            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
            {
                SG_zingtemplate* pzt = NULL;
                SG_zing__get_cached_template__static_dagnum(pCtx, dagnum, &pzt);
                if (
                        SG_CONTEXT__HAS_ERR(pCtx)
                        && (SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
                        )
                {
                    SG_context__err_reset(pCtx);
                    b_index = SG_FALSE;
                }
                else
                {
                    SG_ERR_CHECK_CURRENT;
                }
            }
        }

        if (b_index)
        {
            SG_uint32 count_changesets_in_this_dag = 0;
            SG_ERR_CHECK(  sg_repo__fs3__count_all_dagnodes(pCtx, pRepo, dagnum, &count_changesets_in_this_dag)  );
            count_all_changesets += count_changesets_in_this_dag;
        }
    }

	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_all_changesets, "changesets")  );

    for (i=0; i<count_dags; i++)
    {
        SG_uint64 dagnum = pa_dagnums[i];

        if (SG_DAGNUM__IS_DB(dagnum))
        {
            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
            {
                SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_templates)  );
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_templates, "1")  );
            }
            else
            {
                SG_ERR_CHECK(  sg_fs3_get_list_of_templates(pCtx, pRepo, dagnum, &pva_templates)  );
            }

            SG_ERR_CHECK(  my_fetch_all_templates(pCtx, pRepo, dagnum, pva_templates, &pvec_templates)  );
            if (pvec_templates)
            {
                SG_ERR_CHECK(  sg_fs3_create_schema_token(pCtx, pva_templates, &pstr_new_schema_token)  );
                SG_ERR_CHECK(  pfn_get_dbndx_path(pCtx, p_arg_get_dbndx, dagnum, &pPath_dbndx)  );

                SG_ERR_CHECK(  SG_dbndx__create_composite_schema_for_all_templates(pCtx, dagnum, pvec_templates, &pvh_schema)  );
                SG_VECTOR_NULLFREE(pCtx, pvec_templates);

                SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath_dbndx, SG_SQLITE__SYNC__OFF, &psql_new)  );
                SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql_new, "PRAGMA temp_store=2")  );

                SG_ERR_CHECK(  SG_dbndx__create_new(pCtx, dagnum, psql_new, SG_string__sz(pstr_new_schema_token), pvh_schema)  );
                SG_VHASH_NULLFREE(pCtx, pvh_schema);
                SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);

                SG_ERR_CHECK(  SG_dbndx_update__open(pCtx, pRepo, dagnum, psql_new, &pndx)  );
                SG_ERR_CHECK(  SG_dbndx_update__do(pCtx, pndx, NULL)  );
                SG_DBNDX_UPDATE_NULLFREE(pCtx, pndx);
                SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql_new)  );
                psql_new = NULL;

                SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
            }
            else
            {
                //SG_ERR_CHECK(  SG_log__report_warning(pCtx, "Cannot index dag: %s", psz_dagnum)  );
            }

            SG_VARRAY_NULLFREE(pCtx, pva_templates);
        }
    }

fail:
    if (psql_new)
    {
        SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql_new)  );
    }
    SG_NULLFREE(pCtx, pa_dagnums);
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
    SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VECTOR_NULLFREE(pCtx, pvec_templates);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_STRINGARRAY_NULLFREE(pCtx, psa_changesets);
    SG_log__pop_operation(pCtx);
}

void sg_repo__update_ndx__all__tree(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_get_treendx_callback* pfn_get_treendx,
    void* p_arg_get_treendx
    )
{
    SG_treendx* pTreeNdx = NULL;
    SG_uint32 i = 0;

    SG_vhash* pvh_pcs = NULL;
    SG_uint32 count_dags = 0;
    SG_uint64* pa_dagnums = NULL;
    SG_stringarray* psa_changesets = NULL;
    SG_uint32 count_changesets = 0;
    SG_uint32 count_all_changesets = 0;
    SG_bool b_has_version_control = SG_FALSE;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Indexing tree paths", SG_LOG__FLAG__NONE)  );

    SG_ERR_CHECK(  sg_fs3__list_dags(pCtx, pRepo, &count_dags, &pa_dagnums)  );

    for (i=0; i<count_dags; i++)
    {
        SG_uint64 dagnum = pa_dagnums[i];

        if (SG_DAGNUM__VERSION_CONTROL == dagnum)
        {
            SG_uint32 count_changesets_in_this_dag = 0;
            SG_ERR_CHECK(  sg_repo__fs3__count_all_dagnodes(pCtx, pRepo, dagnum, &count_changesets_in_this_dag)  );
            count_all_changesets += count_changesets_in_this_dag;
            b_has_version_control = SG_TRUE;
            break;
        }
    }

	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_all_changesets, "changesets")  );

    // -------- treendx
    if (b_has_version_control)
    {
        SG_ERR_CHECK(  pfn_get_treendx(pCtx, p_arg_get_treendx, SG_DAGNUM__VERSION_CONTROL, &pTreeNdx)  );
        SG_ERR_CHECK(  sg_repo__fs3__fetch_all_dagnodes(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &psa_changesets)  );
        SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_changesets, &count_changesets)  );
        SG_ERR_CHECK(  SG_treendx__begin_update_multiple(pCtx, pTreeNdx)  );
        for (i=0; i<count_changesets; i++)
        {
            const char* psz_csid = NULL;

            SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_changesets, i, &psz_csid)  );
            SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid, &pvh_pcs)  );
            SG_ERR_CHECK(  SG_treendx__update_one_changeset(pCtx, pTreeNdx, psz_csid, pvh_pcs)  );
            SG_VHASH_NULLFREE(pCtx, pvh_pcs);

            if (
                    (0 == ((i+1) % 10))
                    || ((i+1) >= count_changesets)
               )
            {
                SG_ERR_CHECK(  SG_log__set_finished(pCtx, i+1)  );
            }
        }
        SG_ERR_CHECK(  SG_treendx__end_update_multiple(pCtx, pTreeNdx)  );
        SG_TREENDX_NULLFREE(pCtx, pTreeNdx);
        SG_STRINGARRAY_NULLFREE(pCtx, psa_changesets);
    }

fail:
    SG_NULLFREE(pCtx, pa_dagnums);
    SG_STRINGARRAY_NULLFREE(pCtx, psa_changesets);
    SG_log__pop_operation(pCtx);
}




