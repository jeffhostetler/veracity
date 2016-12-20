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
 * Return a VHASH containing the ISSUES for this item.
 * These are computed by MERGE and UPDATE (and are constant
 * (at least until the next UPDATE/MERGE)).
 *
 * This DOES NOT include an RESOLUTIONS made.
 *
 * We return NULL if there are no issues for this item.
 * 
 */
void SG_wc_tx__get_item_issues(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   const char * pszInput,
							   SG_vhash ** ppvhItemIssues)
{
	SG_vhash * pvhItemIssues = NULL;
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );
	SG_NULLARGCHECK_RETURN( ppvhItemIssues );

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &chDomain)  );
#if TRACE_WC_TX_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__get_item_issues: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPath))  );
#endif
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath,
														  &bKnown, &pLVI)  );
	if (!bKnown)
	{
		// We only get this if the path is completely bogus and
		// took us off into the weeds (as opposed to reporting
		// something just not-controlled).
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown item '%s'.", SG_string__sz(pStringRepoPath))  );
	}

	if (pLVI->pvhIssue)
		SG_ERR_CHECK(  SG_VHASH__ALLOC__COPY(pCtx, &pvhItemIssues, pLVI->pvhIssue)  );

	*ppvhItemIssues = pvhItemIssues;
	pvhItemIssues = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_VHASH_NULLFREE(pCtx, pvhItemIssues);
}
