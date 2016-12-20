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

#define SG_DOUBLE_CHECK__NEW_FILTER 0

#define MY_SLEEP_MS      100
#define MY_TIMEOUT_MS  30000

#define MY_EXPR__TYPE__FROM_ME      "from_me"
#define MY_EXPR__TYPE__TO_ME        "to_me"
#define MY_EXPR__TYPE__XREF         "xref"
#define MY_EXPR__TYPE__USERNAME     "username"
#define MY_EXPR__TYPE__ALIAS        "alias"

#define MY_JOIN__TYPE__USERNAME     "username"
#define MY_JOIN__TYPE__FROM_ME      "from_me"
#define MY_JOIN__TYPE__TO_ME        "to_me"
#define MY_JOIN__TYPE__XREF         "xref"

#define MY_JOIN__TYPE               "type"
#define MY_JOIN__RECTYPE            "rectype"
#define MY_JOIN__REF_TO_ME          "ref_to_me"
#define MY_JOIN__REF_TO_OTHER       "ref_to_other"
#define MY_JOIN__TABLE_ALIAS        "table_alias"
#define MY_JOIN__XREF_TABLE_ALIAS   "xref_table_alias"

#define MY_EXPR__TYPE   "type"
#define MY_EXPR__RECTYPE "rectype"
#define MY_EXPR__REF    "ref"
#define MY_EXPR__FIELD  "field"
#define MY_EXPR__FIELDS "fields"
#define MY_EXPR__ALIAS  "alias"
#define MY_EXPR__USERID "userid"
#define MY_EXPR__XREF  "xref"
#define MY_EXPR__XREF_RECID_ALIAS  "xref_recid_alias"
#define MY_EXPR__REF_TO_ME  "ref_to_me"
#define MY_EXPR__REF_TO_OTHER  "ref_to_other"

#define MY_COLUMN__DEST__KEY        "key"       // keeps multirow records together
#define MY_COLUMN__DEST__RECORD     "record"    // goes into the top record
#define MY_COLUMN__DEST__SUB        "sub"       // goes into a collection
#define MY_COLUMN__DEST__SUB__KEY   "subkey"    // goes into a collection

#define MY_COLUMN__DEST   "dest"

#define MY_COLUMN__SUB    "subrecord"
#define MY_COLUMN__ALIAS  "alias"
#define MY_COLUMN__EXPR   "expr"
#define MY_COLUMN__TYPE   "type"

struct _SG_dbndx_query
{
    SG_repo* pRepo;

    SG_uint64 iDagNum;

    sqlite3* psql;
	SG_bool			bInTransaction;

    SG_pathname* pPath_me;
    SG_pathname* pPath_filters_dir;
};

static void sg_dbndx_query__get_pathname_for_state_filter(
	SG_context* pCtx,
    SG_pathname* pPath_filters_dir,
    SG_uint64 dagnum,
    const char* psz_csid,
    SG_int32 gen,
    SG_pathname** pp
    )
{
    SG_pathname* pPath = NULL;
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
    SG_string* pstr = NULL;

    SG_ASSERT(gen >= 0);

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );
    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "%16s_%08x_%s", buf_dagnum, gen, psz_csid)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath, pPath_filters_dir)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPath,SG_string__sz(pstr))  );

    *pp = pPath;
    pPath = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void sg_dbndx__parse_filter_filename(
	SG_context* pCtx,
    SG_uint64 dagnum,
    const char* psz_name,
    SG_int32* pi_gen,
    const char** ppsz_csid
    )
{
    char buf_gen[9];
    SG_uint32 ugen = 0;
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
    SG_uint32 i;
    SG_uint32 len = SG_STRLEN(psz_name);

    if (len < (16+1+8+1+40))
    {
        *ppsz_csid = NULL;
        goto fail;
    }

    if (psz_name[16] != '_')
    {
        *ppsz_csid = NULL;
        goto fail;
    }

    if (psz_name[25] != '_')
    {
        *ppsz_csid = NULL;
        goto fail;
    }

    for (i=0; i<len; i++)
    {
        char c = psz_name[i];

        if (
                (16 == i)
                || (25 == i)
                || ((c >= '0') && (c <= '9'))
                || ((c >= 'a') && (c <= 'f'))
                || ((c >= 'A') && (c <= 'F'))
           )
        {
            // ok
        }
        else
        {
            *ppsz_csid = NULL;
            goto fail;
        }
    }

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );

    if (0 != memcmp(buf_dagnum, psz_name, 16))
    {
        *ppsz_csid = NULL;
        goto fail;
    }

    memcpy(buf_gen, psz_name + 17, 8);
    buf_gen[8] = 0;
    
    SG_ERR_CHECK(  SG_hex__parse_hex_uint32(pCtx, buf_gen, 8, &ugen)  );

    *pi_gen = (SG_int32) ugen;
    *ppsz_csid = psz_name + 26;

fail:
    ;
}

static void sg_dbndx__do_query__possibly_multirow(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_string* pstr_query,
    SG_bool b_history,
    SG_bool b_multirow_joins,
    SG_bool b_usernames,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_columns,
    SG_vhash* pvh_in,
    SG_varray** ppva
    );

static void sg_dbndx__add_joins(
	SG_context* pCtx,
    const char* psz_my_rectype,
    SG_vhash* pvh_schema,
    SG_string* pstr_table,
    SG_vhash* pvh_joins,
    SG_string* pstr_query,
    const char* pidState,
    SG_bool* pb_usernames
    );

/*
 * ** SQLite user defined function to use with matchinfo() to calculate the
 * ** relevancy of an FTS match. The value returned is the relevancy score
 * ** (a real value greater than or equal to zero). A larger value indicates 
 * ** a more relevant document.
 * **
 * ** The overall relevancy returned is the sum of the relevancies of each 
 * ** column value in the FTS table. The relevancy of a column value is the
 * ** sum of the following for each reportable phrase in the FTS query:
 * **
 * **   (<hit count> / <global hit count>) * <column weight>
 * **
 * ** where <hit count> is the number of instances of the phrase in the
 * ** column value of the current row and <global hit count> is the number
 * ** of instances of the phrase in the same column of all rows in the FTS
 * ** table. The <column weight> is a weighting factor assigned to each
 * ** column by the caller (see below).
 * **
 * ** The first argument to this function must be the return value of the FTS 
 * ** matchinfo() function. Following this must be one argument for each column 
 * ** of the FTS table containing a numeric weight factor for the corresponding 
 * ** column. Example:
 * **
 * **     CREATE VIRTUAL TABLE documents USING fts3(title, content)
 * **
 * ** The following query returns the docids of documents that match the full-text
 * ** query <query> sorted from most to least relevant. When calculating
 * ** relevance, query term instances in the 'title' column are given twice the
 * ** weighting of those in the 'content' column.
 * **
 * **     SELECT docid FROM documents 
 * **     WHERE documents MATCH <query> 
 * **     ORDER BY rank(matchinfo(documents), 1.0, 0.5) DESC
 * */
static void sg_ft_rank(sqlite3_context *pCtx, int nVal, sqlite3_value **apVal)
{
    unsigned int *aMatchinfo;                /* Return value of matchinfo() */
    int nCol;                       /* Number of columns in the table */
    int nPhrase;                    /* Number of phrases in the query */
    int iPhrase;                    /* Current phrase */
    double score = 0.0;             /* Value to return */

    assert( sizeof(int)==4 );

    /* Check that the number of arguments passed to this function is correct.
    ** If not, jump to wrong_number_args. Set aMatchinfo to point to the array
    ** of unsigned integer values returned by FTS function matchinfo. Set
    ** nPhrase to contain the number of reportable phrases in the users full-text
    ** query, and nCol to the number of columns in the table.
    */
    if( nVal<1 ) goto wrong_number_args;

    aMatchinfo = (unsigned int *)sqlite3_value_blob(apVal[0]);
    nPhrase = aMatchinfo[0];
    nCol = aMatchinfo[1];
    if( nVal!=(1+nCol) ) goto wrong_number_args;

    /* Iterate through each phrase in the users query. */
    for(iPhrase=0; iPhrase<nPhrase; iPhrase++)
    {
        int iCol;                     /* Current column */

        /* Now iterate through each column in the users query. For each column,
        ** increment the relevancy score by:
        **
        **   (<hit count> / <global hit count>) * <column weight>
        **
        ** aPhraseinfo[] points to the start of the data for phrase iPhrase. So
        ** the hit count and global hit counts for each column are found in 
        ** aPhraseinfo[iCol*3] and aPhraseinfo[iCol*3+1], respectively.
        */
        unsigned int *aPhraseinfo = &aMatchinfo[2 + iPhrase*nCol*3];

        for(iCol=0; iCol<nCol; iCol++)
        {
            int nHitCount = aPhraseinfo[3*iCol];
            int nGlobalHitCount = aPhraseinfo[3*iCol+1];
            double weight = sqlite3_value_double(apVal[iCol+1]);
            if( nHitCount>0 )
            {
                score += ((double)nHitCount / (double)nGlobalHitCount) * weight;
            }
        }
    }

    sqlite3_result_double(pCtx, score);
    return;

    /* Jump here if the wrong number of arguments are passed to this function */
wrong_number_args:
    sqlite3_result_error(pCtx, "wrong number of arguments to function rank()", -1);
}

void sg_dbndx__get_filters_dir_path(
	SG_context* pCtx,
    SG_pathname* pPath_dbndx_file,
    SG_pathname** ppPath
    )
{
    SG_pathname* pPath_filters_dir = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_filters_dir, pPath_dbndx_file)  );
	SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPath_filters_dir)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath_filters_dir, "f")  );

    *ppPath = pPath_filters_dir;
    pPath_filters_dir = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_filters_dir);
}

typedef struct
{
    SG_vhash* pvh;
    SG_uint64 dagnum;
    SG_pathname* pPath_filters_dir;
} dir_each_data;

static void dir_each(
	SG_context *      pCtx,             //< [in] [out] Error and context info.
	const SG_string * pStringEntryName, //< [in] Name of the current entry.
	SG_fsobj_stat *   pfsStat,          //< [in] Stat data for the current entry (only if SG_DIR__FOREACH__STAT is used).
	void *            pVoidData)        //< [in] Data provided by the caller of SG_dir__foreach.
{
	dir_each_data *ctx = (dir_each_data *)pVoidData;
    SG_int32 gen = -1;
    const char* psz_csid = NULL;
    SG_pathname* pPath = NULL;

    SG_UNUSED(pfsStat);

    SG_ERR_CHECK(  sg_dbndx__parse_filter_filename(pCtx, ctx->dagnum, SG_string__sz(pStringEntryName), &gen, &psz_csid)  );

    if (psz_csid)
    {
        SG_vhash* pvh_f = NULL;
        SG_fsobj_stat st;
        SG_bool b_exists = SG_FALSE;

        // only if the filename matches as a filter do we stat it

        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath, ctx->pPath_filters_dir)  );
        SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPath,SG_string__sz(pStringEntryName))  );
        SG_ERR_CHECK(  SG_fsobj__exists_stat__pathname(pCtx, pPath, &b_exists, &st)  );

        if (b_exists)
        {
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, ctx->pvh, psz_csid, &pvh_f)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_f, "gen", (SG_int64) gen)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_f, "modtime", st.mtime_ms)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_f, "path", SG_pathname__sz(pPath))  );
        }
    }

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void sg_dbndx__list_all_filters(
        SG_context* pCtx, 
        SG_pathname* pPath_filters_dir,
        SG_uint64 dagnum,
        SG_vhash** ppvh
        )
{
    dir_each_data ctx;

    ctx.pPath_filters_dir = pPath_filters_dir;
    ctx.dagnum = dagnum;
    ctx.pvh = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &ctx.pvh)  );
	SG_ERR_CHECK(  SG_dir__foreach(
                pCtx, 
                pPath_filters_dir, 
                SG_DIR__FOREACH__SKIP_SELF | SG_DIR__FOREACH__SKIP_PARENT,
								   dir_each, &ctx)
				);
    *ppvh = ctx.pvh;
    ctx.pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, ctx.pvh);
}

void SG_dbndx__remove(
	SG_context* pCtx,
	SG_uint64 dagnum,
	SG_pathname* pPath
	)
{
    SG_pathname* pPath_filters_dir = NULL;
    SG_pathname* pPath_one_filter = NULL;
    SG_vhash* pvh_filters = NULL;
    SG_uint32 i = 0;
    SG_uint32 count_filters = 0;

    // TODO make this function concurrency safe, especially on Windows

    SG_ERR_CHECK( sg_dbndx__get_filters_dir_path(pCtx, pPath, &pPath_filters_dir)  );

    SG_ERR_CHECK(  sg_dbndx__list_all_filters(pCtx, pPath_filters_dir, dagnum, &pvh_filters)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_filters, &count_filters)  );
    for (i=0; i<count_filters; i++)
    {
        SG_vhash* pvh_f = NULL;
        const char* psz_csid = NULL;
        const char* psz_path = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_filters, i, &psz_csid, &pvh_f)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_f, "path", &psz_path)  );
        SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath_one_filter, psz_path)  );
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_one_filter)  );
        SG_PATHNAME_NULLFREE(pCtx, pPath_one_filter);
    }

    SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_filters);
    SG_PATHNAME_NULLFREE(pCtx, pPath_filters_dir);
    SG_PATHNAME_NULLFREE(pCtx, pPath_one_filter);
}

void SG_dbndx_query__open(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    sqlite3* psql,
	SG_pathname* pPath,
	SG_dbndx_query** ppNew
	)
{
	SG_dbndx_query* pndx = NULL;
    SG_bool b_exists = SG_FALSE;
    int rc = -1;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pndx)  );

    pndx->pRepo = pRepo;
    pndx->iDagNum = iDagNum;
	pndx->bInTransaction = SG_FALSE;

    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );

    if (!b_exists)
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    pndx->psql = psql;
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pndx->pPath_me, pPath)  );

    SG_ERR_CHECK( sg_dbndx__get_filters_dir_path(pCtx, pPath, &pndx->pPath_filters_dir)  );

    SG_fsobj__mkdir__pathname(pCtx, pndx->pPath_filters_dir);
    if (SG_context__err_equals(pCtx, SG_ERR_DIR_ALREADY_EXISTS))
    {
        SG_ERR_DISCARD;
    }
    else
    {
        SG_ERR_CHECK_CURRENT;
    }

    rc = sqlite3_create_function(
            pndx->psql,
            "sg_ft_rank",
            -1,
            SQLITE_UTF8,
            NULL,
            sg_ft_rank,
            NULL,
            NULL
            );
    if (rc)
    {
        SG_ERR_THROW2(SG_ERR_SQLITE(rc), (pCtx, "%s", sqlite3_errmsg(pndx->psql)));
    }

	*ppNew = pndx;
    pndx = NULL;

fail:
    SG_dbndx_query__free(pCtx, pndx);
}

void SG_dbndx_query__free(SG_context* pCtx, SG_dbndx_query* pdbc)
{
	if (!pdbc)
		return;

    SG_PATHNAME_NULLFREE(pCtx, pdbc->pPath_me);
    SG_PATHNAME_NULLFREE(pCtx, pdbc->pPath_filters_dir);

	SG_NULLFREE(pCtx, pdbc);
}

static void sg_dbnx__prepare_attach_stmt(
	SG_context* pCtx,
	sqlite3* psql,
	const SG_pathname* pPath,
	const char* pszAlias,
	sqlite3_stmt** ppStmt)
{
	sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "ATTACH DATABASE ? AS ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, SG_pathname__sz(pPath))  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, pszAlias)  );

	*ppStmt = pStmt;
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

/**
 * We might consider moving this down into sg_sqlite. 
 * ATTACH DATABASE statements occasionally fail with SQLITE_CANTOPEN errors on Windows, presumably
 * because the file is still being created by another thread or process (see B4934.) Because this
 * is somewhat unique to the way sg_dbndx creates and attaches state/filter databases, I'm leaving 
 * it here for now.
 */
static void sg_dbndx__sqlite_exec__retry_open(
	SG_context * pCtx, 
	sqlite3_stmt* pStmt, 
	SG_uint32 sleep_ms, 
	SG_uint32 timeout_ms
	)
{
	SG_uint32 slept = 0;

	while (1)
	{
		int rc = -1;

		SG_ERR_CHECK_RETURN(  sg_sqlite__step__nocheck__retry(pCtx, pStmt, &rc, sleep_ms, timeout_ms)  );

		if (SQLITE_DONE == rc)
		{
			break;
		}
		else if (SQLITE_CANTOPEN == rc)
		{
			if (slept > timeout_ms)
				SG_ERR_THROW_RETURN(  SG_ERR_SQLITE(rc)  );

			SG_sleep_ms(sleep_ms);
			slept += sleep_ms;
			continue;
		}
		else
		{
			SG_ERR_THROW_RETURN(  SG_ERR_SQLITE(rc)  );
		}
	}
}

static void sg_dbndx__attach_usernames(
        SG_context* pCtx, 
        SG_dbndx_query* pndx
        )
{
    SG_pathname* pPath_usernames = NULL;
	sqlite3_stmt* pStmtAttach = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_usernames, pndx->pPath_me)  );
	SG_ERR_CHECK(  SG_pathname__remove_last(pCtx, pPath_usernames)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPath_usernames, "usernames.sqlite3")  );

    SG_ERR_CHECK(  sg_dbnx__prepare_attach_stmt(pCtx, pndx->psql, pPath_usernames, "usernames", &pStmtAttach));
    SG_ERR_CHECK(  sg_dbndx__sqlite_exec__retry_open(pCtx, pStmtAttach, MY_SLEEP_MS, MY_TIMEOUT_MS)  );

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_usernames);
}

static void sg_dbndx__detach_usernames(
        SG_context* pCtx, 
        SG_dbndx_query* pndx
        )
{
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pndx->psql, "DETACH DATABASE usernames")  );

fail:
    ;
}

void sg_dbndx__stuff_recids_into_temp_table(
        SG_context* pCtx,
        sqlite3* psql,
        SG_varray* pva_recids,
        const char* psz_table_name
        )
{
    SG_uint32 count_vals = 0;
    SG_uint32 i_val = 0;
    sqlite3_stmt* pStmt = NULL;

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE TEMP TABLE %s (recid VARCHAR NOT NULL UNIQUE)", psz_table_name)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT INTO %s (recid) VALUES (?)", psz_table_name)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_recids, &count_vals)  );
    for (i_val=0; i_val<count_vals; i_val++)
    {
        const char* psz_val = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_recids, i_val, &psz_val)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_val)  );

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_dbndx_query__query_multiple_record_history(
        SG_context* pCtx,
        SG_dbndx_query* pdbc,
        const char* psz_rectype,
        SG_varray* pva_recids,
        const char* psz_field,
        SG_vhash** ppvh
        )
{
    int rc;
    SG_vhash* pvh = NULL;
	sqlite3_stmt* pStmt = NULL;
    char buf_table_recids[SG_TID_MAX_BUFFER_LENGTH];

    if (SG_DAGNUM__HAS_NO_RECID(pdbc->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_table_recids, sizeof(buf_table_recids))  );

    SG_ERR_CHECK(  sg_dbndx__attach_usernames(pCtx, pdbc)  );

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS ndx_%s_%s_%s_recid ON %s_%s (recid, hidrecrow, %s)", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, psz_field, SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, psz_field)  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_dbndx__stuff_recids_into_temp_table(pCtx, pdbc->psql, pva_recids, buf_table_recids)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

    if (SG_DAGNUM__USERS != pdbc->iDagNum)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pStmt, "SELECT r.recid, hidrecs.hidrec, csids.csid, csids.generation, db_audits.userid, db_audits.timestamp, shadow_users.name, r.%s FROM (db_history INNER JOIN %s_%s r ON (db_history.hidrecrow=r.hidrecrow) INNER JOIN hidrecs ON (hidrecs.hidrecrow=db_history.hidrecrow) INNER JOIN csids ON (db_history.csidrow=csids.csidrow) ) LEFT OUTER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) LEFT OUTER JOIN shadow_users ON (db_audits.userid = shadow_users.userid) WHERE (r.recid IN (SELECT recid FROM %s)) ORDER BY generation DESC", psz_field, SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, buf_table_recids)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pStmt, "SELECT r.recid, hidrecs.hidrec, csids.csid, csids.generation, db_audits.userid, db_audits.timestamp, NULL FROM (db_history INNER JOIN %s_%s r ON (db_history.hidrecrow=r.hidrecrow) INNER JOIN hidrecs ON (hidrecs.hidrecrow=db_history.hidrecrow) INNER JOIN csids ON (db_history.csidrow=csids.csidrow) ) LEFT OUTER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) WHERE (r.recid IN (SELECT recid FROM %s)) ORDER BY generation DESC", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, buf_table_recids)  );
    }

	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
        SG_varray* pva_cur_audits = NULL;
        SG_vhash* pvh_audit = NULL;
        SG_vhash* pvh_cur_csid = NULL;
        SG_vhash* pvh_changesets = NULL;

		const char* psz_recid = (const char*) sqlite3_column_text(pStmt, 0);
		const char* psz_hid_rec = (const char*) sqlite3_column_text(pStmt, 1);
		const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 2);
		SG_int32 generation = sqlite3_column_int(pStmt, 3);
		const char* psz_userid = (const char*) sqlite3_column_text(pStmt, 4);
		SG_int64 timestamp = sqlite3_column_int64(pStmt, 5);
		const char* psz_username = (const char*) sqlite3_column_text(pStmt, 6);
		const char* psz_value = (const char*) sqlite3_column_text(pStmt, 7);

        SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh, psz_recid, &pvh_changesets)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changesets, psz_csid, &pvh_cur_csid)  );
        if (!pvh_cur_csid)
        {
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_changesets, psz_csid, &pvh_cur_csid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur_csid, SG_ZING_FIELD__HIDREC, psz_hid_rec)  );
            if (psz_value)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur_csid, psz_field, psz_value)  );
            }
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_cur_csid, "generation", (SG_int64) generation)  );
            SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_cur_csid, "audits", &pva_cur_audits)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_cur_csid, "audits", &pva_cur_audits)  );
        }

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_cur_audits, &pvh_audit)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_audit, SG_AUDIT__USERID, psz_userid)  );
        if (psz_username)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_audit, SG_AUDIT__USERNAME, psz_username)  );
        }
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_audit, SG_AUDIT__TIMESTAMP, timestamp)  );
	}

	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_FALSE;
    SG_ERR_IGNORE(  sg_dbndx__detach_usernames(pCtx, pdbc)  );

    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_dbndx_query__query_record_history(
        SG_context* pCtx,
        SG_dbndx_query* pdbc,
        const char* psz_recid,
        const char* psz_rectype,
        SG_varray** ppva
        )
{
    int rc;
    SG_varray* pva = NULL;
	sqlite3_stmt* pStmt = NULL;
    SG_vhash* pvh_cur_csid = NULL;

    if (SG_DAGNUM__HAS_NO_RECID(pdbc->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  );
    }

    SG_ERR_CHECK(  sg_dbndx__attach_usernames(pCtx, pdbc)  );
    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

    if (SG_DAGNUM__USERS != pdbc->iDagNum)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pStmt, "SELECT hidrecs.hidrec, csids.csid, csids.generation, db_audits.userid, db_audits.timestamp, shadow_users.name FROM (db_history INNER JOIN %s_%s r ON (db_history.hidrecrow=r.hidrecrow) INNER JOIN hidrecs ON (hidrecs.hidrecrow=db_history.hidrecrow) INNER JOIN csids ON (db_history.csidrow=csids.csidrow) ) LEFT OUTER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) LEFT OUTER JOIN shadow_users ON (db_audits.userid = shadow_users.userid) WHERE (r.recid = ?) ORDER BY generation DESC", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pStmt, "SELECT hidrecs.hidrec, csids.csid, csids.generation, db_audits.userid, db_audits.timestamp, NULL FROM (db_history INNER JOIN %s_%s r ON (db_history.hidrecrow=r.hidrecrow) INNER JOIN hidrecs ON (hidrecs.hidrecrow=db_history.hidrecrow) INNER JOIN csids ON (db_history.csidrow=csids.csidrow) ) LEFT OUTER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) WHERE (r.recid = ?) ORDER BY generation DESC", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype)  );
    }
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_recid)  );

	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
        SG_bool b_new_csid = SG_FALSE;
        SG_varray* pva_cur_audits = NULL;
        SG_vhash* pvh_audit = NULL;

		const char* psz_hid_rec = (const char*) sqlite3_column_text(pStmt, 0);
		const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 1);
		SG_int32 generation = sqlite3_column_int(pStmt, 2);
		const char* psz_userid = (const char*) sqlite3_column_text(pStmt, 3);
		SG_int64 timestamp = sqlite3_column_int64(pStmt, 4);
		const char* psz_username = (const char*) sqlite3_column_text(pStmt, 5);

        if (pvh_cur_csid)
        {
            const char* psz_cur_csid = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_cur_csid, "csid", &psz_cur_csid)  );
            if (0 != strcmp(psz_cur_csid, psz_csid))
            {
                b_new_csid = SG_TRUE;
            }
        }
        else
        {
            b_new_csid = SG_TRUE;
        }

        if (b_new_csid)
        {
            SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh_cur_csid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur_csid, SG_ZING_FIELD__HIDREC, psz_hid_rec)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur_csid, "csid", psz_csid)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_cur_csid, "generation", (SG_int64) generation)  );
            SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_cur_csid, "audits", &pva_cur_audits)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_cur_csid, "audits", &pva_cur_audits)  );
        }

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_cur_audits, &pvh_audit)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_audit, SG_AUDIT__USERID, psz_userid)  );
        if (psz_username)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_audit, SG_AUDIT__USERNAME, psz_username)  );
        }
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_audit, SG_AUDIT__TIMESTAMP, timestamp)  );
	}

	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppva = pva;
    pva = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_FALSE;
    SG_ERR_IGNORE(  sg_dbndx__detach_usernames(pCtx, pdbc)  );

    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void sg_dbndx__remove_filter(
        SG_context* pCtx, 
        SG_dbndx_query* pndx,
        const char* psz_csid
        )
{
    SG_dagnode* pdn = NULL;
    SG_pathname* pPath = NULL;
    SG_int32 gen = -1;

    SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid, &pdn)  );
    SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
    SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid, gen, &pPath)  );
    SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
}

