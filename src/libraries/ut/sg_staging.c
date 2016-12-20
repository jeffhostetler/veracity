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

#define MY_BUSY_TIMEOUT_MS      (30000)

#define REPO_FILENAME "__REPO__"
#define VERSION_FILENAME "__VER__"
#define CLONE_REPO_INFO_FILENAME "__INFO__"

#define MY_VERSION 1

struct _sg_staging_blob_handle
{
	SG_repo* pRepo;  // we don't own this
	SG_pathname* pFragballPathname;
	SG_uint64 offset;
	SG_blob_encoding encoding;
	SG_uint64 len_encoded;
	SG_uint64 len_full;
};
typedef struct _sg_staging_blob_handle sg_staging_blob_handle;

#define MY_CHUNK_SIZE			(16*1024)

struct _sg_staging
{
	sqlite3* psql;
	SG_pathname* pPath;
	SG_byte buf1[MY_CHUNK_SIZE];
};
typedef struct _sg_staging sg_staging;

static void _nullfree_staging_blob_handle(SG_context* pCtx, sg_staging_blob_handle** ppbh)
{
	if (ppbh)
	{
		sg_staging_blob_handle* pbh = *ppbh;
		if (pbh)
		{
			SG_PATHNAME_NULLFREE(pCtx, pbh->pFragballPathname);
			SG_NULLFREE(pCtx, pbh);
		}
		*ppbh = NULL;
	}
}

void SG_staging__free(SG_context* pCtx, SG_staging* pStaging)
{
	if (pStaging)
	{
		sg_staging* ps = (sg_staging*)pStaging;
		SG_PATHNAME_NULLFREE(pCtx, ps->pPath);
		if (ps->psql)
			SG_ERR_CHECK_RETURN(  sg_sqlite__close(pCtx, ps->psql)  );
		SG_NULLFREE(pCtx, ps);
	}
}

static void _get_counts_status_node(
	SG_context* pCtx,
	SG_vhash* pvh_status,
	SG_vhash** ppvh_counts
	)
{
	SG_vhash* pvhTmp = NULL;
	SG_bool b;
	SG_vhash* pvhRefCounts;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_status, SG_SYNC_STATUS_KEY__COUNTS, &b)  );
	if (!b)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhTmp)  );
		pvhRefCounts = pvhTmp;
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__COUNTS, &pvhTmp)  );
	}
	else
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__COUNTS, &pvhRefCounts)  );

	*ppvh_counts = pvhRefCounts;

	return;
fail:
	SG_VHASH_NULLFREE(pCtx, pvhTmp);
}

static void _add_dagfrag_count_to_status(
	SG_context* pCtx,
	SG_dagfrag* pFrag,
	SG_vhash* pvh_status)
{
	SG_vhash* pvhTmp = NULL;

	SG_uint64 iDagnum;
	char bufDagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_uint32 countDagnodes = 0;
	SG_bool b;

	SG_vhash* pvhRefCounts;
	SG_vhash* pvhRefDags;

	SG_ERR_CHECK(  _get_counts_status_node(pCtx, pvh_status, &pvhRefCounts)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefCounts, SG_SYNC_STATUS_KEY__DAGS, &b)  );
	if (!b)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhTmp)  );
		pvhRefDags = pvhTmp;
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhRefCounts, SG_SYNC_STATUS_KEY__DAGS, &pvhTmp)  );
	}
	else
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefCounts, SG_SYNC_STATUS_KEY__DAGS, &pvhRefDags)  );

	SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &iDagnum)  );
	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagnum, bufDagnum, SG_DAGNUM__BUF_MAX__HEX)  );
	SG_ERR_CHECK(  SG_dagfrag__dagnode_count(pCtx, pFrag, &countDagnodes)  );

	SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvhRefDags, bufDagnum, countDagnodes)  );

	return;
fail:
	SG_VHASH_NULLFREE(pCtx, pvhTmp);
}

static void _add_blob_counts_to_status(
	SG_context* pCtx, 
	SG_vhash* pvh_status, 
	SG_int32 countReferenced, 
	SG_int32 countPresent)
{
	SG_vhash* pvhRefCounts;

	SG_ERR_CHECK_RETURN(  _get_counts_status_node(pCtx, pvh_status, &pvhRefCounts)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__update__int64(pCtx, pvhRefCounts, 
		SG_SYNC_STATUS_KEY__BLOBS_REFERENCED, countReferenced)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__update__int64(pCtx, pvhRefCounts, 
		SG_SYNC_STATUS_KEY__BLOBS_PRESENT, countPresent)  );
}

static void _add_new_nodes_to_status(
	SG_context* pCtx,
	const char* const* aszHids,
	SG_uint32 countHids,
	SG_vhash* pvh_status,
	const char* pszDagnum)
{
	SG_bool b = SG_FALSE;
	SG_uint32 i;
	SG_vhash* pvh_dag_new_nodes_container = NULL;
	SG_vhash* pvh_dag_new_nodes_container_ref = NULL;
	SG_vhash* pvh_dag_new_nodes = NULL;

	if (!countHids)
		return;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &b)  );

	if (b)
	{
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &pvh_dag_new_nodes_container_ref)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_dag_new_nodes_container)  );
		pvh_dag_new_nodes_container_ref = pvh_dag_new_nodes_container;
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__NEW_NODES, &pvh_dag_new_nodes_container)  );
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvh_dag_new_nodes, countHids, NULL, NULL)  );

	for (i = 0; i < countHids; i++)
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_dag_new_nodes, aszHids[i])  );

	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_dag_new_nodes_container_ref, pszDagnum, &pvh_dag_new_nodes)  );

	// fall through
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_dag_new_nodes_container);
	SG_VHASH_NULLFREE(pCtx, pvh_dag_new_nodes);

}

static void _add_disconnected_dag_to_status(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_rbtree* prb_missing,
	SG_vhash* pvh_status,
	SG_uint64 iDagNum,
	const char* pszDagnum)
{
	SG_uint32 count;
	SG_bool b = SG_FALSE;
	SG_rbtree_iterator* pit = NULL;
	SG_vhash* pvh_dag_fringe_container = NULL;
	SG_vhash* pvh_fringe = NULL;
	SG_vhash* pvhTmp = NULL;
	SG_rbtree* prbLeaves = NULL;
	SG_repo_fetch_dagnodes_handle* pdh = NULL;
	SG_dagnode* pdn = NULL;
	
	SG_vhash* pvh_dag_fringe_container_ref = NULL;
	const char* pszRefHid = NULL;
	SG_vhash* pvhRefAllLeaves = NULL;
	SG_vhash* pvhRefDagLeaves = NULL;

	SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_missing, &count)  );
	SG_ASSERT(count);

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_status, SG_SYNC_STATUS_KEY__DAGS, &b)  );

	if (b)
	{
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__DAGS, &pvh_dag_fringe_container_ref)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_dag_fringe_container)  );
        pvh_dag_fringe_container_ref = pvh_dag_fringe_container;
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__DAGS, &pvh_dag_fringe_container)  );
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_fringe)  );

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prb_missing, &b, &pszRefHid, NULL)  );
	while (b)
	{
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_fringe, pszRefHid)  );
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &pszRefHid, NULL)  );
	}
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_dag_fringe_container_ref, pszDagnum, &pvh_fringe)  );

	/* Also add the DAG's leaves and their generation to the status vhash. */

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_status, SG_SYNC_STATUS_KEY__LEAVES, &b)  );
	if (b)
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__LEAVES, &pvhRefAllLeaves)  );
	else
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhTmp)  );
		pvhRefAllLeaves = pvhTmp;
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__LEAVES, &pvhTmp)  );
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhTmp)  );
	pvhRefDagLeaves = pvhTmp;
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhRefAllLeaves, pszDagnum, &pvhTmp)  );

	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, iDagNum, &pdh)  );
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, iDagNum, &prbLeaves)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbLeaves, &b, &pszRefHid, NULL)  );
	while (b)
	{
		SG_int32 gen;
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pRepo, pdh, pszRefHid, &pdn)  );
		SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhRefDagLeaves, pszRefHid, gen)  );
		SG_DAGNODE_NULLFREE(pCtx, pdn);
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &pszRefHid, NULL)  );
	}

	// fall through
fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_VHASH_NULLFREE(pCtx, pvh_dag_fringe_container);
    SG_VHASH_NULLFREE(pCtx, pvh_fringe);
	SG_VHASH_NULLFREE(pCtx, pvhTmp);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pdh)  );
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}

