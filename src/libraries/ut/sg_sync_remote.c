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

#define TRACE_SYNC_REMOTE 0

void SG_sync_remote__get_staging_path(SG_context* pCtx, const char* pszPushId, SG_pathname** ppStagingPathname)
{
	SG_NULLARGCHECK_RETURN(ppStagingPathname);
	SG_ERR_CHECK_RETURN(  SG_staging__get_pathname(pCtx, pszPushId, ppStagingPathname)  );
}

void SG_sync_remote__push_begin(
	SG_context* pCtx,
	const char** ppszPushId
	)
{
	char* psz_staging_area_name = NULL;

	SG_NULLARGCHECK_RETURN(ppszPushId);

	SG_ERR_CHECK(  SG_staging__create(pCtx, &psz_staging_area_name, NULL)  );

	SG_RETURN_AND_NULL(psz_staging_area_name, ppszPushId);

	/* fallthru */

fail:
	SG_NULLFREE(pCtx, psz_staging_area_name);
}

void SG_sync_remote__push_add(
	SG_context* pCtx,
	const char* pszPushId,
	SG_repo* pRepo,
	const char* psz_fragball_name,
	SG_vhash** ppResult
	)
{
	SG_staging* pStaging = NULL;
	SG_vhash* pvh_status = NULL;

#if TRACE_SYNC_REMOTE
	SG_int64 startTime;
	SG_int64 endTime;
	double seconds;
#endif

	SG_NULLARGCHECK_RETURN(pszPushId);
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  SG_staging__open(pCtx, pszPushId, &pStaging)  );

#if TRACE_SYNC_REMOTE
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &startTime)  );
#endif

	SG_ERR_CHECK(  SG_staging__slurp_fragball(pCtx, pStaging, psz_fragball_name)  );

#if TRACE_SYNC_REMOTE
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &endTime)  );
	seconds = ((double)endTime-(double)startTime)/1000;
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Fragball slurp took %1.3f seconds\n", seconds)  );
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &startTime)  );
#endif

	SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pRepo,
		SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, SG_FALSE,
		&pvh_status)  );

#if TRACE_SYNC_REMOTE
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &endTime)  );
	seconds = ((double)endTime-(double)startTime)/1000;
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Status check after fragball slurp took %1.3f seconds\n", seconds)  );
#endif

	*ppResult = pvh_status;
	pvh_status = NULL;

	/* fallthru */

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_status);
	SG_STAGING_NULLFREE(pCtx, pStaging);
}

void SG_sync_remote__push_remove(
	SG_context* pCtx,
	const char* pszPushId,
	SG_vhash* pvh_stuff_to_be_removed_from_staging_area,
	SG_vhash** ppResult
	);

void SG_sync_remote__push_commit(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* pszPushId,
	SG_vhash** ppResult
	)
{
	SG_staging* pStaging = NULL;
	SG_vhash* pvh_status = NULL;

	SG_NULLARGCHECK_RETURN(pszPushId);
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  SG_staging__open(pCtx, pszPushId, &pStaging)  );

	SG_ERR_CHECK(  SG_staging__commit(pCtx, pStaging, pRepo)  );

	SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pRepo,
		SG_TRUE, SG_FALSE, SG_TRUE, SG_TRUE, SG_FALSE,
		&pvh_status)  );

	*ppResult = pvh_status;
	pvh_status = NULL;

	/* fallthru */

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_status);
	SG_STAGING_NULLFREE(pCtx, pStaging);
}

void SG_sync_remote__push_end(
	SG_context* pCtx, 						 
	const char* pszPushId)
{
	SG_NULLARGCHECK_RETURN(pszPushId);

	SG_ERR_CHECK_RETURN(  SG_staging__cleanup__by_id(pCtx, pszPushId)  );
}

static void _copy_keys_into(
	SG_context* pCtx,
    SG_vhash* pvh_list,
    SG_vhash* pvh_blobs
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_list, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_list, i, &psz, NULL)  );
        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_blobs, psz)  );
    }

fail:
    ;
}

