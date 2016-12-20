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
 * @file sg_wc_tx__resolve__item__inline1.h
 *
 * @details _item__ routines that I pulled from the PendingTree
 * version of sg_resolve.c.  These were considered STATIC PRIVATE
 * to sg_resolve.c, but I want to split sg_resolve.c into individual
 * classes and these need to be changed to be not static and have
 * proper private prototypes defined for them.  I didn't take time
 * to do that right now.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__ITEM__INLINE1_H
#define H_SG_WC_TX__RESOLVE__ITEM__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Frees an SG_resolve__item.
 */
static void _item__free(
	SG_context*       pCtx, //< [in] [out] Error and context info.
	SG_resolve__item* pItem //< [in] The item to free.
	)
{
	SG_uint32 uIndex = 0u;

	if (pItem == NULL)
	{
		return;
	}

	SG_NULLFREE(pCtx, pItem->szGid);
	SG_STRING_NULLFREE(pCtx, pItem->pStringGidDomainRepoPath);
	SG_NULLFREE(pCtx, pItem->szMergedName);
//	SG_NULLFREE(pCtx, pItem->szMergedContentHid);
	SG_NULLFREE(pCtx, pItem->szRestoreOriginalName);
	SG_NULLFREE(pCtx, pItem->szReferenceOriginalName);
	for (uIndex = 0u; uIndex < SG_RESOLVE__STATE__COUNT; ++uIndex)
	{
		_CHOICE__NULLFREE(pCtx, pItem->aChoices[uIndex]);
	}
	pItem->pResolve          = NULL;
	pItem->eType             = SG_TREENODEENTRY_TYPE__INVALID;
//	pItem->iMergedAttributes = 0;
	SG_NULLFREE(pCtx, pItem);
}

/**
 * NULLFREE wrapper around _item__free.
 */
#define _ITEM__NULLFREE(pCtx, pItem) _sg_generic_nullfree(pCtx, pItem, _item__free)

/**
 * Allocates an item from a merge issue.
 */
static void _item__alloc(
	SG_context*         pCtx,     //< [in] [out] Error and context info.
	SG_uint64           uiAliasGid,   //< [in] alias associated with GID of item.
	SG_resolve*         pResolve, //< [in] The resolve that will own the item.
	SG_resolve__item**  ppItem    //< [out] The created item.
	)
{
	SG_resolve__item*              pItem       = NULL;
	const char*                    szGid       = NULL;
	SG_int64                       iType       = 0;
	SG_uint32                      uIndex      = 0u;
	SG_vhash*                      pOutput     = NULL;
	const char*                    szName      = NULL;
//	const char*                    szHid       = NULL;
	const char * pszRestoreOriginalName = NULL;
	const char * pszReferenceOriginalName = NULL;
	SG_bool                        bFound;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(ppItem);

	// allocate the item
	SG_ERR_CHECK(  SG_alloc1(pCtx, pItem)  );
	// TODO 2012/04/03 BTW SG_alloc1() allocates zeroed memory,
	// TODO            so we don't really need the following.
	pItem->pResolve           = pResolve;
	pItem->szGid              = NULL;
	pItem->pStringGidDomainRepoPath    = NULL;
	pItem->eType              = SG_TREENODEENTRY_TYPE__INVALID;
	pItem->szMergedName       = NULL;
//	pItem->szMergedContentHid = NULL;
//	pItem->iMergedAttributes  = 0;
	pItem->szRestoreOriginalName     = NULL;
	pItem->szReferenceOriginalName     = NULL;
	for (uIndex = 0u; uIndex < SG_RESOLVE__STATE__COUNT; ++uIndex)
	{
		pItem->aChoices[uIndex] = NULL;
	}

	// find the pLVI for this item.  this makes things easier later.
	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx,
														 pResolve->pWcTx,
														 uiAliasGid,
														 &bFound,
														 &pItem->pLVI)  );
	if (!bFound)
	{
		SG_int_to_string_buffer bufui64;
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "resolve:_item__alloc: [alias %s]\n",
						 SG_uint64_to_sz(uiAliasGid, bufui64))  );
	}

	// get the item's GID and type from the issue.
	// NOTE 2012/04/17 historically (PT) we got the GID from the pvhIssue.
	// NOTE            with the WC code we could get it from the uiAliasGid.
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Gid, &szGid)  );
	SG_ERR_CHECK(  SG_strdup(pCtx, szGid, &pItem->szGid)  );

	// Construct a GID-domain extended repo-path to name the item
	// when talking to WC.  This avoids any problems with it
	// already and/or not currently existing and/or the temp
	// name that MERGE gave it when it set the DELETE-VS-* bit.
	SG_ERR_CHECK(  sg_wc_db__path__gid_to_gid_repopath(pCtx, pItem->szGid, &pItem->pStringGidDomainRepoPath)  );

	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Type, &iType)  );
	pItem->eType = (SG_treenode_entry_type)iType;

	// See if MERGE gave it a temp name that we should implicitly restore
	// when we resolve it.  We won't get this for DIVERGENT-RENAMES, but
	// should for other things like DELETE-vs-* conflicts.

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pItem->pLVI->pvhIssue, "restore_original_name", &pszRestoreOriginalName)  );
	if (pszRestoreOriginalName)
		SG_ERR_CHECK(  SG_strdup(pCtx, pszRestoreOriginalName, &pItem->szRestoreOriginalName)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pItem->pLVI->pvhIssue, "reference_original_name", &pszReferenceOriginalName)  );
	if (pszReferenceOriginalName)
		SG_ERR_CHECK(  SG_strdup(pCtx, pszReferenceOriginalName, &pItem->szReferenceOriginalName)  );

	// gather data from the item's merge output
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Output, &pOutput)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pOutput, gszIssueKey_Output_Name, &szName)  );
	SG_ERR_CHECK(  SG_strdup(pCtx, szName, &pItem->szMergedName)  );
