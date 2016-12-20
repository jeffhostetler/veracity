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
 * @file sg_wc_tx__commit__queue__utils.c
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

/**
 * Add a TNE for the given item to the TN of the directory.
 * The given fields may be the either the current or baseline
 * values (we don't care here); use whichever values you want
 * to have in the synthetic/hybrid treenode that needs to be
 * created for the full/partial commit.
 *
 * We optionally return a (read-only) handle the TNE.
 * you DO NOT own this.
 *
 */
void sg_wc_tx__commit__queue__utils__add_tne(SG_context * pCtx,
											 SG_treenode * pTN,
											 const char * pszGid,
											 const char * pszEntryname,
											 SG_treenode_entry_type tneType,
											 const char * pszHidContent,
											 SG_int64 attrbits,
											 const SG_treenode_entry ** ppTNE_ref)
{
	SG_treenode_entry * pTNE_allocated = NULL;
	const SG_treenode_entry * pTNE_ref;		// we do not own this

	SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC(pCtx, &pTNE_allocated)  );

	SG_ERR_CHECK(  SG_treenode_entry__set_entry_name(    pCtx, pTNE_allocated, pszEntryname)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_entry_type(    pCtx, pTNE_allocated, tneType)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_hid_blob(      pCtx, pTNE_allocated, pszHidContent)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_attribute_bits(pCtx, pTNE_allocated, attrbits)  );

	pTNE_ref = pTNE_allocated;	// next line steals ownership
	SG_ERR_CHECK(  SG_treenode__add_entry(pCtx, pTN, pszGid, &pTNE_allocated)  );

	if (ppTNE_ref)
		*ppTNE_ref = pTNE_ref;
	return;

fail:
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTNE_allocated);
}

//////////////////////////////////////////////////////////////////

/**
 * Add the given "entryname" to the portability-collider and THROW
 * if there are any issues/problems.
 *
 * Since a PARTIAL COMMIT has the ability to include some changes
 * and not include other changes, we have to re-verify that the
 * set of items to be included in a TREENODE is well-defined.
 *
 * pszEntryname contains the item's entryname as we want it to
 * appear in the final TREENODE/TREENODEENTRY.
 *
 * pStringRepoPath contains the item's path for error messages.
 * the entryname in this path may be different from the given
 * entryname (such as when we want to test with the baseline
 * version of the entryname for a non-participating item, but
 * want to complain using the current pathname).
 * 
 */
void sg_wc_tx__commit__queue__utils__check_port(SG_context * pCtx,
												SG_wc_port * pPort,
												const char * pszEntryname,
												SG_treenode_entry_type tneType,
												const SG_string * pStringRepoPath)
{
	const SG_string * pStringItemLog = NULL;		// we don't own this
	SG_wc_port_flags portFlags;	
	SG_bool bIsDuplicate;

	SG_ERR_CHECK(  SG_wc_port__add_item(pCtx, pPort, NULL, pszEntryname, tneType,
										&bIsDuplicate)  );
	if (bIsDuplicate)
		SG_ERR_THROW2(  SG_ERR_PENDINGTREE_PARTIAL_CHANGE_COLLISION,
						(pCtx, "Partial commit would cause collision: '%s' (%s)",
						 pszEntryname,
						 SG_string__sz(pStringRepoPath))  );

	SG_ERR_CHECK(  SG_wc_port__get_item_result_flags(pCtx, pPort, pszEntryname,
													 &portFlags, &pStringItemLog)  );
	if (portFlags)
		SG_ERR_THROW2(  SG_ERR_WC_PORT_FLAGS,
						(pCtx, "Partial commit could (potentially) cause problems: '%s' (%s)\n%s",
						 pszEntryname,
						 SG_string__sz(pStringRepoPath),
						 SG_string__sz(pStringItemLog))  );

	// the item does not cause problems.

fail:
	return;
}
