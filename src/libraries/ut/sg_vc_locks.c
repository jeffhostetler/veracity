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

static void x_get_hid(
        SG_context* pCtx,
        SG_tncache* ptn_cache,
        SG_treenode* ptn,
        SG_varray* pva,
        const char* psz_gid,
        char** ppsz_hid
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    const char* psz_result = NULL;
    SG_treenode_entry* ptne = NULL;
    char* psz_found_gid = NULL;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count )  );
    for (i = 0; i < count; i++)
    {
        const char* psz_path = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz_path)  );
        SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path__cached(pCtx, ptn_cache, ptn, 1+psz_path, &psz_found_gid, &ptne)  );
        if (
                ptne
                && (0 == strcmp(psz_gid, psz_found_gid))
           )
        {
            SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne, &psz_result)  );
            break;
        }
        SG_NULLFREE(pCtx, psz_found_gid);
        SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
    }

    if (psz_result)
    {
        SG_ERR_CHECK(  SG_strdup(pCtx, psz_result, ppsz_hid)  );
        psz_result = NULL;
    }

fail:
    SG_NULLFREE(pCtx, psz_found_gid);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
}

void sg_vc_locks__check_for_duplicates(SG_context * pCtx, SG_vhash * pvh_locks_for_one_gid, SG_bool * bHasDuplicates)
{
	SG_uint32 count_locks = 0;
	SG_uint32 i = 0;
	SG_uint32 openLockCount = 0;

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );
	if (count_locks <= 1)
	{
		*bHasDuplicates = SG_FALSE;
		return;
	}

	for ( i=0; i < count_locks; i++)
	{
		SG_bool bIsCompleted = SG_FALSE;
		SG_bool bWasChecked = SG_TRUE;
		SG_vhash * pvh_record = NULL;
		const char * psz_end_csid = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i, NULL, &pvh_record)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_record, "end_csid", &psz_end_csid)  );
	
    	SG_ERR_CHECK( SG_vhash__has(pCtx, pvh_record, "is_completed", &bWasChecked)  );
		if (bWasChecked == SG_TRUE)
		{
			SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_record, "is_completed", &bIsCompleted)  );
			//Test to see if the lock is still open
			if (psz_end_csid == NULL && bIsCompleted == SG_FALSE)
			{
				openLockCount++;
				if (openLockCount > 1)
				{
					*bHasDuplicates = SG_TRUE;
					return;
				}
			}
		}
	}
	//openLockCount never exceeded 1.
	*bHasDuplicates = SG_FALSE;
fail:
	return;
	
}

void sg_vc_locks__decorate_completed_locks(SG_context * pCtx, SG_repo * pRepo, SG_varray * pva_lock_records)
{
	SG_uint32 count_records = 0, i=0;
	SG_bool bIsCompleted = SG_FALSE;
	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_lock_records, &count_records)  );
	for (i=0; i<count_records; i++)
	{
		SG_vhash* pvh_record = NULL;
		
		bIsCompleted = SG_FALSE;
		
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_lock_records, i, &pvh_record)  );
		SG_ERR_CHECK(  SG_vc_locks__check_for_completed_lock(pCtx, pRepo, pvh_record, &bIsCompleted)  );
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_record, "is_completed", bIsCompleted)  );
		
	}
fail:
	return;
}

void SG_vc_locks__check_for_completed_lock(SG_context * pCtx, SG_repo * pRepo, SG_vhash * pvh_lock, SG_bool * pbIsCompleted)
{
	const char* psz_end_csid = NULL;
	SG_dagnode* pdn_end_csid = NULL;

	SG_NULLARGCHECK_RETURN(pbIsCompleted);
	*pbIsCompleted = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_lock, "end_csid", &psz_end_csid)  );
	
    if (psz_end_csid)
    {
        SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, psz_end_csid, &pdn_end_csid);
        if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
        {
            SG_ERR_DISCARD;
			*pbIsCompleted = SG_FALSE;
        }
        else
        {
            SG_ERR_CHECK_CURRENT;
            SG_DAGNODE_NULLFREE(pCtx, pdn_end_csid);
			*pbIsCompleted = SG_TRUE;
        }
    }
fail:
	SG_DAGNODE_NULLFREE(pCtx, pdn_end_csid);
}

