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
 * @file sg_wc_tx__resolve__choice__inline2.h
 *
 * @details SG_resolve__choice__ routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__CHOICE__INLINE2_H
#define H_SG_WC_TX__RESOLVE__CHOICE__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////


void SG_resolve__choice__get_item(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	SG_resolve__item**  ppItem
	)
{
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppItem);

	*ppItem = pChoice->pItem;

fail:
	return;
}

void SG_resolve__choice__get_state(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	SG_resolve__state*  pState
	)
{
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pState);

	*pState = pChoice->eState;

fail:
	return;
}

void SG_resolve__choice__get_causes(
	SG_context*                       pCtx,
	SG_resolve__choice*               pChoice,
	SG_mrg_cset_entry_conflict_flags* pConflicts,
	SG_bool*                          pCollision,
	SG_vhash**                        ppDescriptions
	)
{
	SG_vhash* pDescriptions = NULL;

	SG_NULLARGCHECK(pChoice);

	if (pConflicts != NULL)
	{
		*pConflicts = pChoice->eConflictCauses;
	}
	if (pCollision != NULL)
	{
		*pCollision = pChoice->bCollisionCause;
	}
	if (ppDescriptions != NULL)
	{
		SG_uint32 uIndex = 0u;

		SG_VHASH__ALLOC(pCtx, &pDescriptions);

		// run through each possible cause
		for (uIndex = 0u; uIndex < SG_NrElements(gaCauseData); ++uIndex)
		{
			// if this choice has the current cause
			// then add the cause's name/description to the hash
			if (
				   ((pChoice->eConflictCauses & gaCauseData[uIndex].eConflictCauses) != 0)
				|| (
					   pChoice->bCollisionCause == SG_TRUE
					&& gaCauseData[uIndex].bCollisionCause == SG_TRUE
					)
				|| ((pChoice->ePortCauses != 0) && gaCauseData[uIndex].bPortabilityCause)
				)
			{
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pDescriptions, gaCauseData[uIndex].szName, gaCauseData[uIndex].szDescription)  );
			}
		}

		// return the generated hash
		*ppDescriptions = pDescriptions;
		pDescriptions = NULL;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pDescriptions);
	return;
}

void SG_resolve__choice__get_mergeable(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	SG_bool*            pMergeable
	)
{
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pMergeable);

	if (pChoice->pLeafValues != NULL)
	{
		*pMergeable = SG_TRUE;
	}
	else
	{
		*pMergeable = SG_FALSE;
	}

fail:
	return;
}

void SG_resolve__choice__get_resolved(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	SG_bool * pbChoiceResolved
	)
{
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pbChoiceResolved);

	*pbChoiceResolved = ((pChoice->pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)
						 || (pChoice->pItem->pLVI->statusFlags_x_xr_xu & gaState2wcflags[pChoice->eState].xr));

fail:
	return;
}

void SG_resolve__choice__foreach_colliding_item(
	SG_context*                 pCtx,
	SG_resolve__choice*         pChoice,
	SG_resolve__item__callback* fCallback,
	void*                       pUserData
	)
{
	_item__vector_action__call_resolve_action__data cData;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(fCallback);

	if (pChoice->pCollisionItems == NULL)
	{
		return;
	}

	cData.fCallback = fCallback;
	cData.pUserData = pUserData;
	cData.bContinue = SG_TRUE;
	SG_ERR_CHECK(  SG_vector__foreach(pCtx, pChoice->pCollisionItems, _item__vector_action__call_resolve_action, &cData)  );

fail:
	return;
}

void SG_resolve__choice__foreach_portability_colliding_item(
	SG_context*                 pCtx,
	SG_resolve__choice*         pChoice,
	SG_resolve__item__callback* fCallback,
	void*                       pUserData
	)
{
	_item__vector_action__call_resolve_action__data cData;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(fCallback);

	if (pChoice->pPortabilityCollisionItems == NULL)
	{
		return;
	}

	cData.fCallback = fCallback;
	cData.pUserData = pUserData;
	cData.bContinue = SG_TRUE;
	SG_ERR_CHECK(  SG_vector__foreach(pCtx, pChoice->pPortabilityCollisionItems, _item__vector_action__call_resolve_action, &cData)  );

fail:
	return;
}

void SG_resolve__choice__get_cycle_info(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	const char**        ppPath
	)
{
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppPath);

	*ppPath = pChoice->szCycleHint;

fail:
	return;
}

void SG_resolve__choice__foreach_cycling_item(
	SG_context*                 pCtx,
	SG_resolve__choice*         pChoice,
	SG_resolve__item__callback* fCallback,
	void*                       pUserData
	)
{
	_item__vector_action__call_resolve_action__data cData;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(fCallback);

	if (pChoice->pCycleItems == NULL)
	{
		return;
	}

	cData.fCallback = fCallback;
	cData.pUserData = pUserData;
	cData.bContinue = SG_TRUE;
	SG_ERR_CHECK(  SG_vector__foreach(pCtx, pChoice->pCycleItems, _item__vector_action__call_resolve_action, &cData)  );

fail:
	return;
}

