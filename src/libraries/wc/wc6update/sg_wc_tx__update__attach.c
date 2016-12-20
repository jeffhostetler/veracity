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

/**
 * Attach or detach to/from the branch as requested.
 *
 */
void sg_wc_tx__update__attach(SG_context * pCtx,
							  sg_update_data * pUpdateData,
							  SG_bool * pbWroteBranch)
{
	SG_bool bCurrentlyAttached = (pUpdateData->pszBranchName_Starting && *pUpdateData->pszBranchName_Starting);
	SG_bool bShouldBeAttached  = (pUpdateData->pszAttachChosen        && *pUpdateData->pszAttachChosen       );
	SG_bool bWroteBranch = SG_FALSE;

	if (bShouldBeAttached)
	{
		if (bCurrentlyAttached
			&& (strcmp(pUpdateData->pszAttachChosen,
					   pUpdateData->pszBranchName_Starting) == 0))
		{
			// same as current branch.  do nothing.
		}
		else
		{
			// We have already normalized/validated/verified
			// the chosen name (either from an explicit --attach,
			// or --attach-new argument *OR* implicitly
			// from the branch name associated/not associated
			// with the target CSET.
			//
			// So we don't need to re-validate/re-verify.
			// So we use the db layer rather than the TX layer.

			SG_ERR_CHECK(  sg_wc_db__branch__attach(pCtx, pUpdateData->pWcTx->pDb, pUpdateData->pszAttachChosen,
													SG_VC_BRANCHES__CHECK_ATTACH_NAME__DONT_CARE, SG_FALSE)  );
			bWroteBranch = SG_TRUE;
		}
	}
	else
	{
		if (bCurrentlyAttached)
		{
			SG_ERR_CHECK(  sg_wc_db__branch__detach(pCtx, pUpdateData->pWcTx->pDb)  );
			bWroteBranch = SG_TRUE;
		}
		else
		{
		}
	}

	*pbWroteBranch = bWroteBranch;

fail:
	return;
}
