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
 * @file sg_wc_tx__resolve__choice__inline1.h
 *
 * @details _choice__ routines that I pulled from the PendingTree
 * version of sg_resolve.c.  These were considered STATIC PRIVATE
 * to sg_resolve.c, but I want to split sg_resolve.c into individual
 * classes and these need to be changed to be not static and have
 * proper private prototypes defined for them.  I didn't take time
 * to do that right now.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__CHOICE__INLINE1_H
#define H_SG_WC_TX__RESOLVE__CHOICE__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Frees a choice.
 */
static void _choice__free(
	SG_context*         pCtx,   //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice //< [in] The choice to free.
	)
{
	if (pChoice == NULL)
	{
		return;
	}

	SG_PATHNAME_NULLFREE(pCtx, pChoice->pTempPath);
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pChoice->pValues, _value__sg_action__free);
	SG_VECTOR_NULLFREE(pCtx, pChoice->pLeafValues);
	SG_VECTOR_NULLFREE(pCtx, pChoice->pCollisionItems);
	SG_VECTOR_NULLFREE(pCtx, pChoice->pCycleItems);
	SG_VECTOR_NULLFREE(pCtx, pChoice->pPortabilityCollisionItems);
	SG_NULLFREE(pCtx, pChoice->szCycleHint);
	pChoice->pItem           = NULL;
	pChoice->eState          = SG_RESOLVE__STATE__COUNT;
	pChoice->eConflictCauses = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO;
	pChoice->bCollisionCause = SG_FALSE;
	pChoice->pMergeableResult = NULL;
	SG_NULLFREE(pCtx, pChoice);
}

/**
 * NULLFREE wrapper around _choice__free.
 */
#define _CHOICE__NULLFREE(pCtx, pChoice) _sg_generic_nullfree(pCtx, pChoice, _choice__free)

/**
 * Updates a choice's list of leaves by taking a newly merged value into consideration.
 *
 * The value's parents are removed from the list of leaves.  If either parent was
 * in the list of leaves, then the value itself is added as a leaf.
 */
static void _choice__update_leaves(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The choice to update the leaves of.
	SG_resolve__value*  pValue,    //< [in] The newly merged value.
	                               //<      Must already list pChoice as its owner.
	SG_bool             bOverwrite //< [in] If true, remove any existing leaf with the same label as the new value.
	                               //<      Use this when potentially replacing an old leaf value with a new one.
	)
{
	SG_uint32 uParentLeaves = 0u;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pValue);
	SG_ARGCHECK(pValue->pChoice == pChoice, pValue);

	// if we're potentially overwriting an old leaf value,
	// then remove any existing leaf with the same label as the new one
	if (bOverwrite == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_vector__remove__if(pCtx, pChoice->pLeafValues, _value__vector_predicate__match_label, (void*)pValue->szLabel, NULL, NULL)  );
	}

	// remove any leaf values that are the parents of the new value
	SG_ERR_CHECK(  SG_vector__remove__if(pCtx, pChoice->pLeafValues, _value__vector_predicate__match_parents, (void*)pValue, &uParentLeaves, NULL)  );

	// if any of the values parents were leaves, then add it to the list
	if (uParentLeaves > 0u)
	{
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pLeafValues, (void*)pValue, NULL)  );
	}

fail:
	return;
}

/**
 * Gets the only leaf in a choice.
 */
static void _choice__get_only_leaf(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to get the only leaf from.
	SG_resolve__value** ppLeaf   //< [out] The only leaf value in the choice.
	                             //<       NULL if the choice doesn't have exactly one leaf value.
	)
{
	SG_uint32          uCount = 0u;
	SG_resolve__value* pLeaf  = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppLeaf);

	SG_ERR_CHECK(  SG_vector__length(pCtx, pChoice->pLeafValues, &uCount)  );
#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "GetOnlyLeaf: [uCount %d] [state %s] [gid %s]\n",
							   uCount,
							   gaSerializedStateValues[pChoice->eState],
							   pChoice->pItem->szGid)  );
