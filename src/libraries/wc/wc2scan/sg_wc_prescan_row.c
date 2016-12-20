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
 * @file sg_wc_prescan_row.c
 *
 * @details Routines to manipulate a sg_wc_prescan_row.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_row__free(SG_context * pCtx, sg_wc_prescan_row * pPrescanRow)
{
	if (!pPrescanRow)
		return;

	// we do not own the pPrescanDir backptrs.
	SG_WC_READDIR__ROW__NULLFREE(pCtx, pPrescanRow->pRD);
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pPrescanRow->pTneRow);
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPrescanRow->pPcRow_Ref);
	SG_STRING_NULLFREE(pCtx, pPrescanRow->pStringEntryname);

	SG_NULLFREE(pCtx, pPrescanRow);
}

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_row__get_tne_type(SG_context * pCtx,
									 const sg_wc_prescan_row * pPrescanRow,
									 SG_treenode_entry_type * pTneType)
{
	SG_UNUSED( pCtx );

	*pTneType = pPrescanRow->tneType;
}

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_row__get_current_entryname(SG_context * pCtx,
											  const sg_wc_prescan_row * pPrescanRow,
											  const char ** ppszEntryname)
{
	SG_UNUSED( pCtx );

	*ppszEntryname = SG_string__sz(pPrescanRow->pStringEntryname);

	// TODO 2011/08/24 I should assert that this value matches the
	// TODO            current values in both the pRD and the pPT.
}


void sg_wc_prescan_row__get_current_parent_alias(SG_context * pCtx,
												 const sg_wc_prescan_row * pPrescanRow,
												 SG_uint64 * puiAliasGidParent)
{
	SG_UNUSED( pCtx );

	*puiAliasGidParent = pPrescanRow->pPrescanDir_Ref->uiAliasGidDir;

	// TODO 2011/08/24 I should assert that this value matches
	// TODO            the value in pRD and the pPT.
}
