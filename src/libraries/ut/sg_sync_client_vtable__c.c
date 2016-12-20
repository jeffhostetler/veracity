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
 *
 * @file sg_client_vtable__c.c
 *
 */

#include <sg.h>
#include "sg_sync_client__api.h"

#include "sg_sync_client_vtable__c.h"

struct _sg_client_c_push_handle
{
	char* pszPushId;
};
typedef struct _sg_client_c_push_handle sg_sync_client_c_push_handle;

static void _handle_free(SG_context * pCtx, sg_sync_client_c_push_handle* pPush)
{
	if (pPush)
	{
		SG_NULLFREE(pCtx, pPush->pszPushId);
		SG_NULLFREE(pCtx, pPush);
	}
}
#define _NULLFREE_PUSH_HANDLE(pCtx,p) SG_STATEMENT(  _handle_free(pCtx, p); p=NULL;  )

void sg_sync_client__c__open(SG_context* pCtx,
						SG_sync_client * pClient)
{
	/* Nothing to do for the local vtable. Here only for symmetry with
	   the HTTP vtable. */

	SG_NULLARGCHECK_RETURN(pClient);
}

void sg_sync_client__c__close(SG_context * pCtx, SG_sync_client * pClient)
{
	/* Nothing to do for the local vtable. Here only for symmetry with
	   the HTTP vtable. */

	SG_UNUSED(pCtx);
	SG_UNUSED(pClient);
}

void sg_sync_client__c__push_begin(SG_context* pCtx,
							  SG_sync_client * pClient,
							  SG_pathname** ppFragballDirPathname,
							  SG_sync_client_push_handle** ppPush)
{
	sg_sync_client_c_push_handle* pPush = NULL;

	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppFragballDirPathname);
	SG_NULLARGCHECK_RETURN(ppPush);

	// Alloc a push handle.
	SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(sg_sync_client_c_push_handle), &pPush)  );

	// Start the push.  We get back a push ID, which we save in the push handle.
	SG_ERR_CHECK(  SG_sync_remote__push_begin(pCtx, (const char **)&pPush->pszPushId)  );

	/* This is a local push, so we tell our caller to put fragballs directly in the "remote" staging area. */
	SG_ERR_CHECK(  SG_sync_remote__get_staging_path(pCtx, pPush->pszPushId, ppFragballDirPathname)  );

	// Return the new push handle.
	*ppPush = (SG_sync_client_push_handle*)pPush;
	pPush = NULL;

	/* fall through */

fail:
	_NULLFREE_PUSH_HANDLE(pCtx, pPush);
}

void sg_sync_client__c__push_add(SG_context* pCtx,
							SG_sync_client * pClient,
							SG_sync_client_push_handle* pPush,
							SG_bool bProgressIfPossible,
							SG_pathname** ppPath_fragball,
							SG_vhash** ppResult)
{
	sg_sync_client_c_push_handle* pMyPush = (sg_sync_client_c_push_handle*)pPush;
	SG_string* pFragballName = NULL;
	SG_staging* pStaging = NULL;

	SG_UNUSED(bProgressIfPossible);

	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(pPush);
	SG_NULLARGCHECK_RETURN(ppResult);

	if (!ppPath_fragball || !*ppPath_fragball)
    {
		/* get the push's current status */
		SG_ERR_CHECK(  SG_staging__open(pCtx, pMyPush->pszPushId, &pStaging)  );

		SG_ERR_CHECK(  SG_staging__check_status(pCtx, pStaging, pClient->pRepoOther,
			SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, SG_TRUE, ppResult)  );
    }
    else
    {
		/* add the fragball to the push */

		SG_ERR_CHECK(  SG_pathname__get_last(pCtx, *ppPath_fragball, &pFragballName)  );

        /* Tell the server to add the fragball. */
		SG_ERR_CHECK(  SG_sync_remote__push_add(pCtx, pMyPush->pszPushId, pClient->pRepoOther,
			SG_string__sz(pFragballName), ppResult) );

		SG_PATHNAME_NULLFREE(pCtx, *ppPath_fragball);
    }

	/* fall through */
fail:
	SG_STRING_NULLFREE(pCtx, pFragballName);
	SG_STAGING_NULLFREE(pCtx, pStaging);
}

void sg_sync_client__c__push_commit(SG_context* pCtx,
							   SG_sync_client * pClient,
							   SG_sync_client_push_handle* pPush,
							   SG_vhash** ppResult)
{
	sg_sync_client_c_push_handle* pMyPush = (sg_sync_client_c_push_handle*)pPush;

	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(pPush);
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK_RETURN(  SG_sync_remote__push_commit(pCtx, pClient->pRepoOther, pMyPush->pszPushId, ppResult)  );
}

