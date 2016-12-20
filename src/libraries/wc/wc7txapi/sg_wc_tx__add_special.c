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
 * @file sg_wc_tx__add_special.c
 *
 * @details Special version of ADD used by MERGE to add items
 * present in the other branch (with a known GID).
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc_tx__add_special(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const char * pszInput_Dir,
						   const char * pszEntryname,
						   const char * pszGid,
						   SG_treenode_entry_type tneType,
						   const char * pszHidBlob,
						   SG_int64 attrbits,
						   SG_wc_status_flags statusFlagsAddSpecialReason)
{
	SG_string * pStringRepoPath_Dir = NULL;
	sg_wc_liveview_item * pLVI_Dir;			// we do not own this
	SG_uint64 uiAliasGid;
	SG_bool bKnown;
	char chDomain;

	// we accept the "input" path of the desired parent
	// directory and the item's entryname.  we DO NOT
	// allow a full input/repo-path to the item itself
	// because it won't be present yet.

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput_Dir );	// throw if omitted, do not assume "@/".
	SG_NONEMPTYCHECK_RETURN( pszEntryname );
	SG_NONEMPTYCHECK_RETURN( pszGid );
	if (tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		SG_NONEMPTYCHECK_RETURN( pszHidBlob );

	// we now allow merge- or update-create with or without sparse bit.
	SG_ARGCHECK_RETURN( (statusFlagsAddSpecialReason & (SG_WC_STATUS_FLAGS__S__MERGE_CREATED | SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)),
						statusFlagsAddSpecialReason);

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Dir,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Dir, &chDomain)  );
#if TRACE_WC_TX_ADD
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__add_special: Dir '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Dir, chDomain, SG_string__sz(pStringRepoPath_Dir))  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_Dir,
														  &bKnown, &pLVI_Dir)  );
	if (!bKnown)
	{
		// We only get this if the path is completely bogus and
		// took us off into the weeds (as opposed to reporting
		// something just not-controlled).
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown parent directory '%s'.", SG_string__sz(pStringRepoPath_Dir))  );
	}

	SG_ERR_CHECK(  sg_wc_db__gid__get_or_insert_alias_from_gid(pCtx, pWcTx->pDb, pszGid, &uiAliasGid)  );

	SG_ERR_CHECK(  sg_wc_tx__rp__add_special__lvi_sz(pCtx, pWcTx,
													 pLVI_Dir, pszEntryname,
													 uiAliasGid, tneType,
													 pszHidBlob, attrbits,
													 statusFlagsAddSpecialReason)  );
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Dir);
}
