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
 * @file sg_wc_tx__resolve__value__inline1.h
 *
 * @details _value__ routines that I pulled from the PendingTree
 * version of sg_resolve.c.  These were considered STATIC PRIVATE
 * to sg_resolve.c, but I want to split sg_resolve.c into individual
 * classes and these need to be changed to be not static and have
 * proper private prototypes defined for them.  I didn't take time
 * to do that right now.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__VALUE__INLINE1_H
#define H_SG_WC_TX__RESOLVE__VALUE__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Frees a value.
 */
static void _value__free(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_resolve__value* pValue //< [in] The value to free.
	)
{
	if (pValue == NULL)
	{
		return;
	}

	SG_NULLFREE(pCtx, pValue->szLabel);
	SG_VARIANT_NULLFREE(pCtx, pValue->pVariant);
	SG_STRING_NULLFREE(pCtx, pValue->pVariantFile);
	SG_NULLFREE(pCtx, pValue->szVariantSummary);
	SG_NULLFREE(pCtx, pValue->szMergeTool);
	pValue->pChoice          = NULL;
	pValue->uChildren        = 0u;
	SG_NULLFREE(pCtx, pValue->pszMnemonic);
	pValue->pBaselineParent  = NULL;
	pValue->pOtherParent     = NULL;
	pValue->bAutoMerge       = SG_FALSE;
	pValue->iMergeToolResult = 0;
	pValue->iMergeTime       = 0;
	SG_NULLFREE(pCtx, pValue);
}

/**
 * NULLFREE wrapper around _value__free.
 */
#define _VALUE__NULLFREE(pCtx, pValue) _sg_generic_nullfree(pCtx, pValue, _value__free)

/**
 * Compares two values for equality of their variant data.
 */
static void _value__equal_variants(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	SG_resolve__value* pLeft,  //< [in] Left-hand value to compare.
	SG_resolve__value* pRight, //< [in] Right-hand value to compare.
	SG_bool*           pEqual  //< [out] True if the values have equivalent variant data, false otherwise.
	)
{
	SG_bool bEqual = SG_TRUE;

	SG_NULLARGCHECK(pLeft);
	SG_NULLARGCHECK(pRight);
	SG_NULLARGCHECK(pEqual);

	if (
		   (pLeft->pVariant == NULL || pRight->pVariant == NULL)
		&& (pLeft->pVariant != pRight->pVariant)
		)
	{
		// one variant is NULL and the other isn't
		bEqual = SG_FALSE;
	}
	else
	{
		// both variants are non-NULL, compare them
		SG_ERR_CHECK(  SG_variant__equal(pCtx, pLeft->pVariant, pRight->pVariant, &bEqual)  );
	}

	// return whatever we ended up with
	*pEqual = bEqual;

fail:
	return;
}

/**
 * Allocates a blank value structure.
 */
static void _value__alloc__blank(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice that will own the new value.
	SG_resolve__value** ppValue  //< [out] The allocated value.
	)
{
	SG_resolve__value* pValue = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppValue);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pValue)  );

	pValue->pChoice          = pChoice;
	// TODO 2012/04/03 BTW, the above SG_alloc1() zeroes the
	// TODO            allocated memory, so the following
	// TODO            isn't necessary.
	pValue->szLabel          = NULL;
	pValue->pVariant         = NULL;
	pValue->pVariantFile     = NULL;
	pValue->szVariantSummary = NULL;
	pValue->uVariantSize     = 0u;
	pValue->uChildren        = 0u;
	pValue->pszMnemonic      = NULL;
	pValue->pBaselineParent  = NULL;
	pValue->pOtherParent     = NULL;
	pValue->bAutoMerge       = SG_FALSE;
	pValue->szMergeTool      = NULL;
	pValue->iMergeToolResult = 0;
	pValue->iMergeTime       = 0;

	*ppValue = pValue;
	pValue = NULL;

fail:
	_VALUE__NULLFREE(pCtx, pValue);
}

/**
 * Allocates a value structure that is associated with a changeset.
 */