static void _add_necessary_blobs(
	SG_context* pCtx,
    SG_changeset* pcs,
    SG_vhash* pvh_blobs
    )
{
    SG_uint64 dagnum = 0;

    SG_ERR_CHECK(  SG_changeset__get_dagnum(pCtx, pcs, &dagnum)  );
    if (SG_DAGNUM__IS_DB(dagnum))
    {
        SG_vhash* pvh_changes = NULL;
        SG_uint32 count_parents = 0;
        SG_uint32 i_parent = 0;

        SG_ERR_CHECK(  SG_changeset__db__get_changes(pCtx, pcs, &pvh_changes)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
        for (i_parent=0; i_parent<count_parents; i_parent++)
        {
            SG_vhash* pvh_changes_for_one_parent = NULL;
            SG_vhash* pvh_add = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, NULL, &pvh_changes_for_one_parent)  );
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_for_one_parent, "add", &pvh_add)  );
            if (pvh_add)
            {
                SG_ERR_CHECK(  _copy_keys_into(pCtx, pvh_add, pvh_blobs)  );
            }

            pvh_add = NULL;
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_changes_for_one_parent, "attach_add", &pvh_add)  );
            if (pvh_add)
            {
                SG_ERR_CHECK(  _copy_keys_into(pCtx, pvh_add, pvh_blobs)  );
            }
        }
        if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
        {
            const char* psz_hid_template = NULL;

            SG_ERR_CHECK(  SG_changeset__db__get_template(pCtx, pcs, &psz_hid_template)  );

            SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_blobs, psz_hid_template)  );
        }
    }

    if (SG_DAGNUM__IS_TREE(dagnum))
    {
        SG_vhash* pvh_changes = NULL;
        const char* psz_root = NULL;
        SG_uint32 count_parents = 0;
        SG_uint32 i_parent = 0;

        SG_ERR_CHECK(  SG_changeset__tree__get_root(pCtx, pcs, &psz_root)  );
        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_blobs, psz_root)  );

        SG_ERR_CHECK(  SG_changeset__tree__get_changes(pCtx, pcs, &pvh_changes)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes, &count_parents)  );
        for (i_parent=0; i_parent<count_parents; i_parent++)
        {
            SG_vhash* pvh_changes_for_one_parent = NULL;
            SG_uint32 count_gids = 0;
            SG_uint32 i_gid = 0;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_changes, i_parent, NULL, &pvh_changes_for_one_parent)  );

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_changes_for_one_parent, &count_gids)  );
            for (i_gid=0; i_gid<count_gids; i_gid++)
            {
                const char* psz_hid = NULL;
                const SG_variant* pv = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_changes_for_one_parent, i_gid, NULL, &pv)  );
                if (SG_VARIANT_TYPE_VHASH == pv->type)
                {
                    SG_vhash* pvh_info = NULL;

                    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_info)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_info, SG_CHANGEESET__TREE__CHANGES__HID, &psz_hid)  );
                    if (psz_hid)
                    {
                        SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_blobs, psz_hid)  );
                    }
                }
            }
        }
    }

fail:
    ;
}

static void _do_since(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_vhash* pvh_since,
    SG_fragball_writer* pfb
    )
{
    SG_uint32 count_dagnums = 0;
    SG_uint32 i_dagnum = 0;
    SG_ihash* pih_new = NULL;
    SG_rbtree* prb = NULL;
    SG_vhash* pvh_blobs = NULL;
    SG_changeset* pcs = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(pvh_since);
    SG_NULLARGCHECK_RETURN(pfb);

    // TODO do we need to deal with dags which are present here but not in pvh_since?

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_since, &count_dagnums)  );
    for (i_dagnum=0; i_dagnum<count_dagnums; i_dagnum++)
    {
        const char* psz_dagnum = NULL;
        SG_varray* pva_nodes = NULL;
        SG_uint64 dagnum = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__varray(pCtx, pvh_since, i_dagnum, &psz_dagnum, &pva_nodes)  );

        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

        SG_ERR_CHECK(  SG_repo__find_new_dagnodes_since(pCtx, pRepo, dagnum, pva_nodes, &pih_new)  );

        if (pih_new)
        {
            SG_uint32 count = 0;

            SG_ERR_CHECK(  SG_ihash__count(pCtx, pih_new, &count)  );

            if (count)
            {
                SG_uint32 i = 0;

                SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &prb)  );
                SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_blobs)  );

                for (i=0; i<count; i++)
                {
                    const char* psz_node = NULL;

                    SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pih_new, i, &psz_node, NULL)  );
                    SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, psz_node)  );
                    SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_blobs, psz_node)  );
                    SG_ERR_CHECK(  SG_changeset__load_from_repo(pCtx, pRepo, psz_node, &pcs)  );
                    SG_ERR_CHECK(  _add_necessary_blobs(pCtx, pcs, pvh_blobs)  );
                    SG_CHANGESET_NULLFREE(pCtx, pcs);
                }

                // put all these new nodes in the frag
                SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pfb, dagnum, prb)  );

                // and the blobs
				SG_ERR_CHECK(  SG_sync__add_blobs_to_fragball(pCtx, pfb, pvh_blobs)  );

                SG_VHASH_NULLFREE(pCtx, pvh_blobs);
                SG_RBTREE_NULLFREE(pCtx, prb);
            }
            SG_IHASH_NULLFREE(pCtx, pih_new);
        }
    }

