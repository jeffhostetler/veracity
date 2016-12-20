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
 * Build a vhash of the csets involved with the current WC.
 * If no pending merge, this is:
 *
 * var csets = { "L0" : "<hid_L0>" };
 *
 * If we have a pending merge, this is:
 *
 * var csets = { "L0" : "<hid_L0>",
 *               "L1" : "<hid_L1>",
 *               "A"  : "<hid_ancestor>",
 *               "S0" : "<hid_spca_0>",
 *               .... };
 *
 */
static sg_wc_db__csets__foreach_cb _get_wc_csets__vhash__cb;

static void _get_wc_csets__vhash__cb(SG_context * pCtx,
									 void * pVoidData,
									 sg_wc_db__cset_row ** ppCSetRow)
{
	SG_vhash * pvhList = (SG_vhash *)pVoidData;

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx,
											 pvhList,
											 (*ppCSetRow)->psz_label,
											 (*ppCSetRow)->psz_hid_cset)  );

fail:
	return;
}

void SG_wc_tx__get_wc_csets__vhash(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   SG_vhash ** ppvhCSets)
{
	SG_vhash * pvhCSets = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppvhCSets );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhCSets)  );
	SG_ERR_CHECK(  sg_wc_db__csets__foreach_in_wc(pCtx,
												  pWcTx->pDb,
												  _get_wc_csets__vhash__cb,
												  (void *)pvhCSets)  );
	*ppvhCSets = pvhCSets;
	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhCSets);
}

