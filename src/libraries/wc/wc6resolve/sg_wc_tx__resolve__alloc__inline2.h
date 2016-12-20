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
 * @file sg_wc_tx__resolve__alloc__inline2.h
 *
 * @details Alloc/Free-related SG_resolve__ routines that I pulled
 * from the PendingTree version of sg_resolve.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__ALLOC__INLINE2_H
#define H_SG_WC_TX__RESOLVE__ALLOC__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * We get called once for each ISSUE in the DB.
 *
 * TODO            See W8312.
 *
 */
static sg_wc_db__issue__foreach_cb _alloc_cb;

static void _alloc_cb(SG_context * pCtx,
					  void * pVoidData,
					  SG_uint64 uiAliasGid,
					  SG_wc_status_flags statusFlags_x_xr_xu,
					  SG_vhash ** ppvhIssue,
					  SG_vhash ** ppvhSavedResolutions)
{
	SG_resolve * pResolve = (SG_resolve *)pVoidData;
	SG_resolve__item * pItem = NULL;

	SG_UNUSED( statusFlags_x_xr_xu );  // we no longer use these fields because
	SG_UNUSED( ppvhIssue );            // we always want to reference the pLVI
	SG_UNUSED( ppvhSavedResolutions ); // versions in case there are QUEUED changes.

	SG_ERR_CHECK(  _item__alloc(pCtx, uiAliasGid, pResolve, &pItem)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pResolve->pItems, (void*)pItem, NULL)  );
	pItem = NULL;

fail:
	_ITEM__NULLFREE(pCtx, pItem);
}

//////////////////////////////////////////////////////////////////

/**
 * Allocate a pResolve to be associated with the given pWcTx
 * and OPTIONALLY return it.  If pWcTx already has one, just
 * return it.
 *
 * Yes, this is poorly named.
 *
 */
void SG_resolve__alloc(
	SG_context*     pCtx,
	SG_wc_tx *      pWcTx,		// we borrow, but do not own this.
	SG_resolve**    ppResolve	// YOU DO NOT OWN THIS.
	)
{
	SG_resolve*      pResolve = NULL;
	SG_uint32 uCount;
	SG_uint32 uIndex;

	SG_NULLARGCHECK(pWcTx);
	// ppResolve is optional

	if (pWcTx->pResolve)
	{
		if (ppResolve)
			*ppResolve = pWcTx->pResolve;
		return;
	}

	// allocate the resolve structure and setup its basic data
	SG_ERR_CHECK(  SG_alloc1(pCtx, pResolve)  );
	pResolve->pWcTx = pWcTx;

	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pResolve->pItems, 100)  );

	// retrieve the merge issues and any pre-existing resolve data from WC.
	//
	// TODO 2012/06/04 This uses SQL to get the set of items that have
	// TODO            issues in the DB.  This is OK as long as RESOLVE
	// TODO            is not being started in the middle of a longer TX.
	// TODO            We should really do a full STATUS from ROOT and
	// TODO            iterate thru it and alloc an item for anything
	// TODO            with an __X__ bit set.
	// TODO
	// TODO            See W8312.

	SG_ERR_CHECK(  sg_wc_db__issue__foreach(pCtx, pWcTx->pDb, _alloc_cb, (void *)pResolve)  );

	// Now that all the items are created,
	// have each item gather its related items.
	SG_ERR_CHECK(  SG_vector__length(pCtx, pResolve->pItems, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_resolve__item* pItem = NULL;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pResolve->pItems, uIndex, (void**)&pItem)  );
		SG_ERR_CHECK(  _item__gather_related_items__collisions(pCtx, pItem, pResolve->pItems)  );
		SG_ERR_CHECK(  _item__gather_related_items__portability(pCtx, pItem, pResolve->pItems)  );
	}

#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_resolve_debug__dump_to_console(pCtx, pResolve)  );
#endif

	pWcTx->pResolve = pResolve;		// pWcTx owns this now
	pResolve = NULL;
	
	if (ppResolve)
		*ppResolve = pWcTx->pResolve;

fail:
	SG_RESOLVE_NULLFREE(pCtx, pResolve);
}

void SG_resolve__free(
	SG_context* pCtx,
	SG_resolve* pResolve
	)
{
	if (pResolve == NULL)
		return;

	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pResolve->pItems, _item__sg_action__free);
	pResolve->pWcTx = NULL;

	SG_NULLFREE(pCtx, pResolve);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__ALLOC__INLINE2_H
