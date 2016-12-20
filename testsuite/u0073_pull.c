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
#include "unittests.h"
#include "unittests_pendingtree.h"
#include "unittests_push_pull.h"

//////////////////////////////////////////////////////////////////
// we define a little trick here to prefix all global symbols (type,
// structures, functions) with our test name.  this allows all of
// the tests in the suite to be #include'd into one meta-test (without
// name collisions) when we do a GCOV run.

#define MyMain()				TEST_MAIN(u0073_pull)
#define MyDcl(name)				u0073_pull__##name
#define MyFn(name)				u0073_pull__##name

#define MIN_PULL_ROUNDTRIPS 4

void MyFn(create_file__numbers)(
	SG_context* pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0600, &pFile)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		SG_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;
fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
}

void MyFn(commit_all)(
	SG_context* pCtx,
	const SG_pathname* pPathWorkingDir,
    char ** ppsz_hid_cs
	)
{
	SG_ERR_CHECK_RETURN(  unittests_pendingtree__simple_commit(pCtx, pPathWorkingDir, ppsz_hid_cs)  );
}

void MyFn(test__vc__simple)(SG_context* pCtx)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	char buf_client_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	char buf_server_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_vhash* pvh = NULL;
	SG_repo* pClientRepo = NULL;

	SG_repo* pServerRepo = NULL;
	SG_bool bMatch = SG_FALSE;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,&pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx,pPathTopDir)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_client_repo_name, sizeof(buf_client_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_server_repo_name, sizeof(buf_server_repo_name), 32)  );

	INFOP("test__simple", ("client repo: %s", buf_client_repo_name));
	INFOP("test__simple", ("server repo: %s", buf_server_repo_name));

	/* create the repo */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, buf_server_repo_name)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, buf_server_repo_name, pPathWorkingDir, NULL)  );

	/* open that repo */
	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_server_repo_name, &pServerRepo)  );

	/* create an empty clone to pull into */
    {
        char* psz_hash_method = NULL;
        char* psz_repo_id = NULL;
        char* psz_admin_id = NULL;

        /* We need the ID of the existing repo. */
        VERIFY_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pServerRepo, &psz_hash_method)  );
        VERIFY_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pServerRepo, &psz_repo_id)  );
        VERIFY_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pServerRepo, &psz_admin_id)  );
        VERIFY_ERR_CHECK(  SG_repo__create_empty(pCtx, psz_repo_id, psz_admin_id, psz_hash_method, 
			buf_client_repo_name, SG_REPO_STATUS__NORMAL, &pClientRepo)  );
        SG_NULLFREE(pCtx, psz_hash_method);
        SG_NULLFREE(pCtx, psz_repo_id);
        SG_NULLFREE(pCtx, psz_admin_id);
    }

	/* add stuff to server repo */
	VERIFY_ERR_CHECK(  MyFn(create_file__numbers)(pCtx, pPathWorkingDir, "aaa", 10)  );
	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  MyFn(commit_all)(pCtx, pPathWorkingDir, NULL)  );

	VERIFY_ERR_CHECK(  MyFn(create_file__numbers)(pCtx, pPathWorkingDir, "bbb", 10)  );
	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  MyFn(commit_all)(pCtx, pPathWorkingDir, NULL)  );

	/* verify pre-pull repos are different */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("pre-pull repos differ", !bMatch);

	/* get a client and pull from server repo to empty client repo */
	VERIFY_ERR_CHECK(  SG_pull__all(pCtx, pClientRepo, buf_server_repo_name, NULL, NULL, NULL, NULL)  );

	/* verify post-pull repos are identical */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo DAGs differ", bMatch);
	VERIFY_ERR_CHECK(  SG_sync__compare_repo_blobs(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo blobs differ", bMatch);

	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pClientRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pServerRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );

	/* TODO: verify more stuff? */

	/* Fall through to common cleanup */

fail:
	/* close both repos */
	SG_REPO_NULLFREE(pCtx, pServerRepo);
	SG_REPO_NULLFREE(pCtx, pClientRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);

	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
}

