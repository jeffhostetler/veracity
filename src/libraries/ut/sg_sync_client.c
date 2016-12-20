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

#include <sg.h>
#include "sg_sync_client__api.h"

/* This file is basically just functions that call
 * through the vtable.  Nothing here does any actual
 * work. */

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

void SG_sync_client__open__local(
	SG_context* pCtx,
	SG_repo* pOtherRepo,
	SG_sync_client** ppNew)
{
	SG_sync_client* pClient = NULL;

	SG_NULLARGCHECK_RETURN(pOtherRepo);
	SG_NULLARGCHECK_RETURN(ppNew);

	SG_ERR_CHECK(  SG_alloc1(pCtx,pClient)  );
	
	pClient->pRepoOther = pOtherRepo;
	pClient->bRepoOtherIsMine = SG_FALSE;

	SG_ERR_CHECK(  sg_sync_client__bind_vtable(pCtx, pClient)  );
	SG_ERR_CHECK(  pClient->p_vtable->open(pCtx, pClient)  );

	SG_RETURN_AND_NULL(pClient, ppNew);

	return;

fail:
	SG_SYNC_CLIENT_NULLFREE(pCtx, pClient);
}

void SG_sync_client__open(
	SG_context* pCtx,
	const char* psz_remote_repo_spec,
	const char* psz_username,
	const char* psz_password,
	SG_sync_client** ppNew)
{
	SG_sync_client* pClient = NULL;

	SG_NULLARGCHECK(ppNew);

	SG_ERR_CHECK(  SG_alloc1(pCtx,pClient)  );

	SG_ERR_CHECK(  SG_sz__trim(pCtx, psz_remote_repo_spec, NULL, &pClient->psz_remote_repo_spec)  );
	if(psz_username)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_username, &pClient->psz_username)  );
	if(psz_password)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_password, &pClient->psz_password)  );

	SG_ERR_CHECK(  sg_sync_client__bind_vtable(pCtx, pClient)  );
	SG_ERR_CHECK(  pClient->p_vtable->open(pCtx, pClient)  );

    SG_RETURN_AND_NULL(pClient, ppNew);

	return;

fail:
	SG_SYNC_CLIENT_NULLFREE(pCtx, pClient);
}

#define VERIFY_VTABLE(pClient)												\
	SG_STATEMENT(	SG_NULLARGCHECK_RETURN(pClient);						\
					if (!(pClient)->p_vtable)								\
						SG_ERR_THROW_RETURN(SG_ERR_VTABLE_NOT_BOUND);	\
				)

void SG_sync_client__close_free(
	SG_context * pCtx,
	SG_sync_client * pClient)
{
	if (!pClient)
		return;

	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->close(pCtx, pClient)  );

	sg_sync_client__unbind_vtable(pClient);

	if (pClient->bRepoOtherIsMine)
		SG_REPO_NULLFREE(pCtx, pClient->pRepoOther);

	SG_NULLFREE(pCtx, pClient->psz_remote_repo_spec);
	SG_NULLFREE(pCtx, pClient->psz_username);
	SG_NULLFREE(pCtx, pClient->psz_password);
	SG_NULLFREE(pCtx, pClient);
}

void SG_sync_client__push_begin(
		SG_context* pCtx,
        SG_sync_client * pClient,
		SG_pathname** pFragballDirPathname,
        SG_sync_client_push_handle** ppPush
        )
{
    VERIFY_VTABLE(pClient);

    SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_begin(pCtx, pClient, pFragballDirPathname, ppPush)  );
}

void SG_sync_client__push_add(
		SG_context* pCtx,
        SG_sync_client * pClient,
        SG_sync_client_push_handle* pPush,
		SG_bool bProgressIfPossible,
        SG_pathname** ppPath_fragball,
        SG_vhash** ppResult
        )
{
    VERIFY_VTABLE(pClient);

    SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_add(pCtx, pClient, pPush, bProgressIfPossible, ppPath_fragball, ppResult)  );
}

