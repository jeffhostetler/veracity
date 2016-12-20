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

void sg_wc_tx__merge__add_to_kill_list(SG_context * pCtx,
									   SG_mrg * pMrg,
									   SG_uint64 uiAliasGid)
{
	if (!pMrg->pVecRevertAllKillList)
		SG_ERR_CHECK(  SG_VECTOR_I64__ALLOC(pCtx, &pMrg->pVecRevertAllKillList, 10)  );

	SG_ERR_CHECK(  SG_vector_i64__append(pCtx, pMrg->pVecRevertAllKillList, (SG_int64)uiAliasGid, NULL)  );

fail:
	return;
}

/**
 * During the revert-all setup, we add certain deleted items to the
 * kill-list (so that we'll delete the tbl_pc row for them) effectively
 * marking them clean -- and we don't insert them into the pMrgCSet
 * directly -- we let merge discover they are missing and decide what
 * to do.
 *
 * But if the merge-engine discovers the item and has different plans
 * for it, we cancel the predicted kill for it.
 *
 */
void sg_wc_tx__merge__remove_from_kill_list(SG_context * pCtx,
											SG_mrg * pMrg,
											const SG_uint64 uiAliasGid)
{
	SG_uint32 k, count;

	if (!pMrg->pVecRevertAllKillList)
		return;

	SG_ERR_CHECK(  SG_vector_i64__length(pCtx, pMrg->pVecRevertAllKillList, &count)  );
	for (k=0; k<count; k++)
	{
		SG_uint64 uiAliasGid_k;

		SG_ERR_CHECK(  SG_vector_i64__get(pCtx, pMrg->pVecRevertAllKillList, k, (SG_int64 *)&uiAliasGid_k)  );
		if (uiAliasGid_k == uiAliasGid)
		{
#if TRACE_WC_MERGE
			SG_int_to_string_buffer buf;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "Cancel kill-list for [0x%s]\n",
									   SG_uint64_to_sz__hex(uiAliasGid_k, buf))  );
#endif
			SG_ERR_CHECK(  SG_vector_i64__set(pCtx, pMrg->pVecRevertAllKillList, k, SG_WC_DB__ALIAS_GID__UNDEFINED)  );
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * If we are in a REVERT-ALL and it found some ADDED-SPECIAL + REMOVED
 * items, we need to delete the corresponding row in the pc_L0 table
 * so that they won't appear after the REVERT in subsequent STATUS
 * commands.
 *
 */
void sg_wc_tx__merge__queue_plan__kill_list(SG_context * pCtx,
											SG_mrg * pMrg)
{
	SG_uint32 k, count;

	if (!pMrg->pVecRevertAllKillList)
		return;

	SG_ERR_CHECK(  SG_vector_i64__length(pCtx, pMrg->pVecRevertAllKillList, &count)  );
	for (k=0; k<count; k++)
	{
		SG_int64 uiAliasGid;

		SG_ERR_CHECK(  SG_vector_i64__get(pCtx, pMrg->pVecRevertAllKillList, k, (SG_int64 *)&uiAliasGid)  );
		if (uiAliasGid != SG_WC_DB__ALIAS_GID__UNDEFINED)
			SG_ERR_CHECK(  sg_wc_tx__queue__kill_pc_row(pCtx, pMrg->pWcTx, uiAliasGid)  );
	}

fail:
	return;
}