fail:
    SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_VHASH_NULLFREE(pCtx, pvh_blobs);
    SG_RBTREE_NULLFREE(pCtx, prb);
    SG_IHASH_NULLFREE(pCtx, pih_new);
}

void SG_sync_remote__request_fragball(
	SG_context* pCtx,
	SG_repo* pRepo,
	const SG_pathname* pFragballDirPathname,
	SG_vhash* pvhRequest,
	char** ppszFragballName)
{
	SG_pathname* pFragballPathname = NULL;
	SG_uint64* paDagNums = NULL;
	SG_string* pstrFragballName = NULL;
	SG_rbtree* prbDagnodes = NULL;
	SG_rbtree_iterator* pit = NULL;
	SG_rev_spec* pRevSpec = NULL;
	SG_stringarray* psaFullHids = NULL;
	SG_rbtree* prbDagnums = NULL;
	SG_dagfrag* pFrag = NULL;
	char* pszRepoId = NULL;
	char* pszAdminId = NULL;
    SG_fragball_writer* pfb = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pFragballDirPathname);

    {
        char buf_filename[SG_TID_MAX_BUFFER_LENGTH];
        SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_filename, sizeof(buf_filename))  );
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pFragballPathname, pFragballDirPathname, buf_filename)  );
    }

	if (!pvhRequest)
	{
		// Add leaves from every dag to the fragball.
		SG_uint32 count_dagnums;
		SG_uint32 i;

        SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pRepo, pFragballPathname, SG_TRUE, 2, &pfb)  );
		SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );

		for (i=0; i<count_dagnums; i++)
		{
			SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, paDagNums[i], &prbDagnodes)  );
			SG_ERR_CHECK(  SG_fragball__write__dagnodes(pCtx, pfb, paDagNums[i], prbDagnodes)  );
			SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
		}

		SG_ERR_CHECK(  SG_pathname__get_last(pCtx, pFragballPathname, &pstrFragballName)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pstrFragballName), ppszFragballName)  );
        SG_ERR_CHECK(  SG_fragball_writer__close(pCtx, pfb)  );
	}
	else
	{
		// Specific dags/nodes were requested. Build that fragball.
		SG_bool found;

#if TRACE_SYNC_REMOTE && 0
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhRequest, "fragball request")  );
#endif

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__CLONE, &found)  );
		if (found)
		{
            // SG_SYNC_STATUS_KEY__CLONE_REQUEST is currently ignored
            SG_ERR_CHECK(  SG_repo__fetch_repo__fragball(pCtx, pRepo, 3, pFragballDirPathname, ppszFragballName) );
		}
		else
		{
			// Not a full clone.

            SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pRepo, pFragballPathname, SG_TRUE, 2, &pfb)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__SINCE, &found)  );
			if (found)
			{
                SG_vhash* pvh_since = NULL;

                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__SINCE, &pvh_since)  );

                SG_ERR_CHECK(  _do_since(pCtx, pRepo, pvh_since, pfb)  );
            }

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__DAGS, &found)  );
			if (found)
			{
				// Specific Dagnodes were requested. Add just those nodes to our "start from" rbtree.

				SG_vhash* pvhDags;
				SG_uint32 count_requested_dagnums;
				SG_uint32 i;
				const SG_variant* pvRevSpecs = NULL;
				SG_vhash* pvhRevSpec = NULL;

				// For each requested dag, get rev spec request.
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__DAGS, &pvhDags)  );
				SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhDags, &count_requested_dagnums)  );
				if (count_requested_dagnums)
					SG_ERR_CHECK(  SG_repo__list_dags__rbtree(pCtx, pRepo, &prbDagnums)  );
				for (i=0; i<count_requested_dagnums; i++)
				{
					SG_bool isValidDagnum = SG_FALSE;
					SG_bool bSpecificNodesRequested = SG_FALSE;
					const char* pszRefDagNum = NULL;
					SG_uint64 iDagnum;

					// Get the dag's missing node vhash.
					SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhDags, i, &pszRefDagNum, &pvRevSpecs)  );

					// Verify that requested dagnum exists
					SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbDagnums, pszRefDagNum, &isValidDagnum, NULL)  );
					if (!isValidDagnum)
                        continue;

					SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, pszRefDagNum, &iDagnum)  );
					
					if (pvRevSpecs && pvRevSpecs->type != SG_VARIANT_TYPE_NULL)
					{
						SG_uint32 countRevSpecs = 0;
						
						SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pvRevSpecs, &pvhRevSpec)  );
						SG_ERR_CHECK(  SG_rev_spec__from_vash(pCtx, pvhRevSpec, &pRevSpec)  );

						// Process the rev spec for each dag
						SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
						if (countRevSpecs > 0)
						{
							bSpecificNodesRequested = SG_TRUE;

							SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &psaFullHids, NULL)  );
							SG_ERR_CHECK(  SG_stringarray__to_rbtree_keys(pCtx, psaFullHids, &prbDagnodes)  );
							SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
						}
						SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
					}

					if (!bSpecificNodesRequested)
					{
						// When no specific nodes are in the request, add all leaves.
						SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, iDagnum, &prbDagnodes)  );
					}

					if (prbDagnodes) // can be null when leaves of an empty dag are requested
					{
						// Get the leaves of the other repo, which we need to connect to.
						SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__LEAVES, &found)  );
						if (found)
						{
							SG_vhash* pvhRefAllLeaves;
							SG_vhash* pvhRefDagLeaves;
							SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__LEAVES, &pvhRefAllLeaves)  );
							SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRequest, pszRefDagNum, &found)  );
							{
								SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefAllLeaves, pszRefDagNum, &pvhRefDagLeaves)  );
								SG_ERR_CHECK(  SG_sync__build_best_guess_dagfrag(pCtx, pRepo, iDagnum, 
									prbDagnodes, pvhRefDagLeaves, &pFrag)  );
							}
						}
						else
						{
							// The other repo's leaves weren't provided: add just the requested nodes, make no attempt to connect.
							SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &pszRepoId)  );
							SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &pszAdminId)  );
							SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx, &pFrag, pszRepoId, pszAdminId, iDagnum)  );
							SG_ERR_CHECK(  SG_dagfrag__load_from_repo__simple(pCtx, pFrag, pRepo, prbDagnodes)  );
							SG_NULLFREE(pCtx, pszRepoId);
							SG_NULLFREE(pCtx, pszAdminId);
						}

						SG_ERR_CHECK(  SG_fragball__write__frag(pCtx, pfb, pFrag)  );
						
						SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
						SG_DAGFRAG_NULLFREE(pCtx, pFrag);
					}

				} // dagnum loop
			} // if "dags" exists

			/* Add requested blobs to the fragball */
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__BLOBS, &found)  );
			if (found)
			{
				// Blobs were requested.
				SG_vhash* pvhBlobs;
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRequest, SG_SYNC_STATUS_KEY__BLOBS, &pvhBlobs)  );
				SG_ERR_CHECK(  SG_sync__add_blobs_to_fragball(pCtx, pfb, pvhBlobs)  );
			}

			SG_ERR_CHECK(  SG_pathname__get_last(pCtx, pFragballPathname, &pstrFragballName)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pstrFragballName), ppszFragballName)  );
		}
        SG_ERR_CHECK(  SG_fragball_writer__close(pCtx, pfb)  );
	}

	/* fallthru */
