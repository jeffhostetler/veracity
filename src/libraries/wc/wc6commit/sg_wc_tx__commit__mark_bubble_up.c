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
 * @file sg_wc_tx__commit__mark_bubble_up.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _mark_bubble_up(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	char cDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );

	// validate/normalize whatever they typed into a
	// proper repo-path.  this may or may not have a
	// domain component.
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &cDomain)  );
#if TRACE_WC_COMMIT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__commit__mark_bubble_up: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, cDomain, SG_string__sz(pStringRepoPath))  );
#endif

	// find the GID/alias of the named item while taking
	// account whatever domain component.  this should
	// only respect domains valid during a commit.
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath,
														  &bKnown, &pLVI)  );
	if (!bKnown)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown item '%s'.", SG_string__sz(pStringRepoPath))  );

	SG_ERR_CHECK(  sg_wc_tx__rp__commit__mark_bubble_up__lvi(pCtx, pWcTx, pLVI)  );
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__commit__mark_bubble_up(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_wc_commit_args * pCommitArgs)
{
	if (pCommitArgs->psaInputs)
	{
		SG_uint32 k, count;

		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pCommitArgs->psaInputs, &count)  );
		SG_ASSERT(  (count > 0)  );		// if no items, parse should have returned null.
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pCommitArgs->psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  _mark_bubble_up(pCtx, pWcTx, pszInput_k)  );
		}
	}
	else
	{
		// if no args, we assume "@/" for a COMMIT.
		// no bubble-up needed from "@/".
	}

fail:
	return;
}