#endif
	if (uCount == 1u)
	{
		SG_ERR_CHECK(  SG_vector__get(pCtx, pChoice->pLeafValues, 0u, (void**)&pLeaf)  );
	}

	*ppLeaf = pLeaf;
	pLeaf = NULL;

fail:
	return;
}

/**
 * Checks to see if all of the values in a vector (presumably from a choice)
 * all have equivalent variant data.  If this occurs, the choice is unnecessary.
 */
static void _choice__values_equivalent(
	SG_context* pCtx,    //< [in] [out] Error and context info.
	SG_vector*  pValues, //< [in] Values to compare for equivalency.
	SG_bool*    pEqual   //< [out] True if all values in the vector are equivalent, false otherwise.
	)
{
	SG_bool   bEqual     = SG_TRUE;
	SG_uint32 uCount     = 0u;
	SG_uint32 uLeftIndex = 0u;

	SG_NULLARGCHECK(pValues);
	SG_NULLARGCHECK(pEqual);

	// run through each value in the vector
	// compare each value to all of the values after it
	SG_ERR_CHECK(  SG_vector__length(pCtx, pValues, &uCount)  );
	for (uLeftIndex = 0u; uLeftIndex < uCount - 1u; ++uLeftIndex)
	{
		SG_uint32 uRightIndex = 0u;

		// run through each value after the current one
		// and compare it to the current one
		for (uRightIndex = uLeftIndex + 1u; uRightIndex < uCount; ++uRightIndex)
		{
			SG_resolve__value* pLeftValue   = NULL;
			SG_resolve__value* pRightValue  = NULL;

			// get the two values to compare from the vector
			SG_ERR_CHECK(  SG_vector__get(pCtx, pValues, uLeftIndex, (void**)&pLeftValue)  );
			SG_ERR_CHECK(  SG_vector__get(pCtx, pValues, uRightIndex, (void**)&pRightValue)  );

			// compare the values
			SG_ERR_CHECK(  _value__equal_variants(pCtx, pLeftValue, pRightValue, &bEqual)  );

			// if they're not equal, then we can skip the rest of the checks
			if (bEqual == SG_FALSE)
			{
				break;
			}
		}

		// if any were not equal, then we can skip the rest of the checks
		if (bEqual == SG_FALSE)
		{
			break;
		}
	}

	// return whatever we ended up with
	*pEqual = bEqual;

fail:
	return;
}

/**
 * Allocates all of a choice's values that are specified in a merge issue.
 */
static void _choice__alloc__changeset_values(
	SG_context*         pCtx,   //< [in] [out] Error and context info.
	const SG_vhash*     pIssue, //< [in] The issue we're allocating from.
	SG_resolve__choice* pChoice //< [in] The choice we're allocating within.
	)
{
	SG_vhash*          pInputs                = NULL;
	SG_resolve__value* pValue                 = NULL;
	SG_string*         sLabel                 = NULL;
	SG_bool bHasMnemonic;

	// check the arguments
	SG_NULLARGCHECK(pIssue);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pChoice->pValues);
	SG_NULLARGCHECK(pChoice->pLeafValues);

	// get the list of input changeset data from the issue
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pIssue, gszIssueKey_Inputs, &pInputs)  );

	// create a value for the ancestor changeset, if the item has one

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pInputs, gszMnemonic_Ancestor, &bHasMnemonic)  );
	if (bHasMnemonic)
	{
		SG_ERR_CHECK(  _value__alloc__changeset__mnemonic(pCtx, pChoice, SG_RESOLVE__LABEL__ANCESTOR,
														  pInputs, gszMnemonic_Ancestor, &pValue)  );
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );
		pValue = NULL;
	}

	// create a value for each parent changeset
	SG_ERR_CHECK(  _value__alloc__changeset__mnemonic(pCtx, pChoice, SG_RESOLVE__LABEL__BASELINE,
													  pInputs, gszMnemonic_Baseline, &pValue)  );
	// if we got a value, append it to the choice's lists
	if (pValue != NULL)
	{
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pLeafValues, (void*)pValue, NULL)  );
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );
		pValue = NULL;
	}

	SG_ERR_CHECK(  _generate_indexed_label(pCtx, pChoice, SG_RESOLVE__LABEL__OTHER, &sLabel)  );
	SG_ERR_CHECK(  _value__alloc__changeset__mnemonic(pCtx, pChoice, SG_string__sz(sLabel),
													  pInputs, gszMnemonic_Other, &pValue)  );
	// if we got a value, append it to the choice's lists
	if (pValue != NULL)
	{
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pLeafValues, (void*)pValue, NULL)  );
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );
		pValue = NULL;
	}

