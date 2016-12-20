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
 * @file sg_vv2__tag.c
 *
 * @details Routines to deal with 'vv tag' and 'vv tags' commands.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

/**
 * Set the whoami for the given repository.  If no repository is passed, one is inferred from the current working directory.
 * If the pszUsername is set, the whoami will be set to that username.  Use the bCreate flag to create a new user.  If bCreate
 * is supplied, pszUsername can not be NULL.
 *
 * The value of ppszNewestUsername will be set to the value of whoami for that repository.  If
 * pszUsername is supplied, and setting the whoami succeeded, the same contents will be returned 
 * in ppszNewestUsername.
 */
void SG_vv2__whoami(SG_context * pCtx,
				  const char * pszRepoName,
				  const char * pszUsername,
				  SG_bool bCreate,
				  char ** ppszNewestUsername)
{
	SG_repo * pRepo = NULL;
	SG_bool bRepoNamed = SG_FALSE;
	char * psz_userid = NULL;
	SG_vhash *user = NULL;

	if (bCreate == SG_TRUE)
		SG_NULLARGCHECK_RETURN(pszUsername);

	// open either the named repo or the repo of the WD.
	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	
	if (bCreate)
	{
		SG_user__create(pCtx, pRepo, pszUsername, &psz_userid);
		if (  SG_context__err_equals(pCtx, SG_ERR_ZING_CONSTRAINT)  )
		{
			SG_context__err_reset(pCtx);
		}
		else if (  SG_CONTEXT__HAS_ERR(pCtx)  )
			SG_ERR_RETHROW;
	}
	if (pszUsername != NULL)
	{
		SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, pszUsername, &user)  );

		if (user)
		{
			SG_bool bIsInactive = SG_FALSE;
			SG_ERR_CHECK(  SG_user__is_inactive(pCtx, user, &bIsInactive)  );
			if (bIsInactive)
			{
				SG_ERR_THROW2(SG_ERR_INACTIVE_USER, (pCtx, "%s", pszUsername));
			}
		}
		else
			SG_ERR_THROW2(SG_ERR_USERNOTFOUND, (pCtx, "%s", pszUsername));

		SG_ERR_CHECK(  SG_user__set_user__repo(pCtx, pRepo, pszUsername)  );
	}

	if (ppszNewestUsername != NULL)
	{
		SG_ERR_CHECK(  SG_user__get_username_for_repo(pCtx, pRepo, ppszNewestUsername)  );
	}
    /* fall through */

fail:
	SG_NULLFREE(pCtx, psz_userid);
	SG_VHASH_NULLFREE(pCtx, user);
    SG_REPO_NULLFREE(pCtx, pRepo);
}
