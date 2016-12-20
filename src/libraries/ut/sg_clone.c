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
#include <zlib.h>

#define MY_CHUNK_SIZE			(16*1024)

#define BUF_LEN_TABLE_NAME (100 + SG_DAGNUM__BUF_MAX__HEX)
#define DAG_EDGES_TABLE_NAME "dag_edges_"
#define DAG_INFO_TABLE_NAME "dag_info_"
#define DAG_LEAVES_TABLE_NAME "dag_leaves_"
#define DAG_AUDITS_TABLE_NAME "dag_audits_"

static void _get_table_name(SG_context* pCtx, const char* pszTablePrefix, SG_uint64 iDagNum, char* bufTableName)
{
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_ERR_CHECK_RETURN(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );
	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx, bufTableName, BUF_LEN_TABLE_NAME, "%s_%s", pszTablePrefix, buf_dagnum)  );
}

typedef struct _sg_blobndx SG_blobndx;

#define SG_BLOBNDX_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_blobndx__free)

void SG_blobndx__mark_blobs(
	SG_context * pCtx,
    SG_blobndx* pbs,
    SG_ihash* pih_blob_stored
    );

void SG_blobndx__do_pending_removes(
    SG_context * pCtx,
    SG_blobndx* pbs
    );

void SG_blobndx__list_unmarked_blobs__begin(
        SG_context * pCtx,
        SG_blobndx* pbs,
        SG_uint32* pi_count
        );

void SG_blobndx__list_unmarked_blobs__next(
        SG_context * pCtx,
        SG_blobndx* pbs,
        const char** ppsz_hid,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        const char** ppsz_hid_vcdiff_reference
        );

void SG_blobndx__list_unmarked_blobs__finish(
        SG_context * pCtx,
        SG_blobndx* pbs
        );

void SG_blobndx__insert(
	SG_context * pCtx,
    SG_blobndx* pbc,
    const char* psz_hid,
    SG_uint64 offset,
    SG_blob_encoding encoding,
    SG_uint64 len_encoded,
    SG_uint64 len_full,
    const char* psz_hid_vcdiff
    );

void SG_blobndx__create__with_sqlite_file(
	SG_context * pCtx,
    SG_pathname** ppPath_bindex,
    SG_blobndx** pp
    );

void SG_blobndx__create(
	SG_context * pCtx,
    SG_blobndx** pp
    );

void SG_blobndx__begin_inserting(
	SG_context * pCtx,
    SG_blobndx* pbc
    );

void SG_blobndx__finish_inserting(
	SG_context * pCtx,
    SG_blobndx* pbc
    );

void SG_blobndx__begin_updating_objectids(
	SG_context * pCtx,
    SG_blobndx* pbc
    );

void SG_blobndx__finish_updating_objectids(
	SG_context * pCtx,
    SG_blobndx* pbc
    );

void SG_blobndx__free(
	SG_context * pCtx,
    SG_blobndx* pbc
    );

void SG_blobndx__lookup(
        SG_context * pCtx,
        SG_blobndx* pbc,
        const char * szHidBlob,
        SG_bool* pb_found,
        SG_int64* pi_rowid,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        char** ppsz_hid_vcdiff_reference
        );

void SG_blobndx__list_blobs_for_objectid(
	SG_context* pCtx,
    SG_blobndx* pbs,
	const char* psz_objectid,
	SG_vhash** ppvh
    );

void SG_blobndx__list_pack_candidates(
	SG_context* pCtx,
    SG_blobndx* pbs,
	SG_uint32 nMinRevisionCount,
	SG_varray ** ppvArrayResults
    );

struct _sg_blobndx
{
    sqlite3* psql;
    sqlite3_stmt* pStmt_list;
    sqlite3_stmt* pStmt_insert;
    sqlite3_stmt* pStmt_update_objectid;
    sqlite3_stmt* pStmt_lookup;
    SG_pathname* pPath;
};

void SG_blobndx__update_objectid(
	SG_context * pCtx,
    SG_blobndx* pbs,
    const char* psz_hid,
    const char* psz_objectid
    )
{
    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pbs->pStmt_update_objectid)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pbs->pStmt_update_objectid)  );

    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_update_objectid,1,psz_hid)  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_update_objectid,2,psz_objectid)  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx,pbs->pStmt_update_objectid,SQLITE_DONE)  );

fail:
    ;
}

void SG_blobndx__mark_blobs(
	SG_context * pCtx,
    SG_blobndx* pbs,
    SG_ihash* pih_blob_stored
    )
{
    sqlite3_stmt* pStmt = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_bool b_tx = SG_FALSE;

    SG_ERR_CHECK(  SG_ihash__count(pCtx, pih_blob_stored, &count)  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE TABLE IF NOT EXISTS mark (hidrow INTEGER NOT NULL)")  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    b_tx = SG_TRUE;
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt, "INSERT INTO mark (hidrow) VALUES (?)")  );
    for (i=0; i<count; i++)
    {
        SG_int64 rowid = 0;
        const char* psz_hid = NULL;

        SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pih_blob_stored, i, &psz_hid, &rowid)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, rowid)  );

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    b_tx = SG_FALSE;

fail:
    if (b_tx)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pbs->psql, "ROLLBACK TRANSACTION")  );
    }
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_blobndx__insert(
	SG_context * pCtx,
    SG_blobndx* pbs,
    const char* psz_hid,
    SG_uint64 offset,
    SG_blob_encoding encoding,
    SG_uint64 len_encoded,
    SG_uint64 len_full,
    const char* psz_hid_vcdiff
    )
{
    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pbs->pStmt_insert)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pbs->pStmt_insert)  );

    if (SG_BLOBENCODING__VCDIFF == encoding)
    {
        SG_ASSERT(psz_hid_vcdiff);
    }

    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_insert,1,psz_hid)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbs->pStmt_insert,2,offset)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pbs->pStmt_insert,3,encoding)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbs->pStmt_insert,4,len_encoded)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbs->pStmt_insert,5,len_full)  );
    if (psz_hid_vcdiff)
    {
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbs->pStmt_insert,6,psz_hid_vcdiff)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pbs->pStmt_insert,6)  );
    }
    sg_sqlite__step(pCtx,pbs->pStmt_insert,SQLITE_DONE);

fail:
    ;
}

static void SG_blobndx__create_blobs_table(
        SG_context* pCtx, 
        sqlite3* psql
        )
{
	SG_ERR_CHECK_RETURN(  sg_sqlite__exec__va(pCtx, psql, "CREATE TABLE \"blobs\""
                              "   ("
                              "     \"hid\"                VARCHAR UNIQUE NOT NULL,"
                              "     \"offset\"             INTEGER            NULL,"
                              "     \"encoding\"           INTEGER            NULL,"
                              "     \"len_encoded\"        INTEGER            NULL,"
                              "     \"len_full\"           INTEGER            NULL,"
                              "     \"hid_vcdiff\"         VARCHAR            NULL"
                              "   )")  );
}

void SG_blobndx__create(
	SG_context * pCtx,
    SG_blobndx** pp
    )
{
    SG_blobndx* pbs = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pbs)  );

	SG_ERR_CHECK(  sg_sqlite__open__temporary(pCtx, &pbs->psql)  );
    SG_ERR_CHECK(  SG_blobndx__create_blobs_table(pCtx, pbs->psql)  );

    *pp = pbs;
    pbs = NULL;

fail:
    // TODO free it
    ;
}

void SG_blobndx__list_changesets(
	SG_context * pCtx,
    SG_blobndx* pbs,
    SG_uint64 dagnum,
    SG_varray** ppva
    )
{
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_varray* pva = NULL;
    char bufTableName[BUF_LEN_TABLE_NAME];
    SG_bool b_tx = SG_FALSE;

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    b_tx = SG_TRUE;
    SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, dagnum, bufTableName)  );
    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt, "SELECT child_id FROM %s ORDER BY generation ASC", bufTableName)  );
	while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
        const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, psz_csid)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    b_tx = SG_FALSE;

    *ppva = pva;
    pva = NULL;

fail:
    if (b_tx)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pbs->psql, "ROLLBACK TRANSACTION")  );
    }
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void SG_blobndx__list_templates(
	SG_context * pCtx,
    SG_blobndx* pbs,
    SG_uint64 dagnum,
    SG_varray** ppva
    )
{
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
    SG_varray* pva_templates = NULL;
    SG_bool b_tx = SG_FALSE;

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    b_tx = SG_TRUE;
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt, "SELECT hid FROM templates WHERE dagnum='%s'", buf_dagnum)  );
	while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
        const char* psz_hid = (const char*) sqlite3_column_text(pStmt, 0);
        if (!pva_templates)
        {
            SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva_templates)  );
        }
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_templates, psz_hid)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    b_tx = SG_FALSE;

    *ppva = pva_templates;
    pva_templates = NULL;

fail:
    if (b_tx)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pbs->psql, "ROLLBACK TRANSACTION")  );
    }
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
}

void SG_blobndx__list_dags(
	SG_context * pCtx,
    SG_blobndx* pbs,
    SG_vhash** ppvh_dags
    )
{
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_bool b_tx = SG_FALSE;
    SG_vhash* pvh_dags = NULL;
    SG_uint32 count_dags = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_dags)  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    b_tx = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt, "SELECT dagnum FROM dags")  );
	while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
        const char* psz_dagnum = (const char*) sqlite3_column_text(pStmt, 0);
        SG_vhash* pvh_daginfo = NULL;

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_dags, psz_dagnum, &pvh_daginfo)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    b_tx = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dags, &count_dags)  );
    for (i=0; i<count_dags; i++)
    {
        const char* psz_dagnum = (const char*) sqlite3_column_text(pStmt, 0);
        SG_vhash* pvh_daginfo = NULL;
        SG_uint64 dagnum = 0;
        char bufTableName[BUF_LEN_TABLE_NAME];
        SG_int32 count_changesets = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_dags, i, &psz_dagnum, &pvh_daginfo)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

        SG_ERR_CHECK(  _get_table_name(pCtx, DAG_INFO_TABLE_NAME, dagnum, bufTableName)  );
        SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pbs->psql, &count_changesets,
            "SELECT COUNT(*) FROM %s",
            bufTableName
            )  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_daginfo, "count_changesets", (SG_int64) count_changesets)  );
    }

    *ppvh_dags = pvh_dags;
    pvh_dags = NULL;

fail:
    if (b_tx)
    {
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "ROLLBACK TRANSACTION")  );
    }
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_VHASH_NULLFREE(pCtx, pvh_dags);
}

void SG_blobndx__create__with_sqlite_file(
	SG_context * pCtx,
    SG_pathname** ppPath_bindex,
    SG_blobndx** pp
    )
{
    SG_blobndx* pbs = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pbs)  );
    pbs->pPath = *ppPath_bindex;
    *ppPath_bindex = NULL;

	SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pbs->pPath, SG_SQLITE__SYNC__OFF, &pbs->psql)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE UNIQUE INDEX blobs_hid ON blobs (hid)")  );

    *pp = pbs;
    pbs = NULL;

fail:
    SG_BLOBNDX_NULLFREE(pCtx, pbs);
}

void SG_blobndx__begin_inserting(
	SG_context * pCtx,
    SG_blobndx* pbs
    )
{
    if (pbs->psql)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql,&pbs->pStmt_insert,
                                          "INSERT OR IGNORE INTO \"blobs\" (\"hid\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\") VALUES (?, ?, ?, ?, ?, ?)")  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    }

fail:
    ;
}

void SG_blobndx__finish_inserting(
	SG_context * pCtx,
    SG_blobndx* pbs
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

void SG_blobndx__begin_updating_objectids(
	SG_context * pCtx,
    SG_blobndx* pbs
    )
{
    if (pbs->psql)
    {
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE TABLE IF NOT EXISTS pack (hid VARCHAR NOT NULL UNIQUE, objectid VARCHAR NOT NULL)")  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pbs->pStmt_update_objectid,
                                          "INSERT OR IGNORE INTO \"pack\" (hid, objectid) VALUES (?,?)")  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    }

fail:
    ;
}

void SG_blobndx__finish_updating_objectids(
	SG_context * pCtx,
    SG_blobndx* pbs
    )
{
    if (pbs->psql)
    {
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_update_objectid)  );
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    }

fail:
    ;
}

void SG_blobndx__free(
	SG_context * pCtx,
    SG_blobndx* pbs
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
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_update_objectid)  );
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

void SG_blobndx__list_unmarked_blobs__begin(
        SG_context * pCtx,
        SG_blobndx* pbs,
        SG_uint32* pi_count
        )
{
    SG_int32 count = 0;
	SG_bool bMarkedBlobsExist = SG_FALSE;

	SG_ERR_CHECK(  SG_sqlite__table_exists(pCtx, pbs->psql, "mark", &bMarkedBlobsExist)  );

	if (bMarkedBlobsExist)
	{
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pbs->pStmt_list,
			"SELECT \"hid\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\"  FROM \"blobs\" WHERE blobs.rowid NOT IN (SELECT hidrow FROM mark)")  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN TRANSACTION")  );
		SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pbs->psql, &count,
			"SELECT COUNT(rowid) FROM blobs WHERE rowid NOT IN (SELECT hidrow FROM mark)"
			)  );
	}
	else
	{
		/* Some tests that create weird repos won't have a marked blobs table. */
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pbs->pStmt_list,
			"SELECT \"hid\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\"  FROM \"blobs\"")  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN TRANSACTION")  );
		SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pbs->psql, &count,
			"SELECT COUNT(rowid) FROM blobs"
			)  );
	}

	*pi_count = (SG_uint32) count;

fail:
    ;
}

void SG_blobndx__list_unmarked_blobs__next(
        SG_context * pCtx,
        SG_blobndx* pbs,
        const char** ppsz_hid,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        const char** ppsz_hid_vcdiff_reference
        )
{
    int rc = sqlite3_step(pbs->pStmt_list);
    if (SQLITE_DONE == rc)
    {
        *ppsz_hid = NULL;
        return;
    }
    if (SQLITE_ROW != rc)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    *ppsz_hid = (const char *)sqlite3_column_text(pbs->pStmt_list,0);

    if (p_offset)
    {
        *p_offset = sqlite3_column_int64(pbs->pStmt_list, 1);
    }

    if (p_blob_encoding)
    {
        *p_blob_encoding = (SG_blob_encoding) sqlite3_column_int(pbs->pStmt_list, 2);
    }

    if (p_len_encoded)
    {
        *p_len_encoded = sqlite3_column_int64(pbs->pStmt_list, 3);
    }

    if (p_len_full)
    {
        *p_len_full = sqlite3_column_int64(pbs->pStmt_list, 4);
    }

    if (ppsz_hid_vcdiff_reference)
    {
        *ppsz_hid_vcdiff_reference = (const char *)sqlite3_column_text(pbs->pStmt_list,5);
    }

fail:
    ;
}

void SG_blobndx__list_unmarked_blobs__finish(
        SG_context * pCtx,
        SG_blobndx* pbs
        )
{
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pbs->pStmt_list)  );

fail:
    ;
}

void SG_blobndx__lookup(
        SG_context * pCtx,
        SG_blobndx* pbs,
        const char* psz_hid,
        SG_bool* pb_found,
        SG_int64* p_rowid,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        char** ppsz_hid_vcdiff_reference
        )
{
    int rc;

    SG_uint64 offset = 0;
    SG_blob_encoding blob_encoding = 0;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    const char * psz_hid_vcdiff_reference = NULL;
    SG_int64 rowid = 0;

    if (!pbs->pStmt_lookup)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pbs->pStmt_lookup,
                                          "SELECT \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\", rowid  FROM \"blobs\" WHERE \"hid\" = ?")  );
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
        offset = sqlite3_column_int64(pbs->pStmt_lookup, 0);

        blob_encoding = (SG_blob_encoding) sqlite3_column_int(pbs->pStmt_lookup, 1);

        len_encoded = sqlite3_column_int64(pbs->pStmt_lookup, 2);
        len_full = sqlite3_column_int64(pbs->pStmt_lookup, 3);

        if (SG_IS_BLOBENCODING_FULL(blob_encoding))
        {
            SG_ASSERT(len_encoded == len_full);
        }

        psz_hid_vcdiff_reference = (const char *)sqlite3_column_text(pbs->pStmt_lookup,4);

        rowid = sqlite3_column_int64(pbs->pStmt_lookup, 5);

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
        if (p_offset)
        {
            *p_offset = offset;
        }
        if (p_rowid)
        {
            *p_rowid = rowid;
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

void SG_blobndx__list_blobs_for_objectid(
	SG_context* pCtx,
    SG_blobndx* pbs,
	const char* psz_objectid,
	SG_vhash** ppvh
    )
{
	sqlite3_stmt * pStmt = NULL;
	int rc;
    SG_vhash* pvh_results = NULL;

	SG_NULLARGCHECK_RETURN(pbs);

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_results)  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE INDEX IF NOT EXISTS \"pack_objectid\" ON \"pack\" (\"objectid\")")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE INDEX IF NOT EXISTS \"blobs_hid_vcdiff\" ON \"blobs\" (\"hid_vcdiff\")")  );
	SG_ERR_CHECK(  sg_sqlite__prepare(
                pCtx, 
                pbs->psql,
                &pStmt,
                "SELECT \"len_encoded\", \"encoding\", \"len_full\", \"hid_vcdiff\", z.\"hid\", (SELECT count(*) FROM \"blobs\" q WHERE q.\"hid_vcdiff\"=z.\"hid\") FROM \"blobs\" z INNER JOIN pack p ON (z.hid = p.hid) WHERE \"objectid\" = ? ORDER BY \"len_full\" ASC")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt,1,psz_objectid)  );
	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
        SG_uint64 len_encoded = 0;
        SG_uint64 len_full = 0;
        SG_blob_encoding encoding = 0;
        const char * psz_hid_vcdiff_reference = NULL;
        const char * psz_hid = NULL;
        SG_vhash* pvh_rev = NULL;
        SG_int64 count_blobs_referencing_me = 0;

		len_encoded = (SG_uint64) sqlite3_column_int64(pStmt, 0);
		encoding = (SG_blob_encoding) sqlite3_column_int(pStmt, 1);
		len_full = (SG_uint64) sqlite3_column_int64(pStmt, 2);
		psz_hid_vcdiff_reference = (const char *)sqlite3_column_text(pStmt, 3);
		psz_hid = (const char *)sqlite3_column_text(pStmt, 4);
		count_blobs_referencing_me = (SG_int64) sqlite3_column_int64(pStmt, 5);

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_results, psz_hid, &pvh_rev)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_rev, "len_encoded", len_encoded)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_rev, "len_full", len_full)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_rev, "encoding", encoding)  );
        if (SG_BLOBENCODING__VCDIFF == encoding)
        {
            SG_ASSERT(psz_hid_vcdiff_reference);
        }
        if (psz_hid_vcdiff_reference)
        {
            SG_ASSERT(SG_BLOBENCODING__VCDIFF == encoding);
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_rev, "hid_vcdiff_reference", psz_hid_vcdiff_reference)  );
        }
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_rev, "count_blobs_referencing_me", count_blobs_referencing_me)  );
	}
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    *ppvh = pvh_results;
    pvh_results = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_results);
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_blobndx__list_pack_candidates(
	SG_context* pCtx,
    SG_blobndx* pbs,
	SG_uint32 nMinRevisionCount,
	SG_varray ** ppvArrayResults
    )
{
	sqlite3_stmt * pStmt = NULL;
	int rc;
	SG_varray * pvaResults = NULL;
	SG_vhash * pvhCurrent = NULL;

	SG_NULLARGCHECK_RETURN(pbs);

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE INDEX IF NOT EXISTS \"pack_hid\" ON \"pack\" (\"hid\")")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "CREATE INDEX IF NOT EXISTS \"pack_objectid\" ON \"pack\" (\"objectid\")")  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaResults)  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql,&pStmt,
                            "SELECT COUNT(b.\"hid\") n, SUM(b.\"len_encoded\"), SUM(b.\"len_full\"), p.\"objectid\" FROM \"blobs\" b INNER JOIN \"pack\" p ON (b.hid = p.hid) WHERE \"objectid\" IS NOT NULL GROUP BY \"objectid\" ORDER BY \"n\" DESC")  );

	rc = sqlite3_step(pStmt);
	while (SQLITE_ROW == rc)
	{
        SG_uint32 count = 0;

		count = (SG_uint32) sqlite3_column_int(pStmt, 0);

        if (count >= nMinRevisionCount)
        {
            SG_uint64 len_encoded = 0;
            SG_uint64 len_full = 0;
            const char * objectid = NULL;

            len_encoded = (SG_uint64) sqlite3_column_int64(pStmt, 1);
            len_full = (SG_uint64) sqlite3_column_int64(pStmt, 2);
            objectid = (const char *)sqlite3_column_text(pStmt, 3);

            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhCurrent)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhCurrent, "len_encoded", len_encoded)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhCurrent, "len_full", len_full)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhCurrent, "revision_count", count)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhCurrent, "objectid", objectid)  );
            SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pvaResults, &pvhCurrent)  );
        }

		rc = sqlite3_step(pStmt);
	}
	SG_RETURN_AND_NULL(pvaResults, ppvArrayResults);
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

struct _sg_fragballinfo
{
    SG_pathname* path;
    SG_uint64 length;
    SG_vhash* pvh_header;
    SG_uint16 version;
    SG_uint64 pos_after_initial_header;
    SG_file* file;
    SG_blobndx* pbs;
    SG_vhash* pvh_dags;
};
typedef struct _sg_fragballinfo SG_fragballinfo;

void SG_fragballinfo__free(
    SG_context* pCtx,
    SG_fragballinfo* p
    )
{
    if (!p)
    {
        return;
    }

    SG_BLOBNDX_NULLFREE(pCtx, p->pbs);
    SG_VHASH_NULLFREE(pCtx, p->pvh_header);
    SG_VHASH_NULLFREE(pCtx, p->pvh_dags);
    SG_PATHNAME_NULLFREE(pCtx, p->path);
    SG_FILE_NULLCLOSE(pCtx, p->file);
    SG_NULLFREE(pCtx, p);
}

static void sg_v3_slurp(
	SG_context* pCtx,
    SG_fragballinfo* pfbi
	);

void SG_fragballinfo__alloc(
    SG_context* pCtx,
    SG_pathname** ppath,
    SG_fragballinfo** pp
    )
{
    const char* psz_version = NULL;
    SG_fragballinfo* pfbi = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, pfbi)  );

    pfbi->path = *ppath;
    *ppath = NULL;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pfbi->path, &pfbi->length, NULL)  );
    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pfbi->path, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pfbi->file)  );

    // the fragball header is always a v1-style header, length bytes followed by an uncompressed json
    SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfbi->file, &pfbi->pvh_header)  );

    SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &pfbi->pos_after_initial_header)  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pfbi->pvh_header, "version", &psz_version)  );
	pfbi->version = (SG_uint16) atoi(psz_version);

    if (3 == pfbi->version)
    {
        SG_ERR_CHECK(  sg_v3_slurp(pCtx, pfbi)  );
    }

    *pp = pfbi;
    pfbi = NULL;

