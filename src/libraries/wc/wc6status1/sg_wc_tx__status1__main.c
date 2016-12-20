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
 * @file sg_wc_tx__status1__main.c
 *
 * @details Compute 1-revspec status.  That is, compute status
 * using the live/current WD and an ARBITRARY CSET.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include <sg_vv2__public_typedefs.h>
#include "sg_wc__public_prototypes.h"
#include <sg_vv2__public_prototypes.h>
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void sg_wc_tx__status1_debug__dump(SG_context * pCtx,
								   sg_wc_tx__status1__data * pS1Data)
{
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey;
	sg_wc_tx__status1__item * pS1Item;	// we do not own this
	SG_bool bFound;

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Status1 Dump: [cset %s][isParent %d]\n",
							   pS1Data->pszHidCSet, pS1Data->bIsParent)  );

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pS1Data->prbItems,
											  &bFound, &pszKey, (void **)&pS1Item)  );
	while (bFound)
	{
#define X(v) 	((v) ? v : "")
#define XS(s)	((s) ? SG_string__sz(s) : "")

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "  %s: [%d,%d] [hid %s,%s] [gidparent %s,%s][name %s,%s][repopath %s,%s]\n",
								   pS1Item->pszGid,
								   pS1Item->bPresentIn_H, pS1Item->bPresentIn_WC,
								   X(pS1Item->h.pszHid), X(pS1Item->wc.pszHid),
								   X(pS1Item->h.pszGidParent), X(pS1Item->wc.pszGidParent),
								   X(pS1Item->h.pszEntryname), X(pS1Item->wc.pszEntryname),
								   XS(pS1Item->h.pStringRepoPath), XS(pS1Item->wc.pStringRepoPath))  );
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter,
												 &bFound, &pszKey, (void **)&pS1Item)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}
#endif

//////////////////////////////////////////////////////////////////

static void _s1_item__free(SG_context * pCtx,
						   sg_wc_tx__status1__item * pS1Item)
{
	if (!pS1Item)
		return;

	SG_NULLFREE(pCtx, pS1Item->pszGid);

	SG_NULLFREE(pCtx, pS1Item->h.pszGidParent);
	SG_NULLFREE(pCtx, pS1Item->h.pszEntryname);
	SG_NULLFREE(pCtx, pS1Item->h.pszHid);
	SG_STRING_NULLFREE(pCtx, pS1Item->h.pStringRepoPath);

	SG_NULLFREE(pCtx, pS1Item->wc.pszGidParent);
	SG_NULLFREE(pCtx, pS1Item->wc.pszEntryname);
	SG_NULLFREE(pCtx, pS1Item->wc.pszHid);
	SG_STRING_NULLFREE(pCtx, pS1Item->wc.pStringRepoPath);

	SG_NULLFREE(pCtx, pS1Item);
}

#define _S1_ITEM__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,_s1_item__free)

//////////////////////////////////////////////////////////////////

void sg_wc_tx__status1__data__free(SG_context * pCtx,
								   sg_wc_tx__status1__data * pS1Data)
{
	if (!pS1Data)
		return;

	pS1Data->pWcTx = NULL;
	pS1Data->pRevSpec = NULL;

	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pS1Data->prbItems, ((SG_free_callback *)_s1_item__free));
	SG_RBTREE_NULLFREE(pCtx, pS1Data->prbIndex_H);	// we do not own the associated data
	SG_RBTREE_NULLFREE(pCtx, pS1Data->prbIndex_WC);	// we do not own the associated data
	SG_NULLFREE(pCtx, pS1Data->pszHidCSet);

	SG_NULLFREE(pCtx, pS1Data);
}

//////////////////////////////////////////////////////////////////

static void _s1_add_historical_tne(SG_context * pCtx,
								   sg_wc_tx__status1__data * pS1Data,
								   const char * pszGid,
								   const char * pszGidParent,
								   const SG_treenode_entry * ptne,
								   const char * pszRepoPathParent)
{
	sg_wc_tx__status1__item * pS1Item_Allocated = NULL;
	sg_wc_tx__status1__item * pS1Item;	// we do not own this
	SG_treenode * ptn = NULL;
	const char * psz;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pS1Item_Allocated)  );
	pS1Item = pS1Item_Allocated;
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pS1Data->prbItems, pszGid, pS1Item_Allocated)  );
	pS1Item_Allocated = NULL;		// rbtree owns it now.

	pS1Item->bPresentIn_H = SG_TRUE;

	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGid, &pS1Item->pszGid)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne, &pS1Item->tneType)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, ptne, &pS1Item->h.attrbits)  );

	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGidParent, &pS1Item->h.pszGidParent)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, ptne, &psz)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, psz, &pS1Item->h.pszEntryname)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne, &psz)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, psz, &pS1Item->h.pszHid)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pS1Item->h.pStringRepoPath)  );
	if (pszRepoPathParent)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pS1Item->h.pStringRepoPath, pszRepoPathParent)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pS1Item->h.pStringRepoPath, pS1Item->h.pszEntryname)  );
	}
	else
	{
		static char buf[3] = { '@', SG_VV2__REPO_PATH_DOMAIN__0, 0 };
		SG_ASSERT( (strcmp(pS1Item->h.pszEntryname, "@") == 0) );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pS1Item->h.pStringRepoPath, buf)  );
	}

	if (pS1Item->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_uint32 k, nrItems;

		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pS1Item->h.pStringRepoPath, "/")  );

		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pS1Data->pWcTx->pDb->pRepo, pS1Item->h.pszHid, &ptn)  );
		SG_ERR_CHECK(  SG_treenode__count(pCtx, ptn, &nrItems)  );
		for (k=0; k<nrItems; k++)
		{
			const SG_treenode_entry * ptne_k;
			const char * pszGid_k;

			SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, ptn, k, &pszGid_k, &ptne_k)  );
			SG_ERR_CHECK(  _s1_add_historical_tne(pCtx, pS1Data, pszGid_k, pszGid, ptne_k,
												  SG_string__sz(pS1Item->h.pStringRepoPath))  );
		}
	}
	
