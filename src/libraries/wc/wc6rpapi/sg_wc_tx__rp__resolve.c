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

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#if 0  // not sure if i want to do this or not....
void sg_wc_tx__rp__resolve_merge__lvi(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_liveview_item * pLVI,
									  SG_resolve__state eState,
									  const char * pszTool)
{
	SG_string * pStringRepoPath = NULL;

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pStringRepoPath)  );

	// Not sure I want this error -- especially if we let UPDATE use RESOLVE
	// to clean up the mess.
	if (pWcTx->pCSetRow_Other == NULL)
		SG_ERR_THROW(  SG_ERR_NOT_IN_A_MERGE  );

	if (pLVI->pvhIssue == NULL)		// the item doesn't have any conflict issues
		SG_ERR_THROW2(  SG_ERR_ISSUE_NOT_FOUND,
						(pCtx, "%s", SG_string__sz(pStringRepoPath))  );

	if (!pWcTx->pResolve)
		SG_ERR_CHECK(  SG_resolve__alloc(pCtx, pWcTx, NULL)  );

	.... To be continued ....
	
}
#endif
