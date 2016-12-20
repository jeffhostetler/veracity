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

static void x_get_audits(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_audits = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_audits)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_csid = NULL;
        const char* psz_userid = NULL;
        SG_int64 timestamp = -1;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "csid", &psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "userid", &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, "timestamp", &timestamp)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_audits, psz_csid, &b_already)  );
        if (!b_already)
        {
            SG_vhash* pvh_a = NULL;

            SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_audits, psz_csid, &pvh_a)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_a, "userid", psz_userid)  );
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_a, "timestamp", timestamp)  );
        }
    }

    *ppvh = pvh_audits;
    pvh_audits = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_audits);
}

static void x_get_comments(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_comments = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_comments)  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const char* psz_val = NULL;
        const char* psz_csid = NULL;
        SG_bool b_already = SG_FALSE;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "text", &psz_val)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "csid", &psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_comments, psz_csid, &b_already)  );
        if (!b_already)
        {
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_comments, psz_csid, psz_val)  );
        }
    }

    *ppvh = pvh_comments;
    pvh_comments = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_comments);
}

static void x_get_users(
        SG_context * pCtx,
        SG_varray* pva,
        SG_vhash** ppvh
        )
{
    SG_vhash* pvh_users = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_users)  );
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
    }

    *ppvh = pvh_users;
    pvh_users = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_users);
}

static void write_blobs(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_blobset* pbs,
    SG_vhash* pvh_files,
    SG_file* pfi,
    SG_vhash* pvh_blobid_to_mark,
    SG_uint32* pi_next_mark
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_files, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_path = NULL;
        SG_vhash* pvh_op = NULL;
        const char* psz_hid = NULL;

        SG_uint64 len_full = 0;
        SG_uint64 len_check = 0;
        SG_bool b_already = SG_FALSE;
        SG_int64 my_mark = -1;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_files, i, &psz_path, &pvh_op)  );

        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "hid", &psz_hid)  );

        if (psz_hid)
        {
            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_blobid_to_mark, psz_hid, &b_already)  );
            if (b_already)
            {
                SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_blobid_to_mark, psz_hid, &my_mark)  );
            }
            else
            {
                SG_bool b_found = SG_FALSE;

                my_mark = (*pi_next_mark)++;
                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "blob\n")  );
                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "mark :%d\n", (int) my_mark)  );
                SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_blobid_to_mark, psz_hid, my_mark)  );
                SG_ERR_CHECK(  SG_blobset__lookup(
                            pCtx, 
                            pbs, 
                            psz_hid, 
                            &b_found, 
                            NULL, 
                            NULL, 
                            NULL, 
                            NULL, 
                            &len_full, 
                            NULL
                            )  );
                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "data %d\n", (SG_uint32) len_full)  ); // TODO 64 bit int
                SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(
                            pCtx, 
                            pRepo, 
                            psz_hid, 
                            pfi,
                            &len_check
                            )  );
                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "\n")  );
            }
        }
    }

fail:
    ;
}

static void get_contents__recursive(
    SG_context * pCtx,
    const char* psz_path,
    SG_vhash* pvh_manifest,
    SG_vhash* pvh_contents
    )
{
    SG_string* pstr = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr, psz_path)  );
    SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pstr)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_manifest, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_cur = NULL;
        
        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_manifest, i, &psz_cur, NULL)  );
        if (0 == strncmp(psz_cur, SG_string__sz(pstr), SG_string__length_in_bytes(pstr)))
        {
            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_contents, psz_cur)  );
            SG_ERR_CHECK(  get_contents__recursive(pCtx, psz_cur, pvh_manifest, pvh_contents)  );
        }
    }

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

static void get_contents(
    SG_context * pCtx,
    const char* psz_path,
    SG_vhash* pvh_manifest,
    SG_vhash** ppvh_contents
    )
{
    SG_vhash* pvh_contents = NULL;
    SG_uint32 count = 0;

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_contents)  );
    SG_ERR_CHECK(  get_contents__recursive(pCtx, psz_path, pvh_manifest, pvh_contents)  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_contents, &count)  );
    if (!count)
    {
        SG_VHASH_NULLFREE(pCtx, pvh_contents);
    }

    *ppvh_contents = pvh_contents;
    pvh_contents = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_contents);
}

