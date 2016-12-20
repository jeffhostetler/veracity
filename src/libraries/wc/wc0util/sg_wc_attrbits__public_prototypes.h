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
 * @file sg_wc_attrbits__public_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_ATTRBITS__PUBLIC_PROTOTYPES_H
#define H_SG_WC_ATTRBITS__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_wc_attrbits__get_masks_from_config(SG_context * pCtx,
										   SG_repo * pRepo,
										   const SG_pathname * pPathWorkingDirectoryTop,
										   SG_wc_attrbits_data ** ppAttrbitsData);

void SG_wc_attrbits_data__free(SG_context * pCtx,
							   SG_wc_attrbits_data * pAttrbitsData);

#define SG_WC_ATTRBITS_DATA__NULLFREE(pCtx, p) _sg_generic_nullfree(pCtx,p,SG_wc_attrbits_data__free)

void SG_wc_attrbits__compute_perms_for_new_file_from_attrbits(SG_context * pCtx,
															  const SG_wc_attrbits_data * pAttrbitsData,
															  SG_uint64 attrbits,
															  SG_fsobj_perms * pPerms);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_ATTRBITS__PUBLIC_PROTOTYPES_H
