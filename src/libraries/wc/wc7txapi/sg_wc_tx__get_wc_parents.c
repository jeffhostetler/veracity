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
 * @file sg_wc_tx__get_wc_parents.c
 *
 * @details Routines to return the set of parents of the
 * current WD in various formats.
 *
 * It DOES NOT deal with historical questions.
 *
 * TODO 2011/11/01 I have 2 versions of __get_wc_parents()
 * TODO            (returning a varray or stringarray).
 * TODO            It'd be nice to be able to consolidate these
 * TODO            into just 1 format.
 * TODO
 * TODO            Part of this comes from the original PendingTree
 * TODO            code which had:
 * TODO                SG_pendingtree__get_wd_parents()     -- varray
 * TODO                SG_pendingtree__get_wd_parents_ref() -- const varray
 * TODO                SG_parents__list()                   -- stringarray
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Build a varray of the parents of the current WC.
 * This looks something like:
 *
 * var parents = [ "<hid_0>",
 *                 "<hid_1>" ];
 *
 * By convention, parents[0] is the baseline.
 * And we assume that sg_wc_db__csets__foreach_in_wc()
 * sorts the items by name.
 *
 */
static sg_wc_db__csets__foreach_cb _get_parents__varray__cb;

static void _get_parents__varray__cb(SG_context * pCtx,
									 void * pVoidData,
									 sg_wc_db__cset_row ** ppCSetRow)
{
	SG_varray * pvaList = (SG_varray *)pVoidData;

	if ((*ppCSetRow)->psz_label[0] == 'L')
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,
													 pvaList,
													 (*ppCSetRow)->psz_hid_cset)  );

fail:
	return;
}

void SG_wc_tx__get_wc_parents__varray(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  SG_varray ** ppvaParents)
{
	SG_varray * pvaParents = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppvaParents );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaParents)  );
	SG_ERR_CHECK(  sg_wc_db__csets__foreach_in_wc(pCtx,
												  pWcTx->pDb,
												  _get_parents__varray__cb,
												  (void *)pvaParents)  );
	*ppvaParents = pvaParents;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
}

//////////////////////////////////////////////////////////////////

/**
 * Build a stringarray of the parents of the current WC.
 * This looks something like:
 *
 * var parents = [ "<hid_0>",
 *                 "<hid_1>" ];
 *
 * By convention, parents[0] is the baseline.
 * And we assume that sg_wc_db__csets__foreach_in_wc()
 * sorts the items by name.
 *
 */
static sg_wc_db__csets__foreach_cb _get_parents__stringarray__cb;

static void _get_parents__stringarray__cb(SG_context * pCtx,
										  void * pVoidData,
										  sg_wc_db__cset_row ** ppCSetRow)
{
	SG_stringarray * psaList = (SG_stringarray *)pVoidData;

	if ((*ppCSetRow)->psz_label[0] == 'L')
		SG_ERR_CHECK(  SG_stringarray__add(pCtx,
										   psaList,
										   (*ppCSetRow)->psz_hid_cset)  );

fail:
	return;
}

void SG_wc_tx__get_wc_parents__stringarray(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   SG_stringarray ** ppsaParents)
{
	SG_stringarray * psaParents = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppsaParents );

	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaParents, 10)  );
	SG_ERR_CHECK(  sg_wc_db__csets__foreach_in_wc(pCtx,
												  pWcTx->pDb,
												  _get_parents__stringarray__cb,
												  (void *)psaParents)  );
	*ppsaParents = psaParents;
	return;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaParents);
}

//////////////////////////////////////////////////////////////////

static void _get_baseline__merge(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 char ** ppszHidBaseline,
								 SG_bool * pbHasMerge)
{
	SG_varray * pvaParents = NULL;
	SG_uint32 count = 0;
	const char * psz_0;
		
	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx, pWcTx, &pvaParents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &count)  );
	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 0, &psz_0)  );

	SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_0, ppszHidBaseline)  );
	*pbHasMerge = (count > 1);

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
}

static void _get_baseline(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  char ** ppszHidBaseline)
{
	sg_wc_db__cset_row * pCSetRow = NULL;

	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx,
													 pWcTx->pDb,
													 "L0",
													 NULL,
													 &pCSetRow)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pCSetRow->psz_hid_cset, ppszHidBaseline)  );

fail:
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, pCSetRow);
}

/**
 * Get the HID of the baseline.
 *
 * Optionally determine if we currently have a MERGE in the WD.
 *
 */
void SG_wc_tx__get_wc_baseline(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   char ** ppszHidBaseline,
							   SG_bool * pbHasMerge)
{

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppszHidBaseline );
	// pbHasMerge is optional
	
	if (pbHasMerge)
		SG_ERR_CHECK(  _get_baseline__merge(pCtx, pWcTx, ppszHidBaseline, pbHasMerge)  );
	else
		SG_ERR_CHECK(  _get_baseline(pCtx, pWcTx, ppszHidBaseline)  );

fail:
	return;
}