void SG_fast_export__export(
    SG_context * pCtx,
	const char * psz_repo,
	const char * psz_fi
	)
{
    SG_vhash* pvh_pile = NULL;
	SG_dagnode* pdn = NULL;
    SG_repo* pRepo = NULL;
	char** paszHids = NULL;
	const char* pszHid = NULL;
	SG_repo_fetch_dagnodes_handle* pdh = NULL;
    SG_uint32 count_nodes;
    SG_uint32 count_tags;
    SG_uint32 i;
    SG_uint32 i_changeset;
    SG_uint32 i_tag;
    SG_vhash* pvh_result = NULL;
    SG_file* pfi = NULL;
    SG_pathname* pPath_fi = NULL;
    SG_varray* pva_csid_list__branch_names = NULL;
    SG_uint32 count_pop = 0;

    SG_string* pstr_new_path = NULL;

    SG_varray* pva_all_audits = NULL;
    SG_varray* pva_all_comments = NULL;
    SG_varray* pva_all_tags = NULL;
    SG_varray* pva_all_users = NULL;

    SG_vhash* pvh_branch_names = NULL;
    SG_vhash* pvh_audits = NULL;
    SG_vhash* pvh_comments = NULL;
    SG_vhash* pvh_users = NULL;

    SG_vhash* pvh_deltas = NULL;
    SG_vhash* pvh_add = NULL;
    SG_vhash* pvh_del = NULL;

    SG_vhash* pvh_dir_contents = NULL;
    SG_vhash* pvh_manifest = NULL;
    SG_vhash* pvh_csid_to_commit_mark = NULL;
    SG_vhash* pvh_blobid_to_mark = NULL;
    SG_uint32 next_mark = 1;
    SG_vhash* pvh_cs_current = NULL;
    SG_string* pstr_branch_name = NULL;
    SG_vhash* pvh_branch_name_to_csid = NULL;

    SG_vhash* pvh_paths = NULL;
    SG_blobset* pbs = NULL;

    SG_NULLARGCHECK_RETURN(psz_repo);
    SG_NULLARGCHECK_RETURN(psz_fi);

    SG_ERR_CHECK(  SG_repo__open_repo_instance(pCtx, psz_repo, &pRepo)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_branch_name_to_csid)  );

    SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepo, 0, 0, 0, &pbs)  );

    SG_ERR_CHECK(  SG_repo__fetch_chrono_dagnode_list(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, 0, 0, &count_nodes, &paszHids)  );
    
    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh_result, count_nodes, NULL, NULL)  );
    SG_ERR_CHECK(  SG_varray__alloc__params(pCtx, &pva_csid_list__branch_names, count_nodes, NULL, NULL)  );

    SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, &pdh)  );
    pszHid = (const char*)paszHids;
    for (i = 0; i < count_nodes; i++)
    {
        SG_int32 gen = -1;
        //SG_uint32 count_parents = 0;
        //char** parents = NULL;
        SG_vhash* pvh_dn = NULL;

        SG_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pRepo, pdh, pszHid, &pdn)  );
        SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
        //SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pdn, &count_parents, &parents)  );

        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_csid_list__branch_names, pszHid)  );
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_result, pszHid, &pvh_dn)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_dn, "g", gen)  ); 
#if 0
        if (parents)
        {
            SG_varray* pva_parents = NULL;
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvh_dn, "p", &pva_parents)  );
            for (i=0; i<count_parents; i++)
            {
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_parents, parents[i])  );
            }
        }
