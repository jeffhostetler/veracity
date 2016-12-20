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

/* TODO handle SQLITE_BUSY */

#define MY_SLEEP_MS      100
#define MY_TIMEOUT_MS  30000

struct _SG_treendx
{
	SG_repo* pRepo;
    SG_uint64 iDagNum;

	SG_pathname* pPath_db;  /* the sqlite db file */

    sqlite3* psql;
	SG_bool			bInTransaction;		// TODO sqlite3 does not allow nested transactions, right?

    // TODO bQueryOnly isn't used
    SG_bool bQueryOnly;

    struct
    {
        SG_ihash* pih_gidrow;
        SG_ihash* pih_csidrow;
        SG_bool b_in_update;
        SG_uint32 count_so_far;
        sqlite3_stmt* pStmt_gid;
        sqlite3_stmt* pStmt_csid;
        sqlite3_stmt* pStmt_changes;
        sqlite3_stmt* pStmt_paths;
    } update;
};

static void sg_treendx__create_indexes(SG_context* pCtx, SG_treendx* pdbc)
{
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "CREATE INDEX IF NOT EXISTS changes_idx_gidrow ON changes (gidrow, csidrow)")  );

fail:
    ;
}

static void sg_treendx__create_db(SG_context* pCtx, SG_treendx* pTreeNdx)
{
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pTreeNdx->psql, "CREATE TABLE gids (gidrow INTEGER PRIMARY KEY, gid VARCHAR UNIQUE NOT NULL)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pTreeNdx->psql, "CREATE TABLE csids (csidrow INTEGER PRIMARY KEY, csid VARCHAR UNIQUE NOT NULL, generation INTEGER NOT NULL)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pTreeNdx->psql, "CREATE TABLE changes (gidrow INTEGER NOT NULL, csidrow INTEGER NOT NULL)")  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pTreeNdx->psql, "CREATE TABLE paths (gidrow INTEGER NOT NULL, path VARCHAR NOT NULL, UNIQUE (gidrow,path))")  );

	return;
fail:
	return;
}

static void sg_treendx__check_if_need_to_reindex(
	SG_context* pCtx,
    sqlite3* psql
    )
{
	sqlite3_stmt* pStmt = NULL;
    SG_int32 exists = 0;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "SELECT COUNT(tbl_name) FROM sqlite_master WHERE tbl_name = 'paths' AND type = 'table'")  );

	SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_ROW)  );

	exists = (SG_int32)sqlite3_column_int(pStmt,0);

    if (!exists)
    {
		SG_ERR_THROW(  SG_ERR_NEED_REINDEX  );
    }

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_treendx__open(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	SG_pathname* pPath,
	SG_bool bQueryOnly,
	SG_treendx** ppNew
	)
{
	SG_treendx* pTreeNdx = NULL;
    SG_bool b_exists = SG_FALSE;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pTreeNdx)  );

    pTreeNdx->pRepo = pRepo;
    pTreeNdx->pPath_db = pPath;
    pPath = NULL;
    pTreeNdx->iDagNum = iDagNum;
    pTreeNdx->bQueryOnly = bQueryOnly;
	pTreeNdx->bInTransaction = SG_FALSE;

    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pTreeNdx->pPath_db, &b_exists, NULL, NULL)  );

    if (b_exists)
    {
        SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pTreeNdx->pPath_db, SG_SQLITE__SYNC__OFF, &pTreeNdx->psql)  );
        SG_ERR_CHECK(  sg_treendx__check_if_need_to_reindex(pCtx, pTreeNdx->psql)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pTreeNdx->pPath_db,SG_SQLITE__SYNC__OFF, &pTreeNdx->psql)  );

        SG_ERR_CHECK(  sg_treendx__create_db(pCtx, pTreeNdx)  );
    }

	*ppNew = pTreeNdx;

	return;
fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_TREENDX_NULLFREE(pCtx, pTreeNdx);
}

