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
//load("../js_test_lib/vv_serve_process.js");

/* This file contains tests for HTTP push, pull, and clone. */

function suite_web_sync()
{
	var sourceRepo;
	var sourceRepoHttpPath;

	var destRepo;
	var destRepoHttpPath;

	this.suite_setUp = function()
	{
		var repo;
		var bIdentical;
		var destName = "dest_" + sg.gid();
		var destPath = pathCombine(tempDir, destName);

		destPath = destPath.replace("\\\\","/", "g");
		destPath = destPath.replace("\\","/", "g");

		sourceRepo = repInfo;
		sourceRepoHttpPath = this.rootUrl + "/repos/" + encodeURI(sourceRepo.repoName);

		repo = sg.open_repo(sourceRepo.repoName);
		repo.close();

		print("Cloning from " + sourceRepoHttpPath + " to " + destName);
		sg.clone__exact(sourceRepoHttpPath, destName);

		print("Checking out to " + destPath);
		testlib.execVV("checkout", destName, destPath);

		destRepo = new repoInfo(destName, destPath, destName);
		destRepoHttpPath = this.rootUrl + "/repos/" + encodeURI(destRepo.repoName);

		repo = sg.open_repo(sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(bIdentical, "After HTTP clone, repos are identical");
		repo.close();
	};

	this.testSimplePull = function()
	{
		var bIdentical;

		repInfo = sourceRepo;
		createFileOnDisk("file1.txt", 1);
		addRemoveAndCommit();

		var repo = sg.open_repo(sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After a change to sourceRepo, repos are different");

        o = sg.exec(vv, "incoming", sourceRepoHttpPath, "--dest", destRepo.repoName);
        print(sg.to_json(o));
        testlib.ok(o.exit_status === 0);

		testlib.execVV("incoming", sourceRepoHttpPath, "--dest", destRepo.repoName);

		testlib.execVV("pull", sourceRepoHttpPath, "--dest", destRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(bIdentical, "After HTTP pull, repos are identical");

		repo.close();
	};

	this.testPullArea = function()
	{
		/* It would be better if this tested more. But this basic test catches the failure described in K7765. */
		o = sg.exec(vv, "pull", sourceRepoHttpPath, "--dest", destRepo.repoName, "--area", "scrum");
        print(sg.to_json(o));
        testlib.ok(o.exit_status === 0);
	};

	this.testSimplePush = function()
	{
		var bIdentical;

		repInfo = sourceRepo;
		createFileOnDisk("file2.txt", 1);
		addRemoveAndCommit();

		var repo = sg.open_repo(sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After a change to sourceRepo, repos are different");

        o = sg.exec(vv, "outgoing", destRepoHttpPath, "--src", sourceRepo.repoName);
        print(sg.to_json(o));
        testlib.ok(o.exit_status === 0);

		testlib.execVV("push", destRepoHttpPath, "--src", sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(bIdentical, "After HTTP push, repos are identical");

		repo.close();
	};

	this.testNewHeadsPush = function()
	{
		repInfo = sourceRepo;
		createFileOnDisk("file3.txt", 1);
		addRemoveAndCommit();

		repInfo = destRepo;
		createFileOnDisk("file4.txt", 1);
		addRemoveAndCommit();

		var repo = sg.open_repo(sourceRepo.repoName);
		var bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After a changes to both repos, repos are different");

		testlib.execVV("push", destRepoHttpPath, "--src", sourceRepo.repoName, "--force");
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After forced HTTP push of new head, repos are still different");

		testlib.execVV("zingmerge", "--repo", destRepo.repoName);

		testlib.execVV("pull", destRepoHttpPath, "--dest", sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(bIdentical, "After HTTP pull of new head, repos are identical");

		repo.close();
	};

	this.testHeartbeat = function()
	{
		var url = sourceRepoHttpPath + "/heartbeat.json";
        var o = curl(url);
		testlib.equal("200 OK", o.status);

		print(o.body);
		print(JSON.stringify(sourceRepo));

		var obj = JSON.parse(o.body);
		var repo = sg.open_repo(sourceRepo.repoName);
		try
		{
			testlib.equal(repo.repo_id, obj.RepoID);
			testlib.equal(repo.admin_id, obj.AdminID);
			testlib.equal(repo.hash_method, obj.HashMethod);
		}
		finally
		{
			repo.close();
		}

		url = destRepoHttpPath + "/heartbeat.json";
        o = curl(url);
		testlib.equal("200 OK", o.status);

		print(o.body);
		print(JSON.stringify(sourceRepo));

		obj = JSON.parse(o.body);
		repo = sg.open_repo(destRepo.repoName);
		try
		{
			testlib.equal(repo.repo_id, obj.RepoID);
			testlib.equal(repo.admin_id, obj.AdminID);
			testlib.equal(repo.hash_method, obj.HashMethod);
		}
		finally
		{
			repo.close();
		}
	};
}

