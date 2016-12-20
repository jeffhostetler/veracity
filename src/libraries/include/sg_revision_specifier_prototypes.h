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

#ifndef H_SG_REV_SPEC_PROTOTYPES_H
#define H_SG_REV_SPEC_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_rev_spec__alloc(SG_context* pCtx, SG_rev_spec** ppRevSpec);
void SG_rev_spec__alloc__copy(SG_context* pCtx, const SG_rev_spec* pRevSpecSrc, SG_rev_spec** ppRevSpecDest);

#if defined(DEBUG)
#define __SG_REV_SPEC__ALLOC__(pp,expr)		SG_STATEMENT(	SG_rev_spec * _p = NULL;										\
															expr;															\
															_sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_rev_spec");	\
															*(pp) = _p;														)

#define SG_REV_SPEC__ALLOC(pCtx,ppNew)						__SG_REV_SPEC__ALLOC__(ppNew, SG_rev_spec__alloc(pCtx,&_p)		)
#define SG_REV_SPEC__ALLOC__COPY(pCtx,pOld,ppNew)			__SG_REV_SPEC__ALLOC__(ppNew, SG_rev_spec__alloc__copy(pCtx,pOld,&_p)		)

#else

#define SG_REV_SPEC__ALLOC(pCtx,ppNew)						SG_rev_spec__alloc(pCtx,ppNew)
#define SG_REV_SPEC__ALLOC__COPY(pCtx,pOld,ppNew)			SG_rev_spec__alloc__copy(pCtx,pOld,ppNew)

#endif

void SG_rev_spec__free(SG_context* pCtx, SG_rev_spec* ppRevSpec);
#define SG_REV_SPEC_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_rev_spec__free)

void SG_rev_spec__add_rev(SG_context* pCtx, SG_rev_spec* pRevSpec, const char* pszRev);
void SG_rev_spec__add_tag(SG_context* pCtx, SG_rev_spec* pRevSpec, const char* pszTag);
void SG_rev_spec__add_branch(SG_context* pCtx, SG_rev_spec* pRevSpec, const char* pszBranch);
void SG_rev_spec__add(SG_context * pCtx, SG_rev_spec * pRevSpec, SG_rev_spec_type t, const char * psz);

/**
 * If there are revision specifiers that are valid only for version control, return true.
 */
void SG_rev_spec__has_non_vc_specs(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_bool* pbResult);

void SG_rev_spec__get_dagnum(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_uint64* piDagnum);
void SG_rev_spec__set_dagnum(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_uint64 iDagnum);
/**
 * Return the number of hid/revno revision specifiers.
 * A null pRevSpec is valid: *piCount will be 0.
 */
void SG_rev_spec__count_revs(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount);
/**
 * Return the number of tag revision specifiers.
 * A null pRevSpec is valid: *piCount will be 0.
 */
void SG_rev_spec__count_tags(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount);
/**
 * Return the number of branch revision specifiers.
 * A null pRevSpec is valid: *piCount will be 0.
 */
void SG_rev_spec__count_branches(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount);
/**
 * Return the number of revision specifiers of any kind.
 * A null pRevSpec is valid: *piCount will be 0.
 */
void SG_rev_spec__count(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount);
/**
 * Return SG_TRUE if no revisions are specified. SG_FALSE otherwise.
 * A null pRevSpec is valid: returns SG_TRUE.
 * pCtx is an input parameter ONLY: no need to wrap your call in an SG_ERR_CHECK or SG_ERR_IGNORE.
 */
SG_bool SG_rev_spec__is_empty(SG_context* pCtx, const SG_rev_spec* pRevSpec);

void SG_rev_spec__revs_rbt(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rbtree** pprbRevs);
void SG_rev_spec__tags_rbt(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rbtree** pprbTags);
void SG_rev_spec__branches_rbt(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rbtree** pprbBranches);

void SG_rev_spec__revs(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_stringarray** ppsaRevs);
void SG_rev_spec__tags(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_stringarray** ppsaTags);
void SG_rev_spec__branches(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_stringarray** ppsaBranches);

void SG_rev_spec__get_all(
	SG_context* pCtx, 
	const char* pszRepoName,
	const SG_rev_spec* pRevSpec, 
	SG_bool bAllowAmbiguousBranches,
	SG_stringarray** ppsaFullHids);

void SG_rev_spec__get_all__repo(
	SG_context* pCtx, 
	SG_repo* pRepo,
	const SG_rev_spec* pRevSpec, 
	SG_bool bAllowAmbiguousBranches,
	SG_stringarray** ppsaFullHids,
	SG_stringarray** ppsaMissingHids);

void SG_rev_spec__get_all__repo__dedup(
	SG_context* pCtx, 
	SG_repo* pRepo,
	const SG_rev_spec* pRevSpec, 
	SG_bool bAllowAmbiguousBranches,
	SG_stringarray** ppsaFullHids,
	SG_stringarray** ppsaMissingHids);

/**
 * Return exactly one changeset HID from the revision specs.
 * This will only return successfully if there is exactly one spec and that spec resolves to exactly one changeset HID.
 * If the one spec is a branch:
 *  - that branch must be unambiguous
 *  - the branch name is returned in ppszBranchName
 */
void SG_rev_spec__get_one(
	SG_context* pCtx,
	const char* pszRepoName,
	const SG_rev_spec* pRevSpec,
	SG_bool bIgnoreMissing,
	char** ppszChangesetHid,
	char** ppszBranchName);

/**
 * Return exactly one changeset HID from the revision specs.
 * This will only return successfully if there is exactly one spec and that spec resolves to exactly one changeset HID.
 * If the one spec is a branch:
 *  - that branch must be unambiguous
 *  - the branch name is returned in ppszBranchName
 */
void SG_rev_spec__get_one__repo(
	SG_context* pCtx,
	SG_repo* pRepo,
	const SG_rev_spec* pRevSpec,
	SG_bool bIgnoreMissing,
	char** ppszChangesetHid,
	char** ppszBranchName);

/**
 * Execute a SG_rev_spec_foreach_callback for each revision specifier.
 * When a branch is ambiguous, the callback is executed for each value of the branch.
 * Note that no validation is done first. We'll throw on the first spec that can't be resolved.
 */
void SG_rev_spec__foreach__repo(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	SG_rev_spec* pRevSpec, 
	SG_rev_spec_foreach_callback* cb, 
	void* ctx);

/**
 * Determine whether a rev_spec contains a certain revision.
 */
void SG_rev_spec__contains(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	SG_rev_spec* pRevSpec, 
	const char* szHid, 
	SG_bool* pFound);

void SG_rev_spec__to_vhash(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_bool bIgnoreDupes, SG_vhash** ppvh);
void SG_rev_spec__from_vash(SG_context* pCtx, const SG_vhash* pvh, SG_rev_spec** ppRevSpec);

void SG_rev_spec__merge(SG_context* pCtx, SG_rev_spec* pRevSpecMergeInto, SG_rev_spec** ppRevSpecWillBeFreed);

/**
 * Split the rev spec, creating a new one with all of the single-revisions specs (--rev and --tag).
 * Those returned in the new rev_spec will be removed from the original.
 */
void SG_rev_spec__split_single_revisions(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rev_spec ** ppNewRevSpec);

END_EXTERN_C;

#endif // H_SG_REV_SPEC_PROTOTYPES_H
