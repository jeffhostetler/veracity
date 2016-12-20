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

#ifndef H_SG_WC_TX__QUEUE__PRIVATE_PROTOTYPES_H
#define H_SG_WC_TX__QUEUE__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__append__insert_fake_pc_stmt(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													const sg_wc_db__cset_row * pCSetRow,
													const sg_wc_db__pc_row * pPcRow);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__add(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const sg_wc_db__pc_row * pPcRow,
							const SG_string * pStringRepoPath);

void sg_wc_tx__journal__add_special(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									const char * pszHidBlob,
									SG_int64 attrbits);

void sg_wc_tx__journal__move_rename(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									const SG_string * pStringRepoPath_Src,
									const SG_string * pStringRepoPath_Dest,
									SG_bool bAfterTheFact,
									SG_bool bUseIntermediate);

void sg_wc_tx__journal__remove(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   const sg_wc_db__pc_row * pPcRow,
							   SG_treenode_entry_type tneType,
							   const SG_string * pStringRepoPath,
							   SG_bool bIsUncontrolled,
							   const char * pszDisposition,
							   const SG_pathname * pPathBackup);

void sg_wc_tx__journal__undo_add(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const sg_wc_db__pc_row * pPcRow,
								 SG_treenode_entry_type tneType,
								 const SG_string * pStringRepoPath,
								 const char * pszDisposition);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__store_blob(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const char * pszKey,
								   const char * pszRepoPath,
								   sg_wc_liveview_item * pLVI,
								   SG_int64 alias_gid,
								   const char * pszHid,
								   SG_uint64 sizeContent);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__commit__delete_tne(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_string * pStringRepoPath,
										   SG_int64 alias_gid);

void sg_wc_tx__journal__commit__insert_tne(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const SG_string * pStringRepoPath,
										   SG_int64 alias_gid,
										   SG_int64 alias_gid_parent,
										   const SG_treenode_entry * pTNE);

void sg_wc_tx__journal__commit__delete_pc(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  const SG_string * pStringRepoPath,
										  SG_int64 alias_gid);

void sg_wc_tx__journal__commit__clean_pc_but_leave_sparse(SG_context * pCtx,
														  SG_wc_tx * pWcTx,
														  const SG_string * pStringRepoPath,
														  SG_int64 alias_gid);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__journal__overwrite_file_from_file(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI,
												 const SG_string * pStringRepoPath,
												 const char * pszGid,
												 const SG_pathname * pPathTemp,
												 SG_int64 attrbits);

void sg_wc_tx__journal__overwrite_file_from_repo(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI,
												 const SG_string * pStringRepoPath,
												 const char * pszGid,
												 const char * pszHidBlob,
												 SG_int64 attrbits,
												 const SG_pathname * pPathBackup);

void sg_wc_tx__journal__overwrite_symlink(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  sg_wc_liveview_item * pLVI,
										  const SG_string * pStringRepoPath,
										  const char * pszGid,
										  const char * pszNewTarget,
										  SG_int64 attrbits,
										  const char * pszHidBlob);

void sg_wc_tx__journal__overwrite_symlink_from_repo(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_liveview_item * pLVI,
													const SG_string * pStringRepoPath,
													const char * pszGid,
													SG_int64 attrbits,
													const char * pszHidBlob);

void sg_wc_tx__journal__set_attrbits(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 sg_wc_liveview_item * pLVI,
									 const SG_string * pStringRepoPath,
									 SG_int64 attrbits);

void sg_wc_tx__journal__undo_delete(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									const SG_string * pStringRepoPath_Dest,
									const char * pszHidBlob,
									SG_int64 attrbits,
									const SG_string * pStringRepoPathTempFile);

void sg_wc_tx__journal__undo_lost(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  sg_wc_liveview_item * pLVI,
								  const SG_string * pStringRepoPath_Dest,
								  const char * pszHidBlob,
								  SG_int64 attrbits,
								  const SG_string * pStringRepoPathTempFile);

void sg_wc_tx__journal__resolve_issue__sr(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  SG_uint64 uiAliasGid,
										  SG_wc_status_flags statusFlags_x_xr_xu,
										  const SG_string * pStringResolve);

void sg_wc_tx__journal__resolve_issue__s(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 SG_uint64 uiAliasGid,
										 SG_wc_status_flags statusFlags_x_xr_xu);

void sg_wc_tx__journal__insert_issue(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 const SG_string * pStringRepoPath,
									 SG_uint64 uiAliasGid,
									 SG_wc_status_flags statusFlags_x_xr_xu,	// the initial __X__, __XR__, and __XU__ flags.
									 const SG_string * pStringIssue,
									 const SG_string * pStringResolve);

