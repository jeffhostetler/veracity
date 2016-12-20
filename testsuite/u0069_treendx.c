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
 * @file u0069_treendx.c
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_pendingtree.h"

//////////////////////////////////////////////////////////////////

void u0069_treendx__commit_all(SG_context * pCtx,
	const SG_pathname* pPathWorkingDir,
	char** ppszChangesetGID
	)
{
	SG_ERR_CHECK_RETURN(  unittests_pendingtree__simple_commit(pCtx, pPathWorkingDir, ppszChangesetGID)  );
}

void u0069_treendx__append_to_file__numbers(SG_context * pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;
	SG_uint64 len = 0;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPathFile, &len, NULL)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_OPEN_EXISTING | SG_FILE_RDWR, 0600, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__seek(pCtx, pFile, len)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	return;
}

void u0069_treendx__create_file__numbers(SG_context * pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0600, &pFile)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	return;
}

void u0069_treendx__move(SG_context * pCtx,
		const SG_pathname* pPathWorkingDir,
		const char* relPath1,
		const char* relPath2
		)
{
	SG_pathname * pPath_old_cwd = NULL;

	// this test was originally written using the PendingTree code
	// which did not assume that the CWD was in the WD.  With the
	// conversion to the WC code, we do assume that, so we CD down
	// for the duration of the MOVE.

	SG_ERR_CHECK(  SG_pathname__alloc(pCtx, &pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathWorkingDir)  );

	SG_ERR_CHECK(  SG_wc__move(pCtx, NULL, relPath1, relPath2,
							   SG_FALSE, // bNoAllowAfterTheFact
							   SG_FALSE, // bTest
							   NULL // journal
					   )  );

	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath_old_cwd)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_old_cwd);
}

void u0069_treendx__delete(SG_context * pCtx,
		const SG_pathname* pPathWorkingDir,
		const char* relPath1
		)
{
	SG_pathname * pPath_old_cwd = NULL;

	// this test was originally written using the PendingTree code
	// which did not assume that the CWD was in the WD.  With the
	// conversion to the WC code, we do assume that, so we CD down
	// for the duration of the MOVE.

	SG_ERR_CHECK(  SG_pathname__alloc(pCtx, &pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_pathname__set__from_cwd(pCtx, pPath_old_cwd)  );
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathWorkingDir)  );

	// TODO 2011/08/08 I just added a bNonRecursive option to SG_wc__remove().
	// TODO            I don't know what the test is doing or if it matters
	// TODO            how we set it.  So I'm setting it here to behave the
	// TODO            same as it did before.

	SG_ERR_CHECK(  SG_wc__remove(pCtx, NULL, relPath1,
								 SG_FALSE, // bNonRecursive
								 SG_FALSE, // bForce
								 SG_FALSE, // bNoBackups
								 SG_FALSE, // bKeep
								 SG_FALSE, // bTest
								 NULL // journal
					   )  );

	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPath_old_cwd)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_old_cwd);
}

//This will return the treenodeentry of an item in a particular changeset.
void u0069_treendx__get_treenode_in_changeset(SG_context * pCtx, SG_repo * pRepo, const char* gid, const char * pszChangesetGID, SG_treenode_entry ** ppTreeNode)
{
	VERIFY_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, gid, pszChangesetGID, NULL, ppTreeNode)  );
fail:
	return;
}


void u0069_treendx_test__empty_repo(SG_context * pCtx, SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathFile = NULL;
	char* cset_id = NULL;
	SG_repo* pRepo = NULL;
	char* pszHidTreeNode = NULL;
	char* pszTreeNodeEntryGID = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	SG_treenode_entry * pTreeNodeEntry = NULL;
	SG_treenode_entry * treenode = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* First create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, bufName, pPathWorkingDir, &cset_id)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, bufName, &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, cset_id, &pszHidTreeNode)  );

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidTreeNode, &pTreenodeRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, "@", &pszTreeNodeEntryGID, &pTreeNodeEntry)  );
	VERIFY_COND("pszTreeNodeEntryGID should not be NULL", (NULL != pszTreeNodeEntryGID));
	VERIFY_COND("pTreeNodeEntry should not be NULL", (NULL != pTreeNodeEntry));

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id, &treenode));
	VERIFY_COND("Couldn't find @ in the initial changeset.", (treenode != NULL));

	SG_NULLFREE(pCtx, pszTreeNodeEntryGID);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTreeNodeEntry);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);


	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_NULLFREE(pCtx, cset_id);
	//SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_REPO_NULLFREE(pCtx, pRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);

	return;

fail:
	return;
}