static void sg_dbndx__cleanup_old_filters(
        SG_context* pCtx, 
        SG_dbndx_query* pndx,
        SG_vhash* pvh_filters
        )
{
    SG_uint32 count_filters = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_filters, &count_filters)  );

    // we don't start deleting until we have more than 16 of them

    if (count_filters > 16)
    {
        SG_uint32 i = 0;
        SG_int64 cmp_gen = -1;
        SG_int64 cmp_modtime = -1;

        // sort by modtime DESC
        SG_ERR_CHECK(  SG_vhash__sort__vhash_field_int__desc(pCtx, pvh_filters, "modtime")  );

        // we're going to keep the most recent 16 filters, plus anything
        // within 2 generations or 1 minute.

        for (i=0; i<16; i++)
        {
            SG_vhash* pvh_f = NULL;
            SG_int64 gen = -1;
            SG_int64 modtime = -1;
            const char* psz_csid = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_filters, i, &psz_csid, &pvh_f)  );
            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_f, "gen", &gen)  );
            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_f, "modtime", &modtime)  );

            if (
                    (cmp_gen < 0)
                    || (gen < cmp_gen)
               )
            {
                cmp_gen = gen;
            }

            if (
                    (cmp_modtime < 0)
                    || (modtime < cmp_modtime)
               )
            {
                cmp_modtime = modtime;
            }
        }

        for (i=16; i<count_filters; i++)
        {
            SG_vhash* pvh_f = NULL;
            SG_int64 gen = -1;
            SG_int64 modtime = -1;
            const char* psz_csid = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_filters, i, &psz_csid, &pvh_f)  );
            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_f, "gen", &gen)  );
            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_f, "modtime", &modtime)  );

            if (gen > (cmp_gen - 2))
            {
                continue;
            }

            if (modtime > (cmp_modtime - (60 * 1000)))
            {
                continue;
            }

            SG_ERR_CHECK(  sg_dbndx__remove_filter(pCtx, pndx, psz_csid)  );
        }
    }

fail:
    ;
}

static void sg_dbndx__create_indexes(SG_context* pCtx, sqlite3* psql, const char* pidState, const char* psz_rectype, SG_vhash* pvh_rectype)
{
    SG_uint32 count_fields = 0;
    SG_uint32 i_field = 0;

    SG_NULLARGCHECK_RETURN(psql);

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rectype, &count_fields)  );
    for (i_field=0; i_field<count_fields; i_field++)
    {
        const char* psz_field = NULL;
        SG_vhash* pvh_field = NULL;
        SG_bool b_index = SG_FALSE;
        SG_uint16 type = 0;
        const char* psz_field_type = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_rectype, i_field, &psz_field, &pvh_field)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );
        SG_ERR_CHECK(  SG_zing__field_type__sz_to_uint16(pCtx, psz_field_type, &type)  );
        // never index a bool field
        if (SG_ZING_TYPE__BOOL != type)
        {
            SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__INDEX, &b_index)  );
            if (b_index)
            {
                SG_vhash* pvh_orderings = NULL;
                SG_bool b_unique = SG_FALSE;

                if (0 == strcmp(psz_field, SG_ZING_FIELD__RECID))
                {
                    b_unique = SG_TRUE;
                }
                else
                {
                    SG_bool b_has = SG_FALSE;
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__UNIQUE, &b_has)  );
                    if (b_has)
                    {
                        SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__UNIQUE, &b_unique)  );
                    }
                }

                SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE %s INDEX IF NOT EXISTS \"index$%s_%s_%s\" ON \"%s%s_%s\" (\"%s\")",
                            b_unique ? "UNIQUE" : "",
                            pidState, 
                            psz_rectype,
                            psz_field,
                            SG_DBNDX_RECORD_TABLE_PREFIX,
                            pidState, 
                            psz_rectype,
                            psz_field
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
                        SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE INDEX IF NOT EXISTS \"index$%s_%s_ordering_%s$_%s\" ON \"%s%s_%s\" (\"ordering_%s$_%s\")",
                                    pidState,
                                    psz_rectype,
                        psz_key,
                                    psz_field,
                                    SG_DBNDX_RECORD_TABLE_PREFIX,
                                    pidState,
                                    psz_rectype,
                        psz_key,
                                    psz_field)  );
                    }
                }
            }
        }
    }

fail:
    ;
}

static void sg_dbndx__schema_column_list_in_sql(
	SG_context* pCtx,
    SG_vhash* pvh_one_rectype,
    SG_string** ppstr
    )
{
    SG_uint32 count_fields = 0;
    SG_uint32 i = 0;
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_one_rectype, &count_fields)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, "x.hidrecrow")  );
    for (i=0; i<count_fields; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_name = NULL;
        SG_vhash* pvh_orderings = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_one_rectype, i, &psz_name, &pvh_column)  );
        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, ", x.\"%s\"", psz_name)  );
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_column, SG_DBNDX_SCHEMA__FIELD__ORDERINGS, &pvh_orderings)  );
        if (pvh_orderings)
        {
            SG_uint32 count_orderings = 0;
            SG_uint32 i_ordering = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_orderings, &count_orderings)  );
            for (i_ordering=0; i_ordering<count_orderings; i_ordering++)
            {
                const char* psz_key = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_orderings, i_ordering, &psz_key, NULL)  );
                SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, ", x.\"ordering_%s$_%s\"", psz_key, psz_name)  );
            }
        }
    }

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__create_state_filter__from_root(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_vhash* pvh_schema,
        const char* psz_csid,
        SG_int32 gen
        )
{
    SG_vhash* pvh_add = NULL;
    SG_pathname* pPath_tid = NULL;
    SG_pathname* pPath_new = NULL;
    sqlite3* psql = NULL;
    char buf_tid[SG_TID_MAX_BUFFER_LENGTH];
	sqlite3_stmt* pStmtAttach = NULL;
    sqlite3_stmt* pStmt = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_uint32 i = 0;
    SG_uint32 count_rectypes = 0;
    SG_string* pstr_sql = NULL;
    SG_string* pstr_schema = NULL;
    SG_string* pstr_columns = NULL;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(psz_csid);

    // initially we create this state filter in a temp file
    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_tid, sizeof(buf_tid))  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_tid, pndx->pPath_filters_dir)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPath_tid,buf_tid)  );

    SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath_tid, SG_SQLITE__SYNC__OFF, &psql)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "PRAGMA journal_mode=OFF")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "PRAGMA temp_store=2")  ); // TODO really?  on the iPad?

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_schema)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh_schema, pstr_schema)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE state_schema (json VARCHAR NOT NULL)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT INTO state_schema (json) VALUES (?)")  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, SG_string__sz(pstr_schema))  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_STRING_NULLFREE(pCtx, pstr_schema);

    SG_ERR_CHECK(  sg_dbndx__create_record_tables_from_schema(pCtx, psql, pvh_schema, psz_csid)  );

    SG_ERR_CHECK(  SG_repo__db__calc_delta_from_root(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid, SG_REPO__MAKE_DELTA_FLAG__ROWIDS, &pvh_add)  );

    if (pvh_add)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count)  );

        if (count)
        {
            SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TEMPORARY TABLE tmpadd (hidrecrow INTEGER NOT NULL UNIQUE)")  );

            SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT OR IGNORE INTO tmpadd (hidrecrow) VALUES (?)")  );
            for (i=0; i<count; i++)
            {
                const char* psz_hidrecrow = NULL;
                SG_int64 hidrecrow = -1;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i, &psz_hidrecrow, NULL)  );
                SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &hidrecrow, psz_hidrecrow)  );

                SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
                SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, hidrecrow)  );

                SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
            }
            SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

			SG_ERR_CHECK(  sg_dbnx__prepare_attach_stmt(pCtx, psql, pndx->pPath_me, "main_ndx", &pStmtAttach));
			SG_ERR_CHECK(  sg_dbndx__sqlite_exec__retry_open(pCtx, pStmtAttach, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
			SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
            SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
            for (i=0; i<count_rectypes; i++)
            {
                SG_vhash* pvh_one_rectype = NULL;
                const char* psz_rectype = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

                SG_ERR_CHECK(  sg_dbndx__schema_column_list_in_sql(pCtx, pvh_one_rectype, &pstr_columns)  );
                SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "INSERT INTO %s%s_%s SELECT %s FROM %s_%s x INNER JOIN tmpadd ON (x.hidrecrow = tmpadd.hidrecrow)", SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype, SG_string__sz(pstr_columns), SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype)  );
                SG_STRING_NULLFREE(pCtx, pstr_columns);

                SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
            }
            SG_STRING_NULLFREE(pCtx, pstr_sql);

            // now detach the main_ndx db
            SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DETACH DATABASE main_ndx")  );
        }
    }

    // and create indexes on all the tables we just created

    for (i=0; i<count_rectypes; i++)
    {
        SG_vhash* pvh_one_rectype = NULL;
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  sg_dbndx__create_indexes(pCtx, psql, psz_csid, psz_rectype, pvh_one_rectype)  );
    }

	SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
    psql = NULL;

    // now rename the file
    SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid, gen, &pPath_new)  );
    // we IGNORE here because if it fails, it is probably not a problem
	SG_ERR_IGNORE(  SG_fsobj__move__pathname_pathname(pCtx, pPath_tid, pPath_new)  );
    //SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPath_new, 0400)  ); // TODO 0444 ?

    // but if the move failed, we need to cleanup the TID file
    b_exists = SG_FALSE;
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_tid, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_tid)  );

        // since the rename failed, we assume the filter file already existed
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_new, &b_exists, NULL, NULL)  );
        if (!b_exists)
        {
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_columns);
    SG_STRING_NULLFREE(pCtx, pstr_schema);
    SG_STRING_NULLFREE(pCtx, pstr_sql);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_tid);
    SG_PATHNAME_NULLFREE(pCtx, pPath_new);
    SG_VHASH_NULLFREE(pCtx, pvh_add);
}

static void sg_dbndx__copy_file(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_pathname* pPath_base,
        SG_pathname** ppPath_result
        )
{
    char buf_tid[SG_TID_MAX_BUFFER_LENGTH];
    SG_pathname* pPath_tid = NULL;

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_tid, sizeof(buf_tid))  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath_tid, pndx->pPath_filters_dir)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx,pPath_tid,buf_tid)  );

    SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pPath_base, pPath_tid, 0666)  );

    *ppPath_result = pPath_tid;
    pPath_tid = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_tid);
}

static void sg_dbndx__create_state_filter__from_base(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_vhash* pvh_schema,
        SG_changeset* pcs,
        SG_changeset* pcs_base,
        SG_pathname* pPath_temp_copy
        )
{
    SG_pathname* pPath_new = NULL;
    sqlite3* psql = NULL;
	sqlite3_stmt* pStmtAttach = NULL;
    sqlite3_stmt* pStmt = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_uint32 i = 0;
    SG_uint32 count_rectypes = 0;
    SG_string* pstr_sql = NULL;
    SG_string* pstr_columns = NULL;
    const char* psz_csid = NULL;
    SG_int32 gen = -1;
    const char* psz_csid_base = NULL;
    SG_int32 gen_base = -1;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(pcs_base);

    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs, &psz_csid)  );
    SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs, &gen)  );
    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs_base, &psz_csid_base)  );
    SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs_base, &gen_base)  );

    SG_ERR_CHECK(  SG_repo__db__calc_delta(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid_base, psz_csid, SG_REPO__MAKE_DELTA_FLAG__ROWIDS, &pvh_add, &pvh_remove)  );

    SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath_temp_copy, SG_SQLITE__SYNC__OFF, &psql)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "PRAGMA journal_mode=OFF")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "PRAGMA temp_store=2")  ); // TODO really?  on the iPad?
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DROP TABLE IF EXISTS members")  ); // TODO remove this eventually

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
    for (i=0; i<count_rectypes; i++)
    {
        SG_vhash* pvh_one_rectype = NULL;
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "ALTER TABLE %s%s_%s RENAME TO %s%s_%s", 
                    SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid_base, psz_rectype,
                    SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype
                    )  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
    }
    SG_STRING_NULLFREE(pCtx, pstr_sql);

    if (pvh_remove)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_remove, &count)  );

        if (count)
        {
            {
                SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TEMPORARY TABLE tmpremove (hidrecrow INTEGER NOT NULL UNIQUE)")  );

                SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT OR IGNORE INTO tmpremove (hidrecrow) VALUES (?)")  );
                for (i=0; i<count; i++)
                {
                    const char* psz_hidrecrow = NULL;
                    SG_int64 hidrecrow = -1;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_remove, i, &psz_hidrecrow, NULL)  );
                    SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &hidrecrow, psz_hidrecrow)  );

                    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
                    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

                    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, hidrecrow)  );

                    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
                }
                SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

                SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
                for (i=0; i<count_rectypes; i++)
                {
                    SG_vhash* pvh_one_rectype = NULL;
                    const char* psz_rectype = NULL;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "DELETE FROM %s%s_%s WHERE hidrecrow IN (SELECT hidrecrow FROM tmpremove)", SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype)  );
                    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
                }
                SG_STRING_NULLFREE(pCtx, pstr_sql);
            }
        }
    }

    if (pvh_add)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count)  );

        if (count)
        {
            {
                SG_uint32 count_inserted = 0;
                SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TEMPORARY TABLE tmpadd (hidrecrow INTEGER NOT NULL UNIQUE)")  );

                SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT OR IGNORE INTO tmpadd (hidrecrow) VALUES (?)")  );
                for (i=0; i<count; i++)
                {
                    const char* psz_hidrecrow = NULL;
                    SG_int64 hidrecrow = -1;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i, &psz_hidrecrow, NULL)  );
                    SG_ERR_CHECK(  SG_int64__parse__strict(pCtx, &hidrecrow, psz_hidrecrow)  );

                    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
                    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

                    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, hidrecrow)  );

                    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
                }
                SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

				SG_ERR_CHECK(  sg_dbnx__prepare_attach_stmt(pCtx, psql, pndx->pPath_me, "main_ndx", &pStmtAttach));
				SG_ERR_CHECK(  sg_dbndx__sqlite_exec__retry_open(pCtx, pStmtAttach, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
				SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
                SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
                for (i=0; i<count_rectypes; i++)
                {
                    SG_vhash* pvh_one_rectype = NULL;
                    const char* psz_rectype = NULL;
                    SG_uint32 num_just_inserted = 0;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

                    SG_ERR_CHECK(  sg_dbndx__schema_column_list_in_sql(pCtx, pvh_one_rectype, &pstr_columns)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "INSERT INTO %s%s_%s SELECT %s FROM tmpadd INNER JOIN %s_%s x ON (x.hidrecrow = tmpadd.hidrecrow)", SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype, SG_string__sz(pstr_columns), SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype)  );
                    SG_STRING_NULLFREE(pCtx, pstr_columns);

                    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
                    SG_ERR_CHECK(  sg_sqlite__num_changes(pCtx, psql, &num_just_inserted)  );
                    count_inserted += num_just_inserted;
                    if (count == count_inserted)
                    {
                        // This is safe because a given record is going to get inserted into
                        // exactly one rectype's table.
                        break;
                    }
                }
                SG_STRING_NULLFREE(pCtx, pstr_sql);
                SG_ASSERT(count == count_inserted);

                // now detach the main_ndx db
                SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DETACH DATABASE main_ndx")  );
            }
        }
    }

	SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );

    // now rename the file
    SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid, gen, &pPath_new)  );
    // we IGNORE here because if it fails, it is probably not a problem
	SG_ERR_IGNORE(  SG_fsobj__move__pathname_pathname(pCtx, pPath_temp_copy, pPath_new)  );
    //SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPath_new, 0400)  ); // TODO 0444 ?

    // but if the move failed, we need to cleanup the TID file
    b_exists = SG_FALSE;
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_temp_copy, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_temp_copy)  );

        // since the rename failed, we assume the filter file already existed.  verify this.
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_new, &b_exists, NULL, NULL)  );
        if (!b_exists)
        {
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_columns);
    SG_STRING_NULLFREE(pCtx, pstr_sql);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_new);
    SG_VHASH_NULLFREE(pCtx, pvh_add);
    SG_VHASH_NULLFREE(pCtx, pvh_remove);
}

static void sg_dbndx__create_state_filter__from_parent(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_vhash* pvh_schema,
        SG_changeset* pcs,
        SG_changeset* pcs_base,
        SG_pathname* pPath_temp_copy
        )
{
    SG_pathname* pPath_new = NULL;
    sqlite3* psql = NULL;
	sqlite3_stmt* pStmtAttach = NULL;
    sqlite3_stmt* pStmt = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_uint32 i = 0;
    SG_uint32 count_rectypes = 0;
    SG_string* pstr_sql = NULL;
    SG_string* pstr_columns = NULL;
    const char* psz_csid = NULL;
    SG_int32 gen = -1;
    const char* psz_csid_base = NULL;
    SG_int32 gen_base = -1;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pcs);
	SG_NULLARGCHECK_RETURN(pcs_base);

    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs, &psz_csid)  );
    SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs, &gen)  );
    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs_base, &psz_csid_base)  );
    SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs_base, &gen_base)  );

    {
        SG_vhash* pvh_changes = NULL;
        SG_vhash* pvh_one_parent_changes = NULL;

        SG_ERR_CHECK(  SG_changeset__db__get_changes(pCtx, pcs, &pvh_changes)  );

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_changes, psz_csid_base, &pvh_one_parent_changes)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_parent_changes, "add", &pvh_add)  );
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_parent_changes, "remove", &pvh_remove)  );
    }

    SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath_temp_copy, SG_SQLITE__SYNC__OFF, &psql)  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "PRAGMA journal_mode=OFF")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "PRAGMA temp_store=2")  ); // TODO really?  on the iPad?
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DROP TABLE IF EXISTS members")  ); // TODO remove this eventually

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
    for (i=0; i<count_rectypes; i++)
    {
        SG_vhash* pvh_one_rectype = NULL;
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "ALTER TABLE %s%s_%s RENAME TO %s%s_%s", 
                    SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid_base, psz_rectype,
                    SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype
                    )  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
    }
    SG_STRING_NULLFREE(pCtx, pstr_sql);

	SG_ERR_CHECK(  sg_dbnx__prepare_attach_stmt(pCtx, psql, pndx->pPath_me, "main_ndx", &pStmtAttach));
	SG_ERR_CHECK(  sg_dbndx__sqlite_exec__retry_open(pCtx, pStmtAttach, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
	
	if (pvh_remove)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_remove, &count)  );

        if (count)
        {
            {
                SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
                for (i=0; i<count; i++)
                {
                    const char* psz_hidrec = NULL;
                    SG_uint32 i_rectype = 0;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_remove, i, &psz_hidrec, NULL)  );
                    for (i_rectype=0; i_rectype<count_rectypes; i_rectype++)
                    {
                        SG_vhash* pvh_one_rectype = NULL;
                        const char* psz_rectype = NULL;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i_rectype, &psz_rectype, &pvh_one_rectype)  );

                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "DELETE FROM %s%s_%s WHERE hidrecrow = (SELECT hidrecrow FROM hidrecs WHERE hidrec='%s')", SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype, psz_hidrec)  );
                        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
                        // TODO if this query deleted a row, then we can break the loop, right?
                    }
                }
                SG_STRING_NULLFREE(pCtx, pstr_sql);
            }
        }
    }

    if (pvh_add)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count)  );

        if (count)
        {
            {
                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );


                SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr_sql)  );
                for (i=0; i<count; i++)
                {
                    const char* psz_hidrec = NULL;
                    SG_uint32 i_rectype = 0;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i, &psz_hidrec, NULL)  );

                    for (i_rectype=0; i_rectype<count_rectypes; i_rectype++)
                    {
                        SG_vhash* pvh_one_rectype = NULL;
                        const char* psz_rectype = NULL;
                        SG_uint32 num_just_inserted = 0;

                        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i_rectype, &psz_rectype, &pvh_one_rectype)  );

                        SG_ERR_CHECK(  sg_dbndx__schema_column_list_in_sql(pCtx, pvh_one_rectype, &pstr_columns)  );
                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_sql, "INSERT INTO %s%s_%s SELECT %s FROM %s_%s x, hidrecs h WHERE x.hidrecrow = h.hidrecrow AND h.hidrec = '%s'", SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype, SG_string__sz(pstr_columns), SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, psz_hidrec)  );
                        SG_STRING_NULLFREE(pCtx, pstr_columns);

                        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, SG_string__sz(pstr_sql))  );
                        SG_ERR_CHECK(  sg_sqlite__num_changes(pCtx, psql, &num_just_inserted)  );
                        if (num_just_inserted)
                        {
                            // This is safe because a given record is going to get inserted into
                            // exactly one rectype's table.
                            break;
                        }
                    }
                }
                SG_STRING_NULLFREE(pCtx, pstr_sql);
            }
        }
    }

    // now detach the main_ndx db
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DETACH DATABASE main_ndx")  );

	SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );

    // now rename the file
    SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid, gen, &pPath_new)  );
    // we IGNORE here because if it fails, it is probably not a problem
	SG_ERR_IGNORE(  SG_fsobj__move__pathname_pathname(pCtx, pPath_temp_copy, pPath_new)  );
    //SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPath_new, 0400)  ); // TODO 0444 ?

    // but if the move failed, we need to cleanup the TID file
    b_exists = SG_FALSE;
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_temp_copy, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_temp_copy)  );

        // since the rename failed, we assume the filter file already existed.  verify this.
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_new, &b_exists, NULL, NULL)  );
        if (!b_exists)
        {
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_columns);
    SG_STRING_NULLFREE(pCtx, pstr_sql);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_new);
}

