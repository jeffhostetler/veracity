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

#ifndef H_SG_CERT_PROTOTYPES_H
#define H_SG_CERT_PROTOTYPES_H

BEGIN_EXTERN_C

#if defined(SG_CRYPTO)
void SG_cert__keygen(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    SG_pathname* pPath
    );

void SG_cert__verifykeypair(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username
    );

void SG_cert__revokekey(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username
    );

void SG_cert__sign(
	SG_context* pCtx,
    SG_repo* pRepo,
    const char* psz_username,
    const char* psz_statement,
    SG_stringarray* psa_csids,
    SG_vhash** ppvh_hids_sigs
    );

void SG_cert__verify(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_vhash* pvh_hids_sigs
    );
#endif

END_EXTERN_C

#endif

