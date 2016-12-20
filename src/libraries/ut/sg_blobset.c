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

#if defined(SG_IOS)
#define SG_BLOBSET_MAX_IN_MEMORY (1 * 1000)
#else
#define SG_BLOBSET_MAX_IN_MEMORY (100 * 1000)
#endif

struct _sg_blobset
{
    SG_uint32 count;
    SG_bool b_memory;
    sqlite3* psql;
    sqlite3_stmt* pStmt_list;
    sqlite3_stmt* pStmt_insert;
    sqlite3_stmt* pStmt_lookup;
    SG_pathname* pPath;
};

static void sg_blobset__move_to_temporary_db(
	SG_context * pCtx,
    SG_blobset* pbs
    );

void SG_blobset__insert(
	SG_context * pCtx,
    SG_blobset* pbs,
    const char* psz_hid,
    const char* psz_filename,
    SG_uint64 offset,
    SG_blob_encoding encoding,
    SG_uint64 len_encoded,
    SG_uint64 len_full,
    const char* psz_hid_vcdiff
    )
{
    if (
            pbs->b_memory
            && ((pbs->count + 1) > SG_BLOBSET_MAX_IN_MEMORY)
       )
    {
        SG_ERR_CHECK(  sg_blobset__move_to_temporary_db(pCtx, pbs)  );
    }

    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pbs->pStmt_insert)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pbs->pStmt_insert)  );

    if (SG_BLOBENCODING__VCDIFF == encoding)
    {
        SG_ASSERT(psz_hid_vcdiff);
    }

    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_insert,1,psz_hid)  );
    if (psz_filename)
    {
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_insert,2,psz_filename)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pbs->pStmt_insert,2)  );
    }
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbs->pStmt_insert,3,offset)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pbs->pStmt_insert,4,encoding)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbs->pStmt_insert,5,len_encoded)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbs->pStmt_insert,6,len_full)  );
    if (psz_hid_vcdiff)
    {
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_insert,7,psz_hid_vcdiff)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pbs->pStmt_insert,7)  );
    }
    sg_sqlite__step(pCtx,pbs->pStmt_insert,SQLITE_DONE);
    pbs->count++;

fail:
    ;
}

void SG_blobset__create_blobs_table(
        SG_context* pCtx, 
        sqlite3* psql
        )
{
	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, psql, "CREATE TABLE \"blobs\""
                              "   ("
                              "     \"hid\"                VARCHAR        NOT NULL,"
                              "     \"filename\"           VARCHAR            NULL,"
                              "     \"offset\"             INTEGER            NULL,"
                              "     \"encoding\"           INTEGER            NULL,"
                              "     \"len_encoded\"        INTEGER            NULL,"
                              "     \"len_full\"           INTEGER            NULL,"
                              "     \"hid_vcdiff\"         VARCHAR            NULL"
                              "   )")  );
	SG_ERR_CHECK_RETURN(  sg_sqlite__exec(pCtx, psql, "CREATE UNIQUE INDEX idx_blobs ON blobs (hid)")  );
}

void SG_blobset__create(
	SG_context * pCtx,
    SG_blobset** pp
    )
{
    SG_blobset* pbs = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pbs)  );

    pbs->b_memory = SG_TRUE;
	SG_ERR_CHECK(  sg_sqlite__open__memory(pCtx, &pbs->psql)  );
    SG_ERR_CHECK(  SG_blobset__create_blobs_table(pCtx, pbs->psql)  );

    *pp = pbs;
    pbs = NULL;

fail:
    // TODO free it
    ;
}

void SG_blobset__begin_inserting(
	SG_context * pCtx,
    SG_blobset* pbs
    )
{
    if (pbs->psql)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql,&pbs->pStmt_insert,
                                          "INSERT OR IGNORE INTO \"blobs\" (\"hid\", \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\") VALUES (?, ?, ?, ?, ?, ?, ?)")  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    }

fail:
    ;
}

void SG_blobset__finish_inserting(
	SG_context * pCtx,
    SG_blobset* pbs
    )
{
    if (pbs->psql)
    {
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_insert)  );
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    }

fail:
    ;
}

void SG_blobset__free(
	SG_context * pCtx,
    SG_blobset* pbs
    )
{
    if (!pbs)
    {
        return;
    }

    if (pbs->psql)
    {
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_list)  );
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_lookup)  );
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_insert)  );
        SG_ERR_CHECK(  sg_sqlite__close(pCtx, pbs->psql)  );
    }
    if (pbs->pPath)
    {
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pbs->pPath)  );
        SG_PATHNAME_NULLFREE(pCtx, pbs->pPath);
    }
    SG_NULLFREE(pCtx, pbs);

