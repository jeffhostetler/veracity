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

#include <sg.h>

#define TRACE_SYNC 0

/* There are several common scenarios that cause the generation hints to be non-helpful to the DAG 
 * connection algorithm. In those cases, we add this number of generations to dagfrags, instead. */
#define FALLBACK_GENS_PER_ROUNDTRIP 1000

/**
 * Recursively compare dagnodes depth-first. 
 * Thorough in stupid ways: intended only for use by tests.
 */
static void _silly_exhaustive_compare_dagnodes_for_tests_only(
	SG_context* pCtx,
	SG_uint64 iDagNum,
	SG_repo* pRepo1,
	SG_dagnode* pDagnode1,
	SG_repo* pRepo2,
	SG_dagnode* pDagnode2,
	SG_bool* pbIdentical)
{
	SG_bool bDagnodesEqual = SG_FALSE;
	SG_uint32 iParentCount1, iParentCount2;
	const char** paParentIds1 = NULL;
	const char** paParentIds2 = NULL;
	SG_dagnode* pParentDagnode1 = NULL;
	SG_dagnode* pParentDagnode2 = NULL;

	SG_NULLARGCHECK_RETURN(pDagnode1);
	SG_NULLARGCHECK_RETURN(pDagnode2);
	SG_NULLARGCHECK_RETURN(pbIdentical);

	*pbIdentical = SG_TRUE;

	// Compare the dagnodes.  If they're different, return false.
	SG_ERR_CHECK(  SG_dagnode__equal(pCtx, pDagnode1, pDagnode2, &bDagnodesEqual)  );
	if (!bDagnodesEqual)
	{
#if TRACE_SYNC
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "dagnodes not equal\n")  );
#endif
		*pbIdentical = SG_FALSE;
		return;
	}

	// The dagnodes are identical.  Look at their parents.
	SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pDagnode1, &iParentCount1, &paParentIds1)  );
	SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pDagnode2, &iParentCount2, &paParentIds2)  );
	if (iParentCount1 == iParentCount2)
	{
		// The dagnodes have the same number of parents.  Compare the parents recursively.
		SG_uint32 i;
		for (i = 0; i < iParentCount1; i++)
		{
			SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo1, iDagNum, paParentIds1[i], &pParentDagnode1)  );
			SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo2, iDagNum, paParentIds2[i], &pParentDagnode2)  );

			SG_ERR_CHECK(  _silly_exhaustive_compare_dagnodes_for_tests_only(pCtx, iDagNum, pRepo1, pParentDagnode1, pRepo2, pParentDagnode2, pbIdentical)  );
			SG_DAGNODE_NULLFREE(pCtx, pParentDagnode1);
			SG_DAGNODE_NULLFREE(pCtx, pParentDagnode2);
			if (!(*pbIdentical))
				break;
		}
	}
	else
	{
		// The dagnodes have a different number of parents.
		*pbIdentical = SG_FALSE;
	}

	// fall through
fail:
	SG_DAGNODE_NULLFREE(pCtx, pParentDagnode1);
	SG_DAGNODE_NULLFREE(pCtx, pParentDagnode2);

}

/**
 * Compare all the nodes of a single DAG in two repos.
 */