void SG_treendx__begin_update_multiple(
        SG_context* pCtx, 
        SG_treendx* pdbc
        )
{
    SG_ASSERT(!pdbc->update.b_in_update);

    memset(&pdbc->update, 0, sizeof(pdbc->update));

    pdbc->update.b_in_update = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "BEGIN IMMEDIATE TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_gid, "INSERT OR IGNORE INTO gids (gid) VALUES (?)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_csid, "INSERT OR IGNORE INTO csids (csid,generation) VALUES (?,?)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_changes, "INSERT INTO changes (gidrow, csidrow) VALUES (?, ?)")  );
    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pdbc->psql, &pdbc->update.pStmt_paths, "INSERT OR IGNORE INTO paths (gidrow, path) VALUES (?, ?)")  );
    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pdbc->update.pih_gidrow)  );
    SG_ERR_CHECK(  SG_ihash__alloc(pCtx, &pdbc->update.pih_csidrow)  );

fail:
    ;
}

void SG_treendx__end_update_multiple(SG_context* pCtx, SG_treendx* pdbc)
{
    SG_ASSERT(pdbc->update.b_in_update);

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_gid)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_csid)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_changes)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_paths)  );

    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_gidrow);
    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_csidrow);

    SG_ERR_CHECK(  sg_treendx__create_indexes(pCtx, pdbc)  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_FALSE;

    pdbc->update.b_in_update = SG_FALSE;

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pdbc->psql, "PRAGMA journal_mode=WAL")  );

fail:
    ;
}

void SG_treendx__abort_update_multiple(SG_context* pCtx, SG_treendx* pdbc)
{
    SG_ASSERT(pdbc->update.b_in_update);

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_gid)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_csid)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_changes)  );
	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pdbc->update.pStmt_paths)  );

    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_gidrow);
    SG_IHASH_NULLFREE(pCtx, pdbc->update.pih_csidrow);

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pdbc->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pdbc->bInTransaction = SG_FALSE;

    pdbc->update.b_in_update = SG_FALSE;

fail:
    ;
}