//	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pOutput, gszIssueKey_Output_Contents, &szHid)  );
//	SG_ERR_CHECK(  SG_strdup(pCtx, szHid, &pItem->szMergedContentHid)  );
//	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pOutput, gszIssueKey_Output_Attributes, &pItem->iMergedAttributes)  );

	// NOTE 2012/08/29 With the changes for W1849 we only store
	// NOTE            info for file content values in what was
	// NOTE            the pvhSavedResolutions (aka pvhMergeableData).
	// NOTE
	// NOTE            For other choices, the x_xr_xu bits are sufficient.
	//
	// run through each possible type of choice and allocate one if necessary
	// and/or apply info that we saved last time.
	for (uIndex = 0u; uIndex < SG_RESOLVE__STATE__COUNT; ++uIndex)
	{
		SG_ERR_CHECK(  _choice__alloc(pCtx, pItem, (SG_resolve__state)uIndex)  );
	}

	// return the new item
	*ppItem = pItem;
	pItem = NULL;

fail:
	_ITEM__NULLFREE(pCtx, pItem);
	return;
}

/**
 * Tell WC about any changes/resolutions made to the
 * mergeable data for this item.
 *
 * This creates and writes a new pvhMergeableData for this item
 * and when appropriate updates the overall resolved state
 * associated with the ROW for this item in the Issue Table.
 *
 * Note that this is done *after* the pLVI has been changed
 * and the disk operations QUEUED.
 *
 * This used to be called _item__update_issue_resolved_status()
 * but has been changed for W1849 to only record the data for
 * the mergeable (file content) state.
 * 
 */
static void _item__save_mergeable_data(
	SG_context*       pCtx, //< [in] [out] Error and context info.
	SG_resolve__item* pItem //< [in] The item to update the old resolve status of.
	)
{
	SG_vhash * pvhMergeableData = NULL;

	SG_NULLARGCHECK(pItem);

	// NOTE 2012/08/29 With changes for W1849, we only want to
	// NOTE            include the info for mergeable data.
	// NOTE            We no longer need the per-choice resolved
	// NOTE            field since we have it in the x_xr_xu bits.
	// NOTE            We only want the details for file content
	// NOTE            (auto-merge/manual-merge/overwrite) decisions.

	if (pItem->aChoices[SG_RESOLVE__STATE__CONTENTS]
		&& (pItem->eType == SG_TREENODEENTRY_TYPE_REGULAR_FILE))
	{
		SG_ERR_CHECK(  _choice__build_resolve_vhash(pCtx,
													pItem->aChoices[SG_RESOLVE__STATE__CONTENTS],
													&pvhMergeableData)  );
	}

	SG_ERR_CHECK(  sg_wc_tx__queue__resolve_issue__sr(pCtx,
													  pItem->pResolve->pWcTx,
													  pItem->pLVI,
													  pItem->pLVI->statusFlags_x_xr_xu,
													  &pvhMergeableData)  );   // this gets stolen from us if all goes well.

fail:
	SG_VHASH_NULLFREE(pCtx, pvhMergeableData);
}

