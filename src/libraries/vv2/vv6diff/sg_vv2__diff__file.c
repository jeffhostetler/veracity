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

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * A DELETED file is present in the left cset and not in the right cset.
 * Let the difftool have a generated TEMPFILE for the historical left side
 * and a NULL for the right side file.
 *
 * We expect that the difftool will simply print the entire baseline file
 * with '<' or '-' in front of each line.
 *
 */
static void _diff__file__deleted(SG_context * pCtx,
								 sg_vv6diff__diff_to_stream_data * pData,
								 const SG_vhash * pvhItem,
								 SG_wc_status_flags statusFlags,
								 SG_string * pStringHeader)
{
	SG_string * pStringLabel_left  = NULL;
	SG_string * pStringLabel_right = NULL;
	SG_pathname * pPathAbsolute_left = NULL;
	SG_pathname * pPathAbsolute_right = NULL;
	SG_vhash * pvhSubsection_left = NULL;	// we do not own this
	const char * pszGid;
	const char * pszHid_left;
	const char * pszRepoPath_left;
	const char * pszName_left;
	char * pszToolNameUsed = NULL;
	SG_int32 iResultCode = 0;

	SG_UNUSED( statusFlags );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, pData->pszSubsectionLeft, &pvhSubsection_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "hid", &pszHid_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "path", &pszRepoPath_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "name", &pszName_left)  );
	// left label is "<left-repo-path> <hid>" (we have an HID and we don't have a datestamp)
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, pszRepoPath_left, pszHid_left, NULL, &pStringLabel_left)  );

	// right label is "(not present)"
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, "(not present)", NULL, NULL, &pStringLabel_right)  );

	// fetch the left version of the file into a temp file.
	SG_ERR_CHECK(  sg_vv2__diff__export_to_temp_file(pCtx, pData, pData->pszSubsectionLeft,
													 pszGid, pszHid_left, pszName_left,
													 &pPathAbsolute_left)  );

	// create empty temp file for right side (using the entryname from the left).
	SG_ERR_CHECK(  sg_vv2__diff__create_temp_file(pCtx, pData, pData->pszSubsectionRight,
												  pszGid, pszName_left,
												  &pPathAbsolute_right)  );

	//////////////////////////////////////////////////////////////////

	// Since the (console) difftool API writes to stdout, we need to write the
	// header before we invoke the difftool.

	SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );
	SG_ERR_CHECK(  SG_difftool__run(pCtx, pszRepoPath_left, pData->pRepo,
									pData->pszDiffToolContext, pData->pszTool,
									pPathAbsolute_left, pStringLabel_left,
									pPathAbsolute_right, pStringLabel_right,
									SG_FALSE,
									&iResultCode,
									((pData->pvhResultCodes) ? &pszToolNameUsed : NULL))  );
	if (pData->pvhResultCodes)
	{
		SG_vhash * pvhDetails;	// we do not own this.
		// Note: I'm using __updatenew__ rather than __addnew__ to guard
		// Note: against a duplicate row in the given pvaStatus.  It's not
		// Note: worth blowing up if they say 'vv diff -i foo.c foo.c' and
		// Note: someone forgot to do a unique pass.
		SG_ERR_CHECK(  SG_vhash__updatenew__vhash(pCtx, pData->pvhResultCodes, pszGid, &pvhDetails)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "tool", pszToolNameUsed)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhDetails, "result", (SG_int64)iResultCode)  );
		// The repo-path is for convenience only; we don't use
		// the actual path since it is a temp file.
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "repopath", pszRepoPath_left)  );
	}

fail:
	if (pPathAbsolute_left)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathAbsolute_left)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_left);
	}
	if (pPathAbsolute_right)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathAbsolute_right)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_right);
	}
	SG_STRING_NULLFREE(pCtx, pStringLabel_left);
	SG_STRING_NULLFREE(pCtx, pStringLabel_right);
	SG_NULLFREE(pCtx, pszToolNameUsed);
}

/**
 * An ADDED file was not present in the left cset and by definition
 * is present in the right cset.  Let the difftool have NULL for the
 * file on the left side and a generated TEMPFILE for the historical
 * right side.
 *
 * We expect that the difftool will simply print the entire current
 * file with '>' or '+' in front of each line.
 *
 */ 
