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
 * @file sg_wc__jsglue__safeptr.c
 *
 * @details Safe Pointers for use by wc-related rouines from JavaScript.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#define SG_WC_TX__SAFEPTR_TYPE           "sg_wc_tx"

void sg_wc__jsglue__safeptr__wrap__wc_tx(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 SG_js_safeptr ** ppSafePtr)
{
	SG_ERR_CHECK_RETURN(  SG_js_safeptr__wrap__generic(pCtx,
													   pWcTx,
													   SG_WC_TX__SAFEPTR_TYPE,
													   ppSafePtr)  );
}

void sg_wc__jsglue__safeptr__unwrap__wc_tx(SG_context * pCtx,
										   SG_js_safeptr * pSafePtr,
										   SG_wc_tx ** ppWcTx)
{
	SG_ERR_CHECK_RETURN(  SG_js_safeptr__unwrap__generic(pCtx,
														 pSafePtr,
														 SG_WC_TX__SAFEPTR_TYPE,
														 (void **)ppWcTx)  );
}


