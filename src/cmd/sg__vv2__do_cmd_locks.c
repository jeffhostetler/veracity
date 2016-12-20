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
 * @file sg__vv2__do_cmd_locks.c
 *
 * @details Handle the 'vv locks' command.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_vv2__public_typedefs.h>

#include <sg_wc__public_prototypes.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_locks(SG_context * pCtx, 
					   SG_option_state * pOptSt,
					   const char * psz_username_for_pull,
					   const char * psz_password_for_pull)
{
	char* psz_tied_branch_name = NULL;
	SG_vhash* pvh_locks_by_branch = NULL;
	SG_vhash* pvh_violated_locks = NULL;
	SG_vhash* pvh_duplicate_locks = NULL;
	SG_string * pstr_end_csid = NULL;
	const char * pszBranchName_0 = NULL;

	if (pOptSt->psa_branch_names)
		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pOptSt->psa_branch_names, 0, &pszBranchName_0)  );
	
	SG_ERR_CHECK(  SG_vv2__locks(pCtx, pOptSt->psz_repo, pszBranchName_0,
								 pOptSt->psz_server, psz_username_for_pull, psz_password_for_pull, pOptSt->bPull,
								 pOptSt->bVerbose,
								 &psz_tied_branch_name,
								 &pvh_locks_by_branch, &pvh_violated_locks, &pvh_duplicate_locks)  );

	if (pvh_locks_by_branch)
    {
        SG_vhash* pvh_gids_for_one_branch = NULL;

        SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_locks_by_branch, psz_tied_branch_name, &pvh_gids_for_one_branch)  );
        if (pvh_gids_for_one_branch)
        {
            SG_uint32 count_gids = 0;
            SG_uint32 i_gid = 0;

            SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Locks on this branch (%s):\n", psz_tied_branch_name)  );
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
                    const char* psz_hid_lock = NULL;
                    SG_vhash* pvh_lock = NULL;
                    const char* psz_username = NULL;
                    const char* psz_path = NULL;
					const char* psz_end_csid = NULL;
                    SG_bool b_violated = SG_FALSE;
					SG_bool bWasChecked = SG_FALSE;
					SG_bool bIsCompleted = SG_TRUE; //Assume that the locks are complete.
                    
                    SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_hid_lock, &pvh_lock)  );

                    if (pvh_violated_locks)
                    {
                        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_violated_locks, psz_hid_lock, &b_violated)  );
                    }

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "username", &psz_username)  );
                    SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_lock, "path", &psz_path)  );
					SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_lock, "end_csid", &psz_end_csid)  );
					if (psz_end_csid != NULL)
					{
						SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pstr_end_csid, " -- waiting for changeset %.10s to be pulled", psz_end_csid)  );
						SG_ERR_CHECK( SG_vhash__has(pCtx, pvh_lock, "is_completed", &bWasChecked)  );
						if (bWasChecked == SG_TRUE)
						{
							SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_lock, "is_completed", &bIsCompleted)  );
						}
					}
                    if (psz_path)
                    {
                        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%8s %s: %s%s\n", (b_violated ? "VIOLATED" : ""), psz_username, psz_path, (psz_end_csid != NULL && bIsCompleted == SG_FALSE ? SG_string__sz(pstr_end_csid) : ""))  );
                    }
                    else
                    {
						// TODO 2012/10/10 If we get here does it mean that we received
						// TODO            a lock record (from the locks dag) for an
						// TODO            item that does not yet exist in our VC tree?
                        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%8s %s: (object %s)\n", (b_violated ? "VIOLATED" : ""), psz_username, psz_gid)  );
                    }
					SG_STRING_NULLFREE(pCtx, pstr_end_csid);
                }
            }
        }
#if defined(DEBUG)
		if (pOptSt->bVerbose)
		{
			SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_locks_by_branch, "Locks")  );
			if (pvh_violated_locks)
				SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_violated_locks, "Violated Locks")  );
			if (pvh_duplicate_locks)
				SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_duplicate_locks, "Duplicate Locks")  );
		}
#endif
    }

fail:
	SG_STRING_NULLFREE(pCtx, pstr_end_csid);
	SG_NULLFREE(pCtx, psz_tied_branch_name);
	SG_VHASH_NULLFREE(pCtx, pvh_locks_by_branch);
	SG_VHASH_NULLFREE(pCtx, pvh_duplicate_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_violated_locks);
}