static void sg_dbndx__find_base_state__parent(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_changeset* pcs,
        SG_changeset** ppcs,
        SG_pathname** ppPath
        )
{
    SG_uint32 count_parents = 0;
    SG_varray* pva_parents = NULL;
    SG_uint32 i = 0;
    SG_changeset* pcs_parent = NULL;
    SG_pathname* pPath_filter = NULL;
    SG_pathname* pPath_temp_copy = NULL;
    const char* psz_template = NULL;

    SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_template)  );
    SG_ERR_CHECK(  SG_changeset__get_parents(pCtx, pcs, &pva_parents)  );
    if (pva_parents)
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count_parents)  );
    }
    for (i=0; i<count_parents; i++)
    {
        SG_bool b_exists = SG_FALSE;
        const char* psz_csid_parent = NULL;
        const char* psz_template_parent = NULL;
        SG_bool b_compatible_template = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz_csid_parent)  );
        SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pndx->pRepo, psz_csid_parent, &pcs_parent)  );

        if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pndx->iDagNum))
        {
            b_compatible_template = SG_TRUE;
        }
        else
        {
            SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs_parent, &psz_template_parent)  );
            if (0 == strcmp(psz_template, psz_template_parent))
            {
                b_compatible_template = SG_TRUE;
            }
        }

        if (b_compatible_template)
        {
            SG_int32 gen_parent = -1;
            SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs_parent, &gen_parent) );
            SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid_parent, gen_parent, &pPath_filter)  );

            SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_filter, &b_exists, NULL, NULL)  );
            if (b_exists)
            {
                sg_dbndx__copy_file(pCtx, pndx, pPath_filter, &pPath_temp_copy);
                if (SG_CONTEXT__HAS_ERR(pCtx))
                {
                    SG_context__err_reset(pCtx);
                }
                else
                {
                    break;
                }
            }
        }

        SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
        SG_CHANGESET_NULLFREE(pCtx, pcs_parent);
    }

    *ppcs = pcs_parent;
    pcs_parent = NULL;

    *ppPath = pPath_temp_copy;
    pPath_temp_copy = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_copy);
    SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
    SG_CHANGESET_NULLFREE(pCtx, pcs_parent);
}

static void sg_dbndx__find_base_state__ancestor__recursive(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_changeset* pcs,
        SG_vhash* pvh_filters,
        SG_uint32 depth,
        SG_vhash* pvh_already,
        SG_changeset** ppcs,
        SG_pathname** ppPath
        )
{
    SG_uint32 count_parents = 0;
    SG_uint32 i = 0;
    SG_changeset* pcs_parent = NULL;
    SG_changeset* pcs_result = NULL;
    const char* psz_csid = NULL;
    SG_bool b_already = SG_FALSE;
    SG_pathname* pPath_filter = NULL;
    SG_pathname* pPath_temp_copy = NULL;
    SG_varray* pva_parents = NULL;
    const char* psz_template = NULL;

    // TODO maybe check pvh_filters for the lowest gen and stop descending there

    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs, &psz_csid)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_already, psz_csid, &b_already)  );
    if (b_already)
    {
        goto done;
    }
    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_already, psz_csid)  );

    SG_ERR_CHECK(  SG_changeset__get_parents(pCtx, pcs, &pva_parents)  );
    if (pva_parents)
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count_parents)  );
    }
    SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_template)  );
    for (i=0; i<count_parents; i++)
    {
        SG_bool b_exists = SG_FALSE;
        const char* psz_csid_parent = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz_csid_parent)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_filters, psz_csid_parent, &b_exists)  );
        if (b_exists)
        {
            SG_bool b_compatible_template = SG_FALSE;

            SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pndx->pRepo, psz_csid_parent, &pcs_parent)  );

            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pndx->iDagNum))
            {
                b_compatible_template = SG_TRUE;
            }
            else
            {
                const char* psz_template_parent = NULL;
                SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs_parent, &psz_template_parent)  );
                if (0 == strcmp(psz_template, psz_template_parent))
                {
                    b_compatible_template = SG_TRUE;
                }
            }

            if (b_compatible_template)
            {
                SG_int32 gen_parent = -1;
                SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs_parent, &gen_parent) );
                SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid_parent, gen_parent, &pPath_filter)  );
                sg_dbndx__copy_file(pCtx, pndx, pPath_filter, &pPath_temp_copy);
                if (SG_CONTEXT__HAS_ERR(pCtx))
                {
                    SG_context__err_reset(pCtx);
                }
                else
                {
                    pcs_result = pcs_parent;
                    pcs_parent = NULL;
                    goto done;
                }
            }
        }
        SG_CHANGESET_NULLFREE(pCtx, pcs_parent);
        SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
    }

    if (depth < 32)
    {
        for (i=0; i<count_parents; i++)
        {
            const char* psz_csid_parent = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i, &psz_csid_parent)  );
            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_already, psz_csid_parent, &b_already)  );
            if (!b_already)
            {
                SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pndx->pRepo, psz_csid_parent, &pcs_parent)  );
                SG_ERR_CHECK(  sg_dbndx__find_base_state__ancestor__recursive(pCtx, pndx, pcs_parent, pvh_filters, 1 + depth, pvh_already, &pcs_result, &pPath_temp_copy)  );
                SG_CHANGESET_NULLFREE(pCtx, pcs_parent);

                if (pcs_result)
                {
                    break;
                }
            }
        }
    }

done:
    *ppcs = pcs_result;
    pcs_result = NULL;

    *ppPath = pPath_temp_copy;
    pPath_temp_copy = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_copy);
    SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
    SG_CHANGESET_NULLFREE(pCtx, pcs_result);
    SG_CHANGESET_NULLFREE(pCtx, pcs_parent);
}

static void sg_dbndx__find_base_state__ancestor(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_changeset* pcs,
        SG_vhash* pvh_filters,
        SG_changeset** ppcs,
        SG_pathname** ppPath
        )
{
    SG_vhash* pvh_already = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_already)  );
    SG_ERR_CHECK(  sg_dbndx__find_base_state__ancestor__recursive(pCtx, pndx, pcs, pvh_filters, 0, pvh_already, ppcs, ppPath)  );

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_already);
}

static void sg_dbndx__find_base_state__whatever(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        SG_changeset* pcs,
        SG_vhash* pvh_filters,
        SG_changeset** ppcs,
        SG_pathname** ppPath
        )
{
    SG_uint32 i = 0;
    SG_uint32 count_filters = 0;
    SG_int32 gen_me = -1;
    SG_changeset* pcs_base = NULL;
    SG_pathname* pPath_temp_copy = NULL;
    SG_pathname* pPath_filter = NULL;
    const char* psz_template = NULL;

    SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_template)  );
    SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs, &gen_me)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_filters, &count_filters)  );
    for (i=0; i<count_filters; i++)
    {
        SG_vhash* pvh_f = NULL;
        SG_int32 gen_base = -1;
        const char* psz_csid_base = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_filters, i, &psz_csid_base, &pvh_f)  );
        SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_f, "gen", &gen_base)  );

        if (gen_base < gen_me)
        {
            SG_bool b_compatible_template = SG_FALSE;

            SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pndx->pRepo, psz_csid_base, &pcs_base)  );

            if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(pndx->iDagNum))
            {
                b_compatible_template = SG_TRUE;
            }
            else
            {
                const char* psz_template_base = NULL;
                SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs_base, &psz_template_base)  );
                if (0 == strcmp(psz_template, psz_template_base))
                {
                    b_compatible_template = SG_TRUE;
                }
            }

            if (b_compatible_template)
            {
                SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid_base, gen_base, &pPath_filter)  );

                sg_dbndx__copy_file(pCtx, pndx, pPath_filter, &pPath_temp_copy);
                if (SG_CONTEXT__HAS_ERR(pCtx))
                {
                    SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
                    SG_CHANGESET_NULLFREE(pCtx, pcs_base);
                    SG_context__err_reset(pCtx);
                }
                else
                {
                    break;
                }
            }
            else
            {
                SG_CHANGESET_NULLFREE(pCtx, pcs_base);
            }
        }
    }

    *ppcs = pcs_base;
    pcs_base = NULL;

    *ppPath = pPath_temp_copy;
    pPath_temp_copy = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_copy);
    SG_CHANGESET_NULLFREE(pCtx, pcs_base);
}

static void sg_dbndx__grab_lock(
        SG_context * pCtx,
        SG_pathname* pPath_lock,
        SG_bool* pb
        )
{
    SG_file* pFile = NULL;

    SG_file__open__pathname(pCtx,pPath_lock,SG_FILE_WRONLY|SG_FILE_CREATE_NEW,0644,&pFile);
    SG_FILE_NULLCLOSE(pCtx, pFile);

    /* TODO should we check specifically for the error wherein the lock
     * file already exists?  Or can we just continue the loop on any error
     * at all?  If we're going to check for the error specifically, we
     * probably need SG_file to return a special error code for this
     * case, rather than returning something errno/win32 wrapped. */
    if (SG_CONTEXT__HAS_ERR(pCtx))
    {
        *pb = SG_FALSE;
		SG_context__err_reset(pCtx);
    }
    else
    {
        *pb = SG_TRUE;
    }
}

static void x_blob_stderr(SG_context* pCtx, SG_repo * pRepo, const char* psz_hid)
{
    SG_blob* pblob = NULL;
    SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(psz_hid);

	SG_ERR_CHECK(  SG_repo__get_blob(pCtx, pRepo, psz_hid, &pblob)  );
	if (pblob->length > SG_UINT32_MAX)
		SG_ERR_THROW(SG_ERR_LIMIT_EXCEEDED);
    SG_ERR_CHECK(  SG_vhash__alloc__from_json__buflen(pCtx, &pvh, (char*) pblob->data, (SG_uint32)pblob->length)  );
	SG_ERR_CHECK(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );
    SG_VHASH_STDERR(pvh);

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    if (pblob)
    {
        SG_ERR_IGNORE(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );
    }
}

void sg_dbndx__load_schema_from_sql(
        SG_context* pCtx, 
        sqlite3* psql,
        const char* psz_table,
        SG_vhash** ppvh_schema
        )
{
    sqlite3_stmt* pStmt_schema = NULL;
    int rc = -1;
    SG_vhash* pvh_schema = NULL;

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt_schema, "SELECT json FROM %s", psz_table)  );
    SG_ERR_CHECK(  sg_sqlite__step__nocheck__retry(pCtx, pStmt_schema, &rc, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    if (SQLITE_ROW == rc)
    {
        const char* psz_json = (const char*) sqlite3_column_text(pStmt_schema,0);

        SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh_schema, psz_json)  );
    }
    else
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt_schema)  );

    *ppvh_schema = pvh_schema;
    pvh_schema = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_schema)  );
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
}

static void sg_dbndx__verify_one_filter(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        const char* psz_csid,
        SG_bool b_fail
        )
{
    SG_pathname* pPath_filter = NULL;
    SG_dagnode* pdn = NULL;
    sqlite3_stmt* pStmt = NULL;
    sqlite3* psql = NULL;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_found = NULL;
    int rc = -1;
    SG_uint32 i = 0;
    SG_uint32 count_rectypes = 0;
    SG_vhash* pvh_schema = NULL;
    SG_int32 gen = -1;
	sqlite3_stmt* pStmtAttach = NULL;

	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid, &pdn)  );
    SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
    SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid, gen, &pPath_filter)  );
    SG_repo__db__calc_delta_from_root(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid, 0, &pvh_add);
    if (SG_CONTEXT__HAS_ERR(pCtx))
    {
        SG_context__err_reset(pCtx);
        fprintf(stderr, "    FAILED on SG_repo__db__calc_delta_from_root: %s\n", psz_csid);
        goto fail;
    }

    SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath_filter, SG_SQLITE__SYNC__OFF, &psql)  );

	SG_ERR_CHECK(  sg_dbnx__prepare_attach_stmt(pCtx, psql, pndx->pPath_me, "main_ndx", &pStmtAttach));
	SG_ERR_CHECK(  sg_dbndx__sqlite_exec__retry_open(pCtx, pStmtAttach, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    
    SG_ERR_CHECK(  sg_dbndx__load_schema_from_sql(pCtx, psql, "state_schema", &pvh_schema)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_found)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );
    for (i=0; i<count_rectypes; i++)
    {
        SG_vhash* pvh_one_rectype = NULL;
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i, &psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "SELECT hidrec FROM %s%s_%s r INNER JOIN hidrecs h ON (h.hidrecrow=r.hidrecrow)", SG_DBNDX_RECORD_TABLE_PREFIX, psz_csid, psz_rectype)  );
        while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
        {
            const char* psz_hidrec = (const char*) sqlite3_column_text(pStmt, 0);

            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_found, psz_hidrec)  );
        }
        if (rc != SQLITE_DONE)
        {
            SG_ERR_THROW(SG_ERR_SQLITE(rc));
        }
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    }
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DETACH DATABASE main_ndx")  );
	SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );

    SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_add, SG_FALSE, SG_vhash_sort_callback__increasing)  );
    SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_found, SG_FALSE, SG_vhash_sort_callback__increasing)  );

    { 
        SG_string* pstr_add = NULL; 
        SG_string* pstr_found = NULL; 

        SG_STRING__ALLOC(pCtx, &pstr_add); 
        SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh_add, pstr_add); 
        SG_STRING__ALLOC(pCtx, &pstr_found); 
        SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh_found, pstr_found); 

        if (0 == strcmp(SG_string__sz(pstr_add), SG_string__sz(pstr_found)))
        {
            fprintf(stderr, "    OK: %s\n", psz_csid);
        }
        else
        {
            fprintf(stderr, "    FAIL: %s\n", psz_csid);
            {
                SG_uint32 count_add = 0;
                SG_uint32 count_found = 0;
                SG_uint32 i = 0;

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count_add)  );
                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_found, &count_found)  );

                fprintf(stderr, "      count_add = %5d\n", count_add);
                fprintf(stderr, "    count_found = %5d\n", count_found);

                for (i=0; i<count_add; i++)
                {
                    const char* psz_hidrec = NULL;
                    SG_bool b_exists = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i, &psz_hidrec, NULL)  );
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_found, psz_hidrec, &b_exists)  );
                    if (!b_exists)
                    {
                        fprintf(stderr, "    In deltas but not in dbndx: %s\n", psz_hidrec);
                        SG_ERR_CHECK(  x_blob_stderr(pCtx, pndx->pRepo, psz_hidrec)  );
                    }
                }

                for (i=0; i<count_found; i++)
                {
                    const char* psz_hidrec = NULL;
                    SG_bool b_exists = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_found, i, &psz_hidrec, NULL)  );
                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_add, psz_hidrec, &b_exists)  );
                    if (!b_exists)
                    {
                        fprintf(stderr, "    In dbndx but not in deltas: %s\n", psz_hidrec);
                        SG_ERR_CHECK(  x_blob_stderr(pCtx, pndx->pRepo, psz_hidrec)  );
                    }
                }
            }
            if (b_fail)
            {
                SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
            }
        }

        SG_STRING_NULLFREE(pCtx, pstr_found); 
        SG_STRING_NULLFREE(pCtx, pstr_add); 
    }

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VHASH_NULLFREE(pCtx, pvh_add);
    SG_VHASH_NULLFREE(pCtx, pvh_found);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
}

void sg_dbndx__verify_all_filters(
        SG_context* pCtx, 
        SG_dbndx_query* pndx
        )
{
    SG_vhash* pvh_filters = NULL;
    SG_uint32 count_filters = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  sg_dbndx__list_all_filters(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, &pvh_filters)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_filters, &count_filters)  );
    for (i=0; i<count_filters; i++)
    {
        SG_vhash* pvh_f = NULL;
        const char* psz_csid = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_filters, i, &psz_csid, &pvh_f)  );

        fprintf(stderr, "    %5d of %5d", i+1, count_filters);
        SG_ERR_CHECK(  sg_dbndx__verify_one_filter(pCtx, pndx, psz_csid, SG_FALSE)  );
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_filters);
}

static void sg_dbndx__get_schema_for_csid(
        SG_context* pCtx, 
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid,
        SG_vhash** ppvh_schema
        )
{
    SG_vhash* pvh_schema = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_vhash* pvh_rectypes = NULL;
    SG_uint32 count_rectypes = 0;
    SG_uint32 i = 0;
        
    if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
    {
        SG_ERR_CHECK(  SG_zing__get_cached_template__static_dagnum(pCtx, dagnum, &pzt)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_zing__get_cached_template__csid(pCtx, pRepo, dagnum, psz_csid, &pzt)  );
    }

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_rectypes)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_schema)  );
    SG_ERR_CHECK(  SG_zingtemplate__list_rectypes__update_into_vhash(pCtx, pzt, pvh_rectypes)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rectypes, &count_rectypes)  );
    for (i=0; i<count_rectypes; i++)
    {
        const char* psz_rectype = NULL;
        SG_vhash* pvh_one_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_rectypes, i, &psz_rectype, NULL)  );

        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_schema, psz_rectype, &pvh_one_rectype)  );

        SG_ERR_CHECK(  SG_zing__collect_fields_one_rectype_one_template(pCtx, psz_rectype, pzt, pvh_one_rectype)  );

        // add recid to the field list
        if (!SG_DAGNUM__HAS_NO_RECID(dagnum))
        {
            SG_vhash* pvh_field_recid = NULL;

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_one_rectype, SG_ZING_FIELD__RECID, &pvh_field_recid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_field_recid, SG_DBNDX_SCHEMA__FIELD__TYPE, "string")  );
        SG_ERR_CHECK(  SG_vhash__update__bool(pCtx, pvh_field_recid, SG_DBNDX_SCHEMA__FIELD__INDEX, SG_TRUE)  );
        }
    }

	SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_schema, SG_TRUE, SG_vhash_sort_callback__increasing)  );

    *ppvh_schema = pvh_schema;
    pvh_schema = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_rectypes);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
}

static void sg_dbndx__make_room_to_attach(
        SG_context* pCtx, 
        sqlite3* psql
        )
{
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_varray* pva = NULL;
    SG_uint32 limit = 0;
    SG_uint32 count = 0;

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, 
                psql,
                &pStmt,
                "PRAGMA database_list"  
                )  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char* psz_dbname = (char*) sqlite3_column_text(pStmt, 1);
        if (
                ('q' == psz_dbname[0])
                && ('_' == psz_dbname[1])
                // && looks like a hid
           )
        {
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, psz_dbname)  );
        }
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    limit = (SG_uint32) sqlite3_limit(psql, SQLITE_LIMIT_ATTACHED, 32);
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );

    if (count + 4 > limit)
    {
        SG_uint32 drop = count + 4 - limit;
        SG_uint32 i = 0;

        for (i=0; i<drop; i++)
        {
            char buf[256];
            const char* psz_dbname = NULL;
            
			// These are aliases, not file paths: they don't need escaping.
            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz_dbname)  );
            SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "DETACH DATABASE %s", psz_dbname)  );
            SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, buf)  );
        }
    }

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void sg_dbndx__check_if_attached(
        SG_context* pCtx, 
        sqlite3* psql,
        const char* psz_csid,
        SG_bool* pb
        )
{
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    char buf[256];
    SG_bool b_found = SG_FALSE;

    SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "q_%s", psz_csid)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, 
                psql,
                &pStmt,
                "PRAGMA database_list"  
                )  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char* psz_dbname = (char*) sqlite3_column_text(pStmt, 1);
        if (0 == strcmp(psz_dbname, buf))
        {
            b_found = SG_TRUE;
            break;
        }
    }
    if (
            (rc != SQLITE_DONE)
            && !b_found
       )
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    *pb = b_found;

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void sg_dbndx__update_cur_state(
        SG_context* pCtx, 
        SG_dbndx_query* pndx, 
        const char* psz_csid,
        SG_vhash** ppvh_schema
        )
{
    SG_bool b_exists = SG_FALSE;
    SG_pathname* pPath_filter = NULL;
    SG_pathname* pPath_temp_copy = NULL;
    SG_changeset* pcs = NULL;
    SG_int32 gen = -1;
    SG_changeset* pcs_base = NULL;
	sqlite3_stmt* pStmtAttach = NULL;
    SG_vhash* pvh_filters = NULL;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_remove = NULL;
    SG_pathname* pPath_lock = NULL;
    SG_bool b_locked = SG_FALSE;
    SG_vhash* pvh_schema = NULL;
    SG_bool b_already_attached = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(psz_csid);

	SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pndx->pRepo, psz_csid, &pcs)  );
    SG_ERR_CHECK(  SG_changeset__get_generation(pCtx, pcs, &gen)  );
    SG_ERR_CHECK(  sg_dbndx_query__get_pathname_for_state_filter(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, psz_csid, gen, &pPath_filter)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_filter, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        // TODO touch it
    }
    else
    {
        char buf[SG_HID_MAX_BUFFER_LENGTH + 64];

        SG_ERR_CHECK(  sg_dbndx__get_schema_for_csid(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid, &pvh_schema)  );
        
        // try to have only one thread/process creating this prefiltered file
        SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%s.lock", psz_csid)  );
        SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPath_lock, pndx->pPath_filters_dir, buf)  );
        SG_ERR_CHECK(  sg_dbndx__grab_lock(pCtx, pPath_lock, &b_locked)  );

        if (b_locked)
        {
            // Now we need to create a filter for this state.  If possible, we want
            // to derive it from a filter that already exists, so we don't have to
            // walk all the way back to root to find one.  
            //
            // First we check to see
            // if a filter exists for a parent of this changeset.  
            //
            // Then we look a
            // bit further and look for any close ancestor.  
            //
            // Then we throw our hands
            // up and settle for almost anything with a lower gen number.  
            //
            // Finally, if we just can't find a
            // filter, we go all the way back to root.

            SG_ERR_CHECK(  sg_dbndx__find_base_state__parent(pCtx, pndx, pcs, &pcs_base, &pPath_temp_copy)  );
            if (pcs_base)
            {
                SG_ERR_CHECK(  sg_dbndx__create_state_filter__from_parent(pCtx, pndx, pvh_schema, pcs, pcs_base, pPath_temp_copy)  );
            }
            else
            {
                SG_ERR_CHECK(  sg_dbndx__list_all_filters(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, &pvh_filters)  );
                SG_ERR_CHECK(  SG_vhash__sort__vhash_field_int__desc(pCtx, pvh_filters, "gen")  );
                //SG_VHASH_STDERR(pvh_filters);
                SG_ERR_CHECK(  sg_dbndx__find_base_state__ancestor(pCtx, pndx, pcs, pvh_filters, &pcs_base, &pPath_temp_copy)  );
                if (!pcs_base)
                {
                    SG_ERR_CHECK(  sg_dbndx__find_base_state__whatever(pCtx, pndx, pcs, pvh_filters, &pcs_base, &pPath_temp_copy)  );
                }
                if (pcs_base)
                {
                    const char* psz_csid_base = NULL;

                    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs_base, &psz_csid_base)  );

                    SG_ERR_CHECK(  sg_dbndx__create_state_filter__from_base(pCtx, pndx, pvh_schema, pcs, pcs_base, pPath_temp_copy)  );
                    // TODO touch the base filter
                }
                else
                {
                    SG_ERR_CHECK(  sg_dbndx__create_state_filter__from_root(pCtx, pndx, pvh_schema, psz_csid, gen)  );
                }
            }

            SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_lock)  );
            b_locked = SG_FALSE;
            SG_PATHNAME_NULLFREE(pCtx, pPath_lock);


            if (0 == (gen % 16))
            {
                if (!pvh_filters)
                {
                    SG_ERR_CHECK(  sg_dbndx__list_all_filters(pCtx, pndx->pPath_filters_dir, pndx->iDagNum, &pvh_filters)  );
                }
                SG_ERR_CHECK(  sg_dbndx__cleanup_old_filters(pCtx, pndx, pvh_filters)  );
            }
            SG_VHASH_NULLFREE(pCtx, pvh_filters);

            if (pPath_temp_copy)
            {
                SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_temp_copy)  );
            }