static void _get_blob_from_staging(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	const char* pszHid,
	sg_staging_blob_handle** ppBlobHandleReturned)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;
	sg_staging_blob_handle* pBlobHandleReturned = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
		"SELECT filename, offset, encoding, len_encoded, len_full FROM blobs_present WHERE hid = ? LIMIT 1")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt,1,pszHid)  );
	rc = sqlite3_step(pStmt);

	if (rc == SQLITE_ROW)
	{
		if (ppBlobHandleReturned)
		{
			SG_alloc1(pCtx, pBlobHandleReturned);

			pBlobHandleReturned->pRepo = pRepo;
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pBlobHandleReturned->pFragballPathname,
				pMe->pPath, (const char*)sqlite3_column_text(pStmt, 0))  );
			pBlobHandleReturned->offset = sqlite3_column_int64(pStmt, 1);
			pBlobHandleReturned->encoding = (SG_blob_encoding)sqlite3_column_int(pStmt, 2);
			pBlobHandleReturned->len_encoded = sqlite3_column_int64(pStmt, 3);
			pBlobHandleReturned->len_full = sqlite3_column_int64(pStmt, 4);

			*ppBlobHandleReturned = pBlobHandleReturned;
		}
	}
	else if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}
	else
	{
		// blob doesn't exist
		SG_ERR_THROW(SG_ERR_BLOB_NOT_FOUND);
	}

	SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );

	return;

fail:
	_nullfree_staging_blob_handle(pCtx, &pBlobHandleReturned);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx,pStmt)  );
}

static void _add_missing_blobs_to_status(
	SG_context* pCtx,
	SG_vhash* pvh_status,
	SG_stringarray* psaMissingBlobHids)
{
	SG_bool b = SG_FALSE;
	SG_vhash* pvh_container = NULL;
	SG_vhash* pvh_container_ref = NULL;
	SG_uint32 i, count;

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaMissingBlobHids, &count)  );
	if (count)
	{
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_status, SG_SYNC_STATUS_KEY__BLOBS, &b)  );
		if (b)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__BLOBS, &pvh_container_ref)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_container)  );
			pvh_container_ref = pvh_container;
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_status, SG_SYNC_STATUS_KEY__BLOBS, &pvh_container)  );
		}

		for (i = 0; i < count; i++)
		{
			const char* pszHid;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaMissingBlobHids, i, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_container_ref, pszHid)  );
		}
	}

    return;
fail:
    SG_VHASH_NULLFREE(pCtx, pvh_container);
}

static void _add_referenced_data_blobs_from_tree_changes(
        SG_context * pCtx,
        sg_staging* pMe,
        const char* psz_root,
        const SG_vhash* pvh_changes
        )
{
    SG_uint32 count_parents = 0;
    SG_uint32 i_parent = 0;

    sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx,pMe->psql,&pStmt,
		(
		"INSERT OR IGNORE INTO blobs_referenced "
		"(hid, is_changeset, data_blobs_known) "
		"VALUES (?, ?, 0)"
		) )  );

    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,psz_root)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,pStmt,2,SG_FALSE)  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmt,SQLITE_DONE)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
    for (i_parent=0; i_parent<count_parents; i_parent++)
    {
        const char* psz_csid_parent = NULL;
        SG_vhash* pvh_one_parent_changes = NULL;
        SG_uint32 count_gids = 0;
        SG_uint32 i_gid = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_one_parent_changes)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_one_parent_changes, &count_gids)  );
        for (i_gid=0; i_gid<count_gids; i_gid++)
        {
            const char* psz_gid = NULL;
            const char* psz_hid = NULL;
            const SG_variant* pv = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_one_parent_changes, i_gid, &psz_gid, &pv)  );
            if (SG_VARIANT_TYPE_VHASH == pv->type)
            {
                SG_vhash* pvh_info = NULL;

                SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_info)  );
                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__HID, &psz_hid)  );
                if (psz_hid)
                {
                    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
                    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
                    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,psz_hid)  );
                    SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,pStmt,2,SG_FALSE)  );
                    SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmt,SQLITE_DONE)  );
                }
            }
            else 
            {
                // the item was deleted
                SG_ASSERT(SG_VARIANT_TYPE_NULL == pv->type);
            }
        }
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void _add_referenced_data_blobs_from_vhash(
        SG_context * pCtx,
        sqlite3_stmt* pStmt,
        const SG_vhash* pvh_add
        )
{
    SG_uint32 count_adds = 0;
    SG_uint32 i_add = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count_adds)  );
    for (i_add=0; i_add<count_adds; i_add++)
    {
        const char* psz_hid = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_add, i_add, &psz_hid, NULL)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,psz_hid)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,pStmt,2,SG_FALSE)  );
        SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmt,SQLITE_DONE)  );
    }

fail:
    ;
}

static void _add_one_referenced_blob(
        SG_context * pCtx,
        sg_staging* pMe,
        const char* psz_hid
        )
{
    sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx,pMe->psql,&pStmt,
		(
		"INSERT OR IGNORE INTO blobs_referenced "
		"(hid, is_changeset, data_blobs_known) "
		"VALUES (?, ?, 0)"
		) )  );
    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,psz_hid)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,pStmt,2,SG_FALSE)  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmt,SQLITE_DONE)  );

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void _add_referenced_data_blobs_from_db_changes(
        SG_context * pCtx,
        sg_staging* pMe,
        const SG_vhash* pvh_changes
        )
{
    SG_uint32 count_parents = 0;
    SG_uint32 i_parent = 0;
    sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx,pMe->psql,&pStmt,
		(
		"INSERT OR IGNORE INTO blobs_referenced "
		"(hid, is_changeset, data_blobs_known) "
		"VALUES (?, ?, 0)"
		) )  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
    for (i_parent=0; i_parent<count_parents; i_parent++)
    {
        const char* psz_csid_parent = NULL;
        SG_vhash* pvh_one_parent_changes = NULL;
        SG_vhash* pvh_add = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, &psz_csid_parent, &pvh_one_parent_changes)  );
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_parent_changes, "add", &pvh_add)  );
        if (pvh_add)
        {
            SG_ERR_CHECK(  _add_referenced_data_blobs_from_vhash(pCtx, pStmt, pvh_add)  );
        }
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_one_parent_changes, "attach_add", &pvh_add)  );
        if (pvh_add)
        {
            SG_ERR_CHECK(  _add_referenced_data_blobs_from_vhash(pCtx, pStmt, pvh_add)  );
        }
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void _add_referenced_blobs__from_changeset(
	SG_context* pCtx,
	sg_staging* pMe,
    SG_changeset* pcs
	)
{
    SG_uint64 dagnum = 0;

	SG_ERR_CHECK(  SG_changeset__get_dagnum(pCtx, pcs, &dagnum)  );
    if (SG_DAGNUM__IS_DB(dagnum))
    {
        SG_vhash* pvh_changes = NULL;
        const char* psz_hid_template = NULL;

        SG_ERR_CHECK(  SG_changeset__db__get_changes(pCtx, pcs, &pvh_changes)  );
        if (pvh_changes)
        {
            SG_ERR_CHECK(  _add_referenced_data_blobs_from_db_changes(pCtx, pMe, pvh_changes)  );
        }

        SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_hid_template)  );
        if (psz_hid_template)
        {
            _add_one_referenced_blob(pCtx, pMe, psz_hid_template);
        }
    }
    else
    {
        SG_vhash* pvh_changes = NULL;
        const char* psz_root = NULL;

        SG_ERR_CHECK(  SG_changeset__tree__get_root(pCtx, pcs, &psz_root)  );
        SG_ERR_CHECK(  SG_changeset__tree__get_changes(pCtx, pcs, &pvh_changes)  );
        if (pvh_changes)
        {
            SG_ERR_CHECK(  _add_referenced_data_blobs_from_tree_changes(pCtx, pMe, psz_root, pvh_changes)  );
        }
    }

fail:
    ;
}

static void _add_referenced_blobs(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_bool bChangesetBlobs,
	const char* const* aszHids,
	SG_uint32 countHids
	)
{
	SG_uint32 i;
	sqlite3_stmt* pStmt = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx,pMe->psql,&pStmt,
		(
		"INSERT OR IGNORE INTO blobs_referenced "
		"(hid, is_changeset, data_blobs_known) "
		"VALUES (?, ?, 0)"
		) )  );

	for (i = 0; i < countHids; i++)
	{
		SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
		SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,aszHids[i])  );
		SG_ASSERT(aszHids[i] && strlen(aszHids[i]));
		SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,pStmt,2,bChangesetBlobs)  );
		SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmt,SQLITE_DONE)  );
	}

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx,pStmt)  );
}

