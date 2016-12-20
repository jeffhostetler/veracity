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
 * @file sg_client_prototypes.h
 *
 */

#ifndef H_SG_CLIENT_PROTOTYPES_H
#define H_SG_CLIENT_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_sync_client__spec_is_remote(
	SG_context* pCtx,
	const char* psz_remote_repo_spec,
	SG_bool* pbRemote);

void SG_sync_client__open(
	SG_context* pCtx,
	const char* psz_remote_repo_spec,
	const char* psz_username,
	const char* psz_password,
	SG_sync_client** ppNew
	);

void SG_sync_client__open__local(
	SG_context* pCtx,
	SG_repo* pOtherRepo,	/* < [in]  Required. This is the dest repo for push, or the source repo for pull. */
	SG_sync_client** ppNew	/* < [out] Required. */
	);

void SG_sync_client__close_free(SG_context * pCtx, SG_sync_client * pClient);

void SG_sync_client__push_begin(
		SG_context* pCtx,
		SG_sync_client * pClient,
		SG_pathname** pFragballDirPathname,
        SG_sync_client_push_handle** ppPush
        );

// SG_sync_client takes ownership of the fragball with this call.  It will clean up the file and the SG_pathname.
void SG_sync_client__push_add(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle* pPush,
		SG_bool bProgressIfPossible,  // When possible, emit progress events for this request. Caller should push/pop operation.
        SG_pathname** ppPath_fragball,
        SG_vhash** ppResult
        );

void SG_sync_client__push_commit(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle* pPush,
        SG_vhash** ppResult
        );

void SG_sync_client__push_end(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle** ppPush
        );

/* Verifies the name is valid and available, then prepares to receive the clone fragball. */
void SG_sync_client__push_clone__begin(
	SG_context* pCtx,
	SG_sync_client * pClient,
	const SG_vhash* pvhExistingRepoInfo,
	SG_sync_client_push_handle** ppPush
	);

/* Upload the fragball and create the new clone. */
void SG_sync_client__push_clone__upload_and_commit(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_sync_client_push_handle** ppPush, // SG_sync_client takes ownership and will free *ppPush.
	SG_bool bProgressIfPossible,  // When possible, emit progress events for this request. Caller should push/pop operation.
	SG_pathname** ppPathCloneFragball, // SG_sync_client takes ownership of the fragball with this call.  It will clean up the file and the SG_pathname.
	SG_vhash** ppvhResult);

/* Abort a push clone. Call only when push_clone__begin was successful but push_clone__upload_and_commit won't be called. */
void SG_sync_client__push_clone__abort(
	SG_context* pCtx,
	SG_sync_client * pClient,
	SG_sync_client_push_handle** ppPush // We take ownership and free.
	);

/**
 * Request a fragball from the repo specified in pClient.  It could be local or remote.
 * If prbDagnodes is null, the fragball will contain all the leaves.
 * If psaBlobs is null, the fragball will contain no blobs.
 * The fragball is saved in pStagingPathname with file name ppszFragballName.
 *
 * See SG_sync_remote__request_fragball for the vhash formats of pvhRequest and pvhStatus.
 */
void SG_sync_client__pull_request_fragball(
		SG_context* pCtx,
		SG_sync_client* pClient,
		SG_vhash* pvhRequest,
		SG_bool bProgressIfPossible,         // When possible, emit progress events for this request. Caller should push/pop operation.
		const SG_pathname* pStagingPathname,
		char** ppszFragballName
        );

void SG_sync_client__pull_clone(
	SG_context* pCtx,
	SG_sync_client* pClient,
    SG_vhash* pvh_partial,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName);

void SG_sync_client__get_repo_info(
		SG_context* pCtx,
		SG_sync_client* pClient,
		SG_bool bIncludeBranchInfo,
        SG_bool b_include_areas,
        SG_vhash** ppvh
		);

void SG_sync_client__heartbeat(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash** ppvh
	);

void SG_sync_client__get_dagnode_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo);

END_EXTERN_C;

#endif//H_SG_CLIENT_PROTOTYPES_H

