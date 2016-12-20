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
 * @file sg_wc_tx__resolve__value__inline2.h
 *
 * @details SG_resolve__value__ routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__VALUE__INLINE2_H
#define H_SG_WC_TX__RESOLVE__VALUE__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////


void SG_resolve__value__get_choice(
	SG_context*          pCtx,
	SG_resolve__value*   pValue,
	SG_resolve__choice** ppChoice
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(ppChoice);

	*ppChoice = pValue->pChoice;

fail:
	return;
}

void SG_resolve__value__get_label(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	const char**       ppLabel
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(ppLabel);

	*ppLabel = pValue->szLabel;

fail:
	return;
}

void SG_resolve__value__get_data__bool(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_bool*           pData
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pData);

	SG_ERR_TRY(  SG_variant__get__bool(pCtx, pValue->pVariant, pData)  );
	SG_ERR_CATCH_REPLACE(SG_ERR_VARIANT_INVALIDTYPE, SG_ERR_USAGE);
	SG_ERR_CATCH_END;

fail:
	return;
}

void SG_resolve__value__get_data__int64(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_int64*          pData
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pData);

	// I would call SG_variant__get__int64 here, but at the moment it
	// attempts an implicit conversion from a string, if the variant
	// happens to be a string (and a value's data could very possibly
	// be a string).  Until that's fixed, it's safer here to just check
	// the variant's internal data manually.
	if (pValue->pVariant->type != SG_VARIANT_TYPE_INT64)
	{
		SG_ERR_THROW(SG_ERR_USAGE);
	}
	*pData = pValue->pVariant->v.val_int64;

fail:
	return;
}

void SG_resolve__value__get_data__sz(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	const char**       ppData
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(ppData);

	SG_ERR_TRY(  SG_variant__get__sz(pCtx, pValue->pVariant, ppData)  );
	SG_ERR_CATCH_REPLACE(SG_ERR_VARIANT_INVALIDTYPE, SG_ERR_USAGE);
	SG_ERR_CATCH_END;

fail:
	return;
}

