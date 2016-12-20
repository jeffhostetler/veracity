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

//Test CLC params

function st_CLC() {

	/**
	 * Test 'vv incoming' some.
	 */
	this.testIncoming = function() {
		var clone = repInfo.repoName+"-testIncoming-clone";
		testlib.testResult(sg.exec("vv", "clone", repInfo.repoName, clone).exit_status==0);

		sg.file.write("testIncoming.txt", "testIncoming");
		testlib.testResult(sg.exec("vv", "add", "testIncoming.txt").exit_status==0);

		var o=sg.exec("vv", "commit", "--message=Commit from testIncoming().");
		if(testlib.testResult(o.exit_status==0))
		{
			var i;
			var a=o.stdout.split("\n");
			var revision_line_1=false;
			for(i=0;!revision_line_1 && i<a.length;++i)
				if(a[i].indexOf("revision:")>=0)
					revision_line_1=a[i];
			testlib.testResult(revision_line_1, "revision line from commit");

			var a=sg.exec("vv", "incoming", "--dest="+clone).stdout.split("\n");
			var revision_line_2=false;
			for(i=0;!revision_line_2 && i<a.length;++i)
				if(a[i].indexOf("revision:")>=0)
					revision_line_2=a[i];
			testlib.testResult(revision_line_2, "revision line from incoming");

			testlib.log("Test that the revnum is hidden from 'vv incoming', whereas it was present in 'vv commit'.");
			testlib.log("(What we actually test for is the colon separating the revnum from the changeset hid.)");
			testlib.testResult(revision_line_1.indexOf(":")!=revision_line_1.lastIndexOf(":") && revision_line_1.indexOf(":")==revision_line_2.lastIndexOf(":"));
		}
	};

	/**
	 * Test that only the first line of a comment is printed, unless '--verbose' is used.
	 */
	this.testMultiLineComment = function() {
		var clone = repInfo.repoName+"-testMultiLineComment-clone";
		testlib.testResult(sg.exec("vv", "clone", repInfo.repoName, clone).exit_status==0);

		sg.fs.cd(repInfo.workingPath);

		testlib.testResult(sg.exec("vv", "branch", "new", "testMultiLineComment").exit_status==0);

		sg.file.write("testMultiLineComment.txt", "testMultiLineComment");
		testlib.testResult(sg.exec("vv", "add", "testMultiLineComment.txt").exit_status==0);

		testlib.testResult(
			sg.exec("vv", "commit", "--message=Line 1/3.\nLine 2/3.\nLine 3/3.").stdout.indexOf("Line 3/3.")>0
		);

		testlib.testResult(sg.exec("vv", "history").stdout.indexOf("Line 3/3.")<0);
		testlib.testResult(sg.exec("vv", "history", "--verbose").stdout.indexOf("Line 3/3.")>0);

		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("Line 3/3.")<0);
		testlib.testResult(sg.exec("vv", "parents", "--verbose").stdout.indexOf("Line 3/3.")>0);

		testlib.testResult(sg.exec("vv", "leaves").stdout.indexOf("Line 3/3.")<0);
		testlib.testResult(sg.exec("vv", "leaves", "--verbose").stdout.indexOf("Line 3/3.")>0);

		testlib.testResult(sg.exec("vv", "heads").stdout.indexOf("Line 3/3.")<0);
		testlib.testResult(sg.exec("vv", "heads", "--verbose").stdout.indexOf("Line 3/3.")>0);

		testlib.testResult(sg.exec("vv", "outgoing", clone).stdout.indexOf("Line 3/3.")<0);
		testlib.testResult(sg.exec("vv", "outgoing", clone, "--verbose").stdout.indexOf("Line 3/3.")>0);

		testlib.testResult(sg.exec("vv", "incoming", "--dest="+clone).stdout.indexOf("Line 3/3.")<0);
		testlib.testResult(sg.exec("vv", "incoming", "--dest="+clone, "--verbose").stdout.indexOf("Line 3/3.")>0);
	}

	/**
	 * This basically just tests that leading whitespace is trimmed from comments.
	 *
	 * The test works because only the first line of the comment is printed (as we just tested in testMultiLineComment),
	 * so if the leading whitespace wasn't trimmed we wouldn't see the "1/2" lines.
	 *
	 * (See work item X1848.)
	 */
	this.testMultiLineWhitespace = function() {
		sg.file.write("testMultiLineWhitespace.txt", "testMultiLineWhitespace");
		testlib.testResult(sg.exec("vv", "add", "testMultiLineWhitespace.txt").exit_status==0);

		testlib.testResult(sg.exec("vv", "commit", "--message=\n\nCommit Comment 1/2.\nCommit Comment 2/2.\n\n").exit_status==0);
		testlib.testResult(sg.exec("vv", "comment", "--rev="+sg.wc.parents()[0], "--message=\n\nComment Comment 1/2.\nComment Comment 2/2.\n\n").exit_status==0);

		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("Commit Comment 1/2.")>0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("Comment Comment 1/2.")>0);
	}

    this.testRevertFileCLC = function testRevertFileCLC() {

	var base = 'testRevertFileCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var tmpFile = createTempFile(file);
        insertInFile(file, 20);
        var tmpFile2 = createTempFile(file);

        o = sg.exec(vv, "revert", file);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );

        testlib.ok(compareFiles(tmpFile, file), "Files should be identical after the revert");
        testlib.existsOnDisk(backup);
        testlib.ok(compareFiles(tmpFile2, backup), "Backup file should match edited file");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        vscript_test_wc__statusEmptyExcludingIgnores();
	sg.fs.remove(backup);

    }

    this.testRevertMultipleFilesCLC = function testRevertMultipleFilesCLC() {

        var base = 'testRevertMultipleFilesCLC';
        var folder = createFolderOnDisk(base);
        var file1 = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 20);
        var file3 = createFileOnDisk(pathCombine(folder, "file3.txt"), 6);


        addRemoveAndCommit();
        var tmpFile1 = createTempFile(file1);
        insertInFile(file1, 20);
        var tmpFile1_2 = createTempFile(file1);

        var tmpFile2 = createTempFile(file2);
        insertInFile(file2, 20);
        var tmpFile2_2 = createTempFile(file2);

        var tmpFile3 = createTempFile(file3);
        insertInFile(file3, 20);
        var tmpFile3_2 = createTempFile(file3);

        o = sg.exec(vv, "revert", file1, file2, file3);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        var backup1 = vscript_test_wc__backup_name_of_file( file1, "sg00" );
        var backup2 = vscript_test_wc__backup_name_of_file( file2, "sg00" );
        var backup3 = vscript_test_wc__backup_name_of_file( file3, "sg00" );

        testlib.ok(compareFiles(tmpFile1, file1), "Files should be identical after the revert");
        testlib.existsOnDisk(backup1);
        testlib.ok(compareFiles(tmpFile1_2, backup1), "Backup file should match edited file");
        testlib.ok(compareFiles(tmpFile2, file2), "Files should be identical after the revert");
        testlib.existsOnDisk(backup2);
        testlib.ok(compareFiles(tmpFile2_2, backup2), "Backup file should match edited file");
        testlib.ok(compareFiles(tmpFile3, file3), "Files should be identical after the revert");
        testlib.existsOnDisk(backup3);
        testlib.ok(compareFiles(tmpFile3_2, backup3), "Backup file should match edited file");
        sg.fs.remove(tmpFile1);
        sg.fs.remove(tmpFile1_2);
        sg.fs.remove(tmpFile2);
        sg.fs.remove(tmpFile2_2);
        sg.fs.remove(tmpFile3);
        sg.fs.remove(tmpFile3_2);
        vscript_test_wc__statusEmptyExcludingIgnores();
	sg.fs.remove(backup1);
	sg.fs.remove(backup2);
	sg.fs.remove(backup3);
    }
    this.testRevertFolderCLC = function testRevertFolderCLC() {

        var base = 'testRevertFolderCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder1"), 2);

        addRemoveAndCommit();

        testlib.inRepo(folder);

        var newName = pathCombine(getParentPath(folder), "rename_" + base);

        sg.exec(vv, "rename", folder, "rename_" + base);
        testlib.existsOnDisk(newName);

        o = sg.exec(vv, "revert", base);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(folder);
        testlib.notOnDisk(newName);

    }

    this.testRevertMultipleFoldersCLC = function testRevertMultipleFoldersCLC() {

        var base = 'testRevertMultipleFoldersCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder1"), 2);
        var folder2 = createFolderOnDisk(pathCombine(base + "2", "folder2"), 3);


        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);

        var newName = pathCombine(getParentPath(folder), "rename_" + base);

        sg.exec(vv, "rename", folder, "rename_" + base);
        vscript_test_wc__remove(folder2);
        testlib.existsOnDisk(newName);
        testlib.notOnDisk(folder2);

        o = sg.exec(vv, "revert", base, base + "2");
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(folder);
        testlib.existsOnDisk(folder2);
        testlib.notOnDisk(newName);

    }

    this.testRevertAllCLC = function testRevertAllCLC() {

        var base = 'testRevertAllCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder1"), 2);
        var folder2 = createFolderOnDisk(pathCombine(base + "2", "folder2"), 3);


        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);

        var newName = pathCombine(getParentPath(folder), "rename_" + base);

        sg.exec(vv, "rename", folder, "rename_" + base);
        vscript_test_wc__remove(folder2);
        testlib.existsOnDisk(newName);
        testlib.notOnDisk(folder2);

        o = sg.exec(vv, "revert", "--all");
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(folder);
        testlib.existsOnDisk(folder2);
        testlib.notOnDisk(newName);

    }

    this.testRevertErrorCLC = function testRevertErrorCLC() {

        o = sg.exec(vv, "revert");
        testlib.ok(o.stderr.search("Usage") >= 0, "should be error");
    }

    this.testRevertNonrecursiveCLC = function testRevertNonrecursiveCLC() {

        var base = 'testRevertNonrecursiveCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder"));
        var file1 = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.h"), 20);
        var file3 = createFileOnDisk(pathCombine(folder, "file3.txt"), 6);


        addRemoveAndCommit();
        var tmpFile1 = createTempFile(file1);
        insertInFile(file1, 20);
        var tmpFile1_2 = createTempFile(file1);

        var tmpFile2 = createTempFile(file2);
        insertInFile(file2, 20);
        var tmpFile2_2 = createTempFile(file2);

        var tmpFile3 = createTempFile(file3);
        insertInFile(file3, 20);
        var tmpFile3_2 = createTempFile(file3);

        var newName = pathCombine(getParentPath(folder), "rename_" + base);

        vscript_test_wc__rename(folder, "rename_" + base);
        testlib.existsOnDisk(newName);

        o = sg.exec(vv, "revert", newName, "--nonrecursive");
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        testlib.existsOnDisk(folder);

        testlib.ok(compareFiles(tmpFile1_2, file1), "file should be the same");

        testlib.ok(compareFiles(tmpFile2_2, file2), "file should be the same");

        testlib.ok(compareFiles(tmpFile3_2, file3), "file should be the samme");
        sg.fs.remove(tmpFile1);
        sg.fs.remove(tmpFile1_2);
        sg.fs.remove(tmpFile2);
        sg.fs.remove(tmpFile2_2);
        sg.fs.remove(tmpFile3);
        sg.fs.remove(tmpFile3_2);
        addRemoveAndCommit();
        vscript_test_wc__statusEmptyExcludingIgnores();

    }

    this.testCommitModifiedFileCLC = function testCommitModifiedFileCLC() {

        var base = 'testCommitModifiedFileCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var tmpFile = createTempFile(file);
        insertInFile(file, 20);
        var tmpFile2 = createTempFile(file);

        o = sg.exec(vv, "commit", "--message=fakemessage", file);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        testlib.ok(compareFiles(tmpFile2, file), "Files should be identical after the commit");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        vscript_test_wc__statusEmptyExcludingIgnores();

    }
