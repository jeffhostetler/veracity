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
 * @file sg_wc_tx__rp__overwrite_file_from_repo.c
 *
 * @details Layer-6 rp api to queue an overwrite of a file.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_tx__rp__overwrite_file_from_repo__lvi(SG_context * pCtx,
												 SG_wc_tx * pWcTx,
												 sg_wc_liveview_item * pLVI_Src,
												 const char * pszHidBlob,
												 SG_int64 attrbits,
												 SG_bool bBackupFile,
												 SG_wc_status_flags xu_mask)
{
	SG_string * pStringRepoPath_Src_Computed = NULL;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI_Src );

	// level-7 (sg_wc_tx__overwrite_file_from_repo() took a pszInput which can
	// be arbitrary user input (and especially a gid-domain repo-path).
	// It converted it into a pLVI and verified that it refers to a
	// controlled item.  We take that and compute an official current/live
	// repo-path and use that to queue/journal.

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI_Src,
															  &pStringRepoPath_Src_Computed)  );
	SG_ERR_CHECK(  sg_wc_tx__queue__overwrite_file_from_repo(pCtx,
															 pWcTx,
															 pStringRepoPath_Src_Computed,
															 pLVI_Src,
															 pszHidBlob,
															 attrbits,
															 bBackupFile,
															 xu_mask)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Src_Computed);
}