static void _value__alloc__changeset__mnemonic(
	SG_context*         pCtx,           //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,        //< [in] The choice that will own the value.
	const char*         szLabel,        //< [in] The label to assign to the value.
	                                    //<      The value will create its own copy of this.
	const SG_vhash*     pInputs,        //< [in] The inputs data from the merge issue to look up variant values in.
	const char*         pszMnemonic,    //< [in] The mnemonic of the changeset to associate with the value.
	SG_resolve__value** ppValue         //< [out] The allocated value.
	)
{
	SG_resolve__value* pValue = NULL;
	SG_string * pStringParentRepoPath = NULL;
	SG_pathname * pPath = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(szLabel);
	SG_NULLARGCHECK(pInputs);
	SG_NULLARGCHECK(pszMnemonic);
	SG_NULLARGCHECK(ppValue);

	// allocate a blank value
	SG_ERR_CHECK(  _value__alloc__blank(pCtx, pChoice, &pValue)  );

	// we need to handle EXISTENCE choices somewhat separately
	if (pChoice->eState == SG_RESOLVE__STATE__EXISTENCE)
	{
		// the existence value is just whether or not we have input data for this changeset
		SG_bool bValue = SG_FALSE;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pInputs, pszMnemonic, &bValue)  );
		SG_ERR_CHECK(  _variant__alloc__bool(pCtx, &pValue->pVariant, &pValue->uVariantSize, bValue)  );
	}
	else
	{
		SG_vhash* pInput = NULL;

		// check if we have input data for this changeset
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pInputs, pszMnemonic, &pInput)  );
		if (pInput == NULL)
		{
			// no input data, so there's no value to create
			_VALUE__NULLFREE(pCtx, pValue);
		}
		else
		{
			// we found some input data, go ahead and parse our variant data out of it
			switch (pChoice->eState)
			{
			case SG_RESOLVE__STATE__NAME:
				{
					const char* szValue = NULL;

					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pInput, gszIssueKey_Inputs_Name, &szValue)  );
					SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, &pValue->uVariantSize, szValue)  );

					break;
				}
			case SG_RESOLVE__STATE__LOCATION:
				{
					// stuff the parent-GID into {variant}.
					// stuff the parent-repo-path into {variantSummary}.
					// stuff length(parent-repo-path) into {variantSize}.
					//
					// Since this is a changeset-relative value, I'm going
					// to get the changeset-relative extended-prefix repo-path
					// rather than a live repo-path.  This is safer since this
					// parent directory may not exist in the final result *and*
					// it is consistent with how we report things in WC.

					SG_uint64 uiAliasGidParent;
					sg_wc_liveview_item * pLVI_parent = NULL;	// we do not own this.
					const char* szValue_gid_parent = NULL;
					SG_uint32 len;
					SG_bool bFound;

					// TODO 2012/06/08 Not sure if I like this.  We are looking up *WHERE*
					// TODO            this item *WAS* in cset[mnemonic].  And then reporting
					// TODO            the *CURRENT* repo-path of that directory.  This is
					// TODO            different than reporting the complete historical
					// TODO            cset-relative repo-path of the item.
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pInput, gszIssueKey_Inputs_Location, &szValue_gid_parent)  );
					SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, NULL, szValue_gid_parent)  );
					SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx, pChoice->pItem->pResolve->pWcTx->pDb,
																	 szValue_gid_parent, &uiAliasGidParent)  );
					SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pChoice->pItem->pResolve->pWcTx,
																		 uiAliasGidParent, &bFound, &pLVI_parent)  );
					if (bFound)
					{
						SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pChoice->pItem->pResolve->pWcTx,
																				  pLVI_parent, &pStringParentRepoPath)  );
					}
					else
					{
						// TODO 2012/06/08 Another instance where we don't have any info on the item.
						// TODO            See W7521.  For now, just return gid-domain repo-path.
						SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, szValue_gid_parent, &pStringParentRepoPath)  );
					}
					SG_ERR_CHECK(  SG_string__sizzle(pCtx, &pStringParentRepoPath,
													 (SG_byte **)&pValue->szVariantSummary, &len)  );
					pValue->uVariantSize = len;
					break;
				}
			case SG_RESOLVE__STATE__ATTRIBUTES:
				{
					SG_int64 iValue = 0;

					SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pInput, gszIssueKey_Inputs_Attributes, &iValue)  );
					SG_ERR_CHECK(  _variant__alloc__int64(pCtx, &pValue->pVariant, &pValue->uVariantSize, iValue)  );

					break;
				}
			case SG_RESOLVE__STATE__CONTENTS:
				{
					switch (pChoice->pItem->eType)
					{
					case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
						{
							// stuff the HID of the content into {variant}
							// we will address {pVariantFile, variantSize} further down.
							const char* szValue = NULL;

							SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pInput, gszIssueKey_Inputs_Contents, &szValue)  );
							SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, NULL, szValue)  );
							pValue->uVariantSize = 0u; // we'll fill this in further down, when we deal with szTempRepoPath

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
							// szValue contains the HID of the symlink blob in that changeset.
							// but we want to stuff the symlink-target into {variant, variantSize}.
							const char* szValue = NULL;

							SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pInput, gszIssueKey_Inputs_Contents, &szValue)  );
							SG_ERR_CHECK(  _variant__alloc__blob(pCtx,
																 pChoice->pItem->pResolve->pWcTx,
																 &pValue->pVariant, &pValue->uVariantSize,
																 szValue)  );
							break;
						}
					case SG_TREENODEENTRY_TYPE_SUBMODULE:
						{
							SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "submodule node type")  );
							break;
						}
					default:
						{
							SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown node type: %i", pChoice->pItem->eType));
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
	}

	// if we still have a value, fill in the rest of it
	if (pValue != NULL)
	{
		// duplicate the given label
		SG_ERR_CHECK(  SG_strdup(pCtx, szLabel, &pValue->szLabel)  );

		if (pChoice->eState == SG_RESOLVE__STATE__CONTENTS && pChoice->pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		{
			SG_vhash* pInput = NULL;

			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pInputs, pszMnemonic, &pInput)  );
			if (pInput)
			{
				const char * pszTempRepoPath = NULL;

				SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pInput, gszIssueKey_Inputs_TempRepoPath, &pszTempRepoPath)  );
				if (pszTempRepoPath)
				{
					SG_fsobj_stat cStat;

					// MERGE created a repo-path for the TEMP file.
					// Per [2] I'm just going to stuff the repo-path in {variantFile}
					// rather than store it as an absolute path.
					//
					// TODO 2012/04/05 Think about having MERGE define a new
					// TODO            prefix for these and create an
					// TODO            extended-prefix repo-path for these.
					// TODO            (not sure I like having .sgdrawer appear
					// TODO            in the repo-path.)
					// TODO            See [4].  W4679.

					SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx,
														&pValue->pVariantFile,
														pszTempRepoPath)  );

					// stuff the length of the TEMP file into {variantSize}.
					// It IS SAFE to use SG_pathname__ and SG_fsobj__ routines
					// because this is a TEMP file so we don't need WC to
					// manage the request.

					SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pValue, &pPath)  );
					SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &cStat)  );
					pValue->uVariantSize = cStat.size;
				}
			}
		}

		SG_ERR_CHECK(  SG_strdup(pCtx, pszMnemonic, &pValue->pszMnemonic)  );
	}

	// return the value
	*ppValue = pValue;
	pValue = NULL;

fail:
	_VALUE__NULLFREE(pCtx, pValue);
	SG_STRING_NULLFREE(pCtx, pStringParentRepoPath);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
}

/**
 * Allocates a value structure that is associated with the working directory.
 */