/*
    this.testCommitModifiedFileWithUserOptionCLC = function testCommitModifiedFileWithUserOptionCLC() {

        var base = 'testCommitModifiedFileWithUserOptionCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var tmpFile = createTempFile(file);
        insertInFile(file, 20);
        var tmpFile2 = createTempFile(file);

		var o = sg.exec(vv, "user", "create", "myjstestuser");
        if (! testlib.equal(0, o.exit_status, "createuser exit status"))
		{
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			return;
		}

        o = sg.exec(vv, "commit", "--message=fakemessage", "--user=myjstestuser", file);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        s = o.stdout.trim();

        testlib.testResult(s.search("who:  myjstestuser") >= 0, "Default user should have been overriden by myjstestuser");

        testlib.ok(compareFiles(tmpFile2, file), "Files should be identical after the commit");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        vscript_test_wc__statusEmptyExcludingIgnores();

    }*/

    this.testCommitModifiedFileWithStampOptionCLC = function testCommitModifiedFileWithStampOptionCLC() {

        var base = 'testCommitModifiedFileWithStampOptionCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var tmpFile = createTempFile(file);
        insertInFile(file, 20);
        var tmpFile2 = createTempFile(file);

        o = sg.exec(vv, "commit", "--message=fakemessage", "--stamp=stampone", "--stamp=stamptwo", file);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        s = o.stdout.trim();

        testlib.testResult(s.search("stamp:  stampone") >= 0, "Log should show a stamp:  stampone");

        testlib.testResult(s.search("stamp:  stamptwo") >= 0, "Log should show a stamp:  stamptwo");

        testlib.ok(compareFiles(tmpFile2, file), "Files should be identical after the commit");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        vscript_test_wc__statusEmptyExcludingIgnores();

    }



    this.testCommitModifiedFileWithMessageOptionSimpleCLC = function testCommitModifiedFileWithMessageOptionSimpleCLC() {

        var base = 'testCommitModifiedFileWithMessageOptionSimpleCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var tmpFile = createTempFile(file);
        insertInFile(file, 20);
        var tmpFile2 = createTempFile(file);

        o = sg.exec(vv, "commit", "--message=simple message", file);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        s = o.stdout.trim();

        testlib.testResult(s.search("comment:  simple message") >= 0, "Commit should have the comment: simple message");

        testlib.ok(compareFiles(tmpFile2, file), "Files should be identical after the commit");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        vscript_test_wc__statusEmptyExcludingIgnores();

    }

    //    this.testCommitModifiedFileWithMessageOptionUnicodeCLC = function testCommitModifiedFileWithMessageOptionUnicodeCLC() {

    //        var base = 'testCommitModifiedFileWithMessageOptionUnicodeCLC';
    //        var folder = createFolderOnDisk(base);
    //        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
    //        addRemoveAndCommit();
    //        var tmpFile = createTempFile(file);
    //        insertInFile(file, 20);
    //        var tmpFile2 = createTempFile(file);

    //        o = sg.exec(vv, "commit", "--message=abcde\u00A9", file);
    //        testlib.ok(o.exit_status == 0, "exit status should be 0");

    //        s = o.stdout.trim();

    //        testlib.testResult(s.search("Comment : abcde\u00A9") >= 0, "Commit should have the comment: abcde\u00A9", "abcde\u00A9", s);

    //        testlib.ok(compareFiles(tmpFile2, file), "Files should be identical after the commit");
    //        sg.fs.remove(tmpFile);
    //        sg.fs.remove(tmpFile2);
    //        vscript_test_wc__statusEmptyExcludingIgnores();

    //    }

    //vv rename file-or-folder new-name
    this.testRenameFileNewNameExistsCLC = function testRenameFileNewNameExistsCLC() {

        var base = 'testRenameFileNewNameExistsCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var fileNewName = createFileOnDisk(pathCombine(folder, "file2.txt"), 44);

        var tmpFile = createTempFile(file);
        var tmpFile2 = createTempFile(fileNewName);

        o = sg.exec(vv, "rename", file, "file2.txt");
        testlib.ok(o.exit_status != 0, "exit status should not be 0");

        testlib.ok(compareFiles(tmpFile, file), "Files should have been left as is");
        testlib.ok(compareFiles(tmpFile2, fileNewName), "Files should have been left as is");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        sg.fs.remove(fileNewName);
        vscript_test_wc__statusEmptyExcludingIgnores();

    }

    //vv rename file-or-folder new-name
    this.testRenameInvalidNameCLC = function testRenameInvalidNameCLC() {

        var base = 'testRenameInvalidNameCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();

        o = sg.exec(vv, "rename", file, "file/tmp.txt");
        testlib.ok(o.exit_status != 0, "exit status should not be 0");
        vscript_test_wc__statusEmptyExcludingIgnores();

        o = sg.exec(vv, "rename", file, "file\\tmp.txt");
        testlib.ok(o.exit_status != 0, "exit status should not be 0");
        vscript_test_wc__statusEmptyExcludingIgnores();

    }

    this.testRenameAliasFolderNewNameExistsCLC = function testRenameAliasFolderNewNameExistsCLC() {

        var base = 'testRenameAliasFolderNewNameExistsCLC';
        var folder = createFolderOnDisk(base);
        createFolderWithSubfoldersAndFiles(pathCombine(folder, "folder1"), 2, 2);
        var folderToRename = pathCombine(folder, "folder1");
        addRemoveAndCommit();
        var folderNewName = createFolderOnDisk(pathCombine(folder, "folder2"));


        o = sg.exec(vv, "ren", folderToRename, "folder2");
        testlib.ok(o.exit_status != 0, "exit status should not be 0");

        testlib.log(o.stderr);
        testlib.log(o.stdout);

        testlib.existsOnDisk(folderToRename);
        testlib.existsOnDisk(folderNewName);
        testlib.inRepo(folderToRename);
        testlib.notInRepo(folderNewName);
        addRemoveAndCommit();
        vscript_test_wc__statusEmptyExcludingIgnores();

    }


    this.testRenameFileNewNameExistsAsFolderCLC = function testRenameFileNewNameExistsAsFolderCLC() {

        var base = 'testRenameFileNewNameExistsAsFolderCLC';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        var folderWithNewName = createFolderOnDisk(pathCombine(folder, "folder1"));

        var tmpFile = createTempFile(file);

        o = sg.exec(vv, "rename", file, "folder1");
        testlib.ok(o.exit_status != 0, "exit status should not be 0");

        testlib.ok(compareFiles(tmpFile, file), "Files should have been left as is");
        testlib.existsOnDisk(folderWithNewName);
        sg.fs.remove(tmpFile);
        addRemoveAndCommit();
        vscript_test_wc__statusEmptyExcludingIgnores();

    }

    this.testStatusAllCLC = function testStatusAllCLC() {

        var base = 'testStatusAllCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder"));
        var extra = createFolderOnDisk(pathCombine(base, "folder2"));
        var file1 = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 20);
        var file3 = createFileOnDisk(pathCombine(extra, "file3.txt"), 6);

        o = sg.exec(vv, "status");
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        testlib.ok(o.stdout.search("folder/") >= 0);
        testlib.ok(o.stdout.search("folder2/") >= 0);
        testlib.ok(o.stdout.search("file1.txt") >= 0);
        testlib.ok(o.stdout.search("file2.txt") >= 0);
        testlib.ok(o.stdout.search("file3.txt") >= 0);

    }

    this.testStatusDirCLC = function testStatusDirCLC() {

        var base = 'testStatusDirCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder"));
        var extra = createFolderOnDisk(pathCombine(base, "folder2"));
        var file1 = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 20);
        var file3 = createFileOnDisk(pathCombine(extra, "file3.txt"), 6);
        print(folder);

        o = sg.exec(vv, "status", folder);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        print(o.stdout);
        testlib.ok(o.stdout.search("folder/") >= 0);
        testlib.ok(o.stdout.search("folder2/") < 0);
        testlib.ok(o.stdout.search("file1.txt") >= 0);
        testlib.ok(o.stdout.search("file2.txt") >= 0);
        testlib.ok(o.stdout.search("file3.txt") < 0);

    }

    this.testStatusMultipleDirsCLC = function testStatusMultipleDirsCLC() {

        var base = 'testStatusMultipleDirsCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder3"));
        var extra = createFolderOnDisk(pathCombine(base, "folder2"));
        var file1 = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var fileex = createFileOnDisk(pathCombine(base, "fileex.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 20);
        var file3 = createFileOnDisk(pathCombine(extra, "file3.txt"), 6);
        print(folder);

        o = sg.exec(vv, "status", folder, extra);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        print(o.stdout);
        testlib.ok(o.stdout.search("folder/") >= 0);
        testlib.ok(o.stdout.search("folder2/") >= 0);
        testlib.ok(o.stdout.search("folder3/") < 0);
        testlib.ok(o.stdout.search("fileex.txt") < 0);
        testlib.ok(o.stdout.search("file1.txt") >= 0);
        testlib.ok(o.stdout.search("file2.txt") >= 0);
        testlib.ok(o.stdout.search("file3.txt") >= 0);



    }
    this.testStatusFilesCLC = function testStatusFilesCLC() {
        var base = 'testStatusFilesCLC';
        var folder = createFolderOnDisk(pathCombine(base, "folder"));
        var extra = createFolderOnDisk(pathCombine(base, "folder2"));
        var file1 = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 20);
        var file3 = createFileOnDisk(pathCombine(extra, "file3.txt"), 6);

        o = sg.exec(vv, "status", file1, file2);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        print(o.stdout);

        testlib.ok(o.stdout.search("folder2/") < 0);
        testlib.ok(o.stdout.search("file1.txt") >= 0);
        testlib.ok(o.stdout.search("file2.txt") >= 0);
        testlib.ok(o.stdout.search("file3.txt") < 0);
    }

    this.testStatusNonrecursiveCLC = function testStatusNonrecursiveCLC() {

	// TODO 2010/06/21 All of the tests in this file create the same
	// TODO            set of file and folder names, so the o.stdout.search()
	// TODO            may find files/folders created by others tests rather
	// TODO            than the ones created by this test.  For this reason
	// TODO            I have prefixed all of the ones here with xx_.  We
	// TODO            should probably look at the other tests and do something
	// TODO            similar.

        var base    = 'testStatusNonrecursiveCLC';
        var folder  = createFolderOnDisk(pathCombine(base, "xx_folder"));
        var folder2 = createFolderOnDisk(pathCombine(base, "xx_folder3"));
        var extra   = createFolderOnDisk(pathCombine(base, "xx_folder2"));
        var file1   = createFileOnDisk(pathCombine(folder, "xx_file1.txt"), 39);
        var fileex  = createFileOnDisk(pathCombine(base,   "xx_fileex.h"), 39);
        var file2   = createFileOnDisk(pathCombine(folder, "xx_file2.h"), 20);
        var file3   = createFileOnDisk(pathCombine(extra,  "xx_file3.h"), 6);

	//////////////////////////////////////////////////////////////////
	// This isn't really a test.  Rather, just a sanity check that
	// the WD is setup as expected.

        o = sg.exec(vv, "status", "--no-ignores", base);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print("vv status --no-ignores " + base + ":\n" + o.stdout);

        testlib.ok(o.stdout.search("xx_folder/")   >= 0);
        testlib.ok(o.stdout.search("xx_folder2/")  >= 0);
        testlib.ok(o.stdout.search("xx_folder3/")  >= 0);
        testlib.ok(o.stdout.search("xx_fileex.h")  >= 0);
        testlib.ok(o.stdout.search("xx_file1.txt") >= 0);
        testlib.ok(o.stdout.search("xx_file2.h")   >= 0);
        testlib.ok(o.stdout.search("xx_file3.h")   >= 0);

	//////////////////////////////////////////////////////////////////
	// -N on a named directory.

        o = sg.exec(vv, "status", base, "-N");
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print("vv status -N " + base + ":\n" + o.stdout);

	// TODO 2010/06/21 Currently "vv status -N dir" returns only
	// TODO            status for the directory itself -- NOT THE
	// TODO            CONTENTS of the directory.  We are currently
	// TODO            discussing changing this to have depth 1
	// TODO            and/or changing the option to --depth.
	// TODO            So, for now, these search should not find
	// TODO            anything.
	//
	// TODO 2010/06/21 All of these should be in the FOUND section.
	// TODO            Do we need to do more than just verify that
	// TODO            the file/folder name is present in the output?

        testlib.ok(o.stdout.search("xx_folder/")   < 0);
        testlib.ok(o.stdout.search("xx_folder2/")  < 0);
        testlib.ok(o.stdout.search("xx_folder3/")  < 0);
        testlib.ok(o.stdout.search("xx_fileex.h")  < 0);
        testlib.ok(o.stdout.search("xx_file1.txt") < 0);
        testlib.ok(o.stdout.search("xx_file2.h")   < 0);
        testlib.ok(o.stdout.search("xx_file3.h")   < 0);

	//////////////////////////////////////////////////////////////////
	// -N with NO NAMED directory.
	//
	// TODO 2010/06/21 See sprawl-882.  What should -N without a
	// TODO            NAMED directory do?

        o = sg.exec(vv, "status", "-N");
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print("vv status -N:\n" + o.stdout);

        //testlib.ok(o.stdout.search("xx_folder/")   < 0);
        //testlib.ok(o.stdout.search("xx_folder2/")  < 0);
        //testlib.ok(o.stdout.search("xx_folder3/")  < 0);
        //testlib.ok(o.stdout.search("xx_fileex.h")  < 0);
        //testlib.ok(o.stdout.search("xx_file1.txt") < 0);
        //testlib.ok(o.stdout.search("xx_file2.h")   < 0);
        //testlib.ok(o.stdout.search("xx_file3.h")   < 0);

	//////////////////////////////////////////////////////////////////
	// -N on a file is kinda silly, but we have to allow it.  I guess.

        o = sg.exec(vv, "status", file1, "-N");
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print("vv status -N " + file1 + ":\n" + o.stdout);

        testlib.ok(o.stdout.search("xx_folder2/")   < 0);
        testlib.ok(o.stdout.search("xx_folder3/")   < 0);
        testlib.ok(o.stdout.search("xx_fileex.h")   < 0);
        testlib.ok(o.stdout.search("xx_file1.txt") >= 0);
        testlib.ok(o.stdout.search("xx_file2.h")    < 0);
        testlib.ok(o.stdout.search("xx_file3.h")    < 0);

    }
    this.testMove = function testMove() {
        var base = 'testMove';
	//First, verify that move, and all aliases with no arguments
	//result in the correct errors.
	returnObj = sg.exec(vv, "move");
        testlib.ok(returnObj.stderr.search("Usage: vv move") >= 0);
	returnObj = sg.exec(vv, "mv");
        testlib.ok(returnObj.stderr.search("Usage: vv move") >= 0);

	}
    /*this.testForceOption = function testForceOption() {
        var base = 'testForceOption';
        var origin = createFolderOnDisk(pathCombine(base, "origin"));
        var target = createFolderOnDisk(pathCombine(base, "target"));
        var subFolder = createFolderOnDisk(pathCombine(origin, "subFolder"));
        var subFolderInTarget = createFolderOnDisk(pathCombine(target, "subFolder"));
        var file = createFileOnDisk(pathCombine(origin, "file1.txt"), 39);
        var fileInTarget = createFileOnDisk(pathCombine(target, "file1.txt"), 39);
        addRemoveAndCommit();

	returnObj = sg.exec(vv, "move", "-F", subFolder, target);
	testlib.assertSuccessfulExec(returnObj);
	testlib.existsOnDisk(subFolderInTarget);
	testlib.inRepo(subFolderInTarget);
	testlib.notOnDisk(subFolder);
	returnObj = sg.exec(vv, "move", "-F", file, target);
        testlib.ok(returnObj.exit_status == 0);
	testlib.existsOnDisk(fileInTarget);
	testlib.inRepo(fileInTarget);
	testlib.notOnDisk(file);
	returnObj = sg.exec(vv, "commit", "-m", "\"\"");
        testlib.ok(returnObj.exit_status == 0);
	}*/
    this.testMoveMultiple = function testMoveMultiple() {
        var base = 'testMoveMultiple';
        var origin = createFolderOnDisk(pathCombine(base, "origin"));
        var target = createFolderOnDisk(pathCombine(base, "target"));
        var file1 = createFileOnDisk(pathCombine(origin, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(origin, "file2.txt"), 39);
        var file3 = createFileOnDisk("file3.txt", 39);
        addRemoveAndCommit();

	returnObj = testlib.execVV("move", file1, file2, file3, target);
	testlib.assertSuccessfulExec(returnObj);
	testlib.existsOnDisk(pathCombine(target, "file1.txt"));
	testlib.existsOnDisk(pathCombine(target, "file2.txt"));
	testlib.existsOnDisk(pathCombine(target, "file3.txt"));

        var expect_test = new Array;
	expect_test["Moved"] = [ "@/" + target + "/file1.txt",
				 "@/" + target + "/file2.txt",
				 "@/" + target + "/file3.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	returnObj = testlib.execVV("revert", "--all");
	testlib.assertSuccessfulExec(returnObj);

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////

	returnObj = testlib.execVV_expectFailure("move", file1, "nonexistantFile", file2, file3, target);
	testlib.assertFailedExec(returnObj);

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	// In WC version, I changed the behavior of "move" so that
	// if there are any bogus args, we stop and complain rather
	// than moving the ones that we can.
	// testlib.existsOnDisk(pathCombine(target, "file1.txt"));
	// testlib.existsOnDisk(pathCombine(target, "file2.txt"));
	// testlib.existsOnDisk(pathCombine(target, "file3.txt"));
	}

    this.testNoNesting = function testNoNesting() {
	sleep(5000);

	var base = 'testNoNesting';
	var wd = createFolderOnDisk(pathCombine(base, "wd"));

	if (! sg.fs.cd(wd))
	{
	    testlib.testResult(false, "unable to change to " + wd);
	    return;
	}

	var newRepoName = "myshinynewrepo" + (new Date()).getTime();

	returnObj = sg.exec(vv, "init", newRepoName, ".");

	testlib.assertFailedExec(returnObj);

	for ( var r in sg.list_descriptors() )
	    {
		if (r == newRepoName)
		    testlib.testResult(false, newRepoName + " should not be created");
	    }
    };

    this.testAddIncorrectCase = function testAddIncorrectCase() {
	var base = 'testAddIncorrectCase';
	var wd = createFolderOnDisk(pathCombine(base, "wd"));

	if (! sg.fs.cd(wd))
	{
	    testlib.testResult(false, "unable to change to " + wd);
	    return;
	}


	sg.file.write("Bob.txt", "nothing");
	returnObj = sg.exec(vv, "add", "bob.txt"); //Note the incorrect case
	testlib.assertFailedExec(returnObj);

	//These tests were commented out, since we have decided to
	//accept the standard windows behavior
	//sg.file.write("Readme", "nothing");
	//returnObj = sg.exec(vv, "add", "Readme."); //Note the extra period
	//testlib.assertFailedExec(returnObj);

	//sg.file.write("Readme2.", "nothing");
	//returnObj = sg.exec(vv, "add", "Readme2."); //Note the lack of the extra period
	//testlib.assertSuccessfulExec(returnObj);

	//sg.file.write("Readme3", "nothing");
	//returnObj = sg.exec(vv, "add", "Readme3 "); //Note the extra space
	//testlib.assertFailedExec(returnObj);

	//sg.file.write("Readme4 ", "nothing");
	//returnObj = sg.exec(vv, "add", "Readme4"); //Note the lack of the extra space
	//testlib.assertFailedExec(returnObj);


	//Now do some tests with a subfolder
        sg.fs.mkdir("subfolder");

	sg.file.write("subfolder/Readme4", "nothing");

	returnObj = sg.exec(vv, "add", "SUBfolder/Readme4"); //Note the capitalization of the folder

	testlib.assertFailedExec(returnObj);

	returnObj = sg.exec(vv, "add", "subfolder/readme4"); //Note the capitalization of the subfile

	testlib.assertFailedExec(returnObj);

    };

    this.testAddRemoveIncorrectCase = function testAddRemoveIncorrectCase() {
	var base = 'testAddRemoveIncorrectCase';
	var wd = createFolderOnDisk(pathCombine(base, "wd"));

	if (! sg.fs.cd(wd))
	{
	    testlib.testResult(false, "unable to change to " + wd);
	    return;
	}


	sg.file.write("Bob.txt", "nothing");

	returnObj = sg.exec(vv, "addremove", "bob.txt"); //Note the incorrect case

	testlib.assertFailedExec(returnObj);

	//These tests were commented out, since we have decided to
	//accept the standard windows behavior
	//sg.file.write("Readme", "nothing");
	//returnObj = sg.exec(vv, "addremove", "Readme."); //Note the extra period
	//testlib.assertFailedExec(returnObj);

	//sg.file.write("Readme2.", "nothing");
	//returnObj = sg.exec(vv, "addremove", "Readme2"); //Note the lack of the extra period
	//testlib.assertFailedExec(returnObj);

	//sg.file.write("Readme3", "nothing");
	//returnObj = sg.exec(vv, "addremove", "Readme3 "); //Note the extra space
	//testlib.assertFailedExec(returnObj);

	//sg.file.write("Readme4 ", "nothing");
	//returnObj = sg.exec(vv, "addremove", "Readme4"); //Note the lack of the extra space
	//testlib.assertFailedExec(returnObj);


	//Now do some tests with a subfolder
        sg.fs.mkdir("subfolder");

	sg.file.write("subfolder/Readme4", "nothing");

	returnObj = sg.exec(vv, "addremove", "SUBfolder/Readme4"); //Note the capitalization of the folder

	testlib.assertFailedExec(returnObj);

	returnObj = sg.exec(vv, "addremove", "subfolder/readme4"); //Note the capitalization of the subfile

	testlib.assertFailedExec(returnObj);

    };

}

function st_CLCStatus()
{
	this.testStatusOnMissingObject =function()
	{
		//This should inform you that one of the objects couldn't be found.
		returnObj = sg.exec(vv, "status", "itemThatIsn'tThere");
        	testlib.ok(returnObj.stderr.search("The requested object could not be found") >= 0);
		testlib.assertFailedExec(returnObj);
	}
	this.testStatusOnMissingObjectWithPendingChange =function()
	{
		//This should inform you that one of the objects couldn't be found.
		sg.file.write("bob.txt", "nothing");
		returnObj = sg.exec(vv, "status", "itemThatIsn'tThere");
        	testlib.ok(returnObj.stderr.search("The requested object could not be found") >= 0);
		testlib.assertFailedExec(returnObj);
		returnObj = sg.exec(vv, "status", "itemThatIsn'tThere", "bob.txt");
        	testlib.ok(returnObj.stderr.search("The requested object could not be found") >= 0);
		testlib.assertFailedExec(returnObj);
		sg.fs.remove("bob.txt");
	}
	function verifyThatAtLeastOneLineEndsWith(lines, strSearch)
	{
		for (i in lines)
		{
			if (lines[i].match(strSearch+"$")==strSearch)
				return;
		}
		testlib.equal(1, 0, "Did not find a matching line for: " + strSearch);
	}
    this.testTestOption = function testTestOption() {
        	var base = 'testTestOption';
		createFolderOnDisk(base);
        	addRemoveAndCommit();
		var subfolderName = base + "/subfolder";
		var subfileName = subfolderName + "/subfile";
		var subsubfolderName = subfolderName + "/sub2";
		var subsubfileName = subsubfolderName + "/subfile";
		createFolderOnDisk(subfolderName);
		createFolderOnDisk(subfileName);
		createFolderOnDisk(subsubfolderName);
		createFolderOnDisk(subsubfileName);

	        var expect_test = new Array;
		expect_test["Found"] = [ "@/" + subfolderName,
					 "@/" + subfileName,
					 "@/" + subsubfolderName,
					 "@/" + subsubfileName ];
        	vscript_test_wc__confirm_wii(expect_test);

		linesOfOutput = testlib.execVV_lines("add", "--test", subfolderName);

	        var expect_test = new Array;
		expect_test["Found"] = [ "@/" + subfolderName,
					 "@/" + subfileName,
					 "@/" + subsubfolderName,
					 "@/" + subsubfileName ];
        	vscript_test_wc__confirm_wii(expect_test);

		linesOfOutput = testlib.execVV_lines("addremove", "--test", subfolderName);

	        var expect_test = new Array;
		expect_test["Found"] = [ "@/" + subfolderName,
					 "@/" + subfileName,
					 "@/" + subsubfolderName,
					 "@/" + subsubfileName ];
        	vscript_test_wc__confirm_wii(expect_test);

		linesOfOutput = testlib.execVV_lines("addremove", "--test");

	        var expect_test = new Array;
		expect_test["Found"] = [ "@/" + subfolderName,
					 "@/" + subfileName,
					 "@/" + subsubfolderName,
					 "@/" + subsubfileName ];
        	vscript_test_wc__confirm_wii(expect_test);

		sg.fs.rmdir_recursive( subfolderName );
	}
    this.testTestOption__Remove = function testTestOption__Remove() {
        	var base = 'testTestOption__Remove';
		createFolderOnDisk(base);
		var subfolderName = base + "/subfolder";
		var subfileName = subfolderName + "/subfile";
		var subsubfolderName = subfolderName + "/sub2";
		var subsubfileName = subsubfolderName + "/subfile";
		createFolderOnDisk(subfolderName);
		createFolderOnDisk(subfileName);
		createFolderOnDisk(subsubfolderName);
		createFolderOnDisk(subsubfileName);

	        var expect_test = new Array;
		expect_test["Found"] = [ "@/" + base,
					 "@/" + subfolderName,
					 "@/" + subfileName,
					 "@/" + subsubfolderName,
					 "@/" + subsubfileName ];
        	vscript_test_wc__confirm_wii(expect_test);

        	addRemoveAndCommit();

	        var expect_test = new Array;
        	vscript_test_wc__confirm_wii(expect_test);

		linesOfOutput = testlib.execVV_lines("remove", "--test", subfolderName);

	        var expect_test = new Array;
        	vscript_test_wc__confirm_wii(expect_test);
	}
}

function st_CLCwhoami()
{
	this.testWhoamiWithNoArguments =function()
	{
		//This should print out your current user.
		returnObj = testlib.execVV("whoami");
		testlib.ok(returnObj.stdout.length >= 0);
	}

	this.testUserWithNoArguments = function ()
	{
		//This should print out your current user.
		returnObj = testlib.execVV("user");
		testlib.ok(returnObj.stdout.length >= 0);
	}
}

function st_clc_repo()
{
	this.testRepoCommand =function()
	{
		var clone = repInfo.repoName+"-testRepoCommand-clone";
		var clone1 = repInfo.repoName+"-testRepoCommand-clone-2A";
		var clone2 = repInfo.repoName+"-testRepoCommand-clone-2B";
		sg.exec("vv", "clone", repInfo.repoName, clone);
		sg.exec("vv", "clone", clone, clone1);
		sg.exec("vv", "clone", clone, clone2);

		testlib.log("Repo info on CWD");
		sg.fs.cd(repInfo.workingPath);
		testlib.testResult(sg.exec("vv", "repo").exit_status==0);

		var output = sg.exec("vv", "repo").stdout;
		testlib.testResult(output.indexOf(repInfo.repoName)>=0, "repo name in 'vv repo'");
		testlib.testResult(output.indexOf("Current repository:")<0, "Headers not in 'vv repo'");

		output = sg.exec("vv", "repo", "info").stdout;
		testlib.testResult(output.indexOf(repInfo.repoName)>=0, "repo name in 'vv repo info'");
		testlib.testResult(output.indexOf("Current repository:")>=0, "Headers in 'vv repo info'");

		sg.fs.cd("..");
		testlib.testResult(sg.exec("vv", "repo").exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo").stderr.indexOf("not currently inside a working copy")>=0);
		testlib.testResult(sg.exec("vv", "repo", "info").stderr==sg.exec("vv", "repo").stderr);

		testlib.log("Repo info on descriptor name versus disk path");
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.repoName   , "--verbose").exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.workingPath, "--verbose").exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.repoName   ).stdout.indexOf("Root of Working Copy")<0);
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.workingPath).stdout.indexOf("Root of Working Copy")>=0);

		testlib.log("List repos");
		testlib.testResult(sg.exec("vv", "repo", "list").exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone1)>=0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone2)>=0);
		testlib.testResult(sg.exec("vv", "repos").exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list", "--list-all").exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list", "--list-all").stdout.indexOf(clone1)>=0);
		testlib.testResult(sg.exec("vv", "repo", "list", "--list-all").stdout.indexOf(clone2)>=0);
		testlib.testResult(sg.exec("vv", "repos").stdout==sg.exec("vv", "repo", "list").stdout);

		testlib.log("Delete repo.");
		testlib.testResult(sg.exec("vv", "repo", "delete", clone1, clone2).exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone1)<0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone2)<0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone)>=0);
		testlib.testResult(sg.exec("vv", "repo", "delete", clone).exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone)<0);

		testlib.log("New repo.");
		var new1 = repInfo.repoName+"-testRepoCommand-new1";
		var new2 = repInfo.repoName+"-testRepoCommand-new2";
		testlib.testResult(sg.exec("vv", "repo", "new", new1).exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(new1)>=0);
		testlib.testResult(sg.exec("vv", "repo", "new", new1).exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "new", new1).stderr.indexOf("already exists")>0);
		testlib.testResult(sg.exec("vv", "repo", "new", new2, "--hash=SKEIN/512").exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(new2)>=0);
		// (--shared-users option already tested in st_shared_users.js)

		testlib.log("Invalid usage");
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.repoName, "--all-but").exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.repoName, "--no-prompt").exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "info", repInfo.repoName, clone1).exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "list", "--all-but").exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "list", "--no-prompt").exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "list", repInfo.repoName).exit_status!=0);
		sg.exec("vv", "clone", clone, clone+"3");
		testlib.testResult(sg.exec("vv", "repo", "delete", clone+"3", "--verbose").exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "new", new2+"B", "--hash=SKEIN/256", "--shared-users="+new2).exit_status!=0);

		testlib.log("Rename");
		sg.exec("vv", "clone", repInfo.repoName, clone1);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone1)>=0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone2)<0);
		testlib.testResult(sg.exec("vv", "repo", "rename", "--no-prompt", clone1, clone2).exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone1)<0);
		testlib.testResult(sg.exec("vv", "repo", "list").stdout.indexOf(clone2)>=0);
		testlib.testResult(sg.exec("vv", "repo", "rename", "--no-prompt", clone2, repInfo.repoName).exit_status!=0);
		testlib.testResult(sg.exec("vv", "repo", "rename", "--no-prompt", clone2, clone).exit_status==0);
		testlib.testResult(sg.exec("vv", "repo", "rename", "--no-prompt", clone2, clone1).exit_status!=0);
	}

	/**
	 * This test tests operations in a working copy whose repo does not exist in the closet
	 * (in this case, because it has been deleted).
	 *
	 * Most operations should either succeed (eg. 'vv help') or else fail with a "Not a repository" error message.
	 */
	this.testWorkingFolderOfDeletedRepo =function()
	{
		var clone = repInfo.repoName+"-testWorkingFolderOfDeletedRepo-clone";

		sg.fs.cd(repInfo.workingPath);
		sg.fs.cd("..");

		testlib.testResult(sg.exec("vv", "clone", repInfo.repoName, clone).exit_status==0);
		testlib.testResult(sg.exec("vv", "checkout", clone, clone).exit_status==0);

		sg.fs.cd(clone);

		testlib.log("Setting up history.");

		sg.file.write("FileX.txt", "FileX");
		vscript_test_wc__add("FileX.txt");
		vscript_test_wc__commit();
		testlib.testResult(sg.exec("vv", "update", "--rev=1", "--attach=master").exit_status==0);

		sg.fs.mkdir("ABC");
		sg.fs.cd("ABC");
		sg.file.write("FileA.txt", "FileA");
		vscript_test_wc__add("FileA.txt");
		sg.file.write("FileB.txt", "FileB");
		vscript_test_wc__add("FileB.txt");
		sg.file.write("FileC.txt", "FileC");
		vscript_test_wc__add("FileC.txt");
		sg.fs.cd("..");
		sg.fs.mkdir("DEF");
		sg.fs.cd("DEF");
		sg.file.write("FileD.txt", "FileD");
		vscript_test_wc__add("FileD.txt");
		sg.file.write("FileE.txt", "FileE");
		vscript_test_wc__add("FileE.txt");
		sg.file.write("FileF.txt", "FileF");
		vscript_test_wc__add("FileF.txt");
		vscript_test_wc__commit();

		testlib.log("Deleting the repo now.");
		sg.fs.cd(repInfo.workingPath);
		sg.fs.cd("..");
		testlib.testResult(sg.exec("vv", "repo", "delete", clone).exit_status==0);
		sg.fs.cd(clone);

		var info = sg.exec("vv", "repo").stderr;
		testlib.testResult((info.indexOf(clone)>0)&&(info.indexOf("repository has been renamed or deleted")>=0), "'vv repo' error report");

		var testFailsOk = function(command){
			var o;
			if(arguments.length>=4)      o = sg.exec("vv", command, arguments[1], arguments[2], arguments[3]);
			else if(arguments.length>=3) o = sg.exec("vv", command, arguments[1], arguments[2]);
			else if(arguments.length>=2) o = sg.exec("vv", command, arguments[1]);
			else                         o = sg.exec("vv", command);
			testlib.testResult(o.exit_status!=0, "'vv "+command+"' exit status");
		    print("testFailsOk: " + command + ":");
		    print("     stdout: [" + o.stdout + "]");
		    print("     stderr: [" + o.stderr + "]");
			testlib.testResult(
				o.stdout.indexOf("repository has been renamed or deleted")>=0
				|| o.stderr.indexOf("repository has been renamed or deleted")>=0
				|| o.stderr.indexOf("Not a repository")>=0
				|| o.stderr.indexOf("Usage: vv")>=0,
				"'vv "+command+"' error report");
		}
		var testSuccess = function(command){
			testlib.testResult(sg.exec("vv", command).exit_status==0, "vv "+command+" exit status");
		}

		testFailsOk("status");
		testFailsOk("whoami");

		testSuccess("help");
		testSuccess("help", "whoami");
		testSuccess("config");

		testFailsOk("cat", "FileA.txt");
		testFailsOk("user", "create", "me");
		testFailsOk("diff");
		testFailsOk("diffmerge");
		testFailsOk("heads");
		testFailsOk("heads", "--branch=master");
		testFailsOk("history");
		testFailsOk("leaves");
		testFailsOk("parents");
		testFailsOk("users");

		testFailsOk("merge");
		//resolve

		sg.fs.cd("ABC");

		sg.file.write("FileA2.txt", "FileA2");
		testFailsOk("add", "FileA2.txt");

		testFailsOk("remove", "FileA.txt");

		sg.fs.remove("FileB.txt");
		sg.file.write("FileB2.txt", "FileB2");
		testFailsOk("addremove");

		sg.file.append("FileC.txt", "-Extra!");
		testFailsOk("revert", "FileC.txt");

		sg.fs.cd("../DEF");

		testFailsOk("move", "FileD.txt", "..");
		testFailsOk("rename", "FileE.txt", "FileE-renamed.txt");

		sg.file.append("FileF.txt", "-Extra!");
		testFailsOk("commit", "FileF.txt", "--message=change");

		testFailsOk("comment", "--rev=1", "Hello.");

		testFailsOk("lock", "FileF");
		testFailsOk("locks");
		//unlock

		testFailsOk("update", "--rev=1");

		testFailsOk("branch");
		testFailsOk("branch", "list");
		testFailsOk("branch", "new", "my-super-branch");

		testFailsOk("stamp", "add", "my-super-stamp");
		testFailsOk("stamp", "list");
		testFailsOk("tag", "add", "my-super-tag");
		testFailsOk("tag", "list");

		testFailsOk("incoming");
		testFailsOk("outgoing");
		testFailsOk("pull");
		testFailsOk("push");

		testFailsOk("serve");
		testFailsOk("zip", "my-super-archive.zip");
	}
}

