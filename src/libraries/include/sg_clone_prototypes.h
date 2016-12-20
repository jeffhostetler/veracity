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
 * @file sg_clone_prototypes.h
 */

#ifndef H_SG_CLONE_PROTOTYPES_H
#define H_SG_CLONE_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_clone__to_remote(
    SG_context* pCtx,
    const char* psz_existing_repo_spec,
    const char* psz_username,
    const char* psz_password,
	const char* psz_new_repo_spec,
	SG_vhash** ppvhStatus
    );

void SG_clone__to_local(
    SG_context* pCtx,
    const char* psz_existing_repo_spec,
    const char* psz_username,
    const char* psz_password,
    const char* psz_new_repo_spec,
    struct SG_clone__params__all_to_something* p_params_ats,
    struct SG_clone__params__pack* p_params_pack,
    struct SG_clone__demands* p_demands
    );

void SG_clone__init_demands(
	SG_context* pCtx,
    struct SG_clone__demands* p_demands
    );

void SG_clone__validate_new_repo_name(
	SG_context* pCtx, 
	const char* psz_new_descriptor_name
);

void SG_clone__from_fragball(
	SG_context* pCtx,
	const char* psz_clone_id,
	const char* psz_new_repo_name,
	SG_pathname* pPathStaging,
	const char* pszFragballName);

void SG_clone__from_fragball__prep_usermap(
	SG_context* pCtx,
	const char* psz_clone_id,
	const char* pszNewDescriptor,
	SG_pathname** ppPathStaging,
	const char* pszFragballName);

void SG_clone__prep_usermap(
	SG_context* pCtx,
	const char* psz_existing,
	const char* psz_username,
	const char* psz_password,
	const char* psz_new);

void SG_clone__finish_usermap(
	SG_context* pCtx,
    const char* psz_descriptor_name
    );

END_EXTERN_C;

#endif