fail:
    SG_fragballinfo__free(pCtx, pfbi);
}

void SG_fragballinfo__seek_to_beginning(
    SG_context* pCtx,
    SG_fragballinfo* pfbi
    )
{
    SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, pfbi->pos_after_initial_header)  );

fail:
    ;
}

struct _sg_tempfilecollection
{
    SG_vhash* pvh_tempfiles;
    SG_pathname* pPath_dir;
};
typedef struct _sg_tempfilecollection SG_tfc;

void SG_tfc__free(
	SG_context* pCtx,
    SG_tfc* ptfc
    )
{
    if (!ptfc)
    {
        return;
    }

    // the files are expected to be removed somewhere else

    SG_VHASH_NULLFREE(pCtx, ptfc->pvh_tempfiles);
    SG_PATHNAME_NULLFREE(pCtx, ptfc->pPath_dir);
    SG_NULLFREE(pCtx, ptfc);
}

void SG_tfc__alloc(
	SG_context* pCtx,
    SG_pathname* pPath_dir,
    SG_tfc** pp
    )
{
    SG_tfc* ptfc = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptfc)  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &ptfc->pPath_dir, pPath_dir)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &ptfc->pvh_tempfiles)  );

    *pp = ptfc;
    ptfc = NULL;

fail:
    SG_tfc__free(pCtx, ptfc);
}

void SG_tfc__has(
	SG_context* pCtx,
    SG_tfc* ptfc,
    const char* psz_hid,
    SG_bool* pb
    )
{
    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, ptfc->pvh_tempfiles, psz_hid, pb)  );
}

void SG_tfc__get(
	SG_context* pCtx,
    SG_tfc* ptfc,
    const char* psz_hid,
    SG_pathname** pp
    )
{
    const char* psz_filename = NULL;
    SG_pathname* pPath_temp = NULL;

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, ptfc->pvh_tempfiles, psz_hid, &psz_filename)  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_temp, ptfc->pPath_dir, psz_filename)  );

    *pp = pPath_temp;
    pPath_temp = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp);
}

void SG_tfc__remove(
        SG_context* pCtx,
        SG_tfc* ptfc,
        const char* psz_hid
        )
{
    SG_bool b_has = SG_FALSE;
    SG_pathname* pPath = NULL;

    SG_ERR_CHECK(  SG_vhash__has(pCtx, ptfc->pvh_tempfiles, psz_hid, &b_has)  );
    if (b_has)
    {
        SG_bool b_exists = SG_FALSE;
        const char* psz_name = NULL;
        
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, ptfc->pvh_tempfiles, psz_hid, &psz_name)  );
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, ptfc->pPath_dir, psz_name)  );
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
        if (b_exists)
        {
            SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
        }
        SG_ERR_CHECK(  SG_vhash__remove(pCtx, ptfc->pvh_tempfiles, psz_hid)  );
    }

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_tfc__add(
        SG_context* pCtx,
        SG_tfc* ptfc,
        const char* psz_hid,
        SG_pathname** pp,
        SG_bool* pb_exists
        )
{
    SG_bool b_has = SG_FALSE;
    SG_pathname* pPath = NULL;
    char buf[SG_TID_MAX_BUFFER_LENGTH];
    SG_bool b_exists = SG_FALSE;

    if (psz_hid)
    {
        SG_NULLARGCHECK(pb_exists);
        SG_ERR_CHECK(  SG_vhash__has(pCtx, ptfc->pvh_tempfiles, psz_hid, &b_has)  );
        if (b_has)
        {
            const char* psz_name = NULL;
            
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, ptfc->pvh_tempfiles, psz_hid, &psz_name)  );
            SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, ptfc->pPath_dir, psz_name)  );
            SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
            goto done;
        }
    }
    else
    {
        SG_ARGCHECK(!pb_exists, pb_exists);
    }

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf, sizeof(buf))  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, ptfc->pPath_dir, buf)  );
    b_exists = SG_FALSE;
    if (psz_hid)
    {
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, ptfc->pvh_tempfiles, psz_hid, buf)  );
    }

done:
    *pp = pPath;
    pPath = NULL;

    if (pb_exists)
    {
        *pb_exists = b_exists;
    }

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_fragballinfo__make_tfc(
    SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_tfc** pp
    )
{
    SG_pathname* pPath_tfc = NULL;
    SG_tfc* ptfc = NULL;

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_tfc, pfbi->path)  );
    SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPath_tfc)  );
    SG_ERR_CHECK(  SG_tfc__alloc(pCtx, pPath_tfc, &ptfc)  );

    *pp = ptfc;
    ptfc = NULL;

fail:
    SG_tfc__free(pCtx, ptfc);
    SG_PATHNAME_NULLFREE(pCtx, pPath_tfc);
}


struct _sg_twobuffers
{
	SG_byte buf1[MY_CHUNK_SIZE];
	SG_byte buf2[MY_CHUNK_SIZE];
};
typedef struct _sg_twobuffers sg_twobuffers;

static void SG_clone__get_blob_full(
	SG_context* pCtx,
    SG_pathname* pPath_fragball,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
    SG_blobndx* pbs,
    const char* psz_hid,
    SG_pathname** ppPath,
    SG_uint64* pi_offset,
    SG_uint64* pi_len_full
    );

struct found_blob
{
    SG_int64 rowid;
    SG_uint64 offset;
    SG_blob_encoding encoding;
    SG_uint64 len_encoded;
    SG_uint64 len_full;
    char hid_vcdiff[SG_HID_MAX_BUFFER_LENGTH];
};

void SG_clone__find_blob(
        SG_context* pCtx,
        SG_blobndx* pbs,
        const char* psz_hid,
        struct found_blob* pfb
        )
{
    SG_bool b_found = SG_FALSE;
    char* psz_hid_vcdiff = NULL;

    SG_ERR_CHECK(  SG_blobndx__lookup(
                pCtx, 
                pbs, 
                psz_hid, 
                &b_found, 
                &pfb->rowid,
                &pfb->offset, 
                &pfb->encoding, 
                &pfb->len_encoded, 
                &pfb->len_full, 
                &psz_hid_vcdiff
                )  );

    if (!b_found)
	{
		// blob doesn't exist
		SG_ERR_THROW2(SG_ERR_BLOB_NOT_FOUND, (pCtx, "%s", psz_hid));
	}

    if (psz_hid_vcdiff)
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, pfb->hid_vcdiff, sizeof(pfb->hid_vcdiff), psz_hid_vcdiff)  );
    }
    else
    {
        pfb->hid_vcdiff[0] = 0;
    }

fail:
    SG_NULLFREE(pCtx, psz_hid_vcdiff);
}

static void _store_one_blob(
	SG_context* pCtx,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
	SG_file* pFile,
	SG_repo_tx_handle* ptx,
	const char* psz_hid,
	SG_blob_encoding encoding,
	const char* psz_hid_vcdiff_reference,
	SG_uint64 len_full,
	SG_uint64 len_encoded
	)
{
	SG_uint64 sofar = 0;
	SG_uint32 got = 0;
	SG_repo_store_blob_handle* pbh = NULL;

	SG_NULLARGCHECK_RETURN(ptx);

	SG_ERR_CHECK(  SG_repo__store_blob__begin(
                pCtx, 
                pRepo, 
                ptx, 
                encoding, 
                psz_hid_vcdiff_reference, 
                len_full, 
                len_encoded, 
                psz_hid, 
                &pbh)  );

	while (sofar < len_encoded)
	{
		SG_uint32 want = 0;
		if ((len_encoded - sofar) > (SG_uint64) sizeof(ptwo->buf1))
		{
			want = (SG_uint32) sizeof(ptwo->buf1);
		}
		else
		{
			want = (SG_uint32) (len_encoded - sofar);
		}
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile, want, ptwo->buf1, &got)  );
		SG_ERR_CHECK(  SG_repo__store_blob__chunk(pCtx, pRepo, pbh, got, ptwo->buf1, NULL)  );

		sofar += got;
	}

	SG_ERR_CHECK(  SG_repo__store_blob__end(pCtx, pRepo, ptx, &pbh, NULL)  );

	return;

fail:
	if (pbh)
		SG_ERR_IGNORE(  SG_repo__store_blob__abort(pCtx, pRepo, ptx, &pbh)  );
}

static void _store_one_blob__pathname(
	SG_context* pCtx,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_pathname* pPath,
	SG_repo_tx_handle* ptx,
	const char* psz_hid,
	SG_blob_encoding encoding,
	const char* psz_hid_vcdiff_reference,
	SG_uint64 len_full
	)
{
    SG_file* pFile = NULL;
    SG_uint64 len_encoded = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len_encoded, NULL)  );

    if (SG_IS_BLOBENCODING_FULL(encoding))
    {
        SG_ASSERT(len_full == len_encoded);
    }

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  _store_one_blob(
        pCtx,
        ptwo,
        pRepo,
        pFile,
        ptx,
        psz_hid,
        encoding,
        psz_hid_vcdiff_reference,
        len_full,
        len_encoded
        )  );
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

fail:
    if (pFile)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
    }
}

static void _save_to_tempfile__zlib_to_full(
	SG_context* pCtx,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
	SG_file* pFile_zlib,
	const char* psz_hid,
	SG_uint64 len_encoded,
	SG_uint64 len_full,
    SG_pathname** ppPath
	)
{
	SG_uint64 sofar_encoded = 0;
	SG_uint64 sofar_full = 0;
    SG_pathname* pPath = NULL;
    int zError = -1;
	z_stream zStream;
    SG_file* pFile_full = NULL;
    SG_bool b_done = SG_FALSE;
    SG_bool b_already = SG_FALSE;

    SG_ERR_CHECK(  SG_tfc__add(pCtx, ptfc, psz_hid, &pPath, &b_already)  );

    if (b_already)
    {
        SG_uint64 off = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pFile_zlib, &off)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_zlib, off + len_encoded)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile_full)  );

        memset(&zStream,0,sizeof(zStream));
        zError = inflateInit(&zStream);
        if (zError != Z_OK)
        {
            SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
        }

        while (!b_done)
        {
            SG_uint32 got_full = 0;

            if (0 == zStream.avail_in)
            {
                SG_uint32 want = 0;
                SG_uint32 got_encoded = 0;

                if ((len_encoded - sofar_encoded) > sizeof(ptwo->buf1))
                {
                    want = sizeof(ptwo->buf1);
                }
                else
                {
                    want = (SG_uint32) (len_encoded - sofar_encoded);
                }
                SG_ERR_CHECK(  SG_file__read(pCtx, pFile_zlib, want, ptwo->buf1, &got_encoded)  );

                zStream.next_in = ptwo->buf1;
                zStream.avail_in = got_encoded;

                sofar_encoded += got_encoded;
            }

            zStream.next_out = ptwo->buf2;
            zStream.avail_out = sizeof(ptwo->buf2);

            while (1)
            {
                // let decompressor decompress what it can of our input.  it may or
                // may not take all of it.  this will update next_in, avail_in,
                // next_out, and avail_out.
                //
                // WARNING: we get Z_STREAM_END when the decompressor is finished
                // WARNING: processing all of the input.  it gives this immediately
                // WARNING: (with the last partial buffer) rather than returning OK
                // WARNING: for the last partial buffer and then returning END with
                // WARNING: zero bytes transferred.

                zError = inflate(&zStream,Z_NO_FLUSH);
                if ((zError != Z_OK)  &&  (zError != Z_STREAM_END))
                {
                    SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
                }

                if (zError == Z_STREAM_END)
                {
                    b_done = SG_TRUE;
                    break;
                }

                // if decompressor didn't take all of our input, let it continue
                // with the existing buffer (next_in was advanced).

                if (zStream.avail_in == 0)
                {
                    break;
                }

                if (zStream.avail_out == 0)
                {
                    /* no room left */
                    break;
                }
            }

            if (zStream.avail_out < sizeof(ptwo->buf2))
            {
                got_full = sizeof(ptwo->buf2) - zStream.avail_out;
            }
            else
            {
                got_full = 0;
            }

            if (got_full)
            {
                SG_ERR_CHECK(  SG_file__write(pCtx, pFile_full, got_full, (SG_byte*) ptwo->buf2, NULL)  );
                sofar_full += got_full;
            }
        }

        if (sofar_full != len_full)
        {
            SG_ERR_THROW(  SG_ERR_INCOMPLETEWRITE  );
        }
        if (sofar_encoded != len_encoded)
        {
            SG_ERR_THROW(  SG_ERR_INCOMPLETEWRITE  );
        }

        SG_ASSERT( (zStream.avail_in == 0) );

        inflateEnd(&zStream);

        SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_full)  );
    }

    *ppPath = pPath;
    pPath = NULL;

fail:
    if (pFile_full)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_full)  );
    }
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void _save_to_tempfile__full_to_vcdiff(
	SG_context* pCtx,
    SG_tfc* ptfc,
	SG_file* pFile,
	SG_uint64 len_full,
    SG_pathname* pPath_ref,
    SG_uint64 offset_ref,
    SG_uint64 len_full_ref,
    SG_pathname** ppPath_temp
	)
{
    SG_seekreader* psr_reference = NULL;
    SG_readstream* pstrm_target = NULL;
    SG_writestream* pstrm_delta = NULL;
    SG_pathname* pPath = NULL;

    SG_UNUSED(len_full_ref);

    SG_ERR_CHECK(  SG_tfc__add(pCtx, ptfc, NULL, &pPath, NULL)  );

    SG_ERR_CHECK(  SG_seekreader__alloc__for_file(pCtx, pPath_ref, offset_ref, &psr_reference)  );

    SG_ERR_CHECK(  SG_readstream__alloc(
					   pCtx,
					   pFile,
					   (SG_stream__func__read*) SG_file__read,
                       NULL,
                       len_full,
					   &pstrm_target)  );

    SG_ERR_CHECK(  SG_writestream__alloc__for_file(pCtx, pPath, &pstrm_delta)  );

    SG_ERR_CHECK(  SG_vcdiff__deltify__streams(pCtx, psr_reference, pstrm_target, pstrm_delta)  );

    SG_ERR_CHECK(  SG_readstream__close(pCtx, pstrm_target)  );
    pstrm_target = NULL;
    SG_ERR_CHECK(  SG_writestream__close(pCtx, pstrm_delta)  );
    pstrm_delta = NULL;
    SG_ERR_CHECK(  SG_seekreader__close(pCtx, psr_reference)  );
    psr_reference = NULL;

    *ppPath_temp = pPath;
    pPath = NULL;

fail:
    if (psr_reference)
    {
        SG_ERR_IGNORE(  SG_seekreader__close(pCtx, psr_reference)  );
    }
}

static void _save_to_tempfile__full_to_zlib(
	SG_context* pCtx,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
	SG_file* pFile,
	SG_uint64 len_full,
    SG_pathname** ppPath_temp
	)
{
	SG_uint64 sofar = 0;
	SG_uint32 got = 0;
	z_stream zStream;
    int zError = -1;
    SG_pathname* pPath = NULL;
    SG_file* pFile_temp = NULL;

    SG_ERR_CHECK(  SG_tfc__add(pCtx, ptfc, NULL, &pPath, NULL)  );

    // init the compressor
    memset(&zStream,0,sizeof(zStream));
    zError = deflateInit(&zStream,Z_DEFAULT_COMPRESSION);
    if (zError != Z_OK)
    {
        SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
    }

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile_temp)  );

	while (sofar < len_full)
	{
		SG_uint32 want = 0;
		if ((len_full - sofar) > (SG_uint64) sizeof(ptwo->buf1))
		{
			want = (SG_uint32) sizeof(ptwo->buf1);
		}
		else
		{
			want = (SG_uint32) (len_full - sofar);
		}
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile, want, ptwo->buf1, &got)  );

        // give this chunk to compressor (it will update next_in and avail_in as
        // it consumes the input).

        zStream.next_in = (SG_byte*) ptwo->buf1;
        zStream.avail_in = got;

        while (1)
        {
            // give the compressor the complete output buffer

            zStream.next_out = ptwo->buf2;
            zStream.avail_out = sizeof(ptwo->buf2);

            // let compressor compress what it can of our input.  it may or
            // may not take all of it.  this will update next_in, avail_in,
            // next_out, and avail_out.

            zError = deflate(&zStream,Z_NO_FLUSH);
            if (zError != Z_OK)
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }

            // if there was enough input to generate a compression block,
            // we can write it to our output file.  the amount generated
            // is the delta of avail_out (or equally of next_out).

            if (zStream.avail_out < sizeof(ptwo->buf2))
            {
                SG_uint32 nOut = sizeof(ptwo->buf2) - zStream.avail_out;
                SG_ASSERT ( (nOut == (SG_uint32)(zStream.next_out - ptwo->buf2)) );

                SG_ERR_CHECK(  SG_file__write(pCtx, pFile_temp, nOut, (SG_byte*) ptwo->buf2, NULL)  );
            }

            // if compressor didn't take all of our input, let it
            // continue with the existing buffer (next_in was advanced).

            if (zStream.avail_in == 0)
                break;
        }

		sofar += got;
	}

    // we reached end of the input file.  now we need to tell the
    // compressor that we have no more input and that it needs to
    // compress and flush any partial buffers that it may have.

    SG_ASSERT( (zStream.avail_in == 0) );
    while (1)
    {
        zStream.next_out = ptwo->buf2;
        zStream.avail_out = sizeof(ptwo->buf2);

        zError = deflate(&zStream,Z_FINISH);
        if ((zError != Z_OK) && (zError != Z_STREAM_END))
        {
            SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
        }

        if (zStream.avail_out < sizeof(ptwo->buf2))
        {
            SG_uint32 nOut = sizeof(ptwo->buf2) - zStream.avail_out;
            SG_ASSERT ( (nOut == (SG_uint32)(zStream.next_out - ptwo->buf2)) );

            SG_ERR_CHECK(  SG_file__write(pCtx, pFile_temp, nOut, (SG_byte*) ptwo->buf2, NULL)  );
        }

        if (zError == Z_STREAM_END)
            break;
    }

    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_temp)  );

	deflateEnd(&zStream);

    *ppPath_temp = pPath;
    pPath = NULL;

fail:
    if (pFile_temp)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_temp)  );
    }
}

static void _save_to_tempfile__full_to_vcdiff__pathname(
	SG_context* pCtx,
    SG_tfc* ptfc,
    SG_pathname* pPath_full,
    SG_pathname* pPath_ref,
    SG_uint64 offset_ref,
    SG_uint64 len_full_ref,
    SG_pathname** ppPath_vcdiff
	)
{
    SG_file* pFile = NULL;
    SG_pathname* pPath_temp_vcdiff = NULL;
    SG_uint64 len_full = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_full, &len_full, NULL)  );

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_full, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  _save_to_tempfile__full_to_vcdiff(
                pCtx,
                ptfc,
                pFile,
                len_full,
                pPath_ref,
                offset_ref,
                len_full_ref,
                &pPath_temp_vcdiff
                )  );
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

    *ppPath_vcdiff = pPath_temp_vcdiff;
    pPath_temp_vcdiff = NULL;

fail:
    if (pFile)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
    }
}

static void _save_to_tempfile__full_to_zlib__pathname(
	SG_context* pCtx,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
    SG_pathname* pPath_full,
    SG_pathname** ppPath_zlib
	)
{
    SG_file* pFile = NULL;
    SG_pathname* pPath_temp_zlib = NULL;
    SG_uint64 len_full = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_full, &len_full, NULL)  );

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_full, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
    SG_ERR_CHECK(  _save_to_tempfile__full_to_zlib(
                pCtx,
                ptwo,
                ptfc,
                pFile,
                len_full,
                &pPath_temp_zlib
                )  );
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

    *ppPath_zlib = pPath_temp_zlib;
    pPath_temp_zlib = NULL;

fail:
    if (pFile)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
    }
}

static void _save_to_tempfile__vcdiff_to_full(
	SG_context* pCtx,
    SG_tfc* ptfc,
	SG_file* pFile,
    const char* psz_hid,
    SG_uint64 len_encoded,
    SG_uint64 len_full,
    SG_pathname* pPath_ref,
    SG_uint64 offset_ref,
    SG_uint64 len_full_ref,
    SG_pathname** ppPath_temp
    )
{
    SG_seekreader* psr_reference = NULL;
    SG_readstream* pstrm_delta = NULL;
    SG_file* pFile_full = NULL;
    SG_bool b_done = SG_FALSE;
    SG_pathname* pPath = NULL;
    SG_vcdiff_undeltify_state* pst = NULL;
    SG_uint64 sofar_full = 0;
    SG_bool b_already = SG_FALSE;

    SG_UNUSED(len_full_ref);

    SG_ERR_CHECK(  SG_tfc__add(pCtx, ptfc, psz_hid, &pPath, &b_already)  );
    if (b_already)
    {
        SG_uint64 off = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pFile, &off)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, off + len_encoded)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile_full)  );

        SG_ERR_CHECK(  SG_seekreader__alloc__for_file(pCtx, pPath_ref, offset_ref, &psr_reference)  );

        SG_ERR_CHECK(  SG_readstream__alloc(
                           pCtx,
                           pFile,
                           (SG_stream__func__read*) SG_file__read,
                           NULL,
                           len_encoded,
                           &pstrm_delta)  );

        /* Now begin the undeltify operation */
        SG_ERR_CHECK(  SG_vcdiff__undeltify__begin(pCtx, psr_reference, pstrm_delta, &pst)  );
        SG_ASSERT(pst);

        while (!b_done)
        {
            SG_uint32 got_full = 0;
            SG_byte* p_buf = NULL;

            SG_ERR_CHECK(  SG_vcdiff__undeltify__chunk(pCtx, pst, &p_buf, &got_full)  );
            if (
                    !p_buf
                    || (0 == got_full)
               )
            {
                b_done = SG_TRUE;
            }

            if (got_full)
            {
                SG_ERR_CHECK(  SG_file__write(pCtx, pFile_full, got_full, (SG_byte*) p_buf, NULL)  );
                sofar_full += got_full;
            }
        }

        // end

        SG_ERR_CHECK(  SG_vcdiff__undeltify__end(pCtx, pst)  );
        pst = NULL;

        SG_ERR_CHECK(  SG_readstream__close(pCtx, pstrm_delta)  );
        pstrm_delta = NULL;

        SG_ERR_CHECK(  SG_seekreader__close(pCtx, psr_reference)  );
        psr_reference = NULL;

        SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_full)  );

        if (sofar_full != len_full)
        {
            SG_ERR_THROW(  SG_ERR_INCOMPLETEWRITE  );
        }
    }

    *ppPath_temp = pPath;
    pPath = NULL;

