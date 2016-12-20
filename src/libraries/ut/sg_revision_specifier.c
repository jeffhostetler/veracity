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

//////////////////////////////////////////////////////////////////

BEGIN_EXTERN_C;

typedef struct __revspec
{
	SG_rev_spec_type specType;
	char*	pszGiven;
} _revspec;

/* The structure for which SG_rev_spec is an opaque wrapper.
 * 
 * Because we use vhashes as a serialization structure for an SG_rev_spec, it would make
 * sense to use a vhash for the core data type here. But we don't, on purpose, because
 * the order in which the rev specs were provided is important in some cases, and a vhash
 * won't preserve that. */
typedef struct __sg_rev_spec
{
	SG_uint64	iDagNum;
	SG_vector*	pvecRevSpecs;
	SG_uint32	iCountRevs;
	SG_uint32	iCountTags;
	SG_uint32	iCountBranches;
} _sg_rev_spec;

static void _add_rev_spec(SG_context* pCtx, _sg_rev_spec* pRevSpec, SG_rev_spec_type specType, const char* pszGiven);

//////////////////////////////////////////////////////////////////

/**
 * Lookup all of the heads of the named branch and if
 * they exist in the local repo, put the hids in psaHids.
 *
 * If any are not present in the local repo, either
 * put them in the optional psaMissingHids or throw.
 *
 */
static void _lookup_branch(SG_context * pCtx,
						   SG_repo * pRepo,
						   SG_vhash * pvhPile,
						   const char * pszBranchName,
						   SG_bool bAllowAmbiguousBranches,
						   SG_stringarray * psaHids,
						   SG_stringarray * psaMissingHids)
{
	SG_vhash * pvhRefBranches;
	SG_vhash * pvhRefThisBranch;
	SG_vhash * pvhRefHids;
	SG_uint32 k, nrHids;
	SG_uint32 nrMatched = 0;
	SG_uint32 nrMissing = 0;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhPile, "branches", &pvhRefBranches)  );
	
	SG_vhash__get__vhash(pCtx, pvhRefBranches, pszBranchName, &pvhRefThisBranch);
	SG_ERR_REPLACE(SG_ERR_VHASH_KEYNOTFOUND, SG_ERR_BRANCH_NOT_FOUND);
	SG_ERR_CHECK_CURRENT;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefThisBranch, "values", &pvhRefHids)  );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefHids, &nrHids)  );
	for (k=0; k<nrHids; k++)
	{
		const char * pszHid_k;
		SG_bool bExists_k;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefHids, k, &pszHid_k, NULL)  );
		SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pRepo, pszHid_k, &bExists_k)  );
		if (bExists_k)
		{
			if ((nrMatched > 0) && (!bAllowAmbiguousBranches))
			{
				// Throw if we have more than one head and they requested that
				// the branch only have 1 head.
				//
				// Note that we only counted heads that are actually present
				// in the local repo (which is a little different from the
				// original _unambiguous_lookup code).
				SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE, (pCtx, "%s", pszBranchName)  );
			}

			SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaHids, pszHid_k)  );
			nrMatched++;
		}
		else if (psaMissingHids)	// they want us to collect the misses rather than throw.
		{
			SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaMissingHids, pszHid_k)  );
			nrMissing++;
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT, 
							(pCtx, "Branch '%s' refers to changeset '%s'. Consider pulling.",
							 pszBranchName, pszHid_k)  );
		}
	}

	if ((nrMatched == 0) && (nrMissing > 0))
	{
		if (nrMissing == 1)
		{
			const char * psz_0;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaMissingHids, 0, &psz_0)  );
			SG_ERR_THROW2(  SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT, 
							(pCtx, "Branch '%s' refers to changeset '%s'. Consider pulling.",
							 pszBranchName, psz_0)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT, 
							(pCtx, "Branch '%s' refers to %d missing changesets. Consider pulling.",
							 pszBranchName, nrMissing)  );
		}
	}

fail:
	return;
}


/* TODO: should this routine actually be in SG_vc_branches? 
 *		 I hate poking around inside vhashes created by other modules. */

/* We do branch lookups in these helpers because we look them up in a loop, 
and there's no reason to cleanup or re-retrieve the pile in each iteration. */
static void _ambiguous_branch_lookup(
	SG_context* pCtx, 
	SG_vhash* pvhCleanPileOfBranches, 
	const char* pszBranch, 
	SG_varray** ppvaFullHids)
{
	SG_vhash* pvhRefBranches;
	SG_vhash* pvhRefThisBranch;
	SG_vhash* pvhRefHids;
	SG_varray* pva = NULL;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhCleanPileOfBranches, "branches", &pvhRefBranches)  );
	
	SG_vhash__get__vhash(pCtx, pvhRefBranches, pszBranch, &pvhRefThisBranch);
	SG_ERR_REPLACE(SG_ERR_VHASH_KEYNOTFOUND, SG_ERR_BRANCH_NOT_FOUND);
	SG_ERR_CHECK_CURRENT;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefThisBranch, "values", &pvhRefHids)  );

	SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvhRefHids, &pva)  );

	*ppvaFullHids = pva;
	pva = NULL;

	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}

#if 0 // TODO REMOVE THIS....
/* We do branch lookups in these helpers because we look them up in a loop, 
 * and there's no reason to cleanup or re-retrieve the pile in each iteration. */