static void sg_vc_locks__add(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	const char* psz_csid,
	const char* psz_branch,
	SG_vhash* pvh_gids,
	const SG_audit* pq
	)
{
	char* psz_hid_cs_leaf = NULL;
	SG_zingtx* pztx = NULL;
	SG_zingrecord* prec = NULL;
	SG_dagnode* pdn = NULL;
	SG_changeset* pcs = NULL;
	SG_zingtemplate* pzt = NULL;
	SG_zingfieldattributes* pzfa = NULL;
	SG_uint32 count_gids = 0;
	SG_uint32 i_gid = 0;
    SG_tncache* ptnc = NULL;
    SG_vhash* pvh_paths = NULL;
    char* psz_hid_root_tn = NULL;
    SG_treenode* ptn_root = NULL;
    char* psz_start_hid = NULL;

    SG_ERR_CHECK(  SG_repo__treendx__get_paths(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pvh_gids, &pvh_paths)  );
    SG_ERR_CHECK(  SG_tncache__alloc(pCtx, pRepo, &ptnc)  );

    SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, psz_csid, &psz_hid_root_tn)  );
    SG_ERR_CHECK(  SG_tncache__load(pCtx, ptnc, psz_hid_root_tn, &ptn_root)  );

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_LOCKS, &psz_hid_cs_leaf)  );

	/* start a changeset */
	SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_LOCKS, pq->who_szUserId, psz_hid_cs_leaf, &pztx)  );
	SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
	SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids, &count_gids)  );

	for (i_gid=0; i_gid<count_gids; i_gid++)
	{
		const char* psz_gid = NULL;
        SG_varray* pva_paths = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_gids, i_gid, &psz_gid, NULL)  );

		SG_ERR_CHECK(  SG_zingtx__create_new_record(pCtx, pztx, "lock", &prec)  );

        SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvh_paths, psz_gid, &pva_paths)  );
        SG_ERR_CHECK(  x_get_hid(pCtx, ptnc, ptn_root, pva_paths, psz_gid, &psz_start_hid)  );
        SG_ASSERT(psz_start_hid);
		SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "lock", "start_hid", &pzfa)  );
		SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_start_hid) );
        SG_NULLFREE(pCtx, psz_start_hid);

		SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "lock", "branch", &pzfa)  );
		SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_branch) );
		SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "lock", "gid", &pzfa)  );
		SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_gid) );
		SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "lock", "userid", &pzfa)  );
		SG_ERR_CHECK(  SG_zingrecord__set_field__userid(pCtx, prec, pzfa, pq->who_szUserId) );
	}

	/* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

	// fall thru

fail:
	if (pztx)
	{
		SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
	}
    SG_NULLFREE(pCtx, psz_start_hid);
	SG_NULLFREE(pCtx, psz_hid_root_tn);
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_CHANGESET_NULLFREE(pCtx, pcs);
    SG_TNCACHE_NULLFREE(pCtx, ptnc);
    SG_VHASH_NULLFREE(pCtx, pvh_paths);
}

static void sg_vc_locks__delete(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	SG_vhash* pvh_recids,
	const SG_audit* pq
	)
{
	SG_zingtx* pztx = NULL;
	SG_dagnode* pdn = NULL;
	SG_changeset* pcs = NULL;
	char* psz_hid_cs_leaf = NULL;
	SG_uint32 count = 0;
	SG_uint32 i = 0;

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_LOCKS, &psz_hid_cs_leaf)  );

	SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_LOCKS, pq->who_szUserId, psz_hid_cs_leaf, &pztx)  );
	SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_recids, &count)  );
	for (i=0; i<count; i++)
	{
		const char* psz_recid = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_recids, i, &psz_recid, NULL)  );
		SG_ERR_CHECK(  SG_zingtx__delete_record(pCtx, pztx, "lock", psz_recid)  );
	}

	/* commit the changes */
	SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );

	// fall thru

fail:
	if (pztx)
	{
		SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
	}
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_CHANGESET_NULLFREE(pCtx, pcs);
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

static void sg_vc_locks__list_all(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	SG_varray** ppva
	)
{
	char* psz_hid_cs_leaf = NULL;
	SG_varray* pva_fields = NULL;
	SG_varray* pva = NULL;
	SG_vhash* pvh_username_join = NULL;

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_LOCKS, &psz_hid_cs_leaf)  );

	SG_ASSERT(psz_hid_cs_leaf);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "branch")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "gid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "userid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "start_hid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "end_csid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_fields, &pvh_username_join)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_username_join, "type", "username")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_username_join, "userid", SG_AUDIT__USERID)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_username_join, "alias", SG_AUDIT__USERNAME)  );

	SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, SG_DAGNUM__VC_LOCKS, psz_hid_cs_leaf, NULL, NULL, NULL, 0, 0, pva_fields, &pva)  );

	SG_ERR_CHECK(  sg_vc_locks__decorate_completed_locks(pCtx, pRepo, pva)  );
	*ppva = pva;
	pva = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

static void sg_vc_locks__by_branch(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_vhash** ppvh_by_branch
	)
{
	SG_varray* pva_locks = NULL;
	SG_vhash* pvh_by_branch = NULL;
	SG_uint32 count_locks = 0;

	// ----------------------------------------------------------------
	// build a list of all the locks, organized by branch
	// ----------------------------------------------------------------

	SG_ERR_CHECK(  sg_vc_locks__list_all(pCtx, pRepo, &pva_locks)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_locks, &count_locks)  );

	if (count_locks > 0)
	{
		SG_uint32 i_lock = 0;

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_by_branch)  );
		for (i_lock=0; i_lock<count_locks; i_lock++)
		{
			SG_vhash* pvh_record = NULL;
			const char* psz_branch = NULL;
			const char* psz_gid = NULL;
			const char* psz_recid = NULL;
			SG_vhash* pvh_gids_for_one_branch = NULL;
			SG_vhash* pvh_locks_for_one_gid = NULL;

			// grab data for this record
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_locks, i_lock, &pvh_record)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, "branch", &psz_branch)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, "gid", &psz_gid)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, SG_ZING_FIELD__RECID, &psz_recid)  );

			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_by_branch, psz_branch, &pvh_gids_for_one_branch)  );
			if (!pvh_gids_for_one_branch)
			{
				SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_by_branch, psz_branch, &pvh_gids_for_one_branch)  );
			}

			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_gids_for_one_branch, psz_gid, &pvh_locks_for_one_gid)  );
			if (!pvh_locks_for_one_gid)
			{
				SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_gids_for_one_branch, psz_gid, &pvh_locks_for_one_gid)  );
			}

			{
				SG_vhash* pvh_copy = NULL;

				SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_locks_for_one_gid, psz_recid, &pvh_copy)  );
				SG_ERR_CHECK(  SG_vhash__copy_some_items(
					pCtx, 
					pvh_record, 
					pvh_copy,
                    SG_ZING_FIELD__RECID,
					"username",
					"userid",
					"start_hid",
					"end_csid",
					"is_completed",
					NULL
					)  );
			}
		}
	}

	if (ppvh_by_branch)
	{
		*ppvh_by_branch = pvh_by_branch;
		pvh_by_branch = NULL;
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pva_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_by_branch);
}


