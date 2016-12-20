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
 * @file sg_wc_tx__apply__remove_directory.c
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

void sg_wc_tx__apply__remove_directory(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   const SG_vhash * pvh)
{
	const char * pszDisposition;
	const char * pszRepoPath;
//	const char * pszEntryname;
	SG_pathname * pPath = NULL;
	SG_pathname * pPath_Temp = NULL;

	SG_ERR_CHECK(  SG_vhash__get__sz(  pCtx, pvh, "disposition", &pszDisposition)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(  pCtx, pvh, "src",         &pszRepoPath)  );
//	SG_ERR_CHECK(  SG_vhash__get__sz(  pCtx, pvh, "entryname",   &pszEntryname)  );

#if TRACE_WC_TX_REMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("sg_wc_tx__apply__remove_directory: [disp %s] '%s'\n"),
							   pszDisposition, pszRepoPath)  );
#endif

	if (strcmp(pszDisposition,"delete") == 0)
	{
		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPath, &pPath)  );
		SG_ERR_CHECK(  SG_fsobj__rmdir__pathname(pCtx, pPath)  );
	}
	else if (strcmp(pszDisposition,"keep") == 0)
	{
	}
	else if (strcmp(pszDisposition,"noop") == 0)
	{
	}
	else
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "sg_wc_tx__apply__remove_directory: [disposition %s]", pszDisposition)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath_Temp);
}