fail:
    ;
}

static void SG_blobset__copy_into_sqlite(
	SG_context * pCtx,
    SG_blobset* pbs,
    sqlite3* psql_to
    )
{
    int rc = 0;
    sqlite3_stmt* pStmt_from = NULL;
    sqlite3_stmt* pStmt_to = NULL;
    SG_uint32 count_columns = 0;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt_from,
									  "SELECT \"hid\", \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\" FROM \"blobs\"")  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql_to, &pStmt_to,
                                      "INSERT OR IGNORE INTO \"blobs\" (\"hid\", \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\") VALUES (?, ?, ?, ?, ?, ?, ?)")  );

    while ((rc=sqlite3_step(pStmt_from)) == SQLITE_ROW)
    {
        SG_uint32 i= 0;

        count_columns = sqlite3_column_count(pStmt_from);

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt_to)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt_to)  );

        for (i=0; i<count_columns; i++)
        {
            int t = sqlite3_column_type(pStmt_from, i);

            if (SQLITE_INTEGER == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt_to, i+1, sqlite3_column_int64(pStmt_from, i))  );
            }
            else if (SQLITE_FLOAT == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_double(pCtx, pStmt_to, i+1, sqlite3_column_double(pStmt_from, i))  );
            }
            else if (SQLITE_TEXT == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt_to, i+1, (char*) sqlite3_column_text(pStmt_from, i))  );
            }
            else if (SQLITE_BLOB == t)
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            else if (SQLITE_NULL == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt_to, i+1)  );
            }
        }

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt_to, SQLITE_DONE)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_from)  );
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_to)  );
}

static void sg_blobset__move_to_temporary_db(
	SG_context * pCtx,
    SG_blobset* pbs
    )
{
    sqlite3* psql_new = NULL;
    SG_bool b_inserting = SG_FALSE;

    if (pbs->pStmt_insert)
    {
        SG_ERR_CHECK(  SG_blobset__finish_inserting(pCtx, pbs)  );
        b_inserting = SG_TRUE;
    }

    if (pbs->pStmt_lookup)
    {
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_lookup)  );
    }

	SG_ERR_CHECK(  sg_sqlite__open__temporary(pCtx, &psql_new)  );
    SG_ERR_CHECK(  SG_blobset__create_blobs_table(pCtx, psql_new)  );
    SG_ERR_CHECK(  SG_blobset__copy_into_sqlite(pCtx, pbs, psql_new)  );
    SG_ERR_CHECK(  sg_sqlite__close(pCtx, pbs->psql)  );
    pbs->psql = psql_new;
    pbs->b_memory = SG_FALSE;
    psql_new = NULL;

    if (b_inserting)
    {
        SG_ERR_CHECK(  SG_blobset__begin_inserting(pCtx, pbs)  );
    }

fail:
    if (psql_new)
    {
        SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql_new)  );
    }
}

void SG_blobset__copy_into_fs3(
	SG_context * pCtx,
    SG_blobset* pbs,
    sqlite3* psql_to
    )
{
    int rc = 0;
    sqlite3_stmt* pStmt_from = NULL;
    sqlite3_stmt* pStmt_to = NULL;
    SG_uint32 count_columns = 0;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt_from,
									  "SELECT \"hid\", \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\" FROM \"blobs\"")  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql_to, &pStmt_to,
                                      "INSERT OR IGNORE INTO \"blobs\" (\"hid\", \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\") VALUES (?, ?, ?, ?, ?, ?, ?)")  );

    while ((rc=sqlite3_step(pStmt_from)) == SQLITE_ROW)
    {
        SG_uint32 i= 0;

        count_columns = sqlite3_column_count(pStmt_from);

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt_to)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt_to)  );

        for (i=0; i<count_columns; i++)
        {
            int t = sqlite3_column_type(pStmt_from, i);

            if (SQLITE_INTEGER == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt_to, i+1, sqlite3_column_int64(pStmt_from, i))  );
            }
            else if (SQLITE_FLOAT == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_double(pCtx, pStmt_to, i+1, sqlite3_column_double(pStmt_from, i))  );
            }
            else if (SQLITE_TEXT == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt_to, i+1, (char*) sqlite3_column_text(pStmt_from, i))  );
            }
            else if (SQLITE_BLOB == t)
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            else if (SQLITE_NULL == t)
            {
                SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pStmt_to, i+1)  );
            }
        }

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt_to, SQLITE_DONE)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_from)  );
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_to)  );
}

