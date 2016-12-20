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
 * @file sg_wc_prescan_dir.c
 *
 * @details Routines to combine a set of PCTNE rows and READDIR rows
 * to give a complete view of the contents of a directory AS IT EXISTED
 * AT THE *START* OF THE TRANSACTION.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_dir__free(SG_context * pCtx, sg_wc_prescan_dir * pPrescanDir)
{
	if (!pPrescanDir)
		return;

	SG_STRING_NULLFREE(pCtx, pPrescanDir->pStringRefRepoPath);

	// we do not own the scanrows; these are owned by the TX cache.
	SG_RBTREE_UI64_NULLFREE(pCtx, pPrescanDir->prb64PrescanRows);

	SG_RBTREE_UI64_NULLFREE(pCtx, pPrescanDir->prb64MovedOutItems);

	SG_NULLFREE(pCtx, pPrescanDir);
}

void sg_wc_prescan_dir__alloc(SG_context * pCtx, sg_wc_prescan_dir ** ppPrescanDir,
							  SG_uint64 uiAliasGidDir,
							  const SG_string * pStringRefRepoPath)
{
	sg_wc_prescan_dir * pPrescanDir = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPrescanDir)  );
	pPrescanDir->uiAliasGidDir = uiAliasGidDir;
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pPrescanDir->pStringRefRepoPath, pStringRefRepoPath)  );

	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &pPrescanDir->prb64PrescanRows)  );

	// defer alloc of prb64MovedOutItems until we observe one.

	*ppPrescanDir = pPrescanDir;
	return;

fail:
	SG_WC_PRESCAN_DIR__NULLFREE(pCtx, pPrescanDir);
}

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_dir__foreach(SG_context * pCtx,
								const sg_wc_prescan_dir * pPrescanDir,
								SG_rbtree_ui64_foreach_callback * pfn_cb,
								void * pVoidData)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree_ui64__foreach(pCtx,
												  pPrescanDir->prb64PrescanRows,
												  pfn_cb,
												  pVoidData)  );
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
static SG_rbtree_ui64_foreach_callback _dump_item_in_dir;

static void _dump_item_in_dir(SG_context * pCtx,
							  SG_uint64 uiAliasGid_k,
							  void * pVoid_Assoc_k,
							  void * pVoid_Data)
{
	sg_wc_prescan_row * pRow_k = (sg_wc_prescan_row *)pVoid_Assoc_k;
	SG_int_to_string_buffer bufui64;

	SG_UNUSED( uiAliasGid_k );
	SG_UNUSED( pVoid_Data );

	// This is a short summary of what we know about the row.
	// 
	// TODO think about adding the parent and entryname as they
	// TODO appeared in the baseline.

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "    Item: [tneType %d][flags 0x%08x][Seen %d][Base %d][Pend %d] %s %s\n",
							   (SG_uint32)pRow_k->tneType,
							   (SG_uint32)pRow_k->scan_flags_Ref,
							   (pRow_k->pRD != NULL),			// was observed in directory
							   (pRow_k->pTneRow != NULL),		// was present in baseline
							   (pRow_k->pPcRow_Ref != NULL),	// had pended structural change before start of this TX
							   SG_uint64_to_sz(pRow_k->uiAliasGid, bufui64),
							   SG_string__sz(pRow_k->pStringEntryname))  );
}

void sg_wc_prescan_dir_debug__dump_to_console(SG_context * pCtx,
											  const sg_wc_prescan_dir * pPrescanDir)
{
	SG_int_to_string_buffer bufui64;
	SG_uint32 nrMovedOut = 0;

	if (pPrescanDir->prb64MovedOutItems)
		SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx,
											   // SG_rbtree_ui64 is "subclass" of SG_rbtree
											   (SG_rbtree *)pPrescanDir->prb64MovedOutItems,
											   &nrMovedOut)  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "PrescanDirDump: %s %s [nrMovedOut %d]\n",
							   SG_uint64_to_sz(pPrescanDir->uiAliasGidDir, bufui64),
							   SG_string__sz(pPrescanDir->pStringRefRepoPath),
							   nrMovedOut)  );
	SG_ERR_CHECK_RETURN(  sg_wc_prescan_dir__foreach(pCtx,
													 pPrescanDir,
													 _dump_item_in_dir,
													 NULL)  );
}
#endif