void SG_sync_debug__compare_one_dag(SG_context* pCtx,
							 SG_repo* pRepo1,
							 SG_repo* pRepo2,
							 SG_uint64 iDagNum,
							 SG_bool* pbIdentical)
{
	SG_bool bFinalResult = SG_FALSE;
	SG_rbtree* prbRepo1Leaves = NULL;
	SG_rbtree* prbRepo2Leaves = NULL;
	SG_uint32 iRepo1LeafCount, iRepo2LeafCount;
	SG_rbtree_iterator* pIterator = NULL;
	const char* pszId = NULL;
	SG_dagnode* pRepo1Dagnode = NULL;
	SG_dagnode* pRepo2Dagnode = NULL;
	SG_bool bFoundRepo1Leaf = SG_FALSE;
	SG_bool bFoundRepo2Leaf = SG_FALSE;
	SG_bool bDagnodesEqual = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pRepo1);
	SG_NULLARGCHECK_RETURN(pRepo2);
	SG_NULLARGCHECK_RETURN(pbIdentical);

	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo1, iDagNum, &prbRepo1Leaves)  );
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo2, iDagNum, &prbRepo2Leaves)  );

	SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbRepo1Leaves, &iRepo1LeafCount)  );
	SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbRepo2Leaves, &iRepo2LeafCount)  );

	if (iRepo1LeafCount != iRepo2LeafCount)
	{
#if TRACE_SYNC
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "leaf count differs\n")  );
#endif
		goto Different;
	}

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, prbRepo1Leaves, &bFoundRepo1Leaf, &pszId, NULL)  );
	while (bFoundRepo1Leaf)
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbRepo2Leaves, pszId, &bFoundRepo2Leaf, NULL)  );
		if (!bFoundRepo2Leaf)
		{
#if TRACE_SYNC && 0
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "couldn't locate leaf\r\n")  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Repo 1 leaves:\r\n")  );
			SG_ERR_CHECK(  SG_rbtree_debug__dump_keys_to_console(pCtx, prbRepo1Leaves) );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Repo 2 leaves:\r\n")  );
			SG_ERR_CHECK(  SG_rbtree_debug__dump_keys_to_console(pCtx, prbRepo2Leaves) );
			SG_ERR_CHECK(  SG_console__flush(pCtx, SG_CS_STDERR)  );
#endif
			goto Different;
		}

		SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo1, iDagNum, pszId, &pRepo1Dagnode)  );
		SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo2, iDagNum, pszId, &pRepo2Dagnode)  );

		SG_ERR_CHECK(  _silly_exhaustive_compare_dagnodes_for_tests_only(pCtx, iDagNum, pRepo1, pRepo1Dagnode, pRepo2, pRepo2Dagnode, &bDagnodesEqual)  );

		SG_DAGNODE_NULLFREE(pCtx, pRepo1Dagnode);
		SG_DAGNODE_NULLFREE(pCtx, pRepo2Dagnode);

		if (!bDagnodesEqual)
			goto Different;

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &bFoundRepo1Leaf, &pszId, NULL)  );
	}

	bFinalResult = SG_TRUE;

Different:
	*pbIdentical = bFinalResult;

	// fall through
fail:
	SG_RBTREE_NULLFREE(pCtx, prbRepo1Leaves);
	SG_RBTREE_NULLFREE(pCtx, prbRepo2Leaves);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

void SG_sync_debug__compare_repo_dags(SG_context* pCtx,
								SG_repo* pRepo1,
								SG_repo* pRepo2,
								SG_bool* pbIdentical)
{
	SG_uint64* paRepo1DagNums = NULL;
	SG_uint64* paRepo2DagNums = NULL;
	SG_uint32 iRepo1DagCount, iRepo2DagCount;
	SG_uint32 i;

	SG_NULLARGCHECK_RETURN(pRepo1);
	SG_NULLARGCHECK_RETURN(pRepo2);
	SG_NULLARGCHECK_RETURN(pbIdentical);

	*pbIdentical = SG_TRUE;

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo1, &iRepo1DagCount, &paRepo1DagNums)  );
	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo2, &iRepo2DagCount, &paRepo2DagNums)  );

	if (iRepo1DagCount != iRepo2DagCount)
	{
#if TRACE_SYNC
		{
			char buf[SG_DAGNUM__BUF_MAX__HEX];

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "dag count differs\n")  );

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Repo 1 dags:\r\n")  );
			for (i = 0; i < iRepo1DagCount; i++)
			{
				SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, paRepo1DagNums[i], buf, sizeof(buf))  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "%s\r\n", buf)  );
			}

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Repo 2 dags:\r\n")  );
			for (i = 0; i < iRepo2DagCount; i++)
			{
				SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, paRepo2DagNums[i], buf, sizeof(buf))  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "%s\r\n", buf)  );
			}
		}