static void _check_changeset_for_lock_violation(
	SG_context* pCtx, 
	SG_repo * pRepo,
	const char * psz_gid,
	const char * psz_hid_in_changeset,
	const char * psz_branch,
	SG_vhash * pvh_locks_for_one_gid,
	SG_vhash ** ppvh_violated_locks)
{
	SG_uint32 nOpenOrWaitingLocks = 0;
	SG_uint32 nViolationCount = 0;

	SG_uint32 count_locks = 0;
	SG_uint32 i_lock = 0;
	const char* psz_recid_lock = NULL;
	SG_vhash * pvh_last_violating_lock = NULL;

	SG_bool bIsViolating = SG_FALSE;
	
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );

	for (i_lock=0; i_lock<count_locks; i_lock++)
	{
		SG_vhash* pvh_lock = NULL;
		const char* psz_start_hid = NULL;
		const char* psz_lock_userid = NULL;
		SG_bool bIsCompleted = SG_FALSE;
		SG_bool bWasChecked = SG_FALSE;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_recid_lock, &pvh_lock)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "start_hid", &psz_start_hid)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "userid", &psz_lock_userid)  );

				

		SG_ERR_CHECK( SG_vhash__has(pCtx, pvh_lock, "is_completed", &bWasChecked)  );
		if (bWasChecked == SG_TRUE)
		{
			SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_lock, "is_completed", &bIsCompleted)  );
		}
		else
			SG_ERR_CHECK( SG_vc_locks__check_for_completed_lock(pCtx, pRepo, pvh_lock, &bIsCompleted)  );
        if (bIsCompleted == SG_FALSE)
        {
			nOpenOrWaitingLocks++;
			if (0 != SG_strcmp__null(psz_hid_in_changeset, psz_start_hid))
			{
				nViolationCount++;
				pvh_last_violating_lock = pvh_lock;
			}
        }
	}
	
	if (nOpenOrWaitingLocks == 0)
		bIsViolating =  SG_FALSE;
	else if (nViolationCount == nOpenOrWaitingLocks) //They don't have a hid that matches the starting hid of ANY open or waiting locks.
		bIsViolating = SG_TRUE;

	if (bIsViolating == SG_TRUE && pvh_last_violating_lock != NULL)
	{
		if (!(*ppvh_violated_locks))
		{
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, ppvh_violated_locks)  );
		}

		{
			SG_vhash* pvh_copy = NULL;

			SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, *ppvh_violated_locks, psz_recid_lock, &pvh_copy)  );
			SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pvh_last_violating_lock, pvh_copy)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_copy, "gid", psz_gid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_copy, "branch", psz_branch)  );
		}
	}

fail:
	return;
}

static void sg_vc_locks__check_for_problems(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_vhash* pvh_pile,
	SG_vhash* pvh_locks_by_branch,
	SG_vhash** ppvh_duplicate_locks,
	SG_vhash** ppvh_violated_locks
	)
{
	SG_vhash* pvh_branches = NULL;
	SG_uint32 count_branches_with_locks = 0;
	SG_uint32 i_branch = 0;
	SG_vhash* pvh_duplicate_locks = NULL;
	SG_vhash* pvh_violated_locks = NULL;
    SG_tncache* ptnc = NULL;
    SG_vhash* pvh_paths = NULL;
    SG_vhash* pvh_gids = NULL;
    char* psz_hid_root_tn = NULL;
    SG_treenode* ptn_root = NULL;
    char* psz_this_hid = NULL;

	SG_NULLARGCHECK_RETURN(pvh_locks_by_branch);
	SG_NULLARGCHECK_RETURN(ppvh_duplicate_locks);
	SG_NULLARGCHECK_RETURN(ppvh_violated_locks);

    SG_ERR_CHECK(  SG_tncache__alloc(pCtx, pRepo, &ptnc)  );

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );

	// ----------------------------------------------------------------
	// look for locks that have been violated
	// ----------------------------------------------------------------

	//SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvh_locks_by_branch, "Locks by branch")  );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_by_branch, &count_branches_with_locks)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_gids)  );
	for (i_branch=0; i_branch<count_branches_with_locks; i_branch++)
	{
		const char* psz_branch = NULL;
		SG_vhash* pvh_gids_for_one_branch = NULL;
		SG_uint32 count_gids = 0;
		SG_uint32 i_gid = 0;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_by_branch, i_branch, &psz_branch, &pvh_gids_for_one_branch)  );

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
		for (i_gid=0; i_gid<count_gids; i_gid++)
		{
			const char* psz_gid = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, NULL)  );

            SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_gids, psz_gid)  );
        }
    }

    SG_ERR_CHECK(  SG_repo__treendx__get_paths(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pvh_gids, &pvh_paths)  );

	for (i_branch=0; i_branch<count_branches_with_locks; i_branch++)
	{
		const char* psz_branch = NULL;
		SG_vhash* pvh_gids_for_one_branch = NULL;
		SG_vhash* pvh_branch_info = NULL;
		SG_vhash* pvh_branch_values = NULL;
		SG_uint32 count_branch_values = 0;
		SG_uint32 count_gids = 0;
		SG_uint32 i_gid = 0;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_by_branch, i_branch, &psz_branch, &pvh_gids_for_one_branch)  );

		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_branches, psz_branch, &pvh_branch_info)  );
		if (pvh_branch_info != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_branch_values, &count_branch_values)  );
		}


		// now go through the locks for this branch

		//This logic is a little bit unexpected, but for a reason.
		//The outer loop is over gids, and then branch heads.  A branch head is considered to be
		//violating a lock if the HID for the locked object is not the starting HID 
		//for ANY locks.  Restating it, for a branch head to be non-violating, the 
		//locked object's HID must match the starting HID for at least one open 
		//or waiting lock.  Open lockes have no end_csid, waiting locks have an 
		//end_csid, but the changeset is not present in this repo.
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
		for (i_gid=0; i_gid<count_gids; i_gid++)
		{
			const char* psz_gid = NULL;
			SG_vhash* pvh_locks_for_one_gid = NULL;
			SG_uint32 i_branch_value = 0;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, &pvh_locks_for_one_gid)  );

			for (i_branch_value=0; i_branch_value<count_branch_values; i_branch_value++)
			{
				const char* psz_branch_csid = NULL;
				SG_varray* pva_paths = NULL;
				SG_bool b_has_changeset = SG_FALSE;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, i_branch_value, &psz_branch_csid, NULL)  );

				SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pRepo, psz_branch_csid, &b_has_changeset)  );
				if (b_has_changeset)
				{
					SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, psz_branch_csid, &psz_hid_root_tn)  );
					ptn_root = NULL;
					SG_ERR_CHECK(  SG_tncache__load(pCtx, ptnc, psz_hid_root_tn, &ptn_root)  );
					SG_NULLFREE(pCtx, psz_hid_root_tn);
					SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pvh_paths, psz_gid, &pva_paths)  );
					if (pva_paths)
					{
						SG_ERR_CHECK(  x_get_hid(pCtx, ptnc, ptn_root, pva_paths, psz_gid, &psz_this_hid)  );
						SG_ERR_CHECK(  _check_changeset_for_lock_violation(pCtx, pRepo, psz_gid, psz_this_hid, psz_branch, pvh_locks_for_one_gid, &pvh_violated_locks)  );

						SG_NULLFREE(pCtx, psz_this_hid);
					}
					else
					{
						// TODO now what?
					}
				}
				else
				{
					// TODO what do we do here?  the changeset we want to check does not exist
				}

			}
		}
	}

	// ----------------------------------------------------------------
	// look for duplicate locks
	// ----------------------------------------------------------------

	for (i_branch=0; i_branch<count_branches_with_locks; i_branch++)
	{
		const char* psz_branch = NULL;
		SG_vhash* pvh_gids_for_one_branch = NULL;
		SG_uint32 count_gids = 0;
		SG_uint32 i_gid = 0;
		SG_bool bHasDuplicates = SG_FALSE;
		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_by_branch, i_branch, &psz_branch, &pvh_gids_for_one_branch)  );

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
		for (i_gid=0; i_gid<count_gids; i_gid++)
		{
			SG_vhash* pvh_locks_for_one_gid = NULL;
			const char* psz_gid = NULL;
			bHasDuplicates = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, &pvh_locks_for_one_gid)  );
			SG_ERR_CHECK(  sg_vc_locks__check_for_duplicates(pCtx, pvh_locks_for_one_gid, &bHasDuplicates)  );
			if (bHasDuplicates)
			{
				SG_vhash* pvh_dup_gids_for_one_branch = NULL;

				if (!pvh_duplicate_locks)
				{
					SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_duplicate_locks)  );
				}

				// add to pvh_duplicate_locks
				SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_duplicate_locks, psz_branch, &pvh_dup_gids_for_one_branch)  );
				if (!pvh_dup_gids_for_one_branch)
				{
					SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_duplicate_locks, psz_branch, &pvh_dup_gids_for_one_branch)  );
				}

				{
					SG_vhash* pvh_copy = NULL;

					SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_dup_gids_for_one_branch, psz_gid, &pvh_copy)  );
					SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pvh_locks_for_one_gid, pvh_copy)  );
				}
			}
		}
	}

	if (ppvh_violated_locks)
	{
		*ppvh_violated_locks = pvh_violated_locks;
		pvh_violated_locks = NULL;
	}

	if (ppvh_duplicate_locks)
	{
		*ppvh_duplicate_locks = pvh_duplicate_locks;
		pvh_duplicate_locks = NULL;
	}

