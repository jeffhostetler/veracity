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

#ifndef H_SG_VC_HOOKS_PROTOTYPES_H
#define H_SG_VC_HOOKS_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_vc_hooks__lookup_by_interface(
    SG_context* pCtx, 
    SG_repo* pRepo, 
    const char* psz_interface, 
    SG_varray** ppva
    );

void SG_vc_hooks__install(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_interface,
    const char* psz_js,
	const char *module,
	SG_uint32 version,
	SG_bool replaceOld,
    const SG_audit* pq
    );

void SG_vc_hooks__execute(
    SG_context* pCtx, 
    const char* psz_js,
    SG_vhash* pvh_params,
    SG_vhash** ppvh_result
    );

void SG_vc_hooks__ASK__WIT__ADD_ASSOCIATIONS(
    SG_context* pCtx, 
    SG_repo* pRepo, 
    SG_changeset* pcs,
    const char* psz_tied_branch_name,
    const SG_audit* pq,
    const char* psz_comment,
    const char* const* paszAssocs,
    SG_uint32 count_assocs,
    const SG_stringarray* psa_stamps
    );

void SG_vc_hooks__ASK__WIT__VALIDATE_ASSOCIATIONS(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	const char* const* paszAssocs,
	SG_uint32 count_assocs,
	SG_varray *assocdesc
	);

void SG_vc_hooks__ASK__WIT__LIST_ITEMS(
	SG_context* pCtx, 
	SG_repo* pRepo,
	const char * psz_search_term,
	SG_varray *pBugs
	);

void SG_vc_hooks__BROADCAST__AFTER_COMMIT(
    SG_context* pCtx, 
    SG_repo* pRepo, 
    SG_changeset* pcs,
    const char* psz_tied_branch_name,
    const SG_audit* pq,
    const char* psz_comment,
    const char* const* paszAssocs,
    SG_uint32 count_assocs,
    const SG_stringarray* psa_stamps
    );

END_EXTERN_C;

#endif//H_SG_VC_HOOKS_PROTOTYPES_H