fail:
	_VALUE__NULLFREE(pCtx, pValue);
	SG_STRING_NULLFREE(pCtx, sLabel);
	return;
}

/**
 * Allocates a value to represent an automerge result, if there is one.
 *
 * Note: Right now the only conflict that automerges is a divergent file edit.
 *       Right now, merge issues only support data for 2-way file edit merges.
 *       Therefore, this function currently never creates more than one automerge value.
 *       Once N-way file edit merges are supported, it could conceivably create a whole cascade of automerge values.
 */
static void _choice__alloc__automerge_values(
	SG_context*         pCtx,   //< [in] [out] Error and context info.
	const SG_vhash*     pIssue, //< [in] The issue we're allocating from.
	SG_resolve__choice* pChoice //< [in] The choice we're allocating within.
	)
{
	SG_string*         sLabel    = NULL;
	SG_resolve__value* pValue    = NULL;
	SG_pathname*       pTempFile = NULL;
	char*              szTool    = NULL;

	SG_NULLARGCHECK(pIssue);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pChoice->pValues);
	SG_NULLARGCHECK(pChoice->pLeafValues);

	// no automerge value will exist unless the issue contains a divergent file edit
	if ((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK) != 0)
	{
		SG_int32 eToolResult = SG_FILETOOL__RESULT__COUNT;

		// check exactly what happened when the automerge was run
		if ((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT) != 0)
		{
			eToolResult = SG_MERGETOOL__RESULT__CONFLICT;
		}
		else if ((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_ERROR) != 0)
		{
			eToolResult = SG_FILETOOL__RESULT__FAILURE;
		}
		else if ((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK) != 0)
		{
			eToolResult = SG_FILETOOL__RESULT__SUCCESS;
		}
		// other DIVERGENT_FILE_EDIT__* flags indicate that no automerge was attempted

		// create the value, if necessary
		if (eToolResult != SG_FILETOOL__RESULT__COUNT)
		{
			SG_uint32          uCount         = 0u;
			SG_resolve__value* pBaseline      = NULL;
			SG_resolve__value* pOther         = NULL;
			const char*        szTempRepoPath = NULL;
			const char*        szTempTool     = NULL;
			SG_vhash*          pvhOutput      = NULL;

			// get the number of values in the choice's vector
			SG_ERR_CHECK(  SG_vector__length(pCtx, pChoice->pValues, &uCount)  );
			SG_ARGCHECK(uCount == 3u, pValues);

			// get the baseline and other values
			SG_ERR_CHECK(  SG_vector__get(pCtx, pChoice->pValues, 1u, (void**)&pBaseline)  );
			SG_ERR_CHECK(  SG_vector__get(pCtx, pChoice->pValues, 2u, (void**)&pOther)  );

			// build a label
			SG_ERR_CHECK(  _generate_indexed_label(pCtx, pChoice, SG_RESOLVE__LABEL__AUTOMERGE, &sLabel)  );

			// get repo-path of the TEMP file that MERGE created containing the result of the merge.
			// convert it from a repo path to an absolute one.
			// since this is a TEMP, it is safe to use SG_pathname__ and SG_fsobj__ routines on it.
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pIssue, gszIssueKey_Output, &pvhOutput)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhOutput, gszIssueKey_Output_TempRepoPath, &szTempRepoPath)  );
			SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx,
																   pChoice->pItem->pResolve->pWcTx->pDb,
																   szTempRepoPath,
																   &pTempFile)  );

			// get the tool name from the merge issue and duplicate it
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pIssue, gszIssueKey_File_AutomergeTool, &szTempTool)  );
			SG_ERR_CHECK(  SG_strdup(pCtx, szTempTool, &szTool)  );

			// allocate the value
			SG_ERR_CHECK(  _value__alloc__merge(pCtx,
												pChoice,
												&sLabel,
												pTempFile,
												pBaseline,
												pOther,
												SG_TRUE,
												&szTool,
												eToolResult,
												&pValue)  );

			// if the value represents a successful merge
			// then it replaces its parents in the list of leaves
			if (eToolResult == SG_FILETOOL__RESULT__SUCCESS)
			{
				SG_ERR_CHECK(  _choice__update_leaves(pCtx, pChoice, pValue, SG_FALSE)  );
			}

			// add the new value to the vector
			SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );
			pValue = NULL;
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, sLabel);
	SG_PATHNAME_NULLFREE(pCtx, pTempFile);
	SG_NULLFREE(pCtx, szTool);
	_VALUE__NULLFREE(pCtx, pValue);
	return;
}