static void sg_treendx__build_paths(
        SG_context* pCtx,
        SG_treendx* pndx,
        SG_vhash* pvh_path_changes,
        SG_vhash* pvh_gids_changed,
        const char* psz_hid_treenode,
        const char* psz_path_parent,
        SG_uint32* pi_sofar
        )
{
    SG_treenode* ptn = NULL;
    SG_uint32 count_entries = 0;
    SG_uint32 i = 0;
    SG_string* pstr_path = NULL;
    SG_string* pstr_store = NULL;

    SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pndx->pRepo, psz_hid_treenode, &ptn)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, ptn, &count_entries)  );
	for (i=0; i<count_entries; i++)
	{
		const char* psz_gid = NULL;
		const SG_treenode_entry* pEntry = NULL;
        SG_bool b_new = SG_FALSE;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, ptn, i, &psz_gid, &pEntry)  );

        if (!pvh_path_changes)
        {
            b_new = SG_TRUE;
        }
        else
        {
            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_path_changes, psz_gid, &b_new)  );
            if (b_new)
            {
                *pi_sofar = *pi_sofar + 1;
            }
        }

        if (b_new)
        {
            SG_int64 gidrow = -1;
            const char* psz_name = NULL;
            SG_treenode_entry_type t;

            SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &t)  );

            if (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
            {
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_store, "D")  );
            }
            else if (SG_TREENODEENTRY_TYPE_REGULAR_FILE == t)
            {
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_store, "F")  );
            }
            else if (SG_TREENODEENTRY_TYPE_SYMLINK == t)
            {
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_store, "L")  );
            }
            else if (SG_TREENODEENTRY_TYPE_SUBMODULE == t)
            {
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_store, "S")  );
            }
            else
            {
                // TODO should probably be an error
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_store, "?")  );
            }

            gidrow = -1;
            SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pndx->update.pih_gidrow, psz_gid, &gidrow)  );
            if (gidrow < 0)
            {
                SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pndx->psql, &gidrow, "SELECT gidrow FROM gids WHERE gid='%s'", psz_gid)  );
                SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pndx->update.pih_gidrow, psz_gid, gidrow)  );
            }

            SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &psz_name)  );
            if (psz_path_parent)
            {
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_path, psz_path_parent)  );
                SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pstr_path, psz_name, SG_FALSE)  );
            }
            else
            {
                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_path, psz_name)  );
            }

            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_store, SG_string__sz(pstr_path))  );

            SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pndx->update.pStmt_paths)  );
            SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pndx->update.pStmt_paths)  );
            SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pndx->update.pStmt_paths, 1, gidrow)  );
            SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pndx->update.pStmt_paths, 2, SG_string__sz(pstr_store))  );

            SG_ERR_CHECK(  sg_sqlite__step(pCtx, pndx->update.pStmt_paths, SQLITE_DONE)  );
            SG_STRING_NULLFREE(pCtx, pstr_store);

            if (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
            {
                const char* psz_hid = NULL;

                SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &psz_hid)  );
                SG_ERR_CHECK(  sg_treendx__build_paths(pCtx, pndx, NULL, pvh_gids_changed, psz_hid, SG_string__sz(pstr_path), pi_sofar)  );
            }
        }

        SG_STRING_NULLFREE(pCtx, pstr_path);
	}
    
    if (pvh_path_changes)
    {
        SG_uint32 count_path_changes = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_path_changes, &count_path_changes)  );

        for (i=0; (i<count_entries) && (*pi_sofar < count_path_changes); i++)
        {
            const char* psz_gid = NULL;
            const SG_treenode_entry* pEntry = NULL;
            SG_treenode_entry_type t;
            SG_bool b_new = SG_FALSE;

            SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, ptn, i, &psz_gid, &pEntry)  );
            SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &t)  );

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_path_changes, psz_gid, &b_new)  );

            if (
                    !b_new
                    && (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
                )
            {
                SG_bool b_dive = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_gids_changed, psz_gid, &b_dive)  );

                if (b_dive)
                {
                    const char* psz_hid = NULL;
                    const char* psz_name = NULL;

                    SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &psz_name)  );
                    SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &psz_hid)  );

                    if (psz_path_parent)
                    {
                        SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_path, psz_path_parent)  );
                        SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pstr_path, psz_name, SG_FALSE)  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_path, psz_name)  );
                    }

                    SG_ERR_CHECK(  sg_treendx__build_paths(pCtx, pndx, pvh_path_changes, pvh_gids_changed, psz_hid, SG_string__sz(pstr_path), pi_sofar)  );
                    SG_STRING_NULLFREE(pCtx, pstr_path);
                }
            }
        }
    }

fail:
    SG_TREENODE_NULLFREE(pCtx, ptn);
    SG_STRING_NULLFREE(pCtx, pstr_path);
    SG_STRING_NULLFREE(pCtx, pstr_store);
}

