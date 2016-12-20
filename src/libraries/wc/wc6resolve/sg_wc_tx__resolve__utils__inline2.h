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
 * @file sg_wc_tx__resolve__utils__inline2.h
 *
 * @details Various SG_resolve__ utilities/getters that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__UTILS__INLINE2_H
#define H_SG_WC_TX__RESOLVE__UTILS__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__get_unresolved_count(
	SG_context*       pCtx,
	/*const*/ SG_resolve* pResolve,
	SG_uint32*        pCount
	)
{
	SG_uint32 uCount = 0u;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pCount);

	// count up how many unresolved choices we have
	SG_ERR_CHECK(  SG_resolve__foreach_choice(pCtx, pResolve, SG_TRUE, SG_FALSE, SG_resolve__choice_callback__count, &uCount)  );

	*pCount = uCount;

fail:
	return;
}

void SG_resolve__check_item__gid(
	SG_context*        pCtx,
	SG_resolve*        pResolve,
	const char*        szGid,
	SG_resolve__item** ppItem
	)
{
	SG_resolve__item* pItem = NULL;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(szGid);
	SG_NULLARGCHECK(ppItem);

	SG_ERR_CHECK(  SG_vector__find__first(pCtx, pResolve->pItems, _item__vector_predicate__match_gid, (void*)szGid, NULL, (void**)&pItem)  );

	*ppItem = pItem;
	pItem = NULL;

fail:
	return;
}

void SG_resolve__get_item__gid(
	SG_context*        pCtx,
	SG_resolve*        pResolve,
	const char*        szGid,
	SG_resolve__item** ppItem
	)
{
	SG_resolve__item* pItem = NULL;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(szGid);
	SG_NULLARGCHECK(ppItem);

	SG_ERR_CHECK(  SG_resolve__check_item__gid(pCtx, pResolve, szGid, &pItem)  );
	if (pItem == NULL)
	{
		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "resolve item by GID: %s", szGid));
	}

	*ppItem = pItem;
	pItem = NULL;

fail:
	return;
}

void SG_resolve__foreach_item(
	SG_context*                 pCtx,
	SG_resolve*                 pResolve,
	SG_bool                     bOnlyUnresolved,
	SG_resolve__item__callback* fCallback,
	void*                       pUserData
	)
{
	_item__vector_action__call_resolve_action__data cData;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(fCallback);

	cData.fCallback = fCallback;
	cData.pUserData = pUserData;
	cData.bContinue = SG_TRUE;

	if (bOnlyUnresolved == SG_TRUE)
	{
		SG_bool bResolved = SG_FALSE;
		SG_ERR_CHECK(  SG_vector__find__all(pCtx, pResolve->pItems, _item__vector_predicate__match_resolved, &bResolved, _item__vector_action__call_resolve_action, &cData)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_vector__foreach(pCtx, pResolve->pItems, _item__vector_action__call_resolve_action, &cData)  );
	}

fail:
	return;
}

void SG_resolve__foreach_choice(
	SG_context*                   pCtx,
	SG_resolve*                   pResolve,
	SG_bool                       bOnlyUnresolved,
	SG_bool                       bGroupByState,
	SG_resolve__choice__callback* fCallback,
	void*                         pUserData
	)
{
	_item__vector_action__foreach_choice__data cData;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(fCallback);

	cData.bOnlyUnresolved = bOnlyUnresolved;
	cData.eState          = SG_RESOLVE__STATE__COUNT;
	cData.fCallback       = fCallback;
	cData.pUserData       = pUserData;
	cData.bContinue       = SG_TRUE;

	if (bGroupByState == SG_FALSE)
	{
		if (bOnlyUnresolved == SG_TRUE)
		{
			SG_bool bResolved = SG_FALSE;
			SG_ERR_CHECK(  SG_vector__find__all(pCtx, pResolve->pItems, _item__vector_predicate__match_resolved, &bResolved, _item__vector_action__foreach_choice, &cData)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_vector__foreach(pCtx, pResolve->pItems, _item__vector_action__foreach_choice, &cData)  );
		}
	}
	else
	{
		SG_uint32 uIndex = 0u;

		for (uIndex = 0; uIndex < SG_RESOLVE__STATE__COUNT; ++uIndex)
		{
			cData.eState = (SG_resolve__state)uIndex;
			if (bOnlyUnresolved == SG_TRUE)
			{
				SG_bool bResolved = SG_FALSE;
				SG_ERR_CHECK(  SG_vector__find__all(pCtx, pResolve->pItems, _item__vector_predicate__match_resolved, &bResolved, _item__vector_action__foreach_choice, &cData)  );
			}
			else
			{
				SG_ERR_CHECK(  SG_vector__foreach(pCtx, pResolve->pItems, _item__vector_action__foreach_choice, &cData)  );
			}
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_resolve__hack__get_repo(SG_context * pCtx,
								SG_resolve * pResolve,
								SG_repo ** ppRepo)
{
	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(ppRepo);

	*ppRepo = pResolve->pWcTx->pDb->pRepo;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__UTILS__INLINE2_H