fail:
    if (pFile_full)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_full)  );
    }
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void sg_clone__copy_one_blob(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    const char* psz_hid,
    SG_int64* pi_rowid
    )
{
    struct found_blob fb;

    memset(&fb, 0, sizeof(fb));

    SG_ERR_CHECK(  SG_clone__find_blob(pCtx, pfbi->pbs, psz_hid, &fb)  );

    SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, fb.offset)  );
    SG_ERR_CHECK(  _store_one_blob(
                pCtx,
                ptwo,
                pRepo,
                pfbi->file,
                ptx,
                psz_hid,
                fb.encoding,
                SG_IS_BLOBENCODING_VCDIFF(fb.encoding) ? fb.hid_vcdiff : NULL,
                fb.len_full,
                fb.len_encoded
                )  );

    if (pi_rowid)
    {
        *pi_rowid = fb.rowid;
    }

fail:
    ;
}

static void SG_clone__get_blob_full__mem(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    const char* psz_hid,
    SG_int64* pi_rowid,
    char** pp_data,
    SG_uint32* pi_len
    )
{
    char* p_data = NULL;
    struct found_blob fb;
    char* p_encoded = NULL;
    char* p_reference = NULL;

    memset(&fb, 0, sizeof(fb));

    SG_ERR_CHECK(  SG_clone__find_blob(pCtx, pfbi->pbs, psz_hid, &fb)  );
    SG_ERR_CHECK(  SG_malloc(pCtx, (SG_uint32) (fb.len_full+1), &p_data)  );
    p_data[fb.len_full] = 0;

    if (SG_IS_BLOBENCODING_FULL(fb.encoding))
    {
        SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, fb.offset)  );
        SG_ERR_CHECK(  SG_file__read(pCtx, pfbi->file, (SG_uint32) fb.len_full, (SG_byte*) p_data, NULL)  );
        goto done;
    }
    else if (SG_IS_BLOBENCODING_ZLIB(fb.encoding))
    {
        SG_ERR_CHECK(  SG_malloc(pCtx, (SG_uint32) (fb.len_encoded), &p_encoded)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, fb.offset)  );
        SG_ERR_CHECK(  SG_file__read(pCtx, pfbi->file, (SG_uint32) fb.len_encoded, (SG_byte*) p_encoded, NULL)  );
        SG_ERR_CHECK(  SG_zlib__inflate__memory(pCtx, (SG_byte*) p_encoded, (SG_uint32)fb.len_encoded, (SG_byte*) p_data, (SG_uint32)fb.len_full)  );
        SG_NULLFREE(pCtx, p_encoded);

        goto done;
    }
    else if (SG_IS_BLOBENCODING_VCDIFF(fb.encoding))
    {
        SG_uint32 len_reference = 0;

        // get the reference blob
        SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                    pCtx,
                    pfbi,
                    fb.hid_vcdiff,
                    NULL,
                    &p_reference,
                    &len_reference
                    )  );

        // read the encoded blob
        SG_ERR_CHECK(  SG_malloc(pCtx, (SG_uint32) (fb.len_encoded), &p_encoded)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, fb.offset)  );
        SG_ERR_CHECK(  SG_file__read(pCtx, pfbi->file, (SG_uint32) fb.len_encoded, (SG_byte*) p_encoded, NULL)  );

        SG_ERR_CHECK(  SG_vcdiff__undeltify__in_memory(
                    pCtx,
                    (SG_byte*) p_reference, len_reference,
                    (SG_byte*) p_data, (SG_uint32) fb.len_full,
                    (SG_byte*) p_encoded, (SG_uint32) fb.len_encoded
                    )  );

        SG_NULLFREE(pCtx, p_encoded);
        SG_NULLFREE(pCtx, p_reference);

        goto done;
    }
    else
    {
        SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);
    }

done:
    p_data[fb.len_full] = 0;

    *pp_data = p_data;
    p_data = NULL;

    if (pi_len)
    {
        *pi_len = (SG_uint32) fb.len_full;
    }

    if (pi_rowid)
    {
        *pi_rowid = fb.rowid;
    }

fail:
    SG_NULLFREE(pCtx, p_data);
    SG_NULLFREE(pCtx, p_encoded);
}

static void SG_clone__get_blob_as_vhash(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    const char* psz_hid,
    SG_int64* pi_rowid,
    SG_vhash** ppvh
    )
{
    SG_vhash* pvh = NULL;
    char* psz_json = NULL;

    SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                pCtx,
                pfbi,
                psz_hid,
                pi_rowid,
                &psz_json,
                NULL
                )  );

    SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, psz_json));

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_NULLFREE(pCtx, psz_json);
}

static void SG_clone__get_blob_full(
	SG_context* pCtx,
    SG_pathname* pPath_fragball,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
    SG_blobndx* pbs,
    const char* psz_hid,
    SG_pathname** ppPath,
    SG_uint64* pi_offset,
    SG_uint64* pi_len_full
    )
{
    SG_bool b_has = SG_FALSE;
    SG_pathname* pPath_data = NULL;
    SG_pathname* pPath_temp = NULL;
    SG_pathname* pPath_ref = NULL;
    SG_file* pFile_data = NULL;

    SG_ERR_CHECK(  SG_tfc__has(pCtx, ptfc, psz_hid, &b_has)  );
    if (b_has)
    {
        SG_uint64 len = 0;

        SG_ERR_CHECK(  SG_tfc__get(pCtx, ptfc, psz_hid, &pPath_temp)  );

        SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_temp, &len, NULL)  );

        *ppPath = pPath_temp;
        pPath_temp = NULL;
        *pi_offset = 0;
        *pi_len_full = len;

        goto done;
    }
    else
    {
        struct found_blob fb;

        memset(&fb, 0, sizeof(fb));

        SG_ERR_CHECK(  SG_clone__find_blob(pCtx, pbs, psz_hid, &fb)  );
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_data, pPath_fragball)  );

        if (SG_IS_BLOBENCODING_FULL(fb.encoding))
        {
            *ppPath = pPath_data;
            pPath_data = NULL;
            *pi_offset = fb.offset;
            *pi_len_full = fb.len_full;

            goto done;
        }
        else if (SG_IS_BLOBENCODING_ZLIB(fb.encoding))
        {
			SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_data, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile_data)  );
            SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_data, fb.offset)  );
            SG_ERR_CHECK(  _save_to_tempfile__zlib_to_full(
                        pCtx,
                        ptwo,
                        ptfc,
                        pFile_data,
                        psz_hid,
                        fb.len_encoded,
                        fb.len_full,
                        &pPath_temp
                        )  );
            SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_data)  );

            *ppPath = pPath_temp;
            pPath_temp = NULL;
            *pi_offset = 0;
            *pi_len_full = fb.len_full;

            goto done;
        }
        else if (SG_IS_BLOBENCODING_VCDIFF(fb.encoding))
        {
            SG_uint64 offset_ref = 0;
            SG_uint64 len_full_ref = 0;

            SG_ERR_CHECK(  SG_clone__get_blob_full(
                        pCtx,
                        pPath_fragball,
                        ptwo,
                        ptfc,
                        pbs,
                        fb.hid_vcdiff,
                        &pPath_ref,
                        &offset_ref,
                        &len_full_ref
                        )  );
			SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_data, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile_data)  );
            SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_data, fb.offset)  );
            SG_ERR_CHECK(  _save_to_tempfile__vcdiff_to_full(
                        pCtx,
                        ptfc,
                        pFile_data,
                        psz_hid,
                        fb.len_encoded,
                        fb.len_full,
                        pPath_ref,
                        offset_ref,
                        len_full_ref,
                        &pPath_temp
                        )  );
            SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_data)  );
            SG_PATHNAME_NULLFREE(pCtx, pPath_ref);

            *ppPath = pPath_temp;
            pPath_temp = NULL;
            *pi_offset = 0;
            *pi_len_full = fb.len_full;

            goto done;
        }
        else
        {
            SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);
        }
    }

done:
fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp);
    SG_PATHNAME_NULLFREE(pCtx, pPath_data);
    SG_PATHNAME_NULLFREE(pCtx, pPath_ref);
    if (pFile_data)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile_data)  );
    }
}

static void _convert_one_blob(
	SG_context* pCtx,
    SG_pathname* pPath_fragball,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
    SG_blobndx* pbs,
    SG_repo* pRepo,
	SG_file* pFile_given,
	SG_repo_tx_handle* ptx,
	const char* psz_hid,
	SG_blob_encoding encoding_CURRENT,
	const char* psz_hid_vcdiff_reference,
	SG_uint64 len_full,
	SG_uint64 len_encoded,
	SG_blob_encoding encoding_DESIRED,
	const char* psz_hid_vcdiff_reference_DESIRED,
    SG_bool* pb_vcdiff_result_was_ok,
    SG_uint32 demand_vcdiff_savings_over_full,
    SG_uint32 demand_vcdiff_savings_over_zlib,
    SG_uint32 demand_zlib_savings_over_full
	)
{
    SG_pathname* pPath_temp_full = NULL;
    SG_pathname* pPath_temp_zlib = NULL;
    SG_pathname* pPath_ref = NULL;
    SG_pathname* pPath_temp_vcdiff = NULL;
    SG_uint64 len_encoded_zlib = 0;
    SG_uint64 remember_the_offset = 0;

    SG_ARGCHECK_RETURN( !SG_ARE_BLOBENCODINGS_BASICALLY_EQUAL(encoding_CURRENT, encoding_DESIRED) || SG_IS_BLOBENCODING_VCDIFF(encoding_DESIRED), encoding_DESIRED );

    SG_ARGCHECK_RETURN( ( (demand_vcdiff_savings_over_full <= 100)), demand_vcdiff_savings_over_full);
    SG_ARGCHECK_RETURN( ( (demand_vcdiff_savings_over_zlib <= 100)), demand_vcdiff_savings_over_zlib);
    SG_ARGCHECK_RETURN( ( (demand_zlib_savings_over_full <= 100)), demand_zlib_savings_over_full);

    SG_ARGCHECK_RETURN(
            (
             SG_IS_BLOBENCODING_FULL(encoding_CURRENT)
            || SG_IS_BLOBENCODING_ZLIB(encoding_CURRENT)
            || SG_IS_BLOBENCODING_VCDIFF(encoding_CURRENT)
            ), 
            encoding_CURRENT
            );
    SG_ARGCHECK_RETURN(
            (
             SG_IS_BLOBENCODING_FULL(encoding_DESIRED)
            || SG_IS_BLOBENCODING_ZLIB(encoding_DESIRED)
            || SG_IS_BLOBENCODING_VCDIFF(encoding_DESIRED)
            ), 
            encoding_DESIRED
            );

    SG_ERR_CHECK(  SG_file__tell(pCtx, pFile_given, &remember_the_offset)  );

    if (SG_IS_BLOBENCODING_FULL(encoding_CURRENT))
    {
        SG_ASSERT(len_full == len_encoded);
        SG_ASSERT(!psz_hid_vcdiff_reference);

        // We'll need the file in zlib form no matter what, so convert it.
        SG_ERR_CHECK(  _save_to_tempfile__full_to_zlib(
                    pCtx,
                    ptwo,
                    ptfc,
                    pFile_given,
                    len_full,
                    &pPath_temp_zlib
                    )  );
        SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_temp_zlib, &len_encoded_zlib, NULL)  );
    }
    else 
    {
        if (SG_IS_BLOBENCODING_ZLIB(encoding_CURRENT))
        {
            len_encoded_zlib = len_encoded;
            SG_ERR_CHECK(  _save_to_tempfile__zlib_to_full(
                        pCtx,
                        ptwo,
                        ptfc,
                        pFile_given,
                        psz_hid,
                        len_encoded,
                        len_full,
                        &pPath_temp_full
                        )  );
        }
        else if (SG_IS_BLOBENCODING_VCDIFF(encoding_CURRENT))
        {
            SG_uint64 offset_ref = 0;
            SG_uint64 len_full_ref = 0;

            SG_ERR_CHECK(  SG_clone__get_blob_full(
                        pCtx,
                        pPath_fragball,
                        ptwo,
                        ptfc,
                        pbs,
                        psz_hid_vcdiff_reference,
                        &pPath_ref,
                        &offset_ref,
                        &len_full_ref
                        )  );
            SG_ERR_CHECK(  _save_to_tempfile__vcdiff_to_full(
                        pCtx,
                        ptfc,
                        pFile_given,
                        psz_hid,
                        len_encoded,
                        len_full,
                        pPath_ref,
                        offset_ref,
                        len_full_ref,
                        &pPath_temp_full
                        )  );
            SG_PATHNAME_NULLFREE(pCtx, pPath_ref);

            if (!SG_IS_BLOBENCODING_FULL(encoding_DESIRED))
            {
                SG_ERR_CHECK(  _save_to_tempfile__full_to_zlib__pathname(
                            pCtx,
                            ptwo,
                            ptfc,
                            pPath_temp_full,
                            &pPath_temp_zlib
                            )  );
                SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_temp_zlib, &len_encoded_zlib, NULL)  );
            }
        }
    }

    if (SG_IS_BLOBENCODING_FULL(encoding_DESIRED))
    {
        SG_ERR_CHECK(  _store_one_blob__pathname(
            pCtx,
            ptwo,
            pRepo,
            pPath_temp_full,
            ptx,
            psz_hid,
            encoding_DESIRED,
            NULL,
            len_full
            )  );
    }
    else if (SG_IS_BLOBENCODING_ZLIB(encoding_DESIRED))
    {
        SG_bool b_zlib = SG_TRUE;

        // decide whether to use zlib or not
        if (demand_zlib_savings_over_full)
        {
            SG_uint64 threshold = len_full - (len_full * demand_zlib_savings_over_full / 100);
            if (len_encoded_zlib > threshold)
            {
                b_zlib = SG_FALSE;
            }
        }

        if (b_zlib)
        {
            SG_ERR_CHECK(  _store_one_blob__pathname(
                pCtx,
                ptwo,
                pRepo,
                pPath_temp_zlib,
                ptx,
                psz_hid,
                encoding_DESIRED,
                NULL,
                len_full
                )  );
        }
        else
        {
            // store it full

            if (SG_IS_BLOBENCODING_FULL(encoding_CURRENT))
            {
                SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_given, remember_the_offset)  );
                SG_ERR_CHECK(  _store_one_blob(
                    pCtx,
                    ptwo,
                    pRepo,
                    pFile_given,
                    ptx,
                    psz_hid,
                    SG_BLOBENCODING__KEEPFULLFORNOW,
                    NULL,
                    len_full,
                    len_full
                    )  );
            }
            else
            {
                SG_ERR_CHECK(  _store_one_blob__pathname(
                    pCtx,
                    ptwo,
                    pRepo,
                    pPath_temp_full,
                    ptx,
                    psz_hid,
                    SG_BLOBENCODING__KEEPFULLFORNOW,
                    NULL,
                    len_full
                    )  );
            }
        }
    }
    else if (SG_IS_BLOBENCODING_VCDIFF(encoding_DESIRED))
    {
        SG_uint64 offset_ref = 0;
        SG_uint64 len_full_ref = 0;
        SG_uint64 len_encoded_vcdiff = 0;
        SG_bool b_keep_the_vcdiff_result = SG_TRUE;

        SG_ASSERT(psz_hid_vcdiff_reference_DESIRED);

        SG_ERR_CHECK(  SG_clone__get_blob_full(
                    pCtx,
                    pPath_fragball,
                    ptwo,
                    ptfc,
                    pbs,
                    psz_hid_vcdiff_reference_DESIRED,
                    &pPath_ref,
                    &offset_ref,
                    &len_full_ref
                    )  );
        if (SG_IS_BLOBENCODING_FULL(encoding_CURRENT))
        {
            SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_given, remember_the_offset)  );
            SG_ERR_CHECK(  _save_to_tempfile__full_to_vcdiff(
                        pCtx,
                        ptfc,
                        pFile_given,
                        len_full,
                        pPath_ref,
                        offset_ref,
                        len_full_ref,
                        &pPath_temp_vcdiff
                        )  );
        }
        else
        {
            SG_ERR_CHECK(  _save_to_tempfile__full_to_vcdiff__pathname(
                        pCtx,
                        ptfc,
                        pPath_temp_full,
                        pPath_ref,
                        offset_ref,
                        len_full_ref,
                        &pPath_temp_vcdiff
                        )  );
        }
        SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_temp_vcdiff, &len_encoded_vcdiff, NULL)  );

        b_keep_the_vcdiff_result = SG_TRUE;

        SG_ASSERT(len_encoded_zlib);
        if (len_encoded_zlib < len_full)
        {
            if (demand_vcdiff_savings_over_zlib)
            {
                SG_uint64 threshold = len_encoded_zlib - (len_encoded_zlib * demand_vcdiff_savings_over_zlib / 100);
                if (len_encoded_vcdiff > threshold)
                {
                    b_keep_the_vcdiff_result = SG_FALSE;
                }
            }
        }

        if (b_keep_the_vcdiff_result)
        {
            if (demand_vcdiff_savings_over_full)
            {
                SG_uint64 threshold = len_full - (len_full * demand_vcdiff_savings_over_full / 100);

                if (len_encoded_vcdiff > threshold)
                {
                    b_keep_the_vcdiff_result = SG_FALSE;
                }
            }
        }

        if (b_keep_the_vcdiff_result)
        {
            SG_ERR_CHECK(  _store_one_blob__pathname(
                pCtx,
                ptwo,
                pRepo,
                pPath_temp_vcdiff,
                ptx,
                psz_hid,
                encoding_DESIRED,
                psz_hid_vcdiff_reference_DESIRED,
                len_full
                )  );
            *pb_vcdiff_result_was_ok = SG_TRUE;
        }
        else
        {
            SG_bool b_zlib = SG_TRUE;

            // decide whether to use zlib or not
            if (demand_zlib_savings_over_full)
            {
                SG_uint64 threshold = len_full - (len_full * demand_zlib_savings_over_full / 100);
                if (len_encoded_zlib > threshold)
                {
                    b_zlib = SG_FALSE;
                }
            }

            if (b_zlib)
            {
                // store it ZLIB

                if (SG_IS_BLOBENCODING_ZLIB(encoding_CURRENT))
                {
                    SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_given, remember_the_offset)  );
                    SG_ERR_CHECK(  _store_one_blob(
                        pCtx,
                        ptwo,
                        pRepo,
                        pFile_given,
                        ptx,
                        psz_hid,
                        SG_BLOBENCODING__ZLIB,
                        NULL,
                        len_full,
                        len_encoded_zlib
                        )  );
                }
                else
                {
                    SG_ERR_CHECK(  _store_one_blob__pathname(
                        pCtx,
                        ptwo,
                        pRepo,
                        pPath_temp_zlib,
                        ptx,
                        psz_hid,
                        SG_BLOBENCODING__ZLIB,
                        NULL,
                        len_full
                        )  );
                }
            }
            else
            {
                // store it FULL

                if (SG_IS_BLOBENCODING_FULL(encoding_CURRENT))
                {
                    SG_ERR_CHECK(  SG_file__seek(pCtx, pFile_given, remember_the_offset)  );
                    SG_ERR_CHECK(  _store_one_blob(
                        pCtx,
                        ptwo,
                        pRepo,
                        pFile_given,
                        ptx,
                        psz_hid,
                        SG_BLOBENCODING__KEEPFULLFORNOW,
                        NULL,
                        len_full,
                        len_full
                        )  );
                }
                else
                {
                    SG_ERR_CHECK(  _store_one_blob__pathname(
                        pCtx,
                        ptwo,
                        pRepo,
                        pPath_temp_full,
                        ptx,
                        psz_hid,
                        SG_BLOBENCODING__KEEPFULLFORNOW,
                        NULL,
                        len_full
                        )  );
                }
            }
            *pb_vcdiff_result_was_ok = SG_FALSE;
        }
    }

    if (pPath_temp_vcdiff)
    {
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_temp_vcdiff)  );
    }
    if (pPath_temp_zlib)
    {
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_temp_zlib)  );
    }
    SG_ERR_CHECK(  SG_tfc__remove(pCtx, ptfc, psz_hid)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_ref);
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_full);
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_vcdiff);
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_zlib);
}

static void _slurp__version_1(
	SG_context* pCtx,
    SG_blobndx* pbs,
	SG_file* pfb,
    SG_uint32 unit,
    SG_uint64 which_dagnum,
    SG_vhash** ppvh_changesets
	)
{
	SG_vhash* pvh = NULL;
	const char* psz_op = NULL;
	SG_uint64 iPos = 0;
	SG_dagfrag* pFrag = NULL;
    SG_varray* pva_changesets = NULL;
    SG_vhash* pvh_changesets = NULL;

	SG_NULLARGCHECK_RETURN(pfb);

    SG_ERR_CHECK(  SG_blobndx__begin_inserting(pCtx, pbs)  );

    if (ppvh_changesets)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_changesets)  );
    }

	while (1)
	{
		SG_uint64 offset_begin_record = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pfb, &offset_begin_record)  );
        SG_ERR_CHECK(  SG_log__set_finished(pCtx, (SG_uint32) (offset_begin_record / unit))  );
		SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfb, &pvh)  );
		if (!pvh)
		{
			break;
		}
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &psz_op)  );
		if (0 == strcmp(psz_op, "frag"))
		{
            if (pvh_changesets)
            {
                SG_uint64 dagnum = 0;
                SG_vhash* pvh_frag = NULL;

                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "frag", &pvh_frag)  );
                SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
                SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &dagnum)  );
                if (
                        (0 == which_dagnum)
                        || (dagnum == which_dagnum)
                   )
                {
                    SG_uint32 count = 0;
                    SG_uint32 i = 0;

                    // this is a clone, which implies a full copy, which means that frags will contain
                    // all the nodes all the way back to root, which means there is no fringe anyway
                    SG_ERR_CHECK(  SG_dagfrag__get_members__not_including_the_fringe(pCtx, pFrag, &pva_changesets)  );
                    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_changesets, &count)  );
                    for (i=0; i<count; i++)
                    {
                        const char* psz_csid = NULL;

                        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_changesets, i, &psz_csid)  );
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_changesets, psz_csid)  );
                    }
                    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
                }
                SG_DAGFRAG_NULLFREE(pCtx, pFrag);
            }
		}
        else if (0 == strcmp(psz_op, "blob"))
		{
			SG_int64 i64;
			const char* psz_hid = NULL;
			SG_blob_encoding encoding = 0;
			const char* psz_hid_vcdiff_reference = NULL;
			SG_uint64 len_full  = 0;
			SG_uint64 len_encoded  = 0;

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "hid", &psz_hid)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "encoding", &i64)  );
			encoding = (SG_blob_encoding) i64;
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh, "vcdiff_ref", &psz_hid_vcdiff_reference)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_full", &i64)  );
			len_full = (SG_uint64) i64;
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_encoded", &i64)  );
			len_encoded = (SG_uint64) i64;
			SG_ERR_CHECK(  SG_file__tell(pCtx, pfb, &iPos)  );

            SG_ERR_CHECK(  SG_blobndx__insert(
                        pCtx,
                        pbs,
                        psz_hid,
                        iPos,
                        encoding,
                        len_encoded,
                        len_full,
                        psz_hid_vcdiff_reference
                        )  );

			/* seek ahead to skip the blob */
			SG_ERR_CHECK(  SG_file__seek(pCtx, pfb, iPos + len_encoded)  );
		}
		SG_VHASH_NULLFREE(pCtx, pvh);
	}

    SG_ERR_CHECK(  SG_blobndx__finish_inserting(pCtx, pbs)  );

    if (ppvh_changesets)
    {
        *ppvh_changesets = pvh_changesets;
        pvh_changesets = NULL;
    }

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
	SG_VHASH_NULLFREE(pCtx, pvh_changesets);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
}

