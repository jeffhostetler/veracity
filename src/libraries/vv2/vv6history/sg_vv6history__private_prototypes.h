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

#ifndef H_SG_VV6HISTORY__PRIVATE_PROTOTYPES_H
#define H_SG_VV6HISTORY__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_vv2__history__core(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_stringarray * pStringArrayGIDs,
	SG_stringarray * pStringArrayStartingChangesets, 
	SG_stringarray * pStringArrayStartingChangesets_single_revisions,
	const char* pszUser,
	const char* pszStamp,
	SG_uint32 nResultLimit,
	SG_bool bLeaves,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bRecommendDagWalk,
	SG_bool* pbHasResult,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken);

void sg_vv2__history__get_starting_changesets(
	SG_context * pCtx,
	SG_repo * pRepo,
	const SG_rev_spec* pRevSpec,
	SG_stringarray ** ppsaHidChangesets,
	SG_stringarray ** ppsaHidChangesetsMissing,
	SG_bool * pbRecommendDagWalk,
	SG_bool * pbLeaves);

void sg_vv2__history__lookup_gids_by_repopaths(
	SG_context * pCtx,
	SG_repo * pRepo,
	const char * psz_dagnode_hid,
	const SG_stringarray * psaArgs,    // If present these must be full repo-paths.
	SG_stringarray ** ppStringArrayGIDs);

//////////////////////////////////////////////////////////////////

void sg_vv2__history__repo2(
	SG_context * pCtx,
	SG_repo * pRepo, 
	const SG_stringarray * psaArgs,
	const SG_rev_spec* pRevSpec, // optional
	const SG_rev_spec* pRevSpec_single_revisions, // optional
	const char* pszUser,
	const char* pszStamp,
	SG_uint32 nResultLimit,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bListAll,
	SG_bool bReassembleDag,
	SG_bool* pbHasResult,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken);

void sg_vv2__history__repo(
	SG_context * pCtx,
	const char * pszRepoDescriptorName,
	const SG_stringarray * psaArgs,
	const SG_rev_spec* pRevSpec, // optional
	const SG_rev_spec* pRevSpec_single_revisions, // optional
	const char* pszUser,
	const char* pszStamp,
	SG_uint32 nResultLimit,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bListAll,
	SG_bool bReassembleDag,
	SG_bool* pbHasResult,
	SG_vhash** ppvhBranchPile,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken);

void sg_vv2__history__working_folder(
	SG_context * pCtx,
	const SG_stringarray * psaInputs,
	const SG_rev_spec* pRevSpec, // optional
	const SG_rev_spec* pRevSpec_single_revisions, // optional
	const char* pszUser,
	const char* pszStamp,
	SG_bool bDetectCurrentBranch,
	SG_uint32 nResultLimit,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bListAll,
	SG_bool* pbHasResult,
	SG_vhash** ppvhBranchPile,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken);

//////////////////////////////////////////////////////////////////

void sg_vv2__history__fetch_more__repo(
	SG_context * pCtx,
	const char * pszRepoDescriptorName,
	SG_history_token * pHistoryToken,
	SG_uint32 nResultLimit,
	SG_vhash ** ppvhBranchPile,
	SG_history_result ** ppHistoryResults,
	SG_history_token **ppNewToken);

void sg_vv2__history__fetch_more__repo2(
	SG_context * pCtx,
	SG_repo * pRepo, 
	SG_history_token * pHistoryToken,
	SG_uint32 nResultLimit,
	SG_history_result ** ppHistoryResults,
	SG_history_token **ppNewToken);

void sg_vv2__history__fetch_more__working_folder(
	SG_context * pCtx,
	SG_history_token * pHistoryToken,
	SG_uint32 nResultLimit,
	SG_vhash ** ppvhBranchPile,
	SG_history_result ** ppHistoryResults,
	SG_history_token **ppNewToken);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV6HISTORY__PRIVATE_PROTOTYPES_H
