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
 * @file sg_wc_tx__addremove.c
 *
 * @details Handle details of 'vv addremove'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * With the new extended-prefix repo-paths we can now avoid
 * various path ambiguities during ADDREMOVE.
 *
 * We use the current/live form for FOUND items.
 *
 * We have to do a little gymnastics for LOST items:
 * [] We need the current/live path (where we thought
 *    it was when we lost it) so that we can do the
 *    reverse sort and deal with the deepest items in
 *    the tree first.
 * [] We need the gid so that we can make a gid-domain
 *    repo-path so that we can "name" the item when we
 *    call the regular remove API.
 *
 */
struct _todo_lists
{
	SG_rbtree * prbLostList;	// map[<live-repo-path> ==> gid]
	SG_rbtree * prbFoundList;	// map[<live-repo-path> ==> null]
};

typedef struct _todo_lists todo_lists;

static SG_varray_foreach_callback _build_todo_lists__cb;

/**
 * We get called once for each item that STATUS
 * identified.  This includes LOST and FOUND items
 * as well as anything that has been modified.
 *
 * Insert the LOST or FOUND items into one of the
 * 2 sorted lists.
 *
 */
static void _build_todo_lists__cb(SG_context * pCtx,
								  void * pVoidData,
								  const SG_varray * pva,
								  SG_uint32 ndx,
								  const SG_variant * pv)
{
	todo_lists * pLists = (todo_lists *)pVoidData;
	SG_vhash * pvhItem;
	SG_vhash * pvhItemStatus;
	const char * pszItemPath;
	const char * pszGid;
	SG_bool bIsLost = SG_FALSE;
	SG_bool bIsFound = SG_FALSE;

	SG_UNUSED( pva );
	SG_UNUSED( ndx );

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );

	// We only set the following "is" keys when true, so just checking
	// for the existence of the key is sufficient.

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItemStatus, "isFound", &bIsFound)  );
	if (bIsFound)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszItemPath)  );

#if TRACE_WC_TX_ADDREMOVE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "SG_wc_tx__addremove: identified FOUND %s\n", pszItemPath)  );
#endif
		SG_ERR_CHECK(  SG_rbtree__add(pCtx, pLists->prbFoundList, pszItemPath)  );
		goto done;
	}

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItemStatus, "isLost", &bIsLost)  );
	if (bIsLost)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszItemPath)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );

#if TRACE_WC_TX_ADDREMOVE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "SG_wc_tx__addremove: identified LOST %s\n", pszItemPath)  );
#endif
		SG_ERR_CHECK(  SG_rbtree__add__with_pooled_sz(pCtx, pLists->prbLostList, pszItemPath, pszGid)  );
		goto done;
	}
	
	// ignore this item

done:
	;
fail:
	return;
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _handle_lost_items;

/**
 * We get called once for each LOST item in the list.
 * Call the public REMOVE function for each of them.
 *
 * NOTE: We do NOT require args for --force, --no-backups,
 * nor --keep because none of them apply.
 *
 */
static void _handle_lost_items(SG_context * pCtx,
							   const char * pszLiveRepoPath,
							   void * pVoidAssoc,
							   void * pVoidData)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoidData;
	const char * pszGid = pVoidAssoc;
	SG_string * pStringGidRepoPath = NULL;
	// TODO 2012/08/07 I just added a non-recursive arg to SG_wc_tx__remove().
	// TODO            In theory, we should not need to request a recursive
	// TODO            of this item, since we should have already taken care
	// TODO            of all of the details of stuff within a directory.
	// TODO            But I don't have time to
	// TODO            test this right now.  So I'm setting the flag to match
	// TODO            the behavior of the version of the routine prior to the
	// TODO            new arg.
	SG_bool bNonRecursive = SG_FALSE;
	SG_bool bForce     = SG_FALSE; // A LOST item cannot be dirty, so neither
	SG_bool bNoBackups = SG_FALSE; // --force nor --no-backups matters.
	SG_bool bKeep      = SG_FALSE; // Also, can't keep it since it is lost.

#if TRACE_WC_TX_ADDREMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__addremove: handling LOST %s %s\n",
							   pszLiveRepoPath, pszGid)  );
#else
	SG_UNUSED( pszLiveRepoPath );
#endif

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringGidRepoPath)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringGidRepoPath, "@%s", pszGid)  );

	SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pWcTx,
									SG_string__sz(pStringGidRepoPath),
									bNonRecursive, bForce, bNoBackups, bKeep)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringGidRepoPath);
}

static SG_rbtree_foreach_callback _handle_found_items;

