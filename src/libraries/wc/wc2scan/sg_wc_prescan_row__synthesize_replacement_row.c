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
 * @file sg_wc_prescan_row__synthesize_replacement_row.c
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

/**
 * Synthesize a scanrow with UNCONTROLLED (FOUND/IGNORED) status
 * as a clone/replacement for the given row.
 *
 * This should pretty much match the effect of __bind_leftovers__cb().
 *
 * You DO NOT own the returned row.
 *
 */ 
void sg_wc_prescan_row__synthesize_replacement_row(SG_context * pCtx,
												   sg_wc_prescan_row ** ppPrescanRow_Replacement,
												   SG_wc_tx * pWcTx,
												   const sg_wc_prescan_row * pPrescanRow_Input)
{
	sg_wc_prescan_row * pPrescanRow_Allocated = NULL;
	SG_bool bIsReserved = SG_FALSE;

	// ppPrescanRow_Replacement is optional

	// TODO 2012/10/05 We probably don't need this and should probably
	// TODO            assert that it isn't reserved.  Since we use this
	// TODO            routine to synthesize a FOUND/IGNORED item for an
	// TODO            item that was controlled and is becomming uncontrolled
	// TODO            and they shouldn't have been able to place something
	// TODO            with a reserved name under version control in the
	// TODO            first place.
	SG_ERR_CHECK(  sg_wc_db__path__is_reserved_entryname(pCtx, pWcTx->pDb,
														 SG_string__sz(pPrescanRow_Input->pStringEntryname),
														 &bIsReserved)  );

#if TRACE_WC_TX_SCAN
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Synthesizing (%s) replacement row for: %s\n",
							   ((bIsReserved) ? "reserved" : "uncontrolled"),
							   SG_string__sz(pPrescanRow_Input->pStringEntryname))  );
#endif

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPrescanRow_Allocated)  );

	// Generate a TEMPORARY GID for it.  I hate to do
	// this for uncontrolled items since it led to a
	// few problems in the original PendingTree code,
	// but it solves more than it creates.

	SG_ERR_CHECK(  sg_wc_db__gid__insert_new(pCtx, pWcTx->pDb, SG_TRUE,
											 &pPrescanRow_Allocated->uiAliasGid)  );

	pPrescanRow_Allocated->pPrescanDir_Ref = pPrescanRow_Input->pPrescanDir_Ref;
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
										  &pPrescanRow_Allocated->pStringEntryname,
										  pPrescanRow_Input->pStringEntryname)  );

	SG_ERR_CHECK(  SG_WC_READDIR__ROW__ALLOC__COPY(pCtx,
												   &pPrescanRow_Allocated->pRD,
												   pPrescanRow_Input->pRD)  );
	pPrescanRow_Allocated->pRD->pPrescanRow = pPrescanRow_Allocated;	// back ptr

	pPrescanRow_Allocated->pTneRow = NULL;
	pPrescanRow_Allocated->pPcRow_Ref = NULL;

	pPrescanRow_Allocated->tneType = pPrescanRow_Input->tneType;

	if (bIsReserved)
	{
		// Mark it as RESERVED.  This is a special kind of uncontrolled,
		// but different from FOUND/IGNORED because it CANNOT EVER become
		// controlled.  (You *really* don't want to be able to place .sgdrawer
		// under version control, for example.)
		pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__RESERVED;
	}
	else
	{
		// mark it as generic uncontrolled at this level.  we let STATUS
		// qualify it as FOUND or IGNORED dynamically and upon demand.
		pPrescanRow_Allocated->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__UNCONTROLLED_UNQUALIFIED;
	}

	// scandir borrows pointer to scanrow.
	SG_ERR_CHECK(  SG_rbtree_ui64__add__with_assoc(pCtx,
												   pPrescanRow_Allocated->pPrescanDir_Ref->prb64PrescanRows,
												   pPrescanRow_Allocated->uiAliasGid,
												   pPrescanRow_Allocated)  );

	// row-cache takes ownership of scanrow.
	SG_ERR_CHECK(  sg_wc_tx__cache__add_prescan_row(pCtx, pWcTx, pPrescanRow_Allocated)  );

	if (ppPrescanRow_Replacement)
		*ppPrescanRow_Replacement = pPrescanRow_Allocated;
	return;

fail:
	SG_WC_PRESCAN_ROW__NULLFREE(pCtx, pPrescanRow_Allocated);
}
