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
 * @file sg_wc__status__append.c
 *
 * @details Routines to return STATUS info for a LiveViewItem.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Add lock data to the item.  We populate a VARRAY containing
 * the info for each active lock.  This data comes from the
 * pvh_locks_in_branch data.  We only include the fields we
 * think might be displayed to the user (in Tortoise, for example).
 * The CLC currently doesn't need these fields, since 'vv locks'
 * is still available.
 * 
 *
 * <lock_data_array> ::= [ <lock_data_0>, ... ];
 * 
 * <lock_data> ::= { "username" : "<username>",
 *                   "start_hid" : "<start_hid>",
 *                   "end_csid" : "<end_csid>",
 *                 };
 *
 * WARNING: Since the __L__ bits are a UNION over all of the
 * WARNING: active locks on this item, we do not know if any
 * WARNING: particular bit applies to any particular lock.
 * WARNING: So we must re-interpret the input lock_data and
 * WARNING: set the various fields (just like we did in
 * WARNING: _cf__check_for_locks()).  That is, we don't just
 * WARNING: look at the __L__ bits and assume it applies to
 * WARNING: each row of lock_data.
 * 
 */
static void _do_lock_keys(SG_context * pCtx,
						  SG_vhash * pvh_gid_locks,
						  SG_varray * pvaLocks)
{
	// See note in _cf__check_for_locks() about the format of pvh_gid_locks.

	SG_uint32 k, nrLocks;

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gid_locks, &nrLocks)  );
	for (k=0; k<nrLocks; k++)
	{
		SG_vhash * pvh_output_k = NULL;			// we do not own this
		SG_vhash * pvh_input_k = NULL;		// we do not own this
		const char * psz_recid_k = NULL;		// we do not own this
		const char * psz_username_k = NULL;		// we do not own this
		const char * psz_start_hid_k = NULL;	// we do not own this
		const char * psz_end_csid_k = NULL;		// we do not own this

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gid_locks, k,
													 &psz_recid_k, &pvh_input_k)  );

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx,   pvh_input_k, "username",  &psz_username_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx,   pvh_input_k, "start_hid", &psz_start_hid_k)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_input_k, "end_csid",  &psz_end_csid_k)  );

		SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaLocks, &pvh_output_k)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_output_k, "username", psz_username_k)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_output_k, "start_hid", psz_start_hid_k)  );
		if (psz_end_csid_k)
		{
			// Since we filtered out the "completed" locks,
			// this item is WAITING (for a pull on the VC dag).
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_output_k, "end_csid", psz_end_csid_k)  );
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Pre-load the keys that are always present in a status.
 * Note: the layout of fields here needs to match the layout
 * in sg_vv2__status__summarize.c
 *
 */
