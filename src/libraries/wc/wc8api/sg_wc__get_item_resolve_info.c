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
 *
 *
 */
void SG_wc__get_item_resolve_info(SG_context * pCtx,
								  const SG_pathname* pPathWc,
								  const char * pszInput,
								  SG_vhash ** ppvhResolveInfo)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_item_resolve_info(pCtx, pWcTx,
													   pszInput,
													   ppvhResolveInfo)  )  );
}

