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

struct _SG_dbndx_update
{
	SG_repo* pRepo;
    SG_uint64 iDagNum;

    sqlite3* psql;
	SG_bool			bInTransaction;
    SG_vhash* pvh_schema;

    struct
    {
        SG_bool b_in_update;
        SG_uint32 count_so_far;
        sqlite3_stmt* pStmt_rowid_csid;
        sqlite3_stmt* pStmt_rowid_hidrec;
        sqlite3_stmt* pStmt_history;
        sqlite3_stmt* pStmt_delta_adds;
        sqlite3_stmt* pStmt_delta_dels;
        SG_rbtree* prb_stmt_cache;

        SG_ihash* pih_hidrec;
        SG_ihash* pih_csid;
    } update;
};

#define MY_SLEEP_MS      100
#define MY_TIMEOUT_MS  30000

void SG_dbndx_update__open(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    sqlite3* psql,
	SG_dbndx_update** ppNew
	)
{
	SG_dbndx_update* pndx = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pndx)  );

    pndx->pRepo = pRepo;
    pndx->iDagNum = iDagNum;
	pndx->bInTransaction = SG_FALSE;
    pndx->psql = psql;

    // in this case, we do want the composite schema
    SG_ERR_CHECK(  sg_dbndx__load_schema_from_sql(pCtx, pndx->psql, "composite_schema", &pndx->pvh_schema)  );

	*ppNew = pndx;
    pndx = NULL;

fail:
    SG_dbndx_update__free(pCtx, pndx);
}

void SG_dbndx_update__free(SG_context* pCtx, SG_dbndx_update* pdbc)
{
	if (!pdbc)
		return;

    SG_VHASH_NULLFREE(pCtx, pdbc->pvh_schema);
	SG_NULLFREE(pCtx, pdbc);
}

void sg_dbndx__get_csid(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        const char* psz_csid,
        SG_int64* pi_row
        )
{
    SG_int64 row = -1;

    SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pndx->update.pih_csid, psz_csid, &row)  );
    if (row < 0)
    {
        SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pndx->psql, &row, "SELECT csidrow FROM csids WHERE csid='%s'", psz_csid)  );
    }

    *pi_row = row;

fail:
    ;
}

void sg_dbndx__add_csid(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        const char* psz_csid,
        SG_int32 gen,
        SG_int64* pi_row
        )
{
    SG_int64 row = -1;

    SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pndx->update.pih_csid, psz_csid, &row)  );
    if (row < 0)
    {
        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pndx->update.pStmt_rowid_csid)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pndx->update.pStmt_rowid_csid)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pndx->update.pStmt_rowid_csid, 1, psz_csid)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pndx->update.pStmt_rowid_csid, 2, gen)  );
        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pndx->update.pStmt_rowid_csid, SQLITE_DONE)  );

        if (sqlite3_changes(pndx->psql))
        {
            row = sqlite3_last_insert_rowid(pndx->psql);
        }
        else
        {
            SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pndx->psql, &row, "SELECT csidrow FROM csids WHERE csid='%s'", psz_csid)  );
        }
        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pndx->update.pih_csid, psz_csid, row)  );
    }

    *pi_row = row;

fail:
    ;
}

void sg_dbndx__add_hidrec(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        const char* psz_hidrec,
        SG_int64* pi_row
        )
{
    SG_int64 row = -1;

    SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pndx->update.pih_hidrec, psz_hidrec, &row)  );
    if (row < 0)
    {
        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pndx->update.pStmt_rowid_hidrec)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pndx->update.pStmt_rowid_hidrec)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pndx->update.pStmt_rowid_hidrec, 1, psz_hidrec)  );
        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pndx->update.pStmt_rowid_hidrec, SQLITE_DONE)  );

        if (sqlite3_changes(pndx->psql))
        {
            row = sqlite3_last_insert_rowid(pndx->psql);
        }
        else
        {
            SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pndx->psql, &row, "SELECT hidrecrow FROM hidrecs WHERE hidrec='%s'", psz_hidrec)  );
        }
        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pndx->update.pih_hidrec, psz_hidrec, row)  );
    }

    *pi_row = row;