fail:
	_S1_ITEM__NULLFREE(pCtx, pS1Item_Allocated);
	SG_TREENODE_NULLFREE(pCtx, ptn);
}

//////////////////////////////////////////////////////////////////

static void _s1_add_wc(SG_context * pCtx,
					   sg_wc_tx__status1__data * pS1Data,
					   sg_wc_liveview_item * pLVI,
					   const char * pszGidParent);

static SG_rbtree_ui64_foreach_callback _dive_cb;

struct _dive_data
{
	sg_wc_tx__status1__data * pS1Data;
	const char * pszGidParent;
};

static void _dive_cb(SG_context * pCtx,
					 SG_uint64 uiAliasGid,
					 void * pVoid_LVI,
					 void * pVoid_Data)
{
	struct _dive_data * pDiveData = (struct _dive_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;

	SG_UNUSED( uiAliasGid );

	SG_ERR_CHECK_RETURN(  _s1_add_wc(pCtx, pDiveData->pS1Data, pLVI,
									 pDiveData->pszGidParent)  );
}

static void _s1_add_wc(SG_context * pCtx,
					   sg_wc_tx__status1__data * pS1Data,
					   sg_wc_liveview_item * pLVI,
					   const char * pszGidParent)
{
	sg_wc_tx__status1__item * pS1Item_Allocated = NULL;
	sg_wc_tx__status1__item * pS1Item;	// we do not own this
	char * pszGid = NULL;
	SG_bool bFound;

	// Use _alias3 so that we get 't' prefix on temporary GIDs.
	SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias3(pCtx, pS1Data->pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pS1Data->prbItems, pszGid, &bFound, (void **)&pS1Item)  );
	if (!bFound)
	{
		SG_ERR_CHECK(  SG_alloc1(pCtx, pS1Item_Allocated)  );
		pS1Item = pS1Item_Allocated;
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pS1Data->prbItems, pszGid, pS1Item_Allocated)  );
		pS1Item_Allocated = NULL;		// rbtree owns it now.

		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGid, &pS1Item->pszGid)  );
		pS1Item->tneType = pLVI->tneType;
	}

	pS1Item->bPresentIn_WC = SG_TRUE;

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pS1Data->pWcTx, pLVI,
															  &pS1Item->wc.pStringRepoPath)  );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_current_attrbits(pCtx, pLVI, pS1Data->pWcTx,
															 (SG_uint64 *)&pS1Item->wc.attrbits)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGidParent, &pS1Item->wc.pszGidParent)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pLVI->pStringEntryname), &pS1Item->wc.pszEntryname)  );

	pS1Item->wc.pLVI = pLVI;

	if (pS1Item->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		struct _dive_data d;
		sg_wc_liveview_dir * pLVD;		// we do not own this
		SG_bool bDive = SG_TRUE;

		SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pS1Item->wc.pStringRepoPath)  );

		if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
		{
			// we never dive into .sgdrawer
			bDive = SG_FALSE;
		}
		else if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
		{
			SG_wc_status_flags statusFlags;
			SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pS1Data->pWcTx, pLVI,
														pS1Data->bNoIgnores, pS1Data->bNoTSC,
														&statusFlags)  );
			if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)
			{
				// we should also skip diving in to ignored directories.
				// perhaps the LVI code should not populate the children
				// so that we get the same behavior everywhere.
				// W6508 W5042 B7193
				// 
				// Don't dive into ignored directories; we just want to report
				// that the directory is itself ignored, so we don't care what
				// is *inside* it.  (See sg_wc_tx__rp__status.c)
				bDive = SG_FALSE;
			}
		}

		if (bDive)
		{
			d.pS1Data = pS1Data;
			d.pszGidParent = pszGid;	// GID of self (not our parent)

			SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pS1Data->pWcTx, pLVI, &pLVD)  );
			SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _dive_cb, &d)  );
		}
	}

fail:
	_S1_ITEM__NULLFREE(pCtx, pS1Item_Allocated);
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

/**
 * Build an index on the historical repo-path.
 *
 */
