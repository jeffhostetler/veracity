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

/**
 *
 * @file sg_wc__locks.h
 *
 * @details Routines to perform most of 'vv locks'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void x_get_a_branch_head(
        SG_context* pCtx, 
        SG_vhash* pvh_pile,
        const char* psz_branch_name,
        SG_bool* pb_exists,
        char** ppsz_csid
        )
{
    SG_vhash* pvh_branches = NULL;
    SG_vhash* pvh_branch_info = NULL;
    SG_vhash* pvh_branch_values = NULL;
    SG_uint32 count_values = 0;
    const char* psz_val = NULL;
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branches, psz_branch_name, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branches, psz_branch_name, &pvh_branch_info)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_values)  );

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, 0, &psz_val, NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_val, ppsz_csid)  );

        *pb_exists = SG_TRUE;
    }
    else
    {
        *pb_exists = SG_FALSE;
    }

fail:
    ;
}

static void x_try_to_find_path(
        SG_context * pCtx, 
        SG_repo* pRepo,
        const char* psz_gid,
        const char* psz_csid,
        const char* psz_branch_name,
        SG_string** ppstr)
{
    SG_string* pstr = NULL;

    SG_UNUSED(psz_branch_name);

    SG_repo__treendx__get_path_in_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, psz_gid, psz_csid, &pstr, NULL);
    if (SG_context__err_equals(pCtx, SG_ERR_CHANGESET_BLOB_NOT_FOUND))
    {
        SG_ERR_DISCARD;
        // TODO look elsewhere
    }
    else
    {
		SG_ERR_CHECK_CURRENT;
        // TODO use it
    }

    *ppstr = pstr;
    pstr = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

//////////////////////////////////////////////////////////////////

/**
 * Formerly SG_vv_verbs__locks().
 * See SG_vv2__locks() for a better/pubilc interface.
 *
 */