function st_clc_stamp()
{
	this.testStamps =function()
	{
		sg.file.write("st_clc_stamp.txt", "st_clc_stamp");
		vscript_test_wc__add("st_clc_stamp.txt");
		vscript_test_wc__commit();

		var clone = repInfo.repoName+"-testStamps-clone";
		sg.exec("vv", "clone", repInfo.repoName, clone);

		var repo2 = "--repo="+clone;

		testlib.log("Invalid usage of 'vv stamp add', giving a subcommand-specific usage text.");
		testlib.testResult(sg.exec("vv", "stamp", "add").exit_status!=0);
		testlib.testResult(sg.exec("vv", "stamp", "add").stderr.indexOf("Usage: vv stamp add STAMPNAME")>0);

		testlib.log("Add new stamp to parent.");
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp1").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("stamp1")>0);

		testlib.log("Add new stamp to rev.");
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp2", "-r1").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("stamp2")<0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("stamp2")>0);

		testlib.log("Add existing stamp to parent.");
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp2").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("stamp2")>0);

		testlib.log("Add new stamp to parent in different repo. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp3", repo2).exit_status!=0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("stamp3")<0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("stamp3")<0);

		testlib.log("Add new stamp to in different repo.");
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp3", "-r2", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("stamp3")<0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("stamp3")>0);

		testlib.log("Re-add stamp.");
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp1").exit_status==0);
		testlib.testResult(sg.exec("vv", "stamp", "add", "stamp1").stdout.indexOf("already stamped")>0);

		testlib.log("List stamps.");
		testlib.testResult(sg.exec("vv", "stamp", "list").exit_status==0);
		testlib.testResult(sg.exec("vv", "stamp", "list").stdout.indexOf("stamp1")>=0);
		testlib.testResult(sg.exec("vv", "stamp", "list").stdout.indexOf("stamp2")>=0);
		testlib.testResult(sg.exec("vv", "stamp", "list").stdout.indexOf("stamp3")<0);
		testlib.testResult(sg.exec("vv", "stamps").exit_status==0);
		testlib.testResult(sg.exec("vv", "stamps").stdout==sg.exec("vv", "stamp", "list").stdout);

		testlib.log("List stamps in different repo.");
		testlib.testResult(sg.exec("vv", "stamp", "list", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "stamp", "list", repo2).stdout.indexOf("stamp1")<0);
		testlib.testResult(sg.exec("vv", "stamp", "list", repo2).stdout.indexOf("stamp2")<0);
		testlib.testResult(sg.exec("vv", "stamp", "list", repo2).stdout.indexOf("stamp3")>=0);

		testlib.log("List specific stamp.");
		testlib.testResult(sg.exec("vv", "stamp", "list", "stamp2").exit_status==0);
		testlib.testResult(sg.exec("vv", "stamp", "list", "stamp2").stdout.indexOf("1:")>0);
		testlib.testResult(sg.exec("vv", "stamp", "list", "stamp2").stdout.indexOf("2:")>0);

		testlib.log("List specific stamp in different repo.");
		testlib.testResult(sg.exec("vv", "stamp", "list", "stamp3", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "stamp", "list", "stamp3", repo2).stdout.indexOf("2:")>0);

		testlib.log("Remove stamp using '--all' and a rev. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp1", "--all", "-r2").exit_status!=0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("stamp1")>0);

		testlib.log("Remove non-existing stamp from parent. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp3").exit_status!=0);

		testlib.log("Remove non-existing stamp using '--all'. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp3", "--all").exit_status!=0);

		testlib.log("Remove stamp from rev that does not exist on that rev. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp1", "-r1").exit_status!=0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("stamp1")>0);

		testlib.log("Remove stamp from parent.");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp2").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("stamp2")<0);

		testlib.log("Remove stamp from rev.");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp2", "-r1").exit_status==0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("stamp2")<0);

		testlib.log("Remove stamp using '--all'.");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp1", "--all").exit_status==0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("stamp1")<0);

		testlib.log("Remove stamp from parent in different repo. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp3", repo2).exit_status!=0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("stamp3")>0);

		testlib.log("Remove stamp in different repo.");
		testlib.testResult(sg.exec("vv", "stamp", "remove", "stamp3", "-r2", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("stamp3")<0);

		testlib.log("No subcommand. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp").exit_status!=0);

		testlib.log("Bogus subcommand. (Error.)");
		testlib.testResult(sg.exec("vv", "stamp", "oops").exit_status!=0);
	}
}