#endif
		SG_NULLFREE(pCtx, paRepo1DagNums);
		SG_NULLFREE(pCtx, paRepo2DagNums);
		*pbIdentical = SG_FALSE;
	}
	else
	{
#if TRACE_SYNC
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Both repos have %u DAGs\r\n", iRepo1DagCount)  );
#endif
		
		for (i = 0; i < iRepo1DagCount; i++)
		{
			SG_ERR_CHECK(  SG_sync_debug__compare_one_dag(pCtx, pRepo1, pRepo2, paRepo1DagNums[i], pbIdentical)  );
			if (!(*pbIdentical))
				break;
		}
	}

	// fall through
fail:
	SG_NULLFREE(pCtx, paRepo1DagNums);
	SG_NULLFREE(pCtx, paRepo2DagNums);
}

void SG_sync__compare_repo_blobs(SG_context* pCtx,
								 SG_repo* pRepo1,
								 SG_repo* pRepo2,
								 SG_bool* pbIdentical)
{
    SG_blobset* pbs1 = NULL;
    SG_blobset* pbs2 = NULL;

	SG_NULLARGCHECK_RETURN(pRepo1);
	SG_NULLARGCHECK_RETURN(pRepo2);
	SG_NULLARGCHECK_RETURN(pbIdentical);

    SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepo1, 0, 0, 0, &pbs1)  );
    SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepo1, 0, 0, 0, &pbs2)  );

    SG_ERR_CHECK(  SG_blobset__compare_to(pCtx, pbs1, pbs2, pbIdentical)  );

	// fall through
fail:
    SG_BLOBSET_NULLFREE(pCtx, pbs1);
    SG_BLOBSET_NULLFREE(pCtx, pbs2);
}

void SG_sync__add_blobs_to_fragball(SG_context* pCtx, SG_fragball_writer* pfb, SG_vhash* pvh_missing_blobs)
{
	SG_uint32 iMissingBlobCount;
	const char** paszHids = NULL;

	SG_NULLARGCHECK_RETURN(pfb);
	SG_NULLARGCHECK_RETURN(pvh_missing_blobs);

	SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh_missing_blobs, &iMissingBlobCount)  );
	if (iMissingBlobCount > 0)
	{
		SG_uint32 i;
		SG_ERR_CHECK(  SG_allocN(pCtx, iMissingBlobCount, paszHids)  );
		for (i = 0; i < iMissingBlobCount; i++)
			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_missing_blobs, i, &paszHids[i], NULL)  );
	}

	SG_ERR_CHECK(  SG_fragball__write__blobs(pCtx, pfb, paszHids, iMissingBlobCount)  );

	/* fall through */
fail:
	SG_NULLFREE(pCtx, paszHids);
}


void SG_sync__remember_sync_target(SG_context* pCtx, const char * pszLocalRepoDescriptor, const char * pszSyncTarget)
{
	SG_string * pString = NULL;
	SG_varray * pva_targets = NULL;
	SG_bool bFound = SG_FALSE;
	SG_uint32 nEntry = 0;

	//Save this destination to the local setting of previously used destinations.
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "%s/%s/%s",
									SG_LOCALSETTING__SCOPE__INSTANCE,
									pszLocalRepoDescriptor,
									SG_LOCALSETTING__SYNC_TARGETS)  );
	SG_ERR_CHECK(  SG_localsettings__get__varray(pCtx, SG_string__sz(pString), NULL, &pva_targets, NULL)  );
	if (pva_targets)
		SG_ERR_CHECK(  SG_varray__find__sz(pCtx, pva_targets, pszSyncTarget, &bFound, &nEntry)  );
	else
		SG_VARRAY__ALLOC(pCtx, &pva_targets);
	if (!bFound)
	{
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_targets, pszSyncTarget)  );
		SG_ERR_CHECK(  SG_localsettings__update__varray(pCtx, SG_string__sz(pString), pva_targets)  );
	}
fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_VARRAY_NULLFREE(pCtx, pva_targets);
}