static void _s1_index_historical(SG_context * pCtx,
								 sg_wc_tx__status1__data * pS1Data)
{
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey;
	sg_wc_tx__status1__item * pS1Item;	// we do not own this
	SG_bool bFound;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pS1Data->prbIndex_H)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pS1Data->prbItems,
											  &bFound, &pszKey, (void **)&pS1Item)  );
	while (bFound)
	{
		if (pS1Item->bPresentIn_H)
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pS1Data->prbIndex_H,
													  SG_string__sz(pS1Item->h.pStringRepoPath), pS1Item)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter,
												 &bFound, &pszKey, (void **)&pS1Item)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

/**
 * Build an index on the wc repo-path.
 *
 */
static void _s1_index_wc(SG_context * pCtx,
						 sg_wc_tx__status1__data * pS1Data)
{
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey;
	sg_wc_tx__status1__item * pS1Item;	// we do not own this
	SG_bool bFound;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pS1Data->prbIndex_WC)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pS1Data->prbItems,
											  &bFound, &pszKey, (void **)&pS1Item)  );
	while (bFound)
	{
		if (pS1Item->bPresentIn_WC)
			SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pS1Data->prbIndex_WC,
													  SG_string__sz(pS1Item->wc.pStringRepoPath), pS1Item)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter,
												 &bFound, &pszKey, (void **)&pS1Item)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

/**
 * Setup for a 1-rev-spec status.
 *
 * [] We must validate the given rev-spec.
 * [] We load the historical data from that cset (the entire cset)
 *    into the .h portion of prbItems.
 * [] We load then visit the entire tree in the WD and load it
 *    into the .wc portion of prbItems.
 *
 */
void sg_wc_tx__status1__setup(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  const SG_rev_spec * pRevSpec,
							  SG_bool bListUnchanged,
							  SG_bool bNoIgnores,
							  SG_bool bNoTSC,
							  SG_bool bListSparse,
							  SG_bool bListReserved,
							  sg_wc_tx__status1__data ** ppS1Data)
{
	sg_wc_tx__status1__data * pS1Data = NULL;
	char * pszHidSuperRootTN = NULL;
	SG_treenode * ptnSuperRoot = NULL;
	SG_varray * pvaParents = NULL;
	const char * pszGidRoot;
	const SG_treenode_entry * ptneRoot;
	const char * pszEntrynameRoot;
	const char * pszParent0;
	sg_wc_liveview_item * pLVI_Root;	// we do not own this
	SG_treenode_entry_type tneTypeRoot;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pS1Data)  );
	pS1Data->pWcTx = pWcTx;
	pS1Data->pRevSpec = pRevSpec;
	pS1Data->bListUnchanged = bListUnchanged;
	pS1Data->bNoIgnores = bNoIgnores;
	pS1Data->bNoTSC = bNoTSC;
	pS1Data->bListSparse = bListSparse;
	pS1Data->bListReserved = bListReserved;
	pS1Data->pszWasLabel_H  = "Changeset (0)";
	pS1Data->pszWasLabel_WC = "Working";

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pS1Data->prbItems)  );

	// We assume that pRevSpec names a single unique arbitrary cset.
	// 
	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pS1Data->pWcTx->pDb->pRepo, pRevSpec, SG_TRUE,
											  &pS1Data->pszHidCSet, NULL)  );
	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx, pS1Data->pWcTx, &pvaParents)  );
	SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 0, &pszParent0)  );
	if (strcmp(pS1Data->pszHidCSet, pszParent0) == 0)
	{
		// TODO 2013/01/09 If it maps to the BASELINE, we should fall back and
		// TODO            use the normal STATUS code since it'd be quicker.
		// TODO            For testing purposes, let this run so we can compare
		// TODO            our output with the plan zero-rev-spec status.
		pS1Data->bIsParent = SG_TRUE;
	}
	else
	{
		SG_uint32 nrParents;

		// If the WD has a pending merge, see if the requested cset is the other parent.
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &nrParents)  );
		if (nrParents > 1)
		{
			const char * pszParent1;
			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, 1, &pszParent1)  );
			pS1Data->bIsParent = (strcmp(pS1Data->pszHidCSet, pszParent1) == 0);
		}
	}

	// Get the super-root TN of the CSET.
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pS1Data->pWcTx->pDb->pRepo, pS1Data->pszHidCSet,
													 &pszHidSuperRootTN)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pS1Data->pWcTx->pDb->pRepo, pszHidSuperRootTN, &ptnSuperRoot)  );
	
	// Get the GID and TNE of the "@/" root directory.
	SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, ptnSuperRoot, 0, &pszGidRoot, &ptneRoot)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, ptneRoot, &pszEntrynameRoot)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptneRoot, &tneTypeRoot)  );
	if ((strcmp(pszEntrynameRoot,"@") != 0) || (tneTypeRoot != SG_TREENODEENTRY_TYPE_DIRECTORY))
		SG_ERR_THROW2(  SG_ERR_MALFORMED_SUPERROOT_TREENODE,
						(pCtx, "In cset '%s' / super-root '%s'.", pS1Data->pszHidCSet, pszHidSuperRootTN)  );

	// recursively add a row to the rbtree for "@/" and everything within it from the cset.
	SG_ERR_CHECK(  _s1_add_historical_tne(pCtx, pS1Data, pszGidRoot, SG_WC_DB__GID__NULL_ROOT,
										  ptneRoot, NULL)  );

	// recursively add the current contents of the WD.
	SG_ERR_CHECK(  sg_wc_liveview_item__alloc__root_item(pCtx, pS1Data->pWcTx, &pLVI_Root)  );
	SG_ERR_CHECK(  _s1_add_wc(pCtx, pS1Data, pLVI_Root, SG_WC_DB__GID__NULL_ROOT)  );

	SG_ERR_CHECK(  _s1_index_historical(pCtx, pS1Data)  );
	SG_ERR_CHECK(  _s1_index_wc(pCtx, pS1Data)  );

