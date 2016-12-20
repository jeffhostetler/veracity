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
 * @file sg_wc_tx__resolve__accept__inline1.h
 *
 * @details Accept-related internal functions that I found in
 * the PendingTree version of sg_resolve.c.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__ACCEPT__INLINE1_H
#define H_SG_WC_TX__RESOLVE__ACCEPT__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Re-create the item on disk.
 *
 * Use the details in pValue to help us choose which version
 * of the content to restore.
 *
 */ 
static void _accept__existence__undo_delete(SG_context * pCtx,
											SG_resolve__item * pItem,
											SG_resolve__value * pValue)
{
	// The easiest way to get here is to have a delete-vs-* conflict
	// and let resolve pick the delete choice and then try to re-resolve
	// it and pick the non-delete choice (with --overwrite).
	//
	// There are 2 parts to the story here:
	// (1) The item needs to exist (be re-created) and with the same GID as before.
	// (2) For files/symlinks what should we set the content to?
	//
	// Knowing what to put in the file/symlink is a bit fuzzy.
	// 
	// If they modified the file before they deleted it, do they want the
	// last dirty version (that they most recently deleted as it was at the
	// time of the delete) or do they want the reference version from cset (or
	// one of the alternate named versions, such as "working3" or "merge4") ?
	// 
	// We have to rely on them giving us a LABEL (such as "accept baseline")
	// and trust that that is the version of the content that they want.
	//
	// See W0901, W0960, W3903.

	SG_wc_undo_delete_args args;
	SG_wc_status_flags xu_mask = SG_WC_STATUS_FLAGS__ZERO;

	memset(&args, 0, sizeof(args));

	if (pValue->pszMnemonic)
	{
		// This value refers to a parent (reference) cset.

		SG_vhash * pvhInputs;
		SG_vhash * pvhInput_k;

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pItem->pLVI->pvhIssue, "input", &pvhInputs)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhInputs, pValue->pszMnemonic, &pvhInput_k)  );

		SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvhInput_k, "attrbits", (SG_uint64 *)&args.attrbits)  );
		if (pItem->pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhInput_k, "hid", &args.pszHidBlob)  );
		args.pStringRepoPathTempFile = NULL;

		xu_mask |= (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
		// TODO 2012/08/27 Not sure if I should set anything other than existence.
		xu_mask |= (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__CONTENTS);
		xu_mask |= (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES);
	}
	else if (pValue->pVariantFile)
	{
		// We have a value like "merge3" or "working2" -- get the TMP file if present.
		// See W3903.
		// 
		// Use the temp file that an earlier 'vv resolve merge' command created.
		// I'm not going to bother looking up the attrbits on the temp file
		// because they may not be right and our caller will probably set them
		// in a minute.  Just use the original values.

		SG_ERR_CHECK(  sg_wc_liveview_item__get_original_attrbits(pCtx, pItem->pLVI, pItem->pResolve->pWcTx, (SG_uint64 *)&args.attrbits)  );
		args.pStringRepoPathTempFile = pValue->pVariantFile;
		args.pszHidBlob = NULL;

		xu_mask |= (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE);
		// TODO 2012/08/27 Not sure if I should set anything other than existence.
		xu_mask |= (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__CONTENTS);
	}
	else
	{
		// TODO 2012/06/13 Is this still possible with the fix for W3903?
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "ResolveAcceptUndoDelete: no mnemonic, need to use label: %s",
						 pValue->szLabel)  );
	}
	
	SG_ERR_CHECK(  SG_wc_tx__undo_delete(pCtx, pItem->pResolve->pWcTx, SG_string__sz(pItem->pStringGidDomainRepoPath),
										 NULL, NULL,	// let it take care of *where* to put the item.
										 &args,
										 xu_mask)  );

	// Undo-delete should implicitly resolve the existence question (if set).
	// But it should not necessarily mark everything resolved unless
	// "existence" was the only choice.
	SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE) == 0)  );
	if ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK) == 0)
		SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED) != 0)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Sets the existence of a file to a given value.
 *
 * NOTE 2012/05/31 __accept__existence() gets called for both SG_RESOLVE__STATE__EXISTENCE
 * NOTE            type conflicts (where this is the only thing we need to do) *AND* for other
 * NOTE            types of conflicts (where we need to make sure that the item is present *before*
 * NOTE            we do the actual move/rename/content/etc).
 * NOTE
 * NOTE            We take a bValue to indicate whether the item should exist.
 * NOTE
 * NOTE            We take pValue *ONLY* to get the {szLabel,pszMnemonic} to tell us what version
 * NOTE            of the item to create -- iff we need to re-create it.
 * 
 */