static void _unambiguous_branch_lookup(
	SG_context* pCtx, 
	SG_vhash* pvhCleanPileOfBranches, 
	const char* pszBranch, 
	char** ppszFullHid)
{
	SG_varray* pva = NULL;
	SG_uint32 count = 0;

	SG_ERR_CHECK(  _ambiguous_branch_lookup(pCtx, pvhCleanPileOfBranches, pszBranch, &pva)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
	if (count == 0)
		SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
	else if (count == 1)
	{
		const char* pszRef;
		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, 0, &pszRef)  );
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRef, ppszFullHid)  );
	}
	else
		SG_ERR_THROW2(  SG_ERR_BRANCH_NEEDS_MERGE, (pCtx, "%s", pszBranch)  );

	/* common cleanup */
fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}
#endif // 0

//////////////////////////////////////////////////////////////////

void SG_rev_spec__alloc(SG_context* pCtx, SG_rev_spec** ppRevSpec)
{
	_sg_rev_spec* p = NULL;
	
	SG_NULLARGCHECK_RETURN(ppRevSpec);
	
	SG_ERR_CHECK(  SG_alloc1(pCtx, p)  );
	
	p->iDagNum = SG_DAGNUM__VERSION_CONTROL;
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &p->pvecRevSpecs, 5)  );
	
	*ppRevSpec = (SG_rev_spec*)p;

	return;

fail:
	if (p)
	{
		SG_VECTOR_NULLFREE(pCtx, p->pvecRevSpecs);
		SG_NULLFREE(pCtx, p);
	}
}

void SG_rev_spec__alloc__copy(SG_context* pCtx, const SG_rev_spec* pRevSpecSrc, SG_rev_spec** ppRevSpecDest)
{
	SG_rev_spec* pRevSpecDest = NULL;
	_sg_rev_spec* pDest = NULL;
	_sg_rev_spec* pSrc = (_sg_rev_spec*)pRevSpecSrc;
	SG_uint32 i, count;
	_revspec* psSrc;

	SG_NULLARGCHECK_RETURN(pRevSpecSrc);
	SG_NULLARGCHECK_RETURN(ppRevSpecDest);

	SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpecDest)  );
	pDest = (_sg_rev_spec*)pRevSpecDest;

	SG_ERR_CHECK(  SG_vector__length(pCtx, pSrc->pvecRevSpecs, &count)  );
	for (i = 0; i < count; i++)
	{
		SG_ERR_CHECK(  SG_vector__get(pCtx, pSrc->pvecRevSpecs, i, (void**)&psSrc)  );
		SG_ERR_CHECK(  _add_rev_spec(pCtx, pDest, psSrc->specType, psSrc->pszGiven)  );
	}

	*ppRevSpecDest = pRevSpecDest;
	pRevSpecDest = NULL;

	/* Common cleanup */
fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpecDest);
}

static void _free_revspecs_callback(SG_context* pCtx, void * pVoidData)
{
	if (pVoidData)
	{
		_revspec* prs = (_revspec*)pVoidData;
		SG_NULLFREE(pCtx, prs->pszGiven);
		SG_NULLFREE(pCtx, prs);
	}
}

void SG_rev_spec__free(SG_context* pCtx, SG_rev_spec* pRevSpec)
{
	if (pRevSpec)
	{
		_sg_rev_spec* p = (_sg_rev_spec*)pRevSpec;
		SG_ERR_CHECK_RETURN(  SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, p->pvecRevSpecs, _free_revspecs_callback)  );
		SG_ERR_CHECK_RETURN(  SG_free(pCtx, p)  );
	}
}

//////////////////////////////////////////////////////////////////

static void _add_rev_spec(SG_context* pCtx, _sg_rev_spec* pRevSpec, SG_rev_spec_type specType, const char* pszGiven)
{
	_revspec* p = NULL;
	
	SG_ERR_CHECK(  SG_alloc1(pCtx, p)  );
	p->specType = specType;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszGiven, &p->pszGiven)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pRevSpec->pvecRevSpecs, p, NULL)  );

	switch(specType)
	{
		case SPECTYPE_REV:
			pRevSpec->iCountRevs++;
			break;
		case SPECTYPE_TAG:
			pRevSpec->iCountTags++;
			break;
		case SPECTYPE_BRANCH:
			pRevSpec->iCountBranches++;
			break;
		default:
			SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "unknown rev spec type"));
	}

	return;

fail:
	if (p)
	{
		SG_NULLFREE(pCtx, p->pszGiven);
		SG_NULLFREE(pCtx, p);
	}
}

void SG_rev_spec__add_rev(SG_context* pCtx, SG_rev_spec* pRevSpec, const char* pszRev)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NONEMPTYCHECK_RETURN(pszRev);

	SG_ERR_CHECK_RETURN(  _add_rev_spec(pCtx, pMe, SPECTYPE_REV, pszRev)  );
}

void SG_rev_spec__add_tag(SG_context* pCtx, SG_rev_spec* pRevSpec, const char* pszTag)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NONEMPTYCHECK_RETURN(pszTag);

	SG_ERR_CHECK_RETURN(  _add_rev_spec(pCtx, pMe, SPECTYPE_TAG, pszTag)  );
}

void SG_rev_spec__add_branch(SG_context* pCtx, SG_rev_spec* pRevSpec, const char* pszBranch)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NONEMPTYCHECK_RETURN(pszBranch);

	SG_ERR_CHECK_RETURN(  _add_rev_spec(pCtx, pMe, SPECTYPE_BRANCH, pszBranch)  );
}

void SG_rev_spec__add(SG_context * pCtx, SG_rev_spec * pRevSpec, SG_rev_spec_type t, const char * psz)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NONEMPTYCHECK_RETURN(psz);

	SG_ERR_CHECK_RETURN(  _add_rev_spec(pCtx, pMe, t, psz)  );
}