/**
 * We get called once for each FOUND item in the list.
 * Call the public ADD function for each of them.
 *
 * NOTE: We do NOT require args for --depth or --no-ignores
 * arguments because:
 * [] the depth of the dive was decided on the STATUS
 *    call (rather than here while deep down in the tree
 *    looking at an individual directory).  so we can pass
 *    0 for the depth here on the individual ADD.
 * [] we used the optional --no-ignores setting on the
 *    STATUS call so that the pvaStatus already reflects
 *    whether an item is FOUND or IGNORED, so we don't
 *    need to re-check it here.
 *
 */
static void _handle_found_items(SG_context * pCtx,
								const char * pszRepoPath,
								void * pVoidAssoc,
								void * pVoidData)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoidData;
	SG_uint32 depth = 0;
	SG_bool bNoIgnores = SG_TRUE;

	SG_UNUSED( pVoidAssoc );

#if TRACE_WC_TX_ADDREMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__addremove: handling FOUND %s\n", pszRepoPath)  );
#endif

	SG_ERR_CHECK(  SG_wc_tx__add(pCtx, pWcTx, pszRepoPath,
								 depth, bNoIgnores)  );

fail:
	return;
}


//////////////////////////////////////////////////////////////////

/**
 * I'm going to let ADDREMOVE build upon the STATUS, ADD,
 * and REMOVE commands that we already have and not write
 * another custom tree-dive routine for this.  Therefore,
 * the model/template for ADDREMOVE will deviate from the
 * model/template for ADD and REMOVE.
 *
 *
 * I'm going to define 'depth' to be in terms of the top-
 * level path argument -- and not in terms of each individual
 * directory that we find and need to add.
 *
 *
 */
void SG_wc_tx__addremove(SG_context * pCtx,
						 SG_wc_tx * pWcTx,
						 const char * pszInput,
						 SG_uint32 depth,
						 SG_bool bNoIgnores)
{
	SG_varray * pvaStatus = NULL;
	todo_lists todo_lists;
	SG_bool bListUnchanged = SG_FALSE;
	SG_bool bNoTSC = SG_FALSE;
	SG_bool bListSparse = SG_FALSE;
	SG_bool bListReserved = SG_FALSE;

	// if !pszInput, assume full addremove from root.

	memset(&todo_lists, 0, sizeof(todo_lists));

#if TRACE_WC_TX_ADDREMOVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "SG_wc_tx__addremove: considering '%s' [depth %d][bNoIgnores %d]\n",
							   ((pszInput && *pszInput) ? pszInput : "<root>"),
							   depth, bNoIgnores)  );
#endif

	SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pWcTx,
									pszInput,
									depth,
									bListUnchanged,
									bNoIgnores,
									bNoTSC,
									bListSparse,
									bListReserved,
									SG_TRUE,		// don't sort varray, the code below deals with that.
									&pvaStatus,
									NULL)  );

	// build a bottom-up sorted list of the LOST items
	// and a top-down sorted list of the FOUND items.
	//
	// we want the 2 separate lists so that we can:
	// [] process all of the lost items before we process
	//    the found items (in case of name collisions where
	//    they (lose, file, foo) and (found, directory, foo)).
	// [] run thru and do after-the-fact deletes on all of
	//    the lost items -- starting with the leaf files
	//    and working back up the tree towards the root.
	//    (so that REMOVE doesn't have a chance to complain
	//    about removing a non-empty directory.)
	// [] run thru the found items and add them -- starting
	//    near the root and working towards the leaf files.
	//    (so that the ADD code doesn't have to back track
	//    and bubble-up-add the parents.)

	SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS2(pCtx, &todo_lists.prbLostList, 0, NULL,
											 SG_rbtree__compare_function__reverse_strcmp)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &todo_lists.prbFoundList)  );
	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _build_todo_lists__cb, &todo_lists)  );

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, todo_lists.prbLostList,
									  _handle_lost_items, pWcTx)  );
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, todo_lists.prbFoundList,
									  _handle_found_items, pWcTx)  );

fail:
	SG_RBTREE_NULLFREE(pCtx, todo_lists.prbLostList);
	SG_RBTREE_NULLFREE(pCtx, todo_lists.prbFoundList);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__addremove__stringarray(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_stringarray * psaInputs,
									  SG_uint32 depth,
									  SG_bool bNoIgnores)
{
	if (psaInputs)
	{
		SG_uint32 k, count;

		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
		for (k=0; k<count; k++)
		{
			const char * pszInput_k;

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
			SG_ERR_CHECK(  SG_wc_tx__addremove(pCtx, pWcTx, pszInput_k, depth, bNoIgnores)  );
		}
	}
	else
	{
		// if no args, assume "@/" for the ADDREMOVE.
		SG_ERR_CHECK(  SG_wc_tx__addremove(pCtx, pWcTx, NULL, depth, bNoIgnores)  );
	}

fail:
	return;
}