static void _value__alloc__working(
	SG_context*                    pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice*            pChoice, //< [in] The choice that will own the value.
	SG_resolve__value**            ppValue  //< [out] The allocated value.
	)
{
	SG_resolve__value* pValue        = NULL;
	SG_string*         sLabel        = NULL;
	SG_string*         sTarget       = NULL;
	sg_wc_liveview_item * pLVI        = NULL;	// we do not own this.
	sg_wc_liveview_item * pLVI_parent = NULL;	// we do not own this.
	SG_string * pStringParentRepoPath = NULL;
	char * pszGidParent = NULL;
	char * pszHid_owned = NULL;
	SG_bool bIsNotDeleted;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppValue);

	// allocate a blank value
	SG_ERR_CHECK(  _value__alloc__blank(pCtx, pChoice, &pValue)  );

	SG_ERR_CHECK(  SG_strdup(pCtx, gszMnemonic_Working, &pValue->pszMnemonic)  );

	// We no longer need to look this up each time.
	pLVI = pChoice->pItem->pLVI;
	bIsNotDeleted = (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pChoice->pItem->pLVI->scan_flags_Live) == SG_FALSE);

	// generate a working label
	SG_ERR_CHECK(  _generate_indexed_label(pCtx, pChoice, SG_RESOLVE__LABEL__WORKING, &sLabel)  );
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &sLabel, (SG_byte**)&pValue->szLabel, NULL)  );
	SG_ASSERT(sLabel == NULL);

	if (pChoice->eState == SG_RESOLVE__STATE__EXISTENCE)
	{
		// we only care about whether or not the item currently exists
		// and we already know that without checking the diff
		SG_ERR_CHECK(  _variant__alloc__bool(pCtx,
											 &pValue->pVariant,
											 &pValue->uVariantSize,
											 bIsNotDeleted)  );
	}
	else if (bIsNotDeleted)
	{
		switch (pChoice->eState)
		{
		case SG_RESOLVE__STATE__NAME:
			{
				// stuff the current entryname in {variant, variantSize}.

				SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, &pValue->uVariantSize,
												   SG_string__sz(pLVI->pStringEntryname))  );
			}
			break;
		case SG_RESOLVE__STATE__LOCATION:
			{
				SG_uint32 len;
				SG_bool bFound;

				// stuff the parent-GID in {variant},
				// stuff the parent-repo-path in {variantSummary, variantSize}.

				SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, 
																	 pChoice->pItem->pResolve->pWcTx,
																	 pLVI->pLiveViewDir->uiAliasGidDir,
																	 &bFound,
																	 &pLVI_parent)  );
				SG_ASSERT( (bFound) );
				SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx,
																		  pChoice->pItem->pResolve->pWcTx,
																		  pLVI_parent,
																		  &pStringParentRepoPath)  );
				SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx,
																 pChoice->pItem->pResolve->pWcTx->pDb,
																 pLVI_parent->uiAliasGid,
																 &pszGidParent)  );
				
				SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, NULL, pszGidParent)  );
				SG_ERR_CHECK(  SG_string__sizzle(pCtx,
												 &pStringParentRepoPath,
												 (SG_byte **)&pValue->szVariantSummary,
												 &len)  );
				pValue->uVariantSize = len;
			}
			break;
		case SG_RESOLVE__STATE__ATTRIBUTES:
			{
				// stuff attrbits in {variant(int)} and sizeof(SG_int64) in {variantSize}.
				SG_int64 iValue = 0;

				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits(pCtx, pLVI,
																		 pChoice->pItem->pResolve->pWcTx,
																		 (SG_uint64 *)&iValue)  );
				SG_ERR_CHECK(  _variant__alloc__int64(pCtx, &pValue->pVariant, &pValue->uVariantSize,
													  iValue)  );
			}
			break;
		case SG_RESOLVE__STATE__CONTENTS:
			{
				switch (pChoice->pItem->eType)
				{
				case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
					{
						// stuff GID-domain repo-path in {variantFile} ***See [2]***
						// 
						// stuff content HID in {variant},
						// stuff file length in {variantSize}.
						// WE HAVE TO ASK WC FOR {HID,length} BECAUSE WC IS TRACKING
						// IN-TX STUFF (such that if we have a previous overwrite-file
						// in this TX, we'll get the new/queued value rather the pre-tx
						// value).

						SG_uint64 sizeContent = 0;

						SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx, pLVI,
																					pChoice->pItem->pResolve->pWcTx,
																					SG_FALSE, // trust TSC
																					&pszHid_owned,
																					&sizeContent)  );
						SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, NULL, pszHid_owned)  );
						pValue->uVariantSize = sizeContent;

						SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
															  &pValue->pVariantFile,	// See [2].
															  pChoice->pItem->pStringGidDomainRepoPath)  );
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
						// stuff the symlink target (not the HID) into {variant},
						// stuff the length of the target in {variantSize}.
						// 
						// (Since we are assembling the 'working' value (and if it
						//  has been changed by the user), the current target probably
						//  won't yet exist as a blob in the repo, so we couldn't
						//  address it by HID anyway.)

						SG_ERR_CHECK(
							sg_wc_liveview_item__get_current_symlink_target(pCtx, pLVI,
																			pChoice->pItem->pResolve->pWcTx,
																			&sTarget)  );
						SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, &pValue->uVariantSize,
														   SG_string__sz(sTarget))  );
						break;
					}
				case SG_TREENODEENTRY_TYPE_SUBMODULE:
					{
						SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "submodule node type")  );
						break;
					}
				default:
					{
						SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown node type: %i", pChoice->pItem->eType));
						break;
					}
				}
			}
			break;
		default:
			{
				SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown resolve item state: %i", pChoice->eState));
			}
			break;
		}
	}
	else
	{
		// the item doesn't exist and this is not an EXISTENCE conflict
		// so for now we can't provide a working value for this conflict
		_VALUE__NULLFREE(pCtx, pValue);
	}

	// return the value
	*ppValue = pValue;
	pValue = NULL;