/**
 * Updates pvh_status to reflect blobs in the current push that are
 * not yet present in the staging area or the repo itself.
 */
static void _check_blobs(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	SG_vhash* pvh_status,
	SG_bool bCheckChangesetBlobs,
	SG_bool bCheckDataBlobs,
	SG_bool bGetCounts)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;
	SG_stringarray* psaQueryRepoForBlobs = NULL;
	SG_stringarray* psaBlobsNotInRepo = NULL;
	SG_changeset* pChangeset = NULL;
	sg_staging_blob_handle* pBlobHandle = NULL;

	// If the caller cares about non-changeset blobs, we crack open
	// all the changeset blobs that are present in the staging area and haven't
	// already been explored.  We'll add the data blobs to our blobs_referenced
	// table.
	if (bCheckDataBlobs)
	{
		const char* pszHid;

		// Get a list of all the changesets whose blobs haven't yet been explored.
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
			("SELECT hid FROM blobs_referenced "
			 "WHERE is_changeset = 1 AND data_blobs_known = 0 "
			 " AND hid IN (SELECT hid from blobs_present)"))  );
		while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
		{
			pszHid = (const char*)sqlite3_column_text(pStmt, 0);

			// Read changeset blob from staging to get its list of blobs.
			SG_ERR_CHECK(  _get_blob_from_staging(pCtx, pMe, pRepo, pszHid, &pBlobHandle)  );
			SG_ERR_CHECK(  SG_changeset__load_from_staging(pCtx, pszHid, (SG_staging_blob_handle*)pBlobHandle, &pChangeset)  );
			SG_ERR_CHECK(  _nullfree_staging_blob_handle(pCtx, &pBlobHandle)  );

			// Add the newly discovered non-changeset blobs to the staging area's list of referenced blobs.
			SG_ERR_CHECK(  _add_referenced_blobs__from_changeset(pCtx, pMe, pChangeset)  );

			SG_CHANGESET_NULLFREE(pCtx, pChangeset);
		}
		if (rc != SQLITE_DONE)
			SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
		SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

		// Regardless of the sqlite transaction status, this is safe because
		// staging areas are unique to each invocation of pull_begin/push_begin.
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql,
			("UPDATE blobs_referenced SET data_blobs_known = 1 "
			 "WHERE is_changeset = 1 AND data_blobs_known = 0 "
			 "  AND hid IN (SELECT hid from blobs_present)"))  );

		// At this point the blobs_referenced table is up-to-date given the
		// frags and changeset blobs that are present.
	}

	// Build the list of blobs to ask the repo which ones it needs.
	if (bCheckChangesetBlobs)
	{
		if (bCheckDataBlobs)
		{
			SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
				("SELECT hid FROM blobs_referenced "
				 "WHERE hid NOT IN (SELECT hid FROM blobs_present)"))  );
		}
		else
		{
			SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
				("SELECT hid FROM blobs_referenced "
				 "WHERE is_changeset = 1 "
				 " AND hid NOT IN (SELECT hid FROM blobs_present)"))  );
		}
	}
	else
	{
		if (bCheckDataBlobs)
		{
			SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
				("SELECT hid FROM blobs_referenced "
				"WHERE is_changeset = 0 "
				" AND hid NOT IN (SELECT hid FROM blobs_present)"))  );
		}
	}
	if (pStmt)
	{
		SG_int32 count = 0;

		SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pMe->psql, &count,
			("SELECT (SELECT count(*) FROM blobs_referenced) "
			 " - (SELECT count(*) FROM blobs_present)"))  );

		if (count > 0)
		{
			SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaQueryRepoForBlobs, count)  );

			while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
			{
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaQueryRepoForBlobs,
					(const char*)sqlite3_column_text(pStmt, 0))  );
			}
			if (rc != SQLITE_DONE)
				SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
			SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

			SG_ERR_CHECK(  SG_repo__query_blob_existence(pCtx, pRepo, psaQueryRepoForBlobs, &psaBlobsNotInRepo)  );

			SG_ERR_CHECK(  _add_missing_blobs_to_status(pCtx, pvh_status, psaBlobsNotInRepo)  );
		}
	}

	if (bGetCounts)
	{
		SG_int32 countReferenced = 0;
		SG_int32 countPresent = 0;

		SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pMe->psql, &countReferenced,
			"SELECT count(*) FROM blobs_referenced")  );

		SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pMe->psql, &countPresent,
			"SELECT count(*) FROM blobs_present")  );

		SG_ERR_CHECK(  _add_blob_counts_to_status(pCtx, pvh_status, countReferenced, countPresent)  );
	}

	/* fall through*/
fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_CHANGESET_NULLFREE(pCtx, pChangeset);
	SG_ERR_IGNORE(  _nullfree_staging_blob_handle(pCtx, &pBlobHandle)  );
	SG_STRINGARRAY_NULLFREE(pCtx, psaQueryRepoForBlobs);
	SG_STRINGARRAY_NULLFREE(pCtx, psaBlobsNotInRepo);

}

static void _store_one_frag(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_repo_tx_handle* ptx,
	const char* psz_frag)
{
	SG_vhash* pvh_frag = NULL;
	SG_dagfrag* pFrag = NULL;

	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh_frag, psz_frag)  );
	SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
	SG_VHASH_NULLFREE(pCtx, pvh_frag);

	SG_ERR_CHECK(  SG_repo__store_dagfrag(pCtx, pRepo, ptx, pFrag)  );

	/* fall through */

fail:
	return;
}

static void _check_one_frag(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	SG_bool bCheckConnectedness,
	SG_bool bListNewNodes,
	SG_bool bGetCounts,
	SG_vhash* pvh_status,
	const char* psz_frag)
{
	SG_vhash* pvh_frag = NULL;
	SG_dagfrag* pFrag = NULL;
	SG_uint64 iDagNum = 0;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];				// this just has to be big enough for a "%d"; it is not a GID/HID/TID.
	SG_bool bConnected = SG_FALSE;
	SG_rbtree* prb_missing = NULL;
	SG_stringarray* psa_new_nodes = NULL;

	SG_NULLARGCHECK_RETURN(pvh_status);
	SG_NULLARGCHECK_RETURN(psz_frag);

	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh_frag, psz_frag)  );
	SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
	SG_VHASH_NULLFREE(pCtx, pvh_frag);

	if (bGetCounts)
		SG_ERR_CHECK(  _add_dagfrag_count_to_status(pCtx, pFrag, pvh_status)  );

	SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &iDagNum)  );
	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );

	if (bCheckConnectedness || bListNewNodes)
	{
		SG_ERR_CHECK(  SG_repo__check_dagfrag(pCtx, pRepo, pFrag, &bConnected, &prb_missing, &psa_new_nodes)  );

		// Add the missing parent nodes that prevent the DAG's connecting (the fringe) to the status vhash.
		if (!bConnected)
			SG_ERR_CHECK(  _add_disconnected_dag_to_status(pCtx, pRepo, prb_missing, pvh_status, iDagNum, buf_dagnum)  );
		else
		{
			SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pMe->psql,
				"UPDATE frags SET connected = 1 WHERE iDagnum = '%s'", buf_dagnum)  );
		}

		// Add the blobs that are new to us to the staging area's list of referenced blobs.
		if (psa_new_nodes)
		{
			const char* const* asz = NULL;
			SG_uint32 asz_count = 0;

			SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, psa_new_nodes, &asz, &asz_count)  );
			SG_ERR_CHECK(  _add_referenced_blobs(pCtx, pMe, SG_TRUE, asz, asz_count)  );


			/* If a list of new nodes was requested, add it here. */
			// TODO: when incoming/outgoing do something useful with non-version-control dags, this should change.
			if (bListNewNodes && (SG_DAGNUM__VERSION_CONTROL == iDagNum))
				SG_ERR_CHECK(  _add_new_nodes_to_status(pCtx, asz, asz_count, pvh_status, buf_dagnum)  );
		}
	}

	/* fall through */

fail:
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
	SG_RBTREE_NULLFREE(pCtx, prb_missing);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_new_nodes);
}

