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
 * @file sg_wc_tx__apply__private_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__APPLY__PRIVATE_PROTOTYPES_H
#define H_SG_WC_TX__APPLY__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_tx__run_apply(SG_context * pCtx, SG_wc_tx * pWcTx);

void sg_wc_tx__apply__add(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  const SG_vhash * pvh);

void sg_wc_tx__apply__move_rename(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_vhash * pvh);

void sg_wc_tx__apply__remove_directory(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   const SG_vhash * pvh);

void sg_wc_tx__apply__remove_file(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_vhash * pvh);

void sg_wc_tx__apply__remove_symlink(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__store_directory(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_vhash * pvh);

void sg_wc_tx__apply__store_file(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const SG_vhash * pvh);

void sg_wc_tx__apply__store_symlink(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__delete_tne(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const SG_vhash * pvh);

void sg_wc_tx__apply__insert_tne(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const SG_vhash * pvh);

void sg_wc_tx__apply__delete_pc(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const SG_vhash * pvh);

void sg_wc_tx__apply__clean_pc_but_leave_sparse(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__add_special__file(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh);

void sg_wc_tx__apply__add_special__directory(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 const SG_vhash * pvh);

void sg_wc_tx__apply__add_special__symlink(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_vhash * pvh);

void sg_wc_tx__apply__overwrite_file_from_file(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const SG_vhash * pvh);

void sg_wc_tx__apply__overwrite_file_from_repo(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const SG_vhash * pvh);

void sg_wc_tx__apply__overwrite_symlink(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh);

void sg_wc_tx__apply__set_attrbits(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__undo_delete__file(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh);

void sg_wc_tx__apply__undo_delete__directory(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 const SG_vhash * pvh);

void sg_wc_tx__apply__undo_delete__symlink(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__undo_lost__file(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_vhash * pvh);

void sg_wc_tx__apply__undo_lost__directory(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_vhash * pvh);

void sg_wc_tx__apply__undo_lost__symlink(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__resolve_issue__s(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   const SG_vhash * pvh);

void sg_wc_tx__apply__resolve_issue__sr(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_vhash * pvh);

void sg_wc_tx__apply__insert_issue(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_vhash * pvh);

void sg_wc_tx__apply__delete_issue(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__apply__kill_pc_row(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_vhash * pvh);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__APPLY__PRIVATE_PROTOTYPES_H