fail:
	_VALUE__NULLFREE(pCtx, pValue);
	SG_STRING_NULLFREE(pCtx, sLabel);
	SG_STRING_NULLFREE(pCtx, sTarget);
	SG_STRING_NULLFREE(pCtx, pStringParentRepoPath);
	SG_NULLFREE(pCtx, pszGidParent);
	SG_NULLFREE(pCtx, pszHid_owned);
}

/**
 * Allocates a new value by loading it from a resolve hash (created using _value__build_resolve_vhash).
 */
static void _value__alloc__saved(
	SG_context*         pCtx,     //< [in] [out] Error and context info.
	const SG_vhash*     pHash,    //< [in] Hash to load the value from.
	SG_resolve__choice* pChoice,  //< [in] The choice that will own the value.
	SG_resolve__value** ppValue   //< [out] The allocated value.
	)
{
	SG_resolve__value* pValue   = NULL;
	SG_bool            bBool    = SG_FALSE;
	const char*        szString = NULL;
	SG_variant*        pVariant = NULL;
	SG_int64           iInt     = 0;
	SG_fsobj_stat      cStat;
	SG_pathname *      pPathTempFile = NULL;
	SG_string *        pStringParentRepoPath = NULL;
	sg_wc_liveview_item * pLVI_parent = NULL;	// we do not own this.

	SG_NULLARGCHECK(pHash);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppValue);

	cStat.size = 0u;
	cStat.mtime_ms = 0;

	// start with a blank value
	SG_ERR_CHECK(  _value__alloc__blank(pCtx, pChoice, &pValue)  );

	// read the label
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHash, gszValueKey_Label, &szString)  );
	if (szString != NULL)
	{
		SG_ERR_CHECK(  SG_strdup(pCtx, szString, &pValue->szLabel)  );
		szString = NULL;
	}

	// read the variant
	SG_ERR_CHECK(  SG_vhash__check__variant(pCtx, pHash, gszValueKey_Value, &pVariant)  );
	if (pVariant != NULL)
	{
		SG_ERR_CHECK(  SG_variant__copy(pCtx, pVariant, &pValue->pVariant)  );
		pVariant = NULL;
	}

	// restore the value in pVariantFile.
	// Per [2] pVariantFile now contains a repo-path.
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pHash, gszValueKey_ValueFile, &szString)  );
	if (szString != NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pValue->pVariantFile, szString)  );
		szString = NULL;

		// Call stat() on it to get {size,mtime}.
		// 
		// We ASSUME that this is the path to a TEMP file
		// created by one of the mergetools.  So it is SAFE
		// to call SG_fsobj__ routines on it; we don't need
		// to go thru a WC TX routine to ask about it.

		SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pValue, &pPathTempFile)  );
		SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPathTempFile, &cStat)  );
	}

	// calculate the variant summary
	if (pValue->pChoice->eState == SG_RESOLVE__STATE__LOCATION)
	{
		// {variant} contains the parent-GID.
		// stuff the parent-repo-path in {variantSummary}.
		// {variantSize} is set later.
		// 
		// TODO 2012/04/04 Will this parent-GID ever be different from
		// TODO            the *current* parent-GID ?  If so, we could
		// TODO            eliminate a DB lookup here.

		SG_uint64 uiAliasGidParent;
		SG_bool bFound;

		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx,
														 pChoice->pItem->pResolve->pWcTx->pDb,
														 pValue->pVariant->v.val_sz,
														 &uiAliasGidParent)  );
		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx,
															 pChoice->pItem->pResolve->pWcTx,
															 uiAliasGidParent,
															 &bFound,
															 &pLVI_parent)  );
		SG_ASSERT( (bFound) );
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx,
																  pChoice->pItem->pResolve->pWcTx,
																  pLVI_parent,
																  &pStringParentRepoPath)  );
		
		SG_ERR_CHECK(  SG_string__sizzle(pCtx,
										 &pStringParentRepoPath,
										 (SG_byte **)&pValue->szVariantSummary,
										 NULL)  );
	}
	else if (
		   pValue->pChoice->eState == SG_RESOLVE__STATE__CONTENTS
		&& pValue->pChoice->pItem->eType == SG_TREENODEENTRY_TYPE_SUBMODULE
		)
	{
		SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "submodule node type")  );
	}

	// calculate the variant size
	switch (pValue->pChoice->eState)
	{
	case SG_RESOLVE__STATE__EXISTENCE:
		{
			pValue->uVariantSize = sizeof(pValue->pVariant->v.val_bool);
		}
		break;
	case SG_RESOLVE__STATE__NAME:
		{
			pValue->uVariantSize = strlen(pValue->pVariant->v.val_sz);
		}
		break;
	case SG_RESOLVE__STATE__LOCATION:
		{
			pValue->uVariantSize = strlen(pValue->szVariantSummary);
		}
		break;
	case SG_RESOLVE__STATE__ATTRIBUTES:
		{
			pValue->uVariantSize = sizeof(pValue->pVariant->v.val_int64);
		}
		break;
	case SG_RESOLVE__STATE__CONTENTS:
		{
			switch (pValue->pChoice->pItem->eType)
			{
			case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
				{
					// already ran the stat when we read the filename
					SG_ASSERT(pValue->pVariantFile != NULL);
					pValue->uVariantSize = cStat.size;
				}
				break;
			case SG_TREENODEENTRY_TYPE_DIRECTORY:
				{
					SG_ERR_THROW2(SG_ERR_RESOLVE_INVALID_SAVE_DATA,
								  (pCtx,
								   "Invalid saved resolve data: directory contents conflicts are not possible."));
				}
				break;
			case SG_TREENODEENTRY_TYPE_SYMLINK:
				{
					pValue->uVariantSize = strlen(pValue->pVariant->v.val_sz);
				}
				break;
			case SG_TREENODEENTRY_TYPE_SUBMODULE:
				{
					SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "submodule node type")  );
				}
				break;
			default:
				{
					SG_ERR_THROW2(SG_ERR_RESOLVE_INVALID_SAVE_DATA,
								  (pCtx,
								   "Invalid saved resolve data: Unknown node type: %i",
								   pValue->pChoice->pItem->eType));
				}
				break;
			}
		}
		break;
	default:
		{
			SG_ERR_THROW2(SG_ERR_RESOLVE_INVALID_SAVE_DATA,
						  (pCtx,
						   "Invalid saved resolve data: Unknown resolve item state: %i",
						   pValue->pChoice->eState));
		}
		break;
	}

	// read the changeset data
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pHash, gszValueKey_Mnemonic, &bBool)  );
	if (bBool == SG_TRUE)
	{
		// read the changeset
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHash, gszValueKey_Mnemonic, &szString)  );
		SG_ERR_CHECK(  SG_strdup(pCtx, szString, &pValue->pszMnemonic)  );
		szString = NULL;

		// if this is a working value
		// read in its base/parent value and find a pointer to it
		if (strcmp(pValue->pszMnemonic, gszMnemonic_Working) == 0)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHash, gszValueKey_Baseline, &szString)  );
			SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, szString, 0, &pValue->pBaselineParent)  );
			szString = NULL;
		}
	}

	// read the merge data
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pHash, gszValueKey_Baseline, &bBool)  );
	if (bBool == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pHash, gszValueKey_Other, &bBool)  );
		if (bBool == SG_TRUE)
		{
			// read the baseline's label and find a pointer to it
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHash, gszValueKey_Baseline, &szString)  );
			SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, szString, 0u, &pValue->pBaselineParent)  );
			szString = NULL;

			// read the other's label and find a pointer to it
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHash, gszValueKey_Other, &szString)  );
			SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, szString, 0u, &pValue->pOtherParent)  );
			szString = NULL;

			// update the child count of the value's parents
			pValue->pBaselineParent->uChildren += 1u;
			pValue->pOtherParent->uChildren += 1u;

			// we only save values that were manually merged
			// so any loaded values must be the same
			pValue->bAutoMerge = SG_FALSE;

			// read the merge tool name
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pHash, gszValueKey_Tool, &szString)  );
			SG_ERR_CHECK(  SG_strdup(pCtx, szString, &pValue->szMergeTool)  );
			szString = NULL;

			// read the merge tool result
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pHash, gszValueKey_ToolResult, &iInt)  );
			pValue->iMergeToolResult = (SG_int32)iInt;
			iInt = 0;

			// read the merge time
			// already ran the stat when we read the filename
			SG_ASSERT(pValue->pVariantFile != NULL);
			pValue->iMergeTime = cStat.mtime_ms;
		}
	}

	// return the value
	*ppValue = pValue;
	pValue = NULL;