/* The staging code needs to decompress changesets. 
This creates one big enough to exercise that code. */
void MyFn(test__vc__big_changeset)(SG_context* pCtx)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	char buf_client_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	char buf_server_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_vhash* pvh = NULL;
	SG_repo* pClientRepo = NULL;

	SG_pathname* pPathCsDir = NULL;
	SG_uint32 lines;
	SG_uint32 i, j;

	SG_repo* pServerRepo = NULL;
	SG_bool bMatch = SG_FALSE;
	char buf_filename[7];

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,&pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx,pPathTopDir)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_client_repo_name, sizeof(buf_client_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_server_repo_name, sizeof(buf_server_repo_name), 32)  );

	INFOP("test__big_changeset", ("client repo: %s", buf_client_repo_name));
	INFOP("test__big_changeset", ("server repo: %s", buf_server_repo_name));

	/* create the repo */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, buf_server_repo_name)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, buf_server_repo_name, pPathWorkingDir, NULL)  );

	/* open that repo */
	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_server_repo_name, &pServerRepo)  );

	/* create an empty clone to pull into */
    {
        char* psz_hash_method = NULL;
        char* psz_repo_id = NULL;
        char* psz_admin_id = NULL;

        /* We need the ID of the existing repo. */
        VERIFY_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pServerRepo, &psz_hash_method)  );
        VERIFY_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pServerRepo, &psz_repo_id)  );
        VERIFY_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pServerRepo, &psz_admin_id)  );
        VERIFY_ERR_CHECK(  SG_repo__create_empty(pCtx, psz_repo_id, psz_admin_id, psz_hash_method, 
			buf_client_repo_name, SG_REPO_STATUS__NORMAL, &pClientRepo)  );
        SG_NULLFREE(pCtx, psz_hash_method);
        SG_NULLFREE(pCtx, psz_repo_id);
        SG_NULLFREE(pCtx, psz_admin_id);
    }

	/* add stuff to server repo */
	for (i = 0; i < 1; i++) // number of changesets
	{
		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf_filename, sizeof(buf_filename), "%d", i)  );
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathCsDir, pPathWorkingDir, buf_filename)  );
		VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathCsDir)  );

		for (j = 0; j < 5000; j++) // number of files added per changeset
		{
			VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf_filename, sizeof(buf_filename), "%d", j)  );
			lines = j;

			VERIFY_ERR_CHECK(  MyFn(create_file__numbers)(pCtx, pPathCsDir, buf_filename, lines)  );
		}

		SG_PATHNAME_NULLFREE(pCtx, pPathCsDir);

		VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
		VERIFY_ERR_CHECK(  MyFn(commit_all)(pCtx, pPathWorkingDir, NULL)  );
	}


	/* verify pre-pull repos are different */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("pre-pull repos differ", !bMatch);

	/* get a client and pull from the server repo to the empty client repo */
	VERIFY_ERR_CHECK(  SG_pull__all(pCtx, pClientRepo, buf_server_repo_name, NULL, NULL, NULL, NULL)  );

	/* verify post-pull repos are identical */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo DAGs differ", bMatch);
	VERIFY_ERR_CHECK(  SG_sync__compare_repo_blobs(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo blobs differ", bMatch);

	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pClientRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pServerRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );

	/* TODO: verify more stuff? */

	/* Fall through to common cleanup */

fail:
	/* close both repos */
	SG_REPO_NULLFREE(pCtx, pServerRepo);
	SG_REPO_NULLFREE(pCtx, pClientRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathCsDir);

	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
}