static void _do_fixed_keys(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   sg_wc_liveview_item * pLVI,
						   SG_wc_status_flags statusFlags,
						   SG_vhash * pvhItem,
						   const char * pszWasLabel_B,
						   const char * pszWasLabel_WC)
{
	SG_string * pStringRepoPath = NULL;
	SG_string * pStringSymlinkTarget = NULL;
	const SG_string * pStringRepoPath_B = NULL;
	SG_vhash * pvhProperties = NULL;
	char * pszGid = NULL;
	char * pszGidParent = NULL;
	SG_bool bIsTmp;
	SG_uint32 countHeadings = 0;
	SG_varray * pvaHeadings = NULL;		// we do not own this
	SG_bool bPresentNow;
	SG_bool bPresentInBaseline;
	char * pszHid_owned = NULL;

	// <item> ::= { "status"   : <properties>,		-- vhash with flag bits expanded into JS-friendly fields
	//              "gid"      : "<gid>",
	//              "path"     : "<path>",					-- WC path if present in WC; B path if not.
	//              "headings" : [ "<heading_0>", ... ],	-- list of headers for classic_formatter.
	//              "B"        : <sub-section>,				-- if present in cset B (baseline)
	//              "WC"       : <sub-section>,				-- if present in the WD (live)
	//              "locks"    : [ <lock_data_0>, ... ]		-- data for each active lock
	//            };
	//
	// All fields in a <sub-section> refer to the item relative to that state.
	// 
	// <sub-section> ::= { "path"           : "<path_j>",
	//                     "name"           : "<name_j>",
	//                     "gid_parent"     : "<gid_parent_j>",
	//                     "hid"            : "<hid_j>",			-- when not a directory
	//                     "size"           : "<size_j>",			-- size of content (when hid set)
	//                     "attributes"     : <attrbits_j>,
	//                     "was_label"      : "<label_j>"			-- label for the "# x was..." line
	//                   };
	//
	// NOTE 2012/10/15 With the changes for P3580, we DO NOT populate
	// NOTE            the {hid, size, attributes} 
	// NOTE            fields in the "WC" sub-section for FOUND/IGNORED
	// NOTE            items.

	SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, statusFlags, &pvhProperties)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash( pCtx, pvhItem, "status", &pvhProperties)  );

	//////////////////////////////////////////////////////////////////
	// Add 'alias' and 'gid' for this item.
	// 
	// We assign temporary aliases/gids for found/ignored/etc items.  These are
	// basically valid for the duration of the TX.
	//
	// TODO 2011/12/27 think about removing alias since it is internal use only.

	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhItem, "alias", pLVI->uiAliasGid)  );
	// Use _alias3 so that we get 't' prefix on temporary GIDs.
	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias3(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "gid", pszGid)  );

	// For convenience, we always put a "path" field at top-level.
	// This is the current path if it exists.  Otherwise, it is a
	// baseline- or other-relative repo-path.  See W3520 for
	// merge_created+deleted case.

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pWcTx, pLVI, &pStringRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pStringRepoPath))  );

	bPresentNow = ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)	// not counting LOST
				   == 0);

	bPresentInBaseline = ((statusFlags & (SG_WC_STATUS_FLAGS__S__ADDED
										  |SG_WC_STATUS_FLAGS__S__MERGE_CREATED
										  |SG_WC_STATUS_FLAGS__S__UPDATE_CREATED
										  |SG_WC_STATUS_FLAGS__U__FOUND
										  |SG_WC_STATUS_FLAGS__U__IGNORED
										  |SG_WC_STATUS_FLAGS__R__RESERVED))
						  == 0);

	// create the sub-section for B.

	if (bPresentInBaseline)
	{
		const char * pszPath;
		const char * pszName;
		const char * pszHid;
		SG_uint64 uiAliasParent;
		SG_uint64 attrbits;
		SG_vhash * pvhB;		// we do not own this
		
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem,
											   SG_WC__STATUS_SUBSECTION__B,
											   &pvhB)  );

		// on a normal baseline-vs-wd status, the B column refers to the baseline.
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "was_label", pszWasLabel_B)  );

		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_baseline_repo_path(pCtx, pWcTx, pLVI,
																	  &pStringRepoPath_B)  );
		SG_ASSERT(  (pStringRepoPath_B)  );
		pszPath = SG_string__sz(pStringRepoPath_B);
		SG_ASSERT_RELEASE_FAIL( ((pszPath[0]=='@')
								 && (pszPath[1]==SG_WC__REPO_PATH_DOMAIN__B)
								 && (pszPath[2]=='/')) );
		SG_ASSERT(  (strncmp(pszPath, SG_WC__REPO_PATH_PREFIX__B, 3) == 0)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "path", pszPath)  );

		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_entryname(pCtx, pLVI, &pszName)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "name", pszName)  );

		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_parent_alias(pCtx, pLVI, &uiAliasParent)  );
		SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias2(pCtx, pWcTx->pDb, uiAliasParent, &pszGidParent, &bIsTmp)  );
		SG_ASSERT(  (bIsTmp == SG_FALSE)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "gid_parent", pszGidParent)  );
		SG_NULLFREE(pCtx, pszGidParent);

		switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
		{
		case SG_WC_STATUS_FLAGS__T__DIRECTORY:
			// we do not report 'hid' for directories
			break;

		case SG_WC_STATUS_FLAGS__T__FILE:
			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx, SG_FALSE, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "hid", pszHid)  );
			break;

		case SG_WC_STATUS_FLAGS__T__SYMLINK:
			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx, SG_FALSE, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "hid", pszHid)  );
			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_symlink_target(pCtx, pLVI, pWcTx, &pStringSymlinkTarget)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "target", SG_string__sz(pStringSymlinkTarget))  );
			SG_STRING_NULLFREE(pCtx, pStringSymlinkTarget);
			break;

		//case SG_WC_STATUS_FLAGS__T__SUBREPO:
		//case SG_WC_STATUS_FLAGS__T__DEVICE:	cannot happen since we never commit them (so DEVICEs won't be in baseline)
		default:
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "Status: unsupported item type: %s",
							 SG_string__sz(pStringRepoPath))  );
		}

		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pWcTx, &attrbits)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhB, "attributes", attrbits)  );

	}

	// create the sub-section for the current values.

	if (bPresentNow)
	{
		SG_vhash * pvhWC;		// we do not own this

		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem,
											   SG_WC__STATUS_SUBSECTION__WC,
											   &pvhWC)  );

		// on a normal baseline-vs-wd status, the WC column refers to the working copy
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "was_label", pszWasLabel_WC)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "path", SG_string__sz(pStringRepoPath))  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "name", SG_string__sz(pLVI->pStringEntryname))  );

		if (pLVI->pLiveViewDir)
		{
			SG_ERR_CHECK(  SG_vhash__add__int64( pCtx, pvhWC, "alias_parent", pLVI->pLiveViewDir->uiAliasGidDir)  );
			SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias3(pCtx, pWcTx->pDb, pLVI->pLiveViewDir->uiAliasGidDir,
															  &pszGidParent)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "gid_parent", pszGidParent)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "alias_parent", SG_WC_DB__ALIAS_GID__NULL_ROOT)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "gid_parent", SG_WC_DB__GID__NULL_ROOT)  );
		}

		if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
		{
			// no current values for hid, attrbits, ....
		}
		else if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND | SG_WC_STATUS_FLAGS__U__IGNORED))
		{
			// don't compute HID for uncontrolled things.
			// this should help prevent ignored/found files
			// from appearing in the TSC, for example. P3580.
			//
			// TODO 2012/10/15 I'm wondering if we should just
			// TODO            not have a "WC" section for them.
		}
		else
		{
			SG_uint64 attrbits;
			SG_uint64 sizeContent;

			switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
			{
			case SG_WC_STATUS_FLAGS__T__DIRECTORY:
				// we do not report 'hid' for directories
				break;

			case SG_WC_STATUS_FLAGS__T__FILE:
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid( pCtx, pLVI, pWcTx, SG_FALSE,
																			 &pszHid_owned, &sizeContent)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "hid", pszHid_owned)  );
				SG_NULLFREE(pCtx, pszHid_owned);
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "size", (SG_int64)sizeContent)  );
				break;

			case SG_WC_STATUS_FLAGS__T__SYMLINK:
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid( pCtx, pLVI, pWcTx, SG_FALSE,
																			 &pszHid_owned, &sizeContent)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "hid", pszHid_owned)  );
				SG_NULLFREE(pCtx, pszHid_owned);
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "size", (SG_int64)sizeContent)  );
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_symlink_target(pCtx, pLVI, pWcTx, &pStringSymlinkTarget)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "target", SG_string__sz(pStringSymlinkTarget))  );
				SG_STRING_NULLFREE(pCtx, pStringSymlinkTarget);
				break;

			//case SG_WC_STATUS_FLAGS__T__SUBREPO:
			//case SG_WC_STATUS_FLAGS__T__DEVICE:	cannot happen since we never they will always be found/ignored (caught above)
			default:
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "Status: unsupported item type: %s",
								 SG_string__sz(pStringRepoPath))  );
			}

			SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits( pCtx, pLVI, pWcTx, &attrbits)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "attributes", attrbits)  );
		}
	}

	if (statusFlags & SG_WC_STATUS_FLAGS__L__MASK)
	{
		SG_vhash * pvh_gid_locks;	// we do not own this
		SG_varray * pvaLocks;		// we do not own this

		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pWcTx->pLockInfoForStatus->pvh_locks_in_branch, pszGid,
											  &pvh_gid_locks)  );
		SG_ASSERT_RELEASE_FAIL( (pvh_gid_locks) );

		SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvhItem, "locks", &pvaLocks)  );
		SG_ERR_CHECK(  _do_lock_keys(pCtx, pvh_gid_locks, pvaLocks)  );
	}

	//////////////////////////////////////////////////////////////////
	// Build an array of heading names for the formatter.

