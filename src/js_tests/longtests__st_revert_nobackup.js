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

function st_revert_nobackup() {

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

	vscript_test_wc__revert_all__np( { "no_backups" : true }, base );

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );

        testlib.ok(compareFiles(tmpFile, file), "Files should be identical after the revert");
        testlib.notOnDisk(backup);
        sg.fs.remove(tmpFile);
        vscript_test_wc__statusEmptyExcludingIgnores();

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

        vscript_test_wc__move(file, folder2);
        testlib.notOnDisk(file);
        testlib.existsOnDisk(pathCombine(folder2, "file1.txt"));

	vscript_test_wc__revert_all__np( { "no_backups" : true }, base );

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );

        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder2, "file1.txt"));
        testlib.ok(compareFiles(filev1, file), "Files should be identical after the revert");
        testlib.notOnDisk(backup);
        sg.fs.remove(filev1);
        vscript_test_wc__statusEmptyExcludingIgnores();
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

        vscript_test_wc__rename(file, "renamed_file1.txt");

        testlib.existsOnDisk(pathCombine(folder, "renamed_file1.txt"));
        testlib.notOnDisk(file);

	vscript_test_wc__revert_all__np( { "no_backups" : true }, base );

        var backup = vscript_test_wc__backup_name_of_file( file, "sg00" );

        testlib.existsOnDisk(file);
        testlib.notOnDisk(pathCombine(folder, "renamed_file1.txt"));
        testlib.ok(compareFiles(filev1, file), "Files should be identical after the revert");
        testlib.notOnDisk(backup);
        sg.fs.remove(filev1);
        vscript_test_wc__statusEmptyExcludingIgnores();
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
	// The revert finds a name collision.  See W8268, W6339.
	// We now throw and ask them to move the added/found file 
	// out of the way.

        var i = 0;
        var base = 'testDeleteEditRevert';
        var folder = createFolderOnDisk(base);
        var file = createFileOnDisk(pathCombine(folder, "file.txt"), 10);
        addRemoveAndCommit();

        var filev1 = createTempFile(file);
        vscript_test_wc__remove(file);
        testlib.notOnDisk(file);

        copyFileOnDisk(filev1, file);
        testlib.existsOnDisk(file);
        doRandomEditToFile(file);
	testlib.ok((compareFiles(file,filev1) == false), "doRandomEditToFile modified file.");
        var filev2 = createTempFile(file);
        reportTreeObjectStatus(file);
        vscript_test_wc__addremove();
        reportTreeObjectStatus(file);

	vscript_test_wc__revert_all__expect_error( base,
						   "An item will interfere" );

    }

}