function st_clc_tag()
{
	this.testTags =function()
	{
		sg.file.write("st_clc_tag.txt", "st_clc_tag");
		vscript_test_wc__add("st_clc_tag.txt");
		vscript_test_wc__commit();

		var clone = repInfo.repoName+"-testTags-clone";
		sg.exec("vv", "clone", repInfo.repoName, clone);

		var repo2 = "--repo="+clone;

		testlib.log("Invalid usage of 'vv tag add', giving a subcommand-specific usage text.");
		testlib.testResult(sg.exec("vv", "tag", "add").exit_status!=0);
		testlib.testResult(sg.exec("vv", "tag", "add").stderr.indexOf("Usage: vv tag add NEWTAG")>0);

		testlib.log("Add new tag to parent.");
		testlib.testResult(sg.exec("vv", "tag", "add", "tag1").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag1")>0);

		testlib.log("Add new tag to rev.");
		testlib.testResult(sg.exec("vv", "tag", "add", "tag2", "-r1").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag2")<0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("tag2")>0);

		testlib.log("Add existing tag to parent. (Error.)");
		testlib.testResult(sg.exec("vv", "tag", "add", "tag2").exit_status!=0);
		testlib.testResult(sg.exec("vv", "tag", "add", "tag2").stderr.indexOf("tag already exists")>=0);
		testlib.testResult(sg.exec("vv", "tag", "add", "tag2").stderr.indexOf("vv tag move")>=0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag2")<0);

		testlib.log("Add existing tag to rev. (Error.)");
		testlib.testResult(sg.exec("vv", "tag", "add", "tag1", "-r1").exit_status!=0);
		testlib.testResult(sg.exec("vv", "log", "-r1").stdout.indexOf("tag1")<0);

		testlib.log("Add new tag to parent in different repo. (Error.)");
		testlib.testResult(sg.exec("vv", "tag", "add", "tag3", repo2).exit_status!=0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag3")<0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("tag3")<0);

		testlib.log("Add new tag to in different repo.");
		testlib.testResult(sg.exec("vv", "tag", "add", "tag3", "-r2", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("tag3")<0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("tag3")>0);

		testlib.log("Move existing tag to parent.");
		testlib.testResult(sg.exec("vv", "tag", "move", "tag2").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag2")>0);
		testlib.testResult(sg.exec("vv", "log", "-r1").stdout.indexOf("tag2")<0);

		testlib.log("Move existing tag to rev.");
		testlib.testResult(sg.exec("vv", "tag", "move", "tag1", "-r1").exit_status==0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag1")<0);
		testlib.testResult(sg.exec("vv", "log", "-r1").stdout.indexOf("tag1")>0);

		testlib.log("Move non-existing tag to parent.");
		testlib.testResult(sg.exec("vv", "tag", "move", "tag3").exit_status!=0);
		testlib.testResult(sg.exec("vv", "parents").stdout.indexOf("tag3")<0);

		testlib.log("Move non-existing tag to rev.");
		testlib.testResult(sg.exec("vv", "tag", "move", "tag3", "-r1").exit_status!=0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("tag3")<0);

		testlib.log("Move tag in different repo.");
		testlib.testResult(sg.exec("vv", "tag", "move", "tag3", "-r1", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "log", "-r1", repo2).stdout.indexOf("tag3")>0);
		testlib.testResult(sg.exec("vv", "log", "-r2", repo2).stdout.indexOf("tag3")<0);

		testlib.log("List tags.");
		testlib.testResult(sg.exec("vv", "tag", "list").exit_status==0);
		testlib.testResult(sg.exec("vv", "tag", "list").stdout.indexOf("tag1")>=0);
		testlib.testResult(sg.exec("vv", "tag", "list").stdout.indexOf("tag2")>=0);
		testlib.testResult(sg.exec("vv", "tag", "list").stdout.indexOf("tag3")<0);
		testlib.testResult(sg.exec("vv", "tags").exit_status==0);
		testlib.testResult(sg.exec("vv", "tags").stdout==sg.exec("vv", "tag", "list").stdout);

		testlib.log("List tags in different repo.");
		testlib.testResult(sg.exec("vv", "tag", "list", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "tag", "list", repo2).stdout.indexOf("tag1")<0);
		testlib.testResult(sg.exec("vv", "tag", "list", repo2).stdout.indexOf("tag2")<0);
		testlib.testResult(sg.exec("vv", "tag", "list", repo2).stdout.indexOf("tag3")>=0);

		testlib.log("Remove tag using a rev. (Error.)");
		testlib.testResult(sg.exec("vv", "tag", "remove", "tag1", "-r1").exit_status!=0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("tag1")>0);

		testlib.log("Remove tag.");
		testlib.testResult(sg.exec("vv", "tag", "remove", "tag1").exit_status==0);
		testlib.testResult(sg.exec("vv", "log").stdout.indexOf("tag1")<0);

		testlib.log("Remove tag in different repo.");
		testlib.testResult(sg.exec("vv", "tag", "remove", "tag3", repo2).exit_status==0);
		testlib.testResult(sg.exec("vv", "log", repo2).stdout.indexOf("tag3")<0);

		testlib.log("No subcommand. (Error.)");
		testlib.testResult(sg.exec("vv", "tag").exit_status!=0);

		testlib.log("Bogus subcommand. (Error.)");
		testlib.testResult(sg.exec("vv", "tag", "oops").exit_status!=0);
	}
}

function st_clc_user()
{
	this.testUserCommand = function()
	{
		testlib.log("List users.");
		testlib.testResult(sg.exec("vv", "user", "list").exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(repInfo.userName)>=0);
		testlib.testResult(sg.exec("vv", "users").exit_status==0);
		testlib.testResult(sg.exec("vv", "users").stdout==sg.exec("vv", "user", "list").stdout);

		testlib.log("Create new user.");
		var newUser = "testUserCommand";
		testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(newUser)<0);
		testlib.testResult(sg.exec("vv", "user", "create", newUser).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(newUser)>=0);

		testlib.log("Rename user.");
		var newUserRenamed = "testRenamedUser";
		testlib.testResult(sg.exec("vv", "user", "rename", newUser, newUserRenamed).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(newUser)<0);
		testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(newUserRenamed)>=0);

		testlib.log("Rename user using their userid.");
		var userid = false;
		var repo = sg.open_repo(repInfo.repoName);
		if(repo) try {
			userid = new zingdb(repo, sg.dagnum.USERS).query('user', ['*'], "name == '" + newUserRenamed + "'")[0].recid;
		}
		finally {
			repo.close();
		}
		if(testlib.testResult(userid)) {
			testlib.log("(userid is "+userid+")");
			testlib.testResult(sg.exec("vv", "user", "rename", userid, newUser).exit_status==0);
			testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(newUserRenamed)<0);
			testlib.testResult(sg.exec("vv", "user", "list").stdout.indexOf(newUser)>=0);
		}

		testlib.log("List users in different repo.");
		var r2 = repInfo.repoName+"-2--testUserCommand";
		testlib.testResult(sg.exec("vv", "repo", "new", r2).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).stdout.indexOf(repInfo.userName)<0);
		testlib.testResult(sg.exec("vv", "whoami", "--create", repInfo.userName, "--repo="+r2));
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).stdout.indexOf(repInfo.userName)>=0);

		testlib.log("Create new user in different repo");
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).stdout.indexOf(newUser)<0);
		testlib.testResult(sg.exec("vv", "user", "create", newUser, "--repo="+r2).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).stdout.indexOf(newUser)>=0);

		testlib.log("Rename user in different repo.");
		testlib.testResult(sg.exec("vv", "user", "rename", newUser, newUserRenamed, "--repo="+r2).exit_status==0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).stdout.indexOf(newUser)<0);
		testlib.testResult(sg.exec("vv", "user", "list", "--repo="+r2).stdout.indexOf(newUserRenamed)>=0);

		// no subcommand tested by whoami test, since it does the same thing

		testlib.log("Bogus subcommand. (Error.)");
		testlib.testResult(sg.exec("vv", "user", "oops").exit_status!=0);
	}
}