fail:
    ;
}

static void sg_dbndx__add_to_deltas(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        sqlite3_stmt* pStmt,
        SG_int64 csidrow,
        SG_int64 csidrow_parent,
        SG_vhash* pvh_records
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_records, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_hidrec = NULL;
        SG_int64 hidrecrow = -1;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_records, i, &psz_hidrec, NULL)  );

        SG_ERR_CHECK(  sg_dbndx__add_hidrec(pCtx, pndx, psz_hidrec, &hidrecrow)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, hidrecrow)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 2, csidrow)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 3, csidrow_parent)  );

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    }

fail:
    ;
}

static void SG_dbndx_update__figure_out_new_records(
        SG_context* pCtx, 
        SG_vhash* pvh_changes,
        SG_vhash** ppvh_new
        )
{
    SG_uint32 count_parents = 0;
    SG_vhash* pvh_new = NULL;
    SG_vhash* pvh_intersection = NULL;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );

    if (count_parents < 2)
    {
        SG_ERR_THROW(  SG_ERR_INVALIDARG  );
    }

    {
        const char* psz_csid_parent = NULL;
        SG_vhash* pvh_changes_one_parent = NULL;
        SG_vhash* pvh_record_adds = NULL;
        SG_uint32 count_adds = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, 0, &psz_csid_parent, &pvh_changes_one_parent)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_record_adds)  );
        if (pvh_record_adds)
        {
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_record_adds, &count_adds)  );
            if (count_adds)
            {
                SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &pvh_new, pvh_record_adds)  );
            }
        }
    }

    if (
            pvh_new  
            && (count_parents > 1)
            )
    {
        SG_uint32 i_parent;

        for (i_parent=1; i_parent<count_parents; i_parent++)
        {
            const char* psz_csid_parent = NULL;
            SG_vhash* pvh_changes_one_parent = NULL;
            SG_vhash* pvh_record_adds = NULL;
            SG_uint32 count_adds = 0;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_changes_one_parent)  );

            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_record_adds)  );
            if (pvh_record_adds)
            {
                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_record_adds, &count_adds)  );
                if (count_adds)
                {
                    SG_uint32 count_new = 0;

                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_new, &count_new)  );
                    if (count_new)
                    {
                        SG_uint32 i = 0;

                        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_intersection)  );
                        for (i=0; i<count_new; i++)
                        {
                            const char* psz_hid_rec = NULL;
                            SG_bool b_has = SG_FALSE;

                            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_new, i, &psz_hid_rec, NULL)  );
                            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_record_adds, psz_hid_rec, &b_has)  );
                            if (b_has)
                            {
                                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_intersection, psz_hid_rec)  );
                            }
                        }

                        SG_VHASH_NULLFREE(pCtx, pvh_new);
                        pvh_new = pvh_intersection;
                        pvh_intersection = NULL;

                        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_new, &count_new)  );
                        if (0 == count_new)
                        {
                            SG_VHASH_NULLFREE(pCtx, pvh_new);
                        }
                    }
                }
            }
        }
    }

    *ppvh_new = pvh_new;
    pvh_new = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_new);
    SG_VHASH_NULLFREE(pCtx, pvh_intersection);
}

static void sg_dbndx__add_record_to_history_table(
        SG_context* pCtx,
        SG_dbndx_update* pdbc,
        SG_int64 hidrecrow,
        SG_int64 csidrow
        )
{
    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pdbc->update.pStmt_history)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pdbc->update.pStmt_history)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pdbc->update.pStmt_history, 1, hidrecrow)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pdbc->update.pStmt_history, 2, csidrow)  );

    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pdbc->update.pStmt_history, SQLITE_DONE)  );

