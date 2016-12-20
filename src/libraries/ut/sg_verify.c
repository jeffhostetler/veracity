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

#include "sg.h"

#include <sg_vv2__public_typedefs.h>
#include <sg_wc__public_typedefs.h>

#include <sg_vv2__public_prototypes.h>
#include <sg_wc__public_prototypes.h>

typedef struct
{
    const SG_pathname* path_top;
    SG_vhash* pvh_top;
    const SG_pathname* path_cur;

} _readdirEachContext;

static void _readdirEach(
	SG_context *      pCtx,
	const SG_string * pstr_name,
	SG_fsobj_stat *   pfsStat,
	void *            pVoidData
    )
{
	_readdirEachContext *ctx = (_readdirEachContext *)pVoidData;
    SG_pathname* path_temp = NULL;
    SG_string* pstr_relpath = NULL;

    // path to current entry
    SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &path_temp, ctx->path_cur, SG_string__sz(pstr_name))  );
    // make it relative to the top
	SG_ERR_CHECK(  SG_pathname__make_relative(pCtx, ctx->path_top, path_temp, &pstr_relpath)  );
    if (SG_FSOBJ_TYPE__DIRECTORY == pfsStat->type)
    {
        // directories get added : null
        SG_ERR_CHECK(  SG_vhash__add__null(pCtx, ctx->pvh_top, SG_string__sz(pstr_relpath))  );
    }
    else
    {
        // everything else gets added : file_length
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, ctx->pvh_top, SG_string__sz(pstr_relpath), (SG_int64) pfsStat->size)  );
    }

    if (SG_FSOBJ_TYPE__DIRECTORY == pfsStat->type)
    {
        // recurse
        _readdirEachContext sub;

        memset(&sub, 0, sizeof(sub));
        sub.path_top = ctx->path_top;
        sub.path_cur = path_temp;
        sub.pvh_top = ctx->pvh_top;

        SG_ERR_CHECK(  SG_dir__foreach(pCtx,
                                       path_temp,
                                       SG_DIR__FOREACH__STAT
                                       | SG_DIR__FOREACH__SKIP_SELF
                                       | SG_DIR__FOREACH__SKIP_PARENT,
                                       _readdirEach,
                                       &sub)  );
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_relpath);
    SG_PATHNAME_NULLFREE(pCtx, path_temp);
}

static void ls_recursive_with_relative_paths_to_flat_vhash(
        SG_context* pCtx,
        const SG_pathname* path,
        SG_vhash** ppvh
        )
{
	_readdirEachContext ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.path_top = path;
    ctx.path_cur = path;
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &ctx.pvh_top)  );

	SG_ERR_CHECK(  SG_dir__foreach(pCtx,
								   path,
								   SG_DIR__FOREACH__STAT
								   | SG_DIR__FOREACH__SKIP_SELF
								   | SG_DIR__FOREACH__SKIP_PARENT,
								   _readdirEach,
								   &ctx)  );

    SG_ERR_CHECK(  SG_vhash__sort(pCtx, ctx.pvh_top, SG_FALSE, SG_vhash_sort_callback__increasing)  );

    *ppvh = ctx.pvh_top;
    ctx.pvh_top = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, ctx.pvh_top);
}

static void compare_two_ls_vhashes(
        SG_context * pCtx,
        SG_vhash* pvh_ls_current,
        SG_vhash* pvh_ls_parent,
        SG_vhash** ppvh_diffs
        )
{
    SG_vhash* pvh_diffs = NULL;
    SG_uint32 count_in_current = 0;
    SG_uint32 i_in_current = 0;
    SG_uint32 count_in_parent = 0;
    SG_uint32 i_in_parent = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_ls_parent, &count_in_parent)  );
    for (i_in_parent=0; i_in_parent<count_in_parent; i_in_parent++)
{
        const SG_variant* pv_in_parent = NULL;
        const char* psz_path_in_parent = NULL;
        SG_bool b_in_current = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_ls_parent, i_in_parent, &psz_path_in_parent, &pv_in_parent)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_ls_current, psz_path_in_parent, &b_in_current)  ); 
        if (!b_in_current)
        {
            SG_varray* pva_only_in_parent = NULL;

            if (!pvh_diffs)
            {
                SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_diffs)  );
            }

            SG_ERR_CHECK(  SG_vhash__ensure__varray(pCtx, pvh_diffs, "only_in_parent", &pva_only_in_parent)  );
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_only_in_parent, psz_path_in_parent)  );
        }
    }

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_ls_current, &count_in_current)  );
    for (i_in_current=0; i_in_current<count_in_current; i_in_current++)
    {
        const SG_variant* pv_in_current = NULL;
        const char* psz_path_in_current = NULL;
        SG_bool b_in_parent = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_ls_current, i_in_current, &psz_path_in_current, &pv_in_current)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_ls_parent, psz_path_in_current, &b_in_parent)  ); 
        if (!b_in_parent)
        {
            SG_varray* pva_only_in_current = NULL;

            if (!pvh_diffs)
            {
                SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_diffs)  );
            }

            SG_ERR_CHECK(  SG_vhash__ensure__varray(pCtx, pvh_diffs, "only_in_current", &pva_only_in_current)  );
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_only_in_current, psz_path_in_current)  );
        }
        else
        {
            const SG_variant* pv_in_parent = NULL;

            SG_ERR_CHECK(  SG_vhash__get__variant(pCtx, pvh_ls_parent, psz_path_in_current, &pv_in_parent)  );
            
            if (SG_VARIANT_TYPE_NULL == pv_in_current->type)
            {
                if (SG_VARIANT_TYPE_NULL == pv_in_parent->type)
                {
                    // match
                }
                else
                {
                    // bug
                }
            }
            else
            {
                if (SG_VARIANT_TYPE_NULL == pv_in_parent->type)
                {
                    // bug
                }
                else
                {
                    SG_int64 len_in_current = -1;
                    SG_int64 len_in_parent = -1;

                    SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pv_in_current, &len_in_current)  );
                    SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pv_in_parent, &len_in_parent)  );

                    if (len_in_current != len_in_parent)
                    {
                        SG_varray* pva_different = NULL;

                        if (!pvh_diffs)
                        {
                            SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_diffs)  );
                        }

                        SG_ERR_CHECK(  SG_vhash__ensure__varray(pCtx, pvh_diffs, "different", &pva_different)  );
                        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_different, psz_path_in_current)  );
                    }
                }
            }
        }
    }

    *ppvh_diffs = pvh_diffs;
    pvh_diffs = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_diffs);
}