/**
 * Allocates all of a choice's values that are stored in saved choice data.
 */
static void _choice__alloc__saved_values(
	SG_context*         pCtx,   //< [in] [out] Error and context info.
	const SG_vhash*     pSaved, //< [in] The saved choice data to load values from.
	SG_resolve__choice* pChoice //< [in] The choice we're allocating within.
	)
{
	SG_varray*         pValues   = NULL;
	SG_resolve__value* pValue    = NULL;
	SG_bool            bOwnValue = SG_FALSE;

	SG_NULLARGCHECK(pSaved);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pChoice->pValues);
	SG_NULLARGCHECK(pChoice->pLeafValues);

	// get the list of values from the saved data and iterate through them
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pSaved, gszChoiceKey_Values, &pValues)  );
	if (pValues != NULL)
	{
		SG_uint32 uCount = 0u;
		SG_uint32 uIndex = 0u;

		SG_ERR_CHECK(  SG_varray__count(pCtx, pValues, &uCount)  );
		for (uIndex = 0u; uIndex < uCount; ++uIndex)
		{
			SG_vhash* pHash  = NULL;

			// get the current value hash
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pValues, uIndex, &pHash)  );

			// load a value from the hash
			SG_ERR_CHECK(  _value__alloc__saved(pCtx, pHash, pChoice, &pValue)  );
			bOwnValue = SG_TRUE;

			// add the value to the choice's list
			SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );
			bOwnValue = SG_FALSE;

			// if the value has merge parents, then we need to update the choice's leaves
			if (pValue->pBaselineParent != NULL && pValue->pOtherParent != NULL)
			{
				SG_ERR_CHECK(  _choice__update_leaves(pCtx, pChoice, pValue, SG_FALSE)  );
			}
		}
	}

fail:
	if (bOwnValue == SG_TRUE)
	{
		_VALUE__NULLFREE(pCtx, pValue);
	}
	return;
}

/**
 * Allocates a choice's working value.
 */