fail:
	_VALUE__NULLFREE(pCtx, pValue);
	SG_PATHNAME_NULLFREE(pCtx, pPathTempFile);
	SG_STRING_NULLFREE(pCtx, pStringParentRepoPath);
}

/**
 * Creates and populates a new SG_resolve__value from a merge result.
 */
static void _value__alloc__merge(
	SG_context*         pCtx,        //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,     //< [in] The choice that will own the value.
	SG_string**         ppLabel,     //< [in] The new value's label.
	                                 //<      We steal this pointer and NULL the caller's.
	SG_pathname*        pTempFile,   //< [in] The new value's absolute temporary pathname.
	                                 //<      May be NULL if the merge failed.
	                                 //<      In spite of [2] I'm going to leave this argument
	                                 //<      an absolute pathname because the caller needed it
	                                 //<      to run the merge and we need it to stat() it 
	                                 //<      afterwards.  We'll convert it to a repo-path for
	                                 //<      storage in pVariantFile.
	SG_resolve__value*  pBaseline,   //< [in] The new value's baseline parent.
	SG_resolve__value*  pOther,      //< [in] The new value's other parent.
	SG_bool             bAutomatic,  //< [in] Whether or not the value was created automatically by merge,
	                                 //<      as opposed to manually by resolve.
	char**              ppTool,      //< [in] The merge tool used to create the new value.
	                                 //<      We steal this pointer and NULL the caller's.
	SG_int32            iToolResult, //< [in] The result returned by the specified tool.
	SG_resolve__value** ppValue      //< [out] The new value.
	                                 //<       The pointers we stole now belong to it.
	)
{
	SG_resolve__value* pValue = NULL;
	SG_file*           pFile  = NULL;
	char*              szHid  = NULL;

	SG_NULLARGCHECK(ppLabel);
	SG_NULLARGCHECK(pBaseline);
	SG_NULLARGCHECK(pOther);
	SG_NULLARGCHECK(ppTool);
	SG_NULLARGCHECK(ppValue);

	SG_ERR_CHECK(  _value__alloc__blank(pCtx, pChoice, &pValue)  );

	SG_ERR_CHECK(  SG_string__sizzle(pCtx, ppLabel, (SG_byte**)&pValue->szLabel, NULL)  );
	SG_ASSERT(*ppLabel == NULL);

	if (pTempFile != NULL)
	{
		SG_fsobj_stat cStat;

		// get the file's size/date for tortoise to display in the dialog.
		// It IS SAFE to call SG_fsobj__ routines because this is a TEMP file;
		// we don't need to ask WC for the info.
		//
		// stuff observed file size in {variantSize}.
		SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pTempFile, &cStat)  );
		pValue->uVariantSize = cStat.size;
		pValue->iMergeTime = cStat.mtime_ms;

		// compute/store the HID of the temp file and stuff it in {variant}.
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pTempFile, SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY, 0, &pFile)  );
		SG_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx,
															  pChoice->pItem->pResolve->pWcTx->pDb->pRepo,
															  pFile, &szHid)  );
		SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
		SG_ERR_CHECK(  _variant__alloc__sz(pCtx, &pValue->pVariant, NULL, szHid)  );

		// stuff the repo-path of the pathname in {variantFile}.  See [2].  See [4].
		SG_ERR_CHECK(  sg_wc_db__path__absolute_to_repopath(pCtx,
															pChoice->pItem->pResolve->pWcTx->pDb,
															pTempFile,
															&pValue->pVariantFile)  );
	}

	pValue->pBaselineParent = pBaseline;
	pValue->pOtherParent    = pOther;
	pValue->bAutoMerge      = bAutomatic;

	pValue->szMergeTool = *ppTool;
	*ppTool = NULL;

	pValue->iMergeToolResult = iToolResult;

	pBaseline->uChildren += 1u;
	pOther->uChildren    += 1u;

	*ppValue = pValue;
	pValue = NULL;

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_NULLFREE(pCtx, szHid);
	SG_NULLFREE(pCtx, *ppTool);
	_VALUE__NULLFREE(pCtx, pValue);
	return;
}