/**
 * Iterates over each choice in an item.
 */
static void _item__foreach_choice(
	SG_context*                   pCtx,            //< [in] [out] Error and context info.
	SG_resolve__item*             pItem,           //< [in] The item to iterate choices on.
	SG_bool                       bOnlyUnresolved, //< [in] Whether or not only unresolved choices should be iterated.
	SG_resolve__choice__callback* fCallback,       //< [in] The callback to call with each choice.
	void*                         pUserData,       //< [in] User data to pass to the callback.
	SG_bool*                      pContinue        //< [in] [out] Whether or not to continue iteration.
	)
{
	SG_int32 iIndex = 0;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(fCallback);
	SG_NULLARGCHECK(pContinue);

	for (iIndex = 0; iIndex < SG_RESOLVE__STATE__COUNT; ++iIndex)
	{
		SG_resolve__choice* pChoice = pItem->aChoices[iIndex];

		if (pChoice == NULL)
		{
			continue;
		}

		if (bOnlyUnresolved)
		{
			SG_bool bIsChoiceResolved = SG_FALSE;
			SG_ERR_CHECK(  SG_resolve__choice__get_resolved(pCtx, pChoice, &bIsChoiceResolved)  );
			if (bIsChoiceResolved)
			{
				continue;
			}
		}

		SG_ERR_CHECK(  fCallback(pCtx, pChoice, pUserData, pContinue)  );
		
		if (*pContinue == SG_FALSE)
		{
			break;
		}
	}

fail:
	return;
}

/**
 * An SG_free_callback that frees SG_resolve__item values.
 */
static void _item__sg_action__free(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pData //< [in] The SG_resolve__item to free.
	)
{
	SG_resolve__item* pItem  = (SG_resolve__item*)pData;
	_ITEM__NULLFREE(pCtx, pItem);
}

/**
 * An SG_vector_predicate that matches an SG_resolve__item's
 * szGid against the string given in the user data.
 */
static void _item__vector_predicate__match_gid(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the item being matched.
	void*       pValue,    //< [in] The item being matched.
	void*       pUserData, //< [in] The string to match the item's GID against.
	SG_bool*    pMatch     //< [out] Whether or not the item's GID matches.
	)
{
	SG_resolve__item* pItem      = (SG_resolve__item*)pValue;
	const char*       szMatchGid = (const char*)pUserData;
	const char*       szItemGid  = NULL;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pMatch);

	SG_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pItem, &szItemGid)  );

	if (SG_stricmp(szMatchGid, szItemGid) == 0)
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
 * An SG_vector_predicate that matches an SG_resolve__item's
 * resolve status against the given status.
 */
static void _item__vector_predicate__match_resolved(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the item being matched.
	void*       pValue,    //< [in] The item being matched.
	void*       pUserData, //< [in] Pointer to a bool resolved status to match.
	SG_bool*    pMatch     //< [out] Whether or not the item's resolved status matches.
	)
{
	SG_resolve__item* pItem          = (SG_resolve__item*)pValue;
	SG_bool*          pMatchResolved = (SG_bool*)pUserData;
	SG_bool           bItemResolved  = SG_FALSE;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pMatch);

	if (pItem->pLVI->statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__X__RESOLVED)
		bItemResolved = SG_TRUE;

	if (*pMatchResolved == bItemResolved)
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
 * Type of user data required by _item__vector_action__call_resolve_action.
 */
typedef struct
{
	SG_resolve__item__callback* fCallback; //< The item callback to call.
	void*                       pUserData; //< The user data to pass to the callback.
	SG_bool                     bContinue; //< Whether or not to actually call the callback.
}
_item__vector_action__call_resolve_action__data;