//////////////////////////////////////////////////////////////////

void SG_rev_spec__has_non_vc_specs(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_bool* pbResult)
{
	SG_uint32 count = 0;

	SG_NULLARGCHECK_RETURN(pbResult);

	SG_ERR_CHECK_RETURN(  SG_rev_spec__count_tags(pCtx, pRevSpec, &count)  );
	if (count)
	{
		*pbResult = SG_TRUE;
		return;
	}

	SG_ERR_CHECK_RETURN(  SG_rev_spec__count_branches(pCtx, pRevSpec, &count)  );
	if (count)
	{
		*pbResult = SG_TRUE;
		return;
	}
}

void SG_rev_spec__get_dagnum(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_uint64* piDagnum)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NULLARGCHECK_RETURN(piDagnum);

	*piDagnum = pMe->iDagNum;
}

void SG_rev_spec__set_dagnum(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_uint64 iDagnum)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;

	SG_NULLARGCHECK_RETURN(pRevSpec);

	pMe->iDagNum = iDagnum;
}

//////////////////////////////////////////////////////////////////

void SG_rev_spec__count_revs(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount)
{
	const _sg_rev_spec* pMe = (const _sg_rev_spec*)pRevSpec;
	SG_NULLARGCHECK_RETURN(piCount);

	if (pRevSpec)
		*piCount = pMe->iCountRevs;
	else
		*piCount = 0;
}

void SG_rev_spec__count_tags(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount)
{
	const _sg_rev_spec* pMe = (const _sg_rev_spec*)pRevSpec;
	SG_NULLARGCHECK_RETURN(piCount);

	if (pRevSpec)
		*piCount = pMe->iCountTags;
	else
		*piCount = 0;
}

void SG_rev_spec__count_branches(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount)
{
	const _sg_rev_spec* pMe = (const _sg_rev_spec*)pRevSpec;
	SG_NULLARGCHECK_RETURN(piCount);

	if (pRevSpec)
		*piCount = pMe->iCountBranches;
	else
		*piCount = 0;
}

void SG_rev_spec__count(SG_context* pCtx, const SG_rev_spec* pRevSpec, SG_uint32* piCount)
{
	const _sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;
	SG_NULLARGCHECK_RETURN(piCount);

	if (pRevSpec)
		SG_ERR_CHECK_RETURN(  SG_vector__length(pCtx, pMe->pvecRevSpecs, piCount)  );
	else
		*piCount = 0;
}
SG_bool SG_rev_spec__is_empty(SG_context* pCtx, const SG_rev_spec* pRevSpec)
{
	SG_uint32 iCount=0;
	if(!pRevSpec)
		return SG_TRUE;
	SG_ERR_IGNORE(  SG_rev_spec__count(pCtx, pRevSpec, &iCount)  );
	return iCount==0;
}

//////////////////////////////////////////////////////////////////

static void _to_rbtree(SG_context* pCtx, _sg_rev_spec* pMe, SG_rev_spec_type specType, SG_rbtree** pprb)
{
	SG_rbtree* prb = NULL;
	SG_uint32 count;
	SG_vector* pvecRevSpecs;
	SG_uint32 i = 0;

	SG_NULLARGCHECK_RETURN(pMe);
	pvecRevSpecs = pMe->pvecRevSpecs;
	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &count)  );
	if (count)
	{
		SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, &prb, count, NULL)  );
		for (i = 0; i < count; i++)
		{
			_revspec* p;
			SG_ERR_CHECK(  SG_vector__get(pCtx, pvecRevSpecs, i, (void**)&p)  );
			if (p->specType == specType)
				SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, p->pszGiven)  );
		}

		SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb, &count)  );
		if (count) /* If there's nothing left after filtering out other types, return null. */
			SG_RETURN_AND_NULL(prb, pprb);
	}

fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
}

void SG_rev_spec__revs_rbt(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rbtree** pprbRevs)
{
	SG_ERR_CHECK_RETURN(  _to_rbtree(pCtx, (_sg_rev_spec*)pRevSpec, SPECTYPE_REV, pprbRevs)  );
}

void SG_rev_spec__tags_rbt(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rbtree** pprbTags)
{
	SG_ERR_CHECK_RETURN(  _to_rbtree(pCtx, (_sg_rev_spec*)pRevSpec, SPECTYPE_TAG, pprbTags)  );
}

void SG_rev_spec__branches_rbt(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rbtree** pprbBranches)
{
	SG_ERR_CHECK_RETURN(  _to_rbtree(pCtx, (_sg_rev_spec*)pRevSpec, SPECTYPE_BRANCH, pprbBranches)  );
}

static void _to_stringarray(SG_context* pCtx, _sg_rev_spec* pMe, SG_rev_spec_type specType, SG_stringarray** ppsa)
{
	SG_stringarray* psa = NULL;
	SG_uint32 count;
	SG_vector* pvecRevSpecs;

	SG_NULLARGCHECK_RETURN(pMe);
	pvecRevSpecs = pMe->pvecRevSpecs;
	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &count)  );
	if (count)
	{
		SG_uint32 i;
		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count)  );
		for (i = 0; i < count; i++)
		{
			_revspec* p;
			SG_ERR_CHECK(  SG_vector__get(pCtx, pvecRevSpecs, i, (void**)&p)  );
			if (p->specType == specType)
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, p->pszGiven)  );
		}
	}

	SG_RETURN_AND_NULL(psa, ppsa);

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

