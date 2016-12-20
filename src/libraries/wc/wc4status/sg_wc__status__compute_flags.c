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
 * @file sg_wc__status__compute_flags.c
 *
 * @details Routine to convert the various internal flags
 * (from tne_L0, tbl_pc, and readdir) into a simplified
 * set of public "status" flags.
 *
 * These should be suitable for using to print info on the
 * console or website or tortoise or whatever.
 *
 * These STATUS_FLAGS are similar to, but slightly different
 * from the original PendingTree/Treediff2 flags.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static void _cf__lookup_auto_merge_result(SG_context * pCtx,
										  const SG_vhash * pvhIssue,
										  const char * pszHid_CurrentContent,
										  SG_wc_status_flags * pStatusFlags)
{
	// Pull the auto-merge result from the issue (if present).
	// We know the HID of the file that auto-merge created (if
	// it was successful).  This doesn't change once MERGE has
	// completed.  We also know the HID of the file as it exists
	// on disk now.  Compare the 2 and set __M__ fields.  What
	// we want to be able to report in STATUS/MSTATUS is:
	// [] "L0!=L1, auto-merged"
	// [] "L0!=L1, auto-merged + edits"
	// [] "L0!=L1, not-auto-merged with or without WC edits"
	//
	// We DO NOT count the __M__ bits in the nrChangeBits because
	// this is just qualifying the __C__NON_DIR_MODIFIED bits above.
	//
	// Note that the "automerge_disposable_hid" field is *ONLY* set
	// if the auto-merge tool is successful and *fully automatic*.
	// Anything less and we don't want to make any statements about
	// the resulting file.
	//
	// We now also have the "automerge_generated_hid" field that is
	// set if the tool outputs anything.
	// 
	// Note that the issue.output.hid fiels is not being set for
	// auto-merged items.  It means something different.  (See W6195).

	const char * pszHid_Generated = NULL;
				
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhIssue, "automerge_generated_hid", &pszHid_Generated)  );
	if (!pszHid_Generated)
		return;

	if (strcmp(pszHid_CurrentContent, pszHid_Generated) == 0)
	{
		// WC still contains the auto-merge result that we put there.
		*pStatusFlags |= SG_WC_STATUS_FLAGS__M__AUTO_MERGED;
	}
	else
	{
		// WC has been edited since we did the auto-merge.
		*pStatusFlags |= SG_WC_STATUS_FLAGS__M__AUTO_MERGED_EDITED;
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Load the current LOCK info so that STATUS can compute the __L__ bits.
 * This was defered by begin-tx.
 *
 */
static void _get_lock_data_for_status(SG_context * pCtx,
									  SG_wc_tx * pWcTx)
{
	SG_ERR_CHECK(  SG_alloc1(pCtx, pWcTx->pLockInfoForStatus)  );
	
	SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pWcTx, &pWcTx->pLockInfoForStatus->psz_lock_branch_name)  );
	if (pWcTx->pLockInfoForStatus->psz_lock_branch_name)
	{
		SG_ERR_CHECK(  SG_vc_locks__list_for_one_branch(pCtx, pWcTx->pDb->pRepo,
														pWcTx->pLockInfoForStatus->psz_lock_branch_name,
														SG_FALSE, // do not include completed locks
														&pWcTx->pLockInfoForStatus->pvh_locks_in_branch)  );
		if (pWcTx->pLockInfoForStatus->pvh_locks_in_branch)
		{
			SG_uint32 nrActiveLocks;
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pWcTx->pLockInfoForStatus->pvh_locks_in_branch, &nrActiveLocks)  );
			if (nrActiveLocks == 0)
			{
				// not sure if __list_for_one_branch() will give us an empty
				// vhash or not, but it doesn't hurt to check it.
				SG_VHASH_NULLFREE(pCtx, pWcTx->pLockInfoForStatus->pvh_locks_in_branch);
			}
			else
			{
				SG_audit q;

#if TRACE_WC_TX_STATUS
				SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx,
																	   pWcTx->pLockInfoForStatus->pvh_locks_in_branch,
																	   "LocksInBranch")  );
#endif

				// get the current WhoAmI/UserId -- if not set, we
				// silently continue (and report any locks as owned
				// by others).
				SG_ERR_IGNORE(  SG_audit__init(pCtx, &q, pWcTx->pDb->pRepo,
											   SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
				if (q.who_szUserId && q.who_szUserId[0])
					SG_ERR_CHECK(  SG_STRDUP(pCtx, q.who_szUserId,
											 &pWcTx->pLockInfoForStatus->psz_userid_for_lock_reporting_in_status)  );
			}
		}
	}
	else
	{
		// If we are not attached to a branch, we shouldn't
		// report on locks.  We keep pWcTx->pLockInfoForStatus
		// around (but empty) to indicate that we already
		// tried to load it and don't have anything to report.
	}