#define APPEND_HEADING(name)											\
	SG_STATEMENT(														\
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,				\
													 pvaHeadings,		\
													 (name))  );		\
		)

	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvhItem, "headings", &pvaHeadings)  );

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
		APPEND_HEADING("Added");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
		APPEND_HEADING("Added (Merge)");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
		APPEND_HEADING("Added (Update)");

	if (statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
		APPEND_HEADING("Modified");

	// only one of these __M__AUTO_MERGED should ever be set.
	// __M__AUTO_MERGED should only be set if __T__FILE.
	if (statusFlags & SG_WC_STATUS_FLAGS__M__AUTO_MERGED)
		APPEND_HEADING("Auto-Merged");
	else if (statusFlags & SG_WC_STATUS_FLAGS__M__AUTO_MERGED_EDITED)
		APPEND_HEADING("Auto-Merged (Edited)");

	if (statusFlags & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)
		APPEND_HEADING("Attributes");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
		APPEND_HEADING("Removed");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__RENAMED)
		APPEND_HEADING("Renamed");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MOVED)
		APPEND_HEADING("Moved");

	if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
		APPEND_HEADING("Lost");
	if (statusFlags & SG_WC_STATUS_FLAGS__U__FOUND)
		APPEND_HEADING("Found");
	if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)
		APPEND_HEADING("Ignored");
	if (statusFlags & SG_WC_STATUS_FLAGS__R__RESERVED)
		APPEND_HEADING("Reserved");
	if (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)
		APPEND_HEADING("Sparse");

	// These __M__EXISTENCE_ bits are only set in MSTATUS, but
	// it doesn't hurt to check.

	// only 1 of these __M__EXISTENCE should ever be set.
	if (statusFlags & SG_WC_STATUS_FLAGS__M__ABcM)
		APPEND_HEADING("Existence (B,!C,WC)");
	if (statusFlags & SG_WC_STATUS_FLAGS__M__AbCM)
		APPEND_HEADING("Existence (!B,C,WC)");

	SG_ASSERT_RELEASE_FAIL2(  (((statusFlags & SG_WC_STATUS_FLAGS__X__UNRESOLVED) == 0)
							   || ((statusFlags & SG_WC_STATUS_FLAGS__X__RESOLVED) == 0)),
							  (pCtx, "Both RESOLVED and UNRESOLVED set: %s", SG_string__sz(pStringRepoPath))  );
	if (statusFlags & SG_WC_STATUS_FLAGS__X__UNRESOLVED)
	{
		APPEND_HEADING("Unresolved");

		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__EXISTENCE)	APPEND_HEADING(  "Choice Resolved (Existence)");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__EXISTENCE)	APPEND_HEADING("Choice Unresolved (Existence)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__NAME)		APPEND_HEADING(  "Choice Resolved (Name)");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__NAME)		APPEND_HEADING("Choice Unresolved (Name)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__LOCATION)	APPEND_HEADING(  "Choice Resolved (Location)");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__LOCATION)	APPEND_HEADING("Choice Unresolved (Location)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__ATTRIBUTES)	APPEND_HEADING(  "Choice Resolved (Attributes)");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES)	APPEND_HEADING("Choice Unresolved (Attributes)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__CONTENTS)	APPEND_HEADING(  "Choice Resolved (Contents)");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__CONTENTS)	APPEND_HEADING("Choice Unresolved (Contents)");
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__X__RESOLVED)
	{
		APPEND_HEADING("Resolved");

		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__EXISTENCE)	APPEND_HEADING(  "Choice Resolved (Existence)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__NAME)		APPEND_HEADING(  "Choice Resolved (Name)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__LOCATION)	APPEND_HEADING(  "Choice Resolved (Location)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__ATTRIBUTES)	APPEND_HEADING(  "Choice Resolved (Attributes)");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__CONTENTS)	APPEND_HEADING(  "Choice Resolved (Contents)");
	}

	// TODO 2012/10/09 These lock headings are a little convoluted
	// TODO            in wording.  They have the same prefix so that
	// TODO            in __classic_format they group together; see
	// TODO            aSections.
	if (statusFlags & SG_WC_STATUS_FLAGS__L__LOCKED_BY_USER)
		APPEND_HEADING("Locked (by You)");
	if (statusFlags & SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER)
		APPEND_HEADING("Locked (by Others)");
	if (statusFlags & SG_WC_STATUS_FLAGS__L__WAITING)
		APPEND_HEADING("Locked (Waiting)");
	if (statusFlags & SG_WC_STATUS_FLAGS__L__PENDING_VIOLATION)
		APPEND_HEADING("Locked (Pending Violation)");

	// TODO 2012/10/09 Should we include locks when checking for unchanged?
	// TODO            Since we're just counting the number of headers, it
	// TODO            include the ones for the lock bits.
	// TODO            Are there any other bits/headers that we should not
	// TODO            count when deciding if the item is "unchanged" ?

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaHeadings, &countHeadings)  );
	if (countHeadings == 0)
		APPEND_HEADING("Unchanged");

	//////////////////////////////////////////////////////////////////

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringSymlinkTarget);
	SG_VHASH_NULLFREE(pCtx, pvhProperties);
	SG_NULLFREE(pCtx, pszHid_owned);
	SG_NULLFREE(pCtx, pszGid);
	SG_NULLFREE(pCtx, pszGidParent);
}

