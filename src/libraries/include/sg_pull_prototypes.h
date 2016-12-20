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
 * @file sg_pull_prototypes.h
 *
 */

#ifndef H_SG_PULL_PROTOTYPES_H
#define H_SG_PULL_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_pull__all__using_since(
	SG_context* pCtx, 
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats
    );

/**
 * Pull all changes from all DAGs.
 */
void SG_pull__all(
	SG_context* pCtx,
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec, // local descriptor or remote http://
	const char* pszUsername,       // [in] (Optional) Username to use to authorize the pull.
	const char* pszPassword,       // [in] (Optional) Password to use to authorize the pull.
	SG_vhash** ppvhAccountInfo,    // [out] (Optional) Contains account info when pulling from a hosted account.
	SG_vhash** ppvhStats
	);

/**
 * Begin a pull. 
 * Sets up a local staging area.
 * A roundtrip with the other repo is performed to retrieve IDs and named branch info.
 */
void SG_pull__begin(
	SG_context* pCtx,
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec, // local descriptor or remote http://
	const char* pszUsername,       // [in] (Optional) Username to use to authorize the pull.
	const char* pszPassword,       // [in] (Optional) Password to use to authorize the pull.
	SG_pull** ppPull);

/**
 * Add the requested dagnodes (and implicitly the relevant blobs) to the fragball request.
 * No roundtrip with the other repo is actually performed. This simply adds the request.
 */
void SG_pull__add(
	SG_context* pCtx,
	SG_pull* pPull,
	SG_rev_spec** pRevSpec); // [in] On success, takes ownership.

/**
 * Request all nodes (and implicitly the relevant blobs) for a single DAG.
 * No roundtrip with the other repo is actually performed. This simply adds the request.
 */
void SG_pull__add__dagnum(
	SG_context* pCtx, 
	SG_pull* pPull, 
	SG_uint64 iDagnum);

/**
 * Request all nodes (and implicitly the relevant blobs) for all DAGs in the specified area.
 * No roundtrip with the other repo is actually performed. This simply adds the request.
 */
void SG_pull__add__area(
	SG_context* pCtx,
	SG_pull* pPull,
    const char* psz_area_name
    );

/**
 * Request specific version control nodes and all nodes in the version control area (and implicitly the relevant blobs).
 * No roundtrip with the other repo is actually performed. This simply adds the request.
 */
void SG_pull__add__area__version_control(
	SG_context* pCtx,
	SG_pull* pPull,
	SG_rev_spec** ppRevSpec); // [in] On success, takes ownership.

/**
 * Add all admin DAGs to the pull.
 * No roundtrip with the other repo is actually performed. This simply adds the request.
 */
void SG_pull__add__admin(
	SG_context* pCtx,
	SG_pull* pPull);

/**
 * Peform the pull as set up by the various add routines.
 * Several roundtrips with the other repo may be performed.
 */
void SG_pull__commit(
	SG_context* pCtx,
	SG_pull** ppPull,
	SG_vhash** ppvhAccountInfo,    // [out] (Optional) Contains account info when pulling from a hosted account.
	SG_vhash** ppvhStats
    );

/**
 * Cancel and cleanup a pull that has been started with pull_begin.
 * You can either commit or abort, but not both.
 */
void SG_pull__abort(
	SG_context* pCtx,
	SG_pull** ppPull);

/**
 * Pull all changes from all admin DAGs.
 */
void SG_pull__admin(
	SG_context* pCtx, 
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec,
	const char* pszUsername,       // [in] (Optional) Username to use to authorize the pull.
	const char* pszPassword,       // [in] (Optional) Password to use to authorize the pull.
	SG_vhash** ppvhAccountInfo,    // [out] (Optional) Contains account info when pulling from a hosted account.
	SG_vhash** ppvhStats);

/**
 * Pull all changes from all admin DAGs from a local repository.
 */
void SG_pull__admin__local(
	SG_context* pCtx, 
	SG_repo* pRepoDest, 
	SG_repo* pRepoSrc,
	SG_vhash** ppvhStats);

/**
 * Returns the list of version control changesets that exist in the remote repo but not in the local repo.
 */
void SG_pull__list_incoming_vc(
	SG_context* pCtx,
	SG_repo* pRepoPullInto, 
	const char* pszRemoteRepoSpec, // local descriptor or remote http://
	const char* pszUsername,       // [in] (Optional) Username to use to authorize the pull.
	const char* pszPassword,       // [in] (Optional) Password to use to authorize the pull.
	SG_history_result** ppInfo,
	SG_vhash** ppvhStats // Trace/debug data about the work done, e.g. roundtrip, dagnode, and blob counts.
	);

END_EXTERN_C;

#endif//H_SG_PULL_PROTOTYPES_H