static void _accept__existence(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	SG_resolve__item*  pItem,        //< [in] The item that we're accepting a value on.
	SG_resolve__value* pValue,       //< [in] WE ONLY USE THIS FOR {szLabel,pszMnemonic} WHEN WE HAVE TO RESTORE IT.
	SG_wc_status_flags flags,        //< [in] The complete set; current as of the beginning of the ACCEPT.
	SG_bool            bValue        //< [in] True if the file should exist, false if it should not.
	)
{
	SG_NULLARGCHECK( pItem );
	SG_NULLARGCHECK( pValue );

	// since we get called for more than just EXISTENCE choices,
	// we cannot assert __XU__EXISTENCE | __XR__EXISTENCE here.

#if TRACE_WC_RESOLVE
	{
		SG_int_to_string_buffer bufui64_a;
		SG_int_to_string_buffer bufui64_b;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "AcceptExistence: [bValue %d][flags %s][x_xr_xu %s] [label %s][mnemonic %s]: %s\n",
								   bValue,
								   SG_uint64_to_sz__hex((SG_uint64)flags,  bufui64_a),
								   SG_uint64_to_sz__hex((SG_uint64)pItem->pLVI->statusFlags_x_xr_xu,bufui64_b),
								   pValue->szLabel,
								   ((pValue->pszMnemonic) ? pValue->pszMnemonic : "(null)"),
								   SG_string__sz(pItem->pStringGidDomainRepoPath))  );
	}
#endif

	if (bValue == SG_FALSE)
	{
		// The item SHOULD NOT exist when we are done.
		// It may or may not exist now.

		SG_bool bForce     = SG_TRUE;
		SG_bool bNoBackups = SG_TRUE;
		SG_bool bKeep      = SG_FALSE;

		// This will blindly try to delete it (and
		// silently NOT COMPLAIN if it doesn't exist).
		// that is, we don't have to test the status
		// flags first.
		//
		// The PendingTree version of the code needed
		// to test for and deal with
		// SG_ERR_CANNOT_REMOVE_SAFELY because PendingTree
		// did not allow you to REMOVE something that already
		// had a pending MOVE or RENAME.  WC no longer has
		// that restriction.
		//
		// TODO 2012/06/12 See W9238 on whether we should always assume --force --no-backups
		// TODO            on this subordinate delete.
		//
		// TODO 2012/05/24 Now that we have pItem->pLVI, consider
		// TODO            using lower level routine rather than
		// TODO            using level-7 routine.
		// 
		// TODO 2012/08/07 I just added a non-recursive arg to SG_wc_tx__remove().
		// TODO            In theory, we should not need to request a recursive
		// TODO            of this item, since we should have already taken
		// TODO            care of all of the details.  But I don't have time to
		// TODO            test this right now.  So I'm setting the flag to match
		// TODO            the behavior of the version of the routine prior to the
		// TODO            new arg.
		SG_bool bNonRecursive = SG_FALSE;

		SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pItem->pResolve->pWcTx,
										SG_string__sz(pItem->pStringGidDomainRepoPath),
										bNonRecursive, bForce, bNoBackups, bKeep)  );

		// REMOVE should implicitly resolve the existence question (if set)
		// *AND* REMOVE should implicilty resolve the entire issue.

		SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE) == 0)  );
		SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)   != 0)  );
		
	}
	else /*bValue==SG_TRUE*/
	{
		// The item SHOULD exist when we are done.
		// It may or may not exist now.

		if (flags & SG_WC_STATUS_FLAGS__S__DELETED)
		{
			SG_ERR_CHECK(  _accept__existence__undo_delete(pCtx, pItem, pValue)  );
		}
		else if (flags & SG_WC_STATUS_FLAGS__U__LOST)
		{
			// TODO 2012/04/02 The item should be present, but is currently LOST.
			// TODO            I'm not sure it is appropriate to automatically
			// TODO            assume that we should just silently replace it,
			// TODO            since it may be the case they they did a move/rename
			// TODO            and forgot to tell us about it.
			// TODO
			// TODO            So for now, just throw and advise them to figure out
			// TODO            why/how it was lost and fix that before trying to
			// TODO            resolve it.
			SG_ERR_THROW2(SG_ERR_NOT_FOUND,
						  (pCtx,
						   "AcceptExistence: Unable to reconstruct an item that has been LOST since merging: %s",
						   SG_string__sz(pItem->pStringGidDomainRepoPath)));
		}
		else
		{
			// in other cases, the item's current state already matches the given desired state
			// so there's no work to do to the item on disk, but we still need to ensure that the
			// existence-bit is marked resolved.

			SG_wc_status_flags xu_mask = (SG_WC_STATUS_FLAGS__XU__EXISTENCE);
			SG_wc_status_flags xu_all  = (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
			SG_wc_status_flags xu_exc  = (xu_all & ~xu_mask);
			SG_wc_status_flags xu_inc  = (xu_all &  xu_mask);
			SG_wc_status_flags xu2xr   = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
			SG_wc_status_flags xr      = (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
			SG_wc_status_flags sf      = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
										  | xr | xu2xr | xu_exc);
			SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pItem->pResolve->pWcTx, pItem->pLVI, sf)  );

			//
			// TODO 2012/04/02 not sure about SPARSE. See W3729 maybe.
		}
	}