static void x_which_path_is_correct_in_this_changeset(
        SG_context* pCtx,
        SG_tncache* ptn_cache,
        SG_treenode* ptn_root, 
        SG_varray* pva_possible_paths,
        const char* psz_gid,
        const char** ppsz_path_found,
        SG_treenode_entry** pptne
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    const char* psz_result = NULL;
    SG_treenode_entry* ptne = NULL;
    char* psz_found_gid = NULL;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_possible_paths, &count )  );
	for (i = 0; i < count; i++)
	{
        const char* psz_path = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_possible_paths, i, &psz_path)  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path__cached(pCtx, ptn_cache, ptn_root, 1+psz_path, &psz_found_gid, &ptne)  );
        if (
                ptne
                && (0 == strcmp(psz_gid, psz_found_gid))
           )
        {
            psz_result = 1 + psz_path;
            break;
        }
        SG_NULLFREE(pCtx, psz_found_gid);
        SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
	}

    if (psz_result)
    {
        *ppsz_path_found = psz_result;
        *pptne = ptne;
        ptne = NULL;
    }
    else
    {
        *ppsz_path_found = NULL;
        *pptne = NULL;
    }

fail:
    SG_NULLFREE(pCtx, psz_found_gid);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
}

static void add_error_object(
        SG_context* pCtx,
        SG_varray* pva_violations,
        SG_vhash** ppvh_err,
        ...
        )
{
    va_list ap;
    SG_vhash* pvh = NULL;

    SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_violations, &pvh)  );

    va_start(ap, ppvh_err);
    do
    {
        const char* psz_key = va_arg(ap, const char*);
        const char* psz_type = NULL;

        if (!psz_key)
        {
            break;
        }

        psz_type = va_arg(ap, const char*);

        if (0 == strcmp(psz_type, "sz"))
        {
            const char* psz_val = va_arg(ap, const char*);

			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, psz_key, psz_val)  );
        }
        else if (0 == strcmp(psz_type, "vhash"))
        {
            SG_vhash* pvh_val = va_arg(ap, SG_vhash*);

			SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvh, psz_key, pvh_val)  );
        }
		else
        {
            SG_ERR_THROW(  SG_ERR_UNSPECIFIED  );
        }

    } while (1);

    if (ppvh_err)
    {
        *ppvh_err = pvh;
    }

fail:
    va_end(ap);
}

static void dump_tree(
	SG_context* pCtx, 
    SG_tncache* ptncache,
	SG_treenode* pTreenodeParent, 
    const char* psz_path_dir
    )
{
	SG_uint32 count = 0;
	SG_uint32 i = 0;
	SG_string * pstr_cur_entry = NULL;
	
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenodeParent, &count)  );

    {
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_treenode__get_vhash_ref(pCtx, pTreenodeParent, &pvh)  );
        fprintf(stderr, "\n%s\n", psz_path_dir);
        SG_VHASH_STDERR(pvh);
    }

	for (i=0; i<count; i++)
	{
		const SG_treenode_entry* pEntry = NULL;
		const char* psz_name = NULL;
		const char* pszGIDTemp = NULL;
        SG_treenode_entry_type t = -1;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenodeParent, i, &pszGIDTemp, &pEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &psz_name)  );

        if (psz_path_dir[0])
        {
            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_cur_entry, psz_path_dir)  );
            SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pstr_cur_entry, psz_name, SG_FALSE)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_cur_entry, psz_name)  );
        }

        SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &t)  );

        if (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
        {
            const char* psz_hid = NULL;
            SG_treenode* ptn_cur_entry = NULL;
            SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &psz_hid)  );
            SG_ERR_CHECK(  SG_tncache__load(pCtx, ptncache, psz_hid, &ptn_cur_entry)  );
            SG_ERR_CHECK(  dump_tree(pCtx, ptncache, ptn_cur_entry, SG_string__sz(pstr_cur_entry))  );
        }
        SG_STRING_NULLFREE(pCtx, pstr_cur_entry);
	}

fail:
	SG_STRING_NULLFREE(pCtx, pstr_cur_entry);
}

static void get_manifest(
	SG_context* pCtx, 
    SG_tncache* ptncache,
	SG_treenode* pTreenodeParent, 
    const char* psz_path_dir,
    SG_vhash* pvh_manifest
    )
{
	SG_uint32 count = 0;
	SG_uint32 i = 0;
	SG_string * pstr_cur_entry = NULL;
	
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenodeParent, &count)  );

	for (i=0; i<count; i++)
	{
		const SG_treenode_entry* pEntry = NULL;
		const char* psz_name = NULL;
		const char* pszGIDTemp = NULL;
        SG_treenode_entry_type t = -1;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenodeParent, i, &pszGIDTemp, &pEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &psz_name)  );

        if (psz_path_dir[0])
        {
            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_cur_entry, psz_path_dir)  );
            SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pstr_cur_entry, psz_name, SG_FALSE)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_cur_entry, psz_name)  );
        }

        SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &t)  );

        if (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
        {
            const char* psz_hid = NULL;
            SG_treenode* ptn_cur_entry = NULL;
            SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &psz_hid)  );
            SG_ERR_CHECK(  SG_tncache__load(pCtx, ptncache, psz_hid, &ptn_cur_entry)  );
            SG_ERR_CHECK(  get_manifest(pCtx, ptncache, ptn_cur_entry, SG_string__sz(pstr_cur_entry), pvh_manifest)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_manifest, SG_string__sz(pstr_cur_entry))  );
        }
        SG_STRING_NULLFREE(pCtx, pstr_cur_entry);
	}

