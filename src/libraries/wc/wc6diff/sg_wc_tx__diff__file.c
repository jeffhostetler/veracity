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
//
// Footnote 1.
// ===========
//
// See also W4364.
// 
// When the right side exists we may get a handle to a temp file
// if the user has (in the current TX) has pended changes to
// the file content (such as an already queued overwrite-file-from-file
// during an update/merge).  See the __get_proxy_file_path() code.
//
// If the TX is sql-committed then this pended/temp version will become
// the official version in the WD.  However, if the TX is sql-rolled-back,
// it won't.
//
// If the difftool chosen for this file is read-only (such as splat to the
// console like ":diff" or "/usr/bin/diff") all is fine.  However, if the
// user chooses an interactive tool and starts editing the right side,
// what should happen to their edits if the TX is rolled-back.
//
// We could *try* and pass a read-only flag to the tool, but that is
// about all we can do.  Not all tools have them, the user can configure
// the tool using 'vv config' and may not set it, and even if set the
// tool might respect it (or allow the user to override it).
//
// So I'm going to set a "transient" bit in the result and let the
// caller deal with it and/or prompt/warn the user when interactive.
//
// This is probably only a problem if you have a JS script that is
// doing a big TX and then calls diff in the middle.

//////////////////////////////////////////////////////////////////

/**
 * Select an appropriate difftool to use on this file.
 * We require that the common and right-side fields of
 * pvhDiffItem be filled in first.  (The left-side fields
 * can be after this is called.)
 *
 */
static void _pick_tool(SG_context * pCtx,
					   sg_wc6diff__setup_data * pData,
					   const SG_vhash * pvhItem,
					   SG_vhash * pvhDiffItem,
					   SG_string * pStringHeader)
{
	SG_filetool * pTool = NULL;
	const char * pszRepoPath_live = NULL;
	const char * pszAbsPath = NULL;
	SG_pathname * pPathname = NULL;
	const char * pszToolNameChosen = NULL;
	SG_bool bIsTmp_right = SG_FALSE;

	// use the suffix on the (always present) live-repo-path (rather than
	// the left- or right- version) to pick a tool.
	// 
	// if that fails, use the suffix of current file in the WD
	// (provided it isn't a temp file).  the left side is always
	// a temp file, so don't bother with it.

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhItem, "path", &pszRepoPath_live)  );
	SG_ERR_CHECK(  SG_vhash__check__bool(pCtx, pvhDiffItem, "right_is_temp", &bIsTmp_right)  );
	if (!bIsTmp_right)
	{
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhDiffItem, "right_abs_path", &pszAbsPath)  );
		if (pszAbsPath)
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathname, pszAbsPath)  );
	}

	SG_ERR_CHECK(  SG_difftool__select(pCtx,
									   pData->pszDiffToolContext, // gui or console
									   pszRepoPath_live,          // current/live repo-path
									   pPathname,                 // live or temp absolute pathname
									   pData->pszTool,            // suggested/requested tool
									   pData->pWcTx->pDb->pRepo,
									   &pTool)  );
	if (pTool)
	{
		SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &pszToolNameChosen)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "tool", pszToolNameChosen)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "context", pData->pszDiffToolContext)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader,
												 "=== No difftool configured for ('%s', '%s') in context '%s'.\n",
												 pszRepoPath_live, pszAbsPath, pData->pszDiffToolContext)  );
	}

fail:
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
}
					   
//////////////////////////////////////////////////////////////////

/**
 * For MODIFIED or DELETED items we need to populate the left side
 * of the diff.  This should be called after _pick_tool().
 * 
 * put the various fields that we need to use in the call to SG_difftool__run()
 * into the pvhDiffItem for later.
 *
 */
