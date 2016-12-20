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
 * @file sg_wc_tx__flush_timestamp_cache.c
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
 * Flush the contents of the timestamp cache.
 *
 * This may or may not only update the in-memory
 * cache; that is, it may defer the TSC save until
 * the end of the TX.
 *
 */
void SG_wc_tx__flush_timestamp_cache(SG_context * pCtx, SG_wc_tx * pWcTx)
{
	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK_RETURN(  sg_wc_db__timestamp_cache__remove_all(pCtx, pWcTx->pDb)  );
}