fail:
	SG_STRING_NULLFREE(pCtx, pstr_cur_entry);
}

static void check_for_implicit_deletes(
        SG_context* pCtx,
        SG_tncache* ptn_cache,
        const char* psz_path,
        const SG_treenode_entry* ptne_in_parent,
        const SG_treenode_entry* ptne_in_current,
        SG_vhash* pvh_changes_rel_to_parent,
        SG_vhash* pvh_deltas
        )
{
    const char* psz_hid_in_parent = NULL;
    SG_uint32 count_entries_in_parent = 0;
    SG_uint32 i_entry = 0;
    SG_treenode* ptn_dir_in_current = NULL;
    SG_treenode* ptn_dir_in_parent = NULL;
    SG_string* pstr_path = NULL;

    if (ptne_in_current)
    {
        const char* psz_hid_in_current = NULL;

        SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_in_current, &psz_hid_in_current)  );
        SG_ERR_CHECK(  SG_tncache__load(pCtx, ptn_cache, psz_hid_in_current, &ptn_dir_in_current)  );
    }

    SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_in_parent, &psz_hid_in_parent)  );
    SG_ERR_CHECK(  SG_tncache__load(pCtx, ptn_cache, psz_hid_in_parent, &ptn_dir_in_parent)  );

    SG_ERR_CHECK(  SG_treenode__count(pCtx, ptn_dir_in_parent, &count_entries_in_parent)  );
    for (i_entry=0; i_entry<count_entries_in_parent; i_entry++)
    {
        const SG_treenode_entry* pEntry_in_parent = NULL;
        const SG_treenode_entry* pEntry_in_current = NULL;
        const char* psz_gid_entry = NULL;

        SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, ptn_dir_in_parent, i_entry, &psz_gid_entry, &pEntry_in_parent)  );
        if (ptn_dir_in_current)
        {
            SG_ERR_CHECK(  SG_treenode__check_treenode_entry__by_gid__ref(pCtx, ptn_dir_in_current, psz_gid_entry, &pEntry_in_current)  );
        }

        if (!pEntry_in_current)
        {
            SG_bool b_deleted = SG_FALSE;
            SG_bool b_gid_in_changes = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_changes_rel_to_parent, psz_gid_entry, &b_gid_in_changes)  );
            if (!b_gid_in_changes)
            {
                b_deleted = SG_TRUE;
            }
            else
            {
                const SG_variant* pv = NULL;

                SG_ERR_CHECK(  SG_vhash__get__variant(pCtx, pvh_changes_rel_to_parent, psz_gid_entry, &pv)  );
                if (SG_VARIANT_TYPE_NULL == pv->type)
                {
                    b_deleted = SG_TRUE;
                }
                else
                {
                    SG_vhash* pvh = NULL;

                    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh)  );

                    // TODO could assert that this vhash has "dir", indicating
                    // that the item moved

                    b_deleted = SG_FALSE;
                }
            }

            if (b_deleted)
            {
                // implicit delete
                SG_treenode_entry_type t = -1;
                const char* psz_name = NULL;
                SG_vhash* pvh_del = NULL;
                SG_vhash* pvh_op = NULL;

                SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry_in_parent, &psz_name)  );

                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_path, psz_path)  );
                SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pstr_path, psz_name, SG_FALSE)  );

                SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "del", &pvh_del)  );
                SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_del, SG_string__sz(pstr_path), &pvh_op)  );

                SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry_in_parent, &t)  );
                if (SG_TREENODEENTRY_TYPE_DIRECTORY == t)
                {
                    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_op, "dir")  );

                    SG_ERR_CHECK(  check_for_implicit_deletes(pCtx, ptn_cache, SG_string__sz(pstr_path), pEntry_in_parent, NULL, pvh_changes_rel_to_parent, pvh_deltas)  );
                }
                SG_STRING_NULLFREE(pCtx, pstr_path);
            }
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr_path);
}

