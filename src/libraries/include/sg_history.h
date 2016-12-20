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

/*
 * sg_history.h
 *
 *  Created on: Mar 23, 2010
 *      Author: jeremy
 */

#ifndef SG_HISTORY_H_
#define SG_HISTORY_H_

BEGIN_EXTERN_C

typedef struct _opaque_history_result SG_history_result;
typedef struct _opaque_history_token SG_history_token;

void SG_history_token__free(SG_context * pCtx, SG_history_token * pHistoryToken);

void SG_history__fetch_more(SG_context * pCtx, SG_repo * pRepo, SG_history_token * pHistoryToken, SG_uint32 nNumberOfResultsAtATime, SG_history_result ** ppResult,  SG_history_token **ppNewToken);

void SG_history__get_changeset_description(SG_context* pCtx, 
										   SG_repo* pRepo, 
										   const char* pszChangesetId,
										   SG_bool bChildren,
										   SG_vhash** pvhChangesetDescription
										   );

void SG_history__get_children_description(SG_context* pCtx, SG_repo* pRepo, const char* pChangesetHid, SG_varray** ppvaChildren);

void SG_history__get_changeset_comments(SG_context* pCtx, 
										SG_repo* pRepo, 
										const char* pszDagNodeHID, 
										SG_varray** ppvaComments);

void SG_history__get_revision_details(
		SG_context * pCtx, 
		SG_repo * pRepo, 
		SG_stringarray * pStringArrayChangesets_single_revisions, 
		SG_bool * pbHasResult, 
		SG_history_result ** ppResult);

void SG_history__run(
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
	SG_bool bReassembleDag,
	SG_bool* pbHasResult,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken);

/* Returns a history result serialized to json.  
 * The serialized data shouldn't be used directly: use SG_history__result__from_json
 * to get a SG_history_result, then use the SG_history__result_get routines.
 * This routine claims ownership of *ppResult and NULLs the caller's copy.
 */
void SG_history_result__to_json(SG_context* pCtx, SG_history_result** ppResult, SG_string* pStr);
void SG_history_result__from_json(SG_context* pCtx, const char* pszJson, SG_history_result** ppNew);

void SG_history_result__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piCount);
void SG_history_result__reverse(SG_context* pCtx, SG_history_result* pHistory);
void SG_history_result__next(SG_context* pCtx, SG_history_result* pHistory, SG_bool* pbOk);

void SG_history_result__set_index(SG_context * pCtx, SG_history_result * pHistory, SG_uint32 idx);
void SG_history_result__get_index(SG_context * pCtx, SG_history_result * pHistory, SG_uint32 * p_idx);

void SG_history_result__get_cshid(SG_context* pCtx, SG_history_result* pHistory, const char** pszHidRef);
void SG_history_result__get_revno(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piRevno);
void SG_history_result__get_generation(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piGen);

void SG_history_result__get_parent__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piParentCount);
void SG_history_result__get_parent(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 parentIndex, const char** ppszParentHidRef, SG_uint32* piRevno);


void SG_history_result__get_pseudo_parent__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piPseudoParentCount);
void SG_history_result__get_pseudo_parent(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 parentIndex, const char** ppszParentHidRef, SG_uint32* piRevno);


void SG_history_result__get_tag__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piTagCount);
void SG_history_result__get_tag__text(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 tagIndex, const char** ppszTagRef);

void SG_history_result__get_stamp__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piStampCount);
void SG_history_result__get_stamp__text(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 stampIndex, const char** ppszStampRef);

void SG_history_result__get_audit__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piAuditCount);
void SG_history_result__get_audit__who(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 auditIndex, const char** ppszWho);
void SG_history_result__get_audit__when(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 auditIndex, SG_int64* piWhen);

void SG_history_result__get_comment__count(SG_context* pCtx, SG_history_result* pHistory, SG_uint32* piCommentCount);
void SG_history_result__get_comment__text(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 commentIndex, const char** ppszComment);
void SG_history_result__get_comment__history__ref(SG_context* pCtx, SG_history_result* pHistory, SG_uint32 commentIndex, SG_varray ** ppvhCommentHistory);
void SG_history_result__get_first_comment__text(SG_context* pCtx, SG_history_result* pHistory, const char** ppszComment);

/* You shouldn't use this.  It's here for use by JSGlue */
void SG_history_result__get_root(SG_context* pCtx, SG_history_result* pHistory, SG_varray** ppvaRef);

void SG_history_result__free(SG_context* pCtx, SG_history_result* pResult);

#if defined(DEBUG)
void SG_history_debug__dump_result_to_console(SG_context* pCtx, SG_history_result* pResult);
#endif /* defined(DEBUG) */

// Fills in history details for a varray of vhashes each containing a "changeset_id" member.
void SG_history__fill_in_details(
        SG_context * pCtx,
        SG_repo * pRepo,
        SG_varray* pva_history
        );

END_EXTERN_C
#endif /* SG_HISTORY_H_ */