function st_clc_ambiguous_branch()
{
	/**
	 * Test that you see a list of the current heads when trying to 'vv update' or 'vv checkout' to an ambiguous branch head.
	 */
	this.testFriendlyAmbiguousBranchErrors = function() {
		testlib.log("Test usage errors. You should not be able to use --attach without specifying a revision.");

	    var xxx = sg.exec(vv, "branch");
	    print("Branch is: [" + xxx.stdout + "]");
	    var xxx = sg.exec(vv, "parents");
	    print("Parents are: [" + xxx.stdout + "]");

	    // In WC this is not an error.
		testlib.testResult(sg.exec("vv", "update", "--attach=master").exit_status==0);

	    print("After UPDATE:");
	    var xxx = sg.exec(vv, "branch");
	    print("Branch is: [" + xxx.stdout + "]");
	    var xxx = sg.exec(vv, "parents");
	    print("Parents are: [" + xxx.stdout + "]");


		sg.fs.cd("..");
		testlib.testResult(sg.exec("vv", "checkout", repInfo.repoName, repInfo.repoName+"-wf2", "--attach=master").exit_status!=0);

		testlib.log("Moving on...");
		sg.fs.cd(repInfo.workingPath);

		sg.exec("vv", "update", "-r1", "--attach=master");

		sg.file.write("testFriendlyAmbiguousBranchErrors-A.txt", "testFriendlyAmbiguousBranchErrors-A");
		vscript_test_wc__add("testFriendlyAmbiguousBranchErrors-A.txt");
		var csid1 = vscript_test_wc__commit();

	    // In WC we need --attach to get to go back to a non-head
		sg.exec("vv", "update", "-r1", "--attach=master");

		sg.file.write("testFriendlyAmbiguousBranchErrors-B.txt", "testFriendlyAmbiguousBranchErrors-B");
		vscript_test_wc__add("testFriendlyAmbiguousBranchErrors-B.txt");
		var csid2 = vscript_test_wc__commit();

	    // In WC we need --attach to get to go back to a non-head
		sg.exec("vv", "update", "-r1", "--attach=master");

		var o;

	    // In WC we don't show the CSIDs.   And I think the error code changed.
		testlib.log("Verify 'vv update' when attached complains.");
		o=sg.exec("vv", "update");
		testlib.testResult(o.exit_status!=0);
		testlib.testResult(  (o.stderr.indexOf("The branch needs to be merged") > 0)
				     || (o.stderr.indexOf("There are multiple heads from this changeset") > 0)  );

		testlib.log("Verify 'vv update --branch=...' complains.");
		testlib.testResult(sg.exec("vv", "branch", "detach").exit_status==0);
		o=sg.exec("vv", "update", "--branch=master");
		testlib.testResult(o.exit_status!=0);
		testlib.testResult(  (o.stderr.indexOf("The branch needs to be merged") > 0)
				     || (o.stderr.indexOf("There are multiple heads from this changeset") > 0)  );

		sg.fs.cd("..");

		testlib.log("Verify 'vv checkout' shows you both csids");
		o=sg.exec("vv", "checkout", repInfo.repoName, repInfo.repoName+"-testFriendlyAmbiguousBranchErrors");
		testlib.testResult(o.exit_status!=0);
		testlib.testResult(o.stderr.indexOf("There are too many heads") > 0);
		testlib.testResult(o.stderr.indexOf(csid1)>0);
		testlib.testResult(o.stderr.indexOf(csid2)>0);

		testlib.log("Verify 'vv checkout --branch=...' shows you both csids");
		o=sg.exec("vv", "checkout", repInfo.repoName, repInfo.repoName+"-testFriendlyAmbiguousBranchErrors", "--branch=master");
		testlib.testResult(o.exit_status!=0);
		testlib.testResult(o.stderr.indexOf("There are too many heads") > 0);
		testlib.testResult(o.stderr.indexOf(csid1)>0);
		testlib.testResult(o.stderr.indexOf(csid2)>0);
	}
}


