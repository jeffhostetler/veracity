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

void sg_vv2__history__repo2(
	SG_context * pCtx,
	SG_repo * pRepo, 
	const SG_stringarray * psaArgs,  // if present these must be full repo-paths
	const SG_rev_spec* pRevSpec,
	const SG_rev_spec* pRevSpec_single_revisions,
	const char* pszUser,
	const char* pszStamp,
	SG_uint32 nResultLimit,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bListAll,
	SG_bool bReassembleDag,
	SG_bool* pbHasResult,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken)
{
	SG_stringarray * pStringArrayChangesets_starting = NULL;
	SG_stringarray * pStringArrayChangesetsMissing = NULL;
	SG_stringarray * pStringArrayChangesets_single_revisions = NULL;
	SG_bool bRecommendDagWalk = SG_FALSE;
	SG_bool bLeaves = SG_FALSE;
	const char * pszCurrentDagnodeID = NULL;
	SG_stringarray * pStringArrayGIDs = NULL;
	SG_uint32 i = 0, nChangesetCount = 0;
	SG_uint32 count_args = 0;

	//Determine the starting changeset IDs.  strBranch and bLeaves control this.
	//We do this step here, so that repo paths can be looked up before we call into history__core.
	SG_ERR_CHECK( sg_vv2__history__get_starting_changesets(pCtx, pRepo, pRevSpec,
														   &pStringArrayChangesets_starting,
														   &pStringArrayChangesetsMissing,
														   &bRecommendDagWalk,
														   &bLeaves) );
	if (pStringArrayChangesetsMissing)
	{
		// See K2177, K1322, W0836, W8132.  We requested specific starting
		// points and ran into some csets that were referenced (by --tag
		// or --branch) that are not present in the local repo.  Try to
		// silently ignore them.
		SG_uint32 nrFound = 0;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayChangesets_starting, &nrFound)  );
		if (nrFound > 0)
		{
			// Yes there were missing csets, but we still found some
			// of the referenced ones.  Just ignore the missing ones.
			// This should behave just like we had the older tag/branch
			// dag prior to the push -r on the vc dag.
		}
		else
		{
			const char * psz_0;
			// TODO 2012/10/19 Do we want a different message if the number of missing is > 1 ?
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArrayChangesetsMissing, 0, &psz_0)  );
			SG_ERR_THROW2(  SG_ERR_CHANGESET_BLOB_NOT_FOUND,
							(pCtx, "%s", psz_0)  );
		}
	}

	if (bListAll)
	{
		// See W8493.  If they gave us a --list-all along with a --rev or --tag, they
		// want to force us to show the full history rather than just the info for the
		// named cset.
		bRecommendDagWalk = SG_TRUE;
	}

	if (pRevSpec_single_revisions)
	{
		// We DO NOT pass a psaMissingHids here because we want
		// it to throw if the user names a missing cset.
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo__dedup(pCtx, pRepo, pRevSpec_single_revisions, SG_TRUE,
														 &pStringArrayChangesets_single_revisions, NULL)  );
	}

	// TODO 2012/07/02 Is the following loop really what we want?
	// TODO            As written, it will look at each changeset
	// TODO            in the list and lookup each item's GID with
	// TODO            it.  If cset[k] does not have *ALL* of the
	// TODO            items, we discard the array of GIDs already
	// TODO            discovered and then try cset[k+1].
	// TODO
	// TODO            This seems wasteful and likely to fail in
	// TODO            the presence of lots of adds and deletes.
	// TODO
	// TODO            Seems like it would be better to start the
	// TODO            result list outside of the loop and remove
	// TODO            items from the search list as we find them
	// TODO            as we iterate over the csets.  This would
	// TODO            let us stop as soon as we have them all and
	// TODO            not require us to repeat the expensive mapping
	// TODO            of repo-path to gid.
	// TODO
	// TODO            Then again, I think the caller has limited us
	// TODO            to only having *1* item in the set of files/folders,
	// TODO            so this might not actually matter.

	if (psaArgs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaArgs, &count_args)  );
	if (count_args > 0)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayChangesets_starting, &nChangesetCount)  );
		//Look up the GIDs for all of the arguments.
		//Try every changeset, until we get one that has the GID in question.
		for (i = 0; i < nChangesetCount; i++)
		{
			SG_ERR_CHECK( SG_stringarray__get_nth(pCtx, pStringArrayChangesets_starting, i, &pszCurrentDagnodeID) );

			//This might be used if you have --leaves, or if there are multiple parents
			//since they specified a changeset, we need to use the full repo path @/blah/blah to look up the objects
			sg_vv2__history__lookup_gids_by_repopaths(pCtx, pRepo, pszCurrentDagnodeID, psaArgs,
													  &pStringArrayGIDs);
			if (SG_CONTEXT__HAS_ERR(pCtx))
			{
				if (i == (nChangesetCount - 1) || ! SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND) )
				{
					SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
					SG_ERR_RETHROW;
				}
				else
				{
					SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
					SG_context__err_reset(pCtx);
				}
			}
			else
				break;
		}
	}

	//Call history core with the GIDs
	SG_ERR_CHECK( SG_history__run(pCtx, pRepo, pStringArrayGIDs,
					pStringArrayChangesets_starting, pStringArrayChangesets_single_revisions,
					pszUser, pszStamp, nResultLimit, bLeaves, bHideObjectMerges,
					nFromDate, nToDate, bRecommendDagWalk, bReassembleDag, pbHasResult, ppResult, ppHistoryToken) );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesets_starting);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesetsMissing);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesets_single_revisions);
}

