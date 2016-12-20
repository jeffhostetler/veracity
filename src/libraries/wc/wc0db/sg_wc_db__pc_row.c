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
 * @file sg_wc_db__pc_row.c
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

void sg_wc_db__pc_row__free(SG_context * pCtx, sg_wc_db__pc_row * pPcRow)
{
	if (!pPcRow)
		return;

	SG_WC_DB__STATE_STRUCTURAL__NULLFREE(pCtx, pPcRow->p_s);
	SG_NULLFREE(pCtx, pPcRow->pszHidMerge);
	SG_WC_DB__STATE_DYNAMIC__NULLFREE(pCtx, pPcRow->p_d_sparse);
	SG_NULLFREE(pCtx, pPcRow);
}

void sg_wc_db__pc_row__alloc(SG_context * pCtx, sg_wc_db__pc_row ** ppPcRow)
{
	sg_wc_db__pc_row * pPcRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPcRow)  );
	SG_ERR_CHECK(  sg_wc_db__state_structural__alloc(pCtx, &pPcRow->p_s)  );
	// pPcRow->pszHidMerge is only set when needed.
	// pPcRow->p_d_sparse is only set when needed.

	*ppPcRow = pPcRow;
	return;

fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
}

void sg_wc_db__pc_row__alloc__copy(SG_context * pCtx,
								   sg_wc_db__pc_row ** ppPcRow,
								   const sg_wc_db__pc_row * pPcRow_src)
{
	sg_wc_db__pc_row * pPcRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPcRow)  );

	pPcRow->flags_net = pPcRow_src->flags_net;
	SG_ERR_CHECK(  sg_wc_db__state_structural__alloc__copy(pCtx,
														   &pPcRow->p_s,
														   pPcRow_src->p_s)  );
	if (pPcRow_src->pszHidMerge && *pPcRow_src->pszHidMerge)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pPcRow_src->pszHidMerge, &pPcRow->pszHidMerge)  );

	if (pPcRow_src->p_d_sparse)
		SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc__copy(pCtx,
															&pPcRow->p_d_sparse,
															pPcRow_src->p_d_sparse)  );

	pPcRow->ref_attrbits = pPcRow_src->ref_attrbits;

	*ppPcRow = pPcRow;
	return;

fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
}

void sg_wc_db__pc_row__alloc__from_tne_row(SG_context * pCtx,
										   sg_wc_db__pc_row ** ppPcRow,
										   const sg_wc_db__tne_row * pTneRow,
										   SG_bool bMarkItSparse)
{
	sg_wc_db__pc_row * pPcRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPcRow)  );

	pPcRow->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;
	SG_ERR_CHECK(  sg_wc_db__state_structural__alloc__copy(pCtx,
														   &pPcRow->p_s,
														   pTneRow->p_s)  );
	pPcRow->pszHidMerge = NULL;

	if (bMarkItSparse)
	{
		pPcRow->flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE;
		SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc__copy(pCtx,
															&pPcRow->p_d_sparse,
															pTneRow->p_d)  );
	}

	pPcRow->ref_attrbits = pTneRow->p_d->attrbits;

	*ppPcRow = pPcRow;
	return;

fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
}

void sg_wc_db__pc_row__alloc__from_readdir(SG_context * pCtx,
										   sg_wc_db__pc_row ** ppPcRow,
										   const sg_wc_readdir__row * pRD,
										   SG_uint64 uiAliasGid,
										   SG_uint64 uiAliasGidParent)
{
	sg_wc_db__pc_row * pPcRow = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPcRow)  );
	SG_ERR_CHECK(  sg_wc_db__state_structural__alloc__fields(pCtx,
															 &pPcRow->p_s,
															 uiAliasGid,
															 uiAliasGidParent,
															 SG_string__sz(pRD->pStringEntryname),
															 pRD->tneType)  );

	pPcRow->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;
	pPcRow->pszHidMerge = NULL;

	// since we got the info from readdir, the item isn't sparse.

	// The observed attrbits.
	// TODO 2013/02/06 Not sure I like this here.
	if (pRD->pAttrbits)
		pPcRow->ref_attrbits = *pRD->pAttrbits;
	else
		pPcRow->ref_attrbits = 0;

	*ppPcRow = pPcRow;
	return;

fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
	
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void sg_wc_db__debug__pc_row__print(SG_context * pCtx, const sg_wc_db__pc_row * pPcRow)
{
	SG_int_to_string_buffer bufui64_a;
	SG_int_to_string_buffer bufui64_b;
	SG_int_to_string_buffer bufui64_c;
	SG_int_to_string_buffer bufui64_d;
	SG_int_to_string_buffer bufui64_e;
		
	if (pPcRow->p_d_sparse)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db:pc_row: [gid %s][par %s] [flags net %s][hid-merge %s] [sparse %s,%s] [ref_attr %s] %s\n",
								   SG_uint64_to_sz(pPcRow->p_s->uiAliasGid,       bufui64_a),
								   SG_uint64_to_sz(pPcRow->p_s->uiAliasGidParent, bufui64_b),
								   SG_uint64_to_sz(pPcRow->flags_net, bufui64_c),
								   ((pPcRow->pszHidMerge) ? pPcRow->pszHidMerge : ""),
								   ((pPcRow->p_d_sparse->pszHid) ? pPcRow->p_d_sparse->pszHid : ""),
								   SG_uint64_to_sz(pPcRow->p_d_sparse->attrbits, bufui64_d),
								   SG_uint64_to_sz(pPcRow->ref_attrbits, bufui64_e),
								   pPcRow->p_s->pszEntryname)  );
	else
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_db:pc_row: [gid %s][par %s] [flags net %s][hid-merge %s] [ref_attr %s] %s\n",
								   SG_uint64_to_sz(pPcRow->p_s->uiAliasGid,       bufui64_a),
								   SG_uint64_to_sz(pPcRow->p_s->uiAliasGidParent, bufui64_b),
								   SG_uint64_to_sz(pPcRow->flags_net, bufui64_c),
								   ((pPcRow->pszHidMerge) ? pPcRow->pszHidMerge : ""),
								   SG_uint64_to_sz(pPcRow->ref_attrbits, bufui64_e),
								   pPcRow->p_s->pszEntryname)  );
}
#endif


