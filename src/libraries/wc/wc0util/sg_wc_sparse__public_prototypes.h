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
 * @file sg_wc_sparse__public_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_SPARSE__PUBLIC_PROTOTYPES_H
#define H_SG_WC_SPARSE__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_wc_sparse__build_pattern_list(SG_context * pCtx,
									  SG_repo * pRepo,
									  const char * pszHidCSet,
									  const SG_varray * pvaSparse,
									  SG_file_spec ** ppFilespec);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_SPARSE__PUBLIC_PROTOTYPES_H