void MyFn(test__vc__long_dag)(SG_context* pCtx)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	char buf_client_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	char buf_server_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_vhash* pvh = NULL;
	SG_repo* pClientRepo = NULL;

	SG_pathname* pPathCsDir = NULL;
	SG_uint32 lines;
	SG_uint32 i, j;

	SG_repo* pServerRepo = NULL;
	SG_bool bMatch = SG_FALSE;
	char buf_filename[7];

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,&pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx,pPathTopDir)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_client_repo_name, sizeof(buf_client_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_server_repo_name, sizeof(buf_server_repo_name), 32)  );

	INFOP("test__long_dag", ("client repo: %s", buf_client_repo_name));
	INFOP("test__long_dag", ("server repo: %s", buf_server_repo_name));

	/* create the repo */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, buf_server_repo_name)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, buf_server_repo_name, pPathWorkingDir, NULL)  );

	/* open that repo */
	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_server_repo_name, &pServerRepo)  );

	/* create an empty clone to pull into */
    {
        char* psz_hash_method = NULL;
        char* psz_repo_id = NULL;
        char* psz_admin_id = NULL;

        /* We need the ID of the existing repo. */
        VERIFY_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pServerRepo, &psz_hash_method)  );
        VERIFY_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pServerRepo, &psz_repo_id)  );
        VERIFY_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pServerRepo, &psz_admin_id)  );
        VERIFY_ERR_CHECK(  SG_repo__create_empty(pCtx, psz_repo_id, psz_admin_id, psz_hash_method, 
			buf_client_repo_name, SG_REPO_STATUS__NORMAL, &pClientRepo)  );
        SG_NULLFREE(pCtx, psz_hash_method);
        SG_NULLFREE(pCtx, psz_repo_id);
        SG_NULLFREE(pCtx, psz_admin_id);
    }

	/* add stuff to server repo */
	for (i = 0; i < 20; i++) // number of changesets
	{
		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf_filename, sizeof(buf_filename), "%d", i)  );
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathCsDir, pPathWorkingDir, buf_filename)  );
		VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathCsDir)  );

		for (j = 0; j < 1; j++) // number of files added per changeset
		{
			VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf_filename, sizeof(buf_filename), "%d", j)  );
			lines = (int)(2500.0 * (rand() / (RAND_MAX + 1.0)));

			VERIFY_ERR_CHECK(  MyFn(create_file__numbers)(pCtx, pPathCsDir, buf_filename, lines)  );
		}

		SG_PATHNAME_NULLFREE(pCtx, pPathCsDir);

		VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
		VERIFY_ERR_CHECK(  MyFn(commit_all)(pCtx, pPathWorkingDir, NULL)  );
	}


	/* verify pre-pull repos are different */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("pre-pull repos differ", !bMatch);

	/* get a client and pull from server repo to empty client repo */
	VERIFY_ERR_CHECK(  SG_pull__all(pCtx, pClientRepo, buf_server_repo_name, NULL, NULL, NULL, NULL)  );

	/* verify post-pull repos are identical */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo DAGs differ", bMatch);
	VERIFY_ERR_CHECK(  SG_sync__compare_repo_blobs(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo blobs differ", bMatch);

	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pClientRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pServerRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );

	/* TODO: verify more stuff? */

	/* Fall through to common cleanup */

fail:
	/* close both repos */
	SG_REPO_NULLFREE(pCtx, pServerRepo);
	SG_REPO_NULLFREE(pCtx, pClientRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathCsDir);

	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
}