void u0069_treendx_test__added_only(SG_context * pCtx, SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathFile = NULL;
	char* cset_id_initial_changeset = NULL;
	char* cset_id_after_add = NULL;
	SG_repo* pRepo = NULL;
	char* pszHidTreeNode = NULL;
	char* pszTreeNodeEntryGID = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	SG_treenode_entry * pTreeNodeEntry = NULL;
	SG_treenode_entry * treenode = NULL;
	SG_pathname* pPathDir1 = NULL;
	SG_pathname* pPathDir2 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* First create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, bufName, pPathWorkingDir, &cset_id_initial_changeset)  );

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir1, pPathWorkingDir, "d1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir2, pPathDir1, "d2")  );

	/* add stuff */
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir2)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir1, "a.txt", 20)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir2, "b.txt", 20)  );

	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_add)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, bufName, &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, cset_id_after_add, &pszHidTreeNode)  );

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidTreeNode, &pTreenodeRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, "@/d1/a.txt", &pszTreeNodeEntryGID, &pTreeNodeEntry)  );
	VERIFY_COND("pszTreeNodeEntryGID should not be NULL", (NULL != pszTreeNodeEntryGID));
	VERIFY_COND("pTreeNodeEntry should not be NULL", (NULL != pTreeNodeEntry));

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_add, &treenode));
	VERIFY_COND("Couldn't find @/d1/a.txt in the second changeset.", (treenode != NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_initial_changeset, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the initial changeset.", (treenode == NULL));

	SG_NULLFREE(pCtx, pszTreeNodeEntryGID);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTreeNodeEntry);

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);


	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_NULLFREE(pCtx, cset_id_after_add);
	SG_NULLFREE(pCtx, cset_id_initial_changeset);
	//SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_REPO_NULLFREE(pCtx, pRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir2);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir1);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);

	return;

fail:
	return;
}

void u0069_treendx_test__moved_only(SG_context * pCtx, SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathFile = NULL;
	char* cset_id_initial_changeset = NULL;
	char* cset_id_after_add = NULL;
	char* cset_id_after_move = NULL;
	SG_repo* pRepo = NULL;
	char* pszHidTreeNode = NULL;
	char* pszTreeNodeEntryGID = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	SG_treenode_entry * pTreeNodeEntry = NULL;
	SG_treenode_entry * treenode = NULL;
	SG_pathname* pPathDir1 = NULL;
	SG_pathname* pPathDir2 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* First create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, bufName, pPathWorkingDir, &cset_id_initial_changeset)  );

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir1, pPathWorkingDir, "d1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir2, pPathWorkingDir, "d2")  );

	/* add stuff */
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir2)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir1, "a.txt", 20)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir2, "b.txt", 20)  );

	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_add)  );

	VERIFY_ERR_CHECK(  u0069_treendx__move(pCtx, pPathWorkingDir, "d1", "d2")  );

	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_move)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, bufName, &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, cset_id_after_move, &pszHidTreeNode)  );

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidTreeNode, &pTreenodeRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, "@/d2/d1/a.txt", &pszTreeNodeEntryGID, &pTreeNodeEntry)  );
	VERIFY_COND("pszTreeNodeEntryGID should not be NULL", (NULL != pszTreeNodeEntryGID));
	VERIFY_COND("pTreeNodeEntry should not be NULL", (NULL != pTreeNodeEntry));

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_move, &treenode));
	VERIFY_COND("Couldn't find @/d2/d1/a.txt in the third changeset.", (treenode != NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_add, &treenode));
	VERIFY_COND("Couldn't find @/d1/a.txt in the second changeset.", (treenode != NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_initial_changeset, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the initial changeset.", (treenode == NULL));

	SG_NULLFREE(pCtx, pszTreeNodeEntryGID);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTreeNodeEntry);

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);


	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_NULLFREE(pCtx, cset_id_after_add);
	SG_NULLFREE(pCtx, cset_id_after_move);
	SG_NULLFREE(pCtx, cset_id_initial_changeset);
	//SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_REPO_NULLFREE(pCtx, pRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir2);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir1);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);

	return;

fail:
	return;
}

void u0069_treendx_test__deleted_only(SG_context * pCtx, SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathFile = NULL;
	char* cset_id_initial_changeset = NULL;
	char* cset_id_after_add = NULL;
	char* cset_id_after_delete = NULL;
	SG_repo* pRepo = NULL;
	char* pszHidTreeNode = NULL;
	char* pszTreeNodeEntryGID = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	SG_treenode_entry * pTreeNodeEntry = NULL;
	SG_treenode_entry * treenode = NULL;
	SG_pathname* pPathDir1 = NULL;
	SG_pathname* pPathDir2 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* First create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, bufName, pPathWorkingDir, &cset_id_initial_changeset)  );

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir1, pPathWorkingDir, "d1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir2, pPathWorkingDir, "d2")  );

	/* add stuff */
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir2)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir1, "a.txt", 20)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir2, "b.txt", 20)  );

	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_add)  );

	// 2011/04/12 This test originally just did a delete on "d1" which
	//            marked "@/d1" as REMOVED and "@/d1/a.txt" as LOST.
	//            Prior to the fix for W7399, we allowed commits with
	//            LOST items.  We also have bugs SPRAWL-836 and SPRAWL-863
	//            saying that the stuff within the deleted directory should
	//            be marked REMOVED rather that LOST.  Until these latter 2
	//            are fixed, this test will fail.
	VERIFY_ERR_CHECK(  u0069_treendx__delete(pCtx, pPathWorkingDir, "d1")  );

	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_delete)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, bufName, &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, cset_id_after_add, &pszHidTreeNode)  );  //Load the middle one.

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidTreeNode, &pTreenodeRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, "@/d1/a.txt", &pszTreeNodeEntryGID, &pTreeNodeEntry)  );
	VERIFY_COND("pszTreeNodeEntryGID should not be NULL", (NULL != pszTreeNodeEntryGID));
	VERIFY_COND("pTreeNodeEntry should not be NULL", (NULL != pTreeNodeEntry));

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_delete, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the third changeset.", (treenode == NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_add, &treenode));
	VERIFY_COND("Couldn't find @/d1/a.txt in the second changeset.", (treenode != NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_initial_changeset, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the initial changeset.", (treenode == NULL));

fail:
	SG_NULLFREE(pCtx, pszTreeNodeEntryGID);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTreeNodeEntry);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_NULLFREE(pCtx, cset_id_after_add);
	SG_NULLFREE(pCtx, cset_id_after_delete);
	SG_NULLFREE(pCtx, cset_id_initial_changeset);
	//SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_REPO_NULLFREE(pCtx, pRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir2);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir1);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
}