static void _store_one_blob(
	SG_context* pCtx,
	sg_staging* pMe,
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
		if ((len_encoded - sofar) > (SG_uint64) sizeof(pMe->buf1))
		{
			want = (SG_uint32) sizeof(pMe->buf1);
		}
		else
		{
			want = (SG_uint32) (len_encoded - sofar);
		}
		SG_ERR_CHECK(  SG_file__read(pCtx, pFile, want, pMe->buf1, &got)  );
		SG_ERR_CHECK(  SG_repo__store_blob__chunk(pCtx, pRepo, pbh, got, pMe->buf1, NULL)  );

		sofar += got;
	}

	SG_ERR_CHECK(  SG_repo__store_blob__end(pCtx, pRepo, ptx, &pbh, NULL)  );

	return;

fail:
	if (pbh)
		SG_ERR_IGNORE(  SG_repo__store_blob__abort(pCtx, pRepo, ptx, &pbh)  );
}

static void _open_db(
	SG_context* pCtx,
	const SG_pathname* pPath_staging_dir,
	sqlite3** ppsql
	)
{
	SG_pathname* pPath = NULL;
	SG_bool bExists = SG_FALSE;
	sqlite3* psql = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPath_staging_dir, "pending.db")  );

	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );

	if (bExists)
	{
		SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
		sqlite3_busy_timeout(psql,MY_BUSY_TIMEOUT_MS);
	}
	else
	{
		SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
		sqlite3_busy_timeout(psql,MY_BUSY_TIMEOUT_MS);

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "BEGIN TRANSACTION")  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, ("CREATE TABLE audits"
		                                        "   ("
		                                        "     csid VARCHAR NOT NULL,"
												"     dagnum INTEGER NOT NULL,"
												"     userid VARCHAR NOT NULL,"
		                                        "     timestamp INTEGER NOT NULL"
		                                        "   )"))  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, ("CREATE TABLE frags"
		                                        "   ("
		                                        "     iDagnum VARCHAR UNIQUE NOT NULL,"
												"     connected INTEGER NOT NULL,"
		                                        "     frag TEXT NOT NULL"
		                                        "   )"))  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, ("CREATE TABLE blobs_present"
		                                        "   ("
		                                        "     filename VARCHAR NOT NULL,"
		                                        "     offset INTEGER NOT NULL,"
		                                        "     encoding INTEGER NOT NULL,"
		                                        "     len_encoded INTEGER NOT NULL,"
		                                        "     len_full INTEGER NOT NULL,"
		                                        "     hid_vcdiff VARCHAR NULL,"
		                                        "     hid VARCHAR UNIQUE NOT NULL"
		                                        "   )"))  );

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, ("CREATE TABLE blobs_referenced"
												"    ("
												"      hid VARCHAR UNIQUE NOT NULL,"
												"      is_changeset INTEGER NOT NULL, "
												"      data_blobs_known INTEGER NOT NULL "
												"    )"))  );

		// We don't need an indexes on hids because the unique constraints imply them.

		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "COMMIT TRANSACTION")  );
	}

	SG_PATHNAME_NULLFREE(pCtx, pPath);

	*ppsql = psql;

	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void _store_all_blobs(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	SG_repo_tx_handle* ptx
	)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;

	SG_file* pFile = NULL;
	char* pszLastFile = NULL;
	SG_pathname* pPath_file = NULL;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql,&pStmt,
		"SELECT hid, filename, offset, encoding, len_encoded, len_full, hid_vcdiff FROM blobs_present ORDER BY filename ASC, offset ASC")  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_hid = (const char*) sqlite3_column_text(pStmt, 0);
		const char* psz_filename = (const char*) sqlite3_column_text(pStmt, 1);
		SG_uint64 offset = sqlite3_column_int64(pStmt,2);
		SG_blob_encoding encoding = (SG_blob_encoding)sqlite3_column_int(pStmt,3);
		SG_uint64 len_encoded = sqlite3_column_int64(pStmt, 4);
		SG_uint64 len_full = sqlite3_column_int64(pStmt, 5);
		const char * psz_hid_vcdiff = (const char *)sqlite3_column_text(pStmt,6);

		if ( (!pFile) || (strcmp(pszLastFile, psz_filename) != 0) )
		{
			if (pFile)
			{
				SG_FILE_NULLCLOSE(pCtx, pFile);
				SG_NULLFREE(pCtx, pszLastFile);
			}

			SG_STRDUP(pCtx, psz_filename, &pszLastFile);
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_file, pMe->pPath, psz_filename)  );
			SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_file, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
			SG_PATHNAME_NULLFREE(pCtx, pPath_file);
		}

		// We have to seek past the object header each time.
		SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, offset)  );

		SG_ERR_CHECK(  _store_one_blob(
			pCtx,
			pMe,
			pRepo,
			pFile,
			ptx,
			psz_hid,
			encoding,
			psz_hid_vcdiff,
			len_full,
			len_encoded
			)  );

	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	/* fall thru */

fail:
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx,&pStmt)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, pszLastFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath_file);
}

static void _check_all_frags(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	SG_bool bCheckConnectedness,
	SG_bool bListNewNodes,
	SG_bool bGetCounts,
	SG_vhash* pvh_status
	)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;

	if (!bListNewNodes && !bGetCounts)
	{
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
			"SELECT frag FROM frags WHERE connected = 0")  );
	}
	else
	{
		// If the caller wants new nodes or node counts, we need to check all
		// dags, not just the disconnected ones.
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
			"SELECT frag FROM frags")  );
	}

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_frag = (const char*) sqlite3_column_text(pStmt, 0);

		SG_ERR_CHECK(  _check_one_frag(
			pCtx,
			pMe,
			pRepo,
			bCheckConnectedness,
			bListNewNodes,
			bGetCounts,
			pvh_status,
			psz_frag
			)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	/* fall thru */

fail:
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx,&pStmt)  );
}

static void _store_all_audits(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	SG_repo_tx_handle* ptx
	)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;


	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql,&pStmt, "SELECT csid,dagnum,userid,timestamp FROM audits")  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
		SG_uint64 dagnum = (SG_uint64) sqlite3_column_int64(pStmt, 1);
		const char* psz_userid = (const char*) sqlite3_column_text(pStmt, 2);
		SG_uint64 timestamp = (SG_uint64) sqlite3_column_int64(pStmt, 3);

        SG_ERR_CHECK(  SG_repo__store_audit(pCtx, pRepo, ptx, psz_csid, dagnum, psz_userid, timestamp)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	/* fall thru */

fail:
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx,&pStmt)  );
}

static void _store_all_frags(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_repo* pRepo,
	SG_repo_tx_handle* ptx
	)
{
	sqlite3_stmt* pStmt = NULL;
	int rc;


	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql,&pStmt, "SELECT frag FROM frags")  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_frag = (const char*) sqlite3_column_text(pStmt, 0);

		SG_ERR_CHECK(  _store_one_frag(
			pCtx,
			pRepo,
			ptx,
			psz_frag
			)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	/* fall thru */

fail:
	if (pStmt)
		SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx,&pStmt)  );
}

static void _save_frag(
	SG_context* pCtx,
	sg_staging* pMe,
	SG_vhash* pvh_frag
	)
{
	SG_uint64 iDagNum;
	sqlite3_stmt* pStmt = NULL;
	SG_vhash* pvh_prev_frag = NULL;
	SG_dagfrag* pFrag = NULL;
	SG_dagfrag* pNewFrag = NULL;
	SG_string* pstr = NULL;
	SG_vhash* pvh_combined_frag = NULL;
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_bool bSkipDag = SG_FALSE;

	//SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_frag, "adding frag to staging")  );

	SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_frag)  );
	SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &iDagNum)  );

	SG_ERR_CHECK(  SG_zing__missing_hardwired_template(pCtx, iDagNum, &bSkipDag)  );
	if (!bSkipDag)
	{
		SG_ERR_CHECK_RETURN(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );

		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx,pMe->psql,&pStmt, "SELECT frag FROM frags WHERE iDagnum = ?")  );
		SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,buf_dagnum)  );

		if (sqlite3_step(pStmt) == SQLITE_ROW)
		{
			const char* psz_frag = (const char*) sqlite3_column_text(pStmt, 0);

			SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh_prev_frag, psz_frag)  );
			SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
		}
		SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

		if (pvh_prev_frag)
		{
			pNewFrag = pFrag;
			SG_ERR_CHECK(  SG_dagfrag__alloc__from_vhash(pCtx, &pFrag, pvh_prev_frag)  );
			SG_VHASH_NULLFREE(pCtx, pvh_prev_frag);

			SG_ERR_CHECK(  SG_dagfrag__eat_other_frag(pCtx, pNewFrag, &pFrag)  );

			SG_ERR_CHECK(  SG_dagfrag__to_vhash__shared(pCtx, pNewFrag, NULL, &pvh_combined_frag)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
			SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh_combined_frag, pstr)  );
			SG_VHASH_NULLFREE(pCtx, pvh_combined_frag);
			SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
				"UPDATE frags SET frag = ? WHERE iDagnum = ?")  );
			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,1,SG_string__sz(pstr))  );
			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt,2,buf_dagnum)  );
			SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr)  );
			SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh_frag, pstr)  );

			SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt,
				"INSERT INTO frags (iDagnum, connected, frag) VALUES (?, 0, ?)")  );
			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, buf_dagnum)  );
			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, SG_string__sz(pstr))  );
			SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
		}
	}

	/* fall through */