fail:
	return;
}

/**
 * Sets the name of a file to a given value.
 */
static void _accept__name(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	SG_resolve__item*  pItem,        //< [in] The item that we're accepting a value on.
	const char*        szValue       //< [in] The name to rename the file to.
	)
{
	SG_bool bNoAllowAfterTheFact = SG_TRUE;
	SG_bool bNoEffect = SG_FALSE;

	SG_NULLARGCHECK( pItem );
	SG_NONEMPTYCHECK(szValue);

	// Since we get called when pszRestoreOriginalName is
	// set, (and there isn't a NAME conflict), we cannot
	// assert __XU__NAME | __XR__NAME here.

#if TRACE_WC_RESOLVE
	{
		SG_int_to_string_buffer bufui64_b;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "AcceptName: [x_xr_xu %s] [name %s]: %s\n",
								   SG_uint64_to_sz__hex((SG_uint64)pItem->pLVI->statusFlags_x_xr_xu,bufui64_b),
								   szValue,
								   SG_string__sz(pItem->pStringGidDomainRepoPath))  );
	}
#endif

	// Just go ahead and try to do the RENAME.
	// 
	// If the destination is the same as the item's
	// current name, we'll get a _NO_EFFECT that we
	// can ignore.
	//
	// We should get a _NO_EFFECT whenever the working
	// value is chosen.

	SG_ERR_TRY(  SG_wc_tx__rename(pCtx, pItem->pResolve->pWcTx,
								  SG_string__sz(pItem->pStringGidDomainRepoPath),
								  szValue,
								  bNoAllowAfterTheFact)  );
	SG_ERR_CATCH_SET(  SG_ERR_NO_EFFECT, bNoEffect, SG_TRUE  );
	SG_ERR_CATCH_END;

	if (bNoEffect && (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME))
	{
		SG_wc_status_flags xu_mask = (SG_WC_STATUS_FLAGS__XU__EXISTENCE
									  | SG_WC_STATUS_FLAGS__XU__NAME);
		SG_wc_status_flags xu_all  = (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
		SG_wc_status_flags xu_exc  = (xu_all & ~xu_mask);
		SG_wc_status_flags xu_inc  = (xu_all &  xu_mask);
		SG_wc_status_flags xu2xr   = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags xr      = (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags sf      = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
									  | xr | xu2xr | xu_exc);
		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pItem->pResolve->pWcTx, pItem->pLVI, sf)  );
	}

	// RENAME should implicitly resolve the existence+name questions (if set).
	// But RENAME should not necessarily mark everything resolved
	// unless the name was the only choice.
	SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE) == 0)  );
	SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__NAME) == 0)  );
	if ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK) == 0)
		SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)   != 0)  );

fail:
	return;
}

/**
 * Sets the location of a file to a given folder.
 */
static void _accept__location(
	SG_context*        pCtx,         //< [in] [out] Error and context info.
	SG_resolve__item*  pItem,        //< [in] The item that we're accepting a value on.
	const char*        szValue       //< [in] The GID of the destination parent directory.
	                                 //<      This is different from the PendingTree version.
	)
{
	SG_string * pStringNewParent = NULL;
	SG_bool bNoAllowAfterTheFact = SG_TRUE;
	SG_bool bNoEffect = SG_FALSE;

	SG_NULLARGCHECK( pItem );
	SG_NONEMPTYCHECK(szValue);

	SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION)
							  || (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__LOCATION))  );

	// Use the GID of the destination parent directory to
	// create a GID-domain extended-prefix repo-path rather
	// than bothering to compute the actual live-repo-path.
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringNewParent)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringNewParent, "@%s", szValue)  );

	// Just go ahead and try to do the MOVE.
	// 
	// If the destination is the same as the item's
	// current location, we'll get a _NO_EFFECT that we
	// can ignore.
	//
	// We should get a _NO_EFFECT whenever the working
	// value is chosen.

	SG_ERR_TRY(  SG_wc_tx__move(pCtx, pItem->pResolve->pWcTx,
								SG_string__sz(pItem->pStringGidDomainRepoPath),
								SG_string__sz(pStringNewParent),
								bNoAllowAfterTheFact)  );
	SG_ERR_CATCH_SET(  SG_ERR_NO_EFFECT, bNoEffect, SG_TRUE  );
	SG_ERR_CATCH_END;

	if (bNoEffect && (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION))
	{
		SG_wc_status_flags xu_mask = (SG_WC_STATUS_FLAGS__XU__EXISTENCE
									  | SG_WC_STATUS_FLAGS__XU__LOCATION);
		SG_wc_status_flags xu_all  = (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK);
		SG_wc_status_flags xu_exc  = (xu_all & ~xu_mask);
		SG_wc_status_flags xu_inc  = (xu_all &  xu_mask);
		SG_wc_status_flags xu2xr   = ((xu_inc >> SGWC__XU_OFFSET) << SGWC__XR_OFFSET);
		SG_wc_status_flags xr      = (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK);
		SG_wc_status_flags sf      = (((xu_exc) ? SG_WC_STATUS_FLAGS__X__UNRESOLVED : SG_WC_STATUS_FLAGS__X__RESOLVED)
									  | xr | xu2xr | xu_exc);
		SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__s(pCtx, pItem->pResolve->pWcTx, pItem->pLVI, sf)  );
	}

	// MOVE should implicitly resolve the existence+location questions (if set).
	// But MOVE should not necessarily mark everything resolved
	// unless the location was the only choice.
	SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__EXISTENCE) == 0)  );
	SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__LOCATION) == 0)  );
	if ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK) == 0)
		SG_ASSERT_RELEASE_FAIL(  ((pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)   != 0)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringNewParent);
}

