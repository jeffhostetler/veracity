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
 * @file sg_wc__get_wc_top.c
 *
 * @details Return the pathname of the TOP of the WD.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc__get_wc_top(SG_context * pCtx,
					   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
					   SG_pathname ** ppPath)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_wc_top(pCtx, pWcTx, ppPath)  )  );
}

