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
 * @file sg_vv2__filtered_status__main.c
 *
 * @details Compute a FILTERED STATUS between 2 changesets
 * when given 1 or more ARGV or non-recursive options.
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

struct _filter_data
{
	SG_repo			* pRepo;                  	// we do not own this
	SG_rbtree		* prbTreenodeCache;			// we do not own this
	const char		* pszHidCSet_0;
	const char		* pszHidCSet_1;
	const char		* pszLabel_0;
	const char		* pszLabel_1;
	const char		* pszWasLabel_0;
	const char		* pszWasLabel_1;

	SG_uint32		  depth;
	char			  chDomain_0;
	char			  chDomain_1;
	SG_bool			  b_allow_wd_inputs;

	char			* pszHidForRoot_0;
	char			* pszHidForRoot_1;
	SG_treenode		* ptnRoot_0;
	SG_treenode		* ptnRoot_1;

	SG_rbtree		* prbFiltered;
	SG_varray		* pvaStatusFull;
	SG_varray		* pvaStatusFiltered;
	
};

//////////////////////////////////////////////////////////////////

/**
 * Find all of the items in the full-set that have a
 * cset[<label>] repo-path that is contained within the
 * given repo-path.  (Items that are files/folders within
 * the given directory in cset[<label>].)
 *
 */
static void _match_by_repopath_prefix(SG_context * pCtx,
									  struct _filter_data * pData,
									  const SG_string * pStringPrefix_0,
									  const SG_string * pStringPrefix_1)
{
	SG_uint32 j, nrItems;
	const char * pszPrefix_0 = NULL;
	const char * pszPrefix_1 = NULL;
	SG_uint32 lenPrefix_0 = 0;
	SG_uint32 lenPrefix_1 = 0;
	SG_vhash * pvhNewItem = NULL;

	if (pStringPrefix_0)
	{
		pszPrefix_0 = SG_string__sz(pStringPrefix_0);
		SG_ASSERT( ((pszPrefix_0[0]=='@') && (pszPrefix_0[1]=='/')) );
		lenPrefix_0 = SG_string__length_in_bytes(pStringPrefix_0);
		pszPrefix_0 += 2;
		lenPrefix_0 -= 2;
	}
	if (pStringPrefix_1)
	{
		pszPrefix_1 = SG_string__sz(pStringPrefix_1);
		SG_ASSERT( ((pszPrefix_1[0]=='@') && (pszPrefix_1[1]=='/')) );
		lenPrefix_1 = SG_string__length_in_bytes(pStringPrefix_1);
		pszPrefix_1 += 2;
		lenPrefix_1 -= 2;
	}
	
	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaStatusFull, &nrItems)  );
	for (j=0; j<nrItems; j++)
	{
		/*const*/ SG_vhash * pvhItem;
		const char * pszGid;
		SG_bool bFound = SG_FALSE;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaStatusFull, j, &pvhItem)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prbFiltered, pszGid, &bFound, NULL)  );
		if (!bFound)
		{
			SG_bool bAddIt = SG_FALSE;
			const char * pszRepoPath_0 = NULL;
			const char * pszRepoPath_1 = NULL;

			if (pStringPrefix_0)				// The given directory was present in cset[0].
			{
				/*const*/ SG_vhash * pvhItemSub_0;
				SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem, pData->pszLabel_0, &pvhItemSub_0)  );
				if (pvhItemSub_0)				// Item[j] was present in cset[0].
				{
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItemSub_0, "path", &pszRepoPath_0)  );
					SG_ASSERT( ((pszRepoPath_0[0]=='@') && (pszRepoPath_0[1]==pData->chDomain_0) && (pszRepoPath_0[2]=='/')) );
					pszRepoPath_0 += 3;
					if (strncmp(pszRepoPath_0, pszPrefix_0, lenPrefix_0) == 0)
					{
						// Item[j] in cset[0] is inside the directory sub-tree rooted at
						// the given directory (using the repo-path that that directory
						// had in cset[0]).
						//
						// Make sure it isn't too deep.  Count the slashes in the remaining
						// portion of item[j]'s repo-path in cset[0].
						//
						// (But don't get fooled by a trailing slash.)
						//
						// We ASSUME that the given prefix has a trailing slash, so the suffix
						// we compute here is the remaining portion -- and does not include
						// a leading slash.

						const char * pszSuffix = &pszRepoPath_0[lenPrefix_0];
						SG_uint32 nrSlashes = 0;
						while (*pszSuffix)
						{
							if ((pszSuffix[0]=='/') && (pszSuffix[1]))
								nrSlashes++;
							pszSuffix++;
						}
						if (nrSlashes < pData->depth)
							bAddIt = SG_TRUE;
					}
				}
			}

			if (pStringPrefix_1)
			{
				/*const*/ SG_vhash * pvhItemSub_1;
				SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem, pData->pszLabel_1, &pvhItemSub_1)  );
				if (pvhItemSub_1)				// Item[j] was present in cset[1].
				{
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItemSub_1, "path", &pszRepoPath_1)  );
					SG_ASSERT( ((pszRepoPath_1[0]=='@') && (pszRepoPath_1[1]==pData->chDomain_1) && (pszRepoPath_1[2]=='/')) );
					pszRepoPath_1 += 3;
					if (strncmp(pszRepoPath_1, pszPrefix_1, lenPrefix_1) == 0)
					{
						// Item[j] in cset[1] is inside the directory sub-tree rooted at
						// the given directory (using the repo-path that that directory
						// had in cset[1]).
						//
						// Make sure it isn't too deep.  Count the slashes in the remaining
						// portion of item[j]'s repo-path in cset[1].
						//
						// (But don't get fooled by a trailing slash.)
						//
						// We ASSUME that the given prefix has a trailing slash, so the suffix
						// we compute here is the remaining portion -- and does not include
						// a leading slash.

						const char * pszSuffix = &pszRepoPath_1[lenPrefix_1];
						SG_uint32 nrSlashes = 0;
						while (*pszSuffix)
						{
							if ((pszSuffix[0]=='/') && (pszSuffix[1]))
								nrSlashes++;
							pszSuffix++;
						}
						if (nrSlashes < pData->depth)
							bAddIt = SG_TRUE;
					}
				}
			}

			if (bAddIt)
			{
				SG_ERR_CHECK(  SG_VHASH__ALLOC__COPY(pCtx, &pvhNewItem, pvhItem)  );
				SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pData->prbFiltered, pszGid, pvhNewItem, NULL)  );
				pvhNewItem = NULL;		// rbtree owns it now.
			}