static void _choice__alloc__working_values(
	SG_context*                    pCtx,   //< [in] [out] Error and context info.
	SG_resolve__choice*            pChoice //< [in] The choice we're allocating within.
	)
{
	SG_resolve__value* pValue = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pChoice->pValues);
	SG_NULLARGCHECK(pChoice->pLeafValues);

	// allocate a working value
	SG_ERR_CHECK(  _value__alloc__working(pCtx, pChoice, &pValue)  );
	if (pValue != NULL)
	{
		// check if we need to do anything with the choice's leaves
		if (
			((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD) != 0)
			|| ((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__NO_RULE) != 0)
			)
		{
			// If the working value was not created by an automerge,
			// and the choice would end up being mergeable,
			// then the value effectively came from the baseline's value.
			// This case occurs when merge ran into a situation where it
			// would like to merge two values but couldn't for some reason,
			// usually because no rule specifying a mergetool is defined.
			// Therefore, if the working value is not from an automerge
			// we want to replace the baseline value with the working value
			// in the list of leaves.  That way the user will only need to
			// run a single merge to get the baseline, other, and working
			// changes (since the working value started as the baseline value,
			// the working value contains the baseline changes by default).
			// If the choice is non-mergeable, the entire list of leaves will
			// be going away anyway, so it doesn't hurt in that case.
			SG_resolve__value* pBaseline = NULL;

			// find the baseline value and remove it from the leaves
			SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__BASELINE, 0u, &pBaseline)  );
			SG_ERR_CHECK(  SG_vector__remove__value(pCtx, pChoice->pLeafValues, pBaseline, NULL, NULL)  );

			// add the working value to the leaves
			SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pLeafValues, (void*)pValue, NULL)  );
		}
		else if ((pChoice->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK) == 0)
		{
			// If the automatic merge performed by merge was unsuccessful, then we
			// need to check if the file has been modified in the working copy.  This
			// doesn't matter if the automerge succeeded, because we don't care if
			// it's been modified after the file is resolved, only before.
			// We'll check if the file is modified in the working copy by checking if
			// the working value's data HID is different than any other value's.
			// If that turns out to be the case, then the working value should be a
			// leaf, because it contains changes that the user has made directly in
			// the working directory that aren't accounted for in any of the other
			// values.
			void* pMatch = NULL;

			// check if any other values have the working value's data
			SG_ERR_CHECK(  SG_vector__find__first(pCtx, pChoice->pValues, _value__vector_predicate__match_data, pValue, NULL, &pMatch)  );
			if (pMatch == NULL)
			{
				// didn't find any, add the working value to the leaves
				SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pLeafValues, (void*)pValue, NULL)  );
			}
		}

		// append the working value to the choice's value list
		SG_ERR_CHECK(  SG_vector__append(pCtx, pChoice->pValues, (void*)pValue, NULL)  );
		pValue = NULL;
	}

fail:
	_VALUE__NULLFREE(pCtx, pValue);
	return;
}

/**
 * Allocates and populates a choice of the given type from an issue, if necessary.
 */
static void _choice__alloc(
	SG_context*                    pCtx,    //< [in] [out] Error and context info.
	SG_resolve__item*              pItem,   //< [in] The item that will own the choice.
	SG_resolve__state              eState   //< [in] The type of choice to create.
	)
{
	SG_resolve__choice* pChoice         = NULL;
	SG_int64            iConflictFlags  = 0;
	SG_int64            iPortabilityFlags = 0;
	SG_wc_port_flags    flagsPortability = 0;
	SG_uint32           uConflictFlags  = 0u;
	SG_uint32           uConflictCauses = 0u;
	SG_bool             bCollision      = SG_FALSE;
	SG_bool             bCollisionCause = SG_FALSE;
	SG_wc_port_flags    ePortCauses     = 0;
	SG_bool             bHaveThisChoice = SG_FALSE;
	const SG_vhash*     pIssue = pItem->pLVI->pvhIssue;
	const SG_vhash*     pSaved = NULL;

	SG_NULLARGCHECK(pIssue);
	SG_NULLARGCHECK(pItem);

	bHaveThisChoice = ((pItem->pLVI->statusFlags_x_xr_xu
						& (gaState2wcflags[eState].xr | gaState2wcflags[eState].xu)) != 0);

	// look up the issue data we need to determine cause
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pIssue, gszIssueKey_ConflictFlags, &iConflictFlags)  );
	uConflictFlags = (SG_uint32)iConflictFlags;
	SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pIssue, gszIssueKey_Collision, &bCollision)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pIssue, gszIssueKey_PortabilityFlags, &iPortabilityFlags)  );
	flagsPortability = (SG_wc_port_flags)iPortabilityFlags;

	// check if there are any causes for this type of choice
	uConflictCauses = uConflictFlags & gaConflictFlagMasks[eState];
	bCollisionCause = (bCollision == SG_TRUE && gaCollisionMasks[eState] == SG_TRUE) ? SG_TRUE : SG_FALSE;
	ePortCauses     = (flagsPortability & gaPortFlagMasks[eState]);

