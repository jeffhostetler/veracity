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
 *
 */
void sg_wc_tx__rp__add_special__lvi_sz(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   sg_wc_liveview_item * pLVI_Parent,
									   const char * pszEntryname,
									   SG_uint64 uiAliasGid,
									   SG_treenode_entry_type tneType,
									   const char * pszHidBlob,
									   SG_int64 attrbits,
									   SG_wc_status_flags statusFlagsAddSpecialReason)
{
	SG_string * pStringRepoPath_Parent_Computed = NULL;
	sg_wc_liveview_dir * pLVD_Parent;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI_Parent );
	SG_NONEMPTYCHECK_RETURN( pszEntryname );
	if (tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		SG_NONEMPTYCHECK_RETURN( pszHidBlob );

	// we now allow merge- or update-create with or without sparse bit.
	SG_ARGCHECK_RETURN( (statusFlagsAddSpecialReason & (SG_WC_STATUS_FLAGS__S__MERGE_CREATED | SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)),
						statusFlagsAddSpecialReason);

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Parent,
															  &pStringRepoPath_Parent_Computed)  );

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI_Parent->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_WC_RESERVED_ENTRYNAME,
						(pCtx, "Cannot special-add '%s' to reserved parent directory '%s'.",
						 pszEntryname,
						 SG_string__sz(pStringRepoPath_Parent_Computed))  );
	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI_Parent->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Cannot special-add '%s' to uncontrolled parent directory '%s'.",
						 pszEntryname,
						 SG_string__sz(pStringRepoPath_Parent_Computed))  );
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI_Parent->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Cannot special-add '%s' to deleted parent directory '%s'.",
						 pszEntryname,
						 SG_string__sz(pStringRepoPath_Parent_Computed))  );
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI_Parent->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Cannot special-add '%s' to lost parent directory '%s'.",
						 pszEntryname,
						 SG_string__sz(pStringRepoPath_Parent_Computed))  );

	// TODO 2012/01/31 We should be able to allow "add-special"
	// TODO            when the parent directory is sparse (in
	// TODO            a non-populated portion of the tree), since
	// TODO            we are trying to add an item as it was in
	// TODO            the other branch.
	// TODO
	// TODO            Confirm this.

	// TODO 2013/01/07 Do we need to call sg_wc_liveview_dir__can_add_new_entryname()
	// TODO            and check for potential collisions with FOUND/IGNORED items?

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_Parent, &pLVD_Parent)  );
	SG_ERR_CHECK(  sg_wc_tx__queue__add_special(pCtx, pWcTx,
												pStringRepoPath_Parent_Computed,
												pLVD_Parent, pszEntryname,
												uiAliasGid, tneType,
												pszHidBlob, attrbits,
												statusFlagsAddSpecialReason)  );
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Parent_Computed);
}

