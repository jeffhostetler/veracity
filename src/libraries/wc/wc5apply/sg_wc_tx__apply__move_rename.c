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
 * @file sg_wc_tx__apply__move_rename.c
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

void sg_wc_tx__apply__move_rename(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_vhash * pvh)
{
	SG_bool bAfterTheFact;
	SG_bool bUseIntermediate;
	SG_bool bSrcIsSparse;
	const char * pszRepoPath_Src;
	const char * pszRepoPath_Dest;
	SG_pathname * pPath_Src = NULL;
	SG_pathname * pPath_Dest = NULL;
	SG_pathname * pPath_Temp = NULL;

	SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh, "after_the_fact", &bAfterTheFact)  );
	SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh, "use_intermediate", &bUseIntermediate)  );
	SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh, "src_sparse", &bSrcIsSparse)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "src", &pszRepoPath_Src)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "dest", &pszRepoPath_Dest)  );

#if TRACE_WC_TX_APPLY
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__move_rename: [after-the-fact %d][use-intermediate %d][src-sparse %d] '%s' --> '%s'\n"),
							   bAfterTheFact, bUseIntermediate, bSrcIsSparse,
							   pszRepoPath_Src,
							   pszRepoPath_Dest)  );
#endif

	if (bAfterTheFact || bSrcIsSparse)		// user already did the move/rename.
		return;

	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath_Src,  &pPath_Src )  );
	SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath_Dest, &pPath_Dest)  );

	if (bUseIntermediate)
	{
		// need to use a temp file because of transient
		// collisions ('vv rename foo FOO' on Windows).
		SG_ERR_CHECK(  sg_wc_db__path__get_unique_temp_path(pCtx, pWcTx->pDb, &pPath_Temp)  );
		SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath_Src,  pPath_Temp)  );
		SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath_Temp, pPath_Dest)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath_Src, pPath_Dest)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_Src);
	SG_PATHNAME_NULLFREE(pCtx, pPath_Dest);
	SG_PATHNAME_NULLFREE(pCtx, pPath_Temp);
}