fail:
    SG_NULLFREE(pCtx, psz_this_hid);
    SG_NULLFREE(pCtx, psz_hid_root_tn);
	SG_VHASH_NULLFREE(pCtx, pvh_gids);
	SG_VHASH_NULLFREE(pCtx, pvh_paths);
	SG_VHASH_NULLFREE(pCtx, pvh_duplicate_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_violated_locks);
    SG_TNCACHE_NULLFREE(pCtx, ptnc);
}

void SG_vc_locks__cleanup_completed(
	SG_context* pCtx, 
	SG_repo* pRepo,
    SG_audit* pq
	)
{
	char* psz_hid_cs_leaf = NULL;
	SG_varray* pva_fields = NULL;
	SG_varray* pva = NULL;
	SG_varray* pva_crit = NULL;
    SG_vhash* pvh_recids = NULL;

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_LOCKS, &psz_hid_cs_leaf)  );

	SG_ASSERT(psz_hid_cs_leaf);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "end_csid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "exists")  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "gid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "userid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "start_hid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "end_csid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );

	SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, SG_DAGNUM__VC_LOCKS, psz_hid_cs_leaf, NULL, pva_crit, NULL, 0, 0, pva_fields, &pva)  );

	if (pva)
	{
		SG_uint32 count_records = 0;
		SG_uint32 i = 0;

		SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_records)  );
		for (i=0; i<count_records; i++)
		{
			SG_vhash* pvh_record = NULL;
			const char* psz_recid = NULL;
			SG_bool bIsCompleted = SG_FALSE;
			SG_bool bWasChecked = SG_FALSE;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh_record)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, SG_ZING_FIELD__RECID, &psz_recid)  );

			
			SG_ERR_CHECK( SG_vhash__has(pCtx, pvh_record, "is_completed", &bWasChecked)  );
			if (bWasChecked == SG_TRUE)
			{
				SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_record, "is_completed", &bIsCompleted)  );
			}
			else
				SG_ERR_CHECK(  SG_vc_locks__check_for_completed_lock(pCtx, pRepo, pvh_record, &bIsCompleted)  );

			if (bIsCompleted == SG_TRUE)
			{
				if (!pvh_recids)
                {
                    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_recids)  );
                }

                SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_recids, psz_recid)  );
			}
        }

        if (pvh_recids)
        {
            // then delete the lock records
            SG_ERR_CHECK(  sg_vc_locks__delete(pCtx, pRepo, pvh_recids, pq)  );
        }
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_recids);
	SG_VARRAY_NULLFREE(pCtx, pva_crit);
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

