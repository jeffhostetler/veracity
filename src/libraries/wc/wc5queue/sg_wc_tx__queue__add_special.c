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
 * QUEUE a "SPECIAL-ADD".  This is used by MERGE, for example,
 * to add things that were added in the other branch to the
 * final result.
 *
 * TODO 2011/01/31 This routine really only needs the LVD of the
 * TODO            parent directory and can dynamically compute
 * TODO            the repo-path from it.  I'm just passing it
 * TODO            in here because the first caller just happened
 * TODO            to have it on hand.
 *
 */
void sg_wc_tx__queue__add_special(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_string * pStringLiveRepoPath_Parent,
								  sg_wc_liveview_dir * pLVD_Parent,
								  const char * pszEntryname,
								  SG_uint64 uiAliasGid,
								  SG_treenode_entry_type tneType,
								  const char * pszHidBlob,
								  SG_int64 attrbits,
								  SG_wc_status_flags statusFlagsAddSpecialReason)
{
	sg_wc_liveview_item * pLVI_Entry;				// we do not own this

	SG_UNUSED( pStringLiveRepoPath_Parent );

	SG_ERR_CHECK(  sg_wc_liveview_dir__add_special(pCtx, pWcTx,
												   pLVD_Parent, pszEntryname,
												   uiAliasGid, tneType,
												   pszHidBlob, attrbits,
												   statusFlagsAddSpecialReason,
												   &pLVI_Entry)  );
	SG_ERR_CHECK(  sg_wc_tx__journal__add_special(pCtx, pWcTx, pLVI_Entry,
												  pszHidBlob, attrbits)  );

fail:
	return;
}