static void _get_left_side_details(SG_context * pCtx,
								   sg_wc6diff__setup_data * pData,
								   const SG_vhash * pvhItem,
								   SG_vhash * pvhDiffItem)
{
	SG_string * pStringLabel_left  = NULL;
	SG_pathname * pPathAbsolute_left = NULL;
	SG_vhash * pvhSubsection_left = NULL;		// we do not own this
	const char * pszRepoPath_left;
	const char * pszGid;
	const char * pszHid_left;
	const char * pszToolName = NULL;
	const char * pszName_left;

	// get the repo-path for the left side *AS IT EXISTED IN THE LEFT CSET*.
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, pData->pszSubsectionLeft, &pvhSubsection_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "hid", &pszHid_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "path", &pszRepoPath_left)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_left, "name", &pszName_left)  );

	// left label is "<left_repo-path> <hid>" (we have an HID and since no date makes sense)
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, pszRepoPath_left, pszHid_left, NULL, &pStringLabel_left)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "left_label",    SG_string__sz(pStringLabel_left))  );

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhDiffItem, "tool", &pszToolName)  );
	if (strcmp(pszToolName, SG_DIFFTOOL__INTERNAL__SKIP) == 0)
	{
		// There's no point in exporting a binary file into TEMP so
		// that we can invoke a no-op difftool.
		// See W5937.
	}
	else
	{
		// fetch the baseline version of the file into a temp file.
		// the left side should be read-only because it refers to a
		// historical version (regardless of whether or not we are
		// interactive).

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
		SG_ERR_CHECK(  sg_wc_diff_utils__export_to_temp_file(pCtx, pData->pWcTx, pData->pszSubsectionLeft,
															 pszGid, pszHid_left,
															 pszName_left,
															 &pPathAbsolute_left)  );
		SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPathAbsolute_left, 0400)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "left_abs_path", SG_pathname__sz(pPathAbsolute_left))  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_left);
	SG_STRING_NULLFREE(pCtx, pStringLabel_left);
}

/**
 * For MODIFIED or ADDED items we need to populate the right side
 * of the diff.
 *
 * See footnote 1 above for bIsTmp_right.
 * 
 */
static void _get_right_side_details(SG_context * pCtx,
									sg_wc6diff__setup_data * pData,
									const SG_vhash * pvhItem,
									sg_wc_liveview_item * pLVI,
									SG_vhash * pvhDiffItem)
{
	SG_string * pStringLabel_right  = NULL;
	SG_pathname * pPathAbsolute_right = NULL;
	SG_vhash * pvhSubsection_right = NULL;		// we do not own this
	const char * pszRepoPath_right;
	char bufDate[SG_TIME_FORMAT_LENGTH+1];
	SG_bool bIsTmp_right;
	SG_fsobj_stat st_right;

	// get the repo-path for the right side *AS IT EXISTED IN THE RIGHT CSET*.
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, pData->pszSubsectionRight, &pvhSubsection_right)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection_right, "path", &pszRepoPath_right)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_proxy_file_path(pCtx, pLVI, pData->pWcTx, &pPathAbsolute_right, &bIsTmp_right)  );
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPathAbsolute_right, &st_right)  );
	SG_ERR_CHECK(  SG_time__format_utc__i64(pCtx, st_right.mtime_ms, bufDate, SG_NrElements(bufDate))  );

	// the right label is "<right_repo_path> <datestamp>"
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPathAbsolute_right, &st_right)  );
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, pszRepoPath_right, NULL, bufDate, &pStringLabel_right)  );

	// put the various fields that we need to use in the call to SG_difftool__run()
	// into the pvhDiffItem for later.

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "right_label",    SG_string__sz(pStringLabel_right))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "right_abs_path", SG_pathname__sz(pPathAbsolute_right))  );
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhDiffItem, "right_is_tmp",   bIsTmp_right)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute_right);
	SG_STRING_NULLFREE(pCtx, pStringLabel_right);
}

//////////////////////////////////////////////////////////////////

/**
 * A DELETED file is present in the baseline and is not present in the WD.
 * Let the difftool have a generated TEMPFILE for the historical left side
 * and a NULL for the right side file.
 *
 * We expect that the difftool will simply print the entire baseline file
 * with '<' or '-' in front of each line.
 *
 */