void SG_vc_locks__list_for_one_branch(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	const char* psz_branch_name,
	SG_bool bIncludeCompletedLocks,
	SG_vhash** ppvh_locks
	)
{
	char* psz_hid_cs_leaf = NULL;
	SG_varray* pva_fields = NULL;
	SG_varray* pva = NULL;
	SG_varray* pva_crit = NULL;
	SG_vhash* pvh = NULL;
	SG_vhash* pvh_username_join = NULL;

	SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_LOCKS, &psz_hid_cs_leaf)  );

	SG_ASSERT(psz_hid_cs_leaf);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_crit)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "branch")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, "==")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_crit, psz_branch_name)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "gid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "userid")  ); 
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "start_hid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "end_csid")  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );
	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_fields, &pvh_username_join)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_username_join, "type", "username")  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_username_join, "userid", SG_AUDIT__USERID)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_username_join, "alias", SG_AUDIT__USERNAME)  );

	SG_ERR_CHECK(  SG_repo__dbndx__query(pCtx, pRepo, SG_DAGNUM__VC_LOCKS, psz_hid_cs_leaf, NULL, pva_crit, NULL, 0, 0, pva_fields, &pva)  );

	if (pva)
	{
		SG_uint32 count_records = 0;
		SG_uint32 i = 0;

		SG_ERR_CHECK(  sg_vc_locks__decorate_completed_locks(pCtx, pRepo, pva)  );

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

		SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count_records)  );
		for (i=0; i<count_records; i++)
		{
			SG_vhash* pvh_record = NULL;
			const char* psz_start_hid = NULL;
			const char* psz_end_csid = NULL;
			const char* psz_userid = NULL;
			const char* psz_username = NULL;
			const char* psz_gid = NULL;
			const char* psz_recid = NULL;
			SG_vhash* pvh_new = NULL;
			SG_vhash* pvh_locks_for_one_gid = NULL;

			// grab data for this record
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh_record)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, "gid", &psz_gid)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, "userid", &psz_userid)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, "username", &psz_username)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, "start_hid", &psz_start_hid)  );
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_record, "end_csid", &psz_end_csid)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_record, SG_ZING_FIELD__RECID, &psz_recid)  );

			if (psz_end_csid != NULL && !bIncludeCompletedLocks)
			{
				SG_bool bIsCompleted = SG_FALSE;
				SG_bool bWasChecked = SG_FALSE;
				SG_ERR_CHECK( SG_vhash__has(pCtx, pvh_record, "is_completed", &bWasChecked)  );
				if (bWasChecked == SG_TRUE)
				{
					SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_record, "is_completed", &bIsCompleted)  );
					//If they want us to filter out completed locks, we need to skip this lock.
					if (bIsCompleted == SG_TRUE)
					{
						continue;
					}
				}
				else
					SG_ERR_CHECK(  SG_vc_locks__check_for_completed_lock(pCtx, pRepo, pvh_record, &bIsCompleted)  );
			}

			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh, psz_gid, &pvh_locks_for_one_gid)  );
			if (!pvh_locks_for_one_gid)
			{
				SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, psz_gid, &pvh_locks_for_one_gid)  );
			}

			SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_locks_for_one_gid, psz_recid, &pvh_new)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new, "username", psz_username)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new, "userid", psz_userid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new, "start_hid", psz_start_hid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new, SG_ZING_FIELD__RECID, psz_recid)  );
            if (psz_end_csid)
            {
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_new, "end_csid", psz_end_csid)  );
            }
		}
	}

	*ppvh_locks = pvh;
	pvh = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VARRAY_NULLFREE(pCtx, pva_crit);
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VARRAY_NULLFREE(pCtx, pva_fields);
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
}