void MyFn(test__vc__wide_dag)(SG_context* pCtx)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	char buf_client_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	char buf_server_repo_name[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_vhash* pvh = NULL;
	SG_repo* pClientRepo = NULL;
	char* pszidFirstChangeset = NULL;

	SG_pathname* pPathCsDir = NULL;
	SG_uint32 lines;
	SG_uint32 i, j;

	SG_repo* pServerRepo = NULL;
	SG_bool bMatch = SG_FALSE;
	char buf_filename[7];

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,&pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx,pPathTopDir)  );

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_client_repo_name, sizeof(buf_client_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_server_repo_name, sizeof(buf_server_repo_name), 32)  );

	INFOP("test__wide_dag", ("client repo: %s", buf_client_repo_name));
	INFOP("test__wide_dag", ("server repo: %s", buf_server_repo_name));

	/* create the repo */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, buf_server_repo_name)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, buf_server_repo_name, pPathWorkingDir, &pszidFirstChangeset)  );

	/* open that repo */
	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_server_repo_name, &pServerRepo)  );

	/* create an empty clone to pull into */
    {
        char* psz_hash_method = NULL;
        char* psz_repo_id = NULL;
        char* psz_admin_id = NULL;

        /* We need the ID of the existing repo. */
        VERIFY_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pServerRepo, &psz_hash_method)  );
        VERIFY_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pServerRepo, &psz_repo_id)  );
        VERIFY_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pServerRepo, &psz_admin_id)  );
        VERIFY_ERR_CHECK(  SG_repo__create_empty(pCtx, psz_repo_id, psz_admin_id, psz_hash_method, 
			buf_client_repo_name, SG_REPO_STATUS__NORMAL, &pClientRepo)  );
        SG_NULLFREE(pCtx, psz_hash_method);
        SG_NULLFREE(pCtx, psz_repo_id);
        SG_NULLFREE(pCtx, psz_admin_id);
    }

	/* add stuff to server repo */
	for (i = 0; i < 20; i++) // number of changesets
	{
		VERIFY_ERR_CHECK(  _ut_pt__set_baseline(pCtx, pPathWorkingDir, pszidFirstChangeset)  );

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf_filename, sizeof(buf_filename), "%d", i)  );
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathCsDir, pPathWorkingDir, buf_filename)  );
		VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathCsDir)  );

		for (j = 0; j < 1; j++) // number of files added per changeset
		{
			VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf_filename, sizeof(buf_filename), "%d", j)  );
			lines = (int)(2500.0 * (rand() / (RAND_MAX + 1.0)));

			VERIFY_ERR_CHECK(  MyFn(create_file__numbers)(pCtx, pPathCsDir, buf_filename, lines)  );
		}

		SG_PATHNAME_NULLFREE(pCtx, pPathCsDir);

		VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
		VERIFY_ERR_CHECK(  MyFn(commit_all)(pCtx, pPathWorkingDir, NULL)  );
	}

	/* verify pre-pull repos are different */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("pre-pull repos differ", !bMatch);

	/* get a client and pull from server repo to empty client repo */
	VERIFY_ERR_CHECK(  SG_pull__all(pCtx, pClientRepo, buf_server_repo_name, NULL, NULL, NULL, NULL)  );

	/* verify post-pull repos are identical */
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo DAGs differ", bMatch);
	VERIFY_ERR_CHECK(  SG_sync__compare_repo_blobs(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-pull repo blobs differ", bMatch);

	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pClientRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );
	SG_REPO_NULLFREE(pCtx, pClientRepo);

	/* Make another copy with clone */
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_client_repo_name, sizeof(buf_client_repo_name), 32)  );
	VERIFY_ERR_CHECK(  SG_clone__to_local(pCtx, buf_server_repo_name, NULL, NULL, buf_client_repo_name, NULL, NULL, NULL)  );

	/* verify post-clone repos are identical */
	VERIFY_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, buf_client_repo_name, &pClientRepo)  );
	VERIFY_ERR_CHECK(  SG_sync_debug__compare_repo_dags(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-clone repo DAGs differ", bMatch);
	VERIFY_ERR_CHECK(  SG_sync__compare_repo_blobs(pCtx, pClientRepo, pServerRepo, &bMatch)  );
	VERIFY_COND_FAIL("post-clone repo blobs differ", bMatch);

	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pClientRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__verify__dag_consistency(pCtx, pServerRepo,  SG_DAGNUM__VERSION_CONTROL, NULL)  );

	/* TODO: verify more stuff? */

	/* Fall through to common cleanup */

fail:
	/* close both repos */
	SG_REPO_NULLFREE(pCtx, pServerRepo);
	SG_REPO_NULLFREE(pCtx, pClientRepo);

	SG_NULLFREE(pCtx, pszidFirstChangeset);
	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathCsDir);

	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
}

void MyFn(test__gen_hints__abort_and_basic)(SG_context* pCtx)
{
	SG_repo* pRepoSrc = NULL;
	SG_repo* pRepoDest = NULL; 
	char* pszHidCommonLeaf = NULL;
	SG_pull* pPull = NULL;
	SG_vhash* pvhStats = NULL;

	const char* pszRefSrcName = NULL;
	const char* pszRefDestName = NULL;

	VERIFY_ERR_CHECK(  _create_new_repo(pCtx, &pRepoSrc)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, NULL, 5, &pszHidCommonLeaf)  );

	VERIFY_ERR_CHECK(  _clone(pCtx, pRepoSrc, &pRepoDest)  );

	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 5, NULL)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoSrc, "src")  );
	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest")  );

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefSrcName)  );
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoDest, &pszRefDestName)  );
	VERIFY_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, &pPull)  );
	VERIFY_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__TESTING__NOTHING)  );

	VERIFY_ERR_CHECK(  SG_pull__abort(pCtx, &pPull)  );

	VERIFY_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, &pPull)  );
	VERIFY_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__TESTING__NOTHING)  );
	VERIFY_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, &pvhStats)  );