void SG_treendx__update_one_changeset(
        SG_context* pCtx,
        SG_treendx* pTreeNdx,
        const char* psz_csid,
        SG_vhash* pvh_cs
        )
{
    SG_vhash* pvh_changes_by_parent = NULL;
    SG_vhash* pvh_path_changes = NULL;
    SG_vhash* pvh_gids_changed = NULL;
    SG_int32 generation = -1;
    SG_vhash* pvh_tree = NULL;
    SG_int64 csidrow = -1;

	SG_NULLARGCHECK_RETURN(psz_csid);
	SG_NULLARGCHECK_RETURN(pvh_cs);
	SG_NULLARGCHECK_RETURN(pTreeNdx);

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_gids_changed)  );
	SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_cs, "generation", &generation)  );

    csidrow = -1;
    SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pTreeNdx->update.pih_csidrow, psz_csid, &csidrow)  );
    if (csidrow < 0)
    {
        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pTreeNdx->update.pStmt_csid)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pTreeNdx->update.pStmt_csid)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pTreeNdx->update.pStmt_csid, 1, psz_csid)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pTreeNdx->update.pStmt_csid, 2, generation)  );
        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pTreeNdx->update.pStmt_csid, SQLITE_DONE)  );
        if (sqlite3_changes(pTreeNdx->psql))
        {
            csidrow = sqlite3_last_insert_rowid(pTreeNdx->psql);
        }
        else
        {
            SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pTreeNdx->psql, &csidrow, "SELECT csidrow FROM csids WHERE csid='%s'", psz_csid)  );
        }
        SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pTreeNdx->update.pih_csidrow, psz_csid, csidrow)  );
    }

	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_cs, "tree", &pvh_tree)  );
    if (pvh_tree)
    {
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_tree, "changes", &pvh_changes_by_parent)  );
    }

    if (pvh_changes_by_parent)
    {
        SG_uint32 count_parents = 0;
        SG_uint32 i_parent = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes_by_parent, &count_parents)  );

        for (i_parent=0; i_parent<count_parents; i_parent++)
        {
            SG_uint32 count_changes = 0;
            SG_uint32 i_chg = 0;
            const char* psz_csid_parent = NULL;
            SG_vhash* pvh_changes = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes_by_parent, i_parent, &psz_csid_parent, &pvh_changes)  );
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_changes)  );

            for (i_chg=0; i_chg<count_changes; i_chg++)
            {
                const SG_variant* pv = NULL;
                const char* psz_gid = NULL;
                const char* psz_dir = NULL;
                const char* psz_name = NULL;
                SG_int64 gidrow = -1;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_changes, i_chg, &psz_gid, &pv)  );
                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_gids_changed, psz_gid)  );

                if (SG_VARIANT_TYPE_VHASH == pv->type)
                {
                    SG_vhash* pvh_info = NULL;

                    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_info)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__NAME, &psz_name)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__DIR, &psz_dir)  );
                }
                else
                {
                    SG_ASSERT(SG_VARIANT_TYPE_NULL == pv->type);
                }

                if (psz_dir || psz_name)
                {
                    if (!pvh_path_changes)
                    {
                        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_path_changes)  );
                    }
                    SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_path_changes, psz_gid)  );
                }

                gidrow = -1;
                SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pTreeNdx->update.pih_gidrow, psz_gid, &gidrow)  );
                if (gidrow < 0)
                {
                    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pTreeNdx->update.pStmt_gid)  );
                    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pTreeNdx->update.pStmt_gid)  );
                    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pTreeNdx->update.pStmt_gid, 1, psz_gid)  );
                    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pTreeNdx->update.pStmt_gid, SQLITE_DONE)  );

                    if (sqlite3_changes(pTreeNdx->psql))
                    {
                        gidrow = sqlite3_last_insert_rowid(pTreeNdx->psql);
                    }
                    else
                    {
                        SG_ERR_CHECK(  sg_sqlite__exec__va__int64(pCtx, pTreeNdx->psql, &gidrow, "SELECT gidrow FROM gids WHERE gid='%s'", psz_gid)  );
                    }
                    SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pTreeNdx->update.pih_gidrow, psz_gid, gidrow)  );
                }

                SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pTreeNdx->update.pStmt_changes)  );
                SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pTreeNdx->update.pStmt_changes)  );
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pTreeNdx->update.pStmt_changes, 1, gidrow)  );
                SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pTreeNdx->update.pStmt_changes, 2, csidrow)  );
                SG_ERR_CHECK(  sg_sqlite__step(pCtx, pTreeNdx->update.pStmt_changes, SQLITE_DONE)  );
            }
        }
    }

    pTreeNdx->update.count_so_far++;

    if (pvh_path_changes)
    {
        const char* psz_root = NULL;
        SG_uint32 sofar = 0;

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree, "root", &psz_root)  );

        SG_ERR_CHECK(  sg_treendx__build_paths(pCtx, pTreeNdx, pvh_path_changes, pvh_gids_changed, psz_root, NULL, &sofar)  );
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_path_changes);
    SG_VHASH_NULLFREE(pCtx, pvh_gids_changed);
}