void SG_tree_deltas__convert_to_ops_with_paths(
        SG_context * pCtx,
        const char* psz_repo,
        const char* psz_csid_current,
        SG_vhash* pvh_cs_current,
        const char* psz_csid_parent,
        SG_varray* pva_errors,
        SG_vhash** ppvh_deltas,
        SG_vhash** ppvh_manifest_parent
        )
{
    SG_uint32 count_gids = 0;
    SG_uint32 i_gid = 0;
    SG_vhash* pvh_paths = NULL;
    SG_tncache* ptn_cache = NULL;
    const char* psz_root_current = NULL;
    const char* psz_root_parent = NULL;
    SG_treenode* ptn_root_current = NULL;
    SG_treenode* ptn_root_parent = NULL;
    SG_vhash* pvh_cs_parent = NULL;
    SG_vhash* pvh_tree_current = NULL;
    SG_vhash* pvh_tree_parent = NULL;
    SG_vhash* pvh_changes_rel_to_parent = NULL;
    SG_vhash* pvh_changes = NULL;
    const char* psz_path_in_current = NULL;
    const char* psz_path_in_parent = NULL;
    SG_treenode_entry* ptne_in_current = NULL;
    SG_treenode_entry* ptne_in_parent = NULL;
    SG_treenode_entry* ptne_containing_dir_in_current = NULL;
    SG_treenode_entry* ptne_containing_dir_in_parent = NULL;
    char* psz_gid_containing_dir_in_parent = NULL;
    char* psz_gid_containing_dir_in_current = NULL;
    SG_string* pstr_temp = NULL;
    SG_repo* pRepo = NULL;
    SG_vhash* pvh_deltas = NULL;
    SG_vhash* pvh_manifest_parent = NULL;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs_current, "tree", &pvh_tree_current)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree_current, "root", &psz_root_current)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_tree_current, "changes", &pvh_changes)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_changes, psz_csid_parent, &pvh_changes_rel_to_parent)  );

    SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid_parent, &pvh_cs_parent)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs_parent, "tree", &pvh_tree_parent)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree_parent, "root", &psz_root_parent)  );

    SG_ERR_CHECK(  SG_tncache__alloc(pCtx, pRepo, &ptn_cache)  );
    SG_ERR_CHECK(  SG_tncache__load(pCtx, ptn_cache, psz_root_current, &ptn_root_current)  );
    SG_ERR_CHECK(  SG_tncache__load(pCtx, ptn_cache, psz_root_parent, &ptn_root_parent)  );

    if (ppvh_manifest_parent)
    {
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_manifest_parent)  );
        SG_ERR_CHECK(  get_manifest(pCtx, ptn_cache, ptn_root_parent, "", pvh_manifest_parent)  );
    }

    SG_ERR_CHECK(  SG_repo__treendx__get_paths(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pvh_changes_rel_to_parent, &pvh_paths)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_deltas)  );
    // add all the subs so we can specify the ordering
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_deltas, "del", NULL)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_deltas, "add", NULL)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes_rel_to_parent, &count_gids)  );
    for (i_gid=0; i_gid<count_gids; i_gid++)
    {
        const char* psz_gid = NULL;
        const SG_variant* pv = NULL;
        SG_varray* pva_paths = NULL;
        SG_treenode_entry_type t_parent = -1;
        SG_treenode_entry_type t_current = -1;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_changes_rel_to_parent, i_gid, &psz_gid, &pv)  );
        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_paths, psz_gid, &pva_paths)  );

        SG_ERR_CHECK(  x_which_path_is_correct_in_this_changeset(
                    pCtx, 
                    ptn_cache, 
                    ptn_root_parent, 
                    pva_paths, 
                    psz_gid, 
                    &psz_path_in_parent, 
                    &ptne_in_parent
                    )  );
        if (ptne_in_parent)
        {
            SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne_in_parent, &t_parent)  );
            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_temp, psz_path_in_parent)  );
            SG_ERR_CHECK(  SG_repopath__remove_last(pCtx, pstr_temp)  );
            SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path__cached(pCtx, ptn_cache, ptn_root_parent, SG_string__sz(pstr_temp), &psz_gid_containing_dir_in_parent, &ptne_containing_dir_in_parent)  );
            SG_STRING_NULLFREE(pCtx, pstr_temp);
        }
        SG_ERR_CHECK(  x_which_path_is_correct_in_this_changeset(
                    pCtx, 
                    ptn_cache, 
                    ptn_root_current, 
                    pva_paths, 
                    psz_gid, 
                    &psz_path_in_current, 
                    &ptne_in_current
                    )  );
        if (ptne_in_current)
        {
            SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne_in_current, &t_current)  );
            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_temp, psz_path_in_current)  );
            SG_ERR_CHECK(  SG_repopath__remove_last(pCtx, pstr_temp)  );
            SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path__cached(pCtx, ptn_cache, ptn_root_current, SG_string__sz(pstr_temp), &psz_gid_containing_dir_in_current, &ptne_containing_dir_in_current)  );
            SG_STRING_NULLFREE(pCtx, pstr_temp);
        }

        if (SG_VARIANT_TYPE_NULL == pv->type)
        {
            if (pva_errors)
            {
                if (!psz_path_in_parent)
                {
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "deleted but not present in parent",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
                }

                if (psz_path_in_current)
                {
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "deleted but still present in current",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                "path", "sz", psz_path_in_current,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
                }

                if (psz_path_in_parent && !psz_path_in_current)
                {
                    SG_bool b_has = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_changes_rel_to_parent, psz_gid_containing_dir_in_parent, &b_has)  );

                    if (!b_has)
                    {
                        SG_vhash* pvhe = NULL;
                        SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                    "error", "sz", "deleted but containing dir gid not in delta",
                                    "csid", "sz", psz_csid_current,
                                    "parent", "sz", psz_csid_parent,
                                    "gid", "sz", psz_gid,
                                    "path", "sz", psz_path_in_current,
                                    "gid_containing_dir", "sz", psz_gid_containing_dir_in_parent,
                                    NULL
                                    )  );
                        SG_VHASH_STDERR(pvhe);
                    }
                }
            }

#if 0 // no need to do this.  we process all deletes as implicit
            {
                SG_vhash* pvh_del = NULL;
                SG_vhash* pvh_op = NULL;

                SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "del", &pvh_del)  );
                SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_del, psz_path_in_parent, &pvh_op)  );

                if (SG_TREENODEENTRY_TYPE_DIRECTORY == t_parent)
                {
                    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_op, "dir")  );
                }
            }