#if SG_DOUBLE_CHECK__NEW_FILTER
    SG_ERR_CHECK(  sg_dbndx__verify_one_filter(pCtx, pndx, psz_csid, SG_TRUE)  );
#endif

        }
        else
        {
            // somebody else is creating the filter.  just wait for it to be done.
            SG_uint32 slept = 0;
            while (1)
            {
                SG_bool b_exists = SG_FALSE;
                SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_filter, &b_exists, NULL, NULL)  );
                if (b_exists)
                {
                    break;
                }

                if (slept >= 90000)
                {
                    // too long.  just give up.
                    SG_ERR_THROW(  SG_ERR_DBNDX_FILTER_TIMEOUT  );
                }
                SG_sleep_ms(50);
                slept += 50;
            }
        }
    }

    SG_ERR_CHECK(  sg_dbndx__check_if_attached(pCtx, pndx->psql, psz_csid, &b_already_attached)  );

    if (!b_already_attached)
    {
		char buf[SG_HID_MAX_BUFFER_LENGTH + 5];
        
		SG_ERR_CHECK(  sg_dbndx__make_room_to_attach(pCtx, pndx->psql)  );

		SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "q_%s", psz_csid)  );
		SG_ERR_CHECK(  sg_dbnx__prepare_attach_stmt(pCtx, pndx->psql, pPath_filter, buf, &pStmtAttach));
		SG_ERR_CHECK(  sg_dbndx__sqlite_exec__retry_open(pCtx, pStmtAttach, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    }

    if (!pvh_schema)
    {
        char buf[256];
        SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "q_%s.state_schema", psz_csid)  );
        SG_ERR_CHECK(  sg_dbndx__load_schema_from_sql(pCtx, pndx->psql, buf, &pvh_schema)  );
    }

    *ppvh_schema = pvh_schema;
    pvh_schema = NULL;

fail:
    if (pPath_lock)
    {
        SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_lock)  );
    }
    SG_PATHNAME_NULLFREE(pCtx, pPath_lock);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VHASH_NULLFREE(pCtx, pvh_add);
    SG_VHASH_NULLFREE(pCtx, pvh_remove);
    SG_VHASH_NULLFREE(pCtx, pvh_filters);
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtAttach)  );
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_CHANGESET_NULLFREE(pCtx, pcs_base);
    SG_PATHNAME_NULLFREE(pCtx, pPath_filter);
    SG_PATHNAME_NULLFREE(pCtx, pPath_temp_copy);
}

static void sg_dbndx__calc_orderby(
        SG_context* pCtx, 
        const char* pidState,
        const char* psz_rectype,
        SG_vhash* pvh_rectype_fields,
        const SG_varray* pSort, 
        SG_string** ppstr
        )
{
	SG_uint32 i;
	SG_uint32 levels;
    SG_string* pstr = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pSort, &levels)  );
	for (i=0; i<levels; i++)
	{
        SG_vhash* pvh_key = NULL;
        const char* psz_field_name = NULL;
        SG_bool b_desc = SG_FALSE;
        SG_vhash* pvh_field = NULL;
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pSort, i, &pvh_key)  );

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_key, SG_DBNDX_SORT__NAME, &psz_field_name)  );

        // get the direction
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_key, SG_DBNDX_SORT__DIRECTION, &b_has)  );
        if (b_has)
        {
            const char* psz_direction = NULL;
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_key, SG_DBNDX_SORT__DIRECTION, &psz_direction)  );
            if (0 == strcmp(psz_direction, "desc"))
            {
                b_desc = SG_TRUE;
            }
        }

        // see if this is a field in the rectype being queried
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectype_fields, psz_field_name, &b_has)  );
        if (b_has)
        {
            SG_vhash* pvh_orderings = NULL;

            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype_fields, psz_field_name, &pvh_field)  );
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__ORDERINGS, &pvh_orderings)  );

            if (i > 0)
            {
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ", ")  );
            }

            if (pvh_orderings)
            {
                const char* psz_key = NULL;
                SG_uint32 count_orderings = 0;

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_orderings, &count_orderings)  );
                if (1 != count_orderings)
                {
                    SG_ERR_THROW2(  SG_ERR_INVALIDARG, (pCtx, "A filter used for sorting can only have one ordering")  );
                }
                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_orderings, 0, &psz_key, NULL)  );

                SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "\"%s%s_%s\".\"ordering_%s$_%s\" %s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_key, psz_field_name, b_desc ? "DESC" : "ASC")  );
            }
            else
            {
                SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "\"%s%s_%s\".\"%s\" %s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name, b_desc ? "DESC" : "ASC")  );
            }
        }
        else
        {
            // if it's not a field in the rectype, then it's probably something
            // aliased, like it got here in a from_me style join.

            // TODO walk the columns array and verify this?
            if (i > 0)
            {
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ", ")  );
            }

            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "\"%s\" %s", psz_field_name, b_desc ? "DESC" : "ASC")  );
        }
    }

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__create_temp_tables(
        SG_context* pCtx, 
        sqlite3* psql,
        SG_vhash* pvh_in
        )
{
    SG_uint32 count_tables = 0;
    sqlite3_stmt* pStmt = NULL;

    if (!pvh_in)
    {
        return;
    }
    SG_NULLARGCHECK_RETURN(psql);

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_in, &count_tables)  );

    if (count_tables)
    {
        SG_uint32 i_table = 0;

        for (i_table=0; i_table<count_tables; i_table++)
        {
            const char* psz_tid = NULL;
            SG_varray* pva_vals = NULL;
            SG_uint32 count_vals = 0;
            SG_uint32 i_val = 0;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__varray(pCtx, pvh_in, i_table, &psz_tid, &pva_vals)  );

            SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE TEMP TABLE %s (val VARCHAR NOT NULL UNIQUE)", psz_tid)  );

            SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT OR IGNORE INTO %s (val) VALUES (?)", psz_tid)  );
            SG_ERR_CHECK(  SG_varray__count(pCtx, pva_vals, &count_vals)  );
            for (i_val=0; i_val<count_vals; i_val++)
            {
                const char* psz_val = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_vals, i_val, &psz_val)  );

                SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
                SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_val)  );

                SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
            }
            SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
        }
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void sg_dbndx__calc_where(
        SG_context* pCtx, 
        const char* pidState, 
        const char* psz_rectype,
        SG_vhash* pvh_rectype_fields,
        const SG_varray* pcrit, 
        SG_string** ppstr,
        SG_vhash** ppvh_fts,
        SG_vhash* pvh_in,
        SG_vhash* pvh_columns_used
        )
{
    SG_string* pstr = NULL;
    const char* psz_op = NULL;
    SG_string* pstr_left = NULL;
    SG_string* pstr_right = NULL;
    char* psz_escaped = NULL;

	SG_NULLARGCHECK_RETURN(pcrit);
	SG_NULLARGCHECK_RETURN(ppvh_fts);

    //SG_VARRAY_STDOUT(pcrit);

    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_OP, &psz_op)  );

    if (
        (0 == strcmp("<", psz_op))
        || (0 == strcmp(">", psz_op))
        || (0 == strcmp("<=", psz_op))
        || (0 == strcmp(">=", psz_op))
        )
    {
        SG_int64 intvalue;
        const char* psz_field_name = NULL;
        SG_int_to_string_buffer sz_i;
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_LEFT, &psz_field_name)  );
        SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &intvalue)  );
        SG_int64_to_sz(intvalue, sz_i);

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectype_fields, psz_field_name, &b_has)  );
        if (b_has)
        {
            if (pvh_columns_used)
            {
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_columns_used, psz_field_name)  );
            }
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" %s %s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name, psz_op, sz_i)  );
        }
        else
        {
            // TODO we may want to verify the field name is an alias in the columns area
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s\" %s %s", psz_field_name, psz_op, sz_i)  );
        }

        goto done;
    }
    else if (
        (0 == strcmp("==", psz_op))
        || (0 == strcmp("!=", psz_op))
        )
    {
        const char* psz_field_name = NULL;
        SG_uint16 t;
        SG_bool b_is_a_field = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_LEFT, &psz_field_name)  );
        SG_ERR_CHECK(  SG_varray__typeof(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &t)  );

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectype_fields, psz_field_name, &b_is_a_field)  );

        if (b_is_a_field)
        {
            if (pvh_columns_used)
            {
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_columns_used, psz_field_name)  );
            }
        }

        if (SG_VARIANT_TYPE_NULL == t)
        {
            if (b_is_a_field)
            {
                if (0 == strcmp("==", psz_op))
                {
                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" IS NULL", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );
                    goto done;
                }
                else
                {
                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" IS NOT NULL", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );
                    goto done;
                }
            }
            else
            {
                if (0 == strcmp("==", psz_op))
                {
                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s\" IS NULL", psz_field_name)  );
                    goto done;
                }
                else
                {
                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s\" IS NOT NULL", psz_field_name)  );
                    goto done;
                }
            }
        }

        if (b_is_a_field)
        {
            SG_vhash* pvh_field = NULL;
            SG_bool b_integerish = SG_FALSE;
            const char* psz_field_type = NULL;

            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype_fields, psz_field_name, &pvh_field)  );
#if 0
            {
                SG_bool b_indexed = SG_FALSE;
                SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__INDEX, &b_indexed)  );
                if (!b_indexed)
                {
                    fprintf(stderr, "WARNING: where using non-indexed field:  %s.%s\n", psz_rectype, psz_field_name);
                }
            }
#endif
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );
            SG_ERR_CHECK(  SG_zing__field_type_is_integerish__sz(pCtx, psz_field_type, &b_integerish)  );

            if (b_integerish)
            {
                SG_int64 intvalue;
                SG_int_to_string_buffer sz_i;

                SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &intvalue)  );
                SG_int64_to_sz(intvalue, sz_i);

                SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" %s %s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name, psz_op, sz_i)  );

                goto done;
            }
            else
            {
                const char* psz_right = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &psz_right)  );
                SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_right, &psz_escaped)  );
                SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, 
                            "\"%s%s_%s\".\"%s\" %s '%s'", 
                            SG_DBNDX_RECORD_TABLE_PREFIX, 
                            pidState?pidState:"",
                            psz_rectype, 
                            psz_field_name, 
                            psz_op,
                            psz_escaped ? psz_escaped : psz_right
                            )  );
                SG_NULLFREE(pCtx, psz_escaped);

                goto done;
            }
        }
        else if (0 == strcmp(psz_field_name, SG_ZING_FIELD__LAST_TIMESTAMP))
        {
            SG_int64 intvalue;
            SG_int_to_string_buffer sz_i;

            SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &intvalue)  );
            SG_int64_to_sz(intvalue, sz_i);

            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s\" %s %s", psz_field_name, psz_op, sz_i)  );

            goto done;
        }
        else
        {
            if (SG_VARIANT_TYPE_SZ == t)
            {
                const char* psz_right = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &psz_right)  );
                SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_right, &psz_escaped)  );
                SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, 
                            "\"%s\" %s '%s'", 
                            psz_field_name, 
                            psz_op,
                            psz_escaped ? psz_escaped : psz_right
                            )  );
                SG_NULLFREE(pCtx, psz_escaped);
            }
            else
            {
                SG_int64 intvalue = 0;
                SG_int_to_string_buffer sz_i;

                SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &intvalue)  );
                SG_int64_to_sz(intvalue, sz_i);

                // TODO we may want to verify the field name is an alias in the columns area
                SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
                SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s\" %s %s", psz_field_name, psz_op, sz_i)  );
            }

            goto done;
        }
    }
    else if (
        (0 == strcmp("match", psz_op))
        )
    {
        const char* psz_field_name = NULL;
        SG_vhash* pvh_field = NULL;
        const char* psz_field_type = NULL;
        const char* psz_keywords = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_LEFT, &psz_field_name)  );

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &psz_keywords)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype_fields, psz_field_name, &pvh_field)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );

        // TODO verify full_text_search is set on this field

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
        SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_keywords, &psz_escaped)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, 
                    "\"%s_%s\".\"%s\" MATCH '%s'", 
                    SG_DBNDX_FTS_TABLE_PREFIX, 
                    psz_rectype, 
                    psz_field_name, 
                    psz_escaped ? psz_escaped : psz_keywords
                    )  );
        SG_NULLFREE(pCtx, psz_escaped);

        if (!*ppvh_fts)
        {
            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, ppvh_fts)  );
        }
        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, *ppvh_fts, psz_rectype)  );

        goto done;
    }
    else if (
        (0 == strcmp("in", psz_op))
        )
    {
        const char* psz_field_name = NULL;
        SG_varray* pva_right = NULL;
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_LEFT, &psz_field_name)  );
        SG_ERR_CHECK(  SG_varray__get__varray(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &pva_right)  );
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_right, &count)  );

        if (pvh_columns_used)
        {
            SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_columns_used, psz_field_name)  );
        }

        if (count > 10)
        {
            char buf[SG_TID_MAX_BUFFER_LENGTH];

            SG_ERR_CHECK(  SG_tid__generate(pCtx, buf, sizeof(buf))  );

            SG_ERR_CHECK(  SG_vhash__addcopy__varray(pCtx, pvh_in, buf, pva_right)  );

            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "EXISTS (SELECT * FROM \"%s\" WHERE val = \"%s%s_%s\".\"%s\")", buf, SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" IN (", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );
            for (i=0; i<count; i++)
            {
                const char* psz_val = NULL;

                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_right, i, &psz_val)  );
                if (i > 0)
                {
                    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ",")  );
                }
                SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "'%s'", psz_val)  );
            }
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ")")  );
        }

        goto done;
    }
    else if (
        (0 == strcmp("exists", psz_op))
        )
    {
        const char* psz_field_name = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_LEFT, &psz_field_name)  );

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" IS NOT NULL", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );
        goto done;
    }
    else if (
        (0 == strcmp("isnull", psz_op))
        )
    {
        const char* psz_field_name = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pcrit, SG_CRIT_NDX_LEFT, &psz_field_name)  );

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "\"%s%s_%s\".\"%s\" IS NULL", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );
        goto done;
    }
    else if (
        (0 == strcmp("&&", psz_op))
        || (0 == strcmp("||", psz_op))
        )
    {
        SG_varray* pcrit_left = NULL;
        SG_varray* pcrit_right = NULL;

        SG_ERR_CHECK(  SG_varray__get__varray(pCtx, pcrit, SG_CRIT_NDX_LEFT, &pcrit_left)  );

        SG_ERR_CHECK(  SG_varray__get__varray(pCtx, pcrit, SG_CRIT_NDX_RIGHT, &pcrit_right)  );

        SG_ERR_CHECK(  sg_dbndx__calc_where(pCtx, pidState, psz_rectype, pvh_rectype_fields, pcrit_left, &pstr_left, ppvh_fts, pvh_in, pvh_columns_used)  );
        SG_ERR_CHECK(  sg_dbndx__calc_where(pCtx, pidState, psz_rectype, pvh_rectype_fields, pcrit_right, &pstr_right, ppvh_fts, pvh_in, pvh_columns_used)  );

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
        if (0 == strcmp("&&", psz_op))
        {
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "(%s) AND (%s)", SG_string__sz(pstr_left), SG_string__sz(pstr_right))  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "(%s) OR (%s)", SG_string__sz(pstr_left), SG_string__sz(pstr_right))  );
        }

        SG_STRING_NULLFREE(pCtx, pstr_left);
        SG_STRING_NULLFREE(pCtx, pstr_right);
        goto done;
    }
    else
    {
        SG_ERR_THROW_RETURN(SG_ERR_INVALIDARG); // TODO invalid crit
    }

	// NOTREACHED SG_ASSERT(0);
done:
    //printf("%s\n", SG_string__sz(pstr));

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_STRING_NULLFREE(pCtx, pstr_left);
    SG_STRING_NULLFREE(pCtx, pstr_right);
    SG_NULLFREE(pCtx, psz_escaped);
}

static void sg_dbndx__add_column(
        SG_context* pCtx,
        SG_varray* pva_result,
        ...
        )
{
    va_list ap;
    SG_vhash* pvh_column = NULL;

    SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_result, &pvh_column)  );

    va_start(ap, pva_result);
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

        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_column, psz_key, psz_val)  );
    } while (1);

fail:
    va_end(ap);
}

static void sg_dbndx__count_all_joins(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    SG_uint32* pi
    )
{
    SG_uint32 result = 0;
    SG_uint32 count_subs = 0;
    SG_uint32 i_sub = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_joins, &count_subs)  );
    for (i_sub=0; i_sub<count_subs; i_sub++)
    {
        SG_vhash* pvh_sub = NULL;
        SG_uint32 count_joins_sub = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_joins, i_sub, NULL, &pvh_sub)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_sub, &count_joins_sub)  );
        result += count_joins_sub;
    }

    *pi = result;

fail:
    ;
}

static void sg_dbndx__make_new_table_alias(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    SG_vhash* pvh_join,
    const char** ppsz_join_table_alias
    )
{
    SG_uint32 count_all_joins = 0;
    char buf[SG_GID_BUFFER_LENGTH];

    SG_ERR_CHECK(  sg_dbndx__count_all_joins(pCtx, pvh_joins, &count_all_joins)  );
    SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "t%03d", 1 + count_all_joins)  );

    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, buf)  );
    // TODO we shouldn't have to search for the entry we just created
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, ppsz_join_table_alias)  );

fail:
    ;
}

static void sg_dbndx__make_new_xref_table_alias(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    SG_vhash* pvh_join,
    const char** ppsz_join_table_alias
    )
{
    SG_uint32 count_all_joins = 0;
    char buf[SG_GID_BUFFER_LENGTH];

    SG_ERR_CHECK(  sg_dbndx__count_all_joins(pCtx, pvh_joins, &count_all_joins)  );
    SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "x%03d", 1 + count_all_joins)  );

    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__XREF_TABLE_ALIAS, buf)  );
    // TODO we shouldn't have to search for the entry we just created
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__XREF_TABLE_ALIAS, ppsz_join_table_alias)  );

fail:
    ;
}

static void sg_dbndx__add_join__FROM_ME(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    const char* psz_ref,
    const char** ppsz_join_table_alias
    )
{
    SG_vhash* pvh_join = NULL;
    SG_vhash* pvh_type = NULL;
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_joins, MY_JOIN__TYPE__TO_ME, &pvh_type)  );

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_type, psz_ref, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_type, psz_ref, &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, ppsz_join_table_alias)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_type, psz_ref, &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__TYPE, MY_JOIN__TYPE__FROM_ME)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__REF_TO_OTHER, psz_ref)  );
        SG_ERR_CHECK(  sg_dbndx__make_new_table_alias(pCtx, pvh_joins, pvh_join, ppsz_join_table_alias)  );
    }

fail:
    ;
}

static void sg_dbndx__add_join__XREF(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    const char* psz_xref_rectype,
    const char* psz_ref_to_me,
    const char* psz_ref_to_other,
    const char** ppsz_join_table_alias,
    const char** ppsz_xref_table_alias
    )
{
    SG_vhash* pvh_join = NULL;
    SG_vhash* pvh_type = NULL;
    SG_bool b_has = SG_FALSE;
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_joins, MY_JOIN__TYPE__TO_ME, &pvh_type)  );

    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "%s", psz_xref_rectype)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_type, SG_string__sz(pstr), &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_type, SG_string__sz(pstr), &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, ppsz_join_table_alias)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__XREF_TABLE_ALIAS, ppsz_xref_table_alias)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_type, SG_string__sz(pstr), &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__TYPE, MY_JOIN__TYPE__XREF)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__RECTYPE, psz_xref_rectype)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__REF_TO_ME, psz_ref_to_me)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__REF_TO_OTHER, psz_ref_to_other)  );
        SG_ERR_CHECK(  sg_dbndx__make_new_table_alias(pCtx, pvh_joins, pvh_join, ppsz_join_table_alias)  );
        SG_ERR_CHECK(  sg_dbndx__make_new_xref_table_alias(pCtx, pvh_joins, pvh_join, ppsz_xref_table_alias)  );
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__add_join__TO_ME(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    const char* psz_other_rectype,
    const char* psz_ref_to_me,
    const char** ppsz_join_table_alias
    )
{
    SG_vhash* pvh_join = NULL;
    SG_vhash* pvh_type = NULL;
    SG_bool b_has = SG_FALSE;
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_joins, MY_JOIN__TYPE__TO_ME, &pvh_type)  );

    SG_ERR_CHECK(  SG_string__alloc(pCtx, &pstr)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr, "%s.%s", psz_other_rectype, psz_ref_to_me)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_type, SG_string__sz(pstr), &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_type, SG_string__sz(pstr), &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, ppsz_join_table_alias)  );
        SG_STRING_NULLFREE(pCtx, pstr);
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_type, SG_string__sz(pstr), &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__TYPE, MY_JOIN__TYPE__TO_ME)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__RECTYPE, psz_other_rectype)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__REF_TO_ME, psz_ref_to_me)  );
        SG_ERR_CHECK(  sg_dbndx__make_new_table_alias(pCtx, pvh_joins, pvh_join, ppsz_join_table_alias)  );
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__add_join__USERNAME(
	SG_context* pCtx,
    SG_vhash* pvh_joins,
    const char* psz_userid_field,
    const char** ppsz_join_table_alias
    )
{
    SG_vhash* pvh_join = NULL;
    SG_vhash* pvh_type = NULL;
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_joins, MY_JOIN__TYPE__TO_ME, &pvh_type)  );

    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_type, psz_userid_field, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_type, psz_userid_field, &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, ppsz_join_table_alias)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_type, psz_userid_field, &pvh_join)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__TYPE, MY_JOIN__TYPE__USERNAME)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_join, MY_JOIN__REF_TO_OTHER, psz_userid_field)  );
        SG_ERR_CHECK(  sg_dbndx__make_new_table_alias(pCtx, pvh_joins, pvh_join, ppsz_join_table_alias)  );
    }

fail:
    ;
}

