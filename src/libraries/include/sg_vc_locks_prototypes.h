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
 * @file sg_vc_locks_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VC_LOCKS_PROTOTYPES_H
#define H_SG_VC_LOCKS_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_vc_locks__cleanup_completed(
	SG_context* pCtx, 
	SG_repo* pRepo,
    SG_audit* pq
	);

void SG_vc_locks__post_commit(
	SG_context* pCtx, 
	SG_repo* pRepo,
    const SG_audit* pq,
    const char* psz_tied_branch_name,
    SG_changeset* pcs
    );

void SG_vc_locks__pre_commit(
	SG_context* pCtx, 
	SG_repo* pRepo,
    const char* psz_userid,
    const char* psz_tied_branch_name,
    SG_vhash** ppvh_result
    );

void SG_vc_locks__pull_locks_and_branches(
	SG_context * pCtx,
	const char* pszSrcRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	const char * pszDestRepoDescriptorName
	);

void SG_vc_locks__pull_locks(
	SG_context * pCtx,
	const char* pszSrcRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_repo * pRepoDest
	);

void SG_vc_locks__push_locks(
	SG_context * pCtx,
	SG_repo* pRepo,
	const char * pszDestRepoDescriptorName,
	const char * pszUsername,
	const char * pszPassword
	);

void SG_vc_locks__list_for_one_branch(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	const char* psz_branch_name,
	SG_bool bIncludeCompletedLocks,
	SG_vhash** ppvh_locks
	);

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
	);

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
	);

void SG_vc_locks__check__okay_to_push(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_vhash** ppvh_pile,
	SG_vhash** ppvh_locks_by_branch,
	SG_vhash** ppvh_duplicate_locks,
	SG_vhash** ppvh_violated_locks
	);

void SG_vc_locks__ensure__okay_to_push(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_vhash** ppvh_pile,
	SG_vhash** ppvh_locks_by_branch
	);

void SG_vc_locks__check_for_completed_lock(
	SG_context * pCtx, 
	SG_repo * pRepo, 
	SG_vhash * pvh_lock, 
	SG_bool * pbIsCompleted);

END_EXTERN_C;

#endif //H_SG_VC_LOCKS_PROTOTYPES_H