void u0069_treendx_test__not_the_right_gid(SG_context * pCtx, SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathFile = NULL;
	char* cset_id_initial_changeset = NULL;
	char* cset_id_after_add = NULL;
	char* cset_id_after_delete = NULL;
	char* cset_id_after_second_add = NULL;
	SG_repo* pRepo = NULL;
	char* pszHidTreeNode = NULL;
	char* pszTreeNodeEntryGID = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	SG_treenode_entry * pTreeNodeEntry = NULL;
	SG_treenode_entry * treenode = NULL;
	SG_pathname* pPathDir1 = NULL;
	SG_pathname* pPathDir2 = NULL;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* First create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, bufName, pPathWorkingDir, &cset_id_initial_changeset)  );

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir1, pPathWorkingDir, "d1")  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathDir2, pPathWorkingDir, "d2")  );

	/* add stuff */
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir1)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathDir2)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir1, "a.txt", 20)  );
	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir2, "b.txt", 20)  );

	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_add)  );

	VERIFY_ERR_CHECK(  u0069_treendx__delete(pCtx, pPathWorkingDir, "d1/a.txt")  );

	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_delete)  );

	VERIFY_ERR_CHECK(  u0069_treendx__create_file__numbers(pCtx, pPathDir1, "a.txt", 20)  );
	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0069_treendx__commit_all(pCtx, pPathWorkingDir, &cset_id_after_second_add)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, bufName, &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, cset_id_after_second_add, &pszHidTreeNode)  );  //Load the middle one.

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidTreeNode, &pTreenodeRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, "@/d1/a.txt", &pszTreeNodeEntryGID, &pTreeNodeEntry)  );
	VERIFY_COND("pszTreeNodeEntryGID should not be NULL", (NULL != pszTreeNodeEntryGID));
	VERIFY_COND("pTreeNodeEntry should not be NULL", (NULL != pTreeNodeEntry));

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_delete, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the third changeset.", (treenode == NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_add, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the second changeset.", (treenode == NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_after_second_add, &treenode));
	VERIFY_COND("@/d1/a.txt should  be in the fourth changeset.", (treenode != NULL));

	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	treenode = NULL;

	VERIFY_ERR_CHECK(  u0069_treendx__get_treenode_in_changeset(pCtx, pRepo, pszTreeNodeEntryGID, cset_id_initial_changeset, &treenode));
	VERIFY_COND("@/d1/a.txt should not be in the initial changeset.", (treenode == NULL));

fail:
	SG_NULLFREE(pCtx, pszTreeNodeEntryGID);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pTreeNodeEntry);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, treenode);
	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_NULLFREE(pCtx, cset_id_after_add);
	SG_NULLFREE(pCtx, cset_id_after_second_add);
	SG_NULLFREE(pCtx, cset_id_after_delete);
	SG_NULLFREE(pCtx, cset_id_initial_changeset);
	//SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_REPO_NULLFREE(pCtx, pRepo);

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir2);
	SG_PATHNAME_NULLFREE(pCtx, pPathDir1);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
}

TEST_MAIN(u0069_treendx)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir),  32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathTopDir)  );

	BEGIN_TEST(  u0069_treendx_test__moved_only(pCtx, pPathTopDir)  );
	BEGIN_TEST(  u0069_treendx_test__empty_repo(pCtx, pPathTopDir)  );
	BEGIN_TEST(  u0069_treendx_test__added_only(pCtx, pPathTopDir)  );
	BEGIN_TEST(  u0069_treendx_test__deleted_only(pCtx, pPathTopDir)  );
	BEGIN_TEST(  u0069_treendx_test__not_the_right_gid(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__bogus_gid(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__single_add(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__multiple_adds(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__file_rename(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__folder_rename(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__file_rename_then_rename_back(pCtx, pPathTopDir)  );
//	BEGIN_TEST(  u0069_treendx_test__folder_rename_then_rename_back(pCtx, pPathTopDir)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);

	TEMPLATE_MAIN_END;
}
