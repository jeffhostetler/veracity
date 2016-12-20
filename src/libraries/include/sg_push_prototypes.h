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
 * @file sg_push_prototypes.h
 *
 */

#ifndef H_SG_PUSH_PROTOTYPES_H
#define H_SG_PUSH_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Push all changes from all DAGs.
 */
void SG_push__all(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_remote_repo_spec,
	const char* psz_username,
	const char* psz_password,
	SG_bool bSkipBranchAmbiguityCheck,
	SG_vhash** ppvhAccountInfo,    // [out] (Optional) Contains account info when pushing to a hosted account.
	SG_vhash** ppvhStats
    );

/**
 * Begin a push. 
 * A staging area with be created for the other (sometimes remote) repo.
 * The roundtrip with the other repo also retrieves IDs and named branch info.
 */
void SG_push__begin(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_remote_repo_spec,
	const char* psz_username,
	const char* psz_password,
	SG_push** ppPush);

/**
 * A low-level push routine. You probably don't want to use this.
 * Branch ambiguity checking won't be done if you add version control dagnodes with this routine.
 * To add specific version control dagnodes to a push, use SG_push__add_area__version_control.
 * To add all dagnodes relevant to a repository area, use SG_push__add_area.
 */
void SG_push__add(
	SG_context* pCtx,
	SG_push* pPush,
	SG_uint64 iDagnum,
	SG_rbtree* prbDagnodes);

/**
 * Add all nodes (and implicitly the relevant blobs) for all DAGs in the specified area.
 * No roundtrip with the other repo is actually performed. This simply adds data to a pending push.
 */
void SG_push__add__area(
	SG_context* pCtx,
	SG_push* pPush,
    const char* psz_area_name
    );

/**
 * Add specific version control nodes and all nodes in the version control area (and implicitly the relevant blobs).
 * No roundtrip with the other repo is actually performed. This simply adds data to a pending push.
 */
void SG_push__add__area__version_control(
	SG_context* pCtx,
	SG_push* pPush,
	SG_rev_spec* pRevSpec);

/**
 * Add all admin DAGs to the push.
 * No roundtrip with the other repo is actually performed. This simply adds data to a pending push.
 */
void SG_push__add__admin(
	SG_context* pCtx, 
	SG_push* pPush);

/**
 * Peform the push as set up by the various add routines.
 * Several roundtrips with the other repo may be performed.
 */
void SG_push__commit(
	SG_context* pCtx,
	SG_push** ppPush,
	SG_bool bSkipBranchAmbiguityCheck,
	SG_vhash** ppvhAccountInfo,    // [out] (Optional) Contains account info when pushing to a hosted account.
	SG_vhash** ppvhStats
    );

/**
 * Cancel and cleanup a push that has been started with push_begin.
 * You can either commit or abort, but not both.
 */
void SG_push__abort(
	SG_context* pCtx,
	SG_push** ppPush);

/**
 * Push all changes from all admin DAGs.
 */
void SG_push__admin(
	SG_context* pCtx, 
	SG_repo* pSrcRepo, 
	const char* psz_remote_repo_spec, 
	const char* psz_username,
	const char* psz_password,
	SG_vhash** ppvhAccountInfo,    // [out] (Optional) Contains account info when pushing to a hosted account.
	SG_vhash** ppvhStats);

/**
 * Returns the list of version control changesets that exist in the locat repo but not in the remote repo.
 */
void SG_push__list_outgoing_vc(
	SG_context* pCtx, 
	SG_repo* pSrcRepo, 
	const char* psz_remote_repo_spec, 
	const char* psz_username,
	const char* psz_password,
	SG_history_result** ppInfo,
	SG_vhash** ppvhStats				// Trace/debug data about the work done, e.g. roundtrip, dagnode, and blob counts.
	);

END_EXTERN_C;

#endif//H_SG_PUSH_PROTOTYPES_H