static void sg_dbndx__all_fields(
	SG_context* pCtx,
    SG_vhash* pvh_schema,
    const char* pidState, 
    const char* psz_rectype,
    SG_varray* pva_result
    )
{
    SG_uint32 count_fields = 0;
    SG_uint32 i = 0;
    SG_vhash* pvh_rectype = NULL;
    SG_string* pstr_expr = NULL;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_rectype, &count_fields)  );
    for (i=0; i<count_fields; i++)
    {
        const char* psz_field_name = NULL;
        const char* psz_field_type = NULL;
        SG_vhash* pvh_field = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_rectype, i, &psz_field_name, &pvh_field)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s%s_%s\".\"%s\"", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );


        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                    MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                    MY_COLUMN__ALIAS,   psz_field_name,
                    MY_COLUMN__TYPE,    psz_field_type,
                    NULL
                    )  );
        SG_STRING_NULLFREE(pCtx, pstr_expr);
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_expr);
}

static void sg_dbndx__calc_column_list(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_vhash* pvh_schema,
    const char* pidState, 
    const char* psz_rectype,
    SG_varray* pva_fields,
    SG_varray** ppva,
    SG_vhash** ppvh_joins,
    SG_bool* pb_history,
    SG_bool* pb_multirow_joins
    )
{
    SG_uint32 count_fields = 0;
    SG_uint32 i = 0;
    SG_varray* pva_result = NULL;
    SG_vhash* pvh_rectype = NULL;
    SG_bool b_want_all_fields = SG_FALSE;
    SG_bool b_history = SG_FALSE;
    SG_bool b_want_hidrec = SG_FALSE;
    SG_varray* pva_specific = NULL;
    SG_string* pstr_expr = NULL;
    SG_bool b_multirow_joins = SG_FALSE;
    SG_vhash* pvh_joins = NULL;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_specific)  );

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_fields, &count_fields)  );
    for (i=0; i<count_fields; i++)
    {
        SG_uint16 t = 0;

        const char* psz = NULL;

        SG_ERR_CHECK(  SG_varray__typeof(pCtx, pva_fields, i, &t)  );
        if (SG_VARIANT_TYPE_SZ == t)
        {
            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_fields, i, &psz)  );
            if (0 == strcmp(psz, SG_ZING_FIELD__HISTORY))
            {
                b_history = SG_TRUE;
            }
            else if (0 == strcmp(psz, SG_ZING_FIELD__HIDREC))
            {
                b_want_hidrec = SG_TRUE;
            }
            else if (0 == strcmp(psz, SG_ZING_FIELD__GIMME_ALL_FIELDS))
            {
                b_want_all_fields = SG_TRUE;
            }
            else
            {
                SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pva_specific, i)  );
            }
        }
        else if (SG_VARIANT_TYPE_VHASH == t)
        {
            SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pva_specific, i)  );
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }
    }

    // okay, let's build our column list

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_result)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_joins)  );

    // first we put in a key which can be used to group multirow
    // records.  NOTE:  this must be the first column.
    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
    if (SG_DAGNUM__HAS_NO_RECID(pndx->iDagNum))
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s%s_%s\".\"%s\"", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, "hidrecrow")  );
    }
    else
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s%s_%s\".\"%s\"", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, SG_ZING_FIELD__RECID)  );
    }
    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                MY_COLUMN__DEST,    MY_COLUMN__DEST__KEY,
                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                NULL
                )  );
    SG_STRING_NULLFREE(pCtx, pstr_expr);

    // If we need history, then we put all these columns in
    // as well.  Each of them has a DEST which the history
    // code catches later.

    if (b_history)
    {
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,   "history_hidrec",
                    MY_COLUMN__EXPR,    "h_hidrecs.hidrec",
                    NULL
                    )  );
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,   "history_csid",
                    MY_COLUMN__EXPR,    "h_csids.csid",
                    NULL
                    )  );
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,   "history_generation",
                    MY_COLUMN__EXPR,    "h_csids.generation",
                    NULL
                    )  );
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,   "audit_userid",
                    MY_COLUMN__EXPR,    "db_audits.userid",
                    NULL
                    )  );
        if (SG_DAGNUM__USERS != pndx->iDagNum)
        {
            SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                        MY_COLUMN__DEST,   "audit_username",
                        MY_COLUMN__EXPR,    "shadow_users.name",
                        NULL
                        )  );
        }
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,   "audit_timestamp",
                    MY_COLUMN__EXPR,    "db_audits.timestamp",
                    NULL
                    )  );
    }
    
    if (b_want_hidrec)
    {
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                    MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                    MY_COLUMN__EXPR,    "hidrecs.hidrec",
                    MY_COLUMN__ALIAS,   SG_ZING_FIELD__HIDREC,
                    MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,
                    NULL
                    )  );
    }

    if (b_want_all_fields)
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_specific, &count_fields)  );
        if (count_fields > 0)
        {
            // don't request specific fields AND all fields
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }

        SG_ERR_CHECK(  sg_dbndx__all_fields(pCtx, pvh_schema, pidState, psz_rectype, pva_result)  ); 
    }
    else
    {
        // add just the fields requested
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_specific, &count_fields)  );
        for (i=0; i<count_fields; i++)
        {
            const char* psz_field_name = NULL;
            const char* psz_field_type = NULL;
            SG_vhash* pvh_field = NULL;
            SG_uint16 t = 0;
            SG_bool b_is_a_field = SG_FALSE;
            SG_int64 ndx = -1;

            SG_ERR_CHECK(  SG_varray__get__int64(pCtx, pva_specific, i, &ndx)  );
            SG_ASSERT(ndx>=0);
            SG_ERR_CHECK(  SG_varray__typeof(pCtx, pva_fields, (SG_uint32) ndx, &t)  );
            if (SG_VARIANT_TYPE_SZ == t)
            {
                SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_fields, (SG_uint32) ndx, &psz_field_name)  );
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectype, psz_field_name, &b_is_a_field)  );
                if (b_is_a_field)
                {
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype, psz_field_name, &pvh_field)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );

                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s%s_%s\".\"%s\"", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );

                    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                MY_COLUMN__ALIAS,   psz_field_name,
                                MY_COLUMN__TYPE,    psz_field_type,
                                NULL
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr_expr);
                }
                else if (0 == strcmp(psz_field_name, SG_ZING_FIELD__LAST_TIMESTAMP))
                {
                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "(SELECT MAX(timestamp) FROM db_audits, db_history WHERE db_history.hidrecrow = \"%s%s_%s\".hidrecrow AND db_audits.csidrow = db_history.csidrow)", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype)  );

                    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                MY_COLUMN__ALIAS,   SG_ZING_FIELD__LAST_TIMESTAMP,
                                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__INT,
                                NULL
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr_expr);
                }
                else
                {
                    SG_ERR_THROW2(  SG_ERR_ZING_FIELD_NOT_FOUND, (pCtx, "%s.%s", psz_rectype, psz_field_name)  );
                }
            }
            else if (SG_VARIANT_TYPE_VHASH == t)
            {
                SG_vhash* pvh_expr = NULL;
                const char* psz_expr_type = NULL;

                SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_fields, (SG_uint32) ndx, &pvh_expr)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__TYPE, &psz_expr_type)  );
                if (0 == strcmp(psz_expr_type, MY_EXPR__TYPE__FROM_ME))
                {
                    const char* psz_ref = NULL;
                    const char* psz_other_field = NULL;
                    const char* psz_alias = NULL;
                    SG_vhash* pvh_ref_field = NULL;
                    const char* psz_other_rectype = NULL;
                    SG_vhash* pvh_other_rectype = NULL;
                    SG_vhash* pvh_other_field = NULL;
                    const char* psz_other_field_type = NULL;
                    const char* psz_join_table_alias = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__REF, &psz_ref)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__FIELD, &psz_other_field)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_expr, MY_EXPR__ALIAS, &psz_alias)  );

                    // looup the ref field
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype, psz_ref, &pvh_ref_field)  );
                    // and the rectype on the other end
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_ref_field, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, &psz_other_rectype)  );

                    // now lookup that rectype
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_other_rectype, &pvh_other_rectype)  );
                    // and the given field
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_other_rectype, psz_other_field, &pvh_other_field)  );
                    // get the type of that field, because it will be the type
                    // of the resulting column in the query
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_other_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_other_field_type)  );

                    // add info so we know what tables to join
                    SG_ERR_CHECK(  sg_dbndx__add_join__FROM_ME(pCtx, pvh_joins, psz_ref, &psz_join_table_alias)  );

                    // build the expr
                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"%s\"", psz_join_table_alias, psz_other_field)  );

                    if (!psz_alias)
                    {
                        psz_alias = psz_other_field;
                    }

                    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                MY_COLUMN__ALIAS,   psz_alias,
                                MY_COLUMN__TYPE,    psz_other_field_type,
                                NULL
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr_expr);
                }
                else if (0 == strcmp(psz_expr_type, MY_EXPR__TYPE__XREF))
                {
                    const char* psz_xref_rectype = NULL;
                    const char* psz_other_rectype = NULL;
                    const char* psz_should_be_my_rectype = NULL;
                    const char* psz_ref_to_me = NULL;
                    const char* psz_ref_to_other = NULL;
                    const char* psz_alias = NULL;
                    SG_varray* pva_other_fields = NULL;
                    SG_vhash* pvh_xref_rectype = NULL;
                    SG_vhash* pvh_other_rectype = NULL;
                    SG_vhash* pvh_ref_field_to_me = NULL;
                    SG_vhash* pvh_ref_field_to_other = NULL;
                    SG_uint32 count_other_fields = 0;
                    SG_uint32 i_other_field = 0;
                    const char* psz_join_table_alias = NULL;
                    const char* psz_xref_table_alias = NULL;
                    const char* psz_xref_recid_alias = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__XREF, &psz_xref_rectype)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_expr, MY_EXPR__XREF_RECID_ALIAS, &psz_xref_recid_alias)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__REF_TO_ME, &psz_ref_to_me)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__REF_TO_OTHER, &psz_ref_to_other)  );
                    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_expr, MY_EXPR__FIELDS, &pva_other_fields)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__ALIAS, &psz_alias)  );

                    // lookup the xref rectype
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_xref_rectype, &pvh_xref_rectype)  );
                    // and the given refs
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_xref_rectype, psz_ref_to_me, &pvh_ref_field_to_me)  );
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_xref_rectype, psz_ref_to_other, &pvh_ref_field_to_other)  );

                    // verify the to_me reference is pointing back to me
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_ref_field_to_me, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, &psz_should_be_my_rectype)  );
                    if (0 != strcmp(psz_should_be_my_rectype, psz_rectype))
                    {
                        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  ); // TODO better error
                    }

                    // get the other rectype
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_ref_field_to_other, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, &psz_other_rectype)  );
                    // and look it up
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_other_rectype, &pvh_other_rectype)  );

                    // add info so we know what tables to join
                    SG_ERR_CHECK(  sg_dbndx__add_join__XREF(
                                pCtx, 
                                pvh_joins, 
                                psz_xref_rectype, 
                                psz_ref_to_me,
                                psz_ref_to_other,
                                &psz_join_table_alias,
                                &psz_xref_table_alias)  );

                    // foreach field requested
                    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_other_fields, &count_other_fields)  );

                    for (i_other_field=0; i_other_field<count_other_fields; i_other_field++)
                    {
                        const char* psz_other_field = NULL;
                        SG_vhash* pvh_other_field = NULL;
                        const char* psz_other_field_type = NULL;

                        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_other_fields, i_other_field, &psz_other_field)  );
                        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_other_rectype, psz_other_field, &pvh_other_field)  );

                        // get the type of that field, because it will be the type
                        // of the resulting column in the query
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_other_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_other_field_type)  );
                      
                        // TODO we need a way to sort the contents of a collected
                        // join.
                        
                        // build the expr
                        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"%s\"", psz_join_table_alias, psz_other_field)  );

                        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                    MY_COLUMN__DEST,    MY_COLUMN__DEST__SUB,
                                    MY_COLUMN__SUB,     psz_alias,

                                    MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                    MY_COLUMN__ALIAS,   psz_other_field,
                                    MY_COLUMN__TYPE,    psz_other_field_type,

                                    NULL
                                    )  );
                        SG_STRING_NULLFREE(pCtx, pstr_expr);
                    }

                    if (psz_xref_recid_alias)
                    {
                        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"%s\"", psz_xref_table_alias, SG_ZING_FIELD__RECID)  );

                        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                    MY_COLUMN__DEST,    MY_COLUMN__DEST__SUB,
                                    MY_COLUMN__SUB,     psz_alias,

                                    MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                    MY_COLUMN__ALIAS,   psz_xref_recid_alias,
                                    MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,

                                    NULL
                                    )  );
                        SG_STRING_NULLFREE(pCtx, pstr_expr);
                    }

                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"hidrecrow\"", psz_xref_table_alias)  );

                    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                MY_COLUMN__DEST,    MY_COLUMN__DEST__SUB__KEY,
                                MY_COLUMN__SUB,     psz_alias,
                                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),

                                NULL
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr_expr);

                    b_multirow_joins = SG_TRUE;
                }
                else if (0 == strcmp(psz_expr_type, MY_EXPR__TYPE__TO_ME))
                {
                    const char* psz_other_rectype = NULL;
                    const char* psz_should_be_my_rectype = NULL;
                    const char* psz_ref = NULL;
                    const char* psz_alias = NULL;
                    SG_varray* pva_other_fields = NULL;
                    SG_vhash* pvh_other_rectype = NULL;
                    SG_vhash* pvh_ref_field = NULL;
                    SG_uint32 count_other_fields = 0;
                    SG_uint32 i_other_field = 0;
                    const char* psz_join_table_alias = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__RECTYPE, &psz_other_rectype)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__REF, &psz_ref)  );
                    SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_expr, MY_EXPR__FIELDS, &pva_other_fields)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__ALIAS, &psz_alias)  );

                    // lookup the other rectype
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_other_rectype, &pvh_other_rectype)  );
                    // and the given ref
                    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_other_rectype, psz_ref, &pvh_ref_field)  );
                    // verify the reference is pointing back to me
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_ref_field, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, &psz_should_be_my_rectype)  );

                    if (0 != strcmp(psz_should_be_my_rectype, psz_rectype))
                    {
                        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  ); // TODO better error
                    }

                    // add info so we know what tables to join
                    SG_ERR_CHECK(  sg_dbndx__add_join__TO_ME(
                                pCtx, 
                                pvh_joins, 
                                psz_other_rectype, 
                                psz_ref,
                                &psz_join_table_alias)  );
                        
                    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_other_fields, &count_other_fields)  );

                    for (i_other_field=0; i_other_field<count_other_fields; i_other_field++)
                    {
                        const char* psz_other_field = NULL;
                        SG_vhash* pvh_other_field = NULL;
                        const char* psz_other_field_type = NULL;

                        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_other_fields, i_other_field, &psz_other_field)  );
                        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_other_rectype, psz_other_field, &pvh_other_field)  );

                        // get the type of that field, because it will be the type
                        // of the resulting column in the query
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_other_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_other_field_type)  );
                      
                        // TODO we need a way to sort the contents of a collected
                        // join.

                        // build the expr
                        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"%s\"", psz_join_table_alias, psz_other_field)  );

                        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                    MY_COLUMN__DEST,    MY_COLUMN__DEST__SUB,
                                    MY_COLUMN__SUB,     psz_alias,

                                    MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                    MY_COLUMN__ALIAS,   psz_other_field,
                                    MY_COLUMN__TYPE,    psz_other_field_type,

                                    NULL
                                    )  );
                        SG_STRING_NULLFREE(pCtx, pstr_expr);
                    }

                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"hidrecrow\"", psz_join_table_alias)  );

                    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                MY_COLUMN__DEST,    MY_COLUMN__DEST__SUB__KEY,
                                MY_COLUMN__SUB,     psz_alias,
                                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),

                                NULL
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr_expr);

                    b_multirow_joins = SG_TRUE;
                }
                else if (0 == strcmp(psz_expr_type, MY_EXPR__TYPE__ALIAS))
                {
                    const char* psz_alias = NULL;
                    const char* psz_field_name = NULL;
                    SG_bool b_is_a_field = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__ALIAS, &psz_alias)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__FIELD, &psz_field_name)  );

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectype, psz_field_name, &b_is_a_field)  );
                    if (b_is_a_field)
                    {
                        const char* psz_field_type = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_rectype, psz_field_name, &pvh_field)  );
                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_field, SG_DBNDX_SCHEMA__FIELD__TYPE, &psz_field_type)  );

                        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s%s_%s\".\"%s\"", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype, psz_field_name)  );

                        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                    MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                                    MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                    MY_COLUMN__ALIAS,   psz_alias,
                                    MY_COLUMN__TYPE,    psz_field_type,
                                    NULL
                                    )  );
                        SG_STRING_NULLFREE(pCtx, pstr_expr);
                    }
                    else
                    {
						SG_ERR_THROW2(  SG_ERR_ZING_FIELD_NOT_FOUND, (pCtx, "%s.%s", psz_rectype, psz_field_name)  );
                    }
                }
                else if (0 == strcmp(psz_expr_type, MY_EXPR__TYPE__USERNAME))
                {
                    const char* psz_alias = NULL;
                    const char* psz_userid_field = NULL;
                    const char* psz_join_table_alias = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__USERID, &psz_userid_field)  );
                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_expr, MY_EXPR__ALIAS, &psz_alias)  );

                    // add info so we know what tables to join
                    SG_ERR_CHECK(  sg_dbndx__add_join__USERNAME(
                                pCtx, 
                                pvh_joins, 
                                psz_userid_field,
                                &psz_join_table_alias)  );

                    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
                    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "\"%s\".\"name\"", psz_join_table_alias)  );
                    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_result,
                                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                                MY_COLUMN__ALIAS,   psz_alias,
                                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,
                                NULL
                                )  );
                    SG_STRING_NULLFREE(pCtx, pstr_expr);
                }
                else
                {
                    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  ); // TODO better error
                }
            }
        }
    }

    if (pb_history)
    {
        *pb_history = b_history;
    }
    else
    {
        if (b_history)
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  ); // TODO better error
        }
    }

    if (pb_multirow_joins)
    {
        *pb_multirow_joins = b_multirow_joins;
    }
    else
    {
        if (b_multirow_joins)
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  ); // TODO better error
        }
    }

    *ppva = pva_result;
    pva_result = NULL;

    {
        SG_uint32 count = 0;

        // If pvh_joins is empty, don't even return it
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_joins, &count)  );
        if (!count)
        {
            SG_VHASH_NULLFREE(pCtx, pvh_joins);
        }
    }
    
    if (ppvh_joins)
    {
        *ppvh_joins = pvh_joins;
        pvh_joins = NULL;
    }
    else
    {
	// if the caller didn't provide ppvh_joins, then joins are not allowed.
        if (pvh_joins)
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  ); // TODO better error
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_expr);
    SG_VARRAY_NULLFREE(pCtx, pva_specific);
    SG_VARRAY_NULLFREE(pCtx, pva_result);
    SG_VHASH_NULLFREE(pCtx, pvh_joins);
}

static void sg_dbndx__format_column_list_in_sql(
	SG_context* pCtx,
    SG_varray* pva_columns,
    SG_string** ppstr
    )
{
    SG_uint32 count_fields = 0;
    SG_uint32 i = 0;
    SG_string* pstr = NULL;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_columns, &count_fields)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
    for (i=0; i<count_fields; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_expr = NULL;
        const char* psz_alias = NULL;
        const char* psz_dest = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__EXPR, &psz_expr)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        if (0 == strcmp(psz_dest, MY_COLUMN__DEST__RECORD))
        {
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__ALIAS, &psz_alias)  );
        }

        if (i > 0)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ", ")  );
        }

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "%s", psz_expr)  );
        if (psz_alias)
        {
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, " AS \"%s\"", psz_alias)  );
        }
    }

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void sg_dbndx__calc_query__fts(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_vhash* pvh_schema,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_keywords,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_columns,
    SG_vhash* pvh_joins,
    SG_bool b_history,
    SG_bool b_multirow_joins,
    SG_bool* pb_usernames,
    SG_string** ppstr
	)
{
    SG_string* pstr_query = NULL;
    SG_string* pstr_fields = NULL;
    SG_string* pstr_limit = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_string* pstr_table = NULL;
    SG_string* pstr_fts = NULL;
    SG_string* pstr_map_fts = NULL;
    char* psz_escaped = NULL;
    SG_string* pstr_expr = NULL;
    SG_uint32 count_fts_fields = 0;
    SG_vhash* pvh_fields = NULL;
    SG_uint32 count_fields = 0;
    SG_uint32 i_field = 0;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_columns);
	SG_NULLARGCHECK_RETURN(ppstr);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_table)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_table, "%s%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype)  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_fts)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_fts, "%s_%s", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_map_fts)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_map_fts, "%s_map_%s", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );

    // need to know how many fts fields there are, so we can pass the correct number of arguments to sg_ft_rank
    // TODO or, we could modify sg_ft_rank to remove the weights, since we don't use them anyway
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_fields)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fields, &count_fields)  );
    for (i_field=0; i_field<count_fields; i_field++)
    {
        SG_vhash* pvh_one_field = NULL;
        SG_bool b_has = SG_FALSE;
        SG_bool b_full_text_search = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_fields, i_field, NULL, &pvh_one_field)  );
        
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

    // add another column for the fts_score
    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_expr)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_expr, "round(1000 * sg_ft_rank(matchinfo(\"%s\")", SG_string__sz(pstr_fts))  );
    for (i_field=0; i_field<count_fts_fields; i_field++)
    {
        // all fields are given equal weight
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_expr, ",1.0")  );
    }
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_expr, "))")  );

    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                MY_COLUMN__EXPR,    SG_string__sz(pstr_expr),
                MY_COLUMN__ALIAS,   "fts_score",
                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__INT,
                NULL
                )  );
    SG_STRING_NULLFREE(pCtx, pstr_expr);
    
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    SG_ERR_CHECK(  sg_dbndx__format_column_list_in_sql(pCtx, pva_columns, &pstr_fields)  );

    if (iNumRecordsToReturn || iNumRecordsToSkip)
    {
        if (!b_history && !b_multirow_joins)
        {
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_limit)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_limit, " LIMIT %d OFFSET %d", iNumRecordsToReturn, iNumRecordsToSkip)  );
        }
    }

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_query, "SELECT ")  );
    SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_fields)  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " FROM ")  );
    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, "\"%s\"", SG_string__sz(pstr_table))  );

    if (pvh_joins)
    {
        SG_ERR_CHECK(  sg_dbndx__add_joins(pCtx, psz_rectype, pvh_schema, pstr_table, pvh_joins, pstr_query, pidState, pb_usernames)  );
    }

    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"%s\" ON (\"%s\".hidrecrow = \"%s\".hidrecrow) ", 
                SG_string__sz(pstr_map_fts),
                SG_string__sz(pstr_map_fts),
                SG_string__sz(pstr_table)
                )  );
    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"%s\" ON (\"%s\".ftsrowid = \"%s\".rowid) ", 
                SG_string__sz(pstr_fts),
                SG_string__sz(pstr_map_fts),
                SG_string__sz(pstr_fts)
                )  );

    if (b_history)
    {
        if (SG_DAGNUM__HAS_NO_RECID(pndx->iDagNum))
        {
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN db_history ON (db_history.hidrecrow = \"%s\".hidrecrow) ", SG_string__sz(pstr_table))  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN %s_%s hr ON (hr.recid = \"%s\".recid) ", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, SG_string__sz(pstr_table))  );
        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN db_history ON (db_history.hidrecrow = hr.hidrecrow) ")  );
        }

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) ")  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN hidrecs h_hidrecs ON (db_history.hidrecrow = h_hidrecs.hidrecrow) ")  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN csids h_csids ON (db_history.csidrow = h_csids.csidrow) ")  );

        if (SG_DAGNUM__USERS != pndx->iDagNum)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN shadow_users ON (db_audits.userid = shadow_users.userid) ")  );
        }
    }

    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " WHERE (")  );
    SG_ERR_CHECK(  SG_sqlite__escape(pCtx, psz_keywords, &psz_escaped)  );
    // if psz_field_name is NULL, we end up just giving the name of the table, which is the fts4 hidden column, which tells it to match any of the columns
    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query,
                "\"%s\"%s%s MATCH '%s'", 
                SG_string__sz(pstr_fts),
                 psz_field_name?".":"",
                 psz_field_name?psz_field_name:"",
                psz_escaped ? psz_escaped : psz_keywords
                )  );
    SG_NULLFREE(pCtx, psz_escaped);
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " )")  );

    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " ORDER BY fts_score DESC")  );

    if (pstr_limit)
    {
        SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_limit)  );
    }

    //fprintf(stderr, "%s\n", SG_string__sz(pstr_query));

    *ppstr = pstr_query;
    pstr_query = NULL;

