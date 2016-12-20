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
 * @file sg_wc_tx__cache.c
 *
 * @details Routines to maintain the various caches that
 * we keep inside the TX.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Add the given scandir to the scandir-cache and take ownership of it.
 *
 * We DO NOT null your pointer.
 *
 */
void sg_wc_tx__cache__add_prescan_dir(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_prescan_dir * pPrescanDir)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__add__with_assoc(pCtx,
														  pWcTx->prb64PrescanDirCache,
														  pPrescanDir->uiAliasGidDir,
														  pPrescanDir)  );
}

void sg_wc_tx__cache__find_prescan_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   SG_uint64 uiAliasGid,
									   SG_bool * pbFound,
									   sg_wc_prescan_dir ** ppPrescanDir)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__find(pCtx,
											   pWcTx->prb64PrescanDirCache,
											   uiAliasGid,
											   pbFound,
											   (void **)ppPrescanDir)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__cache__add_prescan_row(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_prescan_row * pPrescanRow)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__add__with_assoc(pCtx,
														  pWcTx->prb64PrescanRowCache,
														  pPrescanRow->uiAliasGid,
														  pPrescanRow)  );

	if (pPrescanRow->uiAliasGid == pWcTx->uiAliasGid_Root)
	{
		// keep a copy of the pointer to the root row in the TX
		// because we'll probably be fetching it quite frequently.
		pWcTx->pPrescanRow_Root = pPrescanRow;
	}
}

void sg_wc_tx__cache__find_prescan_row(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   SG_uint64 uiAliasGid,
									   SG_bool * pbFound,
									   sg_wc_prescan_row ** ppPrescanRow)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__find(pCtx,
											   pWcTx->prb64PrescanRowCache,
											   uiAliasGid,
											   pbFound,
											   (void **)ppPrescanRow)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__cache__add_liveview_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   sg_wc_liveview_dir * pLVD)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__add__with_assoc(pCtx,
														  pWcTx->prb64LiveViewDirCache,
														  pLVD->uiAliasGidDir,
														  pLVD)  );
}

void sg_wc_tx__cache__find_liveview_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   SG_uint64 uiAliasGid,
									   SG_bool * pbFound,
									   sg_wc_liveview_dir ** ppLiveViewDir)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__find(pCtx,
											   pWcTx->prb64LiveViewDirCache,
											   uiAliasGid,
											   pbFound,
											   (void **)ppLiveViewDir)  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_tx__cache__add_liveview_item(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_liveview_item * pLVI)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__add__with_assoc(pCtx,
														  pWcTx->prb64LiveViewItemCache,
														  pLVI->uiAliasGid,
														  pLVI)  );

	if (pLVI->uiAliasGid == pWcTx->uiAliasGid_Root)
	{
		// keep a copy of the pointer to the root item in the TX
		// because we'll probably be fetching it quite frequently.
		pWcTx->pLiveViewItem_Root = pLVI;
	}
}

void sg_wc_tx__cache__find_liveview_item(SG_context * pCtx,
									   SG_wc_tx * pWcTx,
									   SG_uint64 uiAliasGid,
									   SG_bool * pbFound,
									   sg_wc_liveview_item ** ppLiveViewItem)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__find(pCtx,
											   pWcTx->prb64LiveViewItemCache,
											   uiAliasGid,
											   pbFound,
											   (void **)ppLiveViewItem)  );
}