fail:
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
	SG_DAGFRAG_NULLFREE(pCtx, pNewFrag);
	SG_STRING_NULLFREE(pCtx, pstr);
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx,&pStmt)  );
}

static void _slurp__version_2(
	SG_context* pCtx,
	sg_staging* pMe,
	const char* psz_filename,
	SG_file* pfb
	)
{
	SG_vhash* pvh = NULL;
	const char* psz_op = NULL;
	SG_uint64 iPos = 0;
    SG_uint32 count_nodes = 0;
    SG_uint32 count_blobs = 0;

	sqlite3_stmt* pStmtSaveBlobInfo = NULL;
	sqlite3_stmt* pStmt_audits = NULL;
	SG_stringarray* psaDeltaRefHids = NULL;
	const SG_uint32 limitDeltaRefHids = 1000;

	SG_NULLARGCHECK_RETURN(psz_filename);
	SG_NULLARGCHECK_RETURN(pfb);

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmtSaveBlobInfo,
		(
		"INSERT OR IGNORE INTO blobs_present "
		"(hid, filename, offset, encoding, len_encoded, len_full, hid_vcdiff) "
		"VALUES (?, ?, ?, ?, ?, ?, ?)"
		) )  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pMe->psql, &pStmt_audits,
		(
		"INSERT INTO audits "
		"(csid, dagnum, userid, timestamp) "
		"VALUES (?, ?, ?, ?)"
		) )  );

	while (1)
	{
		SG_vhash* pvh_frag = NULL;
		SG_uint64 offset_begin_record = 0;

		SG_ERR_CHECK(  SG_file__tell(pCtx, pfb, &offset_begin_record)  );
		SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfb, &pvh)  );
		if (!pvh)
		{
			break;
		}
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &psz_op)  );
		if (0 == strcmp(psz_op, "frag"))
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, "frag", &pvh_frag)  );
            //fprintf(stderr, "\nfrag found in fragball:\n");
            //SG_VHASH_STDERR(pvh_frag);
			SG_ERR_CHECK(  _save_frag(pCtx, pMe, pvh_frag)  );
		}
		else if (0 == strcmp(psz_op, "audits"))
        {
            SG_int64 dagnum = 0;
            SG_varray* pva_audits = NULL;
            SG_uint32 count_audits = 0;
            SG_uint32 i_audit = 0;

            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "dagnum", &dagnum)  );
            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh, "audits", &pva_audits)  );
            SG_ERR_CHECK(  SG_varray__count(pCtx, pva_audits, &count_audits)  );
            for (i_audit=0; i_audit<count_audits; i_audit++)
            {
                SG_vhash* pvh_audit = NULL;
                const char* psz_userid = NULL;
                const char* psz_csid = NULL;
                SG_int64 t = -1;

                SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_audits, i_audit, &pvh_audit)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_audit, "csid", &psz_csid)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_audit, SG_AUDIT__USERID, &psz_userid)  );
                SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_audit, SG_AUDIT__TIMESTAMP, &t)  );

                SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt_audits)  );
                SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt_audits)  );

                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt_audits,1,psz_csid)  );
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,pStmt_audits,2,dagnum)  );
                SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmt_audits,3,psz_userid)  );
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,pStmt_audits,4,t)  );
                SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmt_audits,SQLITE_DONE)  );

                count_nodes++;
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

			// TODO: Should we check the blobs_referenced table to ensure this is a blob we actually want?
			//       The assumption is that a blob would never arrive here unless it was first returned by
			//       get_status, which means it's already in blobs_referenced.

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "hid", &psz_hid)  );
			SG_ASSERT(psz_hid && strlen(psz_hid));

			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "encoding", &i64)  );
			encoding = (SG_blob_encoding) i64;
			
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh, "vcdiff_ref", &psz_hid_vcdiff_reference)  );
			SG_ASSERT(NULL == psz_hid_vcdiff_reference || strlen(psz_hid_vcdiff_reference));
			
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_full", &i64)  );
			len_full = (SG_uint64) i64;
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "len_encoded", &i64)  );
			len_encoded = (SG_uint64) i64;
			SG_ERR_CHECK(  SG_file__tell(pCtx, pfb, &iPos)  );

			SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmtSaveBlobInfo)  );
			SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmtSaveBlobInfo)  );

			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmtSaveBlobInfo,1,psz_hid)  );
			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmtSaveBlobInfo,2,psz_filename)  );
			SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,pStmtSaveBlobInfo,3,iPos)  );
			SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx,pStmtSaveBlobInfo,4,encoding)  );
			SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,pStmtSaveBlobInfo,5,len_encoded)  );
			SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx,pStmtSaveBlobInfo,6,len_full)  );
			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx,pStmtSaveBlobInfo,7,psz_hid_vcdiff_reference)  );
			SG_ERR_CHECK(  sg_sqlite__step(pCtx,pStmtSaveBlobInfo,SQLITE_DONE)  );

			if (psz_hid_vcdiff_reference && SG_IS_BLOBENCODING_VCDIFF(encoding))
			{
				if (!psaDeltaRefHids)
					SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaDeltaRefHids, limitDeltaRefHids)  );
				else
				{
					SG_uint32 count = 0;
					SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaDeltaRefHids, &count)  );
					if (count == limitDeltaRefHids)
					{
						const char* const* asz = NULL;
						SG_ERR_CHECK(  SG_stringarray__sz_array(pCtx, psaDeltaRefHids, &asz)  );
						SG_ERR_CHECK(  _add_referenced_blobs(pCtx, pMe, SG_FALSE, asz, count)  );
						SG_ERR_CHECK(  SG_stringarray__remove_all(pCtx, psaDeltaRefHids, NULL, NULL)  );
					}
				}

				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaDeltaRefHids, psz_hid_vcdiff_reference)  );
			}

			/* seek ahead to skip the blob */
			SG_ERR_CHECK(  SG_file__seek(pCtx, pfb, iPos + len_encoded)  );

            count_blobs++;
		}
		else
		{
			SG_ERR_THROW(  SG_ERR_FRAGBALL_INVALID_OP  );
		}
		SG_VHASH_NULLFREE(pCtx, pvh);
	}

	if (psaDeltaRefHids)
	{
		SG_uint32 count = 0;
		const char* const* asz = NULL;
		SG_ERR_CHECK(  SG_stringarray__sz_array_and_count(pCtx, psaDeltaRefHids, &asz, &count)  );
		if (count)
			SG_ERR_CHECK(  _add_referenced_blobs(pCtx, pMe, SG_FALSE, asz, count)  );
	}

	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmtSaveBlobInfo)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt_audits)  );
	SG_STRINGARRAY_NULLFREE(pCtx, psaDeltaRefHids);

    //fprintf(stderr, "\nfragball had  %d nodes  %d blobs\n", count_nodes, count_blobs);

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmtSaveBlobInfo)  );
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_audits)  );
	SG_STRINGARRAY_NULLFREE(pCtx, psaDeltaRefHids);
}