//////////////////////////////////////////////////////////////////

/**
 * Create a canonical <item> and insert it into a single
 * canonical list.
 *
 */
static void _do_canonical(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  sg_wc_liveview_item * pLVI,
						  SG_wc_status_flags statusFlags,
						  const char * pszWasLabel_B,
						  const char * pszWasLabel_WC,
						  SG_varray * pvaStatus)
{
	SG_vhash * pvhItem;		// we don't own this

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaStatus, &pvhItem)  );
	SG_ERR_CHECK(  _do_fixed_keys(pCtx, pWcTx, pLVI, statusFlags, pvhItem,
								  pszWasLabel_B, pszWasLabel_WC)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Append a VHASH-ROW containing the CANONICAL STATUS of the given item
 * to the STATUS-VARRAY.
 *
 * The "canonical status" is a VARRAY; each item is completely contained
 * within a single row VHASH within it.  (Unlike the old "classic status"
 * which had sections.)
 * 
 * We put enough info in the VHASH row to let the caller display it however
 * they need -- without needing to touch the LVI or other data structures.
 *
 * NOTE 2012/04/26 We DO NOT inspect the existing VARRAY to see if the given
 * NOTE            item is already present.  That would be WAY too expensive.
 * NOTE            Since duplicates can only happen if the caller was given
 * NOTE            multiple arguments (rather than a single item to recurse),
 * NOTE            we will let the caller be responsible for doing make-unique
 * NOTE            pass.  See W1207.
 * TODO
 * TODO            Perhaps make this an optional argument with a big DONT USE
 * TODO            IT message.  For example, SG_wc_tx__status__stringarray()
 * TODO            would need it; SG_wc_tx__status() would not.
 * 
 */
void sg_wc__status__append(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   sg_wc_liveview_item * pLVI,
						   SG_wc_status_flags statusFlags,
						   SG_bool bListUnchanged,
						   SG_bool bListSparse,
						   SG_bool bListReserved,
						   const char * pszWasLabel_B,
						   const char * pszWasLabel_WC,
						   SG_varray * pvaStatus)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI );
	SG_NULLARGCHECK_RETURN( pvaStatus );

	if ((statusFlags & (SG_WC_STATUS_FLAGS__U__MASK
						|SG_WC_STATUS_FLAGS__S__MASK
						|SG_WC_STATUS_FLAGS__C__MASK
						|SG_WC_STATUS_FLAGS__X__MASK
						|SG_WC_STATUS_FLAGS__L__MASK
			 ))
		|| bListUnchanged
		|| ((statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE) && bListSparse)
		|| ((statusFlags & SG_WC_STATUS_FLAGS__R__MASK) && bListReserved)
		)
	{
		SG_ERR_CHECK(  _do_canonical(pCtx, pWcTx,
									 pLVI,
									 statusFlags,
									 pszWasLabel_B, pszWasLabel_WC,
									 pvaStatus)  );
	}

fail:
	return;
}
