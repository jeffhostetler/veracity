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
 * @file sg_wc_pctne__get_row_by_alias.c
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
 * Get the corresponding rows from the PC table
 * and the tne_L0 table for the requested alias.
 *
 */
void sg_wc_pctne__get_row_by_alias(SG_context * pCtx,
								   sg_wc_db * pDb,
								   const sg_wc_db__cset_row * pCSetRow,
								   SG_uint64 uiAliasGid,
								   SG_bool * pbFound,
								   sg_wc_pctne__row ** ppPT)
{
	sg_wc_pctne__row * pPT = NULL;
	SG_bool bFoundInPc = SG_FALSE;
	SG_bool bFoundInTne = SG_FALSE;

	SG_ERR_CHECK(  SG_WC_PCTNE__ROW__ALLOC(pCtx, &pPT)  );
	SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_alias(pCtx, pDb, pCSetRow,
												   uiAliasGid,
												   &bFoundInTne, &pPT->pTneRow)  );
	SG_ERR_CHECK(  sg_wc_db__pc__get_row_by_alias(pCtx, pDb, pCSetRow,
												  uiAliasGid,
												  &bFoundInPc, &pPT->pPcRow)  );

	if (bFoundInTne || bFoundInPc)
	{
		if (pbFound)
			*pbFound = SG_TRUE;
		*ppPT = pPT;
		return;
	}

	if (pbFound)
	{
		*pbFound = SG_FALSE;
		*ppPT = NULL;
	}
	else
	{
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
	}

fail:
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
}