#ifdef DEBUG
	VERIFY_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "pull stats")  );
#endif

	/* Verify stats */
	{
		SG_vhash* pvhRefDags;
		SG_uint32 count;
		VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStats, SG_SYNC_STATUS_KEY__DAGS, &pvhRefDags)  );
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhRefDags, SG_DAGNUM__TESTING__NOTHING_SZ, &count)  );
		// Up to and including the common leaf. If generation hints were ignored, this would be 10.
		VERIFY_COND("dagnodes touched", count == 6); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, &count)  );
		VERIFY_COND("roundtrips", count == MIN_PULL_ROUNDTRIPS);

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_REFERENCED, &count)  );
		VERIFY_COND("blobs referenced", count == 5); // all the new nodes

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_PRESENT, &count)  );
		VERIFY_COND("blobs present", count == 5); // all the new nodes
	}

	/* Common cleanup */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoSrc);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_NULLFREE(pCtx, pszHidCommonLeaf);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
}

void MyFn(test__gen_hints__off_by_one__unknown_leaf_in_dest)(SG_context* pCtx)
{
	SG_repo* pRepoSrc = NULL;
	SG_repo* pRepoDest = NULL; 
	char* pszHidCommonLeaf = NULL;
	SG_pull* pPull = NULL;
	SG_vhash* pvhStats = NULL;

	const char* pszRefSrcName = NULL;
	const char* pszRefDestName = NULL;

	VERIFY_ERR_CHECK(  _create_new_repo(pCtx, &pRepoSrc)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, NULL, 5, &pszHidCommonLeaf)  );

	VERIFY_ERR_CHECK(  _clone(pCtx, pRepoSrc, &pRepoDest)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoSrc, "src after clone")  );
	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after clone")  );

	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoDest, pszHidCommonLeaf, 1, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 5, NULL)  );

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefSrcName)  );
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoDest, &pszRefDestName)  );

	VERIFY_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, &pPull)  );
	VERIFY_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__TESTING__NOTHING)  );
	VERIFY_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, &pvhStats)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after pull")  );

#ifdef DEBUG
	VERIFY_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "pull stats")  );
#endif

	/* Verify stats */
	{
		SG_vhash* pvhRefDags;
		SG_uint32 count;
		VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStats, SG_SYNC_STATUS_KEY__DAGS, &pvhRefDags)  );
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhRefDags, SG_DAGNUM__TESTING__NOTHING_SZ, &count)  );
		// The generation hint failed because of the unknown leaf. We had to walk all the way to root.
		VERIFY_COND("dagnodes touched", count == 10); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, &count)  );
		VERIFY_COND("roundtrips", count == MIN_PULL_ROUNDTRIPS + 1);

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_REFERENCED, &count)  );
		VERIFY_COND("blobs referenced", count == 5); // all the new nodes

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_PRESENT, &count)  );
		VERIFY_COND("blobs present", count == 5); // all the new nodes
	}

	/* Common cleanup */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoSrc);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_NULLFREE(pCtx, pszHidCommonLeaf);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
}

void MyFn(test__gen_hints__negative__unknown_leaf_in_dest)(SG_context* pCtx)
{
	SG_repo* pRepoSrc = NULL;
	SG_repo* pRepoDest = NULL; 
	char* pszHidCommonLeaf = NULL;
	SG_pull* pPull = NULL;
	SG_vhash* pvhStats = NULL;

	const char* pszRefSrcName = NULL;
	const char* pszRefDestName = NULL;

	VERIFY_ERR_CHECK(  _create_new_repo(pCtx, &pRepoSrc)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, NULL, 5, &pszHidCommonLeaf)  );

	VERIFY_ERR_CHECK(  _clone(pCtx, pRepoSrc, &pRepoDest)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoSrc, "src after clone")  );
	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after clone")  );

	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoDest, pszHidCommonLeaf, 6, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 5, NULL)  );

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefSrcName)  );
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoDest, &pszRefDestName)  );

	VERIFY_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, &pPull)  );
	VERIFY_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__TESTING__NOTHING)  );
	VERIFY_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, &pvhStats)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after pull")  );