fail:
    ;
}

void sg_dbndx__add_db_history_for_new_records(
        SG_context* pCtx,
        SG_dbndx_update* pdbc,
        SG_int64 csidrow,
        SG_vhash* pvh_changes
        )
{
    SG_vhash* pvh_new_freeme = NULL;
    SG_vhash* pvh_new = NULL;
    SG_uint32 count_parents = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
    if (count_parents > 1)
    {
        SG_ERR_CHECK(  SG_dbndx_update__figure_out_new_records(pCtx, pvh_changes, &pvh_new_freeme)  );
        pvh_new = pvh_new_freeme;
    }
    else if (1 == count_parents)
    {
        const char* psz_csid_parent = NULL;
        SG_vhash* pvh_changes_one_parent = NULL;
        SG_vhash* pvh_record_adds = NULL;
        SG_uint32 count_adds = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, 0, &psz_csid_parent, &pvh_changes_one_parent)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_record_adds)  );
        if (pvh_record_adds)
        {
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_record_adds, &count_adds)  );
            if (count_adds)
            {
                pvh_new = pvh_record_adds;
            }
        }
    }

    if (pvh_new)
    {
        SG_uint32 count_new = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_new, &count_new)  );
        if (count_new)
        {
            SG_uint32 i;

            for (i=0; i<count_new; i++)
            {
                const char* psz_hid_rec = NULL;
                SG_int64 hidrecrow = -1;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_new, i, &psz_hid_rec, NULL)  );
                SG_ERR_CHECK(  SG_ihash__get__int64(pCtx, pdbc->update.pih_hidrec, psz_hid_rec, &hidrecrow)  );

                SG_ERR_CHECK(  sg_dbndx__add_record_to_history_table(pCtx, pdbc, hidrecrow, csidrow)  );
            }
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_new_freeme);
}

void sg_dbndx__add_deltas(
        SG_context* pCtx,
        SG_dbndx_update* pdbc,
        SG_int64 csidrow,
        SG_vhash* pvh_changes
        )
{
    SG_uint32 i_parent = 0;
    SG_uint32 count_parents = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
    for (i_parent=0; i_parent<count_parents; i_parent++)
    {
        const char* psz_csid_parent = NULL;
        SG_vhash* pvh_changes_one_parent = NULL;
        SG_vhash* pvh_records = NULL;
        SG_int64 csidrow_parent = -1;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_changes_one_parent)  );

        SG_ERR_CHECK(  sg_dbndx__get_csid(pCtx, pdbc, psz_csid_parent, &csidrow_parent)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_records)  );
        if (pvh_records)
        {
            SG_ERR_CHECK(  sg_dbndx__add_to_deltas(pCtx, pdbc, pdbc->update.pStmt_delta_adds, csidrow, csidrow_parent, pvh_records)  );
        }

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "remove", &pvh_records)  );
        if (pvh_records)
        {
            SG_ERR_CHECK(  sg_dbndx__add_to_deltas(pCtx, pdbc, pdbc->update.pStmt_delta_dels, csidrow, csidrow_parent, pvh_records)  );
        }
    }

fail:
    ;
}

