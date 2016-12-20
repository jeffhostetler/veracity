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
 * @file sg_user_prototypes.h
 *
 */

#ifndef H_SG_USER_PROTOTYPES_H
#define H_SG_USER_PROTOTYPES_H

BEGIN_EXTERN_C

void SG_user__set_user__repo(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username
    );

/**
 * Sanitizes a potential username by replacing invalid characters
 * and adding/removing characters to meet min/max length requirements.
 * The returned string will be a valid username.
 */
void SG_user__sanitize_username(
	SG_context* pCtx,          //< [in] [out] Error and context info.
	const char* psz_username,  //< [in] Username to sanitize.
	char**      ppsz_sanitized //< [out] Sanitized username.
	);

/**
 * Ensures that a username does not already exist in a list of usernames.
 * If it does, then the name is modified by appending a number such that
 * it becomes unique with respect to the given list.
 */
void SG_user__uniqify_username(
	SG_context*     pCtx,          //< [in] [out] Error and context info.
	const char*     psz_username,  //< [in] Username to uniqify.
	const SG_vhash* pvh_userlist,  //< [in] List of users to check against.
	                               //<      The KEYS of this hash constitute the list; values are ignored.
	char**          ppsz_uniqified //< [out] Uniqified username.
	);

void SG_user__create(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    char** ppsz_userid
    );

void SG_user__create__inactive(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    char** ppsz_userid
    );

void SG_user__create_force_recid(
	SG_context* pCtx,
    const char* psz_username,
    const char* psz_recid,
    SG_bool b_inactive,
    SG_repo* pRepo
    );

void SG_user__create_nobody(
	SG_context* pCtx,
    SG_repo* pRepo
    );

void SG_user__rename(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_userid,
	const char* psz_username
	);

void SG_user__set_active(
	SG_context *pCtx,
	SG_repo *pRepo,
	const char *psz_userid,
	SG_bool active);

void SG_user__is_inactive(
	SG_context *pCtx,
	SG_vhash *user,
	SG_bool *inactive);

void SG_user__list_all(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_varray** ppva
    );

void SG_user__list_active(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_varray** ppva
	);
	
void SG_group__list(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_varray** ppva
    );

void SG_user__lookup_by_userid(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_audit* pq,
    const char* psz_userid,
    SG_vhash** ppvh
    );

void SG_user__lookup_by_name(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_audit* pq,
    const char* psz_username,
    SG_vhash** ppvh
    );

void SG_user__get_username_for_repo(
	SG_context * pCtx,
	SG_repo* pRepo,
	char ** ppsz_username);

void SG_group__create(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_name
    );

void SG_group__add_users(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    );

void SG_group__list_all_users(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    SG_vhash** ppvh
    );

void SG_group__add_subgroups(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    );

void SG_group__remove_users(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    );

void SG_group__remove_subgroups(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_group_name,
    const char** paszMemberNames,
    SG_uint32 count_names
    );

void SG_group__get_by_name(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_name,
    SG_vhash** ppvh
    );

void SG_group__get_by_recid(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_recid,
    SG_vhash** ppvh
    );

END_EXTERN_C

#endif
