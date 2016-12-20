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
 * @file sg_wc_tx__get_item_status_flags.c
 *
 * @details Compute the status flags for an item.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Compute the status flags for an item.  This is like a
 * __status_canonical() with depth 0 but cheaper.
 * 
 * Use this is you just want to know if an item is controlled
 * or uncontrolled, found or ignored, or etc. but don't care
 * about the details.
 *
 * If bNoIgnores is set, we treat an uncontrolled item as
 * FOUND and don't bother to classify it as FOUND vs IGNORED.
 *
 * If bNoTSC is set, we will not use the timestamp cache.
 * 
 * If the given repo-path is bogus and takes us off into
 * the weeds, we return __INVALID status.
 *
 * WARNING: because we now support moves/renames+deletes
 * (where you move or rename something and then delete it),
 * an entryname is no longer unique.  For example, the user
 * could do something like:
 *     vv move dirA/foo dirZ
 *     vv delete dirZ/foo
 *     vv move dirB/foo dirZ
 *     vv delete dirZ/foo
 *     vv move dirC/foo dirZ
 * 
 * [1] a STATUS("dirZ", depth=1) should report 2 deleted
 *     items and one present item with name "foo".
 * [2] a STATUS("dirZ/foo") will only tell you about the
 *     last one.
 *
 * The assumption is that if you're asking for an individual
 * item, you are doing something like Tortoise and asking
 * about something that you already saw in the directory.
 *
 * WARNING: In this case, the caller should verify that
 * the TYPE of the item we return info for matches the type
 * of the item that you observed.  For example, the user
 * could do something like:
 *     /bin/mv dirA old_dirA
 *     date > dirA
 *
 * thereby creating a file where vv thinks there is a directory.
 *
 * [1] a STATUS("@/", depth=1) will report:
 *     (LOST, directory, "dirA"),
 *     (FOUND, directory, "old_dirA"),
 *     (FOUND, file, "dirA")
 * [2] a STATUS("dirA") will only return the info for the
 *     (FOUND, file, "dirA")
 * 
 */
void SG_wc_tx__get_item_status_flags(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 const char * pszInput,
									 SG_bool bNoIgnores,
									 SG_bool bNoTSC,
									 SG_wc_status_flags * pStatusFlags,
									 SG_vhash ** ppvhProperties)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );
	SG_NULLARGCHECK_RETURN( pStatusFlags );
	// ppvhProperties is optional

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &chDomain)  );
#if TRACE_WC_TX_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__get_item_status_flags: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPath))  );
#endif
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath,
														  &bKnown, &pLVI)  );
	// if !bKnown, pLVI will be NULL and that is ok for the following.
	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
												bNoIgnores, bNoTSC,
												pStatusFlags)  );

#if TRACE_WC_TX_STATUS
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "SG_wc_tx__get_item_status_flags: [bNoIgnores %d][bNoTSC %d] on '%s' yields '0x%s'\n",
								   bNoIgnores, bNoTSC,
								   SG_string__sz(pStringRepoPath),
								   SG_uint64_to_sz__hex(*pStatusFlags,bufui64))  );
	}
#endif

	if (ppvhProperties)
	{
		SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, *pStatusFlags, ppvhProperties)  );

#if TRACE_WC_TX_STATUS
		SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console(pCtx, *ppvhProperties)  );
#endif
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}
