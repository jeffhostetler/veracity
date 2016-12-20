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
 * Return the GID of an item identified by the given "input".
 * This uses the same algorithm that all of layer-7 WC TX API
 * uses to map arbitrary user input into a specific item.
 *
 * YOU PROBABLY DO NOT NEED/WANT TO CALL THIS ROUTINE since
 * most layers above layer-7 don't need to speak in terms of
 * GIDs and actually try to hide them from the user.
 *
 * WARNING: for UNCONTROLLED items, the returned value will
 * be a TEMP GID -- good for the duration of this TX.
 * 
 */
void SG_wc_tx__get_item_gid(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput,
							char ** ppszGid)
{
	SG_string * pStringRepoPathInput = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	char * pszGid = NULL;
	SG_bool bKnown;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".
	SG_NULLARGCHECK_RETURN( ppszGid );

	// To be consistent with other level-7 TX API routines,
	// we do the full "input ==> lvi" process.
	
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPathInput, &chDomain)  );
#if TRACE_WC_GID
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__get_item_gid: '%s' normalized to [domain %c] '%s'\n",
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

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
	// we get 'g' domain for controlled items and 't' domain IDs for uncontrolled items
	SG_ASSERT_RELEASE_FAIL2(  ((pszGid[0] == 'g') || (pszGid[0] == 't')),
							  (pCtx, "ID of '%s' must have 'g' or 't' prefix: %s",
							   pszInput, pszGid)  );

	*ppszGid = pszGid;
	pszGid = NULL;
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathInput);
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

/**
 * Return a stringarray of GIDs corresponding to the
 * stringarray of "input" paths.
 *
 * See all of the reasons given in SG_wc_tx__get_item_gid()
 * for why you probably don't need to call this.
 *
 */
void SG_wc_tx__get_item_gid__stringarray(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 const SG_stringarray * psaInput,
										 SG_stringarray ** ppsaGid)
{
	SG_stringarray * psaGidAllocated = NULL;
	char * pszGid_k = NULL;
	SG_uint32 k, count;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( psaInput );
	SG_NULLARGCHECK_RETURN( ppsaGid );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInput, &count)  );
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaGidAllocated, count)  );
	for (k=0; k<count; k++)
	{
		const char * pszInput_k;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInput, k, &pszInput_k)  );
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx, pszInput_k, &pszGid_k)  );
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaGidAllocated, pszGid_k)  );
		SG_NULLFREE(pCtx, pszGid_k);
	}

	*ppsaGid = psaGidAllocated;
	psaGidAllocated = NULL;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaGidAllocated);
	SG_NULLFREE(pCtx, pszGid_k);
}

										 
