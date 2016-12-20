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
 * @file sg_wc_tx__resolve__accept__inline2.h
 *
 * @details SG_resolve__accept related routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__ACCEPT__INLINE2_H
#define H_SG_WC_TX__RESOLVE__ACCEPT__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__accept_value(
	SG_context*        pCtx,
	SG_resolve__value* pValue
	)
{
	SG_resolve__choice* pChoice       = pValue->pChoice;
	SG_resolve__item*   pItem         = pChoice->pItem;
	SG_string*          pStringRepoPath = NULL;
	SG_string*          pStringName   = NULL;
	SG_wc_status_flags flags;
	SG_bool bNoIgnores = SG_TRUE;	// don't bother classifying
	SG_bool bNoTSC     = SG_FALSE;	// trust TSC; don't re-hash file
	SG_bool bRevertTempName = SG_FALSE;

	SG_NULLARGCHECK(pValue);

	// I'm going to bypass the full level-7 api to get the status flags
	// because we already have the LVI in hand.
	// BTW, for this we need the full flags, not just the x_xr_xu flags.
	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pItem->pResolve->pWcTx, pItem->pLVI,
												bNoIgnores, bNoTSC, &flags)  );

#if TRACE_WC_RESOLVE
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "AcceptValue: [label %s] [mnemonic %s] [flags %s]: %s\n",
								   pValue->szLabel,
								   ((pValue->pszMnemonic) ? pValue->pszMnemonic : "(null)"),
								   SG_uint64_to_sz__hex((SG_uint64)flags,bufui64),
								   SG_string__sz(pItem->pStringGidDomainRepoPath))  );
	}
#endif

	// check that the value can actually be used as a resolution
	if (pValue->pVariant == NULL && pValue->pVariantFile == NULL)
	{
		// this value is the result of a failed merge
		SG_ASSERT(pValue->pBaselineParent != NULL && pValue->pOtherParent != NULL);
		SG_ASSERT(pValue->iMergeToolResult != SG_FILETOOL__RESULT__SUCCESS);
		SG_ERR_THROW2(SG_ERR_RESOLVE_INVALID_VALUE,
					  (pCtx, "Cannot accept a value that resulted from a failed merge: %s",
					   pValue->szLabel));
	}
	else if (
		(flags & SG_WC_STATUS_FLAGS__S__DELETED)
		&& pChoice->eState != SG_RESOLVE__STATE__EXISTENCE
		&& pValue->pVariant->v.val_bool != SG_FALSE
		)
	{
		// MERGE/UPDATE *DID NOT* mark this as an existence conflict
		// (such as a delete-vs-* problem) and the user did a 'vv remove'
		// on it.  (The delete code does an implicit mark-resolved on it,
		// but that doesn't matter.)  NOW they want RESOLVE to pick one
		// of the proper choices -- WHICH MUST IMPLICITLY RESTORE IT IF
		// POSSIBLE.  (For file content conflicts, we should still have
		// the various versions in tmp.)
		//
		// See W0901 and W3903.

		goto label_1;
	}
	else if (
		(flags & SG_WC_STATUS_FLAGS__U__LOST)
		&& pChoice->eState != SG_RESOLVE__STATE__EXISTENCE
		&& pValue->pVariant->v.val_bool != SG_FALSE
		)
	{
		// can't accept a value on an item that doesn't currently exist
		// unless it's an EXISTENCE choice and we're accepting a FALSE value.
		// 
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
					  (pCtx, "AcceptValue: Cannot accept a value on an item that has been lost: %s",
					   SG_string__sz(pItem->pStringGidDomainRepoPath)));
	}
	else if (pChoice->pMergeableResult == pValue)
	{
		// the choice has already been resolved with this value
		SG_ERR_THROW2(SG_ERR_RESOLVE_ALREADY_RESOLVED,
					  (pCtx, "The conflict has already been resolved using the given value."));
	}

	//////////////////////////////////////////////////////////////////

	// NOTE 2012/08/29 This whole section is only needed to make a
	// NOTE            a backup of an edited file in the drawer before
	// NOTE            we overwrite it with a different version.  The
	// NOTE            original code did not guard this section and
	// NOTE            relied on the various predicates in the
	// NOTE            __check_value__ call to never return anything
	// NOTE            for non-mergeable choices.  I'm adding the guard
	// NOTE            here to make that explicit.

	if ((pChoice->eState == SG_RESOLVE__STATE__CONTENTS)
		&& (pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE))
	{
		SG_resolve__value* pWorking        = NULL;

		// before making any changes, check if we need to save the live working value
		// we'll do that if:
		// 1) this is a mergeable choice
		// 2) the working value has edits
		// 3) the working value isn't the one being accepted
		// that way the user won't lose their edits when we overwrite the working value
		SG_ERR_CHECK(  SG_resolve__choice__check_value__changeset__mnemonic(pCtx, pChoice, gszMnemonic_Working, &pWorking)  );
		if (
			   pWorking != NULL
			&& pWorking != pValue
			&& pWorking->pBaselineParent != NULL
			)
		{
			SG_bool bEqual = SG_FALSE;

			// mergeable choices should always have string data
			// more specifically, they should have HID data
			SG_ASSERT(pWorking->pVariant->type == SG_VARIANT_TYPE_SZ);
			SG_ASSERT(pWorking->pBaselineParent->pVariant->type == SG_VARIANT_TYPE_SZ);

			// check if the working value's HID is equal to its parent's
			// if it's not, it has edits that we need to save
			//
			// Note: we have to keep the CONTENT/FILE HID in {variant}
			// for the following to work (no matter what we do with {variantFile}.
			SG_ERR_CHECK(  _value__equal_variants(pCtx, pWorking, pWorking->pBaselineParent, &bEqual)  );
			if (bEqual == SG_FALSE)
			{
				SG_ERR_CHECK(  _value__working_mergeable__save(pCtx, pWorking)  );
			}
		}
	}

	//////////////////////////////////////////////////////////////////

