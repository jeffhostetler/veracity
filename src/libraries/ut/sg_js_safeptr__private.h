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

#ifndef H_SG_SAFEPTR_H
#define H_SG_SAFEPTR_H

BEGIN_EXTERN_C;

typedef struct _SG_safeptr         SG_safeptr;
typedef struct sg_zingdb sg_zingdb;
typedef struct sg_zinglinkcollection sg_zinglinkcollection;
typedef struct sg_xmlwriter sg_xmlwriter;
typedef struct sg_statetxcombo sg_statetxcombo;

typedef struct 
{
	SG_file* pFile;
	SG_pathname* pPathFile;
	SG_bool bDeleteWhenDone;
} SG_generic_file_response_context;


#define SG_SAFEPTR_TYPE__AUDIT "audit"
#define SG_SAFEPTR_TYPE__BLOBSET "blobset"
#define SG_SAFEPTR_TYPE__CBUFFER "cbuffer"
#define SG_SAFEPTR_TYPE__REPO "repo"
#define SG_SAFEPTR_TYPE__FETCHBLOBHANDLE "fetchblobhandle"
#define SG_SAFEPTR_TYPE__FETCHFILEHANDLE "fetchfilehandle"
#define SG_SAFEPTR_TYPE__DBNDX "dbndx"
#define SG_SAFEPTR_TYPE__ZINGRECORD "zingrecord"
#define SG_SAFEPTR_TYPE__ZINGSTATE "zingdb"
#define SG_SAFEPTR_TYPE__STATETXCOMBO "zingdbtxcombo"
#define SG_SAFEPTR_TYPE__ZINGLINKCOLLECTION "zinglinkcollection"
#define SG_SAFEPTR_TYPE__COMMITTING "committing"
#define SG_SAFEPTR_TYPE__XMLWRITER "xmlwriter"

// TODO 2011/10/03 See also SG_js_safeptr__{wrap,unwrap}__generic()
// TODO            in ../include/sg_js_safeptr_prototypes.h

void SG_safeptr__wrap__audit(SG_context* pCtx, SG_audit* p, SG_safeptr** pp);
void SG_safeptr__unwrap__audit(SG_context* pCtx, SG_safeptr* psafe, SG_audit** pp);

void SG_safeptr__wrap__cbuffer(SG_context* pCtx, SG_cbuffer* p, SG_safeptr** pp);
void SG_safeptr__unwrap__cbuffer(SG_context* pCtx, SG_safeptr* psafe, SG_cbuffer** pp);

void SG_safeptr__wrap__blobset(SG_context* pCtx, SG_blobset* p, SG_safeptr** pp);
void SG_safeptr__unwrap__blobset(SG_context* pCtx, SG_safeptr* psafe, SG_blobset** pp);

void SG_safeptr__wrap__repo(SG_context* pCtx, SG_repo* p, SG_safeptr** pp);
void SG_safeptr__unwrap__repo(SG_context* pCtx, SG_safeptr* psafe, SG_repo** pp);

void SG_safeptr__wrap__fetchblobhandle(SG_context* pCtx, SG_repo_fetch_blob_handle* p, SG_safeptr** pp);
void SG_safeptr__unwrap__fetchblobhandle(SG_context* pCtx, SG_safeptr* psafe, SG_repo_fetch_blob_handle** pp);

void SG_safeptr__wrap__fetchfilehandle(SG_context* pCtx, SG_generic_file_response_context* p, SG_safeptr** pp);
void SG_safeptr__unwrap__fetchfilehandle(SG_context* pCtx, SG_safeptr* psafe, SG_generic_file_response_context** pp);

void SG_safeptr__wrap__committing(SG_context* pCtx, SG_committing* p, SG_safeptr** pp);
void SG_safeptr__unwrap__committing(SG_context* pCtx, SG_safeptr* psafe, SG_committing** pp);

void SG_safeptr__wrap__statetxcombo(SG_context* pCtx, sg_statetxcombo* p, SG_safeptr** pp);
void SG_safeptr__unwrap__statetxcombo(SG_context* pCtx, SG_safeptr* psafe, sg_statetxcombo** pp);

void SG_safeptr__wrap__zingdb(SG_context* pCtx, sg_zingdb* p, SG_safeptr** pp);
void SG_safeptr__unwrap__zingdb(SG_context* pCtx, SG_safeptr* psafe, sg_zingdb** pp);

void SG_safeptr__wrap__zingrecord(SG_context* pCtx, SG_zingrecord* p, SG_safeptr** pp);
void SG_safeptr__unwrap__zingrecord(SG_context* pCtx, SG_safeptr* psafe, SG_zingrecord** pp);

void SG_safeptr__wrap__zinglinkcollection(SG_context* pCtx, sg_zinglinkcollection* p, SG_safeptr** pp);
void SG_safeptr__unwrap__zinglinkcollection(SG_context* pCtx, SG_safeptr* psafe, sg_zinglinkcollection** pp);

void SG_safeptr__wrap__xmlwriter(SG_context* pCtx, sg_xmlwriter* p, SG_safeptr** pp);
void SG_safeptr__unwrap__xmlwriter(SG_context* pCtx, SG_safeptr* psafe, sg_xmlwriter** pp);

void SG_safeptr__free(SG_context * pCtx, SG_safeptr* p);

void SG_safeptr__null(SG_context* pCtx, SG_safeptr* psp);

#define SG_SAFEPTR_NULLFREE(pCtx,p)              SG_STATEMENT( SG_context__push_level(pCtx);             SG_safeptr__free(pCtx, p);    SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )

END_EXTERN_C;

#endif//H_SG_SAFEPTR_H