static void sg_treendx__stuff_gids_into_temp_table__psa(
        SG_context* pCtx,
        sqlite3* psql,
        SG_stringarray* psa_gids,
        const char* psz_table_name
        )
{
    SG_uint32 count_vals = 0;
    SG_uint32 i_val = 0;
    sqlite3_stmt* pStmt = NULL;

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE TEMP TABLE %s (gid VARCHAR NOT NULL UNIQUE)", psz_table_name)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT INTO %s (gid) VALUES (?)", psz_table_name)  );
    SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_gids, &count_vals)  );
    for (i_val=0; i_val<count_vals; i_val++)
    {
        const char* psz_val = NULL;

        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_gids, i_val, &psz_val)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_val)  );

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void sg_treendx__stuff_gids_into_temp_table__pvh(
        SG_context* pCtx,
        sqlite3* psql,
        SG_vhash* pvh_gids,
        const char* psz_table_name
        )
{
    SG_uint32 count_vals = 0;
    SG_uint32 i_val = 0;
    sqlite3_stmt* pStmt = NULL;

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE TEMP TABLE %s (gid VARCHAR NOT NULL UNIQUE)", psz_table_name)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT INTO %s (gid) VALUES (?)", psz_table_name)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids, &count_vals)  );
    for (i_val=0; i_val<count_vals; i_val++)
    {
        const char* psz_val = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_gids, i_val, &psz_val, NULL)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_val)  );

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_treendx__get_paths(
    SG_context* pCtx, 
    SG_treendx* pndx, 
    SG_vhash* pvh_gids,
    SG_vhash** ppvh
    )
{
	sqlite3_stmt* pStmt = NULL;
    SG_vhash* pvh = NULL;
	int rc = 0;
    char buf_table_gids[SG_TID_MAX_BUFFER_LENGTH];

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_table_gids, sizeof(buf_table_gids))  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pndx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );

    SG_ERR_CHECK(  sg_treendx__stuff_gids_into_temp_table__pvh(pCtx, pndx->psql, pvh_gids, buf_table_gids)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pndx->psql, &pStmt, "SELECT g.gid, p.path FROM gids g INNER JOIN paths p ON (g.gidrow = p.gidrow) WHERE g.gid IN (SELECT gid FROM %s)", buf_table_gids)  );

	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_gid = (const char*) sqlite3_column_text(pStmt, 0);
		const char* psz_path = (const char*) sqlite3_column_text(pStmt, 1);
        SG_varray* pva = NULL;

        SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvh, psz_gid, &pva)  );
        if (!pva)
        {
            SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh, psz_gid, &pva)  );
        }

        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, psz_path)  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
	}

	SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pndx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void SG_treendx__get_changesets_where_any_of_these_gids_changed(
        SG_context* pCtx, 
        SG_treendx* pTreeNdx, 
        SG_stringarray* psa_gids,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_results = NULL;
    int rc = 0;
    sqlite3_stmt* pStmt = NULL;
    char buf_table_gids[SG_TID_MAX_BUFFER_LENGTH];

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_table_gids, sizeof(buf_table_gids))  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pTreeNdx->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    SG_ERR_CHECK(  sg_treendx__stuff_gids_into_temp_table__psa(pCtx, pTreeNdx->psql, psa_gids, buf_table_gids)  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pTreeNdx->psql, &pStmt, "SELECT c.csid FROM csids c INNER JOIN changes h ON (h.csidrow = c.csidrow) INNER JOIN gids g ON (g.gidrow = h.gidrow) WHERE g.gid IN (SELECT gid FROM %s) ORDER BY generation DESC", buf_table_gids)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_results)  );

	while ((rc = sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		const char* psz_csid = (const char*) sqlite3_column_text(pStmt, 0);
        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_results, psz_csid)  );

	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(SG_ERR_SQLITE(rc));
	}

    *ppvh = pvh_results;
    pvh_results = NULL;

fail:
    SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pTreeNdx->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    SG_VHASH_NULLFREE(pCtx, pvh_results);
}

void SG_treendx__free(SG_context* pCtx, SG_treendx* pTreeNdx)
{
	if (!pTreeNdx)
		return;

	SG_ERR_IGNORE(  sg_sqlite__close(pCtx, pTreeNdx->psql)  );

	SG_PATHNAME_NULLFREE(pCtx, pTreeNdx->pPath_db);
	SG_NULLFREE(pCtx, pTreeNdx);
}

