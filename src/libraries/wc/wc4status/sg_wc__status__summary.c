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
 * @file sg_wc__status__summary.c
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
 * We get called once for each item in the canonical status.
 * Accumulate the flag bits into one large union.
 *
 */
static SG_varray_foreach_callback _summary_cb;

static void _summary_cb(SG_context * pCtx,
						void * pVoidData,
						const SG_varray * pva,
						SG_uint32 ndx,
						const SG_variant * pv)
{
	SG_wc_status_flags * pStatusFlags = (SG_wc_status_flags *)pVoidData;
	SG_vhash * pvhItem;
	SG_vhash * pvhStatus;
	SG_int64 i64;

	SG_UNUSED( pva );
	SG_UNUSED( ndx );

	// f = array[k].status.flags;

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhStatus)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus, "flags", &i64)  );

	*pStatusFlags |= ((SG_wc_status_flags)i64);

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Compute the summary (union) of all the statusFlags over all
 * of the items in the canonical status.
 *
 * This is a cheap way of asking multiple "any lost?", "any found?",
 * ... questions (when you have more than one such question to ask).
 *
 */
void SG_wc__status__summary(SG_context * pCtx,
							const SG_varray * pvaStatus,
							SG_wc_status_flags * pStatusFlagsSummary)
{
	SG_NULLARGCHECK_RETURN( pvaStatus );
	SG_NULLARGCHECK_RETURN( pStatusFlagsSummary );

	*pStatusFlagsSummary = SG_WC_STATUS_FLAGS__ZERO;

	SG_ERR_CHECK_RETURN(  SG_varray__foreach(pCtx, pvaStatus, _summary_cb,
											 pStatusFlagsSummary)  );
}

