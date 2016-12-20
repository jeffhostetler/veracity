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
 * @file u0086_sync_hardwired_templates.c
 *
 * @details Test to ensure that push, pull, and clone do not transfer DAGs with missing hardwired
 *          templates.
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_push_pull.h"

//////////////////////////////////////////////////////////////////

#define MyMain()				TEST_MAIN(u0086_sync_hardwired_templates)
#define MyDcl(name)				u0086_sync_hardwired_templates__##name
#define MyFn(name)				u0086_sync_hardwired_templates__##name

#define SG_DAGNUM__TESTING__HARDWIRED   \
	SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
	SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__TESTING, \
	SG_DAGNUM__SET_DAGID(3, \
	SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
	SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
	((SG_uint64) 0))))))

//////////////////////////////////////////////////////////////////

static void _add_dag(SG_context* pCtx, SG_repo* pRepo, SG_uint64 dagnum)
{
	SG_audit audit;
	SG_committing* pCommitting = NULL;

	VERIFY_ERR_CHECK(  SG_audit__init(pCtx, &audit, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
	VERIFY_ERR_CHECK(  SG_committing__alloc(pCtx, &pCommitting, pRepo, dagnum, &audit, SG_CSET_VERSION_1)  );
	VERIFY_ERR_CHECK(  SG_committing__end(pCtx, pCommitting, NULL, NULL)  );
	pCommitting = NULL;

	/* fall through */
fail:
;
}

MyMain()
{
	SG_repo* pRepoSrc = NULL;
	SG_repo* pRepoDest = NULL;
	SG_rbtree* prbSrcDagnums = NULL;
	SG_rbtree* prbDestDagnums = NULL;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];
	SG_bool bFound = SG_FALSE;
	const char* pszRefDestName;
	const char* pszRefSrcName;

	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, SG_DAGNUM__TESTING__HARDWIRED, buf_dagnum, sizeof(buf_dagnum))  );

	VERIFY_ERR_CHECK(  _create_new_repo(pCtx, &pRepoSrc)  );
	VERIFY_ERR_CHECK(  _add_dag(pCtx, pRepoSrc, SG_DAGNUM__TESTING__HARDWIRED)  );
	VERIFY_ERR_CHECK(  SG_repo__list_dags__rbtree(pCtx, pRepoSrc, &prbSrcDagnums)  );
	VERIFY_ERR_CHECK(  SG_rbtree__find(pCtx, prbSrcDagnums, buf_dagnum, &bFound, NULL)  );
	VERIFY_COND_FAIL("test DAG should be present in source repo", bFound);

	/* clone */
	VERIFY_ERR_CHECK(  _clone(pCtx, pRepoSrc, &pRepoDest)  );
	VERIFY_ERR_CHECK(  SG_repo__list_dags__rbtree(pCtx, pRepoDest, &prbDestDagnums)  );
	VERIFY_ERR_CHECK(  SG_rbtree__find(pCtx, prbDestDagnums, buf_dagnum, &bFound, NULL)  );
	VERIFY_COND_FAIL("test DAG should not be transferred by clone", !bFound);
	SG_RBTREE_NULLFREE(pCtx, prbDestDagnums);

	/* push */
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoDest, &pszRefDestName)  );
	VERIFY_ERR_CHECK(  SG_push__all(pCtx, pRepoSrc, pszRefDestName, NULL, NULL, SG_FALSE, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__list_dags__rbtree(pCtx, pRepoDest, &prbDestDagnums)  );
	VERIFY_ERR_CHECK(  SG_rbtree__find(pCtx, prbDestDagnums, buf_dagnum, &bFound, NULL)  );
	VERIFY_COND_FAIL("test DAG should not be transferred by push", !bFound);
	SG_RBTREE_NULLFREE(pCtx, prbDestDagnums);

	/* pull */
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefSrcName)  );
	VERIFY_ERR_CHECK(  SG_pull__all(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__list_dags__rbtree(pCtx, pRepoDest, &prbDestDagnums)  );
	VERIFY_ERR_CHECK(  SG_rbtree__find(pCtx, prbDestDagnums, buf_dagnum, &bFound, NULL)  );
	VERIFY_COND_FAIL("test DAG should not be transferred by pull", !bFound);
	SG_RBTREE_NULLFREE(pCtx, prbDestDagnums);

fail:
	SG_REPO_NULLFREE(pCtx, pRepoSrc);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_RBTREE_NULLFREE(pCtx, prbSrcDagnums);
	SG_RBTREE_NULLFREE(pCtx, prbDestDagnums);

	TEMPLATE_MAIN_END;
}

#undef MyMain
#undef MyDcl
#undef MyFn