fail:
	// If we had an error, delete the half-baked fragball.
	if (pFragballPathname && SG_context__has_err(pCtx))
    {
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pFragballPathname)  );
    }

	SG_PATHNAME_NULLFREE(pCtx, pFragballPathname);
	SG_NULLFREE(pCtx, paDagNums);
	SG_STRING_NULLFREE(pCtx, pstrFragballName);
	SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	SG_RBTREE_NULLFREE(pCtx, prbDagnums);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
	SG_NULLFREE(pCtx, pszRepoId);
	SG_NULLFREE(pCtx, pszAdminId);
    SG_FRAGBALL_WRITER_NULLFREE(pCtx, pfb);
}

void SG_sync_remote__get_repo_info(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_bool bIncludeBranches,
    SG_bool b_include_areas,
    SG_vhash** ppvh)
{
    SG_vhash* pvh = NULL;
	char* pszRepoId = NULL;
	char* pszAdminId = NULL;
	char* pszHashMethod = NULL;
	SG_uint32  count_dagnums = 0;
	SG_uint64* paDagNums     = NULL;
    SG_uint32 i = 0;
    SG_vhash* pvh_dags = NULL;
    SG_vhash* pvh_areas = NULL;
	SG_vhash* pvhBranchPile = NULL;

	SG_bool bHasBranchDag = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pRepo);

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	/* Protocol version */
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__PROTOCOL_VERSION, 1)  );


	/* Basic repository info */
	SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &pszRepoId)  );
    SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &pszAdminId)  );
    SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pRepo, &pszHashMethod)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__REPO_ID, pszRepoId)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, pszAdminId)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, pszHashMethod)  );

	/* All DAGs in the repository */
	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );
    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "dags", &pvh_dags)  );
    for (i=0; i<count_dagnums; i++)
    {
        char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

        SG_ERR_CHECK_RETURN(  SG_dagnum__to_sz__hex(pCtx, paDagNums[i], buf_dagnum, sizeof(buf_dagnum))  );
        SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_dags, buf_dagnum)  );

		/* Asking for a DAG for the first time in a repo will create that DAG.
		 * When pushing into an empty repo, we don't want this initial query to create 
		 * empty new DAGs, so we make sure they exist before we query them. */
		if (paDagNums[i] == SG_DAGNUM__VC_BRANCHES)
			bHasBranchDag = SG_TRUE;
    }
    
    // TODO the following code is a problem, because it requires that this repo
    // instance have indexes, and we would prefer to preserve the ability of
    // an index-free instance to support push, pull, and clone.

	/* All areas in the repository */
    if (b_include_areas)
    {
        SG_ERR_CHECK(  SG_area__list(pCtx, pRepo, &pvh_areas)  );
        if (pvh_areas)
        {
            SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, "areas", &pvh_areas)  );
        }
    }

	/* Branches */
	if (bIncludeBranches && bHasBranchDag)
	{
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhBranchPile)  );
		if (pvhBranchPile)
		{
			SG_bool bHasBranches;

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhBranchPile, "branches", &bHasBranches)  );
			if (bHasBranches)
				SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, "branches", &pvhBranchPile)  );
		}
	}

    *ppvh = pvh;
    pvh = NULL;

	/* fall through */