/**
 * Sets the attribute bits of a file to a given value.
 */
static void _accept__attributes(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_resolve__item*  pItem, //< [in] The item that we're accepting a value on.
	SG_int64           iValue //< [in] The attribute bits to set on the file.
	)
{
	SG_NULLARGCHECK(pItem);

	// We don't need to worry about _NO_EFFECT because
	// the WC code doesn't bother to check/throw it for
	// attrbits.

	SG_ERR_CHECK(  SG_wc_tx__set_attrbits(pCtx, pItem->pResolve->pWcTx,
										  SG_string__sz(pItem->pStringGidDomainRepoPath),
										  iValue,
										  SG_WC_STATUS_FLAGS__XU__ATTRIBUTES)  );

fail:
	return;
}

/**
 * Sets the contents of a file to the contents of another given file.
 */
static void _accept__file(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_resolve__item*  pItem, //< [in] The item that we're accepting a value on.
	SG_resolve__value* pValue //< [in] Get pValue->pVariantFile for file contents to be copied.
	)
{
	SG_pathname * pPathTemp = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pValue);

	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pValue, &pPathTemp)  );

	// We don't need to worry about _NO_EFFECT because
	// the WC code doesn't bother to check/throw it for
	// file content.
	//
	// We ASSUME that this will do an in-place copy such that
	// the ATTRBITS on the existing/controlled file
	// are preserved.
	//
	// We also ASSUME that this will properly detect when the
	// source and destination paths refer to the same actual
	// file and do the right thing.
	
	SG_ERR_CHECK(  SG_wc_tx__overwrite_file_from_file2(pCtx, pItem->pResolve->pWcTx,
													   SG_string__sz(pItem->pStringGidDomainRepoPath),
													   pPathTemp,
													   SG_WC_STATUS_FLAGS__XU__CONTENTS)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTemp);
}

/**
 * Sets the target of a symlink to a given target.
 */
static void _accept__symlink(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__item*  pItem,  //< [in] The item that we're accepting a value on.
	const char*        szValue //< [in] The target to set the symlink to.
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NONEMPTYCHECK(szValue);

	// We don't need to worry about _NO_EFFECT because
	// the WC code doesn't bother to check/throw it when
	// setting the target to the same value it has now.
	//
	// We ASSUME that this will set the target in-place such that
	// the ATTRBITS on the existing/controlled symlink
	// are preserved (or it will give us that illusion).

	SG_ERR_CHECK(  SG_wc_tx__overwrite_symlink_target(pCtx, pItem->pResolve->pWcTx,
													  SG_string__sz(pItem->pStringGidDomainRepoPath),
													  szValue,
													  SG_WC_STATUS_FLAGS__XU__CONTENTS)  );

fail:
	return;
}

/**
 * Sets the target of a submodule to a given target.
 */
static void _accept__submodule(
	SG_context*        pCtx,    //< [in] [out] Error and context info.
	SG_resolve__item*  pItem,   //< [in] The item that we're accepting a value on.
	const char*        szValue  //< [in] The HID of a submodule blob to accept.
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NONEMPTYCHECK(szValue);

	// TODO 2012/04/02 Actually do something here.

	SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
					(pCtx, "AcceptSubmodule: TODO [szValue %s]: on %s.",
					 szValue,
					 SG_string__sz(pItem->pStringGidDomainRepoPath))  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__ACCEPT__INLINE1_H