fail:
    SG_NULLFREE(pCtx, psz_escaped);
    SG_STRING_NULLFREE(pCtx, pstr_fts);
    SG_STRING_NULLFREE(pCtx, pstr_map_fts);
    SG_STRING_NULLFREE(pCtx, pstr_table);
    SG_STRING_NULLFREE(pCtx, pstr_fields);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_STRING_NULLFREE(pCtx, pstr_limit);
}

static void sg_dbndx__add_joins__sub(
	SG_context* pCtx,
    const char* psz_my_rectype,
    SG_vhash* pvh_schema,
    SG_string* pstr_table,
    SG_vhash* pvh_joins,
    SG_string* pstr_query,
    const char* pidState,
    SG_bool* pb_usernames
    )
{
    SG_uint32 count_joins = 0;
    SG_uint32 i = 0;
    SG_string* pstr_other_table = NULL;
    SG_string* pstr_xref_table = NULL;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_joins, &count_joins)  );
    for (i=0; i<count_joins; i++)
    {
        SG_vhash* pvh_join = NULL;
        const char* psz_type = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_joins, i, NULL, &pvh_join)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TYPE, &psz_type)  );
        if (0 == strcmp(psz_type, MY_JOIN__TYPE__FROM_ME))
        {
            const char* psz_ref_to_other = NULL;
            const char* psz_other_rectype = NULL;
            SG_vhash* pvh_ref_field = NULL;
            SG_vhash* pvh_my_rectype = NULL;
            const char* psz_table_alias = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, &psz_table_alias)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__REF_TO_OTHER, &psz_ref_to_other)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_my_rectype, &pvh_my_rectype)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_my_rectype, psz_ref_to_other, &pvh_ref_field)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_ref_field, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, &psz_other_rectype)  );
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_other_table)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_other_table, "%s%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_other_rectype)  );

            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, 
                        " LEFT OUTER JOIN \"%s\" AS \"%s\" ON (\"%s\".\"%s\" = \"%s\".recid) ", 
                        SG_string__sz(pstr_other_table), 
                        psz_table_alias, 
                        
                        SG_string__sz(pstr_table), 
                        psz_ref_to_other, 
                        psz_table_alias
                        )  );

            SG_STRING_NULLFREE(pCtx, pstr_other_table);
        }
        else if (0 == strcmp(psz_type, MY_JOIN__TYPE__TO_ME))
        {
            const char* psz_ref_to_me = NULL;
            const char* psz_rectype = NULL;
            const char* psz_table_alias = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, &psz_table_alias)  );

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__REF_TO_ME, &psz_ref_to_me)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__RECTYPE, &psz_rectype)  );

            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_other_table)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_other_table, "%s%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype)  );

            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, 
                        " LEFT OUTER JOIN \"%s\" AS \"%s\"  ON (\"%s\".\"%s\" = \"%s\".recid) ", 
                        SG_string__sz(pstr_other_table), 
                        psz_table_alias, 
                        
                        psz_table_alias, 
                        psz_ref_to_me, 
                        
                        SG_string__sz(pstr_table)
                        )  );
            SG_STRING_NULLFREE(pCtx, pstr_other_table);
        }
        else if (0 == strcmp(psz_type, MY_JOIN__TYPE__XREF))
        {
            const char* psz_ref_to_other = NULL;
            const char* psz_ref_to_me = NULL;
            const char* psz_xref_rectype = NULL;
            SG_vhash* pvh_xref_rectype = NULL;
            SG_vhash* pvh_ref_field = NULL;
            const char* psz_other_rectype = NULL;
            const char* psz_table_alias = NULL;
            const char* psz_xref_table_alias = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__XREF_TABLE_ALIAS, &psz_xref_table_alias)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, &psz_table_alias)  );

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__REF_TO_ME, &psz_ref_to_me)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__REF_TO_OTHER, &psz_ref_to_other)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__RECTYPE, &psz_xref_rectype)  );

            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_xref_rectype, &pvh_xref_rectype)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_xref_rectype, psz_ref_to_other, &pvh_ref_field)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_ref_field, SG_DBNDX_SCHEMA__FIELD__REF_RECTYPE, &psz_other_rectype)  );

            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_xref_table)  );
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_other_table)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_other_table, "%s%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_other_rectype)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_xref_table, "%s%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_xref_rectype)  );

            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN \"%s\" AS \"%s\" ON (\"%s\".\"%s\" = \"%s\".recid) LEFT OUTER JOIN \"%s\" AS \"%s\" ON (\"%s\".\"%s\" = \"%s\".recid)", 
                        SG_string__sz(pstr_xref_table), 
                        psz_xref_table_alias,

                        psz_xref_table_alias, 
                        psz_ref_to_me, 
                        SG_string__sz(pstr_table),

                        SG_string__sz(pstr_other_table), 
                        psz_table_alias,

                        psz_xref_table_alias, 
                        psz_ref_to_other, 
                        psz_table_alias
                        )  );
            SG_STRING_NULLFREE(pCtx, pstr_other_table);
            SG_STRING_NULLFREE(pCtx, pstr_xref_table);

        }
        else if (0 == strcmp(psz_type, MY_JOIN__TYPE__USERNAME))
        {
            const char* psz_ref_to_other = NULL;
            const char* psz_table_alias = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__TABLE_ALIAS, &psz_table_alias)  );

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_join, MY_JOIN__REF_TO_OTHER, &psz_ref_to_other)  );
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN \"shadow_users\" AS \"%s\" ON (\"%s\".\"%s\" = \"%s\".userid) ", psz_table_alias, SG_string__sz(pstr_table), psz_ref_to_other, psz_table_alias)  );
            *pb_usernames = SG_TRUE;
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_other_table);
    SG_STRING_NULLFREE(pCtx, pstr_xref_table);
}

static void sg_dbndx__add_joins(
	SG_context* pCtx,
    const char* psz_my_rectype,
    SG_vhash* pvh_schema,
    SG_string* pstr_table,
    SG_vhash* pvh_joins,
    SG_string* pstr_query,
    const char* pidState,
    SG_bool* pb_usernames
    )
{
    SG_uint32 count_joins = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_joins, &count_joins)  );
    for (i=0; i<count_joins; i++)
    {
        SG_vhash* pvh_sub = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_joins, i, NULL, &pvh_sub)  );
        SG_ERR_CHECK(  sg_dbndx__add_joins__sub(pCtx, psz_my_rectype, pvh_schema, pstr_table, pvh_sub, pstr_query, pidState, pb_usernames)  );
    }

fail:
    ;
}

static void sg_dbndx__add_fts_part(
	SG_context* pCtx,
    SG_string* pstr_table,
    SG_vhash* pvh_fts,
    SG_string* pstr_query
    )
{
    SG_string* pstr_fts = NULL;
    SG_string* pstr_map_fts = NULL;

    SG_uint32 count_fts = 0;
    SG_uint32 i_fts = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_fts, &count_fts)  );
    for (i_fts=0; i_fts<count_fts; i_fts++)
    {
        const char* psz_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_fts, i_fts, &psz_rectype, NULL)  );

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_fts)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_fts, "%s_%s", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );
        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_map_fts)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_map_fts, "%s_map_%s", SG_DBNDX_FTS_TABLE_PREFIX, psz_rectype)  );

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"%s\" ON (\"%s\".hidrecrow = \"%s\".hidrecrow) ", 
                    SG_string__sz(pstr_map_fts),
                    SG_string__sz(pstr_map_fts),
                    SG_string__sz(pstr_table)
                    )  );
        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"%s\" ON (\"%s\".ftsrowid = \"%s\".rowid) ", 
                    SG_string__sz(pstr_fts),
                    SG_string__sz(pstr_map_fts),
                    SG_string__sz(pstr_fts)
                    )  );
        SG_STRING_NULLFREE(pCtx, pstr_fts);
        SG_STRING_NULLFREE(pCtx, pstr_map_fts);
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_fts);
    SG_STRING_NULLFREE(pCtx, pstr_map_fts);
}

static void sg_dbndx__calc_query__recent(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_vhash* pvh_schema,
    const char* psz_rectype,
	const SG_varray* pcrit,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_columns,
    SG_string** ppstr,
    SG_vhash* pvh_in,
    SG_vhash* pvh_columns_used
	)
{
    SG_string* pstr_query = NULL;
    SG_string* pstr_fields = NULL;
    SG_string* pstr_where = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_string* pstr_table = NULL;
    SG_vhash* pvh_fts = NULL;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_columns);
	SG_NULLARGCHECK_RETURN(ppstr);

    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                MY_COLUMN__EXPR,    "h_csids.csid",
                MY_COLUMN__ALIAS,   "csid",
                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,
                NULL
                )  );
    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                MY_COLUMN__EXPR,    "h_csids.generation",
                MY_COLUMN__ALIAS,   "generation",
                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__INT,
                NULL
                )  );
    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                MY_COLUMN__EXPR,    "db_audits.userid",
                MY_COLUMN__ALIAS,   SG_AUDIT__USERID,
                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,
                NULL
                )  );
    if (SG_DAGNUM__USERS != pndx->iDagNum)
    {
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                    MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                    MY_COLUMN__EXPR,    "shadow_users.name",
                    MY_COLUMN__ALIAS,   SG_AUDIT__USERNAME,
                    MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,
                    NULL
                    )  );
    }
    SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                MY_COLUMN__DEST,    MY_COLUMN__DEST__RECORD,
                MY_COLUMN__EXPR,    "db_audits.timestamp",
                MY_COLUMN__ALIAS,   SG_AUDIT__TIMESTAMP,
                MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__INT,
                NULL
                )  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    SG_ERR_CHECK(  sg_dbndx__format_column_list_in_sql(pCtx, pva_columns, &pstr_fields)  );

    if (pcrit)
    {
        SG_ERR_CHECK(  sg_dbndx__calc_where(pCtx, NULL, psz_rectype, pvh_rectype_fields, pcrit, &pstr_where, &pvh_fts, pvh_in, pvh_columns_used)  );
    }

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_table)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_table, "%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype)  );

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_query, "SELECT ")  );
    SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_fields)  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " FROM ")  );
    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, "\"%s\"", SG_string__sz(pstr_table))  );

    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"hidrecs\" ON (\"%s\".hidrecrow = hidrecs.hidrecrow) ", SG_string__sz(pstr_table))  );

    if (pvh_fts)
    {
        SG_ERR_CHECK(  sg_dbndx__add_fts_part(pCtx, pstr_table, pvh_fts, pstr_query)  );
    }

    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN db_history ON (db_history.hidrecrow = \"%s\".hidrecrow) ", SG_string__sz(pstr_table))  );

    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " INNER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) ")  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN hidrecs h_hidrecs ON (db_history.hidrecrow = h_hidrecs.hidrecrow) ")  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN csids h_csids ON (db_history.csidrow = h_csids.csidrow) ")  );

    if (SG_DAGNUM__USERS != pndx->iDagNum)
    {
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN shadow_users ON (db_audits.userid = shadow_users.userid) ")  );
    }

    if (pstr_where)
    {
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " WHERE (")  );
        SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_where)  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " )")  );
    }

    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " ORDER BY db_audits.timestamp DESC")  );

    if (iNumRecordsToReturn || iNumRecordsToSkip)
    {
        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LIMIT %d OFFSET %d", iNumRecordsToReturn, iNumRecordsToSkip)  );
    }

    //printf("%s\n", SG_string__sz(pstr_query));

    *ppstr = pstr_query;
    pstr_query = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_fts);
    SG_STRING_NULLFREE(pCtx, pstr_table);
    SG_STRING_NULLFREE(pCtx, pstr_fields);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_STRING_NULLFREE(pCtx, pstr_where);
}

static void sg_dbndx__calc_query(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_vhash* pvh_schema,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
	const SG_varray* pSort,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_columns,
    SG_vhash* pvh_joins,
    SG_bool b_history,
    SG_bool b_multirow_joins,
    SG_bool* pb_usernames,
    SG_string** ppstr,
    SG_vhash* pvh_in,
    SG_vhash* pvh_columns_used
	)
{
    SG_string* pstr_query = NULL;
    SG_string* pstr_fields = NULL;
    SG_string* pstr_where = NULL;
    SG_string* pstr_orderby = NULL;
    SG_string* pstr_limit = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_vhash* pvh_fts = NULL;
    SG_string* pstr_table = NULL;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_columns);
	SG_NULLARGCHECK_RETURN(ppstr);

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    SG_ERR_CHECK(  sg_dbndx__format_column_list_in_sql(pCtx, pva_columns, &pstr_fields)  );

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_table)  );
    SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_table, "%s%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, pidState?pidState:"", psz_rectype)  );

    if (pcrit)
    {
        SG_ERR_CHECK(  sg_dbndx__calc_where(pCtx, pidState, psz_rectype, pvh_rectype_fields, pcrit, &pstr_where, &pvh_fts, pvh_in, pvh_columns_used)  );
    }

    if (pSort)
    {
        SG_ERR_CHECK(  sg_dbndx__calc_orderby(pCtx, pidState, psz_rectype, pvh_rectype_fields, pSort, &pstr_orderby)  );
    }

    if (iNumRecordsToReturn || iNumRecordsToSkip)
    {
        if (!b_history && !b_multirow_joins)
        {
            // If we have to do limit/skip while dealing with multiple rows per record,
            // then we have to do it the tedious way.

            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_limit)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_limit, " LIMIT %d OFFSET %d", iNumRecordsToReturn, iNumRecordsToSkip)  );
        }
    }

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_query, "SELECT ")  );
    SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_fields)  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " FROM ")  );
    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, "\"%s\"", SG_string__sz(pstr_table))  );

    if (pvh_joins)
    {
        SG_ERR_CHECK(  sg_dbndx__add_joins(pCtx, psz_rectype, pvh_schema, pstr_table, pvh_joins, pstr_query, pidState, pb_usernames)  );
    }

    SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"hidrecs\" ON (\"%s\".hidrecrow = hidrecs.hidrecrow) ", SG_string__sz(pstr_table))  );

    if (pvh_fts)
    {
        SG_ERR_CHECK(  sg_dbndx__add_fts_part(pCtx, pstr_table, pvh_fts, pstr_query)  );
    }

    if (b_history)
    {
        if (SG_DAGNUM__HAS_NO_RECID(pndx->iDagNum))
        {
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN db_history ON (db_history.hidrecrow = \"%s\".hidrecrow) ", SG_string__sz(pstr_table))  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN %s_%s hr ON (hr.recid = \"%s\".recid) ", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype, SG_string__sz(pstr_table))  );
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " LEFT OUTER JOIN db_history ON (db_history.hidrecrow = hr.hidrecrow) ")  );
        }

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN db_audits ON (db_audits.csidrow = db_history.csidrow) ")  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN hidrecs h_hidrecs ON (db_history.hidrecrow = h_hidrecs.hidrecrow) ")  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN csids h_csids ON (db_history.csidrow = h_csids.csidrow) ")  );

        if (SG_DAGNUM__USERS != pndx->iDagNum)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " LEFT OUTER JOIN shadow_users ON (db_audits.userid = shadow_users.userid) ")  );
        }
    }

    if (b_history && !pidState)
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_HISTORY_IF_NO_STATE  );
    }

    if (pstr_where)
    {
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " WHERE (")  );
        SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_where)  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " )")  );
    }

    if (pstr_orderby)
    {
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " ORDER BY ")  );
        SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_orderby)  );
    }

    if (pstr_limit)
    {
        SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_limit)  );
    }

    //printf("%s\n", SG_string__sz(pstr_query));

    *ppstr = pstr_query;
    pstr_query = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_fts);
    SG_STRING_NULLFREE(pCtx, pstr_table);
    SG_STRING_NULLFREE(pCtx, pstr_fields);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_STRING_NULLFREE(pCtx, pstr_where);
    SG_STRING_NULLFREE(pCtx, pstr_orderby);
    SG_STRING_NULLFREE(pCtx, pstr_limit);
}

static void sg_dbndx__add_field(
	SG_context* pCtx,
    SG_vhash* pvh_cur,
    const char* psz_alias,
    SG_uint16 field_type,
    sqlite3_stmt* pStmt,
    SG_uint32 i
    )
{
    const char* psz_val = NULL;

    psz_val = (const char*) sqlite3_column_text(pStmt, i);
    if (psz_val)
    {
        switch (field_type)
        {
            case SG_ZING_TYPE__BOOL:
            {
                int v = (int) sqlite3_column_int(pStmt, i);

                SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_cur, psz_alias, v?SG_TRUE:SG_FALSE)  );
            }
            break;

            case SG_ZING_TYPE__DATETIME:
            {
                SG_int64 v = (SG_int64) sqlite3_column_int64(pStmt, i);

                SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_cur, psz_alias, v)  );
            }
            break;

            case SG_ZING_TYPE__INT:
            {
                SG_int64 v = (SG_int64) sqlite3_column_int64(pStmt, i);

                SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_cur, psz_alias, v)  );
            }
            break;

            case SG_ZING_TYPE__STRING:
            case SG_ZING_TYPE__ATTACHMENT:
            case SG_ZING_TYPE__USERID:
            case SG_ZING_TYPE__REFERENCE:
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur, psz_alias, psz_val)  );
            }
            break;
            
            default:
            {
                SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
            }
            break;
        }
    }
    else
    {
        // do nothing for null values
    }

fail:
    ;
}

static void sg_dbndx__read_fields(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    SG_varray* pva_columns,
    SG_vhash* pvh_cur
    )
{
    SG_uint32 count_columns = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_columns, &count_columns)  );
    for (i=0; i<count_columns; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_dest = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        if (0 == strcmp(psz_dest, MY_COLUMN__DEST__RECORD))
        {
            SG_uint16 field_type = 0;
            const char* psz_alias = NULL;
            const char* psz_field_type = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__ALIAS, &psz_alias)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__TYPE, &psz_field_type)  );
            SG_ERR_CHECK(  SG_zing__field_type__sz_to_uint16(pCtx, psz_field_type, &field_type)  );

            SG_ERR_CHECK(  sg_dbndx__add_field(pCtx, pvh_cur, psz_alias, field_type, pStmt, i)  );
        }
    }

fail:
    ;
}

static void sg_dbndx__skip_one_possibly_multirow_record(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    int* pi_rc
    )
{
    int rc = -1;
    char cur_key[SG_HID_MAX_BUFFER_LENGTH];

    const char* psz_key = (const char*) sqlite3_column_text(pStmt, 0);
    SG_ERR_CHECK(  SG_strcpy(pCtx, cur_key, sizeof(cur_key), psz_key)  );

	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
        psz_key = (const char*) sqlite3_column_text(pStmt, 0);

        if (0 != strcmp(psz_key, cur_key))
        {
            break;
        }
    }

    *pi_rc = rc;

fail:
    ;
}

struct sg_multirow_record_state
{
    SG_varray* pva_columns;
    SG_vhash* pvh_rec;

    SG_vhash* pvh_hist;
    SG_bool b_history;

    SG_vhash* pvh_sub_keys_done;
};

static void sg_dbndx__read_joins(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    struct sg_multirow_record_state* pst
    )
{
    SG_uint32 count_columns = 0;
    SG_uint32 i = 0;
    SG_rbtree* prb_subs = NULL;
    SG_vhash* pvh_freeme = NULL;
    const char* psz_sub = NULL;
	SG_rbtree_iterator* pit = NULL;
    SG_bool b = SG_FALSE;
    SG_vhash* pvh_sub = NULL;
    SG_vhash* pvh_subs_do = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_subs_do)  );

    // first find all the subkeys in this row and figure out
    // which subs we're going to do or not
    SG_ERR_CHECK(  SG_varray__count(pCtx, pst->pva_columns, &count_columns)  );
    for (i=0; i<count_columns; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_dest = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pst->pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        if (0 == strcmp(psz_dest, MY_COLUMN__DEST__SUB__KEY))
        {
            SG_bool b_do = SG_TRUE;
            SG_vhash* pvh_keys_done = NULL;
            const char* psz_key = (const char*) sqlite3_column_text(pStmt, i);

            if (psz_key)
            {
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__SUB, &psz_sub)  );
                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pst->pvh_sub_keys_done, psz_sub, &pvh_keys_done)  );
                if (pvh_keys_done)
                {
                    SG_bool b_has = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_keys_done, psz_key, &b_has)  );
                    if (b_has)
                    {
                        b_do = SG_FALSE;
                    }
                }
                else
                {
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pst->pvh_sub_keys_done, psz_sub, &pvh_keys_done)  );
                }

                if (b_do)
                {
                    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_subs_do, psz_sub)  );
                }

                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_keys_done, psz_key)  );
            }
        }
    }

    SG_ERR_CHECK(  SG_varray__count(pCtx, pst->pva_columns, &count_columns)  );
    for (i=0; i<count_columns; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_dest = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pst->pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        if (0 == strcmp(psz_dest, MY_COLUMN__DEST__SUB))
        {
            const char* psz_alias = NULL;
            const char* psz_type = NULL;
            SG_bool b = SG_FALSE;
            SG_uint16 field_type = 0;
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__SUB, &psz_sub)  );

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_subs_do, psz_sub, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__ALIAS, &psz_alias)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__TYPE, &psz_type)  );
                SG_ERR_CHECK(  SG_zing__field_type__sz_to_uint16(pCtx, psz_type, &field_type)  );

                if (!prb_subs)
                {
                    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &prb_subs)  );
                }

                b = SG_FALSE;
                SG_ERR_CHECK(  SG_rbtree__find(pCtx, prb_subs, psz_sub, &b, (void**) &pvh_sub)  );
                if (!b)
                {
                    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_freeme)  );
                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_subs, psz_sub, pvh_freeme)  );
                    pvh_sub = pvh_freeme;
                    pvh_freeme = NULL;
                }

                SG_ERR_CHECK(  sg_dbndx__add_field(pCtx, pvh_sub, psz_alias, field_type, pStmt, i)  );
            }
        }
    }

    if (prb_subs)
    {
        // now take all the subs out of the rbtree and put them in the
        // right place in the rec.

        // copy all the fields into the dbrecord
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_subs, &b, &psz_sub, (void**) &pvh_sub)  );
        while (b)
        {
            SG_varray* pva = NULL;

            SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pst->pvh_rec, psz_sub, &pva)  );
            if (!pva)
            {
                SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pst->pvh_rec, psz_sub, &pva)  );            
            }

            SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva, &pvh_sub)  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_sub, (void**) &pvh_sub)  );
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_subs_do);
    SG_RBTREE_NULLFREE(pCtx, prb_subs);
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
}

