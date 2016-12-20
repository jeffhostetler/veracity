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

function suite_web_readonly()
{
	var sourceRepo;
	var sourceRepoHttpPath;
	
	var destRepo;
	var destRepoHttpPath;

	this.suite_setUp = function()
	{
		var repo;
		var bIdentical;
		var destName = "dest_" + sg.gid()
		var destPath = pathCombine(tempDir, destName);
		
		print(destPath);
		destPath = destPath.replace("\\\\","/", "g");
		destPath = destPath.replace("\\","/", "g");
		print(destPath);

		sourceRepo = repInfo;
		sourceRepoHttpPath = this.rootReadonlyUrl + "/repos/" + sourceRepo.repoName;
		
		print("Cloning from " + sourceRepoHttpPath + " to " + destName);
		sg.clone__exact(sourceRepoHttpPath, destName);

		print("Checking out to " + destPath);
		testlib.execVV("checkout", destName, destPath);
		
		destRepo = new repoInfo(destName, destPath, destName)
		destRepoHttpPath = this.rootReadonlyUrl + "/repos/" + destRepo.repoName;

		repo = sg.open_repo(sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(bIdentical, "After HTTP clone, repos are identical");
		repo.close();
	};

	this.testSimplePull = function() // Copied from suite_web_sync.
	{
		var bIdentical;
		
		repInfo = sourceRepo;
		createFileOnDisk("file1.txt", 1);
		addRemoveAndCommit();
		
		var repo = sg.open_repo(sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After a change to sourceRepo, repos are different");
		
		testlib.execVV("pull", sourceRepoHttpPath, "--dest", destRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(bIdentical, "After HTTP pull, repos are identical");
		
		repo.close();
	};

	this.testSimplePush = function() // Copied from suite_web_sync, then modified.
	{
		var bIdentical;
		
		repInfo = sourceRepo;
		createFileOnDisk("file2.txt", 1);
		addRemoveAndCommit();
		
		var repo = sg.open_repo(sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After a change to sourceRepo, repos are different");
		
		testlib.execVV_expectFailure("push", destRepoHttpPath, "--src", sourceRepo.repoName);
		bIdentical = repo.compare(destRepo.repoName);
		testlib.testResult(!bIdentical, "After HTTP push, repos are NOT identical");
		
		repo.close();
	};

	this.testSimpleRoutes = function()
	{
		var o;

		// GETs against the readonly server

		o = curl(this.rootReadonlyUrl + "/test/readonly/default.txt");
		testlib.equal("200 OK", o.status);

		o = curl(this.rootReadonlyUrl + "/test/readonly/explicitly-enabled.txt");
		testlib.equal("200 OK", o.status);

		o = curl(this.rootReadonlyUrl + "/test/readonly/explicitly-disabled.txt");
		testlib.equal("404 Not Found", o.status);

		// POSTs against the readonly server

		o = curl("-d", "data", this.rootReadonlyUrl + "/test/readonly/default.txt");
		testlib.equal("405 Method Not Allowed", o.status);

		o = curl("-d", "data", this.rootReadonlyUrl + "/test/readonly/explicitly-enabled.txt");
		testlib.equal("200 OK", o.status);

		o = curl("-d", "data", this.rootReadonlyUrl + "/test/readonly/explicitly-disabled.txt");
		testlib.equal("405 Method Not Allowed", o.status);
	};
}