void SG_sync__build_best_guess_dagfrag(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	SG_rbtree* prbStartFromHids,
	SG_vhash* pvhConnectToHidsAndGens,
	SG_dagfrag** ppFrag)
{
	SG_uint32 i, countConnectTo;
	SG_rbtree_iterator* pit = NULL;
	SG_dagnode* pdn = NULL;
	SG_dagfrag* pFrag = NULL;
	SG_repo_fetch_dagnodes_handle* pdh = NULL;

	SG_int32 minGen = SG_INT32_MAX;
	SG_int32 maxGen = 0;
	SG_uint32 gensToFetch = 0;

	char* psz_repo_id = NULL;
	char* psz_admin_id = NULL;

	SG_bool bNextHid;
	const char* pszRefHid;
	SG_int32 gen;

#if TRACE_SYNC
	SG_int64 startTime;
	SG_int64 endTime;
#endif

	SG_NULLARGCHECK_RETURN(prbStartFromHids);

	/* Find the minimum generation in pertinent "connect to" nodes. */
	if (pvhConnectToHidsAndGens)
	{
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhConnectToHidsAndGens, &countConnectTo)  );
		for (i = 0; i < countConnectTo; i++)
		{
			SG_int32 gen;
			SG_ERR_CHECK(  SG_vhash__get_nth_pair__int32(pCtx, pvhConnectToHidsAndGens, i, &pszRefHid, &gen)  );
			if (gen < minGen)
				minGen = gen;
		}
	}

	/* Happens when pulling into an empty repo, or when an entire dag is specifically requested. */
	if (minGen == SG_INT32_MAX)
		minGen = -1;

	SG_ERR_CHECK(  SG_repo__fetch_dagnodes__begin(pCtx, pRepo, iDagNum, &pdh)  );

	/* Find the maximum generation in pertinent "start from" nodes. */
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, prbStartFromHids, &bNextHid, &pszRefHid, NULL)  );
	while (bNextHid)
	{
		SG_ERR_CHECK(  SG_repo__fetch_dagnodes__one(pCtx, pRepo, pdh, pszRefHid, &pdn)  );
		SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pdn, &gen)  );
		if (gen > maxGen)
			maxGen = gen;
		SG_DAGNODE_NULLFREE(pCtx, pdn);

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &bNextHid, &pszRefHid, NULL)  );
	}
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

	if (maxGen <= minGen)
		gensToFetch = FALLBACK_GENS_PER_ROUNDTRIP;
	else
		gensToFetch = maxGen - minGen;

#if TRACE_SYNC
	{
		char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
		SG_uint32 count;

		SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf_dagnum, sizeof(buf_dagnum))  );

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Building best guess dagfrag for dag %s...\n", buf_dagnum)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Starting from nodes:\n")  );
		SG_ERR_CHECK(  SG_rbtree_debug__dump_keys_to_console(pCtx, prbStartFromHids)  );
		SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhConnectToHidsAndGens, "Connecting to nodes")  );

		SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbStartFromHids, &count)  );

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "result has %d generations from %u starting nodes.\n",
			gensToFetch, count)  );
		SG_ERR_CHECK(  SG_console__flush(pCtx, SG_CS_STDERR)  );

		SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &startTime)  );
	}
#endif

	/* Return a frag with the corresponding generations filled in. */
	SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &psz_repo_id)  );
	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_admin_id)  );
	SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx, &pFrag, psz_repo_id, psz_admin_id, iDagNum)  );

	SG_ERR_CHECK(  SG_dagfrag__load_from_repo__multi(pCtx, pFrag, pRepo, prbStartFromHids, gensToFetch)  );
#if TRACE_SYNC
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &endTime)  );
	{
		SG_uint32 dagnodeCount;
		double seconds = ((double)endTime-(double)startTime)/1000;

		SG_ERR_CHECK(  SG_dagfrag__dagnode_count(pCtx, pFrag, &dagnodeCount)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, " - %u nodes in frag, built in %1.3f seconds\n", dagnodeCount, seconds)  );
		SG_ERR_CHECK(  SG_dagfrag_debug__dump__console(pCtx, pFrag, "best-guess dagfrag", 0, SG_CS_STDERR)  );
	}