#ifdef DEBUG
	VERIFY_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "pull stats")  );
#endif

	/* Verify stats */
	{
		SG_vhash* pvhRefDags;
		SG_uint32 count;
		VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStats, SG_SYNC_STATUS_KEY__DAGS, &pvhRefDags)  );
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhRefDags, SG_DAGNUM__TESTING__NOTHING_SZ, &count)  );
		// The generation hint failed because of the unknown leaf. We had to walk all the way to root.
		VERIFY_COND("dagnodes touched", count == 10); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, &count)  );
		// The gen hint on first roundtrip was known to bad because it was negative, so the fallback 
		// goes all the way to root and we get it done in one shot.
		VERIFY_COND("roundtrips", count == MIN_PULL_ROUNDTRIPS); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_REFERENCED, &count)  );
		VERIFY_COND("blobs referenced", count == 5); // all the new nodes

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_PRESENT, &count)  );
		VERIFY_COND("blobs present", count == 5); // all the new nodes
	}

	/* Common cleanup */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoSrc);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_NULLFREE(pCtx, pszHidCommonLeaf);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
}

void MyFn(test__gen_hints__perfectly_even_leaves)(SG_context* pCtx)
{
	SG_repo* pRepoSrc = NULL;
	SG_repo* pRepoDest = NULL; 
	char* pszHidCommonLeaf = NULL;
	SG_pull* pPull = NULL;
	SG_vhash* pvhStats = NULL;

	const char* pszRefSrcName = NULL;
	const char* pszRefDestName = NULL;

	VERIFY_ERR_CHECK(  _create_new_repo(pCtx, &pRepoSrc)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, NULL, 5, &pszHidCommonLeaf)  );

	VERIFY_ERR_CHECK(  _clone(pCtx, pRepoSrc, &pRepoDest)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoSrc, "src after clone")  );
	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after clone")  );

	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 7, NULL)  );

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefSrcName)  );
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoDest, &pszRefDestName)  );

	VERIFY_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, &pPull)  );
	VERIFY_ERR_CHECK(  SG_pull__add__dagnum(pCtx, pPull, SG_DAGNUM__TESTING__NOTHING)  );
	VERIFY_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, &pvhStats)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after pull")  );

#ifdef DEBUG
	VERIFY_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "pull stats")  );
#endif

	/* Verify stats */
	{
		SG_vhash* pvhRefDags;
		SG_uint32 count;
		VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStats, SG_SYNC_STATUS_KEY__DAGS, &pvhRefDags)  );
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhRefDags, SG_DAGNUM__TESTING__NOTHING_SZ, &count)  );
		// Contrived perfect generation hint should work perfectly. 
		// This should be all the new nodes plus the one common leaf in the fringe.
		VERIFY_COND("dagnodes touched", count == 50); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, &count)  );
		// Perfect gen hint should get this done in the minimum possible roundtrips.
		VERIFY_COND("roundtrips", count == MIN_PULL_ROUNDTRIPS); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_REFERENCED, &count)  );
		VERIFY_COND("blobs referenced", count == 49); // all the new nodes

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_PRESENT, &count)  );
		VERIFY_COND("blobs present", count == 49); // all the new nodes
	}

	/* Common cleanup */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoSrc);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_NULLFREE(pCtx, pszHidCommonLeaf);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
}

