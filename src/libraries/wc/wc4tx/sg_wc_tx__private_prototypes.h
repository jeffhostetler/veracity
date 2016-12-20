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

#ifndef H_SG_WC_TX__PRIVATE_PROTOTYPES_H
#define H_SG_WC_TX__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_tx__cache__add_prescan_dir(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_prescan_dir * pPrescanDir);

void sg_wc_tx__cache__find_prescan_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   SG_uint64 uiAliasGid,
									   SG_bool * pbFound,
									   sg_wc_prescan_dir ** ppPrescanDir);

void sg_wc_tx__cache__add_prescan_row(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_prescan_row * pPrescanRow);

void sg_wc_tx__cache__find_prescan_row(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   SG_uint64 uiAliasGid,
									   SG_bool * pbFound,
									   sg_wc_prescan_row ** ppPrescanRow);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__cache__add_liveview_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   sg_wc_liveview_dir * pLVD);

void sg_wc_tx__cache__find_liveview_dir(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										SG_uint64 uiAliasGid,
										SG_bool * pbFound,
										sg_wc_liveview_dir ** ppLVD);

void sg_wc_tx__cache__add_liveview_item(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI);

void sg_wc_tx__cache__find_liveview_item(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 SG_uint64 uiAliasGid,
										 SG_bool * pbFound,
										 sg_wc_liveview_item ** ppLVI);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__prescan__fetch_row(SG_context * pCtx, SG_wc_tx * pWcTx,
								  SG_uint64 uiAliasGid,
								  sg_wc_prescan_row ** ppPrescanRow);

void sg_wc_tx__prescan__fetch_dir(SG_context * pCtx, SG_wc_tx * pWcTx,
								  SG_uint64 uiAliasGid,
								  sg_wc_prescan_dir ** ppPrescanDir);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__liveview__fetch_item(SG_context * pCtx, SG_wc_tx * pWcTx,
									const SG_string * pStringLiveRepoPath,
									SG_bool * pbKnown,
									sg_wc_liveview_item ** ppLVI);

void sg_wc_tx__liveview__fetch_dir(SG_context * pCtx, SG_wc_tx * pWcTx,
								   const sg_wc_liveview_item * pLVI,
								   sg_wc_liveview_dir ** ppLVD);

void sg_wc_tx__liveview__fetch_random_item(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   SG_uint64 uiAliasGid,
										   SG_bool * pbKnown,
										   sg_wc_liveview_item ** ppLVI);

void sg_wc_tx__liveview__compute_live_repo_path(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												sg_wc_liveview_item * pLVI,
												SG_string ** ppStringLiveRepoPath);

void sg_wc_tx__liveview__compute_baseline_repo_path(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_liveview_item * pLVI,
													const SG_string ** ppStringBaselineRepoPath);

void sg_wc_tx__liveview__fetch_item__domain(SG_context * pCtx, SG_wc_tx * pWcTx,
											const SG_string * pStringRepoPath,
											SG_bool * pbKnown,
											sg_wc_liveview_item ** ppLVI);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__create_session_temp_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__PRIVATE_PROTOTYPES_H
