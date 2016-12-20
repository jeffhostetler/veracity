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
 * @file sg_wc_prescan_row__root.c
 *
 * @details Routine to create a special SCANROW for the root
 * directory (the one corresponding to "@/").  This row is NOT
 * part of a SCANDIR.
 *
 * TODO 2011/09/14 Would it be better to have a pseudo scandir
 * TODO            for the null root (the container of "@/" in
 * TODO            the treenode hierarchy) to have better
 * TODO            symmetry.  The only thing that the root scanrow
 * TODO            needs to record is the attrbits on the
 * TODO            root directory itself and we currently don't
 * TODO            have any attrbits that apply to directories
 * TODO            so I'm not sure it matters much.  The main thing
 * TODO            is how it affects the start of our recursions.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Alloc a special scanrow for the root directory; the one
 * corresponding to "@/".
 *
 * The alias-gid of the root directory is not a compile-time
 * constant like the null-root is, so it must be passed in.
 * I'm wondering if we should make it a fixed constant like
 * we do for the null-root.
 *
 * The scanrow is added to the scanrow-cache in the TX,
 * so you DO NOT own the returned pointer.
 * 
 */
void sg_wc_prescan_row__alloc__row_root(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_prescan_row ** ppPrescanRow)
{
	sg_wc_prescan_row * pPrescanRow = NULL;
	sg_wc_pctne__row * pPT = NULL;
	SG_uint64 uiAliasGidParent_Found;
	SG_treenode_entry_type tneType_Found;
	SG_bool bFound;

	// we assume that __ALIAS_GID__{UNDEFINED,NULL_ROOT} are the
	// first aliases created and therefore the alias for "@/" must
	// be larger than that.
	SG_ASSERT_RELEASE_RETURN(  (pWcTx->uiAliasGid_Root > SG_WC_DB__ALIAS_GID__NULL_ROOT)   );
	
	if (pWcTx->pPrescanRow_Root)
	{
		*ppPrescanRow = pWcTx->pPrescanRow_Root;
		return;
	}
	
	// We primarily use this to get a PCTNE row created
	// that we can then steal the parts from.

	SG_ERR_CHECK(  sg_wc_pctne__get_row_by_alias(pCtx,
												 pWcTx->pDb, pWcTx->pCSetRow_Baseline,
												 pWcTx->uiAliasGid_Root,
												 &bFound, &pPT)  );

	SG_ERR_CHECK(  sg_wc_pctne__row__get_tne_type(pCtx, pPT, &tneType_Found)  );
	SG_ERR_CHECK(  sg_wc_pctne__row__get_current_parent_alias(pCtx, pPT, &uiAliasGidParent_Found)  );
	SG_ASSERT_RELEASE_FAIL(  (tneType_Found == SG_TREENODEENTRY_TYPE_DIRECTORY)  );
	SG_ASSERT_RELEASE_FAIL(  (uiAliasGidParent_Found == SG_WC_DB__ALIAS_GID__NULL_ROOT)  );

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPrescanRow)  );

	// Since the actual name of the ROOT directory (as it
	// appears on disk in the directory above the working
	// directory) is OUTSIDE the scope of our control,
	// we NEVER do a READDIR on the parent directory so we
	// will NEVER have a SCANDIR for the NULL-ROOT directory.
	// 
	// So we don't have a backptr because this row is not
	// in a scandir.

	pPrescanRow->pPrescanDir_Ref = NULL;

	pPrescanRow->uiAliasGid = pWcTx->uiAliasGid_Root;

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pPrescanRow->pStringEntryname, "@")  );
	pPrescanRow->tneType = SG_TREENODEENTRY_TYPE_DIRECTORY;

	// steal the data within the pPT.
	pPrescanRow->pTneRow    = pPT->pTneRow;   pPT->pTneRow = NULL;
	pPrescanRow->pPcRow_Ref = pPT->pPcRow;    pPT->pPcRow  = NULL;
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);

	// Create a trivial READDIR_ROW for the root directory.
	// We don't so much need the data from the disk as we
	// need the pointer to not be null.
	SG_ERR_CHECK(  SG_WC_READDIR__ROW__ALLOC__ROW_ROOT(pCtx, &pPrescanRow->pRD,
													   pWcTx->pDb->pPathWorkingDirectoryTop)  );
	pPrescanRow->pRD->pPrescanRow = pPrescanRow;	// set backptr

	pPrescanRow->scan_flags_Ref = SG_WC_PRESCAN_FLAGS__CONTROLLED_ROOT;

	// add the row to the row-cache in the tx.

	SG_ERR_CHECK(  sg_wc_tx__cache__add_prescan_row(pCtx, pWcTx, pPrescanRow)  );

	*ppPrescanRow = pPrescanRow;
	return;

fail:
	SG_WC_PRESCAN_ROW__NULLFREE(pCtx, pPrescanRow);
}