//////////////////////////////////////////////////////////////////

void sg_vv2__history__repo(
	SG_context * pCtx,
	const char * pszRepoDescriptorName, //The local descriptor name.
	const SG_stringarray * psaArgs,     // If present, these must be full repo-paths.
	const SG_rev_spec* pRevSpec,
	const SG_rev_spec* pRevSpec_single_revisions,
	const char* pszUser,
	const char* pszStamp,
	SG_uint32 nResultLimit,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bListAll,
	SG_bool bReassembleDag,
	SG_bool* pbHasResult, 
	SG_vhash** ppvhBranchPile,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken)
{
	SG_repo * pRepo = NULL;
    
	SG_ERR_CHECK( SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoDescriptorName, &pRepo) );
    SG_ERR_CHECK( sg_vv2__history__repo2(pCtx, pRepo, psaArgs,
										 pRevSpec, pRevSpec_single_revisions,
										 pszUser, pszStamp,
										 nResultLimit,
										 bHideObjectMerges,
										 nFromDate, nToDate,
										 bListAll,
										 bReassembleDag,
										 pbHasResult, ppResult, ppHistoryToken));

	/* This is kind of a hack. History callers often need branch data to format ouput.
	 * But we open the repo down here. I didn't want to open/close it again. So we do this. */
	if (ppvhBranchPile)
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, ppvhBranchPile)  );
	
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

///////////////////////////////////////////////////////////////////

