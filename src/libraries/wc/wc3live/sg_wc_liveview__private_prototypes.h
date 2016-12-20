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
 * @file sg_wc_liveview__private_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_LIVEVIEW__PRIVATE_PROTOTYPES_H
#define H_SG_WC_LIVEVIEW__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_dir__free(SG_context * pCtx, sg_wc_liveview_dir * pLVD);

#define SG_WC_LIVEVIEW_DIR__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_liveview_dir__free)

void sg_wc_liveview_dir__alloc__clone_from_prescan(SG_context * pCtx,
												   SG_wc_tx * pWcTx,
												   sg_wc_prescan_dir * pPrescanDir,
												   sg_wc_liveview_dir ** ppLVD);

void sg_wc_liveview_dir__alloc__add_special(SG_context * pCtx,
											SG_wc_tx * pWcTx,
											SG_uint64 uiAliasGid,
											sg_wc_liveview_dir ** ppLVD);

#if defined(DEBUG)

#define SG_WC_LIVEVIEW_DIR__ALLOC__CLONE_FROM_PRESCAN(pCtx,pWcTx,pPrescan,ppNew) \
	SG_STATEMENT(														\
		sg_wc_liveview_dir * _pNew = NULL;								\
		sg_wc_liveview_dir__alloc__clone_from_prescan(pCtx,pWcTx,pPrescan,&_pNew); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_liveview_dir"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_LIVEVIEW_DIR__ALLOC__ADD_SPECIAL(pCtx,pWcTx,ui,ppNew)	\
	SG_STATEMENT(														\
		sg_wc_liveview_dir * _pNew = NULL;								\
		sg_wc_liveview_dir__alloc__add_special(pCtx,pWcTx,ui,&_pNew); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_liveview_dir"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_LIVEVIEW_DIR__ALLOC__CLONE_FROM_PRESCAN(pCtx,pWcTx,pPrescan,ppNew) \
	sg_wc_liveview_dir__alloc__clone_from_prescan(pCtx,pWcTx,pPrescan,ppNew)

#define SG_WC_LIVEVIEW_DIR__ALLOC__ADD_SPECIAL(pCtx,pWcTx,ui,ppNew)	\
	sg_wc_liveview_dir__alloc__add_special(pCtx,pWcTx,ui,ppNew)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_dir__foreach(SG_context * pCtx,
								 sg_wc_liveview_dir * pLVD,
								 SG_rbtree_ui64_foreach_callback * pfn_cb,
								 void * pVoidData);

void sg_wc_liveview_dir__foreach_moved_out(SG_context * pCtx,
										   sg_wc_liveview_dir * pLVD,
										   SG_rbtree_ui64_foreach_callback * pfn_cb,
										   void * pVoidData);

void sg_wc_liveview_dir__can_add_new_entryname(SG_context * pCtx,
											   sg_wc_db * pDb,
											   sg_wc_liveview_dir * pLVD,
											   sg_wc_liveview_item * pLVI_Old, // optional
											   sg_wc_liveview_item * pLVI_New, // optional
											   const char * pszNewEntryname,
											   SG_treenode_entry_type tneType,
											   SG_bool bDisregardUncontrolledItems);

void sg_wc_liveview_dir__find_item_by_entryname(SG_context * pCtx,
												const sg_wc_liveview_dir * pLVD,
												const char * pszEntryname,
												sg_wc_liveview_dir__find_flags findFlags,
												SG_vector ** ppVecItemsFound);

void sg_wc_liveview_dir__find_item_by_alias(SG_context * pCtx,
											const sg_wc_liveview_dir * pLVD,
											SG_uint64 uiAliasGid,
											SG_bool * pbFound,
											sg_wc_liveview_item ** ppLVI);

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__free(SG_context * pCtx,
							   sg_wc_liveview_item * pLVI);

#define SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_liveview_item__free)

void sg_wc_liveview_item__alloc__clone_from_prescan(SG_context * pCtx,
													sg_wc_liveview_item ** ppLVI,
													SG_wc_tx * pWcTx,
													const sg_wc_prescan_row * pPrescanRow);

void sg_wc_liveview_item__alloc__add_special(SG_context * pCtx,
											 sg_wc_liveview_item ** ppLVI,
											 SG_wc_tx * pWcTx,
											 SG_uint64 uiAliasGid,
											 SG_uint64 uiAliasGidParent,
											 const char * pszEntryname,
											 SG_treenode_entry_type tneType,
											 const char * pszHidMerge,
											 SG_int64 attrbits,
											 SG_wc_status_flags statusFlagsAddSpecialReason);

void sg_wc_liveview_item__alloc__root_item(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   sg_wc_liveview_item ** ppLVI);

#if defined(DEBUG)

#define SG_WC_LIVEVIEW_ITEM__ALLOC__CLONE_FROM_PRESCAN(pCtx,ppNew,pWcTx,pPrescanRow) \
	SG_STATEMENT(														\
		sg_wc_liveview_item * _pNew = NULL;								\
		sg_wc_liveview_item__alloc__clone_from_prescan(pCtx,&_pNew,pWcTx,pPrescanRow); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_liveview_item"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_LIVEVIEW_ITEM__ALLOC__ADD_SPECIAL(pCtx,ppNew,pWcTx,uiAliasGid,uiAliasGidParent,pszEntryname,tneType,pszHidMerge,attrbits,sf) \
	SG_STATEMENT(														\
		sg_wc_liveview_item * _pNew = NULL;								\
		sg_wc_liveview_item__alloc__add_special(pCtx,&_pNew,pWcTx,uiAliasGid,uiAliasGidParent,pszEntryname,tneType,pszHidMerge,attrbits,sf); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_liveview_item"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_LIVEVIEW_ITEM__ALLOC__ROOT_ITEM(pCtx,pWcTx,ppNew)			\
	SG_STATEMENT(														\
		sg_wc_liveview_item * _pNew = NULL;								\
		sg_wc_liveview_item__alloc__root_item(pCtx,pWcTx,&_pNew);		\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_liveview_item"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_LIVEVIEW_ITEM__ALLOC__CLONE_FROM_PRESCAN(pCtx,ppNew,pWcTx,pPrescanRow) \
	sg_wc_liveview_item__alloc__clone_from_prescan(pCtx,ppNew,pWcTx,pPrescanRow)

#define SG_WC_LIVEVIEW_ITEM__ALLOC__ADD_SPECIAL(pCtx,ppNew,pWcTx,uiAliasGid,uiAliasGidParent,pszEntryname,tneType,pszHidMerge,attrbits,sf) \
	sg_wc_liveview_item__alloc__add_special(pCtx,ppNew,pWcTx,uiAliasGid,uiAliasGidParent,pszEntryname,tneType,pszHidMerge,attrbits,sf)

#define SG_WC_LIVEVIEW_ITEM__ALLOC__ROOT_ITEM(pCtx,pWcTx,ppNew)	\
	sg_wc_liveview_item__alloc__root_item(pCtx,pWcTx,ppNew)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__synthesize_replacement_row(SG_context * pCtx,
													 sg_wc_liveview_item ** ppLVI_Replacement,
													 SG_wc_tx * pWcTx,
													 const sg_wc_liveview_item * pLVI_Input);

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__get_original_entryname(SG_context * pCtx,
												 const sg_wc_liveview_item * pLVI,
												 const char ** ppszEntryname);

void sg_wc_liveview_item__get_original_parent_alias(SG_context * pCtx,
													const sg_wc_liveview_item * pLVI,
													SG_uint64 * puiAliasGidParent);

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__get_original_attrbits(SG_context * pCtx,
												sg_wc_liveview_item * pLVI,
												SG_wc_tx * pWcTx,
												SG_uint64 * pAttrbits);

void sg_wc_liveview_item__get_current_attrbits(SG_context * pCtx,
											   sg_wc_liveview_item * pLVI,
											   SG_wc_tx * pWcTx,
											   SG_uint64 * pAttrbits);


void sg_wc_liveview_item__get_original_content_hid(SG_context * pCtx,
												   sg_wc_liveview_item * pLVI,
												   SG_wc_tx * pWcTx,
												   SG_bool bNoTSC,
												   const char ** ppszHidContent);

void sg_wc_liveview_item__get_current_content_hid(SG_context * pCtx,
												  sg_wc_liveview_item * pLVI,
												  SG_wc_tx * pWcTx,
												  SG_bool bNoTSC,
												  char ** ppszHidContent,
												  SG_uint64 * pSize);

void sg_wc_liveview_item__get_original_symlink_target(SG_context * pCtx,
													  sg_wc_liveview_item * pLVI,
													  SG_wc_tx * pWcTx,
													  SG_string ** ppStringTarget);

void sg_wc_liveview_item__get_current_symlink_target(SG_context * pCtx,
													 sg_wc_liveview_item * pLVI,
													 SG_wc_tx * pWcTx,
													 SG_string ** ppStringTarget);

void sg_wc_liveview_item__get_flags_net(SG_context * pCtx,
										const sg_wc_liveview_item * pLVI,
										sg_wc_db__pc_row__flags_net * pFlagsNet);

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__alter_structure__add(SG_context * pCtx,
											   sg_wc_liveview_item * pLVI);

void sg_wc_liveview_dir__add_special(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 sg_wc_liveview_dir * pLVD_Parent,
									 const char * pszEntryname,
									 SG_uint64 uiAliasGid,
									 SG_treenode_entry_type tneType,
									 const char * pszHidMerge,
									 SG_int64 attrbits,
									 SG_wc_status_flags statusFlagsAddSpecialReason,
									 sg_wc_liveview_item ** ppLVI_New);

void sg_wc_liveview_item__alter_structure__move_rename(SG_context * pCtx,
													   sg_wc_liveview_item * pLVI,
													   sg_wc_liveview_dir * pLVD_NewParent,
													   const char * pszNewEntryname);

void sg_wc_liveview_item__alter_structure__remove(SG_context * pCtx,
												  sg_wc_liveview_item * pLVI,
												  SG_bool bFlattenAddSpecialPlusRemove);

void sg_wc_liveview_item__alter_structure__undo_add(SG_context * pCtx,
													sg_wc_liveview_item * pLVI);

void sg_wc_liveview_item__alter_structure__undo_delete(SG_context * pCtx,
													   sg_wc_liveview_item * pLVI,
													   sg_wc_liveview_dir * pLVD_NewParent,
													   const char * pszNewEntryname);

void sg_wc_liveview_item__alter_structure__undo_lost(SG_context * pCtx,
													 sg_wc_liveview_item * pLVI,
													 sg_wc_liveview_dir * pLVD_NewParent,
													 const char * pszNewEntryname);

void sg_wc_liveview_item__alter_structure__overwrite_sparse_fields(SG_context * pCtx,
																   sg_wc_liveview_item * pLVI,
																   const char * pszHid,
																   SG_int64 attrbits);

void sg_wc_liveview_item__alter_structure__overwrite_ref_attrbits(SG_context * pCtx,
																  sg_wc_liveview_item * pLVI,
																  SG_int64 attrbits);

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__remember_last_overwrite(SG_context * pCtx,
												  sg_wc_liveview_item * pLVI,
												  SG_vhash * pvhContent,
												  SG_vhash * pvhAttrbits);

void sg_wc_liveview_item__get_proxy_file_path(SG_context * pCtx,
											  sg_wc_liveview_item * pLVI,
											  SG_wc_tx * pWcTx,
											  SG_pathname ** ppPath,
											  SG_bool * pbIsTmp);

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__set_alternate_baseline_during_update(SG_context * pCtx,
															   SG_wc_tx * pWcTx,
															   sg_wc_liveview_item * pLVI);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_LIVEVIEW__PRIVATE_PROTOTYPES_H
