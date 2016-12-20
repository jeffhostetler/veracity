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
 * @file sg_wc_tx__remove.c
 *
 * @details Handle details of 'vv remove'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Queue diving REMOVE of an item when given a repo-path.
 * This is a top-level-requested-item from the user. (As
 * opposed to a recursive call inside the recursive remove
 * routine.)
 *
 * One reason for distinguishing between user-requested-top-level
 * items and recursive items is that if there is an ambiguous
 * repo-path (e.g. a lost file and a found directory with the
 * same name) and they explicitly typed that name, we want to
 * throw the message ambiguous error message and stop (as a
 * form of data validation); if we run into an ambiguous name
 * while iterating on the contents of a directory, we might
 * be able to handle that without complaining.
 *
 * 
 * Try to QUEUE a REMOVE of this item.  If this item is a directory
 * that means we dive and try to remove everything within it first.
 *
 *
 * Things the WC re-write of PendingTree allows that the original
 * PendingTree did not allow and/or things we want to support:
 * 
 * [W8935] delete of a moved and/or renamed item.
 * 
 * [W1717] bKeepInWC says to remove item from version control, but don't
 *    remove it from the disk.
 *    
 * [W3595] delete of an item with ADDED status behaves like an 'UNDO ADD'
 *    and just cancels the ADD and does not remove the item from disk;
 *    item will return to having FOUND status afterwards.
 *
 * [W3595] delete with --force of an ADDED item *DOES* remove it from
 *    the disk.
 *    
 * [] delete of sparse items should always succeed.
 * [] delete of an otherwise unchanged directory containing
 *    IGNORABLE items should succeed.  (don't let the existence of
 *    ignorable items stop us.)  And yes, we quietly delete the
 *    ignorable items too.
 * [] however, a delete of a directory with FOUND items should
 *    require force.  And then we delete both the directory and
 *    the found items.
 * [] when removing a FILE, we require it to have unmodified content
 *    unless bForce.  we backup files to a special area in our temp
 *    directory rather than making ~files in the WD (because we may
 *    also be deleting the parent directory and we'd lose the backup).
 * [] we don't make any backups (regardless of the need) when bNoBackups.
 * [] we never backup symlinks.
 * [] we don't care about changed ATTRBITS when considering
 *    an item dirty.
 * [] if a directory to be deleted already contains deleted items
 *    within it, don't complain about it.
 * [] if we are asked to delete something that is already deleted,
 *    silently allow it.  So 'vv remove foo foo' would work.
 * [] if we are asked to delete something that is LOST, silently
 *    allow it (as an after-the-fact delete).
 * [] if a directory is dirty because things were moved out,
 *    disregard and do the delete.
 * [] if we are asked to delete an item that merge created (something
 *    that was added in the other branch), then we can safely delete
 *    it without backup and without 'undo add' semantics -- unless
 *    they have modified it after the merge.
 * [] if a file is dirty because of an auto-merge, we can delete it
 *    without backup (because the merge result was auto-generated).
 * [] if a file is dirty because of an non-auto merge/resolve, we
 *    respect the dirt like we do for regular files and either backup
 *    or require force.
 *    
 *    
 *
 * 
 * NOTE: We are in a TX model and this is the TOP of a user-requested
 * delete.  They may have given a list of items to be deleted using
 * one of the __remove__n__ versions, but we don't care about that.
 * We will try to delete (queue) one TOP-LEVEL-REQUESTED-ITEM and that's
 * it.
 *
 * So if they say:
 *       'vv remove foo/bar foo/'
 * or    'vv remove foo/ foo/bar'
 * the calling layers will cause both of these to happen in one TX.
 * And we will handle each top-level-item as they were entered; we
 * DO NOT attempt to sort the list or check for duplicates or any
 * of that stuff.
 *
 * In all cases, we should get the same results as if they said:
 *       'vv remove foo/bar'
 * and   'vv remove foo'
 * (or vice versa) in 2 separate TXs.
 *
 */
void SG_wc_tx__remove(SG_context * pCtx,
					  SG_wc_tx * pWcTx,
					  const char * pszInput,
					  SG_bool bNonRecursive,
					  SG_bool bForce,
					  SG_bool bNoBackups,
					  SG_bool bKeep)
{
	SG_ERR_CHECK_RETURN(  SG_wc_tx__remove2(pCtx, pWcTx, pszInput,
											bNonRecursive, bForce, bNoBackups, bKeep,
											SG_FALSE)  );
}

/**
 * I needed to add an argument to the public SG_wc_tx__remove() routine.
 * This version probably shouldn't be public -- you shouldn't ever need
 * to set 'bFlattenAddSpecialPlusRemove'.
 *
 * Normally when the user does a 'vv remove' on something that was added-by-merge
 * or added-by-update, we record it as an "Add* + Remove" so that STATUS can still
 * report it.  However, we don't want that treatment when you do a REVERT-ALL; it
 * just needs to disappear.
 *
 */
void SG_wc_tx__remove2(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   const char * pszInput,
					   SG_bool bNonRecursive,
					   SG_bool bForce,
					   SG_bool bNoBackups,
					   SG_bool bKeep,
					   SG_bool bFlattenAddSpecialPlusRemove)
{
	SG_string * pStringRepoPath = NULL;
	sg_wc_liveview_item * pLVI;			// we do not own this
	SG_bool bKnown;
	SG_bool bKept = SG_FALSE;
	char chDomain;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszInput );	// throw if omitted, do not assume "@/".

	SG_ERR_CHECK(  sg_wc_db__path__anything_to_repopath(pCtx, pWcTx->pDb, pszInput,
														SG_WC_DB__PATH__IMPORT_FLAGS__TREAT_NULL_AS_ERROR,
														&pStringRepoPath, &chDomain)  );
#if TRACE_WC_TX_REMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__remove: '%s' normalized to [domain %c] '%s'\n",
							   pszInput, chDomain, SG_string__sz(pStringRepoPath))  );
