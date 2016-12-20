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

/**
 * We are given an existing item that needs to be moved/renamed.
 * The caller already tried to move it to where it needs to be
 * but got some kind of error/collision/conflict.
 *
 * Here we "park" it by moving and renaming it to a temporary
 * filename in the root directory.  The caller needs to arrange
 * for an eventual "unpark" step to put it where it should be.
 *
 */
void sg_wc_tx__park__move_rename(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const char * pszGid)
{
	SG_string * pString_RepoPath_Src = NULL;
	SG_string * pString_Prefix = NULL;
	SG_string * pString_ParkedEntryname = NULL;
	SG_uint32 k = 0;

	// Use a gid-domain extended repo-path to name the item.
	// This lets us not care where it actually is in the tree.

	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pString_RepoPath_Src)  );

	// Build parked entryname as ".park.<gid7>.<k>"
	//
	// Normally, we should not get a collision with such
	// a contrived entryname, but if there is garbage in
	// the directory, it could happen.  So we append unique
	// suffixes until we successful.
	//
	// Note that we can't/don't need to call SG_fsobj__exists
	// on this because the LVI code already knows all of this
	// and we are in-tx so the disk may lie.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_Prefix)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pString_Prefix, ".park.")  );
	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pString_Prefix, (const SG_byte *)pszGid, 7)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_ParkedEntryname)  );

	while (1)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString_ParkedEntryname, "%s.%02d",
										  SG_string__sz(pString_Prefix), k++)  );

		SG_wc_tx__move_rename2(pCtx, pWcTx,
							   SG_string__sz(pString_RepoPath_Src),
							   "@/",		// we always park items in the root directory
							   SG_string__sz(pString_ParkedEntryname),
							   SG_FALSE);
		if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
			break;

		if (SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS)
			|| SG_context__err_equals(pCtx, SG_ERR_NO_EFFECT))
		{
			// found existing dirt.  try again.
			SG_context__err_reset(pCtx);
		}
		else
		{
			SG_ERR_RETHROW;
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, pString_RepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pString_Prefix);
	SG_STRING_NULLFREE(pCtx, pString_ParkedEntryname);
}

//////////////////////////////////////////////////////////////////

/**
 * We are given an item that needs "undo_delete" but into a
 * "parked" location.  The caller should have already tried
 * to "undo_delete" it and gotten some kind of error/collision/conflict.
 *
 * Here we "park" it by restoring it to a temporary
 * filename in the root directory.  The caller needs to arrange
 * for an eventual "unpark" step to put it where it should be.
 *
 * This DOES NOT recurse into the contents of restored directories.
 * 
 */
void sg_wc_tx__park__undo_delete(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const char * pszGid)
{
	SG_string * pString_RepoPath_Src = NULL;
	SG_string * pString_Prefix = NULL;
	SG_string * pString_ParkedEntryname = NULL;
	SG_uint32 k = 0;

	// Use a gid-domain extended repo-path to name the item.
	// This lets us not care where it actually is in the tree.

	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pString_RepoPath_Src)  );

	// Build parked entryname as ".park.<gid7>.<k>"
	//
	// Normally, we should not get a collision with such
	// a contrived entryname, but if there is garbage in
	// the directory, it could happen.  So we append unique
	// suffixes until we successful.
	//
	// Note that we can't/don't need to call SG_fsobj__exists
	// on this because the LVI code already knows all of this
	// and we are in-tx so the disk may lie.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_Prefix)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pString_Prefix, ".park.")  );
	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pString_Prefix, (const SG_byte *)pszGid, 7)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_ParkedEntryname)  );

	while (1)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString_ParkedEntryname, "%s.%02d",
										  SG_string__sz(pString_Prefix), k++)  );

		SG_wc_tx__undo_delete(pCtx, pWcTx,
							  SG_string__sz(pString_RepoPath_Src),
							  "@/",		// we always park items in the root directory
							  SG_string__sz(pString_ParkedEntryname),
							  NULL,
							  SG_WC_STATUS_FLAGS__ZERO);
		if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
			break;
		
		if ( ! SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS))
			SG_ERR_RETHROW;

		// found existing dirt.  try again.
		SG_context__err_reset(pCtx);
	}

fail:
	SG_STRING_NULLFREE(pCtx, pString_RepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pString_Prefix);
	SG_STRING_NULLFREE(pCtx, pString_ParkedEntryname);
}

//////////////////////////////////////////////////////////////////

/**
 * We are given an item that needs "undo_lost" but into a
 * "parked" location.  The caller should have already tried
 * to "undo_lost" it and gotten some kind of error/collision/conflict.
 *
 * Here we "park" it by restoring it to a temporary
 * filename in the root directory.  The caller needs to arrange
 * for an eventual "unpark" step to put it where it should be.
 *
 * This DOES NOT recurse into the contents of restored directories.
 * 
 */
void sg_wc_tx__park__undo_lost(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   const char * pszGid)
{
	SG_string * pString_RepoPath_Src = NULL;
	SG_string * pString_Prefix = NULL;
	SG_string * pString_ParkedEntryname = NULL;
	SG_uint32 k = 0;

	// Use a gid-domain extended repo-path to name the item.
	// This lets us not care where it actually is in the tree.

	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pszGid, &pString_RepoPath_Src)  );

	// Build parked entryname as ".park.<gid7>.<k>"
	//
	// Normally, we should not get a collision with such
	// a contrived entryname, but if there is garbage in
	// the directory, it could happen.  So we append unique
	// suffixes until we successful.
	//
	// Note that we can't/don't need to call SG_fsobj__exists
	// on this because the LVI code already knows all of this
	// and we are in-tx so the disk may lie.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_Prefix)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pString_Prefix, ".park.")  );
	SG_ERR_CHECK(  SG_string__append__buf_len(pCtx, pString_Prefix, (const SG_byte *)pszGid, 7)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString_ParkedEntryname)  );

	while (1)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString_ParkedEntryname, "%s.%02d",
										  SG_string__sz(pString_Prefix), k++)  );

		SG_wc_tx__undo_lost(pCtx, pWcTx,
							SG_string__sz(pString_RepoPath_Src),
							"@/",		// we always park items in the root directory
							SG_string__sz(pString_ParkedEntryname),
							NULL,
							SG_WC_STATUS_FLAGS__ZERO);
		if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
			break;
		
		if ( ! SG_context__err_equals(pCtx, SG_ERR_WC__ITEM_ALREADY_EXISTS))
			SG_ERR_RETHROW;

		// found existing dirt.  try again.
		SG_context__err_reset(pCtx);
	}

fail:
	SG_STRING_NULLFREE(pCtx, pString_RepoPath_Src);
	SG_STRING_NULLFREE(pCtx, pString_Prefix);
	SG_STRING_NULLFREE(pCtx, pString_ParkedEntryname);
}