#if TRACE_WC_TX_STATUS1
	SG_ERR_IGNORE(  sg_wc_tx__status1_debug__dump(pCtx, pS1Data)  );
#endif

	SG_RETURN_AND_NULL( pS1Data, ppS1Data );

fail:
	SG_WC_TX__STATUS1__DATA__NULLFREE(pCtx, pS1Data);
	SG_NULLFREE(pCtx, pszHidSuperRootTN);
	SG_TREENODE_NULLFREE(pCtx, ptnSuperRoot);
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
}

//////////////////////////////////////////////////////////////////

static void _s1_compute_flags(SG_context * pCtx,
							  sg_wc_tx__status1__data * pS1Data,
							  sg_wc_tx__status1__item * p,
							  SG_wc_status_flags * pStatusFlags)
{
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	SG_uint32 nrChangeBits__Found    = 0;
	SG_uint32 nrChangeBits__Ignored  = 0;
	SG_uint32 nrChangeBits__Lost     = 0;
	SG_uint32 nrChangeBits__Added    = 0;
	SG_uint32 nrChangeBits__Deleted  = 0;
	SG_uint32 nrChangeBits__Renamed  = 0;
	SG_uint32 nrChangeBits__Moved    = 0;
	SG_uint32 nrChangeBits__Attrbits = 0;
	SG_uint32 nrChangeBits__Content  = 0;

	SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx, p->tneType, &statusFlags)  );

	if (p->bPresentIn_WC)
	{
		// The item is known about in the WC (but it may have a pending delete or be lost).

		if (SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(p->wc.pLVI->scan_flags_Live))
		{
			if (pS1Data->bNoIgnores)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__U__FOUND;
				nrChangeBits__Found = 1;
			}
			else
			{
				SG_bool bIgnorable;
				SG_ERR_CHECK(  sg_wc_db__ignores__matches_ignorable_pattern(pCtx, pS1Data->pWcTx->pDb,
																			p->wc.pStringRepoPath,
																			&bIgnorable)  );
				if (bIgnorable)
				{
					statusFlags |= SG_WC_STATUS_FLAGS__U__IGNORED;
					nrChangeBits__Ignored = 1;
				}
				else
				{
					statusFlags |= SG_WC_STATUS_FLAGS__U__FOUND;
					nrChangeBits__Found = 1;
				}
			}
		}
		else if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(p->wc.pLVI->scan_flags_Live))
		{
			statusFlags |= SG_WC_STATUS_FLAGS__R__RESERVED;
		}
		else
		{
			SG_bool bSkip_C_bits = SG_FALSE;

			SG_ASSERT(  (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED(p->wc.pLVI->scan_flags_Live))  );

			if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_LOST(p->wc.pLVI->scan_flags_Live))
			{
				statusFlags |= SG_WC_STATUS_FLAGS__U__LOST;
				nrChangeBits__Lost = 1;
				bSkip_C_bits = SG_TRUE;
			}

			if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(p->wc.pLVI->scan_flags_Live))
			{
				statusFlags |= SG_WC_STATUS_FLAGS__A__SPARSE;
			}
			
			if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(p->wc.pLVI->scan_flags_Live))
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__DELETED;
				nrChangeBits__Deleted = 1;
				bSkip_C_bits = SG_TRUE;
			}

			if (p->bPresentIn_H)
			{
				if (strcmp(p->h.pszGidParent, p->wc.pszGidParent) != 0)
				{
					statusFlags |= SG_WC_STATUS_FLAGS__S__MOVED;
					nrChangeBits__Moved = 1;
				}

				if (strcmp(p->h.pszEntryname, p->wc.pszEntryname) != 0)
				{
					statusFlags |= SG_WC_STATUS_FLAGS__S__RENAMED;
					nrChangeBits__Renamed = 1;
				}

				if (!bSkip_C_bits)
				{
					if (p->h.attrbits != p->wc.attrbits)
					{
						statusFlags |= SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;
						nrChangeBits__Attrbits = 1;
					}

					switch (p->tneType)
					{
					default:
						SG_ASSERT_RELEASE_FAIL2(  SG_ERR_NOTIMPLEMENTED,
												  (pCtx, "Lookup HID for %s.", SG_string__sz(p->wc.pStringRepoPath))  );
					case SG_TREENODEENTRY_TYPE_DIRECTORY:
					case SG_TREENODEENTRY_TYPE__DEVICE:
						break;

					case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
					case SG_TREENODEENTRY_TYPE_SYMLINK:
						// Only bother to compute the live content HID if the file/symlink
						// is present in both and can therefore be compared.
						SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid(pCtx, p->wc.pLVI, pS1Data->pWcTx,
																					pS1Data->bNoTSC,
																					&p->wc.pszHid, NULL)  );
						if (strcmp(p->h.pszHid, p->wc.pszHid) != 0)
						{
							statusFlags |= SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED;
							nrChangeBits__Content = 1;
						}
						break;
					}
				}
			}
			else
			{
				statusFlags |= SG_WC_STATUS_FLAGS__S__ADDED;
				nrChangeBits__Added = 1;
			}
		}
	}
	else if (p->bPresentIn_H)
	{
		// The item is unknown (not present) in baseline+dirt.
		// This case does not cover LOST or pending DELETES.
		statusFlags |= SG_WC_STATUS_FLAGS__S__DELETED;
		nrChangeBits__Deleted = 1;
	}
	else
	{
		// Item is not present in either side.  this cannot happen.
		SG_ASSERT_RELEASE_FAIL( (0) );
	}

	// We DO NOT report on LOCKS or ISSUES or AUTOMERGE results.
	// We DO NOT qualify ADD as ADD-SPECIAL-*.

	if ((nrChangeBits__Found
		 + nrChangeBits__Ignored
		 + nrChangeBits__Lost
		 + nrChangeBits__Added
		 + nrChangeBits__Deleted
		 + nrChangeBits__Renamed
		 + nrChangeBits__Moved
		 + nrChangeBits__Attrbits
		 + nrChangeBits__Content) > 1)
		statusFlags |= SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE;

	*pStatusFlags = statusFlags;
	
fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _s1_get_symlink_target(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const char * pszHid,
								   SG_string ** ppStringTarget)
{
	SG_string * pStringTarget = NULL;
	SG_byte*    pBuffer   = NULL;
	SG_uint64   uSize     = 0u;

	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo, pszHid,
												   &pBuffer, &uSize)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pStringTarget, pBuffer, (SG_uint32)uSize)  );
	SG_RETURN_AND_NULL( pStringTarget, ppStringTarget );

fail:
	SG_STRING_NULLFREE(pCtx, pStringTarget);
	SG_NULLFREE(pCtx, pBuffer);
}

//////////////////////////////////////////////////////////////////

static void _s1_do_fixed_keys(SG_context * pCtx,
							  sg_wc_tx__status1__data * pS1Data,
							  sg_wc_tx__status1__item * pS1Item,
							  SG_wc_status_flags statusFlags,
							  SG_vhash * pvhItem)
{
	SG_string * pStringRepoPath = NULL;
	SG_string * pStringSymlinkTarget = NULL;
	SG_vhash * pvhProperties = NULL;
	SG_uint32 countHeadings = 0;
	SG_varray * pvaHeadings = NULL;		// we do not own this
	SG_bool bPresentNow;
	char * pszHid_owned = NULL;

	// Note we do not report on LOCKS.

	// <item> ::= { "status"   : <properties>,		-- vhash with flag bits expanded into JS-friendly fields
	//              "gid"      : "<gid>",
	//              "path"     : "<path>",					-- WC path if present in WC; @0/ path if not.
	//              "headings" : [ "<heading_0>", ... ],	-- list of headers for classic_formatter.
	//              "B"        : <sub-section>,				-- if present in cset 0 (rev-spec)
	//              "WC"       : <sub-section>,				-- if present in the WD (live)
	//            };
	//
	// All fields in a <sub-section> refer to the item relative to that state.
	// 
	// <sub-section> ::= { "path"           : "<path_j>",
	//                     "name"           : "<name_j>",
	//                     "gid_parent"     : "<gid_parent_j>",
	//                     "hid"            : "<hid_j>",			-- when not a directory
	//                     "size"           : "<size_j>",			-- size of content (when hid set)
	//                     "attributes"     : <attrbits_j>
	//                     "was_label"      : "<label_j>"			-- label for the "# x was..." line
	//                   };
	//
	// NOTE 2012/10/15 With the changes for P3580, we DO NOT populate
	// NOTE            the {hid, size, attributes} 
	// NOTE            fields in the "WC" sub-section for FOUND/IGNORED
	// NOTE            items.

	SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, statusFlags, &pvhProperties)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash( pCtx, pvhItem, "status", &pvhProperties)  );

	// NOTE We only include a 'gid' field.
	// NOTE We do not include 'alias' fields (since items only present
	// NOTE in the historical cset won't have aliases).
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "gid", pS1Item->pszGid)  );

	// There are 2 ways for an item to be reported as DELETED.
	// [1] It never existing in the baseline.
	// [2] A pending delete in the WD.
	bPresentNow = ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)	// not counting LOST
				   == 0);

	// For convenience, we always put a "path" field at top-level.
	// This is the current path if it exists.  Otherwise, it is a
	// historical-relative repo-path.  If it wasn't present in the
	// historical cset either, we substitute the baseline repo-path.

	if (bPresentNow)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pS1Item->wc.pStringRepoPath))  );
	else if (pS1Item->bPresentIn_H)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pS1Item->h.pStringRepoPath))  );
	else
	{
		SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pS1Data->pWcTx, pS1Item->wc.pLVI, &pStringRepoPath)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pStringRepoPath))  );
	}

	//////////////////////////////////////////////////////////////////
	// create the sub-section for the "@0/" changeset.
	//////////////////////////////////////////////////////////////////

	if (pS1Item->bPresentIn_H)
	{
		SG_vhash * pvhB;		// we do not own this
		
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem, SG_WC__STATUS_SUBSECTION__B, &pvhB)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "was_label", pS1Data->pszWasLabel_H)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "path", SG_string__sz(pS1Item->h.pStringRepoPath))  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "name", pS1Item->h.pszEntryname)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "gid_parent", pS1Item->h.pszGidParent)  );

		switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
		{
		case SG_WC_STATUS_FLAGS__T__DIRECTORY:
			// we do not report 'hid' for directories
			break;

		case SG_WC_STATUS_FLAGS__T__FILE:
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "hid", pS1Item->h.pszHid)  );
			break;

		case SG_WC_STATUS_FLAGS__T__SYMLINK:
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "hid", pS1Item->h.pszHid)  );
			SG_ERR_CHECK(  _s1_get_symlink_target(pCtx, pS1Data->pWcTx, pS1Item->h.pszHid, &pStringSymlinkTarget)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhB, "target", SG_string__sz(pStringSymlinkTarget))  );
			SG_STRING_NULLFREE(pCtx, pStringSymlinkTarget);
			break;

		//case SG_WC_STATUS_FLAGS__T__SUBREPO:
		//case SG_WC_STATUS_FLAGS__T__DEVICE:	cannot happen since we never commit them (so DEVICEs won't be in baseline)
		default:
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "Status: unsupported item type: %s",
							 SG_string__sz(pS1Item->h.pStringRepoPath))  );
		}

		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhB, "attributes", pS1Item->h.attrbits)  );
	}

	//////////////////////////////////////////////////////////////////
	// create the sub-section for the current values.
	//////////////////////////////////////////////////////////////////

	if (bPresentNow)
	{
		SG_vhash * pvhWC;		// we do not own this

		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem, SG_WC__STATUS_SUBSECTION__WC, &pvhWC)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "was_label", pS1Data->pszWasLabel_WC)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "path", SG_string__sz(pS1Item->wc.pStringRepoPath))  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "name", pS1Item->wc.pszEntryname)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "gid_parent", pS1Item->wc.pszGidParent)  );

		if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
		{
			// no current values for hid, attrbits, ....
		}
		else if (statusFlags & (SG_WC_STATUS_FLAGS__U__FOUND | SG_WC_STATUS_FLAGS__U__IGNORED))
		{
			// don't compute HID for uncontrolled things.
		}
		else
		{
			SG_uint64 sizeContent;

			switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
			{
			case SG_WC_STATUS_FLAGS__T__DIRECTORY:
				// we do not report 'hid' for directories
				break;

			case SG_WC_STATUS_FLAGS__T__FILE:
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid( pCtx, pS1Item->wc.pLVI, pS1Data->pWcTx, SG_FALSE,
																			 &pszHid_owned, &sizeContent)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "hid", pszHid_owned)  );
				SG_NULLFREE(pCtx, pszHid_owned);
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "size", (SG_int64)sizeContent)  );
				break;

			case SG_WC_STATUS_FLAGS__T__SYMLINK:
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_content_hid( pCtx, pS1Item->wc.pLVI, pS1Data->pWcTx, SG_FALSE,
																			 &pszHid_owned, &sizeContent)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "hid", pszHid_owned)  );
				SG_NULLFREE(pCtx, pszHid_owned);
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "size", (SG_int64)sizeContent)  );
				SG_ERR_CHECK(  sg_wc_liveview_item__get_current_symlink_target(pCtx, pS1Item->wc.pLVI, pS1Data->pWcTx, &pStringSymlinkTarget)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhWC, "target", SG_string__sz(pStringSymlinkTarget))  );
				SG_STRING_NULLFREE(pCtx, pStringSymlinkTarget);
				break;

			//case SG_WC_STATUS_FLAGS__T__SUBREPO:
			//case SG_WC_STATUS_FLAGS__T__DEVICE:	cannot happen since we never they will always be found/ignored (caught above)
			default:
				SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
								(pCtx, "Status: unsupported item type: %s",
								 SG_string__sz(pS1Item->wc.pStringRepoPath))  );
			}

			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhWC, "attributes", pS1Item->wc.attrbits)  );
		}
	}

	//////////////////////////////////////////////////////////////////
	// Build an array of heading names for the formatter.