fail:
	return;
}


/**
 * Set the various __L__ bits in *pStatusFlags.
 *
 * WARNING: We assume that we DID NOT include completed locks
 * WARNING: when we requested the lock data.
 *
 * WARNING: The StatusFlags form a UNION over all locks on this
 * WARNING: item.  For example, the item may have 3 active locks
 * WARNING: on it, so each __L__ bit means that at least 1 of
 * WARNING: active (open or waiting) locks had that bit.  For
 * WARNING: better/finer detail, see the "locks" section in the
 * WARNING: resulting status vhash.  (See _do_lock_keys())
 * 
 */
static void _cf__check_for_locks(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 sg_wc_liveview_item * pLVI,
								 SG_wc_status_flags * pStatusFlags)
{
	char * pszGid = NULL;
	SG_vhash * pvh_gid_locks;	// we do not own this
	
	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );

	// pvh_locks_in_branch ::= { "<gid_1>" : { <gid_locks_1 },
	//                           "<gid_2>" : { <gid_locks_2 },
	//                           ... };
	//
	// pvh_gid_locks = pvh_locks_in_branch["<gid_y>"];
	//
	// pvh_gid_locks ::= { "<recid_1>" : { <lock_data_1> },
	//                     "<recid_2>" : { <lock_data_2> },
	//                     ... };
	//
	// pvh_lock_data = pvh_gid_locks["<recid_z>"];
	//
	// pvh_lock_data ::= { "recid" : "<recid_z>",
	//                     "username" : "<username>",
	//                     ... };
	//
	// For example:                     
    //     {
    //         "g8d0a7abd6add43f68ae4aeadf8dcf9db83f8648e12e511e2bafd002500da2b78" : 
    //         {
    //             "gbe0ac37a90724c4698ef108ca5a6271f84d5015a12e511e2be52002500da2b78" : 
    //             {
    //                 "recid" : "gbe0ac37a90724c4698ef108ca5a6271f84d5015a12e511e2be52002500da2b78",
    //                 "username" : "john",
    //                 "userid" : "gc4919521f867493082511ad12822093283bc6e2a12e511e2b4a8002500da2b78",
    //                 "start_hid" : "917f0c378aa5d6a1b06bcf5ffcaa048479db3601"
    //             }
    //         }
	//     }

	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pWcTx->pLockInfoForStatus->pvh_locks_in_branch, pszGid, &pvh_gid_locks)  );
	if (pvh_gid_locks)
	{
		SG_uint32 k, nrLocks;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_gid_locks, &nrLocks)  );
		for (k=0; k<nrLocks; k++)
		{
			SG_vhash * pvh_lock_data_k = NULL;		// we do not own this
			const char * psz_recid_k = NULL;		// we do not own this
			const char * psz_userid_k = NULL;		// we do not own this
			const char * psz_end_csid_k = NULL;		// we do not own this

			SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_gid_locks, k,
														 &psz_recid_k, &pvh_lock_data_k)  );

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx,   pvh_lock_data_k, "userid",       &psz_userid_k)  );
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvh_lock_data_k, "end_csid",     &psz_end_csid_k)  );

			if (psz_end_csid_k)
			{
				// Since we filtered out "completed" locks (locks where
				// the end_csid is set and we actually have the vc changeset
				// in our repo), all we have left is that we are waiting on
				// a vc pull.
				*pStatusFlags |= SG_WC_STATUS_FLAGS__L__WAITING;
			}
			else
			{
				// the lock is still active.
			}

			// if we have a 'whoami' set *AND* were able to convert it into a valid username,
			// see if it matches the owner of the lock.

			if (pWcTx->pLockInfoForStatus->psz_userid_for_lock_reporting_in_status
				&& *pWcTx->pLockInfoForStatus->psz_userid_for_lock_reporting_in_status
				&& (strcmp(psz_userid_k,
						   pWcTx->pLockInfoForStatus->psz_userid_for_lock_reporting_in_status) == 0))
			{
				*pStatusFlags |= SG_WC_STATUS_FLAGS__L__LOCKED_BY_USER;
			}
			else
			{
				*pStatusFlags |= SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER;
			
				if ((*pStatusFlags) & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
					*pStatusFlags |= SG_WC_STATUS_FLAGS__L__PENDING_VIOLATION;
			}
		}
	}

fail:
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

