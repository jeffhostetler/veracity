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

load("../js_test_lib/vscript_test_lib.js");

function st_push_vc_branches()
{
	/* A push that starts with this DAG
	*    A
	*    |
	*    B (master)
	*
	* and results in this DAG
	*    A
	*   / \
	*  B   C (both master)
	*
	* should fail unless --forced.
	*/
	this.test_push__basic_branch_ambiguity = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*    B (master)
		*********************************************************************/
		var srcRepoInfo = repInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Clone it, so there's two repos with the A-B DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create node C, giving us this DAG
			*
			*    A
			*   / \
			*  B   C
			*********************************************************************/

			// Update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");

			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Unforced push should fail.
			*********************************************************************/
			var bCorrectFailure = false;
			try
			{
				sg.push_all(srcRepoInfo.repoName, destName, false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			try
			{
				sg.push_branch(srcRepoInfo.repoName, destName, "master", false);
			}
			catch (e1)
			{
				if (e1.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			/*********************************************************************
			* Forced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, true);
			testlib.testResult(srcRepo.compare(destName), "Repos should be identical after successful push.");

		}
		finally
		{
			srcRepo.close();
		}
	};

	/* Pushing from this DAG
	*    A
	*    |
	*    B (master)
	*
	* into this DAG
	*    A
	*    |
	*    B
	*    |
	*    C (master)
	*
	* should be a no-op, but succeed.
	*/
	this.test_push__branch_out_of_date = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*    B (master)
		*********************************************************************/
		var destRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(destRepoInfo.repoName);
		repInfo = destRepoInfo;
		var destRepo = sg.open_repo(destRepoInfo.repoName);

		try
		{
			// Verify nothing but A exists when we start.
			var leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Clone it, so there's two repos with the A-B DAG.
			*********************************************************************/
			var srcName = "src_" + sg.gid();
			sg.clone__exact(destRepoInfo.repoName, srcName);

			/*********************************************************************
			* Create node C in dest, giving us this DAG
			*    A
			*    |
			*    B
			*    |
			*    C (master)
			*********************************************************************/
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure C is a master head.
			var branches = destRepo.named_branches();
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Unforced push should succeed
			*********************************************************************/
			sg.push_all(srcName, destRepoInfo.repoName, false);

			// Dest should look like it did before the push.
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After no-op push, repo has " + leaves.length + " leaves.");
			branches = destRepo.named_branches();
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Create nodes D and E, giving us this DAG in dest:
			*    A
			*    |
			*    B
			*    |
			*    C
			*    |
			*    D
			*    |
			*    E (master)
			*********************************************************************/
			createFileOnDisk("D", 1);
			addRemoveAndCommit();
			createFileOnDisk("E", 1);
			var hidE = addRemoveAndCommit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing E, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Unforced push should succeed
			*********************************************************************/
			sg.push_all(srcName, destRepoInfo.repoName, false);

			// Dest should look like it did before the push.
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After no-op push, repo has " + leaves.length + " leaves.");
			branches = destRepo.named_branches();
			testlib.testResult(branches.values[hidE].hasOwnProperty("master"), "E should be a master head.");
		}
		finally
		{
			destRepo.close();
		}
	};

	/* Start with two repos, src and dest, having this DAG:
	*    A
	*    |
	*    B (master)
	*
	* The dest repo then         But the src
	* changes to this:           repo changes to this:
	*    A                             A
	*    |                             |
	*    B                             B
	*    |                             |
	*    C (master)                    D (master)
	*
	* The push from source to dest should fail unless forced.
	*
	* (Is this test really any different than the one above?)
	*/
	this.test_push__dest_branch_moved = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*    B (master)
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);
		var destRepo = null;
		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Clone it, so there's two repos with the A-B DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			var destPath = pathCombine(tempDir, destName);
			destPath = destPath.replace("\\\\","/", "g");
			destPath = destPath.replace("\\","/", "g");
			var destRepoInfo = new repoInfo(destName, destPath, destName);
			sg.clone__exact(srcRepoInfo.repoName, destName);
			testlib.execVV("checkout", destName, destPath);

			/*********************************************************************
			* In dest, create node C, giving us:
			*    A
			*    |
			*    B
			*    |
			*    C (master)
			*********************************************************************/

			// Commit C in dest
			repInfo = destRepoInfo;
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();

			destRepo = sg.open_repo(destRepoInfo.repoName);
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure C is a master head.
			var branches = destRepo.named_branches();
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* In src, create node D, giving us:
			*    A
			*    |
			*    B
			*    |
			*    D (master)
			*********************************************************************/

			// Commit D in src
			repInfo = srcRepoInfo;
			createFileOnDisk("D", 1);
			var hidD = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing D, repo has " + leaves.length + " leaves.");

			// Make sure D is a master head.
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidD].hasOwnProperty("master"), "D should be a master head.");

			/*********************************************************************
			* Unforced push should fail.
			*********************************************************************/
			var bCorrectFailure = false;
			try
			{
				sg.push_all(srcRepoInfo.repoName, destName, false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			bCorrectFailure = false;
			try
			{
				sg.push_branch(srcRepoInfo.repoName, destName, "master", false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");


			/*********************************************************************
			* Forced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, true);
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After forced push, dest repo has " + leaves.length + " leaves.");
			testlib.testResult(
				(leaves[0] != leaves[1]) &&
				(leaves[0] == hidC || leaves[0] == hidD) &&
				(leaves[1] == hidC || leaves[1] == hidD),
				"After forced push, leaf hids should be " + hidC + " and " + hidD + ".");

		}
		finally
		{
			srcRepo.close();
			if (destRepo !== null)
				destRepo.close();
		}
	};

	/* Test for V00038. When all leaves are in the same branch:
	*
	* A push that starts with this DAG
	*    A
	*   / \
	*  B   C
	*
	* and results in this DAG
	*    A
	*   /|\
	*  B C E
	*   \|
	*    D
	*
	* should fail unless --forced.
	*/
	this.test_push__new_heads__same_leaf_count = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*   / \
		*  B   C
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// Update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");

			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Clone it, so there's two repos with the ABC DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create nodes D and E in the first repo, giving us this DAG
			*
			*    A
			*   /|\
			*  B C E
			*   \|
			*    D
			*********************************************************************/

			// Merge C with B to get node D
			vscript_test_wc__merge_np( { "rev" : hidB } );
			var hidD = vscript_test_wc__commit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing D, repo has " + leaves.length + " leaves.");

			// Update back to A, commit E
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			createFileOnDisk("E", 1);
			var hidE = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing E, repo has " + leaves.length + " leaves.");

			// Make sure D and E are both master heads.
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidD].hasOwnProperty("master"), "D should be a master head.");
			testlib.testResult(branches.values[hidE].hasOwnProperty("master"), "E should be a master head.");

			/*********************************************************************
			* Unforced push should fail.
			*********************************************************************/
			var bCorrectFailure = false;
			try
			{
				sg.push_all(srcRepoInfo.repoName, destName, false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			try
			{
				sg.push_branch(srcRepoInfo.repoName, destName, "master", false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			/*********************************************************************
			* Forced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, true);
			testlib.testResult(srcRepo.compare(destName), "Repos should be identical after successful push.");

		}
		finally
		{
			srcRepo.close();
		}
	};

	/* Test for H3095. When all leaves are in the same branch:
	*
	* A push that starts with this DAG
	*    A
	*   / \
	*  B   C
	*
	* and results in this DAG
	*    A
	*   / \
	*  B   C
	*       \
	*        D
	*
	* should succeed, even when unforced.
	*/
	this.test_push__same_ambiguity_unforced = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*   / \
		*  B   C
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// Update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Clone it, so there's two repos with the ABC DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create node D in the first repo, giving us this DAG
			*
			*    A
			*   / \
			*  B   C
			*       \
			*        D
			*********************************************************************/

			// Update commit D
			createFileOnDisk("D", 1);
			var hidD = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing D, repo has " + leaves.length + " leaves.");

			// Make sure B and D are both master heads.
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidD].hasOwnProperty("master"), "D should be a master head.");

			/*********************************************************************
			* Unforced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, false);
			testlib.testResult(srcRepo.compare(destName), "Repos should be identical after successful push.");
		}
		finally
		{
			srcRepo.close();
		}
	};

	/* Another test for H3095. When all leaves are in the same branch:
	*
	* Clone (src)			Parent (dest)
	* -----					------
	*	A					  A
	*	|					 / \
	*	B					B   D
	*	|
	*	C
	*
	* push from clone to parent should succeed, even when unforced.
	*/
	this.test_push__same_ambiguity_unforced2 = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*    B
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Clone it, so there's two repos with the A-B DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			var destPath = pathCombine(tempDir, destName);
			destPath = destPath.replace("\\\\","/", "g");
			destPath = destPath.replace("\\","/", "g");
			var destRepoInfo = new repoInfo(destName, destPath, destName);
			sg.clone__exact(srcRepoInfo.repoName, destName);
			testlib.execVV("checkout", destName, destPath);

			/*********************************************************************
			* Create node C in the first repo, giving us this DAG
			*	A
			*	|
			*	B
			*	|
			*	C
			*/
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure C is a master head.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Create node D in the second repo, giving us this DAG
			*	  A
			*	 / \
			*	B   D
			*********************************************************************/

			// In dest, update back to A, commit D
			var destRepo = sg.open_repo(destRepoInfo.repoName);
			try
			{
				repInfo = destRepoInfo;
				sg.fs.cd(destPath);
				vscript_test_wc__update( hidA );
				sg.exec(vv, "branch", "attach", "master");
				createFileOnDisk("D", 1);
				var hidD = addRemoveAndCommit();
				leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
				testlib.testResult(leaves.length == 2, "After committing D, repo has " + leaves.length + " leaves.");

				// Make sure B and D are master heads.
				branches = destRepo.named_branches();
				testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
				testlib.testResult(branches.values[hidD].hasOwnProperty("master"), "D should be a master head.");

				/*********************************************************************
				* Unforced push should succeed.
				*********************************************************************/
				sg.push_all(srcRepoInfo.repoName, destRepoInfo.repoName, false);
			}
			finally
			{
				destRepo.close();
			}
		}
		finally
		{
			srcRepo.close();
		}
	};

	/*
	* A push where the destination repo starts with this DAG, B and C both being in named branch "master"
	*    A
	*   / \
	*  Bm Cm
	*
	* and results in this DAG, D being in named branch "other"
	*     A
	*   / | \
	* Bm Cm Do
	*
	* should succeed, even when unforced.
	*/
	this.test_push__other_branch_ambiguous = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*   / \
		*  Bm Cm
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// Update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Clone it, so there's two repos with the ABC DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create node D in the first repo in a new branch, giving us this DAG
			*
			*     A
			*   / | \
			* Bm Cm Do
			*********************************************************************/

			// Update back to A and commit D
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "new", "other");
			createFileOnDisk("D", 1);
			var hidD = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 3, "After committing D, repo has " + leaves.length + " leaves.");

			// Make sure D is an "other" head.
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidD].hasOwnProperty("other"), "D should be head of named branch 'other'.");

			/*********************************************************************
			* Unforced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, false);
			testlib.testResult(srcRepo.compare(destName), "Repos should be identical after successful push.");
		}
		finally
		{
			srcRepo.close();
		}
	};

	/*
	* A push where the destination repo starts with this DAG, B being in named branch "master"
	*    A
	*    |
	*   Bm
	*
	* and we push only named branch "other" (node D) from this DAG
	*     A
	*   / | \
	* Bm Cm Do
	*
	* should succeed, even when unforced, and result in this DAG in destination:
	*     A
	*   /   \
	* Bm    Do
	*/
	this.test_push__other_branch_ambiguous2 = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*   Bm
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Clone it, so there's two repos with the AB DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			// In the src repo, update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");

			/*********************************************************************
			* Create nodes C and D in the first repo, giving us this DAG
			*
			*     A
			*   / | \
			* Bm Cm Do
			*********************************************************************/
			// Create C
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			// Update back to A and commit D
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "new", "other");
			createFileOnDisk("D", 1);
			var hidD = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 3, "After committing D, repo has " + leaves.length + " leaves.");

			// Make sure D is an "other" head.
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidD].hasOwnProperty("other"), "D should be head of named branch 'other'.");

			/*********************************************************************
			* Unforced push of "other" should succeed.
			*********************************************************************/
			sg.push_branch(srcRepoInfo.repoName, destName, "other", false);
			var destRepo = sg.open_repo(destName);
			try
			{
				leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
				testlib.testResult(leaves.length == 2, "After push, dest repo has " + leaves.length + " leaves.");
				testlib.testResult(leaves.indexOf(hidB) >= 0, "After push, dest repo should have B as a leaf.");
				testlib.testResult(leaves.indexOf(hidC) < 0, "After push, dest repo should NOT have C as a leaf.");
				testlib.testResult(leaves.indexOf(hidD) >= 0, "After push, dest repo should have D as a leaf.");
			}
			finally
			{
				destRepo.close();
			}
		}
		finally
		{
			srcRepo.close();
		}
	};

	/*
	* A push where the destination repo starts with this DAG, B and C both being named branch "master"
	*    A
	*   / \
	*  Bm Cm
	*
	* and results in this DAG, D being an anonymous leaf
	*     A
	*   / | \
	* Bm Cm  D
	*
	* should succeed, even when unforced.
	*/
	this.test_push__other_branch_ambiguous__detached = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*   / \
		*  Bm Cm
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// Update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Clone it, so there's two repos with the ABC DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create node D in the first repo in a new branch, giving us this DAG
			*
			*     A
			*   / | \
			* Bm Cm  D
			*********************************************************************/

			// Update back to A and commit D
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "detach"); // Detach to create anonymous leaf with this commit.
			createFileOnDisk("D", 1);

			// the following does detached commit. var hidD = addRemoveAndCommit();
			vscript_test_wc__addremove();
			vscript_test_wc__commit_np( { "message" : "No Message.", "detached" : true } );
			vscript_test_wc__statusEmptyExcludingIgnores();
			var hidD = sg.wc.parents()[0];

			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 3, "After committing D, repo has " + leaves.length + " leaves.");

			// Make sure D is an anonymous leaf.
			branches = srcRepo.named_branches();
			testlib.testResult(!branches.values.hasOwnProperty(hidD), "D should be an anonymous leaf.");

			/*********************************************************************
			* Unforced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, false);
			testlib.testResult(srcRepo.compare(destName), "Repos should be identical after successful push.");
		}
		finally
		{
			srcRepo.close();
		}
	};

	/*
	* A push where the destination repo starts with this DAG, B being in named branch "master"
	*    A
	*    |
	*   Bm
	*
	* and in the source repo we create C and move the branch to it, so it looks like this:
	*    A
	*   / \
	*  B   Cm  (B is anonymous)
	*
	* should succeed when unforced, and result in this DAG in destination:
	*    A
	*   / \
	*  B   Cm  (B is anonymous)
	*
	* This behavior changed for 1.5: see X9177.
	*/
	this.test_push__reinvent_branch = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*   Bm
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Clone it, so there's two repos with the AB DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			// In the src repo, update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			/*********************************************************************
			* Create node C in the first repo and move master there, giving us
			*
			*    A
			*   / \
			*  B   Cm  (B is anonymous)
			*********************************************************************/
			// create C
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			// Move master to C
			var o = sg.exec(vv, "branch", "move-head", "master", "--all", "-r", hidC);
			testlib.ok(o.exit_status === 0);

			/*********************************************************************
			* Unforced push -b master should succeed.
			*********************************************************************/
			sg.push_branch(srcRepoInfo.repoName, destName, "master", false);
			var destRepo = sg.open_repo(destName);
			try
			{
				leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
				testlib.testResult(leaves.length == 2, "After push, dest repo has " + leaves.length + " leaves.");
				testlib.testResult(leaves.indexOf(hidB) >= 0, "After push, dest repo should have B as a leaf.");
				testlib.testResult(leaves.indexOf(hidC) >= 0, "After push, dest repo should have C as a leaf.");

				branches = destRepo.named_branches();
				testlib.testResult(!branches.values.hasOwnProperty(hidB), "B should NOT be head.");
				testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");
			}
			finally
			{
				destRepo.close();
			}
		}
		finally
		{
			srcRepo.close();
		}
	};

   /*
	* A push that introduces a new ambiguous branch should fail unless forced.
	*
	* Pushing this:          Into this:
	*    A                       A
	*   / \
	* Bn   Cn
	*
	* where n is a new branch that doesn't exist in the destination should fail unless forced.
	*/
	this.test_push__new_ambiguous_branch = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			/*********************************************************************
			* Clone it, so there's two repos with the A DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create nodes B and C in branch "new" in the first repo, giving us this DAG
			*
			*    A
			*   / \
			* Bn   Cn
			*********************************************************************/
			// Attach to branch "new"
			var o = sg.exec(vv, "branch", "new", "new");
			testlib.ok(o.exit_status === 0);

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// In the src repo, update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "new");
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both "new" heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("new"), "B should be a head of 'new'.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("new"), "C should be a head of 'new'.");

			/*********************************************************************
			* Unforced push of "new" should fail.
			*********************************************************************/
			var bCorrectFailure = false;
			try
			{
				sg.push_branch(srcRepoInfo.repoName, destName, "new", false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			/*********************************************************************
			* Unforced push all should fail.
			*********************************************************************/
			bCorrectFailure = false;
			try
			{
				sg.push_all(srcRepoInfo.repoName, destName, false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");

			/*********************************************************************
			* Forced push "new" should succeed.
			*********************************************************************/
			sg.push_branch(srcRepoInfo.repoName, destName, "new", true);
			var destRepo = sg.open_repo(destName);
			try
			{
				leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
				testlib.testResult(leaves.length == 2, "After push, dest repo has " + leaves.length + " leaves.");
				testlib.testResult(leaves.indexOf(hidB) >= 0, "After push, dest repo should have B as a leaf.");
				testlib.testResult(leaves.indexOf(hidC) >= 0, "After push, dest repo should have C as a leaf.");

				branches = destRepo.named_branches();
				testlib.testResult(branches.values[hidB].hasOwnProperty("new"), "B should be a head of 'new'.");
				testlib.testResult(branches.values[hidC].hasOwnProperty("new"), "C should be head of 'new'.");
			}
			finally
			{
				destRepo.close();
			}
		}
		finally
		{
			srcRepo.close();
		}
	};

	/**
	* Push changes that are designed to fool the best-guess dagfrag alorithm, such that
	* we discover we'd be pushing a new head on the second dag connection roundtrip.
	*/
	this.test_push__new_head_in_second_roundtrip = function()
	{
		/*********************************************************************
		* Create empty src and dest repos
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		var destName = "dest_" + sg.gid();
		var destPath = pathCombine(tempDir, destName);
		destPath = destPath.replace("\\\\","/", "g");
		destPath = destPath.replace("\\","/", "g");
		var destRepoInfo = new repoInfo(destName, destPath, destName);
		sg.clone__exact(srcRepoInfo.repoName, destName);
		var destRepo = sg.open_repo(destRepoInfo.repoName);
		testlib.execVV("checkout", destName, destPath);

		try
		{
			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B in dest
			repInfo = destRepoInfo;
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// Make sure B is a master head.
			var branches = destRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");

			// In src, commit C, D, and E
			repInfo = srcRepoInfo;
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			createFileOnDisk("D", 1);
			var hidD = addRemoveAndCommit();
			createFileOnDisk("E", 1);
			var hidE = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing E, repo has " + leaves.length + " leaves.");

			// Make sure E is a master head.
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidE].hasOwnProperty("master"), "E should be a master head.");


			/*********************************************************************
			* Unforced push should fail.
			*********************************************************************/
			var bCorrectFailure = false;
			try
			{
				sg.push_all(srcRepoInfo.repoName, destName, false);
			}
			catch (e)
			{
				if (e.message.indexOf("Error 271") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
					bCorrectFailure = true;
				else
					print(sg.to_json__pretty_print(e));
			}
			testlib.testResult(bCorrectFailure, "Push should fail with sglib error 271.");
		}
		finally
		{
			srcRepo.close();
			destRepo.close();
		}
	};

	/* Push a branch that's missing a changeset (K4136). */
	this.test_push__from_branch_missing_head_cs = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{
			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			/*********************************************************************
			* Clone it, so there's two repos with the A DAG.
			*********************************************************************/
			var destName = "dest_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* Create nodes B and C in master branch in the first repo, giving us this DAG
			*
			*    A
			*   / \
			* Bm   Cm
			*********************************************************************/
			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// In the src repo, update back to A, commit C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both "master" heads.
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a head of 'master'.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a head of 'master'.");

			// push just C to dest
			testlib.execVV("push", "-r", hidC, destName);

			// attempt to push everything from dest back to src
			sg.push_all(destName, srcRepoInfo.repoName, false);

			// src should still have B and C leaves in master
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After no-op push, repo has " + leaves.length + " leaves.");
			branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a head of 'master'.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a head of 'master'.");

		}
		finally
		{
			srcRepo.close();
		}
	};

	/* Push a branch that's been manually "moved back" (H7215). */
	this.test_push__src_branch_moved_back = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*    |
		*    B (master)
		*********************************************************************/
		var srcRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(srcRepoInfo.repoName);
		repInfo = srcRepoInfo;
		var srcRepo = sg.open_repo(srcRepoInfo.repoName);

		try
		{
			// Verify nothing but A exists when we start.
			var leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// Commit B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = srcRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");
			var branches = srcRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a head of 'master'.");

			/*********************************************************************
			* Clone it, so there's two repos with the A-B DAG.
			*********************************************************************/
			var destName = "src_" + sg.gid();
			sg.clone__exact(srcRepoInfo.repoName, destName);

			/*********************************************************************
			* In the src repo, move master back to A:
			*    A (master)
			*    |
			*    B
			*********************************************************************/
			var o = sg.exec(vv, "branch", "move-head", "master", "--all", "-r", hidA);
			testlib.ok(o.exit_status === 0);

			/*********************************************************************
			* Unforced push should succeed.
			*********************************************************************/
			sg.push_all(srcRepoInfo.repoName, destName, false);
			testlib.testResult(srcRepo.compare(destName), "Repos should be identical after successful push.");

			var destRepo = sg.open_repo(destName);
			try
			{
				branches = destRepo.named_branches();
				testlib.testResult(branches.values[hidA].hasOwnProperty("master"), "After push, A should be a head of 'master'.");
			}
			finally
			{
				destRepo.close();
			}
		}
		finally
		{
			srcRepo.close();
		}
	};

	/*
	* The source and destination repo start with this DAG:
	*    A
	*   / \
	*  B   C
	*
	* and the destination repo then changes to this:
	*    A
	*   / \
	*  B   C
	*   \ /
	*    D
	*
	* Nothing changes in the source repo.
	* Pushing from source to dest should do nothing and not give a new heads warning (H1791).
	*/
	this.test_push__nothing_to_merged = function()
	{
		/*********************************************************************
		* Create a repo with this DAG
		*
		*    A
		*   / \
		*  B   C
		*********************************************************************/
		var destRepoInfo = createNewRepo("src_" + sg.gid());
		whoami_testing(destRepoInfo.repoName);
		repInfo = destRepoInfo;
		var destRepo = sg.open_repo(destRepoInfo.repoName);

		try
		{

			// Verify nothing but A exists when we start.
			var leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "Initial repo has " + leaves.length + " leaves.");
			var hidA = leaves[0];

			// create B
			createFileOnDisk("B", 1);
			var hidB = addRemoveAndCommit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing B, repo has " + leaves.length + " leaves.");

			// create C
			vscript_test_wc__update( hidA );
			sg.exec(vv, "branch", "attach", "master");
			createFileOnDisk("C", 1);
			var hidC = addRemoveAndCommit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 2, "After committing C, repo has " + leaves.length + " leaves.");

			// Make sure B and C are both master heads.
			var branches = destRepo.named_branches();
			testlib.testResult(branches.values[hidB].hasOwnProperty("master"), "B should be a master head.");
			testlib.testResult(branches.values[hidC].hasOwnProperty("master"), "C should be a master head.");

			/*********************************************************************
			* Clone, so there's two repos with the ABC DAG.
			*********************************************************************/
			var srcName = "dest_" + sg.gid();
			sg.clone__exact(destRepoInfo.repoName, srcName);

			// In the dest repo, merge C with B to get node D
			vscript_test_wc__merge_np( { "rev" : hidB } );
			var hidD = vscript_test_wc__commit();
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After committing D, repo has " + leaves.length + " leaves.");

			/*********************************************************************
			* Unforced push should succeed.
			*********************************************************************/
			sg.push_all(srcName, destRepoInfo.repoName);
			leaves = destRepo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
			testlib.testResult(leaves.length == 1, "After push, dest repo has " + leaves.length + " leaves.");
			testlib.testResult(leaves.indexOf(hidD) >= 0, "After push, dest repo should have D as a leaf.");
		}
		finally
		{
			destRepo.close();
		}
	};
}