void sg_sync_client__c__push_end(SG_context * pCtx,
							SG_sync_client * pClient,
							SG_sync_client_push_handle** ppPush)
{
	sg_sync_client_c_push_handle* pMyPush = NULL;

	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppPush);

	pMyPush = (sg_sync_client_c_push_handle*)*ppPush;

	SG_ERR_CHECK(  SG_sync_remote__push_end(pCtx, pMyPush->pszPushId)  );

	// fall through

fail:
	// We free the push handle even on failure, because SG_push_handle is opaque outside this file:
	// this is the only way to free it.
	_NULLFREE_PUSH_HANDLE(pCtx, pMyPush);
	*ppPush = NULL;
}


void sg_sync_client__c__request_fragball(SG_context* pCtx,
										 SG_sync_client* pClient,
										 SG_vhash* pvhRequest,
										 SG_bool bProgressIfPossible,
										 const SG_pathname* pStagingPathname,
										 char** ppszFragballName)
{
	char* pszFragballName = NULL;

	SG_UNUSED(bProgressIfPossible);

	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppszFragballName);

	/* Tell the server to build its fragball in our staging directory.
	   We can do this just calling pServer directly because we know it's a local repo. */
	SG_ERR_CHECK(  SG_sync_remote__request_fragball(pCtx, pClient->pRepoOther, pStagingPathname, pvhRequest, &pszFragballName) );

	*ppszFragballName = pszFragballName;
	pszFragballName = NULL;

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszFragballName);
}

void sg_sync_client__c__pull_clone(
	SG_context* pCtx,
	SG_sync_client* pClient,
    SG_vhash* pvh_fragball_request,
	const SG_pathname* pStagingPathname,
	char** ppszFragballName)
{
	SG_repo* pRepo = NULL;
	SG_vhash* pvhStatus = NULL;

	SG_NULLARGCHECK_RETURN(pClient);
	SG_NULLARGCHECK_RETURN(ppszFragballName);

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pClient->psz_remote_repo_spec, &pRepo)  );

    SG_UNUSED(pvh_fragball_request);

    SG_ERR_CHECK(  SG_repo__fetch_repo__fragball(pCtx, pRepo, 3, pStagingPathname, ppszFragballName) );

	/* fall through */
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhStatus);
}

void sg_sync_client__c__heartbeat(
	SG_context* pCtx,
	SG_sync_client* pClient,
    SG_vhash** ppvh)
{
	SG_NULLARGCHECK_RETURN(pClient);

	SG_ERR_CHECK_RETURN(  SG_sync_remote__heartbeat(pCtx, pClient->pRepoOther, ppvh)  );
}

void sg_sync_client__c__get_repo_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_bool bIncludeBranchInfo,
    SG_bool b_include_areas, 
	SG_vhash** ppvh)
{
	SG_NULLARGCHECK_RETURN(pClient);

	SG_ERR_CHECK_RETURN(  SG_sync_remote__get_repo_info(pCtx, pClient->pRepoOther, bIncludeBranchInfo, b_include_areas, ppvh)  );
}

void sg_sync_client__c__get_dagnode_info(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_vhash* pvhRequest,
	SG_history_result** ppInfo)
{
	SG_NULLARGCHECK_RETURN(pClient);

	SG_ERR_CHECK_RETURN(  SG_sync_remote__get_dagnode_info(pCtx, pClient->pRepoOther, pvhRequest, ppInfo)  );
}

void sg_sync_client__c__push_clone__begin(
	SG_context* pCtx,
	SG_sync_client * pClient,
	const SG_vhash* pvhExistingRepoInfo,
	SG_sync_client_push_handle** ppPush
	)
{
	SG_UNUSED(pClient);
	SG_UNUSED(pvhExistingRepoInfo);
	SG_UNUSED(ppPush);

	SG_ERR_THROW_RETURN(SG_ERR_NOTIMPLEMENTED);
}

void sg_sync_client__c__push_clone__upload_and_commit(
	SG_context* pCtx,
	SG_sync_client* pClient,
	SG_sync_client_push_handle** ppPush,
	SG_bool bProgressIfPossible,
	SG_pathname** ppPathCloneFragball,
	SG_vhash**ppvhResult)
{
	SG_UNUSED(pClient);
	SG_UNUSED(ppPush);
	SG_UNUSED(bProgressIfPossible);
	SG_UNUSED(ppPathCloneFragball);
	SG_UNUSED(ppvhResult);

	SG_ERR_THROW_RETURN(SG_ERR_NOTIMPLEMENTED);
}

void sg_sync_client__c__push_clone__abort(
	SG_context* pCtx,
	SG_sync_client * pClient,
	SG_sync_client_push_handle** ppPush)
{
	SG_UNUSED(pClient);
	SG_UNUSED(ppPush);

	SG_ERR_THROW_RETURN(SG_ERR_NOTIMPLEMENTED);
}