#endif

	*ppFrag = pFrag;
	pFrag = NULL;

	/* Common cleanup */
fail:
	SG_NULLFREE(pCtx, psz_repo_id);
	SG_NULLFREE(pCtx, psz_admin_id);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
	SG_ERR_IGNORE(  SG_repo__fetch_dagnodes__end(pCtx, pRepo, &pdh)  );
}

void SG_sync__make_temp_path(SG_context* pCtx, const char* psz_name, SG_pathname** ppPath)
{
	SG_pathname* pPath_tempdir = NULL;
	SG_pathname* pPath_staging = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPath_tempdir)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_staging, pPath_tempdir, psz_name)  );

	*ppPath = pPath_staging;
	pPath_staging = NULL;

	/* fallthru */

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_tempdir);
	SG_PATHNAME_NULLFREE(pCtx, pPath_staging);
}

void SG_sync__closet_user_dags(
	SG_context* pCtx, 
	SG_repo* pRepoSrcNotMine, 
	const char* pszRefHidLeafSrc,
	SG_varray** ppvaSyncedUserList)
{
	char* pszSrcAdminId = NULL;
	char* pszHidLeafSrc = NULL;
	char* pszHidLeafDest = NULL;
	SG_vhash* pvhDescriptors = NULL;
	SG_repo* pRepoDest = NULL;
	SG_repo* pRepoSrcMine = NULL;
	char* pszDestAdminId = NULL;

	/* Using disallowed characters to ensure no collision with an actual repo name.
	 * Not that this isn't actually stored anywhere--we just use it as a key in the
	 * vhash below where the /real/ repos have descriptor names. */
	const char* pszRefUserMasterFakeName = "\\/USER_MASTER\\/"; 

	/* The repo routines do a null arg check of pRepoSrcNotMine.
	   The other args are optional. */

	if (!pszRefHidLeafSrc)
	{
		SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepoSrcNotMine, NULL, SG_DAGNUM__USERS, &pszHidLeafSrc)  );
		pszRefHidLeafSrc = pszHidLeafSrc;
	}

	/* Add all repositories in "normal" status, to the list we'll iterate over. */
	SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pvhDescriptors)  );

	/* If it exists, add the user master repo to the list. */
	{
		SG_bool bExists = SG_FALSE;
		SG_ERR_CHECK(  SG_repo__user_master__exists(pCtx, &bExists)  );
		if (bExists)
			SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvhDescriptors, pszRefUserMasterFakeName)  );
	}

	/* Iterate over the repositories, syncing the user database. */
	{
		SG_int32 i = 0;
		SG_uint32 numDescriptors = 0;

		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhDescriptors, &numDescriptors)  );
		for(i = 0; i < (SG_int32)numDescriptors; i++)
		{
			const char* pszRefNameDest = NULL;
			SG_bool bAdminIdsMatch = SG_TRUE;
			const SG_variant* pvRefDest = NULL;

			/* Note that the source repo will be in this loop, too, but we don't need to check for 
			 * it, adding another strcmp, because the leaf hid comparison below will effectively 
			 * skip it. So we do one extra leaf fetch and comparison, total, rather than an extra 
			 * strcmp for every repo in the closet. */

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhDescriptors, i, &pszRefNameDest, &pvRefDest)  );

			if (SG_VARIANT_TYPE_NULL == pvRefDest->type)
				SG_ERR_CHECK(  SG_REPO__USER_MASTER__OPEN(pCtx, &pRepoDest)  );
			else
				SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRefNameDest, &pRepoDest)  );

			SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepoDest, NULL, SG_DAGNUM__USERS, &pszHidLeafDest)  );

			if (strcmp(pszRefHidLeafSrc, pszHidLeafDest))
			{
				/* Pull from source to dest. 
				 * Pull is generally faster than push, so we're using it on purpose. */
				SG_pull__admin__local(pCtx, pRepoDest, pRepoSrcNotMine, NULL);
				if (SG_context__has_err(pCtx))
				{
					/* If there's an admin id mismatch, don't die. Log a warning and move on. */
					if (SG_context__err_equals(pCtx, SG_ERR_ADMIN_ID_MISMATCH))
					{
						const char* pszRefNameSrc = NULL;

						SG_ERR_DISCARD;

						SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrcNotMine, &pszRefNameSrc)  );
						if (!pszRefNameSrc)
							pszRefNameSrc = pszRefUserMasterFakeName;
						SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepoSrcNotMine, &pszSrcAdminId)  );

						SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepoDest, &pszDestAdminId)  );

						SG_ERR_CHECK(  SG_log__report_warning(pCtx, 
							"admin-id mismatch when syncing users: source repo %s has %s, dest repo %s has %s",
							pszRefNameSrc, pszSrcAdminId, pszRefNameDest, pszDestAdminId)  );

						bAdminIdsMatch = SG_FALSE;

						SG_NULLFREE(pCtx, pszDestAdminId);
						SG_NULLFREE(pCtx, pszSrcAdminId);
					}
					else
						SG_ERR_RETHROW;
				}

				if (bAdminIdsMatch)
				{
					SG_NULLFREE(pCtx, pszHidLeafDest);
					SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepoDest, NULL, SG_DAGNUM__USERS, &pszHidLeafDest)  );

					if (strcmp(pszRefHidLeafSrc, pszHidLeafDest))
					{
						/* The pull from source to dest resulted in a new leaf. 
						 * Use the new leaf and restart the loop. */
						SG_NULLFREE(pCtx, pszHidLeafSrc);
						pszRefHidLeafSrc = pszHidLeafSrc = pszHidLeafDest;
						pszHidLeafDest = NULL;

						SG_REPO_NULLFREE(pCtx, pRepoSrcMine);
						pRepoSrcNotMine = pRepoSrcMine = pRepoDest;
						pRepoDest = NULL;

						i = -1; /* start again at the first descriptor */
					}

				}
			}

			SG_NULLFREE(pCtx, pszHidLeafDest);
			SG_REPO_NULLFREE(pCtx, pRepoDest);
		}
	}

	if (ppvaSyncedUserList)
		SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepoSrcNotMine, ppvaSyncedUserList)  );

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszSrcAdminId);
	SG_NULLFREE(pCtx, pszHidLeafSrc);
	SG_NULLFREE(pCtx, pszHidLeafDest);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptors);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_REPO_NULLFREE(pCtx, pRepoSrcMine);
	SG_NULLFREE(pCtx, pszDestAdminId);
}