#endif

        pszHid += SG_HID_MAX_BUFFER_LENGTH;

        SG_DAGNODE_NULLFREE(pCtx, pdn);
    }
    SG_ERR_CHECK(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pdh)  );
    SG_NULLFREE(pCtx, paszHids);

    //SG_VHASH_STDOUT(pvh_result);

    SG_ERR_CHECK(  SG_vc_branches__get_branch_names_for_given_changesets(pCtx, pRepo, &pva_csid_list__branch_names, &pvh_branch_names)  );
    SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pva_all_users)  );
    SG_ERR_CHECK(  SG_repo__query_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, -1, -1, NULL, &pva_all_audits)  );

    SG_ERR_CHECK(  SG_vc_comments__list_all(pCtx, pRepo, &pva_all_comments)  );
    SG_ERR_CHECK(  SG_vc_tags__list_all__with_history(pCtx, pRepo, &pva_all_tags)  );

    SG_ERR_CHECK(  x_get_users(pCtx, pva_all_users, &pvh_users)  );
    SG_ERR_CHECK(  x_get_comments(pCtx, pva_all_comments, &pvh_comments)  );
    SG_ERR_CHECK(  x_get_audits(pCtx, pva_all_audits, &pvh_audits)  );

    //SG_VHASH_STDOUT(pvh_branch_names);

    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh_csid_to_commit_mark, count_nodes, NULL, NULL)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_blobid_to_mark)  );

    SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPath_fi, psz_fi)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_fi, SG_FILE_WRONLY | SG_FILE_CREATE_NEW, 0644, &pfi)  );

    /*

       'commit' SP <ref> LF
       mark?
       ('author' (SP <name>)? SP LT <email> GT SP <when> LF)?
       'committer' (SP <name>)? SP LT <email> GT SP <when> LF
       data
       ('from' SP <committish> LF)?
       ('merge' SP <committish> LF)?
       (filemodify | filedelete | filecopy | filerename | filedeleteall | notemodify)*
       LF?

    */

    SG_ERR_CHECK(  SG_vhash__sort__vhash_field_int__asc(pCtx, pvh_result, "g")  );
    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_result, &count_nodes)  );
    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Exporting", SG_LOG__FLAG__NONE)  );
    count_pop++;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_nodes, NULL)  );

	// the first changeset will always be the auto-generated initial commit by the nobody user
	// we don't want to export that one, just add it to the csid->mark map with special mark 0
	// which means the csid refers to a commit that we're not exporting
	{
		const char* psz_csid_current = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_result, 0u, &psz_csid_current, NULL)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_csid_to_commit_mark, psz_csid_current, 0)  );
		SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
	}

    for (i_changeset = 1; i_changeset < count_nodes; i_changeset++)
    {
        const char* psz_csid_current = NULL;
        const char* psz_branch_name = NULL;
        const char* psz_comment = NULL;
        SG_vhash* pvh_a = NULL;
        SG_vhash* pvh_branch_names_for_current_changeset = NULL;
        const char* psz_csid_parent_from = NULL;
        const char* psz_csid_parent_merge = NULL;
        const char* psz_prev_value_for_branch = NULL;
        SG_uint32 count_branch_names = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_result, i_changeset, &psz_csid_current, NULL)  );

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branch_names, psz_csid_current, &pvh_branch_names_for_current_changeset)  );
        if (pvh_branch_names_for_current_changeset)
        {
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_names_for_current_changeset, &count_branch_names)  );
            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_names_for_current_changeset, 0, &psz_branch_name, NULL)  );
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_branch_name_to_csid, psz_branch_name, &psz_prev_value_for_branch)  );
        }
        else
        {
            psz_branch_name = "vvanonymous";
        }

        SG_ERR_CHECK(  SG_repo__fetch_vhash(pCtx, pRepo, psz_csid_current, &pvh_cs_current)  );

        {
            SG_vhash* pvh_tree = NULL;
            SG_vhash* pvh_changes = NULL;
            const char* psz_root_current = NULL;
            SG_uint32 count_parents = 0;

            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cs_current, "tree", &pvh_tree)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tree, "root", &psz_root_current)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_tree, "changes", &pvh_changes)  );
            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );

            if (count_parents > 0)
            {
                if (psz_prev_value_for_branch)
                {
                    SG_bool b_has = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_changes, psz_prev_value_for_branch, &b_has)  );
                    if (b_has)
                    {
                        psz_csid_parent_from = psz_prev_value_for_branch;
                    }
                }
                if (!psz_csid_parent_from)
                {
                    SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, 0, &psz_csid_parent_from, NULL)  );
                }
            }
            if (count_parents > 1)
            {
                SG_uint32 i = 0;

                for (i=0; i<count_parents; i++)
                {
                    const char* psz = NULL;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_changes, i, &psz, NULL)  );
                    if (0 != strcmp(psz, psz_csid_parent_from))
                    {
                        psz_csid_parent_merge = psz;
                        break;
                    }
                }
            }
        }

        if (psz_csid_parent_from)
        {
            SG_ERR_CHECK(  SG_tree_deltas__convert_to_ops_with_paths(pCtx, psz_repo, psz_csid_current, pvh_cs_current, psz_csid_parent_from, NULL, &pvh_deltas, &pvh_manifest)  );
#if 0
            fprintf(stderr, "csid %s from %s\n", psz_csid_current, psz_csid_parent_from);
            SG_VHASH_STDERR(pvh_cs_current);
            SG_VHASH_STDERR(pvh_deltas);
#endif
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_deltas, "add", &pvh_add)  );
            SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_deltas, "del", &pvh_del)  );
            SG_ERR_CHECK(  write_blobs(pCtx, pRepo, pbs, pvh_add, pfi, pvh_blobid_to_mark, &next_mark)  );
        }

        //SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "# csid %s from %s\n", psz_csid_current, psz_csid_parent_from)  );

        // hmmph.  the git-fast-import spec says that a branch name can
        // have spaces in it, but git itself barfs.

        SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_branch_name, psz_branch_name)  );
        SG_ERR_CHECK( SG_string__replace_bytes(
            pCtx,
            pstr_branch_name,
            (const SG_byte *)" ",1,
            (const SG_byte *)"_",1,
            SG_UINT32_MAX,SG_TRUE,
            NULL
            )  );
        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "commit refs/heads/%s\n", SG_string__sz(pstr_branch_name))  );
        SG_STRING_NULLFREE(pCtx, pstr_branch_name);

        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "mark :%d\n", next_mark)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_csid_to_commit_mark, psz_csid_current, next_mark)  );
        next_mark++;

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_audits, psz_csid_current, &pvh_a)  );
        if (pvh_a)
        {
            const char* psz_userid = NULL;
            const char* psz_username = NULL;
            SG_int64 timestamp = -1;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_a, "userid", &psz_userid)  );
            SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_users, psz_userid, &psz_username)  );
            if (!psz_username)
            {
                psz_username = SG_NOBODY__USERNAME;
            }

            SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_a, "timestamp", &timestamp)  );
            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "committer <%s> %d +0000\n", psz_username, (SG_int32) (timestamp / 1000))  );
        }
        else
        {
            // TODO error no audit
        }

        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_comments, psz_csid_current, &psz_comment)  );
        if (psz_comment)
        {
            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "data %d\n", SG_STRLEN(psz_comment))  );
            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "%s\n", psz_comment)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "data 0\n")  );
        }

        if (i_changeset > 1)
        {
            if (psz_csid_parent_from)
            {
                SG_int32 mark = -1;

                SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_csid_to_commit_mark, psz_csid_parent_from, &mark)  );
                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "from :%d\n", mark)  );

                if (psz_csid_parent_merge)
                {
                    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_csid_to_commit_mark, psz_csid_parent_merge, &mark)  );
                    SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "merge :%d\n", mark)  );
                }
            }
        }

        {
            SG_uint32 count = 0;
            SG_uint32 i = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_del, &count)  );
            for (i=0; i<count; i++)
            {
                const char* psz_path = NULL;
                SG_vhash* pvh_op = NULL;
                SG_bool b_dir = SG_FALSE;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_del, i, &psz_path, &pvh_op)  );
                SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_op, "dir", &b_dir)  );

                if (b_dir)
                {
                    SG_ERR_CHECK(  get_contents(pCtx, psz_path, pvh_manifest, &pvh_dir_contents)  );
                    if (!pvh_dir_contents)
                    {
                        //SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "# already empty %s\n", 2+psz_path)  );
                    }
                    else
                    {
                        const char* psz_tid = NULL;
                        SG_uint32 count_dir_contents = 0;
                        SG_uint32 i = 0;

                        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dir_contents, &count_dir_contents)  );
                        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "mv", &psz_tid)  );
                        if (psz_tid)
                        {
                            for (i=0; i<count_dir_contents; i++)
                            {
                                const char* psz_path_item = NULL;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dir_contents, i, &psz_path_item, NULL)  );
                                
                                SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_new_path, psz_path_item)  );
                                SG_ERR_CHECK( SG_string__replace_bytes(
                                    pCtx,
                                    pstr_new_path,
                                    (const SG_byte *)psz_path,SG_STRLEN(psz_path),
                                    (const SG_byte *)psz_tid,SG_STRLEN(psz_tid),
                                    SG_UINT32_MAX,SG_TRUE,
                                    NULL
                                    )  );

                                SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_manifest, psz_path_item)  );
                                SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_manifest, SG_string__sz(pstr_new_path))  );
                                
                                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "R %s %s\n", 2+psz_path_item, SG_string__sz(pstr_new_path))  );

                                SG_STRING_NULLFREE(pCtx, pstr_new_path);
                            }
                        }
                        else
                        {
                            for (i=0; i<count_dir_contents; i++)
                            {
                                const char* psz_path_item = NULL;

                                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dir_contents, i, &psz_path_item, NULL)  );

                                SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_manifest, psz_path_item)  );

                                SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "D %s\n", 2+psz_path_item)  );
                            }
                        }

                        SG_VHASH_NULLFREE(pCtx, pvh_dir_contents);
                    }
                }
                else
                {
                    const char* psz_tid = NULL;

                    SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_manifest, psz_path)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "mv", &psz_tid)  );
                    if (psz_tid)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_manifest, psz_tid)  );
                        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "R %s %s\n", 2+psz_path, psz_tid)  );
                    }
                    else
                    {
                        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "D %s\n", 2+psz_path)  );
                    }
                }
            }

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_add, &count)  );
            for (i=0; i<count; i++)
            {
                const char* psz_path = NULL;
                SG_vhash* pvh_op = NULL;
                const char* psz_op = NULL;
                const char* psz_mv = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_add, i, &psz_path, &pvh_op)  );

                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "mv", &psz_mv)  );

                if (psz_mv)
                {
                    SG_ERR_CHECK(  get_contents(pCtx, psz_mv, pvh_manifest, &pvh_dir_contents)  );
                    if (pvh_dir_contents)
                    {
                        SG_uint32 count_dir_contents = 0;
                        SG_uint32 i = 0;

                        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_dir_contents, &count_dir_contents)  );
                        for (i=0; i<count_dir_contents; i++)
                        {
                            const char* psz_path_item = NULL;

                            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_dir_contents, i, &psz_path_item, NULL)  );
                            
                            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_new_path, psz_path_item)  );
                            SG_ERR_CHECK( SG_string__replace_bytes(
                                pCtx,
                                pstr_new_path,
                                (const SG_byte *)psz_mv,SG_STRLEN(psz_mv),
                                (const SG_byte *)psz_path,SG_STRLEN(psz_path),
                                SG_UINT32_MAX,SG_TRUE,
                                NULL
                                )  );

                            SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_manifest, psz_path_item)  );
                            SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_manifest, SG_string__sz(pstr_new_path))  );

                            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "R %s %s\n", psz_path_item, 2+SG_string__sz(pstr_new_path))  );
                            
                            SG_STRING_NULLFREE(pCtx, pstr_new_path);
                        }
                        SG_VHASH_NULLFREE(pCtx, pvh_dir_contents);
                    }
                    else
                    {
                        SG_bool b_tid_exists = SG_FALSE;

                        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_manifest, psz_mv, &b_tid_exists)  );

                        if (b_tid_exists)
                        {
                            SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh_manifest, psz_mv)  );
                            SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_manifest, psz_path)  );

                            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "R %s %s\n", psz_mv, 2+psz_path)  );
                        }
                    }
                }

                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_op, "op", &psz_op)  );

                if (psz_op)
                {
                    if (0 == strcmp(psz_op, "mkdir"))
                    {
                        //SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "# mkdir %s\n", 2+psz_path)  );
                    }
                    else if (0 == strcmp(psz_op, "file"))
                    {
                        const char* psz_hid = NULL;
                        SG_int32 my_mark = -1;

                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_op, "hid", &psz_hid)  );

                        SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_blobid_to_mark, psz_hid, &my_mark)  );

                        // TODO 100755 for executables
                        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "M %s :%d %s\n", "100644", (int) my_mark, 2 + psz_path)  );
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_manifest, psz_path)  );
                    }
                    else if (0 == strcmp(psz_op, "symlink"))
                    {
                        const char* psz_hid = NULL;
                        SG_int32 my_mark = -1;

                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_op, "hid", &psz_hid)  );

                        SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_blobid_to_mark, psz_hid, &my_mark)  );

                        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "M %s :%d %s\n", "120000", (int) my_mark, 2 + psz_path)  );
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_manifest, psz_path)  );
                    }
                    else
                    {
                        // TODO throw
                    }
                }
            }
        }

        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "\n")  );

        if (count_branch_names > 1)
        {
            SG_uint32 i = 0;

            for (i=0; i<count_branch_names; i++)
            {
                const char* psz_branch = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_names_for_current_changeset, i, &psz_branch, NULL)  );
                if (0 != strcmp(psz_branch, psz_branch_name))
                {
                    SG_int32 mark = -1;

                    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_branch_name, psz_branch)  );
                    SG_ERR_CHECK( SG_string__replace_bytes(
                        pCtx,
                        pstr_branch_name,
                        (const SG_byte *)" ",1,
                        (const SG_byte *)"_",1,
                        SG_UINT32_MAX,SG_TRUE,
                        NULL
                        )  );
                    SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "reset refs/heads/%s\n", SG_string__sz(pstr_branch_name))  );
                    SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_csid_to_commit_mark, psz_csid_current, &mark)  );
                    SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "from :%d\n", mark)  );
                    SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "\n")  );
                    SG_STRING_NULLFREE(pCtx, pstr_branch_name);
                }
            }
        }

        SG_VHASH_NULLFREE(pCtx, pvh_cs_current);
        SG_VHASH_NULLFREE(pCtx, pvh_deltas);
        SG_VHASH_NULLFREE(pCtx, pvh_manifest);

        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }
    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    count_pop--;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_all_tags, &count_tags)  );
    for (i_tag = 0; i_tag < count_tags; i_tag++)
    {
        SG_vhash* pvh_tag = NULL;
        const char* psz_tag = NULL;
        const char* psz_csid = NULL;
        SG_int32 mark = -1;
        SG_int64 timestamp = 0;
        const char* psz_username = NULL;
        SG_vhash* pvh_a = NULL;
        const char* psz_userid = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_all_tags, i_tag, &pvh_tag)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tag, "tag", &psz_tag)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_tag, "csid", &psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh_csid_to_commit_mark, psz_csid, &mark)  );
		if (mark == 0)
		{
			// this tag indicates a commit that we're not exporting, skip it
			continue;
		}
        {
            SG_varray* pva_history = NULL;
            SG_vhash* pvh_history_1 = NULL;
            SG_varray* pva_audits = NULL;

            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_tag, SG_ZING_FIELD__HISTORY, &pva_history)  );
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_history, 0, &pvh_history_1)  );
            SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_history_1, "audits", &pva_audits)  );
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_audits, 0, &pvh_a)  );
        }
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_a, "userid", &psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_users, psz_userid, &psz_username)  );
        if (!psz_username)
        {
            psz_username = SG_NOBODY__USERNAME;
        }

        SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_a, "timestamp", &timestamp)  );

        // TODO should these tags be in refs/heads/etc?

        SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_branch_name, psz_tag)  );
        SG_ERR_CHECK( SG_string__replace_bytes(
            pCtx,
            pstr_branch_name,
            (const SG_byte *)" ",1,
            (const SG_byte *)"_",1,
            SG_UINT32_MAX,SG_TRUE,
            NULL
            )  );
        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "tag %s\n", SG_string__sz(pstr_branch_name))  );
        SG_STRING_NULLFREE(pCtx, pstr_branch_name);
        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "from :%d\n", mark)  );
        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "tagger <%s> %d +0000\n", psz_username, (SG_int32) (timestamp / 1000))  );
        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "data 0\n")  );
        SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "\n")  );
    }

    {
        SG_vhash* pvh_pile_closed = NULL;
        SG_uint32 i_branch = 0;
        SG_uint32 count_closed_branches = 0;

        SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_pile_closed)  );

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_pile_closed, &count_closed_branches)  );

        for (i_branch=0; i_branch<count_closed_branches; i_branch++)
        {
            const char* psz_name = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_pile_closed, i_branch, &psz_name, NULL)  );

            SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_branch_name, psz_name)  );
            SG_ERR_CHECK( SG_string__replace_bytes(
                pCtx,
                pstr_branch_name,
                (const SG_byte *)" ",1,
                (const SG_byte *)"_",1,
                SG_UINT32_MAX,SG_TRUE,
                NULL
                )  );
            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "reset refs/heads/%s\n", SG_string__sz(pstr_branch_name))  );
            SG_STRING_NULLFREE(pCtx, pstr_branch_name);
            SG_ERR_CHECK(  SG_file__write__format(pCtx, pfi, "\n")  );
        }
    }