void SG_resolve__value__get_summary(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_string**        ppSummary
	)
{
	SG_string* sSummary = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(ppSummary);

	switch (pValue->pChoice->eState)
	{
	case SG_RESOLVE__STATE__EXISTENCE:
		{
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sSummary, "%s", pValue->pVariant->v.val_bool == SG_FALSE ? "deleted" : "exists"));
			break;
		}
	case SG_RESOLVE__STATE__NAME:
		{
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sSummary, pValue->pVariant->v.val_sz)  );
			break;
		}
	case SG_RESOLVE__STATE__LOCATION:
		{
			// pValue->pVariant contains the Parent-GID.

			SG_uint64 uiAliasGid_Parent;
			sg_wc_liveview_item * pLVI_Parent;
			SG_bool bFound;

			SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx,
															 pValue->pChoice->pItem->pResolve->pWcTx->pDb,
															 pValue->pVariant->v.val_sz,
															 &uiAliasGid_Parent)  );
			SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx,
																 pValue->pChoice->pItem->pResolve->pWcTx,
																 uiAliasGid_Parent,
																 &bFound,
																 &pLVI_Parent)  );
			if (bFound)
			{
				SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx,
																		  pValue->pChoice->pItem->pResolve->pWcTx,
																		  pLVI_Parent,
																		  &sSummary)  );
			}
			else
			{
				// TODO 2012/06/08 Another instance where we don't have any info on the item.
				// TODO            See W7521.  For now, just return gid-domain repo-path.
				SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pValue->pVariant->v.val_sz, &sSummary)  );
			}
			break;
		}
	case SG_RESOLVE__STATE__ATTRIBUTES:
		{
			SG_uint64 uValue = 0u;
			SG_ERR_CHECK(  SG_variant__get__uint64(pCtx, pValue->pVariant, &uValue)  );
			// TODO 2012/03/20 %o isn't right here.
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sSummary, "%o", (SG_uint32)uValue)  );
			break;
		}
	case SG_RESOLVE__STATE__CONTENTS:
		{
			switch (pValue->pChoice->pItem->eType)
			{
			case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
				{
					if (pValue->pBaselineParent != NULL && pValue->pOtherParent != NULL)
					{
						SG_time cTime;

						// for merge results, build a sentence of data about the merge
						SG_ERR_CHECK(  SG_time__decode__local(pCtx, pValue->iMergeTime, &cTime)  );
						SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sSummary, "Merge of %s and %s using %s from %s %u at %u:%02u", pValue->pBaselineParent->szLabel, pValue->pOtherParent->szLabel, pValue->szMergeTool, gaTime_Months_Abbr[cTime.month-1u], cTime.mday, cTime.hour, cTime.min)  );
					}
					else if (strcmp(pValue->pszMnemonic, gszMnemonic_Ancestor) == 0)
					{
						// hard-coded string for ancestor values
						SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &sSummary, "File's contents from the merge's common ancestor changeset")  );
					}
					else if (strcmp(pValue->pszMnemonic, gszMnemonic_Working) == 0)
					{
						SG_bool            bMergeableWorking = SG_FALSE;
						SG_resolve__value* pParent           = NULL;
						SG_bool            bModified         = SG_FALSE;
						SG_bool            bLive             = SG_FALSE;

						// we already know the value is mergeable and working, we just need to know if it's live
						SG_ERR_CHECK(  SG_resolve__value__get_mergeable_working(pCtx, pValue, &bMergeableWorking, &pParent, &bModified)  );
						SG_ASSERT(bMergeableWorking == SG_TRUE);
						SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pValue, NULL, NULL, &bLive)  );

						// base summary depends on if the value is live or saved
						if (bLive == SG_TRUE)
						{
							SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &sSummary, "File's current contents in the working copy")  );
						}
						else
						{
							SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &sSummary, "Previous version of working copy contents")  );
						}

						// add information about the edited state of the working value
						if (bModified == SG_TRUE)
						{
							SG_ERR_CHECK(  SG_string__append__format(pCtx, sSummary, " (%s + edits)", pParent->szLabel)  );
						}
						else
						{
							SG_ERR_CHECK(  SG_string__append__format(pCtx, sSummary, " (from %s)", pParent->szLabel)  );
						}
					}
					else if (strcmp(pValue->pszMnemonic, gszMnemonic_Baseline) == 0)
					{
						// hard-coded string for baseline values
						SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &sSummary, "File's contents from the original parent changeset")  );
					}
					else if (strcmp(pValue->pszMnemonic, gszMnemonic_Other) == 0)
					{
						// hard-coded string for otherN values
						SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &sSummary, "File's contents from the merged-in changeset")  );
					}
					else
					{
						SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown changeset mnemonic '%s'", pValue->pszMnemonic));
					}
					break;
				}
			case SG_TREENODEENTRY_TYPE_DIRECTORY:
				{
					SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Cannot resolve a directory contents conflict because they should never occur."));
					break;
				}
			case SG_TREENODEENTRY_TYPE_SYMLINK:
				{
					SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sSummary, pValue->pVariant->v.val_sz)  );
					break;
				}
			case SG_TREENODEENTRY_TYPE_SUBMODULE:
				{
					SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sSummary, pValue->szVariantSummary)  );
					break;
				}
			default:
				{
					SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown node type: %i", pValue->pChoice->pItem->eType));
					break;
				}
			}
			break;
		}
	default:
		{
			SG_ERR_THROW2(SG_ERR_NOTIMPLEMENTED, (pCtx, "Unknown resolve item state: %i", pValue->pChoice->eState));
			break;
		}
	}

	*ppSummary = sSummary;
	sSummary = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sSummary);
	return;
}

void SG_resolve__value__get_size(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_uint64*         pSize
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pSize);

	*pSize = pValue->uVariantSize;

fail:
	return;
}

void SG_resolve__value__get_flags(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_uint32*         pFlags
	)
{
	SG_uint32 uFlags     = 0u;
	SG_bool   bLeaf      = SG_FALSE;
	SG_bool   bMergeable = SG_FALSE;
	SG_bool   bWorking   = SG_FALSE;
	SG_bool   bLive      = SG_FALSE;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pFlags);

	// check if the value is a changeset or a merge
	// and whether or not it was automatically created
	if (pValue->pszMnemonic != NULL)
	{
		uFlags |= SG_RESOLVE__VALUE__CHANGESET;
		uFlags |= SG_RESOLVE__VALUE__AUTOMATIC;
	}
	else if (pValue->pBaselineParent != NULL && pValue->pOtherParent != NULL)
	{
		uFlags |= SG_RESOLVE__VALUE__MERGE;
		if (pValue->bAutoMerge == SG_TRUE)
		{
			uFlags |= SG_RESOLVE__VALUE__AUTOMATIC;
		}
	}

	// check if the value is its choice's result
	// NOTE 2012/08/29 With the changes for W1849 this value
	// NOTE            is only defined for file-contents.
	if (pValue == pValue->pChoice->pMergeableResult)
	{
		uFlags |= SG_RESOLVE__VALUE__RESULT;
	}

	// check if the value can be overwritten
	// meaning it's not automatic and it's not a result
	if ((uFlags & (SG_RESOLVE__VALUE__AUTOMATIC | SG_RESOLVE__VALUE__RESULT)) == 0)
	{
		uFlags |= SG_RESOLVE__VALUE__OVERWRITABLE;
	}

	// check if the value is a merge leaf
	if (pValue->pChoice->pLeafValues != NULL)
	{
		SG_ERR_CHECK(  SG_vector__contains(pCtx, pValue->pChoice->pLeafValues, (void*)pValue, &bLeaf)  );
		if (bLeaf == SG_TRUE)
		{
			uFlags |= SG_RESOLVE__VALUE__LEAF;
		}
	}

	// check if the value is a live working value
	SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pValue, &bMergeable, &bWorking, &bLive)  );
	if (bWorking == SG_TRUE)
	{
		if (bMergeable == SG_TRUE)
		{
			// mergeable choice, value might be live or saved
			if (bLive == SG_TRUE)
			{
				uFlags |= SG_RESOLVE__VALUE__LIVE;
			}
		}
		else
		{
			// non-mergeable choice, the working value is always live
			uFlags |= SG_RESOLVE__VALUE__LIVE;
		}
	}

	// return the flags
	*pFlags = uFlags;

