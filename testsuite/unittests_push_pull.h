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
 * @file unittests_push_pull.h
 *
 * @details Common utility routines used by the push and pull tests.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_UNITTESTS_PUSH_PULL_H
#define H_UNITTESTS_PUSH_PULL_H

#define WHO "testing@sourcegear.com"

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void _commit_dagnode(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_stringarray* psaParentHids,
	SG_dagnode** ppdn)
{
	SG_committing* pCommit = NULL;
	char bufTidRandom[SG_TID_MAX_BUFFER_LENGTH];
	SG_audit pq;
	char* pszHid = NULL;

	SG_STATIC_ASSERT(sizeof(SG_byte) == sizeof(char));

	VERIFY_ERR_CHECK(  SG_audit__init(pCtx, &pq, pRepo, SG_AUDIT__WHEN__NOW, WHO)  );
	VERIFY_ERR_CHECK(  SG_committing__alloc(pCtx, &pCommit, pRepo, SG_DAGNUM__TESTING__NOTHING, 
		&pq, SG_CSET_VERSION__CURRENT)  );

	if (psaParentHids)
	{
		SG_uint32 i, count;
		VERIFY_ERR_CHECK(  SG_stringarray__count(pCtx, psaParentHids, &count)  );
		for (i = 0; i < count; i++)
		{
			const char* pszRefHid;
			VERIFY_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaParentHids, i, &pszRefHid)  );
			VERIFY_ERR_CHECK(  SG_committing__add_parent(pCtx, pCommit, pszRefHid)  );
		}
	}

	VERIFY_ERR_CHECK(  SG_tid__generate(pCtx, bufTidRandom, sizeof(bufTidRandom))  );
	SG_ERR_CHECK(  SG_committing__add_bytes__buflen(pCtx, pCommit,
		(const SG_byte*)bufTidRandom, sizeof(bufTidRandom), SG_FALSE, &pszHid)  );
	SG_ERR_CHECK(  SG_committing__tree__set_root(pCtx, pCommit, pszHid)  );
	SG_NULLFREE(pCtx, pszHid);

	VERIFY_ERR_CHECK(  SG_committing__end(pCtx, pCommit, NULL, ppdn)  );

	return;

fail:
	SG_NULLFREE(pCtx, pszHid);
	if (pCommit)
		SG_ERR_IGNORE(  SG_committing__abort(pCtx, pCommit)  );
}

void _add_line_to_dag(
	SG_context * pCtx,
	SG_repo * pRepo,
	const char* pszHidParent,
	SG_uint32 cGenerations,
	char** ppszHidLeaf)
{
	SG_uint32 iGen;
	SG_stringarray* psaParents = NULL;
	SG_dagnode* pdn = NULL;

	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaParents, 1)  );
	if (pszHidParent)
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaParents, pszHidParent)  );

	for (iGen = 0; iGen < cGenerations; iGen++)
	{
		const char* pszRefHid;

		VERIFY_ERR_CHECK(  _commit_dagnode(pCtx, pRepo, psaParents, &pdn)  );

		SG_STRINGARRAY_NULLFREE(pCtx, psaParents);
		VERIFY_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaParents, 1)  );
		VERIFY_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pdn, &pszRefHid)  );
		VERIFY_ERR_CHECK(  SG_stringarray__add(pCtx, psaParents, pszRefHid)  );
		if (ppszHidLeaf && (cGenerations == iGen+1))
			VERIFY_ERR_CHECK(  SG_STRDUP(pCtx, pszRefHid, ppszHidLeaf)  );
		SG_DAGNODE_NULLFREE(pCtx, pdn);
	}

	SG_STRINGARRAY_NULLFREE(pCtx, psaParents);

	return;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaParents);
	SG_DAGNODE_NULLFREE(pCtx, pdn);
}

void _create_new_repo(SG_context* pCtx, SG_repo** ppRepo)
{
	char buf_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	SG_repo* pRepo = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_repo_name, sizeof(buf_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_vv2__init_new_repo(pCtx, buf_repo_name, NULL, NULL, NULL, SG_TRUE, NULL, SG_FALSE, NULL, NULL)  );

	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_repo_name, &pRepo)  );
	SG_ERR_CHECK_RETURN(  SG_user__create(pCtx, pRepo, WHO, NULL)  );
	SG_ERR_CHECK_RETURN(  SG_user__set_user__repo(pCtx, pRepo, WHO)  );

	*ppRepo = pRepo;
	return;

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void _clone(
	SG_context* pCtx,
	SG_repo* pRepoSrc,
	SG_repo** ppRepoDest)
{
	const char* pszRefName;
	char buf_new_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	SG_repo* pRepoDest = NULL;

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefName)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_new_repo_name, sizeof(buf_new_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_clone__to_local(pCtx, pszRefName, NULL, NULL, buf_new_repo_name, NULL, NULL, NULL)  );

	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_new_repo_name, &pRepoDest)  );
	SG_ERR_CHECK_RETURN(  SG_user__set_user__repo(pCtx, pRepoDest, WHO)  );

	*ppRepoDest = pRepoDest;
	return;

fail:
	SG_REPO_NULLFREE(pCtx, pRepoDest);
}

void _dump_entire_repo_as_dagfrag(SG_context* pCtx, SG_repo* pRepo, const char* pszLabel)
{
#ifdef DEBUG
	SG_dagfrag* pFrag = NULL;
	SG_rbtree* prb = NULL;

	VERIFY_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, SG_DAGNUM__TESTING__NOTHING, &prb)  );
	VERIFY_ERR_CHECK(  SG_dagfrag__alloc_transient(pCtx, SG_DAGNUM__TESTING__NOTHING, &pFrag)  );
	VERIFY_ERR_CHECK(  SG_dagfrag__load_from_repo__multi(pCtx, pFrag, pRepo, prb, SG_INT32_MAX)  );
	VERIFY_ERR_CHECK(  SG_dagfrag_debug__dump__console(pCtx, pFrag, pszLabel, 0, SG_CS_STDERR)  );
	VERIFY_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDERR, "--\n")  );
	/* Common cleanup */
fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);

#else
	SG_UNUSED(pCtx);
	SG_UNUSED(pRepo);
	SG_UNUSED(pszLabel);
#endif

}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_UNITTESTS_PUSH_PULL_H