/**
 * Saves a live working value from a mergeable choice.  This converts the given
 * live value into a saved value by copying its data file from the live working
 * copy into the mergeable choice's temp path and updating the value structure's
 * pVariantFile to point to the copy.  The function will throw an error if the
 * value isn't a live working value from a mergeable choice.
 *
 * TODO: This function should also create and return a new live working value to
 *       replace the one that was saved.  Right now resolve code that modifies
 *       resolve data (like by calling this function) relies on the user allocating
 *       a new resolve instance to pick up side-effect changes like a new live
 *       working value.
 */
static void _value__working_mergeable__save(
	SG_context*        pCtx,  //< [in] [out] Error and context info.
	SG_resolve__value* pValue //< [in] [out] The working value to save.
	)
{
	SG_pathname* pPathSource         = NULL;
	SG_pathname* pPathTarget         = NULL;
	SG_string* pStringSourceRepoPath = NULL;
	SG_string* pStringTargetRepoPath = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pValue->pVariantFile);
	SG_ARGCHECK( (strcmp(pValue->pszMnemonic, gszMnemonic_Working) == 0), pValue->pszMnemonic);
	SG_ARGCHECK(pValue->pChoice->pLeafValues != NULL, pValue->pChoice->pLeafValues);

	// get a pathname from pValue taking into account [2] and [4].
	SG_ERR_CHECK(  _wc__variantFile_to_absolute(pCtx, pValue, &pPathSource)  );

	// generate an absolute temporary pathname to copy the file to
	SG_ERR_CHECK(  _generate_temp_filename(pCtx,
										   pValue->pChoice,
										   gszFilenameFormat_Working,
										   SG_RESOLVE__LABEL__WORKING,
										   &pPathTarget)  );

	// Per [2] we need to map the target path back to a repo-path to store in pVariantFile.
	SG_ERR_CHECK(  sg_wc_db__path__absolute_to_repopath(pCtx,
														pValue->pChoice->pItem->pResolve->pWcTx->pDb,
														pPathTarget,
														&pStringTargetRepoPath)  );

	SG_ERR_CHECK(  SG_fsobj__copy_file(pCtx, pPathSource, pPathTarget, 0400)  );

#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("_value__working_mergeable__save: %s\n"
								"    [%s]->[%s]\n"
								"    [%s]->[%s]\n"),
							   pValue->pszMnemonic,
							   SG_string__sz(pValue->pVariantFile),
							   SG_string__sz(pStringTargetRepoPath),
							   SG_pathname__sz(pPathSource),
							   SG_pathname__sz(pPathTarget))  );
#endif

	// replace pVariantFile to point to the new TEMP file.
	pStringSourceRepoPath = pValue->pVariantFile;
	pValue->pVariantFile  = pStringTargetRepoPath;
	pStringTargetRepoPath = NULL;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathSource);
	SG_PATHNAME_NULLFREE(pCtx, pPathTarget);
	SG_STRING_NULLFREE(pCtx, pStringSourceRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringTargetRepoPath);
}

/**
 * Checks if a value is:
 * a) from a mergeable choice
 * b) is associated with the "working" changeset
 * c) represents the live working copy
 *
 * A live value indicates a file in the actual working copy as its data.
 * A saved value indicates a file in the choice's temp path as its data.
 */
static void _value__mergeable_working_live(
	SG_context*        pCtx,       //< [in] [out] Error and context info.
	SG_resolve__value* pValue,     //< [in] The working value to check for a live file.
	SG_bool*           pMergeable, //< [out] Whether or not the value is from a mergeable choice.
	SG_bool*           pWorking,   //< [out] Whether or not the value is a working value.
	SG_bool*           pLive       //< [out] Whether or not the value represents a live file.
	                               //<       Only used/set if pWorking and pMergeable are both true.
	                               //<       False if the value reflects a live file.
	)
{
	SG_bool bMergeable = SG_FALSE;
	SG_bool bWorking   = SG_FALSE;
	SG_bool bLive      = SG_FALSE;

	SG_NULLARGCHECK(pValue);

	if (pValue->pChoice->pLeafValues != NULL)
	{
		bMergeable = SG_TRUE;
	}
	else
	{
		bMergeable = SG_FALSE;
	}

	if (pValue->pszMnemonic && (strcmp(pValue->pszMnemonic, gszMnemonic_Working) == 0))
	{
		bWorking = SG_TRUE;
	}
	else
	{
		bWorking = SG_FALSE;
	}

	if (bMergeable == SG_TRUE && bWorking == SG_TRUE && pLive != NULL)
	{
		// Per [2] we now have a repo-path in pVariantFile and we always
		// create a GID-domain repo-path for 'working' values.  Values
		// for TEMP files are repo-paths relative to .sgdrawer.

		const char * psz = SG_string__sz(pValue->pVariantFile);

		bLive = ((psz[0] == '@') && (psz[1] == SG_WC__REPO_PATH_DOMAIN__G));
	}

	if (pMergeable != NULL)
	{
		*pMergeable = bMergeable;
	}
	if (pWorking != NULL)
	{
		*pWorking = bWorking;
	}
	if (pLive != NULL)
	{
		*pLive = bLive;
	}

fail:
	return;
}

