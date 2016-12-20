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
 * Fetch the pResolve handle from the TX.
 * This will allocate/populate it if necessary.
 *
 * YOU DO NOT OWN THE RETURNED pResolve.
 * 
 * The TX owns this handle and will free it when the TX is freed.
 *
 */
void SG_wc_tx__get_resolve_info(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								SG_resolve ** ppResolve)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppResolve );

	if (!pWcTx->pResolve)
		SG_ERR_CHECK(  SG_resolve__alloc(pCtx, pWcTx, NULL)  );

	*ppResolve = pWcTx->pResolve;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

#if 0  // not sure if i want to do this or not.....
/**
 * Merge two values on a conflict.
 *
 */
void SG_wc_tx__resolve_merge(SG_context * pCtx,
							 SG_wc_tx * pWcTx,
							 const char * pszInput,      //< [in] a layer-7 "input" -- path to file
							 SG_resolve__state eState,   //< [in] State to merge on.  May be __STATE__COUNT when single conflict.
							 const char * pszTool)       //< [in] name of mergetool.  may be NULL to use the default.
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &chDomain)  );
#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__resolve_merge: '%s' normalized to [domain %c] '%s'\n",
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

	SG_ERR_CHECK(  sg_wc_tx__rp__resolve_merge__lvi(pCtx, pWcTx, pLVI, eState, pszTool)  );
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	
}

#endif