#if TRACE_WC_RESOLVE
	{
		const char * pszGid;
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pIssue, gszIssueKey_Gid, &pszGid)  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "ChoiceAlloc: [%s][%s][%d][%d,%d,%d]\n",
								   pszGid,
								   gaSerializedStateValues[eState],
								   bHaveThisChoice,
								   (uConflictCauses != 0), (bCollisionCause), (ePortCauses != 0))  );
	}
#endif

	if (bHaveThisChoice)
	{
		SG_bool     bExists           = SG_FALSE;
		SG_bool     bValuesEquivalent = SG_FALSE;
		const char* szResultLabel     = NULL;

		// allocate the choice and set its basic info
		SG_ERR_CHECK(  SG_alloc1(pCtx, pChoice)  );
		pChoice->pItem           = pItem;
		pChoice->eState          = eState;
		pChoice->eConflictCauses = uConflictCauses;
		pChoice->bCollisionCause = bCollisionCause;
		pChoice->ePortCauses     = ePortCauses;
		pChoice->pTempPath       = NULL;
		pChoice->pValues         = NULL;
		pChoice->pLeafValues     = NULL;
		pChoice->pMergeableResult = NULL;
		pChoice->pCollisionItems = NULL;
		pChoice->pCycleItems     = NULL;
		pChoice->pPortabilityCollisionItems = NULL;
		pChoice->szCycleHint     = NULL;

		// if the issue has a path for temporary files, retrieve it
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pIssue, gszIssueKey_TempPath, &bExists)  );
		if (bExists == SG_TRUE)
		{
			const char* szTempPath = NULL;

			// This key has the repo-path to a per-item temp directory.

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pIssue, gszIssueKey_TempPath, &szTempPath)  );
			SG_ERR_CHECK(  sg_wc_db__path__sz_repopath_to_absolute(pCtx,
																   pItem->pResolve->pWcTx->pDb,
																   szTempPath,
																   &pChoice->pTempPath)  );
		}

		// if there's a cycle involved and the issue contains a hint, retrieve it
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pIssue, gszIssueKey_Cycle_Hint, &bExists)  );
		if (bExists == SG_TRUE && ((uConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE) != 0))
		{
			const char* szCycleHint = NULL;

			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pIssue, gszIssueKey_Cycle_Hint, &szCycleHint)  );
			SG_ERR_CHECK(  SG_strdup(pCtx, szCycleHint, &pChoice->szCycleHint)  );
		}

		// allocate our value vectors
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pChoice->pValues, 4u)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pChoice->pLeafValues, 4u)  );

		// create values associated with changesets, as specified in the merge issue
		SG_ERR_CHECK(  _choice__alloc__changeset_values(pCtx, pIssue, pChoice)  );

		// check if all the values created so far have the same data, in which case the choice is unnecessary
		// Note that we're doing this while only the changeset values have been
		// allocated, because those are the only ones we want to check.
		SG_ERR_CHECK(  _choice__values_equivalent(pCtx, pChoice->pValues, &bValuesEquivalent)  );
#if TRACE_WC_RESOLVE
		{
			SG_uint32 nrValues;
			SG_ERR_CHECK(  SG_vector__length(pCtx, pChoice->pValues, &nrValues)  );
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "ChoiceAlloc: .... [nrValues %d][bEquiv %d]\n",
									   nrValues, bValuesEquivalent)  );
		}
