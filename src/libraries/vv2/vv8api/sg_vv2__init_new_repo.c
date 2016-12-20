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
 * @file sg_vv_verbs__init_new_repo.h
 *
 * @details Routines to perform most of 'vv init'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

// We ONLY need _WC_ so we can create a new WD if requested.
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

static void _vv_verbs__init_new_repo__get_admin_id(
    SG_context * pCtx,
    const char * psz_shared_users,
    char* buf_admin_id
    )
{
	SG_sync_client* pSyncClient = NULL;
	SG_vhash* pvhRepoInfo = NULL;
    const char* pszRefAdminId = NULL;

	SG_ERR_CHECK(  SG_sync_client__open(pCtx, psz_shared_users, NULL, NULL, &pSyncClient)  );
	SG_ERR_CHECK(  SG_sync_client__get_repo_info(pCtx, pSyncClient, SG_FALSE, SG_FALSE, &pvhRepoInfo)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhRepoInfo, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, &pszRefAdminId)  );

    SG_ERR_CHECK(  SG_strcpy(pCtx, buf_admin_id, SG_GID_BUFFER_LENGTH, pszRefAdminId)  );

fail:
    SG_SYNC_CLIENT_NULLFREE(pCtx, pSyncClient);
	SG_VHASH_NULLFREE(pCtx, pvhRepoInfo);
}

//////////////////////////////////////////////////////////////////

/**
 * Delete the repo that we just created because
 * of other problems.
 *
 * TODO 2010/11/08 Think about promoting this to a
 * TODO            public SG_vv_verb__delete_repo()
 * TODO            and fixing sg.c:do_cmd_deleterepo()
 * TODO            and whatever in jsglue to use it.
 */
static void _vv_verbs__init_new_repo__delete_new_repo(SG_context * pCtx,
													  const char * pszRepoName)
{
    SG_ERR_CHECK_RETURN(  SG_repo__delete_repo_instance(pCtx, pszRepoName)  );
	SG_ERR_CHECK_RETURN(  SG_closet__descriptors__remove(pCtx, pszRepoName)  );
}

/**
 * Create a new repo in the closet.
 */
static void _vv_verbs__init_new_repo__do_init(SG_context * pCtx,
											  const char * pszRepoName,
											  const char * pszStorage,
											  const char * pszHashMethod,
											  const char * psz_shared_users,
											  SG_bool bFromUserMaster,
											  char ** ppszGidRepoId,
											  char ** ppszHidCSetFirst)
{
	SG_repo * pRepo = NULL;
	SG_repo * pRepoUserMaster = NULL;
	char * pszUserMasterAdminId = NULL;
	SG_changeset * pCSetFirst = NULL;
	const char * pszHidCSetFirst_ref;
	char * pszHidCSetFirst = NULL;
	char * pszGidRepoId = NULL;
	char bufAdminId[SG_GID_BUFFER_LENGTH];

	// create a completely new repo in the closet.

	SG_NULLARGCHECK_RETURN( pszRepoName );
	// pszStorage is optional
	// pszHashMethod is optional

	SG_ASSERT(SG_FALSE == (psz_shared_users && bFromUserMaster)); // checked in SG_vv_verbs__init_new_repo

    if (psz_shared_users)
    {
        SG_ERR_CHECK(  _vv_verbs__init_new_repo__get_admin_id(pCtx, psz_shared_users, bufAdminId)  );
    }
	else if (bFromUserMaster)
	{
		SG_ERR_CHECK(  SG_REPO__USER_MASTER__OPEN(pCtx, &pRepoUserMaster)  );
		SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepoUserMaster, &pszUserMasterAdminId)  );
		memcpy(bufAdminId, pszUserMasterAdminId, sizeof(bufAdminId));
		//SG_memcpy2(pszUserMasterAdminId, bufAdminId);
		SG_NULLFREE(pCtx, pszUserMasterAdminId);
	}
    else
    {
        SG_ERR_CHECK(  SG_gid__generate(pCtx, bufAdminId, sizeof(bufAdminId))  );
    }

	SG_ERR_CHECK(  SG_repo__create__completely_new__empty__closet(pCtx, bufAdminId, pszStorage, pszHashMethod, pszRepoName)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	if (!psz_shared_users && !bFromUserMaster)
    {
        SG_ERR_CHECK(  SG_user__create_nobody(pCtx, pRepo)  );
    }

	SG_ERR_CHECK(  SG_repo__setup_basic_stuff(pCtx, pRepo, &pCSetFirst, NULL)  );

	if (psz_shared_users)
	{
		SG_ERR_CHECK(  SG_pull__admin(pCtx, pRepo, psz_shared_users, NULL, NULL, NULL, NULL)  );
	}
	else if (bFromUserMaster)
	{
		SG_ERR_CHECK(  SG_pull__admin__local(pCtx, pRepo, pRepoUserMaster, NULL)  );
	}

	SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pCSetFirst, &pszHidCSetFirst_ref)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidCSetFirst_ref, &pszHidCSetFirst)  );

	SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &pszGidRepoId)  );

	*ppszGidRepoId = pszGidRepoId;
	*ppszHidCSetFirst = pszHidCSetFirst;

    SG_REPO_NULLFREE(pCtx, pRepo);
	SG_REPO_NULLFREE(pCtx, pRepoUserMaster);
	SG_CHANGESET_NULLFREE(pCtx, pCSetFirst);
	return;