void SG_vc_locks__unlock(
	SG_context* pCtx, 
	const char* psz_repo_mine,
	const char* psz_repo_upstream,
	const char* psz_username,
	const char* psz_password,
	const char* psz_branch,
	SG_vhash* pvh_gids,
	SG_bool b_force,
	const SG_audit* pq
	)
{
	SG_repo* pRepo = NULL;
	SG_vhash* pvh_existing_locks = NULL;
	SG_vhash* pvh_recids = NULL;

	SG_NULLARGCHECK_RETURN(pvh_gids);
	SG_NULLARGCHECK_RETURN(psz_branch);
	SG_NULLARGCHECK_RETURN(psz_repo_mine);
	SG_NULLARGCHECK_RETURN(pq);

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo_mine, &pRepo)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_recids)  );

	SG_ERR_CHECK(  SG_vc_locks__pull_locks_and_branches(pCtx, psz_repo_upstream, psz_username, psz_password, psz_repo_mine)  );

	// now find the locks to be deleted
	SG_ERR_CHECK(  SG_vc_locks__list_for_one_branch(pCtx, pRepo, psz_branch, SG_FALSE, &pvh_existing_locks)  );
	if (pvh_existing_locks)
	{
		SG_uint32 count_gids = 0;
		SG_uint32 i_gid = 0;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids, &count_gids)  );
		for (i_gid=0; i_gid<count_gids; i_gid++)
		{
			const char* psz_gid = NULL;
			SG_vhash* pvh_locks_for_one_gid = NULL;
			SG_uint32 count_to_be_deleted_for_this_gid = 0;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_gids, i_gid, &psz_gid, NULL)  );
			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_existing_locks, psz_gid, &pvh_locks_for_one_gid)  );
			if (pvh_locks_for_one_gid)
			{
				SG_uint32 count_locks = 0;
				SG_uint32 i_lock = 0;

				SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );
				for (i_lock=0; i_lock<count_locks; i_lock++)
				{
					const char* psz_recid_lock = NULL;
					SG_vhash* pvh_lock = NULL;
					SG_bool b_delete = SG_FALSE;
                    SG_bool b_ended = SG_FALSE;

					SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_recid_lock, &pvh_lock)  );

                    SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_lock, "end_csid", &b_ended)  );

                    if (b_ended)
                    {
                        SG_ERR_THROW(  SG_ERR_VC_LOCK_CANNOT_DELETE_ENDED  );
                    }
                    else
                    {
                        if (b_force)
                        {
                            b_delete = SG_TRUE;
                        }
                        else
                        {
                            const char* psz_userid = NULL;

                            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "userid", &psz_userid)  );
                            if (0 == strcmp(psz_userid, pq->who_szUserId))
                            {
                                b_delete = SG_TRUE;
                            }
                        }

                        if (b_delete)
                        {
                            count_to_be_deleted_for_this_gid++;
                            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_recids, psz_recid_lock)  );
                        }
                    }
				}
			}

			if (0 == count_to_be_deleted_for_this_gid)
			{
				SG_ERR_THROW(  SG_ERR_VC_LOCK_NOT_FOUND  );
			}
		}
	}

	// then delete the lock records
	SG_ERR_CHECK(  sg_vc_locks__delete(pCtx, pRepo, pvh_recids, pq)  );

	// then push the branches/locks dag back up
	SG_ERR_CHECK(  SG_vc_locks__push_locks(pCtx, pRepo, psz_repo_upstream, psz_username, psz_password)  );
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_existing_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_recids);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_vc_locks__request(
	SG_context* pCtx, 
	const char* psz_repo_mine,
	const char* psz_repo_upstream,
	const char* psz_username,
	const char* psz_password,
	const char* psz_csid,
	const char* psz_branch,
	SG_vhash* pvh_gids,
	const SG_audit* pq
	)
{
	SG_vhash* pvh_pile = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvh_branches = NULL;
	SG_vhash* pvh_existing_locks = NULL;
	SG_bool b_has = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pvh_gids);
	SG_NULLARGCHECK_RETURN(psz_branch);
	SG_NULLARGCHECK_RETURN(psz_csid);
	SG_NULLARGCHECK_RETURN(psz_repo_upstream);
	SG_NULLARGCHECK_RETURN(psz_repo_mine);
	SG_NULLARGCHECK_RETURN(pq);

	// first we need to pull the branches and locks from the other side

	SG_ERR_CHECK(  SG_vc_locks__pull_locks_and_branches(pCtx, psz_repo_upstream, psz_username, psz_password, psz_repo_mine)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, psz_repo_mine, &pRepo)  );

	// do the usual check for branch/lock problems.  at the end of this
	// operation, we're going to push the branch/locks dags back up.  We
	// can't do that if any of the branches need to be merged or if any
	// locks have been violated.

	SG_ERR_CHECK(  SG_vc_locks__ensure__okay_to_push(
		pCtx, 
		pRepo, 
		&pvh_pile,
		NULL // TODO could get locks_by_branch and save a call below
		)  );

	// now make sure the branch exists and the WD is up-to-date with it
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_branches)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_branches, psz_branch, &b_has)  );
	if (b_has)
	{
		const char* psz_branch_csid = NULL;
		SG_vhash* pvh_branch_info = NULL;
		SG_vhash* pvh_branch_values = NULL;

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branches, psz_branch, &pvh_branch_info)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_branch_info, "values", &pvh_branch_values)  );
		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_branch_values, 0, &psz_branch_csid, NULL)  );
		if (0 != strcmp(psz_branch_csid, psz_csid))
		{
			SG_ERR_THROW(  SG_ERR_VC_LOCK_BRANCH_MUST_BE_CURRENT  );
		}
	}
	else
	{
		SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
	}

	// now check to see if the requested locks conflict with
	// any existing locks
	SG_ERR_CHECK(  SG_vc_locks__list_for_one_branch(pCtx, pRepo, psz_branch, SG_FALSE, &pvh_existing_locks)  );
	if (pvh_existing_locks)
	{
		SG_uint32 count_gids = 0;
		SG_uint32 i_gid = 0;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids, &count_gids)  );
		for (i_gid=0; i_gid<count_gids; i_gid++)
		{
			const char* psz_gid = NULL;
			SG_bool b_already = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_gids, i_gid, &psz_gid, NULL)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_existing_locks, psz_gid, &b_already)  );
			if (b_already)
			{
				// TODO we want to be able to include the path here
				// but we already converted everything to gids
				SG_ERR_THROW(  SG_ERR_VC_LOCK_ALREADY_LOCKED  );
			}
		}
	}

	// then create the lock records
	SG_ERR_CHECK(  sg_vc_locks__add(pCtx, pRepo, psz_csid, psz_branch, pvh_gids, pq)  );

	// then push the branches/locks dag back up
	SG_ERR_CHECK(  SG_vc_locks__push_locks(pCtx, pRepo, psz_repo_upstream, psz_username, psz_password)  );

	// TODO then pull it back and see if a race condition occurred?

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_existing_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_vc_locks__push_locks(
		SG_context * pCtx,
		SG_repo* pRepo,
		const char * pszDestRepoDescriptorName,
		const char * pszUsername,
		const char * pszPassword
		)
	{
		SG_push* pPush = NULL;

		SG_ERR_CHECK(  SG_push__begin(pCtx, pRepo, pszDestRepoDescriptorName, pszUsername, pszPassword, &pPush)  );

		SG_ERR_CHECK(  SG_push__add(pCtx, pPush, SG_DAGNUM__USERS, NULL)  );

		//We no longer push branches for lock operations.  See J3693.
		//SG_ERR_CHECK(  SG_push__add(pCtx, pPush, SG_DAGNUM__VC_BRANCHES, NULL)  );

		SG_ERR_CHECK(  SG_push__add(pCtx, pPush, SG_DAGNUM__VC_LOCKS, NULL)  );

		SG_ERR_CHECK(  SG_push__commit(pCtx, &pPush, SG_FALSE, NULL, NULL)  );

		/* fall through */
fail:
		if (pPush)
			SG_ERR_IGNORE(  SG_push__abort(pCtx, &pPush)  );
}

void SG_vc_locks__pull_locks_and_branches(
	SG_context * pCtx,
	const char* pszSrcRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	const char * pszDestRepoDescriptorName
	)
{
	SG_repo* pRepoDest = NULL;
	SG_pull* pPull = NULL;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Synchronizing locks, users, and branches", SG_LOG__FLAG__NONE)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDestRepoDescriptorName, &pRepoDest)  );

	SG_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszSrcRepoSpec, pszUsername, pszPassword, &pPull)  );

	SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__USERS)  );

	SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__VC_BRANCHES)  );

	SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__VC_LOCKS)  );

	SG_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, NULL)  );

	/* fall through */
fail:
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );

}

void SG_vc_locks__pull_locks(
	SG_context * pCtx,
	const char* pszSrcRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_repo * pRepoDest
	)
{
	SG_pull* pPull = NULL;
	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Synchronizing locks and users", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszSrcRepoSpec, pszUsername, pszPassword, &pPull)  );

	SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__USERS)  );
	
	SG_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__VC_LOCKS)  );

	SG_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, NULL)  );

	/* fall through */
