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
 * @file sg_wc_tx__rp__status.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

static SG_rbtree_ui64_foreach_callback _dive_cb;

//////////////////////////////////////////////////////////////////

struct _dive_data
{
	SG_wc_tx *			pWcTx;
	SG_uint32			depth;
	SG_bool				bListUnchanged;
	SG_bool				bNoIgnores;
	SG_bool				bNoTSC;
	SG_bool				bListSparse;
	SG_bool				bListReserved;
	SG_varray *			pvaStatus;
	const char *		pszWasLabel_l;
	const char *		pszWasLabel_r;
};

static void _dive_cb(SG_context * pCtx,
					 SG_uint64 uiAliasGid,
					 void * pVoid_LVI,
					 void * pVoid_Data)
{
	struct _dive_data * pDiveData = (struct _dive_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;

	SG_UNUSED( uiAliasGid );

	SG_ERR_CHECK_RETURN(  sg_wc_tx__rp__status__lvi(pCtx,
													pDiveData->pWcTx,
													pLVI,
													pDiveData->depth,
													pDiveData->bListUnchanged,
													pDiveData->bNoIgnores,
													pDiveData->bNoTSC,
													pDiveData->bListSparse,
													pDiveData->bListReserved,
													pDiveData->pszWasLabel_l,
													pDiveData->pszWasLabel_r,
													pDiveData->pvaStatus)  );
}

/**
 * Compute diving canonical status when given a LVI.
 *
 */
void sg_wc_tx__rp__status__lvi(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   sg_wc_liveview_item * pLVI,
							   SG_uint32 depth,
							   SG_bool bListUnchanged,
							   SG_bool bNoIgnores,
							   SG_bool bNoTSC,
							   SG_bool bListSparse,
							   SG_bool bListReserved,
							   const char * pszWasLabel_l,
							   const char * pszWasLabel_r,
							   SG_varray * pvaStatus)
{
	SG_wc_status_flags statusFlags;

	SG_ERR_CHECK(  sg_wc__status__compute_flags(pCtx, pWcTx,
												pLVI,
												bNoIgnores,
												bNoTSC,
												&statusFlags)  );

	SG_ERR_CHECK(  sg_wc__status__append(pCtx, pWcTx, pLVI,
										 statusFlags,
										 bListUnchanged,
										 bListSparse,
										 bListReserved,
										 pszWasLabel_l, pszWasLabel_r,
										 pvaStatus)  );

	if ((depth > 0) && (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY))
	{
		if (statusFlags & SG_WC_STATUS_FLAGS__R__MASK)
		{
			// We *NEVER* want to dive into RESERVED directories
			// *REGARLESS* of whether the caller wants to know
			// about *THIS* item.
		}
		else if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)
		{
			// For W6508:
			// Don't dive into ignored directories; we just want to report
			// that the directory is itself ignored, so we don't care what
			// is *inside* it.
		}
		else
		{
			sg_wc_liveview_dir * pLVD;		// we do not own this
			struct _dive_data dive_data;
		
			dive_data.pWcTx = pWcTx;
			dive_data.depth = depth - 1;
			dive_data.bListUnchanged = bListUnchanged;
			dive_data.bNoIgnores = bNoIgnores;
			dive_data.bNoTSC = bNoTSC;
			dive_data.bListSparse = bListSparse;
			dive_data.bListReserved = bListReserved;
			dive_data.pvaStatus = pvaStatus;
			dive_data.pszWasLabel_l = pszWasLabel_l;
			dive_data.pszWasLabel_r = pszWasLabel_r;

			SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
			SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _dive_cb, &dive_data)  );
		}
	}

fail:
	return;
}