fail:
	/* If we fail to pull the admin dags after the repo's been created, delete it. */
	if (pRepo)
	{
		SG_REPO_NULLFREE(pCtx, pRepo);
		if (pszRepoName)
			SG_ERR_IGNORE(  _vv_verbs__init_new_repo__delete_new_repo(pCtx, pszRepoName)  );
	}

	SG_REPO_NULLFREE(pCtx, pRepoUserMaster);
	SG_CHANGESET_NULLFREE(pCtx, pCSetFirst);
	SG_NULLFREE(pCtx, pszGidRepoId);
	SG_NULLFREE(pCtx, pszHidCSetFirst);
}

//////////////////////////////////////////////////////////////////

void SG_vv2__init_new_repo(SG_context * pCtx,
						   const char * pszRepoName,
						   const char * pszFolder,
						   const char * pszStorage,
						   const char * pszHashMethod,
						   SG_bool bNoWD,
						   const char * psz_shared_users,
						   SG_bool bFromUserMaster,
						   char ** ppszGidRepoId,
						   char ** ppszHidCSetFirst)
{
	char * pszGidRepoId = NULL;
	char * pszHidCSetFirst = NULL;

	if (bNoWD && pszFolder && *pszFolder)
	{
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "init_new_repo: option 'no-wc' given with a folder argument.")  );
	}

	if (bFromUserMaster && psz_shared_users)
	{
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
			(pCtx, "init_new_repo: options 'shared-users' and 'from-user-master' are incompatible.")  );
	}
	// create the new repo.

	SG_ERR_CHECK(  _vv_verbs__init_new_repo__do_init(pCtx, pszRepoName, pszStorage, pszHashMethod, 
													 psz_shared_users, bFromUserMaster,
													 &pszGidRepoId, &pszHidCSetFirst)  );

	if (!bNoWD)
	{
		// try to create a new WD for the new repo.  the real goal here
		// is to create the directory (if necessary) and then the drawer
		// and control files.  if the directory already exists, we do not
		// auto-add the contents.

		SG_ERR_CHECK(  SG_wc__initialize(pCtx,
										 pszRepoName, pszFolder,
										 pszHidCSetFirst, SG_VC_BRANCHES__DEFAULT)  );
	}
	
	if (ppszGidRepoId)
		*ppszGidRepoId = pszGidRepoId;
	else
		SG_NULLFREE(pCtx, pszGidRepoId);

	if (ppszHidCSetFirst)
		*ppszHidCSetFirst = pszHidCSetFirst;
	else
		SG_NULLFREE(pCtx, pszHidCSetFirst);
	return;

fail:
	if (pszGidRepoId)
	{
		// If we successfully created the repo, but had an error
		// trying to populate the WD, delete the repo.  WE DO NOT
		// delete any mess we created in/around the WD because we
		// don't know how much of it the checkout created and how
		// much already existed.

		SG_ERR_IGNORE(  _vv_verbs__init_new_repo__delete_new_repo(pCtx, pszRepoName)  );
	}

	SG_NULLFREE(pCtx, pszGidRepoId);
	SG_NULLFREE(pCtx, pszHidCSetFirst);
}