/**
 * An SG_vector_foreach_callback that calls an SG_resolve__item__callback.
 */
static void _item__vector_action__call_resolve_action(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_uint32   uIndex,   //< [in] The index of the item.
	void*       pValue,   //< [in] The item.
	void*       pUserData //< [in] An _item__vector_action__call_resolve_action__data structure.
	)
{
	SG_resolve__item*                                pItem = (SG_resolve__item*)pValue;
	_item__vector_action__call_resolve_action__data* pArgs = (_item__vector_action__call_resolve_action__data*)pUserData;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);

	if (pArgs->bContinue == SG_TRUE)
	{
		SG_ERR_CHECK(  pArgs->fCallback(pCtx, pItem, pArgs->pUserData, &pArgs->bContinue)  );
	}

fail:
	return;
}

/**
 * Type of user data required by _item__vector_action__foreach_choice.
 */
typedef struct
{
	SG_bool                       bOnlyUnresolved; //< Whether or not we should only iterate over unresolved choices.
	SG_resolve__state             eState;          //< The state of choice that we should iterate.
	                                               //< If SG_RESOLVE__STATE__COUNT, choices of all states should be iterated.
	SG_resolve__choice__callback* fCallback;       //< The choice callback to call on each choice.
	void*                         pUserData;       //< The user data to pass to the callback.
	SG_bool                       bContinue;       //< Whether or not to actually call the callback.
}
_item__vector_action__foreach_choice__data;

/**
 * An SG_vector_foreach_callback that iterates over the item's choices.
 */
static void _item__vector_action__foreach_choice(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_uint32   uIndex,   //< [in] The index of the item.
	void*       pValue,   //< [in] The item.
	void*       pUserData //< [in] An _item__vector_action__foreach_choice__data structure.
	)
{
	SG_resolve__item*                           pItem = (SG_resolve__item*)pValue;
	_item__vector_action__foreach_choice__data* pArgs = (_item__vector_action__foreach_choice__data*)pUserData;

	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);

	if (pArgs->bContinue == SG_TRUE)
	{
		if (pArgs->eState == SG_RESOLVE__STATE__COUNT)
		{
			SG_ERR_CHECK(  _item__foreach_choice(pCtx, pItem, pArgs->bOnlyUnresolved, pArgs->fCallback, pArgs->pUserData, &pArgs->bContinue)  );
		}
		else if (pItem->aChoices[pArgs->eState])
		{
			SG_bool bU = ((pItem->pLVI->statusFlags_x_xr_xu & gaState2wcflags[pArgs->eState].xu) != 0);
#if defined(DEBUG)
			SG_bool bR = ((pItem->pLVI->statusFlags_x_xr_xu & gaState2wcflags[pArgs->eState].xr) != 0);
			SG_ASSERT( (bR || bU) );
#endif
			if (!pArgs->bOnlyUnresolved || bU)
				SG_ERR_CHECK(  pArgs->fCallback(pCtx, pItem->aChoices[pArgs->eState], pArgs->pUserData, &pArgs->bContinue)  );
		}
	}

fail:
	return;
}

/**
 * Iterates a list of related item GIDs, finds the corresponding item for each
 * in an item vector, and adds each found item to a vector.
 */
static void _item__gather_items(
	SG_context*       pCtx,    //< [in] [out] Error and context info.
	SG_varray*        pGids,   //< [in] An array of item GIDs that are related to pItem.
	SG_vector*        pItems,  //< [in] An SG_vector of items to search for related ones by GID.
	SG_resolve__item* pItem,   //< [in] An item to ignore if found in the list of GIDs.
	                           //<      This item will never be added to either destination.
	SG_vector*        pRelated //< [in] A vector to add the related items to.
	)
{
	SG_uint32 uCount = 0u;
	SG_uint32 uIndex = 0u;

	SG_NULLARGCHECK(pGids);
	SG_NULLARGCHECK(pItems);
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pRelated);

	// run through each GID involved in the collision
	SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char*       szGid        = NULL;
		SG_resolve__item* pRelatedItem = NULL;

		// get the current GID
		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pGids, uIndex, &szGid)  );

		// find the item with this GID in the search vector
		SG_ERR_CHECK(  SG_vector__find__first(pCtx, pItems, _item__vector_predicate__match_gid, (void*)szGid, NULL, (void**)&pRelatedItem)  );
		SG_ASSERT(pRelatedItem != NULL);

		// add the found item to the destination vector(s)
		// unless we found ourselves (the current item may be in the collision list as well)
		if (pRelatedItem != pItem)
		{
			SG_ERR_CHECK(  SG_vector__append(pCtx, pRelated, (void*)pRelatedItem, NULL)  );
		}
	}