#endif
        }
        else
        {
            SG_vhash* pvh_changes_for_one_gid = NULL;
            const char* psz_hid = NULL;
            const char* psz_dir = NULL;
            const char* psz_name = NULL;
            SG_int32 type = -1;

            SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_changes_for_one_gid)  );

            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_changes_for_one_gid, "dir", &psz_dir)  );
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_changes_for_one_gid, "hid", &psz_hid)  );
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_changes_for_one_gid, "name", &psz_name)  );
            SG_ERR_CHECK(  SG_vhash__check__int32(pCtx, pvh_changes_for_one_gid, "type", &type)  );

            if (pva_errors)
            {
                if (!psz_path_in_current)
                {
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "not found in current",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
                }
                
                if (
                        !psz_hid 
                        && (psz_dir || psz_name)
                        && !psz_path_in_parent
                        )
                {
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "path change only, but found in parent",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                "path", "sz", psz_path_in_current,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
                }

                if (
                        psz_name
                        && psz_path_in_parent
                        && (0 == strcmp(psz_path_in_current, psz_path_in_parent))
                   )
                {
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "path change but same path",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                "path", "sz", psz_path_in_current,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
                }

                if (
                        psz_dir
                        && psz_path_in_parent
                        && (0 == strcmp(psz_path_in_current, psz_path_in_parent))
                   )
                {
#if 0 // TODO what does this case mean?
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "path change but same path",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                "path", "sz", psz_path_in_current,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
#endif
                }

                if (
                        ptne_in_parent
                        && ptne_in_current
                   )
                {
                    SG_treenode_entry_type t_parent = -1;

                    SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne_in_parent, &t_parent)  );

                    if (t_current != t_parent)
                    {
                        SG_vhash* pvhe = NULL;
                        SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                    "error", "sz", "type change",
                                    "csid", "sz", psz_csid_current,
                                    "parent", "sz", psz_csid_parent,
                                    "gid", "sz", psz_gid,
                                    "path", "sz", psz_path_in_current,
                                    NULL
                                    )  );
                        SG_VHASH_STDERR(pvhe);
                    }

                }

                if (psz_hid)
                {
                    const char* psz_hid_in_ptne_current = NULL;

                    SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_in_current, &psz_hid_in_ptne_current)  );
                    if (0 != strcmp(psz_hid, psz_hid_in_ptne_current))
                    {
                        SG_vhash* pvhe = NULL;
                        SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                    "error", "sz", "hid mismatch",
                                    "csid", "sz", psz_csid_current,
                                    "parent", "sz", psz_csid_parent,
                                    "gid", "sz", psz_gid,
                                    "path", "sz", psz_path_in_current,
                                    NULL
                                    )  );
                        SG_VHASH_STDERR(pvhe);
                    }

                    if (ptne_in_parent)
                    {
                        const char* psz_hid_in_ptne_parent = NULL;

                        SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_in_parent, &psz_hid_in_ptne_parent)  );
                        if (0 == strcmp(psz_hid_in_ptne_current, psz_hid_in_ptne_parent))
                        {
                            SG_vhash* pvhe = NULL;
                            SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                        "error", "sz", "hid changed but it didn't",
                                        "csid", "sz", psz_csid_current,
                                        "parent", "sz", psz_csid_parent,
                                        "gid", "sz", psz_gid,
                                        "path", "sz", psz_path_in_current,
                                        NULL
                                        )  );
                            SG_VHASH_STDERR(pvhe);
                        }
                    }
                }

                if (
                        (-1 != type)
                        && psz_path_in_parent
                   )
                {
                    SG_vhash* pvhe = NULL;
                    SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                                "error", "sz", "delta shows type change, but not present in parent",
                                "csid", "sz", psz_csid_current,
                                "parent", "sz", psz_csid_parent,
                                "gid", "sz", psz_gid,
                                "path", "sz", psz_path_in_current,
                                NULL
                                )  );
                    SG_VHASH_STDERR(pvhe);
                }
            }

            if (
                    psz_path_in_parent
                    && (
                        (0 != strcmp(psz_path_in_current, psz_path_in_parent))
                        || psz_dir
                       )
               )
            {
                char buf_tid[SG_TID_MAX_BUFFER_LENGTH];
                SG_vhash* pvh_del = NULL;
                SG_vhash* pvh_add = NULL;
                SG_vhash* pvh_op_del = NULL;
                SG_vhash* pvh_op_add = NULL;
                
                SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_tid, sizeof(buf_tid))  );

                SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "del", &pvh_del)  );
                SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "add", &pvh_add)  );

                SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_del, psz_path_in_parent, &pvh_op_del)  );
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op_del, "mv", buf_tid)  );
                if (SG_TREENODEENTRY_TYPE_DIRECTORY == t_parent)
                {
                    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_op_del, "dir")  );
                }
                
                SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_add, psz_path_in_current, &pvh_op_add)  );
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op_add, "mv", buf_tid)  );

                if (SG_TREENODEENTRY_TYPE_DIRECTORY == t_current)
                {
                    // TODO anything to do here?
                }
            }

            if (psz_hid)
            {
                if (SG_TREENODEENTRY_TYPE_DIRECTORY == t_current)
                {
                    if (!psz_path_in_parent)
                    {
                        SG_vhash* pvh_add = NULL;
                        SG_vhash* pvh_op = NULL;

                        SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "add", &pvh_add)  );
                        SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_add, psz_path_in_current, &pvh_op)  );
                        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op, "op", "mkdir")  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  check_for_implicit_deletes(pCtx, ptn_cache, psz_path_in_parent, ptne_in_parent, ptne_in_current, pvh_changes_rel_to_parent, pvh_deltas)  );
                    }
                }
                else if (SG_TREENODEENTRY_TYPE_SYMLINK == t_current)
                {
                    SG_vhash* pvh_add = NULL;
                    SG_vhash* pvh_op = NULL;

                    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "add", &pvh_add)  );
                    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_add, psz_path_in_current, &pvh_op)  );
                    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op, "op", "symlink")  );
                    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op, "hid", psz_hid)  );
                }
                else
                {
                    SG_vhash* pvh_add = NULL;
                    SG_vhash* pvh_op = NULL;

                    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_deltas, "add", &pvh_add)  );
                    SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pvh_add, psz_path_in_current, &pvh_op)  );
                    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op, "op", "file")  );
                    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_op, "hid", psz_hid)  );
                }
            }
        }
        SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_in_current);
        SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_in_parent);
        SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_containing_dir_in_current);
        SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_containing_dir_in_parent);
        SG_NULLFREE(pCtx, psz_gid_containing_dir_in_parent);
        SG_NULLFREE(pCtx, psz_gid_containing_dir_in_current);
    }

    {
        SG_vhash* pvh_sub = NULL;

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_deltas, "del", &pvh_sub)  );
        SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_sub, SG_FALSE, SG_vhash_sort_callback__decreasing)  );

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_deltas, "add", &pvh_sub)  );
        SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_sub, SG_FALSE, SG_vhash_sort_callback__increasing)  );
    }

    *ppvh_deltas = pvh_deltas;
    pvh_deltas = NULL;

    if (ppvh_manifest_parent)
    {
        *ppvh_manifest_parent = pvh_manifest_parent;
        pvh_manifest_parent = NULL;
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_manifest_parent);
    SG_VHASH_NULLFREE(pCtx, pvh_deltas);
	SG_REPO_NULLFREE(pCtx, pRepo);
    SG_VHASH_NULLFREE(pCtx, pvh_cs_parent);
    SG_VHASH_NULLFREE(pCtx, pvh_paths);
    SG_TNCACHE_NULLFREE(pCtx, ptn_cache);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_in_current);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_in_parent);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_containing_dir_in_current);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_containing_dir_in_parent);
    SG_NULLFREE(pCtx, psz_gid_containing_dir_in_parent);
    SG_NULLFREE(pCtx, psz_gid_containing_dir_in_current);
}

