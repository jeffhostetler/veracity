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

#ifndef H_SG_WC_PORT__PRIVATE_PROTOTYPES_H
#define H_SG_WC_PORT__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void _sg_wc_port_item__free(SG_context* pCtx, sg_wc_port_item * pItem);

void _sg_wc_port_item__alloc(SG_context * pCtx,
							 const char * pszGid_Entry,			// optional, for logging.
							 const char * szEntryNameAsGiven,
							 SG_treenode_entry_type tneType,
							 SG_utf8_converter * pConverterRepoCharSet,
							 void * pVoidAssocData,
							 sg_wc_port_item ** ppItem);

//////////////////////////////////////////////////////////////////

void _sg_wc_port_collider_assoc_data__free(SG_context * pCtx, sg_wc_port_collider_assoc_data * pData);

void _sg_wc_port_collider_assoc_data__alloc(SG_context * pCtx,
											SG_wc_port_flags why,
											sg_wc_port_item * pItem,
											sg_wc_port_collider_assoc_data ** ppData);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PORT__PRIVATE_PROTOTYPES_H
