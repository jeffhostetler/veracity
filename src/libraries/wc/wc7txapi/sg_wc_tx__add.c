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
 * @file sg_wc_tx__add.c
 *
 * @details Handle details of 'vv add'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc_tx__add(SG_context * pCtx,
				   SG_wc_tx * pWcTx,
				   const char * pszInput,
				   SG_uint32 depth,
				   SG_bool bNoIgnores)
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
#if TRACE_WC_TX_ADD
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__add: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPath))  );
#endif

	// For ADD it may make sense to restrict the domain component
	// to '/' or maybe 't' (from the point of view of only needing
	// to be able to add something that is uncontrolled), but when
	// consider the depth option, they might just say something
	// like "vv add ." (when it is already controlled as a cheap
	// way of adding anything not controlled).  so we might as well
	// allow any domain component that makes sense on a WD.
	//
	// find the GID/alias of the named item while taking
	// account whatever domain component.
	//
	// Fetch the LVI for this item.  This may implicitly
	// cause a SCANDIR/READIR and/or sync up with the DB.
	// This is good because it also means we will get the
	// exact-match stuff on each component within the
	// pathname.

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

	SG_ERR_CHECK(  sg_wc_tx__rp__add__lvi(pCtx, pWcTx, pLVI, depth, bNoIgnores)  );
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__add__stringarray(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const SG_stringarray * psaInputs,
								SG_uint32 depth,
								SG_bool bNoIgnores)
{
	SG_uint32 k, count;

	SG_NULLARGCHECK_RETURN( pWcTx );

	if (psaInputs)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  SG_wc_tx__add(pCtx, pWcTx, pszInput_k, depth, bNoIgnores)  );
		}
	}
	else
	{
		// if no args, we DO NOT assume "@/" for an ADD.
		// (caller should have thrown USAGE if apropriate.)
		
		SG_NULLARGCHECK_RETURN( psaInputs );
	}

fail:
	return;
}