static void verify__tree_deltas__pair(
        SG_context * pCtx,
        const char* psz_repo,
        const char* psz_csid_current,
        SG_vhash* pvh_cs_current,
        SG_vhash* pvh_ls_current,
        const char* psz_csid_parent,
        SG_varray* pva_errors,
        SG_vhash* pvh_ls_collection
        )
{
    SG_pathname* path_tree_parent = NULL;
    SG_repo* pRepo = NULL;
    SG_vhash* pvh_ls_parent = NULL;
    SG_vhash* pvh_diffs = NULL;
    SG_vhash* pvh_deltas = NULL;
    SG_pathname* path_op = NULL;
    SG_pathname* path_op_other = NULL;
    SG_file* pFile = NULL;
    SG_vhash* pvh_ls_freeme = NULL;
	SG_rev_spec* pRevSpec = NULL;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

    if (pvh_ls_current)
    {
        SG_bool b_already = SG_FALSE;

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &path_tree_parent)  );
		SG_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, path_tree_parent, psz_csid_parent)  );

		SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpec)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_csid_parent)  );

        SG_ERR_CHECK(  SG_vv2__export(pCtx,
									  psz_repo,
									  SG_pathname__sz(path_tree_parent),
									  pRevSpec,
									  NULL
                                      )  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_ls_collection, psz_csid_parent, &b_already)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  ls_recursive_with_relative_paths_to_flat_vhash(pCtx, path_tree_parent, &pvh_ls_freeme)  );
            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_ls_collection, psz_csid_parent, &pvh_ls_freeme)  );
        }
    }

    SG_ERR_CHECK(  SG_tree_deltas__convert_to_ops_with_paths(pCtx, psz_repo, psz_csid_current, pvh_cs_current, psz_csid_parent, pva_errors, &pvh_deltas, NULL)  );

    if (path_tree_parent)
    {
        SG_vhash* pvh_del = NULL;
        SG_vhash* pvh_add = NULL;
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        // now apply all the deltas

        //SG_VHASH_STDERR(pvh_deltas);

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_deltas, "del", &pvh_del)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_del, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_path = NULL;
            SG_vhash* pvh_op = NULL;
            const char* psz_mv = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_del, i, &psz_path, &pvh_op)  );
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "mv", &psz_mv)  );

            SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &path_op, path_tree_parent, 2+psz_path)  );
            if (!psz_mv)
            {
                SG_fsobj_type fsObjType = SG_FSOBJ_TYPE__UNSPECIFIED;
                SG_bool b_exists = SG_FALSE;

                SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, path_op, &b_exists, &fsObjType, NULL)  );
                //fprintf(stderr, "SG_fsobj__remove__pathname: %s\n", SG_pathname__sz(path_op));
                if (SG_FSOBJ_TYPE__DIRECTORY == fsObjType)
                {
                    SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, path_op, SG_TRUE)  );
                }
                else
                {
                    SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, path_op)  );
                }
            }
            else
            {
                SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &path_op_other, path_tree_parent, psz_mv)  );

                SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, path_op, path_op_other)  );
            }
            SG_PATHNAME_NULLFREE(pCtx, path_op);
            SG_PATHNAME_NULLFREE(pCtx, path_op_other);
        }

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_deltas, "add", &pvh_add)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_path = NULL;
            SG_vhash* pvh_op = NULL;
            const char* psz_op = NULL;
            const char* psz_mv = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_add, i, &psz_path, &pvh_op)  );
            SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &path_op, path_tree_parent, 2+psz_path)  );

            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "mv", &psz_mv)  );

            if (psz_mv)
            {
                SG_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &path_op_other, path_tree_parent, psz_mv)  );
                SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, path_op_other, path_op)  );
            }

            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "op", &psz_op)  );

            if (psz_op)
            {
                if (0 == strcmp(psz_op, "mkdir"))
                {
                    SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, path_op)  );
                }
                else if (0 == strcmp(psz_op, "file"))
                {
                    const char* psz_hid = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_op, "hid", &psz_hid)  );

                    SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, path_op)  );
                    //fprintf(stderr, "SG_fsobj__open__pathname: %s\n", SG_pathname__sz(path_op));
                    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, path_op, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile)  );
                    SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pRepo, psz_hid, pFile, NULL)  );
                    SG_FILE_NULLCLOSE(pCtx, pFile);
                }
                else if (0 == strcmp(psz_op, "symlink"))
                {
                    const char* psz_hid = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_op, "hid", &psz_hid)  );

                    SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, path_op)  );
                    //fprintf(stderr, "SG_fsobj__open__pathname: %s\n", SG_pathname__sz(path_op));
                    // TODO real symlink
                    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, path_op, SG_FILE_WRONLY|SG_FILE_CREATE_NEW, 0644, &pFile)  );
                    SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pRepo, psz_hid, pFile, NULL)  );
                    SG_FILE_NULLCLOSE(pCtx, pFile);
                }
                else
                {
                    // TODO throw
                }
            }

            SG_PATHNAME_NULLFREE(pCtx, path_op);
            SG_PATHNAME_NULLFREE(pCtx, path_op_other);
        }

        SG_ERR_CHECK(  ls_recursive_with_relative_paths_to_flat_vhash(pCtx, path_tree_parent, &pvh_ls_parent)  );
        
        SG_ERR_CHECK(  compare_two_ls_vhashes(pCtx, pvh_ls_current, pvh_ls_parent, &pvh_diffs)  );

        if (pvh_diffs)
        {
            SG_vhash* pvhe = NULL;
            SG_ERR_CHECK(  add_error_object(pCtx, pva_errors, &pvhe,
                        "error", "sz", "trees don't match",
                        "csid", "sz", psz_csid_current,
                        "parent", "sz", psz_csid_parent,
                        "diffs", "vhash", pvh_diffs,
                        "deltas", "vhash", pvh_deltas,
                        NULL
                        )  );
            SG_VHASH_STDERR(pvhe);
        }

        SG_VHASH_NULLFREE(pCtx, pvh_diffs);
        SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, path_tree_parent, SG_TRUE)  );
        SG_VHASH_NULLFREE(pCtx, pvh_ls_parent);
    }