static void sg_dbndx__add_record_to_full_text_search(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        SG_int64 hidrecrow,
        SG_vhash* pvh_rec
        )
{
	SG_uint32 count_fields = 0;
	SG_uint32 i;
    const char* psz_rectype = NULL;
    SG_vhash* pvh_fields = NULL;
    sqlite3_stmt* pStmt = NULL;
    SG_uint32 count_fts_fields = 0;
    SG_uint32 i_field = 0;
    SG_string* pstr = NULL;

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pndx->pvh_schema, 0, &psz_rectype, NULL)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_rec, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
    }

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pndx->pvh_schema, psz_rectype, &pvh_fields)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count_fields)  );

    for (i_field=0; i_field<count_fields; i_field++)
    {
        SG_vhash* pvh_one_field = NULL;
        const char* psz_field_name = NULL;
        SG_bool b_has = SG_FALSE;
        SG_bool b_full_text_search = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i_field, &psz_field_name, &pvh_one_field)  );
        
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_full_text_search)  );
        }
        if (b_full_text_search)
        {
            count_fts_fields++;
        }
    }

    if (count_fts_fields)
    {
        SG_uint32 count_so_far = 0;
        SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pstr, "INSERT INTO \"%s_%s\" (", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );

        for (i=0; i<count_fields; i++)
        {
            SG_vhash* pvh_one_field = NULL;
            const char* psz_field_name = NULL;
            SG_bool b_has = SG_FALSE;
            SG_bool b_full_text_search = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i, &psz_field_name, &pvh_one_field)  );

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_full_text_search)  );
            }

            if (b_full_text_search)
            {
                if (count_so_far)
                {
                    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ",")  );
                }
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, psz_field_name)  );
                count_so_far++;
            }
        }

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ") VALUES (?")  );

        if (count_fts_fields > 1)
        {
            for (i=1; i<count_fts_fields; i++)
            {
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ",?")  );
            }
        }

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ")")  );

        count_so_far = 1;

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr))  );

        for (i=0; i<count_fields; i++)
        {
            SG_vhash* pvh_one_field = NULL;
            const char* psz_field_name = NULL;
            const char* psz_val = NULL;
            SG_bool b_has = SG_FALSE;
            SG_bool b_full_text_search = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i, &psz_field_name, &pvh_one_field)  );

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_full_text_search)  );
            }

            if (b_full_text_search)
            {
                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_rec, psz_field_name, &psz_val)  );
                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, count_so_far++, psz_val)  );
            }
        }

        {
            sqlite3_int64 rowid = -1;

            SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
            SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
            rowid = sqlite3_last_insert_rowid(pndx->psql);

            SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "INSERT OR IGNORE INTO \"%s_map_%s\" (ftsrowid, hidrecrow) VALUES (?, ?)", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );
            SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, rowid)  );
            SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 2, hidrecrow)  );
            SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
            SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__add_record_to_records_table(
        SG_context* pCtx,
        SG_dbndx_update* pndx,
        SG_int64 hidrecrow,
        SG_vhash* pvh
        )
{
	SG_uint32 count_fields = 0;
	SG_uint32 i;
	SG_uint32 ndx_bind = 0;
    const char* psz_rectype = NULL;
    sqlite3_stmt* pStmt = NULL;
    SG_vhash* pvh_fields = NULL;
    SG_string* pstr_sql = NULL;
    SG_uint32 count_extra_ndx_columns = 0;
    SG_bool b_stmt_already_in_cache = SG_FALSE;

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pndx->pvh_schema, 0, &psz_rectype, NULL)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_FIELD__RECTYPE, &psz_rectype)  );
    }

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pndx->pvh_schema, psz_rectype, &pvh_fields)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count_fields)  );

    // check to see if we already have the insert stmt cached
    b_stmt_already_in_cache = SG_FALSE;
    pStmt = NULL;
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pndx->update.prb_stmt_cache, psz_rectype, &b_stmt_already_in_cache, (void**) &pStmt)  );

    if (!pStmt)
    {
        // build the stmt

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_sql)  );
        // we use OR IGNORE here because the record tables have hidrec as
        // the primary key, and it should be safe, because if the hidrec
        // matches, then every other field should match exactly as well.
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "INSERT OR IGNORE INTO '%s_%s' (hidrecrow",
                    SG_DBNDX_RECORD_TABLE_PREFIX,
                    psz_rectype
                    )  );

        for (i=0; i<count_fields; i++)
        {
            const char* psz_field_name = NULL;
            SG_vhash* pvh_field = NULL;
            SG_vhash* pvh_orderings = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i, &psz_field_name, &pvh_field)  );

            SG_ERR_CHECK(  SG_string__append__format(
                        pCtx, 
                        pstr_sql, 
                        ", \"%s\"",
                        psz_field_name
                        )  );
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__ORDERINGS, &pvh_orderings)  );
            if (pvh_orderings)
            {
            SG_uint32 count_orderings = 0;
            SG_uint32 i_ordering = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_orderings, &count_orderings)  );
            for (i_ordering=0; i_ordering<count_orderings; i_ordering++)
            {
                const char* psz_key = NULL;
                
                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_orderings, i_ordering, &psz_key, NULL)  );

                count_extra_ndx_columns++;
                SG_ERR_CHECK(  SG_string__append__format(
                            pCtx, 
                            pstr_sql, 
                            ", \"ordering_%s$_%s\"",
				psz_key,
                            psz_field_name
                            )  );
		}
            }
        }

        SG_ERR_CHECK(  SG_string__append__sz(
                    pCtx, 
                    pstr_sql, 
                    ") VALUES (?"
                    )  );

        for (i=0; i<count_fields + count_extra_ndx_columns; i++)
        {
            SG_ERR_CHECK(  SG_string__append__sz(
                        pCtx, 
                        pstr_sql, 
                        ", ?"
                        )  );
        }

        SG_ERR_CHECK(  SG_string__append__sz(
                    pCtx, 
                    pstr_sql, 
                    ")"
                    )  );

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr_sql))  );
        
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pndx->update.prb_stmt_cache, psz_rectype, pStmt)  );

        SG_STRING_NULLFREE(pCtx, pstr_sql);
    }

    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, hidrecrow)  );
    
    // Now bind everything into the stmt
    ndx_bind = 2; // bind indexes are 1-based, and the hid was 1
	for (i=0; i<count_fields; i++)
	{
        SG_vhash* pvh_one_field = NULL;
        const char* psz_field_name = NULL;
        const char* psz_field_type = NULL;
        SG_bool b_integerish = SG_FALSE;
        const char* psz_val = NULL;
        SG_vhash* pvh_orderings = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i, &psz_field_name, &pvh_one_field)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__ORDERINGS, &pvh_orderings)  );
        SG_ERR_CHECK(  SG_zing__field_type_is_integerish__sz(pCtx, psz_field_type, &b_integerish)  );

        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh, psz_field_name, &psz_val)  );

        if (psz_val)
        {
            if (b_integerish)
            {
                SG_int64 intvalue = 0;

                SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &intvalue, psz_val)  );
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, ndx_bind++, intvalue)  );
            }
            else
            {
                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, ndx_bind++, psz_val)  );
                if (pvh_orderings)
                {
                    SG_int64 ndx_val = -1;
                    SG_uint32 count_orderings = 0;
                    SG_uint32 i_ordering = 0;

                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_orderings, &count_orderings)  );
                    for (i_ordering=0; i_ordering<count_orderings; i_ordering++)
                    {
                        const char* psz_key = NULL;
                        SG_vhash* pvh_ord = NULL;
                        
                        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_orderings, i_ordering, &psz_key, &pvh_ord)  );

                        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_ord, psz_val, &ndx_val)  );
                        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, ndx_bind++, ndx_val)  );
                    }
                }
            }
        }
        else
        {
            SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt, ndx_bind++)  );
            if (pvh_orderings)
            {
            SG_uint32 count_orderings = 0;
            SG_uint32 i_ordering = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_orderings, &count_orderings)  );
            for (i_ordering=0; i_ordering<count_orderings; i_ordering++)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt, ndx_bind++)  );
	}
            }
        }
	}

    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