void sg_wc_tx__journal__delete_issue(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 SG_uint64 uiAliasGid,
									 const char * pszGid,
									 const char * pszRepoPathTempDir);

void sg_wc_tx__journal__kill_pc_row(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									SG_uint64 uiAliasGid);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__queue__add(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  sg_wc_liveview_item * pLVI);

void sg_wc_tx__queue__add_special(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_string * pStringLiveRepoPath_Parent,
								  sg_wc_liveview_dir * pLVD_Parent,
								  const char * pszEntryname,
								  SG_uint64 uiAliasGid,
								  SG_treenode_entry_type tneType,
								  const char * pszHidBlob,
								  SG_int64 attrbits,
								  SG_wc_status_flags statusFlagsAddSpecialReason);

void sg_wc_tx__queue__move_rename__after_the_fact(SG_context * pCtx,
												  SG_wc_tx * pWcTx,
												  const SG_string * pStringLiveRepoPath_Src,
												  const SG_string * pStringLiveRepoPath_Dest,
												  sg_wc_liveview_item * pLVI_Src,
												  sg_wc_liveview_item * pLVI_Dest,
												  SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__move_rename__normal(SG_context * pCtx,
										  SG_wc_tx * pWcTx,
										  const SG_string * pStringLiveRepoPath_Src,
										  const SG_string * pStringLiveRepoPath_Dest,
										  sg_wc_liveview_item * pLVI,
										  sg_wc_liveview_dir * pLVD_NewParent,
										  const char * pszNewEntryname,
										  SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__remove(SG_context * pCtx,
							 SG_wc_tx * pWcTx,
							 sg_wc_liveview_item * pLVI,
							 const char * pszDisposition,
							 SG_bool bFlattenAddSpecialPlusRemove,
							 SG_bool * pbKept);

void sg_wc_tx__queue__undo_add(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   sg_wc_liveview_item * pLVI,
							   const char * pszDisposition,
							   SG_bool * pbKept);

void sg_wc_tx__queue__overwrite_file_from_file(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const SG_string * pStringRepoPath,
											   sg_wc_liveview_item * pLVI,
											   const SG_pathname * pPathTemp,
											   SG_int64 attrbits,
											   SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__overwrite_file_from_repo(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   const SG_string * pStringRepoPath,
											   sg_wc_liveview_item * pLVI,
											   const char * pszHidBlob,
											   SG_int64 attrbits,
											   SG_bool bBackupFile,
											   SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__overwrite_symlink(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_string * pStringRepoPath,
										sg_wc_liveview_item * pLVI,
										const char * pszNewTarget,
										SG_int64 attrbits,
										SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__overwrite_symlink_from_repo(SG_context * pCtx,
												  SG_wc_tx * pWcTx,
												  const SG_string * pStringRepoPath,
												  sg_wc_liveview_item * pLVI,
												  SG_int64 attrbits,
												  const char * pszHidBlob,
												  SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__set_attrbits(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_string * pStringRepoPath,
								   sg_wc_liveview_item * pLVI,
								   SG_int64 attrbits,
								   SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__undo_delete(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_string * pStringLiveRepoPath_Dest,	// where we will restore it to
								  sg_wc_liveview_item * pLVI,
								  sg_wc_liveview_item * pLVI_DestDir,
								  const char * pszNewEntryname,
								  const SG_wc_undo_delete_args * pArgs,
								  SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__undo_lost(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const SG_string * pStringLiveRepoPath_Dest,	// where we will restore it to
								sg_wc_liveview_item * pLVI,
								sg_wc_liveview_dir * pLVD_NewParent,
								const char * pszNewEntryname,
								const SG_wc_undo_delete_args * pArgs,
								SG_wc_status_flags xu_mask);

void sg_wc_tx__queue__resolve_issue__sr(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									sg_wc_liveview_item * pLVI,
									SG_wc_status_flags statusFlags_x_xr_xu,
									SG_vhash ** ppvhResolve);

void sg_wc_tx__queue__resolve_issue__s(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   sg_wc_liveview_item * pLVI,
									   SG_wc_status_flags statusFlags_x_xr_xu);

void sg_wc_tx__queue__insert_issue(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   sg_wc_liveview_item * pLVI,
								   SG_wc_status_flags statusFlags_x_xr_xu,	// the initial __X__, __XR__, and __XU__ flags.
								   SG_vhash ** ppvhIssue,
								   SG_vhash ** ppvhResolve);

void sg_wc_tx__queue__delete_issue(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   sg_wc_liveview_item * pLVI);

void sg_wc_tx__queue__delete_issue2(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									SG_uint64 uiAliasGid,
									const SG_vhash * pvhIssue);

void sg_wc_tx__queue__kill_pc_row(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  SG_uint64 uiAliasGid);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__QUEUE__PRIVATE_PROTOTYPES_H