void SG_rev_spec__revs(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_stringarray** ppsaRevs)
{
	SG_ERR_CHECK_RETURN(  _to_stringarray(pCtx, (_sg_rev_spec*)pRevSpec, SPECTYPE_REV, ppsaRevs)  );
}

void SG_rev_spec__tags(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_stringarray** ppsaTags)
{
	SG_ERR_CHECK_RETURN(  _to_stringarray(pCtx, (_sg_rev_spec*)pRevSpec, SPECTYPE_TAG, ppsaTags)  );
}

void SG_rev_spec__branches(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_stringarray** ppsaBranches)
{
	SG_ERR_CHECK_RETURN(  _to_stringarray(pCtx, (_sg_rev_spec*)pRevSpec, SPECTYPE_BRANCH, ppsaBranches)  );
}

//////////////////////////////////////////////////////////////////

void SG_rev_spec__get_all(
	SG_context* pCtx, 
	const char* pszRepoName,
	const SG_rev_spec* pRevSpec, 
	SG_bool bAllowAmbiguousBranches,
	SG_stringarray** ppsaFullHids)
{
	SG_repo* pRepo = NULL;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, bAllowAmbiguousBranches, ppsaFullHids, NULL)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

/**
 * This routine is going to expand all of the various --rev, --tag,
 * and --branch options and build a stringarray doing all of the
 * hid-, tag-, or branch-lookup.  But it doesn't de-dup.
 *
 * See SG_rev_spec__get_all__repo__dedup() below.
 *
 */
void SG_rev_spec__get_all__repo(
	SG_context* pCtx, 
	SG_repo* pRepo,
	const SG_rev_spec* pRevSpec, 
	SG_bool bAllowAmbiguousBranches,
	SG_stringarray** ppsaFullHids,
	SG_stringarray** ppsaMissingBranchHeadHids)
{
	const _sg_rev_spec* pMe = (const _sg_rev_spec*)pRevSpec;
	SG_uint32 count;
	SG_stringarray* psa = NULL;
	SG_stringarray* psaMissing = NULL;
	char* pszFullHid = NULL;
	SG_vhash* pvhCleanPileOfBranches = NULL;
	SG_varray* pvaBranchHids = NULL;

	SG_NULLARGCHECK_RETURN(pRevSpec);

	/* Don't add a null arg check for ppsaFullHids. It's legit to call us this way,
	 * simply to error check the rev spec. */

	// ppsaMissingBranchHeadHids is also optional.  it will be used
	// to collect any csets (referenced by a --branch) that are not
	// present in the local repo (because the branches dag
	// get swapped in full even when they do a limited 'vv push -r'.
	// (See K2177/K1322)

	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &count)  );
	if (count)
	{
		SG_uint32 i;

		/* Get a clean pile of branches once. Not in each loop iteration. */
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhCleanPileOfBranches)  );

		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count)  );
		if (ppsaMissingBranchHeadHids)
			SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaMissing, count)  );

		for (i=0; i < count; i++)
		{
			SG_bool bExists;
			_revspec* p;
			SG_ERR_CHECK(  SG_vector__get(pCtx, pMe->pvecRevSpecs, i, (void**)&p)  );
			switch (p->specType)
			{
			case SPECTYPE_REV:
				SG_ERR_CHECK(  SG_repo__hidlookup__dagnode(pCtx, pRepo, pMe->iDagNum, p->pszGiven, &pszFullHid)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, pszFullHid)  );
				SG_NULLFREE(pCtx, pszFullHid);
				break;
			case SPECTYPE_TAG:
				if ( pMe->iDagNum != SG_DAGNUM__VERSION_CONTROL )
					SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "tags in a non-version-control query.") );
				SG_ERR_CHECK(  SG_vc_tags__lookup__tag(pCtx, pRepo, p->pszGiven, &pszFullHid)  );
				SG_ERR_CHECK(  SG_repo__does_blob_exist(pCtx, pRepo, pszFullHid, &bExists)  );
				if (!bExists)
					SG_ERR_THROW2(  SG_ERR_CHANGESET_BLOB_NOT_FOUND,
									(pCtx, "Tag '%s' refers to changeset '%s'.  Consider pulling.",
									 p->pszGiven, pszFullHid)  );
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa, pszFullHid)  );
				SG_NULLFREE(pCtx, pszFullHid);
				break;
			case SPECTYPE_BRANCH:
				if ( pMe->iDagNum != SG_DAGNUM__VERSION_CONTROL )
					SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "branches in a non-version-control query.") );
				SG_ERR_CHECK(  _lookup_branch(pCtx, pRepo, pvhCleanPileOfBranches, p->pszGiven,
											  bAllowAmbiguousBranches,
											  psa, psaMissing)  );
				break;
			default:
				SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "unknown rev spec type"));
			}
		}
	}

	if (psaMissing)
	{
		SG_uint32 nrMissing;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaMissing, &nrMissing)  );
		if (nrMissing == 0)
			SG_STRINGARRAY_NULLFREE(pCtx, psaMissing);
	}

#if 0 && defined(DEBUG)
	SG_ERR_IGNORE(  SG_stringarray_debug__dump_to_console(pCtx, psa, "SG_rev_spec__get_all__repo[main]")  );
	if (psaMissing)
		SG_ERR_IGNORE(  SG_stringarray_debug__dump_to_console(pCtx, psaMissing, "SG_rev_spec__get_all[missing]")  );