#if 0 && defined(DEBUG)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   ("FilterStatus: [depth %d] %s\n"
										"              gid: %s\n"
										"        Prefix[0]: %s\n"
										"         Found[0]: %s\n"
										"        Prefix[1]: %s\n"
										"         Found[1]: %s\n"),
									   pData->depth,
									   ((bAddIt)        ? "Matched"     : "NotMatched"),
									   pszGid,
									   ((pszPrefix_0)   ? pszPrefix_0   : "(null)"),
									   ((pszRepoPath_0) ? pszRepoPath_0 : "(null)"),
									   ((pszPrefix_1)   ? pszPrefix_1   : "(null)"),
									   ((pszRepoPath_1) ? pszRepoPath_1 : "(null)"))  );
#endif
		}
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhNewItem);
}

//////////////////////////////////////////////////////////////////

/**
 * Process one filename argument.
 *
 * Use the given pszInput to find the GID and TNEs in both csets
 * for the file.  We allow for the case where the item is only
 * present in one side (adds/deletes).
 *
 *
 * Call the csets '0' and '1' for discussion purposes.
 *
 * If b_allow_wd_inputs, we allow pszInput to contain:
 * [] gid-repo-path             "@g1234..."  -- use it
 * [] extended-prefix repo-path "@0/foo/bar" -- use cset '0' to get GID
 * [] extended-prefix repo-path "@1/foo/bar" -- use cset '1' to get GID
 * [] regular repo-path         "@/foo/bar"  -- use WD to get GID
 * [] extended-prefix repo-path "@b/...", "@a/...", -- normal WD lookup (post-merge perhaps)
 * [] relative/absolute path                 -- use WD to get GID
 *
 * Otherwise, we allow pszInput to contain:
 * [] gid-repo-path             "@g1234..."  -- use it
 * [] extended-prefix repo-path "@0/foo/bar" -- use cset '0' to get GID
 * [] extended-prefix repo-path "@1/foo/bar" -- use cset '1' to get GID
 * [] generic repo-path         "@/foo/bar"  -- lookup in both '0' and '1' and require GID match
 *
 */