/**
 * Compute a set of STATUS flags for the item.
 * This can be broken down into various parts:
 * 
 * [1] the structural state. (moves, renames, adds, etc.)
 *     (Stuff that the user tells us to manipulate.)
 *     We *already* know all about this type of info by
 *     the time the LVI is constructed because of the way
 *     we parse/traverse the repo-path (and implicitly
 *     scandir/readdir and/or hit the various db tables
 *     and caches) during the __fetch_lvi.
 *
 * [2] the dynamic state. (edits, attrbits)
 *     (Stuff that the user manipulates directly and
 *     does not tell us about.)
 *     We *could* have computed this information during
 *     the READDIR step, but we deferred it until actually
 *     needed.
 *
 * So this call to compute status *WILL NOT* go looking
 * for any structural changes (or do implicit SCANDIRS,
 * READDIR, etc), but it *MAY* cause us to sniff the item
 * and/or look at timestamps and etc.
 * 
 *
 * If the caller sets "bNoIgnores", then we do not use the
 * IGNORES config settings and simply report all uncontrolled
 * items as FOUND (rather than computing whether they should
 * be marked FOUND vs IGNORED).
 *
 *
 * WARNING: If you make changes here, see
 * sg_vv2__status__od__compute_status_flags().
 * See also sg_wc_tx__fake_merge__update__status().
 * 
 */