#endif

	SG_RETURN_AND_NULL(psa, ppsaFullHids);
	SG_RETURN_AND_NULL(psaMissing, ppsaMissingBranchHeadHids);

fail:
	SG_VHASH_NULLFREE(pCtx, pvhCleanPileOfBranches);
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
	SG_STRINGARRAY_NULLFREE(pCtx, psaMissing);
	SG_NULLFREE(pCtx, pszFullHid);
	SG_VARRAY_NULLFREE(pCtx, pvaBranchHids);
}

void SG_rev_spec__get_one(
	SG_context* pCtx,
	const char* pszRepoName,
	const SG_rev_spec* pRevSpec,
	SG_bool bIgnoreMissingBranchHeads,
	char** ppszChangesetHid,
	char** ppszBranchName)
{
	SG_repo* pRepo = NULL;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, bIgnoreMissingBranchHeads,
											  ppszChangesetHid, ppszBranchName)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void SG_rev_spec__get_one__repo(
	SG_context* pCtx,
	SG_repo* pRepo,
	const SG_rev_spec* pRevSpec,
	SG_bool bIgnoreMissingBranchHeads,
	char** ppszChangesetHid,
	char** ppszBranchName)
{
	const _sg_rev_spec* pMe = (const _sg_rev_spec*)pRevSpec;
	SG_stringarray* psaAll = NULL;
	SG_stringarray* psaMissing = NULL;
	SG_uint32 countResults;

	// TODO 2012/07/02 This routine deviates from the model in that it
	// TODO            directly returns the allocated variables into the
	// TODO            callers variables *BEFORE* we have finished working.
	// TODO            So for example, if we get an error when counting
	// TODO            and/or getting the branch name, we'll leak the cset hid.

	SG_NULLARGCHECK_RETURN(pRevSpec);

	// TODO 2012/10/22 Is this right?  We check the length of the set
	// TODO            of *GIVEN* --rev/--tag/--branch *ARGS* -- rather
	// TODO            than just checking the number of items returned
	// TODO            by __get_all.

	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &countResults)  );
	if (countResults == 1)
	{
		SG_uint32 nrMatched = 0;

		SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, SG_FALSE, &psaAll,
												  ((bIgnoreMissingBranchHeads) ? &psaMissing : NULL))  );
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaAll, &nrMatched)  );
		if (nrMatched == 0)
		{
			if (bIgnoreMissingBranchHeads && psaMissing)
			{
				const char * psz_0 = NULL;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaMissing, 0, &psz_0)  );
				SG_ERR_THROW2(  SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT,
								(pCtx, "Branch refers to changeset '%s'.  Consider pulling.", psz_0));
			}
			else
			{
				SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "no matching changesets"));
			}
		}
		else if (nrMatched == 1)
		{
			if (ppszChangesetHid)
			{
				const char* pszRef;
				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaAll, 0, &pszRef)  );
				SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRef, ppszChangesetHid)  );
			}
			if (ppszBranchName)
			{
				SG_uint32 countBranchSpecs = 0;
				SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, pRevSpec, &countBranchSpecs)  );
				if (countBranchSpecs == 1)
				{
					_revspec* p;
					SG_ERR_CHECK(  SG_vector__get(pCtx, pMe->pvecRevSpecs, 0, (void**)&p)  );
					SG_ERR_CHECK(  SG_STRDUP(pCtx, p->pszGiven, ppszBranchName)  );
				}
			}
		}
		else
		{
			SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "more than one match (%d)", nrMatched)  );
		}
	}
	else if (countResults == 0)
	{
		SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "no matching changesets"));
	}
	else
	{
		SG_ERR_THROW2(SG_ERR_LIMIT_EXCEEDED, (pCtx, "more than one match (%u)", countResults));
	}

	/* common cleanup */
fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaAll);
	SG_STRINGARRAY_NULLFREE(pCtx, psaMissing);
}

//////////////////////////////////////////////////////////////////

void SG_rev_spec__foreach__repo(SG_context* pCtx, SG_repo* pRepo, SG_rev_spec* pRevSpec, SG_rev_spec_foreach_callback* cb, void* ctx)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;
	SG_uint32 i, countBranches;
	char* pszFullHid = NULL;
	SG_vhash* pvhCleanPileOfBranches = NULL;
	SG_varray* pvaFullHids = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NULLARGCHECK_RETURN(cb);

	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhCleanPileOfBranches)  );

	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &countBranches)  );
	for (i = 0; i < countBranches; i++)
	{
		_revspec* p;
		SG_uint32 j;
		SG_uint32 countBranchHids = 0;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pMe->pvecRevSpecs, i, (void**)&p)  );
		switch (p->specType)
		{
		case SPECTYPE_REV:
			SG_ERR_CHECK(  SG_repo__hidlookup__dagnode(pCtx, pRepo, pMe->iDagNum, p->pszGiven, &pszFullHid)  );
			SG_ERR_CHECK(  cb(pCtx, pRepo, SPECTYPE_REV, p->pszGiven, pszFullHid, ctx)  );
			break;
		
		case SPECTYPE_TAG:
			if ( pMe->iDagNum != SG_DAGNUM__VERSION_CONTROL )
				SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "tags in a non-version-control query.") );
			SG_ERR_CHECK(  SG_vc_tags__lookup__tag(pCtx, pRepo, p->pszGiven, &pszFullHid)  );
			SG_ERR_CHECK(  cb(pCtx, pRepo, SPECTYPE_TAG, p->pszGiven, pszFullHid, ctx)  );
			break;
		
		case SPECTYPE_BRANCH:
			if ( pMe->iDagNum != SG_DAGNUM__VERSION_CONTROL )
				SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "branches in a non-version-control query.") );
			SG_ERR_CHECK(  _ambiguous_branch_lookup(pCtx, pvhCleanPileOfBranches, p->pszGiven, &pvaFullHids)  );
			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaFullHids, &countBranchHids)  );
			for (j = 0; j < countBranchHids; j++)
			{
				const char* pszRefHid;
				SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaFullHids, j, &pszRefHid)  );
				SG_ERR_CHECK(  cb(pCtx, pRepo, SPECTYPE_BRANCH, p->pszGiven, pszRefHid, ctx)  );
			}
			SG_VARRAY_NULLFREE(pCtx, pvaFullHids);
			break;
		default:
			SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "unknown rev spec type"));
		}
		SG_NULLFREE(pCtx, pszFullHid);
	}

	SG_VHASH_NULLFREE(pCtx, pvhCleanPileOfBranches);

	return;