label_1:
	// the action we need to take on the file depends on what type of choice this is
	if (pChoice->eState == SG_RESOLVE__STATE__EXISTENCE)
	{
		// accept the existence value on the item's file
		SG_ERR_CHECK(  _accept__existence(pCtx, pItem, pValue, flags, pValue->pVariant->v.val_bool)  );
		if (pValue->pVariant->v.val_bool == SG_TRUE)
		{
			// if the accepted value is existence,
			// we need to revert the file's temporary name
			// (that MERGE gave it when it set the DELETE-VS-* bit).
			bRevertTempName = SG_TRUE;
		}
	}
	else
	{
		// for any non-EXISTENCE case, make sure the item exists before acting
		SG_ERR_CHECK(  _accept__existence(pCtx, pItem, pValue, flags, SG_TRUE)  );

		// make appropriate changes to the item based on the accepted value
		switch (pChoice->eState)
		{
		case SG_RESOLVE__STATE__NAME:
			{
				SG_ERR_CHECK(  _accept__name(pCtx, pItem, pValue->pVariant->v.val_sz)  );
				break;
			}
		case SG_RESOLVE__STATE__LOCATION:
			{
				// With the WC version we now pass the GID of the destination
				// parent directory (rather than the repo-path).
				SG_ERR_CHECK(  _accept__location(pCtx, pItem, pValue->pVariant->v.val_sz)  );
				bRevertTempName = SG_TRUE;
				break;
			}
		case SG_RESOLVE__STATE__ATTRIBUTES:
			{
				SG_ERR_CHECK(  _accept__attributes(pCtx, pItem, pValue->pVariant->v.val_int64)  );
				break;
			}
		case SG_RESOLVE__STATE__CONTENTS:
			{
				switch (pItem->eType)
				{
				case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
					{
						SG_ERR_CHECK(  _accept__file(pCtx, pItem, pValue)  );
						break;
					}
				case SG_TREENODEENTRY_TYPE_DIRECTORY:
					{
						SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED,
									  (pCtx,
									   "Cannot resolve a directory contents conflict because they should never occur."));
						break;
					}
				case SG_TREENODEENTRY_TYPE_SYMLINK:
					{
						SG_ERR_CHECK(  _accept__symlink(pCtx, pItem, pValue->pVariant->v.val_sz)  );
						break;
					}
				case SG_TREENODEENTRY_TYPE_SUBMODULE:
					{
						SG_ERR_CHECK(  _accept__submodule(pCtx, pItem, pValue->pVariant->v.val_sz)  );
						break;
					}
				default:
					{
						SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown node type: %i", pItem->eType));
						break;
					}
				}
				break;
			}
		default:
			{
				SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown resolve item state: %i", pChoice->eState));
				break;
			}
		}
	}

	if (pItem->szRestoreOriginalName && bRevertTempName)
	{
		// MERGE gave the item a temporary name "<entryname>~<gid7>" for various
		// types of conflicts (and this set may change (See W4887)).
		//
		// With the changes for W5589, MERGE gives us the "restore_original_name"
		// field when MERGE gave the item a temporary name *and* it is necessary
		// for RESOLVE to silently/implicitly restore it after dealing with the
		// ISSUE.  With W5589 (and the potential changes for W4487) I made RESOLVE
		// completely dependent on the presence of this issue field rather than
		// having RESOLVE infer the field by looking at the issue.input.* fields.
		//
		//
		// There are a few instances when we shouldn't do this.

		// Note 2012/06/07 When there are name conflicts, we require that the user
		// Note            choose the new name.  MERGE knows this and won't set
		// Note            the "restore_original_name" field for DIVERGENT-RENAME
		// Note            conflicts, so this case could be an ASSERT.
		// 
		// If there's a NAME choice on the same item then we never want to revert the temp name.
		// We'll let the user choose the name (or stick with what they already chose).
		if (pItem->aChoices[SG_RESOLVE__STATE__NAME] != NULL)
		{
			bRevertTempName = SG_FALSE;
		}
		else
		{
			// If the item's name is different from the TEMP name that MERGE gave it,
			// then the user has renamed it manually since the merge, and we shouldn't touch it.
			//
			// Compute the CURRENT repo-path for this item.  This value reflects the
			// current IN-TX value for it.
			// 
			// NOTE 2012/04/02 We DO NOT want to use stricmp() because case is significant.

			SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pItem->pResolve->pWcTx, pItem->pLVI,
																	  &pStringRepoPath)  );
			SG_ERR_CHECK(  SG_repopath__get_last(pCtx, pStringRepoPath, &pStringName)  );

			if (strcmp(SG_string__sz(pStringName), pItem->szMergedName) != 0)
			{
				bRevertTempName = SG_FALSE;
			}
			else
			{
				// Rename the item back to its original name.

				// TODO 2012/04/02 Something about this bothers me.
				// TODO            I think it is possible for this to fail
				// TODO            if they create post-merge dirt that would
				// TODO            collide with this name.  Yes, this is
				// TODO            really contrived.

				SG_ERR_CHECK(  _accept__name(pCtx, pItem, pItem->szRestoreOriginalName)  );
			}
		}
	}

	// NOTE 2012/08/29 With the changes for x_xr_xu (W1612 and W1849)
	// NOTE            the above "_accept__" routines will immediately
	// NOTE            update the tbl_issue with the new x_xr_xu bits.
	// NOTE            (For example, the underlying call to move/rename
	// NOTE            records the new XR/XU bits during the QUEUE step.)
	// NOTE
	// NOTE            So WE DO NOT NEED to do a SAVE on the data in
	// NOTE            pResolve to get the x_xr_xu bits written to the DB.
	// NOTE
	// NOTE            However, if we changed the contents of a file, we
	// NOTE            still want to record that data.

	if ((pChoice->eState == SG_RESOLVE__STATE__CONTENTS)
		&& (pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE))
	{
		pChoice->pMergeableResult = pValue;
		SG_ERR_CHECK(  _item__save_mergeable_data(pCtx, pItem)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringName);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__ACCEPT__INLINE2_H