#define APPEND_HEADING(name)											\
	SG_STATEMENT(														\
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,				\
													 pvaHeadings,		\
													 (name))  );		\
		)

	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvhItem, "headings", &pvaHeadings)  );

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)
		APPEND_HEADING("Added");

	if (statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)
		APPEND_HEADING("Modified");

	if (statusFlags & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)
		APPEND_HEADING("Attributes");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)
		APPEND_HEADING("Removed");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__RENAMED)
		APPEND_HEADING("Renamed");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MOVED)
		APPEND_HEADING("Moved");

	if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
		APPEND_HEADING("Lost");
	if (statusFlags & SG_WC_STATUS_FLAGS__U__FOUND)
		APPEND_HEADING("Found");
	if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)
		APPEND_HEADING("Ignored");
	if (statusFlags & SG_WC_STATUS_FLAGS__R__RESERVED)
		APPEND_HEADING("Reserved");
	if (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)
		APPEND_HEADING("Sparse");

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaHeadings, &countHeadings)  );
	if (countHeadings == 0)
		APPEND_HEADING("Unchanged");

	//////////////////////////////////////////////////////////////////

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringSymlinkTarget);
	SG_VHASH_NULLFREE(pCtx, pvhProperties);
	SG_NULLFREE(pCtx, pszHid_owned);
}

