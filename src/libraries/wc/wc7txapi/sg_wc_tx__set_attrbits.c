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
 * @file sg_wc_tx__set_attrbits.c
 *
 * @details Handle a CHMOD.  This is in the public API because
 * I want to maintain the structure of the level-7 tx api, but
 * doesn't actually need to be visible to the public.  I have
 * it here to try to allow the MERGE code to use the level-7 tx
 * api for everything.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * WARNING 2012/01/30: I'm not sure that I want this
 * routine in the official public API.  I'm not sure
 * it matters one way or the other.
 *
 * Normally, we *detect* changes to ATTRBITS because
 * the user does a CHMOD on the item.
 *
 * We *DO NOT* store ATTRBITS in the tbl_pc table;
 * rather we always have to sniff out the dynamic
 * (as opposed to structural) fields.
 *
 * The intent of this API routine is to allow commands
 * like MERGE, UPDATE, and REVERT request/queue such a
 * change during the TX.
 *
 * For clutters sake, we probably don't want to have
 * a 'vv' command that exposes it; but it might be
 * nice to have a JS glue wrapper (at the sg_wc_tx api
 * layer) that does.
 *
 */
void SG_wc_tx__set_attrbits(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput_Src,
							SG_int64 attrbits,
							SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src = NULL;
	sg_wc_liveview_item * pLVI_Src;			// we do not own this
	char chDomain_Src;
	SG_bool bKnown_Src;

	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput_Src,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath_Src, &chDomain_Src)  );

#if TRACE_WC_TX_SET
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__set_attrbits: source '%s' normalized to [domain %c] '%s'\n",
							   pszInput_Src, chDomain_Src, SG_string__sz(pStringRepoPath_Src))  );
#endif
	
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath_Src,
														  &bKnown_Src, &pLVI_Src)  );
	if (!bKnown_Src || (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_Src->scan_flags_Live)))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Source '%s' (%s)",
						 pszInput_Src, SG_string__sz(pStringRepoPath_Src))  );

	SG_ERR_CHECK(  sg_wc_tx__rp__set_attrbits__lvi(pCtx, pWcTx, pLVI_Src, attrbits, xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src);
}
