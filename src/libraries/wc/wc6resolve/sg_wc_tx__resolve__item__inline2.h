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
 * @file sg_wc_tx__resolve__item__inline2.h
 *
 * @details SG_resolve__item__ routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__ITEM__INLINE2_H
#define H_SG_WC_TX__RESOLVE__ITEM__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__item__get_resolve(
	SG_context*       pCtx,
	SG_resolve__item* pItem,
	SG_resolve**      ppResolve
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppResolve);

	*ppResolve = pItem->pResolve;

fail:
	return;
}

void SG_resolve__item__get_gid(
	SG_context*       pCtx,
	SG_resolve__item* pItem,
	const char**      ppGid
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppGid);

	*ppGid = pItem->szGid;

fail:
	return;
}

void SG_resolve__item__get_type(
	SG_context*             pCtx,
	SG_resolve__item*       pItem,
	SG_treenode_entry_type* pType
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pType);

	*pType = pItem->eType;

fail:
	return;
}

/**
 * Fetch/Compute the current working repo-path for this item.
 * We also return advise on whether the path should be displayed
 * in parens like STATUS.
 * 
 * You own the returned string.
 * 
 */
void SG_resolve__item__get_repo_path__working(
	SG_context*       pCtx,
	const SG_resolve__item* pItem,
	SG_string ** ppStringRepoPath,
	SG_bool * pbShowInParens
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppStringRepoPath);
	SG_NULLARGCHECK(pbShowInParens);

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pItem->pResolve->pWcTx,
															  pItem->pLVI, ppStringRepoPath)  );
	*pbShowInParens = ((SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pItem->pLVI->scan_flags_Live)) != 0);

fail:
	return;
}

void SG_resolve__item__check_choice__state(
	SG_context*          pCtx,
	SG_resolve__item*    pItem,
	SG_resolve__state    eState,
	SG_resolve__choice** ppChoice
	)
{
	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppChoice);

	*ppChoice = pItem->aChoices[eState];

fail:
	return;
}

void SG_resolve__item__get_choice__state(
	SG_context*          pCtx,
	SG_resolve__item*    pItem,
	SG_resolve__state    eState,
	SG_resolve__choice** ppChoice
	)
{
	SG_resolve__choice* pChoice = NULL;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(ppChoice);

	SG_ERR_CHECK(  SG_resolve__item__check_choice__state(pCtx, pItem, eState, &pChoice)  );
	if (pChoice == NULL)
	{
		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "resolve choice by item/state: %s/%d", pItem->szGid, eState));
	}

	*ppChoice = pChoice;
	pChoice = NULL;

fail:
	return;
}

void SG_resolve__item__get_resolved(
	SG_context*       pCtx,
	SG_resolve__item* pItem,
	SG_wc_status_flags * pStatusFlags_x_xr_xu
	)
{
	SG_NULLARGCHECK( pItem );
	SG_NULLARGCHECK( pStatusFlags_x_xr_xu );

	*pStatusFlags_x_xr_xu = pItem->pLVI->statusFlags_x_xr_xu;

fail:
	return;
}

void SG_resolve__item__foreach_choice(
	SG_context*                   pCtx,
	SG_resolve__item*             pItem,
	SG_bool                       bOnlyUnresolved,
	SG_resolve__choice__callback* fCallback,
	void*                         pUserData
	)
{
	SG_bool bContinue = SG_TRUE;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(fCallback);

	SG_ERR_CHECK(  _item__foreach_choice(pCtx, pItem, bOnlyUnresolved, fCallback, pUserData, &bContinue)  );

fail:
	return;
}

void SG_resolve__item_callback__count(
	SG_context*       pCtx,
	SG_resolve__item* pItem,
	void*             pUserData,
	SG_bool*          pContinue
	)
{
	SG_uint32* pCount = (SG_uint32*)pUserData;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	*pCount += 1u;
	*pContinue = SG_TRUE;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__ITEM__INLINE2_H