fail:
    while (count_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_pop--;
    }
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
    SG_STRING_NULLFREE(pCtx, pstr_new_path);
    SG_VHASH_NULLFREE(pCtx, pvh_dir_contents);
    SG_ERR_IGNORE(  SG_file__close(pCtx, &pfi)  );
    SG_BLOBSET_NULLFREE(pCtx, pbs);
    SG_STRING_NULLFREE(pCtx, pstr_branch_name);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fi);
    SG_VHASH_NULLFREE(pCtx, pvh_branch_name_to_csid);
    SG_VHASH_NULLFREE(pCtx, pvh_cs_current);
    SG_VHASH_NULLFREE(pCtx, pvh_deltas);
    SG_VHASH_NULLFREE(pCtx, pvh_manifest);
    SG_VHASH_NULLFREE(pCtx, pvh_csid_to_commit_mark);
    SG_VHASH_NULLFREE(pCtx, pvh_blobid_to_mark);
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    SG_NULLFREE(pCtx, paszHids);
    SG_REPO_NULLFREE(pCtx, pRepo);

    SG_VHASH_NULLFREE(pCtx, pvh_branch_names);
    SG_VHASH_NULLFREE(pCtx, pvh_comments);
    SG_VHASH_NULLFREE(pCtx, pvh_audits);
    SG_VHASH_NULLFREE(pCtx, pvh_users);

    SG_VARRAY_NULLFREE(pCtx, pva_all_tags);
    SG_VARRAY_NULLFREE(pCtx, pva_all_audits);
    SG_VARRAY_NULLFREE(pCtx, pva_all_comments);
    SG_VARRAY_NULLFREE(pCtx, pva_all_users);
    SG_VHASH_NULLFREE(pCtx, pvh_paths);
}

/*

   filerename

   add comment lines with more info?

*/