fail:
    SG_STRING_NULLFREE(pCtx, pstr_sql);
}

void SG_dbndx_update__do_audits(
	SG_context* pCtx,
	SG_dbndx_update* pdbc,
    SG_varray* pva_specified_changesets
	)
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
	sqlite3_stmt* pStmt_add = NULL;
    SG_varray* pva_audits = NULL;

    if (pva_specified_changesets)
    {
        SG_ERR_CHECK(  SG_repo__list_audits_for_given_changesets(pCtx, pdbc->pRepo, pdbc->iDagNum, pva_specified_changesets, &pva_audits)  );
    }
    else
    {
        // if we were not given specific changesets, then we ask for the audits for ALL
        // changesets.  This happens on an initial clone or a reindex.

        SG_ERR_CHECK(  SG_repo__query_audits(pCtx, pdbc->pRepo, pdbc->iDagNum, -1, -1, NULL, &pva_audits)  );
    }

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pStmt_add, "INSERT INTO db_audits (csidrow, userid, timestamp) VALUES (?, ?, ?)")  );

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_audits, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash*pvh = NULL;
        const char* psz_csid = NULL;
        const char* psz_who = NULL;
        SG_int64 i_when = 0;
        SG_int64 csidrow = -1;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_audits, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "csid", &psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_AUDIT__USERID, &psz_who)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, SG_AUDIT__TIMESTAMP, &i_when)  );

        SG_ERR_CHECK(  SG_ihash__get__int64(pCtx, pdbc->update.pih_csid, psz_csid, &csidrow)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt_add)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt_add)  );

        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt_add, 1, csidrow)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt_add, 2, psz_who)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt_add, 3, i_when)  );

        SG_ERR_CHECK(  sg_sqlite__step__retry(pCtx, pStmt_add, SQLITE_DONE, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt_add)  );

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_audits);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_add)  );
}

