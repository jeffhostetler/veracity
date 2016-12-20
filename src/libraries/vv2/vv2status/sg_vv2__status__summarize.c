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
 * @file sg_vv2__status__summarize.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

struct _summary_data
{
	sg_vv2status * pST;				// we do not own this
	SG_varray * pvaStatus;			// we own this
};

//////////////////////////////////////////////////////////////////

/**
 * Build the canonical status VHASH of an item (suitable for
 * appending to a VARRAY of statuses).
 * 
 * Note: the layout of fields here needs to match the layout
 * in sg_wc__status__append.c
 *
 */
void sg_vv2__status__alloc_item(SG_context * pCtx,
								SG_repo * pRepo,
								const char * pszLabel_0,
								const char * pszLabel_1,
								const char * pszWasLabel_0,
								const char * pszWasLabel_1,
								SG_wc_status_flags statusFlags,
								const char * pszGid,
								const SG_string * pStringRefRepoPath,
								const SG_treenode_entry * ptne_0,
								const char * pszParentGid_0,
								const SG_string * pStringCurRepoPath,
								const SG_treenode_entry * ptne_1,
								const char * pszParentGid_1,
								SG_vhash ** ppvhItem)
{
	SG_vhash * pvhItem = NULL;
	SG_vhash * pvhProperties = NULL;
	SG_varray * pvaHeadings = NULL;		// we do not own this
	SG_byte * pBuffer = NULL;
	SG_uint64 uSize = 0;
	
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhItem)  );

	// <item> ::= { "status"    : <properties>,
	//              "gid"       : "<gid>",
	//              "path"      : "<path>",
	//              "headings"  : [ "<heading_k>" ],	-- list of headers for classic_formatter.
	//              "<label_0>" : <sub-section>,		-- if present in cset[0]
	//              "<label_1>" : <sub-section>			-- if present in cset[1]
	//            };
	//
	// All fields in a <sub-section> refer to the item relative to that state.
	// 
	// <sub-section> ::= { "path"           : "<path_j>",
	//                     "name"           : "<name_j>",
	//                     "gid_parent"     : "<gid_parent_j>",
	//                     "hid"            : "<hid_j>",			-- when not a directory
	//                     "attributes"     : <attrbits_j>
	//                     "was_label"      : "<label_j>"			-- label for the "# x was..." line
	//                   };
	//

	SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, statusFlags, &pvhProperties)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash( pCtx, pvhItem, "status", &pvhProperties)  );
	
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "gid", pszGid)  );


	// For convenience, we put a "path" field at the top-level.
	// This is the current path if it exists.

	if (pStringCurRepoPath)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pStringCurRepoPath))  );
	else
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pStringRefRepoPath))  );

	// create the sub-section for cset[0].

	if (ptne_0)
	{
		SG_vhash * pvhSub0;
		const char * pszEntryname;
		const char * pszHid;
		SG_int64 attrbits;

		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem, pszLabel_0, &pvhSub0)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub0, "was_label", pszWasLabel_0)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub0, "path", SG_string__sz(pStringRefRepoPath))  );

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, ptne_0, &pszEntryname)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub0, "name", pszEntryname)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub0, "gid_parent", pszParentGid_0)  );
		
		switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
		{
		case SG_WC_STATUS_FLAGS__T__DIRECTORY:
			break;

		case SG_WC_STATUS_FLAGS__T__FILE:
			SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_0, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub0, "hid", pszHid)  );
			break;

		case SG_WC_STATUS_FLAGS__T__SYMLINK:
			SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_0, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub0, "hid", pszHid)  );
			SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pRepo, pszHid, &pBuffer, &uSize)  );
			SG_ERR_CHECK(  SG_vhash__add__string__buflen(pCtx, pvhSub0, "target",
														 (const char *)pBuffer, (SG_uint32)uSize)  );
			SG_NULLFREE(pCtx, pBuffer);
			break;

		default:
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "sg_vv2__status: item '%s' of unknown type.", SG_string__sz(pStringRefRepoPath))  );
		}

		SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, ptne_0, &attrbits)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhSub0, "attributes", attrbits)  );
	}

	// create the sub-section for cset[1].

	if (ptne_1)
	{
		SG_vhash * pvhSub1;
		const char * pszEntryname;
		const char * pszHid;
		SG_int64 attrbits;

		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem, pszLabel_1, &pvhSub1)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub1, "was_label", pszWasLabel_1)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub1, "path", SG_string__sz(pStringCurRepoPath))  );

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, ptne_1, &pszEntryname)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub1, "name", pszEntryname)  );

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub1, "gid_parent", pszParentGid_1)  );
		
		switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
		{
		case SG_WC_STATUS_FLAGS__T__DIRECTORY:
			break;

		case SG_WC_STATUS_FLAGS__T__FILE:
			SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_1, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub1, "hid", pszHid)  );
			break;

		case SG_WC_STATUS_FLAGS__T__SYMLINK:
			SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_1, &pszHid)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub1, "hid", pszHid)  );
			SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pRepo, pszHid, &pBuffer, &uSize)  );
			SG_ERR_CHECK(  SG_vhash__add__string__buflen(pCtx, pvhSub1, "target",
														 (const char *)pBuffer, (SG_uint32)uSize)  );
			SG_NULLFREE(pCtx, pBuffer);
			break;

		default:
			SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
							(pCtx, "sg_vv2__status: item '%s' of unknown type.", SG_string__sz(pStringRefRepoPath))  );
		}

		SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, ptne_1, &attrbits)  );
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhSub1, "attributes", attrbits)  );
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

	SG_ASSERT(  ((statusFlags & ((SG_WC_STATUS_FLAGS__A__MASK & ~SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE)
								 |SG_WC_STATUS_FLAGS__U__MASK
								 |SG_WC_STATUS_FLAGS__S__MERGE_CREATED
								 |SG_WC_STATUS_FLAGS__X__MASK))
				 == 0)  );

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