/**
 * Creates a hash that contains the value data that is unique to resolve.
 * Data derived from merge issues or pendingtree is not included.
 * If the given value can be entirely constructed from merge/pendingtree data,
 * then no hash is generated at all.
 */
static void _value__build_resolve_vhash(
	SG_context*        pCtx,   //< [in] [out] Error and context data.
	SG_resolve__value* pValue, //< [in] The value to build a vhash from.
	SG_vhash**         ppHash  //< [out] The built vhash, or NULL if none is required.
	)
{
	SG_vhash* pHash      = NULL;
	SG_bool   bMergeable = SG_FALSE;
	SG_bool   bWorking   = SG_FALSE;
	SG_bool   bLive      = SG_FALSE;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(ppHash);

	// check if this is a live working value on a mergeable choice
	SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pValue, &bMergeable, &bWorking, &bLive)  );

	// There are two kinds of values that we need to save out.. any other values
	// can be reconstructed next time from merge issue and/or pendingtree data.
	// 1) Merge results that were manual (generated by resolve rather than merge).
	// 2) Working directory values that aren't attached to the live working copy.
	//    These values are only present in mergeable choices where the working value
	//    has been copied into the temp path because it needs to be saved as it was
	//    at a particular point in the past.  This currently happens for two reasons:
	//    a) The working value is the parent of a manual merge.  In that case we
	//       need the value to remain as it was when it was merged.
	//    b) The working value contained manual edits when another value was
	//       accepted as the result.  In that case we don't want the user to lose
	//       their manual edits, so we save a copy of the edited working value
	//       before overwriting the live value with whatever they accepted.
	//    Working values associated with the live working copy don't need to be saved
	//    because we'll just allocate/create a new one next time.
	if (
		// test for (1)
		(
			   pValue->pBaselineParent != NULL
			&& pValue->pOtherParent    != NULL
			&& pValue->bAutoMerge      == SG_FALSE
		)
		||
		// test for (2)
		(
			   bMergeable == SG_TRUE
			&& bWorking   == SG_TRUE
			&& bLive      == SG_FALSE
		)
		)
	{
		// allocate the vhash
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pHash)  );

		// label
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_Label, pValue->szLabel)  );

		// value
		if (pValue->pVariant != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pHash, gszValueKey_Value, pValue->pVariant)  );
		}

		// save pVariantFile.
		// Per [2] it now contains a repo-path. So we can save it as is.
		if (pValue->pVariantFile != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_ValueFile,
													 SG_string__sz(pValue->pVariantFile))  );
		}

		// changeset data
		if (pValue->pszMnemonic != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_Mnemonic, pValue->pszMnemonic)  );

			// if this value is associated with the working directory
			// then save off its parent/base value as well
			if (strcmp(pValue->pszMnemonic, gszMnemonic_Working) == 0)
			{
				// TODO 2012/05/31 See W1222.
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_Baseline, pValue->pBaselineParent->szLabel)  );
			}
		}

		// merge data
		if (pValue->pBaselineParent != NULL && pValue->pOtherParent != NULL)
		{
			SG_ASSERT(pValue->szMergeTool != NULL);
			// TODO 2012/05/31 See W1222.
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_Baseline, pValue->pBaselineParent->szLabel)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_Other, pValue->pOtherParent->szLabel)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszValueKey_Tool, pValue->szMergeTool)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pHash, gszValueKey_ToolResult, pValue->iMergeToolResult)  );
		}
	}

	// return the hash
	*ppHash = pHash;
	pHash = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pHash);
	return;
}

/**
 * An SG_free_callback that frees values.
 */
static void _value__sg_action__free(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pData //< [in] The SG_resolve__value to free.
	)
{
	SG_resolve__value* pValue = (SG_resolve__value*)pData;
	_VALUE__NULLFREE(pCtx, pValue);
}

/**
 * An SG_vector_predicate that matches an SG_resolve__value's
 * label against a given label.
 */
static void _value__vector_predicate__match_label(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the value being matched.
	void*       pVal,      //< [in] The SG_resolve__value being matched.
	void*       pUserData, //< [in] The string label (const char*) to match against.
	SG_bool*    pMatch     //< [out] Whether or not the value's label matches the given one.
	)
{
	SG_resolve__value* pValue       = (SG_resolve__value*)pVal;
	const char*        szMatchLabel = (const char*)pUserData;
	const char*        szValueLabel = NULL;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pVal);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pMatch);

	SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szValueLabel)  );

	if (SG_stricmp(szMatchLabel, szValueLabel) == 0)
	{
		*pMatch = SG_TRUE;
	}
	else
	{
		*pMatch = SG_FALSE;
	}

fail:
	return;
}

/**
 * An SG_vector_predicate that matches an SG_resolve__value's
 * changeset mnemonic against the given one.
 */
static void _value__vector_predicate__match_changeset__mnemonic(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the value being matched.
	void*       pVal,      //< [in] The SG_resolve__value being matched.
	void*       pUserData, //< [in] The changeset mnemonic string (const char*) to match against.
	SG_bool*    pMatch     //< [out] Whether or not the value's changeset mnemonic matches the given one.
	)
{
	SG_resolve__value* pValue           = (SG_resolve__value*)pVal;
	const char*        pszMatchMnemonic = (const char*)pUserData;
	SG_bool            bHasMnemonic     = SG_FALSE;
	const char*        pszValueMnemonic = NULL;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pVal);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pMatch);

	SG_ERR_CHECK(  SG_resolve__value__get_changeset__mnemonic(pCtx, pValue, &bHasMnemonic, &pszValueMnemonic)  );

	if ((bHasMnemonic == SG_TRUE) && (SG_stricmp(pszMatchMnemonic, pszValueMnemonic) == 0))
	{
		*pMatch = SG_TRUE;
	}
	else
	{
		*pMatch = SG_FALSE;
	}

fail:
	return;
}