void SG_sync__get_account_info__from_repo_info(SG_context* pCtx, const SG_vhash* pvhRepoInfo, SG_vhash** ppvhAccountInfo)
{
	SG_vhash* pvhRef = NULL;
	SG_vhash* pvhAccountInfo = NULL;

	SG_NULLARGCHECK_RETURN(ppvhAccountInfo);

	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhRepoInfo, SG_SYNC_REPO_INFO_KEY__ACCOUNT, &pvhRef)  );
	if (pvhRef)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhAccountInfo)  );
		SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pvhRef, pvhAccountInfo)  );
		*ppvhAccountInfo = pvhAccountInfo;
		pvhAccountInfo = NULL;
	}
	
	return;

fail:
	SG_NULLFREE(pCtx, pvhAccountInfo);
}

void SG_sync__get_account_info__details(SG_context* pCtx, const SG_vhash* pvhAccountInfo, const char** ppszMessage, const char** ppszStatus)
{
	if (pvhAccountInfo)
	{
		if (ppszMessage)
		{
			const char* pszRefMsg = NULL;
			SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvhAccountInfo, SG_SYNC_REPO_INFO_KEY__ACCOUNT_MSG, &pszRefMsg)  );
			*ppszMessage = pszRefMsg;
		}
		if (ppszStatus)
		{
			const char* pszRefStatus = NULL;
			SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pvhAccountInfo, SG_SYNC_REPO_INFO_KEY__ACCOUNT_STATUS, &pszRefStatus)  );
			*ppszStatus = pszRefStatus;
		}
	}
}
