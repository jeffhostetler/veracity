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
 * @file sg_sync_remote_prototypes.h
 * @details  SG_sync_remote contains routines that run on the remote side of a push, pull, incoming,
 *           or outgoing operation. Note that the remote side isn't always actually remote, but
 *           whether a push/pull operation is called through the C or the HTTP vtable, the code that
 *           runs on the other side is in SG_sync_remote.
 */

#ifndef H_SG_SYNC_REMOTE_PROTOTYPES_H
#define H_SG_SYNC_REMOTE_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_sync_remote__get_staging_path(
	SG_context* pCtx, 
	const char* pszPushId, 
	SG_pathname** ppStagingPathname);

void SG_sync_remote__push_begin(
	SG_context*,
	const char** ppszPushId
	);

void SG_sync_remote__push_add(
	SG_context*,
	const char* pszPushId,
	SG_repo* pRepo,
	const char* psz_fragball_name,
	SG_vhash** ppResult
	);

void SG_sync_remote__push_commit(
	SG_context*,
	SG_repo*,
	const char* pszPushId,
	SG_vhash** ppResult
	);

void SG_sync_remote__push_end(
	SG_context*,
	const char* pszPushId
	);

/**
 *  Builds a fragball and writes it to a file in pFragballDirPathname.
 * 
 *	The request vhash can take 3 forms:
 *
 *	(1) A null pvhRequest will fetch the leaves of every dag.
 *
 *	(2) A vhash requesting a full clone of the entire repository looks like this:
 *	{
 *		"clone" : NULL
 *	}
 *
 *	(3) A request for specific nodes and/or blobs looks like this:
 *	{
 *		"dags" :  // NULL for all leaves of all dags, else vhash where keys are dag numbers.
 *		{
 *			<dagnum> :  // NULL for all leaves, else vhash created by SG_rev_spec__to_vhash
 *			{
 *				// (see SG_rev_spec__to_vhash in sg_revision_specifier.c for this format)
 *			},
 *			...
 *		}
 *		"blobs" :
 *		{
 *			<hid> : NULL,
 *			...
 *		},
 *		"leaves" : // If missing, no additional generations are added to the fragball.
 *		{
 *			<dagnum> : // If present and corresponding to a dagnum in the "dags" section above, 
 *			           // generations will be added to connect to the leaves.
 *			{
 *				<hid> : <generation>,
 *				...
 *			},
 *			...
 *		}
 *	}
 *
 *	Any of the top level keys can be omitted: gens, dags, or blobs.
 *	- If "dags" is missing, no dagnodes are included in the fragball and gens is ignored, if present.
 *	- If "blobs" is missing, no blobs are included in the fragball.
 */
void SG_sync_remote__request_fragball(
	SG_context* pCtx,
	SG_repo* pRepo,
	const SG_pathname* pFragballDirPathname, /* The directory where the fragball file will be written */
	SG_vhash* pvhRequest,					 /* Specs for the fragball to build. Can be NULL. */
	char** ppszFragballName					 /* The name of the fragball file. Caller must free. */
	);

void SG_sync_remote__get_repo_info(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_bool bIncludeBranchesAndLocks,
    SG_bool b_include_areas,
	SG_vhash** ppvh);

void SG_sync_remote__heartbeat(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_vhash** ppvh);

void SG_sync_remote__get_dagnode_info(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo);

void SG_sync_remote__push_clone__begin(
	SG_context* pCtx,
	const char* psz_repo_descriptor_name,
	const SG_vhash* pvhRepoInfo,
	const char** ppszCloneId);

void SG_sync_remote__push_clone__commit(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_pathname** ppPathFragball); /*< [in] Required. On success, this routine takes ownership of the pointer. It will free memory and delete the file. */
	
void SG_sync_remote__push_clone__commit_maybe_usermap(
	SG_context* pCtx,
	const char* pszCloneId,
	const char* pszClosetAdminId,
	SG_pathname** ppPathFragball,	/*< [in] Required. On success, this routine takes ownership of the pointer. It will free memory and delete the file. */
	char** ppszDescriptorName,		/*< [out] Optional. Caller should free. */
	SG_bool* pbAvailable,			/*< [out] Optional. */
	SG_closet__repo_status* pStatus,/*< [out] Optional. */
	char** ppszStatus);				/*< [out] Optional. Caller should free. */

END_EXTERN_C;

#endif//H_SG_SYNC_REMOTE_PROTOTYPES_H