void SG_dbndx_update__begin(SG_context* pCtx, SG_dbndx_update* pdbc)
{
    SG_ASSERT(!pdbc->update.b_in_update);

    memset(&pdbc->update, 0, sizeof(pdbc->update));

    pdbc->update.b_in_update = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "BEGIN IMMEDIATE TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_history, "INSERT INTO db_history (hidrecrow, csidrow) VALUES (?, ?)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_rowid_csid, "INSERT OR IGNORE INTO csids (csid, generation) VALUES (?,?)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_rowid_hidrec, "INSERT OR IGNORE INTO hidrecs (hidrec) VALUES (?)")  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_delta_adds, "INSERT INTO db_delta_adds (hidrecrow, csidrow, parentrow) VALUES (?, ?, ?)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_delta_dels, "INSERT INTO db_delta_dels (hidrecrow, csidrow, parentrow) VALUES (?, ?, ?)")  );

    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &pdbc->update.prb_stmt_cache)  );

    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pdbc->update.pih_hidrec)  );
    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pdbc->update.pih_csid)  );

fail:
    ;
}

static void my_finalize_cached_stmts (
        SG_context* pCtx,
        SG_rbtree* prb
       )
{
    SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    const char* psz_rectype = NULL;
    sqlite3_stmt* pStmt = NULL;

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb, &b, &psz_rectype, (void**) &pStmt)  );
    while (b)
    {
        SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_rectype, (void**) &pStmt)  );
    }

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

void SG_dbndx_update__end(SG_context* pCtx, SG_dbndx_update* pdbc)
{
    SG_ASSERT(pdbc->update.b_in_update);

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_rowid_csid)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_rowid_hidrec)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_history)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_delta_adds)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_delta_dels)  );

    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_hidrec);
    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_csid);

    SG_ERR_CHECK(  my_finalize_cached_stmts(pCtx, pdbc->update.prb_stmt_cache)  );
    SG_RBTREE_NULLFREE(pCtx, pdbc->update.prb_stmt_cache);

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS db_delta_adds_idx ON db_delta_adds (csidrow, hidrecrow, parentrow)")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS db_delta_dels_idx ON db_delta_dels (csidrow, hidrecrow, parentrow)")  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS db_history_idx_hidrecrow ON db_history (hidrecrow)")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS db_history_idx_csidrow ON db_history (csidrow)")  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS db_audits_idx_timestamp ON db_audits (timestamp)")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS db_audits_idx_csidrow ON db_audits (csidrow)")  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_FALSE;

    pdbc->update.b_in_update = SG_FALSE;

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "PRAGMA journal_mode=WAL", MY_SLEEP_MS, MY_TIMEOUT_MS)  );