void sg_vv2__history__working_folder(
	SG_context * pCtx,
	const SG_stringarray * psaInputs,
	const SG_rev_spec* pRevSpec,
	const SG_rev_spec* pRevSpec_single_revisions,
	const char* pszUser,
	const char* pszStamp,
	SG_bool bDetectCurrentBranch,
	SG_uint32 nResultLimit,
	SG_bool bHideObjectMerges,
	SG_int64 nFromDate,
	SG_int64 nToDate,
	SG_bool bListAll,
	SG_bool* pbHasResult, 
	SG_vhash** ppvhBranchPile,
	SG_history_result ** ppResult,
	SG_history_token ** ppHistoryToken)
{
	SG_repo * pRepo = NULL;
	SG_stringarray * pStringArrayGIDs = NULL;
	SG_stringarray * pStringArrayChangesets = NULL;
	SG_stringarray * pStringArrayChangesetsMissing = NULL;
	SG_stringarray * pStringArrayChangesets_single_revisions = NULL;
	SG_bool bRecommendDagWalk = SG_FALSE;
	SG_bool bLeaves = SG_FALSE;
	const char * pszBranchName = NULL;	// we do not own this
	SG_vhash* pvhBranchPile = NULL;
	SG_varray* pvaParents = NULL;	// we do not own this
	SG_bool bMyBranchWalkRecommendation = SG_FALSE;
	
	SG_rev_spec* pRevSpec_Allocated = NULL;
	SG_wc_tx * pWcTx = NULL;
	SG_vhash * pvhInfo = NULL;
	SG_uint32 count_args = 0;
	SG_uint32 countRevsSpecified = 0;

	if (psaInputs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInputs, &count_args)  );

	// Use the WD to try to get the initial info.
	// I'm going to deviate from the model and use
	// a read-only TX here so that I can get a bunch
	// of fields that we need later.
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );

	if (count_args > 0)
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid__stringarray(pCtx, pWcTx, psaInputs, &pStringArrayGIDs)  );

	SG_ERR_CHECK(  SG_wc_tx__get_wc_info(pCtx, pWcTx, &pvhInfo)  );
	SG_ERR_CHECK(  SG_wc_tx__get_repo_and_wd_top(pCtx, pWcTx, &pRepo, NULL)  );

	/* If no revisions were specified, and the caller wants us to use the current branch,
	 * create a revision spec with the current branch. */

	if (pRevSpec)
	{
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC__COPY(pCtx, pRevSpec, &pRevSpec_Allocated)  );
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec_Allocated, &countRevsSpecified)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_Allocated)  );
	}

	if (pRevSpec_single_revisions != NULL)
	{
		SG_uint32 countRevsSpecified_singles = 0;
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec_single_revisions, &countRevsSpecified_singles)  );
		countRevsSpecified += countRevsSpecified_singles;
	}

	if (bDetectCurrentBranch && countRevsSpecified == 0) 
	{
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhInfo, "branch", &pszBranchName)  );
		if (pszBranchName)
		{
			/* The working folder is attached to a branch. Does it exist? */
			SG_bool bHasBranches = SG_FALSE;
			SG_bool bBranchExists = SG_FALSE;

			SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhBranchPile)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhBranchPile, "branches", &bHasBranches)  );
			if (bHasBranches)
			{
				SG_vhash* pvhRefBranches;
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhBranchPile, "branches", &pvhRefBranches)  );
				SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranches, pszBranchName, &bBranchExists)  );
			}
				
			if (bBranchExists)
			{
				SG_uint32 numParents, i;
				const char* pszRefParent;

				/* If that branch exists, just add to our rev spec. */
				SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec_Allocated, pszBranchName)  );

				/* Plus, if the working folder's parents are not in the branch (yet), add them as well
				 * (they'll be in it after the user commits something...). */
				SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhInfo, "parents", &pvaParents)  );
				SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &numParents)  );
				for (i = 0; i < numParents; i++)
				{
					SG_bool already_in_rev_spec = SG_FALSE;
					SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, i, &pszRefParent)  );
					SG_ERR_CHECK(  SG_rev_spec__contains(pCtx, pRepo, pRevSpec_Allocated, pszRefParent,
														 &already_in_rev_spec)  );
					if(!already_in_rev_spec)
						SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec_Allocated, pszRefParent)  );
				}
			}
			else
			{
				/* If the branch doesn't exist, add the working folder's baseline(s) to the rev spec
				 * and force a dag walk. */
				SG_uint32 numParents, i;
				const char* pszRefParent;

				SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhInfo, "parents", &pvaParents)  );
				SG_ERR_CHECK(  SG_varray__count(pCtx, pvaParents, &numParents)  );
				for (i = 0; i < numParents; i++)
				{
					SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaParents, i, &pszRefParent)  );
					SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec_Allocated, pszRefParent)  );
				}
				bMyBranchWalkRecommendation = SG_TRUE;
			}

		}
	}

	// Determine the starting changeset IDs.  strBranch and bLeaves control this.
	// We do this step here, so that repo paths can be looked up before we call into history__core.
	SG_ERR_CHECK( sg_vv2__history__get_starting_changesets(pCtx, pRepo, pRevSpec_Allocated,
														   &pStringArrayChangesets,
														   &pStringArrayChangesetsMissing,
														   &bRecommendDagWalk,
														   &bLeaves) );
	if (pStringArrayChangesetsMissing)
	{
		// See K2177, K1322, W0836, W8132.  We requested specific starting
		// points and ran into some csets that were referenced (by --tag
		// or --branch) that are not present in the local repo.  Try to
		// silently ignore them.
		SG_uint32 nrFound = 0;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringArrayChangesets, &nrFound)  );
		if (nrFound > 0)
		{
			// Yes there were missing csets, but we still found some
			// of the referenced ones.  Just ignore the missing ones.
			// This should behave just like we had the older tag/branch
			// dag prior to the push -r on the vc dag.
		}
		else
		{
			const char * psz_0;
			// TODO 2012/10/19 Do we want a different message if the number of missing is > 1 ?
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringArrayChangesetsMissing, 0, &psz_0)  );
			SG_ERR_THROW2(  SG_ERR_CHANGESET_BLOB_NOT_FOUND,
							(pCtx, "%s", psz_0)  );
		}
	}

	bRecommendDagWalk = bRecommendDagWalk || bMyBranchWalkRecommendation;
	
	//This hack is here to  detect when we're being asked for the parent of a certain
	//object from the sg_parents code.  parents always wants the dag walk.
	//The better solution would be to allow users to pass in a flag about their dagwalk
	//preferences
	if (count_args == 1 && nResultLimit == 1)
		bRecommendDagWalk = SG_TRUE;

	if (bListAll)
	{
		// See W8493.  If they gave us a --list-all along with a --rev or --tag, they
		// want to force us to show the full history rather than just the info for the
		// named cset.
		bRecommendDagWalk = SG_TRUE;
	}

	if (pRevSpec_single_revisions)
	{
		// We DO NOT pass a psaMissingHids here because we want
		// it to throw if the user names a missing cset.
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo__dedup(pCtx, pRepo, pRevSpec_single_revisions, SG_TRUE,
														 &pStringArrayChangesets_single_revisions, NULL)  );
	}

	// TODO 2012/07/03 The deviates from the model.  This call directly returns the
	// TODO            allocated data into the caller's pointers.  If anything fails
	// TODO            (such as the call to get the branches below), we'll probably
	// TODO            leak the result and token.

	SG_ERR_CHECK( SG_history__run(pCtx, pRepo, pStringArrayGIDs,
					pStringArrayChangesets, pStringArrayChangesets_single_revisions,
					pszUser, pszStamp, nResultLimit, bLeaves, bHideObjectMerges,
					nFromDate, nToDate, bRecommendDagWalk, SG_FALSE, pbHasResult, ppResult, ppHistoryToken) );

	/* This is kind of a hack. History callers often need branch data to format ouput.
	 * But we open the repo down here. I didn't want to open/close it again. And there's logic
	 * in here about which repo to open. So instead, we do this. */
	if (ppvhBranchPile)
	{
		if (pvhBranchPile)
		{
			*ppvhBranchPile = pvhBranchPile;
			pvhBranchPile = NULL;
		}
		else
			SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, ppvhBranchPile)  );
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_Allocated);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesets);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesetsMissing);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesets_single_revisions);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	SG_VHASH_NULLFREE(pCtx, pvhInfo);
	SG_REPO_NULLFREE(pCtx, pRepo);
}