fail:
    if (SG_context__has_err(pCtx))
    {
        fprintf(stderr, "csid: %s\n", psz_csid_current);
    }
    if (path_tree_parent)
    {
        SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname2(pCtx, path_tree_parent, SG_TRUE)  );
    }
    SG_FILE_NULLCLOSE(pCtx, pFile);
    SG_PATHNAME_NULLFREE(pCtx, path_op);
    SG_PATHNAME_NULLFREE(pCtx, path_op_other);
    SG_VHASH_NULLFREE(pCtx, pvh_ls_freeme);
    SG_VHASH_NULLFREE(pCtx, pvh_deltas);
    SG_VHASH_NULLFREE(pCtx, pvh_diffs);
    SG_VHASH_NULLFREE(pCtx, pvh_ls_parent);
	SG_REPO_NULLFREE(pCtx, pRepo);
    SG_PATHNAME_NULLFREE(pCtx, path_tree_parent);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

void sg_verify__dump_tree(
        SG_context * pCtx,
        const char* psz_repo,
        const char* psz_csid
        )
{
	SG_repo* pRepo = NULL;
    SG_tncache* ptn_cache = NULL;
    SG_vhash* pvh_cs = NULL;
    SG_vhash* pvh_tree = NULL;
    const char* psz_root = NULL;
    SG_treenode* ptn_root = NULL;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid, &pvh_cs)  );
    SG_VHASH_STDERR(pvh_cs);

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs, "tree", &pvh_tree)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree, "root", &psz_root)  );
    SG_ERR_CHECK(  SG_tncache__alloc(pCtx, pRepo, &ptn_cache)  );
    SG_ERR_CHECK(  SG_tncache__load(pCtx, ptn_cache, psz_root, &ptn_root)  );
    
    SG_ERR_CHECK(  dump_tree(pCtx, ptn_cache, ptn_root, "")  );

fail:
    SG_TNCACHE_NULLFREE(pCtx, ptn_cache);
    SG_VHASH_NULLFREE(pCtx, pvh_cs);
    SG_REPO_NULLFREE(pCtx, pRepo);
}

static void verify__tree_deltas__one(
        SG_context * pCtx,
        const char* psz_repo,
        const char* psz_csid,
        SG_varray* pva_errors,
        SG_vhash* pvh_ls_collection
        )
{
	SG_repo* pRepo = NULL;
    SG_vhash* pvh_cs = NULL;
    SG_vhash* pvh_cs_parent = NULL;
    SG_pathname* path_temp_tree = NULL;
    SG_vhash* pvh_ls = NULL;
    SG_vhash* pvh_ls_freeme = NULL;
    SG_vhash* pvh_ls_parent = NULL;
    SG_vhash* pvh_tree = NULL;
    const char* psz_root = NULL;
    SG_vhash* pvh_changes = NULL;
    SG_uint32 count_parents = 0;
    SG_uint32 count_changes_parents = 0;
    SG_varray* pva_parents = NULL;
    SG_int32 gen = -1;
    SG_int32 gen_max_parent = -1;
    SG_uint32 i_parent = 0;
	SG_rev_spec* pRevSpec = NULL;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid, &pvh_cs)  );

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs, "tree", &pvh_tree)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree, "root", &psz_root)  );
    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_tree, "changes", &pvh_changes)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_changes_parents)  );
    SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvh_cs, "parents", &pva_parents)  );
    if (pva_parents)
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_parents, &count_parents)  );
    }
    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_cs, "generation", &gen)  );
    if (
            count_parents
            && (count_parents != count_changes_parents)
       )
    {
        SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "csid %s: %d parents vs %d", psz_csid, count_parents, count_changes_parents)  );
    }

    if (
            (0 == count_parents)
            && (1 != count_changes_parents)
       )
    {
        SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "csid %s: first changeset should have no parents but have one delta against empty string parent", psz_csid)  );
    }

    for (i_parent=0; i_parent<count_parents; i_parent++)
    {
        const char* psz_csid_parent = NULL;
        SG_vhash* pvh_changes_for_one_parent = NULL;
        SG_int32 my_gen = -1;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i_parent, &psz_csid_parent)  );
        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes, psz_csid_parent, &pvh_changes_for_one_parent)  );
        if (!pvh_changes_for_one_parent)
        {
            SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "csid %s: parent mismatch %s", psz_csid, psz_csid_parent)  );
        }

        SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid_parent, &pvh_cs_parent)  );
        SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_cs_parent, "generation", &my_gen)  );
        if (my_gen > gen_max_parent)
        {
            gen_max_parent = my_gen;
        }
        SG_VHASH_NULLFREE(pCtx, pvh_cs_parent);
    }

    if (count_parents)
    {
        if (gen != (1 + gen_max_parent))
        {
            SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "csid %s: gen is %d but should be %d", psz_csid, gen, (1 + gen_max_parent))  );
        }
    }
    else
    {
        if (gen != 1)
        {
            SG_ERR_THROW2(  SG_ERR_UNSPECIFIED, (pCtx, "csid %s: first changeset gen is %d but should be 1", psz_csid, gen)  );
        }
    }

