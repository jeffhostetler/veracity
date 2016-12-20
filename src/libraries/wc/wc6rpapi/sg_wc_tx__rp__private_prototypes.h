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
 * @file sg_wc_tx__rp__private_prototypes.h
 *
 * @details This routines in this API layer mirror the public
 * ones, but take pre-validated repo-paths.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RP__PRIVATE_PROTOTYPES_H
#define H_SG_WC_TX__RP__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__add__lvi(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							sg_wc_liveview_item * pLVI,
							SG_uint32 depth,
							SG_bool bNoIgnores);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__status__lvi(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   sg_wc_liveview_item * pLVI,
							   SG_uint32 depth,
							   SG_bool bListUnchanged,
							   SG_bool bNoIgnores,
							   SG_bool bNoTSC,
							   SG_bool bListSparse,
							   SG_bool bListReserved,
							   const char * pszWasLabel_l,
							   const char * pszWasLabel_r,
							   SG_varray * pvaStatus);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__move_rename__lvi_lvi(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI_Src,
										sg_wc_liveview_item * pLVI_Dest,
										SG_bool bNoAllowAfterTheFact,
										SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__move_rename__lvi_lvi_sz(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   sg_wc_liveview_item * pLVI_Src,
										   sg_wc_liveview_item * pLVI_DestDir,
										   const char * pszNewEntryname,
										   SG_bool bNoAllowAfterTheFact,
										   SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__move_rename__lookup_destdir(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const char * pszInput_DestDir,
											   const SG_string * pStringRepoPath_DestDir,
											   sg_wc_liveview_item ** ppLVI_DestDir);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__remove__lvi(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   sg_wc_liveview_item * pLVI,
							   SG_uint32 level,
							   SG_bool bNonRecursive,
							   SG_bool bForce,
							   SG_bool bNoBackups,
							   SG_bool bKeepInWC,
							   SG_bool bFlattenAddSpecialPlusRemove,
							   SG_bool * pbKept);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__overwrite_file_from_file__lvi(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI_Src,
												 const SG_pathname * pPathTemp,
												 SG_int64 attrbits,
												 SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__overwrite_file_from_file2__lvi(SG_context * pCtx,
												  SG_wc_tx * pWcTx,
												  sg_wc_liveview_item * pLVI_Src,
												  const SG_pathname * pPathTemp,
												  SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__overwrite_file_from_repo__lvi(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI_Src,
												 const char * pszHidBlob,
												 SG_int64 attrbits,
												 SG_bool bBackupFile,
												 SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__overwrite_symlink__lvi(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  sg_wc_liveview_item * pLVI_Src,
										  const char * pszNewTarget,
										  SG_int64 attrbits,
										  SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__overwrite_symlink_from_repo__lvi(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_liveview_item * pLVI_Src,
													SG_int64 attrbits,
													const char * pszHidBlob,
													SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__set_attrbits__lvi(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 sg_wc_liveview_item * pLVI_Src,
									 SG_int64 attrbits,
									 SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__add_special__lvi_sz(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   sg_wc_liveview_item * pLVI_Parent,
									   const char * pszEntryname,
									   SG_uint64 uiAliasGid,
									   SG_treenode_entry_type tneType,
									   const char * pszHidBlob,
									   SG_int64 attrbits,
									   SG_wc_status_flags statusFlagsAddSpecialReason);

void sg_wc_tx__rp__undo_delete__lvi_lvi_sz(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   sg_wc_liveview_item * pLVI_Src,
										   sg_wc_liveview_item * pLVI_DestDir,
										   const char * pszEntryname_Dest,
										   const SG_wc_undo_delete_args * pArgs,
										   SG_wc_status_flags xu_mask);

void sg_wc_tx__rp__undo_lost__lvi_lvi_sz(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 sg_wc_liveview_item * pLVI_Src,
										 sg_wc_liveview_item * pLVI_DestDir,
										 const char * pszEntryname_Dest,
										 const SG_wc_undo_delete_args * pArgs,
										 SG_wc_status_flags xu_mask);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RP__PRIVATE_PROTOTYPES_H
