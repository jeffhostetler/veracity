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
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void SG_vv2__check_attach_name(SG_context * pCtx,
							   const char * pszRepoName,
							   const char * pszCandidateBranchName,
							   SG_bool bMustExist,
							   char ** ppszNormalizedBranchName)
{
	SG_vhash * pvhWcInfo = NULL;
	SG_repo * pRepo = NULL;
	SG_vc_branches__check_attach_name__flags flags;
	SG_bool bValidate;

	// pszRepoName is optional (defaults to WD if present)
	SG_NONEMPTYCHECK_RETURN( pszCandidateBranchName );
	SG_NULLARGCHECK_RETURN( ppszNormalizedBranchName );

	if (!pszRepoName || !*pszRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );
	}

	if (bMustExist)
	{
		flags = SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_EXIST;
		bValidate = SG_FALSE;	// allow existing pre-2.0 names to be selected
	}
	else
	{
		flags = SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_NOT_EXIST;
		bValidate = SG_TRUE;	// require new names to adhere to 2.0 restrictions;
	}

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_vc_branches__check_attach_name(pCtx, pRepo, pszCandidateBranchName,
													 flags, bValidate,
													 ppszNormalizedBranchName)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
}
