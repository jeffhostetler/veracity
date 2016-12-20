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
 * @file sg_wc_liveview_dir__can_add_new_entryname.c
 *
 * @details Routine to determine if the given entryname
 * could safely be added to the given directory.
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

struct _can_add__data
{
	SG_wc_port * pPort;

	sg_wc_liveview_item * pLVI_Old;			// we do not own this
	sg_wc_liveview_item * pLVI_New;			// we do not own this

	SG_bool bDisregardUncontrolledItems;
};

/**
 * Iterate over all of the scan-dir-rows and add each
 * entryname *EXCEPT* for the given found rows.
 *
 * That is, we DO NOT add the row's entryname to the
 * collider for the (<old_entryname>, <tneType>) row
 * (because after the rename this item won't have the
 * old name and thus it shouldn't participate in the
 * collision tests).
 * 
 * If there is a (<old_entryname>, <different_tneType>)
 * we do include it (because it probably means we have
 * an after-the-fact rename on the item with the old
 * name and the original type) but the old name is
 * still present in the directory.
 *
 * We also omit the row for the destination because
 * we want to wait until the end and let our caller
 * do that because we want to handle duplicates differently.
 * 
 * If they do something like:
 *     echo hello > abc      # create 2 files.
 *     echo hello > foo      # 
 *     vv add abc            # add both files.
 *     vv add foo            #
 *     vv commit             #
 *     /bin/rm foo           # ***without telling us***, delete the file
 *     /bin/mkdir foo        #       and create a directory in its place.
 * 
 * liveview_dir should have rows:
 *     (foo,file      ==> controlled,   lost)
 * and (foo,directory ==> uncontrolled, found)
 * 
 * Then if they do a:
 *     vv rename abc xyz     #
 * we will see a duplicate key on "foo" that we want
 * to ignore (because as messed up as foo is, foo is
 * not involved right now).
 * But if there was a stray "xyz" already in the
 * directory, we would want to complain.
 *
 */
static SG_rbtree_ui64_foreach_callback _can_add__cb; 

static void _can_add__cb(SG_context * pCtx,
						 SG_uint64 uiAliasGid,
						 void * pVoidAssoc,
						 void * pVoidData)
{
	struct _can_add__data * pData = (struct _can_add__data *)pVoidData;
	sg_wc_liveview_item * pLVI = (sg_wc_liveview_item *)pVoidAssoc;
	SG_bool bIsDuplicate = SG_FALSE;

	SG_UNUSED( uiAliasGid );

	if (pLVI == pData->pLVI_Old)
		return;

	if (pLVI == pData->pLVI_New)
		return;

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_DELETED(pLVI->scan_flags_Live))
	{
		// This item no longer owns its entryname in the directory.
		return;
	}

	if (pData->bDisregardUncontrolledItems
		&& SG_WC_PRESCAN_FLAGS__IS_UNCONTROLLED_UNQUALIFIED(pLVI->scan_flags_Live))
	{
		// If the directory contains "foo" and "FOO" (on a Linux system)
		// and they are both FOUND/IGNORED and the user tries to ADD *one*
		// of them, we shouldn't complain about the other uncontrolled item.
#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "CanAddNewEntryname: Disregarding uncontrolled '%s'.\n",
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif
		return;
	}

	// bIsDuplicate can happen because we are iterating
	// on (<entryname>, <tneType>).  but we can ignore it.
	//
	// We *DO NOT* use the pLVI->tneType because the items
	// that we are seeing in this callback are observed items
	// in the directory and/or under version control.  They
	// do not represent items that the user is trying to ADD.
	//
	// We pass NULL for pszGid because we don't care to get
	// the set of potential collisions in later lookups/callbacks.
	
	SG_ERR_CHECK_RETURN(  SG_wc_port__add_item(pCtx, pData->pPort,
											   NULL,
											   SG_string__sz(pLVI->pStringEntryname),
											   SG_TREENODEENTRY_TYPE__INVALID,
											   &bIsDuplicate)  );
}