void SG_blobset__get(
        SG_context * pCtx,
        SG_blobset* pbs,
        SG_uint32 limit,
        SG_uint32 skip,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_result = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_result)  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt,
									  "SELECT \"hid\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\"  FROM \"blobs\" ORDER BY \"hid\" ASC LIMIT %d OFFSET %d", limit ? (SG_int32) limit : -1, skip)  );
    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        SG_vhash* pvh = NULL;

        const char* psz_hid = (const char *)sqlite3_column_text(pStmt,0);
        SG_blob_encoding encoding = (SG_blob_encoding) sqlite3_column_int(pStmt, 1);
        SG_uint64 len_encoded = sqlite3_column_int64(pStmt, 2);
        SG_uint64 len_full = sqlite3_column_int64(pStmt, 3);
        const char* psz_hid_vcdiff = (const char *)sqlite3_column_text(pStmt,4);

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_result, psz_hid, &pvh)  );

        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "encoding", encoding)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "len_encoded", len_encoded)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "len_full", len_full)  );

        if (psz_hid_vcdiff)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "hid_vcdiff", psz_hid_vcdiff)  );
        }
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_blobset__lookup__vhash(
        SG_context * pCtx,
        SG_blobset* pbs,
        const char* psz_hid,
        SG_vhash** ppvh
        )
{
    SG_bool b_found = SG_FALSE;
    char* psz_filename = NULL;
    SG_uint64 offset = 0;
    SG_blob_encoding encoding = 0;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    char* psz_hid_vcdiff = NULL;
    SG_vhash* pvh = NULL;

    SG_ERR_CHECK(  SG_blobset__lookup(
                pCtx,
                pbs,
                psz_hid,
                &b_found,
                &psz_filename,
                &offset,
                &encoding,
                &len_encoded,
                &len_full,
                &psz_hid_vcdiff
                )  );

    if (b_found)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "hid", psz_hid)  );
        if (psz_filename)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "filename", psz_filename)  );
        }
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "offset", offset)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "encoding", encoding)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "len_encoded", len_encoded)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "len_full", len_full)  );

        if (psz_hid_vcdiff)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "hid_vcdiff", psz_hid_vcdiff)  );
        }
    }

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_NULLFREE(pCtx, psz_filename);
    SG_NULLFREE(pCtx, psz_hid_vcdiff);
}

void SG_blobset__get_stats(
        SG_context * pCtx,
        SG_blobset* pbs,
        SG_blob_encoding encoding,
        SG_uint32* p_count,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        SG_uint64* p_max_len_encoded,
        SG_uint64* p_max_len_full
        )
{
	sqlite3_stmt * pStmt = NULL;
    SG_uint32 count = 0;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    SG_uint64 max_len_encoded = 0;
    SG_uint64 max_len_full = 0;

    if (encoding)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt,
                                          "SELECT COUNT(\"hid\"), SUM(\"len_encoded\"), SUM(\"len_full\"), MAX(\"len_encoded\"), MAX(\"len_full\") FROM \"blobs\" WHERE \"encoding\" = ?")  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt,1,encoding)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt,
                                          "SELECT COUNT(\"hid\"), SUM(\"len_encoded\"), SUM(\"len_full\"), MAX(\"len_encoded\"), MAX(\"len_full\") FROM \"blobs\"")  );
    }
    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_ROW)  );

    count = sqlite3_column_int(pStmt, 0);
    len_encoded = sqlite3_column_int64(pStmt, 1);
    len_full = sqlite3_column_int64(pStmt, 2);
    max_len_encoded = sqlite3_column_int64(pStmt, 3);
    max_len_full = sqlite3_column_int64(pStmt, 4);
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    if (p_count)
    {
        *p_count = count;
    }
    if (p_len_encoded)
    {
        *p_len_encoded = len_encoded;
    }
    if (p_len_full)
    {
        *p_len_full = len_full;
    }
    if (p_max_len_encoded)
    {
        *p_max_len_encoded = max_len_encoded;
    }
    if (p_max_len_full)
    {
        *p_max_len_full = max_len_full;
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_blobset__lookup(
        SG_context * pCtx,
        SG_blobset* pbs,
        const char* psz_hid,
        SG_bool* pb_found,
        char** ppsz_filename,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        char** ppsz_hid_vcdiff_reference
        )
{
    int rc;

    const char* psz_filename = NULL;
    SG_uint64 offset = 0;
    SG_blob_encoding blob_encoding = 0;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    const char * psz_hid_vcdiff_reference = NULL;

    if (!pbs->pStmt_lookup)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pbs->pStmt_lookup,
                                          "SELECT \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\"  FROM \"blobs\" WHERE \"hid\" = ?")  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pbs->pStmt_lookup)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pbs->pStmt_lookup)  );
    }
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_lookup,1,psz_hid)  );

    rc = sqlite3_step(pbs->pStmt_lookup);
    if (SQLITE_ROW == rc)
    {
        psz_filename = (const char *)sqlite3_column_text(pbs->pStmt_lookup,0);
        offset = sqlite3_column_int64(pbs->pStmt_lookup, 1);

        blob_encoding = (SG_blob_encoding) sqlite3_column_int(pbs->pStmt_lookup, 2);

        len_encoded = sqlite3_column_int64(pbs->pStmt_lookup, 3);
        len_full = sqlite3_column_int64(pbs->pStmt_lookup, 4);

        if (SG_IS_BLOBENCODING_FULL(blob_encoding))
        {
            SG_ASSERT(len_encoded == len_full);
        }

        psz_hid_vcdiff_reference = (const char *)sqlite3_column_text(pbs->pStmt_lookup,5);

        if (ppsz_hid_vcdiff_reference)
        {
            if (psz_hid_vcdiff_reference)
            {
                SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_vcdiff_reference, ppsz_hid_vcdiff_reference)  );
            }
            else
            {
                *ppsz_hid_vcdiff_reference = NULL;
            }
        }

        if (p_blob_encoding)
        {
            *p_blob_encoding = blob_encoding;
        }
        if (p_len_encoded)
        {
            *p_len_encoded = len_encoded;
        }
        if (p_len_full)
        {
            *p_len_full = len_full;
        }
        if (ppsz_filename)
        {
            if (psz_filename)
            {
                SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_filename, ppsz_filename)  );
            }
            else
            {
                *ppsz_filename = NULL;
            }
        }
        if (p_offset)
        {
            *p_offset = offset;
        }

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pbs->pStmt_lookup, SQLITE_DONE)  );

        *pb_found = SG_TRUE;
    }
    else if (SQLITE_DONE == rc)
    {
        *pb_found = SG_FALSE;
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

fail:
    ;
}