void sg_wc__status__compute_flags(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  sg_wc_liveview_item * pLVI,
								  SG_bool bNoIgnores,
								  SG_bool bNoTSC,
								  SG_wc_status_flags * pStatusFlags)
{
	SG_string * pStringLiveRepoPath = NULL;
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;
	char * pszHid_CurrentContent = NULL;
	SG_uint32 nrChangeBits__Found    = 0;
	SG_uint32 nrChangeBits__Ignored  = 0;
	SG_uint32 nrChangeBits__Lost     = 0;
	SG_uint32 nrChangeBits__Added    = 0;
	SG_uint32 nrChangeBits__Deleted  = 0;
	SG_uint32 nrChangeBits__Renamed  = 0;
	SG_uint32 nrChangeBits__Moved    = 0;
	SG_uint32 nrChangeBits__Attrbits = 0;
	SG_uint32 nrChangeBits__Content  = 0;
	SG_uint32 nrChangeBits__Conflict = 0;

	// pLVI is optional so we can set the bogus bit.

	// step 1: fill in the item type.

	SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx,
													((pLVI)
													 ? pLVI->tneType
													 : SG_TREENODEENTRY_TYPE__INVALID),
													&statusFlags)  );
	if (!pLVI)
		goto done;

	// step 2: characterize any structural changes relative
	// to the baseline.

	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		if (bNoIgnores)
		{
			// If '--no-ignores', we list everything as "Found" regardless
			// of how or whether it was qualified.
			statusFlags |= SG_WC_STATUS_FLAGS__U__FOUND;
			nrChangeBits__Found = 1;
		}
		else
		{
			SG_bool bIgnorable;

			// During the scandir/readdir we just marked the item
			// as __UNCONTROLLED_UNQUALIFIED and *DID NOT* try to
			// determine whether it should be considered FOUND or
			// IGNORED.  Part of this was for speed -- defer the
			// "is-ignorable" question until we actually need the
			// answer.  But the main (subtle) reason is that the
			// answer can change between when we did the scandir
			// and now.  If, for example, there are moves/renames
			// (in either this item or a parent directory) that
			// affect the pattern match.
			//
			// Likewise, we do not cache this result.

			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx,
																	  pWcTx,
																	  pLVI,
																	  &pStringLiveRepoPath)  );
			SG_ERR_CHECK(  sg_wc_db__ignores__matches_ignorable_pattern(pCtx,
																		pWcTx->pDb,
																		pStringLiveRepoPath,
																		&bIgnorable)  );
			if (bIgnorable)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__U__IGNORED;
				nrChangeBits__Ignored = 1;
			}
			else
			{
				statusFlags |= SG_WC_STATUS_FLAGS__U__FOUND;
				nrChangeBits__Found = 1;
			}
		}

		goto done;
	}
	else if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		statusFlags |= SG_WC_STATUS_FLAGS__R__RESERVED;
		goto done;
	}
	else
	{
		SG_ASSERT(  (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED(pLVI->scan_flags_Live))  );
		SG_ASSERT(  (pLVI->pPrescanRow || pLVI->pPcRow_PC)  );	// true because controlled.

		// In the original pendingtree code some of the _DIFFSTATUS_ bits
		// were treated as mutually-exclusive.  Not any more.

		if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(pLVI->scan_flags_Live))
		{
			statusFlags |= SG_WC_STATUS_FLAGS__U__LOST;
			nrChangeBits__Lost = 1;
		}

		if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live))
		{
			statusFlags |= SG_WC_STATUS_FLAGS__A__SPARSE;
			// This is not a 'change', just an indication that
			// our view of the tree is sparse.
			//
			// Note that we could also test for __FLAGS_NET__SPARSE
			// in the code below.
		}

		// We want to report the differences relative to the baseline.

		SG_ERR_CHECK(  sg_wc_liveview_item__get_flags_net(pCtx, pLVI, &flags_net)  );
		
		if (flags_net != SG_WC_DB__PC_ROW__FLAGS_NET__ZERO)
		{
			// If we have an entry in the tbl_pc table then (at least at
			// one point in time (before or during the current transaction)
			// there is or was a structual change.  (We may or may not have
			// put the PC row in canonical form.)

			if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADDED)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__ADDED;
				nrChangeBits__Added = 1;
			}

			if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_M)
			{
				// When an item is created by the MERGE
				// (because it was added in the OTHER branch
				// or because MERGE needed to override a delete),
				// the net result is that the item will appear
				// as ADDED to the WD (from the point of view
				// of the BASELINE).
				//
				// We carry this forward in a separate bit.
				// I tried combining it with __S__ADDED, but
				// there are some cases that are awkward to handle.

				statusFlags |= SG_WC_STATUS_FLAGS__S__MERGE_CREATED;
				nrChangeBits__Added = 1;
			}

			if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__UPDATE_CREATED;
				nrChangeBits__Added = 1;
			}

			if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__DELETED)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__DELETED;
				nrChangeBits__Deleted = 1;
			}
			
			if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__RENAMED)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__RENAMED;
				nrChangeBits__Renamed = 1;
			}
			
			if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__MOVED)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__MOVED;
				nrChangeBits__Moved = 1;
			}
		}
	}

	// step 3: look for any dynamic changes (sniffing if necessary).

	if (statusFlags & (SG_WC_STATUS_FLAGS__S__ADDED
					   |SG_WC_STATUS_FLAGS__U__FOUND
					   |SG_WC_STATUS_FLAGS__U__IGNORED))
	{
		// The item was not present in the baseline and is
		// present now (whether controlled or uncontrolled).
		// Can't compute a DIFF when there isn't a left side.
		//
		// Don't set any __C__bits.
		goto skip_C_bits;
	}
	else if (statusFlags & (SG_WC_STATUS_FLAGS__S__DELETED
							|SG_WC_STATUS_FLAGS__U__LOST))
	{
		// NOTE 2012/12/06 I removed __A__SPARSE from this
		// NOTE            section because we now have the
		// NOTE            {sparse_hid,sparse_attrbits} fields
		// NOTE            in tbl_PC.

		// The item does not currently exist in the WD
		// (whether it was in the baseline or merged in
		// doesn't matter).  Either way we cannot compute
		// a DIFF when there isn't a right side.
		//
		// Don't set any __C__ bits.
		goto skip_C_bits;
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
	{
		// The item was not present in the baseline.
		// We cannot compute a diff WRT the baseline.
		//
		// Don't set any __C__ bits.
		goto skip_C_bits;
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
	{
		// The item was not present in the baseline (so we don't
		// have a left side), but we *may* have some info that
		// UPDATE left for us from the previous baseline.
		//
		// Currently the pc_L0 table only has a cached value of
		// the content hid for files, so we can skip looking for
		// changes to the attrbits.
		//
		// TODO 2012/07/25 Should we ask UPDATE to cache these
		// TODO            fields too?
		goto do_C_bits__mod;
	}
	else
	{
		goto do_C_bits;
	}

do_C_bits:
	{
		// The item was present in the baseline *and* is
		// currently present in the WD.  It may or may not
		// be present in the other branch.
		//
		// We define the __C__ bits to be the diff of all
		// of the various dynamic fields using the normal
		// diff(baseline,live) definition.

		{
			SG_uint64 attrbits1 = SG_WC_ATTRBITS__ZERO;
			SG_uint64 attrbits2 = SG_WC_ATTRBITS__ZERO;

			// look for changes in ATTRBITS

			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pLVI, pWcTx, &attrbits1)  );
			SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits( pCtx, pLVI, pWcTx, &attrbits2)  );
			if (attrbits1 != attrbits2)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;
				nrChangeBits__Attrbits = 1;
			}
		}
	}