static void _diff__file__added(SG_context * pCtx,
							   sg_vv6diff__diff_to_stream_data * pData,
							   const SG_vhash * pvhItem,
							   SG_wc_status_flags statusFlags,
							   SG_string * pStringHeader)
{
	SG_string * pStringLabel_left  = NULL;
	SG_string * pStringLabel_right = NULL;
	SG_pathname * pPathAbsolute_left = NULL;
	SG_pathname * pPathAbsolute_right = NULL;
	SG_vhash * pvhSubsection_right = NULL;		// we do not own this
	const char * pszRepoPath_right;
	const char * pszHid_right;
	const char * pszGid;
	const char * pszName_right;
	char * pszToolNameUsed = NULL;
	SG_int32 iResultCode = 0;

	SG_UNUSED( statusFlags );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, pData->pszSubsectionRight, &pvhSubsection_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "hid", &pszHid_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "path", &pszRepoPath_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "name", &pszName_right)  );
	// left label is "(not present)"
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, "(not present)", NULL, NULL, &pStringLabel_left)  );

	// the right label is "<right_repo_path> <hid>"
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, pszRepoPath_right, pszHid_right, NULL, &pStringLabel_right)  );

	// create an empty temp file for the left side (using the entryname from the right).
	SG_ERR_CHECK(  sg_vv2__diff__create_temp_file(pCtx, pData, pData->pszSubsectionLeft,
												  pszGid, pszName_right,
												  &pPathAbsolute_left)  );

	// fetch the right version of the file into a temp file.
	SG_ERR_CHECK(  sg_vv2__diff__export_to_temp_file(pCtx, pData, pData->pszSubsectionRight,
													 pszGid, pszHid_right, pszName_right,
													 &pPathAbsolute_right)  );

	//////////////////////////////////////////////////////////////////

	// Since the (console) difftool API writes to stdout, we need to write the
	// header before we invoke the difftool.

	SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );
	SG_ERR_CHECK(  SG_difftool__run(pCtx, pszRepoPath_right, pData->pRepo,
									pData->pszDiffToolContext, pData->pszTool,
									pPathAbsolute_left, pStringLabel_left,
									pPathAbsolute_right, pStringLabel_right,
									SG_FALSE,
									&iResultCode,
									((pData->pvhResultCodes) ? &pszToolNameUsed : NULL))  );
	if (pData->pvhResultCodes)
	{
		SG_vhash * pvhDetails;	// we do not own this.

		SG_ERR_CHECK(  SG_vhash__updatenew__vhash(pCtx, pData->pvhResultCodes, pszGid, &pvhDetails)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "tool", pszToolNameUsed)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhDetails, "result", (SG_int64)iResultCode)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "repopath", pszRepoPath_right)  );
	}

fail:
	if (pPathAbsolute_left)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathAbsolute_left)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_left);
	}
	if (pPathAbsolute_right)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathAbsolute_right)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_right);
	}
	SG_STRING_NULLFREE(pCtx, pStringLabel_left);
	SG_STRING_NULLFREE(pCtx, pStringLabel_right);
	SG_NULLFREE(pCtx, pszToolNameUsed);
}

/**
 * The file is present in both csets.
 * Show normal diffs between these 2 versions.
 *
 */
static void _diff__file__modified(SG_context * pCtx,
								  sg_vv6diff__diff_to_stream_data * pData,
								  const SG_vhash * pvhItem,
								  SG_wc_status_flags statusFlags,
								  SG_string * pStringHeader)
{
	SG_string * pStringLabel_left  = NULL;
	SG_string * pStringLabel_right = NULL;
	SG_pathname * pPathAbsolute_left = NULL;
	SG_pathname * pPathAbsolute_right = NULL;
	SG_vhash * pvhSubsection_left = NULL;		// we do not own this
	SG_vhash * pvhSubsection_right = NULL;		// we do not own this
	const char * pszRepoPath_left;
	const char * pszRepoPath_right;
	const char * pszGid;
	const char * pszHid_left;
	const char * pszHid_right;
	const char * pszName_left;
	const char * pszName_right;
	char * pszToolNameUsed = NULL;
	SG_int32 iResultCode = 0;

	SG_UNUSED( statusFlags );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, pData->pszSubsectionLeft, &pvhSubsection_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "hid", &pszHid_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "path", &pszRepoPath_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "name", &pszName_left)  );
	// left label is "<left_repo-path> <hid>" (we have an HID and since no date makes sense)
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, pszRepoPath_left, pszHid_left, NULL, &pStringLabel_left)  );

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, pData->pszSubsectionRight, &pvhSubsection_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "hid", &pszHid_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "path", &pszRepoPath_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "name", &pszName_right)  );
	// the right label is "<right_repo_path> <hid>"
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, pszRepoPath_right, pszHid_right, NULL, &pStringLabel_right)  );

	// fetch the left version of the file into a temp file.
	SG_ERR_CHECK(  sg_vv2__diff__export_to_temp_file(pCtx, pData, pData->pszSubsectionLeft,
													 pszGid, pszHid_left, pszName_left,
													 &pPathAbsolute_left)  );

	// fetch the right version of the file into a temp file.
	SG_ERR_CHECK(  sg_vv2__diff__export_to_temp_file(pCtx, pData, pData->pszSubsectionRight,
													 pszGid, pszHid_right, pszName_right,
													 &pPathAbsolute_right)  );

	//////////////////////////////////////////////////////////////////

	// Since the (console) difftool API writes to stdout, we need to write the
	// header before we invoke the difftool.

	SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );
	SG_ERR_CHECK(  SG_difftool__run(pCtx, pszRepoPath_right, pData->pRepo,
									pData->pszDiffToolContext, pData->pszTool,
									pPathAbsolute_left, pStringLabel_left,
									pPathAbsolute_right, pStringLabel_right,
									SG_FALSE,
									&iResultCode, &pszToolNameUsed)  );
	// If the caller wants the individual {tool-name, result-code} from each
	// invocation of a tool, we accumulate it in a vhash for them.  If not,
	// we throw if the result-code indicated a problem in the tool.
	if (pData->pvhResultCodes)
	{
		SG_vhash * pvhDetails;	// we do not own this.

		SG_ERR_CHECK(  SG_vhash__updatenew__vhash(pCtx, pData->pvhResultCodes, pszGid, &pvhDetails)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "tool", pszToolNameUsed)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhDetails, "result", (SG_int64)iResultCode)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDetails, "repopath", pszRepoPath_right)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_difftool__check_result_code__throw(pCtx, iResultCode, pszToolNameUsed)  );
	}