static void _diff__file__deleted(SG_context * pCtx,
								 sg_wc6diff__setup_data * pData,
								 const SG_vhash * pvhItem,
								 SG_vhash * pvhDiffItem,
								 SG_string * pStringHeader)
{
	SG_string * pStringLabel_right = NULL;

	// the file does not exist in the right cset, so any repo-path label
	// we make for the right side is a bit bogus.
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, "(not present)", NULL, NULL, &pStringLabel_right)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "right_label", SG_string__sz(pStringLabel_right))  );

	SG_ERR_CHECK(  _pick_tool(pCtx, pData, pvhItem, pvhDiffItem, pStringHeader)  );
	SG_ERR_CHECK(  _get_left_side_details(pCtx, pData, pvhItem, pvhDiffItem)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringLabel_right);
}

/**
 * An ADDED file was not present in the baseline, and by definition
 * is present in the WD.  Let the difftool have NULL for the file on
 * the left side and the current WD file on the right side.
 *
 * We expect that the difftool will simply print the entire current
 * file with '>' or '+' in front of each line.
 *
 */ 
static void _diff__file__added(SG_context * pCtx,
							   sg_wc6diff__setup_data * pData,
							   const SG_vhash * pvhItem,
							   sg_wc_liveview_item * pLVI,
							   SG_vhash * pvhDiffItem,
							   SG_string * pStringHeader)
{
	SG_string * pStringLabel_left = NULL;

	SG_ERR_CHECK(  _get_right_side_details(pCtx, pData, pvhItem, pLVI, pvhDiffItem)  );
	SG_ERR_CHECK(  _pick_tool(pCtx, pData, pvhItem, pvhDiffItem, pStringHeader)  );

	// the file does not exist in the left cset, so any repo-path label
	// we make for the left side is a bit bogus.
	SG_ERR_CHECK(  sg_wc_diff_utils__make_label(pCtx, "(not present)", NULL, NULL, &pStringLabel_left)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "left_label", SG_string__sz(pStringLabel_left))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringLabel_left);
}

/**
 * A MERGE-CREATED file was not present in the baseline, but
 * is present in the other branch and in the WD.
 *
 * Historically, DIFF shows these files just like ADDED files
 * so that a 'patch' command would have all of the data.  It
 * would be nice to just show the differences between the
 * other branch and the WD version, but that might cause
 * confusion.
 *
 */
static void _diff__file__merge_created(SG_context * pCtx,
									   sg_wc6diff__setup_data * pData,
									   const SG_vhash * pvhItem,
									   sg_wc_liveview_item * pLVI,
									   SG_vhash * pvhDiffItem,
									   SG_string * pStringHeader)
{
	SG_ERR_CHECK_RETURN(  _diff__file__added(pCtx, pData, pvhItem, pLVI, pvhDiffItem, pStringHeader)  );
}

/**
 * An UPDATE-CREATED file was not present in the update-target-changeset.
 * It was present in the pre-update-baseline (and deleted in the target
 * and UPDATE over-ruled the delete).  After the update finishes, we don't
 * have access to the pre-update-baseline, so we don't have a left-side
 * for the diff.
 *
 */
static void _diff__file__update_created(SG_context * pCtx,
										sg_wc6diff__setup_data * pData,
										const SG_vhash * pvhItem,
										sg_wc_liveview_item * pLVI,
										SG_vhash * pvhDiffItem,
										SG_string * pStringHeader)
{
	SG_ERR_CHECK_RETURN(  _diff__file__added(pCtx, pData, pvhItem, pLVI, pvhDiffItem, pStringHeader)  );
}

/**
 * The file is present in both the baseline and the WD.
 * Show normal diffs between these 2 versions.
 *
 */
