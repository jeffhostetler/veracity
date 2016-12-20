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
 * @file sg_sync_client__api.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_CLIENT__API_H
#define H_SG_CLIENT__API_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// Opaque client VTABLE instance data.  (May contain DB handle and
// anything else the VTABLE implementation needs.)  This is stored
// in the SG_sync_client structure after binding.

typedef struct _sg_sync_client__vtable__instance_data sg_sync_client__vtable__instance_data;

//////////////////////////////////////////////////////////////////
// Function prototypes for the routines in the client VTABLE.

typedef void FN__sg_sync_client__open(
		SG_context* pCtx,
        SG_sync_client * pClient
        );

typedef void FN__sg_sync_client__close(
        SG_context * pCtx,
        SG_sync_client * pClient
        );

typedef void FN__sg_sync_client__push_begin(
		SG_context* pCtx,
		SG_sync_client * pClient,
		SG_pathname** pFragballDirPathname,
        SG_sync_client_push_handle** ppPush
        );

typedef void FN__sg_sync_client__push_add(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle* pPush,
		SG_bool bProgressIfPossible,
        SG_pathname** ppPath_fragball,
        SG_vhash** ppResult
        );

typedef void FN__sg_sync_client__push_commit(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle* pPush,
        SG_vhash** ppResult
        );

typedef void FN__sg_sync_client__push_end(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle** ppPush
        );

typedef void FN__sg_sync_client__request_fragball(
		SG_context* pCtx,
		SG_sync_client* pClient,
		SG_vhash* pvhRequest,
		SG_bool bProgressIfPossible,
		const SG_pathname* pStagingPathname,
		char** ppszFragballName
        );

typedef void FN__sg_sync_client__pull_clone(
	SG_context* pCtx,
	SG_sync_client* pClient,
    SG_vhash* pvh_partial,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName);

typedef void FN__sg_sync_client__get_repo_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_bool bIncludeBranchInfo,
	SG_bool b_include_areas,
    SG_vhash** ppvh
	);

typedef void FN__sg_sync_client__heartbeat(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash** ppvh
	);

typedef void FN__sg_sync_client__get_dagnode_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo);

typedef void FN__sg_sync_client__push_clone__begin(
	SG_context* pCtx,
	SG_sync_client * pClient,
	const SG_vhash* pvhExistingRepoInfo,
	SG_sync_client_push_handle** ppPush
	);

typedef void FN__sg_sync_client__push_clone__upload_and_commit(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_sync_client_push_handle** ppPush,
	SG_bool bProgressIfPossible,
	SG_pathname** ppPathCloneFragball,
	SG_vhash** ppvhResult);

typedef void FN__sg_sync_client__push_clone__abort(
	SG_context* pCtx,
	SG_sync_client * pClient,
	SG_sync_client_push_handle** ppPush
	);

//////////////////////////////////////////////////////////////////
// The client VTABLE

struct _sg_sync_client__vtable
{
	FN__sg_sync_client__open							* const	open;
	FN__sg_sync_client__close							* const	close;
	FN__sg_sync_client__push_begin						* const	push_begin;
	FN__sg_sync_client__push_add						* const	push_add;
	FN__sg_sync_client__push_commit						* const	push_commit;
	FN__sg_sync_client__push_end						* const	push_end;
	FN__sg_sync_client__request_fragball				* const	pull_request_fragball;
	FN__sg_sync_client__pull_clone						* const pull_clone;
	FN__sg_sync_client__get_repo_info					* const	get_repo_info;
	FN__sg_sync_client__heartbeat						* const heartbeat;
	FN__sg_sync_client__get_dagnode_info				* const get_dagnode_info;
	FN__sg_sync_client__push_clone__begin				* const push_clone__begin;
	FN__sg_sync_client__push_clone__upload_and_commit	* const push_clone__upload_and_commit;
	FN__sg_sync_client__push_clone__abort				* const push_clone__abort;
};

typedef struct _sg_sync_client__vtable sg_sync_client__vtable;