#if defined(DEBUG)
	{
		SG_uint32 countHeadings = 0;
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaHeadings, &countHeadings)  );
		SG_ASSERT(  (countHeadings > 0)  );
	}
#endif

	SG_RETURN_AND_NULL( pvhItem, ppvhItem );
				   
fail:
	SG_VHASH_NULLFREE(pCtx, pvhProperties);
	SG_VHASH_NULLFREE(pCtx, pvhItem);
}

//////////////////////////////////////////////////////////////////

/**
 * Add this item to the canonical status.
 *
 * WARNING: This code must do something similar to what
 * sg_wc__status__append() does for live items.
 *
 */
static void _summarize__add_item(SG_context * pCtx,
								 struct _summary_data * pData,
								 sg_vv2status_od * pOD)
{
	SG_vhash * pvhItem = NULL;

	// The item potentially has 2 unique paths:
	// [a] the path that it had in cset[0] (if present).  (aka ref_path)
	// [b] the path that it had in cset[1] (if present).  (aka current path)
	// Both paths are properly domain-prefixed so that they are not ambiguous.
	// 
	// (In the pendingtree version of the code we would in the cases of
	// moves/renames attempt to compute the current-path-of-the-previous-parent.
	// We NO LONGER do this; now we always reference the item with a cset-qualified
	// path using the domain prefix.)

	SG_ERR_CHECK(  sg_vv2__status__compute_repo_path(pCtx, pData->pST, pOD)  );
	SG_ERR_CHECK(  sg_vv2__status__compute_ref_repo_path(pCtx, pData->pST, pOD)  );
	SG_ASSERT_RELEASE_FAIL( (pOD->pStringCurRepoPath || pOD->pStringRefRepoPath) );

	SG_ERR_CHECK(  sg_vv2__status__alloc_item(pCtx, pData->pST->pRepo,
											  pData->pST->pszLabel_0, pData->pST->pszLabel_1,
											  pData->pST->pszWasLabel_0, pData->pST->pszWasLabel_1,
											  pOD->statusFlags,
											  pOD->bufGidObject,
											  pOD->pStringRefRepoPath,
											  ((pOD->apInst[0]) ? pOD->apInst[0]->pTNE : NULL),
											  ((pOD->apInst[0]) ? pOD->apInst[0]->bufParentGid : NULL),
											  pOD->pStringCurRepoPath,
											  ((pOD->apInst[1]) ? pOD->apInst[1]->pTNE : NULL),
											  ((pOD->apInst[1]) ? pOD->apInst[1]->bufParentGid : NULL),
											  &pvhItem)  );
	SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pData->pvaStatus, &pvhItem)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhItem);
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _summarize_cb;

static void _summarize_cb(SG_context * pCtx,
						  const char * pszKey_gid,
						  void * pVoidAssoc,
						  void * pVoidData)
{
	struct _summary_data * pData = (struct _summary_data *)pVoidData;
	sg_vv2status_od * pOD = (sg_vv2status_od *)pVoidAssoc;

	SG_UNUSED( pszKey_gid );

	SG_ERR_CHECK(  sg_vv2__status__compute_flags(pCtx, pOD)  );
	if (pOD->statusFlags & (SG_WC_STATUS_FLAGS__S__MASK
							|SG_WC_STATUS_FLAGS__C__MASK))
		SG_ERR_CHECK(  _summarize__add_item(pCtx, pData, pOD)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Run thru the list of all sg_vv2status_od rows that we visited and identify
 * the ones that represent a change of some kind and add them to the
 * canonical status.
 *
 */
void sg_vv2__status__summarize(SG_context * pCtx,
							   sg_vv2status * pST,
							   SG_varray ** ppvaStatus)
{
	struct _summary_data data;

	memset(&data, 0, sizeof(data));
	data.pST = pST;
	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &data.pvaStatus)  );

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,
									  pST->prbObjectDataAllocated,
									  _summarize_cb,
									  &data)  );

	*ppvaStatus = data.pvaStatus;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatus);
}