#endif

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

	if (pLVI == pWcTx->pLiveViewItem_Root)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot REMOVE the root directory '%s'.",
						 SG_string__sz(pStringRepoPath))  );

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		// if they say 'vv remove .sgdrawer' we need to complain.
		// however, if (on Windows) they say 'vv remove *' (and
		// the globbing on Windows includes .dotfiles), we should
		// silently ignore it -- but we can't tell that here.
		// So I'm going to silently eat it.  See R7215.
		// SG_ERR_THROW2(  SG_ERR_WC_RESERVED_ENTRYNAME,
		//				(pCtx, "Cannot remove reserved '%s'.",
		//				 SG_string__sz(pStringRepoPath))  );
		goto done;
	}
	
	if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		// Silently disregard a FOUND or IGNORED item at top-level.
		// This lets them type 'vv remove *' and us not choke on
		// trash in the directory.  (We treat found/ignored stuff
		// differently when we encounter it during a dive (since
		// it might cause us to throw an unsafe-delete on the
		// parent directory).)

		goto done;
	}

	SG_ERR_CHECK(  sg_wc_tx__rp__remove__lvi(pCtx, pWcTx,
											 pLVI, 0,
											 bNonRecursive, bForce, bNoBackups, bKeep,
											 bFlattenAddSpecialPlusRemove,
											 &bKept)  );

	if (bKeep && bKept)
	{
		// A 'vv remove --keep <item>'
		// If they request that we KEEP the item on the REMOVE,
		// we basically just remove it from version control
		// and do not try to remove it from the disk.
		//
		// If we *actually* left the item in the directory
		// (not sparse, for example), then the item needs to
		// appear as FOUND going forward.

		if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
		{
			// the REMOVE got mapped to an UNDO-ADD and the
			// existing LVI got converted to a FOUND item.
			// so all is well.
			SG_ASSERT(  (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__INVALID)  );
		}
		else
		{
			SG_ASSERT(  SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI->scan_flags_Live)  );
			SG_ASSERT(  (pLVI->pPcRow_PC->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__DELETED)  );

			// Synthesize a new LVI with FOUND status and
			// with a *NEW* *TEMP* GID for this item.
			//
			// We *ONLY* do this for top-level items named
			// on the REMOVE.  If this is a directory, the
			// (now uncontrolled) items contained within it
			// will become uncontrolled/unscanned children
			// of a new uncontrolled directory (and we want
			// to defer the scan of the children until we
			// actually need it in the cache).

			SG_ERR_CHECK(  sg_wc_liveview_item__synthesize_replacement_row(pCtx, NULL, pWcTx, pLVI)  );
		}
	}

done:
	;
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__remove__stringarray(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_stringarray * psaInputs,
								   SG_bool bNonRecursive,
								   SG_bool bForce,
								   SG_bool bNoBackups,
								   SG_bool bKeep)
{
	SG_uint32 k, count;

	SG_NULLARGCHECK_RETURN( pWcTx );

	if (psaInputs)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pWcTx, pszInput_k,
											bNonRecursive, bForce, bNoBackups, bKeep)  );
		}
	}
	else
	{
		// if no args, we DO NOT assume "@/" for a REMOVE.
		// (caller should have thrown USAGE if appropriate.)

		SG_NULLARGCHECK_RETURN( psaInputs );
	}

fail:
	return;
}