void SG_resolve__choice__check_value__label(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	const char*         szLabel,
	SG_int32            iIndex,
	SG_resolve__value** ppValue
	)
{
	SG_resolve__value* pValue = NULL;
	SG_string*         sLabel = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(szLabel);
	SG_NULLARGCHECK(ppValue);

	// if they specified a negative index, convert it to a positive one
	if (iIndex < 0)
	{
		// figure out what the highest valid index for this label is
		SG_int32 iMaxIndex = 0;
		do
		{
			++iMaxIndex;
			SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szLabel, iMaxIndex, &pValue)  );
		}
		while (pValue != NULL);

		// convert the negative index into the equivalent positive one
		iIndex = iMaxIndex + iIndex;
		// if we ended up negative again, then the index was out of range
		// we'll check for that later
	}

	// if they specified a valid index, build the actual label
	// We ignore 0 because that signifies that the label is literal.
	// We ignore 1 because the first value in an indexed series doesn't
	// actually have a suffix of 1.  This makes for a friendlier user
	// experience in the 99% of cases where there is only a single value
	// with any given base name.
	if (iIndex > 1)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sLabel)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, sLabel, gszIndexedLabelFormat, szLabel, iIndex)  );
		szLabel = SG_string__sz(sLabel);
	}

	// find the value with the actual label
	// if our index is negative at this point, then it's out of range
	if (iIndex >= 0)
	{
		SG_ERR_CHECK(  SG_vector__find__first(pCtx, pChoice->pValues, _value__vector_predicate__match_label, (void*)szLabel, NULL, (void**)&pValue)  );
	}

	// return whatever we found
	*ppValue = pValue;
	pValue = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sLabel);
	return;
}

void SG_resolve__choice__get_value__label(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	const char*         szLabel,
	SG_int32            iIndex,
	SG_resolve__value** ppValue
	)
{
	SG_resolve__value* pValue = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(szLabel);
	SG_NULLARGCHECK(ppValue);

	SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szLabel, iIndex, &pValue)  );
	if (pValue == NULL)
	{
		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "resolve value by label/index: %s/%d", szLabel, iIndex));
	}

	*ppValue = pValue;
	pValue = NULL;

fail:
	return;
}

void SG_resolve__choice__check_value__changeset__mnemonic(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	const char*         pszMnemonic,
	SG_resolve__value** ppValue
	)
{
	SG_resolve__value*   pValue     = NULL;
	SG_vector_predicate* fPredicate = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pszMnemonic);
	SG_NULLARGCHECK(ppValue);

	// check for the working case
	if (strcmp(pszMnemonic, gszMnemonic_Working) == 0)
	{
		// If they want the value associated with the "working" changeset,
		// that means they want the working value that's currently "live",
		// as opposed to a working value that's been saved.  Use a predicate
		// that will only match a live working value.  This predicate doesn't
		// use the "user data" parameter, so passing a Mnemonic/HID won't harm anything.
		fPredicate = _value__vector_predicate__match_live_working;
	}
	else
	{
		// use a predicate that will match any value with the given changeset mnemonic
		fPredicate = _value__vector_predicate__match_changeset__mnemonic;
	}

	SG_ERR_CHECK(  SG_vector__find__first(pCtx, pChoice->pValues, fPredicate, (void*)pszMnemonic, NULL, (void**)&pValue)  );

	*ppValue = pValue;
	pValue = NULL;

fail:
	return;
}

void SG_resolve__choice__get_value__changeset__mnemonic(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	const char*         pszMnemonic,
	SG_resolve__value** ppValue
	)
{
	SG_resolve__value* pValue = NULL;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pszMnemonic);
	SG_NULLARGCHECK(ppValue);

	SG_ERR_CHECK(  SG_resolve__choice__check_value__changeset__mnemonic(pCtx, pChoice, pszMnemonic, &pValue)  );
	if (pValue == NULL)
	{
		const char* szState = NULL;

		SG_ERR_CHECK(  SG_resolve__state__get_info(pCtx, pChoice->eState, &szState, NULL, NULL)  );
		if (strcmp(pszMnemonic, gszMnemonic_Working) == 0)
		{
			pszMnemonic = "(working/live)";		// this is only for the error message below.
		}

		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "resolve value by choice/changeset: %s/%s", szState, pszMnemonic));
	}

	*ppValue = pValue;
	pValue = NULL;

fail:
	return;
}

void SG_resolve__choice__get_value__accepted(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	SG_resolve__value** ppValue
	)
{
	SG_UNUSED(pCtx);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppValue);

	// NOTE 2012/08/29 With the changes for W1849 this value
	// NOTE            is only defined for file-contents.
	*ppValue = pChoice->pMergeableResult;

fail:
	return;
}

void SG_resolve__choice__foreach_value(
	SG_context*                  pCtx,
	SG_resolve__choice*          pChoice,
	SG_bool                      bOnlyLeaves,
	SG_resolve__value__callback* fCallback,
	void*                        pUserData
	)
{
	SG_vector*                                       pVector = NULL;
	_value__vector_action__call_resolve_action__data cData;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(fCallback);

	cData.fCallback = fCallback;
	cData.pUserData = pUserData;
	cData.bContinue = SG_TRUE;

	if (bOnlyLeaves == SG_TRUE)
	{
		pVector = pChoice->pLeafValues;
	}
	else
	{
		pVector = pChoice->pValues;
	}

	SG_ERR_CHECK(  SG_vector__foreach(pCtx, pVector, _value__vector_action__call_resolve_action, &cData)  );

fail:
	return;
}

void SG_resolve__choice_callback__count(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	void*               pUserData,
	SG_bool*            pContinue
	)
{
	SG_uint32* pCount = (SG_uint32*)pUserData;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	*pCount += 1u;
	*pContinue = SG_TRUE;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__CHOICE__INLINE2_H
