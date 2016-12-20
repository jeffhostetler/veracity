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
 * @file sg_audit_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_AUDIT_PROTOTYPES_H
#define H_SG_AUDIT_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_audit__copy(
        SG_context* pCtx,
		const SG_audit* p,
        SG_audit** pp
        );

void SG_audit__init__maybe_nobody(
        SG_context* pCtx,
        SG_audit* pInfo,
        SG_repo* pRepo,
        SG_int64 itime,
        const char* psz_userid
        );

void SG_audit__init__nobody(
	SG_context* pCtx,
	SG_audit* pInfo,
	SG_repo* pRepo,
	SG_int64 itime);

void SG_audit__init(
        SG_context* pCtx,
		SG_audit* p,
        SG_repo* pRepo,
        SG_int64 itime,
        const char* psz_userid
        );

void SG_audit__init__friendly(SG_context * pCtx,
							  SG_audit * pInfo,
							  SG_repo * pRepo,
							  const char * pszWhen,
							  const char * pszWho);

void SG_audit__init__friendly__reponame(SG_context * pCtx,
										SG_audit * pInfo,
										const char * pszRepoName,
										const char * pszWhen,
										const char * pszWho);

void SG_audit__update_time_to_now(SG_context * pCtx,
								  SG_audit * pInfo);

END_EXTERN_C;

#endif //H_SG_AUDIT_PROTOTYPES_H