void SG_sync_client__push_commit(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle* pPush,
        SG_vhash** ppResult
        )
{
    VERIFY_VTABLE(pClient);

    SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_commit(pCtx, pClient, pPush, ppResult)  );
}

void SG_sync_client__push_end(
		SG_context* pCtx,
		SG_sync_client * pClient,
        SG_sync_client_push_handle** ppPush
        )
{
    VERIFY_VTABLE(pClient);

    SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_end(pCtx, pClient, ppPush)  );
}

void SG_sync_client__push_clone__begin(
	SG_context* pCtx,
	SG_sync_client * pClient,
	const SG_vhash* pvhExistingRepoInfo,
	SG_sync_client_push_handle** ppPush
	)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_clone__begin(pCtx, pClient, pvhExistingRepoInfo, ppPush)  );
}

void SG_sync_client__push_clone__upload_and_commit(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_sync_client_push_handle** ppPush,
	SG_bool bProgressIfPossible,
	SG_pathname** ppPathCloneFragball,
	SG_vhash** ppvhResult)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_clone__upload_and_commit(pCtx, pClient, ppPush, bProgressIfPossible, ppPathCloneFragball, ppvhResult)  );
}

void SG_sync_client__pull_request_fragball(SG_context* pCtx,
									  SG_sync_client* pClient,
									  SG_vhash* pvhRequest,
									  SG_bool bProgressIfPossible,
									  const SG_pathname* pStagingPathname,
									  char** ppszFragballName)
{
    VERIFY_VTABLE(pClient);

    SG_ERR_CHECK_RETURN(  pClient->p_vtable->pull_request_fragball(pCtx, pClient, pvhRequest, bProgressIfPossible, pStagingPathname, ppszFragballName)  );
}

void SG_sync_client__pull_clone(
	SG_context* pCtx,
	SG_sync_client* pClient,
    SG_vhash* pvh_partial,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->pull_clone(pCtx, pClient, pvh_partial, pStagingPathname, ppszFragballName)  );
}

void SG_sync_client__get_repo_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_bool bIncludeBranchInfo,
	SG_bool b_include_areas,
    SG_vhash** ppvh)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Examining sync target", SG_LOG__FLAG__NONE)  );

	SG_ERR_CHECK(  pClient->p_vtable->get_repo_info(pCtx, pClient, bIncludeBranchInfo, b_include_areas, ppvh)  );

fail:
	SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
	return;
}

void SG_sync_client__heartbeat(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash** ppvh)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->heartbeat(pCtx, pClient, ppvh)  );
}

void SG_sync_client__get_dagnode_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->get_dagnode_info(pCtx, pClient, pvhRequest, ppInfo)  );
}

void SG_sync_client__spec_is_remote(
	SG_context* pCtx,
	const char* psz_remote_repo_spec,
	SG_bool* pbRemote)
{
	SG_NULLARGCHECK_RETURN(psz_remote_repo_spec);
	SG_NULLARGCHECK_RETURN(pbRemote);

	// Select the vtable based on pClient's remote repo specification.
	if (SG_STRLEN(psz_remote_repo_spec) > 6)
	{
		if ( (SG_strnicmp("http://", psz_remote_repo_spec, 7) == 0) || (SG_strnicmp("https://", psz_remote_repo_spec, 8) == 0) )
		{
			*pbRemote = SG_TRUE;
			return;
		}
	}
	*pbRemote = SG_FALSE;
}

void SG_sync_client__push_clone__abort(
	SG_context* pCtx,
	SG_sync_client * pClient,
	SG_sync_client_push_handle** ppPush)
{
	VERIFY_VTABLE(pClient);

	SG_ERR_CHECK_RETURN(  pClient->p_vtable->push_clone__abort(pCtx, pClient, ppPush)  );
}