fail:
	SG_NULLFREE(pCtx, pszFullHid);
	SG_VHASH_NULLFREE(pCtx, pvhCleanPileOfBranches);
	SG_VARRAY_NULLFREE(pCtx, pvaFullHids);
}

//////////////////////////////////////////////////////////////////

struct _sg_rev_spec__contains__baton
{
	const char * szHid;
	SG_bool found;
};

static SG_rev_spec_foreach_callback _sg_rev_spec__contains__cb;
static void _sg_rev_spec__contains__cb(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_rev_spec_type specType,
	const char* pszGiven,    /* The value as added to the SG_rev_spec. */
	const char* pszFullHid,	 /* The full HID of the looked-up spec. */
	void* ctx)
{
	struct _sg_rev_spec__contains__baton * baton = (struct _sg_rev_spec__contains__baton *)ctx;
	
	SG_UNUSED(pCtx);
	SG_UNUSED(pRepo);
	SG_UNUSED(specType);
	SG_UNUSED(pszGiven);

	if(baton->found)
		return;

	if(SG_stricmp(pszFullHid, baton->szHid)==0)
		baton->found = SG_TRUE;
}

void SG_rev_spec__contains(
	SG_context* pCtx, 
	SG_repo* pRepo, 
	SG_rev_spec* pRevSpec, 
	const char* szHid, 
	SG_bool* pFound)
{
	struct _sg_rev_spec__contains__baton baton;
	SG_NULLARGCHECK_RETURN(szHid);
	SG_NULLARGCHECK_RETURN(pFound);
	baton.szHid = szHid;
	baton.found = SG_FALSE;
	SG_ERR_CHECK_RETURN(  SG_rev_spec__foreach__repo(pCtx, pRepo, pRevSpec, _sg_rev_spec__contains__cb, &baton)  );
	*pFound = baton.found;
}

//////////////////////////////////////////////////////////////////

#define VHASH_DAGNUM "dagnum"
#define VHASH_REVS "hids"
#define VHASH_TAGS "tags"
#define VHASH_BRANCHES "branches"

/*	A rev spec vhash looks like this:
 *	{
 *		"dagnum" : <dagnum>
 *		"hids" :
 *		{
 *			<full hid> : NULL,
 *			<hid prefix> : NULL,
 *			<revno> : NULL,
 *			...
 *		}
 *		"tags" :
 *		{
 *			<tag name> : NULL,
 *			...
 *		}
 *		"branches" :
 *		{
 *			<branch name> : NULL,
 *			...
 *		}
 *	}
 */

static void _add_to_vhash_throw_nice_or_ignore_dupe(SG_context* pCtx, SG_vhash* pvh, const char* pszKey, SG_bool bIgnoreDupe)
{
	SG_vhash__add__null(pCtx, pvh, pszKey);
	if (bIgnoreDupe)
	{
		SG_ERR_CHECK_RETURN_CURRENT_DISREGARD(SG_ERR_VHASH_DUPLICATEKEY);
	}
	else
	{
		SG_ERR_REPLACE2(SG_ERR_VHASH_DUPLICATEKEY, SG_ERR_INVALID_REV_SPEC, (pCtx, "duplicate spec: %s", pszKey));
		SG_ERR_CHECK_RETURN_CURRENT;
	}
}

/**
 * TODO 2012/07/05 Is it safe to say that the de-dup'ing being
 * TODO            done here is in terms of the raw inputs given?
 * TODO
 * TODO            The various args *AS-GIVEN* to --rev, --tag,
 * TODO            and --branch.  (Which for --rev might be a
 * TODO            hid-prefix, right?)
 * TODO
 * TODO            That is we are NOT de-duping in terms of the 
 * TODO            interpretation of the tag (it's mapping to a
 * TODO            cset) nor the full expansion of the HID....
 * 
 */