//////////////////////////////////////////////////////////////////

/**
 * See if this entryname collides/potentially collides with
 * the DRAWER or CLOSET.
 *
 */
static void _throw_if_reserved(SG_context * pCtx,
							   sg_wc_db * pDb,
							   const char * pszEntryname)
{
	SG_bool bIsReserved = SG_FALSE;

	SG_ERR_CHECK(  sg_wc_db__path__is_reserved_entryname(pCtx, pDb, pszEntryname, &bIsReserved)  );
	if (bIsReserved)
		SG_ERR_THROW2(  SG_ERR_WC_RESERVED_ENTRYNAME, (pCtx, "%s", pszEntryname)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * See if the given NEW entryname is portable
 * given the current contents of the given
 * directory.  While *IGNORING* the existence
 * of the optional rows.
 *
 * That is, can this new entryname be safely
 * added to the directory without issue.
 *
 * This will check for an invalid destination name
 * and for ACTUAL/POTENTIAL ***FINAL*** COLLISIONS
 * with other items.
 *
 */
void sg_wc_liveview_dir__can_add_new_entryname(SG_context * pCtx,
											   sg_wc_db * pDb,
											   sg_wc_liveview_dir * pLVD,
											   sg_wc_liveview_item * pLVI_Old, // optional
											   sg_wc_liveview_item * pLVI_New, // optional
											   const char * pszNewEntryname,
											   SG_treenode_entry_type tneType,
											   SG_bool bDisregardUncontrolledItems)
{
	struct _can_add__data data;
	const SG_string * pStringItemLog = NULL;		// we don't own this
	SG_wc_port_flags portFlags_destination;	
	SG_bool bIsDuplicate;

	memset(&data, 0, sizeof(data));
	data.pLVI_Old = pLVI_Old;
	data.pLVI_New = pLVI_New;
	data.bDisregardUncontrolledItems = bDisregardUncontrolledItems;

	// First check for reserved names.

	SG_ERR_CHECK(  _throw_if_reserved(pCtx, pDb, pszNewEntryname)  );

	// Build portability collider and populate it with
	// the entrynames from all items *EXCEPT* for the
	// optional items named.

	SG_ERR_CHECK(  sg_wc_db__create_port(pCtx, pDb, &data.pPort)  );
	SG_ERR_CHECK(  sg_wc_liveview_dir__foreach(pCtx, pLVD, _can_add__cb, &data)  );

	// Finally, try to add the destination entryname and
	// see if Actually/Potentially collides with anything.
	//
	// We pass NULL for pszGid because we don't care to
	// get the potential collision set in lookups/callbacks.

	SG_ERR_CHECK(  SG_wc_port__add_item(pCtx, data.pPort,
										NULL,
										pszNewEntryname,
										tneType,
										&bIsDuplicate)  );
	if (bIsDuplicate)
	{
		// This should not happen because normally the caller already
		// looked for duplicates, but it can happen during UPDATE
		// because of transient collisions.
		SG_ERR_THROW2(  SG_ERR_WC_PORT_DUPLICATE,
						(pCtx, "Duplicate entryname '%s'.",
						 pszNewEntryname)  );
	}

	// Complain if there are any actual/potential portability
	// problems with the new entryname.
	SG_ERR_CHECK(  SG_wc_port__get_item_result_flags(pCtx, data.pPort, pszNewEntryname,
													 &portFlags_destination, &pStringItemLog)  );
	if (portFlags_destination)
	{
		SG_ERR_THROW2(  SG_ERR_WC_PORT_FLAGS,
						(pCtx,
						 ("The following issues were identified with the new entryname '%s':\n"
						  "%s"),
						 pszNewEntryname,
						 SG_string__sz(pStringItemLog))  );
	}
		
fail:
	SG_WC_PORT_NULLFREE(pCtx, data.pPort);
}