function st_clc_active_user()
{
	this.testCommitInactive = function()
	{
		var repo = null;
		var uname = sg.gid();
		var uid = null;
		var ztx = null;

		try
		{
			repo = sg.open_repo(repInfo.repoName);
			var udb = new zingdb(repo, sg.dagnum.USERS);
			ztx = udb.begin_tx();

			var rec = ztx.new_record('user');
			rec.name = uname;
			uid = rec.recid;
			var ct = ztx.commit();
			if (ct.errors)
				throw("unable to create user");
			ztx = null;

			sg.fs.cd(repInfo.workingPath);
			if (! testlib.equal(0, sg.exec("vv", "whoami", uname).exit_status, "whoami status"))
				return;

			ztx = udb.begin_tx();

			rec = ztx.open_record('user', uid);
			rec.inactive = true;
			ct = ztx.commit();
			if (ct.errors)
				throw("unable to edit user");
			ztx = null;

			sg.file.write("testCommitInactive.txt", "testCommitInactive");
			testlib.testResult(sg.exec("vv", "add", "testCommitInactive.txt").exit_status==0, "commit");

			var o = sg.exec("vv", "commit", "--message=hi");

			if (! testlib.notEqual(0, o.exit_status, "commit should fail"))
				return;

			var matches = o.stderr.match(/inactive/i);

			testlib.ok(!! matches, "output should mention inactive");
		}
		finally
		{
			if (ztx)
				ztx.abort();

			if (repo)
				repo.close();
		}

	};

	this.testWhoamiInactive = function()
	{
		var repo = null;
		var uname = sg.gid();
		var uid = null;
		var ztx = null;

		try
		{
			repo = sg.open_repo(repInfo.repoName);
			var udb = new zingdb(repo, sg.dagnum.USERS);
			ztx = udb.begin_tx();

			var rec = ztx.new_record('user');
			rec.name = uname;
			rec.inactive = true;
			uid = rec.recid;
			var ct = ztx.commit();
			if (ct.errors)
				throw("unable to create user");
			ztx = null;

			sg.fs.cd(repInfo.workingPath);
			var o = sg.exec("vv", "whoami", uname);
			if (! testlib.notEqual(0, o.exit_status, "whoami should fail"))
				return;

			var matches = o.stderr.match(/inactive/i);

			testlib.ok(!! matches, "output should mention inactive");
		}
		finally
		{
			if (ztx)
				ztx.abort();

			if (repo)
				repo.close();
		}

	};

	this.testSetActiveInactiveLocal = function()
	{
		var pullUser = function(username) {
			var repo = null;

			try
			{
				repo = sg.open_repo(repInfo.repoName);
				var db = new zingdb(repo, sg.dagnum.USERS);
				var recs = db.query('user', ['*'], "name == '" + username + "'");

				if (! recs)
					return(null);

				if (recs.length == 0)
					return(null);

				return(recs[0]);
			}
			finally
			{
				if (repo)
					repo.close();
			}
		};

		sg.fs.cd(repInfo.workingPath);
		var o = sg.exec('vv', 'user', 'set-inactive', repInfo.userName);
		if (! testlib.ok(o.exit_status == 0, 'set-inactive'))
		{
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			return;
		}

		var u = pullUser(repInfo.userName);

		if (! testlib.ok(!! u.inactive, 'user should be inactive'))
			return;

		o = sg.exec('vv', 'user', 'set-active', repInfo.userName);
		if (! testlib.ok(o.exit_status == 0, 'set-active'))
		{
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			return;
		}

		u = pullUser(repInfo.userName);

		if (! testlib.ok(! u.inactive, 'user should be active'))
			return;
	};

	this.testSetActiveInactiveExplicit = function()
	{
		var pullUser = function(username) {
			var repo = null;

			try
			{
				repo = sg.open_repo(repInfo.repoName);
				var db = new zingdb(repo, sg.dagnum.USERS);
				var recs = db.query('user', ['*'], "name == '" + username + "'");

				if (! recs)
					return(null);

				if (recs.length == 0)
					return(null);

				return(recs[0]);
			}
			finally
			{
				if (repo)
					repo.close();
			}
		};

		sg.fs.cd(sg.fs.tmpdir());
		var o = sg.exec('vv', 'user', 'set-inactive', repInfo.userName, '--repo', repInfo.repoName);
		if (! testlib.ok(o.exit_status == 0, 'set-inactive'))
		{
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			return;
		}

		var u = pullUser(repInfo.userName);

		if (! testlib.ok(!! u.inactive, 'user should be inactive'))
			return;

		o = sg.exec('vv', 'user', 'set-active', repInfo.userName, '--repo', repInfo.repoName);
		if (! testlib.ok(o.exit_status == 0, 'set-active'))
		{
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			return;
		}

		u = pullUser(repInfo.userName);

		if (! testlib.ok(! u.inactive, 'user should be active'))
			return;
	};
}
