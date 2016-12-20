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
 * @file sg_dagquery_prototypes.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_DAGQUERY_PROTOTYPES_H
#define H_SG_DAGQUERY_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * See if/how the 2 dagnodes/changesets are related.
 *
 * Returns SG_DAGQUERY_RELATIONSHIP_ of hid1 relative to hid2.
 *
 * Currently, this routine focuses on ancestor/descendor/peer
 * relationships.
 *
 * If bSkipDescendantCheck or bSkipAncestorCheck are set, we avoid
 * half of the dagfrag work when the caller doesn't care about that
 * half of the answer and just return _UNKNOWN.
 */
void SG_dagquery__how_are_dagnodes_related(SG_context * pCtx,
										   SG_repo * pRepo,
                                           SG_uint64 dagnum,
										   const char * pszHid1,
										   const char * pszHid2,
										   SG_bool bSkipDescendantCheck,
										   SG_bool bSkipAncestorCheck,
										   SG_dagquery_relationship * pdqRel);

/**
 * Search the DAG for LEAVES/HEADS that are descendants of the given DAGNODE.
 *
 * Returns SG_DAGQUERY_FIND_HEADS_STATUS__ and an rbtree containing the HIDs
 * of the relevant LEAVES/HEADS.
 *
 * Set bStopIfMultiple if you only want the answer if there is a SINGLE
 * descendant leaf (either the starting node or a single proper descendant).
 * This allows us to stop the search if there are multiple descendant leaves
 * when you can't use them.  (In this case, we return __MULTIPLE status and
 * a null rbtree.)
 *
 */
void SG_dagquery__find_descendant_heads(SG_context * pCtx,
										SG_repo * pRepo,
                                        SG_uint64 dagnum,
										const char * pszHidStart,
										SG_bool bStopIfMultiple,
										SG_dagquery_find_head_status * pdqfhs,
										SG_rbtree ** pprbHeads);

/**
 * Search the DAG for LEAVES/HEADS that are descendants of the given DAGNODE.
 *
 * Returns SG_DAGQUERY_FIND_HEADS_STATUS__.
 *
 * If there is exactly one LEAF/HEAD, copy the HID into the provided buffer.
 *
 * This is a convenience wrapper for SG_dagquery__find_descendant_heads()
 * when you know that you only want 1 unique result.
 */
void SG_dagquery__find_single_descendant_head(SG_context * pCtx,
											  SG_repo * pRepo,
                                              SG_uint64 dagnum,
											  const char * pszHidStart,
											  SG_dagquery_find_head_status * pdqfhs,
											  char * bufHidHead,
											  SG_uint32 lenBufHidHead);


/**
 * Returns changeset hids representing nodes that are new between
 * pszOldNodeHid and pszNewNodeHid. In other words, returns the nodes
 * that are ancestors of pszNewNodeHid that are NOT ancestors of 
 * pszOldNodeHid.
 */	
void SG_dagquery__new_since(SG_context * pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	const char* pszOldNodeHid,
	const char* pszNewNodeHid,
	SG_rbtree** pprbNewNodeHids);

/**
 * Returns changeset hids representing nodes that are ancestors of pszNewNodeHid
 * and are NOT ancestors of pszOldNodeHid. This function differs from
 * SG_dagquery__new_since() in that it does not require pszOldNodeHid to be an
 * ancestor of pszNewNodeHid. Thus it is useful for asking the question "If i
 * perform this merge, what new changesets am i getting?". Note that the answer
 * to such a question is different depending on which side of the merge you're
 * asking it from, thus the "old" versus "new" distinction is as significant
 * here as it is in SG_dagquery__new_since().
 * 
 * This function is equivalent to SG_dagquery__new_since() in the case where
 * pszOldNodeHid is an ancestor of pszNewNodeHid. It does not throw an error in
 * that case. The only difference, then, is that this one returns a stringarray
 * of the hids, whereas SG_dagquery__new_since() returns an rbtree.
 *
 * The returned list is in descending order of changeset revno.
 */
void SG_dagquery__find_new_since_common(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_uint64 dagnum,
	const char * pszOldNodeHid,
	const char * pszNewNodeHid,
	SG_stringarray ** ppResults
	);

/**
 * Simply returns the node with the highest revno that is
 * an ancestor of all the given nodes.
 * 
 * A node is considered an ancestor of iself itself, so
 * the output could be equal to one of the input nodes.
 * 
 * Behavior should be considered undefined if there are
 * duplicate entries in the input list.
 */
void SG_dagquery__highest_revno_common_ancestor(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_uint64 dagnum,
	const SG_stringarray * pInputNodeHids,
	char ** ppOutputNodeHid // Caller must free the result.
	);

void SG_repo__dag__find_direct_backward_path(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid_from,
        const char* psz_csid_to,
        SG_varray** ppva
        );

void SG_repo__db__calc_delta(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid_from,
        const char* psz_csid_to,
        SG_uint32 flags,
        SG_vhash** ppvh_add,
        SG_vhash** ppvh_remove
        );

void SG_repo__db__calc_delta_from_root(
        SG_context * pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid_to,
        SG_uint32 flags,
        SG_vhash** ppvh_add
        );

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_DAGQUERY_PROTOTYPES_H
