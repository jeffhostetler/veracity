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
 * @file sg_vv2__tag.c
 *
 * @details Routines to deal with 'vv tag' and 'vv tags' commands.
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

/**
 * Return a list of all TAGS and their associated CSET.
 *
 */
void SG_vv2__tags(SG_context * pCtx,
				  const char * pszRepoName,
				  SG_bool bGetRevno,
				  SG_varray ** ppva_results)
{
	SG_repo * pRepo = NULL;
	SG_varray * pva_results = NULL;
	SG_rbtree * prbtree = NULL;
	SG_rbtree_iterator * pIter = NULL;
	const char * psz_tag = NULL;
	const char * psz_csid = NULL;
	SG_vhash * pvh_current = NULL;	// we don't own this
	SG_dagnode * pDagnode = NULL;
	SG_bool b = SG_FALSE;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional
	SG_NULLARGCHECK_RETURN( ppva_results );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );

	SG_ERR_CHECK(  SG_vc_tags__list(pCtx, pRepo, &prbtree)  );
	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_results)  );

	if (prbtree)
	{
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, prbtree, &b, &psz_tag, (void**)&psz_csid)  );

		while (b)
		{
			SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva_results, &pvh_current)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_current, "tag", psz_tag) );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_current, "csid", psz_csid) );

			if (bGetRevno)
			{
				SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, psz_csid, &pDagnode);
				if (SG_CONTEXT__HAS_ERR(pCtx))
				{
					if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
					{
						// W5479 -- They requested that we get the revno for each cset, but
						//       -- the referenced cset is not present in the local repo.
						//       -- Someone probably did a 'vv push -r' (which limits the
						//       -- push to needed csets, but still pushes the entire tags dag.
						//
						// So don't record a "revno" for this item (since revno's refer to
						// the index of the cset in the local repo and we don't have one).
						// The caller check for this and maybe report "missing" if it wants.
						SG_context__err_reset(pCtx);
					}
					else
					{
						SG_ERR_RETHROW;
					}
				}
				else
				{
					SG_uint32 revno;

					SG_ERR_CHECK(  SG_dagnode__get_revno(pCtx, pDagnode, &revno)  );
					SG_DAGNODE_NULLFREE(pCtx, pDagnode);

					SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_current, "revno", (SG_int64)revno)  );
				}
			}
		
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &b, &psz_tag, (void**)&psz_csid)  );
		}
	}

	*ppva_results = pva_results;
	pva_results = NULL;

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
    SG_VARRAY_NULLFREE(pCtx, pva_results);
	SG_RBTREE_NULLFREE(pCtx, prbtree);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
}

//////////////////////////////////////////////////////////////////

void SG_vv2__tag__add(SG_context * pCtx,
					  const char * pszRepoName,
					  const SG_rev_spec * pRevSpec,
					  const char * pszTagName,
					  SG_bool bForceMove)
{
	SG_repo * pRepo = NULL;
	char * pszHidCSet = NULL;
	SG_audit q;
	SG_uint32 countRevSpecs = 0;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to SINGLE parent of WD if present)
	SG_NONEMPTYCHECK_RETURN( pszTagName );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// because a TAG is unique it can only be assigned to a single CSET.

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, SG_FALSE, &pszHidCSet, NULL)  );
	}
	else if (bRepoNamed)
	{
		// normally the (count==0) case means to use
		// the parents of the current WD, but we have to disallow it here
		// because when they give us a named repo, we do not want to assume
		// anything about the current WD (if present).

		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Must specify which changeset to tag in repo '%s'.", pszRepoName)  );
	}
	else
	{
		SG_bool bHasMerge = SG_FALSE;

		SG_ERR_CHECK(  SG_wc__get_wc_baseline(pCtx, NULL, &pszHidCSet, &bHasMerge)  );
		if (bHasMerge)
			SG_ERR_THROW2(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,
							(pCtx, "The working directory has multiple parents and a tag cannot be appled to multiple changesets."));
	}

	SG_ERR_CHECK(  SG_vc_tags__add(pCtx, pRepo, pszHidCSet, pszTagName, &q, bForceMove)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, pszHidCSet);
}

//////////////////////////////////////////////////////////////////

void SG_vv2__tag__move(SG_context * pCtx,
					   const char * pszRepoName,
					   const SG_rev_spec * pRevSpec,
					   const char * pszTagName)
{
	SG_repo * pRepo = NULL;
	char * pszHidCSet = NULL;
	SG_audit q;
	SG_uint32 countRevSpecs = 0;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to SINGLE parent of WD if present)
	SG_NONEMPTYCHECK_RETURN( pszTagName );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// because a TAG is unique it can only be assigned to a single CSET.

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, SG_FALSE, &pszHidCSet, NULL)  );
	}
	else if (bRepoNamed)
	{
		// normally the (count==0) case means to use
		// the parents of the current WD, but we have to disallow it here
		// because when they give us a named repo, we do not want to assume
		// anything about the current WD (if present).

		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Must specify which changeset to tag in repo '%s'.", pszRepoName)  );
	}
	else
	{
		SG_bool bHasMerge = SG_FALSE;

		SG_ERR_CHECK(  SG_wc__get_wc_baseline(pCtx, NULL, &pszHidCSet, &bHasMerge)  );
		if (bHasMerge)
			SG_ERR_THROW2(  SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE,
							(pCtx, "The working directory has multiple parents and a tag cannot be appled to multiple changesets."));
	}

	SG_ERR_CHECK(  SG_vc_tags__move(pCtx, pRepo, pszHidCSet, pszTagName, &q)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, pszHidCSet);
}

//////////////////////////////////////////////////////////////////

void SG_vv2__tag__remove(SG_context * pCtx,
						 const char * pszRepoName,
						 const char * pszTagName)
{
	SG_repo * pRepo = NULL;
	SG_audit q;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	SG_NONEMPTYCHECK_RETURN( pszTagName );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// Unlike STAMPS, a TAG is unique and therefore we do
	// not require a revspec to tell us which instance.
	//
	// We also don't need to return a vector of what we did
	// since we can either succeed or throw.

	SG_ERR_CHECK(  SG_vc_tags__remove(pCtx, pRepo, &q, 1, &pszTagName)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