#endif
		if (bValuesEquivalent == SG_TRUE)
		{
			// This should only ever occur with collisions, because they can be caused by
			// add/add situations or various combinations of moves and renames.  Therefore, we
			// need to try setting up each potential choice (existence, location, and name) and then
			// abort here if it turns out that the choice is unnecessary, which would happen if the
			// collision wasn't actually caused by a particular action (for instance, the location
			// choice would be unneeded if the collision was caused entirely by renames, and we'll
			// detect that here because the attempted location choice will have all equivalent values).
			if (bCollisionCause || ePortCauses)
			{
#if TRACE_WC_RESOLVE
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
										   "ChoiceAlloc: .... discarding\n")  );
#endif
				_CHOICE__NULLFREE(pCtx, pChoice);
			}
			else
			{
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "All choice values equivalent for item.")  );
			}
		}
		else
		{
#if TRACE_WC_RESOLVE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "ChoiceAlloc: .... registering\n")  );
#endif
			// create any values that were automerged
			SG_ERR_CHECK(  _choice__alloc__automerge_values(pCtx, pIssue, pChoice)  );

			// if we have saved resolve data, load the values from it
			// With the changes for W1849 we only do this for file contents.
			if ((eState == SG_RESOLVE__STATE__CONTENTS)
				&& (pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
				&& (pItem->pLVI->pvhSavedResolutions))
			{
				pSaved = pItem->pLVI->pvhSavedResolutions;
				SG_ERR_CHECK(  _choice__alloc__saved_values(pCtx, pSaved, pChoice)  );
			}

			// create a value for the working directory
			// Note: This has to come after we allocate saved values in order for the
			//       leaf nodes to be properly kept up to date.
			SG_ERR_CHECK(  _choice__alloc__working_values(pCtx, pChoice)  );

			// if we have a result value label saved, get a pointer to the actual value
			// Note: This has to come after we allocate the working value(s), because
			//       the saved result label might refer to a working value that's not
			//       saved.
			if (pSaved != NULL)
			{
				SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pSaved, gszChoiceKey_Result, &szResultLabel)  );
				if (szResultLabel != NULL)
				{
					SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, szResultLabel, 0u,
																		&pChoice->pMergeableResult)  );
				}
			}

			// check if the choice is mergeable
			if ((uConflictCauses & gaMergeableMasks[pItem->eType]) == 0)
			{
				// this choice isn't mergeable, free the leaf values
				// a NULL leaf value list indicates later that this choice isn't mergeable
				SG_VECTOR_NULLFREE(pCtx, pChoice->pLeafValues);
				SG_ASSERT(pChoice->pLeafValues == NULL);
			}
			else if (pChoice->pMergeableResult == NULL)
			{
				// Check if we should mark the choice as already resolved.
				// We'll do that if the choice has a single leaf value that was from an automerge.
				// What this would mean is that "merge" did the resolve work for us.
				//
				// NOTE 2012/08/29 With the changes for W1849 we only set the
				// NOTE            pMergeableResult so that we can remember the
				// NOTE            name of the value (baseline, merge3, etc) to
				// NOTE            display it to the user.  The resolved/unresolved
				// NOTE            state of this choice is indicated by the x_xr_xu
				// NOTE            bits.

				SG_resolve__value* pLeaf = NULL;

				// get the single leaf and see if it's an automerge
				SG_ERR_CHECK(  _choice__get_only_leaf(pCtx, pChoice, &pLeaf)  );
				if (pLeaf != NULL && pLeaf->bAutoMerge == SG_TRUE)
				{
					// that leaf will be our resolution
					pChoice->pMergeableResult = pLeaf;
				}
			}

			// if the choice is mergeable
			// find an appropriate base/parent for the current working value
			if (pChoice->pLeafValues != NULL)
			{
				SG_resolve__value* pWorking = NULL;

				// find the current working value
				// (the only reason for us not to have one is if the file has been deleted from the working copy)
				SG_ERR_CHECK(  SG_resolve__choice__check_value__changeset__mnemonic(pCtx, pChoice, gszMnemonic_Working, &pWorking)  );
				if (pWorking != NULL)
				{
					// it can't have a parent yet because we're just now figuring out what it's parent should be
					SG_ASSERT(pWorking->pBaselineParent == NULL);

					// check if we have a current result
					if (pChoice->pMergeableResult != NULL)
					{
						// we have a current result
						// that's the last value to be copied into the working folder by us
						// so that's the working value's base/parent value
						pWorking->pBaselineParent = pChoice->pMergeableResult;
					}
					else
					{
						// no current result
						SG_resolve__value* pParent = NULL;

						// check if there's an automatic merge value
						SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__AUTOMERGE, -1, &pParent)  );
						if (pParent != NULL)
						{
							// the automatic merge created by merge
							// is the last thing that was copied into the working folder
							SG_ASSERT(pParent->bAutoMerge == SG_TRUE);
							pWorking->pBaselineParent = pParent;
						}
						else
						{
							// if nothing else
							// the working value must be based on the baseline value
							SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__BASELINE, 0, &pParent)  );
							pWorking->pBaselineParent = pParent;
						}
					}
				}
			}

			// if the choice is mergeable and has already been resolved
			// then we no longer care to track what its leaves are
			// because that information is only helpful to arrive at a result
			if (pChoice->pLeafValues != NULL && pChoice->pMergeableResult != NULL)
			{
				// clear the vector of leaves, but don't free it
				// a NULL vector would mean a non-mergeable choice
				// we want to know the choice is mergeable
				// we just don't want it to have leaves anymore
				SG_ERR_CHECK(  SG_vector__clear(pCtx, pChoice->pLeafValues)  );
			}
		}
	}
	else
	{
#if TRACE_WC_RESOLVE
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
										   "ChoiceAlloc: .... skipping\n")  );