static void _write_staging_version(SG_context* pCtx, const SG_pathname* pPath_staging, SG_uint32 iVersion)
{
	SG_file * pFile = NULL;
	SG_pathname* pPath = NULL;
	SG_uint32 written = 0;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPath_staging, VERSION_FILENAME)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile)  );
	SG_ERR_CHECK(  SG_file__write(pCtx, pFile, sizeof(SG_uint32), (SG_byte*)&iVersion, &written)  );
	if (written != sizeof(SG_uint32))
		SG_ERR_THROW(SG_ERR_INCOMPLETEWRITE);
	
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void _read_staging_version(SG_context* pCtx, const SG_pathname* pPath_staging, SG_uint32* piVersion)
{
	SG_uint32 version = 0;
	SG_uint32 got;
	SG_file * pFile = NULL;
	SG_pathname* pPath = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPath_staging, VERSION_FILENAME)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
	SG_ERR_CHECK(  SG_file__read(pCtx, pFile, sizeof(version), (SG_byte*)&version, &got)  );
	if (got != sizeof(version))
		SG_ERR_THROW(SG_ERR_INCOMPLETEREAD);
	else
		*piVersion = version;

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	return;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void _write_repo_descriptor_name(SG_context* pCtx, const SG_pathname* pPath_staging, const char* psz_repo_descriptor_name)
{
	SG_file * pFile = NULL;
	SG_pathname* pPath = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPath_staging, REPO_FILENAME)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile)  );
	SG_ERR_CHECK(  SG_file__write(pCtx, pFile, 1+SG_STRLEN(psz_repo_descriptor_name), (SG_byte*) psz_repo_descriptor_name, NULL)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	/* fallthru */

fail:
    SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

static void _read_repo_descriptor_name(SG_context* pCtx, const SG_pathname* pPath_staging, char** ppsz_repo_descriptor_name)
{
	SG_byte buf[1000];
	SG_uint32 got;
	SG_file * pFile = NULL;
	SG_pathname* pPath = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPath_staging, REPO_FILENAME)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
	SG_ERR_CHECK(  SG_file__read(pCtx, pFile, sizeof(buf), buf, &got)  );
	if (got == sizeof(buf))
		SG_ERR_THROW(SG_ERR_LIMIT_EXCEEDED);
	else
		SG_ERR_CHECK(  SG_STRDUP(pCtx, (const char*)buf, ppsz_repo_descriptor_name)  );

	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	/* fallthru */

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

///////////////////////////////////////////////////////////////////////////////

void SG_staging__create(SG_context* pCtx,
						char** ppsz_tid_staging, // An ID that uniquely identifies the staging area
						SG_staging** ppStaging)
{
	char* psz_staging_area_name = NULL;
	SG_pathname* pPath_staging = NULL;
	sg_staging* pMe = NULL;
	SG_staging* pStaging = NULL;

	SG_ERR_CHECK(  SG_tid__alloc(pCtx, &psz_staging_area_name)  );		// TODO consider SG_tid__alloc2(...)
	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, (const char*)psz_staging_area_name, &pPath_staging)  );

	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath_staging)  );
	SG_ERR_CHECK(  _write_staging_version(pCtx, pPath_staging, MY_VERSION)  );

	/* TODO do we want to write anything else into this staging area directory?  Like a readme? */

	if (ppStaging)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pMe)  );
		pStaging = (SG_staging*)pMe;
		pMe->pPath = pPath_staging;
		pPath_staging = NULL;
		SG_ERR_CHECK(  _open_db(pCtx, pMe->pPath, &pMe->psql)  );
		*ppStaging = pStaging;
		pStaging = NULL;
	}

	SG_RETURN_AND_NULL(psz_staging_area_name, ppsz_tid_staging);

	/* fallthru */

fail:
	SG_STAGING_NULLFREE(pCtx, pStaging);
	SG_NULLFREE(pCtx, psz_staging_area_name);
	SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
}

void SG_staging__open(SG_context* pCtx,
					  const char* psz_tid_staging,
					  SG_staging** ppStaging)
{
	SG_pathname* pPath_staging = NULL;
	SG_uint32 iVersion = 0;
	sg_staging* pMe = NULL;
	SG_staging* pStaging = NULL;
	char* pszDescriptorName = NULL;

	SG_ERR_CHECK(  SG_staging__get_pathname(pCtx, psz_tid_staging, &pPath_staging)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pMe)  );
	pStaging = (SG_staging*)pMe;

	pMe->pPath = pPath_staging;
	pPath_staging = NULL;

	SG_ERR_CHECK(  _read_staging_version(pCtx, pMe->pPath, &iVersion)  );
	if (iVersion != MY_VERSION)
	{
		SG_ERR_THROW2(  SG_ERR_UNKNOWN_STAGING_VERSION,
			(pCtx, "version %u at %s", iVersion, SG_pathname__sz(pMe->pPath))  );
	}

	SG_ERR_CHECK(  _open_db(pCtx, pMe->pPath, &pMe->psql)  );

	SG_RETURN_AND_NULL(pStaging, ppStaging);

	/* fallthru */

fail:
	SG_STAGING_NULLFREE(pCtx, pStaging);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
}

void SG_staging__cleanup(SG_context* pCtx,
						 SG_staging** ppStaging)
{
	sg_staging* pMe;

	SG_NULL_PP_CHECK_RETURN(ppStaging);
	pMe = (sg_staging*)*ppStaging;

	SG_ERR_CHECK(  sg_sqlite__close(pCtx, pMe->psql)  );
	pMe->psql = NULL;
	SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pMe->pPath) );

	/* fall through */
fail:
	SG_STAGING_NULLFREE(pCtx, *ppStaging);
}

void SG_staging__cleanup__by_id(
	SG_context* pCtx,
	const char* pszPushId)
{
	SG_pathname* pPath = NULL;

	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, pszPushId, &pPath)  );
	SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath) );

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void SG_staging__slurp_fragball(
	SG_context* pCtx,
	SG_staging* pStaging,
	const char* psz_filename
	)
{
	SG_vhash* pvh_version = NULL;
	const char* psz_version = NULL;
	SG_uint32 version = 0;
	SG_file* pfb = NULL;
	SG_pathname* pPath = NULL;
	sg_staging* pMe = (sg_staging*)pStaging;
	SG_bool bInSqlTx = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pStaging);
	SG_NULLARGCHECK_RETURN(psz_filename);

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pMe->pPath, psz_filename)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pfb)  );
	SG_PATHNAME_NULLFREE(pCtx, pPath);

	SG_ERR_CHECK(  SG_fragball__v1__read_object_header(pCtx, pfb, &pvh_version)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_version, "version", &psz_version)  );
	version = atoi(psz_version);
	SG_VHASH_NULLFREE(pCtx, pvh_version);

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION")  );
	bInSqlTx = SG_TRUE;
	switch (version)
	{
#if 0
		case 1:
			SG_ERR_CHECK(  _slurp__version_1(pCtx, pMe, psz_filename, pfb)  );
			break;
#endif
		case 2:
			SG_ERR_CHECK(  _slurp__version_2(pCtx, pMe, psz_filename, pfb)  );
			break;

		default:
			SG_ERR_THROW(  SG_ERR_FRAGBALL_INVALID_VERSION  );
			break;
	}
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION")  );
	bInSqlTx = SG_FALSE;

	SG_ERR_CHECK(  SG_file__close(pCtx, &pfb)  );

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_FILE_NULLCLOSE(pCtx, pfb);
	SG_VHASH_NULLFREE(pCtx, pvh_version);
	if (bInSqlTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION")  );
}

void SG_staging__commit(
	SG_context* pCtx,
	SG_staging* pStaging,
	SG_repo* pRepo
	)
{
	SG_repo_tx_handle* ptx = NULL;
	sg_staging* pMe = (sg_staging*)pStaging;
	SG_bool bInRepoTx = SG_FALSE;
	SG_bool bInSqlTx = SG_FALSE;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Committing", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, 3u, NULL)  );

	SG_NULLARGCHECK(pStaging);

	SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pRepo, &ptx)  );
	bInRepoTx = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION")  );
	bInSqlTx = SG_TRUE;

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Storing user data to repository")  );
	SG_ERR_CHECK(  _store_all_blobs(pCtx, pMe, pRepo, ptx)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Storing changesets to repository")  );
	SG_ERR_CHECK(  _store_all_frags(pCtx, pMe, pRepo, ptx)  );
	SG_ERR_CHECK(  _store_all_audits(pCtx, pMe, pRepo, ptx)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION")  );
	bInSqlTx = SG_FALSE;

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Committing all changes")  );
	SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &ptx)  );
	bInRepoTx = SG_FALSE;
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

fail:
	if (bInSqlTx)
		SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pMe->psql, "ROLLBACK TRANSACTION")  );
	if (bInRepoTx)
		SG_ERR_IGNORE(  SG_repo__abort_tx(pCtx, pRepo, &ptx)  );
	SG_log__pop_operation(pCtx);
}