static void _pack__walk_tree(
	SG_context* pCtx,
    const char* psz_hid_tn,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
    SG_vhash* pvh_already
    )
{
    SG_vhash* pvh_tn = NULL;
    SG_vhash* pvh_entries = NULL;
    SG_uint32 count_entries = 0;
    SG_uint32 i = 0;
    SG_bool b_already = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_already, psz_hid_tn, &b_already)  );
    if (b_already)
    {
        return;
    }
    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_already, psz_hid_tn)  );

    SG_ERR_CHECK(  SG_clone__get_blob_as_vhash(pCtx, pfbi, psz_hid_tn, NULL, &pvh_tn)  );
    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_tn, "tne", &pvh_entries)  );
    if (pvh_entries)
    {
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_entries, &count_entries)  );
        for (i=0; i<count_entries; i++)
        {
            const char* psz_gid = NULL;
            SG_vhash* pvh_e = NULL;
            SG_uint32 t = 0;
            const char* psz_hid = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_entries, i, &psz_gid, &pvh_e)  );

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_e, "hid", &psz_hid)  );

            SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_e, "type", &t)  );

            if (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
            {
                SG_ERR_CHECK(  _pack__walk_tree(pCtx, psz_hid, pfbi, ptwo, ptfc, pvh_already)  );
            }
            else if (SG_TREENODEENTRY_TYPE_REGULAR_FILE == t)
            {
                SG_ERR_CHECK(  SG_blobndx__update_objectid(pCtx, pfbi->pbs, psz_hid, psz_gid )  );
            }
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_tn);
}

static void _pack__identify_vc_blobs(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_tfc* ptfc,
    SG_varray* pva_changesets
    )
{
    SG_uint32 count_ops = 0;
    SG_uint32 count_changesets = 0;
    SG_uint32 i = 0;
    SG_vhash* pvh_cs = NULL;
    SG_vhash* pvh_already = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_already)  );

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Identifying version control blobs", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_changesets, &count_changesets)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_changesets, "changesets")  );

    for (i=0; i<count_changesets; i++)
    {
        const char* psz_csid = NULL;
        const char* psz_hid_tn_root = NULL;
        SG_vhash* pvh_tree = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_changesets, i, &psz_csid)  );

        SG_ERR_CHECK(  SG_clone__get_blob_as_vhash(pCtx, pfbi, psz_csid, NULL, &pvh_cs)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs, "tree", &pvh_tree)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree, "root", &psz_hid_tn_root)  );
        
        SG_ERR_CHECK(  SG_blobndx__begin_updating_objectids(pCtx, pfbi->pbs)  );
        SG_ERR_CHECK(  _pack__walk_tree(pCtx, psz_hid_tn_root, pfbi, ptwo, ptfc, pvh_already)  );
        SG_ERR_CHECK(  SG_blobndx__finish_updating_objectids(pCtx, pfbi->pbs)  );

        SG_VHASH_NULLFREE(pCtx, pvh_cs);

        if (
                (0 == ((i+1) % 10))
                || ((i+1) >= count_changesets)
           )
        {
            SG_ERR_CHECK(  SG_log__set_finished(pCtx, i+1)  );
        }
    }

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_already);
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}

static void _prescan__version_1(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_vhash** ppvh_omit,
    SG_vhash** ppvh_audits
	)
{
	SG_vhash* pvh = NULL;
	const char* psz_op = NULL;
	SG_dagfrag* pFrag = NULL;
    SG_vhash* pvh_omit = NULL;
    SG_vhash* pvh_audits = NULL;
    SG_vhash* pvh_cs = NULL;
    SG_varray* pva_changesets = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_omit)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_audits)  );

    SG_ERR_CHECK(  SG_fragballinfo__seek_to_beginning(pCtx, pfbi)  );

    //fprintf(stderr, "length: %d\n", (int) pfbi->length);

	while (1)
	{
		SG_vhash* pvh_frag = NULL;
		SG_uint64 offset_begin_record = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_begin_record)  );
        //fprintf(stderr, "offset_begin_record: %d\n", (int) offset_begin_record);
		SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfbi->file, &pvh)  );
		if (!pvh)
		{
			break;
		}
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &psz_op)  );
		if (0 == strcmp(psz_op, "frag"))
		{
            SG_uint64 dagnum = 0;
			SG_uint64 offset_after_frag  = 0;

            SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_after_frag)  );

            //fprintf(stderr, "\nfound_frag\n");
            //SG_VHASH_STDERR(pvh);

			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "frag", &pvh_frag)  );
			SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
            SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &dagnum)  );
            if (SG_DAGNUM__IS_OLDAUDIT(dagnum))
            {
                SG_uint32 count = 0;
                SG_uint32 i = 0;

                dagnum = dagnum & (~ SG_DAGNUM__FLAG__OLDAUDIT);
                // this is a clone, which implies a full copy, which means that frags will contain
                // all the nodes all the way back to root, which means there is no fringe anyway
                SG_ERR_CHECK(  SG_dagfrag__get_members__not_including_the_fringe(pCtx, pFrag, &pva_changesets)  );
                SG_ERR_CHECK(  SG_varray__count(pCtx, pva_changesets, &count)  );
                for (i=0; i<count; i++)
                {
                    const char* psz_csid = NULL;
                    SG_uint32 count_parents = 0;
                    SG_uint32 i_parent = 0;
                    const char* psz_template = NULL;
                    SG_vhash* pvh_changes = NULL;
                    SG_vhash* pvh_db = NULL;

                    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_changesets, i, &psz_csid)  );
                    SG_ERR_CHECK(  SG_clone__get_blob_as_vhash(pCtx, pfbi, psz_csid, NULL, &pvh_cs)  );
                    //SG_VHASH_STDOUT(pvh_cs);
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_omit, psz_csid)  );
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs, "db", &pvh_db)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_db, "template", &psz_template)  );
                    if (psz_template)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_omit, psz_template)  );
                    }

                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_db, "changes", &pvh_changes)  );
                    //SG_VHASH_STDOUT(pvh_changes);

                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
                    for (i_parent=0; i_parent<count_parents; i_parent++)
                    {
                        SG_vhash* pvh_chgs_one_parent = NULL;
                        SG_vhash* pvh_add = NULL;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, NULL, &pvh_chgs_one_parent)  );
                        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_chgs_one_parent, "add", &pvh_add)  );
                        if (pvh_add)
                        {
                            SG_uint32 count_blobs = 0;
                            SG_uint32 i_blob = 0;

                            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count_blobs)  );
                            for (i_blob=0; i_blob<count_blobs; i_blob++)
                            {
                                const char* psz_hid = NULL;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i_blob, &psz_hid, NULL)  );
                                SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvh_audits, psz_hid, (SG_int64) dagnum)  );
                            }
                        }
                    }
                    SG_VHASH_NULLFREE(pCtx, pvh_cs);
                }
                SG_VARRAY_NULLFREE(pCtx, pva_changesets);

                SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, offset_after_frag)  );
            }
            SG_DAGFRAG_NULLFREE(pCtx, pFrag);
		}
		else if (0 == strcmp(psz_op, "blob"))
		{
			SG_int64 i64 = 0;
			SG_uint64 len_encoded  = 0;
			SG_uint64 offset_begin_blob  = 0;

            SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_begin_blob)  );

			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_encoded", &i64)  );
			len_encoded = (SG_uint64) i64;

            // just seek the file past this blob
            SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, (offset_begin_blob + len_encoded))  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_FRAGBALL_INVALID_OP, (pCtx, "%s", psz_op)  );
		}
		SG_VHASH_NULLFREE(pCtx, pvh);
	}

    //SG_VHASH_STDERR(pvh_audits);

    *ppvh_omit = pvh_omit;
    pvh_omit = NULL;

    *ppvh_audits = pvh_audits;
    pvh_audits = NULL;

	/* fall through */

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
    SG_VHASH_NULLFREE(pCtx, pvh_cs);
	SG_VHASH_NULLFREE(pCtx, pvh_omit);
	SG_VHASH_NULLFREE(pCtx, pvh_audits);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
}

static void sg_clone__slurp(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_uint64 which_dagnum,
    SG_vhash** ppvh_changesets
	);

static void _commit__version_1(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_repo* pRepo
	)
{
	SG_vhash* pvh = NULL;
	const char* psz_op = NULL;
	SG_dagfrag* pFrag = NULL;
    SG_vhash* pvh_audit = NULL;
    sg_twobuffers* ptwo = NULL;
    SG_vhash* pvh_omit = NULL;
    SG_vhash* pvh_audits = NULL;
	SG_repo_tx_handle* ptx = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptwo)  );

    SG_ERR_CHECK(  sg_clone__slurp(pCtx, pfbi, 0, NULL)  );
    SG_ERR_CHECK(  _prescan__version_1(pCtx, pfbi, &pvh_omit, &pvh_audits)  );

    SG_ERR_CHECK(  SG_fragballinfo__seek_to_beginning(pCtx, pfbi)  );

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Copying data from v1 fragball")  );

    SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pRepo, &ptx)  );

	while (1)
	{
		SG_vhash* pvh_frag = NULL;
		SG_uint64 offset_begin_record = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_begin_record)  );
		SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfbi->file, &pvh)  );
		if (!pvh)
		{
			break;
		}
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &psz_op)  );
		if (0 == strcmp(psz_op, "frag"))
		{
            SG_uint64 dagnum = 0;

			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "frag", &pvh_frag)  );
			SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
            SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &dagnum)  );
            if (SG_DAGNUM__IS_OLDAUDIT(dagnum))
            {
                SG_DAGFRAG_NULLFREE(pCtx, pFrag);
            }
            else
            {
                SG_ERR_CHECK(  SG_repo__store_dagfrag(pCtx, pRepo, ptx, pFrag)  );
                pFrag = NULL;
            }
		}
		else if (0 == strcmp(psz_op, "blob"))
		{
			SG_int64 i64 = 0;
			const char* psz_hid = NULL;
			SG_blob_encoding encoding_CURRENT = 0;
			const char* psz_hid_vcdiff_reference = NULL;
			SG_uint64 len_full  = 0;
			SG_uint64 len_encoded  = 0;
            SG_bool b_omit = SG_FALSE;
			SG_uint64 offset_begin_blob  = 0;

            SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_begin_blob)  );

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "hid", &psz_hid)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "encoding", &i64)  );
			encoding_CURRENT = (SG_blob_encoding) i64;
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh, "vcdiff_ref", &psz_hid_vcdiff_reference)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_full", &i64)  );
			len_full = (SG_uint64) i64;
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_encoded", &i64)  );
			len_encoded = (SG_uint64) i64;

            if (SG_IS_BLOBENCODING_VCDIFF(encoding_CURRENT))
            {
                SG_ASSERT(psz_hid_vcdiff_reference);
            }
            else
            {
                SG_ASSERT(!psz_hid_vcdiff_reference);
                if (SG_IS_BLOBENCODING_FULL(encoding_CURRENT))
                {
                    SG_ASSERT(len_full == len_encoded);
                }
                else
                {
                    SG_ASSERT(SG_IS_BLOBENCODING_ZLIB(encoding_CURRENT));
                }
            }

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_omit, psz_hid, &b_omit)  );
            if (b_omit)
            {
                // just seek the file past this blob
                SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, (offset_begin_blob + len_encoded))  );
            }
            else
            {
                SG_bool b_audit = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_audits, psz_hid, &b_audit)  );

                if (b_audit)
                {
                    const char* psz_userid = NULL;
                    SG_int64 timestamp = -1;
                    const char* psz_csid = NULL;
                    SG_int64 dagnum = 0;

                    SG_ERR_CHECK(  SG_clone__get_blob_as_vhash(pCtx, pfbi, psz_hid, NULL, &pvh_audit)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_audit, "csid", &psz_csid)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_audit, SG_AUDIT__USERID, &psz_userid)  );
                    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_audit, SG_AUDIT__TIMESTAMP, &timestamp)  );

                    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_audits, psz_hid, &dagnum)  );

                    SG_ERR_CHECK(  SG_repo__store_audit(pCtx, pRepo, ptx, psz_csid, (SG_uint64) dagnum, psz_userid, timestamp)  );
                    SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, (offset_begin_blob + len_encoded))  );
                    SG_VHASH_NULLFREE(pCtx, pvh_audit);
                }
                else
                {
                    SG_ERR_CHECK(  _store_one_blob(
                                pCtx,
                                ptwo,
                                pRepo,
                                pfbi->file,
                                ptx,
                                psz_hid,
                                encoding_CURRENT,
                                psz_hid_vcdiff_reference,
                                len_full,
                                len_encoded)  );
                }
            }

#if defined(DEBUG)
            {
                SG_uint64 offset_end_blob  = 0;

                SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_end_blob)  );
                SG_ASSERT(offset_end_blob == (offset_begin_blob + len_encoded));
            }
#endif
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_FRAGBALL_INVALID_OP, (pCtx, "%s", psz_op)  );
		}
		SG_VHASH_NULLFREE(pCtx, pvh);
	}

    SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &ptx)  );

	/* fall through */

fail:
	if (ptx)
    {
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pRepo, &ptx)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh_audit);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
    SG_NULLFREE(pCtx, ptwo);
	SG_VHASH_NULLFREE(pCtx, pvh_omit);
	SG_VHASH_NULLFREE(pCtx, pvh_audits);
}

static void SG_blobndx__store_audits(
	SG_context* pCtx,
    SG_blobndx* pbs,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    SG_uint64 dagnum,
    SG_vhash* pvh_usermap,
    SG_vhash* pvh_map_csid
    )
{
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_bool b_tx = SG_FALSE;
    char bufTableName[BUF_LEN_TABLE_NAME];

    SG_ERR_CHECK(  _get_table_name(pCtx, DAG_AUDITS_TABLE_NAME, dagnum, bufTableName)  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    b_tx = SG_TRUE;
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pbs->psql, &pStmt, "SELECT csid, userid, timestamp FROM %s", bufTableName)  );
	while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
        const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
        const char* psz_userid = (const char*) sqlite3_column_text(pStmt, 1);
        SG_int64 timestamp = (SG_int64) sqlite3_column_int64(pStmt, 2);
        const char* psz_the_csid = NULL;
        const char* psz_the_userid = NULL;

        if (pvh_map_csid)
        {
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_map_csid, psz_csid, &psz_the_csid)  );
            if (!psz_the_csid)
            {
                psz_the_csid = psz_csid;
            }
        }
        else
        {
            psz_the_csid = psz_csid;
        }

        if (pvh_usermap)
        {
            if (0 == strcmp(psz_userid, SG_NOBODY__USERID))
            {
                psz_the_userid = SG_NOBODY__USERID;
            }
            else
            {
                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_usermap, psz_userid, &psz_the_userid)  );
                if (!psz_the_userid)
                {
                    psz_the_userid = SG_NOBODY__USERID;
                }
            }
        }
        else
        {
            psz_the_userid = psz_userid;
        }

        SG_ERR_CHECK(  SG_repo__store_audit(pCtx, pRepo, ptx, psz_the_csid, dagnum, psz_the_userid, timestamp)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pbs->psql, "COMMIT TRANSACTION")  );
    b_tx = SG_FALSE;

fail:
    if (b_tx)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pbs->psql, "ROLLBACK TRANSACTION")  );
    }
}

static void sg_clone__pack__vcdiff__one_object(
    SG_context* pCtx,
    SG_blobndx* pbs,
    const char * pszObjectID,
    SG_uint32 nKeyframeDensity,
    SG_uint32 nRevisionsToLeaveFull,
    SG_uint64 low_pass,
    SG_uint64 high_pass,
    SG_vhash* pvh_pack_specific
    )
{
	SG_vhash* pvh_revisions = NULL;
	SG_uint32 i_cur_revision = 0;
	SG_uint32 count_all_revs = 0;
	SG_uint32 count_net_revs = 0;
    char* flags = NULL;

    SG_ARGCHECK_RETURN(nKeyframeDensity > 0, nKeyframeDensity);

    SG_ERR_CHECK_RETURN(  SG_blobndx__list_blobs_for_objectid(pCtx, pbs, pszObjectID, &pvh_revisions)  );
    //fprintf(stderr, "\nblobs for objectid: %s\n", pszObjectID);
    //SG_VHASH_STDERR(pvh_revisions);
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_revisions, &count_all_revs)  );

    if (count_all_revs < 2)
    {
        goto done;
    }

    // mark the last N revisions ZLIB (usually 1)

    if (nRevisionsToLeaveFull > 0)
    {
        SG_uint32 i = 0;

        for (i=0; i<nRevisionsToLeaveFull; i++)
        {
            SG_vhash* pvh_rev = NULL;
            const char* psz_hid = NULL;
            SG_uint32 ndx = 0;
            SG_vhash* pvh_chg = NULL;

            if ((count_all_revs - i) == 0)
            {
                break;
            }

            ndx = count_all_revs - 1 - i;
            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_revisions, ndx, &psz_hid, &pvh_rev)  );

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_pack_specific, psz_hid, &pvh_chg)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(
                        pCtx, 
                        pvh_chg, 
                        SG_CLONE_CHANGES__ENCODING, 
                        (SG_int64) SG_BLOBENCODING__ZLIB
                        )  );
        }
    }

    if (count_all_revs <= nRevisionsToLeaveFull)
	{
		goto done; // early return
	}

    count_net_revs = count_all_revs - nRevisionsToLeaveFull;

    SG_ERR_CHECK(  SG_allocN(pCtx, count_net_revs, flags)  );

    // For pass one, find stuff that is already eligible to be
    // used as a reference blob.

	for (i_cur_revision = 0; i_cur_revision < count_net_revs; i_cur_revision++)
	{
        SG_int64 i64 = 0;
        SG_blob_encoding encoding = 0;
        SG_vhash* pvh_rev = NULL;
        const char* psz_hid = NULL;
        SG_uint64 len_full = 0;
        SG_int64 count_blobs_referencing_me = 0;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_revisions, i_cur_revision, &psz_hid, &pvh_rev)  );

		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_rev, "encoding", &i64)  );
        encoding = (SG_blob_encoding) i64;
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_rev, "len_full", &i64)  );
        len_full = (SG_uint64) len_full;

		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_rev, "count_blobs_referencing_me", &count_blobs_referencing_me)  );

		if (
                encoding == SG_BLOBENCODING__ALWAYSFULL
                )
		{
            // TODO shouldn't we check override here?
            flags[i_cur_revision] = SG_TRUE;
		}
        else if (
                (
                    low_pass
                    && (len_full < low_pass)
                )
                || (
                    high_pass 
                    && (len_full > high_pass)
                )
                )
        {
            // stuff that falls outside the low/high pass range is not
            // supposed to be vcdiffed

            flags[i_cur_revision] = SG_TRUE;
        }
	}

    // Now add keyframes according to the specified density.  Basically
    // we just walk the revisions and keep track of how many consecutive
    // non-keyframes we have seen.  When that number reaches the specified
    // density, we add a keyframe right there.

    {
        SG_uint32 count_non_keyframes_seen = 0;
        for (i_cur_revision=0; i_cur_revision < count_net_revs; i_cur_revision++)
        {
            if (flags[i_cur_revision])
            {
                count_non_keyframes_seen = 0;
            }
            else
            {
                count_non_keyframes_seen++;
            }

            if (
                    (count_non_keyframes_seen >= nKeyframeDensity)
                    || (i_cur_revision == (count_net_revs-1))
               )
            {
                SG_vhash* pvh_rev = NULL;
                SG_int64 i64 = -1;
                const char* psz_hid = NULL;
                SG_blob_encoding encoding = 0;

                flags[i_cur_revision] = SG_TRUE;
                count_non_keyframes_seen = 0;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_revisions, i_cur_revision, &psz_hid, &pvh_rev)  );

                SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_rev, "encoding", &i64)  );
                encoding = (SG_blob_encoding) i64;

                if (SG_IS_BLOBENCODING_VCDIFF(encoding))
                {
                    SG_vhash* pvh_chg = NULL;

                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_pack_specific, psz_hid, &pvh_chg)  );
                    SG_ERR_CHECK(  SG_vhash__add__int64(
                                pCtx, 
                                pvh_chg, 
                                SG_CLONE_CHANGES__ENCODING, 
                                (SG_int64) SG_BLOBENCODING__ZLIB
                                )  );
                }
            }
        }
    }

    // now set up the delta chains
	for (i_cur_revision = 0; i_cur_revision < count_net_revs; i_cur_revision++)
	{
        const char* psz_hid_ref = NULL;
        const char* psz_hid = NULL;
        SG_vhash* pvh_ref = NULL;

        if (flags[i_cur_revision])
        {
            // This is a keyframe.  Don't deltify it.
            continue;
        }

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_revisions, i_cur_revision, &psz_hid, NULL)  );

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_revisions, i_cur_revision + 1, &psz_hid_ref, &pvh_ref)  );

		if (psz_hid_ref)
		{
            SG_vhash* pvh_chg = NULL;
            SG_int64 ref_len_full = -1;

            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_ref, "len_full", &ref_len_full)  );
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_pack_specific, psz_hid, &pvh_chg)  );
            if (0 == ref_len_full)
            {
                SG_ERR_CHECK(  SG_vhash__add__int64(
                            pCtx, 
                            pvh_chg, 
                            SG_CLONE_CHANGES__ENCODING, 
                            (SG_int64) SG_BLOBENCODING__ZLIB
                            )  );
            }
            else
            {
                SG_ERR_CHECK(  SG_vhash__add__int64(
                            pCtx, 
                            pvh_chg, 
                            SG_CLONE_CHANGES__ENCODING, 
                            (SG_int64) SG_BLOBENCODING__VCDIFF
                            )  );

                SG_ERR_CHECK(  SG_vhash__add__string__sz(
                            pCtx,
                            pvh_chg,
                            SG_CLONE_CHANGES__HID_VCDIFF,
                            psz_hid_ref
                            )  );
            }
		}
    }

done:
fail:
    SG_NULLFREE(pCtx, flags);
	SG_VHASH_NULLFREE(pCtx, pvh_revisions);
}