static void _my__parse_one_arg(SG_context * pCtx,
							   struct _filter_data * pData,
							   const char * pszInput)
{
	SG_string * pStr_0 = NULL;
	SG_string * pStr_1 = NULL;
	SG_string * pStr_at0 = NULL;
	SG_string * pStr_at1 = NULL;
	SG_string * pStrParent_0 = NULL;
	SG_string * pStrParent_1 = NULL;
	SG_treenode_entry * ptne_0 = NULL;
	SG_treenode_entry * ptne_1 = NULL;
	SG_treenode_entry * ptneParent_0 = NULL;
	SG_treenode_entry * ptneParent_1 = NULL;
	char * pszGid = NULL;
	char * pszGid_0 = NULL;
	char * pszGid_1 = NULL;
	char * pszGidParent_0 = NULL;
	char * pszGidParent_1 = NULL;
	SG_vhash * pvhItem = NULL;
	SG_wc_status_flags sf;

	if ((pszInput[0]=='@') && (pszInput[1]=='g'))
	{
		SG_bool bValid;
		SG_ERR_CHECK(  SG_gid__verify_format(pCtx, &pszInput[1], &bValid)  );
		if (!bValid)
			SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
							(pCtx, "The input '%s' is not a valid gid-domain input.", pszInput)  );

		SG_ERR_CHECK(  SG_STRDUP(pCtx, &pszInput[1], &pszGid)  );
		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pData->pszHidCSet_0,
															 &pStr_0, &ptne_0)  );
		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pData->pszHidCSet_1,
															 &pStr_1, &ptne_1)  );

#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "VV2_FileDiff: '%s' --> '%s' --> { '%s', '%s' }\n",
								   pszInput, pszGid,
								   ((pStr_0) ? SG_string__sz(pStr_0) : "(null)"),
								   ((pStr_1) ? SG_string__sz(pStr_1) : "(null)"))  );
#endif

		if (!ptne_0 && !ptne_1)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "'%s' not found in either changeset.", pszInput)  );

	}
	else if ((pszInput[0]=='@') && (pszInput[1]==pData->chDomain_0) && (pszInput[2]=='/'))
	{
		// convert "@1/foo/bar" to "@/foo/bar".
		SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStr_0, "@%s", &pszInput[2])  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pData->pRepo, pData->ptnRoot_0,
															   SG_string__sz(pStr_0),
															   &pszGid, &ptne_0)  );
		if (!ptne_0)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "'%s' not found in changeset[0] '%s'.", pszInput, pData->pszHidCSet_0)  );

		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pData->pszHidCSet_1,
															 &pStr_1, &ptne_1)  );

#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "VV2_FileDiff: '%s' --> '%s' --> { '%s', '%s' }\n",
								   pszInput, pszGid,
								   ((pStr_0) ? SG_string__sz(pStr_0) : "(null)"),
								   ((pStr_1) ? SG_string__sz(pStr_1) : "(null)"))  );
#endif

	}
	else if ((pszInput[0]=='@') && (pszInput[1]==pData->chDomain_1) && (pszInput[2]=='/'))
	{
		// convert "@2/foo/bar" to "@/foo/bar".
		SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStr_1, "@%s", &pszInput[2])  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pData->pRepo, pData->ptnRoot_1,
															   SG_string__sz(pStr_1),
															   &pszGid, &ptne_1)  );
		if (!ptne_1)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "'%s' not found in changeset[1] '%s'.", pszInput, pData->pszHidCSet_1)  );

		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pData->pszHidCSet_0,
															 &pStr_0, &ptne_0)  );

#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "VV2_FileDiff: '%s' --> '%s' --> { '%s', '%s' }\n",
								   pszInput, pszGid,
								   ((pStr_0) ? SG_string__sz(pStr_0) : "(null)"),
								   ((pStr_1) ? SG_string__sz(pStr_1) : "(null)"))  );