static void _diff__file__modified(SG_context * pCtx,
								  sg_wc6diff__setup_data * pData,
								  const SG_vhash * pvhItem,
								  sg_wc_liveview_item * pLVI,
								  SG_vhash * pvhDiffItem,
								  SG_string * pStringHeader)
{
	SG_ERR_CHECK(  _get_right_side_details(pCtx, pData, pvhItem, pLVI, pvhDiffItem)  );
	SG_ERR_CHECK(  _pick_tool(pCtx, pData, pvhItem, pvhDiffItem, pStringHeader)  );
	SG_ERR_CHECK(  _get_left_side_details(pCtx, pData, pvhItem, pvhDiffItem)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _diff__file__contents(SG_context * pCtx,
								  sg_wc6diff__setup_data * pData,
								  const SG_vhash * pvhItem,
								  SG_wc_status_flags statusFlags,
								  SG_vhash * pvhDiffItem,
								  SG_string * pStringHeader)
{
	const char * pszGid;
	SG_uint64 uiAliasGid;
	sg_wc_liveview_item * pLVI = NULL;	// we do not own this
	SG_bool bKnown = SG_FALSE;

	if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
	{
		// right side does not exist anymore (as of this point in the TX),
		// so dump trivial '<' or '-' diff of entire left side.
		SG_ERR_CHECK(  _diff__file__deleted(pCtx, pData, pvhItem, pvhDiffItem, pStringHeader)  );
	}
	else
	{
		// the right side exists.  the left may or may not exist.

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pData->pWcTx->pDb, pszGid, &uiAliasGid)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pData->pWcTx, uiAliasGid, &bKnown, &pLVI)  );

		if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
		{
			SG_ERR_CHECK(  _diff__file__added(pCtx, pData, pvhItem, pLVI, pvhDiffItem, pStringHeader)  );
		}
		else if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
		{
			SG_ERR_CHECK(  _diff__file__merge_created(pCtx, pData, pvhItem, pLVI, pvhDiffItem, pStringHeader)  );
		}
		else if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
		{
			SG_ERR_CHECK(  _diff__file__update_created(pCtx, pData, pvhItem, pLVI, pvhDiffItem, pStringHeader)  );
		}
		else if (statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
		{
			SG_ERR_CHECK(  _diff__file__modified(pCtx, pData, pvhItem, pLVI, pvhDiffItem, pStringHeader)  );
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__diff__setup__file(SG_context * pCtx,
								 sg_wc6diff__setup_data * pData,
								 const SG_vhash * pvhItem,
								 SG_wc_status_flags statusFlags)
{
	SG_string * pStringHeader = NULL;
	SG_vhash * pvhDiffItem = NULL;		// we do not own this
	const char * pszGid;				// we do not own this
	const char * pszLiveRepoPath;		// we do not own this
	SG_vhash * pvhItemStatus;			// we do not own this

	if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)
	{
		// The code in wc4status won't create header info for ignored
		// items unless verbose is set and diff never sets it, so we
		// skip this item to avoid getting the divider row of equal signs.

		return;
	}

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pData->pvaDiffSteps, &pvhDiffItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "gid", pszGid)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszLiveRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "path", pszLiveRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );
	SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhDiffItem, "status", pvhItemStatus)  );

	// the following if-else-if-else... DOES NOT imply that these statuses
	// are mutually exclusive (for example, one could have ADDED+LOST), but
	// rather just the cases when we actually want to print the content diffs.

	if (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)
	{
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== %s\n", "Omitting details for sparse item.")  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== %s\n", "Omitting details for lost item.")  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__U__FOUND)
	{
		// just build the header -- we never include content details for an uncontrolled item.
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if ((statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
			 && (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED))
	{
		// just build the header -- no content to diff -- not present now and not present in baseline
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if ((statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
			 && (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED))
	{
		// just build the header -- no content to diff -- not present now and not present in baseline
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if (statusFlags & (SG_WC_STATUS_FLAGS__S__ADDED
							|SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED
							|SG_WC_STATUS_FLAGS__S__DELETED
							|SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED))
	{
		// content added/deleted/changed.
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  _diff__file__contents(pCtx, pData, pvhItem, statusFlags, pvhDiffItem, pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else
	{
		// just build the header -- content did not change -- must be a simple structural change.
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
}