static void sg_clone__store_unmarked_blobs__specific(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
	SG_repo_tx_handle* ptx,
    SG_tfc* ptfc,
    SG_vhash* pvh_pack_specific,
    struct SG_clone__demands* p_demands
	)
{
    SG_uint32 count_ops = 0;
    SG_uint32 count_blobs = 0;
    SG_uint32 count_sofar = 0;
    SG_uint32 demand_vcdiff_savings_over_full = SG_CLONE_DEFAULT__DEMAND_VCDIFF_SAVINGS_OVER_FULL;
    SG_uint32 demand_vcdiff_savings_over_zlib = SG_CLONE_DEFAULT__DEMAND_VCDIFF_SAVINGS_OVER_ZLIB;
    SG_uint32 demand_zlib_savings_over_full =   SG_CLONE_DEFAULT__DEMAND_ZLIB_SAVINGS_OVER_FULL;

    if (p_demands)
    {
        if (p_demands->vcdiff_savings_over_full >= 0)
        {
            demand_vcdiff_savings_over_full = p_demands->vcdiff_savings_over_full;
        }
        if (p_demands->vcdiff_savings_over_zlib >= 0)
        {
            demand_vcdiff_savings_over_zlib = p_demands->vcdiff_savings_over_zlib;
        }
        if (p_demands->zlib_savings_over_full >= 0)
        {
            demand_zlib_savings_over_full = p_demands->zlib_savings_over_full;
        }
    }

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__begin(pCtx, pfbi->pbs, &count_blobs)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_blobs, "blobs")  );

    while (1)
    {
        const char* psz_hid = NULL;
        SG_uint64 offset = 0;
        SG_blob_encoding encoding_CURRENT = 0;
        SG_blob_encoding encoding_DESIRED = 0;
        SG_uint64 len_encoded = 0;
        SG_uint64 len_full = 0;
        const char* psz_hid_vcdiff_reference = NULL;
        const char* psz_hid_vcdiff_reference_DESIRED = NULL;
        SG_bool b_omit = SG_FALSE;

        SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__next(
                    pCtx,
                    pfbi->pbs,
                    &psz_hid,
                    &offset,
                    &encoding_CURRENT,
                    &len_encoded,
                    &len_full,
                    &psz_hid_vcdiff_reference
                    )  );

        if (!psz_hid)
        {
            break;
        }

        SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, offset)  );

        if (pvh_pack_specific)
        {
            SG_vhash* pvh_chg = NULL;

            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_pack_specific, psz_hid, &pvh_chg)  );
            if (pvh_chg)
            {
                SG_bool b_has = SG_FALSE;
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_chg, SG_CLONE_CHANGES__OMIT, &b_has)  );
                if (b_has)
                {
                    SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_chg, SG_CLONE_CHANGES__OMIT, &b_omit)  );
                }

                if (!b_omit)
                {
                    SG_int64 i64 = 0;

                    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_chg, SG_CLONE_CHANGES__ENCODING, &i64)  );
                    encoding_DESIRED = (SG_blob_encoding)  i64;

                    if (SG_IS_BLOBENCODING_VCDIFF(encoding_DESIRED))
                    {
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_chg, SG_CLONE_CHANGES__HID_VCDIFF, &psz_hid_vcdiff_reference_DESIRED)  );
                    }
                }
            }
        }

        if (b_omit)
        {
        }
        else if (encoding_DESIRED)
        {
            if (SG_IS_BLOBENCODING_VCDIFF(encoding_DESIRED))
            {
                SG_ASSERT(psz_hid_vcdiff_reference_DESIRED);

                if (
                        SG_IS_BLOBENCODING_VCDIFF(encoding_CURRENT) 
                        && (0 == strcmp(psz_hid_vcdiff_reference, psz_hid_vcdiff_reference_DESIRED))
                   )
                {
                    // same.  just store it.
                    SG_ERR_CHECK(  _store_one_blob(
                                pCtx,
                                ptwo,
                                pRepo,
                                pfbi->file,
                                ptx,
                                psz_hid,
                                encoding_CURRENT,
                                psz_hid_vcdiff_reference,
                                len_full,
                                len_encoded)  );
                }
                else
                {
                    SG_bool b_vcdiff_result_was_ok = SG_FALSE;

                    SG_ERR_CHECK(  _convert_one_blob(
                                pCtx,
                                pfbi->path,
                                ptwo,
                                ptfc,
                                pfbi->pbs,
                                pRepo,
                                pfbi->file,
                                ptx,
                                psz_hid,
                                encoding_CURRENT,
                                psz_hid_vcdiff_reference,
                                len_full,
                                len_encoded,
                                encoding_DESIRED,
                                psz_hid_vcdiff_reference_DESIRED,
                                &b_vcdiff_result_was_ok,
                                demand_vcdiff_savings_over_full,
                                demand_vcdiff_savings_over_zlib,
                                demand_zlib_savings_over_full
                                )  );
                    if (
                            SG_IS_BLOBENCODING_VCDIFF(encoding_DESIRED)
                            && !b_vcdiff_result_was_ok
                            )
                    {
                        // TODO use this as a keyframe?
                    }
                }
            }
            else
            {
                SG_ASSERT(!psz_hid_vcdiff_reference_DESIRED);

                if (SG_ARE_BLOBENCODINGS_BASICALLY_EQUAL(encoding_DESIRED, encoding_CURRENT))
                {
                    SG_ERR_CHECK(  _store_one_blob(
                                pCtx,
                                ptwo,
                                pRepo,
                                pfbi->file,
                                ptx,
                                psz_hid,
                                encoding_DESIRED,
                                psz_hid_vcdiff_reference,
                                len_full,
                                len_encoded)  );
                }
                else
                {
                    SG_ERR_CHECK(  _convert_one_blob(
                                pCtx,
                                pfbi->path,
                                ptwo,
                                ptfc,
                                pfbi->pbs,
                                pRepo,
                                pfbi->file,
                                ptx,
                                psz_hid,
                                encoding_CURRENT,
                                psz_hid_vcdiff_reference,
                                len_full,
                                len_encoded,
                                encoding_DESIRED,
                                NULL,
                                NULL,
                                0,
                                0,
                                demand_zlib_savings_over_full
                                )  );
                }
            }
        }
        else
        {
            // no requests made.  just store this blob
            // the way we got it.

            SG_ERR_CHECK(  _store_one_blob(
                        pCtx,
                        ptwo,
                        pRepo,
                        pfbi->file,
                        ptx,
                        psz_hid,
                        encoding_CURRENT,
                        psz_hid_vcdiff_reference,
                        len_full,
                        len_encoded)  );
        }

        count_sofar++;
        if (
                (0 == (count_sofar % 10))
                || (count_sofar >= count_blobs)
           )
        {
            SG_ERR_CHECK(  SG_log__set_finished(pCtx, count_sofar)  );
        }
    }

    SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__finish(pCtx, pfbi->pbs)  );

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

	/* fall through */

fail:
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}

static void sg_clone__store_unmarked_blobs__ats(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
	SG_repo_tx_handle* ptx,
    SG_tfc* ptfc,
    struct SG_clone__params__all_to_something* p_params_ats,
    struct SG_clone__demands* p_demands
	)
{
    SG_uint32 count_ops = 0;
    SG_uint32 count_blobs = 0;
    SG_uint32 count_sofar = 0;
    SG_uint32 demand_zlib_savings_over_full =   SG_CLONE_DEFAULT__DEMAND_ZLIB_SAVINGS_OVER_FULL;

    if (p_demands)
    {
        if (p_demands->zlib_savings_over_full >= 0)
        {
            demand_zlib_savings_over_full = p_demands->zlib_savings_over_full;
        }
    }

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__begin(pCtx, pfbi->pbs, &count_blobs)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_blobs, "blobs")  );

    while (1)
    {
        const char* psz_hid = NULL;
        SG_uint64 offset = 0;
        SG_blob_encoding encoding_CURRENT = 0;
        SG_blob_encoding encoding_DESIRED = 0;
        SG_uint64 len_encoded = 0;
        SG_uint64 len_full = 0;
        const char* psz_hid_vcdiff_reference = NULL;

        SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__next(
                    pCtx,
                    pfbi->pbs,
                    &psz_hid,
                    &offset,
                    &encoding_CURRENT,
                    &len_encoded,
                    &len_full,
                    &psz_hid_vcdiff_reference
                    )  );

        if (!psz_hid)
        {
            break;
        }

        SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, offset)  );

        if (
                (SG_BLOBENCODING__ALWAYSFULL == encoding_CURRENT)
                && !(p_params_ats->flags & SG_CLONE_FLAG__OVERRIDE_ALWAYSFULL)
           )
        {
            // leave it alone
        }
        else
        {
            SG_bool b_do = SG_TRUE;

            if (
                    p_params_ats->low_pass
                    && (len_full < p_params_ats->low_pass)
               )
            {
                b_do = SG_FALSE;
            }
            else if (
                    p_params_ats->high_pass
                    && (len_full < p_params_ats->high_pass)
               )
            {
                b_do = SG_FALSE;
            }

            if (b_do)
            {
                encoding_DESIRED = p_params_ats->new_encoding;
            }
        }

        if (
            !encoding_DESIRED
            || (SG_ARE_BLOBENCODINGS_BASICALLY_EQUAL(encoding_DESIRED, encoding_CURRENT))
           )
        {
            SG_ERR_CHECK(  _store_one_blob(
                        pCtx,
                        ptwo,
                        pRepo,
                        pfbi->file,
                        ptx,
                        psz_hid,
                        encoding_CURRENT,
                        psz_hid_vcdiff_reference,
                        len_full,
                        len_encoded)  );
        }
        else
        {
            SG_ERR_CHECK(  _convert_one_blob(
                        pCtx,
                        pfbi->path,
                        ptwo,
                        ptfc,
                        pfbi->pbs,
                        pRepo,
                        pfbi->file,
                        ptx,
                        psz_hid,
                        encoding_CURRENT,
                        psz_hid_vcdiff_reference,
                        len_full,
                        len_encoded,
                        encoding_DESIRED,
                        NULL,
                        NULL,
                        0,
                        0,
                        demand_zlib_savings_over_full
                        )  );
        }

        count_sofar++;
        if (
                (0 == (count_sofar % 10))
                || (count_sofar >= count_blobs)
           )
        {
            SG_ERR_CHECK(  SG_log__set_finished(pCtx, count_sofar)  );
        }
    }

    SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__finish(pCtx, pfbi->pbs)  );

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

	/* fall through */

fail:
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}

static void sg_clone__store_unmarked_blobs__exact(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
	SG_repo_tx_handle* ptx
	)
{
    SG_uint32 count_ops = 0;
    SG_uint32 count_blobs = 0;
    SG_uint32 count_sofar = 0;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__begin(pCtx, pfbi->pbs, &count_blobs)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_blobs, "blobs")  );

    while (1)
    {
        const char* psz_hid = NULL;
        SG_uint64 offset = 0;
        SG_blob_encoding blob_encoding = 0;
        SG_uint64 len_encoded = 0;
        SG_uint64 len_full = 0;
        const char* psz_hid_vcdiff_reference = NULL;

        SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__next(
                    pCtx,
                    pfbi->pbs,
                    &psz_hid,
                    &offset,
                    &blob_encoding,
                    &len_encoded,
                    &len_full,
                    &psz_hid_vcdiff_reference
                    )  );

        if (!psz_hid)
        {
            break;
        }

        SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, offset)  );
        SG_ERR_CHECK(  _store_one_blob(
                    pCtx,
                    ptwo,
                    pRepo,
                    pfbi->file,
                    ptx,
                    psz_hid,
                    blob_encoding,
                    psz_hid_vcdiff_reference,
                    len_full,
                    len_encoded)  );

        count_sofar++;
        if (
                (0 == (count_sofar % 10))
                || (count_sofar >= count_blobs)
           )
        {
            SG_ERR_CHECK(  SG_log__set_finished(pCtx, count_sofar)  );
        }
    }

    SG_ERR_CHECK(  SG_blobndx__list_unmarked_blobs__finish(pCtx, pfbi->pbs)  );

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

	/* fall through */

fail:
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}

static void _slurp__version_3(
	SG_context* pCtx,
    SG_fragballinfo* pfbi
	)
{
	SG_vhash* pvh = NULL;
    SG_pathname* pPath_bindex = NULL;
    SG_file* pFile_bindex = NULL;
    char buf_tid[SG_TID_MAX_BUFFER_LENGTH];

	SG_NULLARGCHECK_RETURN(pfbi);

	while (1)
	{
		SG_uint64 offset_begin_record = 0;
        SG_uint16 type = 0;
        SG_uint16 flags = 0;
        SG_uint32 len_json = 0;
        SG_uint32 len_zlib_json = 0;
        SG_uint64 len_payload = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pfbi->file, &offset_begin_record)  );
		SG_ERR_CHECK(  SG_fragball__v3__read_object_header(pCtx, pfbi->file, &type, &flags, &len_json, &len_zlib_json, &len_payload)  );
		if (!type)
		{
			break;
		}
        if (SG_FRAGBALL_V3_TYPE__BINDEX == type)
		{
            // only one bindex
            if (pPath_bindex)
            {
                SG_ERR_THROW(  SG_ERR_UNSPECIFIED  ); // TODO
            }

            // the json is unused.  just skip it.
			SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, offset_begin_record + 20 + len_zlib_json)  );

            SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_bindex, pfbi->path)  );
            SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPath_bindex)  );
            SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_tid, sizeof(buf_tid))  );
            SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath_bindex, buf_tid)  );

            SG_ERR_CHECK(  SG_zlib__inflate__file__already_open(pCtx, pfbi->file, len_payload, pPath_bindex)  );
            SG_ERR_CHECK(  SG_file__close(pCtx, &pFile_bindex)  );
		}
        else
        {
			SG_ERR_CHECK(  SG_file__seek(pCtx, pfbi->file, offset_begin_record + 20 + len_zlib_json + len_payload)  );
        }
	}

    SG_ERR_CHECK(  SG_blobndx__create__with_sqlite_file(pCtx, &pPath_bindex, &pfbi->pbs)  );
    SG_ERR_CHECK(  SG_blobndx__list_dags(pCtx, pfbi->pbs, &pfbi->pvh_dags)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_bindex);
    SG_FILE_NULLCLOSE(pCtx, pFile_bindex);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

static void sg_v3_slurp(
	SG_context* pCtx,
    SG_fragballinfo* pfbi
	)
{
	SG_NULLARGCHECK_RETURN(pfbi);

    //fprintf(stderr, "length: %d\n", (int) pfbi->length);

    SG_ERR_CHECK(  SG_fragballinfo__seek_to_beginning(pCtx, pfbi)  );

    SG_ERR_CHECK(  _slurp__version_3(pCtx, pfbi)  );

fail:
    ;
}

static void sg_clone__make_dagnode_from_changeset(
	SG_context* pCtx,
    const char* psz_csid,
    SG_vhash* pvh_cs,
    SG_dagnode** pp
    )
{
    SG_int32 gen = -1;
    SG_dagnode* pdn = NULL;

    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_cs, "generation", &gen)  );
    SG_ERR_CHECK(  SG_dagnode__alloc(pCtx, &pdn, psz_csid, gen, 0)  );
    {
        SG_varray* pva_parents = NULL;

        SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvh_cs, "parents", &pva_parents)  );
        if (pva_parents)
        {
            SG_uint32 count_parents = 0;

            SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count_parents)  );

            if (0 == count_parents)
            {
                // nothing
            }
            else if (1 == count_parents)
            {
                const char* psz_parent = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, 0, &psz_parent)  );
                SG_ERR_CHECK(  SG_dagnode__set_parent(pCtx, pdn, psz_parent)  );
            }
            else if (2 == count_parents)
            {
                const char* psz_parent0 = NULL;
                const char* psz_parent1 = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, 0, &psz_parent0)  );
                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, 1, &psz_parent1)  );
                SG_ERR_CHECK(  SG_dagnode__set_parents__2(pCtx, pdn, psz_parent0, psz_parent1)  );
            }
            else
            {
                SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);
            }
        }
    }

    *pp = pdn;
    pdn = NULL;

fail:
    SG_DAGNODE_NULLFREE(pCtx, pdn);
}

void sg_clone__find_all_blobs_in_db_dag(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_uint64 dagnum,
    SG_ihash** ppih_blobs_in_dag
    )
{
    SG_vhash* pvh_cs = NULL;
    SG_uint32 i_changeset = 0;
    SG_ihash* pih_blobs_in_dag = NULL;
    char* psz_json = NULL;
    SG_varray* pva_templates = NULL;
    SG_varray* pva_changesets = NULL;
    SG_uint32 count_changesets = 0;

    SG_ERR_CHECK(  SG_blobndx__list_changesets(pCtx, pfbi->pbs, dagnum, &pva_changesets)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_changesets, &count_changesets)  );

    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pih_blobs_in_dag)  );

    SG_ERR_CHECK(  SG_blobndx__list_templates(pCtx, pfbi->pbs, dagnum, &pva_templates)  );
    if (pva_templates)
    {
        SG_uint32 count_templates = 0;
        SG_uint32 i_template = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_templates, &count_templates)  );
        for (i_template=0; i_template<count_templates; i_template++)
        {
            const char* psz_hid_template = NULL;
            struct found_blob fb;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_templates, i_template, &psz_hid_template)  );

            memset(&fb, 0, sizeof(fb));
            SG_ERR_CHECK(  SG_clone__find_blob(pCtx, pfbi->pbs, psz_hid_template, &fb)  );
            SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_hid_template, fb.rowid)  );
        }
        SG_VARRAY_NULLFREE(pCtx, pva_templates);
    }
    else
    {
        // assert hardwired dagnum
    }

    for (i_changeset=0; i_changeset<count_changesets; i_changeset++)
    {
        const char* psz_csid = NULL;
        SG_uint32 len_full = 0;
        SG_int64 rowid_cs = 0;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_changesets, i_changeset, &psz_csid)  );

        SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                    pCtx,
                    pfbi,
                    psz_csid,
                    &rowid_cs,
                    &psz_json,
                    &len_full
                    )  );
        SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh_cs, psz_json));
        SG_NULLFREE(pCtx, psz_json);
        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_csid, rowid_cs)  );

#if 0
        // could assert the dagnums match
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_cs, "dagnum", &psz_dagnum)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
#endif

        if (SG_DAGNUM__IS_DB(dagnum))
        {
            SG_vhash* pvh_db = NULL;
            SG_vhash* pvh_changes = NULL;

            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_cs, "db", &pvh_db)  );
            if (pvh_db)
            {
                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_db, "changes", &pvh_changes)  );
                if (pvh_changes)
                {
                    SG_uint32 count_parents = 0;
                    SG_uint32 i_parent = 0;
                    
                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
                    for (i_parent=0; i_parent<count_parents; i_parent++)
                    {
                        const char* psz_csid_parent = NULL;
                        SG_vhash* pvh_changes_one_parent = NULL;
                        SG_vhash* pvh_add = NULL;
                        SG_vhash* pvh_attach = NULL;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_changes_one_parent)  );
                        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_add)  );
                        if (pvh_add)
                        {
                            SG_uint32 count_records = 0;
                            SG_uint32 i_record = 0;

                            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count_records)  );
                            for (i_record=0; i_record<count_records; i_record++)
                            {
                                const char* psz_hidrec = NULL;
                                SG_bool b_already = SG_FALSE;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i_record, &psz_hidrec, NULL)  );
                                SG_ERR_CHECK(  SG_ihash__has(pCtx, pih_blobs_in_dag, psz_hidrec, &b_already)  );
                                if (!b_already)
                                {
                                    struct found_blob fb;

                                    memset(&fb, 0, sizeof(fb));
                                    SG_ERR_CHECK(  SG_clone__find_blob(pCtx, pfbi->pbs, psz_hidrec, &fb)  );
                                    SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_hidrec, fb.rowid)  );
                                }
                            }
                        }

                        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "attach_add", &pvh_attach)  );
                        if (pvh_attach)
                        {
                            SG_uint32 count = 0;
                            SG_uint32 i = 0;

                            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_attach, &count)  );
                            for (i=0; i<count; i++)
                            {
                                const char* psz_hid = NULL;
                                SG_bool b_already = SG_FALSE;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_attach, i, &psz_hid, NULL)  );

                                SG_ERR_CHECK(  SG_ihash__has(pCtx, pih_blobs_in_dag, psz_hid, &b_already)  );
                                if (!b_already)
                                {
                                    struct found_blob fb;

                                    memset(&fb, 0, sizeof(fb));
                                    SG_ERR_CHECK(  SG_clone__find_blob(pCtx, pfbi->pbs, psz_hid, &fb)  );
                                    SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_hid, fb.rowid)  );
                                }
                            }
                        }
                    }
                }
            }
        }

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

        SG_VHASH_NULLFREE(pCtx, pvh_cs);
    }

    *ppih_blobs_in_dag = pih_blobs_in_dag;
    pih_blobs_in_dag = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
    SG_NULLFREE(pCtx, psz_json);
    SG_VHASH_NULLFREE(pCtx, pvh_cs);
    SG_IHASH_NULLFREE(pCtx, pih_blobs_in_dag);
}

static void sg_clone__store_changesets__one_dag(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    SG_uint64 dagnum,
    SG_vhash* pvh_usermap,
    SG_uint32* pi_progress,
    SG_uint32 progress_total,
    SG_ihash** ppih_blobs_in_dag
    )
{
    const char* psz_repo_id = NULL;
    const char* psz_admin_id = NULL;
    SG_dagfrag* pFrag = NULL;
    SG_vhash* pvh_cs = NULL;
    SG_uint32 i_changeset = 0;
    SG_dagnode* pdn = NULL;
    SG_ihash* pih_blobs_in_dag = NULL;
    char* psz_json = NULL;
    SG_varray* pva_templates = NULL;
    SG_varray* pva_changesets = NULL;
    SG_uint32 count_changesets = 0;
    SG_bool b_tx = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pfbi->pvh_header, SG_SYNC_REPO_INFO_KEY__REPO_ID, &psz_repo_id)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pfbi->pvh_header, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &psz_admin_id)  );

    SG_ERR_CHECK(  SG_blobndx__list_changesets(pCtx, pfbi->pbs, dagnum, &pva_changesets)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_changesets, &count_changesets)  );

    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pih_blobs_in_dag)  );

    SG_ERR_CHECK(  SG_blobndx__list_templates(pCtx, pfbi->pbs, dagnum, &pva_templates)  );
    if (pva_templates)
    {
        SG_uint32 count_templates = 0;
        SG_uint32 i_template = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_templates, &count_templates)  );
        for (i_template=0; i_template<count_templates; i_template++)
        {
            const char* psz_hid_template = NULL;
            SG_uint32 len_full = 0;
            SG_int64 rowid = 0;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_templates, i_template, &psz_hid_template)  );

            SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                        pCtx,
                        pfbi,
                        psz_hid_template,
                        &rowid,
                        &psz_json,
                        &len_full
                        )  );
            SG_ERR_CHECK(  SG_repo__store_blob__dbtemplate(
                        pCtx,
                        pRepo,
                        ptx,
                        psz_json,
                        len_full,
                        dagnum,
                        psz_hid_template,
                        NULL,
                        NULL
                        )  );
            SG_NULLFREE(pCtx, psz_json);
            SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_hid_template, rowid)  );
        }
        SG_VARRAY_NULLFREE(pCtx, pva_templates);
    }
    else
    {
        // assert hardwired dagnum
    }

    SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx,&pFrag,psz_repo_id,psz_admin_id,dagnum)  );

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pfbi->pbs->psql, "BEGIN IMMEDIATE TRANSACTION")  );
    b_tx = SG_TRUE;
    for (i_changeset=0; i_changeset<count_changesets; i_changeset++)
    {
        const char* psz_csid = NULL;
        SG_uint32 len_full = 0;
        SG_int32 gen = -1;
        SG_int64 rowid_cs = 0;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_changesets, i_changeset, &psz_csid)  );

        SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                    pCtx,
                    pfbi,
                    psz_csid,
                    &rowid_cs,
                    &psz_json,
                    &len_full
                    )  );
        SG_ERR_CHECK(  SG_repo__store_blob__changeset(
                    pCtx,
                    pRepo,
                    ptx,
                    psz_json,
                    len_full,
                    psz_csid,
                    &pvh_cs,
                    NULL
                    )  );
        SG_NULLFREE(pCtx, psz_json);
        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_csid, rowid_cs)  );

        SG_ERR_CHECK(  sg_clone__make_dagnode_from_changeset(pCtx, psz_csid, pvh_cs, &pdn)  );
        SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
        SG_ERR_CHECK(  SG_dagfrag__add_dagnode(pCtx, pFrag, &pdn)  );

