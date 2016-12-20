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
 * @file sg_wc_tx__get_wc_top.c
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

/**
 * Return the pathname of the top of the WD.
 * See SG_wc_tx__get_wc_info() if you need more
 * than just this one field.
 *
 * You own the returned string.
 *
 */
void SG_wc_tx__get_wc_top(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  SG_pathname ** ppPath)
{
	SG_ERR_CHECK_RETURN(  SG_PATHNAME__ALLOC__COPY(pCtx,
												   ppPath,
												   pWcTx->pDb->pPathWorkingDirectoryTop)  );
}

