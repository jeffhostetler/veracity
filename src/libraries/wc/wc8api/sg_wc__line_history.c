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
#include "sg_vv2__public_prototypes.h"

//////////////////////////////////////////////////////////////////

void SG_wc__line_history(SG_context * pCtx,
				const SG_pathname* pPathWc,
				const char * pszInput,
				SG_int32 start_line,
				SG_int32 length,
				SG_varray ** ppvaResults)
{
	
	SG_wc_tx * pWcTx = NULL;

	SG_string * pStringRepoPath = NULL;
	char * pszGid = NULL;
	sg_wc_liveview_item * pLVI;		// we do not own this
	SG_wc_status_flags statusFlags;
	SG_bool bKnown;
	char cDomain;
	const char * psz_original_hid = NULL;
	SG_bool bAreaWasModifiedInWC = SG_FALSE;
	SG_pathname * pTempFile = NULL;
	SG_pathname * pWCPath = NULL;
	SG_int32 nLineNumInParent = 0;
	SG_bool bWasChanged = SG_FALSE;
	char * pszBaseline = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_repo * pRepo = NULL;

	char gidBuf[SG_GID_BUFFER_LENGTH + 1];
	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".
	
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_TRUE)  );
	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &cDomain)  );

	// find the GID/alias of the named item while taking
	// account whatever domain component.  (that is, they
	// could have said "@g12345...." or "@b/....." rather
	// than a live path.)
	//
	// Fetch the LVI for this item.  This may implicitly
	// cause a SCANDIR/READIR and/or sync up with the DB.
	// This is good because it also means we will get the
	// exact-match stuff on each component within the
	// pathname.

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_item__domain(pCtx, pWcTx, pStringRepoPath,
														  &bKnown, &pLVI)  );

	if (!bKnown)
	{
		// We only get this if the path is completely bogus and
		// took us off into the weeds (as opposed to reporting
		// something just not-controlled).
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown item '%s'.", SG_string__sz(pStringRepoPath))  );
	}

	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx, pLVI,
												SG_TRUE,	// --no-ignores (faster)
												SG_FALSE,	// trust TSC
												&statusFlags)  );
	if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND
					   |SG_WC_STATUS_FLAGS__U__IGNORED))
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "%s", pszInput)  );
	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
		SG_ERR_THROW2(  SG_ERR_VC_LINE_HISTORY_NOT_COMMITTED_YET,
						(pCtx, "%s", pszInput)  );

	if ((statusFlags & SG_WC_STATUS_FLAGS__T__FILE) == 0)
	{
		//They passed something that is not a file.  That's an error.
		SG_ERR_THROW2(  SG_ERR_NOTAFILE,
						(pCtx, "%s", pszInput)  );
	}

	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
	SG_ASSERT_RELEASE_FAIL2(  (pszGid[0] == 'g'),	// we get 't' domain IDs for uncontrolled items
							  (pCtx, "%s has temp id %s", pszInput, pszGid)  );

	nLineNumInParent = start_line;
	if (statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
	{
		const char * pszEntrynameOriginal = NULL;

		//If the file is modified, we need to check to see if the
		//file was modified versus the baseline. If it was, then 
		//we need to report that to the user.

		//Fetch the baseline to do the comparison
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx, SG_FALSE, &psz_original_hid )  );
		SG_ERR_CHECK(  sg_wc_db__path__repopath_to_absolute(pCtx, pWcTx->pDb, pStringRepoPath, &pWCPath)  );
		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszEntrynameOriginal)  );
		SG_ERR_CHECK(  sg_wc_diff_utils__export_to_temp_file(pCtx, pWcTx, "0", pszGid, psz_original_hid,
															 pszEntrynameOriginal,
															 &pTempFile)  );
		//Subtract one from the start line before sending it down.
		SG_ERR_CHECK(  SG_linediff(pCtx, pTempFile, pWCPath, start_line - 1, length, &bWasChanged, &nLineNumInParent, NULL)  );

		if (bWasChanged)
			SG_ERR_THROW2(  SG_ERR_VC_LINE_HISTORY_WORKING_COPY_MODIFIED,
						(pCtx, "%s", pszInput)  );
		//Add one back to the parent
		nLineNumInParent += 1;
	}
	
	if (bAreaWasModifiedInWC == SG_FALSE)
	{
		//Call into vv2's line_history
		SG_ERR_CHECK(  SG_wc_tx__get_wc_baseline(pCtx, pWcTx, &pszBaseline, NULL)  );
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszBaseline)  );
		SG_ERR_CHECK(  SG_sprintf(pCtx, gidBuf, SG_GID_BUFFER_LENGTH + 1, "@%s", pszGid)  );
		SG_ERR_CHECK(  SG_wc_tx__get_repo_and_wd_top(pCtx, pWcTx, &pRepo, NULL)  );
		SG_ERR_CHECK(  SG_vv2__line_history__repo(pCtx, pRepo, pRevSpec, gidBuf, nLineNumInParent, length, ppvaResults)   );
	}
fail:
	if (pTempFile)
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx,pTempFile)  );
	SG_NULLFREE(pCtx, pszGid);
	SG_NULLFREE(pCtx, pszBaseline);
	SG_PATHNAME_NULLFREE(pCtx, pTempFile);
	SG_PATHNAME_NULLFREE(pCtx, pWCPath);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
}
