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

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC6STATUS1__PRIVATE_PROTOTYPES_H
#define H_SG_WC6STATUS1__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_tx__status1__data__free(SG_context * pCtx,
								   sg_wc_tx__status1__data * pS1Data);

#define SG_WC_TX__STATUS1__DATA__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_tx__status1__data__free)

void sg_wc_tx__status1__setup(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  const SG_rev_spec * pRevSpec,
							  SG_bool bListUnchanged,
							  SG_bool bNoIgnores,
							  SG_bool bNoTSC,
							  SG_bool bListSparse,
							  SG_bool bListReserved,
							  sg_wc_tx__status1__data ** ppS1Data);

#if defined(DEBUG)
void sg_wc_tx__status1_debug__dump(SG_context * pCtx,
								   sg_wc_tx__status1__data * pS1Data);
#endif

void sg_wc_tx__status1__h(SG_context * pCtx,
						  sg_wc_tx__status1__data * pS1Data,
						  const SG_string * pStringRepoPath,
						  SG_uint32 depth,
						  SG_varray * pvaStatus);

void sg_wc_tx__status1__wc(SG_context * pCtx,
						   sg_wc_tx__status1__data * pS1Data,
						   const SG_string * pStringRepoPath,
						   SG_uint32 depth,
						   SG_varray * pvaStatus);

void sg_wc_tx__status1__g(SG_context * pCtx,
						  sg_wc_tx__status1__data * pS1Data,
						  const char * pszGid,
						  SG_uint32 depth,
						  SG_varray * pvaStatus);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6STATUS1__PRIVATE_PROTOTYPES_H
