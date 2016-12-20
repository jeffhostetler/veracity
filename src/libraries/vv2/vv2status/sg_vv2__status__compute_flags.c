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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Compute the SG_wc_status_flags for the differences in the
 * versions of this entry.
 *
 * WARNING: This code must do something similar
 * to what sg_wc__status__compute_flags() does for
 * live items.
 *
 * We place the result in the pOD->statusFlags field.
 * 
 */
void sg_vv2__status__compute_flags(SG_context * pCtx,
								   sg_vv2status_od * pOD)
{
	const sg_vv2status_odi * pInst_orig;
	const sg_vv2status_odi * pInst_dest;

	pInst_orig = pOD->apInst[SG_VV2__OD_NDX_ORIG];
	pInst_dest = pOD->apInst[SG_VV2__OD_NDX_DEST];

	SG_ERR_CHECK(  sg_vv2__status__compute_flags2(pCtx,
												  ((pInst_orig) ? pInst_orig->pTNE : NULL),
												  ((pInst_orig) ? pInst_orig->bufParentGid : NULL),
												  ((pInst_dest) ? pInst_dest->pTNE : NULL),
												  ((pInst_dest) ? pInst_dest->bufParentGid : NULL),
												  &pOD->statusFlags)  );
fail:
	return;
}

void sg_vv2__status__compute_flags2(SG_context * pCtx,
									const SG_treenode_entry * pTNE_orig,
									const char * pszParentGid_orig,
									const SG_treenode_entry * pTNE_dest,
									const char * pszParentGid_dest,
									SG_wc_status_flags * pStatusFlags)
{
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__ZERO;
	SG_uint32 nrChangeBits = 0;
	SG_treenode_entry_type tneType;

	if (pTNE_orig && pTNE_dest)
	{
		// entry is present in both versions of the tree.  we need to do a simple diff.

		SG_treenode_entry_type tneType;
		const char * pszEntryname_orig;
		const char * pszEntryname_dest;
		SG_int64 attrbits_orig;
		SG_int64 attrbits_dest;
		const char * pszHidContent_orig;
		const char * pszHidContent_dest;

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTNE_orig, &tneType)  );
		SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx, tneType, &statusFlags)  );

		if (strcmp(pszParentGid_orig, pszParentGid_dest) != 0)
		{
			statusFlags |= SG_WC_STATUS_FLAGS__S__MOVED;
			nrChangeBits++;
		}
		
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTNE_orig, &pszEntryname_orig)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTNE_dest, &pszEntryname_dest)  );
		if (strcmp(pszEntryname_orig, pszEntryname_dest) != 0)
		{
			statusFlags |= SG_WC_STATUS_FLAGS__S__RENAMED;
			nrChangeBits++;
		}

		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTNE_orig, &pszHidContent_orig)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTNE_dest, &pszHidContent_dest)  );
		if (strcmp(pszHidContent_orig, pszHidContent_dest) != 0)
		{
			if (tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
			{
				statusFlags |= SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED;
				nrChangeBits++;
			}
		}

		SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pTNE_orig, &attrbits_orig)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pTNE_dest, &attrbits_dest)  );
		if (attrbits_orig != attrbits_dest)
		{
			statusFlags |= SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;
			nrChangeBits++;
		}

	}
	else if (pTNE_orig)
	{
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTNE_orig, &tneType)  );
		SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx, tneType, &statusFlags)  );
		statusFlags |= SG_WC_STATUS_FLAGS__S__DELETED;
		nrChangeBits++;
	}
	else if (pTNE_dest)
	{
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTNE_dest, &tneType)  );
		SG_ERR_CHECK(  SG_wc__status__tne_type_to_flags(pCtx, tneType, &statusFlags)  );
		statusFlags |= SG_WC_STATUS_FLAGS__S__ADDED;
		nrChangeBits++;
	}
	else
	{
		// ?? entry is not present either cset0 nor cset1.  this cannot happen.
		SG_ASSERT_RELEASE_FAIL(  (0)  );
	}

	if (nrChangeBits > 1)
		statusFlags |= SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE;

	*pStatusFlags = statusFlags;
	
fail:
	return;
}