fail:
	return;
}

/**
 * Finds other items related to this one and stores them in its appropriate choices.
 */
static void _item__gather_related_items__collisions(
	SG_context*       pCtx,   //< [in] [out] Error and context info.
	SG_resolve__item* pItem,  //< [in] The item to gather related items into.
	SG_vector*        pItems  //< [in] An SG_vector of items to search for related ones.
	)
{
	SG_varray* pGids = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pItem->pLVI->pvhIssue);
	SG_NULLARGCHECK(pItems);

	// If we have an EXISTENCE choice caused by a collision,
	// find the other items in the collision.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE] != NULL)
		&& (pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE]->bCollisionCause == SG_TRUE)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Collision_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE]->pCollisionItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE]->pCollisionItems)  );
	}

	// If we have a NAME choice caused by a collision,
	// find the other items in the collision.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__NAME] != NULL)
		&& (pItem->aChoices[SG_RESOLVE__STATE__NAME]->bCollisionCause == SG_TRUE)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Collision_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__NAME]->pCollisionItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__NAME]->pCollisionItems)  );
	}

	// If we have a LOCATION choice caused by a collision,
	// find the other items in the collision.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__LOCATION] != NULL)
		&& (pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->bCollisionCause == SG_TRUE)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Collision_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->pCollisionItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->pCollisionItems)  );
	}

	// If we have a LOCATION choice caused by a path cycle,
	// find the other items in the cycle.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__LOCATION] != NULL)
		&& ((pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE) != 0)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_Cycle_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->pCycleItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->pCycleItems)  );
	}

fail:
	return;
}

/**
 * Finds other items related to this one and stores them in its appropriate choices.
 */
static void _item__gather_related_items__portability(
	SG_context*       pCtx,   //< [in] [out] Error and context info.
	SG_resolve__item* pItem,  //< [in] The item to gather related items into.
	SG_vector*        pItems  //< [in] An SG_vector of items to search for related ones.
	)
{
	SG_varray* pGids = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pItem->pLVI->pvhIssue);
	SG_NULLARGCHECK(pItems);

	// If we have an EXISTENCE choice caused by a POTENTIAL collision,
	// find the other items in the collision.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE] != NULL)
		&& (pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE]->ePortCauses != 0)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_PortabilityCollision_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE]->pPortabilityCollisionItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__EXISTENCE]->pPortabilityCollisionItems)  );
	}

	// If we have a NAME choice caused by a collision,
	// find the other items in the collision.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__NAME] != NULL)
		&& (pItem->aChoices[SG_RESOLVE__STATE__NAME]->ePortCauses != 0)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_PortabilityCollision_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__NAME]->pPortabilityCollisionItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__NAME]->pPortabilityCollisionItems)  );
	}

	// If we have a LOCATION choice caused by a collision,
	// find the other items in the collision.
	if (
		   (pItem->aChoices[SG_RESOLVE__STATE__LOCATION] != NULL)
		&& (pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->ePortCauses != 0)
	)
	{
		SG_uint32 uCount = 0u;

		SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pItem->pLVI->pvhIssue, gszIssueKey_PortabilityCollision_Items, &pGids)  );
		SG_ERR_CHECK(  SG_varray__count(pCtx, pGids, &uCount)  );
		SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->pPortabilityCollisionItems, uCount)  );
		SG_ERR_CHECK(  _item__gather_items(pCtx, pGids, pItems, pItem, pItem->aChoices[SG_RESOLVE__STATE__LOCATION]->pPortabilityCollisionItems)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__ITEM__INLINE1_H
