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

void sg_wc_tx__rp__undo_delete__lvi_lvi_sz(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   sg_wc_liveview_item * pLVI_Src,
										   sg_wc_liveview_item * pLVI_DestDir,
										   const char * pszEntryname_Dest,
										   const SG_wc_undo_delete_args * pArgs,
										   SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src_Computed = NULL;
	SG_string * pStringRepoPath_DestDir_Computed = NULL;
	SG_string * pStringRepoPath_Dest_Computed = NULL;
	sg_wc_liveview_item * pLVI_Dest;		// we do not own this
	sg_wc_liveview_dir * pLVD_DestDir;		// we do not own this
	SG_bool bKnown_Dest;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI_Src );
	SG_NULLARGCHECK_RETURN( pLVI_DestDir );
	SG_NONEMPTYCHECK_RETURN( pszEntryname_Dest );
	// pArgs is optional

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Src,
															  &pStringRepoPath_Src_Computed)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_DestDir,
															  &pStringRepoPath_DestDir_Computed)  );

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI_Src->scan_flags_Live) == SG_FALSE)
		SG_ERR_THROW2(  SG_ERR_WC__ITEM_ALREADY_EXISTS,
						(pCtx, "Source '%s'",
						 SG_string__sz(pStringRepoPath_Src_Computed))  );

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI_DestDir->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "The destination directory '%s' has been REMOVED.",
						 SG_string__sz(pStringRepoPath_DestDir_Computed))  );
	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI_DestDir->scan_flags_Live))
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "The destination directory '%s' is currently LOST.",
						 SG_string__sz(pStringRepoPath_DestDir_Computed))  );
	
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
										  &pStringRepoPath_Dest_Computed,
										  pStringRepoPath_DestDir_Computed)  );
	SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx,
												 pStringRepoPath_Dest_Computed,
												 pszEntryname_Dest,
												 SG_FALSE)  );
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item(pCtx, pWcTx,
												  pStringRepoPath_Dest_Computed,
												  &bKnown_Dest, &pLVI_Dest)  );
	if (bKnown_Dest)
		SG_ERR_THROW2(  SG_ERR_WC__ITEM_ALREADY_EXISTS,
						(pCtx, "The destination '%s' already exists.",
						 SG_string__sz(pStringRepoPath_Dest_Computed))  );

	// I'm not going to worry about path-cycle problems
	// because we already know that the source does not
	// exist and the destination directory does, so it
	// isn't possible to have a cycle.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI_DestDir, &pLVD_DestDir)  );
	SG_ERR_CHECK(  sg_wc_liveview_dir__can_add_new_entryname(pCtx, pWcTx->pDb,
															 pLVD_DestDir,
															 NULL,
															 NULL,
															 pszEntryname_Dest,
															 SG_TREENODEENTRY_TYPE__INVALID,
															 SG_FALSE)  );

	SG_ERR_CHECK(  sg_wc_tx__queue__undo_delete(pCtx, pWcTx,
												pStringRepoPath_Dest_Computed,
												pLVI_Src,
												pLVI_DestDir,
												pszEntryname_Dest,
												pArgs,
												xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src_Computed);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_DestDir_Computed);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Dest_Computed);
}


