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
 * @file sg_vc_branches_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VC_BRANCHES_PROTOTYPES_H
#define H_SG_VC_BRANCHES_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_vc_branches__get_branch_names_for_given_changesets(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_varray** ppva_csid_list, 
    SG_vhash** ppvh_results_by_csid
    );

void SG_vc_branches__reopen(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const SG_audit* pq
        );

void SG_vc_branches__close(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const SG_audit* pq
        );

void SG_vc_branches__move_head(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs_from, 
        const char* psz_hid_cs_target, 
        const SG_audit* pq
        );

void SG_vc_branches__move_head__all(
        SG_context*, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_csid, 
        const SG_audit* pq
        );

void SG_vc_branches__add_head(
        SG_context*, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_csid, 
        SG_varray* pva_parents,
        SG_bool b_strict,
        const SG_audit* pq
        );

void SG_vc_branches__add_head__state(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs_target, 
        SG_varray* pva_parents,
        SG_vhash* pvh_state,
        const SG_audit* pq
        );

void SG_vc_branches__remove_head(
        SG_context*, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_csid, 
        const SG_audit* pq
        );

void SG_vc_branches__get_unambiguous(
        SG_context*, 
        SG_repo* pRepo, 
        const char* psz_branch_name,
        SG_bool* pb_exists,
        char** ppsz_csid
        );

void SG_vc_branches__delete(
        SG_context*, 
        SG_repo* pRepo, 
        const char* psz_branch_name, 
        const char* psz_hid_cs,
        const SG_audit* pq
        );

void SG_vc_branches__get_whole_pile(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash** ppvh
        );

void SG_vc_branches__cleanup(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash** ppvh_pile
        );

void SG_vc_branches__exists(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_branch_name,
	SG_bool* pExists
	);

/**
 * Throws an error if the given branch name is not valid.
 */
void SG_vc_branches__ensure_valid_name(
	SG_context* pCtx,
	const char* psz_branch_name
	);

/**
 * Throw SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT with a helpful error message.
 * Note that you can call this when pCtx is already in an error state to preserve the full
 * stack trace, but the current error number must be SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT.
 */
void SG_vc_branches__throw_changeset_not_present(
	SG_context* pCtx,
	const char* pszBranchName,
	const char* pszHidCs);

void SG_vc_branches__normalize_name(SG_context * pCtx,
									const char * pszCandidateBranchName,
									char ** ppszNormalizedBranchName);

void SG_vc_branches__check_attach_name(SG_context * pCtx,
									   SG_repo * pRepo,
									   const char * pszCandidate,
									   SG_vc_branches__check_attach_name__flags flags,
									   SG_bool bValidate,
									   char ** ppszNormalized);

END_EXTERN_C;

#endif //H_SG_VC_BRANCHES_PROTOTYPES_H