/**
 * An SG_vector_predicate that matches a live working value.
 *
 * A "live" working value is one whose data reflects the current state of the
 * working copy.  In certain cases on mergeable choices, there can also be "saved"
 * working values, whose data has actually been stored outside the working copy,
 * and therefore no longer reflects its current state.
 * Every choice should therefore always have exactly one live working value.
 */
static void _value__vector_predicate__match_live_working(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the value being matched.
	void*       pVal,      //< [in] The SG_resolve__value being matched.
	void*       pUserData, //< [in] Unused.
	SG_bool*    pMatch     //< [out] Whether or not the value is the live working value on its choice.
	)
{
	SG_resolve__value* pValue     = (SG_resolve__value*)pVal;
	SG_bool            bMergeable = SG_FALSE;
	SG_bool            bWorking   = SG_FALSE;
	SG_bool            bLive      = SG_FALSE;
	SG_bool            bMatch     = SG_FALSE;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pVal);
	SG_UNUSED(pUserData);
	SG_NULLARGCHECK(pMatch);

	// check if this is a live working value on a mergeable choice
	SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pValue, &bMergeable, &bWorking, &bLive)  );
	if (bMergeable == SG_TRUE && bWorking == SG_TRUE && bLive == SG_TRUE)
	{
		bMatch = SG_TRUE;
	}
	else
	{
		bMatch = SG_FALSE;
	}

	// return what we found
	*pMatch = bMatch;

fail:
	return;
}

/**
 * An SG_vector_predicate that matches either of an
 * SG_resolve__value's parent pointers.
 */
static void _value__vector_predicate__match_parents(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the value being matched.
	void*       pVal,      //< [in] The current SG_resolve__value being matched.
	void*       pUserData, //< [in] The SG_resolve__value whose parents will match.
	SG_bool*    pMatch     //< [out] Whether or not the current value is either of the specified value's parents.
	)
{
	SG_resolve__value* pCurrentValue   = (SG_resolve__value*)pVal;
	SG_resolve__value* pChildValue     = (SG_resolve__value*)pUserData;
	SG_bool            bMerged         = SG_FALSE;
	SG_resolve__value* pBaselineParent = NULL;
	SG_resolve__value* pOtherParent    = NULL;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pVal);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pMatch);

	SG_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pChildValue, &bMerged, &pBaselineParent, &pOtherParent, NULL, NULL, NULL, NULL)  );

	if (
		bMerged == SG_TRUE
		&& (
			   pCurrentValue == pBaselineParent
			|| pCurrentValue == pOtherParent
		)
	)
	{
		*pMatch = SG_TRUE;
	}
	else
	{
		*pMatch = SG_FALSE;
	}

fail:
	return;
}

/**
 * An SG_vector_predicate that matches an SG_resolve__value's data.
 */
static void _value__vector_predicate__match_data(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the value being matched.
	void*       pVal,      //< [in] The current SG_resolve__value being matched.
	void*       pUserData, //< [in] The SG_resolve__value whose data should be matched against.
	SG_bool*    pMatch     //< [out] Whether or not the current value's data matches the given value's.
	)
{
	SG_resolve__value* pCurrentValue = (SG_resolve__value*)pVal;
	SG_resolve__value* pMatchValue   = (SG_resolve__value*)pUserData;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pVal);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pMatch);

	// we can't use SG_variant__equal in the case of strings
	// because we want a case-insensitive comparison,
	// but it will work fine for other cases
	if (
		   pCurrentValue->pVariant->type == SG_VARIANT_TYPE_SZ
		&& pMatchValue->pVariant->type   == SG_VARIANT_TYPE_SZ
		)
	{
		*pMatch = SG_stricmp(pCurrentValue->pVariant->v.val_sz, pMatchValue->pVariant->v.val_sz) == 0 ? SG_TRUE : SG_FALSE;
	}
	else
	{
		SG_ERR_CHECK(  SG_variant__equal(pCtx, pCurrentValue->pVariant, pMatchValue->pVariant, pMatch)  );
	}

fail:
	return;
}

/**
 * An SG_resolve__value__callback that adds each value to a varray as a vhash.
 */
static void _value__resolve_action__add_resolve_vhash_to_varray(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] The SG_varray to add the value to.
	SG_bool*           pContinue  //< [out] Whether or not to continue iteration.
	)
{
	SG_varray* pValues = (SG_varray*)pUserData;
	SG_vhash*  pHash   = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	// get a hash from the value
	SG_ERR_CHECK(  _value__build_resolve_vhash(pCtx, pValue, &pHash)  );

	// if we got a hash back, add it to the supplied array of values
	// (the array takes ownership of the hash)
	if (pHash != NULL)
	{
		SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pValues, &pHash)  );
		pHash = NULL;
	}

	// continue iterating
	*pContinue = SG_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pHash);
	return;
}

/**
 * Type of user data required by _value__vector_action__call_resolve_action.
 */
typedef struct
{
	SG_resolve__value__callback* fCallback; //< The value callback to call.
	void*                        pUserData; //< The user data to pass to the callback.
	SG_bool                      bContinue; //< Whether or not to actually call the callback.
}
_value__vector_action__call_resolve_action__data;

/**
 * An SG_vector_foreach_callback that calls an SG_resolve__value__callback.
 */
static void _value__vector_action__call_resolve_action(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_uint32   uIndex,   //< [in] The index of the item.
	void*       pVal,     //< [in] The value of the item.
	void*       pUserData //< [in] A populated _value__vector_action__call_resolve_action structure.
	)
{
	SG_resolve__value*                                pValue = (SG_resolve__value*)pVal;
	_value__vector_action__call_resolve_action__data* pArgs  = (_value__vector_action__call_resolve_action__data*)pUserData;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pVal);
	SG_NULLARGCHECK(pUserData);

	if (pArgs->bContinue == SG_TRUE)
	{
		SG_ERR_CHECK(  pArgs->fCallback(pCtx, pValue, pArgs->pUserData, &pArgs->bContinue)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__VALUE__INLINE1_H