#endif

	}
	else if (pData->b_allow_wd_inputs)
	{
		// Try to use WD to parse relative/absolute path,
		// plain repo-path, or normal extended-prefix repo-path.

		SG_ERR_CHECK(  SG_wc__get_item_gid(pCtx, NULL, pszInput, &pszGid)  );

		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pData->pszHidCSet_0,
															 &pStr_0, &ptne_0)  );
		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pData->pszHidCSet_1,
															 &pStr_1, &ptne_1)  );

		if (!ptne_0 && !ptne_1)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "'%s' not found in either changeset.", pszInput)  );

#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "VV2_FileDiff: '%s' --> '%s' --> { '%s', '%s' }\n",
								   pszInput, pszGid,
								   ((pStr_0) ? SG_string__sz(pStr_0) : "(null)"),
								   ((pStr_1) ? SG_string__sz(pStr_1) : "(null)"))  );
#endif
	}
	else if ((pszInput[0]=='@') && (pszInput[1]=='/'))
	{
		// They gave a regular repo-path without specifying which cset it refers to.
		// This is ambiguous (because the file (or a parent directory) could have been
		// moved/renamed between the 2 csets).
		//
		// Lookup the repo-path in both csets and complain if they don't refer to the same file.

		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pData->pRepo, pData->ptnRoot_0,
															   pszInput,
															   &pszGid_0, &ptne_0)  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pData->pRepo, pData->ptnRoot_1,
															   pszInput,
															   &pszGid_1, &ptne_1)  );

#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "VV2_FileDiff: '%s' --> { '%s', '%s' }\n",
								   pszInput,
								   ((pszGid_0) ? pszGid_0 : "(null)"),
								   ((pszGid_1) ? pszGid_1 : "(null)"))  );
#endif

		if (pszGid_0 && pszGid_1)
		{
			if (strcmp(pszGid_0, pszGid_1) != 0)
				SG_ERR_THROW2(  SG_ERR_WC_PATH_NOT_UNIQUE,
								(pCtx, "'%s' refers to 2 different items in changesets '%s' and '%s'.  Consider using '@0/' or '@1/' prefix.",
								 pszInput, pData->pszHidCSet_0, pData->pszHidCSet_1)  );

			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGid_0, &pszGid)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStr_0, pszInput)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStr_1, pszInput)  );
		}
		else if (pszGid_0)
		{
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGid_0, &pszGid)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStr_0, pszInput)  );
		}
		else if (pszGid_1)
		{
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGid_1, &pszGid)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStr_1, pszInput)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "'%s' not found in either changeset.", pszInput)  );
		}
	}
	else
	{
		// We disallow non-repo-paths when we reference a repo without assuming a WD.
		SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
						(pCtx, "'%s' must be a full repo-path starting with '@/'.", pszInput)  );
	}

	// TODO 2012/11/06 check on best way to handle root "@/".
	// TODO            we don't need to lookup parents, for example.

	// This is a little expensive (since we may have already walked
	// the treenodes/treendx) but find the GIDs of the parent directories
	// so that we can report on MOVES.

	if (pStr_0)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pStrParent_0, pStr_0)  );
		SG_ERR_CHECK(  SG_repopath__remove_last(pCtx, pStrParent_0)  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pData->pRepo, pData->ptnRoot_0,
															   SG_string__sz(pStrParent_0),
															   &pszGidParent_0, &ptneParent_0)  );
	}
	if (pStr_1)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pStrParent_1, pStr_1)  );
		SG_ERR_CHECK(  SG_repopath__remove_last(pCtx, pStrParent_1)  );
		SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pData->pRepo, pData->ptnRoot_1,
															   SG_string__sz(pStrParent_1),
															   &pszGidParent_1, &ptneParent_1)  );
	}

	//////////////////////////////////////////////////////////////////
	// The main vv2/vv2status code has the remnants of the TreeDiff and
	// PendingTree code (all the OD stuff which does a full treewalk).
	// We don't need to hook into of that because we already have the
	// answers for this item.  So (assuming we don't need to deal with
	// searching the contents of a directory), we cut some corners here
	// and just directly create a status vhash for this item.  We add
	// it to the rbtree so for de-dup purposes.

	SG_ERR_CHECK(  sg_vv2__status__compute_flags2(pCtx, ptne_0, pszGidParent_0, ptne_1, pszGidParent_1, &sf)  );
	if (sf & (SG_WC_STATUS_FLAGS__S__MASK | SG_WC_STATUS_FLAGS__C__MASK))
	{
		SG_vhash * pvhOldItem = NULL;

		if (pStr_0)
		{
			const char * p = SG_string__sz(pStr_0);
			SG_ASSERT( ((p[0]=='@') && (p[1]=='/')) );
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStr_at0, "@%c%s", pData->chDomain_0, &p[1])  );
			if (sf & SG_WC_STATUS_FLAGS__T__DIRECTORY)
				SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pStr_at0)  );
			p = SG_string__sz(pStr_at0);
			SG_ASSERT( ((p[0]=='@') && (p[1]==pData->chDomain_0) && (p[2]=='/')) );
		}
		if (pStr_1)
		{
			const char * p = SG_string__sz(pStr_1);
			SG_ASSERT( ((p[0]=='@') && (p[1]=='/')) );
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pStr_at1, "@%c%s", pData->chDomain_1, &p[1])  );
			if (sf & SG_WC_STATUS_FLAGS__T__DIRECTORY)
				SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pStr_at1)  );
			p = SG_string__sz(pStr_at1);
			SG_ASSERT( ((p[0]=='@') && (p[1]==pData->chDomain_1) && (p[2]=='/')) );
		}