void sg_vv2__locks__main(
        SG_context * pCtx,
		const char * pszRepoName,
		const char * pszBranchName,
		const char * psz_server,
        const char * psz_username,
        const char * psz_password,
		SG_bool b_pull,
		SG_bool bIncludeCompletedLocks,		// aka Verbose
		char ** ppsz_tied_branch_name,
		SG_vhash** ppvh_locks_by_branch,
		SG_vhash** ppvh_violated_locks,
		SG_vhash** ppvh_duplicate_locks
        )
{
	SG_string* pstr_repo_path = NULL;
    char* psz_repo_upstream = NULL;
    char* psz_csid_parent = NULL;
    char* psz_userid = NULL;
    SG_repo* pRepo = NULL;
    SG_audit q;
    SG_vhash* pvh_locks_by_branch = NULL;
    SG_vhash* pvh_duplicate_locks = NULL;
    SG_vhash* pvh_violated_locks = NULL;
    SG_vhash* pvh_pile = NULL;
    char* psz_csid = NULL;
	SG_stringarray * psa_completed_locks = NULL;
	const char* psz_hidrec_completed_lock = NULL;

	SG_NONEMPTYCHECK_RETURN( pszRepoName );		// See SG_vv2__locks() for public interface
	SG_NONEMPTYCHECK_RETURN( pszBranchName );	// See SG_vv2__locks() for public interface

    // now we need the current userid
    SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
    SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

    if (b_pull)
    {
        if (psz_server)
        {
            SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_server, &psz_repo_upstream)  );
        }
        else
        {
            SG_ERR_CHECK(  SG_localsettings__descriptor__get__sz(pCtx, pszRepoName, "paths/default", &psz_repo_upstream)  );
        }

        SG_ERR_CHECK(  SG_vc_locks__pull_locks(pCtx, psz_repo_upstream, psz_username, psz_password, pRepo)  );
    }

    SG_ERR_CHECK(  SG_vc_locks__check__okay_to_push(
                pCtx, 
                pRepo, 
                &pvh_pile,
                &pvh_locks_by_branch,
                &pvh_duplicate_locks,
                &pvh_violated_locks
                )  );

    if (pvh_duplicate_locks)
    {
        SG_uint32 count_branches_with_duplicate_locks = 0;
        SG_uint32 i_branch = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_duplicate_locks, &count_branches_with_duplicate_locks)  );
        for (i_branch=0; i_branch<count_branches_with_duplicate_locks; i_branch++)
        {
            const char* psz_branch = NULL;
            SG_vhash* pvh_gids_for_one_branch = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_duplicate_locks, i_branch, &psz_branch, &pvh_gids_for_one_branch)  );

            if (pvh_gids_for_one_branch)
            {
                SG_uint32 count_gids = 0;
                SG_uint32 i_gid = 0;

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
                for (i_gid=0; i_gid<count_gids; i_gid++)
                {
                    SG_vhash* pvh_locks_for_one_gid = NULL;
                    SG_uint32 count_locks = 0;
                    SG_uint32 i_lock = 0;
                    const char* psz_gid = NULL;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, &pvh_locks_for_one_gid)  );

                    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );
                    for (i_lock=0; i_lock<count_locks; i_lock++)
                    {
                        const char* psz_hidrec = NULL;
                        SG_vhash* pvh_lock = NULL;
                        SG_bool b_branch_exists = SG_FALSE;

                        SG_ERR_CHECK(  x_get_a_branch_head(pCtx, pvh_pile, psz_branch, &b_branch_exists, &psz_csid)  );
						if (b_branch_exists)
						{
							SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_hidrec, &pvh_lock)  );
							SG_ERR_CHECK(  x_try_to_find_path(pCtx, pRepo, psz_gid, psz_csid, psz_branch, &pstr_repo_path)  );
						}
                        SG_NULLFREE(pCtx, psz_csid);
						if (pstr_repo_path)
						{
							SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_lock, "path", SG_string__sz(pstr_repo_path))  );
						}
                        SG_STRING_NULLFREE(pCtx, pstr_repo_path);
                    }
                }
            }
        }
    }

    if (pvh_violated_locks)
    {
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_violated_locks, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz_hidrec = NULL;
            SG_vhash* pvh_lock = NULL;
            const char* psz_gid = NULL;
            SG_bool b_branch_exists = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_violated_locks, i, &psz_hidrec, &pvh_lock)  );
            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "gid", &psz_gid)  );

            SG_ERR_CHECK(  x_get_a_branch_head(pCtx, pvh_pile, pszBranchName, &b_branch_exists, &psz_csid)  );

			if (b_branch_exists)
			{
				SG_ERR_CHECK(  x_try_to_find_path(pCtx, pRepo, psz_gid, psz_csid, NULL, &pstr_repo_path)  );
			}

            SG_NULLFREE(pCtx, psz_csid);
            
			if (pstr_repo_path)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_lock, "path", SG_string__sz(pstr_repo_path))  );
                SG_STRING_NULLFREE(pCtx, pstr_repo_path);
            }
            else
            {
                // TODO what does this mean?  the item was deleted, right?
            }
        }
    }

    if (pvh_locks_by_branch)
    {
        SG_vhash* pvh_gids_for_one_branch = NULL;

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_locks_by_branch, pszBranchName, &pvh_gids_for_one_branch)  );
        if (pvh_gids_for_one_branch)
        {
            SG_uint32 count_gids = 0;
            SG_uint32 i_gid = 0;

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
            for (i_gid=0; i_gid<count_gids; i_gid++)
            {
                SG_vhash* pvh_locks_for_one_gid = NULL;
                SG_uint32 count_locks = 0;
                SG_uint32 i_lock = 0;
				SG_uint32 i_completed_lock = 0;
				SG_uint32 count_completed_locks = 0;
                const char* psz_gid = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, &pvh_locks_for_one_gid)  );

                SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );
                for (i_lock=0; i_lock<count_locks; i_lock++)
                {
                    const char* psz_hidrec = NULL;
                    SG_vhash* pvh_lock = NULL;
                    SG_bool b_branch_exists = SG_FALSE;

                    SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_hidrec, &pvh_lock)  );

                    SG_ERR_CHECK(  x_get_a_branch_head(pCtx, pvh_pile, pszBranchName, &b_branch_exists, &psz_csid)  );
					if (b_branch_exists)
					{
						SG_ERR_CHECK(  x_try_to_find_path(pCtx, pRepo, psz_gid, psz_csid, pszBranchName, &pstr_repo_path)  );
					}
                    SG_NULLFREE(pCtx, psz_csid);

                    if (pstr_repo_path)
                    {
                        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_lock, "path", SG_string__sz(pstr_repo_path))  );
                        SG_STRING_NULLFREE(pCtx, pstr_repo_path);
                    }

					if (!bIncludeCompletedLocks)
					{
						SG_bool bIsCompleted = SG_FALSE;
						//If they want us to filter out completed locks, we should skip it.

						SG_ERR_CHECK( SG_vc_locks__check_for_completed_lock(pCtx, pRepo, pvh_lock, &bIsCompleted)  );
						if (bIsCompleted)
						{
							if (psa_completed_locks == NULL)
								SG_ERR_CHECK( SG_STRINGARRAY__ALLOC(pCtx, &psa_completed_locks, count_locks)  );
							SG_ERR_CHECK( SG_stringarray__add(pCtx, psa_completed_locks, psz_hidrec)  );
						}
					}
                }
				if (psa_completed_locks != NULL)
				{
					SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_completed_locks, &count_completed_locks)  );
					for (i_completed_lock=0; i_completed_lock<count_completed_locks; i_completed_lock++)
					{
						SG_ERR_CHECK( SG_stringarray__get_nth(pCtx, psa_completed_locks, i_completed_lock, &psz_hidrec_completed_lock)  );
						SG_ERR_CHECK( SG_vhash__remove(pCtx, pvh_locks_for_one_gid, psz_hidrec_completed_lock)  );
					}
					SG_ERR_CHECK(  SG_STRINGARRAY_NULLFREE(pCtx, psa_completed_locks)  );
				}
            }
        }
    }

	if (ppsz_tied_branch_name != NULL)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszBranchName, ppsz_tied_branch_name)  );

	if (ppvh_locks_by_branch != NULL)
		SG_RETURN_AND_NULL(pvh_locks_by_branch, ppvh_locks_by_branch);
	if (ppvh_violated_locks != NULL)
		SG_RETURN_AND_NULL(pvh_violated_locks, ppvh_violated_locks);
	if (ppvh_duplicate_locks != NULL)
		SG_RETURN_AND_NULL(pvh_duplicate_locks, ppvh_duplicate_locks);

fail:
    SG_STRING_NULLFREE(pCtx, pstr_repo_path);
    SG_REPO_NULLFREE(pCtx, pRepo);
    SG_NULLFREE(pCtx, psz_repo_upstream);
	SG_NULLFREE(pCtx, psz_userid);
	SG_NULLFREE(pCtx, psz_csid_parent);
    SG_VHASH_NULLFREE(pCtx, pvh_locks_by_branch);
    SG_VHASH_NULLFREE(pCtx, pvh_violated_locks);
    SG_VHASH_NULLFREE(pCtx, pvh_duplicate_locks);
    SG_VHASH_NULLFREE(pCtx, pvh_pile);
    SG_NULLFREE(pCtx, psz_csid);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_completed_locks);
}