fail:
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
	SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
}

void SG_vc_locks__check__okay_to_push(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_vhash** ppvh_pile,
	SG_vhash** ppvh_locks_by_branch,
	SG_vhash** ppvh_duplicate_locks,
	SG_vhash** ppvh_violated_locks
	)
{
	SG_vhash* pvh_pile = NULL;
	SG_vhash* pvh_locks_by_branch = NULL;
	SG_vhash* pvh_duplicate_locks = NULL;
	SG_vhash* pvh_violated_locks = NULL;

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );

	SG_ERR_CHECK(  sg_vc_locks__by_branch(pCtx, pRepo, &pvh_locks_by_branch)  );

	if (pvh_locks_by_branch)
	{
		SG_ERR_CHECK(  sg_vc_locks__check_for_problems(pCtx, pRepo, pvh_pile, pvh_locks_by_branch, &pvh_duplicate_locks, &pvh_violated_locks)  );
	}

	if (ppvh_pile)
	{
		*ppvh_pile = pvh_pile;
		pvh_pile = NULL;
	}

	if (ppvh_locks_by_branch)
	{
		*ppvh_locks_by_branch = pvh_locks_by_branch;
		pvh_locks_by_branch = NULL;
	}

	if (ppvh_duplicate_locks)
	{
		*ppvh_duplicate_locks = pvh_duplicate_locks;
		pvh_duplicate_locks = NULL;
	}

	if (ppvh_violated_locks)
	{
		*ppvh_violated_locks = pvh_violated_locks;
		pvh_violated_locks = NULL;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_VHASH_NULLFREE(pCtx, pvh_locks_by_branch);
	SG_VHASH_NULLFREE(pCtx, pvh_duplicate_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_violated_locks);
}

void SG_vc_locks__pre_commit(
	SG_context* pCtx, 
	SG_repo* pRepo,
    const char* psz_userid,
    const char* psz_tied_branch_name,
    SG_vhash** ppvh_result
    )
{
    SG_vhash* pvh_gids_for_one_branch = NULL;
    SG_vhash* pvh_result = NULL;

	SG_NULLARGCHECK_RETURN(psz_userid);
	SG_NULLARGCHECK_RETURN(psz_tied_branch_name);
	SG_NULLARGCHECK_RETURN(ppvh_result);

	SG_ERR_CHECK(  SG_vc_locks__list_for_one_branch(pCtx, pRepo, psz_tied_branch_name, SG_FALSE, &pvh_gids_for_one_branch)  );
    if (pvh_gids_for_one_branch)
    {
        SG_uint32 count_gids = 0;
        SG_uint32 i_gid = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
        for (i_gid=0; i_gid<count_gids; i_gid++)
        {
            const char* psz_gid = NULL;
            SG_vhash* pvh_locks_for_one_gid = NULL;
            SG_uint32 count_locks = 0;
            SG_uint32 i_lock = 0;
            const char* psz_recid_lock = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, &pvh_locks_for_one_gid)  );

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );
            for (i_lock=0; i_lock<count_locks; i_lock++)
            {
                SG_vhash* pvh_lock = NULL;
                const char* psz_lock_userid = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_recid_lock, &pvh_lock)  );
                SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "userid", &psz_lock_userid)  );
                if (0 != strcmp(psz_userid, psz_lock_userid))
                {
					SG_bool bIsCompleted = SG_FALSE;
					SG_bool bWasChecked = SG_FALSE;
						
					SG_ERR_CHECK( SG_vhash__has(pCtx, pvh_lock, "is_completed", &bWasChecked)  );
					if (bWasChecked == SG_TRUE)
					{
						SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_lock, "is_completed", &bIsCompleted)  );
					}
					else
						SG_ERR_CHECK(  SG_vc_locks__check_for_completed_lock(pCtx, pRepo, pvh_lock, &bIsCompleted)  );

					if (bIsCompleted == SG_FALSE)
					{
						if (!pvh_result)
						{
							SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
						}
						SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_result, psz_gid, psz_lock_userid)  );
						break; //We only need to know about one of the open or waiting locks.  One is enough.
					}
                }
            }
		}
	}
        
    *ppvh_result = pvh_result;
    pvh_result = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_result);
	SG_VHASH_NULLFREE(pCtx, pvh_gids_for_one_branch);
}

