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

void sg_vv2__util__get_repo(SG_context * pCtx,
							const char * pszRepoName,
							SG_repo ** ppRepo,
							SG_bool * pbNamed)
{
	SG_vhash * pvhWcInfo = NULL;

	if (pszRepoName && *pszRepoName)
	{
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, ppRepo)  );
		*pbNamed = SG_TRUE;
	}
	else
	{
		const char * pszRepoName_WD;

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName_WD)  );

		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName_WD, ppRepo)  );
		*pbNamed = SG_FALSE;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
}