#if 1
    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_ls_collection, psz_csid, &pvh_ls)  );
    if (pvh_ls)
    {
        //fprintf(stderr, "\nFound ls for %s\n", psz_csid);
    }
    else
    {
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &path_temp_tree)  );
        SG_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, path_temp_tree, psz_csid)  );

		SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpec)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_csid)  );

        SG_ERR_CHECK(  SG_vv2__export(pCtx,
									  psz_repo,
									  SG_pathname__sz(path_temp_tree),
									  pRevSpec,
									  NULL
                                      )  );
        SG_ERR_CHECK(  ls_recursive_with_relative_paths_to_flat_vhash(pCtx, path_temp_tree, &pvh_ls_freeme)  );
        pvh_ls = pvh_ls_freeme;
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_ls_collection, psz_csid, &pvh_ls_freeme)  );
        SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname2(pCtx, path_temp_tree, SG_TRUE)  );
        SG_PATHNAME_NULLFREE(pCtx, path_temp_tree);
    }
#endif

    for (i_parent=0; i_parent<count_parents; i_parent++)
    {
        const char* psz_csid_parent = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_parents, i_parent, &psz_csid_parent)  );

        SG_ERR_CHECK(  verify__tree_deltas__pair(
                    pCtx,
                    psz_repo,
                    psz_csid,
                    pvh_cs,
                    pvh_ls,
                    psz_csid_parent,
                    pva_errors,
                    pvh_ls_collection
                    )  );
    }

    SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_ls_collection, psz_csid)  );
    SG_VHASH_NULLFREE(pCtx, pvh_cs);

fail:
    if (path_temp_tree)
    {
        SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname2(pCtx, path_temp_tree, SG_TRUE)  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh_ls_freeme);
    SG_VHASH_NULLFREE(pCtx, pvh_cs);
    SG_VHASH_NULLFREE(pCtx, pvh_ls_parent);
    SG_VHASH_NULLFREE(pCtx, pvh_cs_parent);
    SG_PATHNAME_NULLFREE(pCtx, path_temp_tree);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

void SG_verify__tree_deltas(
        SG_context * pCtx,
        const char* psz_repo
        )
{
	SG_repo* pRepo = NULL;
    SG_uint32 i_changesets = 0;
    SG_uint32 count_changesets = 0;
    SG_bool b_pop = SG_FALSE;
    SG_varray* pva_errors = NULL;
    SG_ihash* pih_csids = NULL;
    SG_vhash* pvh_ls_collection = NULL;

    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo, &pRepo)  );

    SG_ERR_CHECK(  SG_repo__fetch_dagnode_ids(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, -1, -1, &pih_csids)  );
    SG_ERR_CHECK(  SG_ihash__count(pCtx, pih_csids, &count_changesets)  );

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Verifying tree deltas", SG_LOG__FLAG__NONE)  );
    b_pop = SG_TRUE;

    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_changesets, NULL)  );

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva_errors)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_ls_collection)  );

    for (i_changesets = 0; i_changesets < count_changesets; i_changesets++)
    {
        const char* psz_csid = NULL;

        SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pih_csids, i_changesets, &psz_csid, NULL)  );

        SG_ERR_CHECK(  verify__tree_deltas__one(pCtx, psz_repo, psz_csid, pva_errors, pvh_ls_collection)  );

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_pop = SG_FALSE;

    {
        SG_uint32 count_errors = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_errors, &count_errors)  );
        if (count_errors)
        {
            SG_VARRAY_STDERR(pva_errors);
        }
    }

fail:
    if (b_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
    SG_VARRAY_NULLFREE(pCtx, pva_errors);
    SG_VHASH_NULLFREE(pCtx, pvh_ls_collection);
    SG_IHASH_NULLFREE(pCtx, pih_csids);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_verify__tree_deltas__one(
        SG_context * pCtx,
        const char* psz_repo,
        const char* psz_csid
        )
{
    SG_varray* pva_errors = NULL;
    SG_vhash* pvh_ls_collection = NULL;

    //SG_ERR_CHECK(  sg_verify__dump_tree(pCtx, psz_repo, psz_csid)  );

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva_errors)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_ls_collection)  );

    SG_ERR_CHECK(  verify__tree_deltas__one(pCtx, psz_repo, psz_csid, pva_errors, pvh_ls_collection)  );

    {
        SG_uint32 count_errors = 0;

        SG_ERR_CHECK(  SG_varray__count(pCtx, pva_errors, &count_errors)  );
        if (count_errors)
        {
            SG_VARRAY_STDERR(pva_errors);
        }
    }

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_errors);
    SG_VHASH_NULLFREE(pCtx, pvh_ls_collection);
}