fail:
	SG_NULLFREE(pCtx, paDagNums);
    SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, pszRepoId);
	SG_NULLFREE(pCtx, pszAdminId);
	SG_NULLFREE(pCtx, pszHashMethod);
	SG_VHASH_NULLFREE(pCtx, pvh_areas);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
}

void SG_sync_remote__heartbeat(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_vhash** ppvh)
{
	SG_vhash* pvh = NULL;
	char* pszRepoId = NULL;
	char* pszAdminId = NULL;
	char* pszHashMethod = NULL;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
	
	SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &pszRepoId)  );
	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &pszAdminId)  );
	SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pRepo, &pszHashMethod)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__REPO_ID, pszRepoId)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, pszAdminId)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, pszHashMethod)  );

	*ppvh = pvh;
	pvh = NULL;

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_NULLFREE(pCtx, pszRepoId);
	SG_NULLFREE(pCtx, pszAdminId);
	SG_NULLFREE(pCtx, pszHashMethod);

}

void SG_sync_remote__get_dagnode_info(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo)
{
	char bufDagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_vhash* pvhRefVersionControlHids = NULL;
	SG_varray* pvaHids = NULL;
	SG_stringarray* psaHids = NULL;

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__VERSION_CONTROL, bufDagnum, sizeof(bufDagnum))  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRequest, bufDagnum, &pvhRefVersionControlHids)  );

	// Ugh.  Is vhash->varray->stringarray the best option?
	SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvhRefVersionControlHids, &pvaHids)  );
	SG_ERR_CHECK(  SG_varray__to_stringarray(pCtx, pvaHids, &psaHids)  );

	SG_ERR_CHECK(  SG_history__get_revision_details(pCtx, pRepo, psaHids, NULL, ppInfo)  );

	/* fall through */
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaHids);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHids);
}

static void _remote_clone_allowed(SG_context* pCtx)
{
	char* pszSetting = NULL;
	SG_bool bAllowed = SG_TRUE;

	SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__SERVER_CLONE_ALLOWED, NULL, &pszSetting, NULL);
	if(!SG_context__has_err(pCtx) && pszSetting != NULL)
		bAllowed = (strcmp(pszSetting, "true")==0);
	if(SG_context__has_err(pCtx))
	{
		SG_log__report_error__current_error(pCtx);
		SG_context__err_reset(pCtx);
	}

	if (!bAllowed)
		SG_ERR_THROW(SG_ERR_SERVER_DISALLOWED_REPO_CREATE_OR_DELETE);

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszSetting);
}

