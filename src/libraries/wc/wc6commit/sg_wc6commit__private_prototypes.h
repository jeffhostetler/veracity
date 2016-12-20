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
 * @file sg_wc6commit__private_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC6COMMIT__PRIVATE_PROTOTYPES_H
#define H_SG_WC6COMMIT__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

void sg_wc_tx__commit__mark_subtree(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									const SG_wc_commit_args * pCommitArgs);

void sg_wc_tx__commit__mark_bubble_up(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_wc_commit_args * pCommitArgs);

#if defined(DEBUG)
void sg_wc_tx__commit__dump_marks(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const char * pszLabel);
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__commit__mark_subtree__lvi(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 sg_wc_liveview_item * pLVI,
											 SG_uint32 depth);

void sg_wc_tx__rp__commit__mark_bubble_up__lvi(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   sg_wc_liveview_item * pLVI);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__commit__queue(SG_context * pCtx,
							 sg_commit_data * pCommitData);

void sg_wc_tx__commit__queue__lvi(SG_context * pCtx,
								  sg_commit_data * pCommitData,
								  sg_wc_liveview_item * pLVI);

void sg_wc_tx__commit__queue__blob__file(SG_context * pCtx,
										 sg_commit_data * pCommitData,
										 const SG_string * pStringRepoPath,
										 sg_wc_liveview_item * pLVI);

void sg_wc_tx__commit__queue__blob__symlink(SG_context * pCtx,
											sg_commit_data * pCommitData,
											const SG_string * pStringRepoPath,
											sg_wc_liveview_item * pLVI);

void sg_wc_tx__commit__queue__blob__dir_contents_participating(SG_context * pCtx,
															   sg_commit_data * pCommitData,
															   const SG_string * pStringRepoPath,
															   sg_wc_liveview_item * pLVI);

void sg_wc_tx__commit__queue__blob__dir_superroot(SG_context * pCtx,
												  sg_commit_data * pCommitData);

void sg_wc_tx__commit__queue__blob__bubble_up_directory(SG_context * pCtx,
														sg_commit_data * pCommitData,
														const SG_string * pStringRepoPath,
														sg_wc_liveview_item * pLVI);

void sg_wc_tx__commit__queue__blob__dir_non_recursive(SG_context * pCtx,
													  sg_commit_data * pCommitData,
													  const SG_string * pStringRepoPath,
													  sg_wc_liveview_item * pLVI);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__commit__queue__utils__add_tne(SG_context * pCtx,
											 SG_treenode * pTN,
											 const char * pszGid,
											 const char * pszEntryname,
											 SG_treenode_entry_type tneType,
											 const char * pszHidContent,
											 SG_int64 attrbits,
											 const SG_treenode_entry ** ppTNE_ref);

void sg_wc_tx__commit__queue__utils__check_port(SG_context * pCtx,
												SG_wc_port * pPort,
												const char * pszEntryname,
												SG_treenode_entry_type tneType,
												const SG_string * pStringRepoPath);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__commit__apply(SG_context * pCtx,
							 sg_commit_data * pCommitData);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6COMMIT__PRIVATE_PROTOTYPES_H
