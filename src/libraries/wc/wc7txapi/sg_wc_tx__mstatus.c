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
 * Compute an MSTATUS.  Throw if not currently in a merge.
 * You own the returned pvaStatus.
 *
 */
void _sg_wc_tx__mstatus(SG_context * pCtx,
						SG_wc_tx * pWcTx,
						SG_bool bNoIgnores,
						SG_bool bNoSort,
						SG_varray ** ppvaStatus,
						SG_vhash ** ppvhLegend)
{
	SG_varray * pvaStatus = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppvaStatus );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaStatus)  );

	SG_ERR_CHECK(  sg_wc_tx__mstatus__main(pCtx, pWcTx, bNoIgnores, pvaStatus, ppvhLegend)  );
	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, pvaStatus)  );

	*ppvaStatus = pvaStatus;
	pvaStatus = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}


/**
 * Compute an MSTATUS and optionally fallback to a regular STATUS.
 * You own the returned pvaStatus.
 *
 */
void SG_wc_tx__mstatus(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   SG_bool bNoIgnores,
					   SG_bool bNoFallback,
					   SG_bool bNoSort,
					   SG_varray ** ppvaStatus,
					   SG_vhash ** ppvhLegend)
{
	_sg_wc_tx__mstatus(pCtx, pWcTx, bNoIgnores, bNoSort, ppvaStatus, ppvhLegend);
	if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
		return;
	
	if ((SG_context__err_equals(pCtx, SG_ERR_NOT_IN_A_MERGE) == SG_FALSE)
		|| bNoFallback)
		SG_ERR_RETHROW;

	SG_context__err_reset(pCtx);
	SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pWcTx,
									NULL, SG_INT32_MAX,	// no filtering
									SG_FALSE,			// bListUnchanged
									bNoIgnores,
									SG_FALSE,			// bNoTSC
									SG_FALSE,			// bListSparse
									SG_FALSE,			// bListReserved
									bNoSort,
									ppvaStatus,
									ppvhLegend)  );
fail:
	return;
}
	