static void sg_dbndx__read_history(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    struct sg_multirow_record_state* pst
    )
{
    const char* psz_history_hidrec = NULL;
    const char* psz_csid = NULL;
    SG_int64 i_generation = -1;
    const char* psz_userid = NULL;
    const char* psz_username = NULL;
    SG_int64 i_timestamp = -1;

    SG_uint32 count_columns = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pst->pva_columns, &count_columns)  );
    for (i=0; i<count_columns; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_dest = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pst->pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        if (0 == strcmp(psz_dest, "history_hidrec"))
        {
            psz_history_hidrec = (const char*) sqlite3_column_text(pStmt, i);
        }
        else if (0 == strcmp(psz_dest, "history_csid"))
        {
            psz_csid = (const char*) sqlite3_column_text(pStmt, i);
        }
        else if (0 == strcmp(psz_dest, "history_generation"))
        {
            i_generation =  sqlite3_column_int64(pStmt, i);
        }
        else if (0 == strcmp(psz_dest, "audit_userid"))
        {
            psz_userid = (const char*) sqlite3_column_text(pStmt, i);
        }
        else if (0 == strcmp(psz_dest, "audit_username"))
        {
            psz_username = (const char*) sqlite3_column_text(pStmt, i);
        }
        else if (0 == strcmp(psz_dest, "audit_timestamp"))
        {
            i_timestamp =  sqlite3_column_int64(pStmt, i);
        }
    }

    if (psz_csid)
    {
        SG_bool b_has = SG_FALSE;
        SG_vhash* pvh_csid = NULL;
        SG_vhash* pvh_audits = NULL;

        // TODO we might want to verify that ALL of the fields are valid

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pst->pvh_hist, psz_csid, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pst->pvh_hist, psz_csid, &pvh_csid)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_csid, "audits", &pvh_audits)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pst->pvh_hist, psz_csid, &pvh_csid)  );
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_csid, "audits", &pvh_audits)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_csid, "csid", psz_csid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_csid, "hidrec", psz_history_hidrec)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_csid, "generation", i_generation)  );
        }

        if (psz_userid)
        {
            char buf_key[SG_GID_BUFFER_LENGTH + 24 + 4];
            SG_int_to_string_buffer sz_i;
            char zeros[32];
            int len = 0;

            SG_int64_to_sz(i_timestamp, sz_i);
            len = SG_STRLEN(sz_i);
            SG_ERR_CHECK(  SG_strcpy(pCtx, zeros, sizeof(zeros), "000000000000000000000000000000")  );
            zeros[24-len] = 0;

            SG_ERR_CHECK(  SG_sprintf(pCtx, buf_key, sizeof(buf_key), "%s%s_%s", zeros, sz_i, psz_userid)  );

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_audits, buf_key, &b_has)  );
            if (!b_has)
            {
                SG_vhash* pvh_new_audit = NULL;

                SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_audits, buf_key, &pvh_new_audit)  );
                // TODO the key already contains the timestamp and the userid.  no need to put them in the vhash too.
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new_audit, SG_AUDIT__USERID, psz_userid)  );
                SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_new_audit, SG_AUDIT__TIMESTAMP, i_timestamp)  );
                if (psz_username)
                {
                    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new_audit, SG_AUDIT__USERNAME, psz_username)  );
                }
            }
        }
    }

fail:
    ;
}

static void sg_dbndx__multirow__read_first(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    struct sg_multirow_record_state* pst
    )
{
    SG_ERR_CHECK(  sg_dbndx__read_fields(pCtx, pStmt, pst->pva_columns, pst->pvh_rec)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pst->pvh_hist)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pst->pvh_sub_keys_done)  );

    SG_ERR_CHECK(  sg_dbndx__read_history(pCtx, pStmt, pst)  );
    SG_ERR_CHECK(  sg_dbndx__read_joins(pCtx, pStmt, pst)  );

fail:
    ;
}

static void sg_dbndx__multirow__read_another(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    struct sg_multirow_record_state* pst
    )
{
    // I guess we *could* check here to see if all the fields
    // in this row match the ones we already read.  They should.

    SG_ERR_CHECK(  sg_dbndx__read_history(pCtx, pStmt, pst)  );
    SG_ERR_CHECK(  sg_dbndx__read_joins(pCtx, pStmt, pst)  );

fail:
    ;
}

static void sg_dbndx__multirow__finish(
	SG_context* pCtx,
    struct sg_multirow_record_state* pst
    )
{
    if (pst->b_history)
    {
        SG_varray* pva_history = NULL;
        SG_uint32 count_changesets = 0;
        SG_uint32 i_changeset = 0;

        SG_ERR_CHECK(  SG_vhash__sort__vhash_field_int__desc(pCtx, pst->pvh_hist, "generation")  );
       
        // copy the hist in.
       
        // TODO if the "history" field of a record used vhashes instead
        // of varrays, then we could just copy it easily instead of having
        // to reformat things like this.

        SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pst->pvh_rec, SG_ZING_FIELD__HISTORY, &pva_history)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pst->pvh_hist, &count_changesets)  );
        for (i_changeset=0; i_changeset<count_changesets; i_changeset++)
        {
            const char* psz_csid = NULL;
            SG_vhash* pvh_changeset = NULL;
            SG_vhash* pvh_copy_changeset = NULL;
            SG_vhash* pvh_audits = NULL;
            const char* psz_history_hidrec = NULL;
            SG_int64 i_generation = -1;
            SG_varray* pva_copy_audits = NULL;
            SG_uint32 count_audits = 0;
            SG_uint32 i_audit = 0;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pst->pvh_hist, i_changeset, &psz_csid, &pvh_changeset)  );
            SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_history, &pvh_copy_changeset)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_changeset, "hidrec", &psz_history_hidrec)  );
            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_changeset, "generation", &i_generation)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_copy_changeset, "csid", psz_csid)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_copy_changeset, "hidrec", psz_history_hidrec)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_copy_changeset, "generation", i_generation)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_changeset, "audits", &pvh_audits)  );
            // sort the audits
            // TODO we could probably just sort on the key, now that it's the timestamp+userid
            SG_ERR_CHECK(  SG_vhash__sort__vhash_field_int__desc(pCtx, pvh_audits, SG_AUDIT__TIMESTAMP)  );
            SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_copy_changeset, "audits", &pva_copy_audits)  );
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_audits, &count_audits)  );
            for (i_audit=0; i_audit<count_audits; i_audit++)
            {
                SG_vhash* pvh_one_audit = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_audits, i_audit, NULL, &pvh_one_audit)  );

                SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pva_copy_audits, pvh_one_audit, NULL)  );
            }
        }
    }

fail:
    ;
}

void x_row_stderr(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    SG_varray* pva_columns,
    const char* psz
    )
{
    SG_uint32 count_columns = 0;
    SG_uint32 i = 0;

    fprintf(stderr, "%s\n", psz);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_columns, &count_columns)  );
    for (i=0; i<count_columns; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_dest = NULL;
        const char* psz_val = NULL;
        const char* psz_alias = NULL;
        const char* psz_field_type = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_column, MY_COLUMN__ALIAS, &psz_alias)  );
        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_column, MY_COLUMN__TYPE, &psz_field_type)  );
        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        psz_val = (const char*) sqlite3_column_text(pStmt, i);

        if (0 == strcmp(psz_dest, MY_COLUMN__DEST__RECORD))
        {
            fprintf(stderr, "    %s (%s) -- %s\n", psz_dest, psz_alias, psz_val);
        }
        else
        {
            fprintf(stderr, "    %s -- %s\n", psz_dest, psz_val);
        }
    }

fail:
    ;
}

static void sg_dbndx__read_one_possibly_multirow_record(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    SG_varray* pva_columns,
    SG_bool b_history,
    int* pi_rc,
    SG_vhash* pvh_rec
    )
{
    int rc = -1;
    char cur_key[SG_HID_MAX_BUFFER_LENGTH];
    struct sg_multirow_record_state st;
    const char* psz_key;
	
	memset(&st, 0, sizeof(st));
	
	psz_key = (const char*) sqlite3_column_text(pStmt, 0);
    SG_ERR_CHECK(  SG_strcpy(pCtx, cur_key, sizeof(cur_key), psz_key)  );

    //SG_ERR_CHECK(  x_row_stderr(pCtx, pStmt, pva_columns, "row:")  );

    st.pvh_rec = pvh_rec;
    st.pva_columns = pva_columns;
    st.b_history = b_history;

    SG_ERR_CHECK(  sg_dbndx__multirow__read_first(pCtx, pStmt, &st)  );
	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
        //SG_ERR_CHECK(  x_row_stderr(pCtx, pStmt, pva_columns, "row:")  );

        psz_key = (const char*) sqlite3_column_text(pStmt, 0);

        if (0 != strcmp(psz_key, cur_key))
        {
            // this row belongs to the next record.  we're done.
            break;
        }

        SG_ERR_CHECK(  sg_dbndx__multirow__read_another(pCtx, pStmt, &st)  );
    }

    SG_ERR_CHECK(  sg_dbndx__multirow__finish(pCtx, &st)  );

    *pi_rc = rc;

fail:
    SG_VHASH_NULLFREE(pCtx, st.pvh_hist);
    SG_VHASH_NULLFREE(pCtx, st.pvh_sub_keys_done);
}

static void SG_dbndx_query__query__one__hidrec(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_hidrec,
    SG_varray* pva_fields,
    SG_vhash** ppvh
	)
{
    SG_string* pstr_query = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = 0;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_joins = NULL;
    SG_vhash* pvh_cur = NULL;
    SG_bool b_history = SG_FALSE;
    SG_bool b_multirow_joins = SG_FALSE;
    SG_varray* pva_crit = NULL;
    SG_vhash* pvh_in = NULL;
    SG_vhash* pvh_schema = NULL;
    SG_bool b_usernames = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_fields);
	SG_NULLARGCHECK_RETURN(ppvh);
	SG_NULLARGCHECK_RETURN(pidState);

    if (SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(pndx->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_UNSPECIFIED  ); // TODO better error
    }

    SG_ERR_CHECK(  sg_dbndx__update_cur_state(pCtx, pndx, pidState, &pvh_schema)  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        const char* psz_only_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_schema, 0, &psz_only_rectype, NULL)  );

        if (psz_rectype)
        {
            SG_ARGCHECK(0 == strcmp(psz_rectype, psz_only_rectype), psz_rectype);
        }
        else
        {
            psz_rectype = psz_only_rectype;
        }
    }
    else
    {
        SG_NULLARGCHECK(psz_rectype);
    }

    SG_ERR_CHECK(  sg_dbndx__calc_column_list(pCtx, pndx, pvh_schema, pidState, psz_rectype, pva_fields, &pva_columns, &pvh_joins, &b_history, &b_multirow_joins)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "hidrec")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_hidrec)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_in)  );
    SG_ERR_CHECK(  sg_dbndx__calc_query(pCtx, pndx, pvh_schema, pidState, psz_rectype, pva_crit, NULL, 0, 0, pva_columns, pvh_joins, b_history, b_multirow_joins, &b_usernames, &pstr_query, pvh_in, NULL)  );

    //printf("%s\n", SG_string__sz(pstr_query));

    if (b_history || b_usernames)
    {
        SG_ERR_CHECK(  sg_dbndx__attach_usernames(pCtx, pndx)  );
    }
    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_dbndx__create_temp_tables(pCtx, pndx->psql, pvh_in)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr_query))  );

    if ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_cur)  );
        SG_ERR_CHECK(  sg_dbndx__read_one_possibly_multirow_record(pCtx, pStmt, pva_columns, b_history, &rc, pvh_cur)  );
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppvh = pvh_cur;
    pvh_cur = NULL;

fail:
    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }

    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    pndx->bInTransaction = SG_FALSE;
    if (b_history || b_usernames)
    {
        SG_ERR_IGNORE(  sg_dbndx__detach_usernames(pCtx, pndx)  );
    }

    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    SG_VHASH_NULLFREE(pCtx, pvh_in);
    SG_VHASH_NULLFREE(pCtx, pvh_cur);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
    SG_VHASH_NULLFREE(pCtx, pvh_joins);
}

static void SG_dbndx_query__query__one__recid(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_field_value,
    SG_varray* pva_fields,
    SG_vhash** ppvh
	)
{
    SG_string* pstr_query = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = 0;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_joins = NULL;
    SG_vhash* pvh_cur = NULL;
    SG_bool b_history = SG_FALSE;
    SG_bool b_multirow_joins = SG_FALSE;
    SG_varray* pva_crit = NULL;
    SG_vhash* pvh_in = NULL;
    SG_vhash* pvh_schema = NULL;
    SG_bool b_usernames = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_fields);
	SG_NULLARGCHECK_RETURN(ppvh);
	SG_NULLARGCHECK_RETURN(pidState);

    if (!SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(pndx->iDagNum))
    {
        SG_ERR_THROW(  SG_ERR_ZING_NO_RECID  ); // TODO better error?
    }

    SG_ERR_CHECK(  sg_dbndx__update_cur_state(pCtx, pndx, pidState, &pvh_schema)  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        const char* psz_only_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_schema, 0, &psz_only_rectype, NULL)  );

        if (psz_rectype)
        {
            SG_ARGCHECK(0 == strcmp(psz_rectype, psz_only_rectype), psz_rectype);
        }
        else
        {
            psz_rectype = psz_only_rectype;
        }
    }
    else
    {
        SG_NULLARGCHECK(psz_rectype);
    }

    if (0 != strcmp(psz_field_name, SG_ZING_FIELD__RECID))
    {
        SG_vhash* pvh_rectype = NULL;
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_rectype, psz_field_name, &b_has)  );
        if (!b_has)
        {
			SG_ERR_THROW2(  SG_ERR_ZING_FIELD_NOT_FOUND, (pCtx, "%s.%s", psz_rectype, psz_field_name)  );
        }

        // TODO require that this field has a unique constraint
    }

    SG_ERR_CHECK(  sg_dbndx__calc_column_list(pCtx, pndx, pvh_schema, pidState, psz_rectype, pva_fields, &pva_columns, &pvh_joins, &b_history, &b_multirow_joins)  );

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_field_name)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_field_value)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_in)  );
    SG_ERR_CHECK(  sg_dbndx__calc_query(pCtx, pndx, pvh_schema, pidState, psz_rectype, pva_crit, NULL, 0, 0, pva_columns, pvh_joins, b_history, b_multirow_joins, &b_usernames, &pstr_query, pvh_in, NULL)  );

    //fprintf(stderr, "%s\n", SG_string__sz(pstr_query));

    if (b_history || b_usernames)
    {
        SG_ERR_CHECK(  sg_dbndx__attach_usernames(pCtx, pndx)  );
    }
    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_dbndx__create_temp_tables(pCtx, pndx->psql, pvh_in)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr_query))  );

    if ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_cur)  );
        SG_ERR_CHECK(  sg_dbndx__read_one_possibly_multirow_record(pCtx, pStmt, pva_columns, b_history, &rc, pvh_cur)  );
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppvh = pvh_cur;
    pvh_cur = NULL;

fail:
    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }

    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    pndx->bInTransaction = SG_FALSE;
    if (b_history || b_usernames)
    {
        SG_ERR_IGNORE(  sg_dbndx__detach_usernames(pCtx, pndx)  );
    }

    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VARRAY_NULLFREE(pCtx, pva_crit);
    SG_VHASH_NULLFREE(pCtx, pvh_in);
    SG_VHASH_NULLFREE(pCtx, pvh_cur);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
    SG_VHASH_NULLFREE(pCtx, pvh_joins);
}

void SG_dbndx_query__query__one(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_field_value,
    SG_varray* pva_fields,
    SG_vhash** ppvh
	)
{
    if (SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(pndx->iDagNum))
    {
        SG_ERR_CHECK_RETURN(  SG_dbndx_query__query__one__recid(pCtx, pndx, pidState, psz_rectype, psz_field_name, psz_field_value, pva_fields, ppvh)  );
    }
    else
    {
        // TODO make sure psz_field_name is "hidrec"?
        SG_ERR_CHECK_RETURN(  SG_dbndx_query__query__one__hidrec(pCtx, pndx, pidState, psz_rectype, psz_field_value, pva_fields, ppvh)  );
    }
}

void SG_dbndx_query__fts(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_keywords,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	)
{
    SG_string* pstr_query = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_joins = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_bool b_history = SG_FALSE;
    SG_bool b_multirow_joins = SG_FALSE;
    SG_vhash* pvh_schema = NULL;
    SG_bool b_usernames = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_fields);
	SG_NULLARGCHECK_RETURN(ppva);
	SG_NULLARGCHECK_RETURN(pidState);

    SG_ERR_CHECK(  sg_dbndx__update_cur_state(pCtx, pndx, pidState, &pvh_schema)  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        const char* psz_only_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_schema, 0, &psz_only_rectype, NULL)  );

        if (psz_rectype)
        {
            SG_ARGCHECK(0 == strcmp(psz_rectype, psz_only_rectype), psz_rectype);
        }
        else
        {
            psz_rectype = psz_only_rectype;
        }
    }
    else
    {
        SG_NULLARGCHECK(psz_rectype);
    }

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    SG_ERR_CHECK(  sg_dbndx__calc_column_list(pCtx, pndx, pvh_schema, pidState, psz_rectype, pva_fields, &pva_columns, &pvh_joins, &b_history, &b_multirow_joins)  );

    SG_ERR_CHECK(  sg_dbndx__calc_query__fts(pCtx, pndx, pvh_schema, pidState, psz_rectype, psz_field_name, psz_keywords, iNumRecordsToReturn, iNumRecordsToSkip, pva_columns, pvh_joins, b_history, b_multirow_joins, &b_usernames, &pstr_query)  );

    SG_ERR_CHECK(  sg_dbndx__do_query__possibly_multirow(pCtx, pndx, pstr_query, b_history, b_multirow_joins, b_usernames, iNumRecordsToReturn, iNumRecordsToSkip, pva_columns, NULL, &pva)  );

    SG_ASSERT(pva);
    //SG_VARRAY_STDOUT(pva);

    *ppva = pva;
    pva = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
    SG_VHASH_NULLFREE(pCtx, pvh_joins);
}

void SG_dbndx_query__query__recent(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    const char* psz_rectype,
	const SG_varray* pcrit,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	)
{
    SG_string* pstr_query = NULL;
    SG_varray* pva = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = 0;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_vhash* pvh_in = NULL;
    SG_vhash* pvh_schema = NULL;
    SG_vhash* pvh_columns_used = NULL;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_fields);
	SG_NULLARGCHECK_RETURN(ppva);

    SG_ERR_CHECK(  sg_dbndx__load_schema_from_sql(pCtx, pndx->psql, "composite_schema", &pvh_schema)  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        const char* psz_only_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_schema, 0, &psz_only_rectype, NULL)  );

        if (psz_rectype)
        {
            SG_ARGCHECK_RETURN(0 == strcmp(psz_rectype, psz_only_rectype), psz_rectype);
        }
        else
        {
            psz_rectype = psz_only_rectype;
        }
    }
    else
    {
        SG_NULLARGCHECK_RETURN(psz_rectype);
    }

    if (pcrit)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_columns_used)  );
    }

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    // note that pidState is NULL in the line below
    SG_ERR_CHECK(  sg_dbndx__calc_column_list(pCtx, pndx, pvh_schema, NULL, psz_rectype, pva_fields, &pva_columns, NULL, NULL, NULL)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_in)  );
    SG_ERR_CHECK(  sg_dbndx__calc_query__recent(pCtx, pndx, pvh_schema, psz_rectype, pcrit, iNumRecordsToReturn, iNumRecordsToSkip, pva_columns, &pstr_query, pvh_in, pvh_columns_used)  );

    //fprintf(stderr, "%s\n", SG_string__sz(pstr_query));

    if (pvh_columns_used)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_columns_used, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_field = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_columns_used, i, &psz_field, NULL)  );

            SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "CREATE INDEX IF NOT EXISTS \"index$%s_%s\" ON \"%s_%s\" (\"%s\")",
                        psz_rectype,
                        psz_field,
                        SG_DBNDX_RECORD_TABLE_PREFIX,
                        psz_rectype,
                        psz_field
                        )  );
        }
    }

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

    SG_ERR_CHECK(  sg_dbndx__attach_usernames(pCtx, pndx)  );
    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_dbndx__create_temp_tables(pCtx, pndx->psql, pvh_in)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr_query))  );

    while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        SG_vhash* pvh_rec = NULL;

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh_rec)  );

        // __recent does not result in multirow records

        SG_ERR_CHECK(  sg_dbndx__read_fields(pCtx, pStmt, pva_columns, pvh_rec)  );
    }

    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    SG_ASSERT(pva);
    //SG_VARRAY_STDOUT(pva);

    *ppva = pva;
    pva = NULL;

fail:
    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }

    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    pndx->bInTransaction = SG_FALSE;
    SG_ERR_IGNORE(  sg_dbndx__detach_usernames(pCtx, pndx)  );

    SG_VHASH_NULLFREE(pCtx, pvh_columns_used);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VHASH_NULLFREE(pCtx, pvh_in);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
}

static void sg_dbndx__do_query__possibly_multirow(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_string* pstr_query,
    SG_bool b_history,
    SG_bool b_multirow_joins,
    SG_bool b_usernames,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_columns,
    SG_vhash* pvh_in,
    SG_varray** ppva
    )
{
    SG_varray* pva = NULL;
    SG_bool b_multirow = SG_FALSE;
    sqlite3_stmt* pStmt = NULL;
    int rc = 0;

    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

    SG_ERR_CHECK(  sg_dbndx__create_temp_tables(pCtx, pndx->psql, pvh_in)  );
    if (b_history || b_usernames)
    {
        SG_ERR_CHECK(  sg_dbndx__attach_usernames(pCtx, pndx)  );
    }
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr_query))  );

    if (b_history || b_multirow_joins)
    {
        b_multirow = SG_TRUE;
    }