#endif
	}

	pItem->aChoices[eState] = pChoice;
	return;

fail:
	_CHOICE__NULLFREE(pCtx, pChoice);
}


/**
 * Creates a hash that contains the choice data that is unique to resolve.
 * Data derived from merge issues or pendingtree is not included.
 * If the given choice can be entirely constructed from merge/pendingtree data,
 * then no hash is generated at all.
 *
 * NOTE 2012/08/29 With the changes for W1894 this should only be called
 * NOTE            for the mergeable file content data.
 * 
 */
static void _choice__build_resolve_vhash(
	SG_context*         pCtx,    //< [in] [out] Error and context data.
	SG_resolve__choice* pChoice, //< [in] The choice to build a vhash from.
	SG_vhash**          ppHash   //< [out] The built vhash, or NULL if none is required.
	)
{
	SG_uint32  uCount  = 0u;
	SG_vhash*  pHash   = NULL;
	SG_varray* pValues = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppHash);

	SG_ASSERT(  ((pChoice->eState == SG_RESOLVE__STATE__CONTENTS)
				 && (pChoice->pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE))  );

	// if there are any values for this choice, build a hash for it
	SG_ERR_CHECK(  SG_vector__length(pCtx, pChoice->pValues, &uCount)  );
	if (uCount > 0u)
	{
		// allocate a vhash to store the choice's properties in
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pHash)  );

		// add the choice's basic properties to the hash
		if (pChoice->pMergeableResult != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, gszChoiceKey_Result, pChoice->pMergeableResult->szLabel)  );
		}

		// add the choice's values to a varray
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pValues)  );
		SG_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pChoice, SG_FALSE, _value__resolve_action__add_resolve_vhash_to_varray, (void*)pValues)  );

		// if any values were saved to the array, add it to the hash (the hash takes ownership of the array)
		SG_ERR_CHECK(  SG_varray__count(pCtx, pValues, &uCount)  );
		if (uCount > 0u)
		{
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pHash, gszChoiceKey_Values, &pValues)  );
			SG_ASSERT(pValues == NULL);
		}
	}

	// return the hash
	*ppHash = pHash;
	pHash = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pHash);
	SG_VARRAY_NULLFREE(pCtx, pValues);
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__CHOICE__INLINE1_H
