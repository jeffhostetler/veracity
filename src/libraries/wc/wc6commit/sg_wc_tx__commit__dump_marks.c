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
 * @file sg_wc_tx__commit__dump_marks.c
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

#if defined(DEBUG)
struct dump_data
{
	SG_wc_tx * pWcTx;
	SG_uint32 indent;
};

static SG_rbtree_ui64_foreach_callback _dump_marks__dir__current__cb;
static SG_rbtree_ui64_foreach_callback _dump_marks__dir__moved_out__cb;

static void _dump_marks__lvi(SG_context * pCtx,
							 SG_wc_tx * pWcTx,
							 sg_wc_liveview_item * pLVI,
							 SG_uint32 indent,
							 const char * pszPrefix,
							 SG_bool bDive)
{
	SG_int_to_string_buffer bufui64;

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "%*c %02x 0x%s %s %s\n",
							   indent, ' ',
							   pLVI->commitFlags,
							   SG_uint64_to_sz__hex((SG_uint64)pLVI->statusFlags_commit,bufui64),
							   pszPrefix,
							   SG_string__sz(pLVI->pStringEntryname))  );

	if ((bDive) && (pLVI->tneType == SG_TREENODEENTRY_TYPE_DIRECTORY))
	{
		sg_wc_liveview_dir * pLVD;		// we do not own this
		struct dump_data dump_data;

		dump_data.pWcTx = pWcTx;
		dump_data.indent = indent + 4;

		SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_dir(pCtx, pWcTx, pLVI, &pLVD)  );
		if (pLVD->prb64LiveViewItems)
		{
			SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx,
													   pLVD,
													   _dump_marks__dir__current__cb,
													   &dump_data)  );
		}
		if (pLVD->prb64MovedOutItems)
		{
			SG_ERR_CHECK(  sg_wc_liveview_dir__foreach_moved_out(pCtx,
																 pLVD,
																 _dump_marks__dir__moved_out__cb,
																 &dump_data)  );
		}
	}

fail:
	return;
}

static void _dump_marks__dir__moved_out__cb(SG_context * pCtx,
											SG_uint64 uiAliasGid,
											void * pVoid_Assoc,
											void * pVoid_Data)
{
	struct dump_data * pDumpData = (struct dump_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI; 	// we do not own this
	SG_bool bKnown;

	SG_UNUSED( pVoid_Assoc );

	SG_ERR_CHECK_RETURN(  sg_wc_tx__liveview__fetch_random_item(pCtx, pDumpData->pWcTx, uiAliasGid,
																&bKnown, &pLVI)  );
	SG_ASSERT( (bKnown) );


	SG_ERR_CHECK_RETURN(  _dump_marks__lvi(pCtx, pDumpData->pWcTx, pLVI, pDumpData->indent, "(moved-out)", SG_FALSE)  );
}

static void _dump_marks__dir__current__cb(SG_context * pCtx,
										  SG_uint64 uiAliasGid,
										  void * pVoid_LVI,
										  void * pVoid_Data)
{
	struct dump_data * pDumpData = (struct dump_data *)pVoid_Data;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoid_LVI;

	SG_UNUSED( uiAliasGid );
	
	SG_ERR_CHECK_RETURN(  _dump_marks__lvi(pCtx, pDumpData->pWcTx, pLVI, pDumpData->indent, "           ", SG_TRUE)  );
}

/**
 * Dump the commitFlags for the entire tree.
 *
 */
void sg_wc_tx__commit__dump_marks(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const char * pszLabel)
{
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "CommitDump: %s\n",
							   pszLabel)  );

	SG_ERR_CHECK_RETURN(  _dump_marks__lvi(pCtx, pWcTx, pWcTx->pLiveViewItem_Root, 0, "           ", SG_TRUE)  );
}
#endif

