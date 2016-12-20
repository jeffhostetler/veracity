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
 * @file sg_wc_tx__apply__remove_file.c
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

void sg_wc_tx__apply__remove_file(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const SG_vhash * pvh)
{
	const char * pszDisposition = NULL;
	const char * pszRepoPath = NULL;
	const char * pszBackupPath = NULL;
	SG_pathname * pPath = NULL;
	SG_pathname * pPathBackup = NULL;

	SG_ERR_CHECK(  SG_vhash__get__sz(  pCtx, pvh, "disposition", &pszDisposition)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(  pCtx, pvh, "src",         &pszRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh, "backup_path", &pszBackupPath)  );

#if TRACE_WC_TX_REMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__remove_file: [disp %s] '%s'\n"),
							   pszDisposition, pszRepoPath)  );
#endif

	if (strcmp(pszDisposition,"delete") == 0)
	{
		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );
		SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
	}
	else if (strcmp(pszDisposition,"backup") == 0)
	{
		SG_ASSERT(  (pszBackupPath)  );
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathBackup, pszBackupPath)  );

		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );

		SG_ERR_CHECK(  SG_fsobj__move__pathname_pathname(pCtx, pPath,  pPathBackup)  );
#if TRACE_WC_TX_REMOVE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("sg_wc_tx__apply__remove_file: backing-up '%s' as '%s'.\n"),
								   SG_pathname__sz(pPath),
								   SG_pathname__sz(pPathBackup))  );
#endif
	}
	else if (strcmp(pszDisposition,"keep") == 0)
	{
		// leave the item as it is on disk.
	}
	else if (strcmp(pszDisposition,"noop") == 0)
	{
		// the item does not exist on the disk.  so nothing to do.
	}
	else
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "sg_wc_tx__apply__remove_file: [disposition %s]", pszDisposition)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPathBackup);
}