try_again:
    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_TRUE;

    // tell sqlite to step to the first row
    rc = sqlite3_step(pStmt);
    if (SQLITE_BUSY == rc)
    {
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pndx->psql, "ROLLBACK TRANSACTION")  );
        pndx->bInTransaction = SG_FALSE;
        SG_sleep_ms(50);
        goto try_again;
    }

    // either there were no rows (SQLITE_DONE) or we got a row (SQLITE_ROW)
    // or there was an error.
    if (SQLITE_DONE == rc)
    {
        // no rows
        goto no_rows;
    }
    if (SQLITE_ROW != rc)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }

    //SG_VARRAY_STDOUT(pva_columns);

    // multirow records with limit/skip have to be handled manually
    if (b_multirow && (iNumRecordsToReturn || iNumRecordsToSkip))
    {
        SG_uint32 count_added = 0;

        if (iNumRecordsToSkip)
        {
            SG_uint32 i_skip = 0;

            for (i_skip=0; i_skip<iNumRecordsToSkip; i_skip++)
            {
                SG_ERR_CHECK(  sg_dbndx__skip_one_possibly_multirow_record(pCtx, pStmt, &rc)  );
                if (SQLITE_ROW != rc)
                {
                    break;
                }
            }
        }

        while (SQLITE_ROW == rc)
        {
            SG_vhash* pvh_rec = NULL;

            SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh_rec)  );
            SG_ERR_CHECK(  sg_dbndx__read_one_possibly_multirow_record(pCtx, pStmt, pva_columns, b_history, &rc, pvh_rec)  );

            count_added++;
            if (iNumRecordsToReturn)
            {
                if (count_added >= iNumRecordsToReturn)
                {
                    rc = SQLITE_DONE;
                    break;
                }
            }

            if (SQLITE_DONE == rc)
            {
                break;
            }
        }
    }
    else if (b_multirow)
    {
        while (SQLITE_ROW == rc)
        {
            SG_vhash* pvh_rec = NULL;

            SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh_rec)  );
            SG_ERR_CHECK(  sg_dbndx__read_one_possibly_multirow_record(pCtx, pStmt, pva_columns, b_history, &rc, pvh_rec)  );
        }
    }
    else
    {
        while (SQLITE_ROW == rc)
        {
            SG_vhash* pvh_rec = NULL;

            SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh_rec)  );
            //SG_ERR_CHECK(  x_row_stderr(pCtx, pStmt, pva_columns, "row:")  );
            SG_ERR_CHECK(  sg_dbndx__read_fields(pCtx, pStmt, pva_columns, pvh_rec)  );
            //SG_VHASH_STDERR(pvh_rec);
            rc = sqlite3_step(pStmt);
        }
    }

    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }

no_rows:
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }

    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    pndx->bInTransaction = SG_FALSE;
    if (b_history || b_usernames)
    {
        SG_ERR_IGNORE(  sg_dbndx__detach_usernames(pCtx, pndx)  );
    }
}

void SG_dbndx_query__prep(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState
	)
{
    SG_vhash* pvh_schema = NULL;

    SG_ERR_CHECK(  sg_dbndx__update_cur_state(pCtx, pndx, pidState, &pvh_schema)  );

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
}

void SG_dbndx_query__count(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
    SG_uint32* pi_result
	)
{
    SG_string* pstr_query = NULL;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_vhash* pvh_in = NULL;
    SG_int64 count = 0;
    SG_vhash* pvh_schema = NULL;
    SG_bool b_usernames = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pi_result);
	SG_NULLARGCHECK_RETURN(pidState);

    SG_ERR_CHECK(  sg_dbndx__update_cur_state(pCtx, pndx, pidState, &pvh_schema)  );

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        const char* psz_only_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_schema, 0, &psz_only_rectype, NULL)  );

        if (psz_rectype)
        {
            SG_ARGCHECK(0 == strcmp(psz_rectype, psz_only_rectype), psz_rectype);
        }
        else
        {
            psz_rectype = psz_only_rectype;
        }
    }
    else
    {
        SG_NULLARGCHECK(psz_rectype);
    }

    // the rectype might not exist because it might be newly created,
    // so zing is doing rechecks, but the dbndx hasn't been rebuilt
    // yet, so its composite schema doesn't have the new rectype yet.

    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    if (pvh_rectype_fields)
    {
        SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva_columns)  );
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                    MY_COLUMN__DEST,   "result",
                    MY_COLUMN__EXPR,    "count(*)",
                    NULL
                    )  );

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_in)  );
        SG_ERR_CHECK(  sg_dbndx__calc_query(pCtx, pndx, pvh_schema, pidState, psz_rectype, pcrit, NULL, 0, 0, pva_columns, NULL, SG_FALSE, SG_FALSE, &b_usernames, &pstr_query, pvh_in, NULL)  );

        SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
        pndx->bInTransaction = SG_TRUE;

        SG_ERR_CHECK(  sg_dbndx__create_temp_tables(pCtx, pndx->psql, pvh_in)  );

        SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pndx->psql, &count, "%s", SG_string__sz(pstr_query))  );

        SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
        pndx->bInTransaction = SG_FALSE;
    }

    *pi_result = (SG_uint32) count;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
    SG_VHASH_NULLFREE(pCtx, pvh_in);
}

void SG_dbndx_query__query(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
	const SG_varray* pSort,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	)
{
    SG_string* pstr_query = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_joins = NULL;
    SG_vhash* pvh_rectype_fields = NULL;
    SG_bool b_history = SG_FALSE;
    SG_bool b_multirow_joins = SG_FALSE;
    SG_vhash* pvh_in = NULL;
    SG_vhash* pvh_schema = NULL;
    SG_vhash* pvh_columns_used = NULL;
    SG_bool b_usernames = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pndx);
	SG_NULLARGCHECK_RETURN(pva_fields);
	SG_NULLARGCHECK_RETURN(ppva);

    // we allow iNumRecordsToReturn even if pSort is NULL.  There is not
    // much reason to care what gets returned in this case, but it IS
    // reasonable to set iNumRecordsToReturn to 1 when all you want to do
    // is check to see if the query returns anything at all.

    if (pidState)
    {
        SG_ERR_CHECK(  sg_dbndx__update_cur_state(pCtx, pndx, pidState, &pvh_schema)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_dbndx__load_schema_from_sql(pCtx, pndx->psql, "composite_schema", &pvh_schema)  );
    }

    if (SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
    {
        const char* psz_only_rectype = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_schema, 0, &psz_only_rectype, NULL)  );

        if (psz_rectype)
        {
            SG_ARGCHECK(0 == strcmp(psz_rectype, psz_only_rectype), psz_rectype);
        }
        else
        {
            psz_rectype = psz_only_rectype;
        }
    }
    else
    {
        SG_NULLARGCHECK(psz_rectype);
    }

    if (pcrit && !pidState)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_columns_used)  );
    }

    // the rectype might not exist because it might be newly created,
    // so zing is doing rechecks, but the dbndx hasn't been rebuilt
    // yet, so its composite schema doesn't have the new rectype yet.

    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_schema, psz_rectype, &pvh_rectype_fields)  );
    if (pvh_rectype_fields)
    {
        SG_ERR_CHECK(  sg_dbndx__calc_column_list(pCtx, pndx, pvh_schema, pidState, psz_rectype, pva_fields, &pva_columns, &pvh_joins, &b_history, &b_multirow_joins)  );

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_in)  );
        SG_ERR_CHECK(  sg_dbndx__calc_query(pCtx, pndx, pvh_schema, pidState, psz_rectype, pcrit, pSort, iNumRecordsToReturn, iNumRecordsToSkip, pva_columns, pvh_joins, b_history, b_multirow_joins, &b_usernames, &pstr_query, pvh_in, pvh_columns_used)  );

        //fprintf(stderr, "%s\n", SG_string__sz(pstr_query));

        if (pvh_columns_used)
        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_columns_used, &count)  );
            for (i=0; i<count; i++)
            {
                const char* psz_field = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_columns_used, i, &psz_field, NULL)  );

                SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "CREATE INDEX IF NOT EXISTS \"index$%s_%s\" ON \"%s_%s\" (\"%s\")",
                            psz_rectype,
                            psz_field,
                            SG_DBNDX_RECORD_TABLE_PREFIX,
                            psz_rectype,
                            psz_field
                            )  );
            }
        }

        SG_ERR_CHECK(  sg_dbndx__do_query__possibly_multirow(pCtx, pndx, pstr_query, b_history, b_multirow_joins, b_usernames, iNumRecordsToReturn, iNumRecordsToSkip, pva_columns, pvh_in, &pva)  );

        SG_ASSERT(pva);
        //SG_VARRAY_STDOUT(pva);
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_columns_used);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
    SG_VHASH_NULLFREE(pCtx, pvh_joins);
    SG_VHASH_NULLFREE(pCtx, pvh_in);
}

static void get_gen(
    SG_context* pCtx,
    SG_dbndx_query* pndx,
    const char* psz_csid,
    SG_int32* pi
    )
{
    SG_dagnode* pdn = NULL;
    SG_int32 v = -1;

    SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pndx->pRepo, pndx->iDagNum, psz_csid, &pdn)  );
    SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &v)  );

    *pi = v;

fail:
    SG_DAGNODE_NULLFREE(pCtx, pdn);
}

static void sg_do_delta(
	SG_context* pCtx,
    SG_vhash* pvh_path_members,
    SG_uint32 flags,
    sqlite3_stmt* pStmt,
    SG_int32 val,
    SG_vhash* pvh
    )
{
    int rc = -1;

    while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char* psz_result = NULL;
        SG_int_to_string_buffer buf;

        const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 1);
        const char* psz_csid_parent = (const char*) sqlite3_column_text(pStmt, 2);
        SG_bool b_has = SG_FALSE;

        if (flags & SG_REPO__MAKE_DELTA_FLAG__ROWIDS)
        {
            SG_int64 hidrecrow = sqlite3_column_int64(pStmt, 0);
            psz_result = SG_int64_to_sz(hidrecrow, buf);
        }
        else
        {
            psz_result = (const char*) sqlite3_column_text(pStmt, 0);
        }

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_path_members, psz_csid, &b_has)  );
        if (b_has)
        {
            b_has = SG_FALSE;
            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_path_members, psz_csid_parent, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__addtoval__int64(pCtx, pvh, psz_result, val)  );
            }
        }
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }

fail:
    ;
}

static void x_get_delta_counts(
	SG_context* pCtx,
	SG_dbndx_query* pndx,
    SG_vhash* pvh_path_members,
    SG_uint32 flags,
    SG_int32 gen_min,
    SG_int32 gen_max,
    SG_vhash** ppvh
	)
{
    sqlite3_stmt* pStmt = NULL;
    SG_vhash* pvh = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );

    if (flags & SG_REPO__MAKE_DELTA_FLAG__ROWIDS)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT d.hidrecrow,c1.csid,c2.csid FROM db_delta_adds d INNER JOIN csids c1 ON (d.csidrow=c1.csidrow) INNER JOIN csids c2 ON (d.parentrow=c2.csidrow) WHERE (c1.generation >= %d) AND (c1.generation <= %d)", gen_min, gen_max)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT h.hidrec,c1.csid,c2.csid FROM db_delta_adds d INNER JOIN hidrecs h ON (d.hidrecrow=h.hidrecrow) INNER JOIN csids c1 ON (d.csidrow=c1.csidrow) INNER JOIN csids c2 ON (d.parentrow=c2.csidrow) WHERE (c1.generation >= %d) AND (c1.generation <= %d)", gen_min, gen_max)  );
    }
    SG_ERR_CHECK(  sg_do_delta(pCtx, pvh_path_members, flags, pStmt, 1, pvh)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    if (flags & SG_REPO__MAKE_DELTA_FLAG__ROWIDS)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT d.hidrecrow,c1.csid,c2.csid FROM db_delta_dels d INNER JOIN csids c1 ON (d.csidrow=c1.csidrow) INNER JOIN csids c2 ON (d.parentrow=c2.csidrow) WHERE (c1.generation >= %d) AND (c1.generation <= %d)", gen_min, gen_max)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT h.hidrec,c1.csid,c2.csid FROM db_delta_dels d INNER JOIN hidrecs h ON (d.hidrecrow=h.hidrecrow) INNER JOIN csids c1 ON (d.csidrow=c1.csidrow) INNER JOIN csids c2 ON (d.parentrow=c2.csidrow) WHERE (c1.generation >= %d) AND (c1.generation <= %d)", gen_min, gen_max)  );
    }
    SG_ERR_CHECK(  sg_do_delta(pCtx, pvh_path_members, flags, pStmt, -1, pvh)  );
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppvh = pvh;
    pvh = NULL;

fail:
    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_dbndx_query__make_delta_from_path(
    SG_context* pCtx,
    SG_dbndx_query* pndx,
    SG_varray* pva_path,
    SG_uint32 flags,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
    )
{
    SG_uint32 count = 0;
    SG_vhash* pvh_counts = NULL;
    const char* psz_csid_max = NULL;
    const char* psz_csid_min = NULL;
    SG_int32 gen_max = -1;
    SG_int32 gen_min = -1;
    SG_uint32 i = 0;
    SG_vhash* pvh_path_members = NULL;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_path, &count)  );
    SG_ASSERT(count >= 2);

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_path_members)  );
    for (i=0; i<(count); i++)
    {
        const char* psz_csid_cur = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_path, i, &psz_csid_cur)  );

        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_path_members, psz_csid_cur)  );
    }

    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_path, 0, &psz_csid_max)  );
    SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_path, count-1, &psz_csid_min)  );
    SG_ERR_CHECK(  get_gen(pCtx, pndx, psz_csid_max, &gen_max)  );
    if (psz_csid_min[0])
    {
        SG_ERR_CHECK(  get_gen(pCtx, pndx, psz_csid_min, &gen_min)  );
    }
    else
    {
        gen_min = 0;
    }

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_TRUE;
    SG_ERR_CHECK(  x_get_delta_counts(pCtx, pndx, pvh_path_members, flags, gen_min, gen_max, &pvh_counts)  );
    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_FALSE;
#if 0
    fprintf(stderr, "counts:\n");
    SG_VHASH_STDERR(pvh_counts);
#endif
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_counts, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_hidrec = NULL;
        const SG_variant* pv = NULL;
        SG_int64 c = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_counts, i, &psz_hidrec, &pv)  );
        SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pv, &c)  );

        if (
                (c < -1)
                || (c > 1)
           )
        {
            char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

            SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, pndx->iDagNum, buf_dagnum, sizeof(buf_dagnum))  );

            SG_ERR_THROW2(  SG_ERR_DB_DELTA, (pCtx, "db delta problem.  dagnum=%s  hidrec=%s  csid_min=%s  csid_max=%s  c=%d", buf_dagnum, psz_hidrec, psz_csid_min, psz_csid_max, (int) c)   );
        }

#if 0
        SG_ASSERT(
                (-1 == c)
                ||
                (0 == c)
                ||
                (1 == c)
                );
#endif

        if (c < 0)
        {
            SG_bool b = SG_FALSE;
            SG_ERR_CHECK(  SG_vhash__remove_if_present(pCtx, pvh_remove, psz_hidrec, &b)  );
            if (!b)
            {
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_add, psz_hidrec)  );
            }
        }
        else if (c > 0)
        {
            SG_bool b = SG_FALSE;
            SG_ERR_CHECK(  SG_vhash__remove_if_present(pCtx, pvh_add, psz_hidrec, &b)  );
            if (!b)
            {
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_remove, psz_hidrec)  );
            }
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_path_members);
    SG_VHASH_NULLFREE(pCtx, pvh_counts);
}

static void sg_dbndx__read_fields_as_strings(
	SG_context* pCtx,
    sqlite3_stmt* pStmt,
    SG_varray* pva_columns,
    SG_vhash* pvh_cur
    )
{
    SG_uint32 count_columns = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_columns, &count_columns)  );
    for (i=0; i<count_columns; i++)
    {
        SG_vhash* pvh_column = NULL;
        const char* psz_dest = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_columns, i, &pvh_column)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__DEST, &psz_dest)  );

        if (0 == strcmp(psz_dest, MY_COLUMN__DEST__RECORD))
        {
            const char* psz_alias = NULL;
            const char* psz_val = (const char*) sqlite3_column_text(pStmt, i);

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_column, MY_COLUMN__ALIAS, &psz_alias)  );
            if (psz_val)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_cur, psz_alias, psz_val)  );
            }
        }
    }

fail:
    ;
}

void SG_dbndx_query__raw_history(
        SG_context* pCtx,
        SG_dbndx_query* pndx,
        SG_int64 min_timestamp,
        SG_int64 max_timestamp,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh = NULL;
    SG_int_to_string_buffer buf_min;
    SG_int_to_string_buffer buf_max;
    SG_vhash* pvh_records = NULL;
    SG_vhash* pvh_changesets = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_string* pstr_query = NULL;
    SG_varray* pva = NULL;
    SG_varray* pva_columns = NULL;
    SG_vhash* pvh_schema = NULL;
    SG_uint32 count_rectypes = 0;
    SG_uint32 i_rectype = 0;
    SG_string* pstr_fields = NULL;
    SG_string* pstr_table = NULL;
    char buf_c[SG_TID_MAX_BUFFER_LENGTH];
    char buf_d[SG_TID_MAX_BUFFER_LENGTH];
    char buf_h[SG_TID_MAX_BUFFER_LENGTH];

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_c, sizeof(buf_c))  );
    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_d, sizeof(buf_d))  );
    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_h, sizeof(buf_h))  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "CREATE TEMP TABLE %s AS SELECT a.csidrow, a.userid, a.timestamp FROM db_audits a WHERE (a.timestamp >= %s) AND (a.timestamp <= %s) ORDER BY a.timestamp DESC", buf_c, SG_int64_to_sz(min_timestamp, buf_min), SG_int64_to_sz(max_timestamp, buf_max))  );
    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "CREATE INDEX ndx_%s_csidrow ON %s (csidrow)", buf_c, buf_c)  );

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "CREATE TEMP TABLE %s AS SELECT d.hidrecrow, d.csidrow, d.parentrow, 1 AS val FROM db_delta_adds d WHERE d.csidrow IN (SELECT csidrow FROM %s)", buf_d, buf_c)  );
    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "INSERT INTO %s SELECT r.hidrecrow, r.csidrow, r.parentrow, -1 AS val FROM db_delta_dels r WHERE r.csidrow IN (SELECT csidrow FROM %s)", buf_d, buf_c)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "changesets", &pvh_changesets)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT c1.csid, c1.generation, a.userid, a.timestamp FROM %s a, csids c1 WHERE a.csidrow=c1.csidrow", buf_c)  );
    while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
        //SG_int32 gen = (SG_int32) sqlite3_column_int(pStmt, 1);
        const char* psz_userid = (const char*) sqlite3_column_text(pStmt, 2);
        SG_int64 timestamp = (SG_int64) sqlite3_column_int64(pStmt,3);
        SG_vhash* pvh_cs = NULL;
        SG_varray* pva_audits = NULL;
        SG_vhash* pvh_a = NULL;

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changesets, psz_csid, &pvh_cs)  );
        if (!pvh_cs)
        {
            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_changesets, psz_csid, &pvh_cs)  );
            //SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_cs, "gen", (SG_int64) gen)  );
        }
        SG_ERR_CHECK(  SG_vhash__ensure__varray(pCtx, pvh_cs, "a", &pva_audits)  );

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_audits, &pvh_a)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_a, "t", timestamp)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_a, "u", psz_userid)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT c1.csid, c2.csid, h.hidrec, d.val FROM %s d, csids c1, csids c2, hidrecs h WHERE d.csidrow=c1.csidrow AND d.parentrow=c2.csidrow AND d.hidrecrow=h.hidrecrow", buf_d)  );
    while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
        const char* psz_parent = (const char*) sqlite3_column_text(pStmt, 1);
        const char* psz_hidrec = (const char*) sqlite3_column_text(pStmt, 2);
        SG_int32 val = (SG_int32) sqlite3_column_int(pStmt, 3);
        SG_vhash* pvh_cs = NULL;
        SG_vhash* pvh_changes = NULL;
        SG_vhash* pvh_parent = NULL;
        SG_varray* pva_section = NULL;

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_changesets, psz_csid, &pvh_cs)  );
        SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_cs, "d", &pvh_changes)  );
        SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_changes, psz_parent, &pvh_parent)  );

        if (val > 0)
        {
            SG_ERR_CHECK(  SG_vhash__ensure__varray(pCtx, pvh_parent, "+", &pva_section)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_vhash__ensure__varray(pCtx, pvh_parent, "-", &pva_section)  );
        }

        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_section, psz_hidrec)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(SG_ERR_SQLITE(rc));
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "records", &pvh_records)  );
    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pndx->psql, "CREATE TEMP TABLE %s AS SELECT DISTINCT hidrecrow FROM %s", buf_h, buf_d)  );

    SG_ERR_CHECK(  sg_dbndx__load_schema_from_sql(pCtx, pndx->psql, "composite_schema", &pvh_schema)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_schema, &count_rectypes)  );

    for (i_rectype=0; i_rectype<count_rectypes; i_rectype++)
    {
        const char* psz_rectype = NULL;
        SG_vhash* pvh_rectype_fields = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_schema, i_rectype, &psz_rectype, &pvh_rectype_fields)  );

        SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva_columns)  );
        SG_ERR_CHECK(  sg_dbndx__add_column(pCtx, pva_columns,
                    MY_COLUMN__DEST,    MY_COLUMN__DEST__KEY,
                    MY_COLUMN__EXPR,    "hidrecs.hidrec",
                    MY_COLUMN__ALIAS,   SG_ZING_FIELD__HIDREC,
                    MY_COLUMN__TYPE,    SG_ZING_TYPE_NAME__STRING,
                    NULL
                    )  );
        SG_ERR_CHECK(  sg_dbndx__all_fields(pCtx, pvh_schema, NULL, psz_rectype, pva_columns)  );

        SG_ERR_CHECK(  sg_dbndx__format_column_list_in_sql(pCtx, pva_columns, &pstr_fields)  );

        SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_table)  );
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_table, "%s_%s", SG_DBNDX_RECORD_TABLE_PREFIX, psz_rectype)  );

        SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_query, "SELECT ")  );
        SG_ERR_CHECK(  SG_string__append__string(pCtx, pstr_query, pstr_fields)  );
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_query, " FROM ")  );
        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, "\"%s\"", SG_string__sz(pstr_table))  );

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " INNER JOIN \"hidrecs\" ON (\"%s\".hidrecrow = hidrecs.hidrecrow) ", SG_string__sz(pstr_table))  );

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr_query, " WHERE \"%s\".hidrecrow IN (SELECT hidrecrow FROM  %s)", SG_string__sz(pstr_table), buf_h)  );

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "%s", SG_string__sz(pstr_query))  );
        while (SQLITE_ROW == (rc = sqlite3_step(pStmt)))
        {
            SG_vhash* pvh_rec = NULL;
            const char* psz_hidrec = (const char*) sqlite3_column_text(pStmt, 0);

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_records, psz_hidrec, &pvh_rec)  );
            if (!SG_DAGNUM__HAS_SINGLE_RECTYPE(pndx->iDagNum))
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_rec, SG_ZING_FIELD__RECTYPE, psz_rectype)  );
            }

            SG_ERR_CHECK(  sg_dbndx__read_fields_as_strings(pCtx, pStmt, pva_columns, pvh_rec)  );
            //SG_VHASH_STDERR(pvh_rec);
        }

        if (rc != SQLITE_DONE)
        {
            SG_ERR_THROW(SG_ERR_SQLITE(rc));
        }

        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

        SG_STRING_NULLFREE(pCtx, pstr_query);
        SG_VARRAY_NULLFREE(pCtx, pva_columns);
        SG_STRING_NULLFREE(pCtx, pstr_table);
        SG_STRING_NULLFREE(pCtx, pstr_fields);
        SG_STRING_NULLFREE(pCtx, pstr_query);
    }

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__exec__va(pCtx, pndx->psql, "DROP TABLE %s", buf_c)  );
    SG_ERR_IGNORE(  sg_sqlite__exec__va(pCtx, pndx->psql, "DROP TABLE %s", buf_d)  );
    SG_ERR_IGNORE(  sg_sqlite__exec__va(pCtx, pndx->psql, "DROP TABLE %s", buf_h)  );
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pndx->bInTransaction = SG_FALSE;
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_STRING_NULLFREE(pCtx, pstr_query);
    SG_VARRAY_NULLFREE(pCtx, pva_columns);
    SG_STRING_NULLFREE(pCtx, pstr_table);
    SG_STRING_NULLFREE(pCtx, pstr_fields);
    SG_STRING_NULLFREE(pCtx, pstr_query);
}


