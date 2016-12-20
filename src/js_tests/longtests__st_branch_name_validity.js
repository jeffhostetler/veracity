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

// Tests various "branch" commands to ensure that they correctly validate branch names.
function st_branch_name_validity()
{
	this.setUp = function()
	{
		this.repo = createNewRepoWithUser();
		this.changeset = sg.wc.parents()[0];
		this.emptyFolder = pathCombine(tempDir, this.repo.folderName + "_empty");
		sg.fs.mkdir_recursive(this.emptyFolder);
	}

	this.testChars = function()
	{
		var invalidChars = "#%/?`~!$^&*()=[]{}\\|;:'\"<>";

		// verify valid characters first
		o = testlib.execVV("branch", "new", "abc123");
		o = testlib.execVV("branch", "add-head", "--rev", this.changeset, "abc123");

		// run through each invalid character
		for (var i = 0; i < invalidChars.length; ++i) {
			var currentChar = invalidChars[i];
			if (currentChar == "\\") {
				currentChar = "\\\\";
			} else if (currentChar == "\"") {
				currentChar = "\\\"";
			}

			sg.fs.cd(this.repo.workingPath);

			// make sure "branch new" validates
			o = testlib.execVV_expectFailure("branch", "new", "abc" + currentChar + "123");
			o = testlib.execVV_expectFailure("branch", "new", currentChar + "abc123");
			o = testlib.execVV_expectFailure("branch", "new", "abc123" + currentChar);

			// make sure "branch add-head" validates
			o = testlib.execVV_expectFailure("branch", "add-head", "--rev", this.changeset, "abc" + currentChar + "123");
			o = testlib.execVV_expectFailure("branch", "add-head", "--rev", this.changeset, currentChar + "abc123");
			o = testlib.execVV_expectFailure("branch", "add-head", "--rev", this.changeset, "abc123" + currentChar);

			sg.fs.cd(this.emptyFolder);

			// make sure "checkout --attach" validates
			o = testlib.execVV_expectFailure("checkout", "--rev", this.changeset, "--attach", "abc" + currentChar + "123", this.repo.repoName);
			o = testlib.execVV_expectFailure("checkout", "--rev", this.changeset, "--attach", currentChar + "abc123", this.repo.repoName);
			o = testlib.execVV_expectFailure("checkout", "--rev", this.changeset, "--attach", "abc123" + currentChar, this.repo.repoName);
		}

		sg.fs.cd(this.repo.workingPath);
	}

	this.testLengths = function()
	{
		var minLength = 1;
		var maxLength = 256;
		var lengthInterval = 5; // ensures that maxLength is hit exactly
		var invalidCount = 5;

		// run through various valid lengths
		for (var i = minLength; i <= maxLength; i = i + lengthInterval) {
			var currentName = new Array(i + 1).join("a");
			o = testlib.execVV("branch", "new", currentName);
			o = testlib.execVV("branch", "add-head", "--rev", this.changeset, currentName);
		}

		// run through various invalid lengths
		o = testlib.execVV_expectFailure("branch", "new", "");
		o = testlib.execVV_expectFailure("branch", "add-head", "--rev", this.changeset, "");
		for (var i = maxLength + 1; i <= maxLength + 1 + invalidCount; ++i) {
			var currentName = new Array(i + 1).join("a");
			o = testlib.execVV_expectFailure("branch", "new", currentName);
			o = testlib.execVV_expectFailure("branch", "add-head", "--rev", this.changeset, currentName);
		}
	}
}