#if 0
        // could assert the dagnums match
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_cs, "dagnum", &psz_dagnum)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
#endif

        if (SG_DAGNUM__IS_DB(dagnum))
        {
            SG_vhash* pvh_db = NULL;
            SG_vhash* pvh_changes = NULL;

            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_cs, "db", &pvh_db)  );
            if (pvh_db)
            {
                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_db, "changes", &pvh_changes)  );
                if (pvh_changes)
                {
                    SG_uint32 count_parents = 0;
                    SG_uint32 i_parent = 0;
                    
                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
                    for (i_parent=0; i_parent<count_parents; i_parent++)
                    {
                        const char* psz_csid_parent = NULL;
                        SG_vhash* pvh_changes_one_parent = NULL;
                        SG_vhash* pvh_add = NULL;
                        SG_vhash* pvh_attach = NULL;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_changes_one_parent)  );
                        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_add)  );
                        if (pvh_add)
                        {
                            SG_uint32 count_records = 0;
                            SG_uint32 i_record = 0;

                            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count_records)  );
                            for (i_record=0; i_record<count_records; i_record++)
                            {
                                const char* psz_hidrec = NULL;
                                SG_bool b_already = SG_FALSE;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i_record, &psz_hidrec, NULL)  );
                                SG_ERR_CHECK(  SG_ihash__has(pCtx, pih_blobs_in_dag, psz_hidrec, &b_already)  );
                                if (!b_already)
                                {
                                    SG_int64 rowid = 0;

                                    SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                                                pCtx,
                                                pfbi,
                                                psz_hidrec,
                                                &rowid,
                                                &psz_json,
                                                &len_full
                                                )  );
                                    SG_ERR_CHECK(  SG_repo__store_blob__dbrecord(
                                                pCtx,
                                                pRepo,
                                                ptx,
                                                psz_json,
                                                len_full,
                                                dagnum,
                                                psz_hidrec,
                                                NULL,
                                                NULL
                                                )  );
                                    SG_NULLFREE(pCtx, psz_json);
                                    SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_hidrec, rowid)  );
                                }
                            }
                        }

                        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "attach_add", &pvh_attach)  );
                        if (pvh_attach)
                        {
                            SG_uint32 count = 0;
                            SG_uint32 i = 0;

                            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_attach, &count)  );
                            for (i=0; i<count; i++)
                            {
                                const char* psz_hid = NULL;
                                SG_bool b_already = SG_FALSE;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_attach, i, &psz_hid, NULL)  );

                                SG_ERR_CHECK(  SG_ihash__has(pCtx, pih_blobs_in_dag, psz_hid, &b_already)  );
                                if (!b_already)
                                {
                                    SG_int64 rowid = 0;

                                    SG_ERR_CHECK(  sg_clone__copy_one_blob(
                                                pCtx,
                                                pfbi,
                                                ptwo,
                                                pRepo,
                                                ptx,
                                                psz_hid,
                                                &rowid
                                                )  );
                                    SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_blobs_in_dag, psz_hid, rowid)  );
                                }
                            }
                        }
                    }
                }
            }
        }

        (*pi_progress)++;
        if (
                (0 == ((*pi_progress) % 10))
                || (*pi_progress >= progress_total)
           )
        {
            SG_ERR_CHECK(  SG_log__set_finished(pCtx, (*pi_progress))  );
        }

        SG_VHASH_NULLFREE(pCtx, pvh_cs);
    }
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pfbi->pbs->psql, "COMMIT TRANSACTION")  );
    b_tx = SG_FALSE;

    SG_ERR_CHECK(  SG_repo__store_dagfrag(pCtx, pRepo, ptx, pFrag)  );
    pFrag = NULL;
    SG_DAGFRAG_NULLFREE(pCtx, pFrag);

    SG_ERR_CHECK(  SG_blobndx__store_audits(pCtx, pfbi->pbs, pRepo, ptx, dagnum, pvh_usermap, NULL)  );

    SG_ERR_CHECK(  SG_repo__done_with_dag(pCtx, pRepo, ptx, dagnum)  );

    *ppih_blobs_in_dag = pih_blobs_in_dag;
    pih_blobs_in_dag = NULL;

fail:
    if (b_tx)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pfbi->pbs->psql, "ROLLBACK TRANSACTION")  );
    }
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
    SG_DAGFRAG_NULLFREE(pCtx, pFrag);
    SG_NULLFREE(pCtx, psz_json);
    SG_VHASH_NULLFREE(pCtx, pvh_cs);
    SG_IHASH_NULLFREE(pCtx, pih_blobs_in_dag);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
}

static void sg_clone__store_changesets(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    SG_vhash* pvh_dagnums_except,
    SG_vhash* pvh_usermap
    )
{
    SG_uint32 count_dagnums = 0;
    SG_uint32 total_count_changesets = 0;
    SG_uint32 i_dagnum = 0;
    SG_uint32 count_ops = 0;
    SG_ihash* pih_blobs_in_dag = NULL;
    SG_uint32 i_progress = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pfbi->pvh_dags, &count_dagnums)  );

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    count_ops++;

    for (i_dagnum=0; i_dagnum<count_dagnums; i_dagnum++)
    {
        const char* psz_dagnum = NULL;
        SG_vhash* pvh_daginfo = NULL;
        SG_uint32 count_changesets = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pfbi->pvh_dags, i_dagnum, &psz_dagnum, &pvh_daginfo)  );

        // for the purpose of logging progress, we could all changesets, whether they are
        // excluded or not, because we have to walk them all anyway
        SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_daginfo, "count_changesets", &count_changesets)  );
        total_count_changesets += count_changesets;
    }
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, total_count_changesets, "changesets")  );

    i_progress = 0;
    for (i_dagnum=0; i_dagnum<count_dagnums; i_dagnum++)
    {
        const char* psz_dagnum = NULL;
        SG_uint64 dagnum = 0;
        SG_bool b_include_this_dag = SG_TRUE;
        SG_vhash* pvh_daginfo = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pfbi->pvh_dags, i_dagnum, &psz_dagnum, &pvh_daginfo)  );

        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

        if (pvh_dagnums_except)
        {
            SG_bool b_out = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_dagnums_except, psz_dagnum, &b_out)  );
            b_include_this_dag = !b_out;
        }
        
        if (b_include_this_dag)
        {
            SG_ERR_CHECK(  sg_clone__store_changesets__one_dag(
                        pCtx,
                        pfbi,
                        ptwo,
                        pRepo,
                        ptx,
                        dagnum,
                        pvh_usermap,
                        &i_progress,
                        total_count_changesets,
                        &pih_blobs_in_dag
                        )  );
            SG_ERR_CHECK(  SG_blobndx__mark_blobs(pCtx, pfbi->pbs, pih_blobs_in_dag)  );
        }
        else
        {
            if (SG_DAGNUM__IS_DB(dagnum))
            {
                SG_ERR_CHECK(  sg_clone__find_all_blobs_in_db_dag(
                            pCtx,
                            pfbi,
                            dagnum,
                            &pih_blobs_in_dag
                            )  );
            }
            else
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            SG_ERR_CHECK(  SG_blobndx__mark_blobs(pCtx, pfbi->pbs, pih_blobs_in_dag)  );
        }
        SG_IHASH_NULLFREE(pCtx, pih_blobs_in_dag);
    }

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

fail:
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
    SG_IHASH_NULLFREE(pCtx, pih_blobs_in_dag);
}

static void sg_clone__store_fragball__only_the_users_dag(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_repo* pRepo
	)
{
    sg_twobuffers* ptwo = NULL;
    SG_repo_tx_handle* ptx = NULL;
    SG_ihash* pih_blobs_stored = NULL;
    SG_uint32 count_ops = 0;
    SG_vhash* pvh_daginfo = NULL;
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
    SG_uint32 count_changesets = 0;
    SG_uint32 total_count_changesets = 0;
    SG_uint32 i_progress = 0;

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__USERS, buf_dagnum, sizeof(buf_dagnum))  );

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptwo)  );

    SG_ERR_CHECK(  SG_repo__begin_tx__flags(pCtx, pRepo, SG_REPO_TX_FLAG__CLONING, &ptx)  );

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pfbi->pvh_dags, buf_dagnum, &pvh_daginfo)  );

    SG_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvh_daginfo, "count_changesets", &count_changesets)  );
    total_count_changesets += count_changesets;
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, total_count_changesets, "changesets")  );

    i_progress = 0;
    SG_ERR_CHECK(  sg_clone__store_changesets__one_dag(
                pCtx, 
                pfbi,
                ptwo,
                pRepo,
                ptx,
                SG_DAGNUM__USERS,
                NULL,
                &i_progress,
                total_count_changesets,
                &pih_blobs_stored
                )  );
    SG_IHASH_NULLFREE(pCtx, pih_blobs_stored);
    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

    SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &ptx)  );

	/* fall through */

fail:
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
	if (ptx)
    {
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pRepo, &ptx)  );
    }
    SG_NULLFREE(pCtx, ptwo);
}

static void sg_clone__calculate_pack(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_tfc* ptfc,
    sg_twobuffers* ptwo,
    struct SG_clone__params__pack* p_params_pack,
    SG_varray* pva_vc_changesets,
    SG_vhash** ppvh_specific
    )
{
    SG_varray * pva_candidates = NULL;
    SG_uint32 count_candidates = 0;
    SG_uint32 i = 0;
    SG_vhash* pvh_pack_specific = NULL;

    SG_ERR_CHECK(  _pack__identify_vc_blobs(pCtx, pfbi, ptwo, ptfc, pva_vc_changesets)  );

    SG_ERR_CHECK(  SG_blobndx__list_pack_candidates(pCtx, pfbi->pbs, p_params_pack->nMinRevisions, &pva_candidates)  );

    //fprintf(stderr, "\ncandidates:\n");
    //SG_VARRAY_STDERR(pva_candidates);

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_pack_specific)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_candidates, &count_candidates)  );
    for (i=0; i < count_candidates; i++)
    {
        SG_vhash* pvh_one_candidate = NULL;
        const char* pszgid = NULL;
        SG_int64 nRevisionCount = 0;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_candidates, i, &pvh_one_candidate)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_one_candidate, "objectid", &pszgid)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_one_candidate, "revision_count", &nRevisionCount)  );

        if (nRevisionCount  < p_params_pack->nMinRevisions)
        {
            continue;
        }

        SG_ERR_CHECK( sg_clone__pack__vcdiff__one_object(pCtx, pfbi->pbs, pszgid, p_params_pack->nKeyframeDensity, p_params_pack->nRevisionsToLeaveFull, p_params_pack->low_pass, p_params_pack->high_pass, pvh_pack_specific)  );
    }

    *ppvh_specific = pvh_pack_specific;
    pvh_pack_specific = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_candidates);
    SG_VHASH_NULLFREE(pCtx, pvh_pack_specific);
}

static void sg_clone__store_fragball(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_repo* pRepo,
    SG_vhash* pvh_dagnums_except,
    SG_vhash* pvh_usermap,
    struct SG_clone__params__all_to_something* p_params_ats,
    struct SG_clone__params__pack* p_params_pack,
    struct SG_clone__demands* p_demands
	)
{
    sg_twobuffers* ptwo = NULL;
    SG_repo_tx_handle* ptx = NULL;
    SG_tfc* ptfc = NULL;
    SG_vhash* pvh_pack_specific = NULL;
    SG_varray* pva_vc_changesets = NULL;

    if (p_params_ats && p_params_pack)
    {
		SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "not both") );
    }

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptwo)  );

    SG_ERR_CHECK(  SG_repo__begin_tx__flags(pCtx, pRepo, SG_REPO_TX_FLAG__CLONING, &ptx)  );

    SG_ERR_CHECK(  sg_clone__store_changesets(
                pCtx, 
                pfbi,
                ptwo,
                pRepo,
                ptx,
                pvh_dagnums_except,
                pvh_usermap
                )  );

    if (p_params_ats || p_params_pack)
    {
        SG_ERR_CHECK(  SG_fragballinfo__make_tfc(pCtx, pfbi, &ptfc)  );
    }

    if (p_params_pack)
    {
        SG_ERR_CHECK(  SG_blobndx__list_changesets(pCtx, pfbi->pbs, SG_DAGNUM__VERSION_CONTROL, &pva_vc_changesets)  );
        SG_ERR_CHECK(  sg_clone__calculate_pack(
                    pCtx,
                    pfbi,
                    ptfc,
                    ptwo,
                    p_params_pack,
                    pva_vc_changesets,
                    &pvh_pack_specific
                    )  );

        //fprintf(stderr, "\npack_specific:\n");
        //SG_VHASH_STDERR(pvh_pack_specific);

        SG_ERR_CHECK(  sg_clone__store_unmarked_blobs__specific(
                    pCtx, 
                    pfbi, 
                    ptwo, 
                    pRepo, 
                    ptx,
                    ptfc,
                    pvh_pack_specific,
                    p_demands
                    )  );
    }
    else if (p_params_ats)
    {
        SG_ERR_CHECK(  sg_clone__store_unmarked_blobs__ats(
                    pCtx, 
                    pfbi, 
                    ptwo, 
                    pRepo, 
                    ptx,
                    ptfc,
                    p_params_ats,
                    p_demands
                    )  );
    }
    else
    {
        SG_ERR_CHECK(  sg_clone__store_unmarked_blobs__exact(
                    pCtx, 
                    pfbi, 
                    ptwo, 
                    pRepo, 
                    ptx
                    )  );
    }

    SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &ptx)  );

	/* fall through */

fail:
    SG_tfc__free(pCtx, ptfc);
	if (ptx)
    {
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pRepo, &ptx)  );
    }
    SG_VARRAY_NULLFREE(pCtx, pva_vc_changesets);
    SG_NULLFREE(pCtx, ptwo);
    SG_VHASH_NULLFREE(pCtx, pvh_pack_specific);
}

static void sg_clone__check_one_template_for_userid(
	SG_context* pCtx,
    const char* psz_json,
    SG_bool* pb
    )
{
    SG_NULLARGCHECK_RETURN(psz_json);
    SG_NULLARGCHECK_RETURN(pb);

    // TODO er, strstr probably isn't the best way to do this
    if (strstr(psz_json, "userid"))
    {
        *pb = SG_TRUE;
    }
    else
    {
        *pb = SG_FALSE;
    }
}

static void sg_clone__find_db_dags_with_userid_fields(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_vhash** ppvh
    )
{
    SG_uint32 count_dagnums = 0;
    SG_uint32 i_dagnum = 0;
    SG_vhash* pvh_result = NULL;
    char* psz_json = NULL;
    SG_string* pstr = NULL;
    SG_varray* pva_templates = NULL;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pfbi->pvh_dags, &count_dagnums)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_result)  );
    for (i_dagnum=0; i_dagnum<count_dagnums; i_dagnum++)
    {
        const char* psz_dagnum = NULL;
        SG_uint64 dagnum = 0;
        SG_vhash* pvh_daginfo = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pfbi->pvh_dags, i_dagnum, &psz_dagnum, &pvh_daginfo)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

        if (
                (SG_DAGNUM__USERS != dagnum)
                && (SG_DAGNUM__IS_DB(dagnum))
           )
        {
            SG_bool b_yes = SG_FALSE;

            SG_ERR_CHECK(  SG_blobndx__list_templates(pCtx, pfbi->pbs, dagnum, &pva_templates)  );
            if (pva_templates)
            {
                SG_uint32 count_templates = 0;
                SG_uint32 i_template = 0;

                SG_ERR_CHECK(  SG_varray__count(pCtx, pva_templates, &count_templates)  );
                for (i_template=0; i_template<count_templates; i_template++)
                {
                    const char* psz_hid_template = NULL;

                    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_templates, i_template, &psz_hid_template)  );

                    SG_ERR_CHECK(  SG_clone__get_blob_full__mem(
                                pCtx,
                                pfbi,
                                psz_hid_template,
                                NULL,
                                &psz_json,
                                NULL
                                )  );

                    SG_ERR_CHECK(  sg_clone__check_one_template_for_userid(
                                pCtx,
                                psz_json,
                                &b_yes
                                )  );
                    SG_NULLFREE(pCtx, psz_json);
                    if (b_yes)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_result, psz_dagnum)  );
                        break;
                    }
                }
                SG_VARRAY_NULLFREE(pCtx, pva_templates);
            }
            else
            {
                SG_zing__fetch_static_template__json(pCtx, dagnum, &pstr);
                if (SG_context__has_err(pCtx) && SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
                {
                    SG_context__err_reset(pCtx);
                }
                else
                {
                    SG_ERR_CHECK_CURRENT;
                    SG_ERR_CHECK(  sg_clone__check_one_template_for_userid(
                                pCtx,
                                SG_string__sz(pstr),
                                &b_yes
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr);
                    if (b_yes)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_result, psz_dagnum)  );
                    }
                }
            }
        }
    }

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_NULLFREE(pCtx, psz_json);
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
}

static void sg_clone__usermap__fix_one_record(
	SG_context* pCtx,
    SG_vhash* pvh_usermap,
    SG_vhash* pvh_rec
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    // TODO maybe constrain this to fields which are actually of type userid?

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rec, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_name = NULL;
        const char* psz_value = NULL;
        const char* psz_new_userid = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_rec, i, &psz_name, &psz_value)  );
        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_usermap, psz_value, &psz_new_userid)  );
        if (psz_new_userid)
        {
            SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_rec, psz_name, psz_new_userid)  );
        }
    }

fail:
    ;
}

static void sg_clone__store_vhash(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    SG_vhash* pvh,
    char** ppsz_hid
    )
{
    SG_string* pstr = NULL;
    char* psz_hid = NULL;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
	SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh, SG_TRUE, SG_vhash_sort_callback__increasing)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );

    SG_ERR_CHECK(  SG_repo__store_blob_from_memory(
            pCtx,
            pRepo,
            ptx,
            SG_FALSE, // alwaysfull
            (const SG_byte *)SG_string__sz(pstr),
            SG_string__length_in_bytes(pstr),
            &psz_hid)  );

    *ppsz_hid = psz_hid;
    psz_hid = NULL;

fail:
    SG_NULLFREE(pCtx, psz_hid);
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_clone__usermap__rewrite_one_changeset(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    const char* psz_csid,
    SG_vhash* pvh_cs,
    SG_vhash* pvh_usermap,
    SG_vhash* pvh_map_csid,
    SG_vhash* pvh_map_hidrec,
    SG_vhash** ppvh_new_cs
    )
{
    SG_int32 gen = -1;
    SG_vhash* pvh_db = NULL;
    SG_vhash* pvh_changes = NULL;
    SG_varray* pva_parents = NULL;
    SG_vhash* pvh_rec = NULL;
    SG_string* pstr_rec = NULL;
    char* psz_new_hidrec_freeme = NULL;
    char* psz_new_csid = NULL;
    SG_vhash* pvh_new_cs = NULL;
    const char* psz_dagnum = NULL;
    SG_int64 ver = -1;

#if 0
    fprintf(stderr, "\nOLDCS: %s\n", psz_csid);
    SG_VHASH_STDERR(pvh_cs);
#endif
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_new_cs)  );

    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_cs, "generation", &gen)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_new_cs, "generation", (SG_int64) gen)  );

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_cs, "dagnum", &psz_dagnum)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new_cs, "dagnum", psz_dagnum)  );

    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_cs, "ver", &ver)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_new_cs, "ver", ver)  );

    SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvh_cs, "parents", &pva_parents)  );
    if (pva_parents)
    {
        SG_varray* pva_new_parents = NULL;
        SG_uint32 count_parents = 0;
        SG_uint32 i_parent = 0;

        SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_new_cs, "parents", &pva_new_parents)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count_parents)  );

        for (i_parent=0; i_parent<count_parents; i_parent++)
        {
            const char* psz_parent = NULL;
            const char* psz_new_parent = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i_parent, &psz_parent)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_map_csid, psz_parent, &psz_new_parent)  );
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_new_parents, psz_new_parent)  );
        }
    }

    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_cs, "db", &pvh_db)  );
    if (pvh_db)
    {
        SG_vhash* pvh_new_db = NULL;
        SG_varray* pva_all_templates = NULL;
        const char* psz_template = NULL;

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_new_cs, "db", &pvh_new_db)  );

        SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvh_db, "all_templates", &pva_all_templates)  );
        if (pva_all_templates)
        {
            SG_ERR_CHECK(  SG_vhash__addcopy__varray(pCtx, pvh_new_db, "all_templates", pva_all_templates)  );
        }

        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_db, "template", &psz_template)  );
        if (psz_template)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new_db, "template", psz_template)  );
        }

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_db, "changes", &pvh_changes)  );
        if (pvh_changes)
        {
            SG_uint32 count_parents = 0;
            SG_uint32 i_parent = 0;
            SG_vhash* pvh_new_changes = NULL;
            
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_new_db, "changes", &pvh_new_changes)  );

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
            for (i_parent=0; i_parent<count_parents; i_parent++)
            {
                const char* psz_csid_parent = NULL;
                SG_vhash* pvh_changes_one_parent = NULL;
                SG_vhash* pvh_new_changes_one_parent = NULL;
                SG_vhash* pvh_add = NULL;
                SG_vhash* pvh_remove = NULL;
                SG_vhash* pvh_attach = NULL;
                const char* psz_new_csid_parent = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_changes_one_parent)  );
                if (psz_csid_parent[0])
                {
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_map_csid, psz_csid_parent, &psz_new_csid_parent)  );
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_new_changes, psz_new_csid_parent, &pvh_new_changes_one_parent)  );

                    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "add", &pvh_add)  );
                    if (pvh_add)
                    {
                        SG_uint32 count_records = 0;
                        SG_uint32 i_record = 0;
                        SG_vhash* pvh_new_add = NULL;

                        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_new_changes_one_parent, "add", &pvh_new_add)  );

                        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count_records)  );
                        for (i_record=0; i_record<count_records; i_record++)
                        {
                            const char* psz_hidrec = NULL;
                            const char* psz_new_hidrec = NULL;

                            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i_record, &psz_hidrec, NULL)  );

                            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_map_hidrec, psz_hidrec, &psz_new_hidrec)  );
                            if (psz_new_hidrec)
                            {
                                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_new_add, psz_new_hidrec)  );
                            }
                            else
                            {
                                SG_ERR_CHECK(  SG_clone__get_blob_as_vhash(pCtx, pfbi, psz_hidrec, NULL, &pvh_rec)  );
                                SG_ERR_CHECK(  sg_clone__usermap__fix_one_record(pCtx, pvh_usermap, pvh_rec)  );

                                SG_ERR_CHECK(  sg_clone__store_vhash(pCtx, pRepo, ptx, pvh_rec, &psz_new_hidrec_freeme)  );
                                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_map_hidrec, psz_hidrec, psz_new_hidrec_freeme)  );
                                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_new_add, psz_new_hidrec_freeme)  );
                                SG_NULLFREE(pCtx, psz_new_hidrec_freeme);
                                SG_VHASH_NULLFREE(pCtx, pvh_rec);
                            }
                        }
                    }

                    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "remove", &pvh_remove)  );
                    if (pvh_remove)
                    {
                        SG_uint32 count_records = 0;
                        SG_uint32 i_record = 0;
                        SG_vhash* pvh_new_remove = NULL;

                        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_new_changes_one_parent, "remove", &pvh_new_remove)  );
                        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_remove, &count_records)  );
                        for (i_record=0; i_record<count_records; i_record++)
                        {
                            const char* psz_hidrec = NULL;
                            const char* psz_new_remove_hidrec = NULL;

                            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_remove, i_record, &psz_hidrec, NULL)  );
                            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_map_hidrec, psz_hidrec, &psz_new_remove_hidrec)  );
                            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_new_remove, psz_new_remove_hidrec)  );
                        }
                    }

                    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_one_parent, "attach_add", &pvh_attach)  );
                    if (pvh_attach)
                    {
                        SG_uint32 count = 0;
                        SG_uint32 i = 0;
                        SG_vhash* pvh_new_attach = NULL;

                        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_new_changes_one_parent, "attach_add", &pvh_new_attach)  );
                        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_attach, &count)  );
                        for (i=0; i<count; i++)
                        {
                            const char* psz_hid = NULL;

                            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_attach, i, &psz_hid, NULL)  );

                            SG_ERR_CHECK(  sg_clone__copy_one_blob(
                                        pCtx,
                                        pfbi,
                                        ptwo,
                                        pRepo,
                                        ptx,
                                        psz_hid,
                                        NULL
                                        )  );
                            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_new_attach, psz_hid)  );
                        }
                    }
                }
            }
        }
    }

    // TODO should maybe walk through any OTHER fields in the old cs and copy them over

    SG_ERR_CHECK(  sg_clone__store_vhash(pCtx, pRepo, ptx, pvh_new_cs, &psz_new_csid)  );

