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
 * @file sg_wc_attrbits__private_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_ATTRBITS__PRIVATE_PROTOTYPES_H
#define H_SG_WC_ATTRBITS__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_attrbits__compute_from_perms(SG_context * pCtx,
										const SG_wc_attrbits_data * pAttrbitsData,
										SG_treenode_entry_type tneType,
										SG_fsobj_perms perms,
										SG_uint64 * pAttrbitsResult);

void sg_wc_attrbits__compute_effective_attrbits(SG_context * pCtx,
												const SG_wc_attrbits_data * pAttrbitsData,
												SG_uint64 attrbitsBaseline,
												SG_uint64 attrbitsObserved,
												SG_uint64 * pAttrbitsEffective);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_ATTRBITS__PRIVATE_PROTOTYPES_H