do_C_bits__mod:
	{
		if (pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			const char * pszHid1 = NULL;

			// look for changes in non-directory content

			SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx, bNoTSC, &pszHid1)  );
			SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid( pCtx, pLVI, pWcTx, bNoTSC,
																		 &pszHid_CurrentContent, NULL)  );
			if (strcmp( ((pszHid1) ? pszHid1 : ""),
						((pszHid_CurrentContent) ? pszHid_CurrentContent : "")) != 0)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED;
				nrChangeBits__Content = 1;
			}
		}
	}

skip_C_bits:
	//////////////////////////////////////////////////////////////////
	// step 5: see if there is a pending merge and if so
	// is this item present in the other cset and were
	// there any conflicts/issues.

	if (pLVI->pvhIssue)
	{
		statusFlags |= pLVI->statusFlags_x_xr_xu;

		// Per W5770 we DO NOT count the __X__ bits when
		// setting __A__MULTIPLE_CHANGE.
		// nrChangeBits__Conflict = 1;

		// See if we can tell them something about an auto-merge
		// result, if present.
		//
		// We DO NOT count any __M__AUTO_MERGE* bits when
		// setting __A__MULTIPLE_CHANGES.

		if ((pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
			&& pszHid_CurrentContent)
			SG_ERR_CHECK(  _cf__lookup_auto_merge_result(pCtx, pLVI->pvhIssue,
														 pszHid_CurrentContent,
														 &statusFlags)  );
	}

	//////////////////////////////////////////////////////////////////
	// step 6: see if there are any LOCK bits on the item.
	// 
	// TODO 2012/10/10 Should we count LOCK bits in __A__MULTIPLE_CHANGE?

	if (pLVI->tneType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
	{
		if (!pWcTx->pLockInfoForStatus)
		{
			// we defered loading the locks in begin-tx until now.
			SG_ERR_CHECK(  _get_lock_data_for_status(pCtx, pWcTx)  );
		}
		if (pWcTx->pLockInfoForStatus->psz_lock_branch_name
			&& pWcTx->pLockInfoForStatus->pvh_locks_in_branch)
		{
			SG_ERR_CHECK(  _cf__check_for_locks(pCtx, pWcTx, pLVI, &statusFlags)  );
		}
	}

done:

	if ((nrChangeBits__Found
		 + nrChangeBits__Ignored
		 + nrChangeBits__Lost
		 + nrChangeBits__Added
		 + nrChangeBits__Deleted
		 + nrChangeBits__Renamed
		 + nrChangeBits__Moved
		 + nrChangeBits__Attrbits
		 + nrChangeBits__Content
		 + nrChangeBits__Conflict) > 1)
	{
		statusFlags |= SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE;
	}

#if TRACE_WC_TX_STATUS && defined(DEBUG)
	SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags_to_console(pCtx, statusFlags,
															   ((pLVI && pLVI->pStringEntryname)
																? SG_string__sz(pLVI->pStringEntryname)
																: "(null)"))  );
#endif

	*pStatusFlags = statusFlags;

fail:
	SG_STRING_NULLFREE(pCtx, pStringLiveRepoPath);
	SG_NULLFREE(pCtx, pszHid_CurrentContent);
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_wc_debug__status__dump_flags(SG_context * pCtx,
									 SG_wc_status_flags statusFlags,
									 const char * pszLabel,
									 SG_string ** ppString)
{
	SG_string * pString = NULL;
	SG_int_to_string_buffer buf;


	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(
		SG_string__sprintf(
			pCtx, pString,
			"%s: [%s] L=%x, XU=%x, XR=%x, X=%x, M=%x, C=%x, S=%x, U=%x, R=%x, A=%x, T=%x",
			pszLabel,
			SG_uint64_to_sz__hex(statusFlags, buf),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__L__MASK )>>SGWC__L_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__XU__MASK)>>SGWC__XU_OFFSET),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__XR__MASK)>>SGWC__XR_OFFSET),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__X__MASK )>>SGWC__X_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__M__MASK )>>SGWC__M_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__C__MASK )>>SGWC__C_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__S__MASK )>>SGWC__S_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__U__MASK )>>SGWC__U_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__R__MASK )>>SGWC__R_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__A__MASK )>>SGWC__A_OFFSET ),
			(SG_uint32)((statusFlags & SG_WC_STATUS_FLAGS__T__MASK )>>SGWC__T_OFFSET ))  );

	*ppString = pString;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

void SG_wc_debug__status__dump_flags_to_console(SG_context * pCtx,
												SG_wc_status_flags statusFlags,
												const char * pszLabel)
{
	SG_string * pString = NULL;

	SG_ERR_CHECK(  SG_wc_debug__status__dump_flags(pCtx, statusFlags, pszLabel, &pString)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s\n", SG_string__sz(pString))  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}
#endif