#if 0
    fprintf(stderr, "\nNEWCS: %s\n", psz_new_csid);
    SG_VHASH_STDERR(pvh_new_cs);
#endif

    SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_map_csid, psz_csid, psz_new_csid)  );

    *ppvh_new_cs = pvh_new_cs;
    pvh_new_cs = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_new_cs);
    SG_VHASH_NULLFREE(pCtx, pvh_rec);
    SG_STRING_NULLFREE(pCtx, pstr_rec);
    SG_NULLFREE(pCtx, psz_new_hidrec_freeme);
    SG_NULLFREE(pCtx, psz_new_csid);
}

static void sg_clone__usermap__rewrite_one_dag(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    SG_uint64 dagnum,
    SG_vhash* pvh_usermap
    )
{
    SG_varray* pva_changesets = NULL;
    SG_uint32 count_changesets = 0;
    SG_uint32 i_changeset = 0;
    SG_varray* pva_templates = NULL;
    SG_vhash* pvh_cs = NULL;
    SG_vhash* pvh_map_csid = NULL;
    SG_vhash* pvh_map_hidrec = NULL;
    SG_dagfrag* pFrag = NULL;
    SG_dagnode* pdn = NULL;
    SG_vhash* pvh_new_cs = NULL;

    SG_ERR_CHECK(  SG_blobndx__list_templates(pCtx, pfbi->pbs, dagnum, &pva_templates)  );
    if (pva_templates)
    {
        SG_uint32 count_templates = 0;
        SG_uint32 i_template = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_templates, &count_templates)  );
        for (i_template=0; i_template<count_templates; i_template++)
        {
            const char* psz_hid_template = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_templates, i_template, &psz_hid_template)  );
            SG_ERR_CHECK(  sg_clone__copy_one_blob(
                        pCtx,
                        pfbi,
                        ptwo,
                        pRepo,
                        ptx,
                        psz_hid_template,
                        NULL
                        )  );
        }
    }

    SG_ERR_CHECK(  SG_blobndx__list_changesets(pCtx, pfbi->pbs, dagnum, &pva_changesets)  );

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_changesets, &count_changesets)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_map_csid)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_map_hidrec)  );

    {
        const char* psz_repo_id = NULL;
        const char* psz_admin_id = NULL;

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pfbi->pvh_header, SG_SYNC_REPO_INFO_KEY__REPO_ID, &psz_repo_id)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pfbi->pvh_header, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &psz_admin_id)  );
        SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx,&pFrag,psz_repo_id,psz_admin_id,dagnum)  );
    }

    for (i_changeset=0; i_changeset<count_changesets; i_changeset++)
    {
        const char* psz_csid = NULL;
        const char* psz_new_csid = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_changesets, i_changeset, &psz_csid)  );
        SG_ERR_CHECK(  SG_clone__get_blob_as_vhash(pCtx, pfbi, psz_csid, NULL, &pvh_cs)  );

        SG_ERR_CHECK(  sg_clone__usermap__rewrite_one_changeset(
                    pCtx, 
                    pfbi,
                    ptwo,
                    pRepo,
                    ptx,
                    psz_csid,
                    pvh_cs,
                    pvh_usermap,
                    pvh_map_csid,
                    pvh_map_hidrec,
                    &pvh_new_cs
                    )  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_map_csid, psz_csid, &psz_new_csid)  );
        SG_ERR_CHECK(  sg_clone__make_dagnode_from_changeset(pCtx, psz_new_csid, pvh_new_cs, &pdn)  );
        SG_ERR_CHECK(  SG_dagfrag__add_dagnode(pCtx, pFrag, &pdn)  );

        SG_VHASH_NULLFREE(pCtx, pvh_cs);
        SG_VHASH_NULLFREE(pCtx, pvh_new_cs);
    }

    SG_ERR_CHECK(  SG_repo__store_dagfrag(pCtx, pRepo, ptx, pFrag)  );
    pFrag = NULL;
    SG_DAGFRAG_NULLFREE(pCtx, pFrag);

    SG_ERR_CHECK(  SG_blobndx__store_audits(pCtx, pfbi->pbs, pRepo, ptx, dagnum, pvh_usermap, pvh_map_csid)  );

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_DAGFRAG_NULLFREE(pCtx, pFrag);
    SG_VHASH_NULLFREE(pCtx, pvh_cs);
    SG_VHASH_NULLFREE(pCtx, pvh_new_cs);
    SG_VHASH_NULLFREE(pCtx, pvh_map_csid);
    SG_VHASH_NULLFREE(pCtx, pvh_map_hidrec);
}

static void sg_clone__usermap__rewrite_db_dags_with_userid_fields(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    sg_twobuffers* ptwo,
    SG_repo* pRepo,
    SG_repo_tx_handle* ptx,
    SG_vhash* pvh_dags,
    SG_vhash* pvh_usermap,
    SG_vhash* pvh_dags_with_userid_fields
    )
{
    SG_uint32 count_dagnums = 0;
    SG_uint32 i_dagnum = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dags, &count_dagnums)  );

    for (i_dagnum=0; i_dagnum<count_dagnums; i_dagnum++)
    {
        const char* psz_dagnum = NULL;
        SG_bool b_has = SG_FALSE;
        SG_vhash* pvh_daginfo = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_dags, i_dagnum, &psz_dagnum, &pvh_daginfo)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_dags_with_userid_fields, psz_dagnum, &b_has)  );
        if (b_has)
        {
            SG_uint64 dagnum = 0;
            SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
            SG_ERR_CHECK(  sg_clone__usermap__rewrite_one_dag(
                        pCtx,
                        pfbi,
                        ptwo,
                        pRepo,
                        ptx,
                        dagnum,
                        pvh_usermap
                        )  );
        }
    }

fail:
    ;
}

static void sg_clone__store_fragball__with_usermap(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_repo* pRepo,
    SG_vhash* pvh_usermap
	)
{
    sg_twobuffers* ptwo = NULL;
    SG_vhash* pvh_db_dags_with_userid_fields = NULL;
    SG_vhash* pvh_dagnums_except = NULL;
    SG_repo_tx_handle* ptx = NULL;

    SG_ERR_CHECK(  SG_alloc1(pCtx, ptwo)  );

    SG_ERR_CHECK(  sg_clone__find_db_dags_with_userid_fields(pCtx, pfbi, &pvh_db_dags_with_userid_fields)  );

    SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &pvh_dagnums_except, pvh_db_dags_with_userid_fields)  );
    {
        char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

        SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__USERS, buf_dagnum, sizeof(buf_dagnum))  );
        SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_dagnums_except, buf_dagnum)  );
    }

    SG_ERR_CHECK(  sg_clone__store_fragball(
                pCtx, 
                pfbi, 
                pRepo,
                pvh_dagnums_except,
                pvh_usermap,
                NULL,
                NULL,
                NULL
                )  );

    SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pRepo, &ptx)  );
    SG_ERR_CHECK(  sg_clone__usermap__rewrite_db_dags_with_userid_fields(
                pCtx, 
                pfbi, 
                ptwo,
                pRepo, 
                ptx,
                pfbi->pvh_dags,
                pvh_usermap, 
                pvh_db_dags_with_userid_fields
                )  );
    SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &ptx)  );

fail:
	if (ptx)
    {
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pRepo, &ptx)  );
    }
    SG_NULLFREE(pCtx, ptwo);
    SG_VHASH_NULLFREE(pCtx, pvh_db_dags_with_userid_fields);
    SG_VHASH_NULLFREE(pCtx, pvh_dagnums_except);
}

static void sg_clone__slurp(
	SG_context* pCtx,
    SG_fragballinfo* pfbi,
    SG_uint64 which_dagnum,
    SG_vhash** ppvh_changesets
	)
{
    SG_blobndx* pbs = NULL;
    SG_uint32 count_ops = 0;
    SG_uint32 unit = 1024;
    const char* psz_unit = "KB";

	SG_NULLARGCHECK_RETURN(pfbi);

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Scanning", SG_LOG__FLAG__NONE)  );
    count_ops++;

	SG_ERR_CHECK(  SG_log__set_steps(pCtx, (SG_uint32) (pfbi->length / unit), psz_unit)  );

    SG_ERR_CHECK(  SG_fragballinfo__seek_to_beginning(pCtx, pfbi)  );

    SG_ERR_CHECK(  SG_blobndx__create(pCtx, &pbs)  );

	switch (pfbi->version)
	{
		case 1:
			SG_ERR_CHECK(  _slurp__version_1(pCtx, pbs, pfbi->file, unit, which_dagnum, ppvh_changesets)  );
			break;

		case 2:
			SG_ERR_CHECK(  _slurp__version_1(pCtx, pbs, pfbi->file, unit, which_dagnum, ppvh_changesets)  );
			break;

		default:
			SG_ERR_THROW(  SG_ERR_FRAGBALL_INVALID_VERSION  );
			break;
	}

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

    pfbi->pbs = pbs;
    pbs = NULL;

fail:
    SG_BLOBNDX_NULLFREE(pCtx, pbs);
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}

void SG_clone__to_remote(
	SG_context* pCtx,
    const char* psz_existing,
    const char* psz_username,
    const char* psz_password,
    const char* psz_new,
	SG_vhash** ppvhResult
    )
{
	SG_sync_client* pSyncClient = NULL;
	SG_sync_client_push_handle* pPush = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvhRepoInfo = NULL;
	SG_pathname* pPathTemp = NULL;
	char* pszFragballName = NULL;
	SG_bool bPop2ndOp = SG_FALSE;
    SG_fragballinfo* pfbi = NULL;
    SG_pathname* pPath_copy = NULL;
	char * pszCurrentDefault = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Cloning", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 3u, NULL)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Preparing remote clone")  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_existing, &pRepo)  );
	SG_ERR_CHECK(  SG_sync_remote__get_repo_info(pCtx, pRepo, SG_FALSE, SG_FALSE, &pvhRepoInfo)  );

	SG_ERR_CHECK(  SG_sync_client__open(pCtx, psz_new, psz_username, psz_password, &pSyncClient)  );
	SG_ERR_CHECK(  SG_sync_client__push_clone__begin(pCtx, pSyncClient, pvhRepoInfo, &pPush)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );


	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Copying local repository")  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathTemp)  );
	SG_ERR_CHECK(  SG_repo__fetch_repo__fragball(pCtx, pRepo, 3, pPathTemp, &pszFragballName) );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathTemp, pszFragballName)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_copy, pPathTemp)  );
    SG_ERR_CHECK(  SG_fragballinfo__alloc(pCtx, &pPath_copy, &pfbi)  );
    if (3 != pfbi->version)
    {
        SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "SG_clone__to_remote only supports v3 fragballs.  This repo must be upgraded before it can be push-cloned to a remote server.") );
    }
    SG_fragballinfo__free(pCtx, pfbi);
    pfbi = NULL;

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Uploading")  );
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Uploading repository", SG_LOG__FLAG__NONE)  );
	bPop2ndOp = SG_TRUE;
	SG_ERR_CHECK(  SG_sync_client__push_clone__upload_and_commit(pCtx, pSyncClient, &pPush, SG_TRUE, &pPathTemp, ppvhResult)  );
	
	//If there isn't a default sync path, set it.  Otherwise, add the new location to the paths.
	SG_localsettings__descriptor__get__sz(pCtx, psz_existing, SG_LOCALSETTING__PATHS_DEFAULT, &pszCurrentDefault);
	SG_ERR_CHECK_CURRENT_DISREGARD( SG_ERR_NOT_FOUND);
	if (pszCurrentDefault == NULL)
		SG_ERR_CHECK(  SG_localsettings__descriptor__update__sz(pCtx, psz_existing, SG_LOCALSETTING__PATHS_DEFAULT, psz_new)  );
	else
		SG_ERR_CHECK(  SG_sync__remember_sync_target(pCtx,  psz_existing, psz_new)  );
	
	SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
	bPop2ndOp = SG_FALSE;
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* fall through */
fail:
    SG_fragballinfo__free(pCtx, pfbi);
    pfbi = NULL;
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
	SG_PATHNAME_NULLFREE(pCtx, pPath_copy);
	SG_PATHNAME_NULLFREE(pCtx, pPathTemp);
	SG_NULLFREE(pCtx, pszFragballName);
	SG_NULLFREE(pCtx, pszCurrentDefault);
	if (pSyncClient)
	{
		if (pPush)
			SG_ERR_IGNORE(  SG_sync_client__push_clone__abort(pCtx, pSyncClient, &pPush)  );
		SG_SYNC_CLIENT_NULLFREE(pCtx, pSyncClient);
	}
	SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
	if (bPop2ndOp)
		SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
}
		
void sg_clone__get_fragball(
	SG_context* pCtx,
    SG_sync_client* pClient,
    SG_pathname* pPath_staging,
    SG_pathname** pp
    )
{
    SG_pathname* pPath_fragball = NULL;
    char* psz_fragball_name = NULL;

    SG_ERR_CHECK(  SG_sync_client__pull_clone(pCtx, pClient, NULL, pPath_staging, &psz_fragball_name)  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_fragball, pPath_staging, psz_fragball_name)  );

    *pp = pPath_fragball;
    pPath_fragball = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    SG_NULLFREE(pCtx, psz_fragball_name);
}

void sg_clone__make_staging_area(
    SG_context* pCtx,
    SG_pathname** pp
    )
{
    char* psz_staging_area_name = NULL;
    SG_pathname* pPath_staging = NULL;

    SG_ERR_CHECK(  SG_tid__alloc(pCtx, &psz_staging_area_name)  );
    SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, (const char*)psz_staging_area_name, &pPath_staging)  );
    SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath_staging)  );

    *pp = pPath_staging;
    pPath_staging = NULL;

fail:
    SG_NULLFREE(pCtx, psz_staging_area_name);
    SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
}

static void sg_clone__create_repo(
    SG_context* pCtx,
    const char* psz_new_repo_name,
    const SG_vhash* pvh_new_repo_info,
    SG_repo** ppRepo
    )
{
    const char* psz_repo_id = NULL;
    const char* psz_admin_id = NULL;
    const char* psz_hash_method = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_new_repo_info, SG_SYNC_REPO_INFO_KEY__REPO_ID, &psz_repo_id)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_new_repo_info, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &psz_admin_id)  );
    SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh_new_repo_info, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, &psz_hash_method)  );

    // create the new repo
    SG_ERR_CHECK_RETURN(  SG_repo__create_empty(
                pCtx, 
                psz_repo_id,
                psz_admin_id,
                psz_hash_method,
                psz_new_repo_name,
                SG_REPO_STATUS__CLONING,
                ppRepo)
            );
}

void SG_clone__to_local(
    SG_context* pCtx,
    const char* psz_existing_repo_spec,
    const char* psz_username,
    const char* psz_password,
    const char* psz_new_repo_spec,
    struct SG_clone__params__all_to_something* p_params_ats,
    struct SG_clone__params__pack* p_params_pack,
    struct SG_clone__demands* p_demands
    )
{
	SG_bool bNewIsRemote = SG_FALSE;
    SG_pathname* pPath_staging = NULL;
    SG_sync_client* pClient = NULL;
    SG_vhash* pvh_repo_info = NULL;
    SG_pathname* pPath_fragball = NULL;
    SG_repo* pRepo = NULL;
    SG_bool bSuccess = SG_FALSE;
	SG_bool bNewRepoIsOurs = SG_FALSE;
    SG_uint32 count_ops = 0;
    SG_fragballinfo* pfbi = NULL;
    SG_bool bExistingIsRemote = SG_FALSE;

    SG_NULLARGCHECK_RETURN(psz_existing_repo_spec);
    SG_NULLARGCHECK_RETURN(psz_new_repo_spec);

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, psz_existing_repo_spec, &bExistingIsRemote)  );
	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, psz_new_repo_spec, &bNewIsRemote)  );

	if (bNewIsRemote)
    {
		SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "SG_clone__to_local does not support push clone") );
    }

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Cloning", SG_LOG__FLAG__NONE)  );
    count_ops++;

    // psz_new_repo_spec is actually a descriptor name.  validate it.
    SG_ERR_CHECK(  SG_clone__validate_new_repo_name(pCtx, psz_new_repo_spec)  );

    // make a directory for our staging area
    SG_ERR_CHECK(  sg_clone__make_staging_area(pCtx, &pPath_staging)  );

    // open connection to the other side
    SG_ERR_CHECK(  SG_sync_client__open(pCtx, psz_existing_repo_spec, psz_username, psz_password, &pClient)  );

    /* If this is a local->local clone, this might be a clone performed on the server, 
     * initiated via the web UI. We want the repo to quickly claim the descriptor name and 
     * appear in the "Cloning..." state, so we do it here. */
    if (!bExistingIsRemote)
    {
        SG_ERR_CHECK(  SG_sync_client__get_repo_info(pCtx, pClient, SG_FALSE, SG_FALSE, &pvh_repo_info)  );
        SG_ERR_CHECK(  sg_clone__create_repo(pCtx, psz_new_repo_spec, pvh_repo_info, &pRepo)  );
        bNewRepoIsOurs = SG_TRUE;
    }

    // get the fragball
    SG_ERR_CHECK(  sg_clone__get_fragball(pCtx, pClient, pPath_staging, &pPath_fragball)  );

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Creating new instance", SG_LOG__FLAG__NONE)  );
    count_ops++;

    SG_ERR_CHECK(  SG_fragballinfo__alloc(pCtx, &pPath_fragball, &pfbi)  );

    /* If we didn't already create the repo above (because this isn't a local->local clone) 
     * create it here using the fragball header when possible to avoid another roundtrip. */
    if (!pRepo)
    {
        if (1 == pfbi->version)
        {
            // need to ask the other side for this info
            SG_ERR_CHECK(  SG_sync_client__get_repo_info(pCtx, pClient, SG_FALSE, SG_FALSE, &pvh_repo_info)  );
            SG_ERR_CHECK(  sg_clone__create_repo(pCtx, psz_new_repo_spec, pvh_repo_info, &pRepo)  );
        }
        else
        {
            // for post-v1 fragballs, the info is in the header
            SG_ERR_CHECK(  sg_clone__create_repo(pCtx, psz_new_repo_spec, pfbi->pvh_header, &pRepo)  );
        }
        bNewRepoIsOurs = SG_TRUE;
    }

    if (1 == pfbi->version)
    {
        SG_ERR_CHECK(  _commit__version_1(pCtx, pfbi, pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_clone__store_fragball(
                    pCtx, 
                    pfbi, 
                    pRepo,
                    NULL,  // except
                    NULL,
                    p_params_ats,
                    p_params_pack,
                    p_demands
                    )  );
    }

    bSuccess = SG_TRUE;

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

    SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_new_repo_spec, SG_REPO_STATUS__NORMAL)  );

    SG_fragballinfo__free(pCtx, pfbi);
    pfbi = NULL;
    SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath_staging) );
    SG_PATHNAME_NULLFREE(pCtx, pPath_staging);

	/* fall through */

    if (count_ops)
    {
        SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }

fail:
    SG_fragballinfo__free(pCtx, pfbi);
    if (pPath_staging)
    {
        SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath_staging) );
        SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
    }
    SG_VHASH_NULLFREE(pCtx, pvh_repo_info);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    SG_SYNC_CLIENT_NULLFREE(pCtx, pClient);
    SG_REPO_NULLFREE(pCtx, pRepo);
	/* If we failed after creating the descriptor, clean it up. */
	if (bNewRepoIsOurs && !bSuccess)
	{
		SG_ERR_IGNORE(  SG_repo__delete_repo_instance(pCtx, psz_new_repo_spec)  );
		SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, psz_new_repo_spec)  );
	}
    while (count_ops)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_ops--;
    }
}