#if 0 && defined(DEBUG)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("FilterStatus: ExactMatch\n"
									"         gid: %s\n"
									"     path[0]: %s\n"
									"     path[1]: %s\n"),
								   pszGid,
								   ((pStr_at0) ? SG_string__sz(pStr_at0) : "(null)"),
								   ((pStr_at1) ? SG_string__sz(pStr_at1) : "(null)"))  );
#endif

		SG_ERR_CHECK(  sg_vv2__status__alloc_item(pCtx, pData->pRepo,
												  pData->pszLabel_0, pData->pszLabel_1,
												  pData->pszWasLabel_0, pData->pszWasLabel_1,
												  sf, pszGid,
												  pStr_at0, ptne_0, pszGidParent_0,
												  pStr_at1, ptne_1, pszGidParent_1,
												  &pvhItem)  );
		SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pData->prbFiltered, pszGid, pvhItem, (void **)&pvhOldItem)  );
		pvhItem = NULL;		// rbtree owns it now
		SG_VHASH_NULLFREE(pCtx, pvhOldItem);
	}

	if ((sf & SG_WC_STATUS_FLAGS__T__DIRECTORY) && (pData->depth > 0))
	{
		// rats! they named a directory on the command line and they
		// want a recursive status.  let the full/complete status code
		// run.  this will take care of all the treewalk stuff.  then
		// we search the results for changed items that are/were
		// contained within this directory and add them to the filtered
		// result set.

		if (!pData->pvaStatusFull)
			SG_ERR_CHECK(  sg_vv2__status__main(pCtx, pData->pRepo, pData->prbTreenodeCache,
												pData->pszHidCSet_0, pData->pszHidCSet_1,
												pData->chDomain_0, pData->chDomain_1,
												pData->pszLabel_0, pData->pszLabel_1,
												pData->pszWasLabel_0, pData->pszWasLabel_1,
												SG_TRUE,	// no sort -- we'll sort the filtered result instead
												&pData->pvaStatusFull, NULL)  );

		if (pStr_0)
			SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pStr_0)  );
		if (pStr_1)
			SG_ERR_CHECK(  SG_repopath__ensure_final_slash(pCtx, pStr_1)  );

		SG_ERR_CHECK(  _match_by_repopath_prefix(pCtx, pData, pStr_0, pStr_1)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStr_0);
	SG_STRING_NULLFREE(pCtx, pStr_1);
	SG_STRING_NULLFREE(pCtx, pStr_at0);
	SG_STRING_NULLFREE(pCtx, pStr_at1);
	SG_STRING_NULLFREE(pCtx, pStrParent_0);
	SG_STRING_NULLFREE(pCtx, pStrParent_1);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_0);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_1);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptneParent_0);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptneParent_1);
	SG_NULLFREE(pCtx, pszGid);
	SG_NULLFREE(pCtx, pszGid_0);
	SG_NULLFREE(pCtx, pszGid_1);
	SG_NULLFREE(pCtx, pszGidParent_0);
	SG_NULLFREE(pCtx, pszGidParent_1);
	SG_VHASH_NULLFREE(pCtx, pvhItem);
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _my__collect_result_vhashes;

