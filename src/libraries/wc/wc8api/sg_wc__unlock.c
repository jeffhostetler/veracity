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
 * We get called once for each input in the stringarray
 * of files to unlock.
 *
 * Validate it and find the item's GID and add it to
 * the given VHASH.
 *
 * TODO 2012/03/02 This is a little different from the
 * TODO            version in sg_wc__lock.c.  Is this
 * TODO            really necessary.
 * 
 */
static void _map_input(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   SG_vhash * pvh_gids,
					   const char * pszInput)
{
	SG_string * pStringRepoPath = NULL;
	char * pszGid = NULL;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_wc_status_flags statusFlags;
	SG_bool bKnown;
	char cDomain;

	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &cDomain)  );
#if TRACE_WC_LOCK
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc__unlock: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, cDomain, SG_string__sz(pStringRepoPath))  );
#endif

	// find the GID/alias of the named item while taking
	// account whatever domain component.  (that is, they
	// could have said "@g12345...." or "@b/....." rather
	// than a live path.)
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

	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		SG_ERR_THROW2(  SG_ERR_VC_LOCK_FILES_ONLY,
                        (pCtx, "%s", pszInput)  );

	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
												SG_TRUE,	// --no-ignores (faster)
												SG_FALSE,	// trust TSC
												&statusFlags)  );
	if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND
					   |SG_WC_STATUS_FLAGS__U__IGNORED))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "%s", pszInput)  );
	// TODO 2012/03/02 Not sure checking for __S__ADDED is needed on an unlock.
	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
		SG_ERR_THROW2(  SG_ERR_VC_LOCK_NOT_COMMITTED_YET,
						(pCtx, "%s", pszInput)  );

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
	SG_ASSERT_RELEASE_FAIL2(  (pszGid[0] == 'g'),	// we get 't' domain IDs for uncontrolled items
							  (pCtx, "%s has temp id %s", pszInput, pszGid)  );

	// TODO 2012/03/02 The original PendingTree version of the
	// TODO            code did a SG_vhash__add__null() which
	// TODO            will throw on duplicates.  I'm going to
	// TODO            soften that.

	SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh_gids, pszGid)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

/**
 * Request to UNLOCK on one or more files.
 *
 * WARNING: This routine deviates from the model of most
 * WARNING: of the SG_wc__ level-8 and/or SG_wc_tx level-7
 * WARNING: API routines because we cannot just "queue" an
 * WARNING: unlock like we do a RENAME with everything
 * WARNING: contained within the pWcTx; we actually have
 * WARNING: to update the locks dag (which is outside of
 * WARNING: the scope of the WC TX).
 * WARNING:
 * WARNING: So we only have a level-8 API
 * WARNING: so that we can completely control/bound the TX.
 *
 * We also deviate in that we don't take a --test
 * nor --verbose option.  Which means we don't have a
 * JOURNAL to mess with.
 *
 */
void SG_wc__unlock(SG_context * pCtx,
				   const SG_pathname* pPathWc,
				   const SG_stringarray * psaInputs,
				   SG_bool bForce,
				   const char * psz_username,
				   const char * psz_password,
				   const char * psz_repo_upstream)
{
	SG_wc_tx * pWcTx = NULL;
    SG_audit q;
	SG_uint32 nrInputs = 0;
	SG_uint32 k;
    char * psz_tied_branch_name = NULL;
	char * psz_repo_upstream_allocated = NULL;
	SG_vhash * pvh_gids = NULL;
	const char * pszRepoDescriptorName = NULL;	// we do not own this

	if (psaInputs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &nrInputs)  );
	if (nrInputs == 0)
        SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Nothing to unlock")  );

	// psz_username is optional (assume no auth required)
	// psz_password is optional (assume no auth required)
	// psz_server is optional (assume default server)

	// Begin a WC TX so that we get all of the good stuff
	// (like mapping the CWD into a REPO handle and mapping
	// the inputs into GIDs).
	//
	// At this point I don't believe that setting a lock
	// will actually make any changes in WC.DB, so I'm
	// making it a READ-ONLY TX.
	//
	// My assumption is that the lock actually gets
	// written to the Locks DAG and shared with the server.
	// But I still want a TX handle for all of the other stuff.

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_FALSE)  );

	// We need the repo descriptor name later for the push/pull
	// and to optionally look up the default destination for
	// this repo.  The pRepo stores this *IFF* it was properly
	// opened (using a name).

	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pWcTx->pDb->pRepo,
												&pszRepoDescriptorName)  );
	SG_ASSERT_RELEASE_FAIL2(  (pszRepoDescriptorName && *pszRepoDescriptorName),
							  (pCtx, "SG_wc__unlock: Could not get repo descriptor name.")  );

    // now we need to know what branch we are tied to.
    // if we're not tied, fail
    SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pWcTx, &psz_tied_branch_name)  );
    if (!psz_tied_branch_name)
        SG_ERR_THROW(  SG_ERR_NOT_TIED  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_gids)  );
	for (k=0; k<nrInputs; k++)
	{
		const char * pszInput_k;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
		SG_ERR_CHECK(  _map_input(pCtx, pWcTx, pvh_gids, pszInput_k)  );
	}

	if (!psz_repo_upstream)
	{
		SG_localsettings__descriptor__get__sz(pCtx, pszRepoDescriptorName,
											  "paths/default",
											  &psz_repo_upstream_allocated);
		if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
			SG_ERR_REPLACE_ANY_RETHROW(  SG_ERR_NO_SERVER_SPECIFIED  );
		else
			SG_ERR_CHECK_CURRENT;

		psz_repo_upstream = psz_repo_upstream_allocated;
	}

	SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pWcTx->pDb->pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	
    // OK, we have all the pieces.  Time to call the unlock code
    SG_ERR_CHECK(  SG_vc_locks__unlock(
                pCtx,
                pszRepoDescriptorName,
				psz_repo_upstream,
				psz_username,
				psz_password,
                psz_tied_branch_name,
                pvh_gids,
				bForce,
                &q
                )  );

	// Fall through and let the normal fail code discard/cancel
	// the read-only WC TX.  This will not affect the Locks DAG
	// nor the server.

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_NULLFREE(pCtx, psz_tied_branch_name);
    SG_NULLFREE(pCtx, psz_repo_upstream_allocated);
    SG_VHASH_NULLFREE(pCtx, pvh_gids);
}