void MyFn(test__gen_hints__uneven_leaves_and_off_by_two_gen_hint)(SG_context* pCtx)
{
	SG_repo* pRepoSrc = NULL;
	SG_repo* pRepoDest = NULL; 
	char* pszHidCommonLeaf = NULL;
	SG_pull* pPull = NULL;
	SG_vhash* pvhStats = NULL;
	SG_rev_spec* pRevSpec = NULL;

	const char* pszRefSrcName = NULL;
	const char* pszRefDestName = NULL;

	VERIFY_ERR_CHECK(  _create_new_repo(pCtx, &pRepoSrc)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, NULL, 1, &pszHidCommonLeaf)  );

	VERIFY_ERR_CHECK(  _clone(pCtx, pRepoSrc, &pRepoDest)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoSrc, "src after clone")  );
	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after clone")  );

	// Add one leaf in dest, unknown to src, to throw off the gen hint and try to fool
	// it into missing the 1 new leaf in src.
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoDest, pszHidCommonLeaf, 1, NULL)  );

	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 12, NULL)  );
	VERIFY_ERR_CHECK(  _add_line_to_dag(pCtx, pRepoSrc, pszHidCommonLeaf, 2, NULL)  );

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &pszRefSrcName)  );
	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoDest, &pszRefDestName)  );

	VERIFY_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszRefSrcName, NULL, NULL, &pPull)  );
	
	/* Using basic pull__add here to give it some test coverage. */
	VERIFY_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	VERIFY_ERR_CHECK(  SG_rev_spec__set_dagnum(pCtx, pRevSpec, SG_DAGNUM__TESTING__NOTHING)  );
	VERIFY_ERR_CHECK(  SG_pull__add(pCtx, pPull, &pRevSpec)  );
	
	VERIFY_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, NULL, &pvhStats)  );

	VERIFY_ERR_CHECK(  _dump_entire_repo_as_dagfrag(pCtx, pRepoDest, "dest after pull")  );

#ifdef DEBUG
	VERIFY_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhStats, "pull stats")  );
#endif

	/* Verify stats */
	{
		SG_vhash* pvhRefDags;
		SG_uint32 count;
		VERIFY_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhStats, SG_SYNC_STATUS_KEY__DAGS, &pvhRefDags)  );
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhRefDags, SG_DAGNUM__TESTING__NOTHING_SZ, &count)  );
		// Slightly off gen hint means we'll walk all the way to root.
		// This should be all the nodes in the src repo.
		VERIFY_COND("dagnodes touched", count == 15); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__ROUNDTRIPS, &count)  );
		// Slightly off gen hint means one extra roundtrip.
		VERIFY_COND("roundtrips", count == MIN_PULL_ROUNDTRIPS + 1); 

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_REFERENCED, &count)  );
		VERIFY_COND("blobs referenced", count == 14); // all the new nodes

		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx, pvhStats, SG_SYNC_STATUS_KEY__BLOBS_PRESENT, &count)  );
		VERIFY_COND("blobs present", count == 14); // all the new nodes
	}

	/* Common cleanup */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoSrc);
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_NULLFREE(pCtx, pszHidCommonLeaf);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
}

MyMain()
{
	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  MyFn(test__gen_hints__abort_and_basic)(pCtx)  );
	VERIFY_ERR_CHECK(  MyFn(test__gen_hints__off_by_one__unknown_leaf_in_dest)(pCtx)  );
	VERIFY_ERR_CHECK(  MyFn(test__gen_hints__negative__unknown_leaf_in_dest)(pCtx)  );
	VERIFY_ERR_CHECK(  MyFn(test__gen_hints__perfectly_even_leaves)(pCtx)  );
	VERIFY_ERR_CHECK(  MyFn(test__gen_hints__uneven_leaves_and_off_by_two_gen_hint)(pCtx)  );

	VERIFY_ERR_CHECK(  MyFn(test__vc__simple)(pCtx)  );

#ifdef SG_NIGHTLY_BUILD
	VERIFY_ERR_CHECK(  MyFn(test__vc__long_dag)(pCtx)  );
	VERIFY_ERR_CHECK(  MyFn(test__vc__wide_dag)(pCtx)  );
#endif

#ifdef SG_LONGTESTS
	VERIFY_ERR_CHECK(  MyFn(test__vc__big_changeset)(pCtx)  );
#endif

	// Fall through to common cleanup.
fail:

	TEMPLATE_MAIN_END;
}

#undef MyMain
#undef MyDcl
#undef MyFn