static void _my__collect_result_vhashes(SG_context* pCtx, const char* pszKey, void* assocData, void * pVoidData)
{
	struct _filter_data * pData = (struct _filter_data *)pVoidData;
	SG_vhash * pvhItem = NULL;

	// Steal the assoc data from the rbtree and append to the result-set.
	// Technically we are modifying the rbtree during an iteration, but
	// we are not adding/removing items from the rbtree -- only stealing
	// the assoc data.

	SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pData->prbFiltered, pszKey, NULL, (void **)&pvhItem)  );
	SG_ASSERT( (assocData == pvhItem) );
	SG_UNUSED( assocData );

	SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pData->pvaStatusFiltered, &pvhItem)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Compute filtered status.
 *
 */
void sg_vv2__filtered_status(SG_context * pCtx,
							 SG_repo * pRepo,
							 SG_rbtree * prbTreenodeCache_Shared,	// optional
							 const char * pszHidCSet_0,
							 const char * pszHidCSet_1,
							 SG_bool b_allow_wd_inputs,
							 const SG_stringarray * psaInputs,
							 SG_uint32 depth,
							 const char chDomain_0,
							 const char chDomain_1,
							 const char * pszLabel_0,
							 const char * pszLabel_1,
							 const char * pszWasLabel_0,
							 const char * pszWasLabel_1,
							 SG_bool bNoSort,
							 SG_varray ** ppvaStatusFiltered,
							 SG_vhash ** ppvhLegend)
{
	struct _filter_data data;
	SG_vhash * pvhLegend = NULL;
	SG_uint32 k, count;

	// ppvhLegend is optional

	memset(&data, 0, sizeof(data));

	SG_NULLARGCHECK_RETURN( psaInputs );

	data.pRepo = pRepo;
	data.prbTreenodeCache = prbTreenodeCache_Shared;
	data.pszHidCSet_0 = pszHidCSet_0;
	data.pszHidCSet_1 = pszHidCSet_1;
	data.chDomain_0 = chDomain_0;
	data.chDomain_1 = chDomain_1;
	data.pszLabel_0 = pszLabel_0;
	data.pszLabel_1 = pszLabel_1;
	data.pszWasLabel_0 = pszWasLabel_0;
	data.pszWasLabel_1 = pszWasLabel_1;
	data.depth = depth;
	data.b_allow_wd_inputs = b_allow_wd_inputs;
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, data.pRepo, data.pszHidCSet_0, &data.pszHidForRoot_0)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, data.pRepo, data.pszHidCSet_1, &data.pszHidForRoot_1)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, data.pRepo, data.pszHidForRoot_0, &data.ptnRoot_0)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, data.pRepo, data.pszHidForRoot_1, &data.ptnRoot_1)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &data.prbFiltered)  );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszInput_k;
		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInputs, k, &pszInput_k)  );
		SG_ERR_CHECK(  _my__parse_one_arg(pCtx, &data, pszInput_k)  );
	}
	
	// We accumulated in data.prbFiltered a set of VHASHES that should be
	// reported in the filtered-result-set.  The rbtree owns the vhashes.
	// This was done so for de-dup purposes.  Now moved the vhashes into
	// the result varray.

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &data.pvaStatusFiltered)  );	
	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, data.prbFiltered, _my__collect_result_vhashes, &data)  );

	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, data.pvaStatusFiltered)  );

	if (ppvhLegend)
		SG_ERR_CHECK(  sg_vv2__status__make_legend(pCtx, &pvhLegend,
												   pszHidCSet_0, pszHidCSet_1,
												   pszLabel_0, pszLabel_1)  );

	SG_RETURN_AND_NULL( data.pvaStatusFiltered, ppvaStatusFiltered );
	SG_RETURN_AND_NULL( pvhLegend, ppvhLegend );

fail:
	SG_NULLFREE(pCtx, data.pszHidForRoot_0);
	SG_NULLFREE(pCtx, data.pszHidForRoot_1);
	SG_TREENODE_NULLFREE(pCtx, data.ptnRoot_0);
	SG_TREENODE_NULLFREE(pCtx, data.ptnRoot_1);
	SG_RBTREE_NULLFREE(pCtx, data.prbFiltered);
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatusFull);
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatusFiltered);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
}