void SG_clone__from_fragball(
	SG_context* pCtx,
	const char* psz_clone_id,
	const char* psz_new_repo_name,
	SG_pathname* pPath_staging,
	const char* psz_fragball_name
    )
{
    SG_pathname* pPath_fragball = NULL;
    SG_repo* pRepo = NULL;
    SG_bool bSuccess = SG_FALSE;
    SG_fragballinfo* pfbi = NULL;
	SG_bool bRepoIsOurs = SG_FALSE;
	SG_vhash* pvhRefDescriptor = NULL; // Owned by the repo. Don't free.

	SG_NONEMPTYCHECK_RETURN(psz_clone_id);
    SG_NONEMPTYCHECK_RETURN(psz_new_repo_name);
	SG_NULLARGCHECK_RETURN(pPath_staging);
    SG_NONEMPTYCHECK_RETURN(psz_fragball_name);

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Cloning", SG_LOG__FLAG__NONE)  );

    /* We don't validate the repo name here. That was done before the fragball was uploaded. */
    // construct path to fragball
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_fragball, pPath_staging, psz_fragball_name)  );

    SG_ERR_CHECK(  SG_fragballinfo__alloc(pCtx, &pPath_fragball, &pfbi)  );

    if (3 != pfbi->version)
    {
        SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "SG_clone__from_fragball only supports v3 fragballs") );
    }

    /* Open the repo into which the clone will be committed. Make sure it's ours. */
	{
		SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
		const SG_vhash* pvhRefDescriptor = NULL;
		const char* pszRefCloneId = NULL;

		SG_ERR_CHECK(  SG_repo__open_unavailable_repo_instance(pCtx, psz_new_repo_name, NULL, &status, NULL, &pRepo)  );
		SG_ERR_CHECK(  SG_repo__get_descriptor(pCtx, pRepo, &pvhRefDescriptor)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhRefDescriptor, SG_SYNC_DESCRIPTOR_KEY__CLONE_ID, &pszRefCloneId)  );
		if ( (NULL == pszRefCloneId) || (0 != strcmp(pszRefCloneId, psz_clone_id)) || SG_REPO_STATUS__CLONING != status )
			SG_ERR_THROW(SG_ERR_REPO_ALREADY_EXISTS);
	}
	bRepoIsOurs = SG_TRUE;

    SG_ERR_CHECK(  sg_clone__store_fragball(
                pCtx, 
                pfbi, 
                pRepo,
                NULL,  // except
                NULL,
                NULL,
                NULL,
                NULL
                )  );

	SG_ERR_CHECK(  SG_repo__get_descriptor__ref(pCtx, pRepo, &pvhRefDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvhRefDescriptor, SG_SYNC_DESCRIPTOR_KEY__CLONE_ID)  );
	SG_ERR_CHECK(  SG_closet__descriptors__update(pCtx, psz_new_repo_name, pvhRefDescriptor, SG_REPO_STATUS__NORMAL)  );

    bSuccess = SG_TRUE;
	/* fall through */
fail:
    SG_fragballinfo__free(pCtx, pfbi);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    SG_REPO_NULLFREE(pCtx, pRepo);
	/* If we failed and we know this is our repo, clean it up. */
	if (!bSuccess && bRepoIsOurs)
	{
		SG_ERR_IGNORE(  SG_repo__delete_repo_instance(pCtx, psz_new_repo_name)  );
		SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, psz_new_repo_name)  );
	}
	SG_log__pop_operation(pCtx);
}

static void x_get_users(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh,
		SG_vhash** ppvhNames
        )
{
    SG_vhash* pvh_users = NULL;
	SG_vhash* pvh_usernames = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_users)  );
	SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_usernames)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_username = NULL;
        const char* psz_userid = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "name", &psz_username)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_ZING_FIELD__RECID, &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_users, psz_userid, &b_already)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_users, psz_userid, psz_username)  );
        }
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_usernames, psz_username, &b_already)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_usernames, psz_username)  );
        }
    }

    *ppvh = pvh_users;
    pvh_users = NULL;
	*ppvhNames = pvh_usernames;
	pvh_usernames = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_users);
	SG_VHASH_NULLFREE(pCtx, pvh_usernames);
}

static void sg_clone__get_users_and_hash_method_from_temporary_usermap_repo(
	SG_context* pCtx,
    const char* psz_descriptor_name,
    SG_vhash** ppvhUsers,
	SG_vhash** ppvhUsernames,
	char** ppszHashMethod
    )
{
    SG_vhash* pvh_users = NULL;
	SG_vhash* pvh_usernames = NULL;
    SG_varray* pva_users = NULL;
    SG_varray* pva_fields = NULL;
    char* psz_leaf = NULL;
    SG_repo* pRepo = NULL;
	char* pszHashMethod = NULL;

	SG_ERR_CHECK(  SG_repo__open_unavailable_repo_instance(pCtx, psz_descriptor_name, NULL, NULL, NULL, &pRepo)  );

	SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pRepo, &pszHashMethod)  );

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__USERS, &psz_leaf)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "*")  );

	SG_ERR_CHECK(  SG_zing__query(pCtx, pRepo, SG_DAGNUM__USERS, psz_leaf, "user", NULL, "name", 0, 0, pva_fields, &pva_users)  );

    SG_ERR_CHECK(  x_get_users(pCtx, pva_users, &pvh_users, &pvh_usernames)  );

	*ppszHashMethod = pszHashMethod;
	pszHashMethod = NULL;
    *ppvhUsers = pvh_users;
    pvh_users = NULL;
	*ppvhUsernames = pvh_usernames;
	pvh_usernames = NULL;

fail:
	SG_NULLFREE(pCtx, pszHashMethod);
    SG_NULLFREE(pCtx, psz_leaf);
    SG_VHASH_NULLFREE(pCtx, pvh_users);
	SG_VHASH_NULLFREE(pCtx, pvh_usernames);
    SG_VARRAY_NULLFREE(pCtx, pva_users);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_clone__finish_usermap(
	SG_context* pCtx,
    const char* psz_repo_being_usermapped  
    )
{
    SG_pathname* pPath_fragball = NULL;
    SG_vhash* pvh_users = NULL;
	SG_vhash* pvh_usernames = NULL;
    char* psz_admin_id = NULL;
    char* psz_hash_method = NULL;
    const char* psz_fragball_hash_method = NULL;
    SG_vhash* pvh_usermap = NULL;
    SG_fragballinfo* pfbi = NULL;
    SG_uint32 count_users_in_map = 0;
    SG_uint32 i = 0;
    char buf_repo_id[SG_GID_BUFFER_LENGTH];
    char* psz_actual_new_userid = NULL;
    SG_repo* pRepo = NULL;
    SG_repo* pRepoUserMaster = NULL;
	char* psz_sanitized_username = NULL;
	char* psz_unique_username = NULL;

    SG_NULLARGCHECK_RETURN(psz_repo_being_usermapped);

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Importing", SG_LOG__FLAG__NONE)  );
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, 2u, NULL)  );

    SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_repo_being_usermapped, SG_REPO_STATUS__IMPORTING)  );

    // get the users from the temporary repo
	// We can also get the appropriate hash method for the new "real" repo from the temporary one.
    SG_ERR_CHECK(  sg_clone__get_users_and_hash_method_from_temporary_usermap_repo(pCtx, psz_repo_being_usermapped, &pvh_users, &pvh_usernames, &psz_hash_method)  );

    // get admin id from the user master
    SG_ERR_CHECK(  SG_REPO__USER_MASTER__OPEN(pCtx, &pRepoUserMaster) );   
    SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepoUserMaster, &psz_admin_id)  );  

    // empty husk
    SG_ERR_CHECK(  SG_gid__generate(pCtx, buf_repo_id, sizeof(buf_repo_id))  );
    SG_ERR_CHECK(  SG_repo__replace_empty(
                pCtx, 
                buf_repo_id,
                psz_admin_id,
                psz_hash_method,
                psz_repo_being_usermapped,
                SG_REPO_STATUS__IMPORTING,
                &pRepo)
            );
    // pull the users dag into the husk
	SG_ERR_CHECK(  SG_pull__admin__local(pCtx, pRepo, pRepoUserMaster, NULL)  );
    
    // create any NEW users from the user map
    SG_ERR_CHECK(  SG_usermap__users__get_all(pCtx, psz_repo_being_usermapped, &pvh_usermap)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_usermap, &count_users_in_map)  );
    for (i=0; i<count_users_in_map; i++)
    {
        const char* psz_old_userid = NULL;
        const char* psz_new_userid = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pvh_usermap, i, &psz_old_userid, &psz_new_userid)  );
        if (0 == strcmp("NEW", psz_new_userid))
        {
            const char* psz_username = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_users, psz_old_userid, &psz_username)  );

			// make sure that the username is valid
			SG_ERR_CHECK(  SG_user__sanitize_username(pCtx, psz_username, &psz_sanitized_username)  );

			// if sanitizing the username changed it
			// then we also need to make sure the new username isn't already in use
			if (strcmp(psz_username, psz_sanitized_username) != 0)
			{
				SG_ERR_CHECK(  SG_user__uniqify_username(pCtx, psz_sanitized_username, pvh_usernames, &psz_unique_username)  );
				SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_usernames, psz_unique_username)  );
				SG_NULLFREE(pCtx, psz_sanitized_username);
				psz_sanitized_username = psz_unique_username;
				psz_unique_username = NULL;
			}

            SG_ERR_CHECK(  SG_user__create__inactive(pCtx, pRepo, psz_sanitized_username, &psz_actual_new_userid)  );
            SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_usermap, psz_old_userid, psz_actual_new_userid)  );
			SG_NULLFREE(pCtx, psz_sanitized_username);
            SG_NULLFREE(pCtx, psz_actual_new_userid);
        }
    }
    SG_REPO_NULLFREE(pCtx, pRepoUserMaster);
    SG_REPO_NULLFREE(pCtx, pRepo);
    
    // construct path to fragball
    SG_ERR_CHECK(  SG_usermap__get_fragball_path(pCtx, psz_repo_being_usermapped, &pPath_fragball)  );

    SG_ERR_CHECK(  SG_fragballinfo__alloc(pCtx, &pPath_fragball, &pfbi)  );

    if (3 != pfbi->version)
    {
        SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "SG_clone__finish_usermap only supports v3 fragballs") );
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pfbi->pvh_header, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, &psz_fragball_hash_method)  );
    if (0 != strcmp(psz_hash_method, psz_fragball_hash_method))
    {
		SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "Usermap clone requires the same hash methods") );
    }

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Copying data from v3 fragball")  );
	SG_ERR_CHECK(  SG_repo__open_unavailable_repo_instance(pCtx, psz_repo_being_usermapped, NULL, NULL, NULL, &pRepo)  );
    SG_ERR_CHECK(  sg_clone__store_fragball__with_usermap(pCtx, pfbi, pRepo, pvh_usermap)  );
    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_repo_being_usermapped, SG_REPO_STATUS__NORMAL)  );
    SG_fragballinfo__free(pCtx, pfbi);
    pfbi = NULL;
	SG_ERR_CHECK(  SG_usermap__delete(pCtx, psz_repo_being_usermapped)  );

	/* fall through */
fail:
    SG_fragballinfo__free(pCtx, pfbi);
	SG_REPO_NULLFREE(pCtx, pRepoUserMaster);
    SG_REPO_NULLFREE(pCtx, pRepo);
    SG_NULLFREE(pCtx, psz_admin_id);
    SG_NULLFREE(pCtx, psz_hash_method);
    SG_VHASH_NULLFREE(pCtx, pvh_usermap);
    SG_VHASH_NULLFREE(pCtx, pvh_users);
	SG_VHASH_NULLFREE(pCtx, pvh_usernames);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
	SG_NULLFREE(pCtx, psz_sanitized_username);
	SG_NULLFREE(pCtx, psz_unique_username);
	SG_log__pop_operation(pCtx);
}

// TODO this function is actually only used for testing purposes
void SG_clone__prep_usermap(
	SG_context* pCtx,
    const char* psz_existing_repo_spec,
    const char* psz_username,
    const char* psz_password,
    const char* psz_new_repo_spec
    )
{
	SG_bool bNewIsRemote = SG_FALSE;
    SG_pathname* pPath_staging = NULL;
    SG_sync_client* pClient = NULL;
    SG_vhash* pvh_repo_info = NULL;
    SG_pathname* pPath_fragball = NULL;
    SG_pathname* pPath_orig_fragball = NULL;
    SG_repo* pRepo = NULL;
    SG_bool bSuccess = SG_FALSE;
	SG_bool bNewRepoIsOurs = SG_FALSE;
    SG_fragballinfo* pfbi = NULL;

    SG_NULLARGCHECK_RETURN(psz_existing_repo_spec);
    SG_NULLARGCHECK_RETURN(psz_new_repo_spec);

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, psz_new_repo_spec, &bNewIsRemote)  );

	if (bNewIsRemote)
	{
		SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "Usermap clone must be local") );
	}
	else
	{
        SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Cloning", SG_LOG__FLAG__NONE)  );
        SG_ERR_CHECK(  SG_log__set_steps(pCtx, 2u, NULL)  );

        // psz_new_repo_spec is actually a descriptor name.  validate it.
		SG_ERR_CHECK(  SG_clone__validate_new_repo_name(pCtx, psz_new_repo_spec)  );

        // make a directory for our staging area
        SG_ERR_CHECK(  sg_clone__make_staging_area(pCtx, &pPath_staging)  );

        // open connection to the other side
        SG_ERR_CHECK(  SG_log__set_step(pCtx, "Retrieving fragball")  );
        SG_ERR_CHECK(  SG_sync_client__open(pCtx, psz_existing_repo_spec, psz_username, psz_password, &pClient)  );

        // get the fragball
        SG_ERR_CHECK(  sg_clone__get_fragball(pCtx, pClient, pPath_staging, &pPath_orig_fragball)  );

        // rename the fragball file
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_fragball, pPath_staging, SG_USERMAP_FRAGBALL_NAME)  );
        SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath_orig_fragball, pPath_fragball)  );
        SG_PATHNAME_NULLFREE(pCtx, pPath_orig_fragball);

        SG_ERR_CHECK(  SG_fragballinfo__alloc(pCtx, &pPath_fragball, &pfbi)  );

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

        if (3 != pfbi->version)
        {
            SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "SG_clone__prep_usermap only supports v3 fragballs") );
        }

        SG_ERR_CHECK(  sg_clone__create_repo(pCtx, psz_new_repo_spec, pfbi->pvh_header, &pRepo)  );
		bNewRepoIsOurs = SG_TRUE;

        SG_ERR_CHECK(  SG_log__set_step(pCtx, "Copying data from v3 fragball")  );

        SG_ERR_CHECK(  sg_clone__store_fragball__only_the_users_dag(
                    pCtx, 
                    pfbi, 
                    pRepo
                    )  );
        SG_fragballinfo__free(pCtx, pfbi);
        pfbi = NULL;
        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

        bSuccess = SG_TRUE;

        SG_ERR_CHECK(  SG_usermap__create(pCtx, psz_new_repo_spec, pPath_staging)  );
        SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_new_repo_spec, SG_REPO_STATUS__NEED_USER_MAP)  );
    }

    //SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath_staging) );
    SG_PATHNAME_NULLFREE(pCtx, pPath_staging);

	/* fall through */

fail:
    SG_fragballinfo__free(pCtx, pfbi);
    if (pPath_staging)
    {
        SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath_staging) );
        SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
    }
    SG_VHASH_NULLFREE(pCtx, pvh_repo_info);
    SG_PATHNAME_NULLFREE(pCtx, pPath_orig_fragball);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    SG_SYNC_CLIENT_NULLFREE(pCtx, pClient);
    SG_REPO_NULLFREE(pCtx, pRepo);
	/* If we failed after creating the descriptor, clean it up. */
	if (bNewRepoIsOurs && !bSuccess)
	{
		SG_ERR_IGNORE(  SG_repo__delete_repo_instance(pCtx, psz_new_repo_spec)  );
		SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, psz_new_repo_spec)  );
	}
	SG_log__pop_operation(pCtx);
}

void SG_clone__from_fragball__prep_usermap(
	SG_context* pCtx,
	const char* psz_clone_id,
	const char* psz_new_repo_name,
	SG_pathname** ppPath_staging,
	const char* psz_fragball_name
    )
{
    SG_pathname* pPath_staging = NULL;
    SG_pathname* pPath_fragball = NULL;
    SG_pathname* pPath_orig_fragball = NULL;
    SG_repo* pRepo = NULL;
    SG_bool bSuccess = SG_FALSE;
	SG_bool bRepoIsOurs = SG_FALSE;
    SG_fragballinfo* pfbi = NULL;

	SG_NONEMPTYCHECK_RETURN(psz_clone_id);
    SG_NULLARGCHECK_RETURN(psz_new_repo_name);
    SG_NULLARGCHECK_RETURN(ppPath_staging);

    pPath_staging = *ppPath_staging;

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Cloning", SG_LOG__FLAG__NONE)  );
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, 2u, NULL)  );

	/* We don't validate the repo name here. It was validated before the fragball was uploaded. */

    // construct path to fragball
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_orig_fragball, pPath_staging, psz_fragball_name)  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_fragball, pPath_staging, SG_USERMAP_FRAGBALL_NAME)  );
    SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath_orig_fragball, pPath_fragball)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_orig_fragball);

    SG_ERR_CHECK(  SG_fragballinfo__alloc(pCtx, &pPath_fragball, &pfbi)  );

    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	/* Open the repo into which the clone will be committed. Make sure it's ours. */
	{
		SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
		const SG_vhash* pvhRefDescriptor = NULL;
		const char* pszRefCloneId = NULL;

		SG_ERR_CHECK(  SG_repo__open_unavailable_repo_instance(pCtx, psz_new_repo_name, NULL, &status, NULL, &pRepo)  );
		SG_ERR_CHECK(  SG_repo__get_descriptor(pCtx, pRepo, &pvhRefDescriptor)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhRefDescriptor, SG_SYNC_DESCRIPTOR_KEY__CLONE_ID, &pszRefCloneId)  );
		if ( (NULL == pszRefCloneId) || (0 != strcmp(pszRefCloneId, psz_clone_id)) || SG_REPO_STATUS__CLONING != status )
			SG_ERR_THROW(SG_ERR_REPO_ALREADY_EXISTS);
	}
	bRepoIsOurs = SG_TRUE;

    if (3 != pfbi->version)
    {
        SG_ERR_THROW2( SG_ERR_INVALIDARG, (pCtx, "SG_clone__from_fragball__prep_usermap only supports v3 fragballs") );
    }

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Copying data from v3 fragball")  );

    SG_ERR_CHECK(  sg_clone__store_fragball__only_the_users_dag(
                pCtx, 
                pfbi, 
                pRepo
                )  );
    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_fragballinfo__free(pCtx, pfbi);
    pfbi = NULL;
	SG_ERR_CHECK(  SG_usermap__create(pCtx, psz_new_repo_name, pPath_staging)  );
	SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_new_repo_name, SG_REPO_STATUS__NEED_USER_MAP)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
    *ppPath_staging = NULL;

    bSuccess = SG_TRUE;

	/* fall through */

fail:
    SG_fragballinfo__free(pCtx, pfbi);
    SG_PATHNAME_NULLFREE(pCtx, pPath_orig_fragball);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fragball);
    SG_REPO_NULLFREE(pCtx, pRepo);
	/* If we failed after creating the descriptor, clean it up. */
	if (bRepoIsOurs && !bSuccess)
	{
		SG_ERR_IGNORE(  SG_repo__delete_repo_instance(pCtx, psz_new_repo_name)  );
		SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, psz_new_repo_name)  );
	}
	SG_log__pop_operation(pCtx);
}

void SG_clone__init_demands(
	SG_context* pCtx,
    struct SG_clone__demands* p_demands
    )
{
    SG_NULLARGCHECK_RETURN(p_demands);

    p_demands->vcdiff_savings_over_full = -1;
    p_demands->vcdiff_savings_over_zlib = -1;
    p_demands->zlib_savings_over_full = -1;
}

void SG_clone__validate_new_repo_name(SG_context* pCtx, const char* psz_new_descriptor_name)
{
	char* szNormalizedDescriptorName = NULL;
	SG_bool alreadyExists = SG_FALSE;

	SG_ERR_CHECK(  SG_repo__validate_repo_name(pCtx, psz_new_descriptor_name, &szNormalizedDescriptorName)  );
	SG_ERR_CHECK(  SG_closet__descriptors__exists(pCtx, szNormalizedDescriptorName, &alreadyExists)  );
	if (alreadyExists)
    {
		SG_ERR_THROW(SG_ERR_REPO_ALREADY_EXISTS);
    }

fail:
	SG_NULLFREE(pCtx, szNormalizedDescriptorName);
}

#if 0
void sg_dump_vcdiff_chains_from_fs3_sqlite3_file(
    SG_context* pCtx,
    const char* psz_path
    )
{
    SG_pathname* pPath = NULL;
    sqlite3* psql = NULL;
	sqlite3_stmt* pStmt = NULL;
	int rc;
    SG_vhash* pvh_blobs = NULL;
    SG_uint32 count_blobs = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_blobs)  );
    SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath, psz_path)  );
    SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath, &psql)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
		"SELECT hid, encoding, len_encoded, len_full, hid_vcdiff FROM blobs")  );

	while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
        const char* psz_hid = (const char*) sqlite3_column_text(pStmt, 0);
        SG_int64 encoding = sqlite3_column_int(pStmt, 1);
        //SG_int64 len_encoded = sqlite3_column_int64(pStmt, 2);
        //SG_int64 len_full = sqlite3_column_int64(pStmt, 3);
        const char* psz_vcdiff = (const char*) sqlite3_column_text(pStmt, 4);
        SG_vhash* pvh_b = NULL;

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_blobs, psz_hid, &pvh_b)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_b, "encoding", encoding)  );
        //SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_b, "len_encoded", len_encoded)  );
        //SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_b, "len_full", len_full)  );
        if (psz_vcdiff)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_b, "hid_vcdiff", psz_vcdiff)  );
        }
	}

	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_blobs, &count_blobs)  );
    for (i=0; i<count_blobs; i++)
    {
        const char* psz_hid = NULL;
        SG_vhash* pvh_b = NULL;
        SG_int64 i64 = -1;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_blobs, i, &psz_hid, &pvh_b)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_b, "encoding", &i64)  );
        if ('v' == i64)
        {
            SG_uint32 len = 1;
            SG_vhash* pvh_cur = pvh_b;

            while (1)
            {
                const char* psz_vcdiff = NULL;
                SG_int64 i2 = -1;

                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_cur, "hid_vcdiff", &psz_vcdiff)  );
                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_blobs, psz_vcdiff, &pvh_cur)  );
                SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_cur, "encoding", &i2)  );
                if ('v' == i2)
                {
                    len++;
                }
                else
                {
                    break;
                }
            }

            printf("%06d -- %s\n", len, psz_hid);
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_blobs);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}
#endif

