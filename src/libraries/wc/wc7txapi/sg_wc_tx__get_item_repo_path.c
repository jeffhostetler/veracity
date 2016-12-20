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
 * Return the CURRENT/LIVE repo-path for an
 * item ****AS OF THIS POINT IN THE TX****.
 *
 * The returned value reflects all QUEUED
 * structural changes in this TX and is ***ONLY
 * GOOD UNTIL THE NEXT STRUCTURAL CHANGE IN
 * THIS TX*** that affects this item or any
 * parent directory of this item.
 *
 * It MAY OR MAY NOT tell you anything about
 * where the item is in the current WD (or
 * even if it exists) (since the item should
 * still be in its pre-TX location/state until
 * we do the APPLY phase).
 *
 * YOU PROBABLY DO NOT NEED/WANT TO CALL THIS
 * ROUTINE.  However, you could use this to
 * map a GID-domain repo-path into an actual
 * live path FOR REPORTING PURPOSES, for example.
 *
 */
void SG_wc_tx__get_item_repo_path(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const char * pszInput,
								  SG_string ** ppStringRepoPathComputed)
{
	SG_string * pStringRepoPathInput = NULL;
	SG_string * pStringRepoPathComputed = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".
	SG_NULLARGCHECK_RETURN( ppStringRepoPathComputed );

	// To be consistent with other level-7 TX API routines,
	// we do the full "input ==> lvi ==> repo-path" process.
	
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPathInput, &chDomain)  );
#if TRACE_WC_TX_ADD
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__add: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPathInput))  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPathInput,
														  &bKnown, &pLVI)  );
	if (!bKnown)
	{
		// We only get this if the path is completely bogus and
		// took us off into the weeds (as opposed to reporting
		// something just not-controlled).
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown item '%s'.", SG_string__sz(pStringRepoPathInput))  );
	}

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI,
															  &pStringRepoPathComputed)  );


	*ppStringRepoPathComputed = pStringRepoPathComputed;
	pStringRepoPathComputed = NULL;
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathInput);
	SG_STRING_NULLFREE(pCtx, pStringRepoPathComputed);
}

