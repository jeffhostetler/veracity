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

#if TRACE_WC_MERGE
void _sg_wc_tx__merge__trace_msg__dump_raw_stats(SG_context * pCtx,
												 SG_mrg * pMrg)
{
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MrgStats: [unch %d][ch %d][del %d][add %d+%d][mrg %d][rest %d][res %d][unres %d]\n",
							   pMrg->nrUnchanged,
							   pMrg->nrChanged,
							   pMrg->nrDeleted,
							   pMrg->nrAddedByOther, pMrg->nrOverrideDelete,
							   pMrg->nrAutoMerged,
							   pMrg->nrOverrideLost,
							   pMrg->nrResolved,
							   pMrg->nrUnresolved)  );
}
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__make_stats(SG_context * pCtx,
								 SG_mrg * pMrg,
								 SG_vhash ** ppvhStats)
{
	SG_vhash * pvh = NULL;
	SG_string * pStringSynopsys = NULL;

	SG_NULLARGCHECK_RETURN( ppvhStats );

	// Build a synopsys/summary message suitable for
	// printing on the console after the merge.
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringSynopsys)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringSynopsys,
									  "%d updated, %d deleted, %d added, %d merged, %d restored, %d resolved, %d unresolved",
									  pMrg->nrChanged,
									  pMrg->nrDeleted,
									  pMrg->nrAddedByOther+pMrg->nrOverrideDelete,
									  pMrg->nrAutoMerged,
									  pMrg->nrOverrideLost,
									  pMrg->nrResolved,
									  pMrg->nrUnresolved)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "synopsys", SG_string__sz(pStringSynopsys))  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "unchanged",       pMrg->nrUnchanged)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "changed",         pMrg->nrChanged)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "deleted",         pMrg->nrDeleted)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "added_by_other",  pMrg->nrAddedByOther)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "override_delete", pMrg->nrOverrideDelete)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "merged",          pMrg->nrAutoMerged)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "restored_lost",   pMrg->nrOverrideLost)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "resolved",        pMrg->nrResolved)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "unresolved",      pMrg->nrUnresolved)  );

	*ppvhStats = pvh;
	pvh = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pStringSynopsys);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