void SG_sync_remote__push_clone__begin(
	SG_context* pCtx,
	const char* psz_repo_descriptor_name,
	const SG_vhash* pvhRepoInfo,
	const char** ppszCloneId)
{
	const char* psz_repo_id = NULL;
	const char* psz_admin_id = NULL;
	const char* psz_hash_method = NULL;
	const char* pszRefValidatedName = NULL;
	SG_vhash* pvhFullDescriptor = NULL; // pNewRepo owns this. Don't free.

	SG_vhash* pvhDescriptorPartial = NULL;
	SG_closet_descriptor_handle* ph = NULL;
	SG_repo* pNewRepo = NULL;
	const char* pszRefCloneId = NULL;

	SG_NULLARGCHECK_RETURN(ppszCloneId);

	SG_ERR_CHECK_RETURN(  _remote_clone_allowed(pCtx)  );

	/* We'll create a descriptor and empty repo immediately, so that we validate and claim the name
	 * before a potentially long upload. */

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhRepoInfo, SG_SYNC_REPO_INFO_KEY__REPO_ID, &psz_repo_id)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhRepoInfo, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &psz_admin_id)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvhRepoInfo, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, &psz_hash_method)  );

	SG_ERR_CHECK(  SG_closet__descriptors__add_begin(pCtx, 
		psz_repo_descriptor_name, 
		NULL, 
		psz_repo_id, 
		psz_admin_id, 
		&pszRefValidatedName, &pvhDescriptorPartial, &ph)  );

	SG_ERR_CHECK(  SG_repo__create_repo_instance(pCtx,
		pszRefValidatedName,
		pvhDescriptorPartial,
		SG_TRUE,
		psz_hash_method,
		psz_repo_id,
		psz_admin_id,
		&pNewRepo)  );
	SG_VHASH_NULLFREE(pCtx, pvhDescriptorPartial);

	SG_ERR_CHECK_RETURN(  SG_staging__clone__create(pCtx, psz_repo_descriptor_name, pvhRepoInfo, &pszRefCloneId)  );

	/* We temporarily add the clone ID to the repo descriptor so that we can verify 
	 * we're committing to the correct repo later, when the fragball's been uploaded. */
	SG_ERR_CHECK(  SG_repo__get_descriptor__ref(pCtx, pNewRepo, &pvhFullDescriptor)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhFullDescriptor, SG_SYNC_DESCRIPTOR_KEY__CLONE_ID, pszRefCloneId)  );

	SG_ERR_CHECK(  SG_closet__descriptors__add_commit(pCtx, &ph, pvhFullDescriptor, SG_REPO_STATUS__CLONING)  );

	SG_REPO_NULLFREE(pCtx, pNewRepo);

	*ppszCloneId = pszRefCloneId;

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhDescriptorPartial);
	SG_REPO_NULLFREE(pCtx, pNewRepo);
	SG_ERR_IGNORE(  SG_closet__descriptors__add_abort(pCtx, &ph)  );
}

void SG_sync_remote__push_clone__commit(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_pathname** ppPathFragball)
{
	SG_ERR_CHECK_RETURN(  _remote_clone_allowed(pCtx)  );
	SG_ERR_CHECK_RETURN(  SG_staging__clone__commit(pCtx, pszCloneId, ppPathFragball)  );
}

void SG_sync_remote__push_clone__commit_maybe_usermap(
	SG_context* pCtx,
	const char* pszCloneId,
	const char* pszClosetAdminId,
	SG_pathname** ppPathFragball,	/*< [in] Required. On success, this routine takes ownership of the pointer. It will free memory and delete the file. */
	char** ppszDescriptorName,		/*< [out] Optional. Caller should free. */
	SG_bool* pbAvailable,			/*< [out] Optional. */
	SG_closet__repo_status* pStatus,/*< [out] Optional. */
	char** ppszStatus				/*< [out] Optional. Caller should free. */
)
{
	SG_ERR_CHECK_RETURN(  _remote_clone_allowed(pCtx)  );
	SG_ERR_CHECK_RETURN(  SG_staging__clone__commit_maybe_usermap(pCtx, pszCloneId, pszClosetAdminId, ppPathFragball,
		ppszDescriptorName, pbAvailable, pStatus, ppszStatus)  );
}
