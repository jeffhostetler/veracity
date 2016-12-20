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

function st_RevertTests2() {

    this.testRevertMoveFileToRenamedFile = function testRevertMoveFileToRenamedFile() {
        // rename folder1/sharedname.txt to uniquename.txt
        // move folder2/sharedname.txt to folder1
        // revert
        var base = 'testRevertMoveFileToRenamedFile';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var file1 = createFileOnDisk(pathCombine(folder1, "sharedname.txt"), 9);
        var file2 = createFileOnDisk(pathCombine(folder2, "sharedname.txt"), 20)

        addRemoveAndCommit();

        testlib.inRepo(folder1);
        testlib.inRepo(folder2);
        testlib.inRepo(file1);
        testlib.inRepo(file2);
        vscript_test_wc__rename(file1, "uniquename.txt");
        vscript_test_wc__move(file2, folder1);
	vscript_test_wc__revert_all();
        vscript_test_wc__statusEmptyExcludingIgnores();

        testlib.inRepo(file1);
        testlib.inRepo(file2);
        testlib.existsOnDisk(file1);
	// TODO verify file1 has 9 lines
        testlib.existsOnDisk(file2);
	// TODO verify file2 has 20 lines
    }

    this.testRevertCreateFileWithRenamedFile = function testRevertCreateFileWithRenamedFile() 
    {
	// Test W6339 and W9411 on how we deal with ADDED/FOUND
	// items that are in the way when we do a revert-all.
	//
        // rename folder1/sharedname.txt to uniquename.txt
        // create new file: folder1/sharedname.txt
        // revert

        var base = 'testRevertCreateFileWithRenamedFile';
	var base_rp = "@/" + base;

        createFolderOnDisk( base + "/folder1" );
        createFileOnDisk(   base + "/folder1/sharedname.txt", 3);
	var hid3 = sg.vv2.hid( { "path" : base + "/folder1/sharedname.txt" } );
        addRemoveAndCommit();

        vscript_test_wc__rename( base + "/folder1/sharedname.txt", "uniquename.txt");

	var expect_test = new Array;
	expect_test["Renamed"]  = [ base_rp + "/folder1/uniquename.txt" ];
        vscript_test_wc__confirm(expect_test);

        createFileOnDisk(   base + "/folder1/sharedname.txt", 15);
	var hid15 = sg.vv2.hid( { "path" : base + "/folder1/sharedname.txt" } );
        vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Renamed"]  = [ base_rp + "/folder1/uniquename.txt" ];
	expect_test["Added"]    = [ base_rp + "/folder1/sharedname.txt" ];
        vscript_test_wc__confirm(expect_test);

	// With W6339 REVERT-ALL will throw when there is an uncontrolled
	// item in the way of a full revert.  (Technically, the new
	// "sharedname" directory currently appears as ADDED, but REVERT
	// will UN-ADD it before trying to move "uniquename" back, so it
	// behaves as if it were currently just FOUND.)

	vscript_test_wc__revert_all__expect_error( base,
						   "An item will interfere" );

	// nothing should have changed on disk since before the revert-all.
        vscript_test_wc__confirm(expect_test);

	// TODO 2012/08/01 See W6339 and W9411.  We want some type of --force
	// TODO            with or without backup that will either move the
	// TODO            uncontrolled item out of the way or delete it.
	// TODO
	// TODO            Also use hid3 and hid15 to verify content of each 
	// TODO            version of the file.
	// TODO
        // TODO vscript_test_wc__statusEmptyExcludingIgnores();
    }

    this.testRevertRenameFileAsRenamedFile = function testRevertRenameFileAsRenamedFile() {
        // rename folder1/sharedname.txt to uniquename.txt
        // rename folder1/oldname.txt to sharedname.txt
        // revert
        var base = 'testRevertRenameFileAsRenamedFile';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var fileA = createFileOnDisk(pathCombine(folder1, "sharedname.txt"), 3);
        var fileB = createFileOnDisk(pathCombine(folder1, "oldname.txt"), 8);

        addRemoveAndCommit();

        vscript_test_wc__rename(fileA, "uniquename.txt");
        var fileC = pathCombine(folder1, "uniquename.txt");
        testlib.existsOnDisk(fileC);
        testlib.notOnDisk(fileA);

        vscript_test_wc__rename(fileB, "sharedname.txt");
        var newfileB = pathCombine(folder1, "sharedname.txt");
        testlib.existsOnDisk(newfileB);
        testlib.notOnDisk(fileB);

	vscript_test_wc__revert_all();

        testlib.notOnDisk(fileC);
        testlib.existsOnDisk(fileB);
        testlib.existsOnDisk(fileA);

        vscript_test_wc__statusEmptyExcludingIgnores();
    }

    this.testRevertCreateFolderWithRenamedFolder = function testRevertCreateFolderWithRenamedFolder() 
    {
        var base    = 'testRevertCreateFolderWithRenamedFolder';
	var base_rp = "@/" + base;

	// create <base>/sharedname
	// create <base>/sharedname/a.txt

        createFolderOnDisk( base + "/sharedname" );
	createFileOnDisk(   base + "/sharedname/a.txt" );
        addRemoveAndCommit();

	// rename <base>/sharedname <base>/uniquename
	// ---> giving <base>/uniquename
	// ---> giving <base>/uniquename/a.txt
        vscript_test_wc__rename( base + "/sharedname", "uniquename");
        testlib.existsOnDisk( base + "/uniquename" );
	testlib.existsOnDisk( base + "/uniquename/a.txt" );
        testlib.notOnDisk(    base + "/sharedname" );

	var expect_test = new Array;
	expect_test["Renamed"] = [ base_rp + "/uniquename" ];
        vscript_test_wc__confirm(expect_test);

	// create (new) <base>/sharedname
	// create (new) <base>/sharedname/b.txt
        createFolderOnDisk( base + "/sharedname" );
	createFileOnDisk(   base + "/sharedname/b.txt" );
        vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Renamed"] = [ base_rp + "/uniquename" ];
	expect_test["Added"]   = [ base_rp + "/sharedname",
				   base_rp + "/sharedname/b.txt" ];
        vscript_test_wc__confirm(expect_test);

	// With W6339 REVERT-ALL will throw when there is an uncontrolled
	// item in the way of a full revert.  (Technically, the new
	// "sharedname" directory currently appears as ADDED, but REVERT
	// will UN-ADD it before trying to move "uniquename" back, so it
	// behaves as if it were currently just FOUND.)

	vscript_test_wc__revert_all__expect_error( base,
						   "An item will interfere" );

	// nothing should have changed on disk since before the revert-all.
        vscript_test_wc__confirm(expect_test);

	// TODO 2012/08/01 See W6339 and W9411.  We want some type of --force
	// TODO            with or without backup that will either move the
	// TODO            uncontrolled item out of the way or delete it.
	// TODO
	// TODO vscript_test_wc__statusEmptyExcludingIgnores();
    }

    this.testRevertMoveFileThenDelete = function testRevertMoveFileThenDelete() {
        // Move a file to another folder. Delete the file.  Try to revert.
        // Do this using both deleteOnDisk and pending_tree.remove
        var base = 'testRevertMoveFileThenDelete';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        vscript_test_wc__move(fileA, folder2);
        testlib.existsOnDisk(pathCombine(folder2, "fileA.txt"));
        testlib.notOnDisk(fileA);
        deleteFileOnDisk(pathCombine(folder2, "fileA.txt"));  // comment this line and revert will succeed

	vscript_test_wc__revert_all();

        vscript_test_wc__statusEmptyExcludingIgnores();
    }

    this.testRevertMoveFileThenRemove = function testRevertMoveFileThenRemove() {
        // Move a file to another folder. Delete the file.  Try to revert.
        // Do this using both deleteOnDisk and pending_tree.remove
        var base = getTestName(arguments.callee.toString());
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        vscript_test_wc__move(fileA, folder2);
        testlib.existsOnDisk(pathCombine(folder2, "fileA.txt"));
        testlib.notOnDisk(fileA);
        vscript_test_wc__remove(pathCombine(folder2, "fileA.txt"));
        testlib.notOnDisk(pathCombine(folder2, "fileA.txt"));

	var expect_test = new Array;
	expect_test["Moved"]   = [ "@b/" + fileA ];
	expect_test["Removed"] = [ "@b/" + fileA ];
        vscript_test_wc__confirm(expect_test);

        vscript_test_wc__revert_all();
    }

    this.testRevertComplicatedMoves = function testRevertComplicatedMoves() {
	// mkdir base/parentFolder/moveMe/subFolder/
	// mkdir base/other/
	// commit
	//
	// mv base/parentFolder/moveMe/ base/other/  
	// so we should have:
	//     base/parentFolder/
	//     base/other/moveMe/
	//     base/other/moveMe/subFolder/
	//
	// mv base/parentFolder/ base/other/moveMe/subFolder/
	// so we should have:
	//     base/other/moveMe/
	//     base/other/moveMe/subFolder/
	//     base/other/moveMe/subFolder/parentFolder/
	//
	// revert
        var base = 'testRevertComplicatedMoves';
        var folder1 = createFolderOnDisk(pathCombine(base, "parentFolder/moveMe/subFolder"));
        var folder2 = createFolderOnDisk(pathCombine(base, "other"));
        addRemoveAndCommit();

        testlib.inRepo(folder1);
        testlib.inRepo(folder2);

        indentLine('-> vscript_test_wc__move(getParentPath(folder1), folder2);');
        vscript_test_wc__move(getParentPath(folder1), folder2);
        var move2 = pathCombine(base, "other/moveMe/subFolder");
        indentLine('-> vscript_test_wc__move(pathCombine(base, "parentFolder"), move2);');
        vscript_test_wc__move(pathCombine(base, "parentFolder"), move2);
	vscript_test_wc__revert_all();
        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(folder1);
        testlib.existsOnDisk(folder2);

    }

    this.testRevertFolderDelete = function testRevertFolderDelete() {

        var base = 'testRevertFolderDelete';
        var folder1 = createFolderOnDisk(base + "1");
        var folder2 = createFolderOnDisk(pathCombine(folder1, base + 2));
        var file = createFileOnDisk(pathCombine(folder2, "file1.txt"));

        addRemoveAndCommit();
        vscript_test_wc__remove(folder2);
	vscript_test_wc__revert_all();

        testlib.existsOnDisk(folder2);
	testlib.existsOnDisk(file);
        testlib.inRepo(folder2);
    }

    this.testSimpleFileMoveDownRevert = function testSimpleFileMoveDownRevert() {

        var base = 'testSimpleFileMoveDownRevert';
        folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        nestedFile = createFileOnDisk(pathCombine(folder1, "file1"), 10);
        rootFile = pathCombine(base, "file1");

        addRemoveAndCommit();

        testlib.inRepo(nestedFile);

        indentLine("*> " + formatFunctionString("vscript_test_wc__move", [nestedFile, base]));
        vscript_test_wc__move(nestedFile, base);
        indentLine("*> " + formatFunctionString("vscript_test_wc__remove", [folder1]));
        vscript_test_wc__remove(folder1);

        testlib.existsOnDisk(rootFile);
        testlib.notOnDisk(folder1);

	vscript_test_wc__revert_all("about to revert ...");

        indentLine("!! testing for " + nestedFile);
        testlib.notOnDisk(rootFile);
        testlib.existsOnDisk(nestedFile);
    }

    this.testDeepFileMoveDownRevert = function testDeepFileMoveDownRevert() {
        // this is the same as testSimpleFileMoveDownRevert, but with more depth

        var base = 'testDeepFileMoveDownRevert';
        var depth = 20;
        var rootFolder = [];
        var deepFolder = [];
        var tip = base;
        var i;

        for (i=1; i<=depth; i++) {
            rootFolder[i] = pathCombine(base, "f" + i);
            deepFolder[i] = pathCombine(tip, "f" + i);
            createFolderOnDisk(deepFolder[i]);
            createFileOnDisk(pathCombine(deepFolder[i], "file" + i), i);
            tip = deepFolder[i];
        }

        addRemoveAndCommit();

        testlib.inRepo(pathCombine(tip, "file" + depth));

        for (i=depth; i>=1; i--) {
            indentLine("*> " + formatFunctionString("vscript_test_wc__move", [pathCombine(deepFolder[i], "file" + i), base]));
            vscript_test_wc__move(pathCombine(deepFolder[i], "file" + i), base);
            indentLine("*> " + formatFunctionString("vscript_test_wc__remove", [deepFolder[i]]));
            vscript_test_wc__remove(deepFolder[i]);
        }

        for (i=1; i<=depth; i++) {
            testlib.existsOnDisk(pathCombine(base,"file" + i));
            testlib.notOnDisk(deepFolder[i]);
        }

	vscript_test_wc__revert_all("about to revert ...");

        for (i=1; i<=depth; i++) {
            indentLine("!! testing for " + deepFolder[i] + "/file" + i);
            testlib.notOnDisk(pathCombine(base,"file" + i));
            testlib.existsOnDisk(pathCombine(deepFolder[i], "file" + i));
        }
    }

}