void SG_staging__check_status(
	SG_context* pCtx,
	SG_staging* pStaging,
	SG_repo* pRepo,
	SG_bool bCheckConnectedness,
	SG_bool bListNewNodes,
	SG_bool bCheckChangesetBlobs,
	SG_bool bCheckDataBlobs,
	SG_bool bGetCounts,
	SG_vhash** ppResult)
{
	SG_vhash* pvh_status = NULL;
	sg_staging* pMe = (sg_staging*)pStaging;

	SG_NULLARGCHECK_RETURN(pStaging);

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_status)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "BEGIN TRANSACTION")  );
	SG_ERR_CHECK(  _check_all_frags(pCtx, pMe, pRepo, bCheckConnectedness, bListNewNodes, bGetCounts, pvh_status)  );
	if (bCheckChangesetBlobs || bCheckDataBlobs || bGetCounts)
		SG_ERR_CHECK(  _check_blobs(pCtx, pMe, pRepo, pvh_status, bCheckChangesetBlobs, bCheckDataBlobs, bGetCounts)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pMe->psql, "COMMIT TRANSACTION")  );

	*ppResult = pvh_status;
	pvh_status = NULL;

	/* fall through */

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_status);
}

void SG_staging__fetch_blob_into_memory(SG_context* pCtx,
										SG_staging_blob_handle* pStagingBlobHandle,
										SG_byte** ppBuf,
										SG_uint32* pLenFull)
{
	sg_staging_blob_handle* pbh = (sg_staging_blob_handle*)pStagingBlobHandle;
	SG_byte* pBufFull = NULL;
	SG_byte* pBufCompressed = NULL;
	SG_file* pFile = NULL;
	SG_uint32 len_got;

	SG_NULLARGCHECK_RETURN(pStagingBlobHandle);
	SG_NULLARGCHECK_RETURN(ppBuf);

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pbh->pFragballPathname, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pFile)  );
	SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, pbh->offset)  );

	switch(pbh->encoding)
	{
		case SG_BLOBENCODING__ALWAYSFULL:
		case SG_BLOBENCODING__FULL:
		{
			SG_ERR_CHECK(  SG_malloc(pCtx, (SG_uint32)pbh->len_full + 1, &pBufFull)  );
            pBufFull[pbh->len_full] = 0;

			SG_ERR_CHECK(  SG_file__read(pCtx, pFile, (SG_uint32)pbh->len_full, pBufFull, &len_got)  );
			if (len_got != pbh->len_full)
				SG_ERR_THROW(SG_ERR_INCOMPLETEREAD);

			break;
		}

		case SG_BLOBENCODING__ZLIB:
		{

			SG_ERR_CHECK(  SG_malloc(pCtx, (SG_uint32)pbh->len_full + 1, &pBufFull)  );
            pBufFull[pbh->len_full] = 0;

            // TODO the extra zero byte after zlib data shouldn't be necessary
			SG_ERR_CHECK(  SG_malloc(pCtx, (SG_uint32)pbh->len_encoded + 1, &pBufCompressed)  );
            pBufCompressed[pbh->len_encoded] = 0;

			SG_ERR_CHECK(  SG_file__read(pCtx, pFile, (SG_uint32)pbh->len_encoded, pBufCompressed, &len_got)  );
			if (len_got != pbh->len_encoded)
				SG_ERR_THROW(SG_ERR_INCOMPLETEREAD);

			SG_ERR_CHECK(  SG_zlib__inflate__memory(pCtx, pBufCompressed, (SG_uint32)pbh->len_encoded, pBufFull, (SG_uint32)pbh->len_full)  );
			SG_NULLFREE(pCtx, pBufCompressed);

			break;
		}

		case SG_BLOBENCODING__VCDIFF:
			/* Unecessary today because changesets are never deltified. Implement if that changes. */
			SG_ERR_THROW(SG_ERR_NOTIMPLEMENTED);
			break;

		default:
			SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Unknown blob encoding: %d", pbh->encoding));
	}

	SG_FILE_NULLCLOSE(pCtx, pFile);

	if (pLenFull)
		*pLenFull = (SG_uint32)pbh->len_full;
	*ppBuf = pBufFull;

	return;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, pBufFull);
	SG_NULLFREE(pCtx, pBufCompressed);
}

void SG_staging__get_pathname(SG_context* pCtx, const char* psz_tid_staging, SG_pathname** ppPathname)
{
	SG_pathname* pPath_staging = NULL;

	SG_NONEMPTYCHECK_RETURN(psz_tid_staging);
	SG_NULLARGCHECK_RETURN(ppPathname);

	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, psz_tid_staging, &pPath_staging)  );
#if defined(DEBUG)
	{
		// Ensure the staging path exists and that it's not a file.
		SG_bool staging_path_exists = SG_FALSE;
		SG_fsobj_type fsobj_type;
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_staging, &staging_path_exists, &fsobj_type, NULL)  );
		SG_ASSERT(staging_path_exists);
		SG_ASSERT(fsobj_type != SG_FSOBJ_TYPE__REGULAR);
	}
#endif

	*ppPathname = pPath_staging;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
}

void SG_staging__clone__create(
	SG_context* pCtx,
	const char* psz_new_descriptor_name,
	const SG_vhash* pvhRepoInfo,
	const char** ppszCloneId)
{
	char* pszCloneId = NULL;
	SG_pathname* pPathCloneStaging = NULL;
	SG_pathname* pPathRepoInfoVfile = NULL;
	SG_vfile* pvf = NULL;

	SG_ERR_CHECK(  SG_clone__validate_new_repo_name(pCtx, psz_new_descriptor_name)  );

	SG_ERR_CHECK(  SG_tid__alloc(pCtx, &pszCloneId)  );		// TODO consider SG_tid__alloc2(...)
	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, pszCloneId, &pPathCloneStaging)  );
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathCloneStaging)  );

	SG_ERR_CHECK(  _write_staging_version(pCtx, pPathCloneStaging, MY_VERSION)  );
	SG_ERR_CHECK(  _write_repo_descriptor_name(pCtx, pPathCloneStaging, psz_new_descriptor_name)  );

	// Write repo info vhash into __INFO__ file.
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathRepoInfoVfile, pPathCloneStaging, CLONE_REPO_INFO_FILENAME)  );
	SG_ERR_CHECK(  SG_vfile__overwrite(pCtx, pPathRepoInfoVfile, pvhRepoInfo)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathRepoInfoVfile);
	SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	
	SG_RETURN_AND_NULL(pszCloneId, ppszCloneId);

	return;

fail:
	SG_NULLFREE(pCtx, pszCloneId);
	SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	SG_PATHNAME_NULLFREE(pCtx, pPathRepoInfoVfile);
	SG_ERR_IGNORE(  SG_vfile__abort(pCtx, &pvf)  );
}

static void _clone__commit_prep(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_pathname* pPathFragball,
	SG_pathname** ppPathCloneStaging,
	char** ppszDescriptorName,
	SG_vhash** ppvhRepoInfo,
	SG_string** ppstrFragballFilename)
{
	SG_pathname* pPathCloneStaging = NULL;
	SG_uint32 iVersion = 0;
	char* pszDescriptorName = NULL;
	SG_pathname* pPathRepoInfoVfile = NULL;
	SG_vhash* pvhRepoInfo = NULL;
	SG_string* pstrFraballFilename = NULL;
	SG_pathname* pPathDestFragball = NULL;

	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, pszCloneId, &pPathCloneStaging)  );
	SG_ERR_CHECK(  _read_staging_version(pCtx, pPathCloneStaging, &iVersion)  );
	if (iVersion != MY_VERSION)
	{
		SG_ERR_THROW2(  SG_ERR_UNKNOWN_STAGING_VERSION,
			(pCtx, "version %u at %s", iVersion, SG_pathname__sz(pPathCloneStaging))  );
	}

	SG_ERR_CHECK(  _read_repo_descriptor_name(pCtx, pPathCloneStaging, &pszDescriptorName)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathRepoInfoVfile, pPathCloneStaging, CLONE_REPO_INFO_FILENAME)  );
	SG_ERR_CHECK(  SG_vfile__slurp(pCtx, pPathRepoInfoVfile, &pvhRepoInfo)  );

	SG_ERR_CHECK(  SG_pathname__get_last(pCtx, pPathFragball, &pstrFraballFilename)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDestFragball, pPathCloneStaging, SG_string__sz(pstrFraballFilename))  );
	SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPathFragball, pPathDestFragball)  );

	*ppPathCloneStaging = pPathCloneStaging;
	pPathCloneStaging = NULL;
	*ppszDescriptorName = pszDescriptorName;
	pszDescriptorName = NULL;
	*ppvhRepoInfo = pvhRepoInfo;
	pvhRepoInfo = NULL;
	*ppstrFragballFilename = pstrFraballFilename;
	pstrFraballFilename = NULL;

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_PATHNAME_NULLFREE(pCtx, pPathRepoInfoVfile);
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
	SG_STRING_NULLFREE(pCtx, pstrFraballFilename);
	SG_PATHNAME_NULLFREE(pCtx, pPathDestFragball);
}