//////////////////////////////////////////////////////////////////

static void _s1_report(SG_context * pCtx,
					   sg_wc_tx__status1__data * pS1Data,
					   sg_wc_tx__status1__item * pS1Item,
					   SG_varray * pvaStatus)
{
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;

	if (pS1Item->bReported)
		return;
	pS1Item->bReported = SG_TRUE;

	SG_ERR_CHECK(  _s1_compute_flags(pCtx, pS1Data, pS1Item, &statusFlags)  );

#if TRACE_WC_TX_STATUS1
	SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags_to_console(pCtx, statusFlags, pS1Item->pszGid)  );
#endif

	if ((statusFlags & (SG_WC_STATUS_FLAGS__U__MASK
						|SG_WC_STATUS_FLAGS__S__MASK
						|SG_WC_STATUS_FLAGS__C__MASK
						|SG_WC_STATUS_FLAGS__X__MASK
						|SG_WC_STATUS_FLAGS__L__MASK
			 ))
		|| pS1Data->bListUnchanged
		|| ((statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE) && pS1Data->bListSparse)
		|| ((statusFlags & SG_WC_STATUS_FLAGS__R__MASK) && pS1Data->bListReserved)
		)
	{
		SG_vhash * pvhItem;		// we don't own this

		SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaStatus, &pvhItem)  );
		SG_ERR_CHECK(  _s1_do_fixed_keys(pCtx, pS1Data, pS1Item, statusFlags, pvhItem)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Report on everything that was WITHIN this directory in the
 * historical cset.
 *
 */
static void _s1_dive_on_index(SG_context * pCtx,
							  sg_wc_tx__status1__data * pS1Data,
							  const SG_rbtree * prbIndex,
							  const SG_string * pStringRepoPath,
							  SG_uint32 depth,
							  SG_varray * pvaStatus)
{
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey;
	sg_wc_tx__status1__item * pS1Item_k;	// we do not own this
	SG_bool bFound;
	const char * pszPrefix = SG_string__sz(pStringRepoPath);
	SG_uint32 lenPrefix = SG_string__length_in_bytes(pStringRepoPath);

	// TODO 2013/01/10 We need a version of SG_rbtree__find() that returns
	// TODO            an iterator primed to the item found, such that you
	// TODO            can call __iterator__next() and read sequentially
	// TODO            the items following the matched item rather than
	// TODO            always starting the iterator at __first().

	// I hate to do this, but do a linear search on the index (which is
	// sorted by the "@0/repopath") and consider reporting any items
	// contained within this directory (subject to the depth limit).

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, prbIndex,
											  &bFound, &pszKey, (void **)&pS1Item_k)  );
	while (bFound)
	{
		SG_int32 cmp = strncmp(pszKey, pszPrefix, lenPrefix);
		if (cmp > 0)
			break;
		if (cmp == 0)
		{
			// count the slashes in the remainder of the string
			// not counting any trailing slash.
			const char * pszTail = pszKey + lenPrefix;
			SG_uint32 nrSlashes = 0;
			while (pszTail[0])
			{
				if ((pszTail[0] == '/') && (pszTail[1]))
					nrSlashes++;
				pszTail++;
			}

			if (nrSlashes < depth)
				SG_ERR_CHECK(  _s1_report(pCtx, pS1Data, pS1Item_k, pvaStatus)  );
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter,
												 &bFound, &pszKey, (void **)&pS1Item_k)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

static void _s1_dive(SG_context * pCtx,
					 sg_wc_tx__status1__data * pS1Data,
					 sg_wc_tx__status1__item * pS1Item,
					 SG_uint32 depth,
					 SG_varray * pvaStatus)
{
	if ((pS1Item->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY) && (depth > 0))
	{
		if (pS1Item->bPresentIn_H)
			SG_ERR_CHECK(  _s1_dive_on_index(pCtx, pS1Data, pS1Data->prbIndex_H,
											 pS1Item->h.pStringRepoPath, depth,
											 pvaStatus)  );
		if (pS1Item->bPresentIn_WC)
			SG_ERR_CHECK(  _s1_dive_on_index(pCtx, pS1Data, pS1Data->prbIndex_WC,
											 pS1Item->wc.pStringRepoPath, depth,
											 pvaStatus)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Report the items that match the (repo-path, depth) criteria
 * where the repo-path is interpreted relative to the historical
 * cset.
 *
 */
void sg_wc_tx__status1__h(SG_context * pCtx,
						  sg_wc_tx__status1__data * pS1Data,
						  const SG_string * pStringRepoPath,
						  SG_uint32 depth,
						  SG_varray * pvaStatus)
{
	SG_string * pStringRepoPath_Allocated = NULL;
	sg_wc_tx__status1__item * pS1Item;		// we do not own this
	SG_bool bFound;

#if defined(DEBUG)
	{
		const char * psz = SG_string__sz(pStringRepoPath);
		if (strncmp(psz, SG_VV2__REPO_PATH_PREFIX__0, 3) != 0)	// "@0/"
			SG_ERR_THROW(  SG_ERR_INVALIDARG  );
	}
#endif

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pS1Data->prbIndex_H, SG_string__sz(pStringRepoPath),
								   &bFound, (void **)&pS1Item)  );
	if (!bFound)
	{
		SG_bool bHasFinalSlash = SG_FALSE;
		SG_ERR_CHECK(  SG_repopath__has_final_slash(pCtx, pStringRepoPath, &bHasFinalSlash)  );
		if (bHasFinalSlash)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Item '%s' not found in changeset '%s'.",
							 SG_string__sz(pStringRepoPath), pS1Data->pszHidCSet)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pStringRepoPath_Allocated, pStringRepoPath)  );
		SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pStringRepoPath_Allocated)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pS1Data->prbIndex_H, SG_string__sz(pStringRepoPath_Allocated),
									   &bFound, (void **)&pS1Item)  );
		if (!bFound)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Item '%s' not found in changeset '%s'.",
							 SG_string__sz(pStringRepoPath), pS1Data->pszHidCSet)  );
		pStringRepoPath = pStringRepoPath_Allocated;
	}

	SG_ERR_CHECK(  _s1_report(pCtx, pS1Data, pS1Item, pvaStatus)  );

	if ((pS1Item->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY) && (depth > 0))
		SG_ERR_CHECK(  _s1_dive(pCtx, pS1Data, pS1Item, depth, pvaStatus)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Allocated);
}

