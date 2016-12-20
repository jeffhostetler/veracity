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

#define MY_SLEEP_MS      100
#define MY_TIMEOUT_MS  30000

static void sg_dbndx__collect_all_rectypes(
	SG_context* pCtx,
    SG_vector* pvec_templates,
    SG_vhash** ppvh
    )
{
    SG_uint32 count_templates = 0;
    SG_vhash* pvh = NULL;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vector__length(pCtx, pvec_templates, &count_templates)  );
    SG_ARGCHECK(count_templates > 0, pvec_templates);

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    for (i=0; i<count_templates; i++)
    {
        SG_zingtemplate* pzt = NULL;

        SG_ERR_CHECK(  SG_vector__get(pCtx, pvec_templates, i, (void**) &pzt)  );

        SG_ERR_CHECK(  SG_zingtemplate__list_rectypes__update_into_vhash(pCtx, pzt, pvh)  );
    }

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_dbndx__create_composite_schema_for_all_templates(
	SG_context* pCtx,
    SG_uint64 dagnum,
    SG_vector* pvec_templates,
    SG_vhash** ppvh
    )
{
    SG_vhash* pvh_rectypes = NULL;
    SG_vhash* pvh_result = NULL;
    SG_uint32 count_rectypes = 0;
    SG_uint32 i = 0;
    SG_uint32 count_templates = 0;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );

    SG_ERR_CHECK(  sg_dbndx__collect_all_rectypes(pCtx, pvec_templates, &pvh_rectypes)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rectypes, &count_rectypes)  );
    SG_ERR_CHECK(  SG_vector__length(pCtx, pvec_templates, &count_templates)  );
    for (i=0; i<count_rectypes; i++)
    {
        const char* psz_rectype = NULL;
        SG_uint32 i_template = 0;
        SG_vhash* pvh_one_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_rectypes, i, &psz_rectype, NULL)  );

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_result, psz_rectype, &pvh_one_rectype)  );

        for (i_template=0; i_template<count_templates; i_template++)
        {
            SG_zingtemplate* pzt = NULL;

            SG_ERR_CHECK(  SG_vector__get(pCtx, pvec_templates, i_template, (void**) &pzt)  );

            SG_ERR_CHECK(  SG_zing__collect_fields_one_rectype_one_template(pCtx, psz_rectype, pzt, pvh_one_rectype)  );
        }

        // add recid to the field list
        if (!SG_DAGNUM__HAS_NO_RECID(dagnum))
        {
            SG_vhash* pvh_field_recid = NULL;

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_one_rectype, SG_ZING_FIELD__RECID, &pvh_field_recid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_field_recid, SG_DBNDX_SCHEMA__FIELD__TYPE, "string")  );
        }
    }

	SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_result, SG_TRUE, SG_vhash_sort_callback__increasing)  );

    //SG_VHASH_STDOUT(pvh_result);

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_rectypes);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
}

