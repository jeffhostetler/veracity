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
 * @file sg_wc_tx__queue__undo_add.c
 *
 * @details Manipulate the LVI/LVD caches to effect an UNDO_ADD.
 * This is part of the QUEUE phase of the TX where we accumulate
 * the changes and look for problems.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * QUEUE an UNDO_ADD operation.
 *
 * I decided to let this __queue__ api NOT take
 * a repo-path as an argument and let it compute
 * it from the cache.  This saves the caller from
 * having to do pathname juggling in the middle of
 * the recursive REMOVE code.
 * 
 */
void sg_wc_tx__queue__undo_add(SG_context * pCtx,
							 SG_wc_tx * pWcTx,
							 sg_wc_liveview_item * pLVI,
							 const char * pszDisposition,
							 SG_bool * pbKept)
{
	SG_string * pStringRepoPath = NULL;

	// update pLVI to have deleted status
	SG_ERR_CHECK(  sg_wc_liveview_item__alter_structure__undo_add(pCtx, pLVI)  );

	// add stuff to journal to do:
	// [1] update the database.
	// [2] if item is present, deal with it.
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pStringRepoPath)  );
	SG_ERR_CHECK(  sg_wc_tx__journal__undo_add(pCtx,
											   pWcTx,
											   pLVI->pPcRow_PC,
											   pLVI->tneType,
											   pStringRepoPath,
											   pszDisposition)  );

	*pbKept = (strcmp(pszDisposition,"keep") == 0);

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}