fail:
	return;
}

void SG_resolve__value__has_flag(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_uint32          uFlag,
	SG_bool*           pHasFlag
	)
{
	SG_uint32 uFlags = 0u;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pHasFlag);

	SG_ERR_CHECK(  SG_resolve__value__get_flags(pCtx, pValue, &uFlags)  );
	if ((uFlags & uFlag) == uFlag)
	{
		*pHasFlag = SG_TRUE;
	}
	else
	{
		*pHasFlag = SG_FALSE;
	}

fail:
	return;
}

void SG_resolve__value__get_changeset__mnemonic(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	SG_bool*           pHasChangeset,
	const char**       ppszMnemonic
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pHasChangeset);

	if (pValue->pszMnemonic == NULL)
	{
		*pHasChangeset = SG_FALSE;
		if (ppszMnemonic)
			*ppszMnemonic = NULL;
	}
	else
	{
		*pHasChangeset = SG_TRUE;
		if (ppszMnemonic != NULL)
		{
			*ppszMnemonic = pValue->pszMnemonic;
		}
	}

fail:
	return;
}

void SG_resolve__value__get_mergeable_working(
	SG_context*         pCtx,
	SG_resolve__value*  pValue,
	SG_bool*            pIsMergeableWorking,
	SG_resolve__value** ppParent,
	SG_bool*            pModified
	)
{
	SG_bool bMergeable = SG_FALSE;
	SG_bool bWorking   = SG_FALSE;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pIsMergeableWorking);

	SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pValue, &bMergeable, &bWorking, NULL)  );

	if (bMergeable == SG_FALSE || bWorking == SG_FALSE)
	{
		*pIsMergeableWorking = SG_FALSE;
	}
	else
	{
		*pIsMergeableWorking = SG_TRUE;
		if (ppParent != NULL)
		{
			*ppParent = pValue->pBaselineParent;
		}
		if (pModified != NULL)
		{
			SG_bool bEqual = SG_FALSE;

			// check if the value's data is equal to its parent's
			SG_ERR_CHECK(  SG_variant__equal(pCtx, pValue->pVariant, pValue->pBaselineParent->pVariant, &bEqual)  );
			if (bEqual == SG_TRUE)
			{
				*pModified = SG_FALSE;
			}
			else
			{
				*pModified = SG_TRUE;
			}
		}
	}

fail:
	return;
}

void SG_resolve__value__get_merge(
	SG_context*         pCtx,
	SG_resolve__value*  pValue,
	SG_bool*            pWasMerged,
	SG_resolve__value** ppBaseline,
	SG_resolve__value** ppOther,
	SG_bool*            pAutomatic,
	const char**        ppTool,
	SG_int32*           pToolResult,
	SG_int64*           pMergeTime
	)
{
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pWasMerged);

	if (pValue->pBaselineParent == NULL || pValue->pOtherParent == NULL)
	{
		*pWasMerged = SG_FALSE;
	}
	else
	{
		*pWasMerged = SG_TRUE;
		if (ppBaseline != NULL)
		{
			*ppBaseline = pValue->pBaselineParent;
		}
		if (ppOther != NULL)
		{
			*ppOther = pValue->pOtherParent;
		}
		if (pAutomatic != NULL)
		{
			*pAutomatic = pValue->bAutoMerge;
		}
		if (ppTool != NULL)
		{
			*ppTool = pValue->szMergeTool;
		}
		if (pToolResult != NULL)
		{
			*pToolResult = pValue->iMergeToolResult;
		}
		if (pMergeTime != NULL)
		{
			*pMergeTime = pValue->iMergeTime;
		}
	}

fail:
	return;
}

void SG_resolve__value_callback__count(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	void*              pUserData,
	SG_bool*           pContinue
	)
{
	SG_uint32* pCount = (SG_uint32*)pUserData;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	*pCount += 1u;
	*pContinue = SG_TRUE;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__VALUE__INLINE2_H