void SG_vc_locks__post_commit(
	SG_context* pCtx, 
	SG_repo* pRepo,
    const SG_audit* pq,
    const char* psz_tied_branch_name,
    SG_changeset* pcs_tree
    )
{
    SG_vhash* pvh_gids_for_one_branch = NULL;
    SG_vhash* pvh_locks = NULL;
	char* psz_hid_cs_leaf = NULL;
    char* psz_hid_root_tn = NULL;
	SG_zingtx* pztx = NULL;
	SG_zingrecord* prec = NULL;
	SG_dagnode* pdn = NULL;
	SG_changeset* pcs = NULL;
	SG_zingtemplate* pzt = NULL;
	SG_zingfieldattributes* pzfa = NULL;
    char* psz_this_hid = NULL;
    const char* psz_new_csid = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(pq);
    SG_NULLARGCHECK_RETURN(psz_tied_branch_name);
    SG_NULLARGCHECK_RETURN(pcs_tree);

    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcs_tree, &psz_new_csid)  );

	SG_ERR_CHECK(  SG_vc_locks__list_for_one_branch(pCtx, pRepo, psz_tied_branch_name, SG_FALSE, &pvh_gids_for_one_branch)  );
    if (pvh_gids_for_one_branch)
    {
        SG_uint32 count_gids = 0;
        SG_uint32 i_gid = 0;

        SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gids_for_one_branch, &count_gids)  );
        for (i_gid=0; i_gid<count_gids; i_gid++)
        {
            const char* psz_gid = NULL;
            SG_vhash* pvh_locks_for_one_gid = NULL;
            SG_uint32 count_locks = 0;
            SG_uint32 i_lock = 0;
            const char* psz_recid_lock = NULL;

            SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gids_for_one_branch, i_gid, &psz_gid, &pvh_locks_for_one_gid)  );

            SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_locks_for_one_gid, &count_locks)  );
            for (i_lock=0; i_lock<count_locks; i_lock++)
            {
                SG_vhash* pvh_lock = NULL;
                const char* psz_end_csid = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_locks_for_one_gid, i_lock, &psz_recid_lock, &pvh_lock)  );
                SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_lock, "end_csid", &psz_end_csid)  );
                if (!psz_end_csid)
                {
                    const char* psz_lock_userid = NULL;

                    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "userid", &psz_lock_userid)  );
                    if (0 == strcmp(pq->who_szUserId, psz_lock_userid))
                    {
                        if (!pvh_locks)
                        {
                            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_locks)  );
                        }
                        SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvh_locks, psz_gid, pvh_lock)  );
                    }
                }
            }
        }
    }

    if (pvh_locks)
    {
        SG_uint32 count_parents = 0;
        SG_uint32 i_parent = 0;

        SG_vhash* pvh_changes = NULL;

        SG_ERR_CHECK(  SG_changeset__tree__get_changes(pCtx, pcs_tree, &pvh_changes)  );

        SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, SG_DAGNUM__VC_LOCKS, &psz_hid_cs_leaf)  );

        /* start a changeset */
        SG_ERR_CHECK(  SG_zing__begin_tx(pCtx, pRepo, SG_DAGNUM__VC_LOCKS, pq->who_szUserId, psz_hid_cs_leaf, &pztx)  );
        SG_ERR_CHECK(  SG_zingtx__add_parent(pCtx, pztx, psz_hid_cs_leaf)  );
        SG_ERR_CHECK(  SG_zingtx__get_template(pCtx, pztx, &pzt)  );

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
                const char* psz_gid = NULL;
                SG_vhash* pvh_changes_for_one_gid = NULL;
                SG_vhash* pvh_lock = NULL;
                const SG_variant* pv = NULL;

                SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_changes_for_one_parent, i_gid, &psz_gid, &pv)  );

                if (SG_VARIANT_TYPE_NULL == pv->type)
                {
                    pvh_changes_for_one_gid = NULL; // deleted
                }
                else
                {
                    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh_changes_for_one_gid)  );
                }
                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_locks, psz_gid, &pvh_lock)  );
                if (pvh_lock)
                {
                    SG_bool b_end_lock = SG_FALSE;

                    if (pvh_changes_for_one_gid)
                    {
                        const char* psz_start_hid = NULL;
                        const char* psz_new_hid = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, "start_hid", &psz_start_hid)  );

                        SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_changes_for_one_gid, SG_CHANGEESET__TREE__CHANGES__HID, &psz_new_hid)  );

                        if (
                                psz_new_hid 
                                && (0 != strcmp(psz_new_hid, psz_start_hid))
                           )
                        {
                            b_end_lock = SG_TRUE;
                        }
                    }
                    else
                    {
                        // deleted
                        b_end_lock = SG_TRUE;
                    }

                    if (b_end_lock)
                    {
                        const char* psz_recid_lock = NULL;

                        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_lock, SG_ZING_FIELD__RECID, &psz_recid_lock)  );

                        SG_ERR_CHECK(  SG_zingtx__get_record(pCtx, pztx, "lock", psz_recid_lock, &prec)  );

                        SG_ERR_CHECK(  SG_zingtemplate__get_field_attributes(pCtx, pzt, "lock", "end_csid", &pzfa)  );
                        SG_ERR_CHECK(  SG_zingrecord__set_field__string(pCtx, prec, pzfa, psz_new_csid) );
                    }
                }
            }
        }

        /* commit the changes */
        SG_ERR_CHECK(  SG_zing__commit_tx(pCtx, pq->when_int64, &pztx, &pcs, &pdn, NULL)  );
    }

fail:
    SG_NULLFREE(pCtx, psz_this_hid);
	SG_VHASH_NULLFREE(pCtx, pvh_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_gids_for_one_branch);
	if (pztx)
	{
		SG_ERR_IGNORE(  SG_zing__abort_tx(pCtx, &pztx)  );
	}
	SG_NULLFREE(pCtx, psz_hid_root_tn);
	SG_NULLFREE(pCtx, psz_hid_cs_leaf);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_CHANGESET_NULLFREE(pCtx, pcs);
}

void SG_vc_locks__ensure__okay_to_push(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_vhash** ppvh_pile,
	SG_vhash** ppvh_locks_by_branch
	)
{
	SG_vhash* pvh_pile = NULL;
	SG_vhash* pvh_locks_by_branch = NULL;
	SG_vhash* pvh_branches_needing_merge = NULL;
	SG_vhash* pvh_duplicate_locks = NULL;
	SG_vhash* pvh_violated_locks = NULL;

	SG_ERR_CHECK(  SG_vc_locks__check__okay_to_push(
		pCtx, 
		pRepo, 
		&pvh_pile,
		&pvh_locks_by_branch,
		&pvh_duplicate_locks,
		&pvh_violated_locks
		)  );

	if (pvh_branches_needing_merge)
	{
		SG_ERR_THROW(  SG_ERR_BRANCH_NEEDS_MERGE  );
	}
	if (pvh_duplicate_locks)
	{
		SG_ERR_THROW(  SG_ERR_VC_LOCK_DUPLICATE  );
	}
	if (pvh_violated_locks)
	{
		SG_ERR_THROW(  SG_ERR_VC_LOCK_VIOLATION  );
	}

	if (ppvh_pile)
	{
		*ppvh_pile = pvh_pile;
		pvh_pile = NULL;
	}

	if (ppvh_locks_by_branch)
	{
		*ppvh_locks_by_branch = pvh_locks_by_branch;
		pvh_locks_by_branch = NULL;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_branches_needing_merge);
	SG_VHASH_NULLFREE(pCtx, pvh_violated_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_duplicate_locks);
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_VHASH_NULLFREE(pCtx, pvh_locks_by_branch);
}