void SG_blobset__inserting(
	SG_context* pCtx,
    SG_blobset* pbs,
    SG_bool* pb
    )
{
	SG_NULLARGCHECK_RETURN(pbs);

    if (pbs->pStmt_insert)
    {
        *pb = SG_TRUE;
    }
    else
    {
        *pb = SG_FALSE;
    }
}

void SG_blobset__count(
	SG_context* pCtx,
    SG_blobset* pbs,
    SG_uint32* pi_count
    )
{
	SG_NULLARGCHECK_RETURN(pbs);

    *pi_count = pbs->count;
}

void SG_blobset__compare_to(
	SG_context* pCtx,
    SG_blobset* pbs_me,
    SG_blobset* pbs_other,
    SG_bool* pb
    )
{
    sqlite3_stmt* pStmt_me = NULL;
    sqlite3_stmt* pStmt_other = NULL;
    int rc_me = 0;
    int rc_other = 0;

	SG_NULLARGCHECK_RETURN(pbs_me);
	SG_NULLARGCHECK_RETURN(pbs_other);
	SG_NULLARGCHECK_RETURN(pb);

    if (pbs_me->count != pbs_other->count)
    {
        *pb = SG_FALSE;
        goto done;
    }

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs_me->psql, &pStmt_me,
									  "SELECT \"hid\", \"len_full\" FROM \"blobs\" ORDER BY \"hid\" ASC")  );
	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs_other->psql, &pStmt_other,
									  "SELECT \"hid\", \"len_full\" FROM \"blobs\" ORDER BY \"hid\" ASC")  );

    while (1)
    {
        rc_me=sqlite3_step(pStmt_me);
        rc_other=sqlite3_step(pStmt_other);

        if (
                (SQLITE_ROW == rc_me)
                && (SQLITE_ROW == rc_other)
           )
        {
            const char* psz_hid_me = (const char *)sqlite3_column_text(pStmt_me,0);
            SG_uint64 len_full_me = sqlite3_column_int64(pStmt_me, 1);

            const char* psz_hid_other = (const char *)sqlite3_column_text(pStmt_other,0);
            SG_uint64 len_full_other = sqlite3_column_int64(pStmt_other, 1);

            if (len_full_me != len_full_other)
            {
                *pb = SG_FALSE;
                goto done;
            }

            if (0 != strcmp(psz_hid_me, psz_hid_other))
            {
                *pb = SG_FALSE;
                goto done;
            }
        }
        else
        {
            break;
        }
    }

    if (
            (SQLITE_ROW == rc_me)
            || (SQLITE_ROW == rc_other)
       )
    {
        *pb = SG_FALSE;
        goto done;
    }

    if (rc_me != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc_me)  );
    }
    if (rc_other != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc_other)  );
    }

    *pb = SG_TRUE;

done:
fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_me)  );
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_other)  );
}

