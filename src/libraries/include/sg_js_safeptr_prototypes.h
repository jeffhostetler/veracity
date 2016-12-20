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
 * @file sg_js_safeptr_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_JS_SAFEPTR_PROTOTYPES_H
#define H_SG_JS_SAFEPTR_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef struct _SG_safeptr         SG_js_safeptr;

//////////////////////////////////////////////////////////////////

void SG_js_safeptr__wrap__generic(SG_context * pCtx,
								  void * pVoidGeneric,
								  const char * pszType,
								  SG_js_safeptr ** ppSafePtr);

void SG_js_safeptr__unwrap__generic(SG_context * pCtx,
									SG_js_safeptr * pSafePtr,
									const char * pszType,
									void ** ppVoidGeneric);

//////////////////////////////////////////////////////////////////

void SG_js_safeptr__free(SG_context * pCtx, SG_js_safeptr* p);

#define SG_JS_SAFEPTR_NULLFREE(pCtx,p)  SG_STATEMENT( SG_context__push_level(pCtx); SG_js_safeptr__free(pCtx, p); SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_JS_SAFEPTR_PROTOTYPES_H