/**
 * Report the items that match the (repo-path, depth) criteria
 * where the repo-path is interpreted relative to the current/live WD.
 *
 */
void sg_wc_tx__status1__wc(SG_context * pCtx,
						   sg_wc_tx__status1__data * pS1Data,
						   const SG_string * pStringRepoPath,
						   SG_uint32 depth,
						   SG_varray * pvaStatus)
{
	SG_string * pStringRepoPath_Allocated = NULL;
	sg_wc_tx__status1__item * pS1Item;		// we do not own this
	SG_bool bFound;

#if defined(DEBUG)
	{
		const char * psz = SG_string__sz(pStringRepoPath);
		if (strncmp(psz, "@/", 2) != 0)
			SG_ERR_THROW(  SG_ERR_INVALIDARG  );
	}
#endif

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pS1Data->prbIndex_WC, SG_string__sz(pStringRepoPath),
								   &bFound, (void **)&pS1Item)  );
	if (!bFound)
	{
		SG_bool bHasFinalSlash = SG_FALSE;
		SG_ERR_CHECK(  SG_repopath__has_final_slash(pCtx, pStringRepoPath, &bHasFinalSlash)  );
		if (bHasFinalSlash)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Item '%s' not found in working copy.",
							 SG_string__sz(pStringRepoPath))  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pStringRepoPath_Allocated, pStringRepoPath)  );
		SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pStringRepoPath_Allocated)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pS1Data->prbIndex_WC, SG_string__sz(pStringRepoPath_Allocated),
									   &bFound, (void **)&pS1Item)  );
		if (!bFound)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Item '%s' not found in working copy.",
							 SG_string__sz(pStringRepoPath))  );
		pStringRepoPath = pStringRepoPath_Allocated;
	}

	SG_ERR_CHECK(  _s1_report(pCtx, pS1Data, pS1Item, pvaStatus)  );

	if ((pS1Item->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY) && (depth > 0))
		SG_ERR_CHECK(  _s1_dive(pCtx, pS1Data, pS1Item, depth, pvaStatus)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_Allocated);
}

/**
 * Report the items that match the (gid-repo-path, depth) criteria.
 *
 */
void sg_wc_tx__status1__g(SG_context * pCtx,
						  sg_wc_tx__status1__data * pS1Data,
						  const char * pszGid,
						  SG_uint32 depth,
						  SG_varray * pvaStatus)
{
	sg_wc_tx__status1__item * pS1Item;		// we do not own this
	SG_bool bFound;

	if ((pszGid[0] != 'g') && (pszGid[0] != 't'))
		SG_ERR_THROW(  SG_ERR_INVALIDARG  );

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pS1Data->prbItems, pszGid,
								   &bFound, (void **)&pS1Item)  );
	if (!bFound)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Item '@%s' not found in working copy.", pszGid)  );

	SG_ERR_CHECK(  _s1_report(pCtx, pS1Data, pS1Item, pvaStatus)  );

	if ((pS1Item->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY) && (depth > 0))
		SG_ERR_CHECK(  _s1_dive(pCtx, pS1Data, pS1Item, depth, pvaStatus)  );

fail:
	return;
}
