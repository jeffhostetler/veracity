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
 * @file sg_vv2__status__compute_repo_path.c
 *
 * @details Private routines with VV2-STATUS to compute the
 * historical repo-path on an item.
 *
 * Each item can have 2 UNIQUE repo-paths; one corresponding
 * to the FULL-PATH as it appeared in the ORIGIN CSET and one
 * for the FULL-PATH as it appeared in the DESTINATION CSET.
 * These ARE NOT related.
 *
 * (Contrast these with the moved-from-path of an item which
 * might use the current-entryname and the old-parent's current-path
 * in WC.)
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Compute the DESTINATION repo-path of this item (if not already set).
 *
 * We have a bias towards the DESTINATION and report the
 * status from that point of view.  For example, we say
 * that an item present in the destination that was not
 * present in the original cset has been ADDED.
 *
 * So we try to report the thing using the "CURRENT"
 * parent, path, and name as they appear in the
 * DESTINATION cset (if present).
 *
 * This may all sound pretty obvious, but it gets a little
 * hairy when you combine it with moves/renames and the
 * possibility of parent directories that were also moved/
 * renamed.
 *
 * Since a cset-vs-cset/historical comparison is fixed,
 * we can cache the computed repo-paths in the pOD.
 *
 * We DO NOT silently substitute a source-relative repo-path
 * when the item does not have a destination-relative repo-path.
 * 
 */
void sg_vv2__status__compute_repo_path(SG_context * pCtx,
									   sg_vv2status * pST,
									   sg_vv2status_od * pOD)
{
	const sg_vv2status_odi * p1 = pOD->apInst[1];

	if (pOD->pStringCurRepoPath)
		return;

	if (!p1)
	{
		// The item does not have info for cset-1.
		// It was probably DELETED.  So just return null
		// and our caller will infer that this item doesn't
		// have a current repo-path.
		return;
	}

	if (!*p1->bufParentGid)			// if no parent, we are root
	{
		char buf[4];
		buf[0] = '@';
		buf[1] = pST->chDomain_1;
		buf[2] = '/';
		buf[3] = 0;
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pOD->pStringCurRepoPath, buf)  );
	}
	else
	{
		const char * pszEntryname;
		sg_vv2status_od * pOD_parent;
		SG_treenode_entry_type tneType;
		SG_bool bFound;
		
		SG_ERR_CHECK(  SG_rbtree__find(pCtx,
									   pST->prbObjectDataAllocated,
									   p1->bufParentGid,
									   &bFound,
									   (void **)&pOD_parent)  );
		SG_ASSERT_RELEASE_FAIL2(  (bFound),
								  (pCtx,
								   "Object [GID %s] parent [Gid %s] not in cache.\n",
								   pOD->bufGidObject,
								   p1->bufParentGid)  );

		SG_ERR_CHECK(  sg_vv2__status__compute_repo_path(pCtx, pST, pOD_parent)  );

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, p1->pTNE, &pszEntryname)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, p1->pTNE, &tneType)  );

		SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
												SG_string__sz(pOD_parent->pStringCurRepoPath),
												pszEntryname,
												(tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
												&pOD->pStringCurRepoPath)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Compute the REFERENCE repo-path of this item (if not already set).
 *
 * We have a bias towards the destination and report the
 * status from that point of view.  For example, we say
 * that an item present in the destination that was not
 * present in the original cset has been ADDED.
 *
 * But here we want the repo-path as it existed in the
 * source cset.
 *
 * Since a cset-vs-cset/historical comparison is fixed,
 * we can cache the computed repo-paths in the pOD.
 *
 * We DO NOT silently substitute the destination-relative repo-path
 * when the item does not have a reference/source-relative repo-path.
 * 
 */
void sg_vv2__status__compute_ref_repo_path(SG_context * pCtx,
										   sg_vv2status * pST,
										   sg_vv2status_od * pOD)
{
	const sg_vv2status_odi * p0 = pOD->apInst[0];

	if (pOD->pStringRefRepoPath)
		return;

	if (!p0)
	{
		// The item is not present in cset-0.  So it was
		// probably ADDED and has a value in p1. Just return null
		// and our caller will infer that this item doesn't
		// have a reference repo-path.
		return;
	}

	if (!*p0->bufParentGid)			// if no parent, we are root
	{
		char buf[4];
		buf[0] = '@';
		buf[1] = pST->chDomain_0;
		buf[2] = '/';
		buf[3] = 0;
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pOD->pStringRefRepoPath, buf)  );
	}
	else
	{
		const char * pszEntryname;
		sg_vv2status_od * pOD_parent;
		SG_treenode_entry_type tneType;
		SG_bool bFound;
		
		SG_ERR_CHECK(  SG_rbtree__find(pCtx,
									   pST->prbObjectDataAllocated,
									   p0->bufParentGid,
									   &bFound,
									   (void **)&pOD_parent)  );
		SG_ASSERT_RELEASE_FAIL2(  (bFound),
								  (pCtx,
								   "Object [GID %s] parent [Gid %s] not in cache.\n",
								   pOD->bufGidObject,
								   p0->bufParentGid)  );

		SG_ERR_CHECK(  sg_vv2__status__compute_ref_repo_path(pCtx, pST, pOD_parent)  );

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, p0->pTNE, &pszEntryname)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, p0->pTNE, &tneType)  );

		SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx,
												SG_string__sz(pOD_parent->pStringRefRepoPath),
												pszEntryname,
												(tneType == SG_TREENODEENTRY_TYPE_DIRECTORY),
												&pOD->pStringRefRepoPath)  );
	}

fail:
	return;
}



