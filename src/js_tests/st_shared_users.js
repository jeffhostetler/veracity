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

/* Test the sharing of admin DAGS */
function st_shared_users()
{
    this.test_shared_users = function()
    {
		var srcRepoInfo = repInfo;

		/********************************
		 * CLI init with --shared-users *
		 ********************************/
		testlib.execVV("user", "create", "admin-test-1", "--repo", srcRepoInfo.repoName);
		testlib.execVV("user", "create", "admin-test-2", "--repo", srcRepoInfo.repoName);
		var srcResults = testlib.execVV("user", "list", "--repo", srcRepoInfo.repoName);

		var destName = "dest_" + sg.gid();
		testlib.execVV("repo", "new", destName, "--shared-users", srcRepoInfo.repoName);
		var destResults = testlib.execVV("user", "list", "--repo", destName);
		
		testlib.testResult(srcResults.stdout == destResults.stdout, "Users lists should match.");
		
		
		/*****************************************************************************
		 * CLI push between repos that have the same admin ID but differing repo IDs *
		 *****************************************************************************/
		testlib.execVV("user", "create", "admin-test-3", "--repo", srcRepoInfo.repoName);
		testlib.execVV("user", "create", "admin-test-4", "--repo", srcRepoInfo.repoName);
		srcResults = testlib.execVV("user", "list", "--repo", srcRepoInfo.repoName);
		
		var syncResults = testlib.execVV("push", destName, "--src", srcRepoInfo.repoName);
		testlib.testResult(syncResults.stdout.indexOf("WARNING") != -1, "Push should print warning message.");
		destResults = testlib.execVV("user", "list", "--repo", destName);
		
		testlib.testResult(srcResults.stdout == destResults.stdout, "Users lists should match.");
		
		/*****************************************************************************
		 * CLI pull between repos that have the same admin ID but differing repo IDs *
		 *****************************************************************************/
		testlib.execVV("user", "create", "admin-test-5", "--repo", srcRepoInfo.repoName);
		testlib.execVV("user", "create", "admin-test-6", "--repo", srcRepoInfo.repoName);
		srcResults = testlib.execVV("user", "list", "--repo", srcRepoInfo.repoName);
		
		syncResults = testlib.execVV("pull", srcRepoInfo.repoName, "--dest", destName);
		testlib.testResult(syncResults.stdout.indexOf("WARNING") != -1, "Pull should print warning message.");
		destResults = testlib.execVV("user", "list", "--repo", destName);
		
		testlib.testResult(srcResults.stdout == destResults.stdout, "Users lists should match.");
    };
}
