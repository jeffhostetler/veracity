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
 * @file sg_client__api.c
 *
 * This file will contain the static declarations of all VTABLES.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "sg_sync_client__api.h"

/* The "c" implementation of the client vtable can push/pull with local repositories. */
#include "sg_sync_client_vtable__c.h"
DCL__CLIENT_VTABLE(c);

/* The "http" implementation of the client vtable can push/pull with remote repositories. */
#include "sg_sync_client_vtable__http.h"
DCL__CLIENT_VTABLE(http);

//////////////////////////////////////////////////////////////////

void sg_sync_client__bind_vtable(SG_context* pCtx, SG_sync_client * pClient)
{
	SG_bool bRemote = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pClient);

	if (pClient->p_vtable) // can only be bound once
		SG_ERR_THROW2_RETURN(SG_ERR_INVALIDARG, (pCtx, "vtable already bound"));

	// Select the vtable based on pClient's remote repo specification or the presence of a local repo handle.
	if (pClient->psz_remote_repo_spec)
		SG_ERR_CHECK_RETURN(  SG_sync_client__spec_is_remote(pCtx, pClient->psz_remote_repo_spec, &bRemote)  );
	else if (!pClient->pRepoOther)
		SG_ERR_THROW2_RETURN(SG_ERR_INVALIDARG, (pCtx, "a repo spec or a local repo handle must be set"));
		
	if (bRemote)
		pClient->p_vtable = &s_client_vtable__http;
	else
	{
		pClient->p_vtable = &s_client_vtable__c;
		/* It would be better if this were done inside the C vtable, where pClient->pRepoOther is 
		 * actually used, but there's not a sane way to do that now; it would require a significant 
		 * re-org. */
		if (!pClient->pRepoOther)
		{
			SG_ERR_CHECK_RETURN(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pClient->psz_remote_repo_spec, &pClient->pRepoOther)  );
			pClient->bRepoOtherIsMine = SG_TRUE;
		}
	}
}

void sg_sync_client__unbind_vtable(SG_sync_client * pClient)
{
	if (!pClient)
		return;

	pClient->p_vtable = NULL;
}

