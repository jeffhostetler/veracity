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

function st_RevertTests() {

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.testEditFileRevert = function testEditFileRevert() {

        var base = 'testEditFileRevert';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        addRemoveAndCommit();
        testlib.inRepo(folder);
        testlib.inRepo(file);

        var tmpFile = createTempFile(file);

        insertInFile(file, 20);

        var tmpFile2 = createTempFile(file);

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );
	print("Projected backup file is: " + backup);

	vscript_test_wc__revert_all( base );

        testlib.ok(compareFiles(tmpFile, file), "Files should be identical after the revert");
        testlib.existsOnDisk(backup);
        testlib.ok(compareFiles(tmpFile2, backup), "Backup file should match edited file");
        sg.fs.remove(tmpFile);
        sg.fs.remove(tmpFile2);
        sg.fs.remove(backup);
        vscript_test_wc__statusEmptyExcludingIgnores();

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.testDeleteFileRevert = function testDeleteFileRevert() {
        var base = 'testDeleteFileRevert';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 29);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(file);

        deleteFileOnDisk(file);
	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(file);
    }

    this.testDeleteFolderRevert = function testDeleteFolderRevert() {

        var base = 'testDeleteFolderRevert';
        var folder = createFolderOnDisk(base, 3);
        var folder2 = createFolderOnDisk(pathCombine(folder, base + "2"));

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);

        deleteFolderOnDisk(folder2);
	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(folder2);
    }

    this.testMoveRevert = function testRevertMove() {

        var base = 'testRevertMove';
        var folder = createFolderOnDisk(base, 3);
        var folder2 = createFolderOnDisk(pathCombine(folder, base + 4));
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 25);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);
        testlib.inRepo(file);

        vscript_test_wc__move(file, folder2);
        testlib.existsOnDisk(pathCombine(folder2, "file1.txt"));

	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder2, "file1.txt"));
    }

    this.testFileRenameRevert = function testFileRenameRevert() {

        var base = 'testFileRenameRevert';
        var folder = createFolderOnDisk(base, 3);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 25);

        addRemoveAndCommit();

        testlib.inRepo(folder);

        testlib.inRepo(file);

        vscript_test_wc__rename(file, "rename_file1.txt");
        testlib.existsOnDisk(pathCombine(folder, "rename_file1.txt"));

	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder, "rename_file1.txt"));
    }

    this.testFolderRenameRevert = function testFolderRenameRevert() {

        var base = 'testFolderRenameRevert';
        var folder = createFolderOnDisk(base, 2);

        addRemoveAndCommit();

        testlib.inRepo(folder);

        var newName = pathCombine(getParentPath(folder), "rename_" + base);


        vscript_test_wc__rename(folder, "rename_" + base);
        testlib.existsOnDisk(newName);

	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(folder);
        testlib.notOnDisk(newName);
    }

    this.testMoveBackToOriginal = function testMoveBackToOriginal() {
        /*As per combo test instructions -
           
        vscript_test_wc__move("folder_1/file_1.txt", "folder_2/"); 
        vscript_test_wc__move("folder_2/file_1.txt", "folder_1/"); 
        */
        var base = 'testMoveBackToOriginal';
        var folder1 = createFolderOnDisk(base + "1");
        var folder2 = createFolderOnDisk(base + "2");
        var file = createFileOnDisk(pathCombine(folder1, "file1.txt"));

        addRemoveAndCommit();

        testlib.inRepo(folder1);
        testlib.inRepo(folder2);
        testlib.inRepo(file);

        vscript_test_wc__move(file, folder2);
        vscript_test_wc__move(pathCombine(folder2, "file1.txt"), folder1);

	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder2, "file1.txt"));

        insertInFile(file, 3);
        vscript_test_wc__move(file, folder2);
        vscript_test_wc__move(pathCombine(folder2, "file1.txt"), folder1);
        commit();
        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder2, "file1.txt"));

    }

    this.testMoveAndDeleteFileRevert = function testMoveAndDeleteFileRevert() {
        //same as above - delete file with move & revert
        var base = 'testMoveAndDeleteFileRevert';
	var base_rp = "@/" + base;
	var base_b_rp = "@b/" + base;

        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var file = createFileOnDisk(pathCombine(folder1, "file.txt"), 10);
        addRemoveAndCommit();
        vscript_test_wc__remove(file);
        testlib.notOnDisk(file);
        vscript_test_wc__move(folder1, folder2);
        testlib.notOnDisk(folder1);
        testlib.existsOnDisk(pathCombine(folder2, "folder1"));

	var expect_test = new Array;
	expect_test["Removed"]  = [ base_b_rp + "/folder1/file.txt" ];
	expect_test["Moved"]    = [ base_rp   + "/folder2/folder1" ];
        vscript_test_wc__confirm(expect_test);

	vscript_test_wc__revert_items( base_b_rp + "/folder1/file.txt" );

	var expect_test = new Array;
	expect_test["Moved"]    = [ base_rp   + "/folder2/folder1" ];
        vscript_test_wc__confirm(expect_test);

        testlib.existsOnDisk(pathCombine(folder2, "folder1/file.txt"));
        testlib.notOnDisk(folder1);
        vscript_test_wc__commit(); // flush pending stuff

    }

    this.testMoveAndDeleteFolderRevert = function testMoveAndDeleteFolderRevert() {
        //same as above - delete folder with move & revert
        var base = 'testMoveAndDeleteFolderRevert';
	var base_rp = "@/" + base;
	var base_b_rp = "@b/" + base;

        var folder1 = createFolderOnDisk(pathCombine(base, "folder1"));
        var folder2 = createFolderOnDisk(pathCombine(base, "folder2"));
        var subfolder = createFolderOnDisk(pathCombine(folder1, "subfolder"));
        addRemoveAndCommit();
        vscript_test_wc__remove(subfolder);
        testlib.notOnDisk(subfolder);
        vscript_test_wc__move(folder1, folder2);
        testlib.notOnDisk(folder1);
        testlib.existsOnDisk(pathCombine(folder2, "folder1"));

	var expect_test = new Array;
	expect_test["Removed"]  = [ base_b_rp + "/folder1/subfolder" ];
	expect_test["Moved"]    = [ base_rp   + "/folder2/folder1" ];
        vscript_test_wc__confirm(expect_test);

	vscript_test_wc__revert_items( base_b_rp + "/folder1/subfolder" );

	var expect_test = new Array;
	expect_test["Moved"]    = [ base_rp   + "/folder2/folder1" ];
        vscript_test_wc__confirm(expect_test);

        testlib.existsOnDisk(pathCombine(folder2, "folder1/subfolder"));
        testlib.notOnDisk(folder1);
        vscript_test_wc__commit(); // flush pending stuff

    }

    this.testEditMoveRevert = function testEditMoveRevert() {
        //edit a file and move the same file then revert.
        var base = 'testEditMoveRevert';
        var folder = createFolderOnDisk(base + "1");
        var folder2 = createFolderOnDisk(base + "2", 3);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 120);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(folder2);
        testlib.inRepo(file);

        var filev1 = createTempFile(file);

        doRandomEditToFile(file);
	testlib.ok((compareFiles(file,filev1) == false), "doRandomEditToFile modified file.");

        var filev2 = createTempFile(file);

        vscript_test_wc__move(file, folder2);
        testlib.notOnDisk(file);
        testlib.existsOnDisk(pathCombine(folder2, "file1.txt"));

	vscript_test_wc__revert_all( base );

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );

        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder2, "file1.txt"));
        testlib.ok(compareFiles(filev1, file), "Files should be identical after the revert");
        testlib.existsOnDisk(backup);
        testlib.ok(compareFiles(filev2, backup), "Backup file should match edited file");
        sg.fs.remove(filev1);
        sg.fs.remove(filev2);
        sg.fs.remove(backup);
        vscript_test_wc__statusEmptyExcludingIgnores();

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.testEditRenameRevert = function testEditRenameRevert() {
        //edit a file and move the same file then revert.
        var base = 'testEditRenameRevert';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 120);

        addRemoveAndCommit();

        testlib.inRepo(folder);
        testlib.inRepo(file);

        var filev1 = createTempFile(file);

        doRandomEditToFile(file);
	testlib.ok((compareFiles(file,filev1) == false), "doRandomEditToFile modified file.");

        var filev2 = createTempFile(file);

        vscript_test_wc__rename(file, "renamed_file1.txt");

        testlib.existsOnDisk(pathCombine(folder, "renamed_file1.txt"));
        testlib.notOnDisk(file);

	vscript_test_wc__revert_all( base );

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );

        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder, "renamed_file1.txt"));
        testlib.ok(compareFiles(filev1, file), "Files should be identical after the revert");
        testlib.existsOnDisk(backup);
        testlib.ok(compareFiles(filev2, backup), "Backup file should match edited file");
        sg.fs.remove(filev1);
        sg.fs.remove(filev2);
        sg.fs.remove(backup);
        vscript_test_wc__statusEmptyExcludingIgnores();

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.testRenameFileLikeParentRevert = function testRenameFileLikeParentRevert() {
        // Verify that you can rename a file with the same name as the folder where the file resides.
        // ???? not sure if we want to revert to or from the above condition ... doing both

        // revert from:
        var base = 'testRenameFileLikeParentRevert';
        var folder1 = createFolderOnDisk(pathCombine(base + "_from", "folder1"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        var fpath = getParentPath(fileA);
        var fname = getFileNameFromPath(fpath);
        var newFileA = pathCombine(fpath, fname);
        vscript_test_wc__rename(fileA, fname);
        testlib.existsOnDisk(newFileA);
        testlib.notOnDisk(fileA);

	vscript_test_wc__revert_all( base );

        testlib.inRepo(fileA);
        testlib.notInRepo(newFileA);
        testlib.existsOnDisk(fileA);
        testlib.notOnDisk(newFileA);

        // revert to:
        folder1 = createFolderOnDisk(pathCombine(base + "_to", "commonName"));
        fileA = createFileOnDisk(pathCombine(folder1, "commonName"), 3);

        addRemoveAndCommit();

        fpath = getParentPath(fileA);
        fname = "uncommonName";
        newFileA = pathCombine(fpath, fname);
        vscript_test_wc__rename(fileA, fname);
        testlib.existsOnDisk(newFileA);
        testlib.notOnDisk(fileA);

	vscript_test_wc__revert_all( base );

        testlib.inRepo(fileA);
        testlib.notInRepo(newFileA);
        testlib.existsOnDisk(fileA);
        testlib.notOnDisk(newFileA);
    }

    this.testRenameFolderLikeChildRevert = function testRenameFolderLikeChildRevert() {
        // Verify that you can rename a folder with the same name as a file that exists in that folder. 
        // ???? not sure if we want to revert to or from the above condition ... doing both

        // revert from:
        var base = 'testRenameFolderLikeChildRevert';
        var folder1 = createFolderOnDisk(pathCombine(base + "_from", "folder1"));
        var fileA = createFileOnDisk(pathCombine(folder1, "fileA.txt"), 3);

        addRemoveAndCommit();

        var fpath = getParentPath(folder1);
        var fname = getFileNameFromPath(fileA);
        var newFolder1 = pathCombine(fpath, fname);
        vscript_test_wc__rename(folder1, fname);
        testlib.existsOnDisk(newFolder1);
        testlib.notOnDisk(folder1);

	vscript_test_wc__revert_all( base );

        testlib.inRepo(folder1);
        testlib.notInRepo(newFolder1);
        testlib.existsOnDisk(folder1);
        testlib.notOnDisk(newFolder1);

        // revert to:
        folder1 = createFolderOnDisk(pathCombine(base + "_to", "commonName"));
        fileA = createFileOnDisk(pathCombine(folder1, "commonName"), 3);

        addRemoveAndCommit();

        fpath = getParentPath(folder1);
        fname = "uncommonName";
        newFolder1 = pathCombine(fpath, fname);
        vscript_test_wc__rename(folder1, fname);
        testlib.existsOnDisk(newFolder1);
        testlib.notOnDisk(folder1);

	vscript_test_wc__revert_all( base );

        testlib.inRepo(folder1);
        testlib.notInRepo(newFolder1);
        testlib.existsOnDisk(folder1);
        testlib.notOnDisk(newFolder1);
    }

    this.testDeleteEditRevert = function testDeleteEditRevert() {
        // delete a file and edit the same file then revert 
	//
	// TODO 2011/04/18 The above comment is a little misleading.
	// TODO            What we do here is pend the DELETE of a file
	// TODO            and then pend an ADD a new file with the same 
	// TODO            name and MODIFY it (unnecessary because it is
	// TODO            new).  And then REVERT.
	//
	// The revert finds a name collision -- the revert of the ADDED 
	// file should just leave it in the directory (with FOUND status)
	// and the revert of the DELETED file should re-create it using
	// the contents stored in the repo.
	//
	// Because of the name collision -- move the found one to a temporary
	// name and let the restored version have the file name so that the
	// WD is in sync.
	//
	// Because this type of temp name is not a backup of the original
	// file, I've changed the ~suffix~.

        var i = 0;
        var base = 'testDeleteEditRevert';
	var base_rp = "@/" + base;
	var base_b_rp = "@/b" + base;

        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file.txt"), 10);
        addRemoveAndCommit();

        var filev1 = createTempFile(file);
        vscript_test_wc__remove(file);
        testlib.notOnDisk(file);

	var expect_test = new Array;
	expect_test["Found"]   = [ "@/" + filev1 ];
	expect_test["Removed"] = [ "@b/" + folder + "/file.txt" ];
        vscript_test_wc__confirm(expect_test);

        copyFileOnDisk(filev1, file);
        testlib.existsOnDisk(file);
        doRandomEditToFile(file);
	testlib.ok((compareFiles(file,filev1) == false), "doRandomEditToFile modified file.");
        var filev2 = createTempFile(file);
        reportTreeObjectStatus(file);
        vscript_test_wc__addremove();
        reportTreeObjectStatus(file);

	var expect_test = new Array;
	expect_test["Added"]   = [ "@/" + filev1,
				   "@/" + filev2,
				   "@/" + file    ];
	expect_test["Removed"] = [ "@b/" + folder + "/file.txt" ];
        vscript_test_wc__confirm(expect_test);

	// With W6339 REVERT-ALL will throw when there is an uncontrolled
	// item in the way of a full revert.  (Technically, the new
	// "file.txt" currently appears as ADDED, but REVERT
	// will UN-ADD it before trying to move "uniquename" back, so it
	// behaves as if it were currently just FOUND.)

	vscript_test_wc__revert_all__expect_error( base,
						   "An item will interfere" );

	// Move the new file out of the way and try again.

	vscript_test_wc__rename(file, "new_file.txt");
	vscript_test_wc__revert_all( base );

	var expect_test = new Array;
	expect_test["Found"]   = [ "@/" + filev1,
				   "@/" + filev2,
				   "@/" + folder + "/new_file.txt" ];
        vscript_test_wc__confirm(expect_test);

	var new_file = pathCombine(folder, "new_file.txt");
        testlib.existsOnDisk(file);
        testlib.ok(compareFiles(file, filev1), "Files should be identical after the revert");
        testlib.existsOnDisk(new_file);
        testlib.ok(compareFiles(new_file, filev2), "V2 file should match edited file");
        sg.fs.remove(filev1);
        sg.fs.remove(filev2);
        sg.fs.remove(new_file);
        vscript_test_wc__statusEmptyExcludingIgnores();

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.testStairMoveUpRevert = function testStairMoveUpRevert() {

        var base = 'testStairMoveUpRevert';
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

	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.notOnDisk(pathCombine(folder1, "folder2"));
        testlib.existsOnDisk(folder1);
        testlib.existsOnDisk(folder2);
        testlib.existsOnDisk(folder3);
    }

    this.testStairMoveDownRevert = function testStairMoveDownRevert() {

        var base = 'testStairMoveDownRevert';
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
        
	vscript_test_wc__revert_all( base );

        vscript_test_wc__statusEmptyExcludingIgnores();
        testlib.notOnDisk(pathCombine(base, "folder2"));
        testlib.notOnDisk(pathCombine(base, "folder3"));
        testlib.existsOnDisk(folder3);
    }

    this.testDeepStairMoveUpRevert = function testDeepStairMoveUpRevert() {

        var base = 'testDeepStairMoveUpRevert';
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

	vscript_test_wc__revert_all( base );

        for (i=1; i<=depth; i++) {
            testlib.existsOnDisk(pathCombine(rootFolder[i],"file" + i));
        }
        testlib.notOnDisk(pathCombine(rootFolder[1], "f2"));
    }

    this.testDeepStairMoveDownRevert = function testDeepStairMoveDownRevert() {

        var base = 'testDeepStairMoveDownRevert';
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

	vscript_test_wc__revert_all( base );

        testlib.existsOnDisk(pathCombine(tip, "file" + depth));
        for (i=2; i<=depth; i++) {
            testlib.notOnDisk(rootFolder[i]);
        }
    }

}