void SG_staging__clone__commit(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_pathname** ppPathFragball)
{
	SG_pathname* pPathCloneStaging = NULL;
	char* pszDescriptorName = NULL;
	SG_vhash* pvhRepoInfo = NULL;
	SG_string* pstrFragballFilename = NULL;

	SG_NULLARGCHECK_RETURN(pszCloneId);
	SG_NULL_PP_CHECK_RETURN(ppPathFragball);

	SG_ERR_CHECK(  SG_log__report_info(pCtx, "Commiting remote clone %s from %s", pszCloneId, SG_pathname__sz(*ppPathFragball))  );

	SG_ERR_CHECK(  _clone__commit_prep(pCtx, pszCloneId, *ppPathFragball, &pPathCloneStaging, 
		&pszDescriptorName, &pvhRepoInfo, &pstrFragballFilename)  );
	SG_PATHNAME_NULLFREE(pCtx, *ppPathFragball);

	SG_ERR_CHECK(  SG_clone__from_fragball(pCtx, pszCloneId, pszDescriptorName, 
		pPathCloneStaging, SG_string__sz(pstrFragballFilename))  );

	/* fall through */
fail:
	if (pPathCloneStaging)
	{
		SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathCloneStaging)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	}
	if (ppPathFragball && *ppPathFragball)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, *ppPathFragball)  );
		SG_PATHNAME_NULLFREE(pCtx, *ppPathFragball);
	}
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_STRING_NULLFREE(pCtx, pstrFragballFilename);
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
}

void SG_staging__clone__commit_maybe_usermap(
	SG_context* pCtx,
	const char* pszCloneId,
	const char* pszClosetAdminId,
	SG_pathname** ppPathFragball,
	char** ppszDescriptorName,
	SG_bool* pbAvailable,
	SG_closet__repo_status* pStatus,
	char** ppszStatus)
{
	SG_pathname* pPathCloneStaging = NULL;
	char* pszDescriptorName = NULL;
	SG_vhash* pvhRepoInfo = NULL;
	SG_string* pstrFragballFilename = NULL;
	char* pszStatus = NULL;
	
	const char* pszRefRepoAdminId = NULL;

	SG_NONEMPTYCHECK_RETURN(pszCloneId);
	SG_NONEMPTYCHECK_RETURN(pszClosetAdminId);
	SG_NULL_PP_CHECK_RETURN(ppPathFragball);

	SG_ERR_CHECK(  _clone__commit_prep(pCtx, pszCloneId, *ppPathFragball, &pPathCloneStaging, 
		&pszDescriptorName, &pvhRepoInfo, &pstrFragballFilename)  );

	/* Commit for usermap if the repo has a different adminid. Otherwise commit normally. */
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhRepoInfo, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &pszRefRepoAdminId)  );
	if (strcmp(pszClosetAdminId, pszRefRepoAdminId))
	{
		SG_ERR_CHECK(  SG_log__report_info(pCtx, "Commiting usermap clone %s from %s", pszCloneId, SG_pathname__sz(*ppPathFragball))  );
		SG_ERR_CHECK(  SG_clone__from_fragball__prep_usermap(pCtx, pszCloneId, pszDescriptorName, 
			&pPathCloneStaging, SG_string__sz(pstrFragballFilename))  );
	}
	else
	{
		SG_ERR_CHECK(  SG_log__report_info(pCtx, "Commiting adminid-matched clone %s from %s", pszCloneId, SG_pathname__sz(*ppPathFragball))  );
		SG_ERR_CHECK(  SG_clone__from_fragball(pCtx, pszCloneId, pszDescriptorName, 
			pPathCloneStaging, SG_string__sz(pstrFragballFilename))  );
	}
	SG_PATHNAME_NULLFREE(pCtx, *ppPathFragball);

	if (pbAvailable || pStatus || ppszStatus)
	{
		SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
		SG_bool bAvailable = SG_FALSE;
		SG_ERR_CHECK(  SG_closet__descriptors__get__unavailable(pCtx, pszDescriptorName, NULL, 
			&bAvailable, &status, &pszStatus, NULL)  );
	
		if (pStatus)
			*pStatus = status;
		if (pbAvailable)
			*pbAvailable = bAvailable;
		SG_RETURN_AND_NULL(pszStatus, ppszStatus);
	}

	SG_RETURN_AND_NULL(pszDescriptorName, ppszDescriptorName);

	/* fall through */
fail:
	if (pPathCloneStaging)
	{
		SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathCloneStaging)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	}
	if (ppPathFragball && *ppPathFragball)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, *ppPathFragball)  );
		SG_PATHNAME_NULLFREE(pCtx, *ppPathFragball);
	}
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
	SG_STRING_NULLFREE(pCtx, pstrFragballFilename);
	SG_NULLFREE(pCtx, pszStatus);
}

void SG_staging__clone__abort(
	SG_context* pCtx,
	const char* pszCloneId)
{
	SG_pathname* pPathCloneStaging = NULL;
	SG_uint32 iVersion = 0;
	char* pszDescriptorName = NULL;
	SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
	SG_vhash* pvhDescriptor = NULL;
	const char* pszRefCloneId = NULL;

	SG_NONEMPTYCHECK_RETURN(pszCloneId);

	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, pszCloneId, &pPathCloneStaging)  );
	SG_ERR_CHECK(  _read_staging_version(pCtx, pPathCloneStaging, &iVersion)  );
	if (iVersion != MY_VERSION)
	{
		SG_ERR_THROW2(  SG_ERR_UNKNOWN_STAGING_VERSION,
			(pCtx, "version %u at %s", iVersion, SG_pathname__sz(pPathCloneStaging))  );
	}

	SG_ERR_CHECK(  _read_repo_descriptor_name(pCtx, pPathCloneStaging, &pszDescriptorName)  );
	SG_ERR_CHECK(  SG_closet__descriptors__get__unavailable(pCtx, pszDescriptorName, NULL, NULL, &status, NULL, &pvhDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhDescriptor, SG_SYNC_DESCRIPTOR_KEY__CLONE_ID, &pszRefCloneId)  );
	if ( (NULL != pszRefCloneId) && (0 == strcmp(pszRefCloneId, pszCloneId)) && (SG_REPO_STATUS__CLONING == status) )
	{
		/* It's our repo. Clean it up. */
		SG_ERR_CHECK(  SG_repo__delete_repo_instance(pCtx, pszDescriptorName)  );
		SG_ERR_CHECK(  SG_closet__descriptors__remove(pCtx, pszDescriptorName)  );
	}

	SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathCloneStaging) );

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	SG_NULLFREE(pCtx, pszDescriptorName);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
}

void SG_staging__clone__get_info(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_vhash** ppvhRepoInfo)
{
	SG_pathname* pPathCloneStaging = NULL;
	SG_uint32 iVersion = 0;
	SG_pathname* pPathRepoInfoVfile = NULL;
	SG_vhash* pvhRepoInfo = NULL;

	SG_NULLARGCHECK_RETURN(ppvhRepoInfo);

	SG_ERR_CHECK(  SG_sync__make_temp_path(pCtx, pszCloneId, &pPathCloneStaging)  );
	SG_ERR_CHECK(  _read_staging_version(pCtx, pPathCloneStaging, &iVersion)  );
	if (iVersion != MY_VERSION)
	{
		SG_ERR_THROW2(  SG_ERR_UNKNOWN_STAGING_VERSION,
			(pCtx, "version %u at %s", iVersion, SG_pathname__sz(pPathCloneStaging))  );
	}

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathRepoInfoVfile, pPathCloneStaging, CLONE_REPO_INFO_FILENAME)  );
	SG_ERR_CHECK(  SG_vfile__slurp(pCtx, pPathRepoInfoVfile, &pvhRepoInfo)  );

	*ppvhRepoInfo = pvhRepoInfo;
	pvhRepoInfo = NULL;

	/* fall through */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathCloneStaging);
	SG_PATHNAME_NULLFREE(pCtx, pPathRepoInfoVfile);
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
}