void SG_rev_spec__to_vhash(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_bool bIgnoreDupes, SG_vhash** ppvh)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;
	SG_vhash* pvhReturn = NULL;
	SG_vhash* pvhHids = NULL;
	SG_vhash* pvhTags = NULL;
	SG_vhash* pvhBranches = NULL;
	SG_uint32 countAllRevSpecs = 0;
	SG_uint32 i;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NULLARGCHECK_RETURN(ppvh);

	SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhReturn, 4, NULL, NULL)  );

	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, pMe->iDagNum, buf_dagnum, sizeof(buf_dagnum))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhReturn, VHASH_DAGNUM, buf_dagnum)  );

	if (pMe->iCountRevs)
		SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhHids, pMe->iCountRevs, NULL, NULL)  );
	if (pMe->iCountTags)
		SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhTags, pMe->iCountTags, NULL, NULL)  );
	if (pMe->iCountBranches)
		SG_ERR_CHECK(  SG_VHASH__ALLOC__PARAMS(pCtx, &pvhBranches, pMe->iCountBranches, NULL, NULL)  );
	
	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &countAllRevSpecs)  );
	for (i = 0; i < countAllRevSpecs; i++)
	{
		_revspec* p;
		SG_ERR_CHECK(  SG_vector__get(pCtx, pMe->pvecRevSpecs, i, (void**)&p)  );
		switch (p->specType)
		{
		case SPECTYPE_REV:
			SG_ERR_CHECK(  _add_to_vhash_throw_nice_or_ignore_dupe(pCtx, pvhHids, p->pszGiven, bIgnoreDupes)  );
			break;

		case SPECTYPE_TAG:
			if ( pMe->iDagNum != SG_DAGNUM__VERSION_CONTROL )
				SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "tags in a non-version-control query.") );
			SG_ERR_CHECK(  _add_to_vhash_throw_nice_or_ignore_dupe(pCtx, pvhTags, p->pszGiven, bIgnoreDupes)  );
			break;

		case SPECTYPE_BRANCH:
			if ( pMe->iDagNum != SG_DAGNUM__VERSION_CONTROL )
				SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "branches in a non-version-control query.") );
			SG_ERR_CHECK(  _add_to_vhash_throw_nice_or_ignore_dupe(pCtx, pvhBranches, p->pszGiven, bIgnoreDupes)  );
			break;
		default:
			SG_ERR_THROW2(SG_ERR_INVALID_REV_SPEC, (pCtx, "unknown rev spec type"));
		}
	}

	if (pvhHids)
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhReturn, VHASH_REVS, &pvhHids)  );
	if (pvhTags)
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhReturn, VHASH_TAGS, &pvhTags)  );
	if (pvhBranches)
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhReturn, VHASH_BRANCHES, &pvhBranches)  );

	SG_RETURN_AND_NULL(pvhReturn, ppvh);

fail:
	SG_VHASH_NULLFREE(pCtx, pvhReturn);
}

void SG_rev_spec__from_vash(SG_context* pCtx, const SG_vhash* pvh, SG_rev_spec** ppRevSpec)
{
	_sg_rev_spec* pMe = NULL;
	SG_rev_spec* pRevSpec = NULL;
	
	SG_bool bHas;
	SG_vhash* pvhTemp;
	SG_uint32 i, count;
	const char* pszTemp;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(ppRevSpec);

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	pMe = (_sg_rev_spec*)pRevSpec;

	/* A rev spec without a dagnum is invalid, so there's no need to do a __has lookup here first. */
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "dagnum", &pszTemp)  );
	SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, pszTemp, &pMe->iDagNum)  );

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh, VHASH_REVS, &bHas)  );
	if (bHas)
	{
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, VHASH_REVS, &pvhTemp)  );
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhTemp, &count)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhTemp, i, &pszTemp, NULL)  );
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszTemp)  );
		}
	}

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh, VHASH_TAGS, &bHas)  );
	if (bHas)
	{
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, VHASH_TAGS, &pvhTemp)  );
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhTemp, &count)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhTemp, i, &pszTemp, NULL)  );
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTemp)  );
		}
	}

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh, VHASH_BRANCHES, &bHas)  );
	if (bHas)
	{
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, VHASH_BRANCHES, &pvhTemp)  );
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhTemp, &count)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhTemp, i, &pszTemp, NULL)  );
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszTemp)  );
		}
	}

	SG_RETURN_AND_NULL(pRevSpec, ppRevSpec);

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

//////////////////////////////////////////////////////////////////

void SG_rev_spec__merge(SG_context* pCtx, SG_rev_spec* pRevSpecMergeInto, SG_rev_spec** ppRevSpecWillBeFreed)
{
	_sg_rev_spec* pTarget = (_sg_rev_spec*)pRevSpecMergeInto;
	_sg_rev_spec* pMerge;
	SG_uint32 count, i;

	SG_NULLARGCHECK_RETURN(pRevSpecMergeInto);
	if (!ppRevSpecWillBeFreed || !*ppRevSpecWillBeFreed)
		return;
	pMerge = (_sg_rev_spec*)*ppRevSpecWillBeFreed;

	if (pTarget->iDagNum != pMerge->iDagNum)
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "rev specs have differing dagnums"));

	SG_ERR_CHECK(  SG_vector__length(pCtx, pMerge->pvecRevSpecs, &count)  );

	for (i = 0; i < count; i++)
	{
		_revspec* p;
		SG_ERR_CHECK(  SG_vector__get(pCtx, pMerge->pvecRevSpecs, i, (void**)&p)  );
		SG_ERR_CHECK(  _add_rev_spec(pCtx, pTarget, p->specType, p->pszGiven)  );
	}

	SG_REV_SPEC_NULLFREE(pCtx, *ppRevSpecWillBeFreed);

	return;

fail:
	;
}