//////////////////////////////////////////////////////////////////
// Convenience macro to declare a prototype for each function in the
// client VTABLE for a specific implementation.

#define DCL__CLIENT_VTABLE_PROTOTYPES(name)																		\
	FN__sg_sync_client__open							sg_sync_client__##name##__open;							\
	FN__sg_sync_client__close							sg_sync_client__##name##__close;						\
	FN__sg_sync_client__push_begin						sg_sync_client__##name##__push_begin;					\
	FN__sg_sync_client__push_add						sg_sync_client__##name##__push_add;						\
	FN__sg_sync_client__push_commit						sg_sync_client__##name##__push_commit;					\
	FN__sg_sync_client__push_end						sg_sync_client__##name##__push_end;						\
	FN__sg_sync_client__request_fragball				sg_sync_client__##name##__request_fragball;				\
	FN__sg_sync_client__pull_clone						sg_sync_client__##name##__pull_clone;					\
	FN__sg_sync_client__get_repo_info					sg_sync_client__##name##__get_repo_info;				\
	FN__sg_sync_client__heartbeat						sg_sync_client__##name##__heartbeat;					\
	FN__sg_sync_client__get_dagnode_info				sg_sync_client__##name##__get_dagnode_info;				\
	FN__sg_sync_client__push_clone__begin				sg_sync_client__##name##__push_clone__begin;			\
	FN__sg_sync_client__push_clone__upload_and_commit	sg_sync_client__##name##__push_clone__upload_and_commit;\
	FN__sg_sync_client__push_clone__abort				sg_sync_client__##name##__push_clone__abort;


// Convenience macro to declare a properly initialized static
// CLIENT VTABLE for a specific implementation.

#define DCL__CLIENT_VTABLE(name)								\
	static sg_sync_client__vtable s_client_vtable__##name =		\
	{	sg_sync_client__##name##__open,							\
		sg_sync_client__##name##__close,						\
		sg_sync_client__##name##__push_begin,					\
		sg_sync_client__##name##__push_add,						\
		sg_sync_client__##name##__push_commit,					\
		sg_sync_client__##name##__push_end,						\
		sg_sync_client__##name##__request_fragball,				\
		sg_sync_client__##name##__pull_clone,					\
		sg_sync_client__##name##__get_repo_info,				\
		sg_sync_client__##name##__heartbeat,					\
		sg_sync_client__##name##__get_dagnode_info,				\
		sg_sync_client__##name##__push_clone__begin,			\
		sg_sync_client__##name##__push_clone__upload_and_commit,\
		sg_sync_client__##name##__push_clone__abort				\
	}

//////////////////////////////////////////////////////////////////

/**
 * Private routine to select a specific VTABLE implementation
 * and bind it to the given pClient.
 */
void sg_sync_client__bind_vtable(SG_context* pCtx, SG_sync_client * pClient);

/**
 * Private routine to unbind the VTABLE from the given pClient.
 * This routine will NULL the vtable pointer in the SG_sync_client and free it
 * if appropriate.
 */
void sg_sync_client__unbind_vtable(SG_sync_client * pClient);

//////////////////////////////////////////////////////////////////

/**
 * Private sync_client.  This is hidden from everybody except
 * sg_sync_client__ routines.
 */
struct _SG_sync_client
{
	SG_repo* pRepoOther;											// for a local push/pull, we keep an open handle on the "other" repo here
	SG_bool bRepoOtherIsMine;										// If true, pRepoOther is closed and freed by SG_sync__close_free()

	char* psz_remote_repo_spec;										// this is used to select a VTABLE implementation
	char* psz_username;												// Username provided to SG_sync_client__open()
	char* psz_password;												// Password provided to SG_sync_client__open()

	sg_sync_client__vtable *				p_vtable;				// the binding to a specific CLIENT VTABLE implementation
	sg_sync_client__vtable__instance_data *	p_vtable_instance_data;	// binding-specific instance data (opaque outside of imp)
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_CLIENT__API_H