static void sg_dbndx__make_sql__create_record_table(
	SG_context* pCtx,
    const char* psz_table_prefix,
    const char* psz_rectype,
    SG_vhash* pvh_fields,
    const char* pidState,
    SG_string** ppstr
    )
{
    SG_string* pstr_sql = NULL;
    SG_uint32 count_fields = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_sql)  );

    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "CREATE TABLE \"%s%s_%s\" (hidrecrow INTEGER PRIMARY KEY NOT NULL", psz_table_prefix, pidState?pidState:"", psz_rectype)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count_fields)  );
    for (i=0; i<count_fields; i++)
    {
        SG_vhash* pvh_one_field = NULL;
        const char* psz_field_name = NULL;
        const char* psz_field_type = NULL;
        SG_bool b_integerish = SG_FALSE;
        SG_vhash* pvh_orderings = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i, &psz_field_name, &pvh_one_field)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );
        
        SG_ERR_CHECK(  SG_zing__field_type_is_integerish__sz(pCtx, psz_field_type, &b_integerish)  );

        SG_ERR_CHECK(  SG_string__append__format(
                    pCtx, 
                    pstr_sql, 
                    ", \"%s\" %s NULL",
                    psz_field_name,
                    b_integerish ? "INTEGER" : "VARCHAR"
                    )  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__ORDERINGS, &pvh_orderings)  );
        if (pvh_orderings)
        {
            SG_uint32 count_orderings = 0;
            SG_uint32 i_ordering = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_orderings, &count_orderings)  );
            for (i_ordering=0; i_ordering<count_orderings; i_ordering++)
            {
                const char* psz_key = NULL;
                
                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_orderings, i_ordering, &psz_key, NULL)  );

                SG_ERR_CHECK(  SG_string__append__format(
                            pCtx, 
                            pstr_sql, 
                            ", \"ordering_%s$_%s\" INTEGER NULL",
                            psz_key,
                            psz_field_name
                            )  );
            }

        }
    }

    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_sql, ")")  );

    *ppstr = pstr_sql;
    pstr_sql = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr_sql);
}

static void sg_dbndx__store_schema(
	SG_context* pCtx,
    sqlite3* psql,
    const char* psz_schema_token,
    SG_vhash* pvh_schema
    )
{
    SG_string* pstr = NULL;
    sqlite3_stmt* pStmt = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh_schema, pstr)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT INTO composite_schema (token, json) VALUES (?, ?)")  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_schema_token)  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, SG_string__sz(pstr))  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

fail:
    if (pStmt)
    {
        SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
    SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_dbndx__update_schema_token(
	SG_context* pCtx,
    sqlite3* psql,
    const char* psz_schema_token
	)
{
    sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "UPDATE composite_schema SET token = ?")  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_schema_token)  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

fail:
    if (pStmt)
    {
        SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
}

void SG_dbndx__get_schema_info(
	SG_context* pCtx,
    sqlite3* psql,
    char** ppsz_schema_token,
    char** ppsz_schema
	)
{
    int rc = -1;
    sqlite3_stmt* pStmt_schema = NULL;
    char* psz_schema_token = NULL;
    char* psz_schema = NULL;

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt_schema, "SELECT token, json FROM composite_schema")  );
    if ((rc = sqlite3_step(pStmt_schema)) == SQLITE_ROW)
    {
        const char* psz_token = (const char*) sqlite3_column_text(pStmt_schema,0);
        const char* psz_json = (const char*) sqlite3_column_text(pStmt_schema,1);

        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_token, &psz_schema_token)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_json, &psz_schema)  );
    }
    else if (SQLITE_DONE != rc)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt_schema)  );
    
    *ppsz_schema_token = psz_schema_token;
    psz_schema_token = NULL;

    *ppsz_schema = psz_schema;
    psz_schema = NULL;

fail:
    SG_NULLFREE(pCtx, psz_schema_token);
    SG_NULLFREE(pCtx, psz_schema);
}

void sg_dbndx__create_record_tables_from_schema(
	SG_context* pCtx,
    sqlite3* psql,
    SG_vhash* pvh_schema,
    const char* pidState
    )
{
    SG_uint32 count_rectypes = 0;
    SG_uint32 i = 0;
    SG_string* pstr_sql = NULL;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
    for (i=0; i<count_rectypes; i++)
    {
        SG_vhash* pvh_one_rectype = NULL;
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  sg_dbndx__make_sql__create_record_table(pCtx, SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, pvh_one_rectype, pidState, &pstr_sql)  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );

        SG_STRING_NULLFREE(pCtx, pstr_sql);
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_sql);
}

