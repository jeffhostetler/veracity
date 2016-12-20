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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

//////////////////////////////////////////////////////////////////

/**
 * Create a new repo and *always* create a WD for it.
 *
 */
void vv2__do_cmd_init_new_repo(SG_context * pCtx,
							  const char * pszRepoName,
							  const char * pszFolder,
							  const char * pszStorage,
							  const char * pszHashMethod,
							  const char * psz_shared_users)
{
	SG_pathname * pPathLocalDirectory = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathLocalDirectory, pszFolder)  );
	SG_workingdir__find_mapping(pCtx, pPathLocalDirectory, NULL, NULL, NULL);
	SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOT_A_WORKING_COPY);

	SG_ERR_CHECK(  SG_vv2__init_new_repo(pCtx,
										 pszRepoName,
										 pszFolder,
										 pszStorage,
										 pszHashMethod,
										 SG_FALSE,			// bNoWD
										 psz_shared_users,
										 SG_FALSE,
										 NULL,
										 NULL)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathLocalDirectory);

	// TODO 2010/10/25 Decide which error message we should try to provide a better error message for.
	// TODO            Candidates include
	// TODO SG_ERR_INVALID_REPO_NAME
	// TODO SG_ERR_AMBIGUOUS_ID_PREFIX
	// TODO SG_ERR_MULTIPLE_HEADS_FROM_DAGNODE
	// TODO
	// TODO            But we should look and see what the lower layers are throwing now.  Some of
	// TODO            of the messages that were here are now being handled.
	return;
}