fail:
	if (pPathAbsolute_left)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathAbsolute_left)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_left);
	}
	if (pPathAbsolute_right)
	{
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathAbsolute_right)  );
		SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_right);
	}
	SG_STRING_NULLFREE(pCtx, pStringLabel_left);
	SG_STRING_NULLFREE(pCtx, pStringLabel_right);
	SG_NULLFREE(pCtx, pszToolNameUsed);
}

//////////////////////////////////////////////////////////////////

static void _diff__file__contents(SG_context * pCtx,
								  sg_vv6diff__diff_to_stream_data * pData,
								  const SG_vhash * pvhItem,
								  SG_wc_status_flags statusFlags,
								  SG_string * pStringHeader)
{
	// TODO 2012/05/07 Use repo-path to select the difftool directly
	// TODO            rather than using SG_difftool__run() which takes
	// TODO            care of everything.
	// 
	// TODO 2012/05/04 If the file is a binary file type, we could
	// TODO            just print "=== binary files are different"
	// TODO            and be done.  No need to export a 1Gb .iso
	// TODO            just to have :binary call :skip and do nothing.
	// TODO
	// TODO            We could even fake up some left/right header labels
	// TODO            using just the repo-path and the HIDs.
	// TODO
	// TODO            See W5937.

	if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
	{
		// right side does not exist anymore (as of this point in the TX),
		// so dump trivial '<' or '-' diff of entire left side.
		SG_ERR_CHECK(  _diff__file__deleted(pCtx, pData, pvhItem, statusFlags, pStringHeader)  );
		goto done;
	}
	
	// TODO 2012/05/08 In all cases, the both the left and right sides *MUST* be
	// TODO            considered as READ-ONLY (since this is a historical diff).
	// TODO            And we pass a read-only flag to the difftool, BUT THAT IS
	// TODO            ONLY A SUGGESTION -- if interactive or a tool was given,
	// TODO            it may or may not respect it.  After the tool finishes, we
	// TODO            are going to DELETE THE TEMPFILES whether the user edited
	// TODO            them or not.

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
	{
		SG_ERR_CHECK(  _diff__file__added(pCtx, pData, pvhItem, statusFlags, pStringHeader)  );
		goto done;
	}

	if (statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
	{
		SG_ERR_CHECK(  _diff__file__modified(pCtx, pData, pvhItem, statusFlags, pStringHeader)  );
		goto done;
	}

done:
	;
fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_vv2__diff__file(SG_context * pCtx,
						sg_vv6diff__diff_to_stream_data * pData,
						const SG_vhash * pvhItem,
						SG_wc_status_flags statusFlags)
{
	SG_string * pStringHeader = NULL;

	SG_ERR_CHECK(  sg_vv2__diff__header(pCtx, pvhItem, &pStringHeader)  );

	if (statusFlags & (SG_WC_STATUS_FLAGS__S__ADDED
					   |SG_WC_STATUS_FLAGS__S__MERGE_CREATED
					   |SG_WC_STATUS_FLAGS__S__DELETED
					   |SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED))
	{
		// content added/deleted/changed.
		SG_ERR_CHECK(  _diff__file__contents(pCtx, pData, pvhItem, statusFlags, pStringHeader)  );
	}
	else
	{
		// just print the header -- content did not change -- must be a simple structural change.
		SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
}