static void sg_dbndx__create_fts_tables_from_schema(
	SG_context* pCtx,
    sqlite3* psql,
    SG_vhash* pvh_schema
    )
{
    SG_uint32 count_rectypes = 0;
    SG_uint32 i_rectype = 0;
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
    for (i_rectype=0; i_rectype<count_rectypes; i_rectype++)
    {
        SG_vhash* pvh_one_rectype = NULL;
        const char* psz_rectype = NULL;
        SG_uint32 count_fields = 0;
        SG_uint32 i_field = 0;
        SG_uint32 count_fts_fields = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i_rectype, &psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_one_rectype, &count_fields)  );
        for (i_field=0; i_field<count_fields; i_field++)
        {
            SG_vhash* pvh_one_field = NULL;
            const char* psz_field_name = NULL;
            SG_bool b_has = SG_FALSE;
            SG_bool b_full_text_search = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_one_rectype, i_field, &psz_field_name, &pvh_one_field)  );
            
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
            SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pstr, "CREATE VIRTUAL TABLE \"%s_%s\" USING FTS4 (", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );
            for (i_field=0; i_field<count_fields; i_field++)
            {
                SG_vhash* pvh_one_field = NULL;
                const char* psz_field_name = NULL;
                SG_bool b_has = SG_FALSE;
                SG_bool b_full_text_search = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_one_rectype, i_field, &psz_field_name, &pvh_one_field)  );
                
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_has)  );
                if (b_has)
                {
                    SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_one_field, SG_DBNDX_SCHEMA__FIELD__FULL_TEXT_SEARCH, &b_full_text_search)  );
                }
                if (b_full_text_search)
                {
                    if (count_so_far)
                    {
                        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ", ")  );
                    }
                    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, psz_field_name)  );
                    count_so_far++;
                }
            }
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ")")  );
            SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr))  );
            SG_STRING_NULLFREE(pCtx, pstr);

            SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE TABLE \"%s_map_%s\" (hidrecrow INTEGER PRIMARY KEY NOT NULL, ftsrowid INTEGER NOT NULL UNIQUE)", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__create_db(SG_context* pCtx, sqlite3* psql, SG_uint64 dagnum)
{
    SG_UNUSED(dagnum);

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE hidrecs (hidrecrow INTEGER PRIMARY KEY, hidrec VARCHAR UNIQUE NOT NULL)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE csids (csidrow INTEGER PRIMARY KEY, csid VARCHAR UNIQUE NOT NULL, generation INTEGER NOT NULL)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "INSERT INTO csids (csid, generation) VALUES ('', 0)")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE INDEX IF NOT EXISTS csids_idx_generation ON csids (generation)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE db_audits (csidrow INTEGER NOT NULL, userid VARCHAR NOT NULL, timestamp INTEGER NOT NULL)")  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE db_history (hidrecrow INTEGER NOT NULL, csidrow INTEGER NOT NULL)")  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE composite_schema (token VARCHAR NOT NULL, json VARCHAR NOT NULL)")  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE db_delta_adds (hidrecrow INTEGER NOT NULL, csidrow INTEGER NOT NULL, parentrow INTEGER NOT NULL)")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE db_delta_dels (hidrecrow INTEGER NOT NULL, csidrow INTEGER NOT NULL, parentrow INTEGER NOT NULL)")  );

fail:
    ;
}

void SG_dbndx__create_new(
	SG_context* pCtx,
    SG_uint64 dagnum,
    sqlite3* psql,
    const char* psz_schema_token,
    SG_vhash* pvh_schema
	)
{
    SG_NULLARGCHECK(psql);

    SG_ERR_CHECK(  sg_dbndx__create_db(pCtx, psql, dagnum)  );
    SG_ERR_CHECK(  sg_dbndx__create_record_tables_from_schema(pCtx, psql, pvh_schema, NULL)  );
    SG_ERR_CHECK(  sg_dbndx__create_fts_tables_from_schema(pCtx, psql, pvh_schema)  );
    SG_ERR_CHECK(  sg_dbndx__store_schema(pCtx, psql, psz_schema_token, pvh_schema)  );

fail:
    ;
}

