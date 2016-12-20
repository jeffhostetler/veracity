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

function st_SmokeTests() {

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.testAddFolderToRoot = function testAddFolderToRoot() {

        var newFolder = 'testAddFolderToRoot';
        addFolder('testAddFolderToRoot');

    }

    this.testAddFileToRoot = function testAddFileToRoot() {
        var file = 'testAddFileToRoot' + ".txt";
        createFileOnDisk(file, 100);

        vscript_test_wc__add(file);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        //  print(file);
        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(file);

    }

    this.testAddRemoveWithAdd = function testAddRemoveWithAdd() {
        var folder = 'testAddRemoveWithAdd';
        var folder2 = pathCombine(folder, "newFolder1");
        var file = folder + ".txt";
        var file2 = pathCombine(folder2, "file1.txt");
        var file3 = pathCombine(folder, file);

        createFolderOnDisk(folder);
        createFileOnDisk(file, 200);
        createFolderOnDisk(folder2);
        createFileOnDisk(file3, 50);
        createFileOnDisk(file2, 150);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);
        testlib.inRepo(file);
        testlib.inRepo(file2);
        testlib.inRepo(file3);

    }

    this.testAddRemoveWithDeleteFile = function testAddRemoveWithDeleteFile() {
        var baseName = 'testAddRemoveWithDeleteFile';
        var file1 = baseName + "1.txt";

        var folder = baseName;
        createFolderOnDisk(folder);
        var file2 = pathCombine(folder, baseName + "2.txt");
        createFileOnDisk(file1, 100);
        createFileOnDisk(file2, 20);

        addRemoveAndCommit();
        //  print(file);
        testlib.inRepo(folder);

        testlib.inRepo(file1);
        testlib.inRepo(file2);

        deleteFileOnDisk(file1);
        deleteFileOnDisk(file2);

        addRemoveAndCommit();

        testlib.notOnDisk(file1);
        testlib.notOnDisk(file2);

        testlib.notInRepo(file1);
        testlib.notInRepo(file2);


    }

    this.testAddRemoveWithDeleteFolder = function testAddRemoveWithDeleteFolder() {
        var baseName = 'testAddRemoveWithDeleteFolder';
        var folder1 = baseName + "1";
        var folder2 = pathCombine(folder1, baseName + "2");
        createFolderOnDisk(folder2);

        addRemoveAndCommit()

        testlib.inRepo(folder1);
        testlib.inRepo(folder2);

        deleteFolderOnDisk(folder1);

        addRemoveAndCommit();

        testlib.notOnDisk(folder1);
        testlib.notOnDisk(folder2);

        testlib.notInRepo(folder1);
        testlib.notInRepo(folder2);

    }



    this.testRemoveFile = function testRemoveFile() {

        var baseName = 'testRemoveFile';
        var file1 = baseName + "1.txt";

        var folder = baseName;
        createFolderOnDisk(folder);
        var file2 = pathCombine(folder, baseName + "2.txt");
        createFileOnDisk(file1, 110);
        createFileOnDisk(file2, 10);

        addRemoveAndCommit();

        //  print(file);
        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(file1);
        testlib.inRepo(folder);
        testlib.inRepo(file2);

        vscript_test_wc__remove(file1);
        vscript_test_wc__remove(file2);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.notInRepo(file1);
        testlib.notInRepo(file2);

        testlib.notOnDisk(file1);
        testlib.notOnDisk(file2);


    }

    this.testRemoveFolder = function testRemoveFolder() {
        var baseName = 'testRemoveFolder';
        var folder1 = baseName + "1";

        var folder2 = pathCombine(folder1, baseName + "2");
        createFolderOnDisk(folder2);

        addRemoveAndCommit()

        //  print(file);
        testlib.inRepo(folder1);
        testlib.inRepo(folder2);

        vscript_test_wc__remove(folder1);

	// we used to be able to a commit when there were LOST items.
	// i fixed that for W7399 to throw if the LOST item was to
	// be commited.  (which broke this test.)  Bug SPRAWL-836 and 
	// SPRAWL-863 are concerned with having the stuff within the
	// deleted directory to also be marked deleted rather than
	// appearing as lost.

	var expect_test = new Array;
	expect_test["Removed"] = [ "@b/"+folder1,
				   "@b/"+folder2 ];
        vscript_test_wc__confirm_wii(expect_test);

        vscript_test_wc__commit();

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.notOnDisk(folder1);
        testlib.notOnDisk(folder2);

        testlib.notInRepo(folder1);
        testlib.notInRepo(folder2);


    }

    this.testRemoveFolderAndParent = function testRemoveFolderAndParent() {

        var folder1 = 'testRemoveFolderAndParent';
        var folder2_0 = createFolderOnDisk(folder1, 1);
        var folder2_1 = createFolderOnDisk(folder1, 2);
        var folder2_2 = createFolderOnDisk(folder1, 3);
        var folder2_3 = createFolderOnDisk(folder1, 4);
        var folder2_4 = createFolderOnDisk(folder1, 5);

        addRemoveAndCommit()

        //  print(file);
        testlib.inRepo(folder1);
        testlib.inRepo(folder2_4);

        vscript_test_wc__remove(folder2_4);
        vscript_test_wc__remove(folder1);

	// we used to be able to a commit when there were LOST items.
	// i fixed that for W7399 to throw if the LOST item was to
	// be commited.  (which broke this test.)  Bug SPRAWL-836 and 
	// SPRAWL-863 are concerned with having the stuff within the
	// deleted directory to also be marked deleted rather than
	// appearing as lost.

	var expect_test = new Array;
	expect_test["Removed"] = [ "@b/"+folder1,
				   "@b/"+folder2_0,
				   "@b/"+folder2_1,
				   "@b/"+folder2_2,
				   "@b/"+folder2_3,
				   "@b/"+folder2_4 ];
        vscript_test_wc__confirm_wii(expect_test);

        vscript_test_wc__commit();

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.notOnDisk(folder1);
        testlib.notOnDisk(folder2_4);

        testlib.notInRepo(folder1);
        testlib.notInRepo(folder2_4);

    }

    this.testRemoveFileAndParent = function testRemoveFileAndParent() {

        var base = 'testRemoveFileAndParent';
        var folder2 = createFolderOnDisk(base, 2);
        //print(folder2);
        var file = pathCombine(folder2, base + ".txt");
        //  print(file);

        createFileOnDisk(file, 10);

        addRemoveAndCommit()

        //  print(file);
        testlib.inRepo(base);
        testlib.inRepo(folder2);
        testlib.inRepo(file);

        vscript_test_wc__remove(file);
        vscript_test_wc__remove(folder2);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.notOnDisk(file);
        testlib.notOnDisk(folder2);

        testlib.notInRepo(file);
        testlib.notInRepo(folder2);

    }


    this.testRenameFile = function testRenameFile() {

        var base = 'testRenameFile';

        var file = pathCombine(base, base + ".txt");
        createFolderOnDisk(base);
        createFileOnDisk(file, 10);

        addRemoveAndCommit()

        testlib.inRepo(base);
        testlib.inRepo(file);


        vscript_test_wc__rename(file, "newFileName.txt");
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();

        testlib.notOnDisk(file);
        testlib.existsOnDisk(pathCombine(getParentPath(file), "newFileName.txt"));
        testlib.inRepo(pathCombine(getParentPath(file), "newFileName.txt"));
    }

    this.testRenameFolder = function testRenameFolder() {

        var base = 'testRenameFolder';

        var folder = createFolderOnDisk(base, 2);

        addRemoveAndCommit()

        testlib.inRepo(folder);
        vscript_test_wc__rename(folder, "renamedFolder");
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();

        testlib.notOnDisk(folder);
        testlib.existsOnDisk(pathCombine(getParentPath(folder), "renamedFolder"));
        testlib.inRepo(pathCombine(getParentPath(folder), "renamedFolder"));

    }

    this.testMoveFile = function testMoveFile() {
        var newFolder1base = 'testMoveFile' + "1";
        var newFolder2base = 'testMoveFile' + "2";
        var newFolder1 = createFolderOnDisk(pathCombine(newFolder1base, "name"));
        var newFolder2 = createFolderOnDisk(pathCombine(newFolder2base, "name"), 2);
        var file = createFileOnDisk(pathCombine(newFolder1, "file.txt"), 11);

        addRemoveAndCommit();

        testlib.inRepo(newFolder1);
        testlib.inRepo(newFolder2);
        testlib.inRepo(file);

        vscript_test_wc__move(file, newFolder2);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        vscript_test_wc__statusEmptyExcludingIgnores();

        testlib.notInRepo(pathCombine(newFolder1, "file.txt"));
        testlib.inRepo(pathCombine(newFolder2, "file.txt"));
        testlib.existsOnDisk(pathCombine(newFolder2, "file.txt"));
        testlib.notOnDisk(file);

    }

    this.testMoveFolder = function testMoveFolder() {
        var newFolder1base = 'testMoveFolder' + "1";
        var newFolder2base = 'testMoveFolder' + "2";
        var newFolder1 = createFolderOnDisk(newFolder1base);
        var newFolder2 = createFolderOnDisk(newFolder2base);

        addRemoveAndCommit();

        testlib.inRepo(newFolder1);
        testlib.inRepo(newFolder2);

        vscript_test_wc__move(newFolder1, newFolder2);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(pathCombine(newFolder2, newFolder1base));
        testlib.existsOnDisk(pathCombine(newFolder2, newFolder1base));
        testlib.notOnDisk(newFolder1);

    }
    this.testMoveMultipleJSGlue = function testMoveMultiple() {
        var base = 'testMoveMultipleJSGlue';
        var origin = createFolderOnDisk(pathCombine(base, "origin"));
        var target = createFolderOnDisk(pathCombine(base, "target"));
        var file1 = createFileOnDisk(pathCombine(origin, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(origin, "file2.txt"), 39);
        var file3 = createFileOnDisk("file3.txt", 39);
        addRemoveAndCommit();

        vscript_test_wc__move_n( [ file1, file2, file3] , target);
        testlib.existsOnDisk(pathCombine(target, "file1.txt"));
        testlib.existsOnDisk(pathCombine(target, "file2.txt"));
        testlib.existsOnDisk(pathCombine(target, "file3.txt"));

	vscript_test_wc__revert_all();
	testlib.existsOnDisk(file1);
	testlib.existsOnDisk(file2);
	testlib.existsOnDisk(file3);
        try
	{
            vscript_test_wc__move_n( [ file1, "notThere", file2, file3], target);
	    testlib.ok( (0), "Expected throw on move of non-existent file.");
	}
	catch (e)
	{
	    testlib.ok( (1), "Expected throw on move of non-existent file.");
	}
	// NOTE 2012/07/24 The PendingTree version of this routine expected that
	// NOTE            the above move would move 3 of 4 and return an error
	// NOTE            for the other 1.  The WC version pre-checks them all
	// NOTE            before doing anything.  So we expect them to be where
	// NOTE            they were.
	testlib.existsOnDisk(file1);
	testlib.existsOnDisk(file2);
	testlib.existsOnDisk(file3);
        }


    this.testEditFile = function testEditFile() {

        var folder = createFolderOnDisk('testEditFile');
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 120);

        addRemoveAndCommit();
        testlib.inRepo(folder);
        testlib.inRepo(file);

        appendToFile(file, 12);
        commit();

        insertInFile(file, 2);
        commit();

        replaceInFile(file, 10);
        commit();

        deleteInFile(file);
        commit();
    }


    this.testEditFileMove = function testEditFileMove() {
        var base = 'testEditFileMove';
        var folder = createFolderOnDisk(base);
        var folder2 = createFolderOnDisk(base + "1", 3);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 120);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);
        testlib.inRepo(file);

        doRandomEditToFile(file);

        vscript_test_wc__move(file, folder2);
        commit();

        testlib.inRepo(pathCombine(folder2, "file1.txt"));
        testlib.notInRepo(file);
        testlib.existsOnDisk(pathCombine(folder2, "file1.txt"));
        testlib.notOnDisk(file);
    }

    this.testEditFileRename = function testEditFileRename() {
        var base = 'testEditFileRename';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 120);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(file);

        doRandomEditToFile(file);

        vscript_test_wc__rename(file, "renamed_file1.txt");
        commit();

        testlib.inRepo(pathCombine(folder, "renamed_file1.txt"));
        testlib.notInRepo(file);
        testlib.existsOnDisk(pathCombine(folder, "renamed_file1.txt"));
        testlib.notOnDisk(file);
    }

    this.testAddFolderRecursive = function testAddFolderRecursive() {

        var f = createFolderRecursive_Fixed('testAddFolderRecursive', 10, 10);
        var baseFolder = f[0];
        var fsObjs = f[1];
        vscript_test_wc__add(fsObjs[0]);  
        commit();
       
        testlib.allInRepo(fsObjs);
    }

    this.testAddFolderNonRecursive = function testAddFolderNonRecursive() {

	print("testAddFolderNonRecursive: beginning");
	var s = sg.exec(vv, "status"); print(s.stdout);

        var folderBase = createFolderOnDisk('testAddFolderNonRecursive' + "_base");
        var folder = createFolderWithSubfoldersAndFiles(pathCombine(folderBase, folderBase + "_sub"), 5, 5);

        var file = createFileOnDisk(pathCombine(folder[0], "file1.txt"));

	var expect_test = new Array;
	expect_test["Found"] = [];
	expect_test["Found"].push( "@/" + folderBase );
	expect_test["Found"].push( "@/" + folder[0] );
	for (var k in folder[1])
	    expect_test["Found"].push( "@/" + folder[1][k] );
	expect_test["Found"].push( "@/" + file );
        vscript_test_wc__confirm_wii(expect_test);

        vscript_test_wc__add_np( { "src" : folderBase,
				   "depth" : 0 } );

	var expect_test = new Array;
	expect_test["Added"] = [];
	expect_test["Added"].push( "@/" + folderBase );
	expect_test["Found"] = [];
	expect_test["Found"].push( "@/" + folder[0] );
	for (var k in folder[1])
	    expect_test["Found"].push( "@/" + folder[1][k] );
	expect_test["Found"].push( "@/" + file );
        vscript_test_wc__confirm_wii(expect_test);

        vscript_test_wc__commit();

	var expect_test = new Array;
	expect_test["Found"] = [];
	expect_test["Found"].push( "@/" + folder[0] );
	for (var k in folder[1])
	    expect_test["Found"].push( "@/" + folder[1][k] );
	expect_test["Found"].push( "@/" + file );
        vscript_test_wc__confirm_wii(expect_test);


        testlib.inRepo(folderBase);
        testlib.notInRepo(file);
        testlib.notInRepo(folder[0]);
    }

    this.testAddRemoveNonRecursive = function testAddRemoveNonRecursive() {

        var folderBase = createFolderOnDisk('testAddRemoveNonRecursive' + "_base");
        var folder = createFolderWithSubfoldersAndFiles(pathCombine(folderBase, "sub"), 5, 5);
        var file = createFileOnDisk(pathCombine(folderBase, "file.txt"));
        var subfile = createFileOnDisk(pathCombine(folder[0], "subfile.txt"));

        vscript_test_wc__addremove_np({"src":folderBase, "depth":0 });
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        testlib.inRepo(folderBase);

        testlib.notInRepo(subfile);
	// changed the following to agree with the current behavior, which agrees with the behavior of nonrecursive add
        testlib.notInRepo(file);
        testlib.notInRepo(folder[0]);

    }
  

    this.testAddRenameMove = function testAddRenameMove() {

        var newFolder1base = 'testAddRenameMove' + "1";
        var newFolder2base = 'testAddRenameMove' + "2";
        var newFolder1 = pathCombine(newFolder1base, "name");
        var newFolder2 = pathCombine(newFolder2base, "name");
        createFolderOnDisk(newFolder1);
        createFolderOnDisk(newFolder2);

        vscript_test_wc__addremove();
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(newFolder1);
        testlib.inRepo(newFolder2);

        vscript_test_wc__rename(newFolder1, "newName");
        vscript_test_wc__move(pathCombine(newFolder1base, "newName"), newFolder2base);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(pathCombine(newFolder2base, "newName"));

    }

    this.testAddMoveRename = function testAddMoveRename() {

        var newFolder1base = 'testAddMoveRename' + "1";
        var newFolder2base = 'testAddMoveRename' + "2";
        var newFolder1 = pathCombine(newFolder1base, "name1");
        var newFolder2 = pathCombine(newFolder2base, "name2");
        createFolderOnDisk(newFolder1);
        createFolderOnDisk(newFolder2);

        addRemoveAndCommit();

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(newFolder1);
        testlib.inRepo(newFolder2);

        vscript_test_wc__move(newFolder1, newFolder2base);
        vscript_test_wc__rename(pathCombine(newFolder2base, "name1"), "newName");
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.inRepo(pathCombine(newFolder2base, "newName"));

    }

    this.testMoveAndDeleteCommit = function testMoveAndDeleteCommit() {
        // Delete a file in a folder. Verify that you can now move this folder to another folder. 
        // View the History of the folder the file was moved to and undelete the file. Verify that the file 
        // now appears in the file list for the folder it was moved to and not at the old location.

        // Delete a subfolder of a folder. Verify that you can now move this folder to another folder. 
        // View the History of the folder the folder was moved to and undelete the subfolder Verify that the 
        // subfolder now appears in the project tree in the folder it was moved to and not at the old location.
    }

    this.testMoveErrorsFilesCollide = function testMoveErrorsFilesCollide() {
        // Select a file and then select Move. Attempt to move the file into a folder which already 
        // has a file with that name. Verify an appropriate error is given.
        var base = 'testMoveErrorsFilesCollide';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var fileAtRoot = createFileOnDisk(pathCombine(base, "file.txt"), 10);
        var fileInFolder = createFileOnDisk(pathCombine(folder1, "file.txt"), 10);
        addRemoveAndCommit();

        var cmd = formatFunctionString("vscript_test_wc__move", [fileAtRoot, folder1]);
        testlib.verifyFailure(cmd, "Move file to folder with samename file should fail");
        testlib.existsOnDisk(fileAtRoot);

        if (!vscript_test_wc__verifyCleanStatus()) {
            vscript_test_wc__commit(); // flush pending stuff
	    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        }
    }

    this.testMoveErrorsFoldersCollide = function testMoveErrorsFoldersCollide() {
        // Select a folder and then select Move. Attempt to move the folder into a folder which 
        // already has a folder with that name. Verify an appropriate error is given. 
        var base = 'testMoveErrorsFoldersCollide';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folderAtRoot = createFolderOnDisk(pathCombine(base, "foldername"));
        var folderInFolder = createFolderOnDisk(pathCombine(folder1, "foldername"));
        addRemoveAndCommit();

        var cmd = formatFunctionString("vscript_test_wc__move", [folderAtRoot, folder1]);
        testlib.verifyFailure(cmd, "Move folder to folder with samename folder should fail");
        testlib.existsOnDisk(folderAtRoot);

        if (!vscript_test_wc__verifyCleanStatus()) {
            vscript_test_wc__commit(); // flush pending stuff
	    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        }
    }

    this.testMoveErrorsFolderToChild = function testMoveErrorsFolderToChild() {
        // Attempt to move a folder into one of its children's folders. Verify that this fails with 
        // an error message saying this cannot be done.
        var base = 'testMoveErrorsFolderToChild';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var sub11 = createFolderOnDisk(pathCombine(base, "folder1/sub1/sub1.1"));
        addRemoveAndCommit();

        var cmd = formatFunctionString("vscript_test_wc__move", [folder1, sub11]);
        testlib.verifyFailure(cmd, "Move folder into a child folder should fail");
        testlib.existsOnDisk(folder1);
        testlib.notOnDisk(pathCombine(sub11, "folder1"));

        if (!vscript_test_wc__verifyCleanStatus()) {
            vscript_test_wc__commit(); // flush pending stuff
	    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        }
    }

    this.testMoveErrorsFolderToSelf = function testMoveErrorsFolderToSelf() {
        // Attempt to move a folder into itself. Verify that this fails with an error message saying 
        // this cannot be done.
        var base = 'testMoveErrorsFolderToSelf';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        addRemoveAndCommit();

        var cmd = formatFunctionString("vscript_test_wc__move", [folder1, folder1]);
        testlib.verifyFailure(cmd, "Move folder into itself should fail");
        testlib.existsOnDisk(folder1);
        testlib.notOnDisk(pathCombine(folder1, "folder1"));

        if (!vscript_test_wc__verifyCleanStatus()) {
            vscript_test_wc__commit(); // flush pending stuff
	    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        }
    }

    this.testMoveErrorsFolderToParent = function testMoveErrorsFolderToParent() {
        // Attempt to move a folder into its parent folder. Verify that this fails with an error 
        // message saying this is not allowed.
        var base = 'testMoveErrorsFolderToParent';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));

        var cmd = formatFunctionString("vscript_test_wc__move", [folder1, base]);
        testlib.verifyFailure(cmd, "Move folder into its parent should fail");
        testlib.existsOnDisk(folder1);

        deleteFolderOnDisk(folder1);
        deleteFolderOnDisk(base);
        if (!vscript_test_wc__verifyCleanStatus()) {
            vscript_test_wc__commit(); // flush pending stuff
	    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        }
    }

    this.testRenameCharacters = function testRenameCharacters() {

        // Renaming with  / \ : * ? " < > | should fail for files and folders
        var base = 'testRenameCharacters';

        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, base + ".txt"), 10);

        addRemoveAndCommit()

        testlib.inRepo(base);
        testlib.inRepo(file);
        var bogus = "*t.txt";
        //  var bogus = pathCombine(getParentPath(file), '*t.txt');
        var cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);

        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = "/t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);

        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = "\t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);
        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = ":t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);
        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = "?t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);
        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = "<t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);
        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = ">t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);
        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);

        bogus = "|t.txt";
        cmd = formatFunctionString("vscript_test_wc__rename", [file, bogus]);
        testlib.verifyFailure(cmd, "rename with illegal chars should fail");
        testlib.existsOnDisk(file);
    }

    this.testRenameFileCollisionCommit = function testRenameFileCollisionCommit() {
        // Try to rename a file to the same name as another file or subfolder contained in the folder. 
        // Verify that you get an error 
        var base = 'testRenameFileCollisionCommit';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var subfolderA = createFolderOnDisk(pathCombine(folder1, "subfolderA"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);
        var fileB = createFileOnDisk(pathCombine(folder1, "fileB.txt"), 10);

        addRemoveAndCommit();

        deleteFileOnDisk(fileA);
        deleteFolderOnDisk(subfolderA);

        cmd = formatFunctionString("vscript_test_wc__rename", [fileB, "fileA.txt"]);
        testlib.verifyFailure(cmd, "rename collision file to file should fail");
        testlib.existsOnDisk(fileB);

        cmd = formatFunctionString("vscript_test_wc__rename", [fileB, "subfolderA"]);
        testlib.verifyFailure(cmd, "rename collision file to folder should fail");
        testlib.existsOnDisk(fileB);
    }

    this.testRenameFolderCollisionCommit = function testRenameFolderCollisionCommit() {
        // Try to rename a subfolder to the same name as another subfolder or file in the folder. 
        // Verify that you get an error
        var base = 'testRenameFolderCollisionCommit';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var subfolderA = createFolderOnDisk(pathCombine(folder1, "subfolderA"));
        var subfolderB = createFolderOnDisk(pathCombine(folder1, "subfolderB"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        deleteFileOnDisk(fileA);
        deleteFolderOnDisk(subfolderA);

        cmd = formatFunctionString("vscript_test_wc__rename", [subfolderB, "fileA.txt"]);
        testlib.verifyFailure(cmd, "rename collision folder to file should fail");
        testlib.existsOnDisk(subfolderB);

        cmd = formatFunctionString("vscript_test_wc__rename", [subfolderB, "subfolderA"]);
        testlib.verifyFailure(cmd, "rename collision folder to folder should fail");
        testlib.existsOnDisk(subfolderB);

        vscript_test_wc__revert_all();

    }

    this.testRenameFileLikeParentCommit = function testRenameFileLikeParentCommit() {
        // Verify that you can rename a file with the same name as the folder where the file resides.
        var base = 'testRenameFileLikeParentCommit';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        var fpath = getParentPath(fileA);
        var fname = getFileNameFromPath(fpath);
        var newFileA = pathCombine(fpath, fname);
        vscript_test_wc__rename(fileA, fname);
        testlib.existsOnDisk(newFileA);
        testlib.notOnDisk(fileA);

        commit();

        testlib.inRepo(newFileA);
        testlib.notInRepo(fileA);
    }

    this.testRenameFolderLikeChildCommit = function testRenameFolderLikeChildCommit() {
        // Verify that you can rename a folder with the same name as a file that exists in that folder. 
        var base = 'testRenameFolderLikeChildCommit';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        var fpath = getParentPath(folder1);
        var fname = getFileNameFromPath(fileA);
        var newFolder1 = pathCombine(fpath, fname);
        vscript_test_wc__rename(folder1, fname);
        testlib.existsOnDisk(newFolder1);
        testlib.notOnDisk(folder1);

        commit();

        testlib.inRepo(newFolder1);
        testlib.notInRepo(folder1);
    }

    this.testMoveRenameSameNameCommit = function testMoveRenameSameNameCommit() {

        // move a file to another folder containing a 
        // file of the same name. Also, rename that file in the destination folder to
        // a different name. Verify that the transaction can be completed. 
        var newFolder1base = 'testMoveRenameSameNameCommit' + "1";
        var newFolder2base = 'testMoveRenameSameNameCommit' + "2";
        var newFolder1 = createFolderOnDisk(newFolder1base);
        var newFolder2 = createFolderOnDisk(newFolder2base, 2);
        var file = createFileOnDisk(pathCombine(newFolder1, "file.txt"), 9);
        var file2 = createFileOnDisk(pathCombine(newFolder2, "file.txt"), 20)

        addRemoveAndCommit();

        tmp = createTempFile(file);
        testlib.inRepo(newFolder1);
        testlib.inRepo(newFolder2);
        testlib.inRepo(file);
        testlib.inRepo(file2);

        vscript_test_wc__rename(file2, "file_rename.txt");
        vscript_test_wc__move(file, newFolder2);
        vscript_test_wc__commit();
	//sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
        

        testlib.notInRepo(file);
        testlib.inRepo(pathCombine(newFolder2, "file_rename.txt"));
        testlib.inRepo(file2);
        testlib.existsOnDisk(file2);
        testlib.notOnDisk(file);
        testlib.ok(compareFiles(file2, tmp), "files should be identical");
        deleteFileOnDisk(tmp);
        vscript_test_wc__statusEmptyExcludingIgnores();

    }

    this.testDeleteEditCommit = function testDeleteEditCommit() {
        // delete a file and edit the same file in the same atomic transaction.
        var i = 0;
        var base = 'testDeleteEditCommit';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file.txt"), 10);
        addRemoveAndCommit();

        var filev1 = createTempFile(file);
        vscript_test_wc__remove(file);
        testlib.notOnDisk(file);

        renameOnDisk(filev1, "file.txt");
        testlib.existsOnDisk(file);
        doRandomEditToFile(file);
        reportTreeObjectStatus(file);
        vscript_test_wc__addremove();
        reportTreeObjectStatus(file);

	print(sg.exec(vv, "status", "--verbose").stdout);
        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/testDeleteEditCommit/file.txt" ];	// these are 2 different files.
	expect_test["Added"]   = [ "@/testDeleteEditCommit/file.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

        commit();
        testlib.inRepo(file);
    }

    this.testMoveRenameError = function testMoveRenameError() {
        //move a file to another folder. Also, rename 
        // a different file in that folder to the name of the file that is about to be 
        // moved in. Verify that on commit an appropriate error is given.
        var base = 'testMoveRenameError';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);
        var fileB = createFileOnDisk(pathCombine(folder2, "fileB.txt"), 9);

        addRemoveAndCommit();

        vscript_test_wc__move(fileA, folder2);

        // Actually, pending_tree.rename fails.  You never get to commit();
        cmd = formatFunctionString("vscript_test_wc__rename", [fileB, "fileA.txt"]);
        testlib.verifyFailure(cmd, "name collision on folder move should fail");
        testlib.existsOnDisk(fileB);

        // commit();
        vscript_test_wc__revert_all();

    }

    this.testStairMoveUpCommit = function testStairMoveUpCommit() {

        var base = 'testStairMoveUpCommit';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var folder3 = createFolderOnDisk(pathCombine(base, "folder3"));

        addRemoveAndCommit();

        testlib.inRepo(folder1);
        testlib.inRepo(folder2);
        testlib.inRepo(folder3);

        vscript_test_wc__move(folder3, folder2);
        vscript_test_wc__move(folder2, folder1);
        testlib.existsOnDisk(pathCombine(folder1, "folder2/folder3"));

        commit();
        testlib.notInRepo(folder2);
        testlib.inRepo(pathCombine(folder1, "folder2/folder3"));
    }

    this.testStairMoveDownCommit = function testStairMoveDownCommit() {

        var base = 'testStairMoveDownCommit';
        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(folder1, "folder2"));
        var folder3 = createFolderOnDisk(pathCombine(folder2, "folder3"));

        addRemoveAndCommit();

        testlib.inRepo(folder3);

        vscript_test_wc__move(folder2, base);
        vscript_test_wc__move(pathCombine(base, "folder2/folder3"), base);

        testlib.existsOnDisk(folder1);
        testlib.existsOnDisk(pathCombine(base, "folder2"));
        testlib.existsOnDisk(pathCombine(base, "folder3"));
        
        commit();
        testlib.inRepo(pathCombine(base, "folder2"));
        testlib.inRepo(pathCombine(base, "folder3"));
        testlib.notInRepo(folder3);
    }

    this.testDeepStairMoveUpCommit = function testDeepStairMoveUpCommit() {

        var base = 'testDeepStairMoveUpCommit';
        var depth = 20;
        var rootFolder = [];
        var deepFolder = [];
        var tip = base;
        var i;

        for (i=1; i<=depth; i++) {
            rootFolder[i] = pathCombine(base, "f" + i);
            deepFolder[i] = pathCombine(tip, "f" + i);
            createFolderOnDisk(rootFolder[i]);
            createFileOnDisk(pathCombine(rootFolder[i], "file" + i), i);
            tip = deepFolder[i];
        }

        addRemoveAndCommit();

        for (i=1; i<=depth; i++) {
            testlib.inRepo(pathCombine(rootFolder[i],"file" + i));
        }

        for (i=depth; i>=2; i--) {
            indentLine("*> " + formatFunctionString("vscript_test_wc__move", [rootFolder[i], rootFolder[i-1]]));
            vscript_test_wc__move(rootFolder[i], rootFolder[i-1]);
        }

        for (i=2; i<=depth; i++) {
            testlib.notOnDisk(rootFolder[i]);
        }
        testlib.existsOnDisk(pathCombine(tip, "file" + depth));

        commit();

        for (i=2; i<=depth; i++) {
            testlib.notInRepo(rootFolder[i]);
        }
        testlib.inRepo(pathCombine(tip, "file" + depth));
    }

    this.testDeepStairMoveDownCommit = function testDeepStairMoveDownCommit() {

        var base = 'testDeepStairMoveDownCommit';
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

        for (i=2; i<=depth; i++) {
            indentLine("*> " + formatFunctionString("vscript_test_wc__move", [pathCombine(rootFolder[i-1], "f" + i), base]));
            vscript_test_wc__move(pathCombine(rootFolder[i-1], "f" + i), base);
        }

        for (i=1; i<=depth; i++) {
            testlib.existsOnDisk(pathCombine(rootFolder[i],"file" + i));
        }

        commit();

        testlib.notOnDisk(deepFolder[2]);
        testlib.notInRepo(deepFolder[2]);

        for (i=1; i<=depth; i++) {
            testlib.existsOnDisk(pathCombine(rootFolder[i], "file" + i));
            testlib.inRepo(pathCombine(rootFolder[i], "file" + i));
        }
    }
    
    this.testReadLocalSettings = function testReadLocalSettings() {
    	var timeStamp = new Date().getTime();
	var localSettings = sg.local_settings("/testing42", "" + timeStamp);
	testlib.equal(localSettings.testing42, "" + timeStamp, "The local setting should have been remembered");
    	timeStamp = new Date().getTime();
	localSettings = sg.local_settings("/testing42", "" + timeStamp);
	testlib.equal(localSettings.testing42, timeStamp, "The local setting should have been remembered");
	//localSettings = sg.local_settings(["reset"], "testing42");
	//testlib.testResult(localSettings.testing42=="" || localSettings.testing42==origUserName, "The local setting should have been reset");
    }

}
