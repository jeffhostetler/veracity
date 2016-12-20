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

static SG_varray_foreach_callback _del__cb;

static void _del__cb(SG_context * pCtx,
					 void * pVoid_Data,
					 const SG_varray * pva,
					 SG_uint32 ndx,
					 const SG_variant * pVariant)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoid_Data;
	SG_vhash * pvh;
	const char * pszOp;
	const char * pszIssue;
	SG_vhash * pvhIssue = NULL;
	const char * pszRepoPathTempDir = NULL;
	SG_pathname * pPath = NULL;

	SG_UNUSED( pva );
	SG_UNUSED( ndx );

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pVariant, &pvh)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "op", &pszOp)  );

	if (strcmp(pszOp, "insert_issue") != 0)
		return;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "issue", &pszIssue)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhIssue, pszIssue)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhIssue, "repopath_tempdir", &pszRepoPathTempDir)  );
	if (pszRepoPathTempDir)
	{
#if TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "sg_wc_tx__merge__delete_automerge_tempfiles_on_abort: tempdir '%s'\n",
								   pszRepoPathTempDir)  );
#endif

		SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx, pWcTx->pDb, pszRepoPathTempDir, &pPath)  );
		// actually deleting the tempdir can fail for any number of reasons,
		// but that should not cause our ABORT to ABORT, so we TRY/CATCH
		// here rather than _CHECK().
		// We force it because the files might be 0400 rather than 0600 and
		// that causes Windows to complain.
		SG_ERR_TRY(  SG_fsobj__rmdir_recursive__pathname2(pCtx, pPath, SG_TRUE)  );
		SG_ERR_CATCH_ANY
		{
#if TRACE_WC_MERGE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "rmdir_recursive failed on: %s\n", SG_pathname__sz(pPath))  );
#endif
		}
		SG_ERR_CATCH_END;
		
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_VHASH_NULLFREE(pCtx, pvhIssue);
}

//////////////////////////////////////////////////////////////////

/**
 * When MERGE or UPDATE are given "--test" we ABORT/CANCEL the TX
 * after computing what needs to be done and before actually making
 * any changes in the WD.
 *
 * HOWEVER, if MERGE or UPDATE needed to do an automerge on any files,
 * we created a set of temp files for each of them.  These need to be
 * deleted.
 *
 * Note we may also get called when crawling out of a fatal error.
 *
 */
void sg_wc_tx__merge__delete_automerge_tempfiles_on_abort(SG_context * pCtx,
														  SG_wc_tx * pWcTx)
{
#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__merge__delete_automerge_tempfiles_on_abort: start....\n")  );
#endif

	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pWcTx->pvaJournal, _del__cb, (void *)pWcTx)  );

#if TRACE_WC_MERGE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_tx__merge__delete_automerge_tempfiles_on_abort: end....\n")  );
#endif

fail:
	return;
}

