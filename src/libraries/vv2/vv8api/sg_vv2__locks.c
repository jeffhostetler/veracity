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
 * @file sg_vv2__locks.c
 *
 * @details Handle details of 'vv locks'
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void SG_vv2__locks(SG_context * pCtx,
				   const char * pszRepoName,
				   const char * pszBranchName,
				   const char * pszServerName,
				   const char * pszUserName,
				   const char * pszPassword,
				   SG_bool bPull,
				   SG_bool bVerbose,
				   char ** ppsz_tied_branch_name,
				   SG_vhash** ppvh_locks_by_branch,
				   SG_vhash** ppvh_violated_locks,
				   SG_vhash** ppvh_duplicate_locks)
{
	SG_vhash * pvhWcInfo = NULL;

	// pszRepoName is optional (defaults to WD if present)
	// pszBranchName is optional (defaults to tied branch in WD, if present and set)
	// pszServerName is optional (we try to get it from config if necessary)
	// pszUserName is optional (we do not default this)
	// pszPassword is optional (we do not default this)

	// TODO 2012/07/06 Should we take a SG_stringarray of branch names?
	// TODO            Does it make sense to ask for the locks on more
	// TODO            that 1 branch at a time?

	if (!pszRepoName || !*pszRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );

		// Since we had to sniff for a WD, we can also get the branch if necessary and if set.

		if (!pszBranchName || !*pszBranchName)
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhWcInfo, "branch", &pszBranchName)  );
		if (!pszBranchName || !*pszBranchName)
			SG_ERR_THROW(  SG_ERR_NOT_TIED  );
	}
	else
	{
		// when explicitly given a --repo, should we force a --branch or
		// silently assume 'master' ?   We DO NOT want to sniff the WD
		// for it.

		if (!pszBranchName || !*pszBranchName)
			pszBranchName = "master";
	}

	SG_ERR_CHECK(  sg_vv2__locks__main(pCtx, pszRepoName, pszBranchName, pszServerName,
									   pszUserName, pszPassword,
									   bPull, bVerbose,
									   ppsz_tied_branch_name,
									   ppvh_locks_by_branch,
									   ppvh_violated_locks,
									   ppvh_duplicate_locks)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
}