void SG_rev_spec__split_single_revisions(SG_context* pCtx, SG_rev_spec* pRevSpec, SG_rev_spec ** ppNewRevSpec)
{
	_sg_rev_spec* pMe = (_sg_rev_spec*)pRevSpec;
	SG_uint32 count;
	SG_vector* pvecRevSpecs;
	SG_vector* pvecRevSpecs_to_remove = NULL;
	SG_uint32 count_to_remove;
	SG_uint32 i = 0;
	SG_rev_spec * pNewRevSpec = NULL;

	SG_NULLARGCHECK_RETURN(pRevSpec);
	SG_NULLARGCHECK_RETURN(ppNewRevSpec);

	pvecRevSpecs = pMe->pvecRevSpecs;
	SG_ERR_CHECK(  SG_vector__length(pCtx, pMe->pvecRevSpecs, &count)  );
	if (count)
	{
		for (i = 0; i < count; i++)
		{
			_revspec* p;
			SG_ERR_CHECK(  SG_vector__get(pCtx, pvecRevSpecs, i, (void**)&p)  );
			if (p->specType == SPECTYPE_REV)
			{
				if (pNewRevSpec == NULL)
				{
					SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pNewRevSpec)  );
					SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pvecRevSpecs_to_remove, count)  );
				}
				SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pNewRevSpec, p->pszGiven)  );
				SG_ERR_CHECK(  SG_vector__append(pCtx, pvecRevSpecs_to_remove, p, NULL)  );
			}
			else if (p->specType == SPECTYPE_TAG)
			{
				if (pNewRevSpec == NULL)
				{
					SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pNewRevSpec)  );
					SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pvecRevSpecs_to_remove, count)  );
				}
				SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pNewRevSpec, p->pszGiven)  );
				SG_ERR_CHECK(  SG_vector__append(pCtx, pvecRevSpecs_to_remove, p, NULL)  );
			}
		}

		if (pNewRevSpec) 
		{
			SG_RETURN_AND_NULL(pNewRevSpec, ppNewRevSpec);

			//Now remove the rev specs that were put in the new one.
			SG_ERR_CHECK(  SG_vector__length(pCtx, pvecRevSpecs_to_remove, &count_to_remove)  );
			if (count_to_remove)
			{
				for (i = 0; i < count_to_remove; i++)
				{
					_revspec* p;
					SG_ERR_CHECK(  SG_vector__get(pCtx, pvecRevSpecs_to_remove, i, (void**)&p)  );
					SG_ERR_CHECK(  SG_vector__remove__value(pCtx, pvecRevSpecs, (void *) p, NULL, _free_revspecs_callback)  );
				}
				pMe->iCountRevs = 0;
				pMe->iCountTags = 0;
			}	
		}
	}
	
fail:
	SG_REV_SPEC_NULLFREE(pCtx, pNewRevSpec);
	SG_VECTOR_NULLFREE(pCtx, pvecRevSpecs_to_remove);
}

//////////////////////////////////////////////////////////////////

/**
 * See W8816.
 *
 * This routine is going to expand all of the various --rev, --tag,
 * and --branch options and build a stringarray doing all of the
 * hid-, tag-, or branch-lookup.
 *
 * THIS VERSION DE-DUPLICATES (after the -lookup).
 *
 * This version tries to preserve the input order.
 *
 */
void SG_rev_spec__get_all__repo__dedup(
	SG_context* pCtx, 
	SG_repo* pRepo,
	const SG_rev_spec* pRevSpec, 
	SG_bool bAllowAmbiguousBranches,
	SG_stringarray** ppsaFullHids,
	SG_stringarray** ppsaMissingBranchHeadHids)
{
	SG_stringarray * psaMatched = NULL;
	SG_stringarray * psaMissing = NULL;
	
	/* Don't add a null arg check for ppsaFullHids. It's legit to call us this way,
	 * simply to error check the rev spec. */

	// ppsaMissingBranchHeadHids is also optional.  it will be used
	// to collect any csets (referenced by a --branch) that are not
	// present in the local repo (because the tags and branches dag
	// get swapped in full even when they do a limited 'vv push -r'.
	// (See K2177/K1322)

	SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, bAllowAmbiguousBranches,
											  ((ppsaFullHids) ? &psaMatched : NULL),
											  ((ppsaMissingBranchHeadHids) ? &psaMissing : NULL))  );
	if (ppsaFullHids && psaMatched)
	{
		SG_uint32 nrRawMatched;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaMatched, &nrRawMatched)  );
		if (nrRawMatched > 1)
		{
			SG_stringarray * psaCopy = NULL;
#if 0 && defined(DEBUG)
			SG_ERR_IGNORE(  SG_stringarray_debug__dump_to_console(pCtx, psaMatched, "psaMatched Before DeDup")  );
#endif
			SG_ERR_CHECK(  SG_stringarray__alloc_copy_dedup(pCtx, psaMatched, &psaCopy)  );
			SG_STRINGARRAY_NULLFREE(pCtx, psaMatched);
			psaMatched = psaCopy;
#if 0 && defined(DEBUG)
			SG_ERR_IGNORE(  SG_stringarray_debug__dump_to_console(pCtx, psaMatched, "psaMatched After DeDup")  );
#endif
		}
	}

	if (ppsaMissingBranchHeadHids && psaMissing)
	{
		SG_uint32 nrRawMissing;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaMissing, &nrRawMissing)  );
		if (nrRawMissing > 1)
		{
			SG_stringarray * psaCopy = NULL;
			SG_ERR_CHECK(  SG_stringarray__alloc_copy_dedup(pCtx, psaMissing, &psaCopy)  );
			SG_STRINGARRAY_NULLFREE(pCtx, psaMissing);
			psaMissing = psaCopy;
		}
	}

	SG_RETURN_AND_NULL(psaMatched, ppsaFullHids);
	SG_RETURN_AND_NULL(psaMissing, ppsaMissingBranchHeadHids);

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaMatched);
	SG_STRINGARRAY_NULLFREE(pCtx, psaMissing);
}

END_EXTERN_C;