fail:
    ;
}

static void SG_dbndx_update__abort(SG_context* pCtx, SG_dbndx_update* pdbc)
{
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_rowid_csid)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_rowid_hidrec)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_history)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_delta_adds)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_delta_dels)  );

    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_hidrec);
    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_csid);

    SG_ERR_CHECK(  my_finalize_cached_stmts(pCtx, pdbc->update.prb_stmt_cache)  );
    SG_RBTREE_NULLFREE(pCtx, pdbc->update.prb_stmt_cache);

    if (pdbc->update.b_in_update)
    {
        if (pdbc->bInTransaction)
        {
            SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
            pdbc->bInTransaction = SG_FALSE;
        }
    }

    pdbc->update.b_in_update = SG_FALSE;

fail:
    ;
}

void SG_dbndx_update__do(
        SG_context* pCtx, 
        SG_dbndx_update* pdbc, 
        SG_vhash* pvh_specified_changesets
        )
{
    SG_changeset* pcs = NULL;
    SG_vhash* pvh_rec = NULL;
	sqlite3_stmt* pStmt = NULL;
    SG_uint32 i_changeset = 0;
    SG_uint32 count_changesets = 0;
    SG_vhash* pvh_new_freeme = NULL;
    SG_varray* pva_specified_changesets_for_audits = NULL;
	SG_stringarray* psa_all_changesets = NULL;

	SG_NULLARGCHECK_RETURN(pdbc);
	SG_ARGCHECK_RETURN( pdbc->pRepo, pdbc );

    //SG_ASSERT(pdbc->update.b_in_update);

    if (pvh_specified_changesets)
    {
        SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvh_specified_changesets, &pva_specified_changesets_for_audits)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_specified_changesets, &count_changesets)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_repo__fs3__fetch_all_dagnodes(pCtx, pdbc->pRepo, pdbc->iDagNum, &psa_all_changesets)  );
        SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_all_changesets, &count_changesets)  );
        pva_specified_changesets_for_audits = NULL;
    }

    SG_ERR_CHECK(  SG_dbndx_update__begin(pCtx, pdbc)  );

    for (i_changeset=0; i_changeset<count_changesets; i_changeset++)
    {
        const char* psz_csid = NULL;
        SG_vhash* pvh_changes = NULL;
        SG_int32 generation = 0;
        SG_int64 csidrow = -1;

        if (pvh_specified_changesets)
        {
            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_specified_changesets, i_changeset, &psz_csid, NULL)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_all_changesets, i_changeset, &psz_csid)  );
        }

        SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pdbc->pRepo, psz_csid, &pcs)  );
        SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs, &generation)  );
        SG_ERR_CHECK(  SG_changeset__db__get_changes(pCtx, pcs, &pvh_changes)  );

        SG_ERR_CHECK(  sg_dbndx__add_csid(pCtx, pdbc, psz_csid, generation, &csidrow)  );

        if (pvh_changes)
        {
            SG_vhash* pvh_new = NULL;
            SG_uint32 count_parents = 0;

            SG_ERR_CHECK(  sg_dbndx__add_deltas(pCtx, pdbc, csidrow, pvh_changes)  );
        
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
            if (count_parents > 1)
            {
                SG_ERR_CHECK(  SG_dbndx_update__figure_out_new_records(pCtx, pvh_changes, &pvh_new_freeme)  );
                pvh_new = pvh_new_freeme;
            }
            else if (1 == count_parents)
            {
                const char* psz_csid_parent = NULL;
                SG_vhash* pvh_changes_one_parent = NULL;
                SG_vhash* pvh_record_adds = NULL;
                SG_uint32 count_adds = 0;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, 0, &psz_csid_parent, &pvh_changes_one_parent)  );

                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_record_adds)  );
                if (pvh_record_adds)
                {
                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_record_adds, &count_adds)  );
                    if (count_adds)
                    {
                        pvh_new = pvh_record_adds;
                    }
                }
            }

            if (pvh_new)
            {
                SG_uint32 count_new = 0;

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_new, &count_new)  );
                if (count_new)
                {
                    SG_uint32 i;

                    for (i=0; i<count_new; i++)
                    {
                        const char* psz_hid_rec = NULL;
                        SG_int64 hidrecrow = -1;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_new, i, &psz_hid_rec, NULL)  );
                        SG_ERR_CHECK(  SG_ihash__get__int64(pCtx, pdbc->update.pih_hidrec, psz_hid_rec, &hidrecrow)  );

                        SG_ERR_CHECK(  sg_dbndx__add_record_to_history_table(pCtx, pdbc, hidrecrow, csidrow)  );
                    }
                }
            }

            if (pvh_new)
            {
                SG_uint32 count_new = 0;

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_new, &count_new)  );
                if (count_new)
                {
                    SG_uint32 i;

                    for (i=0; i<count_new; i++)
                    {
                        const char* psz_hid_rec = NULL;
                        SG_int64 hidrecrow = -1;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_new, i, &psz_hid_rec, NULL)  );
                        SG_ERR_CHECK(  SG_ihash__get__int64(pCtx, pdbc->update.pih_hidrec, psz_hid_rec, &hidrecrow)  );

                        SG_ERR_CHECK(  SG_repo__fetch_vhash__utf8_fix(pCtx, pdbc->pRepo, psz_hid_rec, &pvh_rec)  );

                        SG_ERR_CHECK(  sg_dbndx__add_record_to_records_table(pCtx, pdbc, hidrecrow, pvh_rec)  );
                        SG_ERR_CHECK(  sg_dbndx__add_record_to_full_text_search(pCtx, pdbc, hidrecrow, pvh_rec)  );

                        SG_VHASH_NULLFREE(pCtx, pvh_rec);
                    }
                }
            }

            SG_VHASH_NULLFREE(pCtx, pvh_new_freeme);
        }

        SG_CHANGESET_NULLFREE(pCtx, pcs);

        pdbc->update.count_so_far++;

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }

    SG_ERR_CHECK(  SG_dbndx_update__do_audits(pCtx, pdbc, pva_specified_changesets_for_audits)  );

    SG_ERR_CHECK(  SG_dbndx_update__end(pCtx, pdbc)  );

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa_all_changesets);
    SG_VARRAY_NULLFREE(pCtx, pva_specified_changesets_for_audits);
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    if (pdbc->update.b_in_update)
    {
        SG_ERR_IGNORE(  SG_dbndx_update__abort(pCtx, pdbc)  );
    }

    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    }

    SG_VHASH_NULLFREE(pCtx, pvh_rec);
    SG_VHASH_NULLFREE(pCtx, pvh_new_freeme);
}

void sg_dbndx_update__one_record(
    SG_context* pCtx, 
    SG_dbndx_update* pndx, 
    const char* psz_hid_rec,
    SG_vhash* pvh
    )
{
    SG_int64 hidrecrow = -1;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pvh);

    SG_ERR_CHECK(  SG_ihash__get__int64(pCtx, pndx->update.pih_hidrec, psz_hid_rec, &hidrecrow)  );

    SG_ERR_CHECK(  sg_dbndx__add_record_to_records_table(pCtx, pndx, hidrecrow, pvh)  );
    SG_ERR_CHECK(  sg_dbndx__add_record_to_full_text_search(pCtx, pndx, hidrecrow, pvh)  );

fail:
    ;
}

